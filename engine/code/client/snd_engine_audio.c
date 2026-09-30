/*
===========================================================================
  snd_engine_audio.c

  Procedural engine audio emitter management and mixer entry points.
===========================================================================
*/

#include "snd_engine_audio.h"
#include "snd_engine_dsp.h"
#include "snd_engine_presets.h"
#include "snd_codec.h"

#include <math.h>

#define ENGINE_AUDIO_RECORDED_ACCEL_CLIPS 4
#define ENGINE_AUDIO_RECORDED_CLIP_COUNT ( ENGINE_AUDIO_RECORDED_ACCEL_CLIPS + 1 )

typedef struct engineAudioRecordedClip_s {
    short *monoSamples;
    int frameCount;
    int sampleRate;
    int loopStart;
    int loopEnd;
    int crossfadeFrames;
    float sourceRpm;
    qboolean loaded;
} engineAudioRecordedClip_t;

typedef struct engineAudioEmitterInternal_s {
    engineAudioEmitterPublicState_t pub;
    engineAudioSynthState_t synth;
    float recordedPhase[ENGINE_AUDIO_RECORDED_CLIP_COUNT];
    float recordedRate[ENGINE_AUDIO_RECORDED_CLIP_COUNT];
    float recordedLoad;
    float recordedAccelBlend;
    float recordedLowpass;
    float recordedGain;
    int recordedAccelLow;
    int recordedAccelHigh;
    qboolean previousBackfireEvent;

    qboolean initialized;
    int generation;
    int lastUpdateFrame;
} engineAudioEmitterInternal_t;

static engineAudioEmitterInternal_t s_engineEmitters[MAX_ENGINE_AUDIO_EMITTERS];
static engineAudioRecordedClip_t s_engineRecordedClips[ENGINE_AUDIO_RECORDED_CLIP_COUNT];
static sfxHandle_t s_engineRecordedGearShift = -1;
static sfxHandle_t s_engineRecordedBackfires[5] = { -1, -1, -1, -1, -1 };
static int s_engineRecordedBackfireIndex;
static qboolean s_engineRecordedSamplesInitialized;
static qboolean s_engineRecordedEventsInitialized;
static int s_engineAudioFrameCounter;
static int s_engineAudioNextDebugPrintTime;

static void S_DebugPrintEngineAudioState( void );

static engineAudioEmitterInternal_t *S_GetEngineEmitterForEntity( int entityNum ) {
    int i;

    for ( i = 0; i < MAX_ENGINE_AUDIO_EMITTERS; ++i ) {
        if ( s_engineEmitters[i].pub.active &&
             s_engineEmitters[i].pub.entityNum == entityNum ) {
            return &s_engineEmitters[i];
        }
    }

    return NULL;
}

static engineAudioEmitterInternal_t *S_AllocEngineEmitter( int entityNum ) {
    int i;

    for ( i = 0; i < MAX_ENGINE_AUDIO_EMITTERS; ++i ) {
        if ( !s_engineEmitters[i].pub.active ) {
            engineAudioEmitterInternal_t *em = &s_engineEmitters[i];

            Com_Memset( em, 0, sizeof( *em ) );
            em->pub.active = qtrue;
            em->pub.entityNum = entityNum;
            em->generation++;
            em->lastUpdateFrame = s_engineAudioFrameCounter;

            return em;
        }
    }

    return NULL;
}

static void S_FreeEngineRecordedClip( engineAudioRecordedClip_t *clip ) {
    if ( !clip ) {
        return;
    }

    if ( clip->monoSamples ) {
        Z_Free( clip->monoSamples );
    }
    Com_Memset( clip, 0, sizeof( *clip ) );
}

