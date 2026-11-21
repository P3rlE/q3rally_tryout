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
#include "../client/keycodes.h"

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
	const float		*rowNameColor[MH_ROWS_MAX];	/* NULL: white */
	char			elimValue[24], elimCaption[48];
	float			elimFrac;
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
	qhandle_t		note, prev, play, next, chevron, message, gun, mine, smoke;
	char			slotValue[MH_SLOTS_MAX][8];
	const float		*slotColor[MH_SLOTS_MAX];
	char			killA[MH_KILLS_MAX][32], killB[MH_KILLS_MAX][32];
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
	         cgs.gametype == GT_TEAM_RACING || cgs.gametype == GT_RACING_DM || cgs.gametype == GT_TEAM_RACING_DM ||
	         cgs.gametype == GT_ELIMINATION );
}

/* the gaps to the leader for every place (-1: none yet). The order is the
 * server's, by the way still to go; the gaps are taken at the checkpoints.
 * Where they disagree (just overtaken), a place is shown no closer than the
 * one ahead of it. */
static int mhGap[MAX_CLIENTS + 1];

static void MH_Gaps( const int at[MAX_CLIENTS + 1], int last ) {
	int position, gap, floor = 0;
	mhGap[1] = 0;
	for ( position = 2; position <= last && position <= MAX_CLIENTS; position++ ) {
		mhGap[position] = -1;
		if ( at[1] < 0 || at[position] < 0 || !CG_GetRaceSplitGap( at[1], at[position], &gap ) ) continue;
		if ( gap < floor ) gap = floor;
		mhGap[position] = floor = gap;
	}
}

/* elimination: the last one over the line in each lap is out. The card
 * shows how many are left, who is last now and how many are through the lap
 * that decides; the order lists who is still in, the last one in DANGER, and
 * under it who is out, the latest first. */
