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
static const vec4_t mhZoneFree  = { 58 / 255.0f, 58 / 255.0f, 58 / 255.0f, 1.0f };	/* #3a3a3a: a point nobody holds */

static struct {
	int				time;			/* the frame this was laid out for */
	qboolean		ready;
	mhRaceLayout_t	l;
	char			title[MH_TEXT_MAX], title2[48], pos[16], of[16], lap[16];
	char			progressLeft[24], progressRight[16];
	float			progressFrac;
	int				progressDone;
	char			kvKey[MH_KV_MAX][16], kvValue[MH_KV_MAX][32];
	const float		*kvColor[MH_KV_MAX];
	char			rowPos[MH_ROWS_MAX][8], rowName[MH_ROWS_MAX][40], rowValue[MH_ROWS_MAX][16];
	qboolean		rowMe[MH_ROWS_MAX];
	const float		*rowColor[MH_ROWS_MAX];		/* NULL: #ddd */
	const float		*rowNameColor[MH_ROWS_MAX];	/* NULL: white */
	float			rowFrac[MH_ROWS_MAX];
	qboolean		rowLow[MH_ROWS_MAX];			/* under 35: the bar in the accent */
	const float		*lapColor;					/* the right of the place line, NULL: label/white */
	int				derbyZone[4];				/* damage taken per zone: front, left, right, rear */
	int				derbyHitSeen, derbyMaxHealth;
	const float		*derbyStatusColor;
	char			elimValue[24], elimCaption[48];
	float			elimFrac;
	const float		*posColor, *posLabelColor;
	qboolean		flagAway[MH_FLAGS_MAX];		/* the state in the accent */
	const float		*zoneColor[MH_ZONES_MAX];	/* the owner's colour, mhZoneFree: nobody */
	const float		*zoneLineColor, *zoneBarColor;	/* the capture row, its bar */
	const float		*hillColor;					/* KOTH: the owner's colour, muted: nobody */
	char			bugLeft[8], bugRight[8], bugClock[16];
	const float		*treeColor[MH_TREE_ROWS][2];
	const float		*tachColor[MH_TACH_SEGMENTS];
	const float		*tachRpmColor, *tachMidColor, *tachLeftColor, *tachRightColor;
	char			tachRpm[16], tachGear[8], tachSpeed[16], tachMid[32], labelRowRight[24], labelRowLeft[40];
	qboolean		bugSubAccent;
	float			zoneFrac;
	char			zoneLabel[24], zoneValue[16];
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
	qhandle_t		note, prev, play, next, chevron, message, gun, mine, smoke, flag, hill, dot;
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
	         cgs.gametype == GT_ELIMINATION || cgs.gametype == GT_LCS || cgs.gametype == GT_DERBY ||
	         cgs.gametype == GT_DEATHMATCH || cgs.gametype == GT_TEAM || cgs.gametype == GT_CTF ||
	         cgs.gametype == GT_CTF4 || cgs.gametype == GT_DOMINATION || cgs.gametype == GT_KOTH ||
	         cgs.gametype == GT_AUTOBALL || cgs.gametype == GT_DRAG );
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

/* last car standing: one life each. Who is out, by the scores (sent every
 * two seconds), the entity and our own playerState. */
static qboolean MH_LastCarOut( int client ) {
	int i;
	if ( client == cg.snap->ps.clientNum )
		return cg.snap->ps.pm_type == PM_DEAD || cg.snap->ps.stats[STAT_HEALTH] <= 0;
	if ( cg_entities[client].currentState.eFlags & EF_DEAD ) return qtrue;
	/* the derby sets the finish time of a car that is out (the winner's at the end) */
	if ( cgs.gametype == GT_DERBY && cg_entities[client].finishRaceTime && !cg.intermissionStarted ) return qtrue;
	for ( i = 0; i < cg.numScores; i++ ) {
		if ( cg.scores[i].client == client ) return cg.scores[i].integrity == 0;
	}
	return qfalse;
}

/* integrity in percent, as the server sends it with the scores; -1: not yet */
static int MH_LastCarIntegrity( int client ) {
	const playerState_t *ps = &cg.snap->ps;
	int i;
	if ( client == ps->clientNum ) {
		if ( ps->stats[STAT_HEALTH] <= 0 ) return 0;
		/* capped like the server caps the others' (a bonus can lift health over the max) */
		i = ps->stats[STAT_MAX_HEALTH] > 0 ? ps->stats[STAT_HEALTH] * 100 / ps->stats[STAT_MAX_HEALTH] : ps->stats[STAT_HEALTH];
		return i > 100 ? 100 : i;
	}
	for ( i = 0; i < cg.numScores; i++ ) {
		if ( cg.scores[i].client == client ) return cg.scores[i].integrity;
	}
	return -1;
}

/* how many are still in, not counting `except` (-1: none) */
static int MH_LastCarAlive( int except ) {
	int i, n = 0;
	for ( i = 0; i < cgs.maxclients && i < MAX_CLIENTS; i++ ) {
		if ( i == except || !cgs.clientinfo[i].infoValid || cgs.clientinfo[i].team == TEAM_SPECTATOR ) continue;
		if ( !MH_LastCarOut( i ) ) n++;
	}
	return n;
}

static int MH_Team( int client );
static const float *MH_TeamColor( int team );

/* "1st" "2nd" "=3rd" (tied) */
static void MH_Ordinal( int rank, qboolean tied, char *out, int size ) {
	static const char *suffix[4] = { "th", "st", "nd", "rd" };
	int k = ( rank % 100 >= 11 && rank % 100 <= 13 ) || rank % 10 > 3 ? 0 : rank % 10;
	Com_sprintf( out, size, "%s%d%s", tied ? "=" : "", rank, suffix[k] );
}

/* the match clock: what is left of the time limit, or the time played */
static const char *MH_MatchClock( char *out, int size ) {
	int played = cg.time - cgs.levelStartTime, left;
	if ( cgs.timelimit > 0 ) {
		left = cgs.timelimit * 60000 - played;
		if ( left < 0 ) left = 0;
		Com_sprintf( out, size, "%d:%02d", left / 60000, ( left / 1000 ) % 60 );
		return "TIME LEFT";
	}
	if ( played < 0 ) played = 0;
	Com_sprintf( out, size, "%d:%02d", played / 60000, ( played / 1000 ) % 60 );
	return "TIME";
}

/* deathmatch: our frags against the limit and our rank, the clock and the
 * leader; the frag order from the scores (sent every two seconds) */
static void MH_Deathmatch( mhRaceLayout_t *l, const playerState_t *ps ) {
	int order[MAX_CLIENTS], count = 0, me = -1, i, client, first, rows, rank;
	const char *key;

	for ( i = 0; i < cg.numScores; i++ ) {
		client = cg.scores[i].client;
		if ( client < 0 || client >= MAX_CLIENTS || !cgs.clientinfo[client].infoValid ) continue;
		if ( cgs.clientinfo[client].team == TEAM_SPECTATOR ) continue;
		if ( client == ps->clientNum ) me = count;
		order[count++] = i;
	}
	if ( cgs.gametype == GT_AUTOBALL ) {
		/* by goals (the server's order, by points, among equals) */
		int k, swap;
		for ( i = 1; i < count; i++ ) {
			for ( k = i; k > 0 && cg.scores[order[k]].autoballGoals > cg.scores[order[k - 1]].autoballGoals; k-- ) {
				swap = order[k]; order[k] = order[k - 1]; order[k - 1] = swap;
			}
		}
		for ( i = 0, me = -1; i < count; i++ ) if ( cg.scores[order[i]].client == ps->clientNum ) me = i;
	}

	Com_sprintf( mh.pos, sizeof( mh.pos ), "%d", ps->persistant[PERS_SCORE] );
	l->pos = mh.pos;
	if ( cgs.fraglimit > 0 ) {
		Com_sprintf( mh.of, sizeof( mh.of ), "/ %d", cgs.fraglimit );
		l->of = mh.of;
	} else {
		l->of = "";
	}
	rank = ( ps->persistant[PERS_RANK] & ~RANK_TIED_FLAG ) + 1;
	MH_Ordinal( rank, ( ps->persistant[PERS_RANK] & RANK_TIED_FLAG ) != 0, mh.lap, sizeof( mh.lap ) );
	l->lapLabel = "RANK";
	l->lapValue = mh.lap;
	l->progress = qfalse;

	l->kvCount = 0;
	key = MH_MatchClock( mh.kvValue[0], sizeof( mh.kvValue[0] ) );
	Q_strncpyz( mh.kvKey[0], key, sizeof( mh.kvKey[0] ) );
	mh.kvColor[0] = mhWhite;
	l->kvCount = 1;
	if ( count > 0 ) {
		char name[32];
		MH_Plain( cgs.clientinfo[cg.scores[order[0]].client].name, name, sizeof( name ) );
		Q_strncpyz( mh.kvKey[1], "LEADER", sizeof( mh.kvKey[1] ) );
		Com_sprintf( mh.kvValue[1], sizeof( mh.kvValue[1] ), "%s \xb7 %d", name, cg.scores[order[0]].score );
		mh.kvColor[1] = mhWhite;
		l->kvCount = 2;
	}
	for ( i = 0; i < l->kvCount; i++ ) {
		l->kvKey[i] = mh.kvKey[i];
		l->kvValue[i] = mh.kvValue[i];
		l->kvSize[i] = 0.0f;
	}

	l->orderTitle = "Frag order";
	l->orderRight = "Frags";
	l->standings = cg_hudShowOpponentList.integer && count > 0 && !cg.showScores &&
	               cg.predictedPlayerState.pm_type != PM_INTERMISSION && !CG_IntroCam_IsActive();
	if ( !l->standings ) return;
	rows = count < MH_ROWS_MAX ? count : MH_ROWS_MAX;
	first = 0;
	if ( count > rows ) {
		first = ( me >= 0 ? me : 0 ) - rows / 2;
		if ( first < 0 ) first = 0;
		if ( first > count - rows ) first = count - rows;
	}
	for ( i = 0; i < rows; i++ ) {
		const score_t *sc = &cg.scores[order[first + i]];
		Com_sprintf( mh.rowPos[i], sizeof( mh.rowPos[0] ), "%02d", first + i + 1 );
		MH_Plain( cgs.clientinfo[sc->client].name, mh.rowName[i], sizeof( mh.rowName[0] ) );
		Com_sprintf( mh.rowValue[i], sizeof( mh.rowValue[0] ), "%d",
		             cgs.gametype == GT_CTF || cgs.gametype == GT_CTF4 ? sc->captures :
		             cgs.gametype == GT_AUTOBALL ? sc->autoballGoals : sc->score );
		mh.rowMe[i] = sc->client == ps->clientNum;
		mh.rowTeam[i] = MH_Team( sc->client );
		mh.rowColor[i] = mh.rowNameColor[i] = NULL;
		l->rowPos[i] = mh.rowPos[i];
		l->rowName[i] = mh.rowName[i];
		l->rowValue[i] = mh.rowValue[i];
	}
	l->rowCount = rows;
}

