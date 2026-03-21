/*
===========================================================================
  cg_engine_audio.c

  Draft client-side vehicle audio control collection for the procedural
  engine audio system. This file is intentionally not integrated into the
  build yet; it documents the proposed runtime API and control flow.
===========================================================================
*/

#include "cg_engine_audio.h"

static cgVehicleAudioDebug_t s_cgVehicleAudioDebug;

static float CG_CalcEngineAudioRpmNorm( float rpm, float idleRpm, float redlineRpm ) {
    float norm;

    if ( redlineRpm <= idleRpm ) {
        return 0.0f;
    }

    norm = ( rpm - idleRpm ) / ( redlineRpm - idleRpm );

    if ( norm < 0.0f ) {
        norm = 0.0f;
    }
    else if ( norm > 1.0f ) {
        norm = 1.0f;
    }

    return norm;
}

static float CG_ApproxVehicleThrottle( centity_t *cent ) {
    (void)cent;

    if ( !cg.snap ) {
        return 0.0f;
    }

    return CG_CalcEngineAudioRpmNorm(
        (float)cg.predictedPlayerState.stats[STAT_RPM],
        900.0f,
        8000.0f );
}

static float CG_ApproxVehicleLoad( centity_t *cent ) {
    float throttle;

    throttle = CG_ApproxVehicleThrottle( cent );
    return 0.25f + 0.75f * throttle;
}

static qboolean CG_IsVehicleEntity( centity_t *cent ) {
    if ( !cent ) {
        return qfalse;
    }

    if ( cent->currentState.eType != ET_PLAYER ) {
        return qfalse;
    }

    return qtrue;
}

void CG_EngineAudio_Init( void ) {
    Com_Memset( &s_cgVehicleAudioDebug, 0, sizeof( s_cgVehicleAudioDebug ) );
}

void CG_EngineAudio_Shutdown( void ) {
    Com_Memset( &s_cgVehicleAudioDebug, 0, sizeof( s_cgVehicleAudioDebug ) );
}

qboolean CG_BuildVehicleAudioState( centity_t *cent, vehicleAudioState_t *outState ) {
    float rpm;
    float throttle;
    float load;

    if ( !cent || !outState || !cg.snap ) {
        return qfalse;
    }

    if ( !CG_IsVehicleEntity( cent ) ) {
        return qfalse;
    }

    Com_Memset( outState, 0, sizeof( *outState ) );

    if ( cent->currentState.number == cg.predictedPlayerState.clientNum ) {
        rpm = (float)cg.predictedPlayerState.stats[STAT_RPM];
        outState->gear = cg.predictedPlayerState.stats[STAT_GEAR];
        outState->speed = VectorLength( cg.predictedPlayerState.velocity );
    }
    else {
        rpm = 2000.0f;
        outState->gear = 2;
        outState->speed = VectorLength( cent->currentState.pos.trDelta );
    }

    throttle = CG_ApproxVehicleThrottle( cent );
    load = CG_ApproxVehicleLoad( cent );

    outState->rpm = rpm;
    outState->rpmNorm = CG_CalcEngineAudioRpmNorm( rpm, 900.0f, 8000.0f );
    outState->throttle = throttle;
    outState->load = load;

    outState->clutchSlip = 0.0f;
    outState->wheelSlip = 0.0f;
    outState->turboBoost = 0.0f;

    outState->ignitionCut = qfalse;
    outState->fuelCut = qfalse;
    outState->limiterActive = ( rpm > 7800.0f ) ? qtrue : qfalse;
    outState->backfireEvent = qfalse;
    outState->damaged = qfalse;

    return qtrue;
}

engineAudioQualityTier_t CG_ChooseEngineAudioQuality( centity_t *cent ) {
    float dist;
    vec3_t delta;

    if ( !cent || !cg.snap ) {
        return EA_QUALITY_OFF;
    }

    if ( cent->currentState.number == cg.predictedPlayerState.clientNum ) {
        return EA_QUALITY_HERO;
    }

    VectorSubtract( cent->lerpOrigin, cg.refdef.vieworg, delta );
    dist = VectorLength( delta );

    if ( dist < 700.0f ) {
        return EA_QUALITY_NEAR;
    }

    if ( dist < 1800.0f ) {
        return EA_QUALITY_FAR;
    }

    return EA_QUALITY_OFF;
}

const cgVehicleAudioDebug_t *CG_GetVehicleAudioDebug( void ) {
    return &s_cgVehicleAudioDebug;
}

void CG_EngineAudio_Frame( void ) {
    /*
    Draft control flow only.

    Intended future flow:
    - iterate active vehicle entities
    - build vehicleAudioState_t per entity
    - choose quality tier
    - register/update sound backend engine emitters
    - expose one representative debug sample to HUD diagnostics
    */
    Com_Memset( &s_cgVehicleAudioDebug, 0, sizeof( s_cgVehicleAudioDebug ) );
}