static qboolean S_LoadEngineRecordedClip(
    engineAudioRecordedClip_t *clip,
    const char *path,
    float sourceRpm ) {
    snd_info_t info;
    short *sourceSamples;
    int i;

    if ( !clip || !path || !path[0] ) {
        return qfalse;
    }

    Com_Memset( &info, 0, sizeof( info ) );
    sourceSamples = (short *)S_CodecLoad( path, &info );
    if ( !sourceSamples ) {
        Com_Printf( S_COLOR_YELLOW "EngineAudio: could not load recorded sample %s\n", path );
        return qfalse;
    }

    if ( info.width != 2 || info.channels < 1 || info.channels > 2 ||
         info.samples <= 0 || info.rate <= 0 ) {
        Com_Printf( S_COLOR_YELLOW "EngineAudio: unsupported recorded sample format %s\n", path );
        Hunk_FreeTempMemory( sourceSamples );
        return qfalse;
    }

    clip->monoSamples = (short *)Z_Malloc( info.samples * sizeof( short ) );
    clip->frameCount = info.samples;
    clip->sampleRate = info.rate;
    clip->sourceRpm = sourceRpm;

    for ( i = 0; i < info.samples; ++i ) {
        int sample;

        if ( info.channels == 2 ) {
            sample = ( (int)sourceSamples[i * 2] + (int)sourceSamples[i * 2 + 1] ) / 2;
        }
        else {
            sample = sourceSamples[i];
        }

        clip->monoSamples[i] = (short)sample;
    }

    Hunk_FreeTempMemory( sourceSamples );

    clip->loopStart = (int)( info.rate * 0.10f );
    clip->loopEnd = info.samples - (int)( info.rate * 0.10f );
    clip->crossfadeFrames = (int)( info.rate * 0.12f );

    if ( clip->loopEnd - clip->loopStart < clip->crossfadeFrames * 2 ) {
        clip->loopStart = 0;
        clip->loopEnd = info.samples;
        clip->crossfadeFrames = info.rate / 20;
    }

    if ( clip->loopEnd <= clip->loopStart || clip->crossfadeFrames <= 0 ) {
        S_FreeEngineRecordedClip( clip );
        return qfalse;
    }

    clip->loaded = qtrue;
    Com_Printf( "EngineAudio: loaded %s (%d Hz, %d frames, source RPM %.0f)\n",
        path, clip->sampleRate, clip->frameCount, clip->sourceRpm );
    return qtrue;
}

static void S_InitEngineRecordedSamples( void ) {
    static const char *clipPaths[ENGINE_AUDIO_RECORDED_CLIP_COUNT] = {
        "sound/engine_audio/prototype_golf_r/engine_idle.wav",
        "sound/engine_audio/prototype_golf_r/engine_accel_01.wav",
        "sound/engine_audio/prototype_golf_r/engine_accel_03.wav",
        "sound/engine_audio/prototype_golf_r/engine_accel_04.wav",
        "sound/engine_audio/prototype_golf_r/engine_accel_02.wav"
    };
    /* Initial pitch anchors estimated from the supplied recordings. */
    static const float clipSourceRpm[ENGINE_AUDIO_RECORDED_CLIP_COUNT] = {
        2080.0f, 2200.0f, 2300.0f, 2860.0f, 3400.0f
    };
    int i;

    if ( s_engineRecordedSamplesInitialized ) {
        return;
    }

    for ( i = 0; i < ENGINE_AUDIO_RECORDED_CLIP_COUNT; ++i ) {
        S_FreeEngineRecordedClip( &s_engineRecordedClips[i] );
        S_LoadEngineRecordedClip( &s_engineRecordedClips[i], clipPaths[i], clipSourceRpm[i] );
    }

    s_engineRecordedSamplesInitialized = qtrue;
}

/* Sound registration is deferred until the first mode-3 frame. S_Init runs
 * before S_BeginRegistration, when the base mixer has not built its sound pool. */
static void S_InitEngineRecordedEvents( void ) {
    static const char *backfirePaths[5] = {
        "sound/engine_audio/prototype_golf_r/backfire_01.wav",
        "sound/engine_audio/prototype_golf_r/backfire_02.wav",
        "sound/engine_audio/prototype_golf_r/backfire_03.wav",
        "sound/engine_audio/prototype_golf_r/backfire_04.wav",
        "sound/engine_audio/prototype_golf_r/backfire_05.wav"
    };
    int i;

    if ( s_engineRecordedEventsInitialized ) {
        return;
    }

    s_engineRecordedGearShift = S_RegisterSound(
        "sound/engine_audio/prototype_golf_r/gear_shift.wav", qfalse );
    for ( i = 0; i < 5; ++i ) {
        s_engineRecordedBackfires[i] = S_RegisterSound( backfirePaths[i], qfalse );
    }
    s_engineRecordedBackfireIndex = 0;
    s_engineRecordedEventsInitialized = qtrue;
}

