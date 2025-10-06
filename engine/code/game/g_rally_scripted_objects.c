/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2025 Q3Rally Team (Per Thormann - q3rally@gmail.com)

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
#include "../shared/rally_script_parser.h"

/* collision types */
#define		CT_BOX			0
#define		CT_CONE			1
#define		CT_CYLINDER		2

/* Global temporary force vector for testing */
vec3_t		tempForce;

qboolean G_ParseScriptedObject( gentity_t *ent ){
	rallyScriptObjectDef_t config;
	char			text[RSP_MAX_SCRIPT_TEXT];
	char			filename[MAX_QPATH];

	/* setup defaults */
	ent->takedamage = qfalse;
	VectorLength(ent->r.mins);
	VectorLength(ent->r.maxs);
	ent->elasticity = 0.1f;
	ent->mass = 100;
	ent->moveable = qfalse;
	ent->number = 0;

	if (!ent->script || ent->script[0] == 0){
		Com_Printf("No Script file specified\n");
		return qfalse;
	}

	/* for debugging only load one object */
	/* if( Q_stricmp( ent->script, "models/mapobjects/barrels/barrel01" ) ) */
	/*	return qfalse; */

	if ( !RSP_LoadScriptText( ent->script, text, sizeof( text ), filename, sizeof( filename ),
			trap_FS_FOpenFile, trap_FS_Read, trap_FS_FCloseFile, Com_Printf, g_developer.integer ) ) {
		return qfalse;
	}

	if ( !RSP_ParseScriptedObject( text, filename, &config, Com_Printf ) ) {
		return qfalse;
	}

	/* send script file name in CS so we dont need */
	/* to do all of the drawing and stuff server side. */
	ent->s.modelindex = G_ScriptIndex( ent->script );

	if ( config.hasType ) {
		ent->s.weapon = config.type;
	}

	if ( config.hasMoveable ) {
		ent->moveable = config.moveable;
	}

	if ( config.hasElasticity ) {
		ent->elasticity = config.elasticity;
	}

	if ( config.hasMass ) {
		ent->mass = config.mass;
		if ( ent->mass <= 0 )
			ent->mass = 100;
	}

	if ( config.hasHealth ) {
		ent->maxHealth = ent->health = config.health;
		if ( ent->health >= 0 )
			ent->takedamage = qtrue;
	}

	if ( config.hasMins ) {
		VectorCopy( config.mins, ent->r.mins );
	}

	if ( config.hasMaxs ) {
		VectorCopy( config.maxs, ent->r.maxs );
	}

	if ( g_developer.integer )
		Com_Printf("Successfully parsed script file\n");

	return qtrue;
}
void G_ScriptedObject_Destroy( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ){
	Com_Printf("Destroying scripted map object %s\n", self->classname);

	self->s.eFlags |= EF_DEAD;
}

void G_ScriptedObject_Touch ( gentity_t *self, gentity_t *other, trace_t *trace ){
	vec3_t		dir;
	float		dot;

	/* Invert collision normal */
	VectorInverse( trace->plane.normal );

	/* Calculate relative velocity */
	VectorMA( self->s.pos.trDelta, -1, other->s.pos.trDelta, dir );

	/* Apply collision response with elasticity */
	dot = DotProduct( trace->plane.normal, dir );
	VectorMA( dir, -dot * ( 1.0f + self->elasticity ), trace->plane.normal, dir );
	VectorAdd( other->s.pos.trDelta, dir, self->s.pos.trDelta );

        /* Optional: Apply advanced collision physics */
        /* G_RallyObject_ApplyCollision( self, trace->endpos, trace->plane.normal, self->elasticity ); */
}

void G_ScriptedObject_Think ( gentity_t *self ){
	float time = ( level.time - self->updateTime ) / 1000.0f;

	/* Apply gravity as default force */
	VectorSet( self->netForce, 0, 0, -CP_CURRENT_GRAVITY * self->mass );
	VectorClear( self->netMoment );

	/* Optional: Add random impulse force every 5 seconds for testing */
	/*
	if( level.time % 5000 < 1000 )
	{
		if( tempForce[0] == 0 && tempForce[1] == 0 && tempForce[2] == 0 )
		{
			tempForce[0] = crandom();
			tempForce[1] = crandom();
			tempForce[2] = random() * 1.5f;
		}
		VectorMA( self->netForce, CP_CURRENT_GRAVITY * self->mass, tempForce, self->netForce );
	}
	else
	{
		VectorClear( tempForce );
	}
	*/

	/* Only run physics simulation if object is moveable */
	if( self->moveable )
	{
                /* Check for collisions and apply collision response */
                G_RallyObject_TracePhysics( self, 0.050f );

                /* Integrate physics - update position and rotation */
                G_RallyObject_IntegratePhysics( self, 0.050f );
	}

	/* Update entity position and angles for rendering */
	VectorCopy( self->s.pos.trBase, self->r.currentOrigin );
	VectorCopy( self->s.pos.trBase, self->s.origin );
	VectorCopy( self->s.apos.trBase, self->r.currentAngles );
	VectorCopy( self->s.apos.trBase, self->s.angles );
	
	/* Link entity into world for collision detection */
	trap_LinkEntity( self );

	/* Schedule next physics update in 50ms */
	self->nextthink = level.time + 50;
	self->updateTime = level.time;
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

void SP_rally_scripted_object( gentity_t *ent ){
	/* Check if script file can be loaded and parsed */
	if ( !G_ParseScriptedObject( ent ) ){
		/* If there was a problem loading the script, remove this entity */
		G_FreeEntity(ent);
		return;
	}

	/* Set entity type for client-side rendering */
	ent->s.eType = ET_SCRIPTED;

	/* Set collision properties - non-moveable objects block movement */
	if (!ent->moveable)
		ent->r.contents = CONTENTS_BODY;

	/* Set up entity callbacks */
	ent->die = G_ScriptedObject_Destroy;
	ent->touch = G_ScriptedObject_Touch;    /* Enable collision with vehicles */
	ent->pain = G_ScriptedObject_Pain;
	ent->think = G_ScriptedObject_Think;
	
	/* Schedule first think in 1 second */
	ent->nextthink = level.time + 1000;
	ent->updateTime = ent->nextthink;

	/* Initialize physics state */
	VectorClear( ent->netForce );
	VectorClear( ent->netMoment );
	VectorClear( ent->angularMomentum );

	/* Set trajectory types for interpolation */
	ent->s.pos.trType = TR_LINEAR;
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