/* team deathmatch: on top of the deathmatch, our team in its colour and our
 * frags, the frag limit, and the teams' frags over the order */
static void MH_TeamDeathmatch( mhRaceLayout_t *l, const playerState_t *ps ) {
	int team = MH_Team( ps->clientNum ), i;

	if ( team == TEAM_RED || team == TEAM_BLUE ) {
		l->posLabel = "YOUR TEAM";
		l->pos = team == TEAM_RED ? "Red" : "Blue";
		l->posSize = 44.0f;
		l->of = "";
		mh.posColor = mh.posLabelColor = MH_TeamColor( team );
	}
	Com_sprintf( mh.lap, sizeof( mh.lap ), "%d", ps->persistant[PERS_SCORE] );
	l->lapLabel = "YOUR FRAGS";
	l->lapValue = mh.lap;

	/* the clock stays, the leader makes way for the limit */
	l->kvCount = 1;
	if ( cgs.fraglimit > 0 ) {
		Q_strncpyz( mh.kvKey[1], "LIMIT", sizeof( mh.kvKey[1] ) );
		Com_sprintf( mh.kvValue[1], sizeof( mh.kvValue[1] ), "%d", cgs.fraglimit );
		mh.kvColor[1] = mhWhite;
		l->kvKey[1] = mh.kvKey[1];
		l->kvValue[1] = mh.kvValue[1];
		l->kvCount = 2;
	}

	l->orderTitle = "Team frag order";
	for ( i = 0; i < 2; i++ ) {
		Com_sprintf( mh.teamValue[i], sizeof( mh.teamValue[i] ), "%d", cg.teamScores[i] );
		l->teamValue[i] = mh.teamValue[i];
	}
	l->teamName[0] = "RED";
	l->teamName[1] = "BLUE";
	l->teamCount = 2;
	l->rowSwatch = qtrue;
}

/* capture the flag (two or four teams): our team in a row, a line per
 * flag (at base, carried by whom, dropped and when it goes home), the
 * clock; the order by score with the captures, the teams' captures over it */
#define MH_FLAG_RETURN	30000	/* a dropped flag goes home after 30 s (g_items.c) */
static int mhFlagDropped[MH_FLAGS_MAX], mhFlagLast[MH_FLAGS_MAX];

static void MH_CaptureTheFlag( mhRaceLayout_t *l, const playerState_t *ps ) {
	static const char *names[MH_FLAGS_MAX] = { "Red", "Blue", "Green", "Yellow" };
	static const char *cells[2][MH_FLAGS_MAX] = { { "RED", "BLUE", "", "" }, { "R", "B", "G", "Y" } };
	static char notes[MH_FLAGS_MAX][32];
	int teams = cgs.gametype == GT_CTF4 ? 4 : 2, team = MH_Team( ps->clientNum ), status[MH_FLAGS_MAX], i, c, left;

	status[0] = cgs.redflag;
	status[1] = cgs.blueflag;
	status[2] = cgs.greenflag;
	status[3] = cgs.yellowflag;

	l->posLabel = "YOUR TEAM";
	l->pos = team >= TEAM_RED && team < TEAM_RED + teams ? names[team - TEAM_RED] : "--";
	l->headerRow = qtrue;
	l->posSize = 30.0f;
	l->of = NULL;
	l->lapValue = NULL;
	l->progress = qfalse;
	mh.posColor = mh.posLabelColor = MH_TeamColor( team );

	for ( i = 0; i < teams; i++ ) {
		/* when it was dropped, for the time until it goes home */
		if ( status[i] == 2 && mhFlagLast[i] != 2 ) mhFlagDropped[i] = cg.time;
		mhFlagLast[i] = status[i];
		notes[i][0] = '\0';
		if ( status[i] == 1 ) {
			/* the carrier, if we can see him (or it is us) */
			for ( c = 0; c < MAX_CLIENTS; c++ ) {
				const centity_t *ce = &cg_entities[c];
				int powerups = c == ps->clientNum ? ps->powerups[PW_REDFLAG + i] ? ( 1 << ( PW_REDFLAG + i ) ) : 0
				                                  : ( ce->currentValid ? ce->currentState.powerups : 0 );
				if ( cgs.clientinfo[c].infoValid && ( powerups & ( 1 << ( PW_REDFLAG + i ) ) ) ) {
					MH_Plain( cgs.clientinfo[c].name, notes[i], sizeof( notes[i] ) );
					break;
				}
			}
		} else if ( status[i] == 2 ) {
			left = MH_FLAG_RETURN - ( cg.time - mhFlagDropped[i] );
			if ( left < 0 ) left = 0;
			Com_sprintf( notes[i], sizeof( notes[i] ), "%d:%02d", left / 60000, ( left / 1000 ) % 60 );
		}
		l->flagName[i] = names[i];
		l->flagState[i] = status[i] == 1 ? "CARRIED" : status[i] == 2 ? "DROPPED" : "AT BASE";
		l->flagNote[i] = notes[i];
		mh.flagAway[i] = status[i] != 0;
	}
	l->flagCount = teams;

	/* the clock only: the flags are the story here */
	l->kvCount = 1;

	l->orderTitle = cgs.gametype == GT_CTF4 ? "CTF driver order" : "CTF order";
	l->orderRight = "Caps";
	for ( i = 0; i < teams; i++ ) {
		Com_sprintf( mh.teamValue[i], sizeof( mh.teamValue[i] ), "%d", cg.teamScores[i] );
		l->teamValue[i] = mh.teamValue[i];
		l->teamName[i] = cells[teams == 4][i];
	}
	l->teamCount = teams;
	l->rowSwatch = qtrue;
}

/* domination: a tile per control point in its owner's colour; under it the
 * point we are capturing (the server sends how far, STAT_DOM_CAPTURE), else
 * how many our team holds */
static void MH_Domination( mhRaceLayout_t *l, const playerState_t *ps ) {
	static const char *letters[MH_ZONES_MAX] = { "A", "B", "C", "D", "E" };
	static const char *names[MH_TEAMS_MAX] = { "RED", "BLUE", "GREEN", "YELLOW" };
	static const char *cells[MH_TEAMS_MAX] = { "R", "B", "G", "Y" };
	int team = MH_Team( ps->clientNum ), capture = ps->stats[STAT_DOM_CAPTURE], count = 0, held = 0, i;
	const float *mine = MH_TeamColor( team );
	static int lastCapture, lastCaptureTime, lastClient;

	/* the server sets the capture only in frames the car touches the point:
	 * keep it over the gaps while the car moves about on it */
	if ( capture ) {
		lastCapture = capture;
		lastCaptureTime = cg.time;
		lastClient = ps->clientNum;
	} else if ( lastClient == ps->clientNum && cg.time - lastCaptureTime < 600 && cg.time >= lastCaptureTime ) {
		capture = lastCapture;
	}

	for ( i = 0; i < MH_ZONES_MAX && i < MAX_SIGILS && cgs.sigil[i] != SIGIL_NONE; i++ ) {
		int owner = cgs.sigil[i] >= SIGIL_ISRED ? TEAM_RED + cgs.sigil[i] - SIGIL_ISRED : TEAM_FREE;
		l->zoneName[i] = letters[i];
		mh.zoneColor[i] = owner != TEAM_FREE ? MH_TeamColor( owner ) : mhZoneFree;
		if ( owner != TEAM_FREE && owner == team ) held++;
		count++;
	}
	l->zoneCount = count;
	/* taken in the meantime: the hold is over */
	if ( capture && DOM_CAPTURE_POINT( capture ) < count &&
	     cgs.sigil[DOM_CAPTURE_POINT( capture )] == SIGIL_ISRED + DOM_CAPTURE_TEAM( capture ) - TEAM_RED ) capture = 0;
	if ( capture && DOM_CAPTURE_POINT( capture ) < count ) {
		Com_sprintf( mh.zoneLabel, sizeof( mh.zoneLabel ), "CAPTURING %s", letters[DOM_CAPTURE_POINT( capture )] );
		Com_sprintf( mh.zoneValue, sizeof( mh.zoneValue ), "%d%%", DOM_CAPTURE_PERCENT( capture ) );
		mh.zoneFrac = DOM_CAPTURE_PERCENT( capture ) / 100.0f;
		mh.zoneLineColor = MH_TeamColor( DOM_CAPTURE_TEAM( capture ) );
	} else {
		Q_strncpyz( mh.zoneLabel, "POINTS HELD", sizeof( mh.zoneLabel ) );
		Com_sprintf( mh.zoneValue, sizeof( mh.zoneValue ), "%d / %d", held, count );
		mh.zoneFrac = count > 0 ? (float)held / count : 0.0f;
		mh.zoneLineColor = mine;
	}
	mh.zoneBarColor = mh.zoneLineColor;
	l->zoneLabel = mh.zoneLabel;
	l->zoneValue = mh.zoneValue;

	/* no score line: the points are the story; the team and the clock */
	l->pos = NULL;
	l->of = NULL;
	l->lapValue = NULL;
	l->progress = qfalse;
	Q_strncpyz( mh.kvValue[1], mh.kvValue[0], sizeof( mh.kvValue[1] ) );
	Q_strncpyz( mh.kvKey[1], mh.kvKey[0], sizeof( mh.kvKey[1] ) );
	mh.kvColor[1] = mhWhite;
	Q_strncpyz( mh.kvKey[0], "YOUR TEAM", sizeof( mh.kvKey[0] ) );
	Q_strncpyz( mh.kvValue[0], team >= TEAM_RED && team < TEAM_RED + MH_TEAMS_MAX ? names[team - TEAM_RED] : "--",
	            sizeof( mh.kvValue[0] ) );
	mh.kvColor[0] = mine;
	l->kvCount = 2;
	for ( i = 0; i < 2; i++ ) {
		l->kvKey[i] = mh.kvKey[i];
		l->kvValue[i] = mh.kvValue[i];
		l->kvSize[i] = 0.0f;
	}

	l->orderTitle = "Domination order";
	l->orderRight = "Score";
	for ( i = 0; i < MH_TEAMS_MAX; i++ ) {
		Com_sprintf( mh.teamValue[i], sizeof( mh.teamValue[i] ), "%d", cg.teamScores[i] );
		l->teamValue[i] = mh.teamValue[i];
		l->teamName[i] = cells[i];
	}
	l->teamCount = MH_TEAMS_MAX;
	l->rowSwatch = qtrue;
}

/* king of the hill: who holds the hill, what goes on there (capturing,
 * contested, held, empty) with the players of each team on it, the next
 * respawn wave and the clock; the order by points, the teams' over it */
