#ifndef ENGINE_AUDIO_SHARED_H
#define ENGINE_AUDIO_SHARED_H

#include "q_shared.h"

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
    qboolean exteriorView;
} vehicleAudioState_t;

#endif /* ENGINE_AUDIO_SHARED_H */