static void S_ShutdownEngineRecordedAudio( void ) {
    int i;

    for ( i = 0; i < ENGINE_AUDIO_RECORDED_CLIP_COUNT; ++i ) {
        S_FreeEngineRecordedClip( &s_engineRecordedClips[i] );
    }

    s_engineRecordedGearShift = -1;
    for ( i = 0; i < 5; ++i ) {
        s_engineRecordedBackfires[i] = -1;
    }
    s_engineRecordedSamplesInitialized = qfalse;
    s_engineRecordedEventsInitialized = qfalse;
}

static void S_FreeEngineEmitter( engineAudioEmitterInternal_t *em ) {
    if ( !em ) {
        return;
    }

    Com_Memset( em, 0, sizeof( *em ) );
}

void S_EngineAudio_Init( void ) {
    S_ShutdownEngineRecordedAudio();
    Com_Memset( s_engineEmitters, 0, sizeof( s_engineEmitters ) );
    s_engineAudioFrameCounter = 0;
    s_engineAudioNextDebugPrintTime = 0;
    s_engineRecordedBackfireIndex = 0;
    S_LoadEngineAudioPresets();
}

void S_EngineAudio_Shutdown( void ) {
    S_ShutdownEngineRecordedAudio();
    Com_Memset( s_engineEmitters, 0, sizeof( s_engineEmitters ) );
    s_engineAudioFrameCounter = 0;
    s_engineAudioNextDebugPrintTime = 0;
}

void S_EngineAudio_BeginFrame( void ) {
    int i;

    ++s_engineAudioFrameCounter;

    for ( i = 0; i < MAX_ENGINE_AUDIO_EMITTERS; ++i ) {
        engineAudioEmitterInternal_t *em = &s_engineEmitters[i];

        if ( !em->pub.active ) {
            continue;
        }

        if ( em->lastUpdateFrame + 2 < s_engineAudioFrameCounter ) {
            if ( s_engineAudioDebug && s_engineAudioDebug->integer >= 3 && em->pub.preset ) {
                Com_Printf( "EngineAudio: freed stale emitter ent=%d preset=%s\n", em->pub.entityNum, em->pub.preset->name );
            }
            S_FreeEngineEmitter( em );
        }
    }
}

void S_RegisterEngineEmitter( int entityNum, int presetHandle ) {
    engineAudioEmitterInternal_t *em;
    const engineAudioPreset_t *preset;

    em = S_GetEngineEmitterForEntity( entityNum );
    if ( !em ) {
        em = S_AllocEngineEmitter( entityNum );
    }

    if ( !em ) {
        Com_Printf( S_COLOR_YELLOW "S_RegisterEngineEmitter: no free emitter slots\n" );
        return;
    }

    preset = S_GetEngineAudioPresetByHandle( presetHandle );
    if ( !preset ) {
        Com_Printf( S_COLOR_YELLOW "S_RegisterEngineEmitter: invalid preset handle %d\n", presetHandle );
        return;
    }

    em->pub.preset = preset;

    em->lastUpdateFrame = s_engineAudioFrameCounter;

    if ( !em->initialized ) {
        float sampleRate = ( dma.speed > 0 ) ? (float)dma.speed : 44100.0f;

        S_EngineDSP_Reset( &em->synth, sampleRate );
        S_EngineDSP_InitWaveguide( &em->synth, em->pub.preset, sampleRate );
        em->initialized = qtrue;
    }
}

void S_RemoveEngineEmitter( int entityNum ) {
    engineAudioEmitterInternal_t *em;

    em = S_GetEngineEmitterForEntity( entityNum );
    if ( em ) {
        S_FreeEngineEmitter( em );
    }
}

