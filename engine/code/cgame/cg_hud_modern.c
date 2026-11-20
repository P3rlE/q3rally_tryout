/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The in-game HUD of v0.9, in the look of the menus: panels in the corners,
the centre of the screen kept free (design: tools/ui_webstyle/hud_design).
It takes over a game mode at a time; the others keep the old HUD.
cg_hudStyle 0 brings the old one back everywhere.

The layout (cg_hud_modernlayout.c) works in design pixels, 900 to the
screen height; this file scales them to the screen and fills in the game.
*/

#include "cg_local.h"
#include "cg_hud_elements.h"
#include "cg_hud_modernlayout.h"

#define MH_MINIMAP			176.0f	/* the design's minimap at the default cg_mmap_size */
#define MH_MINIMAP_DEFAULT	1.0f	/* ui_main.c registers cg_mmap_size first, at 1.0 */
#define MH_LOW_INTEGRITY	0.30f	/* from here down the integrity turns orange */

static const vec4_t mhPanel     = { 13 / 255.0f, 13 / 255.0f, 13 / 255.0f, 0.82f };
static const vec4_t mhLine      = { 55 / 255.0f, 55 / 255.0f, 55 / 255.0f, 1.0f };
static const vec4_t mhAccent    = { 1.0f, 85 / 255.0f, 0.0f, 1.0f };
static const vec4_t mhFast      = { 90 / 255.0f, 176 / 255.0f, 1.0f, 1.0f };	/* #5ab0ff: ahead, slipstream */
static const vec4_t mhWhite     = { 1.0f, 1.0f, 1.0f, 1.0f };
static const vec4_t mhLabel     = { 187 / 255.0f, 187 / 255.0f, 187 / 255.0f, 1.0f };	/* #bbb */
static const vec4_t mhMuted     = { 153 / 255.0f, 153 / 255.0f, 153 / 255.0f, 1.0f };	/* #999 */
static const vec4_t mhValue     = { 221 / 255.0f, 221 / 255.0f, 221 / 255.0f, 1.0f };	/* #ddd */
static const vec4_t mhSoft      = { 230 / 255.0f, 230 / 255.0f, 230 / 255.0f, 1.0f };	/* #e6e6e6 */
static const vec4_t mhLit       = { 242 / 255.0f, 242 / 255.0f, 242 / 255.0f, 1.0f };	/* #f2f2f2 */
static const vec4_t mhTrack     = { 44 / 255.0f, 44 / 255.0f, 44 / 255.0f, 1.0f };	/* #2c2c2c */
static const vec4_t mhDarkTrack = { 30 / 255.0f, 30 / 255.0f, 30 / 255.0f, 1.0f };	/* #1e1e1e */
static const vec4_t mhMe        = { 1.0f, 85 / 255.0f, 0.0f, 0.18f };
static const vec4_t mhGlass     = { 1.0f, 1.0f, 1.0f, 0.35f };
#define MH_TICKS_MAX 12
static const vec4_t mhTeam[MH_TEAMS_MAX] = {	/* the design's red, blue, green, yellow */
	{ 244 / 255.0f, 61 / 255.0f, 56 / 255.0f, 1.0f }, { 71 / 255.0f, 143 / 255.0f, 1.0f, 1.0f },
	{ 46 / 255.0f, 224 / 255.0f, 82 / 255.0f, 1.0f }, { 1.0f, 209 / 255.0f, 41 / 255.0f, 1.0f }
};
static const vec4_t mhOnTeam    = { 17 / 255.0f, 17 / 255.0f, 17 / 255.0f, 1.0f };	/* #111 on a team's colour */
static const vec4_t mhTickOpen  = { 90 / 255.0f, 90 / 255.0f, 90 / 255.0f, 1.0f };	/* #5a5a5a */

static struct {
	int				time;			/* the frame this was laid out for */
	qboolean		ready;
	mhRaceLayout_t	l;
	char			title[MH_TEXT_MAX], pos[16], of[16], lap[16];
	char			progressLeft[24], progressRight[16];
	float			progressFrac;
	int				progressDone;
	char			kvKey[MH_KV_MAX][16], kvValue[MH_KV_MAX][32];
	const float		*kvColor[MH_KV_MAX];
	char			rowPos[MH_ROWS_MAX][8], rowName[MH_ROWS_MAX][40], rowValue[MH_ROWS_MAX][16];
	qboolean		rowMe[MH_ROWS_MAX];
	const float		*rowColor[MH_ROWS_MAX];		/* NULL: #ddd */
	const float		*posColor;
	int				rowTeam[MH_ROWS_MAX];			/* TEAM_RED.., for the swatch */
	char			teamValue[MH_TEAMS_MAX][16];
	cgHudToastView_t	toast;
	cgJukeboxView_t	jukebox;
	char			integrity[16], armor[16];
	char			slotName[MH_SLOTS_MAX][32];
	qhandle_t		slotIcon[MH_SLOTS_MAX];
	float			integrityFrac;
	char			slip[16];
	float			slipFrac, slipAlpha;
	char			speed[16], gear[8], nos[8], fuel[8];
	float			rpmFrac, nosFrac, fuelFrac;
	qhandle_t		note, prev, play, next, chevron, message;
} mh;

static float mhScale;

/* -------------------------------------------------------------------------
   drawing in design pixels
   ------------------------------------------------------------------------- */

static float MH_Snap( float v ) {
	return WS_Round( v * mhScale );
}

static void MH_Fill( float x, float y, float w, float h, const float *color, float alpha ) {
	vec4_t c;
	float x0 = MH_Snap( x ), y0 = MH_Snap( y ), x1 = MH_Snap( x + w ), y1 = MH_Snap( y + h );
	if ( x1 <= x0 || y1 <= y0 ) return;
	Vector4Copy( color, c );
	c[3] *= alpha;
	WS_FillRect( x0, y0, x1 - x0, y1 - y0, c );
}

static void MH_FillRect( const wsRect_t *r, const float *color, float alpha ) {
	MH_Fill( r->x, r->y, r->w, r->h, color, alpha );
}