static void MH_KingOfTheHill( mhRaceLayout_t *l, const playerState_t *ps ) {
	int owner = cgs.kothOwner, pct = Com_Clamp( 0, 100, cgs.kothCapturePct ), wave, i;
	int red = cgs.kothPresence[0], blue = cgs.kothPresence[1];
	qboolean capturing = !cgs.kothContested && ( cgs.kothCapturingTeam == TEAM_RED || cgs.kothCapturingTeam == TEAM_BLUE ) &&
	                     cgs.kothCapturingTeam != owner && pct > 0 && pct < 100;

	l->pos = NULL;
	l->of = NULL;
	l->lapValue = NULL;
	l->progress = qfalse;
	l->hillLabel = "HILL HELD BY";
	l->hillOwner = owner == TEAM_RED ? "Red" : owner == TEAM_BLUE ? "Blue" : "Nobody";
	mh.hillColor = owner == TEAM_RED || owner == TEAM_BLUE ? MH_TeamColor( owner ) : mhMuted;

	if ( red >= 0 && blue >= 0 ) Com_sprintf( mh.zoneValue, sizeof( mh.zoneValue ), "%d v %d", red, blue );
	else mh.zoneValue[0] = '\0';
	if ( cgs.kothContested ) {
		Q_strncpyz( mh.zoneLabel, "CONTESTED", sizeof( mh.zoneLabel ) );
		mh.zoneLineColor = mhAccent;
		mh.zoneFrac = 0.0f;
	} else if ( capturing ) {
		Q_strncpyz( mh.zoneLabel, "CAPTURING", sizeof( mh.zoneLabel ) );
		Com_sprintf( mh.zoneValue, sizeof( mh.zoneValue ), "%d%%", pct );
		mh.zoneLineColor = MH_TeamColor( cgs.kothCapturingTeam );
		mh.zoneFrac = pct / 100.0f;
	} else if ( owner == TEAM_RED || owner == TEAM_BLUE ) {
		qboolean there = ( owner == TEAM_RED ? red : blue ) != 0;
		Q_strncpyz( mh.zoneLabel, there ? "HOLDING" : "HILL EMPTY", sizeof( mh.zoneLabel ) );
		mh.zoneLineColor = there ? MH_TeamColor( owner ) : mhMuted;
		mh.zoneBarColor = MH_TeamColor( owner );	/* held all the same */
		mh.zoneFrac = 1.0f;
	} else {
		Q_strncpyz( mh.zoneLabel, "HILL EMPTY", sizeof( mh.zoneLabel ) );
		mh.zoneLineColor = mhMuted;
		mh.zoneFrac = 0.0f;
	}
	if ( !( owner == TEAM_RED || owner == TEAM_BLUE ) || cgs.kothContested || capturing ) mh.zoneBarColor = mh.zoneLineColor;
	l->zoneLabel = mh.zoneLabel;
	l->zoneValue = mh.zoneValue;

	/* the next respawn wave (waves come at whole multiples of the server
	 * time), then the clock */
	l->kvCount = 0;
	wave = cg_kothRespawnWave.integer;
	if ( wave > 0 && wave < 1000 ) wave *= 1000;
	Q_strncpyz( mh.kvKey[1], mh.kvKey[0], sizeof( mh.kvKey[1] ) );
	Q_strncpyz( mh.kvValue[1], mh.kvValue[0], sizeof( mh.kvValue[1] ) );
	if ( wave > 0 ) {
		int left = ( cg.snap->ps.pm_type == PM_DEAD && cg.kothRespawnAt > cg.time ) ? cg.kothRespawnAt - cg.time
		           : wave - ( cg.time % wave );
		Q_strncpyz( mh.kvKey[0], "RESPAWN WAVE", sizeof( mh.kvKey[0] ) );
		Com_sprintf( mh.kvValue[0], sizeof( mh.kvValue[0] ), "%d:%02d", ( left + 999 ) / 60000, ( ( left + 999 ) / 1000 ) % 60 );
		mh.kvColor[0] = ps->pm_type == PM_DEAD ? mhAccent : mhWhite;
		mh.kvColor[1] = mhWhite;
		l->kvCount = 2;
	} else {
		Q_strncpyz( mh.kvKey[0], mh.kvKey[1], sizeof( mh.kvKey[0] ) );
		Q_strncpyz( mh.kvValue[0], mh.kvValue[1], sizeof( mh.kvValue[0] ) );
		mh.kvColor[0] = mhWhite;
		l->kvCount = 1;
	}
	for ( i = 0; i < l->kvCount; i++ ) {
		l->kvKey[i] = mh.kvKey[i];
		l->kvValue[i] = mh.kvValue[i];
		l->kvSize[i] = 0.0f;
	}

	l->orderTitle = "KOTH driver order";
	l->orderRight = "Pts";
	for ( i = 0; i < 2; i++ ) {
		Com_sprintf( mh.teamValue[i], sizeof( mh.teamValue[i] ), "%d", cg.teamScores[i] );
		l->teamValue[i] = mh.teamValue[i];
	}
	l->teamName[0] = "RED";
	l->teamName[1] = "BLUE";
	l->teamCount = 2;
	l->rowSwatch = qtrue;
}

/* autoball: the score bug at the top (in place of the rear view), our team,
 * where the ball is, our goals and how long our team had the ball (the team
 * of the last touch, counted here while the ball is in play) */
static int mhPossession[2], mhPossessionTime;

static void MH_Autoball( mhRaceLayout_t *l, const playerState_t *ps ) {
	int team = MH_Team( ps->clientNum ), msec, seconds, i, touch, total;
	qboolean overtime = qfalse;
	centity_t *ball = CG_Autoball_FindBall();

	/* the score bug: the clock stands before the first whistle */
	msec = cg.time - cgs.levelStartTime;
	if ( msec < 0 || cgs.autoballState == AUTOBALL_STATE_WAITING ) msec = 0;
	if ( cgs.timelimit > 0 ) {
		msec = cgs.timelimit * 60000 - msec;
		if ( msec < 0 ) {
			overtime = qtrue;
			msec = -msec;
		}
	}
	seconds = msec / 1000;
	Com_sprintf( mh.bugClock, sizeof( mh.bugClock ), "%s%d:%02d", overtime ? "+" : "", seconds / 60, seconds % 60 );
	Com_sprintf( mh.bugLeft, sizeof( mh.bugLeft ), "%d", cgs.scores1 );
	Com_sprintf( mh.bugRight, sizeof( mh.bugRight ), "%d", cgs.scores2 );
	l->scorebug = qtrue;
	l->bugLeftLabel = "RED";
	l->bugRightLabel = "BLUE";
	l->bugLeft = mh.bugLeft;
	l->bugRight = mh.bugRight;
	l->bugClock = mh.bugClock;
	l->bugSub = overtime ? "OVERTIME" : cgs.autoballState == AUTOBALL_STATE_WAITING ? "WARM-UP" :
	            cgs.autoballState == AUTOBALL_STATE_KICKOFF ? "KICK-OFF" : cgs.autoballState == AUTOBALL_STATE_GOAL ? "GOAL" :
	            cgs.timelimit > 0 ? "TIME LEFT" : "MATCH";
	mh.bugSubAccent = overtime || cgs.autoballState == AUTOBALL_STATE_GOAL;

	/* possession: the team of the last touch, while the ball is in play */
	if ( cg.time > mhPossessionTime ) {
		touch = ball ? ( ball->currentState.generic1 & SCRIPTED_GENERIC1_TEAM_MASK ) >> SCRIPTED_GENERIC1_TEAM_SHIFT : 0;
		if ( mhPossessionTime && cgs.autoballState == AUTOBALL_STATE_LIVE && ( touch == TEAM_RED || touch == TEAM_BLUE ) &&
		     cg.time - mhPossessionTime < 1000 ) {
			mhPossession[touch - TEAM_RED] += cg.time - mhPossessionTime;
		}
		mhPossessionTime = cg.time;
	}

	/* our team in a row */
	l->posLabel = "YOUR TEAM";
	l->pos = team == TEAM_RED ? "Red" : team == TEAM_BLUE ? "Blue" : "--";
	l->headerRow = qtrue;
	l->posSize = 30.0f;
	l->of = NULL;
	l->lapValue = NULL;
	l->progress = qfalse;
	mh.posColor = mh.posLabelColor = MH_TeamColor( team );

	/* the ball: in whose half (by the goals), how far from us */
	Q_strncpyz( mh.kvKey[0], "BALL", sizeof( mh.kvKey[0] ) );
	if ( ball ) {
		float metres = Distance( ball->lerpOrigin, cg.predictedPlayerState.origin ) / CP_M_2_QU;
		if ( ps->clientNum != cg.predictedPlayerState.clientNum || cg.snap->ps.pm_flags & PMF_FOLLOW )
			metres = Distance( ball->lerpOrigin, cg_entities[ps->clientNum].lerpOrigin ) / CP_M_2_QU;
		if ( cgs.autoballHaveGoals ) {
			vec3_t axis, rel;
			float t;
			VectorSubtract( cgs.autoballGoal[1], cgs.autoballGoal[0], axis );
			VectorSubtract( ball->lerpOrigin, cgs.autoballGoal[0], rel );
			t = DotProduct( rel, axis ) / DotProduct( axis, axis );
			Com_sprintf( mh.kvValue[0], sizeof( mh.kvValue[0] ), "%s HALF \xb7 %d M", t < 0.5f ? "RED" : "BLUE", (int)metres );
		} else {
			Com_sprintf( mh.kvValue[0], sizeof( mh.kvValue[0] ), "%d M", (int)metres );
		}
	} else {
		Q_strncpyz( mh.kvValue[0], "--", sizeof( mh.kvValue[0] ) );
	}
	Q_strncpyz( mh.kvKey[1], "YOUR GOALS", sizeof( mh.kvKey[1] ) );
	Q_strncpyz( mh.kvValue[1], "0", sizeof( mh.kvValue[1] ) );
	for ( i = 0; i < cg.numScores; i++ ) {
		if ( cg.scores[i].client == ps->clientNum )
			Com_sprintf( mh.kvValue[1], sizeof( mh.kvValue[1] ), "%d", cg.scores[i].autoballGoals );
	}
	Q_strncpyz( mh.kvKey[2], "POSSESSION", sizeof( mh.kvKey[2] ) );
	total = mhPossession[0] + mhPossession[1];
	if ( total > 0 && ( team == TEAM_RED || team == TEAM_BLUE ) )
		Com_sprintf( mh.kvValue[2], sizeof( mh.kvValue[2] ), "%d%%", ( mhPossession[team - TEAM_RED] * 100 + total / 2 ) / total );
	else
		Q_strncpyz( mh.kvValue[2], "--", sizeof( mh.kvValue[2] ) );
	l->kvCount = 3;
	for ( i = 0; i < 3; i++ ) {
		l->kvKey[i] = mh.kvKey[i];
		l->kvValue[i] = mh.kvValue[i];
		l->kvSize[i] = 0.0f;
		mh.kvColor[i] = mhWhite;
	}

	l->orderTitle = "Autoball order";
	l->orderRight = "Goals";
	l->teamCount = 0;
	l->rowSwatch = qtrue;
}