void S_UpdateEngineEmitterState(
    int entityNum,
    const vehicleAudioState_t *state,
    const vec3_t exhaustOrigin,
    const vec3_t engineBayOrigin,
    const vec3_t velocity,
    engineAudioQualityTier_t quality ) {
    engineAudioEmitterInternal_t *em;
    vec3_t eventOrigin;

    if ( !state ) {
        return;
    }

    em = S_GetEngineEmitterForEntity( entityNum );
    if ( !em ) {
        em = S_AllocEngineEmitter( entityNum );
        if ( !em ) {
            return;
        }
    }

    if ( state->recordedSampleMode ) {
        S_InitEngineRecordedSamples();
        S_InitEngineRecordedEvents();
        VectorCopy( exhaustOrigin, eventOrigin );
        if ( em->pub.control.recordedSampleMode &&
             em->pub.control.gear > 0 && state->gear > 0 &&
             em->pub.control.gear != state->gear && state->rpm > 1400.0f &&
             s_engineRecordedGearShift > 0 ) {
            S_StartSound( eventOrigin, entityNum, CHAN_AUTO, s_engineRecordedGearShift );
        }

        if ( em->pub.control.recordedSampleMode &&
             state->backfireEvent && !em->previousBackfireEvent ) {
            int backfireIndex;
            sfxHandle_t backfireSound;

            backfireIndex = ( s_engineRecordedBackfireIndex + entityNum ) % 5;
            s_engineRecordedBackfireIndex = ( s_engineRecordedBackfireIndex + 1 ) % 5;
            if ( backfireIndex < 0 ) {
                backfireIndex += 5;
            }
            backfireSound = s_engineRecordedBackfires[backfireIndex];
            if ( backfireSound > 0 ) {
                S_StartSound( eventOrigin, entityNum, CHAN_AUTO, backfireSound );
            }
        }
    }

    em->previousBackfireEvent = state->backfireEvent;
    em->lastUpdateFrame = s_engineAudioFrameCounter;
    em->pub.control = *state;
    em->pub.quality = quality;
    VectorCopy( exhaustOrigin, em->pub.exhaustOrigin );
    VectorCopy( engineBayOrigin, em->pub.engineBayOrigin );
    VectorCopy( velocity, em->pub.velocity );

    if ( state->backfireEvent ) {
        S_EngineDSP_TriggerBackfire( &em->synth );
    }
}

void S_SetEngineEmitterPreset( int entityNum, int presetHandle ) {
    engineAudioEmitterInternal_t *em;
    const engineAudioPreset_t *preset;

    em = S_GetEngineEmitterForEntity( entityNum );
    preset = S_GetEngineAudioPresetByHandle( presetHandle );

    if ( em && preset ) {
        em->lastUpdateFrame = s_engineAudioFrameCounter;
        em->pub.preset = preset;
        if ( em->initialized ) {
            S_EngineDSP_InitWaveguide(
                &em->synth,
                preset,
                dma.speed > 0 ? (float)dma.speed : 44100.0f );
        }
    }
}

void S_StopAllEngineEmitters( void ) {
    Com_Memset( s_engineEmitters, 0, sizeof( s_engineEmitters ) );
    s_engineAudioFrameCounter = 0;
    s_engineAudioNextDebugPrintTime = 0;
}

static const char *S_EngineAudioQualityName( engineAudioQualityTier_t quality ) {
    switch ( quality ) {
    case EA_QUALITY_HERO:
        return "hero";
    case EA_QUALITY_NEAR:
        return "near";
    case EA_QUALITY_FAR:
        return "far";
    default:
        return "off";
    }
}

