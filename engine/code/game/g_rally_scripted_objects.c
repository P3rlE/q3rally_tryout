/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

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

#define		MAX_SCRIPT_TEXT		8192
#define SCRIPTED_OBJECT_PHYSICS_STEP_MSEC	10
#define SCRIPTED_OBJECT_MAX_SUBSTEPS		10
#define SCRIPTED_OBJECT_MAX_FRAME_MSEC		100

/* collision types */
#define		CT_BOX			0
#define		CT_CONE			1
#define		CT_CYLINDER		2

/* Global temporary force vector for testing */
vec3_t		tempForce;

static qboolean G_ParseScriptVector( char **text_p, vec3_t value ) {
	char *token;
	int parsed;
	int i;

	token = COM_Parse( text_p );
	if ( !token || !token[0] )
		return qfalse;

	parsed = sscanf( token, "%f %f %f", &value[0], &value[1], &value[2] );
	if ( parsed == 3 )
		return qtrue;

	value[0] = atof( token );
	for ( i = 1; i < 3; i++ ) {
		token = COM_Parse( text_p );
		if ( !token || !token[0] )
			return qfalse;
		value[i] = atof( token );
	}
	return qtrue;
}

qboolean SeekToSection( char **pointer, char *str ){
	char		*token;

	/* UPDATE: using strstr instead? */
	/* UPDATE: check if end of file is inside of a bracket (ie bad brackets in script file) */

	/* seek to 'str {' */
	while ( 1 ) {
		token = COM_Parse( pointer );

		if( !token || token[0] == 0 )
			return qfalse;

		if ( !Q_stricmp( token, "{" ) ){
			/* loop through this */
			while ( 1 ) {
				token = COM_Parse( pointer );

				if( !token || token[0] == 0 )
					return qfalse;

				if ( !Q_stricmp( token, "}" ) )
					break;
			}
		}

		if ( !Q_stricmp( token, str ) )
			break;
	}

	if( !token || token[0] == 0 ) /* model not found */
		return qfalse;

	return qtrue;
}