/* drag: the time slip in the card (reaction, the splits, the finish, the
 * trap speed), the duel by lanes with the elapsed times, the tree under it
 * and the tachometer across the bottom. The splits are taken here: how far
 * the car is from where it stood staged, at 60 ft, 330 ft and 1/8 mile
 * (docs/drag-race-map.md); the finish and the trap are the server's. */
#define MH_DRAG_LAUNCH_MIN	4000	/* cg_hud_racing.c: the launch band */
#define MH_DRAG_LAUNCH_MAX	5000
#define MH_DRAG_SHIFT_RPM	5900
static const vec4_t mhDim       = { 119 / 255.0f, 119 / 255.0f, 119 / 255.0f, 1.0f };	/* #777: not yet */
static const vec4_t mhLampYellow = { 1.0f, 209 / 255.0f, 41 / 255.0f, 1.0f };			/* #ffd129 */
static const vec4_t mhLampAmberOff = { 58 / 255.0f, 50 / 255.0f, 32 / 255.0f, 1.0f };	/* #3a3220 */
static const vec4_t mhLampGreen = { 46 / 255.0f, 224 / 255.0f, 82 / 255.0f, 1.0f };		/* #2ee052 */
static const vec4_t mhLampGreenOff = { 28 / 255.0f, 58 / 255.0f, 36 / 255.0f, 1.0f };	/* #1c3a24 */
static const vec4_t mhSegWindowOff = { 42 / 255.0f, 60 / 255.0f, 77 / 255.0f, 1.0f };	/* #2a3c4d */
static const vec4_t mhSegLimitOff = { 74 / 255.0f, 36 / 255.0f, 18 / 255.0f, 1.0f };	/* #4a2412 */

static struct {
	int		client, heat;		/* whose run, which heat (its countdown start) */
	vec3_t	start;				/* where the car stood staged */
	float	lastDist;
	int		lastTime;
	int		split[3];			/* ms after the launch, 0: not yet */
} mhDragRun;

static void MH_DragTime( int ms, char *out, int size ) {
	Com_sprintf( out, size, "%d.%03d", ms / 1000, ms % 1000 );
}

static void MH_DragSplits( const centity_t *car, int client, const vec3_t origin ) {
	static const float feet[3] = { 60.0f, 330.0f, 660.0f };
	float dist;
	int i;
	if ( mhDragRun.client != client || mhDragRun.heat != car->dragCountdownTime ) {
		memset( &mhDragRun, 0, sizeof( mhDragRun ) );
		mhDragRun.client = client;
		mhDragRun.heat = car->dragCountdownTime;
		VectorCopy( origin, mhDragRun.start );
	}
	if ( car->dragPhase < DRAG_RUNNING || !car->dragLaunchTime ) {
		VectorCopy( origin, mhDragRun.start );	/* still on the line */
		mhDragRun.lastDist = 0.0f;
		mhDragRun.lastTime = cg.time;
		return;
	}
	if ( car->dragPhase != DRAG_RUNNING || cg.time <= mhDragRun.lastTime ) return;
	dist = sqrt( ( origin[0] - mhDragRun.start[0] ) * ( origin[0] - mhDragRun.start[0] ) +
	             ( origin[1] - mhDragRun.start[1] ) * ( origin[1] - mhDragRun.start[1] ) );
	for ( i = 0; i < 3; i++ ) {
		float mark = feet[i] * 0.3048f * CP_M_2_QU;
		if ( !mhDragRun.split[i] && dist >= mark && mhDragRun.lastDist < mark && dist > mhDragRun.lastDist ) {
			/* between the last frame and this one, where it crossed */
			int t = mhDragRun.lastTime + (int)( ( mark - mhDragRun.lastDist ) / ( dist - mhDragRun.lastDist ) *
			                                    ( cg.time - mhDragRun.lastTime ) );
			mhDragRun.split[i] = t - car->dragLaunchTime > 0 ? t - car->dragLaunchTime : 1;
		}
	}
	mhDragRun.lastDist = dist;
	mhDragRun.lastTime = cg.time;
}

/* the two drivers of the duel, the one in the left lane first: from where
 * the cars stand across the first driver's heading */
static int MH_DragLanes( int *out ) {
	int count = 0, i, swap;
	if ( cgs.dragMatch.count == 2 ) {
		out[0] = cgs.dragMatch.client[0];
		out[1] = cgs.dragMatch.client[1];
		count = 2;
	} else {
		for ( i = 0; i < MAX_CLIENTS && count < 2; i++ ) {
			if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team != TEAM_SPECTATOR ) out[count++] = i;
		}
	}
	if ( count == 2 ) {
		vec3_t right, delta, angles;
		const centity_t *a = &cg_entities[out[0]], *b = &cg_entities[out[1]];
		VectorSet( angles, 0.0f, a->lerpAngles[YAW], 0.0f );
		AngleVectors( angles, NULL, right, NULL );
		VectorSubtract( b->lerpOrigin, a->lerpOrigin, delta );
		if ( DotProduct( delta, right ) < 0.0f ) {
			swap = out[0]; out[0] = out[1]; out[1] = swap;
		}
	}
	return count;
}

/* a driver's result in the duel: the elapsed time, or why there is none */
static const char *MH_DragResult( const centity_t *car, char *out, int size, const float **color ) {
	*color = NULL;
	if ( car->dragPhase == DRAG_FALSE_START ) { *color = mhAccent; return "FOUL"; }
	if ( car->dragPhase == DRAG_DNF ) { *color = mhAccent; return "DNF"; }
	if ( car->dragPhase == DRAG_FINISHED && car->dragFinishTime > car->dragLaunchTime ) {
		MH_DragTime( car->dragFinishTime - car->dragLaunchTime, out, size );
		return out;
	}
	*color = mhDim;
	return "--";
}

static void MH_DragThousands( int value, char *out, int size ) {
	if ( value >= 1000 ) Com_sprintf( out, size, "%d,%03d", value / 1000, value % 1000 );
	else Com_sprintf( out, size, "%d", value );
}