static void S_DebugPrintEngineAudioState( void ) {
    int i;
    int activeCount;
    int heroCount;
    int nearCount;
    int farCount;
    int now;

    if ( !s_engineAudioDebug || !s_engineAudioDebug->integer ) {
        return;
    }

    now = Com_Milliseconds();
    if ( now < s_engineAudioNextDebugPrintTime ) {
        return;
    }

    s_engineAudioNextDebugPrintTime = now + 1000;
    activeCount = 0;
    heroCount = 0;
    nearCount = 0;
    farCount = 0;

    for ( i = 0; i < MAX_ENGINE_AUDIO_EMITTERS; ++i ) {
        engineAudioEmitterInternal_t *em = &s_engineEmitters[i];

        if ( !em->pub.active || em->pub.quality == EA_QUALITY_OFF || !em->pub.preset ) {
            continue;
        }

        ++activeCount;
        if ( em->pub.quality == EA_QUALITY_HERO ) {
            ++heroCount;
        }
        else if ( em->pub.quality == EA_QUALITY_NEAR ) {
            ++nearCount;
        }
        else if ( em->pub.quality == EA_QUALITY_FAR ) {
            ++farCount;
        }
    }

    Com_Printf( "EngineAudio: active=%d hero=%d near=%d far=%d gain=%.2f exh=%.2f int=%.2f mech=%.2f trans=%.2f srcExh=%.2f srcBay=%.2f evtExh=%.2f evtBay=%.2f cockpit=%d limiter=%d backfire=%d\n",
        activeCount,
        heroCount,
        nearCount,
        farCount,
        s_engineAudioGain ? s_engineAudioGain->value : 1.0f,
        s_engineAudioExhaustGainScale ? s_engineAudioExhaustGainScale->value : 1.0f,
        s_engineAudioIntakeGainScale ? s_engineAudioIntakeGainScale->value : 1.0f,
        s_engineAudioMechanicalGainScale ? s_engineAudioMechanicalGainScale->value : 1.0f,
        s_engineAudioTransmissionGainScale ? s_engineAudioTransmissionGainScale->value : 1.0f,
        s_engineAudioExhaustSourceGainScale ? s_engineAudioExhaustSourceGainScale->value : 1.0f,
        s_engineAudioEngineBaySourceGainScale ? s_engineAudioEngineBaySourceGainScale->value : 1.0f,
        s_engineAudioExhaustEventGainScale ? s_engineAudioExhaustEventGainScale->value : 1.0f,
        s_engineAudioEngineBayEventGainScale ? s_engineAudioEngineBayEventGainScale->value : 1.0f,
        s_engineAudioCockpitEnable ? s_engineAudioCockpitEnable->integer : 1,
        s_engineAudioLimiterEnable ? s_engineAudioLimiterEnable->integer : 1,
        s_engineAudioBackfireEnable ? s_engineAudioBackfireEnable->integer : 1 );

    if ( s_engineAudioDebug->integer < 2 ) {
        return;
    }

    for ( i = 0; i < MAX_ENGINE_AUDIO_EMITTERS; ++i ) {
        engineAudioEmitterInternal_t *em = &s_engineEmitters[i];

        if ( !em->pub.active || em->pub.quality == EA_QUALITY_OFF || !em->pub.preset ) {
            continue;
        }

        Com_Printf( "  ent=%d preset=%s quality=%s rpm=%.0f throttle=%.2f load=%.2f slip=%.2f turbo=%.2f\n",
            em->pub.entityNum,
            em->pub.preset->name,
            S_EngineAudioQualityName( em->pub.quality ),
            em->pub.control.rpm,
            em->pub.control.throttle,
            em->pub.control.load,
            em->pub.control.wheelSlip,
            em->pub.control.turboBoost );
    }
}

static void S_ComputeEngineEmitterSpatialGains(
    const vec3_t origin,
    engineAudioQualityTier_t quality,
    float *leftGain,
    float *rightGain ) {
    int leftVol;
    int rightVol;
    int masterVol;
    vec3_t spatialOrigin;
    if ( !leftGain || !rightGain ) {
        return;
    }

    *leftGain = 0.0f;
    *rightGain = 0.0f;

    masterVol = 220;
    if ( quality == EA_QUALITY_NEAR ) {
        masterVol = 180;
    }
    else if ( quality == EA_QUALITY_FAR ) {
        masterVol = 132;
    }

    VectorCopy( origin, spatialOrigin );
    S_SpatializeOrigin( spatialOrigin, masterVol, &leftVol, &rightVol );

    *leftGain = leftVol / 255.0f;
    *rightGain = rightVol / 255.0f;
}

static float S_ClampEngineAudioFloat( float value, float minimum, float maximum ) {
    if ( value < minimum ) {
        return minimum;
    }
    if ( value > maximum ) {
        return maximum;
    }
    return value;
}

static float S_SmoothEngineAudioStep( float value ) {
    value = S_ClampEngineAudioFloat( value, 0.0f, 1.0f );
    return value * value * ( 3.0f - 2.0f * value );
}