/* a 1px border (at least one screen pixel), the top one thicker if asked */
static void MH_Border( const wsRect_t *r, const float *color, float top, const float *topColor, float alpha ) {
	float x0 = MH_Snap( r->x ), y0 = MH_Snap( r->y ), x1 = MH_Snap( r->x + r->w ), y1 = MH_Snap( r->y + r->h );
	float t = mhScale >= 1.5f ? WS_Round( mhScale ) : 1.0f;
	float tt = WS_Round( top * mhScale );
	vec4_t c, ct;
	if ( tt < t ) tt = t;
	Vector4Copy( color, c );
	c[3] *= alpha;
	Vector4Copy( topColor ? topColor : color, ct );
	ct[3] *= alpha;
	WS_FillRect( x0, y0, x1 - x0, tt, ct );
	WS_FillRect( x0, y1 - t, x1 - x0, t, c );
	WS_FillRect( x0, y0 + tt, t, y1 - y0 - tt - t, c );
	WS_FillRect( x1 - t, y0 + tt, t, y1 - y0 - tt - t, c );
}

/* panel(): the dark glass, a 1px line, a 2px accent at the top */
static void MH_Panel( const wsRect_t *r, qboolean accent, float alpha ) {
	MH_FillRect( r, mhPanel, alpha );
	MH_Border( r, mhLine, accent ? 2.0f : 1.0f, accent ? mhAccent : NULL, alpha );
}

static void MH_Str( const mhText_t *t, const float *color, float alpha ) {
	wsTextStyle_t style = t->style;
	vec4_t c;
	if ( !t->text[0] ) return;
	style.size *= mhScale;
	style.spacing *= mhScale;
	style.lineHeight *= mhScale;
	Vector4Copy( color, c );
	c[3] *= alpha;
	WS_DrawText( &style, t->box.x * mhScale, t->box.y * mhScale, t->text, strlen( t->text ), c );
}

static void MH_Bar( const wsRect_t *r, float frac, const float *fill, const float *track, float alpha ) {
	frac = WS_Clamp( 0.0f, frac, 1.0f );
	MH_FillRect( r, track, alpha );
	if ( frac > 0.0f ) MH_Fill( r->x, r->y, r->w * frac, r->h, fill, alpha );
}

static void MH_Pic( const wsRect_t *r, float inset, qhandle_t shader, const float *color, float alpha ) {
	vec4_t c;
	float x0, y0, x1, y1;
	if ( !shader ) return;
	x0 = MH_Snap( r->x + inset );
	y0 = MH_Snap( r->y + inset );
	x1 = MH_Snap( r->x + r->w - inset );
	y1 = MH_Snap( r->y + r->h - inset );
	Vector4Copy( color, c );
	c[3] *= alpha;
	trap_R_SetColor( c );
	trap_R_DrawStretchPic( x0, y0, x1 - x0, y1 - y0, 0, 0, 1, 1, shader );
	trap_R_SetColor( NULL );
}

/* -------------------------------------------------------------------------
   the game's figures
   ------------------------------------------------------------------------- */

static void MH_Time( int ms, char *out, int size ) {
	if ( ms <= 0 ) {
		Q_strncpyz( out, "-:--.--", size );
		return;
	}
	Com_sprintf( out, size, "%d:%02d.%02d", ms / 60000, ( ms / 1000 ) % 60, ( ms % 1000 ) / 10 );
}

static void MH_Gap( int ms, char *out, int size ) {
	int a = ms < 0 ? -ms : ms;
	Com_sprintf( out, size, "%c%d.%02d", ms < 0 ? '-' : '+', a / 1000, ( a % 1000 ) / 10 );
}

/* without the ^colour codes, keeping Latin-1 (Q_CleanStr drops it) */
static void MH_Plain( const char *in, char *out, int size ) {
	int n = 0;
	for ( ; *in && n < size - 1; in++ ) {
		if ( Q_IsColorString( in ) ) {
			in++;
			continue;
		}
		if ( (unsigned char)*in < ' ' ) continue;
		out[n++] = *in;
	}
	out[n] = '\0';
}

/* who is where in the race (the old race order's rules for a plain race) */
static int MH_RaceOrder( int at[MAX_CLIENTS + 1], int *local ) {
	int i, position, last = 0;
	for ( i = 0; i <= MAX_CLIENTS; i++ ) at[i] = -1;
	*local = 0;
	for ( i = 0; i < cgs.maxclients && i < MAX_CLIENTS; i++ ) {
		if ( !cgs.clientinfo[i].infoValid || cgs.clientinfo[i].team == TEAM_SPECTATOR ) continue;
		position = cg_entities[i].currentPosition;
		if ( position <= 0 ) position = cgs.clientinfo[i].position;
		if ( position <= 0 || position > MAX_CLIENTS || at[position] >= 0 ) continue;
		at[position] = i;
		if ( position > last ) last = position;
		if ( i == cg.snap->ps.clientNum ) *local = position;
	}
	return last;
}

qboolean CG_ModernHUD_Active( void ) {
	return cg_hudStyle.integer && cg.snap &&
	       ( cgs.gametype == GT_RACING || cgs.gametype == GT_SPRINT || cgs.gametype == GT_GHOST ||
	         cgs.gametype == GT_TEAM_RACING );
}

/* a driver's team: our own from the snapshot, the freshest */
static int MH_Team( int client ) {
	if ( client == cg.snap->ps.clientNum ) return cg.snap->ps.persistant[PERS_TEAM];
	return cgs.clientinfo[client].team;
}

static const float *MH_TeamColor( int team ) {
	return team >= TEAM_RED && team < TEAM_RED + MH_TEAMS_MAX ? mhTeam[team - TEAM_RED] : mhMuted;
}

/* the checkpoints of the course: numbered 1..n (start and finish are 0) */
static int MH_Checkpoints( void ) {
	int i, n = 0;
	for ( i = 0; i < cg.snap->numEntities; i++ ) {
		const entityState_t *es = &cg.snap->entities[i];
		if ( es->eType == ET_CHECKPOINT && es->weapon > n ) n = es->weapon;
	}
	return n;
}

/* 1252 -> "1,252" */
static void MH_Thousands( int value, char *out, int size ) {
	if ( value >= 1000 ) Com_sprintf( out, size, "%d,%03d", value / 1000, value % 1000 );
	else Com_sprintf( out, size, "%d", value );
}