static void MH_Elimination( mhRaceLayout_t *l, const int at[MAX_CLIENTS + 1], int last,
                            const playerState_t *ps, const centity_t *cent ) {
	int alive[MAX_CLIENTS], out[MAX_CLIENTS], aliveCount = 0, outCount = 0, total, position, i, j;
	int lap = 0, through = 0, me = -1, count, first, row = 0, time;
	static const vec4_t outName = { 119 / 255.0f, 119 / 255.0f, 119 / 255.0f, 1.0f };	/* #777 */

	for ( position = 1; position <= last; position++ ) {
		if ( at[position] >= 0 && !cg_entities[at[position]].eliminationOut ) alive[aliveCount++] = at[position];
	}
	for ( i = 0; i < cgs.maxclients && i < MAX_CLIENTS; i++ ) {
		if ( !cgs.clientinfo[i].infoValid || !cg_entities[i].eliminationOut ) continue;
		/* the latest out first */
		for ( j = outCount; j > 0 && cg_entities[out[j - 1]].finishRaceTime < cg_entities[i].finishRaceTime; j-- ) out[j] = out[j - 1];
		out[j] = i;
		outCount++;
	}
	total = aliveCount + outCount;
	for ( i = 0; i < aliveCount; i++ ) {
		if ( alive[i] == ps->clientNum ) me = i;
		if ( !lap || cg_entities[alive[i]].currentLap < lap ) lap = cg_entities[alive[i]].currentLap;
	}
	for ( i = 0; i < aliveCount; i++ ) {
		if ( cg_entities[alive[i]].currentLap > lap ) through++;
	}

	/* the card: place among those still in, how many are left */
	if ( me >= 0 ) {
		Com_sprintf( mh.pos, sizeof( mh.pos ), "P%d", me + 1 );
		Com_sprintf( mh.of, sizeof( mh.of ), "/ %d", aliveCount );
		l->pos = mh.pos;
		l->of = mh.of;
	} else {
		l->pos = NULL;
	}
	Com_sprintf( mh.lap, sizeof( mh.lap ), "%d/%d", aliveCount, total );
	l->lapLabel = "LEFT";
	l->lapValue = mh.lap;
	l->progress = qfalse;
	l->elim = cent->startRaceTime && aliveCount > 1 && lap > 0;
	if ( l->elim ) {
		if ( alive[aliveCount - 1] == ps->clientNum && !( ps->pm_flags & PMF_FOLLOW ) ) Q_strncpyz( mh.elimValue, "YOU", sizeof( mh.elimValue ) );
		else {
			MH_Plain( cgs.clientinfo[alive[aliveCount - 1]].name, mh.elimValue, sizeof( mh.elimValue ) );
			Q_strupr( mh.elimValue );
		}
		Com_sprintf( mh.elimCaption, sizeof( mh.elimCaption ), "%d OF %d THROUGH LAP %d", through, aliveCount - 1, lap );
		mh.elimFrac = (float)through / ( aliveCount - 1 );
		l->elimLabel = "NEXT OUT";
		l->elimValue = mh.elimValue;
		l->elimCaption = mh.elimCaption;
	}
	l->kvCount = 0;
	if ( cg_hudShowTimes.integer ) {
		if ( cent->finishRaceTime ) time = cent->finishRaceTime - cent->startRaceTime;
		else if ( cent->startRaceTime && cg.time > cent->startRaceTime ) time = cg.time - cent->startRaceTime;
		else time = 0;
		Q_strncpyz( mh.kvKey[0], "TIME", sizeof( mh.kvKey[0] ) );
		MH_Time( time, mh.kvValue[0], sizeof( mh.kvValue[0] ) );
		mh.kvColor[0] = mhWhite;
		l->kvKey[0] = mh.kvKey[0];
		l->kvValue[0] = mh.kvValue[0];
		l->kvCount = 1;
	}

	/* the order: who is still in around us, then who is out */
	l->orderTitle = "Still in";
	l->orderRight = "Status";
	if ( !l->standings ) return;
	count = aliveCount < MH_ROWS_MAX ? aliveCount : MH_ROWS_MAX;
	first = 0;
	if ( aliveCount > count ) {
		first = ( me >= 0 ? me : 0 ) - count / 2;
		if ( first < 0 ) first = 0;
		if ( first > aliveCount - count ) first = aliveCount - count;
	}
	for ( i = first; i < first + count; i++, row++ ) {
		Com_sprintf( mh.rowPos[row], sizeof( mh.rowPos[0] ), "%02d", i + 1 );
		MH_Plain( cgs.clientinfo[alive[i]].name, mh.rowName[row], sizeof( mh.rowName[0] ) );
		Q_strncpyz( mh.rowValue[row], i == aliveCount - 1 && aliveCount > 1 ? "DANGER" : "SAFE", sizeof( mh.rowValue[0] ) );
		mh.rowColor[row] = i == aliveCount - 1 && aliveCount > 1 ? mhAccent : NULL;
		mh.rowNameColor[row] = NULL;
		mh.rowMe[row] = alive[i] == ps->clientNum;
	}
	for ( i = 0; i < outCount && row < MH_ROWS_MAX; i++, row++ ) {
		Q_strncpyz( mh.rowPos[row], "--", sizeof( mh.rowPos[0] ) );
		MH_Plain( cgs.clientinfo[out[i]].name, mh.rowName[row], sizeof( mh.rowName[0] ) );
		Q_strncpyz( mh.rowValue[row], "OUT", sizeof( mh.rowValue[0] ) );
		mh.rowColor[row] = mhMuted;
		mh.rowNameColor[row] = outName;
		mh.rowMe[row] = out[i] == ps->clientNum;
	}
	for ( i = 0; i < row; i++ ) {
		l->rowPos[i] = mh.rowPos[i];
		l->rowName[i] = mh.rowName[i];
		l->rowValue[i] = mh.rowValue[i];
	}
	l->rowCount = row;
}

/* the race with weapons, and the races in teams */
#define MH_COMBAT()		( cgs.gametype == GT_RACING_DM || cgs.gametype == GT_TEAM_RACING_DM || \
						  ( cgs.gametype == GT_ELIMINATION && cgs.eliminationWeapons ) )
