/*
===========================================================================
Copyright (C) 2002-2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*
===========================================================================
Autoball, phase 1: physics prototype.

A ball is an ordinary dynamic rally_scripted_object with a sphere-shaped
Bullet body that never sleeps and has a speed cap. Nothing here knows about
teams, goals or scoring yet; that is phase 2 (GT_AUTOBALL).

Ways to get a ball:
  - map entity "autoball_ball" (all rally_scripted_object keys apply), or
  - cheat commands on any map: ball_spawn, ball_reset, ball_remove.
Set g_autoballDebug 1 to print car and ball speed for every touch.
===========================================================================
*/

#include "g_local.h"

#define AUTOBALL_CLASSNAME			"autoball_ball"
#define AUTOBALL_MODEL				"models/autoball/ball.md3"	/* radius 75 */
#define AUTOBALL_DEFAULT_RADIUS		75.0f
#define AUTOBALL_DEFAULT_MASS		400
#define AUTOBALL_DEFAULT_MAX_SPEED	3000.0f
#define AUTOBALL_SPAWN_DISTANCE		400.0f
#define AUTOBALL_MAX_TEST_BALLS		4

/*
Starting values from the design document. Map keys on autoball_ball
override them (radius, mass, elasticity, friction, rolling_friction,
spinning_friction, vehicle_impact_scale, vehicle_vertical_scale,
vehicle_lift, max_speed, never_sleep).
Order on a car contact: vertical_scale flattens the contact normal first,
then vehicle_lift guarantees a minimum upward share.
*/
static void G_Autoball_ApplyDefaults( gentity_t *ent ) {
	const float radius = AUTOBALL_DEFAULT_RADIUS;

	ent->moveable = qtrue;
	ent->mass = AUTOBALL_DEFAULT_MASS;
	ent->inertiaShape = RALLY_OBJECT_INERTIA_SPHERE;
	ent->collisionShape = RALLY_PHYSICS_SHAPE_SPHERE;
	ent->ballRadius = radius;
	VectorSet( ent->r.mins, -radius, -radius, -radius );
	VectorSet( ent->r.maxs, radius, radius, radius );
	ent->elasticity = 0.6f;
	ent->friction = 0.4f;
	ent->rollingFriction = 0.02f;
	ent->spinningFriction = 0.02f;
	ent->vehicleImpactScale = 1.0f;
	ent->weaponImpactScale = 1.0f;
	ent->neverSleep = qtrue;
	ent->maxSpeed = AUTOBALL_DEFAULT_MAX_SPEED;
	ent->vehicleVerticalScale = 0.45f;	/* measured contacts point 35-45 deg up; this gives ~15-20 deg */
	ent->vehicleLift = 0.15f;
	ent->health = 0;
	ent->maxHealth = 0;
	ent->takedamage = qfalse;
}

static void G_Autoball_WarnLegacySolver( void ) {
	if ( !G_RallyPhysics_Enabled() ) {
		G_Printf( S_COLOR_YELLOW "autoball: Bullet backend is off (g_scriptedObjectBullet 0); "
			"the ball falls back to the legacy box solver and will not roll like a sphere.\n" );
	}
}

/*
QUAKED autoball_ball (1 .5 0) (-75 -75 -75) (75 75 75)
Autoball game ball. Its origin is the kick-off spot.
Keys: same as rally_scripted_object; defaults are tuned for Autoball.
"model" defaults to models/autoball/ball.md3 (made for radius 75).
*/
void SP_autoball_ball( gentity_t *ent ) {
	if ( !ent->model || !ent->model[0] )
		ent->model = AUTOBALL_MODEL;

	if ( !G_ParseScriptedObject( ent ) ) {
		G_FreeEntity( ent );
		return;
	}
	G_Autoball_ApplyDefaults( ent );
	/* map keys override the ball defaults and register the model */
	G_ApplyScriptedObjectMapProperties( ent );
	G_Autoball_WarnLegacySolver();
	G_ScriptedObject_FinishSpawn( ent );
	VectorCopy( ent->s.pos.trBase, ent->ballHome );
}

gentity_t *G_Autoball_SpawnBall( const vec3_t origin ) {
	gentity_t *ent;
	vec3_t spawnOrigin;

	VectorCopy( origin, spawnOrigin );
	ent = G_Spawn();
	ent->classname = AUTOBALL_CLASSNAME;
	ent->model = AUTOBALL_MODEL;
	G_SetOrigin( ent, spawnOrigin );
	VectorCopy( spawnOrigin, ent->s.origin );

	if ( !G_ParseScriptedObject( ent ) ) {
		G_FreeEntity( ent );
		return NULL;
	}
	G_Autoball_ApplyDefaults( ent );
	ent->s.modelindex2 = G_ModelIndex( ent->model );
	G_Autoball_WarnLegacySolver();
	G_ScriptedObject_FinishSpawn( ent );
	VectorCopy( ent->s.pos.trBase, ent->ballHome );
	return ent;
}