qboolean G_ParseScriptedObject( gentity_t *ent ){
	char		*text_p;
	int			len;
	char		*token;
	char		text[MAX_SCRIPT_TEXT];
	char		filename[MAX_QPATH];
	fileHandle_t	f;

	/* setup defaults */
	ent->takedamage = qfalse;
	VectorSet( ent->r.mins, -16.0f, -16.0f, -16.0f );
	VectorSet( ent->r.maxs,  16.0f,  16.0f,  16.0f );
	ent->elasticity = 0.1f;
	ent->friction = 0.6f;
	ent->mass = 100;
	ent->moveable = qfalse;
	ent->number = 0;
	ent->health = 0;
	ent->maxHealth = 0;
	ent->s.modelindex = 0;

	if (!ent->script || ent->script[0] == 0){
		/* A direct model key is enough for a basic map-authored physics prop. */
		return ( ent->model && ent->model[0] ) ? qtrue : qfalse;
	}

	/* for debugging only load one object */
	/* if( Q_stricmp( ent->script, "models/mapobjects/barrels/barrel01" ) ) */
	/*	return qfalse; */

	Q_strncpyz(filename, ent->script, sizeof(filename));
	token = strchr(filename, '.');
	if (!token)
		Q_strcat(filename, sizeof(filename), ".script");

	if (g_developer.integer)
		Com_Printf("Attempting to load script %s\n", filename);

	/* load the file */
	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( !f ){
		Com_Printf("Could not find script %s\n", filename);
		return qfalse;
	}

	if ( len >= MAX_SCRIPT_TEXT ) {
		len = MAX_SCRIPT_TEXT - 1;
	}

	trap_FS_Read( text, len, f );
	text[len] = 0;

	trap_FS_FCloseFile( f );

	/* parse the text */
	text_p = text;

	/* seek to "rally_scripted_object {" */
	if ( !SeekToSection( &text_p, "rally_scripted_object" ) ){
		Com_Printf( "Script file '%s' did not contain rally_scripted_object\n", filename );
		return qfalse;
	}

	/* send script file name in CS so we dont need */
	/* to do all of the drawing and stuff server side. */
	ent->s.modelindex = G_ScriptIndex( ent->script );

	/* read optional parameters */
	while ( 1 ) {
		token = COM_Parse( &text_p );

		if( !token || token[0] == 0 || !Q_stricmp( token, "}" ) ) {
			break;
		}

		if ( !Q_stricmp( token, "{" ) )
			continue;

		if (g_developer.integer)
			Com_Printf("Found token: %s\n", token);

		if ( !Q_stricmp( token, "type" ) ){
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}

			ent->s.weapon = atoi(token);

			continue;
		}
		else if ( !Q_stricmp( token, "model" ) ){
			COM_Parse( &text_p );
		}
		else if ( !Q_stricmp( token, "deadmodel" ) ){
			COM_Parse( &text_p );
		}
		else if ( !Q_stricmp( token, "moveable" ) ){
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}

			ent->moveable = atoi(token);

			continue;
		}
		else if ( !Q_stricmp( token, "elasticity" ) ){
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}

			ent->elasticity = atof(token);

			continue;
		}
		else if ( !Q_stricmp( token, "mass" ) ){
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}

			ent->mass = atoi(token);
			if (ent->mass <= 0)
				ent->mass = 100;

			continue;
		}
		else if ( !Q_stricmp( token, "frames" ) ){
			COM_Parse( &text_p );
			COM_Parse( &text_p );
			COM_Parse( &text_p );
			COM_Parse( &text_p );
		}
		else if ( !Q_stricmp( token, "health" ) ){
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}

			ent->maxHealth = ent->health = atoi(token);
			ent->takedamage = ( ent->health > 0 ) ? qtrue : qfalse;

			continue;
		}
		else if ( !Q_stricmp( token, "mins" ) ){
			if ( !G_ParseScriptVector( &text_p, ent->r.mins ) )
				return qfalse;

			continue;
		}
		else if ( !Q_stricmp( token, "maxs" ) ){
			if ( !G_ParseScriptVector( &text_p, ent->r.maxs ) )
				return qfalse;

			continue;
		}
		else if ( !Q_stricmp( token, "friction" ) ){
			token = COM_Parse( &text_p );
			if ( !token )
				break;
			ent->friction = atof( token );
			continue;
		}
		else if ( !Q_stricmp( token, "hitsound" ) ){
			COM_Parse( &text_p );
		}
		else if ( !Q_stricmp( token, "presound" ) ){
			COM_Parse( &text_p );
		}
		else if ( !Q_stricmp( token, "postsound" ) ){
			COM_Parse( &text_p );
		}
		else if ( !Q_stricmp( token, "destroysound" ) ){
			COM_Parse( &text_p );
		}
		else if ( !Q_stricmp( token, "gibs" ) ) {
			/* skip gibs part of script (it is only used client side) */
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}

			if ( !Q_stricmp( token, "{" ) ){
				/* loop through this */
				while ( 1 ) {
					token = COM_Parse( &text_p );

					if( !token || token[0] == 0 )
						return qfalse;

					if ( !Q_stricmp( token, "}" ) )
						break;
				}
			}

			continue;
		}
		else {
			Com_Printf("Warning: Skipping unknown token %s in %s\n", token, filename);
			continue;
		}
	}

	if (g_developer.integer)
		Com_Printf("Successfully parsed script file\n");

	return qtrue;
}

void G_ScriptedObject_Destroy( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ){
	if (g_developer.integer) Com_Printf("Destroying scripted map object %s\n", self->classname);

	self->s.eFlags |= EF_DEAD;
	self->takedamage = qfalse;
	self->moveable = qfalse;
	self->r.contents = 0;
	self->think = NULL;
	self->nextthink = 0;
	trap_LinkEntity( self );
}

