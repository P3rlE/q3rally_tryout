/*
===========================================================================
  snd_engine_presets.c

  MVP preset registry with hardcoded fallback presets.
===========================================================================
*/

#include "snd_engine_presets.h"

#define MAX_ENGINE_AUDIO_PRESETS 16

static engineAudioPreset_t s_enginePresets[MAX_ENGINE_AUDIO_PRESETS];
static int s_enginePresetCount = 0;

static void S_BuildDefaultSportI4Preset( engineAudioPreset_t *p ) {
    Com_Memset( p, 0, sizeof( *p ) );

    Q_strncpyz( p->name, "sport_i4", sizeof( p->name ) );

    p->cylinderCount = 4;
    p->strokeCycle = 4;
    p->firingOrder[0] = 1;
    p->firingOrder[1] = 3;
    p->firingOrder[2] = 4;
    p->firingOrder[3] = 2;
    p->firingOrderLength = 4;

    p->idleRpm = 950.0f;
    p->redlineRpm = 8000.0f;

    p->exhaustGain = 1.0f;
    p->intakeGain = 0.58f;
    p->mechanicalGain = 0.32f;
    p->transmissionGain = 0.18f;

    p->exhaustResonatorCount = 2;
    p->exhaustResonators[0].frequencyHz = 92.0f;
    p->exhaustResonators[0].q = 1.15f;
    p->exhaustResonators[0].gain = 1.00f;
    p->exhaustResonators[1].frequencyHz = 184.0f;
    p->exhaustResonators[1].q = 1.35f;
    p->exhaustResonators[1].gain = 0.62f;

    p->intakeResonatorCount = 2;
    p->intakeResonators[0].frequencyHz = 410.0f;
    p->intakeResonators[0].q = 0.95f;
    p->intakeResonators[0].gain = 0.55f;
    p->intakeResonators[1].frequencyHz = 760.0f;
    p->intakeResonators[1].q = 0.90f;
    p->intakeResonators[1].gain = 0.35f;

    p->harmonicGains[0] = 1.00f;
    p->harmonicGains[1] = 0.42f;
    p->harmonicGains[2] = 0.16f;
    p->harmonicGains[3] = 0.08f;

    p->distortionDrive = 0.14f;
    p->noiseGain = 0.07f;
    p->backfireGain = 0.35f;
    p->limiterGain = 0.24f;
    p->cockpitLowpassHz = 4500.0f;
    p->exteriorPresenceGain = 1.05f;
}

qboolean S_ValidateEngineAudioPreset( const engineAudioPreset_t *preset ) {
    if ( !preset ) {
        return qfalse;
    }

    if ( preset->cylinderCount <= 0 ) {
        return qfalse;
    }

    if ( preset->strokeCycle != 2 && preset->strokeCycle != 4 ) {
        return qfalse;
    }

    if ( preset->idleRpm <= 0.0f || preset->redlineRpm <= preset->idleRpm ) {
        return qfalse;
    }

    return qtrue;
}

int S_RegisterEngineAudioPreset( const engineAudioPreset_t *preset ) {
    if ( !preset || !S_ValidateEngineAudioPreset( preset ) ) {
        return -1;
    }

    if ( s_enginePresetCount >= MAX_ENGINE_AUDIO_PRESETS ) {
        return -1;
    }

    s_enginePresets[s_enginePresetCount] = *preset;
    return s_enginePresetCount++;
}

int S_FindEngineAudioPreset( const char *name ) {
    int i;

    if ( !name || !name[0] ) {
        return -1;
    }

    for ( i = 0; i < s_enginePresetCount; ++i ) {
        if ( !Q_stricmp( s_enginePresets[i].name, name ) ) {
            return i;
        }
    }

    return -1;
}

const engineAudioPreset_t *S_GetEngineAudioPresetByHandle( int handle ) {
    if ( handle < 0 || handle >= s_enginePresetCount ) {
        return NULL;
    }

    return &s_enginePresets[handle];
}

qboolean S_ParseEngineAudioPresetFile( const char *path, engineAudioPreset_t *outPreset ) {
    (void)path;
    (void)outPreset;
    return qfalse;
}

void S_LoadEngineAudioPresets( void ) {
    engineAudioPreset_t preset;

    s_enginePresetCount = 0;

    S_BuildDefaultSportI4Preset( &preset );
    S_RegisterEngineAudioPreset( &preset );
}