/* Puts the ball back on its home spot at rest (kick-off, debug reset). */
void G_Autoball_ResetBall( gentity_t *ball ) {
	if ( !ball || !ball->inuse )
		return;

	if ( G_RallyPhysics_Enabled() )
		trap_RallyPhysicsResetBody( ball->s.number, ball->ballHome, vec3_origin, vec3_origin );

	VectorCopy( ball->ballHome, ball->s.pos.trBase );
	VectorCopy( ball->ballHome, ball->s.origin );
	VectorCopy( ball->ballHome, ball->r.currentOrigin );
	VectorClear( ball->s.apos.trBase );
	VectorClear( ball->s.angles );
	VectorClear( ball->r.currentAngles );
	VectorClear( ball->s.pos.trDelta );
	VectorClear( ball->s.apos.trDelta );
	VectorClear( ball->angularMomentum );
	VectorClear( ball->lastNonZeroVelocity );
	ball->s.pos.trTime = level.time;
	ball->s.apos.trTime = level.time;
	ball->updateTime = level.time;
	ball->physicsAccumulatorMsec = 0;
	ball->physicsQuietSince = -1;
	ball->physicsSleeping = qfalse;
	/* clients snap to the new spot instead of interpolating across the map */
	ball->s.eFlags ^= EF_TELEPORT_BIT;
	trap_LinkEntity( ball );
}

/* G_FreeEntity does not know about Bullet, so the body must go first. */
void G_Autoball_RemoveBall( gentity_t *ball ) {
	if ( !ball || !ball->inuse )
		return;
	if ( G_RallyPhysics_Enabled() )
		trap_RallyPhysicsRemoveBody( ball->s.number );
	G_FreeEntity( ball );
}

static int G_Autoball_CountBalls( void ) {
	gentity_t *ball = NULL;
	int count = 0;

	while ( ( ball = G_Find( ball, FOFS( classname ), AUTOBALL_CLASSNAME ) ) != NULL )
		count++;
	return count;
}

/*
==================
Cmd_BallSpawn_f

Drops a test ball AUTOBALL_SPAWN_DISTANCE units ahead of the player's car.
==================
*/
void Cmd_BallSpawn_f( gentity_t *ent ) {
	vec3_t angles, forward, start, end, mins, maxs;
	trace_t trace;
	gentity_t *ball;
	int clientNum;

	if ( !ent || !ent->client )
		return;
	clientNum = ent - g_entities;

	if ( G_Autoball_CountBalls() >= AUTOBALL_MAX_TEST_BALLS ) {
		trap_SendServerCommand( clientNum, va( "print \"There are already %d test balls; use ball_remove first.\n\"",
			AUTOBALL_MAX_TEST_BALLS ) );
		return;
	}

	VectorSet( angles, 0.0f, ent->client->ps.viewangles[YAW], 0.0f );
	AngleVectors( angles, forward, NULL, NULL );

	/* Lift the start above the car so the ball box does not begin inside the
	 * floor, then sweep forward and stop short of any wall in the way. */
	VectorSet( mins, -AUTOBALL_DEFAULT_RADIUS, -AUTOBALL_DEFAULT_RADIUS, -AUTOBALL_DEFAULT_RADIUS );
	VectorSet( maxs, AUTOBALL_DEFAULT_RADIUS, AUTOBALL_DEFAULT_RADIUS, AUTOBALL_DEFAULT_RADIUS );
	VectorCopy( ent->client->ps.origin, start );
	start[2] += AUTOBALL_DEFAULT_RADIUS + 16.0f;
	VectorMA( start, AUTOBALL_SPAWN_DISTANCE, forward, end );
	trap_Trace( &trace, start, mins, maxs, end, clientNum, MASK_PLAYERSOLID );
	if ( trace.startsolid || trace.allsolid ) {
		trap_SendServerCommand( clientNum, "print \"No room for a ball here; drive into the open and try again.\n\"" );
		return;
	}

	ball = G_Autoball_SpawnBall( trace.endpos );
	if ( !ball ) {
		trap_SendServerCommand( clientNum, "print \"ball_spawn: could not create the ball.\n\"" );
		return;
	}
	trap_SendServerCommand( clientNum, va( "print \"Ball %d spawned (%s). ball_reset puts it back here, ball_remove deletes all test balls.\n\"",
		ball->s.number, G_RallyPhysics_Enabled() ? "Bullet sphere" : "legacy solver" ) );
}