void G_ScriptedObject_Touch ( gentity_t *self, gentity_t *other, trace_t *trace ){
	vec3_t		vehicleVelocity;
	vec3_t		relativeVelocity;
	vec3_t	outwardNormal;
	float		closingSpeed;

	if ( !self->moveable || !other || !trace )
		return;

	/* ClientImpacts builds this from contact toward the touched prop; it is the
	 * outward direction in which the car should impart momentum. */
	VectorCopy( trace->plane.normal, outwardNormal );
	if ( VectorNormalize( outwardNormal ) == 0.0f )
		return;

	VectorCopy( other->s.pos.trDelta, vehicleVelocity );
	VectorSubtract( self->s.pos.trDelta, vehicleVelocity, relativeVelocity );
	closingSpeed = DotProduct( relativeVelocity, outwardNormal );
	if ( closingSpeed >= -0.01f )
		return;

	/* Resolve the contact in the prop's frame; the heavier car acts as a
	 * moving kinematic body. The impulse also contributes angular momentum. */
	VectorCopy( relativeVelocity, self->s.pos.trDelta );
	G_RallyObject_ApplyCollision( self, trace->endpos, outwardNormal, self->elasticity );
	VectorAdd( self->s.pos.trDelta, vehicleVelocity, self->s.pos.trDelta );
	VectorCopy( self->s.pos.trDelta, self->lastNonZeroVelocity );
}

void G_ScriptedObject_Think ( gentity_t *self ){
	int elapsedMsec;
	int steps;

	if ( !self->moveable )
		return;

	elapsedMsec = level.time - self->updateTime;
	if ( elapsedMsec < 0 )
		elapsedMsec = 0;
	if ( elapsedMsec > SCRIPTED_OBJECT_MAX_FRAME_MSEC )
		elapsedMsec = SCRIPTED_OBJECT_MAX_FRAME_MSEC;
	self->updateTime = level.time;
	self->physicsAccumulatorMsec += elapsedMsec;

	steps = 0;
	while ( self->physicsAccumulatorMsec >= SCRIPTED_OBJECT_PHYSICS_STEP_MSEC &&
		steps < SCRIPTED_OBJECT_MAX_SUBSTEPS ) {
		float stepSeconds;

		stepSeconds = SCRIPTED_OBJECT_PHYSICS_STEP_MSEC * 0.001f;
		VectorSet( self->netForce, 0.0f, 0.0f, -CP_CURRENT_GRAVITY * self->mass );
		VectorClear( self->netMoment );
		G_RallyObject_IntegratePhysics( self, stepSeconds );
		G_RallyObject_TracePhysics( self, stepSeconds );

		self->physicsAccumulatorMsec -= SCRIPTED_OBJECT_PHYSICS_STEP_MSEC;
		steps++;
	}

	/* Update entity position and angles for rendering */
	VectorCopy( self->s.pos.trBase, self->r.currentOrigin );
	VectorCopy( self->s.pos.trBase, self->s.origin );
	VectorCopy( self->s.apos.trBase, self->r.currentAngles );
	VectorCopy( self->s.apos.trBase, self->s.angles );
	
	/* Link entity into world for collision detection */
	trap_LinkEntity( self );

	/* Accumulated fixed 10 ms steps make server frame jitter irrelevant. */
	self->nextthink = level.time + SCRIPTED_OBJECT_PHYSICS_STEP_MSEC;
}

void G_ScriptedObject_Pain ( gentity_t *self, gentity_t *attacker, int damage ){
	/* Optional: Hit sound and frame animation based on health */
	/*
	if (self->number > 0){
		self->s.frame = self->number - ((self->maxHealth / (float)self->number) * self->health);
	}
	*/

	/* Debug output for pain events */
	/* Com_Printf("Scripted map object %s was hit\n", self->classname); */
}