#define MH_TEAMRACE()	( cgs.gametype == GT_TEAM_RACING || cgs.gametype == GT_TEAM_RACING_DM )

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

/* the weapons in the car's card: the front one (or the one being picked, in
 * the accent) and the first rear one with ammo, as the old HUD chose them */
static void MH_WeaponSlot( mhRaceLayout_t *l, const char *label, int weapon, int ammo, qhandle_t icon, const float *color ) {
	int i = l->slotCount++;
	const gitem_t *item = BG_FindItemForWeapon( weapon );
	mh.slotIcon[i] = icon;
	mh.slotColor[i] = color;
	if ( ammo >= 0 ) Com_sprintf( mh.slotValue[i], sizeof( mh.slotValue[i] ), "%d", ammo );
	else Q_strncpyz( mh.slotValue[i], "--", sizeof( mh.slotValue[i] ) );
	Q_strncpyz( mh.slotName[i], item && item->pickup_name ? item->pickup_name : "", sizeof( mh.slotName[i] ) );
	l->slotLabel[i] = label;
	l->slotValue[i] = mh.slotValue[i];
	l->slotName[i] = mh.slotName[i];
}

static void MH_Weapons( mhRaceLayout_t *l, const playerState_t *ps ) {
	int front = ps->weapon, rear = 0, fallback = 0, w;
	const float *color = mhWhite;
	if ( cg.weaponSelectTime > 0 && cg.time >= cg.weaponSelectTime && cg.time - cg.weaponSelectTime < WEAPON_SELECT_TIME &&
	     cg.weaponSelect > WP_NONE && cg.weaponSelect < RWP_SMOKE && ( ps->stats[STAT_WEAPONS] & ( 1u << cg.weaponSelect ) ) ) {
		front = cg.weaponSelect;
		color = mhAccent;
	}
	if ( front > WP_NONE && front < RWP_SMOKE ) MH_WeaponSlot( l, "FRONT", front, ps->ammo[front], mh.gun, color );
	for ( w = RWP_SMOKE; w < WP_NUM_WEAPONS; w++ ) {
		if ( !( ps->stats[STAT_WEAPONS] & ( 1u << w ) ) ) continue;
		if ( !fallback ) fallback = w;
		if ( ps->ammo[w] > 0 ) {
			rear = w;
			break;
		}
	}
	if ( !rear ) rear = fallback;
	if ( rear ) MH_WeaponSlot( l, "REAR", rear, ps->ammo[rear], rear == RWP_SMOKE ? mh.smoke : mh.mine, mhWhite );
}

/* the kill feed: who got whom with what, the last three of six seconds */
#define MH_KILL_TIME	6000
static struct {
	int		time;
	char	a[32], b[32];
	const char	*weapon;
} mhKills[MH_KILLS_MAX];

static const char *MH_KillWeapon( int mod ) {
	switch ( mod ) {
	case MOD_MACHINEGUN: return "MG";
	case MOD_SHOTGUN: return "SHOTGUN";
	case MOD_GAUNTLET: return "GAUNTLET";
	case MOD_GRENADE: case MOD_GRENADE_SPLASH: return "GRENADE";
	case MOD_ROCKET: case MOD_ROCKET_SPLASH: return "ROCKET";
	case MOD_PLASMA: case MOD_PLASMA_SPLASH: return "PLASMA";
	case MOD_FLAME_THROWER: case MOD_FIRE: return "FLAMES";
	case MOD_RAILGUN: return "RAIL";
	case MOD_LIGHTNING: return "LIGHTNING";
	case MOD_BFG: case MOD_BFG_SPLASH: return "BFG";
	case MOD_MINE: case MOD_MINE_SPLASH: return "MINES";
	case MOD_BIOHAZARD: return "BIO";
	case MOD_POISON: return "GAS";
	case MOD_VEHICLE_COLLISION: case MOD_DERBY_NO_RAM: return "RAM";
	case MOD_TELEFRAG: return "TELEFRAG";
	case MOD_SUICIDE: return "SUICIDE";
	case MOD_WORLD_COLLISION: case MOD_HIGH_FORCES: case MOD_UPSIDEDOWN: case MOD_BO_SHOCKS:
	case MOD_FALLING: case MOD_CRUSH: return "CRASH";
	case MOD_WATER: case MOD_SLIME: case MOD_LAVA: case MOD_TRIGGER_HURT: case MOD_TARGET_LASER: return "OUT";
	default: return "KILL";
	}
}

