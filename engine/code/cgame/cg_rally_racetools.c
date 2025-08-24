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

static int      cg_mapBestLapTime;
static int      cg_mapBestScore;
static char     cg_mapBestPlayer[64];

static void CG_LoadMapRecord( void ) {
       fileHandle_t    f;
       char            filename[MAX_QPATH];
       char            buffer[256];
       int             len;
       char            mapname[MAX_QPATH];
       typedef struct {
               char    key[64];
               char    value[128];
       } record_t;
       record_t       records[16];
       int             count = 0;
       char            *line;
       int             i;
       const char      *val;

       cg_mapBestLapTime = 0;
       cg_mapBestScore = 0;
       Q_strncpyz( cg_mapBestPlayer, "Unknown", sizeof( cg_mapBestPlayer ) );

       COM_StripExtension( cgs.mapname, mapname, sizeof( mapname ) );
       Com_sprintf( filename, sizeof( filename ), "records/%s.record", mapname );

       len = trap_FS_FOpenFile( filename, &f, FS_READ );
       if ( len <= 0 ) {
               return;
       }
       if ( len >= sizeof( buffer ) ) {
               len = sizeof( buffer ) - 1;
       }
       trap_FS_Read( buffer, len, f );
       buffer[len] = '\0';
       trap_FS_FCloseFile( f );

       for ( line = strtok( buffer, "\n" ); line && count < ARRAY_LEN( records ); line = strtok( NULL, "\n" ) ) {
               char *eq = strchr( line, '=' );
               if ( !eq ) {
                       continue;
               }
               *eq = '\0';
               Q_strncpyz( records[count].key, line, sizeof( records[count].key ) );
               Q_strncpyz( records[count].value, eq + 1, sizeof( records[count].value ) );
               count++;
       }

       // new format: prefer player lap time, then score
       val = NULL;
       for ( i = 0; i < count; i++ ) {
               if ( !strcmp( records[i].key, "best_lap_time_player" ) ) {
                       cg_mapBestLapTime = atoi( records[i].value );
               } else if ( !strcmp( records[i].key, "player_best_lap_time_player" ) ) {
                       Q_strncpyz( cg_mapBestPlayer, records[i].value, sizeof( cg_mapBestPlayer ) );
               } else if ( !strcmp( records[i].key, "best_score" ) ) {
                       cg_mapBestScore = atoi( records[i].value );
               } else if ( !strcmp( records[i].key, "player_best_score" ) ) {
                       if ( !cg_mapBestLapTime ) {
                               Q_strncpyz( cg_mapBestPlayer, records[i].value, sizeof( cg_mapBestPlayer ) );
                       }
               }
       }

       if ( cg_mapBestLapTime ) {
               // ensure we have player name for lap time
               val = NULL;
               for ( i = 0; i < count; i++ ) {
                       if ( !strcmp( records[i].key, "player_best_lap_time_player" ) ) {
                               val = records[i].value;
                               break;
                       }
               }
               if ( val ) {
                       Q_strncpyz( cg_mapBestPlayer, val, sizeof( cg_mapBestPlayer ) );
               }
       } else if ( cg_mapBestScore ) {
               val = NULL;
               for ( i = 0; i < count; i++ ) {
                       if ( !strcmp( records[i].key, "player_best_score" ) ) {
                               val = records[i].value;
                               break;
                       }
               }
               if ( val ) {
                       Q_strncpyz( cg_mapBestPlayer, val, sizeof( cg_mapBestPlayer ) );
               }
       } else {
               // old format fallback
               const char *player = NULL;
               for ( i = 0; i < count; i++ ) {
                       if ( !strcmp( records[i].key, "best_lap_time" ) ) {
                               cg_mapBestLapTime = atoi( records[i].value );
                       } else if ( !strcmp( records[i].key, "best_score" ) ) {
                               cg_mapBestScore = atoi( records[i].value );
                       } else if ( !strcmp( records[i].key, "player" ) ) {
                               player = records[i].value;
                       }
               }
               if ( player ) {
                       Q_strncpyz( cg_mapBestPlayer, player, sizeof( cg_mapBestPlayer ) );
               }
       }
}

void CG_NewLapTime( int client, int lap, int time ) {
	centity_t	*cent;
	char		*t;

	cent = &cg_entities[client];

	if ((time - cent->startLapTime) < cent->bestLapTime || cent->bestLapTime == 0){
		// New bestlap
		cent->bestLapTime = (time - cent->startLapTime);
		cent->bestLap = cent->currentLap;

		t = getStringForTime( cent->bestLapTime );

		if ( client == cg.snap->ps.clientNum ) {
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

	if ((time - cent->startLapTime) < cent->bestLapTime || cent->bestLapTime == 0){
		// New bestlap
		cent->bestLapTime = (time - cent->startLapTime);
		cent->bestLap = cent->currentLap;

		t = getStringForTime( cent->bestLapTime );

		if ( client == cg.snap->ps.clientNum ) {
                        Com_Printf("You got a personal record lap time of %s!\n", t);
		}
	}

	cent->finishRaceTime = time;
}

void CG_StartRace( int time ) {
	int			i;
	centity_t	*player;

	CG_LoadMapRecord();

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

       if ( cg_mapBestLapTime || cg_mapBestScore ) {
               char    line[128];
               if ( cg_mapBestLapTime ) {
                       Com_sprintf( line, sizeof( line ), "Best Lap: %s by %s", getStringForTime( cg_mapBestLapTime ), cg_mapBestPlayer );
               } else {
                       Com_sprintf( line, sizeof( line ), "Best Score: %d by %s", cg_mapBestScore, cg_mapBestPlayer );
               }
               CG_DrawStringExt( 5, 5, line, colorWhite, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );
       }
}

void CG_RaceCountDown( const char *str, int secondsLeft ){
	cg.centerPrintTime = 0;
	cg.countDownEnd = cg.time + secondsLeft * 1000;
	Q_strncpyz( cg.countDownPrint, str, sizeof(cg.countDownPrint) );
}
