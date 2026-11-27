/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.
===========================================================================
*/

/*
===========================================================================
  cg_hud_racing.c

  Race state the v0.9 HUD (cg_hud_modern.c) reads:
    - the checkpoint arrow at the top of the screen, and wrong way
    - whether a drag tournament's bracket is due
===========================================================================
*/

#include "cg_local.h"
#include "cg_hud_elements.h"

static qboolean CG_IsMovingAwayFromCheckpoint( const vec3_t velocity,
	const vec3_t toCheckpoint, float angleDiff ) {
	return velocity[0] * velocity[0] + velocity[1] * velocity[1] > 1600 &&
		( toCheckpoint[0] != 0 || toCheckpoint[1] != 0 ) && fabs( angleDiff ) > 100;
}

/* -----------------------------------------------------------------------
   CG_DrawArrowToCheckpoint
   cg_checkpointArrowMode 1 draws the 3D arrow at the top of the screen
   (mode 2 is the one above the car, cg_players.c). Also keeps
   cg.wrongWayTime / wrongWayStartTime: the car moves away from the next
   checkpoint; the HUD shows "WRONG WAY" after two seconds of it.
   ----------------------------------------------------------------------- */
void CG_DrawArrowToCheckpoint( void ) {
	centity_t	*cent;
	vec3_t		forward, origin, angles;
	int			i;
	float		angle1, angle2, angleDiff;
	float		fx, fy, fw, fh;
	refdef_t	refdef;
	refEntity_t	ent;
	vec3_t		mins, maxs, v;

	/* Suppress checkpoint arrow and Wrong Way during intro camera */
	if ( CG_IntroCam_IsActive() ||
		cg.snap->ps.pm_type != PM_NORMAL ||
		!cg_entities[cg.snap->ps.clientNum].startRaceTime ||
		cg.time < cg_entities[cg.snap->ps.clientNum].startRaceTime ||
		cg_entities[cg.snap->ps.clientNum].finishRaceTime ) {
		cg.wrongWayStartTime = 0;
		return;
	}

	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		cent = &cg_entities[i];
		if ( cent->currentState.eType != ET_CHECKPOINT ) continue;
		if ( cent->currentState.weapon != cg.snap->ps.stats[STAT_NEXT_CHECKPOINT] ) continue;
		break;
	}

	if ( i == MAX_GENTITIES ) {
		cg.wrongWayStartTime = 0;
		return;
	}

	trap_R_ModelBounds( cgs.inlineDrawModel[cent->currentState.modelindex], mins, maxs );

	if ( cent->currentState.frame == 0 ) {
		VectorAdd( mins, cent->currentState.origin, mins );
		VectorAdd( maxs, cent->currentState.origin, maxs );
	}

	for ( i = 0; i < 3; i++ ) {
		if      ( cg.predictedPlayerState.origin[i] < mins[i] ) v[i] = mins[i] - cg.predictedPlayerState.origin[i];
		else if ( cg.predictedPlayerState.origin[i] > maxs[i] ) v[i] = maxs[i] - cg.predictedPlayerState.origin[i];
		else                                                      v[i] = 0;
	}

	angle2 = ( v[0] == 0 && v[1] == 0 && v[2] == 0 )
	         ? cg.predictedPlayerState.viewangles[YAW]
	         : vectoyaw( v );

	if ( cg_checkpointArrowMode.integer == 1 ) {
		AngleVectors( cg.refdefViewAngles, forward, NULL, NULL );
		angle1     = vectoyaw( forward );
		angleDiff  = AngleDifference( angle1, angle2 );

		VectorSet( origin, 80, 0, 20 );
		VectorClear( angles );
		angles[YAW] = -angleDiff;

		fx = 320 - 64;  fy = 16;  fw = 128;  fh = 96;
		CG_AdjustFrom640( &fx, &fy, &fw, &fh );

		memset( &refdef, 0, sizeof( refdef ) );
		memset( &ent,    0, sizeof( ent    ) );

		AnglesToAxis( angles, ent.axis );
		VectorCopy( origin, ent.origin );
		VectorCopy( origin, ent.lightingOrigin );
		ent.hModel   = cgs.media.checkpointArrow;
		ent.renderfx = RF_NOSHADOW;

		refdef.rdflags = RDF_NOWORLDMODEL;
		vectoangles( origin, angles );
		AnglesToAxis( angles, refdef.viewaxis );
		refdef.fov_x  = 40;
		refdef.fov_y  = 30;
		refdef.x      = fx;  refdef.y      = fy;
		refdef.width  = fw;  refdef.height = fh;
		refdef.time   = cg.time;

		trap_R_ClearScene();
		trap_R_AddRefEntityToScene( &ent );
		trap_R_RenderScene( &refdef );
	}

	/* Judge travel direction, not where a stationary/reversing car is facing. */
	angle1    = vectoyaw( cg.predictedPlayerState.velocity );
	angleDiff = AngleDifference( angle1, angle2 );

	if ( CG_IsMovingAwayFromCheckpoint( cg.predictedPlayerState.velocity, v, angleDiff ) ) {
		cg.wrongWayTime = cg.time;
		if ( !cg.wrongWayStartTime )
			cg.wrongWayStartTime = cg.time;
	} else {
		cg.wrongWayStartTime = 0;
	}
}

/* A drag tournament's bracket is due: the match's result had its five
 * seconds, and TAB is not held. */
qboolean CG_DragTourneyDue( void ) {
	static int since, lastSeen;
	if ( cgs.gametype != GT_DRAG || !cgs.dragTourney.size || !cg.snap ) return qfalse;
	if ( cg.time - lastSeen > 1000 || cg.time < lastSeen ) since = cg.time;
	lastSeen = cg.time;
	return cg.time - since >= 5000 && !cg.showScores;
}