/* the ghost race: who the ghost is and its lap, or what the game is doing to get one */
static void MH_GhostLine( char *out, int size ) {
	char name[40], plain[40];
	const char *status;
	qboolean error;
	int lapMs;

	if ( CG_GhostRace_Opponent( name, sizeof( name ), &lapMs ) ) {
		char lapText[16];
		MH_Time( lapMs, lapText, sizeof( lapText ) );
		if ( CG_GhostPlaybackMode() == 1 ) Q_strncpyz( plain, "PERSONAL", sizeof( plain ) );
		else if ( CG_GhostPlaybackMode() == 2 ) Q_strncpyz( plain, "SERVER", sizeof( plain ) );
		else {
			MH_Plain( name, plain, 17 );	/* a ladder name: the row has room for sixteen */
			Q_strupr( plain );
		}
		Com_sprintf( out, size, "%s \xb7 %s", plain, lapText );
		return;
	}
	switch ( CG_GhostPlaybackMode() ) {
	case 1:
		CG_LoadPersonalGhost();
		status = cg.personalGhostSearchValid && !cg.personalGhostSearchFound ? "NO GHOST FOR CAR" : "SEARCHING";
		break;
	case 2:
		if ( cg.baseGhostTransferFailed ) status = "TRANSFER FAILED";
		else if ( cg.baseGhostTransferPending ) status = "RECEIVING";
		else if ( cg.baseGhostStatusKnown ) status = "NO SERVER GHOST";
		else status = "WAITING";
		break;
	case 3:
		status = CG_LadderGhost_StatusText( &error );
		if ( !Q_stricmpn( status, "VS ", 3 ) ) status += 3;
		break;
	default:
		status = "OFF";
		break;
	}
	Q_strncpyz( out, status, size );
}

/* the ghost race: this lap's splits (the time of each sector, how it went
 * against the ghost) and the sector we are in */
static void MH_GhostSplits( mhRaceLayout_t *l, const centity_t *cent, const playerState_t *ps ) {
	int n = cg.ghostSplitLapStart == cent->startLapTime ? cg.ghostSplitCount : 0;
	qboolean running = cent->startLapTime && !cent->finishRaceTime && cg.time >= cent->startLapTime;
	int shown = running ? MH_ROWS_MAX - 1 : MH_ROWS_MAX;
	int i, row = 0;

	l->orderTitle = "Splits";
	l->orderRight = "vs ghost";
	for ( i = n > shown ? n - shown : 0; i < n; i++, row++ ) {
		int sector = cg.ghostSplitDeltaAt[i] - ( i > 0 ? cg.ghostSplitDeltaAt[i - 1] : 0 );
		Com_sprintf( mh.rowPos[row], sizeof( mh.rowPos[0] ), "S%d", cg.ghostSplitCheckpoint[i] );
		MH_Time( cg.ghostSplitElapsed[i] - ( i > 0 ? cg.ghostSplitElapsed[i - 1] : 0 ), mh.rowName[row], sizeof( mh.rowName[0] ) );
		MH_Gap( sector, mh.rowValue[row], sizeof( mh.rowValue[0] ) );
		mh.rowColor[row] = sector <= 0 ? mhFast : mhAccent;
		mh.rowMe[row] = qfalse;
	}
	if ( running ) {
		int next = ps->stats[STAT_NEXT_CHECKPOINT];
		Com_sprintf( mh.rowPos[row], sizeof( mh.rowPos[0] ), "S%d", next > 0 ? next : 1 );
		MH_Time( cg.time - cent->startLapTime - ( n > 0 ? cg.ghostSplitElapsed[n - 1] : 0 ), mh.rowName[row], sizeof( mh.rowName[0] ) );
		Q_strncpyz( mh.rowValue[row], "--", sizeof( mh.rowValue[0] ) );
		mh.rowColor[row] = NULL;
		mh.rowMe[row++] = qtrue;
	}
	for ( i = 0; i < row; i++ ) {
		l->rowPos[i] = mh.rowPos[i];
		l->rowName[i] = mh.rowName[i];
		l->rowValue[i] = mh.rowValue[i];
	}
	l->rowCount = row;
}

static void MH_Register( void ) {
	if ( mh.note ) return;
	mh.note = trap_R_RegisterShaderNoMip( "menu/art/hud_note" );
	mh.prev = trap_R_RegisterShaderNoMip( "menu/art/hud_prev" );
	mh.play = trap_R_RegisterShaderNoMip( "menu/art/hud_play" );
	mh.next = trap_R_RegisterShaderNoMip( "menu/art/hud_next" );
	mh.chevron = trap_R_RegisterShaderNoMip( "menu/art/hud_chevron" );
	mh.message = trap_R_RegisterShaderNoMip( "menu/art/hud_message" );
}

static qboolean MH_Views( void ) {
	return cg.snap->ps.pm_type != PM_INTERMISSION &&
	       cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR &&
	       !CG_IntroCam_IsActive();
}