void CG_ModernHUD_Obituary( int target, int attacker, int mod ) {
	int i;
	if ( target < 0 || target >= MAX_CLIENTS ) return;
	for ( i = 0; i < MH_KILLS_MAX - 1; i++ ) mhKills[i] = mhKills[i + 1];
	i = MH_KILLS_MAX - 1;
	mhKills[i].time = cg.time;
	mhKills[i].weapon = MH_KillWeapon( mod );
	if ( attacker >= 0 && attacker < MAX_CLIENTS && attacker != target ) {
		MH_Plain( cgs.clientinfo[attacker].name, mhKills[i].a, sizeof( mhKills[i].a ) );
		MH_Plain( cgs.clientinfo[target].name, mhKills[i].b, sizeof( mhKills[i].b ) );
	} else {
		/* no one else to blame: the one who went out */
		MH_Plain( cgs.clientinfo[target].name, mhKills[i].a, sizeof( mhKills[i].a ) );
		mhKills[i].b[0] = '\0';
	}
}

static void MH_Kills( mhRaceLayout_t *l ) {
	int i;
	l->killCount = 0;
	if ( !MH_COMBAT() || cg.showScores ) return;
	for ( i = 0; i < MH_KILLS_MAX; i++ ) {
		if ( !mhKills[i].time || mhKills[i].time > cg.time || cg.time - mhKills[i].time > MH_KILL_TIME ) continue;
		l->killA[l->killCount] = mhKills[i].a;
		l->killWeapon[l->killCount] = mhKills[i].weapon;
		l->killB[l->killCount++] = mhKills[i].b;
	}
}

