/*
===========================================================================
cg_enginesound.c  --  Pulse-train engine sound synthesizer for Q3Rally

See cg_enginesound.h for a full description of the approach.

Signal chain per frame:
  crankAngle += rpm_to_angle_per_sample * numSamples
    └─ for each cylinder:
         if crankAngle crosses cylinder's firing angle
           → trigger envelope (attack → decay)
    └─ impulse = sum of all active cylinder envelopes * throttle * amplitude
    └─ waveguide = impulse + feedback * delayLine[wgWrite - wgLen]
                   (models exhaust pipe resonance)
    └─ output  = lowpass( waveguide )   (models engine-block damping)
  → trap_S_RawSamples() fills the Q3 raw-stream ring buffer
===========================================================================
*/

#include "cg_local.h"
#include "cg_enginesound.h"

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------

#define SPEED_OF_SOUND_MS   343.0f          // m/s at room temperature

// ---------------------------------------------------------------------------
// global synthesizer state  (one instance for the local player only)
// ---------------------------------------------------------------------------
cgEngineSynthState_t cg_engineSynth;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
// Impulse amplitude pre-waveguide. Keep well below 1.0 so the feedback
// delay line stays stable. Final output is scaled to int16 at render time.
#define CG_ES_IMPULSE_AMP   0.8f
#define CG_ES_MIN_RPM       ((float)CP_RPM_MIN)

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

// Convert degrees to radians (not used here, but handy to have)
// #define DEG2RAD(x) ((x) * (M_PI / 180.0f))

/*
==================
CG_EngineSound_DefaultConfig

4-cylinder inline engine, roughly equivalent to a 2.0 L rally car.
Firing order 1-3-4-2, 180° apart on a 4-stroke (720° cycle).
==================
*/
void CG_EngineSound_DefaultConfig( cgEngineConfig_t *cfg ) {
    Com_Memset( cfg, 0, sizeof( *cfg ) );

    cfg->numCylinders   = 4;
    // firing order 1-3-4-2 mapped to crank angles (0°, 180°, 360°, 540°)
    cfg->firingOrder[0] =   0.0f;
    cfg->firingOrder[1] = 180.0f;
    cfg->firingOrder[2] = 360.0f;
    cfg->firingOrder[3] = 540.0f;

    cfg->impulseAttack  = 0.002f;   // 2 ms rise
    cfg->impulseDecay   = 0.012f;   // 12 ms fall
    cfg->impulsePeak    = 1.0f;

    cfg->exhaustLengthM = 1.0f;     // 1 m pipe → resonance ~172 Hz
    cfg->exhaustFeedback= 0.55f;
    cfg->exhaustDamping = 0.995f;

    cfg->bodyFilterCutoff = 800.0f; // Hz
}

// ---------------------------------------------------------------------------
// waveguide buffer management
// ---------------------------------------------------------------------------

/*
==================
CG_ES_RebuildWaveguide

Recompute delay-line length from exhaust length in metres and zero the buffer.
Called on init and whenever the config changes.
==================
*/
static void CG_ES_RebuildWaveguide( cgEngineSynthState_t *state, const cgEngineConfig_t *cfg ) {
    int len;

    len = (int)( (2.0f * cfg->exhaustLengthM / SPEED_OF_SOUND_MS) * CG_ES_SAMPLE_RATE );
    if ( len < 1 )   len = 1;
    if ( len > CG_ES_WG_MAX_LEN ) len = CG_ES_WG_MAX_LEN;

    state->wgLen      = len;
    state->wgWrite    = 0;
    state->wgFeedback = cfg->exhaustFeedback;

    Com_Memset( state->wgBuffer, 0, sizeof( state->wgBuffer ) );
}

// ---------------------------------------------------------------------------
// public API
// ---------------------------------------------------------------------------

/*
==================
CG_EngineSound_Init
==================
*/
void CG_EngineSound_Init( void ) {
    cgEngineConfig_t cfg;

    Com_Memset( &cg_engineSynth, 0, sizeof( cg_engineSynth ) );

    CG_EngineSound_DefaultConfig( &cfg );
    CG_ES_RebuildWaveguide( &cg_engineSynth, &cfg );

    cg_engineSynth.initialized = qtrue;
}

void CG_EngineSound_Shutdown( void ) {
    Com_Memset( &cg_engineSynth, 0, sizeof( cg_engineSynth ) );
}

