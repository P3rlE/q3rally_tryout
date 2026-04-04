/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

#define INTRO_CAM_DEFAULT_DURATION_MS 1000
#define INTRO_CAM_MIN_DURATION_MS 100
#define INTRO_CAM_MAX_DURATION_MS 60000
#define INTRO_CAM_DEFAULT_FOV 90.0f
#define INTRO_CAM_MIN_FOV 10.0f
#define INTRO_CAM_MAX_FOV 170.0f

static int G_ParseIntroCamBlendType( const char *blendName ) {
	if ( !blendName || !blendName[0] ) {
		return INTRO_CAM_BLEND_CUT;
	}

	if ( !Q_stricmp( blendName, "linear" ) ) {
		return INTRO_CAM_BLEND_LINEAR;
	}

	if ( !Q_stricmp( blendName, "ease" ) || !Q_stricmp( blendName, "easeinout" ) ) {
		return INTRO_CAM_BLEND_EASE_IN_OUT;
	}

	return INTRO_CAM_BLEND_CUT;
}

static qboolean G_ObserverCamSequence_ShouldRegisterSpot( void ) {
	char *sequenceName;
	char *introValue;

	G_SpawnString( "sequence", "", &sequenceName );
	if ( sequenceName[0] ) {
		return ( Q_stricmp( sequenceName, "intro" ) == 0 ) ? qtrue : qfalse;
	}

	G_SpawnString( "intro", "", &introValue );
	if ( !introValue[0] ) {
		return qtrue;
	}

	if ( !Q_stricmp( introValue, "0" ) || !Q_stricmp( introValue, "false" ) || !Q_stricmp( introValue, "no" ) ) {
		return qfalse;
	}

	if ( !Q_stricmp( introValue, "1" ) || !Q_stricmp( introValue, "true" ) || !Q_stricmp( introValue, "yes" ) ) {
		return qtrue;
	}

	return ( Q_stricmp( introValue, "intro" ) == 0 ) ? qtrue : qfalse;
}

static void G_ObserverCamSequence_SortNodesByOrder( void ) {
	int i, j;

	for ( i = 1; i < level.introCamNodeCount; i++ ) {
		intro_cam_node_t key = level.introCamNodes[i];
		j = i - 1;
		while ( j >= 0 && level.introCamNodes[j].order > key.order ) {
			level.introCamNodes[j + 1] = level.introCamNodes[j];
			j--;
		}
		level.introCamNodes[j + 1] = key;
	}
}

static void G_ObserverCamSequence_ResolveLookAtTargets( void ) {
	int i;

	for ( i = 0; i < level.introCamNodeCount; i++ ) {
		gentity_t *targetEnt;
		intro_cam_node_t *node;

		node = &level.introCamNodes[i];
		if ( !node->lookAtTargetName || !node->lookAtTargetName[0] ) {
			continue;
		}

		targetEnt = G_Find( NULL, FOFS( targetname ), node->lookAtTargetName );
		if ( !targetEnt ) {
			G_Printf( "Warning: Intro observer spot order=%d references unknown lookat_target '%s'.\n",
				node->order, node->lookAtTargetName );
			continue;
		}

		VectorCopy( targetEnt->s.origin, node->lookAt );
		node->hasLookAt = qtrue;
	}
}