static void MH_Drag( mhRaceLayout_t *l, const playerState_t *ps ) {
	static const char *slip[MH_KV_MAX] = { "REACTION", "60 FT", "330 FT", "1/8 MILE", "1/4 MILE", "TRAP" };
	static char rows[2][24];
	const centity_t *car = &cg_entities[ps->clientNum];
	const vec_t *origin = ps->clientNum == cg.predictedPlayerState.clientNum && !( cg.snap->ps.pm_flags & PMF_FOLLOW )
	                      ? cg.predictedPlayerState.origin : car->lerpOrigin;
	int lanes[2], count, i, k, rpm, gear, reds, greenTime, launch;
	qboolean green, launchGuide, running = car->dragPhase == DRAG_RUNNING || car->dragPhase == DRAG_FINISHED;
	qboolean stressed, blown, shift;
	float speed;

	MH_DragSplits( car, ps->clientNum, origin );
	count = MH_DragLanes( lanes );

	/* the card: who, which lane; then the time slip */
	l->pos = NULL;
	l->of = NULL;
	l->lapValue = NULL;
	l->progress = qfalse;
	if ( ps->clientNum == cg.predictedPlayerState.clientNum && !( cg.snap->ps.pm_flags & PMF_FOLLOW ) )
		Q_strncpyz( mh.labelRowLeft, "YOU", sizeof( mh.labelRowLeft ) );
	else
		MH_Plain( cgs.clientinfo[ps->clientNum].name, mh.labelRowLeft, sizeof( mh.labelRowLeft ) );
	mh.labelRowRight[0] = '\0';
	if ( count == 2 && ( lanes[0] == ps->clientNum || lanes[1] == ps->clientNum ) )
		Q_strncpyz( mh.labelRowRight, lanes[0] == ps->clientNum ? "LANE LEFT" : "LANE RIGHT", sizeof( mh.labelRowRight ) );
	l->labelRowLeft = mh.labelRowLeft;
	l->labelRowRight = mh.labelRowRight;

	for ( i = 0; i < MH_KV_MAX; i++ ) {
		Q_strncpyz( mh.kvKey[i], slip[i], sizeof( mh.kvKey[i] ) );
		Q_strncpyz( mh.kvValue[i], "--", sizeof( mh.kvValue[i] ) );
		mh.kvColor[i] = mhDim;
	}
	launch = running && car->dragGreenTime > 0 && car->dragLaunchTime >= car->dragGreenTime;
	if ( car->dragPhase == DRAG_FALSE_START ) {
		Q_strncpyz( mh.kvValue[0], "FOUL", sizeof( mh.kvValue[0] ) );
		mh.kvColor[0] = mhAccent;
	} else if ( launch ) {
		MH_DragTime( car->dragLaunchTime - car->dragGreenTime, mh.kvValue[0], sizeof( mh.kvValue[0] ) );
		mh.kvColor[0] = mhWhite;
	}
	for ( i = 0; i < 3; i++ ) {
		if ( launch && mhDragRun.split[i] ) {
			MH_DragTime( mhDragRun.split[i], mh.kvValue[1 + i], sizeof( mh.kvValue[0] ) );
			mh.kvColor[1 + i] = mhWhite;
		}
	}
	if ( car->dragPhase == DRAG_DNF ) {
		Q_strncpyz( mh.kvValue[4], "DNF", sizeof( mh.kvValue[4] ) );
		mh.kvColor[4] = mhAccent;
	} else if ( car->dragPhase == DRAG_FINISHED && car->dragFinishTime > car->dragLaunchTime ) {
		MH_DragTime( car->dragFinishTime - car->dragLaunchTime, mh.kvValue[4], sizeof( mh.kvValue[4] ) );
		mh.kvColor[4] = mhWhite;
		if ( car->dragTrapSpeed > 0 ) {
			/* tenths of km/h */
			if ( cg_metricUnits.integer )
				Com_sprintf( mh.kvValue[5], sizeof( mh.kvValue[5] ), "%d.%d KM/H", car->dragTrapSpeed / 10, car->dragTrapSpeed % 10 );
			else
				Com_sprintf( mh.kvValue[5], sizeof( mh.kvValue[5] ), "%.1f MPH", car->dragTrapSpeed * 0.0621371f );
			mh.kvColor[5] = mhWhite;
		}
	}
	l->kvCount = MH_KV_MAX;
	for ( i = 0; i < MH_KV_MAX; i++ ) {
		l->kvKey[i] = mh.kvKey[i];
		l->kvValue[i] = mh.kvValue[i];
		l->kvSize[i] = 0.0f;
	}

	/* the duel */
	if ( cgs.dragMatch.bestOf > 1 && cgs.dragMatch.heat ) {
		Com_sprintf( mh.title2, sizeof( mh.title2 ), "Duel \xb7 Heat %d/%d", cgs.dragMatch.heat, cgs.dragMatch.bestOf );
		l->orderTitle = mh.title2;
	} else {
		l->orderTitle = count == 2 ? "Duel" : "Practice";
	}
	l->orderRight = "ET";
	l->teamCount = 0;
	l->rowSwatch = qfalse;
	l->rowCount = count;
	for ( i = 0; i < count; i++ ) {
		const float *color;
		const char *result = MH_DragResult( &cg_entities[lanes[i]], rows[i], sizeof( rows[i] ), &color );
		Q_strncpyz( mh.rowPos[i], count == 2 ? ( i ? "R" : "L" ) : "", sizeof( mh.rowPos[0] ) );
		MH_Plain( cgs.clientinfo[lanes[i]].name, mh.rowName[i], sizeof( mh.rowName[0] ) );
		Q_strncpyz( mh.rowValue[i], result, sizeof( mh.rowValue[0] ) );
		mh.rowMe[i] = lanes[i] == ps->clientNum;
		mh.rowColor[i] = color;
		mh.rowNameColor[i] = NULL;
		l->rowPos[i] = mh.rowPos[i];
		l->rowName[i] = mh.rowName[i];
		l->rowValue[i] = mh.rowValue[i];
	}
	l->standings = cg_hudShowOpponentList.integer && count > 0 && !cg.showScores &&
	               cg.predictedPlayerState.pm_type != PM_INTERMISSION && !CG_IntroCam_IsActive();

	/* the tree: a column per lane; PRE when staged, STAGE once the heat is
	 * armed, the ambers shared, GO green (red after a false start) */
	CG_DragStartLightState( &reds, &green, &greenTime );
	l->tree = !cg.showScores && cg.predictedPlayerState.pm_type != PM_INTERMISSION && !CG_IntroCam_IsActive();
	for ( i = 0; i < MH_TREE_ROWS; i++ ) l->treeLabel[i] = i == 0 ? "PRE" : i == 1 ? "STAGE" : i == 5 ? "GO" : "";
	for ( k = 0; k < 2; k++ ) {
		const centity_t *d = k < count ? &cg_entities[lanes[k]] : NULL;
		int phase = d ? d->dragPhase : DRAG_WAITING;
		qboolean inHeat = d && phase >= DRAG_STAGED;
		mh.treeColor[0][k] = inHeat ? mhLampYellow : mhLampAmberOff;
		mh.treeColor[1][k] = d && phase >= DRAG_COUNTDOWN ? mhLampYellow : mhLampAmberOff;
		for ( i = 0; i < 3; i++ ) mh.treeColor[2 + i][k] = d && !green && phase >= DRAG_COUNTDOWN && reds > i ? mhLampYellow : mhLampAmberOff;
		mh.treeColor[5][k] = phase == DRAG_FALSE_START ? mhTeam[0] : d && green && phase >= DRAG_COUNTDOWN ? mhLampGreen : mhLampGreenOff;
	}

	/* the tachometer */
	rpm = Com_Clamp( 0, CP_RPM_MAX, ps->stats[STAT_RPM] );
	gear = ps->stats[STAT_GEAR];
	launchGuide = car->dragPhase < DRAG_RUNNING && gear == 0;
	stressed = ( ps->stats[STAT_DRAG_ENGINE] & DRAG_ENGINE_STRESS_MASK ) > 0;
	blown = ( ps->stats[STAT_DRAG_ENGINE] & DRAG_ENGINE_BLOWN ) != 0;
	shift = !launchGuide && gear > 0 && gear < 6 && rpm >= MH_DRAG_SHIFT_RPM && !blown && car->dragPhase < DRAG_FINISHED;
	l->tach = qtrue;
	MH_DragThousands( rpm, mh.tachRpm, sizeof( mh.tachRpm ) );
	l->tachRpm = mh.tachRpm;
	if ( gear < 0 ) Q_strncpyz( mh.tachGear, "R", sizeof( mh.tachGear ) );
	else if ( gear == 0 ) Q_strncpyz( mh.tachGear, "N", sizeof( mh.tachGear ) );
	else Com_sprintf( mh.tachGear, sizeof( mh.tachGear ), "%d", gear );
	l->tachGear = mh.tachGear;
	speed = Q3VelocityToRL( sqrt( ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1] ) );
	Com_sprintf( mh.tachSpeed, sizeof( mh.tachSpeed ), "%d", (int)( speed + 0.5f ) );
	l->tachSpeed = mh.tachSpeed;
	l->tachUnit = cg_metricUnits.integer ? "KM/H" : "MPH";
	/* the middle: the launch window while on the line, then what goes on */
	mh.tachMidColor = mhFast;
	if ( blown ) {
		l->tachMidLabel = "ENGINE BLOWN";
		l->tachMidValue = "";
		mh.tachMidColor = mhAccent;
	} else if ( launchGuide ) {
		l->tachMidLabel = car->dragPhase == DRAG_FALSE_START ? "FALSE START" :
		                  car->dragPhase == DRAG_COUNTDOWN && green ? "SELECT FIRST" :
		                  car->dragPhase == DRAG_COUNTDOWN || car->dragPhase == DRAG_STAGED ? "LAUNCH WINDOW \xb7 WAIT FOR GREEN" :
		                  "LAUNCH WINDOW \xb7 GAS IN N TO STAGE";
		Com_sprintf( mh.tachMid, sizeof( mh.tachMid ), "%d,%03d - %d,%03d", MH_DRAG_LAUNCH_MIN / 1000, MH_DRAG_LAUNCH_MIN % 1000,
		             MH_DRAG_LAUNCH_MAX / 1000, MH_DRAG_LAUNCH_MAX % 1000 );
		l->tachMidValue = mh.tachMid;
		if ( car->dragPhase == DRAG_FALSE_START ) mh.tachMidColor = mhAccent;
		else if ( car->dragPhase == DRAG_COUNTDOWN && green ) mh.tachMidColor = mhLampGreen;
	} else if ( car->dragPhase == DRAG_FINISHED ) {
		l->tachMidLabel = "FINISHED";
		l->tachMidValue = "";
		mh.tachMidColor = mhWhite;
	} else if ( car->dragPhase == DRAG_DNF || car->dragPhase == DRAG_FALSE_START ) {
		l->tachMidLabel = car->dragPhase == DRAG_DNF ? "DID NOT FINISH" : "FALSE START";
		l->tachMidValue = "";
		mh.tachMidColor = mhAccent;
	} else if ( stressed && rpm >= DRAG_ENGINE_REDLINE_RPM ) {
		l->tachMidLabel = "OVER-REV";
		l->tachMidValue = "ENGINE HEATING";
		mh.tachMidColor = mhAccent;
	} else if ( shift ) {
		l->tachMidLabel = "SHIFT UP";
		l->tachMidValue = "";
		mh.tachMidColor = mhLampGreen;
	} else {
		l->tachMidLabel = car->dragPhase == DRAG_RUNNING ? "RUNNING" : "";
		l->tachMidValue = "";
		mh.tachMidColor = mhLabel;
	}
	mh.tachRpmColor = rpm >= DRAG_ENGINE_REDLINE_RPM ? mhAccent : launchGuide && rpm >= MH_DRAG_LAUNCH_MIN && rpm <= MH_DRAG_LAUNCH_MAX ? mhFast : mhWhite;
	l->tachLeft = "IDLE";
	l->tachRight = "REV LIMIT \xb7 ENGINE BLOWS";
	mh.tachLeftColor = mhMuted;
	/* the engine heating in the red: the warning blinks */
	mh.tachRightColor = stressed && !blown && ( cg.time / 150 ) & 1 ? mhWhite : mhAccent;
	for ( i = 0; i < MH_TACH_SEGMENTS; i++ ) {
		int segRpm = ( i + 1 ) * CP_RPM_MAX / MH_TACH_SEGMENTS, lo = i * CP_RPM_MAX / MH_TACH_SEGMENTS;
		qboolean lit = rpm > lo;
		if ( lo >= DRAG_ENGINE_REDLINE_RPM ) mh.tachColor[i] = lit ? mhAccent : mhSegLimitOff;
		else if ( segRpm > MH_DRAG_LAUNCH_MIN && lo < MH_DRAG_LAUNCH_MAX ) mh.tachColor[i] = lit ? mhFast : mhSegWindowOff;
		else mh.tachColor[i] = lit ? mhLit : mhTrack;
	}
}

/* derby: the score, the time left to ram someone before the game takes the
 * car out, and the car from above. The server has one integrity for the
 * car; the zones show where the hits landed (cg_playerstate.c works out
 * the zone of each hit), added up here since the car was whole. */
static const char *mhZoneName[4] = { "FRONT", "LEFT", "RIGHT", "REAR" };

static int MH_DerbyZoneState( int zone, int maxHealth ) {
	if ( mh.derbyZone[zone] * 100 >= maxHealth * 30 ) return 2;	/* critical */
	if ( mh.derbyZone[zone] * 100 >= maxHealth * 12 ) return 1;	/* worn */
	return 0;
}

static void MH_Derby( mhRaceLayout_t *l, const playerState_t *ps, const centity_t *cent ) {
	static char status[24];
	int seconds = ps->stats[STAT_DERBY_NORAM], maxHealth = ps->stats[STAT_MAX_HEALTH] > 0 ? ps->stats[STAT_MAX_HEALTH] : 100;
	int zone, worst = -1, worstState = 0, state;

	Com_sprintf( mh.pos, sizeof( mh.pos ), "%d", ps->persistant[PERS_SCORE] );
	l->posLabel = "SCORE";
	l->pos = mh.pos;
	l->posSize = 56.0f;
	l->of = "";
	if ( cent->startRaceTime && seconds > 0 ) Com_sprintf( mh.lap, sizeof( mh.lap ), "%d:%02d", seconds / 60, seconds % 60 );
	else Q_strncpyz( mh.lap, "--", sizeof( mh.lap ) );
	l->lapLabel = "NO RAM";
	l->lapValue = mh.lap;
	mh.lapColor = mhAccent;

	/* the hits on our car since it was whole */
	if ( ps->stats[STAT_HEALTH] >= maxHealth || ps->pm_type == PM_SPECTATOR ) memset( mh.derbyZone, 0, sizeof( mh.derbyZone ) );
	if ( cg.derbyHitFxTime && cg.derbyHitFxTime != mh.derbyHitSeen ) {
		mh.derbyHitSeen = cg.derbyHitFxTime;
		if ( cg.derbyHitFxDir >= 0 && cg.derbyHitFxDir < 4 ) mh.derbyZone[cg.derbyHitFxDir] += cg.derbyHitFxDamage;
		else for ( zone = 0; zone < 4; zone++ ) mh.derbyZone[zone] += cg.derbyHitFxDamage / 4;
	}
	for ( zone = 0; zone < 4; zone++ ) {
		state = MH_DerbyZoneState( zone, maxHealth );
		if ( worst < 0 || state > worstState || ( state == worstState && mh.derbyZone[zone] > mh.derbyZone[worst] ) ) {
			worst = zone;
			worstState = state;
		}
	}
	if ( worstState == 2 ) Com_sprintf( status, sizeof( status ), "%s CRITICAL", mhZoneName[worst] );
	else if ( worstState == 1 ) Com_sprintf( status, sizeof( status ), "%s WORN", mhZoneName[worst] );
	else Q_strncpyz( status, "NO DAMAGE", sizeof( status ) );
	l->derbyStatus = status;
	mh.derbyStatusColor = worstState == 2 ? mhAccent : worstState == 1 ? mhSoft : mhMuted;
	mh.derbyMaxHealth = maxHealth;
}