static void MH_Register( void ) {
	if ( mh.note ) return;
	mh.note = trap_R_RegisterShaderNoMip( "menu/art/hud_note" );
	mh.prev = trap_R_RegisterShaderNoMip( "menu/art/hud_prev" );
	mh.play = trap_R_RegisterShaderNoMip( "menu/art/hud_play" );
	mh.next = trap_R_RegisterShaderNoMip( "menu/art/hud_next" );
	mh.chevron = trap_R_RegisterShaderNoMip( "menu/art/hud_chevron" );
	mh.message = trap_R_RegisterShaderNoMip( "menu/art/hud_message" );
	mh.gun = trap_R_RegisterShaderNoMip( "menu/art/hud_gun" );
	mh.mine = trap_R_RegisterShaderNoMip( "menu/art/hud_mine" );
	mh.smoke = trap_R_RegisterShaderNoMip( "menu/art/hud_smoke" );
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
	int at[MAX_CLIENTS + 1], local, last, i, first, count, lap, lapTime, racers, turbo, maxHealth;
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
	MH_Gaps( at, last );
	racers = cgs.numRacers > 0 ? cgs.numRacers : ( last > 0 ? last : 1 );

	/* the mode card */
	map = CG_ConfigString( CS_MESSAGE );
	if ( !map || !map[0] ) map = COM_SkipPath( cgs.mapname );
	MH_Plain( map, mh.rowName[0], sizeof( mh.rowName[0] ) );
	Com_sprintf( mh.title, sizeof( mh.title ), "%s \xb7 %s",
	             cgs.gametype == GT_GHOST ? "Ghost race" : cgs.gametype == GT_SPRINT ? "Sprint" :
	             cgs.gametype == GT_TEAM_RACING ? "Team race" : cgs.gametype == GT_RACING_DM ? "Race combat" :
	             cgs.gametype == GT_TEAM_RACING_DM ? "Team race combat" :
	             cgs.gametype == GT_ELIMINATION ? "Elimination" : "Race", mh.rowName[0] );
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
	} else if ( MH_COMBAT() ) {
		/* with weapons: the frags count */
		Q_strncpyz( mh.kvKey[l->kvCount], "SCORE", sizeof( mh.kvKey[0] ) );
		Com_sprintf( mh.kvValue[l->kvCount], sizeof( mh.kvValue[0] ), "%d", ps->persistant[PERS_SCORE] );
		mh.kvColor[l->kvCount++] = mhWhite;
	} else if ( local > 1 && at[local - 1] >= 0 ) {
		/* the same figures as the race order: the difference of the two gaps */
		Com_sprintf( mh.kvKey[l->kvCount], sizeof( mh.kvKey[0] ), "TO P%d", local - 1 );
		if ( mhGap[local] >= 0 && mhGap[local - 1] >= 0 ) MH_Gap( mhGap[local] - mhGap[local - 1], mh.kvValue[l->kvCount], sizeof( mh.kvValue[0] ) );
		else Q_strncpyz( mh.kvValue[l->kvCount], "--", sizeof( mh.kvValue[0] ) );
		mh.kvColor[l->kvCount++] = mhAccent;
	} else if ( local == 1 && last > 1 && at[2] >= 0 ) {
		Q_strncpyz( mh.kvKey[l->kvCount], "LEAD", sizeof( mh.kvKey[0] ) );
		if ( mhGap[2] >= 0 ) MH_Gap( mhGap[2], mh.kvValue[l->kvCount], sizeof( mh.kvValue[0] ) );
		else Q_strncpyz( mh.kvValue[l->kvCount], "--", sizeof( mh.kvValue[0] ) );
		mh.kvColor[l->kvCount++] = mhFast;
	}
	/* the team race: whose colours we drive in */
	if ( MH_TEAMRACE() && ( MH_Team( ps->clientNum ) == TEAM_RED || MH_Team( ps->clientNum ) == TEAM_BLUE ) ) {
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
	l->orderTitle = MH_TEAMRACE() ? "Team race order" : "Race order";
	l->orderRight = "Gap";
	if ( cgs.gametype == GT_TEAM_RACING_DM ) {
		/* with weapons: the teams' frags, as the old HUD showed them */
		for ( i = 0; i < 2; i++ ) {
			Com_sprintf( mh.teamValue[i], sizeof( mh.teamValue[i] ), "%d", cg.teamScores[i] );
			l->teamValue[i] = mh.teamValue[i];
		}
		l->teamName[0] = "RED";
		l->teamName[1] = "BLUE";
		l->teamCount = 2;
		l->rowSwatch = qtrue;
	} else if ( cgs.gametype == GT_TEAM_RACING ) {
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
	for ( i = 0; i < MH_ROWS_MAX; i++ ) mh.rowColor[i] = mh.rowNameColor[i] = NULL;
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
				else if ( mhGap[position] >= 0 ) MH_Gap( mhGap[position], mh.rowValue[i], sizeof( mh.rowValue[i] ) );
				else Q_strncpyz( mh.rowValue[i], "--", sizeof( mh.rowValue[i] ) );
			}
			l->rowPos[i] = mh.rowPos[i];
			l->rowName[i] = mh.rowName[i];
			l->rowValue[i] = mh.rowValue[i];
		}
		l->rowCount = count;
	}

	if ( cgs.gametype == GT_ELIMINATION ) MH_Elimination( l, at, last, ps, cent );

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
	/* with weapons: the front and the rear weapon with their ammo first */
	l->slotCount = 0;
	if ( MH_COMBAT() ) {
		l->vehicleWidth = 330.0f;
		MH_Weapons( l, ps );
	}
	/* the trunk: each item in a slot, "USE" beside its icon */
	{
		int value = ps->stats[STAT_HOLDABLE_ITEM], mask = BG_TrunkMask( value ), items[TRUNK_ITEM_COUNT], n = 0, k;
		for ( i = 0; i < TRUNK_ITEM_COUNT; i++ ) {
			if ( mask & ( 1 << i ) ) items[n++] = BG_FindItemForHoldable( bg_trunkItems[i] ) - bg_itemlist;
		}
		if ( !n && ( value = BG_HoldableItemIndex( value ) ) != 0 ) items[n++] = value;
		for ( k = 0; k < n && l->slotCount < MH_SLOTS_MAX; k++ ) {
			i = l->slotCount++;
			CG_RegisterItemVisuals( items[k] );
			mh.slotIcon[i] = cg_items[items[k]].icon;
			mh.slotColor[i] = mhWhite;
			Q_strncpyz( mh.slotName[i], bg_itemlist[items[k]].pickup_name ? bg_itemlist[items[k]].pickup_name : "", sizeof( mh.slotName[i] ) );
			l->slotLabel[i] = "ITEM";
			l->slotValue[i] = "USE";
			l->slotName[i] = mh.slotName[i];
		}
	}
	MH_Kills( l );

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
	if ( l->elim ) {
		MH_Str( &l->elimLabelT, mhAccent, 1.0f );
		MH_Str( &l->elimValueT, mhAccent, 1.0f );
		MH_Bar( &l->elimBar, mh.elimFrac, mhAccent, mhTrack, 1.0f );
		MH_Str( &l->elimCaptionT, mhMuted, 1.0f );
	}
	for ( i = 0; i < l->kvCount; i++ ) {
		MH_Str( &l->kvKeyT[i], mhMuted, 1.0f );
		MH_Str( &l->kvValueT[i], mh.kvColor[i], 1.0f );
	}
}