void G_ObserverCamSequence_RegisterSpot( gentity_t *ent ) {
	int				nodeIndex;
	int				order;
	float				durationSeconds;
	char				*blendName;
	int				durationMs;
	int				lookAtProvided;
	int				hasOrder;
	int				orderValue;
	float				fov;
	char				*lookAtTargetName;

	if ( !ent ) {
		return;
	}

	if ( !G_ObserverCamSequence_ShouldRegisterSpot() ) {
		return;
	}

	if ( level.introCamNodeCount >= MAX_INTRO_CAM_NODES ) {
		G_Printf( "Warning: Too many intro observer spots (max %i); ignoring '%s'\n",
			MAX_INTRO_CAM_NODES, vtos( ent->s.origin ) );
		level.raceIntroFallback = qtrue;
		return;
	}

	order = level.introCamNodeCount;
	hasOrder = G_SpawnInt( "order", "0", &orderValue );
	if ( hasOrder ) {
		order = orderValue;
	}
	if ( order < 0 ) {
		G_Printf( "Warning: Intro observer spot at %s has invalid order=%d; clamping to 0.\n",
			vtos( ent->s.origin ), order );
		order = 0;
	}

	durationSeconds = 0.0f;
	G_SpawnFloat( "duration", "0", &durationSeconds );
	durationMs = ( durationSeconds > 0.0f ) ? (int)( durationSeconds * 1000.0f ) : INTRO_CAM_DEFAULT_DURATION_MS;
	if ( durationMs < INTRO_CAM_MIN_DURATION_MS || durationMs > INTRO_CAM_MAX_DURATION_MS ) {
		G_Printf( "Warning: Intro observer spot at %s has invalid duration=%.3fs; using default %.3fs.\n",
			vtos( ent->s.origin ), durationSeconds, INTRO_CAM_DEFAULT_DURATION_MS / 1000.0f );
		durationMs = INTRO_CAM_DEFAULT_DURATION_MS;
	}

	fov = INTRO_CAM_DEFAULT_FOV;
	G_SpawnFloat( "fov", "90", &fov );
	if ( fov < INTRO_CAM_MIN_FOV || fov > INTRO_CAM_MAX_FOV ) {
		G_Printf( "Warning: Intro observer spot at %s has invalid fov=%.1f; clamping to %.1f.\n",
			vtos( ent->s.origin ), fov,
			(fov < INTRO_CAM_MIN_FOV) ? INTRO_CAM_MIN_FOV : INTRO_CAM_MAX_FOV );
		fov = (fov < INTRO_CAM_MIN_FOV) ? INTRO_CAM_MIN_FOV : INTRO_CAM_MAX_FOV;
	}

	nodeIndex = level.introCamNodeCount;
	VectorCopy( ent->s.origin, level.introCamNodes[nodeIndex].position );
	VectorCopy( ent->s.angles, level.introCamNodes[nodeIndex].angles );
	level.introCamNodes[nodeIndex].durationMs = durationMs;
	level.introCamNodes[nodeIndex].order = order;
	level.introCamNodes[nodeIndex].fov = fov;
	level.introCamNodes[nodeIndex].hasLookAt = qfalse;
	level.introCamNodes[nodeIndex].lookAtTargetName = NULL;

	G_SpawnString( "blend", "", &blendName );
	level.introCamNodes[nodeIndex].blendType = G_ParseIntroCamBlendType( blendName );

	lookAtProvided = G_SpawnVector( "lookat", "0 0 0", level.introCamNodes[nodeIndex].lookAt );
	if ( lookAtProvided ) {
		level.introCamNodes[nodeIndex].hasLookAt = qtrue;
	}

	G_SpawnString( "lookat_target", "", &lookAtTargetName );
	if ( lookAtTargetName[0] ) {
		if ( level.introCamNodes[nodeIndex].hasLookAt ) {
			G_Printf( "Warning: Intro observer spot at %s has both lookat and lookat_target; keeping lookat vector.\n",
				vtos( ent->s.origin ) );
		} else {
			level.introCamNodes[nodeIndex].lookAtTargetName = lookAtTargetName;
		}
	}

	level.introCamNodeCount++;
}

void G_ObserverCamSequence_Finalize( void ) {
	int i;

	level.raceIntroDurationMs = 0;
	level.raceIntroHasSequence = ( level.introCamNodeCount > 0 ) ? qtrue : qfalse;

	if ( !level.raceIntroHasSequence ) {
		level.raceIntroFallback = qtrue;
		G_Printf( "Info: No intro camera sequence found; using countdown fallback.\n" );
		return;
	}

	G_ObserverCamSequence_SortNodesByOrder();
	G_ObserverCamSequence_ResolveLookAtTargets();

	for ( i = 0; i < level.introCamNodeCount; i++ ) {
		level.raceIntroDurationMs += level.introCamNodes[i].durationMs;
	}

	if ( level.raceIntroDurationMs <= 0 ) {
		level.raceIntroFallback = qtrue;
		level.raceIntroHasSequence = qfalse;
		G_Printf( "Warning: Intro camera sequence has invalid duration; using countdown fallback.\n" );
	}
}

/*
==============
SP_info_observer_spot

Intro sequence keys (all optional):
 - sequence / intro: sequence selector. Defaults to intro when omitted.
   `sequence` supports "intro"; `intro` supports 1/0, true/false, yes/no, or "intro".
 - order: playback order. Default: spawn order index.
 - duration: node duration in seconds. Default: 1.0.
 - lookat_target: targetname of an entity to look at. Default: unused.
 - lookat: explicit world position override. Default: unused.
 - fov: node field-of-view. Default: 90.
==============
*/
void SP_info_observer_spot( gentity_t *ent ){
	G_SetOrigin(ent, ent->s.origin);

	if( ent->target )
	{
		ent->spawnflags |= OBSERVERCAM_FIXED;
	}

	G_ObserverCamSequence_RegisterSpot( ent );
}