static float S_InterpolateEngineRecordedSample(
    const engineAudioRecordedClip_t *clip,
    float framePosition ) {
    int frame0;
    int frame1;
    float fraction;

    frame0 = (int)framePosition;
    fraction = framePosition - (float)frame0;
    frame1 = frame0 + 1;
    if ( frame0 < 0 ) {
        frame0 = 0;
    }
    if ( frame0 >= clip->frameCount ) {
        frame0 = clip->frameCount - 1;
    }
    if ( frame1 >= clip->frameCount ) {
        frame1 = clip->frameCount - 1;
    }

    return (float)clip->monoSamples[frame0] +
        ( (float)clip->monoSamples[frame1] - (float)clip->monoSamples[frame0] ) * fraction;
}

static float S_ReadEngineRecordedClip(
    const engineAudioRecordedClip_t *clip,
    float *phase,
    float rate ) {
    float framePosition;
    float sample;
    float outputRate;
    float step;
    int crossfadeStart;

    if ( !clip || !clip->loaded || !phase || clip->frameCount <= 0 ) {
        return 0.0f;
    }

    if ( *phase < clip->loopStart || *phase >= clip->loopEnd ) {
        *phase = (float)clip->loopStart;
    }

    framePosition = *phase;
    sample = S_InterpolateEngineRecordedSample( clip, framePosition );
    crossfadeStart = clip->loopEnd - clip->crossfadeFrames;
    if ( framePosition >= crossfadeStart ) {
        float blend;
        float headPosition;
        float headSample;

        blend = ( framePosition - (float)crossfadeStart ) / (float)clip->crossfadeFrames;
        headPosition = (float)clip->loopStart + ( framePosition - (float)crossfadeStart );
        headSample = S_InterpolateEngineRecordedSample( clip, headPosition );
        sample += ( headSample - sample ) * blend;
    }

    outputRate = dma.speed > 0 ? (float)dma.speed : 44100.0f;
    step = ( (float)clip->sampleRate / outputRate ) * rate;
    if ( step < 0.01f ) {
        step = 0.01f;
    }
    *phase += step;

    if ( *phase >= clip->loopEnd ) {
        float loopSpan;
        loopSpan = (float)( clip->loopEnd - clip->loopStart - clip->crossfadeFrames );
        if ( loopSpan < 1.0f ) {
            loopSpan = 1.0f;
        }
        while ( *phase >= clip->loopEnd ) {
            *phase -= loopSpan;
        }
    }

    return sample / 32768.0f;
}

static void S_GetEngineRecordedAccelBlend(
    float rpm,
    int *lowIndex,
    int *highIndex,
    float *blend ) {
    int i;
    int first;
    int previous;
    int next;

    *lowIndex = -1;
    *highIndex = -1;
    *blend = 0.0f;
    first = -1;
    previous = -1;

    for ( i = 1; i < ENGINE_AUDIO_RECORDED_CLIP_COUNT; ++i ) {
        if ( !s_engineRecordedClips[i].loaded ) {
            continue;
        }
        if ( first < 0 ) {
            first = i;
        }
        if ( rpm <= s_engineRecordedClips[i].sourceRpm ) {
            if ( previous < 0 ) {
                *lowIndex = *highIndex = i;
            }
            else {
                next = i;
                *lowIndex = previous;
                *highIndex = next;
                *blend = ( rpm - s_engineRecordedClips[previous].sourceRpm ) /
                    ( s_engineRecordedClips[next].sourceRpm - s_engineRecordedClips[previous].sourceRpm );
                *blend = S_SmoothEngineAudioStep( *blend );
            }
            return;
        }
        previous = i;
    }

    if ( previous >= 0 ) {
        *lowIndex = *highIndex = previous;
    }
    else if ( first >= 0 ) {
        *lowIndex = *highIndex = first;
    }
}