/* an SVG stroke: `width` wide, centred on the rectangle's edge */
static void MH_Outline( const wsRect_t *r, const float *color, float width ) {
	float h = width * 0.5f;
	MH_Fill( r->x - h, r->y - h, r->w + width, width, color, 1.0f );
	MH_Fill( r->x - h, r->y + r->h - h, r->w + width, width, color, 1.0f );
	MH_Fill( r->x - h, r->y + h, width, r->h - width, color, 1.0f );
	MH_Fill( r->x + r->w - h, r->y + h, width, r->h - width, color, 1.0f );
}

/* the car from above, in the design's 110x170 view box */
static void MH_DrawDerbyCar( const mhRaceLayout_t *l ) {
	static const float zones[4][4] = { { 31, 14, 48, 16 }, { 13, 40, 10, 90 }, { 87, 40, 10, 90 }, { 31, 140, 48, 16 } };
	static const vec4_t worn = { 122 / 255.0f, 122 / 255.0f, 122 / 255.0f, 1.0f };	/* #7a7a7a */
	static const vec4_t line = { 90 / 255.0f, 90 / 255.0f, 90 / 255.0f, 1.0f };		/* #5a5a5a */
	const wsRect_t *c = &l->derbyCar;
	const float *legend[3] = { mhSoft, worn, mhAccent };
	wsRect_t r;
	int zone, i, hitAge = cg.time - cg.derbyHitFxTime;

	r.x = c->x + 25; r.y = c->y + 10; r.w = 60; r.h = 150;
	MH_Outline( &r, line, 2.0f );
	r.x = c->x + 36; r.y = c->y + 52; r.w = 38; r.h = 56;
	MH_Outline( &r, line, 2.0f );
	for ( zone = 0; zone < 4; zone++ ) {
		float alpha = 1.0f;
		r.x = c->x + zones[zone][0]; r.y = c->y + zones[zone][1]; r.w = zones[zone][2]; r.h = zones[zone][3];
		/* the zone just hit blinks for two seconds */
		if ( cg.derbyHitFxTime && hitAge >= 0 && hitAge < 2200 && ( cg.derbyHitFxDir == zone || cg.derbyHitFxDir < 0 ) )
			alpha = ( ( cg.time / 150 ) & 1 ) ? 0.35f : 1.0f;
		MH_FillRect( &r, legend[MH_DerbyZoneState( zone, mh.derbyMaxHealth )], alpha );
	}
	MH_Str( &l->derbyStatusT, mh.derbyStatusColor, 1.0f );
	for ( i = 0; i < 3; i++ ) {
		MH_FillRect( &l->derbySwatch[i], legend[i], 1.0f );
		MH_Str( &l->derbyLegendT[i], mhLabel, 1.0f );
	}
}

/* the card: cars left and the score, the match time and the leader; the
 * order: everyone's integrity as a bar, those still in first */
static void MH_LastCar( mhRaceLayout_t *l, const playerState_t *ps, const centity_t *cent ) {
	static const vec4_t outName = { 119 / 255.0f, 119 / 255.0f, 119 / 255.0f, 1.0f };
	int order[MAX_CLIENTS], count = 0, alive = 0, pass, i, client, row, time;

	/* the server's order (by integrity), those still in first */
	for ( pass = 0; pass < 2; pass++ ) {
		for ( i = 0; i < cg.numScores; i++ ) {
			client = cg.scores[i].client;
			if ( client < 0 || client >= MAX_CLIENTS || !cgs.clientinfo[client].infoValid ) continue;
			if ( cgs.clientinfo[client].team == TEAM_SPECTATOR ) continue;
			if ( MH_LastCarOut( client ) != ( pass == 1 ) ) continue;
			order[count++] = client;
			if ( !pass ) alive++;
		}
	}

	l->progress = qfalse;
	l->kvCount = 0;
	if ( cgs.gametype == GT_DERBY ) {
		MH_Derby( l, ps, cent );
	} else {
		Com_sprintf( mh.pos, sizeof( mh.pos ), "%d / %d", alive, count );
		l->posLabel = "LEFT";
		l->pos = mh.pos;
		l->posSize = 56.0f;
		l->of = "";
		Com_sprintf( mh.lap, sizeof( mh.lap ), "%d", ps->persistant[PERS_SCORE] );
		l->lapLabel = "SCORE";
		l->lapValue = mh.lap;
	}
	if ( cg_hudShowTimes.integer && cgs.gametype == GT_LCS ) {
		if ( cent->finishRaceTime ) time = cent->finishRaceTime - cent->startRaceTime;
		else if ( cent->startRaceTime && cg.time > cent->startRaceTime ) time = cg.time - cent->startRaceTime;
		else time = 0;
		Q_strncpyz( mh.kvKey[l->kvCount], "MATCH", sizeof( mh.kvKey[0] ) );
		MH_Time( time, mh.kvValue[l->kvCount], sizeof( mh.kvValue[0] ) );
		mh.kvColor[l->kvCount++] = mhWhite;
	}
	if ( alive > 0 && cgs.gametype == GT_LCS ) {
		Q_strncpyz( mh.kvKey[l->kvCount], "LEADER", sizeof( mh.kvKey[0] ) );
		MH_Plain( cgs.clientinfo[order[0]].name, mh.kvValue[l->kvCount], sizeof( mh.kvValue[0] ) );
		mh.kvColor[l->kvCount++] = mhWhite;
	}
	for ( i = 0; i < l->kvCount; i++ ) {
		l->kvKey[i] = mh.kvKey[i];
		l->kvValue[i] = mh.kvValue[i];
		l->kvSize[i] = 0.0f;
	}

	l->orderTitle = cgs.gametype == GT_DERBY ? "Derby order" : "Still in";
	l->orderRight = "Integrity";
	l->rowBars = qtrue;
	/* no race order here: the scores decide */
	l->standings = ( cgs.gametype == GT_DERBY ? cg_hudShowDerbyList.integer : cg_hudShowOpponentList.integer ) &&
	               count > 0 && !cg.showScores &&
	               cg.predictedPlayerState.pm_type != PM_INTERMISSION && !CG_IntroCam_IsActive();
	if ( !l->standings ) return;
	for ( row = 0; row < count && row < MH_ROWS_MAX; row++ ) {
		int integrity = MH_LastCarIntegrity( order[row] );
		qboolean out = row >= alive;
		mh.rowPos[row][0] = '\0';
		MH_Plain( cgs.clientinfo[order[row]].name, mh.rowName[row], sizeof( mh.rowName[0] ) );
		if ( out ) Q_strncpyz( mh.rowValue[row], "OUT", sizeof( mh.rowValue[0] ) );
		else if ( integrity < 0 ) Q_strncpyz( mh.rowValue[row], "--", sizeof( mh.rowValue[0] ) );
		else Com_sprintf( mh.rowValue[row], sizeof( mh.rowValue[0] ), "%d%%", integrity );
		mh.rowFrac[row] = out || integrity < 0 ? 0.0f : WS_Clamp( 0.0f, integrity / 100.0f, 1.0f );
		mh.rowLow[row] = integrity >= 0 && integrity < 28;	/* the design's 35 of 125 */
		mh.rowColor[row] = out ? mhMuted : NULL;
		mh.rowNameColor[row] = out ? outName : NULL;
		mh.rowMe[row] = order[row] == ps->clientNum;
		l->rowPos[row] = mh.rowPos[row];
		l->rowName[row] = mh.rowName[row];
		l->rowValue[row] = mh.rowValue[row];
	}
	l->rowCount = row;
}

