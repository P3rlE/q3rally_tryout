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

static qboolean CG_SelectGhostFrames( int targetOffset, ghostFrame_t **previous, ghostFrame_t **next, float *lerp ) {
	ghostRecording_t *recording = &cg.ghostPlayback;
	int i;

	if ( !recording->valid || recording->frameCount <= 0 ) {
		return qfalse;
	}

	*previous = &recording->frames[recording->startIndex];
	*next = *previous;
	*lerp = 0.0f;

	if ( targetOffset <= (*previous)->timeOffset ) {
		return qtrue;
	}

	for ( i = 1; i < recording->frameCount; i++ ) {
		int index = ( recording->startIndex + i ) % MAX_GHOST_FRAMES;
		ghostFrame_t *candidate = &recording->frames[index];

		if ( targetOffset <= candidate->timeOffset ) {
			*next = candidate;
			if ( candidate->timeOffset != (*previous)->timeOffset ) {
				*lerp = (float)( targetOffset - (*previous)->timeOffset ) /
					(float)( candidate->timeOffset - (*previous)->timeOffset );
			}
			return qtrue;
		}

		*previous = candidate;
	}

	return qtrue;
}

void CG_BeginGhostRecording( int startTime ) {
	memset( &cg.ghostRecording, 0, sizeof( cg.ghostRecording ) );
	cg.ghostRecording.startIndex = 0;
	cg.ghostRecording.writeIndex = 0;
	cg.ghostRecording.frameCount = 0;
	cg.ghostRecording.duration = 0;
	cg.ghostRecording.valid = qfalse;

	cg.ghostRecordingActive = qtrue;
	cg.ghostRecordingStartTime = startTime;
}

void CG_EndGhostRecording( int finishTime ) {
	if ( !cg.ghostRecordingActive ) {
		return;
	}

	cg.ghostRecordingActive = qfalse;

	if ( cg.ghostRecording.frameCount > 1 ) {
		int duration = finishTime > cg.ghostRecordingStartTime
			? finishTime - cg.ghostRecordingStartTime
			: cg.ghostRecording.duration;

		cg.ghostRecording.duration = duration;
		cg.ghostRecording.valid = qtrue;
		cg.ghostPlayback = cg.ghostRecording;
	}
}

void CG_RecordGhostFrame( void ) {
	usercmd_t cmd;
	ghostFrame_t *frame;

	if ( !cg.ghostRecordingActive ) {
		return;
	}

	if ( !( isRallyRace() || cgs.gametype == GT_DERBY || cgs.gametype == GT_LCS ) ) {
		return;
	}

	if ( !cg.snap || cg.snap->ps.clientNum >= MAX_CLIENTS ) {
		return;
	}

	if ( cg_entities[cg.snap->ps.clientNum].finishRaceTime ) {
		return;
	}

	if ( cg.time < cg.ghostRecordingStartTime ) {
		return;
	}

	frame = &cg.ghostRecording.frames[cg.ghostRecording.writeIndex];

	frame->timeOffset = cg.time - cg.ghostRecordingStartTime;
	VectorCopy( cg.predictedPlayerState.origin, frame->origin );
	VectorCopy( cg.predictedPlayerState.viewangles, frame->angles );
	VectorCopy( cg.predictedPlayerState.velocity, frame->velocity );

	trap_GetUserCmd( trap_GetCurrentCmdNumber(), &cmd );
	frame->buttons = cmd.buttons;
	frame->forwardmove = cmd.forwardmove;
	frame->upmove = cmd.upmove;

	cg.ghostRecording.writeIndex = ( cg.ghostRecording.writeIndex + 1 ) % MAX_GHOST_FRAMES;
	if ( cg.ghostRecording.frameCount < MAX_GHOST_FRAMES ) {
		cg.ghostRecording.frameCount++;
	} else {
		cg.ghostRecording.startIndex = cg.ghostRecording.writeIndex;
	}

	cg.ghostRecording.duration = frame->timeOffset;
	cg.ghostRecording.valid = cg.ghostRecording.frameCount > 1;
}