/* everything the frame shows, laid out once */
static void MH_Prepare( void ) {
	mhRaceLayout_t *l = &mh.l;
	playerState_t *ps = &cg.snap->ps;
	centity_t *cent = &cg_entities[ps->clientNum];
	int at[MAX_CLIENTS + 1], local, last, i, first, count, gap, lap, lapTime, racers, turbo, maxHealth;
	const char *map;
	vec3_t forward;
	float factor;

	if ( mh.ready && mh.time == cg.time ) return;
	mh.time = cg.time;
	mh.ready = qtrue;
	MH_Register();
	mhScale = cgs.glconfig.vidHeight / MH_DESIGN_HEIGHT;
	memset( l, 0, sizeof( *l ) );
	l->viewWidth = cgs.glconfig.vidWidth / mhScale;

	last = MH_RaceOrder( at, &local );
	racers = cgs.numRacers > 0 ? cgs.numRacers : ( last > 0 ? last : 1 );

	/* the mode card */
	map = CG_ConfigString( CS_MESSAGE );
	if ( !map || !map[0] ) map = COM_SkipPath( cgs.mapname );
	MH_Plain( map, mh.rowName[0], sizeof( mh.rowName[0] ) );
	Com_sprintf( mh.title, sizeof( mh.title ), "%s \xb7 %s",
	             cgs.gametype == GT_GHOST ? "Ghost race" : cgs.gametype == GT_SPRINT ? "Sprint" :
	             cgs.gametype == GT_TEAM_RACING ? "Team race" : "Race", mh.rowName[0] );
	l->title = mh.title;
	mh.posColor = mhWhite;
	if ( cgs.gametype == GT_GHOST ) {
		/* alone on the track: the gap to the ghost at the last checkpoint */
		if ( CG_GhostPlaybackMode() && cg.ghostSplitDeltaValid ) {
			MH_Gap( cg.ghostSplitDeltaMs, mh.pos, sizeof( mh.pos ) );
			mh.posColor = cg.ghostSplitDeltaMs <= 0 ? mhFast : mhAccent;
		} else {
			Q_strncpyz( mh.pos, "--", sizeof( mh.pos ) );
		}
		l->posLabel = "VS GHOST";
		l->pos = mh.pos;
		l->of = "";
	} else if ( cg_hudShowPosition.integer ) {
		if ( cent->currentPosition > 0 ) Com_sprintf( mh.pos, sizeof( mh.pos ), "P%d", cent->currentPosition );
		else Q_strncpyz( mh.pos, "P-", sizeof( mh.pos ) );
		Com_sprintf( mh.of, sizeof( mh.of ), "/ %d", racers );
		l->pos = mh.pos;
		l->of = mh.of;
	}
	if ( CG_IsSprintTrack() ) {
		/* a sprint: the way to the finish instead of the lap */
		float total = cgs.trackLength * CG_RaceLapLimit();
		int remain = ps->stats[STAT_DISTANCE_REMAIN], ticks = MH_Checkpoints(), next = ps->stats[STAT_NEXT_CHECKPOINT];
		char metres[16];
		mh.progressFrac = 0.0f;
		if ( cent->finishRaceTime ) mh.progressFrac = 1.0f;
		else if ( total > 0.0f && remain > 0 ) mh.progressFrac = WS_Clamp( 0.0f, 1.0f - remain / total, 1.0f );
		if ( cent->finishRaceTime ) remain = 0;
		MH_Thousands( remain > 0 ? remain : 0, metres, sizeof( metres ) );
		Com_sprintf( mh.lap, sizeof( mh.lap ), "%s m", metres );
		l->lapLabel = "TO GO";
		l->lapValue = mh.lap;
		if ( total > 0.0f ) {
			mh.progressDone = cent->finishRaceTime || next > ticks ? ticks : ( next > 1 ? next - 1 : 0 );
			Com_sprintf( mh.progressLeft, sizeof( mh.progressLeft ), "CP %d / %d", mh.progressDone, ticks );
			Com_sprintf( mh.progressRight, sizeof( mh.progressRight ), "%d%%", (int)( mh.progressFrac * 100.0f ) );
			l->progress = qtrue;
			l->progressTicks = ticks;
			l->progressLeft = mh.progressLeft;
			l->progressRight = mh.progressRight;
		}
	} else if ( cg_hudShowLaps.integer ) {
		lap = cent->currentLap > 0 ? cent->currentLap : 1;
		if ( cgs.laplimit > 1 ) Com_sprintf( mh.lap, sizeof( mh.lap ), "%d/%d", lap, cgs.laplimit );
		else Com_sprintf( mh.lap, sizeof( mh.lap ), "%d", lap );
		l->lapLabel = "LAP";
		l->lapValue = mh.lap;
	}
	if ( cg_hudShowTimes.integer ) {
		if ( cent->finishRaceTime ) lapTime = cent->finishRaceTime - cent->startLapTime;
		else if ( cent->startRaceTime ) lapTime = cg.time - cent->startLapTime;
		else lapTime = 0;
		Q_strncpyz( mh.kvKey[0], CG_IsSprintTrack() ? "TIME" : "LAP", sizeof( mh.kvKey[0] ) );
		MH_Time( lapTime, mh.kvValue[0], sizeof( mh.kvValue[0] ) );
		mh.kvColor[0] = mhWhite;
		Q_strncpyz( mh.kvKey[1], "BEST", sizeof( mh.kvKey[1] ) );
		MH_Time( cent->bestLapTime, mh.kvValue[1], sizeof( mh.kvValue[1] ) );
		mh.kvColor[1] = mhWhite;
		l->kvCount = 2;
	}
	/* the ghost race: the ghost; else the gap to the car ahead, or the lead over the next one */
	if ( cgs.gametype == GT_GHOST ) {
		Q_strncpyz( mh.kvKey[l->kvCount], "GHOST", sizeof( mh.kvKey[0] ) );
		MH_GhostLine( mh.kvValue[l->kvCount], sizeof( mh.kvValue[0] ) );
		l->kvSize[l->kvCount] = 14.0f;
		mh.kvColor[l->kvCount++] = mhValue;
	} else if ( local > 1 && at[local - 1] >= 0 ) {
		Com_sprintf( mh.kvKey[l->kvCount], sizeof( mh.kvKey[0] ), "TO P%d", local - 1 );
		/* the splits are taken at the checkpoints: an older one can look ahead */
		if ( CG_GetRaceSplitGap( at[local - 1], ps->clientNum, &gap ) && gap > 0 ) MH_Gap( gap, mh.kvValue[l->kvCount], sizeof( mh.kvValue[0] ) );
		else Q_strncpyz( mh.kvValue[l->kvCount], "--", sizeof( mh.kvValue[0] ) );
		mh.kvColor[l->kvCount++] = mhAccent;
	} else if ( local == 1 && last > 1 && at[2] >= 0 ) {
		Q_strncpyz( mh.kvKey[l->kvCount], "LEAD", sizeof( mh.kvKey[0] ) );
		if ( CG_GetRaceSplitGap( ps->clientNum, at[2], &gap ) && gap > 0 ) MH_Gap( gap, mh.kvValue[l->kvCount], sizeof( mh.kvValue[0] ) );
		else Q_strncpyz( mh.kvValue[l->kvCount], "--", sizeof( mh.kvValue[0] ) );
		mh.kvColor[l->kvCount++] = mhFast;
	}
	/* the team race: whose colours we drive in */
	if ( cgs.gametype == GT_TEAM_RACING && ( MH_Team( ps->clientNum ) == TEAM_RED || MH_Team( ps->clientNum ) == TEAM_BLUE ) ) {
		Q_strncpyz( mh.kvKey[l->kvCount], "YOUR TEAM", sizeof( mh.kvKey[0] ) );
		Q_strncpyz( mh.kvValue[l->kvCount], MH_Team( ps->clientNum ) == TEAM_RED ? "RED" : "BLUE", sizeof( mh.kvValue[0] ) );
		mh.kvColor[l->kvCount++] = MH_TeamColor( MH_Team( ps->clientNum ) );
	}
	for ( i = 0; i < l->kvCount; i++ ) {
		l->kvKey[i] = mh.kvKey[i];
		l->kvValue[i] = mh.kvValue[i];
	}

	/* the standings: up to six rows around our place */
	l->standings = cg_hudShowOpponentList.integer && last > 0 && !cg.showScores &&
	               cg.predictedPlayerState.pm_type != PM_INTERMISSION && !CG_IntroCam_IsActive();
	l->orderTitle = cgs.gametype == GT_TEAM_RACING ? "Team race order" : "Race order";
	l->orderRight = "Gap";
	if ( cgs.gametype == GT_TEAM_RACING ) {
		/* the teams' average race time, as the server ranks them. Until the
		 * first car is home it counts everyone at the time so far, the same
		 * for both teams: "--" then. */
		qboolean home = qfalse;
		for ( i = 0; i < cgs.maxclients && i < MAX_CLIENTS; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cg_entities[i].finishRaceTime ) home = qtrue;
		}
		for ( i = 0; i < 2; i++ ) {
			if ( home && cg.teamTimes[i] > 0 && cg.teamTimes[i] < ( 1 << 30 ) ) MH_Time( cg.teamTimes[i], mh.teamValue[i], sizeof( mh.teamValue[i] ) );
			else Q_strncpyz( mh.teamValue[i], "--", sizeof( mh.teamValue[i] ) );
			l->teamValue[i] = mh.teamValue[i];
		}
		l->teamName[0] = "RED";
		l->teamName[1] = "BLUE";
		l->teamCount = 2;
		l->rowSwatch = qtrue;
	}
	for ( i = 0; i < MH_ROWS_MAX; i++ ) mh.rowColor[i] = NULL;
	if ( cgs.gametype == GT_GHOST ) {
		if ( l->standings ) MH_GhostSplits( l, cent, ps );
	} else if ( l->standings ) {
		count = last < MH_ROWS_MAX ? last : MH_ROWS_MAX;
		first = 1;
		if ( last > count ) {
			first = ( local > 0 ? local : 1 ) - count / 2;
			if ( first < 1 ) first = 1;
			if ( first > last - count + 1 ) first = last - count + 1;
		}
		for ( i = 0; i < count; i++ ) {
			int position = first + i, client = at[position];
			Com_sprintf( mh.rowPos[i], sizeof( mh.rowPos[i] ), "%02d", position );
			mh.rowMe[i] = client >= 0 && client == ps->clientNum;
			mh.rowTeam[i] = client >= 0 ? MH_Team( client ) : TEAM_FREE;
			if ( client < 0 ) {
				mh.rowName[i][0] = '\0';
				mh.rowValue[i][0] = '\0';
			} else {
				MH_Plain( cgs.clientinfo[client].name, mh.rowName[i], sizeof( mh.rowName[i] ) );
				if ( position == 1 ) Q_strncpyz( mh.rowValue[i], "LEADER", sizeof( mh.rowValue[i] ) );
				else if ( at[1] >= 0 && CG_GetRaceSplitGap( at[1], client, &gap ) ) MH_Gap( gap, mh.rowValue[i], sizeof( mh.rowValue[i] ) );
				else Q_strncpyz( mh.rowValue[i], "--", sizeof( mh.rowValue[i] ) );
			}
			l->rowPos[i] = mh.rowPos[i];
			l->rowName[i] = mh.rowName[i];
			l->rowValue[i] = mh.rowValue[i];
		}
		l->rowCount = count;
	}

	/* the top centre */
	l->mirror = cg_drawRearView.integer && MH_Views() && !cg.scoreBoardShowing;
	l->toast = CG_HudToastView( &mh.toast );
	if ( l->toast ) {
		l->toastIcon = qtrue;
		l->toastTag = mh.toast.tag;
		l->toastTitle = mh.toast.title;
		l->toastSub = mh.toast.sub[0] ? mh.toast.sub : NULL;
	}

	/* bottom left */
	l->jukebox = CG_JukeboxView( &mh.jukebox );
	if ( l->jukebox ) {
		l->jukeboxOpen = mh.jukebox.open;
		l->jukeboxLine = l->jukeboxOpen ? mh.jukebox.modes : mh.jukebox.line;
		l->jukeboxTime = l->jukeboxOpen ? mh.jukebox.times : mh.jukebox.time;
		l->jukeboxTitle = mh.jukebox.title;
		l->jukeboxArtist = mh.jukebox.artist;
		l->jukeboxState = mh.jukebox.state;
	}
	maxHealth = ps->stats[STAT_MAX_HEALTH] > 0 ? ps->stats[STAT_MAX_HEALTH] : 100;
	Com_sprintf( mh.integrity, sizeof( mh.integrity ), "%d", ps->stats[STAT_HEALTH] > 0 ? ps->stats[STAT_HEALTH] : 0 );
	mh.integrityFrac = (float)ps->stats[STAT_HEALTH] / maxHealth;
	l->integrity = mh.integrity;
	if ( ps->stats[STAT_ARMOR] > 0 ) {
		Com_sprintf( mh.armor, sizeof( mh.armor ), "%d", ps->stats[STAT_ARMOR] );
		l->armor = mh.armor;
	}
	/* the trunk: each item in a slot, "USE" beside its icon */
	{
		int value = ps->stats[STAT_HOLDABLE_ITEM], mask = BG_TrunkMask( value ), items[TRUNK_ITEM_COUNT], n = 0;
		for ( i = 0; i < TRUNK_ITEM_COUNT; i++ ) {
			if ( mask & ( 1 << i ) ) items[n++] = BG_FindItemForHoldable( bg_trunkItems[i] ) - bg_itemlist;
		}
		if ( !n && ( value = BG_HoldableItemIndex( value ) ) != 0 ) items[n++] = value;
		for ( i = 0; i < n && i < MH_SLOTS_MAX; i++ ) {
			CG_RegisterItemVisuals( items[i] );
			mh.slotIcon[i] = cg_items[items[i]].icon;
			Q_strncpyz( mh.slotName[i], bg_itemlist[items[i]].pickup_name ? bg_itemlist[items[i]].pickup_name : "", sizeof( mh.slotName[i] ) );
			l->slotLabel[i] = "ITEM";
			l->slotValue[i] = "USE";
			l->slotName[i] = mh.slotName[i];
		}
		l->slotCount = i;
	}

	/* bottom right */
	l->minimap = cg_drawMMap.integer && MH_Views();
	l->minimapSize = MH_MINIMAP * WS_Clamp( 0.4f, cg_mmap_size.value / MH_MINIMAP_DEFAULT, 1.7f );
	factor = 0.0f;
	mh.slipAlpha = 0.0f;
	if ( cg_hudShowSlipstream.integer && cg.predictedPlayerState.stats[STAT_HEALTH] > 0 &&
	     cg.predictedPlayerState.pm_type == PM_NORMAL ) {
		factor = BG_SlipstreamFactor( &cg.predictedPlayerState );
	}
	if ( factor > 0.0f ) {
		l->slipstream = qtrue;
		mh.slipAlpha = WS_Clamp( 0.0f, factor / 0.03f, 1.0f );
		mh.slipFrac = factor / 0.25f;
		Com_sprintf( mh.slip, sizeof( mh.slip ), "-%d%% DRAG", (int)( factor * 100.0f + 0.5f ) );
		l->slipValue = mh.slip;
	}
	AngleVectors( ps->viewangles, forward, NULL, NULL );
	Com_sprintf( mh.speed, sizeof( mh.speed ), "%d", (int)fabs( Q3VelocityToRL( DotProduct( ps->velocity, forward ) ) ) );
	l->speed = mh.speed;
	l->unit = cg_metricUnits.integer ? "KPH" : "MPH";
	if ( ps->stats[STAT_GEAR] < 0 ) Q_strncpyz( mh.gear, "R", sizeof( mh.gear ) );
	else if ( ps->stats[STAT_GEAR] == 0 ) Q_strncpyz( mh.gear, "N", sizeof( mh.gear ) );
	else Com_sprintf( mh.gear, sizeof( mh.gear ), "%d", ps->stats[STAT_GEAR] );
	l->gear = mh.gear;
	mh.rpmFrac = WS_Clamp( 0.0f, ps->stats[STAT_RPM] / (float)CP_RPM_MAX, 1.0f );
	turbo = ps->powerups[PW_TURBO];
	if ( turbo < 0 ) turbo = -turbo;
	else if ( turbo > cg.time ) turbo -= cg.time;
	else turbo = 0;
	mh.nosFrac = WS_Clamp( 0.0f, turbo / (float)RALLY_TURBO_MAX_MSEC, 1.0f );
	Com_sprintf( mh.nos, sizeof( mh.nos ), "%d%%", (int)( mh.nosFrac * 100.0f + 0.5f ) );
	mh.fuelFrac = WS_Clamp( 0.0f, ps->stats[STAT_FUEL] / 100.0f, 1.0f );
	Com_sprintf( mh.fuel, sizeof( mh.fuel ), "%d%%", (int)( mh.fuelFrac * 100.0f + 0.5f ) );
	l->nos = mh.nos;
	l->fuel = mh.fuel;

	MH_RaceLayout( l );
}

