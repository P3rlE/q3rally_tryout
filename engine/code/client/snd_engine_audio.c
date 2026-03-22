/*
===========================================================================
  snd_engine_audio.c

  Draft procedural engine audio emitter management and mixer entry points.
  This file is intentionally not wired into the build yet; it exists as a
  concrete starting point for implementation.
===========================================================================
*/

#include "snd_engine_audio.h"
#include "snd_engine_dsp.h"
#include "snd_engine_presets.h"

typedef struct engineAudioEmitterInternal_s {
    engineAudioEmitterPublicState_t pub;
    engineAudioSynthState_t synth;

    qboolean initialized;
    int generation;
    int lastUpdateFrame;
} engineAudioEmitterInternal_t;

static engineAudioEmitterInternal_t s_engineEmitters[MAX_ENGINE_AUDIO_EMITTERS];
static int s_engineAudioFrameCounter;

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

static void S_FreeEngineEmitter( engineAudioEmitterInternal_t *em ) {
    if ( !em ) {
        return;
    }

    Com_Memset( em, 0, sizeof( *em ) );
}

void S_EngineAudio_Init( void ) {
    Com_Memset( s_engineEmitters, 0, sizeof( s_engineEmitters ) );
    s_engineAudioFrameCounter = 0;
    S_LoadEngineAudioPresets();
}

void S_EngineAudio_Shutdown( void ) {
    Com_Memset( s_engineEmitters, 0, sizeof( s_engineEmitters ) );
    s_engineAudioFrameCounter = 0;
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
        S_EngineDSP_Reset( &em->synth, dma.speed > 0 ? (float)dma.speed : 44100.0f );
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
    const vec3_t origin,
    const vec3_t velocity,
    engineAudioQualityTier_t quality ) {
    engineAudioEmitterInternal_t *em;

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

    em->lastUpdateFrame = s_engineAudioFrameCounter;
    em->pub.control = *state;
    em->pub.quality = quality;
    VectorCopy( origin, em->pub.origin );
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
    }
}

void S_StopAllEngineEmitters( void ) {
    Com_Memset( s_engineEmitters, 0, sizeof( s_engineEmitters ) );
    s_engineAudioFrameCounter = 0;
}

static void S_ComputeEngineEmitterSpatialGains(
    const engineAudioEmitterInternal_t *em,
    float *leftGain,
    float *rightGain ) {
    int leftVol;
    int rightVol;
    int masterVol;
    vec3_t origin;

    if ( !leftGain || !rightGain ) {
        return;
    }

    *leftGain = 0.0f;
    *rightGain = 0.0f;

    if ( !em ) {
        return;
    }

    masterVol = 220;
    if ( em->pub.quality == EA_QUALITY_NEAR ) {
        masterVol = 180;
    }
    else if ( em->pub.quality == EA_QUALITY_FAR ) {
        masterVol = 132;
    }

    VectorCopy( em->pub.origin, origin );
    S_SpatializeOrigin( origin, masterVol, &leftVol, &rightVol );

    *leftGain = leftVol / 255.0f;
    *rightGain = rightVol / 255.0f;
}

void S_RenderEngineAudio( portable_samplepair_t *buffer, int sampleCount ) {
    int i;
    static float tempLeft[4096];
    static float tempRight[4096];

    if ( !buffer || sampleCount <= 0 ) {
        return;
    }

    if ( sampleCount > 4096 ) {
        sampleCount = 4096;
    }

    for ( i = 0; i < MAX_ENGINE_AUDIO_EMITTERS; ++i ) {
        int s;
        float leftGain;
        float rightGain;
        engineAudioEmitterInternal_t *em = &s_engineEmitters[i];

        if ( !em->pub.active || em->pub.quality == EA_QUALITY_OFF || !em->pub.preset ) {
            continue;
        }

        Com_Memset( tempLeft, 0, sizeof(float) * sampleCount );
        Com_Memset( tempRight, 0, sizeof(float) * sampleCount );

        S_ComputeEngineEmitterSpatialGains( em, &leftGain, &rightGain );
        if ( leftGain <= 0.0f && rightGain <= 0.0f ) {
            continue;
        }

        S_EngineDSP_RenderEmitter(
            &em->synth,
            em->pub.preset,
            &em->pub.control,
            em->pub.quality,
            sampleCount,
            tempLeft,
            tempRight );

        for ( s = 0; s < sampleCount; ++s ) {
            int l = buffer[s].left + (int)( tempLeft[s] * leftGain * 2000.0f );
            int r = buffer[s].right + (int)( tempRight[s] * rightGain * 2000.0f );

            if ( l > 32767 ) l = 32767;
            if ( l < -32768 ) l = -32768;
            if ( r > 32767 ) r = 32767;
            if ( r < -32768 ) r = -32768;

            buffer[s].left = l;
            buffer[s].right = r;
        }
    }
}