void CG_AddGhostEntity( void ) {
	ghostFrame_t *from, *to;
	float lerp;
	int offset;
	refEntity_t ghost;
	clientInfo_t *ci;
	vec3_t origin;
	vec3_t angles;
	int i;

	if ( !cg_ghostPlayback.integer ) {
		return;
	}

	if ( !( isRallyRace() || cgs.gametype == GT_DERBY || cgs.gametype == GT_LCS ) ) {
		return;
	}

	if ( !cg.ghostPlayback.valid || cg.ghostPlayback.frameCount <= 0 ) {
		return;
	}

	if ( !cg.snap || cg.snap->ps.clientNum >= MAX_CLIENTS ) {
		return;
	}

	if ( !cg_entities[cg.snap->ps.clientNum].startRaceTime ) {
		return;
	}

	offset = cg.time - cg_entities[cg.snap->ps.clientNum].startRaceTime;
	if ( offset < 0 ) {
		return;
	}

	if ( !CG_SelectGhostFrames( offset, &from, &to, &lerp ) ) {
		return;
	}

	ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	if ( !ci->bodyModel ) {
		return;
	}

	for ( i = 0; i < 3; i++ ) {
		origin[i] = from->origin[i] + lerp * ( to->origin[i] - from->origin[i] );
		angles[i] = from->angles[i] + lerp * AngleSubtract( to->angles[i], from->angles[i] );
	}

	memset( &ghost, 0, sizeof( ghost ) );
	ghost.hModel = ci->bodyModel;
	ghost.customSkin = ci->bodySkin;
	VectorCopy( origin, ghost.origin );
	VectorCopy( origin, ghost.lightingOrigin );
	ghost.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	AnglesToAxis( angles, ghost.axis );
	ghost.shaderRGBA[0] = 255;
	ghost.shaderRGBA[1] = 255;
	ghost.shaderRGBA[2] = 255;
	ghost.shaderRGBA[3] = 160;

	trap_R_AddRefEntityToScene( &ghost );
}


void CG_NewLapTime( int client, int lap, int time ) {
	centity_t	*cent;
	char		*t;

	cent = &cg_entities[client];

	if ((time - cent->startLapTime) < cent->bestLapTime || cent->bestLapTime == 0){
		// New bestlap
		cent->bestLapTime = (time - cent->startLapTime);
		cent->bestLap = cent->currentLap;

		if ( client == cg.snap->ps.clientNum ) {
			t = getStringForTime( cent->bestLapTime );

			Com_Printf("You got a personal record lap time of %s!\n", t);
		}
	}

	cent->currentLap = lap;
	cent->lastStartLapTime = cent->startLapTime;
	cent->startLapTime = time;
}

void CG_FinishedRace( int client, int time ) {
	centity_t	*cent;
	char		*t;

	cent = &cg_entities[client];

	if ( client == cg.snap->ps.clientNum
		&& ((time - cent->startLapTime) < cent->bestLapTime || cent->bestLapTime == 0) ){
		// New bestlap
		cent->bestLapTime = (time - cent->startLapTime);
		cent->bestLap = cent->currentLap;

		t = getStringForTime( cent->bestLapTime );

		Com_Printf("You got a personal record lap time of %s!\n", t);
	}

        cent->finishRaceTime = time;

        if ( client == cg.snap->ps.clientNum ) {
                CG_EndGhostRecording( time );
        }

        if ( cgs.gametype == GT_ELIMINATION || cgs.gametype == GT_LCS ) {
		int lastClient;
		int remaining;

		remaining = CG_GetPlayersRemaining( &lastClient );
		CG_CheckEliminationWarning( remaining );
	}
}

void CG_StartRace( int time ) {
	int			i;
	centity_t	*player;

        for (i = 0; i < MAX_CLIENTS; i++){
                player = &cg_entities[i];
                if (!player) continue;

		if (!player->startRaceTime){
			player->startRaceTime = time;
			player->finishRaceTime = 0;
			player->startLapTime = time;
			player->currentLap = 1;
			player->bestLapTime = 0;
			player->lastStartLapTime = 0;
                }
        }

        CG_BeginGhostRecording( time );

        cg.eliminationWarningActive = qfalse;
        cg.eliminationWarningTime = 0;
        cg.eliminationPlayersRemaining = CG_GetPlayersRemaining( NULL );
}

void CG_DrawRaceCountDown( void ){
	float	f, scale;
	int		x, y, w, h;
	vec4_t	color;

	if (cg.countDownEnd + 1000 < cg.time || cg.countDownPrint[0] == 0)
		return;

	f = cg.countDownEnd < cg.time ? 0.0f : (cg.countDownEnd - cg.time) / 3000.0f;

	color[0] = 1.0f * f;
	color[1] = 1.0f * (1-f);
	color[2] = 0;
	color[3] = 1.0f;

	scale = cg.countDownEnd < cg.time ? 0.8f : ((cg.countDownEnd - cg.time) % 1000) / 1000.0f;
	w = 3*GIANTCHAR_WIDTH * scale;
	h = 3*GIANTCHAR_HEIGHT * scale;
	x = 320 - (strlen(cg.countDownPrint) * w) / 2;
	y = 240 - h/2;
	CG_DrawStringExt( x, y, cg.countDownPrint, color, qfalse, qtrue, w, h, 0 );
}

void CG_RaceCountDown( const char *str, int secondsLeft ){
	cg.centerPrintTime = 0;
	cg.countDownEnd = cg.time + secondsLeft * 1000;
	Q_strncpyz( cg.countDownPrint, str, sizeof(cg.countDownPrint) );
}