/* -------------------------------------------------------------------------
   the 3D views: rear view and minimap in their frames
   ------------------------------------------------------------------------- */

static void MH_ViewRect( const wsRect_t *r, refdef_t *view, float fovX ) {
	float tmp;
	view->x = (int)MH_Snap( r->x );
	view->y = (int)MH_Snap( r->y );
	view->width = (int)( MH_Snap( r->x + r->w ) - view->x );
	view->height = (int)( MH_Snap( r->y + r->h ) - view->y );
	view->fov_x = fovX;
	tmp = view->width / tan( view->fov_x / 360.0f * M_PI );
	view->fov_y = atan2( view->height, tmp ) * 360.0f / M_PI;
	view->time = cg.time;
	view->rdflags = 0;
}

void CG_ModernHUD_DrawViews( void ) {
	mhRaceLayout_t *l = &mh.l;
	MH_Prepare();
	if ( l->mirror ) {
		MH_ViewRect( &l->mirrorBox, &cg.mirrorRefdef, 70.0f );
		CG_AddObjectsToScene( cg_rearViewRenderLevel.integer );
		trap_R_RenderScene( &cg.mirrorRefdef );
	}
	if ( l->minimap ) {
		MH_ViewRect( &l->minimapView, &cg.mmapRefdef, cg_mmap_fov.value );
		CG_AddObjectsToScene( cg_mmap_renderLevel.integer );
		CG_AddKOTHHillIndicatorToScene( qtrue );
		if ( cg_mmap_renderLevel.integer & RL_PLAYERS ) {
			CG_AddCEntity( &cg_entities[cg.snap->ps.clientNum] );
		}
		trap_R_RenderScene( &cg.mmapRefdef );
	}
}