static void S_RenderRecordedEngineVoice(
    engineAudioEmitterInternal_t *em,
    portable_samplepair_t *buffer,
    int sampleCount,
    float leftGain,
    float rightGain,
    const int paintbufferClamp,
    const float mixScale ) {
    float targetRpm;
    float targetLoad;
    float targetGain;
    float lowpassAlpha;
    float outputRate;
    float clipRates[ENGINE_AUDIO_RECORDED_CLIP_COUNT];
    float accelBlend;
    float idleWeight;
    int accelLow;
    int accelHigh;
    int i;

    if ( !em || !buffer || sampleCount <= 0 ) {
        return;
    }

    if ( !s_engineRecordedClips[0].loaded ) {
        return;
    }

    targetRpm = em->pub.control.rpm;
    if ( targetRpm <= 0.0f ) {
        targetRpm = em->pub.preset ? em->pub.preset->idleRpm : 950.0f;
    }
    targetRpm = S_ClampEngineAudioFloat(
        targetRpm,
        em->pub.preset ? em->pub.preset->idleRpm : 950.0f,
        em->pub.preset ? em->pub.preset->redlineRpm : 8000.0f );

    targetLoad = S_SmoothEngineAudioStep( ( em->pub.control.load - 0.08f ) / 0.48f );
    S_GetEngineRecordedAccelBlend( targetRpm, &accelLow, &accelHigh, &accelBlend );
    if ( accelLow < 1 || accelHigh < 1 ) {
        targetLoad = 0.0f;
    }
    if ( em->recordedAccelLow != accelLow || em->recordedAccelHigh != accelHigh ) {
        em->recordedAccelLow = accelLow;
        em->recordedAccelHigh = accelHigh;
        em->recordedAccelBlend = accelBlend;
    }

    outputRate = dma.speed > 0 ? (float)dma.speed : 44100.0f;
    for ( i = 0; i < ENGINE_AUDIO_RECORDED_CLIP_COUNT; ++i ) {
        const engineAudioRecordedClip_t *clip = &s_engineRecordedClips[i];
        float targetRate;

        targetRate = clip->loaded ? targetRpm / clip->sourceRpm : 1.0f;
        targetRate = S_ClampEngineAudioFloat( targetRate, 0.35f, 4.0f );
        if ( em->recordedRate[i] <= 0.0f ) {
            em->recordedRate[i] = targetRate;
        }
        clipRates[i] = targetRate;
    }

    targetGain = ( s_engineAudioSampleGain ? s_engineAudioSampleGain->value : 4.0f ) *
        ( s_engineAudioGain ? s_engineAudioGain->value : 1.0f ) *
        ( 0.70f + 0.30f * em->pub.control.throttle );
    targetGain = S_ClampEngineAudioFloat( targetGain, 0.0f, 8.0f );

    {
        float cutoff;
        cutoff = em->pub.control.exteriorView ? 10000.0f :
            ( em->pub.preset ? em->pub.preset->cockpitLowpassHz : 2500.0f );
        cutoff = S_ClampEngineAudioFloat( cutoff, 500.0f, outputRate * 0.45f );
        lowpassAlpha = 1.0f - expf( -6.28318530718f * cutoff / outputRate );
    }

    for ( i = 0; i < sampleCount; ++i ) {
        float idleSample;
        float accelA;
        float accelB;
        float accelSample;
        float sample;
        int l;
        int r;
        int j;

        idleSample = S_ReadEngineRecordedClip(
            &s_engineRecordedClips[0], &em->recordedPhase[0], em->recordedRate[0] );

        accelA = 0.0f;
        accelB = 0.0f;
        for ( j = 1; j < ENGINE_AUDIO_RECORDED_CLIP_COUNT; ++j ) {
            float clipSample;

            if ( j != accelLow && j != accelHigh ) {
                continue;
            }

            em->recordedRate[j] += ( clipRates[j] - em->recordedRate[j] ) * 0.0005f;
            clipSample = S_ReadEngineRecordedClip(
                &s_engineRecordedClips[j], &em->recordedPhase[j], em->recordedRate[j] );
            if ( j == accelLow ) {
                accelA = clipSample;
            }
            if ( j == accelHigh ) {
                accelB = clipSample;
            }
        }

        em->recordedRate[0] += ( clipRates[0] - em->recordedRate[0] ) * 0.0005f;
        em->recordedLoad += ( targetLoad - em->recordedLoad ) * 0.0005f;
        em->recordedAccelBlend += ( accelBlend - em->recordedAccelBlend ) * 0.0005f;
        idleWeight = 1.0f - em->recordedLoad;
        accelSample = accelA + ( accelB - accelA ) * em->recordedAccelBlend;
        sample = idleSample * idleWeight + accelSample * em->recordedLoad;

        em->recordedGain += ( targetGain - em->recordedGain ) * 0.0005f;
        if ( !em->pub.control.exteriorView ) {
            em->recordedLowpass += ( sample - em->recordedLowpass ) * lowpassAlpha;
            sample = em->recordedLowpass;
        }
        else {
            em->recordedLowpass = sample;
        }

        sample *= em->recordedGain;
        l = buffer[i].left + (int)( sample * leftGain * mixScale );
        r = buffer[i].right + (int)( sample * rightGain * mixScale );
        if ( l > paintbufferClamp ) l = paintbufferClamp;
        if ( l < -paintbufferClamp ) l = -paintbufferClamp;
        if ( r > paintbufferClamp ) r = paintbufferClamp;
        if ( r < -paintbufferClamp ) r = -paintbufferClamp;
        buffer[i].left = l;
        buffer[i].right = r;
    }
}