/*
==================
CG_EngineSound_Update

Called once per rendered frame for the local player.
Generates enough PCM samples to cover the current frame duration
and submits via trap_S_RawSamples into the Q3 raw-stream ring buffer.
==================
*/
void CG_EngineSound_Update( int entityNum, int rpm, int gear, float throttle ) {
    cgEngineConfig_t    cfg;
    cgEngineSynthState_t *state;
    static short        pcmBuffer[4410 * 2];
    float               sample;
    float               impulse;
    float               wgOut;
    float               degreesPerSample;
    float               attackSamples, decaySamples;
    float               lpCoeff;
    int                 i, c;
    int                 numCyl;
    float               firingAngle;
    float               prevAngle;
    qboolean            crossed;
    int                 readIdx;
    float               delayed;
    int                 s16;
    float               fc_norm;
    int                 numSamples;
    float               gain;
    int                 debugPaintedEst;
    int                 debugRawEndEst;
    int                 debugAhead;
    qboolean            debugWouldSkip;

    state = &cg_engineSynth;

    if ( !state->initialized ) {
        CG_EngineSound_Init();
    }

    // Calculate samples for this frame based on actual frametime.
    // Add a small fixed surplus (96 samples = 2ms) so s_rawend stays ahead
    // of s_paintedtime even when frames arrive slightly late.
    {
        int msec = ( cg.frametime > 0 && cg.frametime < 100 ) ? cg.frametime : 20;
        numSamples = msec * CG_ES_SAMPLE_RATE / 1000 + 96;
        if ( numSamples > 960 ) numSamples = 960;
        if ( numSamples < 64  ) numSamples = 64;
    }

    debugWouldSkip = qfalse;
    debugPaintedEst = 0;
    debugRawEndEst = state->rawEndEst;
    debugAhead = 0;

    // Overflow guard: estimate how far s_rawend is ahead of s_paintedtime.
    // s_paintedtime ≈ cg.time * CG_ES_SAMPLE_RATE / 1000 (within a few ms).
    // Allow at most 80ms (~3840 samples) of pre-buffered audio.
    {
        int paintedEst  = (int)( (float)cg.time * ( CG_ES_SAMPLE_RATE / 1000.0f ) );
        int rawEndEst   = state->rawEndEst;
        int maxAhead    = 3840; /* 80ms @ 48kHz, well below MAX_RAW_SAMPLES/4 */

        debugPaintedEst = paintedEst;
        debugRawEndEst = rawEndEst;
        debugAhead = rawEndEst - paintedEst;

        if ( rawEndEst - paintedEst > maxAhead ) {
            /* buffer is full enough -- synthesize but don't submit */
            debugWouldSkip = qtrue;

            if ( ( cg.time / 500 ) != state->debugSkipBucket ) {
                state->debugSkipBucket = cg.time / 500;
                Com_Printf( "^3ESBUF: SKIP time=%d frame=%d samples=%d rawEndEst=%d paintedEst=%d ahead=%d maxAhead=%d\n",
                            cg.time, cg.frametime, numSamples, rawEndEst, paintedEst,
                            rawEndEst - paintedEst, maxAhead );
            }
            return;
        }
        state->rawEndEst = rawEndEst + numSamples;
        debugRawEndEst = state->rawEndEst;
        debugAhead = state->rawEndEst - paintedEst;
    }

    // debug: print every 2 seconds to show live RPM values
    if ( ( cg.time / 2000 ) != state->configHash ) {
        state->configHash = cg.time / 2000;
        Com_Printf( "^2ES: time=%d frame=%d rpm=%d gear=%d throttle=%.2f samples=%d rawEndEst=%d paintedEst=%d ahead=%d skip=%d\n",
                    cg.time, cg.frametime, rpm, gear, throttle, numSamples,
                    debugRawEndEst, debugPaintedEst, debugAhead, debugWouldSkip );
    }

    CG_EngineSound_DefaultConfig( &cfg );

    // apply caller-supplied overrides read directly via trap (no vmCvar handle needed)
    {
        char buf[32];
        trap_Cvar_VariableStringBuffer( "cg_engineSoundExhaust", buf, sizeof(buf) );
        if ( buf[0] ) {
            float val = (float)atof( buf );
            if ( val > 0.01f ) {
                cfg.exhaustLengthM = val;
                CG_ES_RebuildWaveguide( state, &cfg );
            }
        }
        trap_Cvar_VariableStringBuffer( "cg_engineSoundCylinders", buf, sizeof(buf) );
        if ( buf[0] ) {
            int val = atoi( buf );
            if ( val > 0 && val <= CG_ES_MAX_CYLINDERS ) {
                cfg.numCylinders = val;
            }
        }
        trap_Cvar_VariableStringBuffer( "cg_engineSoundGain", buf, sizeof(buf) );
        gain = buf[0] ? (float)atof( buf ) : 1.0f;
        if ( gain <= 0.0f ) gain = 1.0f;  // never silent from misconfigured cvar
    }
    numCyl = cfg.numCylinders;

    // Clamp RPM
    if ( rpm < (int)CG_ES_MIN_RPM ) rpm = (int)CG_ES_MIN_RPM;
    if ( rpm > CP_RPM_MAX )         rpm = CP_RPM_MAX;

    // Clamp throttle (derived from forwardmove: 0..1)
    if ( throttle < 0.0f ) throttle = 0.0f;
    if ( throttle > 1.0f ) throttle = 1.0f;

    // --- per-sample envelope durations ---
    attackSamples = cfg.impulseAttack  * CG_ES_SAMPLE_RATE;
    decaySamples  = cfg.impulseDecay   * CG_ES_SAMPLE_RATE;

    // degrees of crank rotation per sample
    // RPM = revolutions/min → rev/sec = rpm/60 → deg/sec = rpm/60*360
    // 4-stroke: one power stroke per 2 revolutions = 720 degrees of crank
    degreesPerSample = ( (float)rpm / 60.0f * 360.0f ) / (float)CG_ES_SAMPLE_RATE;

    // one-pole LP coefficient from cutoff frequency
    // coeff = exp( -2π * fc / fs )  approximated as 1 - (2π*fc/fs) for fc << fs
    fc_norm = cfg.bodyFilterCutoff / (float)CG_ES_SAMPLE_RATE;
    lpCoeff = 1.0f - ( 2.0f * M_PI * fc_norm );
    if ( lpCoeff < 0.0f ) lpCoeff = 0.0f;
    if ( lpCoeff > 0.9999f ) lpCoeff = 0.9999f;

    // --- synthesize numSamples samples ---
    for ( i = 0; i < numSamples; i++ ) {

        // advance crank angle
        state->crankAngle += degreesPerSample;
        if ( state->crankAngle >= 720.0f ) {
            state->crankAngle -= 720.0f;
        }

        // --- check each cylinder for a firing event ---
        impulse = 0.0f;
        for ( c = 0; c < numCyl; c++ ) {
            firingAngle = cfg.firingOrder[c];
            prevAngle   = state->crankAngle - degreesPerSample;

            // handle wrap around 720°
            if ( prevAngle < 0.0f ) prevAngle += 720.0f;

            // crossed the firing angle this sample?
            crossed = qfalse;
            if ( prevAngle <= firingAngle && state->crankAngle > firingAngle ) {
                crossed = qtrue;
            } else if ( prevAngle > state->crankAngle ) {
                // wrapped around 720° this sample
                if ( firingAngle >= prevAngle || firingAngle <= state->crankAngle ) {
                    crossed = qtrue;
                }
            }

            if ( crossed ) {
                state->cylPhase[c] = 1; // start attack
                state->cylEnv[c]   = 0.0f;
            }

            // --- advance cylinder envelope ---
            if ( state->cylPhase[c] == 1 ) {
                // attack
                state->cylEnv[c] += cfg.impulsePeak / attackSamples;
                if ( state->cylEnv[c] >= cfg.impulsePeak ) {
                    state->cylEnv[c] = cfg.impulsePeak;
                    state->cylPhase[c] = 2;
                }
            } else if ( state->cylPhase[c] == 2 ) {
                // decay
                state->cylEnv[c] -= cfg.impulsePeak / decaySamples;
                if ( state->cylEnv[c] <= 0.0f ) {
                    state->cylEnv[c]  = 0.0f;
                    state->cylPhase[c] = 0;
                }
            }

            impulse += state->cylEnv[c];
        }

        // throttle-modulate amplitude: at idle throttle=0, some baseline remains
        // so the engine still makes sound when coasting
        impulse *= ( 0.3f + 0.7f * throttle ) * CG_ES_IMPULSE_AMP * gain;

        // --- waveguide (exhaust resonance) ---
        readIdx = ( state->wgWrite - state->wgLen ) & CG_ES_WG_MASK;
        delayed = state->wgBuffer[readIdx] * cfg.exhaustDamping;
        wgOut = impulse + state->wgFeedback * delayed;
        // clamp before writing back -- prevents feedback explosion
        if      ( wgOut >  1.0f ) wgOut =  1.0f;
        else if ( wgOut < -1.0f ) wgOut = -1.0f;
        state->wgBuffer[state->wgWrite] = wgOut;
        state->wgWrite = ( state->wgWrite + 1 ) & CG_ES_WG_MASK;

        // --- one-pole body low-pass filter ---
        state->lpState = state->lpState * lpCoeff + wgOut * ( 1.0f - lpCoeff );
        sample = state->lpState;

        // --- scale to int16 range and write stereo ---
        sample *= 32760.0f;
        if ( sample >  32767.0f ) s16 =  32767;
        else if ( sample < -32768.0f ) s16 = -32768;
        else s16 = (int)sample;
        pcmBuffer[i * 2 + 0] = (short)s16;
        pcmBuffer[i * 2 + 1] = (short)s16;
    }

    // submit to Q3 raw-sample ring buffer via cgame trap
    trap_S_RawSamples(
        CG_ES_RAW_STREAM,
        numSamples,
        CG_ES_SAMPLE_RATE,
        2,
        2,
        (const byte *)pcmBuffer,
        1.0f,
        entityNum
    );
}