/* -------------------------------------------------------------------------
   the panels
   ------------------------------------------------------------------------- */

static void MH_DrawCard( const mhRaceLayout_t *l ) {
	int i;
	MH_Panel( &l->card, qtrue, 1.0f );
	MH_Str( &l->titleT, mhLabel, 1.0f );
	if ( l->pos ) {
		if ( l->posLabel ) MH_Str( &l->posLabelT, mhLabel, 1.0f );
		MH_Str( &l->posT, mh.posColor, 1.0f );
		MH_Str( &l->ofT, mhMuted, 1.0f );
	}
	if ( l->lapValue ) {
		MH_Str( &l->lapLabelT, mhLabel, 1.0f );
		MH_Str( &l->lapValueT, mhWhite, 1.0f );
	}
	if ( l->progress ) {
		const wsRect_t *b = &l->progressBar;
		MH_Bar( b, mh.progressFrac, mhAccent, mhTrack, 1.0f );
		/* past a dozen checkpoints the ticks turn into a ruler: the CP line says it */
		for ( i = 1; l->progressTicks <= MH_TICKS_MAX && i <= l->progressTicks; i++ ) {
			MH_Fill( b->x + b->w * i / ( l->progressTicks + 1 ), b->y - 3.0f, 2.0f, 12.0f,
			         i <= mh.progressDone ? mhWhite : mhTickOpen, 1.0f );
		}
		MH_Str( &l->progressLeftT, mhMuted, 1.0f );
		MH_Str( &l->progressRightT, mhMuted, 1.0f );
	}
	if ( l->pos || l->lapValue || l->progress ) MH_FillRect( &l->sep, mhLine, 1.0f );
	for ( i = 0; i < l->kvCount; i++ ) {
		MH_Str( &l->kvKeyT[i], mhMuted, 1.0f );
		MH_Str( &l->kvValueT[i], mh.kvColor[i], 1.0f );
	}
}

void CG_ModernHUD_DrawStandings( void ) {
	const mhRaceLayout_t *l = &mh.l;
	int i;
	MH_Prepare();
	if ( !l->standings ) return;
	MH_Panel( &l->order, qtrue, 1.0f );
	MH_Str( &l->orderTitleT, mhLabel, 1.0f );
	MH_Str( &l->orderRightT, mhMuted, 1.0f );
	for ( i = 0; i < l->teamCount; i++ ) {
		MH_FillRect( &l->teamCell[i], mhTeam[i], 1.0f );
		MH_Str( &l->teamNameT[i], mhOnTeam, 1.0f );
		MH_Str( &l->teamValueT[i], mhOnTeam, 1.0f );
	}
	for ( i = 0; i < l->rowCount; i++ ) {
		if ( mh.rowMe[i] ) MH_FillRect( &l->row[i], mhMe, 1.0f );
		if ( l->rowSwatch && l->rowName[i] && l->rowName[i][0] ) MH_FillRect( &l->rowSwatchBox[i], MH_TeamColor( mh.rowTeam[i] ), 1.0f );
		MH_Str( &l->rowPosT[i], mhMuted, 1.0f );
		MH_Str( &l->rowNameT[i], mhWhite, 1.0f );
		MH_Str( &l->rowValueT[i], mh.rowColor[i] ? mh.rowColor[i] : mhValue, 1.0f );
	}
}