void S_RenderEngineAudio( portable_samplepair_t *buffer, int sampleCount ) {
    int i;
    const int paintbufferClamp = 0x00ffff00;
    const float mixScale = 2000.0f * 256.0f;
    static float tempExhaustLeft[4096];
    static float tempExhaustRight[4096];
    static float tempEngineBayLeft[4096];
    static float tempEngineBayRight[4096];

    if ( !buffer || sampleCount <= 0 ) {
        return;
    }

    if ( sampleCount > 4096 ) {
        sampleCount = 4096;
    }

    S_DebugPrintEngineAudioState();

    for ( i = 0; i < MAX_ENGINE_AUDIO_EMITTERS; ++i ) {
        int s;
        float exhaustLeftGain;
        float exhaustRightGain;
        float engineBayLeftGain;
        float engineBayRightGain;
        engineAudioEmitterInternal_t *em = &s_engineEmitters[i];

        if ( !em->pub.active || em->pub.quality == EA_QUALITY_OFF || !em->pub.preset ) {
            continue;
        }

        Com_Memset( tempExhaustLeft, 0, sizeof(float) * sampleCount );
        Com_Memset( tempExhaustRight, 0, sizeof(float) * sampleCount );
        Com_Memset( tempEngineBayLeft, 0, sizeof(float) * sampleCount );
        Com_Memset( tempEngineBayRight, 0, sizeof(float) * sampleCount );

        S_ComputeEngineEmitterSpatialGains( em->pub.exhaustOrigin, em->pub.quality, &exhaustLeftGain, &exhaustRightGain );
        S_ComputeEngineEmitterSpatialGains( em->pub.engineBayOrigin, em->pub.quality, &engineBayLeftGain, &engineBayRightGain );
        if ( exhaustLeftGain <= 0.0f && exhaustRightGain <= 0.0f &&
             engineBayLeftGain <= 0.0f && engineBayRightGain <= 0.0f ) {
            continue;
        }

        if ( em->pub.control.recordedSampleMode && s_engineRecordedClips[0].loaded ) {
            if ( em->pub.control.exteriorView ) {
                S_RenderRecordedEngineVoice(
                    em, buffer, sampleCount,
                    exhaustLeftGain, exhaustRightGain,
                    paintbufferClamp, mixScale );
            }
            else {
                S_RenderRecordedEngineVoice(
                    em, buffer, sampleCount,
                    engineBayLeftGain, engineBayRightGain,
                    paintbufferClamp, mixScale );
            }
            continue;
        }

        S_EngineDSP_RenderVehicle(
            &em->synth,
            em->pub.preset,
            &em->pub.control,
            em->pub.quality,
            sampleCount,
            tempExhaustLeft,
            tempExhaustRight,
            tempEngineBayLeft,
            tempEngineBayRight );

        for ( s = 0; s < sampleCount; ++s ) {
            int l = buffer[s].left +
                (int)( tempExhaustLeft[s] * exhaustLeftGain * mixScale ) +
                (int)( tempEngineBayLeft[s] * engineBayLeftGain * mixScale );
            int r = buffer[s].right +
                (int)( tempExhaustRight[s] * exhaustRightGain * mixScale ) +
                (int)( tempEngineBayRight[s] * engineBayRightGain * mixScale );

            if ( l > paintbufferClamp ) l = paintbufferClamp;
            if ( l < -paintbufferClamp ) l = -paintbufferClamp;
            if ( r > paintbufferClamp ) r = paintbufferClamp;
            if ( r < -paintbufferClamp ) r = -paintbufferClamp;

            buffer[s].left = l;
            buffer[s].right = r;
        }
    }
}