static const vec4_t mhKillBack = { 13 / 255.0f, 13 / 255.0f, 13 / 255.0f, 0.62f };

static void MH_DrawKillFeed( const mhRaceLayout_t *l ) {
	int i;
	for ( i = 0; i < l->killCount; i++ ) {
		MH_FillRect( &l->killBox[i], mhKillBack, 1.0f );
		MH_Str( &l->killAT[i], mhWhite, 1.0f );
		MH_Str( &l->killWeaponT[i], mhAccent, 1.0f );
		MH_Str( &l->killBT[i], mhValue, 1.0f );
	}
}

void CG_ModernHUD_DrawStandings( void ) {
	const mhRaceLayout_t *l = &mh.l;
	int i;
	MH_Prepare();
	MH_DrawKillFeed( l );
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
		MH_Str( &l->rowNameT[i], mh.rowNameColor[i] ? mh.rowNameColor[i] : mhWhite, 1.0f );
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
			MH_Pic( &l->slotIcon[i], 0.0f, mh.slotIcon[i], mh.slotColor[i] ? mh.slotColor[i] : mhWhite, 1.0f );
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

/* a key's name as the controls menu shows it, for the hints */
static void MH_KeyName( int key, char *out, int size ) {
	static const struct { int key; const char *name; } names[] = {
		{ K_TAB, "TAB" }, { K_ENTER, "ENTER" }, { K_SPACE, "SPACE" }, { K_BACKSPACE, "BACKSPACE" },
		{ K_UPARROW, "UP" }, { K_DOWNARROW, "DOWN" }, { K_LEFTARROW, "LEFT" }, { K_RIGHTARROW, "RIGHT" },
		{ K_ALT, "ALT" }, { K_CTRL, "CTRL" }, { K_SHIFT, "SHIFT" }, { K_INS, "INS" }, { K_DEL, "DEL" },
		{ K_PGDN, "PGDN" }, { K_PGUP, "PGUP" }, { K_HOME, "HOME" }, { K_END, "END" },
		{ K_KP_INS, "KP INS" }, { K_KP_DEL, "KP DEL" }, { K_KP_ENTER, "KP ENTER" }, { K_KP_HOME, "KP 7" },
		{ K_KP_UPARROW, "KP 8" }, { K_KP_PGUP, "KP 9" }, { K_KP_LEFTARROW, "KP 4" }, { K_KP_5, "KP 5" },
		{ K_KP_RIGHTARROW, "KP 6" }, { K_KP_END, "KP 1" }, { K_KP_DOWNARROW, "KP 2" }, { K_KP_PGDN, "KP 3" },
		{ K_MOUSE1, "MOUSE 1" }, { K_MOUSE2, "MOUSE 2" }, { K_MOUSE3, "MOUSE 3" }, { K_MOUSE4, "MOUSE 4" },
		{ K_MOUSE5, "MOUSE 5" }, { K_MWHEELUP, "WHEEL UP" }, { K_MWHEELDOWN, "WHEEL DOWN" }
	};
	int i;
	if ( key >= K_F1 && key <= K_F12 ) {
		Com_sprintf( out, size, "F%d", key - K_F1 + 1 );
		return;
	}
	if ( key > 32 && key < 127 ) {
		Com_sprintf( out, size, "%c", toupper( key ) );
		return;
	}
	for ( i = 0; i < ARRAY_LEN( names ); i++ ) {
		if ( names[i].key == key ) {
			Q_strncpyz( out, names[i].name, size );
			return;
		}
	}
	Q_strncpyz( out, "REAR FIRE", size );	/* unbound or unnamed: the control's name */
}

void CG_ModernHUD_DrawSpectator( void ) {
	mhSpectatorLayout_t sp;
	char name[40], key[16], view[32];
	int watched;
	if ( !cg.snap || CG_IntroCam_IsActive() ) return;
	MH_Prepare();
	memset( &sp, 0, sizeof( sp ) );
	sp.viewWidth = mh.l.viewWidth;
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		MH_Plain( cgs.clientinfo[cg.snap->ps.clientNum].name, name, sizeof( name ) );
		sp.label = "FOLLOWING";
		sp.name = name;
	} else if ( cg.snap->ps.pm_flags & PMF_OBSERVE ) {
		/* the track cameras: the driver they turn to, if they turn */
		watched = cg.snap->ps.persistant[PERS_ATTACKER];
		if ( watched >= 0 && watched < MAX_CLIENTS && cgs.clientinfo[watched].infoValid )
			MH_Plain( cgs.clientinfo[watched].name, name, sizeof( name ) );
		else
			Q_strncpyz( name, "Fixed view", sizeof( name ) );
		sp.label = "TRACK CAMERA";
		sp.name = name;
	} else {
		sp.label = "SPECTATOR";
		sp.name = "Free camera";
	}
	sp.hint1 = "FIRE NEXT";
	MH_KeyName( trap_Key_GetKey( "+button12" ), key, sizeof( key ) );
	Com_sprintf( view, sizeof( view ), "%s VIEW", key );
	sp.hint2 = view;
	MH_SpectatorLayout( &sp );
	MH_Panel( &sp.box, qtrue, 1.0f );
	MH_Str( &sp.labelT, mhLabel, 1.0f );
	MH_Str( &sp.nameT, mhWhite, 1.0f );
	MH_Str( &sp.hint1T, mhMuted, 1.0f );
	MH_Str( &sp.hint2T, mhMuted, 1.0f );
}

/* elimination: someone is out. The drivers still in read their place and
 * that they are safe, everybody else how many are left. */
void CG_ModernHUD_Eliminated( int clientNum, int remaining ) {
	static const char *suffix[4] = { "TH", "ST", "ND", "RD" };
	char name[40];
	int me, place;
	if ( !cg.snap || clientNum < 0 || clientNum >= MAX_CLIENTS || !CG_ModernHUD_Active() ) return;
	MH_Plain( cgs.clientinfo[clientNum].name, name, sizeof( name ) );
	me = cg.snap->ps.clientNum;
	place = cg_entities[me].currentPosition;
	if ( !( cg.snap->ps.pm_flags & PMF_FOLLOW ) && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR &&
	     !cg_entities[me].eliminationOut && place > 0 ) {
		CG_CenterPrint( va( "%d%s \xb7 SAFE\n%s is out", place,
		                    suffix[( place % 100 >= 11 && place % 100 <= 13 ) || place % 10 > 3 ? 0 : place % 10], name ),
		                SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
	} else {
		CG_CenterPrint( va( "%s is out\n%d left", name, remaining ), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
	}
}