static void MH_DrawToast( const mhRaceLayout_t *l ) {
	const float *tone = mh.toast.tone == 1 ? mhFast : ( mh.toast.tone == 2 ? mhMuted : mhAccent );
	float a = mh.toast.alpha;
	qhandle_t icon = mh.toast.icon;
	wsRect_t box = l->toastBox;

	MH_FillRect( &box, mhPanel, a );
	MH_Border( &box, mhLine, 2.0f, tone, a );
	MH_Border( &l->toastIconBox, mhLine, 1.0f, NULL, a );
	if ( icon ) {
		MH_Pic( &l->toastIconBox, 8.0f, icon, mhWhite, a );
	} else {
		MH_Pic( &l->toastIconBox, 11.0f, mh.toast.tone == 2 ? mh.message : mh.chevron, tone, a );
	}
	MH_Str( &l->toastTagT, tone, a );
	MH_Str( &l->toastTitleT, mhWhite, a );
	if ( l->toastSub ) MH_Str( &l->toastSubT, mhLabel, a );
}

static void MH_DrawJukebox( const mhRaceLayout_t *l ) {
	int i;
	if ( !l->jukeboxOpen ) {
		MH_FillRect( &l->jukeboxBox, mhPanel, 1.0f );
		MH_Border( &l->jukeboxBox, mhLine, 1.0f, NULL, 1.0f );
		MH_Pic( &l->jukeboxIcon, 0.0f, mh.note, mhAccent, 1.0f );
		MH_Str( &l->jukeboxLineT, mhSoft, 1.0f );
		MH_Str( &l->jukeboxTimeT, mhMuted, 1.0f );
		MH_Bar( &l->jukeboxBar, mh.jukebox.progress, mhAccent, mhDarkTrack, 1.0f );
		return;
	}
	MH_Panel( &l->jukeboxBox, qtrue, 1.0f );
	MH_Str( &l->jukeboxStateT, mhAccent, 1.0f );
	MH_Str( &l->jukeboxTimeT, mhMuted, 1.0f );
	MH_Str( &l->jukeboxTitleT, mhWhite, 1.0f );
	MH_Str( &l->jukeboxArtistT, mhLabel, 1.0f );
	MH_Bar( &l->jukeboxBar, mh.jukebox.progress, mhAccent, mhTrack, 1.0f );
	for ( i = 0; i < 3; i++ ) {
		MH_Border( &l->jukeboxButton[i], mhLine, 1.0f, NULL, 1.0f );
		MH_Pic( &l->jukeboxButton[i], 9.0f, i == 0 ? mh.prev : ( i == 1 ? mh.play : mh.next ), mhValue, 1.0f );
	}
	MH_Str( &l->jukeboxModesT, mhMuted, 1.0f );
}

static void MH_DrawVehicle( const mhRaceLayout_t *l ) {
	const float *tone = mh.integrityFrac <= MH_LOW_INTEGRITY ? mhAccent : mhWhite;
	MH_Panel( &l->vehicle, qtrue, 1.0f );
	MH_Str( &l->integrityLabel, mhLabel, 1.0f );
	MH_Str( &l->integrityT, tone, 1.0f );
	if ( l->armor ) {
		MH_Str( &l->armorLabel, mhLabel, 1.0f );
		MH_Str( &l->armorT, mhValue, 1.0f );
	}
	MH_Bar( &l->integrityBar, mh.integrityFrac, mh.integrityFrac <= MH_LOW_INTEGRITY ? mhAccent : mhLit, mhTrack, 1.0f );
	if ( l->slotCount ) {
		int i;
		MH_FillRect( &l->vehicleSep, mhLine, 1.0f );
		for ( i = 0; i < l->slotCount; i++ ) {
			MH_Str( &l->slotLabelT[i], mhMuted, 1.0f );
			MH_Pic( &l->slotIcon[i], 0.0f, mh.slotIcon[i], mhWhite, 1.0f );
			MH_Str( &l->slotValueT[i], mhAccent, 1.0f );
			MH_Str( &l->slotNameT[i], mhLabel, 1.0f );
		}
	}
}

static void MH_DrawCockpit( const mhRaceLayout_t *l ) {
	float gap = 3.0f, segW = ( l->rpm.w - ( MH_RPM_SEGMENTS - 1 ) * gap ) / MH_RPM_SEGMENTS;
	int lit = (int)( mh.rpmFrac * MH_RPM_SEGMENTS + 0.5f ), i;

	MH_Panel( &l->cockpit, qtrue, 1.0f );
	MH_Str( &l->speedT, mhWhite, 1.0f );
	MH_Str( &l->unitT, mhLabel, 1.0f );
	MH_Str( &l->gearLabel, mhMuted, 1.0f );
	MH_Border( &l->gearBox, mhLine, 1.0f, NULL, 1.0f );
	MH_Str( &l->gearT, mhWhite, 1.0f );
	for ( i = 0; i < MH_RPM_SEGMENTS; i++ ) {
		const float *c = i >= lit ? mhTrack : ( i >= MH_RPM_HOT ? mhAccent : mhLit );
		MH_Fill( l->rpm.x + i * ( segW + gap ), l->rpm.y, segW, l->rpm.h, c, 1.0f );
	}
	MH_Str( &l->nosLabel, mhLabel, 1.0f );
	MH_Str( &l->nosValue, mhWhite, 1.0f );
	MH_Bar( &l->nosBar, mh.nosFrac, mhAccent, mhTrack, 1.0f );
	MH_Str( &l->fuelLabel, mhLabel, 1.0f );
	MH_Str( &l->fuelValue, mhWhite, 1.0f );
	MH_Bar( &l->fuelBar, mh.fuelFrac, mhSoft, mhTrack, 1.0f );
}

