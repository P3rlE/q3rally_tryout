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

#include "cg_local.h"
#include "../shared/rally_script_parser.h"
#define GIB_VELOCITY 250
#define GIB_JUMP 100

qboolean CG_ParseScriptedObject( centity_t *cent, const char *scriptName ){
	rallyScriptObjectDef_t config;
	char			text[RSP_MAX_SCRIPT_TEXT];
	char			filename[MAX_QPATH];
	char			model[MAX_QPATH];
	char			deadmodel[MAX_QPATH];
	char			*text_p;
	int			i;

	if (!scriptName || scriptName[0] == 0){
		Com_Printf("No Script file specified\n");
		return qfalse;
	}

	cent->numGibModels = 0;
	cent->gibsSpawned = qfalse;
	memset( cent->gibModels, 0, sizeof( cent->gibModels ) );
	memset( cent->gibSounds, 0, sizeof( cent->gibSounds ) );
	cent->hitSound = 0;
	cent->preSoundLoop = 0;
	cent->postSoundLoop = 0;
	cent->destroySound = 0;
	cent->modelHandle = 0;
	cent->deadModelHandle = 0;

	if ( !RSP_LoadScriptText( scriptName, text, sizeof( text ), filename, sizeof( filename ),
			trap_FS_FOpenFile, trap_FS_Read, trap_FS_FCloseFile, Com_Printf, cg_developer.integer ) ) {
		return qfalse;
	}

	if ( !RSP_ParseScriptedObject( text, filename, &config, Com_Printf ) ) {
		return qfalse;
	}

	model[0] = 0;
	deadmodel[0] = 0;
	if ( config.hasModel ) {
		Q_strncpyz( model, config.model, sizeof( model ) );
	}
	if ( config.hasDeadModel ) {
		Q_strncpyz( deadmodel, config.deadmodel, sizeof( deadmodel ) );
	}

	if ( config.hasHitSound ) {
		cent->hitSound = trap_S_RegisterSound( config.hitSound, qfalse );
	}
	if ( config.hasPreSound ) {
		cent->preSoundLoop = trap_S_RegisterSound( config.preSound, qfalse );
	}
	if ( config.hasPostSound ) {
		cent->postSoundLoop = trap_S_RegisterSound( config.postSound, qfalse );
	}
	if ( config.hasDestroySound ) {
		cent->destroySound = trap_S_RegisterSound( config.destroySound, qfalse );
	}

	for ( i = 0; i < config.numGibs && cent->numGibModels < MAX_SCRIPT_GIBS; i++ ) {
		if ( config.gibs[i].model[0] ) {
			int current = cent->numGibModels++;
			cent->gibModels[current] = trap_R_RegisterModel( config.gibs[i].model );
			if ( config.gibs[i].sound[0] ) {
				cent->gibSounds[current] = trap_S_RegisterSound( config.gibs[i].sound, qfalse );
			}
		}
	}

	if ( model[0] ) {
		text_p = text;
		if ( !RSP_SeekToSection( &text_p, model ) ) {
			cent->modelHandle = trap_R_RegisterModel( deadmodel );
		} else {
			Com_Printf( "Loading model info for '%s'\n", model );
			/* load model info */
		}
	}

	if ( deadmodel[0] ) {
		text_p = text;
		if ( !RSP_SeekToSection( &text_p, deadmodel ) ) {
			cent->deadModelHandle = trap_R_RegisterModel( deadmodel );
		} else {
			Com_Printf( "Loading deadmodel info for '%s'\n", deadmodel );
			/* load deadmodel info */
		}
	}

	return qtrue;
}
/*
void CG_ScriptedObject_Destroy( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ){
}


void CG_ScriptedObject_Touch ( gentity_t *self, gentity_t *other, trace_t *trace ){
}


void CG_ScriptedObject_Think ( gentity_t *self ){
	self->nextthink = cg.time + 100;
}


void CG_ScriptedObject_Pain ( gentity_t *self, gentity_t *attacker, int damage ){
}
*/


void CG_Scripted_Object( centity_t *cent ){
	refEntity_t			ent;
	entityState_t		*s1;
	const char			*scriptName;

	s1 = &cent->currentState;

//	CG_LogPrintf("Spawning a rally_scripted_object\n");

	// if no script file for it then return
	if (!s1->modelindex) {
		return;
	}

        if ( !cent->scriptLoadTime ){
                scriptName = CG_ConfigString( CS_SCRIPTS + s1->modelindex );
                if ( !scriptName[0] ) {
                        return;
                }

		if ( CG_ParseScriptedObject( cent, scriptName ) )
			cent->scriptLoadTime = cg.time;
		else
			return;
        }

       if ( (cent->currentState.eFlags & EF_DEAD) && !cent->gibsSpawned ) {
               int i;
               vec3_t velocity;

               cent->gibsSpawned = qtrue;

               for ( i = 0; i < cent->numGibModels; i++ ) {
                       velocity[0] = crandom() * GIB_VELOCITY;
                       velocity[1] = crandom() * GIB_VELOCITY;
                       velocity[2] = GIB_JUMP + crandom() * GIB_VELOCITY;
                       CG_LaunchGib( cent->lerpOrigin, velocity, cent->gibModels[i], -1, 0, qfalse );
                       if ( cent->gibSounds[i] ) {
                               trap_S_StartSound( cent->lerpOrigin, ENTITYNUM_WORLD, CHAN_AUTO, cent->gibSounds[i] );
                       }
               }
       }

        memset (&ent, 0, sizeof(ent));

	if ( cent->currentState.eFlags & EF_DEAD )
		ent.hModel = cent->deadModelHandle;
	else
		ent.hModel = cent->modelHandle;

	if ( !ent.hModel )
		return;

	// set frame
//	ent.oldframe = ent.frame;
//	ent.frame = s1->frame;
	ent.frame = ent.oldframe = 0;
	ent.backlerp = 0;

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	// convert angles to axis
	AnglesToAxis( cent->lerpAngles, ent.axis );

	// add to refresh list
	trap_R_AddRefEntityToScene (&ent);
}
