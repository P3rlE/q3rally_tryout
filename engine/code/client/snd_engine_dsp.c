/*
===========================================================================
  snd_engine_dsp.c

  Minimal procedural engine DSP stub.
===========================================================================
*/

#include <math.h>

#include "snd_engine_dsp.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void S_EngineDSP_Reset( engineAudioSynthState_t *state, float sampleRate ) {
    if ( !state ) {
        return;
    }

    Com_Memset( state, 0, sizeof( *state ) );
    state->sampleRate = ( sampleRate > 0.0f ) ? sampleRate : 44100.0f;
    state->noiseSeed = 1u;
}

void S_EngineDSP_TriggerBackfire( engineAudioSynthState_t *state ) {
    if ( state ) {
        state->backfireEnvelope = 1.0f;
    }
}

void S_EngineDSP_RenderEmitter(
    engineAudioSynthState_t *synth,
    const engineAudioPreset_t *preset,
    const vehicleAudioState_t *control,
    engineAudioQualityTier_t quality,
    int sampleCount,
    float *outLeft,
    float *outRight ) {
    int i;
    float sr;
    float rpm;
    float throttle;
    float baseHz;
    float amp;

    if ( !synth || !preset || !control || !outLeft || !outRight || sampleCount <= 0 ) {
        return;
    }

    sr = ( synth->sampleRate > 0.0f ) ? synth->sampleRate : 44100.0f;

    synth->smoothedRpm += ( control->rpm - synth->smoothedRpm ) * 0.05f;
    synth->smoothedThrottle += ( control->throttle - synth->smoothedThrottle ) * 0.05f;
    synth->smoothedLoad += ( control->load - synth->smoothedLoad ) * 0.05f;

    rpm = synth->smoothedRpm;
    throttle = synth->smoothedThrottle;

    baseHz = ( rpm / 60.0f ) * 0.5f;
    amp = 0.03f + 0.08f * throttle;

    if ( quality == EA_QUALITY_FAR ) {
        amp *= 0.55f;
    }
    else if ( quality == EA_QUALITY_NEAR ) {
        amp *= 0.85f;
    }

    for ( i = 0; i < sampleCount; ++i ) {
        float tone;
        float harmonic2;
        float noise;
        float sample;

        synth->phase += ( 2.0f * (float)M_PI * baseHz ) / sr;
        if ( synth->phase > 2.0f * (float)M_PI ) {
            synth->phase -= 2.0f * (float)M_PI;
        }

        tone = sinf( synth->phase );
        harmonic2 = sinf( synth->phase * 2.0f ) * 0.35f;

        synth->noiseSeed = synth->noiseSeed * 1664525u + 1013904223u;
        noise = ( ( ( synth->noiseSeed >> 8 ) & 0xFFFFu ) / 32768.0f ) - 1.0f;
        noise *= 0.02f * throttle * preset->noiseGain;

        sample = ( tone + harmonic2 ) * amp + noise;

        if ( synth->backfireEnvelope > 0.001f ) {
            float popNoise;

            synth->noiseSeed = synth->noiseSeed * 1664525u + 1013904223u;
            popNoise = ( ( ( synth->noiseSeed >> 8 ) & 0xFFFFu ) / 32768.0f ) - 1.0f;

            sample += popNoise * synth->backfireEnvelope * preset->backfireGain * 0.1f;
            synth->backfireEnvelope *= 0.97f;
        }

        outLeft[i] += sample;
        outRight[i] += sample;
    }
}