/* the parts of the HUD that go with the status bar (cg_drawStatus) */
void CG_ModernHUD_Draw( void ) {
	const mhRaceLayout_t *l = &mh.l;
	MH_Prepare();
	if ( l->mirror ) MH_Border( &l->mirrorBox, mhGlass, 1.0f, NULL, 1.0f );
	if ( l->minimap ) {
		/* the frame around the view, which is already drawn */
		const wsRect_t *b = &l->minimapBox, *v = &l->minimapView;
		MH_Fill( b->x, b->y, b->w, v->y - b->y, mhPanel, 1.0f );
		MH_Fill( b->x, v->y + v->h, b->w, b->y + b->h - v->y - v->h, mhPanel, 1.0f );
		MH_Fill( b->x, v->y, v->x - b->x, v->h, mhPanel, 1.0f );
		MH_Fill( v->x + v->w, v->y, b->x + b->w - v->x - v->w, v->h, mhPanel, 1.0f );
		MH_Border( b, mhLine, 1.0f, NULL, 1.0f );
	}
	if ( CG_IntroCam_IsActive() ) return;
	MH_DrawCard( l );
	if ( l->toast ) MH_DrawToast( l );
	if ( l->jukebox ) MH_DrawJukebox( l );
	MH_DrawVehicle( l );
	if ( l->slipstream ) {
		MH_Panel( &l->slip, qfalse, mh.slipAlpha );
		MH_Str( &l->slipLabel, mhFast, mh.slipAlpha );
		MH_Str( &l->slipValueT, mhFast, mh.slipAlpha );
		MH_Bar( &l->slipBar, mh.slipFrac, mhFast, mhTrack, mh.slipAlpha );
	}
	MH_DrawCockpit( l );
}

/* -------------------------------------------------------------------------
   big messages in the middle (gen.py bigmsg): the countdown, wrong way and
   the centerprints, one at a time, in the menu fonts
   ------------------------------------------------------------------------- */

static const vec4_t mhShadow = { 0.0f, 0.0f, 0.0f, 0.6f };

/* text-shadow: 0 2px 12px: a soft shadow 2px down */
static void MH_ShadowStr( const mhText_t *t, const float *color, float alpha ) {
	mhText_t shadow = *t;
	shadow.box.y += 2.0f;
	MH_Str( &shadow, mhShadow, alpha );
	MH_Str( t, color, alpha );
}

static void MH_DrawMessage( const char *title, float size, const float *color, int subCount, const char **sub, float alpha ) {
	mhMessageLayout_t m;
	int i;
	memset( &m, 0, sizeof( m ) );
	m.viewWidth = mh.l.viewWidth;
	m.title = title;
	m.titleSize = size;
	m.subCount = subCount;
	for ( i = 0; i < subCount && i < MH_SUB_MAX; i++ ) m.sub[i] = sub[i];
	MH_MessageLayout( &m );
	MH_ShadowStr( &m.titleT, color, alpha );
	for ( i = 0; i < m.subCount; i++ ) MH_ShadowStr( &m.subT[i], mhValue, alpha );
}

/* "3" "2" "1" "GO!": until a second after the start, the last one fading */
static qboolean MH_Countdown( void ) {
	float alpha = 1.0f;
	if ( !cg.countDownPrint[0] || cg.time > cg.countDownEnd + 1000 ) return qfalse;
	if ( cg.time > cg.countDownEnd ) alpha = 1.0f - ( cg.time - cg.countDownEnd ) / 1000.0f;
	MH_DrawMessage( cg.countDownPrint, 96.0f, Q_stricmp( cg.countDownPrint, "GO!" ) ? mhWhite : mhAccent, 0, NULL, alpha );
	return qtrue;
}

/* two seconds the wrong way round (cg_hud_racing.c keeps the time) */
static qboolean MH_WrongWay( void ) {
	static const char *sub[1] = { "TURN AROUND" };
	float *color;
	if ( !cg.wrongWayStartTime || cg.wrongWayStartTime > cg.time - 2000 ) return qfalse;
	color = CG_FadeColor( cg.wrongWayTime, 300 );
	if ( !color ) return qfalse;
	MH_DrawMessage( "WRONG WAY", 56.0f, mhAccent, 1, sub, color[3] );
	return qtrue;
}

/* a centerprint (also target_print): the first line big, the next ones under it */
static qboolean MH_CenterPrint( void ) {
	static char lines[1 + MH_SUB_MAX][MH_TEXT_MAX];
	const char *sub[MH_SUB_MAX], *s;
	float *color;
	int count = 0, n;

	if ( !cg.centerPrintTime || CG_LadderGhost_PickerIsOpen() ) return qfalse;
	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) return qfalse;
	for ( s = cg.centerPrint; *s && count < 1 + MH_SUB_MAX; ) {
		char raw[MH_TEXT_MAX];
		for ( n = 0; s[n] && s[n] != '\n' && n < MH_TEXT_MAX - 1; n++ ) raw[n] = s[n];
		raw[n] = '\0';
		while ( s[n] && s[n] != '\n' ) n++;
		s += n;
		if ( *s == '\n' ) s++;
		MH_Plain( raw, lines[count], sizeof( lines[0] ) );
		if ( lines[count][0] ) count++;
	}
	if ( !count ) return qfalse;
	for ( n = 1; n < count; n++ ) sub[n - 1] = lines[n];
	MH_DrawMessage( lines[0], 44.0f, mhWhite, count - 1, sub, color[3] );
	return qtrue;
}

void CG_ModernHUD_DrawMessages( void ) {
	if ( !cg.snap ) return;
	MH_Prepare();
	if ( MH_Countdown() ) return;
	if ( MH_WrongWay() ) return;
	MH_CenterPrint();
}

/* -------------------------------------------------------------------------
   the spectator: whom we follow, or the free camera, at the bottom
   ------------------------------------------------------------------------- */

void CG_ModernHUD_DrawSpectator( void ) {
	mhSpectatorLayout_t sp;
	char name[40];
	if ( !cg.snap || CG_IntroCam_IsActive() ) return;
	MH_Prepare();
	memset( &sp, 0, sizeof( sp ) );
	sp.viewWidth = mh.l.viewWidth;
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		MH_Plain( cgs.clientinfo[cg.snap->ps.clientNum].name, name, sizeof( name ) );
		sp.label = "FOLLOWING";
		sp.name = name;
		sp.hint1 = "FIRE NEXT";
	} else {
		sp.label = "SPECTATOR";
		sp.name = "Watching";
		sp.hint1 = "FIRE FOLLOW";
	}
	sp.hint2 = "ESC MENU";
	MH_SpectatorLayout( &sp );
	MH_Panel( &sp.box, qtrue, 1.0f );
	MH_Str( &sp.labelT, mhLabel, 1.0f );
	MH_Str( &sp.nameT, mhWhite, 1.0f );
	MH_Str( &sp.hint1T, mhMuted, 1.0f );
	MH_Str( &sp.hint2T, mhMuted, 1.0f );
}