static void G_ApplyScriptedObjectMapProperties( gentity_t *ent ) {
	char *physics;
	int value;
	float floatValue;
	vec3_t vectorValue;
	qboolean physicsSpecified;

	physicsSpecified = G_SpawnString( "physics", NULL, &physics );
	if ( physicsSpecified ) {
		if ( !Q_stricmp( physics, "dynamic" ) || !Q_stricmp( physics, "movable" ) ) {
			ent->moveable = qtrue;
		} else if ( !Q_stricmp( physics, "static" ) ) {
			ent->moveable = qfalse;
		} else {
			Com_Printf( "rally_scripted_object: unknown physics mode '%s' (use static or dynamic)\n", physics );
		}
	} else if ( G_SpawnInt( "moveable", "0", &value ) ) {
		/* Legacy spelling remains supported; map keys override the archetype. */
		ent->moveable = value ? qtrue : qfalse;
	}

	if ( G_SpawnInt( "mass", "100", &value ) ) {
		if ( value < 1 ) value = 1;
		if ( value > 100000 ) value = 100000;
		ent->mass = value;
	}
	if ( G_SpawnFloat( "elasticity", "0.1", &floatValue ) ) {
		ent->elasticity = Com_Clamp( 0.0f, 1.0f, floatValue );
	}
	if ( G_SpawnFloat( "friction", "0.6", &floatValue ) ) {
		ent->friction = Com_Clamp( 0.0f, 4.0f, floatValue );
	}
	if ( G_SpawnInt( "health", "0", &value ) ) {
		ent->health = value > 0 ? value : 0;
		ent->maxHealth = ent->health;
		ent->takedamage = ent->health > 0 ? qtrue : qfalse;
	} else if ( ent->health <= 0 ) {
		/* Both legacy -1 and documented 0 mean indestructible. */
		ent->health = 0;
		ent->maxHealth = 0;
		ent->takedamage = qfalse;
	}
	if ( G_SpawnVector( "mins", "0 0 0", vectorValue ) )
		VectorCopy( vectorValue, ent->r.mins );
	if ( G_SpawnVector( "maxs", "0 0 0", vectorValue ) )
		VectorCopy( vectorValue, ent->r.maxs );
	if ( ent->mass < 1 ) ent->mass = 1;
	if ( ent->mass > 100000 ) ent->mass = 100000;
	ent->elasticity = Com_Clamp( 0.0f, 1.0f, ent->elasticity );
	ent->friction = Com_Clamp( 0.0f, 4.0f, ent->friction );

	if ( ent->model && ent->model[0] ) {
		/* Radiant's standard model key overrides the archetype model. */
		ent->s.modelindex2 = G_ModelIndex( ent->model );
	}

	for ( value = 0; value < 3; value++ ) {
		if ( ent->r.mins[value] >= ent->r.maxs[value] ||
			ent->r.mins[value] < -4096.0f || ent->r.mins[value] > 4096.0f ||
			ent->r.maxs[value] < -4096.0f || ent->r.maxs[value] > 4096.0f ) {
			Com_Printf( "rally_scripted_object: invalid collision bounds; using a 32-unit box\n" );
			VectorSet( ent->r.mins, -16.0f, -16.0f, -16.0f );
			VectorSet( ent->r.maxs,  16.0f,  16.0f,  16.0f );
			break;
		}
	}
}

void SP_rally_scripted_object( gentity_t *ent ){
	/* Check if script file can be loaded and parsed */
	if ( !G_ParseScriptedObject( ent ) ){
		/* If there was a problem loading the script, remove this entity */
		G_FreeEntity(ent);
		return;
	}
	G_ApplyScriptedObjectMapProperties( ent );

	/* Set entity type for client-side rendering */
	ent->s.eType = ET_SCRIPTED;

	/* Static and dynamic props both participate in vehicle body traces. */
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;

	/* Set up entity callbacks */
	ent->die = G_ScriptedObject_Destroy;
	ent->touch = G_ScriptedObject_Touch;    /* Enable collision with vehicles */
	ent->pain = G_ScriptedObject_Pain;
	ent->think = ent->moveable ? G_ScriptedObject_Think : NULL;
	
	/* Static props remain asleep; dynamic props start on the next simulation step. */
	ent->nextthink = ent->moveable ? level.time + SCRIPTED_OBJECT_PHYSICS_STEP_MSEC : 0;
	ent->updateTime = level.time;
	ent->physicsAccumulatorMsec = 0;

	/* Initialize physics state */
	VectorClear( ent->netForce );
	VectorClear( ent->netMoment );
	VectorClear( ent->angularMomentum );

	/* Positions are authoritative on the server and interpolated from snapshots. */
	ent->s.pos.trType = TR_INTERPOLATE;
	ent->s.apos.trType = TR_INTERPOLATE;

	/* Initialize velocity tracking */
	VectorSet( ent->lastNonZeroVelocity, 0, 0, 0 );

	/* Optional: Set looping sound if defined in script */
	/*
	if (ent->preSoundLoop)
		ent->s.loopSound = ent->preSoundLoop;
	*/

	/* Drop object to ground level */
	DropToFloor(ent);

	/* Link entity into world */
	trap_LinkEntity (ent);
}