/* the race with weapons, and the races in teams */
#define MH_COMBAT()		( cgs.gametype == GT_RACING_DM || cgs.gametype == GT_TEAM_RACING_DM || cgs.gametype == GT_LCS || \
						  cgs.gametype == GT_DEATHMATCH || cgs.gametype == GT_TEAM || \
						  cgs.gametype == GT_CTF || cgs.gametype == GT_CTF4 || cgs.gametype == GT_DOMINATION || cgs.gametype == GT_KOTH || \
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
	/* last car standing and derby: one life, so every death is someone out */
	if ( ( cgs.gametype == GT_LCS || cgs.gametype == GT_DERBY ) && cg.snap && CG_ModernHUD_Active() ) {
		char name[40];
		int left = MH_LastCarAlive( target );
		MH_Plain( cgs.clientinfo[target].name, name, sizeof( name ) );
		if ( target == cg.snap->ps.clientNum && !( cg.snap->ps.pm_flags & PMF_FOLLOW ) )
			CG_CenterPrint( va( "You are out\n%d left", left ), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
		else
			CG_CenterPrint( va( "%s is out\n%d left", name, left ), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
	}
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
	mh.flag = trap_R_RegisterShaderNoMip( "menu/art/hud_flag" );
	mh.hill = trap_R_RegisterShaderNoMip( "menu/art/hud_hill" );
	mh.dot = trap_R_RegisterShaderNoMip( "menu/art/hud_dot" );
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
	mh.lapColor = NULL;
	mh.posLabelColor = NULL;
	l->viewWidth = cgs.glconfig.vidWidth / mhScale;

	last = MH_RaceOrder( at, &local );
	MH_Gaps( at, last );
	racers = cgs.numRacers > 0 ? cgs.numRacers : ( last > 0 ? last : 1 );

	/* the mode card */
	map = CG_ConfigString( CS_MESSAGE );
	if ( !map || !map[0] ) map = COM_SkipPath( cgs.mapname );
	MH_Plain( map, mh.rowName[0], sizeof( mh.rowName[0] ) );
	COM_StripExtension( mh.rowName[0], mh.rowName[0], sizeof( mh.rowName[0] ) );
	Com_sprintf( mh.title, sizeof( mh.title ), "%s \xb7 %s",
	             cgs.gametype == GT_GHOST ? "Ghost race" : cgs.gametype == GT_SPRINT ? "Sprint" :
	             cgs.gametype == GT_TEAM_RACING ? "Team race" : cgs.gametype == GT_RACING_DM ? "Race combat" :
	             cgs.gametype == GT_TEAM_RACING_DM ? "Team race combat" :
	             cgs.gametype == GT_ELIMINATION ? "Elimination" : cgs.gametype == GT_LCS ? "Last car standing" :
	             cgs.gametype == GT_DERBY ? "Derby" : cgs.gametype == GT_DEATHMATCH ? "Deathmatch" :
	             cgs.gametype == GT_TEAM ? "Team combat" : cgs.gametype == GT_CTF ? "Capture the flag" :
	             cgs.gametype == GT_CTF4 ? "Four team CTF" : cgs.gametype == GT_DOMINATION ? "Domination" :
	             cgs.gametype == GT_KOTH ? "King of the hill" : cgs.gametype == GT_AUTOBALL ? "Autoball" : cgs.gametype == GT_DRAG ? "Drag" : "Race",
	             mh.rowName[0] );
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
	if ( cgs.gametype == GT_LCS || cgs.gametype == GT_DERBY ) MH_LastCar( l, ps, cent );
	if ( cgs.gametype == GT_DEATHMATCH || cgs.gametype == GT_TEAM ) MH_Deathmatch( l, ps );
	if ( cgs.gametype == GT_TEAM ) MH_TeamDeathmatch( l, ps );
	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF4 ) {
		MH_Deathmatch( l, ps );
		MH_CaptureTheFlag( l, ps );
	}
	if ( cgs.gametype == GT_DOMINATION ) {
		MH_Deathmatch( l, ps );
		MH_Domination( l, ps );
	}
	if ( cgs.gametype == GT_KOTH ) {
		MH_Deathmatch( l, ps );
		MH_KingOfTheHill( l, ps );
	}
	if ( cgs.gametype == GT_AUTOBALL ) {
		MH_Deathmatch( l, ps );
		MH_Autoball( l, ps );
	}
	if ( cgs.gametype == GT_DRAG ) MH_Drag( l, ps );

	/* the top centre */
	l->mirror = cg_drawRearView.integer && MH_Views() && !cg.scoreBoardShowing && !l->scorebug && !l->tach;
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
	/* three slots need the wider card (the boards: 330 with three, 300 with two) */
	if ( l->slotCount >= 3 ) l->vehicleWidth = 330.0f;
	MH_Kills( l );

	/* bottom right */
	l->minimap = cg_drawMMap.integer && MH_Views() && !l->tach;	/* drag: the tachometer in its place */
	l->minimapSize = MH_MINIMAP * WS_Clamp( 0.4f, cg_mmap_size.value / MH_MINIMAP_DEFAULT, 1.7f );
	factor = 0.0f;
	mh.slipAlpha = 0.0f;
	if ( cg_hudShowSlipstream.integer && cg.predictedPlayerState.stats[STAT_HEALTH] > 0 &&
	     cg.predictedPlayerState.pm_type == PM_NORMAL ) {
		factor = BG_SlipstreamFactor( &cg.predictedPlayerState );
	}
	if ( factor > 0.0f && !l->tach ) {
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
		if ( l->posLabel ) MH_Str( &l->posLabelT, mh.posLabelColor ? mh.posLabelColor : mhLabel, 1.0f );
		MH_Str( &l->posT, mh.posColor, 1.0f );
		MH_Str( &l->ofT, mhMuted, 1.0f );
	}
	if ( l->lapValue ) {
		MH_Str( &l->lapLabelT, mh.lapColor ? mh.lapColor : mhLabel, 1.0f );
		MH_Str( &l->lapValueT, mh.lapColor ? mh.lapColor : mhWhite, 1.0f );
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
	if ( l->labelRowLeft ) {
		MH_Str( &l->labelRowLeftT, mhLabel, 1.0f );
		MH_Str( &l->labelRowRightT, mhMuted, 1.0f );
	}
	if ( l->hillOwner ) {
		MH_Pic( &l->hillIcon, 0.0f, mh.hill, mh.hillColor, 1.0f );
		MH_Str( &l->hillLabelT, mhLabel, 1.0f );
		MH_Str( &l->hillOwnerT, mh.hillColor, 1.0f );
	}
	if ( l->zoneCount || l->zoneLabel ) {
		for ( i = 0; i < l->zoneCount; i++ ) {
			MH_FillRect( &l->zoneTile[i], mh.zoneColor[i], 1.0f );
			MH_Str( &l->zoneNameT[i], mh.zoneColor[i] == mhZoneFree ? mhValue : mhOnTeam, 1.0f );
		}
		MH_Str( &l->zoneLabelT, mh.zoneLineColor, 1.0f );
		MH_Str( &l->zoneValueT, mh.zoneLineColor, 1.0f );
		MH_Bar( &l->zoneBar, mh.zoneFrac, mh.zoneBarColor, mhTrack, 1.0f );
	}
	if ( l->pos || l->lapValue || l->progress || l->zoneCount || l->zoneLabel || l->labelRowLeft ) MH_FillRect( &l->sep, mhLine, 1.0f );
	if ( l->flagCount ) {
		for ( i = 0; i < l->flagCount; i++ ) {
			MH_Pic( &l->flagIcon[i], 0.0f, mh.flag, mhTeam[i], 1.0f );
			MH_Str( &l->flagNameT[i], mhWhite, 1.0f );
			MH_Str( &l->flagStateT[i], mh.flagAway[i] ? mhAccent : mhValue, 1.0f );
			MH_Str( &l->flagNoteT[i], mhMuted, 1.0f );
		}
		MH_FillRect( &l->sep2, mhLine, 1.0f );
	}
	if ( l->derbyStatus ) MH_DrawDerbyCar( l );
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

/* drag: the tree under the standings */
static void MH_DrawTree( const mhRaceLayout_t *l ) {
	int i, k;
	MH_Panel( &l->treeBox, qtrue, 1.0f );
	MH_Str( &l->treeTitleT, mhLabel, 1.0f );
	for ( i = 0; i < MH_TREE_ROWS; i++ ) {
		for ( k = 0; k < 2; k++ ) MH_Pic( &l->treeLamp[i][k], 0.0f, mh.dot, mh.treeColor[i][k], 1.0f );
		MH_Str( &l->treeLabelT[i], mhMuted, 1.0f );
	}
}

/* drag: the tachometer across the bottom */
static void MH_DrawTach( const mhRaceLayout_t *l ) {
	int i;
	MH_Panel( &l->tachBox, qtrue, 1.0f );
	MH_Str( &l->tachRpmLabelT, mhLabel, 1.0f );
	MH_Str( &l->tachRpmT, mh.tachRpmColor, 1.0f );
	MH_Str( &l->tachMidLabelT, mh.tachMidColor, 1.0f );
	MH_Str( &l->tachMidValueT, mh.tachMidColor, 1.0f );
	MH_Str( &l->tachGearLabelT, mhLabel, 1.0f );
	MH_Str( &l->tachGearT, mhWhite, 1.0f );
	MH_Str( &l->tachUnitT, mhLabel, 1.0f );
	MH_Str( &l->tachSpeedT, mhWhite, 1.0f );
	for ( i = 0; i < MH_TACH_SEGMENTS; i++ ) MH_FillRect( &l->tachSeg[i], mh.tachColor[i], 1.0f );
	MH_Str( &l->tachLeftT, mh.tachLeftColor, 1.0f );
	MH_Str( &l->tachRightT, mh.tachRightColor, 1.0f );
}

void CG_ModernHUD_DrawStandings( void ) {
	const mhRaceLayout_t *l = &mh.l;
	int i;
	MH_Prepare();
	MH_DrawKillFeed( l );
	if ( l->tree ) MH_DrawTree( l );
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
		if ( l->rowBars ) {
			/* integrity: orange when low; the value at the right of its column */
			mhText_t value = l->rowValueT[i];
			MH_Bar( &l->rowBar[i], mh.rowFrac[i], mh.rowLow[i] ? mhAccent : mhSoft, mhTrack, 1.0f );
			value.box.x += value.box.w - WS_TextWidth( &value.style, value.text, strlen( value.text ) );
			MH_Str( &value, mh.rowColor[i] ? mh.rowColor[i] : mhValue, 1.0f );
			continue;
		}
		MH_Str( &l->rowValueT[i], mh.rowColor[i] ? mh.rowColor[i] : mhValue, 1.0f );
	}
}

static void MH_DrawToast( const mhRaceLayout_t *l ) {
	const float *tone = mh.toast.tone == 1 || mh.toast.tone == 3 ? mhFast : ( mh.toast.tone == 2 ? mhMuted : mhAccent );
	float a = mh.toast.alpha;
	qhandle_t icon = mh.toast.icon;
	wsRect_t box = l->toastBox;

	MH_FillRect( &box, mhPanel, a );
	MH_Border( &box, mhLine, 2.0f, tone, a );
	MH_Border( &l->toastIconBox, mhLine, 1.0f, NULL, a );
	if ( icon ) {
		MH_Pic( &l->toastIconBox, 8.0f, icon, mhWhite, a );
	} else {
		MH_Pic( &l->toastIconBox, 11.0f, mh.toast.tone >= 2 ? mh.message : mh.chevron, tone, a );
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
/* domination: the control points in the scene. In view the marker sits
 * over the point, out of view at the edge towards it; either way in the
 * band between the panels at the top and at the bottom */
#define MH_MARKER_TOP		270.0f
#define MH_MARKER_BOTTOM	650.0f
#define MH_MARKER_SIDE		50.0f
#define MH_MARKER_NEAR		8.0f	/* metres */

/* out of view: at the side it is on, the further round behind us the
 * lower, so the markers do not pile up */
static void MH_MarkerSide( float left, float forward, float w, float *sx, float *sy ) {
	float bearing = fabs( atan2( left, forward ) ) * 180.0f / M_PI, half = cg.refdef.fov_x * 0.5f;
	float t = half < 179.0f ? ( bearing - half ) / ( 180.0f - half ) : 1.0f;
	*sx = left > 0.0f ? MH_MARKER_SIDE : w - MH_MARKER_SIDE;
	*sy = MH_MARKER_TOP + WS_Clamp( 0.0f, t, 1.0f ) * ( MH_MARKER_BOTTOM - MH_MARKER_TOP );
}

/* a marker: a dark plate, the tile, its text, the distance under it */
static void MH_DrawMarker( mhZoneMarker_t *m, const float *tile, const float *text ) {
	wsRect_t back;
	MH_ZoneMarkerLayout( m );
	back.x = m->distanceT.box.x < m->tile.x ? m->distanceT.box.x : m->tile.x;
	back.w = m->distanceT.box.w > m->tile.w ? m->distanceT.box.w : m->tile.w;
	back.x -= 4.0f;
	back.w += 8.0f;
	back.y = m->tile.y - 4.0f;
	back.h = m->distanceT.box.y + m->distanceT.box.h + 3.0f - back.y;
	MH_FillRect( &back, mhPanel, 1.0f );
	MH_FillRect( &m->tile, tile, 1.0f );
	MH_Str( &m->letterT, text, 1.0f );
	MH_Str( &m->distanceT, mhValue, 1.0f );
}

/* autoball: the ball out of view (in view cg_autoball.c puts brackets
 * round it), in the colour of the team that touched it last */
static void MH_DrawBallMarker( const mhRaceLayout_t *l ) {
	centity_t *ball;
	mhZoneMarker_t m;
	vec3_t delta;
	float forward, left, up, sx, sy, w = l->viewWidth;
	float focalY = MH_DESIGN_HEIGHT * 0.5f / tan( cg.refdef.fov_y * M_PI / 360.0f );
	char distance[16];
	int touch;

	if ( !cg_autoballIndicator.integer || cg.showScores || cg.snap->ps.pm_type == PM_INTERMISSION ||
	     cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ||
	     cg.snap->ps.stats[STAT_HEALTH] <= 0 ) return;
	ball = CG_Autoball_FindBall();
	if ( !ball ) return;
	VectorSubtract( ball->lerpOrigin, cg.refdef.vieworg, delta );
	forward = DotProduct( delta, cg.refdef.viewaxis[0] );
	left = DotProduct( delta, cg.refdef.viewaxis[1] );
	up = DotProduct( delta, cg.refdef.viewaxis[2] );
	if ( forward > 1.0f ) {
		/* in view as cg_autoball.c has it (24px in from a 640x480 screen) */
		sx = 320.0f - left / forward * ( 320.0f / tan( cg.refdef.fov_x * M_PI / 360.0f ) );
		sy = 240.0f - up / forward * ( 240.0f / tan( cg.refdef.fov_y * M_PI / 360.0f ) );
		if ( sx > 24.0f && sx < 616.0f && sy > 24.0f && sy < 456.0f ) return;
	}
	MH_MarkerSide( left, forward, w, &sx, &sy );
	if ( forward > 1.0f ) sy = WS_Clamp( MH_MARKER_TOP, MH_DESIGN_HEIGHT * 0.5f - up / forward * focalY, MH_MARKER_BOTTOM );
	touch = ( ball->currentState.generic1 & SCRIPTED_GENERIC1_TEAM_MASK ) >> SCRIPTED_GENERIC1_TEAM_SHIFT;
	Com_sprintf( distance, sizeof( distance ), "%d m", (int)( VectorLength( delta ) / CP_M_2_QU ) );
	m.x = sx;
	m.y = sy;
	m.letter = "BALL";
	m.distance = distance;
	MH_DrawMarker( &m, touch == TEAM_RED || touch == TEAM_BLUE ? MH_TeamColor( touch ) : mhLit, mhOnTeam );
}

static void MH_DrawZoneMarkers( const mhRaceLayout_t *l ) {
	static const char *letters[MH_ZONES_MAX] = { "A", "B", "C", "D", "E" };
	static char distance[MH_ZONES_MAX][16];
	mhZoneMarker_t m[MH_ZONES_MAX], swap;
	float range[MH_ZONES_MAX], r;
	int owner[MH_ZONES_MAX], count = 0, i, k, o;
	float w = l->viewWidth, cx = w * 0.5f;
	float focalX = w * 0.5f / tan( cg.refdef.fov_x * M_PI / 360.0f );
	float focalY = MH_DESIGN_HEIGHT * 0.5f / tan( cg.refdef.fov_y * M_PI / 360.0f );

	if ( cgs.gametype != GT_DOMINATION || !cg_sigilLocator.integer || cg.showScores ||
	     cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.pm_type == PM_INTERMISSION ) return;
	for ( i = 0; i < cg.snap->numEntities && count < MH_ZONES_MAX; i++ ) {
		const centity_t *cent = &cg_entities[cg.snap->entities[i].number];
		const entityState_t *es = &cent->currentState;
		vec3_t point, delta;
		float forward, left, up, sx, sy;
		int index = es->generic1 - 1;

		if ( es->eType != ET_ITEM || index < 0 || index >= MH_ZONES_MAX ||
		     es->modelindex <= 0 || es->modelindex >= bg_numItems || bg_itemlist[es->modelindex].giType != IT_SIGIL ) continue;
		VectorCopy( cent->lerpOrigin, point );
		point[2] += 48.0f;
		VectorSubtract( point, cg.refdef.vieworg, delta );
		forward = DotProduct( delta, cg.refdef.viewaxis[0] );
		left = DotProduct( delta, cg.refdef.viewaxis[1] );
		up = DotProduct( delta, cg.refdef.viewaxis[2] );
		sx = forward > 1.0f ? cx - left / forward * focalX : -1.0f;
		sy = forward > 1.0f ? MH_DESIGN_HEIGHT * 0.5f - up / forward * focalY : -1.0f;
		if ( forward <= 1.0f || sx < MH_MARKER_SIDE || sx > w - MH_MARKER_SIDE ) MH_MarkerSide( left, forward, w, &sx, &sy );
		range[count] = VectorLength( delta );
		if ( range[count] < MH_MARKER_NEAR * CP_M_2_QU ) continue;	/* we are on it: the card says it */
		o = cgs.sigil[index];
		owner[count] = o >= SIGIL_ISRED ? TEAM_RED + o - SIGIL_ISRED : TEAM_FREE;
		Com_sprintf( distance[count], sizeof( distance[0] ), "%d m", (int)( range[count] / CP_M_2_QU ) );
		m[count].x = sx;
		m[count].y = WS_Clamp( MH_MARKER_TOP, sy, MH_MARKER_BOTTOM );
		m[count].letter = letters[index];
		m[count].distance = distance[count];
		count++;
	}
	/* far ones first: the nearer point lies on top */
	for ( i = 1; i < count; i++ ) {
		for ( k = i; k > 0 && range[k] > range[k - 1]; k-- ) {
			swap = m[k]; m[k] = m[k - 1]; m[k - 1] = swap;
			r = range[k]; range[k] = range[k - 1]; range[k - 1] = r;
			o = owner[k]; owner[k] = owner[k - 1]; owner[k - 1] = o;
		}
	}
	for ( i = 0; i < count; i++ ) {
		MH_DrawMarker( &m[i], owner[i] != TEAM_FREE ? MH_TeamColor( owner[i] ) : mhZoneFree,
		               owner[i] != TEAM_FREE ? mhOnTeam : mhValue );
	}
}

void CG_ModernHUD_Draw( void ) {
	const mhRaceLayout_t *l = &mh.l;
	MH_Prepare();
	if ( !CG_IntroCam_IsActive() ) {
		MH_DrawZoneMarkers( l );
		if ( cgs.gametype == GT_AUTOBALL ) MH_DrawBallMarker( l );
	}
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
	if ( l->scorebug ) {
		MH_Border( &l->bugBox, mhLine, 1.0f, NULL, 1.0f );
		MH_FillRect( &l->bugCell[0], mhTeam[0], 1.0f );
		MH_FillRect( &l->bugCell[1], mhPanel, 1.0f );
		MH_FillRect( &l->bugCell[2], mhTeam[1], 1.0f );
		MH_Str( &l->bugLeftLabelT, mhOnTeam, 1.0f );
		MH_Str( &l->bugLeftT, mhOnTeam, 1.0f );
		MH_Str( &l->bugRightT, mhOnTeam, 1.0f );
		MH_Str( &l->bugRightLabelT, mhOnTeam, 1.0f );
		MH_Str( &l->bugClockT, mh.bugSubAccent ? mhAccent : mhWhite, 1.0f );
		MH_Str( &l->bugSubT, mh.bugSubAccent ? mhAccent : mhMuted, 1.0f );
	}
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
	if ( l->tach ) MH_DrawTach( l );
	else MH_DrawCockpit( l );
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
	if ( cgs.gametype == GT_DRAG && Q_stricmp( cg.countDownPrint, "GO!" ) ) return qfalse;	/* the tree has the lights */
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

/* autoball: "BLUE SCORES!" in the team's colour, the scorer and the ball's
 * speed under it; three seconds, then a second to fade */
static qboolean MH_AutoballGoal( void ) {
	static char line[MH_TEXT_MAX];
	const char *sub[1];
	int elapsed;
	float alpha;
	char name[40];

	if ( cgs.gametype != GT_AUTOBALL || cgs.autoballState != AUTOBALL_STATE_GOAL ) return qfalse;
	elapsed = cg.time - cgs.autoballGoalTime;
	if ( elapsed < 0 ) elapsed = 0;
	alpha = elapsed < 3000 ? 1.0f : 1.0f - ( elapsed - 3000 ) / 1000.0f;
	if ( alpha <= 0.0f ) return qfalse;
	if ( cgs.autoballScorer >= 0 && cgs.autoballScorer < MAX_CLIENTS && cgs.clientinfo[cgs.autoballScorer].infoValid ) {
		MH_Plain( cgs.clientinfo[cgs.autoballScorer].name, name, sizeof( name ) );
	} else {
		Q_strncpyz( name, cgs.autoballScorer == -2 ? "Own goal" : "", sizeof( name ) );
	}
	if ( cg_metricUnits.integer )
		Com_sprintf( line, sizeof( line ), "%s%s%d KM/H", name, name[0] ? " \xb7 " : "", cgs.autoballGoalSpeed );
	else
		Com_sprintf( line, sizeof( line ), "%s%s%d MPH", name, name[0] ? " \xb7 " : "", (int)( cgs.autoballGoalSpeed * 0.621371f + 0.5f ) );
	sub[0] = line;
	MH_DrawMessage( cgs.autoballGoalTeam == TEAM_BLUE ? "BLUE SCORES!" : "RED SCORES!", 72.0f,
	                MH_TeamColor( cgs.autoballGoalTeam == TEAM_BLUE ? TEAM_BLUE : TEAM_RED ), 1, sub, alpha );
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
	if ( MH_AutoballGoal() ) return;
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
	if ( mh.l.tach ) sp.bottom = mh.l.tachBox.y - 10.0f;	/* over the tachometer */
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

/* "OUT OF AMMO" / "LOW AMMO WARNING" under the mirror, in the label face */
void CG_ModernHUD_DrawAmmoWarning( const char *text, qboolean out ) {
	wsTextStyle_t style;
	mhText_t t;
	if ( !cg.snap ) return;
	MH_Prepare();
	memset( &t, 0, sizeof( t ) );
	MH_Label( &style, 14.0f );
	t.style = style;
	Q_strncpyz( t.text, text, sizeof( t.text ) );
	t.box.w = WS_TextWidth( &t.style, t.text, strlen( t.text ) );
	t.box.h = WS_ContentHeight( &t.style );
	t.box.x = ( mh.l.viewWidth - t.box.w ) * 0.5f;
	t.box.y = 128.0f;
	MH_ShadowStr( &t, out ? mhAccent : mhValue, 1.0f );
}
