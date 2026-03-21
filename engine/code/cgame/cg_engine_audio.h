#ifndef CG_ENGINE_AUDIO_H
#define CG_ENGINE_AUDIO_H

#include "cg_local.h"

typedef enum engineAudioQualityTier_e {
    EA_QUALITY_OFF = 0,
    EA_QUALITY_FAR,
    EA_QUALITY_NEAR,
    EA_QUALITY_HERO
} engineAudioQualityTier_t;

typedef struct vehicleAudioState_s {
    float rpm;
    float rpmNorm;

    float throttle;
    float load;
    float speed;

    float clutchSlip;
    float wheelSlip;
    float turboBoost;

    int gear;

    qboolean ignitionCut;
    qboolean fuelCut;
    qboolean limiterActive;
    qboolean backfireEvent;
    qboolean damaged;
} vehicleAudioState_t;

typedef struct cgVehicleAudioDebug_s {
    qboolean valid;
    int entityNum;
    engineAudioQualityTier_t quality;

    float rpm;
    float throttle;
    float load;
    float wheelSlip;
    int gear;
} cgVehicleAudioDebug_t;

void CG_EngineAudio_Init( void );
void CG_EngineAudio_Shutdown( void );
void CG_EngineAudio_Frame( void );
qboolean CG_BuildVehicleAudioState( centity_t *cent, vehicleAudioState_t *outState );
engineAudioQualityTier_t CG_ChooseEngineAudioQuality( centity_t *cent );
const cgVehicleAudioDebug_t *CG_GetVehicleAudioDebug( void );

#endif /* CG_ENGINE_AUDIO_H */