void Cmd_BallReset_f( gentity_t *ent ) {
	gentity_t *ball = NULL;
	int count = 0;

	while ( ( ball = G_Find( ball, FOFS( classname ), AUTOBALL_CLASSNAME ) ) != NULL ) {
		G_Autoball_ResetBall( ball );
		count++;
	}
	if ( ent && ent->client )
		trap_SendServerCommand( ent - g_entities, va( "print \"%d ball(s) reset.\n\"", count ) );
	else
		G_Printf( "%d ball(s) reset.\n", count );
}

void Cmd_BallRemove_f( gentity_t *ent ) {
	gentity_t *ball = NULL;
	int count = 0;

	while ( ( ball = G_Find( ball, FOFS( classname ), AUTOBALL_CLASSNAME ) ) != NULL ) {
		G_Autoball_RemoveBall( ball );
		count++;
	}
	if ( ent && ent->client )
		trap_SendServerCommand( ent - g_entities, va( "print \"%d ball(s) removed.\n\"", count ) );
	else
		G_Printf( "%d ball(s) removed.\n", count );
}

/*
==================
Server console / rcon commands (no cheat protection; the admin owns these)

  ball_spawn_at <x> <y> <z>     spawn a test ball at a world position
  ball_kick <vx> <vy> <vz>      give every ball this velocity (units/s)
  ball_info                     position, speed and state of every ball
  ball_reset / ball_remove      as the client commands
==================
*/
static qboolean G_Autoball_ReadVector( int firstArg, vec3_t out ) {
	char buffer[MAX_TOKEN_CHARS];
	int i;

	if ( trap_Argc() < firstArg + 3 )
		return qfalse;
	for ( i = 0; i < 3; i++ ) {
		trap_Argv( firstArg + i, buffer, sizeof( buffer ) );
		out[i] = atof( buffer );
	}
	return qtrue;
}

void Svcmd_BallSpawnAt_f( void ) {
	vec3_t origin;
	gentity_t *ball;

	if ( !G_Autoball_ReadVector( 1, origin ) ) {
		G_Printf( "usage: ball_spawn_at <x> <y> <z>\n" );
		return;
	}
	if ( G_Autoball_CountBalls() >= AUTOBALL_MAX_TEST_BALLS ) {
		G_Printf( "There are already %d test balls; use ball_remove first.\n", AUTOBALL_MAX_TEST_BALLS );
		return;
	}
	ball = G_Autoball_SpawnBall( origin );
	if ( ball )
		G_Printf( "Ball %d spawned at (%.0f %.0f %.0f).\n", ball->s.number,
			ball->s.pos.trBase[0], ball->s.pos.trBase[1], ball->s.pos.trBase[2] );
}

void Svcmd_BallKick_f( void ) {
	vec3_t velocity;
	gentity_t *ball = NULL;
	int count = 0;

	if ( !G_Autoball_ReadVector( 1, velocity ) ) {
		G_Printf( "usage: ball_kick <vx> <vy> <vz>  (units/s; %.2f units = 1 m)\n", CP_M_2_QU );
		return;
	}
	if ( !G_RallyPhysics_Enabled() ) {
		G_Printf( "ball_kick needs the Bullet backend (g_scriptedObjectBullet 1).\n" );
		return;
	}
	while ( ( ball = G_Find( ball, FOFS( classname ), AUTOBALL_CLASSNAME ) ) != NULL ) {
		trap_RallyPhysicsResetBody( ball->s.number, ball->r.currentOrigin,
			ball->r.currentAngles, velocity );
		count++;
	}
	G_Printf( "%d ball(s) kicked with %.0f km/h.\n", count,
		VectorLength( velocity ) / CP_M_2_QU * 3.6f );
}

void Svcmd_BallInfo_f( void ) {
	gentity_t *ball = NULL;
	int count = 0;

	while ( ( ball = G_Find( ball, FOFS( classname ), AUTOBALL_CLASSNAME ) ) != NULL ) {
		G_Printf( "ball %d: pos (%.1f %.1f %.1f)  speed %.1f km/h (%.0f u/s)  spin %.0f deg/s  %s%s\n",
			ball->s.number, ball->r.currentOrigin[0], ball->r.currentOrigin[1],
			ball->r.currentOrigin[2],
			VectorLength( ball->s.pos.trDelta ) / CP_M_2_QU * 3.6f,
			VectorLength( ball->s.pos.trDelta ), VectorLength( ball->s.apos.trDelta ),
			ball->physicsSleeping ? "sleeping" : "awake",
			G_RallyPhysics_Enabled() ? "" : " (legacy solver)" );
		count++;
	}
	if ( !count )
		G_Printf( "No balls. Use ball_spawn (client, cheats) or ball_spawn_at <x> <y> <z>.\n" );
}