gentity_t *FindBestObserverSpot( gentity_t *self, gentity_t *target, vec3_t spot, vec3_t angles){
	gentity_t		*ent;
	trace_t			tr;
	vec3_t			delta;
	vec3_t			targetOrigin;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };
	float			dist, bestDist;
	gentity_t		*foundSpot;

	// Use ps.origin as the target reference for both trace and distance checks
	// so observer spot selection stays consistent and more deterministic.
	VectorCopy(target->client->ps.origin, targetOrigin);

	foundSpot = NULL;
	dist = 0;
	bestDist = 0;
	ent = NULL;
	while ( (ent = G_Find (ent, FOFS(classname), "info_observer_spot")) != NULL )
	{
//		if ( !trap_InPVS( ent->s.origin, target->s.origin) ) continue;

//		Com_Printf("Found an observer spot in PVS\n");
//		VectorCopy(ent->s.origin, spot);
//		foundSpot = ent;
//		return foundSpot;
		
		trap_Trace(&tr, ent->r.currentOrigin, mins, maxs, targetOrigin, target->s.number, CONTENTS_SOLID);

		if (tr.startsolid || tr.allsolid || tr.fraction < 1.0) continue;

		VectorSubtract(targetOrigin, ent->s.origin, delta);
		dist = VectorNormalize(delta);

		// check for spot with locked angles
		if (ent->spawnflags & OBSERVERCAM_FIXED)
		{
			vec3_t	forward;

			AngleVectors(ent->s.angles, forward, NULL, NULL);
			if (DotProduct(delta, forward) < -0.40)
			{
				VectorCopy(ent->s.origin, spot);
				VectorCopy(ent->s.angles, angles);

				self->spotflags = ent->spawnflags;

				// use this one
				return ent;
			}
		}

		if (dist < bestDist || bestDist == 0)
		{
			bestDist = dist;
			VectorCopy(ent->s.origin, spot);
			VectorCopy(ent->s.angles, angles);

//			Com_Printf("Found a valid observer spot\n");
			self->spotflags = ent->spawnflags;
			foundSpot = ent;
		}
	}

	return foundSpot;
}

void UpdateObserverSpot( gentity_t *ent, qboolean forceUpdate ){
	vec3_t			origin, angles;
	trace_t			tr;
	int				clientNum;
	gclient_t		*targetClient;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };

	clientNum = ent->client->sess.spectatorClient;
	if ( clientNum == -1 )
		clientNum = level.follow1;
	else if ( clientNum == -2 )
		clientNum = level.follow2;

	if (clientNum < 0)
	{
//		ent->client->sess.spectatorState = SPECTATOR_FREE;
//		G_DebugLogPrintf( "UpdateObserverSpot: drop back to free\n" );
		StopFollowing( ent );
//		ClientSpawn( ent );
		return;
	}

	if ( clientNum < 0 || clientNum >= level.maxclients )
	{
		StopFollowing( ent );
		return;
	}

	targetClient = &level.clients[clientNum];
	if ( targetClient->pers.connected != CON_CONNECTED || targetClient->sess.sessionTeam == TEAM_SPECTATOR )
	{
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	}

	trap_Trace( &tr, ent->client->ps.origin, mins, maxs, targetClient->ps.origin, ent->s.number, CONTENTS_SOLID );
	if ( forceUpdate || tr.fraction < 1 )
	{
		if ( !FindBestObserverSpot(ent, &g_entities[clientNum], origin, angles) )
		{
			if (ent->updateTime + 500 < level.time){
				ent->updateTime = level.time;
				trap_SendServerCommand( ent - g_entities, "print \"Couldnt find valid observer spot, dropping back to follow mode.\n\"" );
				ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
				return;
			}
		}
		else
		{
//			Com_Printf( "Updating observer position" );

			G_SetOrigin(ent, origin);
			VectorCopy(origin, ent->client->ps.origin);
			VectorCopy(angles, ent->client->ps.viewangles);
			ent->updateTime = level.time;
		}
	}
	else {
		ent->updateTime = level.time;
	}
}
