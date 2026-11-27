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

#define MH_CHAT_INPUT_MAX	256		/* the engine's MAX_EDIT_LINE */
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
	int				chatLine[MH_CHAT_MAX];	/* the layout's chat lines: which of mhChat */
	qboolean		chatTyping;
	int				chatInputMode;
	char			chatInputLabel[48], chatInputText[MH_CHAT_INPUT_MAX];
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
	qhandle_t		note, prev, play, next, chevron, message, gun, mine, smoke, flag, hill, dot, trophy;
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

/* the splits of a run, measured here from where the car stood staged (the
 * server sends only the launch and the finish): for every car in the duel,
 * written into the heat log for the end screen */
typedef struct {
	int		heat;				/* the heat (its countdown start) */
	vec3_t	start;				/* where the car stood staged */
	float	lastDist;
	int		lastTime;
	int		split[3];			/* ms after the launch, 0: not yet */
} mhDragRun_t;
static mhDragRun_t mhDragRuns[MAX_CLIENTS];

static void MH_DragTime( int ms, char *out, int size ) {
	Com_sprintf( out, size, "%d.%03d", ms / 1000, ms % 1000 );
}

static const mhDragRun_t *MH_DragSplits( const centity_t *car, int client, const vec3_t origin ) {
	static const float feet[3] = { 60.0f, 330.0f, 660.0f };
	mhDragRun_t *run = &mhDragRuns[client];
	float dist;
	int i;
	if ( run->heat != car->dragCountdownTime ) {
		memset( run, 0, sizeof( *run ) );
		run->heat = car->dragCountdownTime;
		VectorCopy( origin, run->start );
	}
	if ( car->dragPhase < DRAG_RUNNING || !car->dragLaunchTime ) {
		VectorCopy( origin, run->start );	/* still on the line */
		run->lastDist = 0.0f;
		run->lastTime = cg.time;
		return run;
	}
	if ( car->dragPhase != DRAG_RUNNING || cg.time <= run->lastTime ) return run;
	dist = sqrt( ( origin[0] - run->start[0] ) * ( origin[0] - run->start[0] ) +
	             ( origin[1] - run->start[1] ) * ( origin[1] - run->start[1] ) );
	for ( i = 0; i < 3; i++ ) {
		float mark = feet[i] * 0.3048f * CP_M_2_QU;
		if ( !run->split[i] && dist >= mark && run->lastDist < mark && dist > run->lastDist ) {
			/* between the last frame and this one, where it crossed */
			int t = run->lastTime + (int)( ( mark - run->lastDist ) / ( dist - run->lastDist ) *
			                               ( cg.time - run->lastTime ) );
			run->split[i] = t - car->dragLaunchTime > 0 ? t - car->dragLaunchTime : 1;
			CG_DragLogSplit( &cgs.dragLog, client, car->dragCountdownTime, i, run->split[i] );
		}
	}
	run->lastDist = dist;
	run->lastTime = cg.time;
	return run;
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

	const mhDragRun_t *run = MH_DragSplits( car, ps->clientNum, origin );
	count = MH_DragLanes( lanes );
	for ( i = 0; i < count; i++ ) {	/* the other lane too, for the end screen */
		if ( lanes[i] != ps->clientNum ) MH_DragSplits( &cg_entities[lanes[i]], lanes[i], cg_entities[lanes[i]].lerpOrigin );
	}

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
		if ( launch && run->split[i] ) {
			MH_DragTime( run->split[i], mh.kvValue[1 + i], sizeof( mh.kvValue[0] ) );
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
/* in DERBY_HIT_SEG_* order, as cg.derbyHitFxDir: front, left, right, rear */
static const char *mhZoneName[4] = { "FRONT", "LEFT", "RIGHT", "REAR" };

/* the zone of our car a hit lit, -1: no side (all four) */
static int MH_DerbyZoneOfDir( int dir ) {
	return dir >= DERBY_HIT_SEG_FRONT && dir <= DERBY_HIT_SEG_REAR ? dir : -1;
}

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
		if ( MH_DerbyZoneOfDir( cg.derbyHitFxDir ) >= 0 ) mh.derbyZone[MH_DerbyZoneOfDir( cg.derbyHitFxDir )] += cg.derbyHitFxDamage;
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
	static const float zones[4][4] = { { 31, 14, 48, 16 }, { 13, 40, 10, 90 }, { 87, 40, 10, 90 }, { 31, 140, 48, 16 } };	/* DERBY_HIT_SEG_* */
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
		if ( cg.derbyHitFxTime && hitAge >= 0 && hitAge < 2200 &&
		     ( MH_DerbyZoneOfDir( cg.derbyHitFxDir ) == zone || MH_DerbyZoneOfDir( cg.derbyHitFxDir ) < 0 ) )
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

/* the combat modes' match, written down for the end screen (cg_combat_log.c) */
static cgCombatLog_t mhCombat;
static int mhCombatStart = -1, mhCombatEnd;

/* the objective modes: the team deathmatch's page with what the teams played for */
static qboolean MH_EndIsObjective( void ) {
	return cgs.gametype == GT_CTF || cgs.gametype == GT_CTF4 || cgs.gametype == GT_DOMINATION || cgs.gametype == GT_KOTH ||
	       cgs.gametype == GT_AUTOBALL;
}

static qboolean MH_EndIsCombat( void ) {
	return cgs.gametype == GT_DEATHMATCH || cgs.gametype == GT_TEAM || cgs.gametype == GT_DERBY || cgs.gametype == GT_LCS ||
	       MH_EndIsObjective();
}

/* the kills are written down here too: the races with weapons */
static qboolean MH_CombatLogged( void ) {
	return MH_EndIsCombat() || cgs.gametype == GT_RACING_DM || cgs.gametype == GT_TEAM_RACING_DM || cgs.gametype == GT_ELIMINATION;
}

/* Domination: the zones you took (the one you were taking became your team's) */
static int mhDomCaptures, mhDomCapZone = -1, mhDomCapTeam, mhDomCapAt;

/* a new match: a new log */
static void MH_CombatCheck( void ) {
	if ( mhCombatStart == cgs.levelStartTime ) return;
	mhCombatStart = cgs.levelStartTime;
	mhCombatEnd = 0;
	CG_CombatReset( &mhCombat, cgs.levelStartTime );
	mhDomCaptures = 0;
	mhDomCapZone = -1;
}

/* every frame (cg_view.c): who holds the hill or the zones */
void CG_ModernHUD_ObjectiveFrame( void ) {
	int owner[CG_COMBAT_ZONES], zones = 0, i;
	if ( cgs.gametype != GT_DOMINATION && cgs.gametype != GT_KOTH ) return;
	if ( !cg.snap || cg.warmup || cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_CombatZonesPause( &mhCombat );
		return;
	}
	MH_CombatCheck();
	if ( cgs.gametype == GT_KOTH ) {
		owner[0] = cgs.kothOwner >= TEAM_RED && cgs.kothOwner <= TEAM_BLUE ? cgs.kothOwner - TEAM_RED : -1;
		zones = 1;
	} else {
		int pack = cg.snap->ps.stats[STAT_DOM_CAPTURE];
		for ( i = 0; i < MAX_SIGILS && i < CG_COMBAT_ZONES; i++ ) {
			owner[i] = cgs.sigil[i] >= SIGIL_ISRED && cgs.sigil[i] <= SIGIL_ISYELLOW ? cgs.sigil[i] - SIGIL_ISRED : -1;
			if ( cgs.sigil[i] != SIGIL_NONE ) zones = i + 1;
		}
		/* the zone you were taking (within a second) went to your team: yours */
		if ( mhDomCapZone >= 0 && mhDomCapZone < zones && cg.time - mhDomCapAt < 1500 &&
		     owner[mhDomCapZone] == mhDomCapTeam && mhCombat.owner[mhDomCapZone] != mhDomCapTeam ) {
			mhDomCaptures++;
			mhDomCapZone = -1;
		}
		if ( pack && DOM_CAPTURE_TEAM( pack ) >= TEAM_RED ) {
			mhDomCapZone = DOM_CAPTURE_POINT( pack );
			mhDomCapTeam = DOM_CAPTURE_TEAM( pack ) - TEAM_RED;
			mhDomCapAt = cg.time;
		}
	}
	CG_CombatZones( &mhCombat, cg.time, owner, zones );
}

/* Autoball: a goal (cg_autoball.c), team TEAM_RED or TEAM_BLUE */
void CG_ModernHUD_Goal( int team, int scorer, int speed ) {
	if ( cgs.gametype != GT_AUTOBALL || !cg.snap || cg.warmup ) return;
	MH_CombatCheck();
	CG_CombatGoal( &mhCombat, cg.time, team - TEAM_RED, scorer, speed );
}

/* the scores came: keep them for the minute (cg_servercmds.c) */
void CG_ModernHUD_CombatScores( void ) {
	int score[CG_COMBAT_CLIENTS], damage[CG_COMBAT_CLIENTS], team[CG_COMBAT_TEAMS], i;
	if ( !MH_CombatLogged() || cg.warmup || ( cg.snap && cg.snap->ps.pm_type == PM_INTERMISSION ) ) return;
	MH_CombatCheck();
	for ( i = 0; i < CG_COMBAT_CLIENTS; i++ ) { score[i] = CG_COMBAT_NONE; damage[i] = 0; }
	for ( i = 0; i < cg.numScores; i++ ) {
		int c = cg.scores[i].client;
		if ( c < 0 || c >= CG_COMBAT_CLIENTS || cgs.clientinfo[c].team == TEAM_SPECTATOR ) continue;
		score[c] = cg.scores[i].score;
		damage[c] = cg.scores[i].damageDealt;
	}
	for ( i = 0; i < CG_COMBAT_TEAMS; i++ ) team[i] = cg.teamScores[i];
	CG_CombatSample( &mhCombat, cg.time, score, damage, team );
}

void CG_ModernHUD_Obituary( int target, int attacker, int mod ) {
	int i;
	if ( target < 0 || target >= MAX_CLIENTS ) return;
	if ( MH_CombatLogged() && !cg.warmup ) {
		MH_CombatCheck();
		CG_CombatKill( &mhCombat, attacker >= 0 && attacker < MAX_CLIENTS ? attacker : -1, target, cg.time );
	}
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

/* ---------------------------------------------------------------------------
   the chat (gen.py chat): under the card, a line a message, the name in the
   accent (team chat: the team's colour, a private message: violet), gone after
   MH_CHAT_TIME. While the HUD shows them they stay out of the console's
   notify lines at the top left (they still go into the console).
   ------------------------------------------------------------------------- */

#define MH_CHAT_TIME	8000
#define MH_CHAT_FADE	1000

static struct {
	char	name[40];
	char	message[160];
	int		time;
	int		mode;		/* 0 everyone, 1 team, 2 to you */
} mhChat[MH_CHAT_MAX];

/* the v0.9 HUD is up and will show them (CG_ModernHUD_DrawChat) */
static qboolean MH_ChatShown( void ) {
	return cg.snap && CG_ModernHUD_Active() && cg_drawStatus.integer && !cg.showScores &&
	       cg.snap->ps.pm_type != PM_INTERMISSION && !CG_IntroCam_IsActive();
}

/* "Sam^7\x19: ^2nice shot", "\x19(Sam^7\x19) (Base)\x19: ^5go",
 * "\x19[Sam^7\x19]\x19: ^3hi" (g_cmds.c G_Say); the server's own "server: hi" */
static void MH_ChatSplit( const char *raw, char *name, int nameSize, char *message, int messageSize, int *mode ) {
	char who[128];
	const char *sep = NULL, *p, *text;
	char *open, *close;
	for ( p = raw; ( p = strstr( p, "\x19: " ) ) != NULL; p++ ) sep = p;
	if ( sep ) text = sep + 3;
	else if ( ( sep = strstr( raw, ": " ) ) != NULL ) text = sep + 2;
	else { sep = raw; text = raw; }
	Q_strncpyz( who, raw, sep - raw + 1 < (int)sizeof( who ) ? sep - raw + 1 : (int)sizeof( who ) );
	MH_Plain( who, who, sizeof( who ) );
	/* team: "(Sam) (Base)", private: "[Sam]" */
	if ( ( who[0] == '(' && ( close = strchr( who, ')' ) ) != NULL ) || ( who[0] == '[' && ( close = strchr( who, ']' ) ) != NULL ) ) {
		if ( who[0] == '[' ) *mode = 2;
		*close = '\0';
		open = who + 1;
		Q_strncpyz( name, open, nameSize );
	} else Q_strncpyz( name, who, nameSize );
	MH_Plain( text, message, messageSize );
}

qboolean CG_ModernHUD_Chat( const char *raw, qboolean team ) {
	int i, mode = team ? 1 : 0;
	if ( !raw || !raw[0] ) return qfalse;
	for ( i = 0; i < MH_CHAT_MAX - 1; i++ ) mhChat[i] = mhChat[i + 1];
	i = MH_CHAT_MAX - 1;
	MH_ChatSplit( raw, mhChat[i].name, sizeof( mhChat[i].name ), mhChat[i].message, sizeof( mhChat[i].message ), &mode );
	mhChat[i].mode = mode;
	mhChat[i].time = cg.time;
	return MH_ChatShown();
}

/* the line being typed (client/cl_console.c Con_ChatInputUpdate): the
 * engine leaves it to us while cg_hudChatInput is 1 */
static int mhChatInputOwned = -1;

void CG_ModernHUD_ChatInputFrame( void ) {
	int want = MH_ChatShown() ? 1 : 0;
	if ( want == mhChatInputOwned ) return;
	mhChatInputOwned = want;
	trap_Cvar_Set( "cg_hudChatInput", want ? "1" : "0" );
}

void CG_ModernHUD_ChatShutdown( void ) {
	mhChatInputOwned = -1;
	trap_Cvar_Set( "cg_hudChatInput", "0" );
}

/* "<mode> <client> <cursor> <text>": mode 0 say, 1 say_team, 2 tell */
static qboolean MH_ChatTyping( mhRaceLayout_t *l ) {
	char value[MH_CHAT_INPUT_MAX + 32];
	const char *p = value;
	int mode, client = -1, cursor = 0, k;
	if ( !( trap_Key_GetCatcher() & KEYCATCH_MESSAGE ) || mhChatInputOwned != 1 ) return qfalse;
	trap_Cvar_VariableStringBuffer( "cl_chatInput", value, sizeof( value ) );
	if ( !value[0] ) return qfalse;		/* an engine without it draws its own line */
	mode = atoi( p );
	for ( k = 0; k < 3; k++ ) {
		if ( k == 1 ) client = atoi( p );
		if ( k == 2 ) cursor = atoi( p );
		while ( *p && *p != ' ' ) p++;
		if ( *p ) p++;
	}
	Q_strncpyz( mh.chatInputText, p, sizeof( mh.chatInputText ) );
	if ( mode == 2 && client >= 0 && client < MAX_CLIENTS && cgs.clientinfo[client].infoValid ) {
		char name[40];
		MH_Plain( cgs.clientinfo[client].name, name, sizeof( name ) );
		Com_sprintf( mh.chatInputLabel, sizeof( mh.chatInputLabel ), "To %s", name );
	} else Q_strncpyz( mh.chatInputLabel, mode == 1 ? "Team" : mode == 2 ? "Tell" : "Say", sizeof( mh.chatInputLabel ) );
	mh.chatInputMode = mode;
	l->chatInput = 1;
	l->chatInputLabel = mh.chatInputLabel;
	l->chatInputText = mh.chatInputText;
	l->chatInputCursor = cursor;
	return qtrue;
}

static void MH_Chats( mhRaceLayout_t *l ) {
	int i;
	l->chatCount = 0;
	mh.chatTyping = qfalse;
	if ( !MH_ChatShown() ) return;
	/* while typing, the last messages stay (and do not fade) */
	mh.chatTyping = MH_ChatTyping( l );
	/* where the card is, also when it is not drawn (watching, dead): the chat does not jump */
	for ( i = 0; i < MH_CHAT_MAX; i++ ) {
		if ( !mhChat[i].time || mhChat[i].time > cg.time ) continue;
		if ( cg.time - mhChat[i].time > MH_CHAT_TIME && !mh.chatTyping ) continue;
		mh.chatLine[l->chatCount] = i;
		l->chatName[l->chatCount] = mhChat[i].name;
		l->chatMessage[l->chatCount++] = mhChat[i].message;
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
	mh.trophy = trap_R_RegisterShaderNoMip( "menu/art/hud_trophy" );
}

static qboolean MH_Views( void ) {
	return cg.snap->ps.pm_type != PM_INTERMISSION && !cg.showScores &&
	       cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR &&
	       !CG_IntroCam_IsActive();
}

/* everything the frame shows, laid out once */
/* A drag tournament's cars. The server sends away the bots that do not ride
 * the match up next, their cars with them; the podium after the final still
 * shows them: the showroom's handles of every entrant, kept whenever it is
 * seen (a tournament lives across map_restart, the cgame with it). */
static struct {
	char			name[16];
	cgShowroomCar_t	car;
	qboolean		valid;
} mhTourneyCars[CG_BRACKET_MAX];

static void MH_TourneyRemember( void ) {
	char name[MAX_QPATH];
	cgShowroomCar_t car;
	int i, e;
	if ( cgs.gametype != GT_DRAG || !cgs.dragTourney.size ) return;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !CG_ShowroomCarOf( i, &car ) ) continue;
		MH_Plain( cgs.clientinfo[i].name, name, sizeof( name ) );
		for ( e = 0; e < cgs.dragTourney.size && e < CG_BRACKET_MAX; e++ ) {
			if ( Q_stricmp( cgs.dragTourney.name[e], name ) ) continue;
			Q_strncpyz( mhTourneyCars[e].name, cgs.dragTourney.name[e], sizeof( mhTourneyCars[e].name ) );
			mhTourneyCars[e].car = car;
			mhTourneyCars[e].valid = qtrue;
		}
	}
}

/* an entrant's car: as it is now, or as it was last seen, or (a bot never
 * seen here) from its bot file, loaded once */
static qboolean MH_TourneyCar( int entrant, cgShowroomCar_t *car ) {
	if ( entrant < 0 || entrant >= CG_BRACKET_MAX ) return qfalse;
	MH_TourneyRemember();
	if ( !mhTourneyCars[entrant].valid || Q_stricmp( mhTourneyCars[entrant].name, cgs.dragTourney.name[entrant] ) ) {
		if ( !CG_ShowroomCarLoad( cgs.dragTourney.name[entrant], cgs.dragTourneyCar[entrant], MAX_CLIENTS - 1 - entrant, car ) ) return qfalse;
		Q_strncpyz( mhTourneyCars[entrant].name, cgs.dragTourney.name[entrant], sizeof( mhTourneyCars[entrant].name ) );
		mhTourneyCars[entrant].car = *car;
		mhTourneyCars[entrant].valid = qtrue;
	}
	*car = mhTourneyCars[entrant].car;
	return qtrue;
}

static void MH_Prepare( void ) {
	mhRaceLayout_t *l = &mh.l;
	playerState_t *ps = &cg.snap->ps;
	centity_t *cent = &cg_entities[ps->clientNum];
	int at[MAX_CLIENTS + 1], local, last, i, first, count, lap, lapTime, racers, turbo, maxHealth;
	const char *map;
	vec3_t forward;
	float factor;

	MH_TourneyRemember();

	if ( mh.ready && mh.time == cg.time ) return;
	mh.time = cg.time;
	mh.ready = qtrue;
	MH_Register();
	mhScale = cgs.glconfig.vidHeight / MH_DESIGN_HEIGHT;
	memset( l, 0, sizeof( *l ) );
	mh.lapColor = NULL;
	mh.posLabelColor = NULL;
	l->viewWidth = cgs.glconfig.vidWidth / mhScale;

	/* the end screen's top speed: ours, while we race */
	if ( cent->startRaceTime && !cent->finishRaceTime && cg.time >= cent->startRaceTime &&
	     !( ps->pm_flags & PMF_FOLLOW ) && ps->pm_type == PM_NORMAL ) {
		float v = Q3VelocityToRL( sqrt( cg.predictedPlayerState.velocity[0] * cg.predictedPlayerState.velocity[0] +
		                                cg.predictedPlayerState.velocity[1] * cg.predictedPlayerState.velocity[1] ) );
		if ( v > cg.raceTopSpeed ) cg.raceTopSpeed = v;
	}

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
	MH_Chats( l );

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

static void MH_DrawChat( const mhRaceLayout_t *l ) {
	static const vec4_t chatBack = { 13 / 255.0f, 13 / 255.0f, 13 / 255.0f, 0.55f };
	static const vec4_t chatText = { 230 / 255.0f, 230 / 255.0f, 230 / 255.0f, 1.0f };	/* #e6e6e6 */
	int i;
	for ( i = 0; i < l->chatCount; i++ ) {
		int k = mh.chatLine[i], left = MH_CHAT_TIME - ( cg.time - mhChat[k].time );
		float a = left < MH_CHAT_FADE && !mh.chatTyping ? left / (float)MH_CHAT_FADE : 1.0f;
		static const vec4_t chatPrivate = { 214 / 255.0f, 128 / 255.0f, 1.0f, 1.0f };	/* to you: violet, apart from the teams */
		const float *nameColor = mhChat[k].mode == 2 ? chatPrivate :
		                         mhChat[k].mode == 1 && cg.snap ? MH_TeamColor( cg.snap->ps.persistant[PERS_TEAM] ) : mhAccent;
		if ( mhChat[k].mode == 1 && nameColor == mhMuted ) nameColor = mhAccent;
		MH_FillRect( &l->chatBox[i], chatBack, a );
		MH_Str( &l->chatNameT[i], nameColor, a );
		MH_Str( &l->chatMessageT[i], chatText, a );
		if ( l->chatMoreT[i].text[0] ) MH_Str( &l->chatMoreT[i], chatText, a );
	}
	if ( l->chatInput ) {
		static const vec4_t inputBack = { 13 / 255.0f, 13 / 255.0f, 13 / 255.0f, 0.82f };
		static const vec4_t chatPrivate = { 214 / 255.0f, 128 / 255.0f, 1.0f, 1.0f };
		const float *bar = mh.chatInputMode == 2 ? chatPrivate :
		                   mh.chatInputMode == 1 && cg.snap ? MH_TeamColor( cg.snap->ps.persistant[PERS_TEAM] ) : mhAccent;
		if ( bar == mhMuted ) bar = mhAccent;
		MH_FillRect( &l->chatInputBox, inputBack, 1.0f );
		MH_Border( &l->chatInputBox, mhLine, 1.0f, NULL, 1.0f );
		MH_FillRect( &l->chatInputBar, bar, 1.0f );
		MH_Str( &l->chatInputLabelT, bar, 1.0f );
		MH_Str( &l->chatInputTextT, mhWhite, 1.0f );
		if ( ( trap_Milliseconds() / 500 ) % 2 == 0 ) MH_FillRect( &l->chatInputCaret, mhAccent, 1.0f );
	}
}

/* alive, dead or watching: cg_draw.c CG_Draw2D, whenever the v0.9 HUD is on */
void CG_ModernHUD_DrawChat( void ) {
	if ( !MH_ChatShown() ) return;
	MH_Prepare();
	MH_DrawChat( &mh.l );
}

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
	if ( !cg.snap || CG_IntroCam_IsActive() || cg.showScores ) return;
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

/* -------------------------------------------------------------------------
   the end screen of a race (gen.py EndRacing): in the intermission, in
   place of the scoreboard (TAB shows that one)
   ------------------------------------------------------------------------- */

static const vec4_t mhPurple = { 180 / 255.0f, 107 / 255.0f, 1.0f, 1.0f };	/* #b46bff: the fastest lap */
static const vec4_t mhVeil  = { 0.0f, 0.0f, 0.0f, 0.68f };					/* over the world behind */
static const vec4_t mhShowroom = { 24 / 255.0f, 23 / 255.0f, 22 / 255.0f, 1.0f };	/* behind a car */
static const vec4_t mhLapBest = { 230 / 255.0f, 230 / 255.0f, 230 / 255.0f, 1.0f };	/* #e6e6e6 */
static const vec4_t mhLapBar  = { 90 / 255.0f, 90 / 255.0f, 90 / 255.0f, 1.0f };	/* #5a5a5a */

static struct {
	mhEndLayout_t	e;
	int				client[3];			/* on the podium, by place */
	int				rowClient[MH_END_ROWS];
	int				fastest;			/* the client with the fastest lap, -1 */
	char			sub[96], pos[12], of[8], doneTime[16], doneNote[48];
	char			stat[MH_END_STATS][16], lap[MH_END_LAPS][12], lapName[MH_END_LAPS][8];
	char			podName[3][40], podTime[3][32], podGap[3][16];
	char			awardName[MH_END_AWARDS][64], awardValue[MH_END_AWARDS][16];
	const float		*awardColor[MH_END_AWARDS];
	qhandle_t		awardIcon[MH_END_AWARDS];
	char			rowPos[MH_END_ROWS][8], rowName[MH_END_ROWS][40], rowTime[MH_END_ROWS][16], rowBest[MH_END_ROWS][16];
	char			listRight[16], readyLabel[40], readyValue[24];
	float			readyFrac;
	const float		*statColor[MH_END_STATS];
	qboolean		rowMe[MH_END_ROWS];
	float			scale;				/* the page scaled down on a narrow screen */
	/* team races */
	qboolean		teams;
	const float		*posColor;			/* the place and its label: the team's colour */
	int				team[MH_END_TEAMS], teamCount;	/* the teams by their average */
	int				podiumTeam[3], rowTeam[MH_END_ROWS];
	char			doneLabel[40], teamName[MH_END_TEAMS][12], teamTime[MH_END_TEAMS][16], spreadValue[MH_END_TEAMS][16];
	char			memberPos[3][MH_END_MEMBERS][8], memberName[3][MH_END_MEMBERS][40];
	qboolean		memberMe[3][MH_END_MEMBERS];
	/* drag */
	qboolean		drag;
	const float		*doneNoteColor;
	int				slipClient[2];
	char			slipName[2][40], slipValue[2][MH_END_SLIP_ROWS][16], slipTitle[32];
	qboolean		slipBetter[2][MH_END_SLIP_ROWS];
	const float		*rowBestColor[MH_END_ROWS];
	qboolean		tourney;			/* the drag tournament over */
	cgShowroomCar_t	podCar[3];			/* its podium: the cars kept (their drivers may be gone) */
	qboolean		podCarValid[3];
	/* sprint */
	qboolean		sprint;
	char			lapPlace[MH_END_LAPS][8], avgHead[16];
	qboolean		lapPlaceLit[MH_END_LAPS];	/* the place changed over the sector */
	/* ghost race */
	qboolean		ghost;
	const float		*lapPlaceColor[MH_END_LAPS];	/* the running gap: blue ahead, orange behind */
	char			gap[16], duelTag[2][32], duelName[2][40], duelLap[2][16], duelNote[2][24];
	const float		*duelNoteColor[2];
	cgShowroomCar_t	duelCar[2];
	qboolean		duelCarValid[2];
	char			ladderPos[MH_END_LADDER][8], ladderName[MH_END_LADDER][40], ladderLap[MH_END_LADDER][16], ladderRight[16], ladderOf[16];
	/* combat */
	qboolean		combat;
	qboolean		lapLit[MH_END_LAPS];		/* the best minute */
	qboolean		racingDM;
	/* Autoball: the goals */
	char			goalMinute[MH_END_GOALS][8], goalName[MH_END_GOALS][40], goalShot[MH_END_GOALS][16], goalScore[MH_END_GOALS][8];
	int				goalTeam[MH_END_GOALS];
} mhEnd;

qboolean CG_ModernHUD_EndActive( void ) {
	int i;
	if ( !cg_hudStyle.integer || !cg.snap || cg.snap->ps.pm_type != PM_INTERMISSION ) return qfalse;
	if ( cgs.gametype != GT_RACING && cgs.gametype != GT_TEAM_RACING && cgs.gametype != GT_SPRINT && cgs.gametype != GT_GHOST &&
	     cgs.gametype != GT_DRAG && cgs.gametype != GT_RACING_DM && cgs.gametype != GT_TEAM_RACING_DM &&
	     cgs.gametype != GT_ELIMINATION && !MH_EndIsCombat() ) return qfalse;
	if ( cgs.gametype == GT_DRAG ) return cgs.dragLog.count > 0;	/* the heats the client wrote down */
	if ( MH_EndIsCombat() ) return cg.numScores > 0;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( CG_RaceResult( i )->present && CG_RaceResult( i )->finishTime ) return qtrue;
	}
	return qfalse;
}

/* the seconds a lap lost to the best one: "+1.22" */
static void MH_EndLost( int ms, char *out, int size ) {
	Com_sprintf( out, size, "+%d.%02d", ms / 1000, ( ms % 1000 ) / 10 );
}

static const char *mhEndTeamNames[MH_END_TEAMS] = { "RED", "BLUE", "GREEN", "YELLOW" };

/* a team race: the teams on the podium by their average time, the server's
 * ranking. Over what the race result put up: the place and the time are the
 * team's, the lap chart gives way to how each average came about. */
static void MH_EndTeams( const cgRaceResult_t *all, const int *order, int count, int me, const cgRaceResult_t *mine, int place ) {
	mhEndLayout_t *e = &mhEnd.e;
	int avg[MH_END_TEAMS], members[MH_END_TEAMS], i, k, t, myTeam = MH_Team( me ), myRank = 0, playerClient = -1, under = 0;
	int lo = 0, hi = 0, span;
	char text[40];

	/* the teams with drivers, and their averages: the server's, or ours of
	 * those home when it sent none */
	mhEnd.teams = qtrue;
	mhEnd.teamCount = 0;
	for ( t = 0; t < MH_END_TEAMS; t++ ) {
		int sum = 0, home = 0;
		members[t] = 0;
		for ( i = 0; i < count; i++ ) {
			if ( MH_Team( order[i] ) != TEAM_RED + t ) continue;
			members[t]++;
			if ( all[order[i]].finishTime ) { sum += CG_ResultTotal( &all[order[i]] ); home++; }
		}
		avg[t] = cg.teamTimes[t] > 0 && cg.teamTimes[t] < ( 1 << 30 ) ? cg.teamTimes[t] : home ? sum / home : 0;
		if ( members[t] ) mhEnd.team[mhEnd.teamCount++] = t;
	}
	for ( i = 1; i < mhEnd.teamCount; i++ ) {	/* by average, none last */
		for ( k = i; k > 0; k-- ) {
			int a = mhEnd.team[k - 1], b = mhEnd.team[k];
			if ( avg[a] && ( !avg[b] || avg[a] <= avg[b] ) ) break;
			mhEnd.team[k - 1] = b;
			mhEnd.team[k] = a;
		}
	}
	for ( i = 0; i < mhEnd.teamCount; i++ ) {
		t = mhEnd.team[i];
		Q_strncpyz( mhEnd.teamName[t], mhEndTeamNames[t], sizeof( mhEnd.teamName[t] ) );
		if ( avg[t] ) CG_ResultTime( avg[t], mhEnd.teamTime[t], sizeof( mhEnd.teamTime[t] ) );
		else Q_strncpyz( mhEnd.teamTime[t], "--", sizeof( mhEnd.teamTime[t] ) );
		if ( TEAM_RED + t == myTeam ) myRank = i + 1;
	}
	if ( !myRank ) {	/* not in a team: the winners' */
		myRank = 1;
		myTeam = mhEnd.teamCount ? TEAM_RED + mhEnd.team[0] : TEAM_RED;
	}
	t = myTeam - TEAM_RED;

	/* the header */
	e->title = "TEAM RACE RESULT";
	{
		char *dot = strstr( mhEnd.sub, " \xb7 " );	/* the map, as the race put it */
		char map[48];
		Q_strncpyz( map, mhEnd.sub, sizeof( map ) );
		if ( dot && dot - mhEnd.sub < (int)sizeof( map ) ) map[dot - mhEnd.sub] = '\0';
		if ( cgs.laplimit > 0 )
			Com_sprintf( mhEnd.sub, sizeof( mhEnd.sub ), "%s \xb7 %d LAP%s \xb7 %d TEAMS \xb7 %d DRIVERS", map, cgs.laplimit,
			             cgs.laplimit == 1 ? "" : "S", mhEnd.teamCount, count );
		else
			Com_sprintf( mhEnd.sub, sizeof( mhEnd.sub ), "%s \xb7 %d TEAMS \xb7 %d DRIVERS", map, mhEnd.teamCount, count );
	}

	/* the left: your team's place and average, then yours */
	mhEnd.posColor = MH_TeamColor( myTeam );
	Com_sprintf( mhEnd.pos, sizeof( mhEnd.pos ), "P%d", myRank );
	Com_sprintf( mhEnd.of, sizeof( mhEnd.of ), "/ %d", mhEnd.teamCount );
	Com_sprintf( mhEnd.doneLabel, sizeof( mhEnd.doneLabel ), "TEAM %s \xb7 AVERAGE", mhEndTeamNames[t] );
	e->doneLabel = mhEnd.doneLabel;
	Q_strncpyz( mhEnd.doneTime, mhEnd.teamTime[t], sizeof( mhEnd.doneTime ) );
	mhEnd.doneNote[0] = '\0';
	if ( mhEnd.teamCount > 1 && avg[t] ) {
		int other = myRank == 1 ? avg[mhEnd.team[1]] : avg[mhEnd.team[0]];
		if ( other ) {
			CG_ResultDiff( myRank == 1 ? other - avg[t] : avg[t] - other, text, sizeof( text ) );
			if ( myRank == 1 ) Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "WON BY %s", text + 1 );
			else Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "%s TO THE WINNER", text );
		}
	}

	e->statLabel[0] = "YOUR PLACE";
	if ( place ) Com_sprintf( mhEnd.stat[0], sizeof( mhEnd.stat[0] ), "P%d", place );
	else Q_strncpyz( mhEnd.stat[0], "--", sizeof( mhEnd.stat[0] ) );
	e->statLabel[1] = "YOUR TIME";
	if ( mine->finishTime ) CG_ResultTime( CG_ResultTotal( mine ), mhEnd.stat[1], sizeof( mhEnd.stat[1] ) );
	else Q_strncpyz( mhEnd.stat[1], "DNF", sizeof( mhEnd.stat[1] ) );
	e->statUnit[1] = NULL;
	e->statLabel[2] = "VS TEAM AVERAGE";
	mhEnd.statColor[2] = mhWhite;
	if ( mine->finishTime && avg[t] ) {
		int d = CG_ResultTotal( mine ) - avg[t];
		CG_ResultDiff( d, mhEnd.stat[2], sizeof( mhEnd.stat[2] ) );
		mhEnd.statColor[2] = d < 0 ? mhFast : d > 0 ? mhAccent : mhWhite;
	} else Q_strncpyz( mhEnd.stat[2], "--", sizeof( mhEnd.stat[2] ) );
	e->statLabel[3] = "BEST LAP";
	if ( CG_ResultBestLap( mine ) ) CG_ResultTime( CG_ResultBestLap( mine ), mhEnd.stat[3], sizeof( mhEnd.stat[3] ) );
	else Q_strncpyz( mhEnd.stat[3], "--", sizeof( mhEnd.stat[3] ) );

	/* how each average came about: a dot a driver home, the average a tick */
	e->chartTitle = "TEAM AVERAGES";
	e->chartNote = "A DOT A DRIVER \xb7 TO THE WINNER";
	e->lapCount = 0;
	for ( i = 0; i < count; i++ ) {
		int ms;
		if ( !all[order[i]].finishTime ) continue;
		ms = CG_ResultTotal( &all[order[i]] );
		if ( !lo || ms < lo ) lo = ms;
		if ( ms > hi ) hi = ms;
	}
	for ( i = 0; i < mhEnd.teamCount; i++ ) {
		int a = avg[mhEnd.team[i]];
		if ( !a ) continue;
		if ( !lo || a < lo ) lo = a;
		if ( a > hi ) hi = a;
	}
	span = hi - lo;
	if ( span < 1000 ) span = 1000;
	lo -= span / 10;
	span += span / 5;
	for ( i = 0; i < mhEnd.teamCount && i < MH_END_TEAMS; i++ ) {
		int team = mhEnd.team[i];
		e->spreadCount++;
		e->spreadTick[i] = avg[team] ? (float)( avg[team] - lo ) / span : 0.0f;
		for ( k = 0; k < count && e->spreadDots[i] < MH_END_DOTS; k++ ) {
			if ( MH_Team( order[k] ) != TEAM_RED + team || !all[order[k]].finishTime ) continue;
			e->spreadDot[i][e->spreadDots[i]++] = (float)( CG_ResultTotal( &all[order[k]] ) - lo ) / span;
		}
		if ( i == 0 ) Q_strncpyz( mhEnd.spreadValue[i], "best", sizeof( mhEnd.spreadValue[i] ) );
		else if ( avg[team] && avg[mhEnd.team[0]] ) CG_ResultDiff( avg[team] - avg[mhEnd.team[0]], mhEnd.spreadValue[i], sizeof( mhEnd.spreadValue[i] ) );
		else Q_strncpyz( mhEnd.spreadValue[i], "--", sizeof( mhEnd.spreadValue[i] ) );
		e->spreadValue[i] = mhEnd.spreadValue[i];
	}

	/* the podium: the first three teams, each with its best car; the drivers under */
	e->teamPodium = qtrue;
	e->podiumYou = "YOUR TEAM";
	e->podiumCount = 0;
	for ( i = 0; i < 3 && i < mhEnd.teamCount; i++ ) {
		static const char *tags[3] = { "WINNER", "SECOND", "THIRD" };
		int team = mhEnd.team[i], m = 0;
		mhEnd.podiumTeam[i] = team;
		mhEnd.client[i] = -1;
		Q_strncpyz( mhEnd.podName[i], mhEndTeamNames[team], sizeof( mhEnd.podName[i] ) );
		Q_strlwr( mhEnd.podName[i] + 1 );
		Q_strncpyz( mhEnd.podTime[i], mhEnd.teamTime[team], sizeof( mhEnd.podTime[i] ) );
		mhEnd.podGap[i][0] = '\0';
		if ( i > 0 ) Q_strncpyz( mhEnd.podGap[i], mhEnd.spreadValue[i], sizeof( mhEnd.podGap[i] ) );
		for ( k = 0; k < count; k++ ) {
			if ( MH_Team( order[k] ) != TEAM_RED + team ) continue;
			if ( mhEnd.client[i] < 0 ) mhEnd.client[i] = order[k];
			if ( m < MH_END_MEMBERS ) {
				Com_sprintf( mhEnd.memberPos[i][m], sizeof( mhEnd.memberPos[i][m] ), "P%d", k + 1 );
				MH_Plain( cgs.clientinfo[order[k]].name, mhEnd.memberName[i][m], sizeof( mhEnd.memberName[i][m] ) );
				mhEnd.memberMe[i][m] = order[k] == cg.snap->ps.clientNum;
				e->podiumMemberPos[i][m] = mhEnd.memberPos[i][m];
				e->podiumMemberName[i][m] = mhEnd.memberName[i][m];
				m++;
			}
		}
		e->podiumMembers[i] = m;
		e->podiumName[i] = mhEnd.podName[i];
		e->podiumTime[i] = mhEnd.podTime[i];
		e->podiumGap[i] = mhEnd.podGap[i];
		e->podiumTag[i] = tags[i];
		e->podiumMe[i] = TEAM_RED + team == MH_Team( cg.snap->ps.clientNum );
		e->podiumCount = i + 1;
	}

	/* the team player in place of the best sector: the most under the team's average */
	for ( i = 0; i < count; i++ ) {
		const cgRaceResult_t *r = &all[order[i]];
		int team = MH_Team( order[i] ) - TEAM_RED, d;
		if ( !r->finishTime || team < 0 || team >= MH_END_TEAMS || !avg[team] ) continue;
		d = avg[team] - CG_ResultTotal( r );
		if ( d > under ) { under = d; playerClient = order[i]; }
	}
	for ( k = 0; k < e->awardCount && Q_stricmp( e->awardLabel[k], "BEST SECTOR" ); k++ );
	if ( k < e->awardCount && playerClient < 0 ) {	/* nobody under: the sector award goes */
		for ( ; k < e->awardCount - 1; k++ ) {
			e->awardLabel[k] = e->awardLabel[k + 1];
			Q_strncpyz( mhEnd.awardName[k], mhEnd.awardName[k + 1], sizeof( mhEnd.awardName[k] ) );
			Q_strncpyz( mhEnd.awardValue[k], mhEnd.awardValue[k + 1], sizeof( mhEnd.awardValue[k] ) );
			mhEnd.awardColor[k] = mhEnd.awardColor[k + 1];
			mhEnd.awardIcon[k] = mhEnd.awardIcon[k + 1];
		}
		e->awardCount--;
	} else if ( playerClient >= 0 ) {
		int team = MH_Team( playerClient ) - TEAM_RED;
		if ( k >= e->awardCount ) k = e->awardCount < MH_END_AWARDS ? e->awardCount++ : MH_END_AWARDS - 1;
		e->awardLabel[k] = "TEAM PLAYER";
		MH_Plain( cgs.clientinfo[playerClient].name, mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ) );
		CG_ResultDiff( under, text, sizeof( text ) );
		Q_strcat( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), va( " \xb7 %s under ", text + 1 ) );
		Q_strcat( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), mhEnd.teamName[team] );
		Q_strlwr( mhEnd.awardName[k] + strlen( mhEnd.awardName[k] ) - strlen( mhEnd.teamName[team] ) );
		CG_ResultDiff( -under, mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ) );
		mhEnd.awardColor[k] = mhFast;
		mhEnd.awardIcon[k] = mh.flag;
	}

	/* the classification: the teams over the drivers, each with the team's square */
	e->listRight = "TEAM AVERAGE";
	e->rowSwatch = qtrue;
	for ( i = 0; i < e->rowCount; i++ ) mhEnd.rowTeam[i] = MH_Team( mhEnd.rowClient[i] );
	e->teamCellCount = mhEnd.teamCount;
	for ( i = 0; i < mhEnd.teamCount; i++ ) {
		e->teamCellName[i] = mhEnd.teamName[mhEnd.team[i]];
		e->teamCellValue[i] = mhEnd.teamTime[mhEnd.team[i]];
	}
}

/* -------------------------------------------------------------------------
   the end screen of a drag race (gen.py EndDrag): the match, a time slip a
   lane of the deciding heat, the heats; from the heat log the client keeps
   ------------------------------------------------------------------------- */

static void MH_EndDragTrap( int trap, char *out, int size ) {
	if ( trap >= 0 ) Com_sprintf( out, size, "%d.%d km/h", trap / 10, trap % 10 );
	else Q_strncpyz( out, "--", size );
}

/* the heats of this duel (or of the one driver on the strip), in order */
static int MH_EndDragHeats( const int *duel, int drivers, int *heats ) {
	const cgDragLog_t *log = &cgs.dragLog;
	int i, n = 0;
	for ( i = 0; i < log->count && n < CG_DRAG_HEATS; i++ ) {
		const cgDragHeat_t *h = &log->heat[i];
		if ( !CG_DragLogRun( h, duel[0] ) ) continue;
		if ( drivers == 2 && !CG_DragLogRun( h, duel[1] ) ) continue;
		heats[n++] = i;
	}
	return n;
}

static void MH_EndDrag( void ) {
	mhEndLayout_t *e = &mhEnd.e;
	const cgDragLog_t *log = &cgs.dragLog;
	int duel[2], lanes[2], drivers, heats[CG_DRAG_HEATS], heatCount, i, k, me, rival = -1, decider = -1;
	int myBestEt = 0, rivalBestEt = 0, bestRt = -1, bestTotal = 0, topTrap = -1, fouls = 0, worst = 0;
	int awardRt = -1, awardRtWho = -1, awardRtHeat = 0, awardEt = 0, awardEtWho = -1, awardEtHeat = 0;
	char name[40], map[40];
	const char *info;

	mhEnd.drag = qtrue;
	info = CG_ConfigString( CS_MESSAGE );
	if ( !info || !info[0] ) info = COM_SkipPath( cgs.mapname );
	MH_Plain( info, map, sizeof( map ) );
	COM_StripExtension( map, map, sizeof( map ) );
	Q_strupr( map );
	/* who drove: the match's two, or the one on the strip */
	if ( cgs.dragMatch.count >= 1 ) {
		drivers = cgs.dragMatch.count;
		duel[0] = cgs.dragMatch.client[0];
		duel[1] = drivers == 2 ? cgs.dragMatch.client[1] : -1;
	} else {
		drivers = 1;
		duel[0] = cg.snap->ps.clientNum;
		duel[1] = -1;
	}
	me = cg.snap->ps.clientNum;
	if ( me != duel[0] && me != duel[1] ) {	/* watching: the winner's side */
		me = cgs.dragMatch.winner >= 0 && cgs.dragMatch.winner < MAX_CLIENTS && cgs.dragMatch.winner == duel[1] ? duel[1] : duel[0];
	}
	if ( drivers == 2 ) rival = me == duel[0] ? duel[1] : duel[0];
	heatCount = MH_EndDragHeats( duel, drivers, heats );

	/* the header */
	e->title = "DRAG RESULT";
	if ( cgs.dragMatch.bestOf > 1 )
		Com_sprintf( mhEnd.sub, sizeof( mhEnd.sub ), "%s \xb7 QUARTER MILE \xb7 BEST OF %d", map, cgs.dragMatch.bestOf );
	else
		Com_sprintf( mhEnd.sub, sizeof( mhEnd.sub ), "%s \xb7 QUARTER MILE", map );

	/* your heats: the quickest, the chart, the figures */
	e->lapCount = 0;
	for ( i = 0; i < heatCount; i++ ) {
		const cgDragRun_t *r = CG_DragLogRun( &log->heat[heats[i]], me ), *o = rival >= 0 ? CG_DragLogRun( &log->heat[heats[i]], rival ) : NULL;
		int m;
		if ( r->state == CG_DRAG_FOUL ) fouls++;
		if ( r->state == CG_DRAG_DONE && r->et > 0 ) {
			if ( !myBestEt || r->et < myBestEt ) myBestEt = r->et;
			if ( r->et > worst ) worst = r->et;
			if ( r->total > 0 && ( !bestTotal || r->total < bestTotal ) ) bestTotal = r->total;
			if ( r->trap > topTrap ) topTrap = r->trap;
		}
		if ( r->state != CG_DRAG_FOUL && r->rt >= 0 && ( bestRt < 0 || r->rt < bestRt ) ) bestRt = r->rt;
		if ( o && o->state == CG_DRAG_DONE && o->et > 0 && ( !rivalBestEt || o->et < rivalBestEt ) ) rivalBestEt = o->et;
		if ( CG_DragLogWinner( &log->heat[heats[i]], &m ) >= 0 ) decider = i;
		/* the highlights, over both */
		for ( k = 0; k < 2; k++ ) {
			const cgDragRun_t *x = &log->heat[heats[i]].run[k];
			if ( x->client < 0 ) continue;
			if ( x->state != CG_DRAG_FOUL && x->rt >= 0 && ( awardRt < 0 || x->rt < awardRt ) ) {
				awardRt = x->rt; awardRtWho = x->client; awardRtHeat = i + 1;
			}
			if ( x->state == CG_DRAG_DONE && x->et > 0 && ( !awardEt || x->et < awardEt ) ) {
				awardEt = x->et; awardEtWho = x->client; awardEtHeat = i + 1;
			}
		}
	}
	for ( i = 0; i < heatCount && e->lapCount < MH_END_LAPS; i++ ) {
		const cgDragRun_t *r = CG_DragLogRun( &log->heat[heats[i]], me );
		k = e->lapCount++;
		Com_sprintf( mhEnd.lapName[k], sizeof( mhEnd.lapName[k] ), "H%d", i + 1 );
		if ( r->state == CG_DRAG_DONE && r->et > 0 && myBestEt ) {
			e->lapFrac[k] = worst > myBestEt ? (float)( r->et - myBestEt ) / ( worst - myBestEt ) : 0.0f;
			if ( r->et == myBestEt ) Q_strncpyz( mhEnd.lap[k], "best", sizeof( mhEnd.lap[k] ) );
			else Com_sprintf( mhEnd.lap[k], sizeof( mhEnd.lap[k] ), "+%d.%03d", ( r->et - myBestEt ) / 1000, ( r->et - myBestEt ) % 1000 );
		} else {
			e->lapFrac[k] = 0.0f;
			Q_strncpyz( mhEnd.lap[k], r->state == CG_DRAG_FOUL ? "foul" : "dnf", sizeof( mhEnd.lap[k] ) );
		}
		e->lapValue[k] = mhEnd.lap[k];
		e->lapName[k] = mhEnd.lapName[k];
	}
	e->chartTitle = "YOUR HEATS";
	e->chartNote = "LOST TO YOUR QUICKEST";
	if ( e->lapCount < 2 ) {	/* one heat: nothing to compare, no chart, no heading */
		e->lapCount = 0;
		e->chartTitle = e->chartNote = "";
	}

	/* the left: the match, your quickest run */
	if ( drivers == 2 ) {
		int mine = cgs.dragMatch.client[0] == me ? cgs.dragMatch.wins[0] : cgs.dragMatch.wins[1];
		int theirs = cgs.dragMatch.client[0] == me ? cgs.dragMatch.wins[1] : cgs.dragMatch.wins[0];
		Com_sprintf( mhEnd.pos, sizeof( mhEnd.pos ), "%d-%d", mine, theirs );
		if ( me == cg.snap->ps.clientNum )
			Com_sprintf( mhEnd.doneLabel, sizeof( mhEnd.doneLabel ), "%s \xb7 YOUR QUICKEST RUN",
			             cgs.dragMatch.winner == me ? "MATCH WON" : cgs.dragMatch.winner >= 0 ? "MATCH LOST" : "MATCH" );
		else {
			MH_Plain( cgs.clientinfo[me].name, name, sizeof( name ) );
			Com_sprintf( mhEnd.doneLabel, sizeof( mhEnd.doneLabel ), "%s \xb7 QUICKEST RUN", name );
		}
	} else {
		int done = 0;
		for ( i = 0; i < heatCount; i++ ) if ( CG_DragLogRun( &log->heat[heats[i]], me )->state == CG_DRAG_DONE ) done++;
		Com_sprintf( mhEnd.pos, sizeof( mhEnd.pos ), "%d", done );
		Q_strncpyz( mhEnd.doneLabel, "CLEAN RUNS \xb7 YOUR QUICKEST", sizeof( mhEnd.doneLabel ) );
	}
	Com_sprintf( mhEnd.of, sizeof( mhEnd.of ), "/ %d", cgs.dragMatch.bestOf > 1 ? cgs.dragMatch.bestOf : heatCount > 0 ? heatCount : 1 );
	e->doneLabel = mhEnd.doneLabel;
	if ( myBestEt ) MH_DragTime( myBestEt, mhEnd.doneTime, sizeof( mhEnd.doneTime ) );
	else Q_strncpyz( mhEnd.doneTime, "--", sizeof( mhEnd.doneTime ) );
	mhEnd.doneNote[0] = '\0';
	mhEnd.doneNoteColor = mhAccent;
	if ( rival >= 0 && myBestEt && rivalBestEt && myBestEt != rivalBestEt ) {
		int d = rivalBestEt - myBestEt, a = d < 0 ? -d : d;
		MH_Plain( cgs.clientinfo[rival].name, name, sizeof( name ) );
		Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "%d.%03d %s THAN %s", a / 1000, a % 1000, d > 0 ? "QUICKER" : "SLOWER", name );
		mhEnd.doneNoteColor = d > 0 ? mhFast : mhAccent;
	}

	e->statLabel[0] = "BEST REACTION";
	if ( bestRt >= 0 ) MH_DragTime( bestRt, mhEnd.stat[0], sizeof( mhEnd.stat[0] ) );
	else Q_strncpyz( mhEnd.stat[0], "--", sizeof( mhEnd.stat[0] ) );
	e->statLabel[1] = "TOP TRAP";
	e->statUnit[1] = NULL;
	if ( topTrap >= 0 ) {
		Com_sprintf( mhEnd.stat[1], sizeof( mhEnd.stat[1] ), "%d.%d", topTrap / 10, topTrap % 10 );
		e->statUnit[1] = "KM/H";
	} else Q_strncpyz( mhEnd.stat[1], "--", sizeof( mhEnd.stat[1] ) );
	e->statLabel[2] = "BEST TOTAL";
	if ( bestTotal ) MH_DragTime( bestTotal, mhEnd.stat[2], sizeof( mhEnd.stat[2] ) );
	else Q_strncpyz( mhEnd.stat[2], "--", sizeof( mhEnd.stat[2] ) );
	e->statLabel[3] = "FOULS";
	Com_sprintf( mhEnd.stat[3], sizeof( mhEnd.stat[3] ), "%d", fouls );
	for ( i = 0; i < 4; i++ ) mhEnd.statColor[i] = mhWhite;
	if ( fouls ) mhEnd.statColor[3] = mhAccent;

	/* the middle: the deciding heat (or the last), a slip a lane */
	e->podiumCount = 0;
	if ( decider < 0 ) decider = heatCount - 1;
	if ( decider >= 0 ) {
		const cgDragHeat_t *h = &log->heat[heats[decider]];
		int margin, win = CG_DragLogWinner( h, &margin ), n = 0;
		if ( drivers == 2 && MH_DragLanes( lanes ) == 2 && ( lanes[0] == duel[0] || lanes[0] == duel[1] ) ) {
			/* the lanes as the cars stand */
		} else {
			lanes[0] = duel[0];
			lanes[1] = duel[1];
		}
		e->slipRows = MH_END_SLIP_ROWS;
		e->slipLabel[0] = "REACTION";
		e->slipLabel[1] = "60 FT";
		e->slipLabel[2] = "330 FT";
		e->slipLabel[3] = "1/8 MILE";
		e->slipLabel[4] = "1/4 MILE";
		e->slipLabel[5] = "TRAP";
		for ( k = 0; k < drivers && k < 2; k++ ) {
			int client = lanes[k];
			const cgDragRun_t *r = CG_DragLogRun( h, client );
			int value[MH_END_SLIP_ROWS], row;
			if ( !r ) continue;
			mhEnd.slipClient[n] = client;
			MH_Plain( cgs.clientinfo[client].name, mhEnd.slipName[n], sizeof( mhEnd.slipName[n] ) );
			e->slipName[n] = mhEnd.slipName[n];
			e->slipLane[n] = drivers == 1 ? "YOUR LANE" : k == 0 ? "LEFT LANE" : "RIGHT LANE";
			e->slipMe[n] = client == cg.snap->ps.clientNum;
			e->slipWin[n] = drivers == 2 && win >= 0 && h->run[win].client == client;
			value[0] = r->state == CG_DRAG_FOUL ? -1 : r->rt;
			value[1] = r->split[0] ? r->split[0] : -1;
			value[2] = r->split[1] ? r->split[1] : -1;
			value[3] = r->split[2] ? r->split[2] : -1;
			value[4] = r->state == CG_DRAG_DONE ? r->et : -1;
			value[5] = r->state == CG_DRAG_DONE ? r->trap : -1;
			for ( row = 0; row < MH_END_SLIP_ROWS; row++ ) {
				if ( row == 0 && r->state == CG_DRAG_FOUL ) Q_strncpyz( mhEnd.slipValue[n][row], "FOUL", sizeof( mhEnd.slipValue[n][row] ) );
				else if ( row == 4 && r->state == CG_DRAG_OUT ) Q_strncpyz( mhEnd.slipValue[n][row], "DNF", sizeof( mhEnd.slipValue[n][row] ) );
				else if ( value[row] < 0 ) Q_strncpyz( mhEnd.slipValue[n][row], "--", sizeof( mhEnd.slipValue[n][row] ) );
				else if ( row == 5 ) MH_EndDragTrap( value[row], mhEnd.slipValue[n][row], sizeof( mhEnd.slipValue[n][row] ) );
				else MH_DragTime( value[row], mhEnd.slipValue[n][row], sizeof( mhEnd.slipValue[n][row] ) );
				e->slipValue[n][row] = mhEnd.slipValue[n][row];
				mhEnd.slipBetter[n][row] = qfalse;
			}
			/* the better figure of each line: compared once both slips are in */
			if ( n == 1 ) {
				const cgDragRun_t *a = CG_DragLogRun( h, mhEnd.slipClient[0] );
				int va[MH_END_SLIP_ROWS];
				va[0] = a->state == CG_DRAG_FOUL ? -1 : a->rt;
				va[1] = a->split[0] ? a->split[0] : -1;
				va[2] = a->split[1] ? a->split[1] : -1;
				va[3] = a->split[2] ? a->split[2] : -1;
				va[4] = a->state == CG_DRAG_DONE ? a->et : -1;
				va[5] = a->state == CG_DRAG_DONE ? a->trap : -1;
				for ( row = 0; row < MH_END_SLIP_ROWS; row++ ) {
					if ( va[row] < 0 || value[row] < 0 || va[row] == value[row] ) continue;
					if ( row == 5 ? va[row] > value[row] : va[row] < value[row] ) mhEnd.slipBetter[0][row] = qtrue;
					else mhEnd.slipBetter[1][row] = qtrue;
				}
			}
			n++;
		}
		e->slipCount = n;
		Com_sprintf( mhEnd.slipTitle, sizeof( mhEnd.slipTitle ), cgs.dragMatch.over && win >= 0 && drivers == 2 ? "HEAT %d \xb7 DECIDING" : "HEAT %d",
		             decider + 1 );
		e->slipTitle = mhEnd.slipTitle;
		e->slipNote = n == 2 ? "BETTER FIGURE IN BLUE" : "";
	}

	/* the highlights: the best reaction and the quickest run of the match */
	e->awardCount = 0;
	if ( awardRtWho >= 0 ) {
		k = e->awardCount++;
		e->awardLabel[k] = "BEST REACTION";
		MH_Plain( cgs.clientinfo[awardRtWho].name, name, sizeof( name ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 heat %d", name, awardRtHeat );
		MH_DragTime( awardRt, mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ) );
		mhEnd.awardColor[k] = mhFast;
		mhEnd.awardIcon[k] = mh.flag;
	}
	if ( awardEtWho >= 0 ) {
		k = e->awardCount++;
		e->awardLabel[k] = "QUICKEST RUN";
		MH_Plain( cgs.clientinfo[awardEtWho].name, name, sizeof( name ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 heat %d", name, awardEtHeat );
		MH_DragTime( awardEt, mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ) );
		mhEnd.awardColor[k] = mhPurple;
		mhEnd.awardIcon[k] = mh.trophy;
	}

	/* the right: the heats, who won each, by how much */
	e->listTitle = "HEATS";
	if ( cgs.dragMatch.bestOf > 1 ) Com_sprintf( mhEnd.listRight, sizeof( mhEnd.listRight ), "BEST OF %d", cgs.dragMatch.bestOf );
	else mhEnd.listRight[0] = '\0';
	e->listRight = mhEnd.listRight;
	e->listHead[0] = "HEAT";
	e->listHead[1] = "WINNER";
	e->listHead[2] = "ET";
	e->listHead[3] = "MARGIN";
	e->rowCount = 0;
	e->compact = heatCount > 12;
	for ( i = 0; i < heatCount && e->rowCount < MH_END_ROWS; i++ ) {
		const cgDragHeat_t *h = &log->heat[heats[i]];
		int margin, win = CG_DragLogWinner( h, &margin );
		k = e->rowCount++;
		Com_sprintf( mhEnd.rowPos[k], sizeof( mhEnd.rowPos[k] ), "H%d", i + 1 );
		mhEnd.rowClient[k] = -1;
		mhEnd.rowMe[k] = qfalse;
		mhEnd.rowBestColor[k] = mhMuted;
		if ( win >= 0 ) {
			const cgDragRun_t *w = &h->run[win], *l = &h->run[1 - win];
			MH_Plain( cgs.clientinfo[w->client].name, mhEnd.rowName[k], sizeof( mhEnd.rowName[k] ) );
			MH_DragTime( w->et, mhEnd.rowTime[k], sizeof( mhEnd.rowTime[k] ) );
			mhEnd.rowMe[k] = w->client == cg.snap->ps.clientNum;
			if ( margin >= 0 ) Com_sprintf( mhEnd.rowBest[k], sizeof( mhEnd.rowBest[k] ), "+%d.%03d", margin / 1000, margin % 1000 );
			else if ( !CG_DragLogPresent( l ) ) Q_strncpyz( mhEnd.rowBest[k], "SOLO", sizeof( mhEnd.rowBest[k] ) );
			else {
				Q_strncpyz( mhEnd.rowBest[k], l->state == CG_DRAG_FOUL ? "FOUL" : "DNF", sizeof( mhEnd.rowBest[k] ) );
				mhEnd.rowBestColor[k] = mhAccent;
			}
		} else {
			const cgDragRun_t *a = &h->run[0];
			Q_strncpyz( mhEnd.rowName[k], "No winner", sizeof( mhEnd.rowName[k] ) );
			Q_strncpyz( mhEnd.rowTime[k], "--", sizeof( mhEnd.rowTime[k] ) );
			Q_strncpyz( mhEnd.rowBest[k], a->state == CG_DRAG_FOUL || h->run[1].state == CG_DRAG_FOUL ? "FOUL" :
			            a->state == CG_DRAG_RUNNING ? "--" : "RERUN", sizeof( mhEnd.rowBest[k] ) );
			mhEnd.rowBestColor[k] = mhAccent;
		}
		e->rowPos[k] = mhEnd.rowPos[k];
		e->rowName[k] = mhEnd.rowName[k];
		e->rowTime[k] = mhEnd.rowTime[k];
		e->rowBest[k] = mhEnd.rowBest[k];
	}
}

/* the keys; who is ready (bots do not count) */
static void MH_EndReady( mhEndLayout_t *e, const char *fire, const char *tab ) {
	static char fireOk[48];
	int i, humans = 0, ready = 0;
	qboolean mine = ( cg.snap->ps.stats[STAT_CLIENTS_READY] & ( 1 << cg.snap->ps.clientNum ) ) != 0;
	Com_sprintf( fireOk, sizeof( fireOk ), "%s \xb7 OK", fire );
	e->keyName[0] = "FIRE";
	e->keyAction[0] = mine ? fireOk : fire;
	e->keyName[1] = "TAB";
	e->keyAction[1] = tab;
	e->keyName[2] = "ESC";
	e->keyAction[2] = "MENU";
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !cgs.clientinfo[i].infoValid || cgs.clientinfo[i].botSkill ) continue;
		humans++;
		if ( i < 32 && ( cg.snap->ps.stats[STAT_CLIENTS_READY] & ( 1 << i ) ) ) ready++;
	}
	if ( humans < 1 ) humans = 1;
	if ( ready >= humans ) Q_strncpyz( mhEnd.readyLabel, "EVERYBODY READY", sizeof( mhEnd.readyLabel ) );
	else Com_sprintf( mhEnd.readyLabel, sizeof( mhEnd.readyLabel ), "WAITING FOR %d DRIVER%s", humans - ready, humans - ready == 1 ? "" : "S" );
	Com_sprintf( mhEnd.readyValue, sizeof( mhEnd.readyValue ), "%d / %d READY", ready, humans );
	mhEnd.readyFrac = (float)ready / humans;
	e->readyLabel = mhEnd.readyLabel;
	e->readyValue = mhEnd.readyValue;
}

/* -------------------------------------------------------------------------
   the drag tournament (gen.py Tourney, Tourney16, TourneyEnd): the bracket
   between the matches, the podium after the final; both after the match's
   own result had its five seconds
   ------------------------------------------------------------------------- */

static mhBracketLayout_t mhBracket;
static struct {
	char	sub[96], upName[24], hintText[80];
	char	wins[MH_BRACKET_ROUNDS][MH_BRACKET_SLOTS][4], thirdWins[2][4];
	char	pathRound[MH_PATH_ROWS][24], pathWhat[MH_PATH_ROWS][48];
	const float	*pathColor[MH_PATH_ROWS], *pathTagColor[MH_PATH_ROWS];
	int		self;
} mhTourney;

static const vec4_t mhBracketLine = { 58 / 255.0f, 58 / 255.0f, 58 / 255.0f, 1.0f };	/* #3a3a3a */
static const vec4_t mhBracketNone = { 85 / 255.0f, 85 / 255.0f, 85 / 255.0f, 1.0f };	/* #555 */
static const vec4_t mhBracketBot  = { 102 / 255.0f, 102 / 255.0f, 102 / 255.0f, 1.0f };	/* #666 */

static void MH_TourneyBracket( cgDragBracket_t *br ) {
	CG_BracketBuild( br, cgs.dragTourney.size, cgs.dragTourney.played, cgs.dragTourney.winner );
}

/* the own entrant: a human of the field with this client's name */
static int MH_TourneySelf( void ) {
	char self[MAX_QPATH];
	int i;
	MH_Plain( cgs.clientinfo[cg.snap->ps.clientNum].name, self, sizeof( self ) );
	for ( i = 0; i < cgs.dragTourney.size && i < CG_BRACKET_MAX; i++ ) {
		if ( cgs.dragTourney.human[i] && !Q_stricmp( cgs.dragTourney.name[i], self ) ) return i;
	}
	return -1;
}

/* "SEMI-FINAL" as the big type has it: "Semi-final" */
static void MH_TourneyTitle( const char *in, char *out, int size ) {
	Q_strncpyz( out, in, size );
	if ( out[0] ) Q_strlwr( out + 1 );
}

/* an entrant's matches: the side it rode, the heats either way */
static void MH_TourneyScore( const cgDragBracket_t *br, int match, int entrant, int *mine, int *theirs, int *opponent ) {
	int a, c, side;
	CG_BracketEntrants( br, match, &a, &c );
	side = c == entrant ? 1 : 0;
	*opponent = side ? a : c;
	*mine = cgs.dragTourney.wins[match][side];
	*theirs = cgs.dragTourney.wins[match][!side];
}

/* your way: a line a round, the ones still to come empty */
static int MH_TourneyPath( const cgDragBracket_t *br, int self, const char **round, const char **what, const char **tag ) {
	int path[CG_BRACKET_ROUNDS + 1], n, i, count = 0, last = -1, r;
	qboolean alive = qtrue;
	if ( self < 0 ) return 0;
	n = CG_BracketPath( br, self, path, CG_BRACKET_ROUNDS + 1 );
	for ( i = 0; i < n && count < MH_PATH_ROWS; i++ ) {
		int m = path[i], mine, theirs, opp;
		const char *oppName;
		MH_TourneyScore( br, m, self, &mine, &theirs, &opp );
		oppName = opp >= 0 ? cgs.dragTourney.name[opp] : "?";
		MH_TourneyTitle( CG_BracketMatchName( br, m ), mhTourney.pathRound[count], sizeof( mhTourney.pathRound[count] ) );
		Q_strupr( mhTourney.pathRound[count] );
		if ( m < br->played ) {
			qboolean won;
			int a, c;
			CG_BracketEntrants( br, m, &a, &c );
			won = ( br->winner[m] ? c : a ) == self;
			Com_sprintf( mhTourney.pathWhat[count], sizeof( mhTourney.pathWhat[count] ), won ? "beat %s %d-%d" : "lost to %s %d-%d",
			             oppName, mine, theirs );
			tag[count] = won ? "WON" : m == br->size - 4 + 0 || m == br->size - 3 ? "LOST" : "OUT";
			mhTourney.pathColor[count] = won ? mhWhite : mhDim;
			mhTourney.pathTagColor[count] = won ? mhFast : mhAccent;
			if ( !won && tag[count][0] == 'O' ) alive = qfalse;
		} else if ( m == br->played ) {
			Com_sprintf( mhTourney.pathWhat[count], sizeof( mhTourney.pathWhat[count] ), "against %s", oppName );
			tag[count] = "UP NEXT";
			mhTourney.pathColor[count] = mhAccent;
			mhTourney.pathTagColor[count] = mhAccent;
		} else {	/* later on: the rival, once known */
			if ( opp >= 0 ) Com_sprintf( mhTourney.pathWhat[count], sizeof( mhTourney.pathWhat[count] ), "against %s", oppName );
			else mhTourney.pathWhat[count][0] = '\0';
			tag[count] = "";
			mhTourney.pathColor[count] = mhMuted;
			mhTourney.pathTagColor[count] = mhBracketNone;
		}
		round[count] = mhTourney.pathRound[count];
		what[count] = mhTourney.pathWhat[count];
		last = m;
		count++;
	}
	/* the rounds still to come, while it is in */
	if ( alive && last >= 0 && last < br->size - 2 ) {
		for ( r = CG_BracketRound( br, last ) + 1; r < br->rounds && count < MH_PATH_ROWS; r++ ) {
			Q_strncpyz( mhTourney.pathRound[count], CG_BracketMatchName( br, CG_BracketMatch( br, r, 0 ) ), sizeof( mhTourney.pathRound[count] ) );
			mhTourney.pathWhat[count][0] = '\0';
			round[count] = mhTourney.pathRound[count];
			what[count] = mhTourney.pathWhat[count];
			tag[count] = "";
			mhTourney.pathColor[count] = mhBracketNone;
			mhTourney.pathTagColor[count] = mhBracketNone;
			count++;
		}
	}
	return count;
}

/* the end screen's frame on a narrow screen, as MH_EndPrepare has it */
static void MH_TourneyFrame( mhEndLayout_t *e ) {
	float viewWidth = cgs.glconfig.vidWidth / mhScale;
	mhEnd.scale = viewWidth < 1600.0f ? viewWidth / 1600.0f : 1.0f;
	e->viewWidth = viewWidth / mhEnd.scale;
	e->top = ( MH_DESIGN_HEIGHT / mhEnd.scale - MH_DESIGN_HEIGHT ) * 0.5f;
}

static void MH_TourneyEntrant( mhBracketEntrant_t *out, const cgDragBracket_t *br, int entrant, int match, int side, char *wins, int size ) {
	memset( out, 0, sizeof( *out ) );
	out->wins = "";
	if ( entrant < 0 ) return;
	out->name = cgs.dragTourney.name[entrant];
	out->bot = !cgs.dragTourney.human[entrant];
	out->me = entrant == mhTourney.self;
	if ( match >= 0 && match < br->played ) {
		Com_sprintf( wins, size, "%d", cgs.dragTourney.wins[match][side] );
		out->wins = wins;
		out->out = br->winner[match] != side;
	}
}

/* between the matches: the bracket */
static void MH_BracketPrepare( const cgDragBracket_t *br ) {
	mhBracketLayout_t *b = &mhBracket;
	mhEndLayout_t *e = &b->frame;
	int r, j, a, c, m = br->played;

	memset( b, 0, sizeof( *b ) );
	MH_TourneyFrame( e );
	mhTourney.self = MH_TourneySelf();
	e->title = "DRAG TOURNAMENT";
	Com_sprintf( mhTourney.sub, sizeof( mhTourney.sub ), "%d DRIVERS \xb7 BEST OF %d \xb7 MATCH %d OF %d",
	             br->size, cgs.dragTourney.bestOf, m + 1, br->size );
	e->subtitle = mhTourney.sub;

	/* the match up next, who rides it */
	b->upLabel = "UP NEXT";
	MH_TourneyTitle( CG_BracketMatchName( br, m ), mhTourney.upName, sizeof( mhTourney.upName ) );
	b->upName = mhTourney.upName;
	CG_BracketEntrants( br, m, &a, &c );
	b->vsName[0] = a >= 0 ? cgs.dragTourney.name[a] : NULL;
	b->vsName[1] = c >= 0 ? cgs.dragTourney.name[c] : NULL;
	b->vsTag[0] = a < 0 ? "" : a == mhTourney.self ? "YOU" : !cgs.dragTourney.human[a] ? "BOT" : "";
	b->vsTag[1] = c < 0 ? "" : c == mhTourney.self ? "YOU" : !cgs.dragTourney.human[c] ? "BOT" : "";
	if ( a >= 0 && c >= 0 && !cgs.dragTourney.human[a] && !cgs.dragTourney.human[c] ) {
		b->hintLabel = "A BOT MATCH";
		b->hintText = "/dragskip skips it, /dragskip all skips to your next match.";
	}
	b->pathCount = MH_TourneyPath( br, mhTourney.self, b->pathRound, b->pathWhat, b->pathTag );

	/* the bracket */
	b->size = br->size;
	for ( r = 0; r < br->rounds && r < MH_BRACKET_ROUNDS; r++ ) {
		for ( j = 0; j < br->size >> r; j++ ) {
			MH_TourneyEntrant( &b->slot[r][j], br, br->slot[r][j], CG_BracketMatch( br, r, j / 2 ), j & 1,
			                   mhTourney.wins[r][j], sizeof( mhTourney.wins[r][j] ) );
		}
		b->head[r] = CG_BracketRoundHead( br, r );
	}
	b->head[br->rounds] = "WINNER";
	for ( j = 0; j < 2; j++ ) {
		MH_TourneyEntrant( &b->third[j], br, br->third[j], br->size - 2, j, mhTourney.thirdWins[j], sizeof( mhTourney.thirdWins[j] ) );
	}
	b->nextRound = b->nextPair = -1;
	if ( m == br->size - 2 ) b->nextThird = qtrue;
	else if ( m < br->size ) {
		b->nextRound = CG_BracketRound( br, m );
		b->nextPair = m == br->size - 1 ? 0 : m - br->start[b->nextRound];
	}
	MH_EndReady( e, "CONTINUE", "HEAT RESULTS" );
	MH_BracketLayout( b );
}

static void MH_BracketDrawCard( const wsRect_t *card, const wsRect_t *divider, const mhBracketEntrant_t *in,
                                const mhText_t *names, const mhText_t *bots, const mhText_t *wins, const wsRect_t *rows, qboolean next ) {
	int k;
	MH_FillRect( card, mhPanel, 1.0f );
	MH_Border( card, next ? mhAccent : mhLine, 1.0f, next ? mhAccent : mhLine, 1.0f );
	MH_FillRect( divider, mhLine, 1.0f );
	for ( k = 0; k < 2; k++ ) {
		const float *color = !in[k].name ? mhBracketNone : in[k].out ? mhDim : in[k].bot ? mhLabel : mhWhite;
		if ( in[k].me ) MH_FillRect( &rows[k], mhMe, 1.0f );
		MH_Str( &names[k], color, 1.0f );
		MH_Str( &bots[k], mhBracketBot, 1.0f );
		MH_Str( &wins[k], in[k].out ? mhDim : mhWhite, 1.0f );
	}
}

static void MH_BracketDraw( void ) {
	mhBracketLayout_t *b = &mhBracket;
	mhEndLayout_t *e = &b->frame;
	float base = mhScale;
	int r, j, i;

	WS_FillRect( 0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, mhVeil );
	mhScale = base * mhEnd.scale;
	MH_Str( &e->titleT, mhAccent, 1.0f );
	MH_Str( &e->subtitleT, mhMuted, 1.0f );

	MH_Str( &b->upLabelT, mhAccent, 1.0f );
	MH_Str( &b->upNameT, mhWhite, 1.0f );
	for ( i = 0; i < 2; i++ ) {
		MH_Str( &b->vsNameT[i], !Q_stricmp( b->vsTag[i], "YOU" ) ? mhAccent : mhWhite, 1.0f );
		MH_Str( &b->vsTagT[i], !Q_stricmp( b->vsTag[i], "YOU" ) ? mhAccent : mhDim, 1.0f );
	}
	MH_Str( &b->againstT, mhDim, 1.0f );
	MH_FillRect( &b->sep[0], mhLine, 1.0f );
	if ( b->pathCount ) MH_Str( &b->pathTitleT, mhLabel, 1.0f );
	for ( i = 0; i < b->pathCount; i++ ) {
		MH_Str( &b->pathRoundT[i], mhMuted, 1.0f );
		MH_Str( &b->pathWhatT[i], mhTourney.pathColor[i], 1.0f );
		MH_Str( &b->pathTagT[i], mhTourney.pathTagColor[i], 1.0f );
	}
	if ( b->hintText ) {
		MH_FillRect( &b->sep[1], mhLine, 1.0f );
		MH_Str( &b->hintLabelT, mhLabel, 1.0f );
		MH_Str( &b->hintTextT, mhMuted, 1.0f );
	}

	for ( i = 0; i < b->lineCount; i++ ) MH_FillRect( &b->line[i], mhBracketLine, 1.0f );
	for ( r = 0; r <= b->rounds; r++ ) MH_Str( &b->headT[r], mhMuted, 1.0f );
	for ( r = 0; r < b->rounds; r++ ) {
		for ( j = 0; j < b->size >> ( r + 1 ); j++ ) {
			MH_BracketDrawCard( &b->card[r][j], &b->divider[r][j], &b->slot[r][2 * j], &b->nameT[r][2 * j], &b->botT[r][2 * j],
			                    &b->winsT[r][2 * j], &b->row[r][2 * j], r == b->nextRound && j == b->nextPair && !b->nextThird );
		}
	}
	MH_Str( &b->upNextT, mhAccent, 1.0f );
	MH_FillRect( &b->winnerCard, mhPanel, 1.0f );
	MH_Border( &b->winnerCard, mhLine, 2.0f, b->champion ? mhAccent : mhLine, 1.0f );
	MH_Pic( &b->trophy, 0.0f, mh.trophy, b->champion ? mhAccent : mhBracketNone, 1.0f );
	MH_Str( &b->winnerT, b->champion ? mhWhite : mhBracketNone, 1.0f );
	MH_Str( &b->thirdLabelT, b->nextThird ? mhAccent : mhMuted, 1.0f );
	MH_BracketDrawCard( &b->thirdCard, &b->thirdDivider, b->third, b->thirdNameT, b->thirdBotT, b->thirdWinsT, b->thirdRow,
	                    b->nextThird );

	for ( i = 0; i < 3; i++ ) {
		MH_Str( &e->keyNameT[i], mhWhite, 1.0f );
		MH_Str( &e->keyActionT[i], i == 0 && strstr( e->keyAction[0], "OK" ) ? mhAccent : mhDim, 1.0f );
	}
	MH_Str( &e->readyLabelT, mhLabel, 1.0f );
	MH_Str( &e->readyValueT, mhWhite, 1.0f );
	MH_Bar( &e->readyBar, mhEnd.readyFrac, mhAccent, mhTrack, 1.0f );
	mhScale = base;
}

/* after the final: the end screen's page for the whole tournament */
static void MH_EndTourney( void ) {
	mhEndLayout_t *e = &mhEnd.e;
	cgDragBracket_t br;
	int place[3], i, k, self, final, third, a, c, mine, theirs, opp, path[CG_BRACKET_ROUNDS + 1], n;
	int matchesWon = 0, matchesLost = 0, heatsWon = 0, heatsLost = 0, view, last;
	int bestHeats = -1, bestHeatsWho = -1, bestHeatsMatches = 0;

	MH_TourneyBracket( &br );
	final = br.size - 1;
	third = br.size - 2;
	self = mhTourney.self = MH_TourneySelf();
	CG_BracketEntrants( &br, final, &a, &c );
	place[0] = cgs.dragTourney.winner[final] ? c : a;
	place[1] = cgs.dragTourney.winner[final] ? a : c;
	CG_BracketEntrants( &br, third, &a, &c );
	place[2] = br.rounds >= 2 ? ( cgs.dragTourney.winner[third] ? c : a ) : -1;
	view = self >= 0 ? self : place[0];

	/* clear what the match's own page put up */
	e->slipCount = 0;
	e->lapCount = 0;
	e->spreadCount = 0;
	e->teamPodium = qfalse;
	mhEnd.drag = qfalse;

	e->title = "DRAG TOURNAMENT";
	Com_sprintf( mhEnd.sub, sizeof( mhEnd.sub ), "%d DRIVERS \xb7 BEST OF %d \xb7 %s WINS", br.size, cgs.dragTourney.bestOf,
	             place[0] >= 0 ? cgs.dragTourney.name[place[0]] : "NOBODY" );
	Q_strupr( mhEnd.sub );
	e->subtitle = mhEnd.sub;

	/* the left: where you ended, your last match, your figures, your way */
	k = CG_BracketPlace( &br, view );
	Com_sprintf( mhEnd.pos, sizeof( mhEnd.pos ), "P%d", k > 0 ? k : 1 );
	Com_sprintf( mhEnd.of, sizeof( mhEnd.of ), "/ %d", br.size );
	n = CG_BracketPath( &br, view, path, CG_BRACKET_ROUNDS + 1 );
	for ( i = 0; i < n; i++ ) {
		if ( path[i] >= br.played ) continue;
		MH_TourneyScore( &br, path[i], view, &mine, &theirs, &opp );
		heatsWon += mine;
		heatsLost += theirs;
		CG_BracketEntrants( &br, path[i], &a, &c );
		if ( ( br.winner[path[i]] ? c : a ) == view ) matchesWon++;
		else matchesLost++;
	}
	last = n > 0 ? path[n - 1] : final;
	MH_TourneyScore( &br, last, view, &mine, &theirs, &opp );
	Com_sprintf( mhEnd.doneTime, sizeof( mhEnd.doneTime ), "%d-%d", mine, theirs );
	if ( self < 0 ) Com_sprintf( mhEnd.doneLabel, sizeof( mhEnd.doneLabel ), "%s WINS", cgs.dragTourney.name[view] );
	else if ( k == 1 ) Q_strncpyz( mhEnd.doneLabel, "YOU WIN THE TOURNAMENT", sizeof( mhEnd.doneLabel ) );
	else if ( k == 2 ) Q_strncpyz( mhEnd.doneLabel, "YOU FINISH SECOND", sizeof( mhEnd.doneLabel ) );
	else if ( k == 3 ) Q_strncpyz( mhEnd.doneLabel, "YOU FINISH THIRD", sizeof( mhEnd.doneLabel ) );
	else if ( k == 4 ) Q_strncpyz( mhEnd.doneLabel, "YOU FINISH FOURTH", sizeof( mhEnd.doneLabel ) );
	else Com_sprintf( mhEnd.doneLabel, sizeof( mhEnd.doneLabel ), "YOU WENT OUT IN THE %s", CG_BracketMatchName( &br, last ) );
	Q_strupr( mhEnd.doneLabel );
	e->doneLabel = mhEnd.doneLabel;
	{
		qboolean won = mine > theirs;
		const char *what = last == final ? "THE FINAL" : last == third ? "THIRD PLACE" : CG_BracketMatchName( &br, last );
		Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), won ? "WON %s%s AGAINST %s" : "LOST %s%s TO %s",
		             last == final || last == third ? "" : "THE ", what, opp >= 0 ? cgs.dragTourney.name[opp] : "?" );
		Q_strupr( mhEnd.doneNote );
		mhEnd.doneNoteColor = won ? mhFast : mhAccent;
	}
	e->statCount = 4;
	e->statLabel[0] = "MATCHES WON";
	e->statLabel[1] = "MATCHES LOST";
	e->statLabel[2] = "HEATS WON";
	e->statLabel[3] = "HEATS LOST";
	Com_sprintf( mhEnd.stat[0], sizeof( mhEnd.stat[0] ), "%d", matchesWon );
	Com_sprintf( mhEnd.stat[1], sizeof( mhEnd.stat[1] ), "%d", matchesLost );
	Com_sprintf( mhEnd.stat[2], sizeof( mhEnd.stat[2] ), "%d", heatsWon );
	Com_sprintf( mhEnd.stat[3], sizeof( mhEnd.stat[3] ), "%d", heatsLost );
	for ( i = 0; i < 4; i++ ) {
		e->statUnit[i] = NULL;
		mhEnd.statColor[i] = mhWhite;
	}
	e->pathCount = MH_TourneyPath( &br, view, e->pathRound, e->pathWhat, e->pathTag );

	/* the podium: the champion, the finalist, third place, each with the way there */
	e->podiumCount = 0;
	for ( i = 0; i < 3; i++ ) {
		static const char *tags[3] = { "CHAMPION", "FINALIST", "THIRD" };
		int pn, q;
		if ( place[i] < 0 ) break;
		mhEnd.client[i] = -1;
		mhEnd.podCarValid[i] = MH_TourneyCar( place[i], &mhEnd.podCar[i] );
		Q_strncpyz( mhEnd.podName[i], cgs.dragTourney.name[place[i]], sizeof( mhEnd.podName[i] ) );
		mhEnd.podTime[i][0] = '\0';
		pn = CG_BracketPath( &br, place[i], path, CG_BRACKET_ROUNDS + 1 );
		for ( q = 0; q < pn; q++ ) {
			if ( i == 2 && path[q] != third ) continue;
			MH_TourneyScore( &br, path[q], place[i], &mine, &theirs, &opp );
			if ( mhEnd.podTime[i][0] ) Q_strcat( mhEnd.podTime[i], sizeof( mhEnd.podTime[i] ), " \xb7 " );
			Q_strcat( mhEnd.podTime[i], sizeof( mhEnd.podTime[i] ), va( i == 2 ? "THIRD PLACE %d-%d" : "%d-%d", mine, theirs ) );
		}
		mhEnd.podGap[i][0] = '\0';
		e->podiumName[i] = mhEnd.podName[i];
		e->podiumTime[i] = mhEnd.podTime[i];
		e->podiumGap[i] = mhEnd.podGap[i];
		e->podiumTag[i] = tags[i];
		e->podiumMe[i] = place[i] == self;
		e->podiumCount = i + 1;
	}

	/* the highlights: a clean sweep, the most heats won */
	e->awardCount = 0;
	if ( place[0] >= 0 ) {
		int won = 0, lost = 0;
		n = CG_BracketPath( &br, place[0], path, CG_BRACKET_ROUNDS + 1 );
		for ( i = 0; i < n; i++ ) {
			MH_TourneyScore( &br, path[i], place[0], &mine, &theirs, &opp );
			won += mine;
			lost += theirs;
		}
		if ( !lost && won > 0 ) {
			k = e->awardCount++;
			e->awardLabel[k] = "CLEAN SWEEP";
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 no heat lost", cgs.dragTourney.name[place[0]] );
			Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d-0", won );
			mhEnd.awardColor[k] = mhFast;
			mhEnd.awardIcon[k] = mh.flag;
		}
	}
	for ( i = 0; i < br.size; i++ ) {
		int won = 0, q;
		n = CG_BracketPath( &br, i, path, CG_BRACKET_ROUNDS + 1 );
		for ( q = 0; q < n; q++ ) {
			MH_TourneyScore( &br, path[q], i, &mine, &theirs, &opp );
			won += mine;
		}
		if ( won > bestHeats ) {
			bestHeats = won;
			bestHeatsWho = i;
			bestHeatsMatches = n;
		}
	}
	if ( bestHeatsWho >= 0 && bestHeats > 0 ) {
		k = e->awardCount++;
		e->awardLabel[k] = "MOST HEATS WON";
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 %d matches", cgs.dragTourney.name[bestHeatsWho], bestHeatsMatches );
		Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", bestHeats );
		mhEnd.awardColor[k] = mhPurple;
		mhEnd.awardIcon[k] = mh.trophy;
	}
	for ( i = 0; i < e->awardCount; i++ ) {
		e->awardName[i] = mhEnd.awardName[i];
		e->awardValue[i] = mhEnd.awardValue[i];
	}

	/* the right: every match in the order they were raced */
	e->listTitle = "EVERY MATCH";
	Com_sprintf( mhEnd.listRight, sizeof( mhEnd.listRight ), "%d DRIVERS", br.size );
	e->listRight = mhEnd.listRight;
	e->listHead[0] = "RND";
	e->listHead[1] = "WINNER";
	e->listHead[2] = "HEATS";
	e->listHead[3] = "BEATEN";
	e->listMatches = qtrue;
	e->rowSwatch = qfalse;
	e->rowCount = 0;
	e->compact = br.size > 12;
	for ( i = 0; i < br.size && e->rowCount < MH_END_ROWS; i++ ) {
		int w;
		CG_BracketEntrants( &br, i, &a, &c );
		if ( a < 0 || c < 0 ) continue;
		k = e->rowCount++;
		w = cgs.dragTourney.winner[i];
		Q_strncpyz( mhEnd.rowPos[k], CG_BracketMatchShort( &br, i ), sizeof( mhEnd.rowPos[k] ) );
		Q_strncpyz( mhEnd.rowName[k], cgs.dragTourney.name[w ? c : a], sizeof( mhEnd.rowName[k] ) );
		Com_sprintf( mhEnd.rowTime[k], sizeof( mhEnd.rowTime[k] ), "%d-%d", cgs.dragTourney.wins[i][w], cgs.dragTourney.wins[i][!w] );
		Q_strncpyz( mhEnd.rowBest[k], cgs.dragTourney.name[w ? a : c], sizeof( mhEnd.rowBest[k] ) );
		mhEnd.rowMe[k] = self >= 0 && ( w ? c : a ) == self;
		mhEnd.rowClient[k] = -1;
		e->rowPos[k] = mhEnd.rowPos[k];
		e->rowName[k] = mhEnd.rowName[k];
		e->rowTime[k] = mhEnd.rowTime[k];
		e->rowBest[k] = mhEnd.rowBest[k];
	}
	MH_EndReady( e, "NEXT TOURNAMENT", "HEAT RESULTS" );
}

void CG_ModernHUD_DrawTourney( void ) {
	cgDragBracket_t br;
	if ( !cg.snap || !cgs.dragTourney.size ) return;
	MH_Register();
	MH_TourneyBracket( &br );
	if ( br.played >= br.size ) {
		mhEnd.tourney = qtrue;
		CG_ModernHUD_DrawEnd();
		mhEnd.tourney = qfalse;
		return;
	}
	mhScale = cgs.glconfig.vidHeight / MH_DESIGN_HEIGHT;
	MH_BracketPrepare( &br );
	MH_BracketDraw();
}

/* ---------------------------------------------------------------------------
   the end screen of a sprint (gen.py EndSprint): one run from A to B, also an
   A to B course in a race. In place of the laps your sectors, checkpoint to
   checkpoint and the last one to the finish: what each lost to the quickest
   anybody drove there, your place at its end under it. The highlights: the
   best sector, the closest finish, the biggest gain; the classification
   with the average speed.
   ------------------------------------------------------------------------- */

#define MH_END_SPRINT_PARTS	8	/* the sector chart's columns */
#define MH_END_LADDER_SHOWN	6	/* a ghost race: the ladder's rows around yours */

static qboolean MH_EndIsSprint( void ) {
	return cgs.gametype == GT_SPRINT || ( cgs.gametype == GT_RACING && CG_IsSprintTrack() );
}

/* what a sector lost: "+0.42" "+12.4" "+1:02" */
static void MH_EndSprintLost( int ms, char *out, int size ) {
	if ( ms < 10000 ) Com_sprintf( out, size, "+%d.%02d", ms / 1000, ( ms % 1000 ) / 10 );
	else if ( ms < 60000 ) Com_sprintf( out, size, "+%d.%d", ms / 1000, ( ms % 1000 ) / 100 );
	else Com_sprintf( out, size, "+%d:%02d", ms / 60000, ( ms / 1000 ) % 60 );
}

/* a stretch of the run, sectors from .. to: ms, 0: not all driven */
static int MH_EndSprintPart( const cgRaceResult_t *r, int from, int to, int cps ) {
	int sct, sum = 0;
	for ( sct = from; sct <= to; sct++ ) {
		int t = CG_ResultSector( r, sct, cps );
		if ( !t ) return 0;
		sum += t;
	}
	return sum;
}

/* over the whole run, in the player's units; 0: not home or no length */
static float MH_EndAvgSpeed( const cgRaceResult_t *r ) {
	int total = CG_ResultTotal( r );
	if ( total <= 0 || cgs.trackLength <= 0.0f ) return 0.0f;
	return cgs.trackLength / ( total / 1000.0f ) * ( cg_metricUnits.integer ? 3.6f : 2.236936f );
}

static void MH_EndSprint( const cgRaceResult_t *all, const int *order, int count, int me, const cgRaceResult_t *mine, int place, const char *map ) {
	mhEndLayout_t *e = &mhEnd.e;
	int cps = CG_ResultCheckpoints( all, MAX_CLIENTS ), sectors = cps + 1, quick[CG_RESULT_CPS + 1];
	int parts = sectors < MH_END_SPRINT_PARTS ? sectors : MH_END_SPRINT_PARTS, partEnd[MH_END_SPRINT_PARTS + 1], partQuick[MH_END_SPRINT_PARTS];
	int i, k, sct, worst = 0, quickest = 0, prev, best = 0, bestSct = 0, bestClient = -1;
	int closest = 0, closeAt = -1, gain = 0, gainClient = -1;
	char dist[16], name[40], other[40];
	const char *speedUnit = cg_metricUnits.integer ? "KM/H" : "MPH";
	float avg;

	/* the header: the length, the checkpoints */
	dist[0] = '\0';
	if ( cgs.trackLength > 0.0f ) {
		if ( cg_metricUnits.integer ) Com_sprintf( dist, sizeof( dist ), "%.2f KM", cgs.trackLength / 1000.0f );
		else Com_sprintf( dist, sizeof( dist ), "%.2f MI", cgs.trackLength / 1609.344f );
	}
	e->title = "SPRINT RESULT";
	Com_sprintf( mhEnd.sub, sizeof( mhEnd.sub ), "%s%s%s \xb7 %d CHECKPOINT%s \xb7 %d DRIVERS", map, dist[0] ? " \xb7 " : "", dist,
	             cps, cps == 1 ? "" : "S", count );
	if ( !mine->finishTime ) {
		Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "OUT AFTER CP %d OF %d", CG_ResultReached( mine ), cps );
	}

	/* the quickest anybody drove each sector */
	for ( sct = 1; sct <= sectors; sct++ ) {
		quick[sct] = 0;
		for ( i = 0; i < count; i++ ) {
			int t = CG_ResultSector( &all[order[i]], sct, cps );
			if ( t && ( !quick[sct] || t < quick[sct] ) ) quick[sct] = t;
			if ( t && ( !best || t < best ) ) { best = t; bestSct = sct; bestClient = order[i]; }
		}
		if ( quick[sct] && CG_ResultSector( mine, sct, cps ) == quick[sct] ) quickest++;
	}

	/* the figures: the average speed and the sectors quickest in place of the laps */
	avg = MH_EndAvgSpeed( mine );
	e->statLabel[0] = "AVG SPEED";
	if ( avg > 0.0f ) {
		Com_sprintf( mhEnd.stat[0], sizeof( mhEnd.stat[0] ), "%.1f", avg );
		e->statUnit[0] = speedUnit;
	} else {
		Q_strncpyz( mhEnd.stat[0], "--", sizeof( mhEnd.stat[0] ) );
		e->statUnit[0] = NULL;
	}
	e->statLabel[3] = "SECTORS QUICKEST";
	Com_sprintf( mhEnd.stat[3], sizeof( mhEnd.stat[3] ), "%d / %d", quickest, sectors );
	mhEnd.statColor[3] = quickest > 0 ? mhFast : mhWhite;

	/* your sectors: what each lost, your place at its end. Past eight the
	 * run in eight stretches, named by the checkpoint they end at */
	e->chartTitle = "YOUR SECTORS";
	e->chartNote = "LOST TO THE QUICKEST \xb7 YOUR PLACE";
	e->lapCount = 0;
	partEnd[0] = 0;
	for ( k = 1; k <= parts; k++ ) partEnd[k] = ( k * sectors + parts / 2 ) / parts;
	for ( k = 0; k < parts; k++ ) {
		int t = MH_EndSprintPart( mine, partEnd[k] + 1, partEnd[k + 1], cps );
		partQuick[k] = 0;
		for ( i = 0; i < count; i++ ) {
			int q = MH_EndSprintPart( &all[order[i]], partEnd[k] + 1, partEnd[k + 1], cps );
			if ( q && ( !partQuick[k] || q < partQuick[k] ) ) partQuick[k] = q;
		}
		if ( t && partQuick[k] && t - partQuick[k] > worst ) worst = t - partQuick[k];
	}
	prev = mine->grid;
	for ( k = 0; k < parts; k++ ) {
		int t = MH_EndSprintPart( mine, partEnd[k] + 1, partEnd[k + 1], cps ), at = CG_ResultPlaceAt( all, MAX_CLIENTS, me, partEnd[k + 1], cps );
		sct = partEnd[k + 1];
		e->lapCount++;
		if ( !t ) {
			Q_strncpyz( mhEnd.lap[k], "--", sizeof( mhEnd.lap[k] ) );
			e->lapFrac[k] = 0.0f;
		} else if ( t <= partQuick[k] ) {
			Q_strncpyz( mhEnd.lap[k], "best", sizeof( mhEnd.lap[k] ) );
			e->lapFrac[k] = 0.0f;
		} else {
			MH_EndSprintLost( t - partQuick[k], mhEnd.lap[k], sizeof( mhEnd.lap[k] ) );
			e->lapFrac[k] = worst > 0 ? (float)( t - partQuick[k] ) / worst : 0.0f;
		}
		if ( sct > cps ) Q_strncpyz( mhEnd.lapName[k], "FIN", sizeof( mhEnd.lapName[k] ) );
		else if ( parts < sectors ) Com_sprintf( mhEnd.lapName[k], sizeof( mhEnd.lapName[k] ), "CP%d", sct );
		else Com_sprintf( mhEnd.lapName[k], sizeof( mhEnd.lapName[k] ), "S%d", sct );
		if ( at ) Com_sprintf( mhEnd.lapPlace[k], sizeof( mhEnd.lapPlace[k] ), "P%d", at );
		else Q_strncpyz( mhEnd.lapPlace[k], "--", sizeof( mhEnd.lapPlace[k] ) );
		mhEnd.lapPlaceLit[k] = at && at != prev;
		if ( at ) prev = at;
		e->lapValue[k] = mhEnd.lap[k];
		e->lapName[k] = mhEnd.lapName[k];
		e->lapPlace[k] = mhEnd.lapPlace[k];
	}

	/* the highlights */
	e->awardCount = 0;
	mhEnd.fastest = -1;
	for ( i = 1; i < count; i++ ) {
		const cgRaceResult_t *a = &all[order[i - 1]], *b = &all[order[i]];
		if ( a->finishTime && b->finishTime && ( closeAt < 0 || b->finishTime - a->finishTime < closest ) ) {
			closest = b->finishTime - a->finishTime;
			closeAt = i;
		}
	}
	for ( i = 0; i < count; i++ ) {
		const cgRaceResult_t *r = &all[order[i]];
		if ( r->finishTime && r->grid > 0 && r->grid - ( i + 1 ) > gain ) { gain = r->grid - ( i + 1 ); gainClient = order[i]; }
	}
	if ( bestClient >= 0 ) {
		k = e->awardCount++;
		e->awardLabel[k] = "BEST SECTOR";
		MH_Plain( cgs.clientinfo[bestClient].name, name, sizeof( name ) );
		if ( bestSct > cps ) Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 to the finish", name );
		else Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 S%d", name, bestSct );
		CG_ResultTime( best, mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ) );
		mhEnd.awardColor[k] = mhFast;
		mhEnd.awardIcon[k] = mh.flag;
	}
	if ( closeAt > 0 ) {
		k = e->awardCount++;
		e->awardLabel[k] = "CLOSEST FINISH";
		MH_Plain( cgs.clientinfo[order[closeAt]].name, name, sizeof( name ) );
		MH_Plain( cgs.clientinfo[order[closeAt - 1]].name, other, sizeof( other ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 behind %s", name, other );
		Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d.%03d", closest / 1000, closest % 1000 );
		mhEnd.awardColor[k] = mhPurple;
		mhEnd.awardIcon[k] = mh.trophy;
	}
	if ( gainClient >= 0 && e->awardCount < MH_END_AWARDS ) {
		int to = 0;
		for ( i = 0; i < count; i++ ) if ( order[i] == gainClient ) to = i + 1;
		k = e->awardCount++;
		e->awardLabel[k] = "BIGGEST GAIN";
		MH_Plain( cgs.clientinfo[gainClient].name, name, sizeof( name ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 P%d to P%d", name, all[gainClient].grid, to );
		Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "+%d", gain );
		mhEnd.awardColor[k] = mhAccent;
		mhEnd.awardIcon[k] = mh.chevron;
	}

	/* the classification: the average speed, or how far one got */
	Com_sprintf( mhEnd.avgHead, sizeof( mhEnd.avgHead ), "AVG %s", speedUnit );
	e->listHead[0] = "POS";
	e->listHead[1] = "DRIVER";
	e->listHead[2] = "TIME";
	e->listHead[3] = mhEnd.avgHead;
	Q_strncpyz( mhEnd.listRight, dist, sizeof( mhEnd.listRight ) );
	for ( k = 0; k < e->rowCount; k++ ) {
		const cgRaceResult_t *r = &all[mhEnd.rowClient[k]];
		avg = MH_EndAvgSpeed( r );
		if ( avg > 0.0f ) Com_sprintf( mhEnd.rowBest[k], sizeof( mhEnd.rowBest[k] ), "%.1f", avg );
		else if ( CG_ResultReached( r ) ) Com_sprintf( mhEnd.rowBest[k], sizeof( mhEnd.rowBest[k] ), "CP %d", CG_ResultReached( r ) );
		else Q_strncpyz( mhEnd.rowBest[k], "--", sizeof( mhEnd.rowBest[k] ) );
	}
	(void)place;
}

/* ---------------------------------------------------------------------------
   the end screen of a ghost race (gen.py EndGhost): you against a recorded
   lap. The margin big; your best lap against the ghost sector by sector
   (bars up: gained, down: lost) with the running gap under it; you and the
   ghost side by side in place of the podium; everybody's result against
   their ghost, and the ghost ladder with your time put in.
   ------------------------------------------------------------------------- */

/* "-0.842" "+0.214" */
static void MH_EndSigned( int ms, char *out, int size ) {
	int a = ms < 0 ? -ms : ms;
	Com_sprintf( out, size, "%c%d.%03d", ms < 0 ? '-' : '+', a / 1000, a % 1000 );
}

/* "-0.21" "+0.12": hundredths */
static void MH_EndSigned2( int ms, char *out, int size ) {
	int a = ( ms < 0 ? -ms : ms ) / 10;
	Com_sprintf( out, size, "%c%d.%02d", ms < 0 ? '-' : '+', a / 100, a % 100 );
}

/* a driver's best lap and the ghost's: the server's result, ours until it
 * comes, the best lap alone without a ghost */
static void MH_GhostOf( const cgRaceResult_t *all, int client, int *best, int *ghostMs, char *name, int nameSize ) {
	*best = CG_ResultBestLap( &all[client] );
	*ghostMs = 0;
	if ( name ) name[0] = '\0';
	if ( cg.ghostRaceResults[client].valid ) {
		if ( cg.ghostRaceResults[client].playerMs > 0 ) *best = cg.ghostRaceResults[client].playerMs;
		*ghostMs = cg.ghostRaceResults[client].ghostMs;
		if ( name ) Q_strncpyz( name, cg.ghostRaceResults[client].name, nameSize );
	} else if ( cg.snap && client == cg.snap->ps.clientNum && ( cg.ghostResultValid || cg.ghostFinishOnly ) ) {
		/* our own lap as the ghost race times it: from the start line */
		*best = cg.ghostResultPlayerMs;
		*ghostMs = cg.ghostResultValid ? cg.ghostResultGhostMs : 0;
		if ( name ) Q_strncpyz( name, cg.ghostResultName, nameSize );
	}
}

/* the ghost's car once: registering it takes file lookups */
static qboolean MH_GhostCar( const char *name, const char *vehicle, cgShowroomCar_t *car ) {
	static char loaded[MAX_QPATH];
	static cgShowroomCar_t kept;
	static qboolean ok;
	if ( Q_stricmp( loaded, vehicle ) ) {
		Q_strncpyz( loaded, vehicle, sizeof( loaded ) );
		ok = CG_ShowroomCarLoad( name, vehicle, MAX_CLIENTS - 1, &kept );
	}
	*car = kept;
	return ok;
}

/* "Murcielago" from "murcielago/red" */
static void MH_VehicleName( const char *vehicle, char *out, int size ) {
	char *slash;
	Q_strncpyz( out, vehicle, size );
	slash = strchr( out, '/' );
	if ( slash ) *slash = '\0';
	if ( out[0] >= 'a' && out[0] <= 'z' ) out[0] += 'A' - 'a';
}

static void MH_EndGhost( const cgRaceResult_t *all, const int *order, int count, int me, const cgRaceResult_t *mine, const char *map ) {
	mhEndLayout_t *e = &mhEnd.e;
	qboolean local = cg.snap && me == cg.snap->ps.clientNum;
	int best, ghostMs, i, k, n, sorted[MAX_CLIENTS], faster = 0, laps, cps, parts, partEnd[MH_END_SPRINT_PARTS + 1];
	int cum[MAX_GHOST_SPLITS + 1], cumCount = 0, worst = 0, gainMs = 0, gainAt = -1, fastest = 0, fastClient = -1;
	char ghostName[40], tmp[40], top[16];

	MH_GhostOf( all, me, &best, &ghostMs, ghostName, sizeof( ghostName ) );
	/* not home: the ghost raced against (once home, a new personal ghost
	 * may already have replaced it) */
	if ( local && !ghostMs && !mine->finishTime ) CG_GhostRace_Opponent( ghostName, sizeof( ghostName ), &ghostMs );
	e->title = "GHOST RACE RESULT";
	if ( CG_IsSprintTrack() ) {	/* one run from A to B: no laps */
		Com_sprintf( mhEnd.sub, sizeof( mhEnd.sub ), "%s \xb7 A TO B \xb7 %d DRIVER%s", map, count, count == 1 ? "" : "S" );
		mhEnd.listRight[0] = '\0';
	} else {
		Com_sprintf( mhEnd.sub, sizeof( mhEnd.sub ), "%s \xb7 %d LAP%s \xb7 %d DRIVER%s", map, cgs.laplimit, cgs.laplimit == 1 ? "" : "S",
		             count, count == 1 ? "" : "S" );
	}

	/* the margin big, the best lap under it; without a ghost the place */
	e->doneLabel = "BEST LAP";
	if ( best > 0 ) CG_ResultTime( best, mhEnd.doneTime, sizeof( mhEnd.doneTime ) );
	else Q_strncpyz( mhEnd.doneTime, "--", sizeof( mhEnd.doneTime ) );
	if ( best > 0 && ghostMs > 0 ) {
		char lap[16], upper[40];
		MH_EndSigned( best - ghostMs, mhEnd.pos, sizeof( mhEnd.pos ) );
		mhEnd.posColor = best < ghostMs ? mhFast : mhAccent;
		e->posSize = 120.0f;
		e->of = "";
		CG_ResultTime( ghostMs, lap, sizeof( lap ) );
		MH_Plain( ghostName, upper, sizeof( upper ) );
		Q_strupr( upper );
		Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "%s \xb7 %s %s", best < ghostMs ? "GHOST BEATEN" : "GHOST WINS", upper, lap );
		mhEnd.doneNoteColor = best < ghostMs ? mhFast : mhAccent;
	} else {
		Q_strncpyz( mhEnd.doneNote, best > 0 ? "NO GHOST" : "NO LAP", sizeof( mhEnd.doneNote ) );
		mhEnd.doneNoteColor = mhMuted;
	}

	/* the figures: the ladder place, the personal best, top speed, laps faster */
	Q_strncpyz( top, mhEnd.stat[1], sizeof( top ) );
	for ( i = 0; i < 4; i++ ) { e->statUnit[i] = NULL; mhEnd.statColor[i] = mhWhite; }
	e->statLabel[0] = "LADDER PLACE";
	Q_strncpyz( mhEnd.stat[0], "--", sizeof( mhEnd.stat[0] ) );
	if ( local && best > 0 && cg.ladderGhostEntryCount > 0 ) {
		int rank = 1;
		for ( i = 0; i < cg.ladderGhostEntryCount; i++ ) if ( cg.ladderGhostEntries[i].lapMs > 0 && cg.ladderGhostEntries[i].lapMs < best ) rank++;
		Com_sprintf( mhEnd.stat[0], sizeof( mhEnd.stat[0] ), "#%d", rank );
		Com_sprintf( mhEnd.ladderOf, sizeof( mhEnd.ladderOf ), "OF %d", cg.ladderGhostEntryCount );
		e->statUnit[0] = mhEnd.ladderOf;
	}
	e->statLabel[1] = "PERSONAL BEST";
	Q_strncpyz( mhEnd.stat[1], "--", sizeof( mhEnd.stat[1] ) );
	if ( local && best > 0 ) {
		if ( !cg.ghostPrevPersonalMs ) {
			Q_strncpyz( mhEnd.stat[1], "NEW", sizeof( mhEnd.stat[1] ) );
			mhEnd.statColor[1] = mhFast;
		} else {
			MH_EndSigned2( best - cg.ghostPrevPersonalMs, mhEnd.stat[1], sizeof( mhEnd.stat[1] ) );
			if ( best < cg.ghostPrevPersonalMs ) { mhEnd.statColor[1] = mhFast; e->statUnit[1] = "NEW"; }
		}
	}
	e->statLabel[2] = "TOP SPEED";
	Q_strncpyz( mhEnd.stat[2], top, sizeof( mhEnd.stat[2] ) );
	if ( Q_stricmp( top, "--" ) ) e->statUnit[2] = cg_metricUnits.integer ? "KM/H" : "MPH";
	e->statLabel[3] = "LAPS FASTER";
	laps = mine->lapCount < CG_RESULT_LAPS ? mine->lapCount : CG_RESULT_LAPS;
	for ( i = 0; i < laps; i++ ) if ( ghostMs > 0 && mine->laps[i] > 0 && mine->laps[i] < ghostMs ) faster++;
	if ( laps == 1 ) faster = best > 0 && best < ghostMs;	/* one lap (A to B): timed from the start line */
	if ( ghostMs > 0 && laps > 0 ) {
		Com_sprintf( mhEnd.stat[3], sizeof( mhEnd.stat[3] ), "%d / %d", faster, laps );
		if ( faster ) mhEnd.statColor[3] = mhFast;
	} else Q_strncpyz( mhEnd.stat[3], "--", sizeof( mhEnd.stat[3] ) );

	/* your best lap against the ghost: the running gap at each checkpoint,
	 * the finish the lap's margin; past eight sectors in eight stretches */
	if ( local && ghostMs > 0 && cg.ghostBestSplitCount > 0 && cg.ghostBestSplitLapMs == best ) {
		n = cg.ghostBestSplitCount < MAX_GHOST_SPLITS ? cg.ghostBestSplitCount : MAX_GHOST_SPLITS;
		for ( i = 0; i < n; i++ ) cum[cumCount++] = cg.ghostBestSplitDeltaAt[i];
		cum[cumCount++] = best - ghostMs;
		cps = cumCount;
		parts = cps < MH_END_SPRINT_PARTS ? cps : MH_END_SPRINT_PARTS;
		partEnd[0] = 0;
		for ( k = 1; k <= parts; k++ ) partEnd[k] = ( k * cps + parts / 2 ) / parts;
		for ( k = 0; k < parts; k++ ) {
			int d = cum[partEnd[k + 1] - 1] - ( partEnd[k] > 0 ? cum[partEnd[k] - 1] : 0 );
			if ( ( d < 0 ? -d : d ) > worst ) worst = d < 0 ? -d : d;
		}
		for ( i = 0; i < cps; i++ ) {	/* the biggest gain: a real sector */
			int d = cum[i] - ( i > 0 ? cum[i - 1] : 0 );
			if ( d < gainMs ) { gainMs = d; gainAt = i; }
		}
		e->chartTitle = "YOUR BEST LAP";
		e->chartNote = "AGAINST THE GHOST \xb7 RUNNING GAP";
		e->lapDiverge = qtrue;
		e->lapCount = 0;
		for ( k = 0; k < parts; k++ ) {
			int last = partEnd[k + 1] - 1, d = cum[last] - ( partEnd[k] > 0 ? cum[partEnd[k] - 1] : 0 );
			e->lapCount++;
			e->lapFrac[k] = worst > 0 ? (float)d / worst : 0.0f;
			MH_EndSigned2( d, mhEnd.lap[k], sizeof( mhEnd.lap[k] ) );
			if ( last == cps - 1 ) Q_strncpyz( mhEnd.lapName[k], "FIN", sizeof( mhEnd.lapName[k] ) );
			else Com_sprintf( mhEnd.lapName[k], sizeof( mhEnd.lapName[k] ), parts < cps ? "CP%d" : "S%d", cg.ghostBestSplitCheckpoint[last] );
			MH_EndSigned2( cum[last], mhEnd.lapPlace[k], sizeof( mhEnd.lapPlace[k] ) );
			mhEnd.lapPlaceColor[k] = cum[last] <= 0 ? mhFast : mhAccent;
			e->lapValue[k] = mhEnd.lap[k];
			e->lapName[k] = mhEnd.lapName[k];
			e->lapPlace[k] = mhEnd.lapPlace[k];
		}
	}

	/* without a ghost the podium stays, with the best laps (a ghost race
	 * times laps, not the race) */
	for ( k = 0; k < e->podiumCount; k++ ) {
		int b, g;
		MH_GhostOf( all, mhEnd.client[k], &b, &g, NULL, 0 );
		if ( b > 0 ) CG_ResultTime( b, mhEnd.podTime[k], sizeof( mhEnd.podTime[k] ) );
		mhEnd.podGap[k][0] = '\0';
	}

	/* you and the ghost in place of the podium */
	if ( ghostMs > 0 && best > 0 ) {
		const char *vehicle = local ? CG_GhostRace_Vehicle() : "";
		e->duel = qtrue;
		e->podiumCount = 0;
		Q_strncpyz( mhEnd.duelTag[0], "YOU", sizeof( mhEnd.duelTag[0] ) );
		MH_Plain( cgs.clientinfo[me].name, mhEnd.duelName[0], sizeof( mhEnd.duelName[0] ) );
		Q_strncpyz( mhEnd.duelLap[0], mhEnd.doneTime, sizeof( mhEnd.duelLap[0] ) );
		Q_strncpyz( mhEnd.duelNote[0], mhEnd.pos, sizeof( mhEnd.duelNote[0] ) );
		mhEnd.duelNoteColor[0] = mhEnd.posColor;
		if ( local && CG_GhostPlaybackMode() == 3 && cg.ladderGhostSelected >= 0 ) {
			int rank = 1;
			for ( i = 0; i < cg.ladderGhostEntryCount; i++ )
				if ( cg.ladderGhostEntries[i].lapMs < cg.ladderGhostEntries[cg.ladderGhostSelected].lapMs ) rank++;
			Com_sprintf( mhEnd.duelTag[1], sizeof( mhEnd.duelTag[1] ), "GHOST \xb7 LADDER #%d", rank );
		} else if ( local && CG_GhostPlaybackMode() == 1 ) Q_strncpyz( mhEnd.duelTag[1], "GHOST \xb7 PERSONAL", sizeof( mhEnd.duelTag[1] ) );
		else if ( local && CG_GhostPlaybackMode() == 2 ) Q_strncpyz( mhEnd.duelTag[1], "GHOST \xb7 SERVER", sizeof( mhEnd.duelTag[1] ) );
		else Q_strncpyz( mhEnd.duelTag[1], "GHOST", sizeof( mhEnd.duelTag[1] ) );
		MH_Plain( ghostName, mhEnd.duelName[1], sizeof( mhEnd.duelName[1] ) );
		CG_ResultTime( ghostMs, mhEnd.duelLap[1], sizeof( mhEnd.duelLap[1] ) );
		mhEnd.duelNoteColor[1] = mhMuted;
		/* the ghost's car: its vehicle, our own one ("") */
		mhEnd.duelCarValid[0] = CG_ShowroomCarOf( me, &mhEnd.duelCar[0] );
		if ( vehicle && vehicle[0] ) {
			MH_VehicleName( vehicle, mhEnd.duelNote[1], sizeof( mhEnd.duelNote[1] ) );
			mhEnd.duelCarValid[1] = MH_GhostCar( ghostName, vehicle, &mhEnd.duelCar[1] );
		} else {
			MH_VehicleName( cgs.clientinfo[me].modelName, mhEnd.duelNote[1], sizeof( mhEnd.duelNote[1] ) );
			mhEnd.duelCar[1] = mhEnd.duelCar[0];
			mhEnd.duelCarValid[1] = mhEnd.duelCarValid[0];
		}
		for ( k = 0; k < 2; k++ ) {
			e->duelTag[k] = mhEnd.duelTag[k];
			e->duelName[k] = mhEnd.duelName[k];
			e->duelLap[k] = mhEnd.duelLap[k];
			e->duelNote[k] = mhEnd.duelNote[k];
		}
	}

	/* everybody by their best lap, with the result against their ghost */
	n = 0;
	for ( i = 0; i < count; i++ ) {
		int b, g;
		MH_GhostOf( all, order[i], &b, &g, NULL, 0 );
		for ( k = n; k > 0; k-- ) {
			int bk, gk;
			MH_GhostOf( all, sorted[k - 1], &bk, &gk, NULL, 0 );
			if ( b > 0 && ( bk <= 0 || b < bk ) ) sorted[k] = sorted[k - 1];
			else break;
		}
		sorted[k] = order[i];
		n++;
		if ( b > 0 && ( !fastest || b < fastest ) ) { fastest = b; fastClient = order[i]; }
	}
	e->listHead[0] = "POS";
	e->listHead[1] = "DRIVER";
	e->listHead[2] = "BEST";
	e->listHead[3] = "VS GHOST";
	e->listTitle = "RESULTS AGAINST THE GHOST";
	e->rowCount = 0;
	for ( i = 0; i < n && e->rowCount < MH_END_ROWS; i++ ) {
		int b, g;
		k = e->rowCount++;
		MH_GhostOf( all, sorted[i], &b, &g, NULL, 0 );
		mhEnd.rowClient[k] = sorted[i];
		Com_sprintf( mhEnd.rowPos[k], sizeof( mhEnd.rowPos[k] ), "%02d", i + 1 );
		MH_Plain( cgs.clientinfo[sorted[i]].name, mhEnd.rowName[k], sizeof( mhEnd.rowName[k] ) );
		if ( b > 0 ) CG_ResultTime( b, mhEnd.rowTime[k], sizeof( mhEnd.rowTime[k] ) );
		else Q_strncpyz( mhEnd.rowTime[k], "--", sizeof( mhEnd.rowTime[k] ) );
		if ( b > 0 && g > 0 ) {
			MH_EndSigned( b - g, mhEnd.rowBest[k], sizeof( mhEnd.rowBest[k] ) );
			mhEnd.rowBestColor[k] = b < g ? mhFast : mhAccent;
		} else {
			Q_strncpyz( mhEnd.rowBest[k], "--", sizeof( mhEnd.rowBest[k] ) );
			mhEnd.rowBestColor[k] = mhMuted;
		}
		mhEnd.rowMe[k] = sorted[i] == ( cg.snap ? cg.snap->ps.clientNum : -1 );
		e->rowPos[k] = mhEnd.rowPos[k];
		e->rowName[k] = mhEnd.rowName[k];
		e->rowTime[k] = mhEnd.rowTime[k];
		e->rowBest[k] = mhEnd.rowBest[k];
	}

	/* the ladder: six rows around where your best lap ranks */
	e->ladderCount = 0;
	if ( local && best > 0 && cg.ladderGhostEntryCount > 0 ) {
		int idx[MAX_LADDER_GHOST_ENTRIES + 1], total = 0, mineAt = 0, first;
		for ( i = 0; i < cg.ladderGhostEntryCount; i++ ) {
			if ( cg.ladderGhostEntries[i].lapMs <= 0 ) continue;
			for ( k = total; k > 0 && cg.ladderGhostEntries[idx[k - 1]].lapMs > cg.ladderGhostEntries[i].lapMs; k-- ) idx[k] = idx[k - 1];
			idx[k] = i;
			total++;
		}
		for ( i = 0; i < total && cg.ladderGhostEntries[idx[i]].lapMs < best; i++ ) {}
		mineAt = i;
		for ( k = total; k > mineAt; k-- ) idx[k] = idx[k - 1];
		idx[mineAt] = -1;	/* you */
		total++;
		first = mineAt - 2;
		if ( first > total - MH_END_LADDER_SHOWN ) first = total - MH_END_LADDER_SHOWN;
		if ( first < 0 ) first = 0;
		for ( i = first; i < total && e->ladderCount < MH_END_LADDER_SHOWN; i++ ) {
			k = e->ladderCount++;
			Com_sprintf( mhEnd.ladderPos[k], sizeof( mhEnd.ladderPos[k] ), "#%d", i + 1 );
			if ( idx[i] < 0 ) {
				MH_Plain( cgs.clientinfo[me].name, mhEnd.ladderName[k], sizeof( mhEnd.ladderName[k] ) );
				CG_ResultTime( best, mhEnd.ladderLap[k], sizeof( mhEnd.ladderLap[k] ) );
			} else {
				MH_Plain( cg.ladderGhostEntries[idx[i]].name, mhEnd.ladderName[k], sizeof( mhEnd.ladderName[k] ) );
				CG_ResultTime( cg.ladderGhostEntries[idx[i]].lapMs, mhEnd.ladderLap[k], sizeof( mhEnd.ladderLap[k] ) );
			}
			e->ladderPos[k] = mhEnd.ladderPos[k];
			e->ladderName[k] = mhEnd.ladderName[k];
			e->ladderLap[k] = mhEnd.ladderLap[k];
			e->ladderTag[k] = idx[i] >= 0 && idx[i] == cg.ladderGhostSelected && CG_GhostPlaybackMode() == 3 ? "VS" : "";
			e->ladderMe[k] = idx[i] < 0;
		}
		Com_sprintf( tmp, sizeof( tmp ), "%d GHOSTS", cg.ladderGhostEntryCount );
		e->ladderTitle = "GHOST LADDER";
		Q_strncpyz( mhEnd.ladderRight, tmp, sizeof( mhEnd.ladderRight ) );
		e->ladderRight = mhEnd.ladderRight;
	}

	/* the highlights */
	e->awardCount = 0;
	mhEnd.fastest = -1;
	if ( gainAt >= 0 ) {
		k = e->awardCount++;
		e->awardLabel[k] = "BIGGEST GAIN";
		if ( gainAt == cumCount - 1 ) Q_strncpyz( mhEnd.awardName[k], "To the line \xb7 against the ghost", sizeof( mhEnd.awardName[k] ) );
		else Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "S%d \xb7 against the ghost", cg.ghostBestSplitCheckpoint[gainAt] );
		MH_EndSigned2( gainMs, mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ) );
		mhEnd.awardColor[k] = mhFast;
		mhEnd.awardIcon[k] = mh.flag;
	}
	if ( fastClient >= 0 ) {
		int lap = 0;
		const cgRaceResult_t *r = &all[fastClient];
		for ( i = 0; i < r->lapCount && i < CG_RESULT_LAPS; i++ ) if ( r->laps[i] == fastest ) lap = i + 1;
		if ( CG_IsSprintTrack() ) lap = 0;	/* the run */
		k = e->awardCount++;
		e->awardLabel[k] = "FASTEST LAP";
		MH_Plain( cgs.clientinfo[fastClient].name, tmp, sizeof( tmp ) );
		if ( lap ) Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 lap %d", tmp, lap );
		else Q_strncpyz( mhEnd.awardName[k], tmp, sizeof( mhEnd.awardName[k] ) );
		CG_ResultTime( fastest, mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ) );
		mhEnd.awardColor[k] = mhPurple;
		mhEnd.awardIcon[k] = mh.trophy;
		mhEnd.fastest = fastClient;
	}
	if ( local && best > 0 && ( !cg.ghostPrevPersonalMs || best < cg.ghostPrevPersonalMs ) && e->awardCount < MH_END_AWARDS ) {
		k = e->awardCount++;
		e->awardLabel[k] = "PERSONAL BEST";
		MH_Plain( cgs.clientinfo[me].name, tmp, sizeof( tmp ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 new ghost saved", tmp );
		if ( cg.ghostPrevPersonalMs ) MH_EndSigned2( best - cg.ghostPrevPersonalMs, mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ) );
		else Q_strncpyz( mhEnd.awardValue[k], "NEW", sizeof( mhEnd.awardValue[k] ) );
		mhEnd.awardColor[k] = mhSoft;
		mhEnd.awardIcon[k] = mh.chevron;
	}
}

/* ---------------------------------------------------------------------------
   the end screens of the combat modes (gen.py EndDeathmatch, EndDerby,
   EndTeamDM): the race's page with the score; in place of the laps the
   match minute by minute (frags or damage, under it the place, the drivers
   left or the team's lead), the podium of drivers or teams, the highlights
   from the kills written down (cg_combat_log.c), the classification with
   K / D or when each went out.
   ------------------------------------------------------------------------- */

/* "3:42" */
static void MH_Clock( int ms, char *out, int size ) {
	int s = ms > 0 ? ms / 1000 : 0;
	Com_sprintf( out, size, "%d:%02d", s / 60, s % 60 );
}

static const char *mhTeamWord[4] = { "RED", "BLUE", "GREEN", "YELLOW" };
static const char *mhTeamName[4] = { "Red", "Blue", "Green", "Yellow" };

/* ---------------------------------------------------------------------------
   the objective modes (gen.py EndCTF, EndDomination, EndKOTH, EndAutoball):
   the team deathmatch's page with what the teams played for: the figures,
   the chart (captures, zones or the hill a minute; Autoball the goals one
   by one), the highlights and the last column.
   ------------------------------------------------------------------------- */

static qboolean MH_IsCTF( void ) {
	return cgs.gametype == GT_CTF || cgs.gametype == GT_CTF4;
}

/* a team's score at the end of a minute (the last one seen before) */
static int MH_TeamAt( int t, int minute ) {
	const cgCombatLog_t *log = &mhCombat;
	if ( log->sampled <= 0 || minute < 0 ) return 0;
	if ( minute >= log->sampled ) minute = log->sampled - 1;
	return log->team[minute][t];
}

/* a stretch of minutes from..to for team t: the captures in it, the zones
 * held on average, the seconds on the hill a minute */
static int MH_ObjectivePart( int t, int from, int to, qboolean lastPart, int matchMs ) {
	const cgCombatLog_t *log = &mhCombat;
	int m, sum = 0, span, v;
	if ( MH_IsCTF() ) {
		v = ( lastPart ? cg.teamScores[t] : MH_TeamAt( t, to - 1 ) ) - ( from > 0 ? MH_TeamAt( t, from - 1 ) : 0 );
		return v > 0 ? v : 0;
	}
	for ( m = from; m < to && m < CG_COMBAT_MINUTES; m++ ) sum += log->held[m][t];
	span = lastPart ? matchMs - from * 60000 : ( to - from ) * 60000;
	if ( span < 1000 ) span = 1000;
	if ( cgs.gametype == GT_DOMINATION ) return ( sum + span / 2 ) / span;
	v = ( sum * 60 + span / 2 ) / span;
	return v > 60 ? 60 : v;
}

/* "3 caps", "131 pts", "1 goal" */
static void MH_TeamScoreWord( int score, char *out, int size ) {
	if ( MH_IsCTF() ) Com_sprintf( out, size, "%d cap%s", score, score == 1 ? "" : "s" );
	else if ( cgs.gametype == GT_AUTOBALL ) Com_sprintf( out, size, "%d goal%s", score, score == 1 ? "" : "s" );
	else if ( cgs.gametype == GT_DOMINATION || cgs.gametype == GT_KOTH ) Com_sprintf( out, size, "%d pts", score );
	else Com_sprintf( out, size, "%d frags", score );
}

/* the four figures */
static void MH_EndObjectiveStats( const score_t *sc, int me, int team, int matchMs ) {
	mhEndLayout_t *e = &mhEnd.e;
	if ( MH_IsCTF() ) {
		e->statLabel[0] = "YOUR CAPTURES";
		Com_sprintf( mhEnd.stat[0], sizeof( mhEnd.stat[0] ), "%d", sc->captures );
		if ( sc->captures ) mhEnd.statColor[0] = mhFast;
		e->statLabel[1] = "ASSISTS";
		Com_sprintf( mhEnd.stat[1], sizeof( mhEnd.stat[1] ), "%d", sc->assistCount );
		e->statLabel[2] = "DEFENDS";
		Com_sprintf( mhEnd.stat[2], sizeof( mhEnd.stat[2] ), "%d", sc->defendCount );
		e->statLabel[3] = "YOUR FRAGS";
		Com_sprintf( mhEnd.stat[3], sizeof( mhEnd.stat[3] ), "%d", mhCombat.kills[me] );
	} else if ( cgs.gametype == GT_DOMINATION ) {
		int m, sum = 0;
		e->statLabel[0] = "YOUR CAPTURES";
		Com_sprintf( mhEnd.stat[0], sizeof( mhEnd.stat[0] ), "%d", mhDomCaptures );
		if ( mhDomCaptures ) mhEnd.statColor[0] = mhFast;
		e->statLabel[1] = "YOUR FRAGS";
		Com_sprintf( mhEnd.stat[1], sizeof( mhEnd.stat[1] ), "%d", mhCombat.kills[me] );
		e->statLabel[2] = "DEATHS";
		Com_sprintf( mhEnd.stat[2], sizeof( mhEnd.stat[2] ), "%d", mhCombat.deaths[me] );
		/* the zones your team held on average */
		e->statLabel[3] = "ZONES HELD";
		if ( team >= 0 && matchMs > 0 ) {
			int tenths;
			for ( m = 0; m < CG_COMBAT_MINUTES; m++ ) sum += mhCombat.held[m][team];
			tenths = (int)( ( sum * 10.0f + matchMs * 0.5f ) / matchMs );
			Com_sprintf( mhEnd.stat[3], sizeof( mhEnd.stat[3] ), "%d.%d", tenths / 10, tenths % 10 );
			e->statUnit[3] = "AVG";
		} else Q_strncpyz( mhEnd.stat[3], "--", sizeof( mhEnd.stat[3] ) );
	} else if ( cgs.gametype == GT_KOTH ) {
		e->statLabel[0] = "HILL TIME";
		MH_Clock( sc->kothHoldTimeMs, mhEnd.stat[0], sizeof( mhEnd.stat[0] ) );
		e->statLabel[1] = "CONTESTED";
		MH_Clock( sc->kothContestTimeMs, mhEnd.stat[1], sizeof( mhEnd.stat[1] ) );
		e->statLabel[2] = "HILL KILLS";
		Com_sprintf( mhEnd.stat[2], sizeof( mhEnd.stat[2] ), "%d", sc->kothHillKills );
		if ( sc->kothHillKills ) mhEnd.statColor[2] = mhFast;
		e->statLabel[3] = "YOUR FRAGS";
		Com_sprintf( mhEnd.stat[3], sizeof( mhEnd.stat[3] ), "%d", mhCombat.kills[me] );
	} else {
		e->statLabel[0] = "YOUR GOALS";
		Com_sprintf( mhEnd.stat[0], sizeof( mhEnd.stat[0] ), "%d", sc->autoballGoals );
		if ( sc->autoballGoals ) mhEnd.statColor[0] = mhFast;
		e->statLabel[1] = "ASSISTS";
		Com_sprintf( mhEnd.stat[1], sizeof( mhEnd.stat[1] ), "%d", sc->autoballAssists );
		e->statLabel[2] = "SAVES";
		Com_sprintf( mhEnd.stat[2], sizeof( mhEnd.stat[2] ), "%d", sc->autoballSaves );
		e->statLabel[3] = "BEST SHOT";
		if ( sc->autoballBestShot > 0 ) {
			Com_sprintf( mhEnd.stat[3], sizeof( mhEnd.stat[3] ), "%d", sc->autoballBestShot );
			e->statUnit[3] = "KM/H";
		} else Q_strncpyz( mhEnd.stat[3], "--", sizeof( mhEnd.stat[3] ) );
	}
}

/* Autoball, in place of the chart: the goals one by one (the last ones),
 * the score after each red : blue */
static void MH_EndGoals( void ) {
	mhEndLayout_t *e = &mhEnd.e;
	const cgCombatLog_t *log = &mhCombat;
	int kept = log->goals < CG_COMBAT_GOALS ? log->goals : CG_COMBAT_GOALS, first, i, k, score[2];
	e->chartTitle = "GOALS";
	e->chartNote = kept ? "MINUTE \xb7 SCORER \xb7 SHOT \xb7 RED : BLUE" : "NO GOALS";
	/* the score before the first one kept: the final one less those */
	score[0] = cg.teamScores[0];
	score[1] = cg.teamScores[1];
	for ( i = 0; i < kept; i++ ) if ( log->goalTeam[i] < 2 ) score[log->goalTeam[i]]--;
	if ( score[0] < 0 ) score[0] = 0;
	if ( score[1] < 0 ) score[1] = 0;
	first = kept > MH_END_GOALS ? kept - MH_END_GOALS : 0;
	for ( i = 0; i < kept; i++ ) {
		int t = log->goalTeam[i], c = log->goalScorer[i];
		if ( t < 2 ) score[t]++;
		if ( i < first ) continue;
		k = e->goalCount++;
		Com_sprintf( mhEnd.goalMinute[k], sizeof( mhEnd.goalMinute[k] ), "%d'", log->goalAt[i] / 60000 + 1 );
		if ( c >= 0 && c < MAX_CLIENTS ) MH_Plain( cgs.clientinfo[c].name, mhEnd.goalName[k], sizeof( mhEnd.goalName[k] ) );
		else Q_strncpyz( mhEnd.goalName[k], c == -2 ? "Own goal" : mhTeamName[t], sizeof( mhEnd.goalName[k] ) );
		if ( log->goalSpeed[i] > 0 ) Com_sprintf( mhEnd.goalShot[k], sizeof( mhEnd.goalShot[k] ), "%d km/h", log->goalSpeed[i] );
		else Q_strncpyz( mhEnd.goalShot[k], "--", sizeof( mhEnd.goalShot[k] ) );
		Com_sprintf( mhEnd.goalScore[k], sizeof( mhEnd.goalScore[k] ), "%d:%d", score[0], score[1] );
		mhEnd.goalTeam[k] = t;
		e->goalMinute[k] = mhEnd.goalMinute[k];
		e->goalName[k] = mhEnd.goalName[k];
		e->goalShot[k] = mhEnd.goalShot[k];
		e->goalScore[k] = mhEnd.goalScore[k];
	}
}

/* a highlight more, -1: full */
static int MH_EndAwardAdd( const char *label, qhandle_t icon, const float *color ) {
	mhEndLayout_t *e = &mhEnd.e;
	int k;
	if ( e->awardCount >= MH_END_AWARDS ) return -1;
	k = e->awardCount++;
	e->awardLabel[k] = label;
	mhEnd.awardIcon[k] = icon;
	mhEnd.awardColor[k] = color;
	mhEnd.awardName[k][0] = mhEnd.awardValue[k][0] = '\0';
	return k;
}

/* the client with the most of a figure (at least min), -1: nobody */
#define MH_MOST( field, min, who, best ) do { int i_; ( who ) = -1; ( best ) = ( min ) - 1; \
	for ( i_ = 0; i_ < count; i_++ ) { const score_t *s_ = &cg.scores[order[i_]]; \
		if ( s_->field > ( best ) ) { ( best ) = s_->field; ( who ) = s_->client; } } } while ( 0 )

static void MH_EndObjectiveAwards( const int *order, int count, int me ) {
	int k, who, best, times, zone, team;
	char name[40], tmp[24];
#define NAME( c ) ( MH_Plain( cgs.clientinfo[c].name, name, sizeof( name ) ), name )
	if ( MH_IsCTF() ) {
		MH_MOST( captures, 1, who, best );
		if ( who >= 0 && ( k = MH_EndAwardAdd( "MOST CAPTURES", mh.flag, mhFast ) ) >= 0 ) {
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 %d capture%s", NAME( who ), best, best == 1 ? "" : "s" );
			Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", best );
		}
		MH_MOST( defendCount, 1, who, best );
		if ( who >= 0 && ( k = MH_EndAwardAdd( "BEST DEFENDER", mh.trophy, mhPurple ) ) >= 0 ) {
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 %d defend%s", NAME( who ), best, best == 1 ? "" : "s" );
			Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", best );
		}
	} else if ( cgs.gametype == GT_DOMINATION ) {
		int ms = CG_CombatLongest( &mhCombat, mhCombatEnd, &zone, &team );
		if ( ms >= 1000 && team >= 0 && ( k = MH_EndAwardAdd( "LONGEST HOLD", mh.hill, mhPurple ) ) >= 0 ) {
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 zone %c", mhTeamName[team], 'A' + zone );
			MH_Clock( ms, mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ) );
		}
		for ( k = 0, who = -1, best = 0; k < count; k++ ) {
			int c = cg.scores[order[k]].client;
			if ( mhCombat.kills[c] > best ) { best = mhCombat.kills[c]; who = c; }
		}
		if ( who >= 0 && ( k = MH_EndAwardAdd( "MOST FRAGS", mh.gun, mhFast ) ) >= 0 ) {
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 %d frag%s", NAME( who ), best, best == 1 ? "" : "s" );
			Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", best );
		}
	} else if ( cgs.gametype == GT_KOTH ) {
		int ms;
		MH_MOST( kothHoldTimeMs, 1000, who, best );
		if ( who >= 0 && ( k = MH_EndAwardAdd( "KING OF THE HILL", mh.hill, mhPurple ) ) >= 0 ) {
			MH_Clock( best, tmp, sizeof( tmp ) );
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 %s on the hill", NAME( who ), tmp );
			Q_strncpyz( mhEnd.awardValue[k], tmp, sizeof( mhEnd.awardValue[k] ) );
		}
		MH_MOST( kothHillKills, 1, who, best );
		if ( who >= 0 && ( k = MH_EndAwardAdd( "MOST HILL KILLS", mh.gun, mhFast ) ) >= 0 ) {
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 %d on the hill", NAME( who ), best );
			Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", best );
		}
		ms = CG_CombatLongest( &mhCombat, mhCombatEnd, &zone, &team );
		if ( ms >= 1000 && team >= 0 && ( k = MH_EndAwardAdd( "LONGEST REIGN", mh.chevron, mhAccent ) ) >= 0 ) {
			MH_Clock( ms, tmp, sizeof( tmp ) );
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 held it %s", mhTeamName[team], tmp );
			Q_strncpyz( mhEnd.awardValue[k], tmp, sizeof( mhEnd.awardValue[k] ) );
		}
		return;
	} else {
		MH_MOST( score, -99999, who, best );
		if ( who >= 0 && ( k = MH_EndAwardAdd( "MAN OF THE MATCH", mh.trophy, mhPurple ) ) >= 0 ) {
			const score_t *s = NULL;
			int i;
			for ( i = 0; i < count; i++ ) if ( cg.scores[order[i]].client == who ) s = &cg.scores[order[i]];
			Q_strncpyz( mhEnd.awardName[k], NAME( who ), sizeof( mhEnd.awardName[k] ) );
			if ( s && ( s->autoballGoals || s->autoballAssists || s->autoballSaves ) ) {
				const char *sep = " \xb7 ";
				if ( s->autoballGoals ) {
					Q_strcat( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), va( "%s%d goal%s", sep, s->autoballGoals, s->autoballGoals == 1 ? "" : "s" ) );
					sep = ", ";
				}
				if ( s->autoballAssists ) {
					Q_strcat( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), va( "%s%d assist%s", sep, s->autoballAssists, s->autoballAssists == 1 ? "" : "s" ) );
					sep = ", ";
				}
				if ( s->autoballSaves )
					Q_strcat( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), va( "%s%d save%s", sep, s->autoballSaves, s->autoballSaves == 1 ? "" : "s" ) );
			}
			if ( s ) Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", s->autoballGoals );
		}
		MH_MOST( autoballBestShot, 1, who, best );
		if ( who >= 0 && ( k = MH_EndAwardAdd( "HARDEST SHOT", mh.chevron, mhFast ) ) >= 0 ) {
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 %d km/h", NAME( who ), best );
			Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", best );
		}
		MH_MOST( autoballSaves, 1, who, best );
		if ( who >= 0 && ( k = MH_EndAwardAdd( "MOST SAVES", mh.flag, mhAccent ) ) >= 0 ) {
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 %d save%s", NAME( who ), best, best == 1 ? "" : "s" );
			Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", best );
		}
		return;
	}
	/* CTF and Domination: your nemesis last */
	who = CG_CombatNemesis( &mhCombat, me, &times );
	if ( who >= 0 && times > 0 && ( k = MH_EndAwardAdd( "YOUR NEMESIS", mh.chevron, mhAccent ) ) >= 0 ) {
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 got you %d time%s", NAME( who ), times, times == 1 ? "" : "s" );
		Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", times );
	}
#undef NAME
}
#undef MH_MOST

static void MH_EndCombat( void ) {
	mhEndLayout_t *e = &mhEnd.e;
	const cgCombatLog_t *log = &mhCombat;
	qboolean objective = MH_EndIsObjective(), teams = cgs.gametype == GT_TEAM || objective;
	qboolean survival = cgs.gametype == GT_DERBY || cgs.gametype == GT_LCS;
	int order[MAX_CLIENTS], value[MH_END_SPRINT_PARTS], count = 0, i, k, me, mineAt = -1, matchMs, parts;
	int partEnd[MH_END_SPRINT_PARTS + 1], teamOrder[4], teamCount = 0, myTeam = -1, top = 0, prevPlace = 0, best, who, times;
	int stillIn = 0;	/* derby, LCS: more than one at the end: the time ran out */
	const char *map;
	char name[40], tmp[24], mapName[40];

	MH_CombatCheck();
	if ( !mhCombatEnd ) mhCombatEnd = cg.time;
	matchMs = mhCombatEnd - mhCombat.start;
	memset( mhEnd.lapPlaceColor, 0, sizeof( mhEnd.lapPlaceColor ) );
	mhEnd.combat = qtrue;
	mhEnd.posColor = mhWhite;
	mhEnd.doneNoteColor = mhAccent;
	mhEnd.fastest = -1;

	/* the drivers in the server's order, spectators out */
	for ( i = 0; i < cg.numScores; i++ ) {
		int c = cg.scores[i].client;
		if ( c < 0 || c >= MAX_CLIENTS || !cgs.clientinfo[c].infoValid || cgs.clientinfo[c].team == TEAM_SPECTATOR ) continue;
		order[count++] = i;
	}
	if ( count <= 0 ) return;
	me = cg.snap->ps.clientNum;
	for ( i = 0; i < count; i++ ) if ( cg.scores[order[i]].client == me ) mineAt = i;
	if ( mineAt < 0 ) { mineAt = 0; me = cg.scores[order[0]].client; }	/* a spectator: the winner's match */
#define SC( n ) ( &cg.scores[order[n]] )
	for ( i = 0; i < count; i++ ) if ( !mhCombat.outAt[SC( i )->client] ) stillIn++;

	/* the teams by their score */
	if ( teams ) {
		for ( k = 0; k < 4; k++ ) {
			qboolean there = qfalse;
			for ( i = 0; i < count; i++ ) if ( cgs.clientinfo[SC( i )->client].team == TEAM_RED + k ) there = qtrue;
			if ( !there ) continue;
			for ( i = teamCount; i > 0 && cg.teamScores[teamOrder[i - 1]] < cg.teamScores[k]; i-- ) teamOrder[i] = teamOrder[i - 1];
			teamOrder[i] = k;
			teamCount++;
		}
		for ( i = 0; i < teamCount; i++ ) if ( cgs.clientinfo[me].team == TEAM_RED + teamOrder[i] ) myTeam = i;
	}

	/* the header */
	map = CG_ConfigString( CS_MESSAGE );
	if ( !map || !map[0] ) map = COM_SkipPath( cgs.mapname );
	MH_Plain( map, mapName, sizeof( mapName ) );
	COM_StripExtension( mapName, mapName, sizeof( mapName ) );
	Q_strupr( mapName );
	e->title = cgs.gametype == GT_TEAM ? "TEAM DEATHMATCH RESULT" : cgs.gametype == GT_DERBY ? "DERBY RESULT" :
	           cgs.gametype == GT_LCS ? "LAST CAR STANDING RESULT" : MH_IsCTF() ? "CAPTURE THE FLAG RESULT" :
	           cgs.gametype == GT_DOMINATION ? "DOMINATION RESULT" : cgs.gametype == GT_KOTH ? "KING OF THE HILL RESULT" :
	           cgs.gametype == GT_AUTOBALL ? "AUTOBALL RESULT" : "DEATHMATCH RESULT";
	Q_strncpyz( mhEnd.sub, mapName, sizeof( mhEnd.sub ) );
	if ( objective ) {
		if ( MH_IsCTF() && cgs.capturelimit > 0 ) Q_strcat( mhEnd.sub, sizeof( mhEnd.sub ), va( " \xb7 CAPTURELIMIT %d", cgs.capturelimit ) );
		else if ( cgs.timelimit > 0 ) Q_strcat( mhEnd.sub, sizeof( mhEnd.sub ), va( " \xb7 TIMELIMIT %d", cgs.timelimit ) );
	} else if ( !survival && cgs.fraglimit > 0 ) Q_strcat( mhEnd.sub, sizeof( mhEnd.sub ), va( " \xb7 FRAGLIMIT %d", cgs.fraglimit ) );
	if ( teams ) Q_strcat( mhEnd.sub, sizeof( mhEnd.sub ), va( " \xb7 %d TEAMS", teamCount ) );
	Q_strcat( mhEnd.sub, sizeof( mhEnd.sub ), va( " \xb7 %d DRIVER%s", count, count == 1 ? "" : "S" ) );
	e->subtitle = mhEnd.sub;

	/* the left: the place and the score */
	if ( teams && myTeam >= 0 ) {
		int mine = cg.teamScores[teamOrder[myTeam]];
		int lead = teamCount > 1 ? mine - cg.teamScores[teamOrder[myTeam == 0 ? 1 : 0]] : 0;
		mhEnd.teams = qtrue;
		mhEnd.posColor = MH_TeamColor( TEAM_RED + teamOrder[myTeam] );
		Com_sprintf( mhEnd.pos, sizeof( mhEnd.pos ), "P%d", myTeam + 1 );
		Com_sprintf( mhEnd.of, sizeof( mhEnd.of ), "/ %d", teamCount );
		Com_sprintf( mhEnd.doneLabel, sizeof( mhEnd.doneLabel ), "TEAM %s \xb7 %s", mhTeamWord[teamOrder[myTeam]],
		             MH_IsCTF() ? "CAPTURES" : cgs.gametype == GT_AUTOBALL ? "GOALS" : "SCORE" );
		Com_sprintf( mhEnd.doneTime, sizeof( mhEnd.doneTime ), "%d", mine );
		if ( teamCount < 2 ) mhEnd.doneNote[0] = '\0';
		else if ( cgs.gametype == GT_AUTOBALL )	/* a match: the score, yours first */
			Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "%s %d:%d", lead > 0 ? "WON" : lead < 0 ? "LOST" : "DRAW", mine, mine - lead );
		else if ( lead == 0 ) Q_strncpyz( mhEnd.doneNote, "DRAW", sizeof( mhEnd.doneNote ) );
		else if ( myTeam == 0 ) Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "WON BY %d", lead );
		else Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "%d BEHIND %s", -lead, mhTeamWord[teamOrder[0]] );
	} else {
		Com_sprintf( mhEnd.pos, sizeof( mhEnd.pos ), "P%d", mineAt + 1 );
		Com_sprintf( mhEnd.of, sizeof( mhEnd.of ), "/ %d", count );
		if ( survival ) {
			int out = mhCombat.outAt[me];
			Q_strncpyz( mhEnd.doneLabel, "SURVIVED", sizeof( mhEnd.doneLabel ) );
			MH_Clock( out ? out : matchMs, mhEnd.doneTime, sizeof( mhEnd.doneTime ) );
			if ( mineAt == 0 && stillIn <= 1 ) Q_strncpyz( mhEnd.doneNote, "LAST CAR STANDING", sizeof( mhEnd.doneNote ) );
			else if ( !out ) Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "TIME UP \xb7 %d STILL IN", stillIn );
			else if ( out ) {
				int before = ( matchMs - out ) / 1000;
				if ( before < 0 ) before = 0;
				MH_Plain( cgs.clientinfo[SC( 0 )->client].name, name, sizeof( name ) );
				Q_strupr( name );
				if ( before < 60 ) Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "OUT %d S BEFORE %s", before, name );
				else Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "OUT %d:%02d BEFORE %s", before / 60, before % 60, name );
			} else Q_strncpyz( mhEnd.doneNote, "STILL IN AT THE END", sizeof( mhEnd.doneNote ) );
		} else {
			Q_strncpyz( mhEnd.doneLabel, "SCORE", sizeof( mhEnd.doneLabel ) );
			Com_sprintf( mhEnd.doneTime, sizeof( mhEnd.doneTime ), "%d", SC( mineAt )->score );
			if ( mineAt == 0 ) Q_strncpyz( mhEnd.doneNote, "WINNER", sizeof( mhEnd.doneNote ) );
			else Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "%d BEHIND THE WINNER", SC( 0 )->score - SC( mineAt )->score );
		}
	}
	e->pos = mhEnd.pos;
	e->of = mhEnd.of;
	e->doneLabel = mhEnd.doneLabel;
	e->doneTime = mhEnd.doneTime;
	e->doneNote = mhEnd.doneNote;

	/* four figures */
	e->statCount = 4;
	for ( i = 0; i < 4; i++ ) { e->statUnit[i] = NULL; mhEnd.statColor[i] = mhWhite; e->statValue[i] = mhEnd.stat[i]; }
	if ( survival ) {
		e->statLabel[0] = "SCORE";
		MH_Thousands( SC( mineAt )->score, mhEnd.stat[0], sizeof( mhEnd.stat[0] ) );
		e->statLabel[1] = "WRECKED";
		Com_sprintf( mhEnd.stat[1], sizeof( mhEnd.stat[1] ), "%d", mhCombat.kills[me] );
		if ( mhCombat.kills[me] ) mhEnd.statColor[1] = mhFast;
		e->statLabel[2] = "DAMAGE DEALT";
		MH_Thousands( SC( mineAt )->damageDealt, mhEnd.stat[2], sizeof( mhEnd.stat[2] ) );
		e->statLabel[3] = "DAMAGE TAKEN";
		MH_Thousands( SC( mineAt )->damageTaken, mhEnd.stat[3], sizeof( mhEnd.stat[3] ) );
	} else if ( objective ) {
		MH_EndObjectiveStats( SC( mineAt ), me, myTeam >= 0 ? teamOrder[myTeam] : -1, matchMs );
	} else if ( teams ) {
		e->statLabel[0] = "YOUR PLACE";
		Com_sprintf( mhEnd.stat[0], sizeof( mhEnd.stat[0] ), "P%d", mineAt + 1 );
		e->statLabel[1] = "YOUR FRAGS";
		Com_sprintf( mhEnd.stat[1], sizeof( mhEnd.stat[1] ), "%d", mhCombat.kills[me] );
		e->statLabel[2] = "DEATHS";
		Com_sprintf( mhEnd.stat[2], sizeof( mhEnd.stat[2] ), "%d", mhCombat.deaths[me] );
		e->statLabel[3] = "BEST STREAK";
		Com_sprintf( mhEnd.stat[3], sizeof( mhEnd.stat[3] ), "%d", mhCombat.bestStreak[me] );
		if ( mhCombat.bestStreak[me] ) mhEnd.statColor[3] = mhFast;
	} else {
		e->statLabel[0] = "KILLS";
		Com_sprintf( mhEnd.stat[0], sizeof( mhEnd.stat[0] ), "%d", mhCombat.kills[me] );
		e->statLabel[1] = "DEATHS";
		Com_sprintf( mhEnd.stat[1], sizeof( mhEnd.stat[1] ), "%d", mhCombat.deaths[me] );
		e->statLabel[2] = "BEST STREAK";
		Com_sprintf( mhEnd.stat[2], sizeof( mhEnd.stat[2] ), "%d", mhCombat.bestStreak[me] );
		if ( mhCombat.bestStreak[me] ) mhEnd.statColor[2] = mhFast;
		e->statLabel[3] = "DAMAGE DEALT";
		MH_Thousands( SC( mineAt )->damageDealt, mhEnd.stat[3], sizeof( mhEnd.stat[3] ) );
	}

	/* the match minute by minute, past eight in eight stretches */
	parts = log->minutes < MH_END_SPRINT_PARTS ? log->minutes : MH_END_SPRINT_PARTS;
	e->chartTitle = survival ? "YOUR DAMAGE" : "YOUR FRAGS";
	e->chartNote = survival ? "A BAR A MINUTE \xb7 DRIVERS LEFT" : teams ? "A BAR A MINUTE \xb7 YOUR TEAM'S LEAD" : "A BAR A MINUTE \xb7 YOUR PLACE";
	if ( objective && myTeam >= 0 ) {
		if ( MH_IsCTF() ) e->chartTitle = "TEAM CAPTURES";
		else if ( cgs.gametype == GT_DOMINATION ) {
			e->chartTitle = "ZONES HELD";
			e->chartNote = "A BAR A MINUTE \xb7 YOUR TEAM'S PLACE";
		} else if ( cgs.gametype == GT_KOTH ) {
			e->chartTitle = "HILL HELD";
			e->chartNote = "SECONDS A MINUTE \xb7 YOUR TEAM'S LEAD";
		} else {
			MH_EndGoals();
			parts = 0;
		}
	}
	e->lapCount = 0;
	partEnd[0] = 0;
	for ( k = 1; k <= parts; k++ ) partEnd[k] = ( k * log->minutes + parts / 2 ) / parts;
	for ( k = 0; k < parts; k++ ) {
		int m, last = partEnd[k + 1] - 1;
		value[k] = 0;
		if ( survival ) value[k] = log->damage[last][me] - ( partEnd[k] > 0 ? log->damage[partEnd[k] - 1][me] : 0 );
		else if ( objective && myTeam >= 0 )
			value[k] = MH_ObjectivePart( teamOrder[myTeam], partEnd[k], partEnd[k + 1], k == parts - 1, matchMs );
		else for ( m = partEnd[k]; m <= last; m++ ) value[k] += log->minuteKills[m][me];
		if ( value[k] < 0 ) value[k] = 0;
		if ( value[k] > top ) top = value[k];
	}
	for ( k = 0; k < parts; k++ ) {
		int last = partEnd[k + 1] - 1;
		e->lapCount++;
		e->lapFrac[k] = top > 0 ? (float)value[k] / top : 0.0f;
		mhEnd.lapLit[k] = top > 0 && value[k] == top;
		MH_Thousands( value[k], mhEnd.lap[k], sizeof( mhEnd.lap[k] ) );
		Com_sprintf( mhEnd.lapName[k], sizeof( mhEnd.lapName[k] ), "M%d", last + 1 );
		if ( survival ) {
			int left = CG_CombatLeftAt( log, last );
			Com_sprintf( mhEnd.lapPlace[k], sizeof( mhEnd.lapPlace[k] ), "%d", left );
			mhEnd.lapPlaceColor[k] = k == 0 || left != prevPlace ? mhWhite : mhDim;
			prevPlace = left;
		} else if ( teams && myTeam >= 0 ) {
			/* the teams' scores then: the last ones seen in the stretch, the final ones at the end */
#define TEAM_THEN( t ) ( k == parts - 1 ? cg.teamScores[t] : MH_TeamAt( t, last ) )
			int mine = TEAM_THEN( teamOrder[myTeam] ), rival = mine, t, first = 1, above = 0;
			for ( t = 0; t < teamCount; t++ ) {
				if ( t == myTeam ) continue;
				if ( TEAM_THEN( teamOrder[t] ) > mine ) above++;
				if ( first || TEAM_THEN( teamOrder[t] ) > rival ) rival = TEAM_THEN( teamOrder[t] );
				first = 0;
			}
#undef TEAM_THEN
			if ( cgs.gametype == GT_DOMINATION ) {	/* four teams: the place */
				Com_sprintf( mhEnd.lapPlace[k], sizeof( mhEnd.lapPlace[k] ), "P%d", above + 1 );
				mhEnd.lapPlaceColor[k] = k == 0 || above + 1 != prevPlace ? mhWhite : mhDim;
				prevPlace = above + 1;
			} else {
				Com_sprintf( mhEnd.lapPlace[k], sizeof( mhEnd.lapPlace[k] ), mine > rival ? "+%d" : "%d", mine - rival );
				mhEnd.lapPlaceColor[k] = mine > rival ? mhFast : mine < rival ? mhAccent : mhWhite;
			}
		} else {
			int at = CG_CombatPlaceAt( log, me, last );
			if ( at ) Com_sprintf( mhEnd.lapPlace[k], sizeof( mhEnd.lapPlace[k] ), "P%d", at );
			else Q_strncpyz( mhEnd.lapPlace[k], "--", sizeof( mhEnd.lapPlace[k] ) );
			mhEnd.lapPlaceColor[k] = k == 0 || at != prevPlace ? mhWhite : mhDim;
			prevPlace = at;
		}
		e->lapValue[k] = mhEnd.lap[k];
		e->lapName[k] = mhEnd.lapName[k];
		e->lapPlace[k] = mhEnd.lapPlace[k];
	}

	/* the podium: drivers, or teams with their drivers under them */
	e->podiumCount = 0;
	if ( teams ) {
		e->teamPodium = qtrue;
		e->podiumYou = "YOUR TEAM";
		for ( k = 0; k < teamCount && k < 3; k++ ) {
			static const char *tags[3] = { "WINNER", "SECOND", "THIRD" };
			int t = teamOrder[k], n = 0;
			mhEnd.podiumTeam[k] = t;
			mhEnd.client[k] = -1;
			Q_strncpyz( mhEnd.podName[k], mhTeamName[t], sizeof( mhEnd.podName[k] ) );
			MH_TeamScoreWord( cg.teamScores[t], mhEnd.podTime[k], sizeof( mhEnd.podTime[k] ) );
			if ( k > 0 ) Com_sprintf( mhEnd.podGap[k], sizeof( mhEnd.podGap[k] ), "-%d", cg.teamScores[teamOrder[0]] - cg.teamScores[t] );
			else mhEnd.podGap[k][0] = '\0';
			for ( i = 0; i < count; i++ ) {
				int c = SC( i )->client;
				if ( cgs.clientinfo[c].team != TEAM_RED + t ) continue;
				if ( mhEnd.client[k] < 0 ) mhEnd.client[k] = c;	/* its best: its car */
				if ( n < MH_END_MEMBERS ) {
					Com_sprintf( mhEnd.memberPos[k][n], sizeof( mhEnd.memberPos[k][n] ), "P%d", i + 1 );
					MH_Plain( cgs.clientinfo[c].name, mhEnd.memberName[k][n], sizeof( mhEnd.memberName[k][n] ) );
					mhEnd.memberMe[k][n] = c == cg.snap->ps.clientNum;
					e->podiumMemberPos[k][n] = mhEnd.memberPos[k][n];
					e->podiumMemberName[k][n] = mhEnd.memberName[k][n];
					n++;
				}
			}
			e->podiumMembers[k] = n;
			e->podiumName[k] = mhEnd.podName[k];
			e->podiumTime[k] = mhEnd.podTime[k];
			e->podiumGap[k] = mhEnd.podGap[k];
			e->podiumTag[k] = tags[k];
			e->podiumMe[k] = k == myTeam;
			e->podiumCount = k + 1;
		}
	} else {
		for ( k = 0; k < count && k < 3; k++ ) {
			static const char *tags[3] = { "WINNER", "SECOND", "THIRD" };
			const score_t *sc = SC( k );
			mhEnd.client[k] = sc->client;
			MH_Plain( cgs.clientinfo[sc->client].name, mhEnd.podName[k], sizeof( mhEnd.podName[k] ) );
			mhEnd.podGap[k][0] = '\0';
			if ( survival ) {
				MH_Thousands( sc->score, tmp, sizeof( tmp ) );
				Com_sprintf( mhEnd.podTime[k], sizeof( mhEnd.podTime[k] ), "%s pts", tmp );
				if ( k > 0 && mhCombat.outAt[sc->client] ) {
					MH_Clock( mhCombat.outAt[sc->client], tmp, sizeof( tmp ) );
					Com_sprintf( mhEnd.podGap[k], sizeof( mhEnd.podGap[k] ), "out %s", tmp );
				}
			} else {
				Com_sprintf( mhEnd.podTime[k], sizeof( mhEnd.podTime[k] ), "%d frags", sc->score );
				if ( k > 0 ) Com_sprintf( mhEnd.podGap[k], sizeof( mhEnd.podGap[k] ), "-%d", SC( 0 )->score - sc->score );
			}
			e->podiumName[k] = mhEnd.podName[k];
			e->podiumTime[k] = mhEnd.podTime[k];
			e->podiumGap[k] = mhEnd.podGap[k];
			e->podiumTag[k] = tags[k];
			e->podiumMe[k] = sc->client == cg.snap->ps.clientNum;
			e->podiumCount = k + 1;
		}
	}

	/* the highlights */
	e->awardCount = 0;
	if ( objective ) {
		MH_EndObjectiveAwards( order, count, me );
		goto awarded;
	}
	if ( survival && stillIn <= 1 ) {
		k = e->awardCount++;
		e->awardLabel[k] = "LAST CAR STANDING";
		MH_Plain( cgs.clientinfo[SC( 0 )->client].name, name, sizeof( name ) );
		MH_Clock( matchMs, tmp, sizeof( tmp ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 %s", name, tmp );
		MH_Thousands( SC( 0 )->score, mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ) );
		mhEnd.awardColor[k] = mhAccent;
		mhEnd.awardIcon[k] = mh.trophy;
	} else if ( !survival ) {
		for ( i = 0, best = 0, who = -1; i < count; i++ ) {
			int c = SC( i )->client;
			if ( mhCombat.bestStreak[c] > best ) { best = mhCombat.bestStreak[c]; who = c; }
		}
		if ( who >= 0 && best >= 2 ) {
			k = e->awardCount++;
			e->awardLabel[k] = "BEST STREAK";
			MH_Plain( cgs.clientinfo[who].name, name, sizeof( name ) );
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 %d in a row", name, best );
			Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", best );
			mhEnd.awardColor[k] = mhPurple;
			mhEnd.awardIcon[k] = mh.trophy;
		}
	}
	for ( i = 0, best = 0, who = -1; i < count; i++ ) {
		if ( SC( i )->damageDealt > best ) { best = SC( i )->damageDealt; who = SC( i )->client; }
	}
	if ( who >= 0 ) {
		k = e->awardCount++;
		e->awardLabel[k] = "MOST DAMAGE";
		MH_Plain( cgs.clientinfo[who].name, name, sizeof( name ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 dealt", name );
		MH_Thousands( best, mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ) );
		mhEnd.awardColor[k] = mhFast;
		mhEnd.awardIcon[k] = mh.gun;
	}
	if ( survival ) {
		if ( mhCombat.outAt[me] && mhCombat.outBy[me] >= 0 && mineAt > 0 && e->awardCount < MH_END_AWARDS ) {
			k = e->awardCount++;
			e->awardLabel[k] = "TOOK YOU OUT";
			MH_Plain( cgs.clientinfo[mhCombat.outBy[me]].name, name, sizeof( name ) );
			MH_Clock( mhCombat.outAt[me], tmp, sizeof( tmp ) );
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 %s", name, tmp );
			mhEnd.awardValue[k][0] = '\0';
			mhEnd.awardColor[k] = mhAccent;
			mhEnd.awardIcon[k] = mh.chevron;
		}
	} else {
		who = CG_CombatNemesis( log, me, &times );
		if ( who >= 0 && times > 0 && e->awardCount < MH_END_AWARDS ) {
			k = e->awardCount++;
			e->awardLabel[k] = "YOUR NEMESIS";
			MH_Plain( cgs.clientinfo[who].name, name, sizeof( name ) );
			Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 got you %d time%s", name, times, times == 1 ? "" : "s" );
			Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", times );
			mhEnd.awardColor[k] = mhAccent;
			mhEnd.awardIcon[k] = mh.chevron;
		}
	}
awarded:
	for ( i = 0; i < e->awardCount; i++ ) {
		e->awardName[i] = mhEnd.awardName[i];
		e->awardValue[i] = mhEnd.awardValue[i];
	}

	/* the classification */
	e->listTitle = "FINAL CLASSIFICATION";
	e->listHead[0] = "POS";
	e->listHead[1] = "DRIVER";
	e->listHead[2] = "SCORE";
	e->listHead[3] = survival ? "OUT" : MH_IsCTF() ? "CAPS" : cgs.gametype == GT_KOTH ? "HILL" :
	                 cgs.gametype == GT_AUTOBALL ? "G / A" : "K / D";
	if ( survival ) {
		MH_Clock( matchMs, tmp, sizeof( tmp ) );
		Com_sprintf( mhEnd.listRight, sizeof( mhEnd.listRight ), "MATCH %s", tmp );
	} else if ( MH_IsCTF() && cgs.capturelimit > 0 ) Com_sprintf( mhEnd.listRight, sizeof( mhEnd.listRight ), "CAPTURELIMIT %d", cgs.capturelimit );
	else if ( cgs.gametype == GT_AUTOBALL && cgs.timelimit > 0 ) Com_sprintf( mhEnd.listRight, sizeof( mhEnd.listRight ), "TIMELIMIT %d", cgs.timelimit );
	else if ( teams ) Q_strncpyz( mhEnd.listRight, "TEAM SCORE", sizeof( mhEnd.listRight ) );
	else if ( cgs.fraglimit > 0 ) Com_sprintf( mhEnd.listRight, sizeof( mhEnd.listRight ), "FRAGLIMIT %d", cgs.fraglimit );
	else mhEnd.listRight[0] = '\0';
	e->listRight = mhEnd.listRight;
	e->compact = count > 8;
	e->rowCount = 0;
	for ( i = 0; i < count && e->rowCount < MH_END_ROWS; i++ ) {
		const score_t *sc = SC( i );
		int c = sc->client;
		k = e->rowCount++;
		mhEnd.rowClient[k] = c;
		Com_sprintf( mhEnd.rowPos[k], sizeof( mhEnd.rowPos[k] ), "%02d", i + 1 );
		MH_Plain( cgs.clientinfo[c].name, mhEnd.rowName[k], sizeof( mhEnd.rowName[k] ) );
		MH_Thousands( sc->score, mhEnd.rowTime[k], sizeof( mhEnd.rowTime[k] ) );
		mhEnd.rowBestColor[k] = mhMuted;
		if ( survival ) {
			if ( !mhCombat.outAt[c] ) {
				Q_strncpyz( mhEnd.rowBest[k], stillIn <= 1 ? "LAST" : "IN", sizeof( mhEnd.rowBest[k] ) );
				if ( stillIn <= 1 ) mhEnd.rowBestColor[k] = mhAccent;
			} else MH_Clock( mhCombat.outAt[c], mhEnd.rowBest[k], sizeof( mhEnd.rowBest[k] ) );
		} else if ( MH_IsCTF() ) Com_sprintf( mhEnd.rowBest[k], sizeof( mhEnd.rowBest[k] ), "%d", sc->captures );
		else if ( cgs.gametype == GT_KOTH ) MH_Clock( sc->kothHoldTimeMs, mhEnd.rowBest[k], sizeof( mhEnd.rowBest[k] ) );
		else if ( cgs.gametype == GT_AUTOBALL ) Com_sprintf( mhEnd.rowBest[k], sizeof( mhEnd.rowBest[k] ), "%d / %d", sc->autoballGoals, sc->autoballAssists );
		else Com_sprintf( mhEnd.rowBest[k], sizeof( mhEnd.rowBest[k] ), "%d / %d", mhCombat.kills[c], mhCombat.deaths[c] );
		mhEnd.rowMe[k] = c == cg.snap->ps.clientNum;
		mhEnd.rowTeam[k] = cgs.clientinfo[c].team;
		e->rowPos[k] = mhEnd.rowPos[k];
		e->rowName[k] = mhEnd.rowName[k];
		e->rowTime[k] = mhEnd.rowTime[k];
		e->rowBest[k] = mhEnd.rowBest[k];
	}
	if ( teams ) {
		e->rowSwatch = qtrue;
		e->teamCellCount = teamCount < MH_END_TEAMS ? teamCount : MH_END_TEAMS;
		for ( i = 0; i < e->teamCellCount; i++ ) {
			mhEnd.team[i] = teamOrder[i];
			Q_strncpyz( mhEnd.teamName[i], mhTeamWord[teamOrder[i]], sizeof( mhEnd.teamName[i] ) );
			Com_sprintf( mhEnd.teamTime[i], sizeof( mhEnd.teamTime[i] ), "%d", cg.teamScores[teamOrder[i]] );
			e->teamCellName[i] = mhEnd.teamName[i];
			e->teamCellValue[i] = mhEnd.teamTime[i];
		}
	}
#undef SC
}

/* ---------------------------------------------------------------------------
   the race variants (gen.py EndRacingDM, EndElimination): Racing DM and Team
   Racing DM on the race's and the team race's page with frags and deaths
   (the kills written down as in the combat modes); Elimination ordered by
   who lasted longest, from the server's elimination events.
   ------------------------------------------------------------------------- */

/* Elimination: when each went out (the event's round and time), the winner
 * the one with the highest round */
static int mhElimRound[MAX_CLIENTS], mhElimAt[MAX_CLIENTS], mhElimWinner = -1;

void CG_ModernHUD_ElimReset( void ) {
	memset( mhElimRound, 0, sizeof( mhElimRound ) );
	memset( mhElimAt, 0, sizeof( mhElimAt ) );
	mhElimWinner = -1;
}

/* the server's elim_status with the winner */
void CG_ModernHUD_ElimWinner( int client ) {
	if ( client >= 0 && client < MAX_CLIENTS ) mhElimWinner = client;
}

/* the order: the winner, then the later out first; without an event still
 * in (above those out) unless there is a winner already (then at the end) */
static int MH_ElimRank( int c ) {
	if ( c == mhElimWinner ) return 100000;
	if ( mhElimRound[c] ) return mhElimRound[c];
	return mhElimWinner >= 0 ? -1 : 99999;
}

void CG_ModernHUD_ElimEvent( int client, int round, int time ) {
	if ( client < 0 || client >= MAX_CLIENTS || mhElimRound[client] ) return;
	mhElimRound[client] = round > 0 ? round : 1;
	mhElimAt[client] = time;
}

/* Racing DM: frags and deaths in place of the last two figures, K / D in
 * place of the best lap, the most frags and your nemesis as highlights */
static void MH_EndRacingDM( int me ) {
	mhEndLayout_t *e = &mhEnd.e;
	int i, k, best = 0, who = -1, times;
	char name[40];

	e->statLabel[2] = "FRAGS";
	Com_sprintf( mhEnd.stat[2], sizeof( mhEnd.stat[2] ), "%d", mhCombat.kills[me] );
	mhEnd.statColor[2] = mhCombat.kills[me] ? mhFast : mhWhite;
	e->statUnit[2] = NULL;
	e->statLabel[3] = "DEATHS";
	Com_sprintf( mhEnd.stat[3], sizeof( mhEnd.stat[3] ), "%d", mhCombat.deaths[me] );
	mhEnd.statColor[3] = mhWhite;
	e->statUnit[3] = NULL;

	e->listHead[0] = "POS";
	e->listHead[1] = "DRIVER";
	e->listHead[2] = "TIME";
	e->listHead[3] = "K / D";
	for ( k = 0; k < e->rowCount; k++ ) {
		int c = mhEnd.rowClient[k];
		Com_sprintf( mhEnd.rowBest[k], sizeof( mhEnd.rowBest[k] ), "%d / %d", mhCombat.kills[c], mhCombat.deaths[c] );
		mhEnd.rowBestColor[k] = mhMuted;
	}

	/* the fastest lap stays first; then the most frags, your nemesis */
	if ( e->awardCount > 1 ) e->awardCount = 1;
	if ( e->awardCount == 1 && Q_stricmp( e->awardLabel[0], "FASTEST LAP" ) ) e->awardCount = 0;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( cgs.clientinfo[i].infoValid && mhCombat.kills[i] > best ) { best = mhCombat.kills[i]; who = i; }
	}
	if ( who >= 0 ) {
		k = e->awardCount++;
		e->awardLabel[k] = "MOST FRAGS";
		MH_Plain( cgs.clientinfo[who].name, name, sizeof( name ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 %d frag%s", name, best, best == 1 ? "" : "s" );
		Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", best );
		mhEnd.awardColor[k] = mhFast;
		mhEnd.awardIcon[k] = mh.gun;
	}
	who = CG_CombatNemesis( &mhCombat, me, &times );
	if ( who >= 0 && times > 0 && e->awardCount < MH_END_AWARDS ) {
		k = e->awardCount++;
		e->awardLabel[k] = "YOUR NEMESIS";
		MH_Plain( cgs.clientinfo[who].name, name, sizeof( name ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 got you %d time%s", name, times, times == 1 ? "" : "s" );
		Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "%d", times );
		mhEnd.awardColor[k] = mhAccent;
		mhEnd.awardIcon[k] = mh.chevron;
	}
}

/* the laps a driver made: the winner all, one out one less than the lap he
 * went out in, one still in (the race ended on the lap limit) his own */
static int MH_ElimLaps( const cgRaceResult_t *all, int c, int winner, int rounds ) {
	if ( c == winner ) return rounds;
	if ( mhElimRound[c] ) return mhElimRound[c] - 1;
	return all[c].lapCount < rounds ? all[c].lapCount : rounds;	/* nobody past the winner */
}

/* Elimination: the order is who lasted longest; under the laps how many
 * were left after each */
static void MH_EndElimination( const cgRaceResult_t *all, const int *order, int count, int me, const cgRaceResult_t *mine ) {
	mhEndLayout_t *e = &mhEnd.e;
	int sorted[MAX_CLIENTS], i, k, n = 0, place = 0, winner = -1, top = 0, first = -1, gain = 0, gainClient = -1, laps, firstLap, rounds;
	char name[40], *dot;

	for ( i = 0; i < count; i++ ) {
		int c = order[i], a = MH_ElimRank( c );
		for ( k = n; k > 0; k-- ) {
			int o = sorted[k - 1], b = MH_ElimRank( o );
			if ( a > b || ( a == b && mhElimAt[c] > mhElimAt[o] ) ) sorted[k] = sorted[k - 1];
			else break;
		}
		sorted[k] = c;
		n++;
	}
	for ( i = 0; i < n; i++ ) {
		if ( mhElimRound[sorted[i]] > top ) top = mhElimRound[sorted[i]];
		if ( sorted[i] == me ) place = i + 1;
	}
	if ( n < 1 ) return;
	winner = sorted[0];	/* the winner, or the first of those still in */
	if ( !place ) place = 1;
	/* the laps raced: the winner's round is one past the last one out */
	rounds = mhElimRound[winner] ? mhElimRound[winner] - 1 : top;
	/* the race ended on the lap limit (no winner's event), or was won before
	 * anyone went out: the winner's laps */
	if ( !mhElimRound[winner] || rounds < 1 ) rounds = MAX( rounds, all[winner].lapCount );
	for ( i = 0; i < n; i++ ) {
		int r = mhElimRound[sorted[i]];
		if ( r && sorted[i] != winner && ( first < 0 || r < mhElimRound[first] ) ) first = sorted[i];
	}

	e->title = "ELIMINATION RESULT";
	dot = strstr( mhEnd.sub, " \xb7 " );
	if ( dot ) Com_sprintf( dot, sizeof( mhEnd.sub ) - ( dot - mhEnd.sub ),  " \xb7 THE LAST ONE OUT EVERY LAP \xb7 %d DRIVER%s", n, n == 1 ? "" : "S" );
	Com_sprintf( mhEnd.pos, sizeof( mhEnd.pos ), "P%d", place );
	Com_sprintf( mhEnd.of, sizeof( mhEnd.of ), "/ %d", n );
	Q_strncpyz( mhEnd.doneLabel, "SURVIVED", sizeof( mhEnd.doneLabel ) );
	e->doneLabel = mhEnd.doneLabel;
	{
		int end = mhElimAt[me] ? mhElimAt[me] : mine->finishTime ? mine->finishTime : cg.time;
		int sec = end > mine->startTime ? ( end - mine->startTime ) / 1000 : 0;
		Com_sprintf( mhEnd.doneTime, sizeof( mhEnd.doneTime ), "%d:%02d", sec / 60, sec % 60 );	/* minutes and seconds */
	}
	if ( me == winner ) Q_strncpyz( mhEnd.doneNote, "WINNER", sizeof( mhEnd.doneNote ) );
	else {
		MH_Plain( cgs.clientinfo[winner].name, name, sizeof( name ) );
		Q_strupr( name );
		if ( mhElimRound[me] && mhElimRound[me] >= rounds )
			Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "OUT IN THE LAST LAP \xb7 %s WON", name );
		else if ( mhElimRound[me] ) Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "OUT IN LAP %d \xb7 %s WON", mhElimRound[me], name );
		else Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "%s WON", name );
	}

	/* the figures: best lap and top speed stay; laps survived, places gained */
	laps = MH_ElimLaps( all, me, winner, rounds );
	e->statLabel[2] = "LAPS SURVIVED";
	Com_sprintf( mhEnd.stat[2], sizeof( mhEnd.stat[2] ), "%d / %d", laps, rounds );
	mhEnd.statColor[2] = mhWhite;
	e->statUnit[2] = NULL;
	e->statLabel[3] = "PLACES GAINED";
	e->statUnit[3] = NULL;
	mhEnd.statColor[3] = mhWhite;
	if ( mine->grid > 0 ) {
		int g = mine->grid - place;
		Com_sprintf( mhEnd.stat[3], sizeof( mhEnd.stat[3] ), g > 0 ? "+%d" : "%d", g );
		mhEnd.statColor[3] = g > 0 ? mhFast : g < 0 ? mhAccent : mhWhite;
	} else Q_strncpyz( mhEnd.stat[3], "--", sizeof( mhEnd.stat[3] ) );

	/* under the laps: how many were left after each */
	e->chartNote = "LOST TO YOUR BEST \xb7 DRIVERS LEFT";
	firstLap = mine->lapCount < CG_RESULT_LAPS ? mine->lapCount : CG_RESULT_LAPS;
	firstLap = firstLap > MH_END_LAPS ? firstLap - MH_END_LAPS : 0;
	for ( k = 0; k < e->lapCount; k++ ) {
		int lap = firstLap + k + 1, left = 0;
		for ( i = 0; i < n; i++ ) if ( !mhElimRound[sorted[i]] || mhElimRound[sorted[i]] > lap ) left++;
		Com_sprintf( mhEnd.lapPlace[k], sizeof( mhEnd.lapPlace[k] ), "%d", left );
		mhEnd.lapPlaceLit[k] = k == 0 || Q_stricmp( mhEnd.lapPlace[k], mhEnd.lapPlace[k - 1] );
		e->lapPlace[k] = mhEnd.lapPlace[k];
	}

	/* the podium: the laps each made, when they went out */
	e->podiumCount = 0;
	for ( k = 0; k < n && k < 3; k++ ) {
		static const char *tags[3] = { "WINNER", "SECOND", "THIRD" };
		int c = sorted[k], made = MH_ElimLaps( all, c, winner, rounds );
		mhEnd.client[k] = c;
		MH_Plain( cgs.clientinfo[c].name, mhEnd.podName[k], sizeof( mhEnd.podName[k] ) );
		Com_sprintf( mhEnd.podTime[k], sizeof( mhEnd.podTime[k] ), "%d lap%s", made, made == 1 ? "" : "s" );
		if ( c != winner && mhElimRound[c] ) Com_sprintf( mhEnd.podGap[k], sizeof( mhEnd.podGap[k] ), "out lap %d", mhElimRound[c] );
		else mhEnd.podGap[k][0] = '\0';
		e->podiumName[k] = mhEnd.podName[k];
		e->podiumTime[k] = mhEnd.podTime[k];
		e->podiumGap[k] = mhEnd.podGap[k];
		e->podiumTag[k] = tags[k];
		e->podiumMe[k] = c == cg.snap->ps.clientNum;
		e->podiumCount = k + 1;
	}

	/* the highlights: the fastest lap stays; who went out first; the biggest gain */
	if ( e->awardCount > 1 ) e->awardCount = 1;
	if ( e->awardCount == 1 && Q_stricmp( e->awardLabel[0], "FASTEST LAP" ) ) e->awardCount = 0;
	if ( first >= 0 ) {
		k = e->awardCount++;
		e->awardLabel[k] = "FIRST OUT";
		MH_Plain( cgs.clientinfo[first].name, name, sizeof( name ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 lap %d", name, mhElimRound[first] );
		mhEnd.awardValue[k][0] = '\0';
		mhEnd.awardColor[k] = mhAccent;
		mhEnd.awardIcon[k] = mh.flag;
	}
	for ( i = 0; i < n; i++ ) {
		int g = all[sorted[i]].grid > 0 ? all[sorted[i]].grid - ( i + 1 ) : 0;
		if ( g > gain ) { gain = g; gainClient = sorted[i]; }
	}
	if ( gainClient >= 0 && e->awardCount < MH_END_AWARDS ) {
		int to = 0;
		for ( i = 0; i < n; i++ ) if ( sorted[i] == gainClient ) to = i + 1;
		k = e->awardCount++;
		e->awardLabel[k] = "BIGGEST GAIN";
		MH_Plain( cgs.clientinfo[gainClient].name, name, sizeof( name ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 P%d to P%d", name, all[gainClient].grid, to );
		Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "+%d", gain );
		mhEnd.awardColor[k] = mhFast;
		mhEnd.awardIcon[k] = mh.chevron;
	}

	/* the classification: who lasted longest, when each went out, the best lap */
	e->listHead[0] = "POS";
	e->listHead[1] = "DRIVER";
	e->listHead[2] = "OUT";
	e->listHead[3] = "BEST";
	e->rowCount = 0;
	for ( i = 0; i < n && e->rowCount < MH_END_ROWS; i++ ) {
		int c = sorted[i], b = CG_ResultBestLap( &all[c] );
		k = e->rowCount++;
		mhEnd.rowClient[k] = c;
		Com_sprintf( mhEnd.rowPos[k], sizeof( mhEnd.rowPos[k] ), "%02d", i + 1 );
		MH_Plain( cgs.clientinfo[c].name, mhEnd.rowName[k], sizeof( mhEnd.rowName[k] ) );
		if ( c == winner ) Q_strncpyz( mhEnd.rowTime[k], "WINNER", sizeof( mhEnd.rowTime[k] ) );
		else if ( mhElimRound[c] ) Com_sprintf( mhEnd.rowTime[k], sizeof( mhEnd.rowTime[k] ), "LAP %d", mhElimRound[c] );
		else Q_strncpyz( mhEnd.rowTime[k], mhElimWinner >= 0 ? "--" : "IN", sizeof( mhEnd.rowTime[k] ) );
		if ( b ) CG_ResultTime( b, mhEnd.rowBest[k], sizeof( mhEnd.rowBest[k] ) );
		else Q_strncpyz( mhEnd.rowBest[k], "--", sizeof( mhEnd.rowBest[k] ) );
		mhEnd.rowMe[k] = c == cg.snap->ps.clientNum;
		e->rowPos[k] = mhEnd.rowPos[k];
		e->rowName[k] = mhEnd.rowName[k];
		e->rowTime[k] = mhEnd.rowTime[k];
		e->rowBest[k] = mhEnd.rowBest[k];
	}
}

static void MH_EndPrepare( void ) {
	mhEndLayout_t *e = &mhEnd.e;
	const cgRaceResult_t *all = CG_RaceResult( 0 ), *mine, *winner;
	int order[MAX_CLIENTS], count, n = 0, i, k, me, place = 0, best, bestSector = 0, sectorClient = -1, gainClient = -1, gain = 0;
	int fastest = 0, first;
	const char *map;
	char name[40], mapName[40];
	float viewWidth;

	MH_Prepare();
	memset( e, 0, sizeof( *e ) );
	viewWidth = cgs.glconfig.vidWidth / mhScale;
	mhEnd.scale = viewWidth < 1600.0f ? viewWidth / 1600.0f : 1.0f;
	e->viewWidth = viewWidth / mhEnd.scale;
	e->top = ( MH_DESIGN_HEIGHT / mhEnd.scale - MH_DESIGN_HEIGHT ) * 0.5f;
	mhEnd.combat = qfalse;
	mhEnd.teams = qfalse;
	if ( MH_EndIsCombat() ) {
		MH_EndCombat();
		MH_EndReady( e, "READY", "DETAILS" );
		MH_EndLayout( e );
		return;
	}

	/* the order, drivers still in the game only (in a team race: in a team) */
	count = CG_ResultOrder( all, MAX_CLIENTS, order );
	for ( i = 0; i < count; i++ ) {
		int team = MH_Team( order[i] );
		if ( !cgs.clientinfo[order[i]].infoValid ) continue;
		if ( ( cgs.gametype == GT_TEAM_RACING || cgs.gametype == GT_TEAM_RACING_DM ) && ( team < TEAM_RED || team >= TEAM_RED + MH_END_TEAMS ) ) continue;
		order[n++] = order[i];
	}
	count = n;
	winner = count > 0 ? &all[order[0]] : NULL;
	me = cg.snap->ps.clientNum;
	mine = CG_RaceResult( me );
	for ( i = 0; i < count; i++ ) if ( order[i] == me ) place = i + 1;
	if ( !place && count > 0 ) {
		me = order[0];	/* a spectator, or out of it: the winner's race */
		mine = &all[me];
		place = 1;
	}

	/* the header */
	map = CG_ConfigString( CS_MESSAGE );
	if ( !map || !map[0] ) map = COM_SkipPath( cgs.mapname );
	MH_Plain( map, name, sizeof( name ) );
	COM_StripExtension( name, name, sizeof( name ) );
	Q_strupr( name );
	Q_strncpyz( mapName, name, sizeof( mapName ) );
	e->title = "RACE RESULT";
	if ( cgs.laplimit > 0 )
		Com_sprintf( mhEnd.sub, sizeof( mhEnd.sub ), "%s \xb7 %d LAP%s \xb7 %d DRIVERS", name, cgs.laplimit, cgs.laplimit == 1 ? "" : "S", count );
	else
		Com_sprintf( mhEnd.sub, sizeof( mhEnd.sub ), "%s \xb7 %d DRIVERS", name, count );
	e->subtitle = mhEnd.sub;

	/* the left: the place, the time */
	Com_sprintf( mhEnd.pos, sizeof( mhEnd.pos ), "P%d", place );
	Com_sprintf( mhEnd.of, sizeof( mhEnd.of ), "/ %d", count );
	e->pos = mhEnd.pos;
	e->of = mhEnd.of;
	e->doneLabel = mine->finishTime ? "FINISHED" : "DID NOT FINISH";
	if ( mine->finishTime ) CG_ResultTime( CG_ResultTotal( mine ), mhEnd.doneTime, sizeof( mhEnd.doneTime ) );
	else Q_strncpyz( mhEnd.doneTime, "--", sizeof( mhEnd.doneTime ) );
	e->doneTime = mhEnd.doneTime;
	if ( place == 1 ) Q_strncpyz( mhEnd.doneNote, "WINNER", sizeof( mhEnd.doneNote ) );
	else if ( mine->finishTime && winner ) {
		char gap[16];
		CG_ResultGap( winner, mine, gap, sizeof( gap ) );
		Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "%s TO THE WINNER", gap );
	} else Com_sprintf( mhEnd.doneNote, sizeof( mhEnd.doneNote ), "%d LAP%s DONE", mine->lapCount, mine->lapCount == 1 ? "" : "S" );
	e->doneNote = mhEnd.doneNote;

	/* four figures */
	best = CG_ResultBestLap( mine );
	e->statCount = 4;
	e->statLabel[0] = "BEST LAP";
	if ( best ) CG_ResultTime( best, mhEnd.stat[0], sizeof( mhEnd.stat[0] ) );
	else Q_strncpyz( mhEnd.stat[0], "--", sizeof( mhEnd.stat[0] ) );
	e->statLabel[1] = "TOP SPEED";
	if ( me == cg.snap->ps.clientNum && cg.raceTopSpeed > 0.0f ) {
		Com_sprintf( mhEnd.stat[1], sizeof( mhEnd.stat[1] ), "%d", (int)( cg.raceTopSpeed + 0.5f ) );
		e->statUnit[1] = cg_metricUnits.integer ? "KM/H" : "MPH";
	} else Q_strncpyz( mhEnd.stat[1], "--", sizeof( mhEnd.stat[1] ) );
	e->statLabel[2] = "PLACES GAINED";
	mhEnd.statColor[2] = mhWhite;
	if ( mine->grid > 0 && place > 0 ) {
		int gained = mine->grid - place;
		Com_sprintf( mhEnd.stat[2], sizeof( mhEnd.stat[2] ), gained > 0 ? "+%d" : gained < 0 ? "%d" : "0", gained );
		mhEnd.statColor[2] = gained > 0 ? mhFast : gained < 0 ? mhAccent : mhWhite;
	} else Q_strncpyz( mhEnd.stat[2], "--", sizeof( mhEnd.stat[2] ) );
	e->statLabel[3] = "AVG LAP";
	{
		int sum = 0, laps = mine->lapCount < CG_RESULT_LAPS ? mine->lapCount : CG_RESULT_LAPS;
		for ( i = 0; i < laps; i++ ) sum += mine->laps[i];
		if ( laps > 0 ) CG_ResultTime( sum / laps, mhEnd.stat[3], sizeof( mhEnd.stat[3] ) );
		else Q_strncpyz( mhEnd.stat[3], "--", sizeof( mhEnd.stat[3] ) );
	}
	for ( i = 0; i < 4; i++ ) {
		e->statValue[i] = mhEnd.stat[i];
		if ( i != 2 ) mhEnd.statColor[i] = mhWhite;
	}

	/* your laps, the last twelve: what each lost to the best */
	{
		int laps = mine->lapCount < CG_RESULT_LAPS ? mine->lapCount : CG_RESULT_LAPS, worst = 0;
		first = laps > MH_END_LAPS ? laps - MH_END_LAPS : 0;
		for ( i = first; i < laps; i++ ) if ( mine->laps[i] - best > worst ) worst = mine->laps[i] - best;
		for ( i = first; i < laps; i++ ) {
			int lost = mine->laps[i] - best;
			k = e->lapCount++;
			e->lapFrac[k] = worst > 0 ? (float)lost / worst : 0.0f;
			if ( lost <= 0 ) Q_strncpyz( mhEnd.lap[k], "best", sizeof( mhEnd.lap[k] ) );
			else MH_EndLost( lost, mhEnd.lap[k], sizeof( mhEnd.lap[k] ) );
			Com_sprintf( mhEnd.lapName[k], sizeof( mhEnd.lapName[k] ), "L%d", i + 1 );
			e->lapValue[k] = mhEnd.lap[k];
			e->lapName[k] = mhEnd.lapName[k];
		}
	}

	/* the podium: the first three home */
	for ( i = 0; i < 3 && i < count && all[order[i]].finishTime; i++ ) {
		static const char *tags[3] = { "WINNER", "SECOND", "THIRD" };
		const cgRaceResult_t *r = &all[order[i]];
		mhEnd.client[i] = order[i];
		MH_Plain( cgs.clientinfo[order[i]].name, mhEnd.podName[i], sizeof( mhEnd.podName[i] ) );
		CG_ResultTime( CG_ResultTotal( r ), mhEnd.podTime[i], sizeof( mhEnd.podTime[i] ) );
		mhEnd.podGap[i][0] = '\0';
		if ( i > 0 ) CG_ResultGap( winner, r, mhEnd.podGap[i], sizeof( mhEnd.podGap[i] ) );
		e->podiumName[i] = mhEnd.podName[i];
		e->podiumTime[i] = mhEnd.podTime[i];
		e->podiumGap[i] = mhEnd.podGap[i];
		e->podiumTag[i] = tags[i];
		e->podiumMe[i] = order[i] == cg.snap->ps.clientNum;
		e->podiumCount = i + 1;
	}

	/* the highlights */
	mhEnd.fastest = -1;
	for ( i = 0; i < count; i++ ) {
		const cgRaceResult_t *r = &all[order[i]];
		int b = CG_ResultBestLap( r );
		if ( b && ( !fastest || b < fastest ) ) { fastest = b; mhEnd.fastest = order[i]; }
		if ( r->bestSector && ( !bestSector || r->bestSector < bestSector ) ) { bestSector = r->bestSector; sectorClient = order[i]; }
		if ( r->finishTime && r->grid > 0 && r->grid - ( i + 1 ) > gain ) { gain = r->grid - ( i + 1 ); gainClient = order[i]; }
	}
	if ( mhEnd.fastest >= 0 ) {
		k = e->awardCount++;
		e->awardLabel[k] = "FASTEST LAP";
		MH_Plain( cgs.clientinfo[mhEnd.fastest].name, mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ) );
		CG_ResultTime( fastest, mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ) );
		mhEnd.awardColor[k] = mhPurple;
		mhEnd.awardIcon[k] = mh.trophy;
	}
	if ( sectorClient >= 0 ) {
		k = e->awardCount++;
		e->awardLabel[k] = "BEST SECTOR";
		MH_Plain( cgs.clientinfo[sectorClient].name, name, sizeof( name ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 S%d", name, all[sectorClient].bestSectorNum );
		CG_ResultTime( bestSector, mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ) );
		mhEnd.awardColor[k] = mhFast;
		mhEnd.awardIcon[k] = mh.flag;
	}
	if ( gainClient >= 0 ) {
		int to = 0;
		for ( i = 0; i < count; i++ ) if ( order[i] == gainClient ) to = i + 1;
		k = e->awardCount++;
		e->awardLabel[k] = "BIGGEST GAIN";
		MH_Plain( cgs.clientinfo[gainClient].name, name, sizeof( name ) );
		Com_sprintf( mhEnd.awardName[k], sizeof( mhEnd.awardName[k] ), "%s \xb7 P%d to P%d", name, all[gainClient].grid, to );
		Com_sprintf( mhEnd.awardValue[k], sizeof( mhEnd.awardValue[k] ), "+%d", gain );
		mhEnd.awardColor[k] = mhAccent;
		mhEnd.awardIcon[k] = mh.chevron;
	}

	/* the classification: past 16 the first 15 and yours */
	e->listTitle = "FINAL CLASSIFICATION";
	if ( cgs.laplimit > 0 ) Com_sprintf( mhEnd.listRight, sizeof( mhEnd.listRight ), "%d LAP%s", cgs.laplimit, cgs.laplimit == 1 ? "" : "S" );
	else mhEnd.listRight[0] = '\0';
	e->listRight = mhEnd.listRight;
	e->compact = count > 8;
	for ( i = 0; i < count && e->rowCount < MH_END_ROWS; i++ ) {
		int at = i;
		const cgRaceResult_t *r;
		if ( count > MH_END_ROWS && e->rowCount == MH_END_ROWS - 1 && place > MH_END_ROWS - 1 ) at = place - 1;
		r = &all[order[at]];
		k = e->rowCount++;
		mhEnd.rowClient[k] = order[at];
		Com_sprintf( mhEnd.rowPos[k], sizeof( mhEnd.rowPos[k] ), "%02d", at + 1 );
		MH_Plain( cgs.clientinfo[order[at]].name, mhEnd.rowName[k], sizeof( mhEnd.rowName[k] ) );
		CG_ResultGap( winner, r, mhEnd.rowTime[k], sizeof( mhEnd.rowTime[k] ) );
		if ( CG_ResultBestLap( r ) ) CG_ResultTime( CG_ResultBestLap( r ), mhEnd.rowBest[k], sizeof( mhEnd.rowBest[k] ) );
		else Q_strncpyz( mhEnd.rowBest[k], "--", sizeof( mhEnd.rowBest[k] ) );
		mhEnd.rowMe[k] = order[at] == cg.snap->ps.clientNum;
		e->rowPos[k] = mhEnd.rowPos[k];
		e->rowName[k] = mhEnd.rowName[k];
		e->rowTime[k] = mhEnd.rowTime[k];
		e->rowBest[k] = mhEnd.rowBest[k];
	}

	mhEnd.teams = qfalse;
	mhEnd.drag = qfalse;
	mhEnd.racingDM = qfalse;
	mhEnd.sprint = MH_EndIsSprint() && cgs.gametype != GT_GHOST;
	mhEnd.ghost = cgs.gametype == GT_GHOST;
	mhEnd.posColor = mhWhite;
	mhEnd.doneNoteColor = mhAccent;
	if ( cgs.gametype == GT_TEAM_RACING || cgs.gametype == GT_TEAM_RACING_DM ) MH_EndTeams( all, order, count, me, mine, place );
	if ( cgs.gametype == GT_RACING_DM || cgs.gametype == GT_TEAM_RACING_DM ) {
		MH_CombatCheck();
		MH_EndRacingDM( me );
		mhEnd.racingDM = qtrue;	/* K / D in its own colour */
		e->title = cgs.gametype == GT_RACING_DM ? "RACING DM RESULT" : "TEAM RACING DM RESULT";
	}
	if ( cgs.gametype == GT_ELIMINATION ) MH_EndElimination( all, order, count, me, mine );
	if ( cgs.gametype == GT_DRAG ) MH_EndDrag();
	if ( mhEnd.sprint ) MH_EndSprint( all, order, count, me, mine, place, mapName );
	if ( mhEnd.ghost ) MH_EndGhost( all, order, count, me, mine, mapName );
	for ( i = 0; i < e->awardCount; i++ ) {
		e->awardName[i] = mhEnd.awardName[i];
		e->awardValue[i] = mhEnd.awardValue[i];
	}

	MH_EndReady( e, "READY", "DETAILS" );
	if ( mhEnd.tourney ) MH_EndTourney();
	MH_EndLayout( e );
}

void CG_ModernHUD_DrawEnd( void ) {
	mhEndLayout_t *e = &mhEnd.e;
	float base;
	int i, k;

	MH_EndPrepare();
	WS_FillRect( 0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, mhVeil );
	base = mhScale;
	mhScale = base * mhEnd.scale;

	MH_Str( &e->titleT, mhAccent, 1.0f );
	MH_Str( &e->subtitleT, mhMuted, 1.0f );
	MH_Str( &e->posT, mhEnd.posColor, 1.0f );
	MH_Str( &e->ofT, mhDim, 1.0f );
	MH_Str( &e->doneLabelT, mhEnd.teams ? mhEnd.posColor : mhLabel, 1.0f );
	MH_Str( &e->doneTimeT, mhWhite, 1.0f );
	MH_Str( &e->doneNoteT, mhEnd.doneNoteColor, 1.0f );
	MH_FillRect( &e->sep[0], mhLine, 1.0f );
	MH_FillRect( &e->sep[1], mhLine, 1.0f );
	for ( i = 0; i < e->statCount; i++ ) {
		MH_Str( &e->statLabelT[i], mhMuted, 1.0f );
		MH_Str( &e->statValueT[i], mhEnd.statColor[i], 1.0f );
		MH_Str( &e->statUnitT[i], mhDim, 1.0f );
	}
	MH_Str( &e->lapsTitleT, mhLabel, 1.0f );
	MH_Str( &e->lapsNoteT, mhDim, 1.0f );
	if ( e->pathCount ) MH_Str( &e->pathTitleT, mhLabel, 1.0f );
	for ( i = 0; i < e->pathCount; i++ ) {
		MH_Str( &e->pathRoundT[i], mhMuted, 1.0f );
		MH_Str( &e->pathWhatT[i], mhTourney.pathColor[i], 1.0f );
		MH_Str( &e->pathTagT[i], mhTourney.pathTagColor[i], 1.0f );
	}
	for ( i = 0; i < e->lapCount; i++ ) {
		qboolean isBest = !Q_stricmp( e->lapValue[i], "best" );
		qboolean foul = !Q_stricmp( e->lapValue[i], "foul" ) || !Q_stricmp( e->lapValue[i], "dnf" );
		/* a sprint: the quickest of all in a sector in blue */
		const float *bestBar = mhEnd.sprint ? mhFast : mhLapBest, *bestText = mhEnd.sprint ? mhFast : mhWhite;
		if ( e->lapDiverge ) {	/* a ghost race: gained up in blue, lost down in orange */
			const float *c = e->lapFrac[i] <= 0.0f ? mhFast : mhAccent;
			MH_FillRect( &e->lapAxis[i], mhDim, 1.0f );
			MH_FillRect( &e->lapBar[i], c, 1.0f );
			MH_Str( &e->lapValueT[i], c, 1.0f );
		} else if ( mhEnd.combat ) {	/* the best minute lit */
			MH_FillRect( &e->lapBar[i], mhEnd.lapLit[i] ? mhLapBest : mhLapBar, 1.0f );
			MH_Str( &e->lapValueT[i], mhEnd.lapLit[i] ? mhWhite : mhLabel, 1.0f );
		} else {
			MH_FillRect( &e->lapBar[i], foul ? mhAccent : isBest ? bestBar : mhLapBar, 1.0f );
			MH_Str( &e->lapValueT[i], foul ? mhAccent : isBest ? bestText : mhLabel, 1.0f );
		}
		MH_Str( &e->lapNameT[i], mhDim, 1.0f );
		if ( e->lapPlace[i] ) {
			MH_FillRect( &e->lapRule[i], mhLine, 1.0f );
			MH_Str( &e->lapPlaceT[i], ( mhEnd.ghost || mhEnd.combat ) && mhEnd.lapPlaceColor[i] ? mhEnd.lapPlaceColor[i] :
			        mhEnd.lapPlaceLit[i] ? mhWhite : mhDim, 1.0f );
		}
	}
	for ( i = 0; i < e->goalCount; i++ ) {	/* Autoball: the goals */
		mhText_t shot, score;
		MH_Str( &e->goalMinuteT[i], mhMuted, 1.0f );
		MH_FillRect( &e->goalSwatch[i], MH_TeamColor( TEAM_RED + mhEnd.goalTeam[i] ), 1.0f );
		MH_Str( &e->goalNameT[i], mhWhite, 1.0f );
		shot = e->goalShotT[i];	/* at the right of their columns */
		score = e->goalScoreT[i];
		shot.box.x += shot.box.w - WS_TextWidth( &shot.style, shot.text, strlen( shot.text ) );
		score.box.x += score.box.w - WS_TextWidth( &score.style, score.text, strlen( score.text ) );
		MH_Str( &shot, mhMuted, 1.0f );
		MH_Str( &score, mhWhite, 1.0f );
	}
	for ( i = 0; i < e->spreadCount; i++ ) {
		const float *c = MH_TeamColor( TEAM_RED + mhEnd.team[i] );
		MH_FillRect( &e->spreadSwatch[i], c, 1.0f );
		MH_FillRect( &e->spreadTrack[i], mhTrack, 1.0f );
		for ( k = 0; k < e->spreadDots[i]; k++ ) MH_Pic( &e->spreadDotR[i][k], 0.0f, mh.dot, c, 1.0f );
		if ( e->spreadTick[i] > 0.0f ) MH_FillRect( &e->spreadTickR[i], mhWhite, 1.0f );
		{
			mhText_t v = e->spreadValueT[i];
			v.box.x += v.box.w - WS_TextWidth( &v.style, v.text, strlen( v.text ) );
			MH_Str( &v, i == 0 ? mhWhite : mhLabel, 1.0f );
		}
	}

	/* the podium: the panels, then the cars, then the words */
	for ( k = 0; k < e->podiumCount; k++ ) {
		const float *top = k == 0 ? mhAccent : e->podiumMe[k] ? mhSoft : mhLine;
		if ( mhEnd.teams ) top = MH_TeamColor( TEAM_RED + mhEnd.podiumTeam[k] );
		MH_FillRect( &e->podium[k], mhPanel, 1.0f );
		MH_Border( &e->podium[k], mhLine, mhEnd.teams ? 3.0f : 2.0f, top, 1.0f );
		MH_FillRect( &e->podiumCar[k], mhShowroom, 1.0f );
	}
	for ( k = 0; k < e->podiumCount; k++ ) {
		const wsRect_t *c = &e->podiumCar[k];
		float yaw = AngleMod( 150.0f + k * 40.0f + cg.time * 0.018f );
		if ( mhEnd.tourney && mhEnd.podCarValid[k] ) {
			CG_DrawShowroomModel( MH_Snap( c->x ), MH_Snap( c->y ), MH_Snap( c->x + c->w ) - MH_Snap( c->x ),
			                      MH_Snap( c->y + c->h ) - MH_Snap( c->y ), &mhEnd.podCar[k], yaw );
			continue;
		}
		if ( mhEnd.client[k] < 0 ) continue;
		CG_DrawShowroomCar( MH_Snap( c->x ), MH_Snap( c->y ), MH_Snap( c->x + c->w ) - MH_Snap( c->x ),
		                    MH_Snap( c->y + c->h ) - MH_Snap( c->y ), mhEnd.client[k], yaw );
	}
	for ( k = 0; k < e->podiumCount; k++ ) {
		MH_Str( &e->podiumPlaceT[k], mhEnd.teams ? MH_TeamColor( TEAM_RED + mhEnd.podiumTeam[k] ) : k == 0 ? mhAccent : mhWhite, 1.0f );
		MH_Str( &e->podiumYouT[k], mhAccent, 1.0f );
		MH_Str( &e->podiumTagT[k], mhMuted, 1.0f );
		MH_Str( &e->podiumNameT[k], mhWhite, 1.0f );
		MH_Str( &e->podiumTimeT[k], mhValue, 1.0f );
		MH_Str( &e->podiumGapT[k], mhAccent, 1.0f );
		if ( e->podiumMembers[k] ) MH_FillRect( &e->podiumRule[k], mhLine, 1.0f );
		for ( i = 0; i < e->podiumMembers[k]; i++ ) {
			MH_Str( &e->podiumMemberPosT[k][i], mhMuted, 1.0f );
			MH_Str( &e->podiumMemberNameT[k][i], mhEnd.memberMe[k][i] ? mhAccent : mhWhite, 1.0f );
		}
	}

	/* a ghost race: you and the ghost; the ghost's car under a blue veil */
	if ( e->duel ) {
		static const vec4_t ghostDim = { 0.0f, 0.0f, 0.0f, 0.45f }, ghostTint = { 90 / 255.0f, 176 / 255.0f, 1.0f, 0.16f };
		for ( k = 0; k < 2; k++ ) {
			const wsRect_t *c = &e->duelCar[k];
			MH_FillRect( &e->duelCard[k], mhPanel, 1.0f );
			MH_Border( &e->duelCard[k], mhLine, 2.0f, k == 0 ? mhAccent : mhFast, 1.0f );
			MH_FillRect( c, mhShowroom, 1.0f );
			if ( mhEnd.duelCarValid[k] ) {
				CG_DrawShowroomModel( MH_Snap( c->x ), MH_Snap( c->y ), MH_Snap( c->x + c->w ) - MH_Snap( c->x ),
				                      MH_Snap( c->y + c->h ) - MH_Snap( c->y ), &mhEnd.duelCar[k],
				                      AngleMod( 150.0f + k * 40.0f + cg.time * 0.018f ) );
			}
			if ( k == 1 ) {
				MH_FillRect( c, ghostDim, 1.0f );
				MH_FillRect( c, ghostTint, 1.0f );
			}
			MH_Str( &e->duelTagT[k], k == 0 ? mhAccent : mhFast, 1.0f );
			MH_Str( &e->duelNameT[k], mhWhite, 1.0f );
			MH_Str( &e->duelLapT[k], mhWhite, 1.0f );
			MH_Str( &e->duelNoteT[k], mhEnd.duelNoteColor[k], 1.0f );
		}
		MH_Str( &e->vsT, mhDim, 1.0f );
	}

	/* a drag duel: the time slips */
	if ( e->slipCount ) {
		MH_Str( &e->slipTitleT, mhLabel, 1.0f );
		MH_Str( &e->slipNoteT, mhDim, 1.0f );
	}
	for ( k = 0; k < e->slipCount; k++ ) {
		const wsRect_t *c = &e->slipCar[k];
		MH_FillRect( &e->slip[k], mhPanel, 1.0f );
		MH_Border( &e->slip[k], mhLine, 2.0f, e->slipWin[k] ? mhAccent : e->slipMe[k] ? mhSoft : mhLine, 1.0f );
		MH_FillRect( c, mhShowroom, 1.0f );
		CG_DrawShowroomCar( MH_Snap( c->x ), MH_Snap( c->y ), MH_Snap( c->x + c->w ) - MH_Snap( c->x ),
		                    MH_Snap( c->y + c->h ) - MH_Snap( c->y ), mhEnd.slipClient[k],
		                    AngleMod( 150.0f + k * 60.0f + cg.time * 0.018f ) );
		MH_Str( &e->slipLaneT[k], mhMuted, 1.0f );
		MH_Str( &e->slipYouT[k], mhAccent, 1.0f );
		MH_Str( &e->slipTagT[k], mhAccent, 1.0f );
		MH_Str( &e->slipNameT[k], mhWhite, 1.0f );
		for ( i = 0; i < e->slipRows; i++ ) {
			const wsRect_t *rule = &e->slipRule[k][i];
			float dx;
			MH_Str( &e->slipLabelT[k][i], mhMuted, 1.0f );
			MH_Str( &e->slipValueT[k][i], !Q_stricmp( e->slipValue[k][i], "FOUL" ) || !Q_stricmp( e->slipValue[k][i], "DNF" ) ? mhAccent :
			        mhEnd.slipBetter[k][i] ? mhFast : mhValue, 1.0f );
			for ( dx = 0.0f; dx < rule->w; dx += 6.0f )	/* dashed */
				MH_Fill( rule->x + dx, rule->y, dx + 3.0f > rule->w ? rule->w - dx : 3.0f, 1.0f, mhTrack, 1.0f );
		}
	}

	if ( e->awardCount ) MH_Str( &e->awardsTitleT, mhLabel, 1.0f );
	for ( i = 0; i < e->awardCount; i++ ) {
		wsRect_t strip = e->award[i];
		strip.w = 3.0f;
		MH_FillRect( &e->award[i], mhPanel, 1.0f );
		MH_Border( &e->award[i], mhLine, 1.0f, NULL, 1.0f );
		MH_FillRect( &strip, mhEnd.awardColor[i], 1.0f );
		MH_Pic( &e->awardIcon[i], 0.0f, mhEnd.awardIcon[i], mhEnd.awardColor[i], 1.0f );
		MH_Str( &e->awardLabelT[i], mhMuted, 1.0f );
		MH_Str( &e->awardNameT[i], mhWhite, 1.0f );
		MH_Str( &e->awardValueT[i], mhEnd.awardColor[i], 1.0f );
	}

	MH_Panel( &e->list, qtrue, 1.0f );
	MH_Str( &e->listTitleT, mhLabel, 1.0f );
	MH_Str( &e->listRightT, mhMuted, 1.0f );
	for ( i = 0; i < e->teamCellCount; i++ ) {
		MH_FillRect( &e->teamCell[i], MH_TeamColor( TEAM_RED + mhEnd.team[i] ), 1.0f );
		MH_Str( &e->teamCellNameT[i], mhOnTeam, 1.0f );
		MH_Str( &e->teamCellValueT[i], mhOnTeam, 1.0f );
	}
	for ( i = 0; i < 4; i++ ) {
		mhText_t head = e->headT[i];
		float tw = WS_TextWidth( &head.style, head.text, strlen( head.text ) );
		if ( e->listMatches ) { if ( i == 2 ) head.box.x += ( head.box.w - tw ) * 0.5f; }	/* the heats centred, the beaten left */
		else if ( i >= 2 ) head.box.x += head.box.w - tw;
		MH_Str( &head, mhMuted, 1.0f );
	}
	for ( i = 0; i < e->rowCount; i++ ) {
		mhText_t time = e->rowTimeT[i], best = e->rowBestT[i];
		if ( mhEnd.rowMe[i] ) MH_FillRect( &e->row[i], mhMe, 1.0f );
		MH_Str( &e->rowPosT[i], !mhEnd.drag && !Q_stricmp( e->rowPos[i], "01" ) ? mhAccent : mhMuted, 1.0f );
		MH_Str( &e->rowNameT[i], mhWhite, 1.0f );
		if ( e->rowSwatch ) MH_FillRect( &e->rowSwatchR[i], MH_TeamColor( mhEnd.rowTeam[i] ), 1.0f );
		if ( e->listMatches ) {	/* a match: the heats centred, the one beaten in grey */
			time.box.x += ( time.box.w - WS_TextWidth( &time.style, time.text, strlen( time.text ) ) ) * 0.5f;
			MH_Str( &time, mhValue, 1.0f );
			MH_Str( &best, mhDim, 1.0f );
			continue;
		}
		time.box.x += time.box.w - WS_TextWidth( &time.style, time.text, strlen( time.text ) );
		best.box.x += best.box.w - WS_TextWidth( &best.style, best.text, strlen( best.text ) );
		MH_Str( &time, mhValue, 1.0f );
		MH_Str( &best, mhEnd.drag || mhEnd.ghost || mhEnd.combat || mhEnd.racingDM ? mhEnd.rowBestColor[i] : mhEnd.rowClient[i] == mhEnd.fastest ? mhPurple : mhMuted, 1.0f );
	}

	/* a ghost race: the ladder, yours lit, the ghost raced marked */
	if ( e->ladderCount ) {
		MH_Panel( &e->ladder, qtrue, 1.0f );
		MH_Str( &e->ladderTitleT, mhLabel, 1.0f );
		MH_Str( &e->ladderRightT, mhMuted, 1.0f );
		for ( i = 0; i < e->ladderCount; i++ ) {
			mhText_t lap = e->ladderLapT[i], tag = e->ladderTagT[i];
			if ( e->ladderMe[i] ) MH_FillRect( &e->ladderRow[i], mhMe, 1.0f );
			MH_Str( &e->ladderPosT[i], e->ladderMe[i] ? mhWhite : mhMuted, 1.0f );
			MH_Str( &e->ladderNameT[i], mhWhite, 1.0f );
			lap.box.x += lap.box.w - WS_TextWidth( &lap.style, lap.text, strlen( lap.text ) );
			tag.box.x += tag.box.w - WS_TextWidth( &tag.style, tag.text, strlen( tag.text ) );
			MH_Str( &lap, e->ladderMe[i] ? mhValue : mhMuted, 1.0f );
			MH_Str( &tag, mhFast, 1.0f );
		}
	}

	for ( i = 0; i < 3; i++ ) {
		MH_Str( &e->keyNameT[i], mhWhite, 1.0f );
		MH_Str( &e->keyActionT[i], i == 0 && !Q_stricmp( e->keyAction[0], "READY \xb7 OK" ) ? mhAccent : mhDim, 1.0f );
	}
	MH_Str( &e->readyLabelT, mhLabel, 1.0f );
	MH_Str( &e->readyValueT, mhWhite, 1.0f );
	MH_Bar( &e->readyBar, mhEnd.readyFrac, mhAccent, mhTrack, 1.0f );

	mhScale = base;
}


/* =========================================================================
   the TAB table (gen.py TabRacing, TabTeamRacing, TabDeathmatch, TabCTF):
   held with TAB, and on its own between maps where no end screen is
   ========================================================================= */

static const vec4_t mhTabCar   = { 102 / 255.0f, 102 / 255.0f, 102 / 255.0f, 1.0f };	/* #666 */
static const vec4_t mhTabVeil  = { 0.0f, 0.0f, 0.0f, 0.62f };

static struct {
	mhTabLayout_t	t;
	char			sub[96], rightValue[16], server[80], spectators[160];
	char			cell[MH_TAB_ROWS][MH_TAB_COLS][24];
	char			car[MH_TAB_ROWS][24];
	const float		*color[MH_TAB_ROWS][MH_TAB_COLS];
	qboolean		rowMe[MH_TAB_ROWS];
	int				rowTeam[MH_TAB_ROWS];
	char			sideTitle[MH_TAB_SIDES][16], sideNote[MH_TAB_SIDES][24], sideScore[MH_TAB_SIDES][8];
	int				sideTeam[MH_TAB_SIDES];
	char			cellName[MH_TAB_SIDES][16], cellValue[MH_TAB_SIDES][40], cellPlace[MH_TAB_SIDES][8];
	int				cellTeam[MH_TAB_SIDES];
	int				scoresAsked;
} mhTab;

static const char *mhTabTeamNames[MH_TAB_SIDES] = { "RED", "BLUE", "GREEN", "YELLOW" };

/* a column: its head, width (0: the rest), right-aligned, kind */
static int MH_TabCol( const char *head, float w, qboolean right, mhTabKind_t kind ) {
	mhTabLayout_t *t = &mhTab.t;
	int c = t->colCount;
	if ( c >= MH_TAB_COLS ) return MH_TAB_COLS - 1;
	t->colHead[c] = head;
	t->colW[c] = w;
	t->colRight[c] = right;
	t->colKind[c] = kind;
	t->colCount++;
	return c;
}

static void QDECL MH_TabSet( int row, int col, const float *color, const char *fmt, ... ) __attribute__ ((format (printf, 4, 5)));
static void QDECL MH_TabSet( int row, int col, const float *color, const char *fmt, ... ) {
	va_list ap;
	if ( row < 0 || row >= MH_TAB_ROWS || col < 0 || col >= MH_TAB_COLS ) return;
	va_start( ap, fmt );
	Q_vsnprintf( mhTab.cell[row][col], sizeof( mhTab.cell[row][col] ), fmt, ap );
	va_end( ap );
	mhTab.color[row][col] = color;
}

/* the name and the car of a row; the place in front where there is a column for it */
static void MH_TabDriver( int row, int client, int posCol, int place, int nameCol, qboolean out ) {
	clientInfo_t *ci = &cgs.clientinfo[client];
	char plain[40];
	if ( posCol >= 0 ) {
		if ( place > 0 ) MH_TabSet( row, posCol, place == 1 ? mhAccent : mhMuted, "%02d", place );
		else MH_TabSet( row, posCol, mhMuted, "--" );
	}
	MH_Plain( ci->name, plain, sizeof( plain ) );
	MH_TabSet( row, nameCol, out ? mhDim : mhWhite, "%s", plain );
	Q_strncpyz( mhTab.car[row], ci->modelName, sizeof( mhTab.car[row] ) );
	mhTab.rowMe[row] = client == cg.snap->ps.clientNum;
	mhTab.rowTeam[row] = MH_Team( client );
}

static void MH_TabPing( int row, int col, int client ) {
	int i, ping = -1;
	if ( cgs.clientinfo[client].botSkill > 0 ) {
		MH_TabSet( row, col, mhDim, "BOT" );
		return;
	}
	for ( i = 0; i < cg.numScores; i++ ) if ( cg.scores[i].client == client ) ping = cg.scores[i].ping;
	if ( ping < 0 ) MH_TabSet( row, col, mhMuted, "--" );
	else MH_TabSet( row, col, ping >= 100 ? mhAccent : mhMuted, "%d", ping );
}

static const score_t *MH_TabScore( int client ) {
	int i;
	for ( i = 0; i < cg.numScores; i++ ) if ( cg.scores[i].client == client ) return &cg.scores[i];
	return NULL;
}

/* the rows a table shows: all of them up to 14, then 24 low ones; past that
 * the first ones and you */
static int MH_TabFit( int *list, int count, int me, int limit ) {
	int i;
	if ( count <= limit ) return count;
	for ( i = 0; i < count; i++ ) if ( list[i] == me ) break;
	if ( i >= limit ) list[limit - 1] = me;
	return limit;
}

static void MH_TabDragText( const centity_t *car, int what, char *out, int size ) {
	int ms = -1;
	Q_strncpyz( out, "--", size );
	if ( car->dragPhase == DRAG_FALSE_START ) return;
	if ( what == 3 ) {
		if ( car->dragPhase == DRAG_FINISHED && car->dragTrapSpeed >= 0 )
			Com_sprintf( out, size, "%d.%d", car->dragTrapSpeed / 10, car->dragTrapSpeed % 10 );
		return;
	}
	if ( what == 0 && car->dragGreenTime > 0 && car->dragLaunchTime >= car->dragGreenTime ) {
		ms = car->dragLaunchTime - car->dragGreenTime;
	} else if ( car->dragPhase == DRAG_FINISHED && car->dragFinishTime >= car->dragLaunchTime && car->dragLaunchTime > 0 ) {
		if ( what == 1 ) ms = car->dragFinishTime - car->dragLaunchTime;
		else if ( what == 2 && car->dragGreenTime > 0 ) ms = car->dragFinishTime - car->dragGreenTime;
	}
	if ( ms >= 0 ) Com_sprintf( out, size, "%d.%03d", ms / 1000, ms % 1000 );
}

static const char *MH_TabDragStatus( int phase ) {
	switch ( phase ) {
	case DRAG_FALSE_START: return "FALSE START";
	case DRAG_DNF:         return "DNF";
	case DRAG_FINISHED:    return "FINISHED";
	case DRAG_RUNNING:     return "RUNNING";
	case DRAG_STAGED:
	case DRAG_COUNTDOWN:   return "STAGED";
	default:               return "WAIT";
	}
}

static qboolean MH_TabOver( void ) {
	return cg.intermissionStarted || cg.snap->ps.pm_type == PM_INTERMISSION;
}

/* the races: by place, the gap to the leader, laps, last and best lap */
static void MH_TabRace( const char *map ) {
	mhTabLayout_t *t = &mhTab.t;
	int at[MAX_CLIENTS + 1], list[MAX_CLIENTS], count = 0, local, last, i, k, client, leader;
	int laps = CG_RaceLapLimit(), fastest = 0, leaderLap = 0, rows;
	qboolean teams = cgs.gametype == GT_TEAM_RACING || cgs.gametype == GT_TEAM_RACING_DM;
	qboolean combat = cgs.gametype == GT_RACING_DM || cgs.gametype == GT_TEAM_RACING_DM;
	qboolean drag = cgs.gametype == GT_DRAG, lapped = laps > 1 && !drag;
	int posCol, nameCol, lapCol = -1, gapCol = -1, lastCol = -1, bestCol = -1, timeCol = -1, fragCol = -1, pingCol;
	int dragCol[4] = { -1, -1, -1, -1 }, statusCol = -1, swatchCol = -1;
	char text[24];

	last = MH_RaceOrder( at, &local );
	MH_Gaps( at, last );
	for ( i = 1; i <= last; i++ ) if ( at[i] >= 0 ) list[count++] = at[i];
	/* drivers the order has no place for yet: after it */
	for ( i = 0; i < cgs.maxclients && i < MAX_CLIENTS; i++ ) {
		if ( !cgs.clientinfo[i].infoValid || cgs.clientinfo[i].team == TEAM_SPECTATOR ) continue;
		for ( k = 0; k < count && list[k] != i; k++ );
		if ( k == count ) list[count++] = i;
	}
	leader = count > 0 ? list[0] : -1;
	for ( i = 0; i < count; i++ ) {
		int best = cg_entities[list[i]].bestLapTime;
		if ( best > 0 && ( !fastest || best < fastest ) ) fastest = best;
	}
	if ( leader >= 0 ) leaderLap = cg_entities[leader].currentLap;
	if ( laps > 0 && leaderLap > laps ) leaderLap = laps;

	/* the columns */
	posCol = MH_TabCol( "POS", 34.0f, qfalse, MH_TAB_MONO );
	if ( teams ) swatchCol = MH_TabCol( "", 10.0f, qfalse, MH_TAB_SWATCH );
	nameCol = MH_TabCol( "DRIVER", 0.0f, qfalse, MH_TAB_NAME );
	if ( drag ) {
		dragCol[0] = MH_TabCol( "RT", 64.0f, qtrue, MH_TAB_MONO );
		dragCol[1] = MH_TabCol( "ET", 64.0f, qtrue, MH_TAB_MONO );
		dragCol[2] = MH_TabCol( "TOTAL", 72.0f, qtrue, MH_TAB_MONO );
		dragCol[3] = MH_TabCol( "KM/H", 56.0f, qtrue, MH_TAB_MONO );
		statusCol = MH_TabCol( "STATUS", 110.0f, qtrue, MH_TAB_MONO );
	} else {
		if ( lapped ) lapCol = MH_TabCol( "LAP", 44.0f, qtrue, MH_TAB_MONO );
		gapCol = MH_TabCol( "GAP", 96.0f, qtrue, MH_TAB_MONO );
		if ( lapped ) {
			lastCol = MH_TabCol( "LAST", 84.0f, qtrue, MH_TAB_MONO );
			bestCol = MH_TabCol( "BEST", 84.0f, qtrue, MH_TAB_MONO );
		} else {
			timeCol = MH_TabCol( "TIME", 96.0f, qtrue, MH_TAB_MONO );
		}
		if ( combat ) fragCol = MH_TabCol( "FRAGS", 52.0f, qtrue, MH_TAB_MONO );
	}
	pingCol = MH_TabCol( "PING", 44.0f, qtrue, MH_TAB_PING );
	t->iconCol = gapCol;

	rows = MH_TabFit( list, count, cg.snap->ps.clientNum, count > 14 ? 24 : 14 );
	t->compact = count > 14;
	for ( i = 0; i < rows; i++ ) {
		centity_t *car;
		const cgRaceResult_t *r;
		qboolean out, done, dnf;
		int place;
		client = list[i];
		car = &cg_entities[client];
		r = CG_RaceResult( client );
		place = i < count && client == list[i] ? i + 1 : 0;
		for ( k = 1; k <= last; k++ ) if ( at[k] == client ) place = k;
		done = car->finishRaceTime > 0 && !( cgs.gametype == GT_ELIMINATION && car->eliminationOut );
		out = ( cgs.gametype == GT_ELIMINATION && car->eliminationOut ) ||
		      ( drag && ( car->dragPhase == DRAG_FALSE_START || car->dragPhase == DRAG_DNF ) );
		dnf = !done && MH_TabOver() && car->startRaceTime > 0;
		MH_TabDriver( i, client, posCol, place, nameCol, out || dnf );
		if ( swatchCol >= 0 ) mhTab.color[i][swatchCol] = MH_TeamColor( mhTab.rowTeam[i] );
		t->rowIcon[i] = done && !drag;

		if ( drag ) {
			for ( k = 0; k < 4; k++ ) {
				MH_TabDragText( car, k, text, sizeof( text ) );
				MH_TabSet( i, dragCol[k], k == 2 ? mhValue : mhMuted, "%s", text );
			}
			MH_TabSet( i, statusCol, out ? mhAccent : car->dragPhase == DRAG_FINISHED ? mhWhite : mhMuted,
			           "%s", MH_TabDragStatus( car->dragPhase ) );
		} else {
			if ( lapCol >= 0 ) {
				int lap = car->currentLap;
				if ( laps > 0 && ( lap > laps || done ) ) lap = laps;
				if ( lap > 0 ) MH_TabSet( i, lapCol, out ? mhDim : mhValue, "%d", lap );
				else MH_TabSet( i, lapCol, mhDim, "-" );
			}
			/* the gap: the leader's own line, out, a lap or more down, the split */
			if ( out ) MH_TabSet( i, gapCol, mhDim, "OUT" );
			else if ( dnf ) MH_TabSet( i, gapCol, mhDim, "DNF" );
			else if ( client == leader ) {
				if ( done ) {
					CG_ResultTime( car->finishRaceTime - car->startRaceTime, text, sizeof( text ) );
					MH_TabSet( i, gapCol, mhAccent, "%s", text );
				} else MH_TabSet( i, gapCol, mhAccent, "LEADER" );
			} else if ( done && leader >= 0 && cg_entities[leader].finishRaceTime > 0 ) {
				MH_Gap( ( car->finishRaceTime - car->startRaceTime ) -
				        ( cg_entities[leader].finishRaceTime - cg_entities[leader].startRaceTime ), text, sizeof( text ) );
				MH_TabSet( i, gapCol, mhValue, "%s", text );
			} else if ( lapped && !done && leaderLap - car->currentLap >= 1 && car->currentLap > 0 ) {
				k = leaderLap - car->currentLap;
				MH_TabSet( i, gapCol, mhValue, "+%d LAP%s", k, k == 1 ? "" : "S" );
			} else if ( place > 1 && place <= MAX_CLIENTS && mhGap[place] >= 0 ) {
				MH_Gap( mhGap[place], text, sizeof( text ) );
				MH_TabSet( i, gapCol, mhValue, "%s", text );
			} else MH_TabSet( i, gapCol, mhMuted, "--" );

			if ( lastCol >= 0 ) {
				if ( r->lapCount > 0 ) {
					CG_ResultTime( r->laps[r->lapCount - 1], text, sizeof( text ) );
					MH_TabSet( i, lastCol, mhMuted, "%s", text );
				} else MH_TabSet( i, lastCol, mhMuted, "--" );
				if ( car->bestLapTime > 0 ) {
					CG_ResultTime( car->bestLapTime, text, sizeof( text ) );
					MH_TabSet( i, bestCol, car->bestLapTime == fastest ? mhPurple : mhMuted, "%s", text );
				} else MH_TabSet( i, bestCol, mhMuted, "--" );
			}
			if ( timeCol >= 0 ) {
				int ms = done ? car->finishRaceTime - car->startRaceTime :
				         car->startRaceTime > 0 && !dnf && !out ? cg.time - car->startRaceTime : 0;
				if ( ms > 0 ) {
					CG_ResultTime( ms, text, sizeof( text ) );
					MH_TabSet( i, timeCol, done ? mhWhite : mhMuted, "%s", text );
				} else MH_TabSet( i, timeCol, mhMuted, "--" );
			}
			if ( fragCol >= 0 ) {
				const score_t *sc = MH_TabScore( client );
				MH_TabSet( i, fragCol, mhValue, "%d", sc ? sc->score : 0 );
			}
		}
		MH_TabPing( i, pingCol, client );
	}
	t->rowCount = rows;

	/* the header: the lap the leader is on, the time the leader has run */
	t->mode = cgs.gametype == GT_SPRINT ? "SPRINT" : cgs.gametype == GT_GHOST ? "GHOST RACE" :
	          cgs.gametype == GT_DRAG ? "DRAG RACE" : cgs.gametype == GT_TEAM_RACING ? "TEAM RACE" :
	          cgs.gametype == GT_RACING_DM ? "RACE COMBAT" : cgs.gametype == GT_TEAM_RACING_DM ? "TEAM RACE COMBAT" :
	          cgs.gametype == GT_ELIMINATION ? "ELIMINATION" : "RACE";
	{
		char lapText[24], teamText[24];
		lapText[0] = teamText[0] = '\0';
		if ( lapped ) Com_sprintf( lapText, sizeof( lapText ), " \xb7 LAP %d / %d", leaderLap > 0 ? leaderLap : 1, laps );
		if ( teams ) {
			int n = 0;
			for ( k = 0; k < MH_TAB_SIDES; k++ ) {
				for ( i = 0; i < count && MH_Team( list[i] ) != TEAM_RED + k; i++ );
				if ( i < count ) n++;
			}
			Com_sprintf( teamText, sizeof( teamText ), " \xb7 %d TEAMS", n );
		}
		Com_sprintf( mhTab.sub, sizeof( mhTab.sub ), "%s%s%s \xb7 %d DRIVER%s", map, lapText, teamText, count, count == 1 ? "" : "S" );
	}
	t->rightLabel = "RACE TIME";
	if ( leader >= 0 && cg_entities[leader].startRaceTime > 0 ) {
		centity_t *l = &cg_entities[leader];
		MH_Time( ( l->finishRaceTime > 0 ? l->finishRaceTime : cg.time ) - l->startRaceTime, mhTab.rightValue, sizeof( mhTab.rightValue ) );
	} else Q_strncpyz( mhTab.rightValue, "0:00.00", sizeof( mhTab.rightValue ) );

	/* a team race: the teams over the drivers, by their drivers' places */
	if ( teams ) {
		int sum[MH_TAB_SIDES], n[MH_TAB_SIDES], best[MH_TAB_SIDES], order[MH_TAB_SIDES], teamCount = 0;
		for ( k = 0; k < MH_TAB_SIDES; k++ ) {
			sum[k] = n[k] = best[k] = 0;
			for ( i = 0; i < count; i++ ) {
				if ( MH_Team( list[i] ) != TEAM_RED + k ) continue;
				sum[k] += i + 1;
				if ( !best[k] ) best[k] = i + 1;
				n[k]++;
			}
			if ( n[k] ) order[teamCount++] = k;
		}
		for ( i = 1; i < teamCount; i++ ) {	/* by the average place, the better best place among equals */
			for ( k = i; k > 0; k-- ) {
				int a = order[k - 1], b = order[k];
				if ( sum[a] * n[b] < sum[b] * n[a] || ( sum[a] * n[b] == sum[b] * n[a] && best[a] <= best[b] ) ) break;
				order[k - 1] = b;
				order[k] = a;
			}
		}
		for ( i = 0; i < teamCount; i++ ) {
			k = order[i];
			mhTab.cellTeam[i] = TEAM_RED + k;
			Q_strncpyz( mhTab.cellName[i], mhTabTeamNames[k], sizeof( mhTab.cellName[i] ) );
			Com_sprintf( mhTab.cellValue[i], sizeof( mhTab.cellValue[i] ), "BEST P%d \xb7 %d DRIVER%s", best[k], n[k], n[k] == 1 ? "" : "S" );
			Com_sprintf( mhTab.cellPlace[i], sizeof( mhTab.cellPlace[i] ), "P%d", i + 1 );
			t->teamCellName[i] = mhTab.cellName[i];
			t->teamCellValue[i] = mhTab.cellValue[i];
			t->teamCellPlace[i] = mhTab.cellPlace[i];
		}
		t->teamCellCount = teamCount;
	}
}

/* combat for everybody: by score, the damage dealt and taken */
static void MH_TabCombat( const char *map ) {
	mhTabLayout_t *t = &mhTab.t;
	int list[MAX_CLIENTS], count = 0, i, rows, client;
	int posCol, nameCol, scoreCol, dealtCol, takenCol, pingCol;
	qboolean lastCar = cgs.gametype == GT_LCS || cgs.gametype == GT_DERBY;
	char text[16], clock[16];

	for ( i = 0; i < cg.numScores; i++ ) {
		client = cg.scores[i].client;
		if ( client < 0 || client >= MAX_CLIENTS || !cgs.clientinfo[client].infoValid ) continue;
		if ( cgs.clientinfo[client].team == TEAM_SPECTATOR ) continue;
		list[count++] = client;
	}
	posCol = MH_TabCol( "POS", 34.0f, qfalse, MH_TAB_MONO );
	nameCol = MH_TabCol( "DRIVER", 0.0f, qfalse, MH_TAB_NAME );
	scoreCol = MH_TabCol( "SCORE", 64.0f, qtrue, MH_TAB_SCORE );
	dealtCol = MH_TabCol( "DEALT", 84.0f, qtrue, MH_TAB_MONO );
	takenCol = MH_TabCol( "TAKEN", 84.0f, qtrue, MH_TAB_MONO );
	pingCol = MH_TabCol( "PING", 44.0f, qtrue, MH_TAB_PING );
	t->iconCol = -1;

	rows = MH_TabFit( list, count, cg.snap->ps.clientNum, count > 14 ? 24 : 14 );
	t->compact = count > 14;
	for ( i = 0; i < rows; i++ ) {
		const score_t *sc = MH_TabScore( list[i] );
		qboolean out = lastCar && MH_LastCarOut( list[i] );
		int place = 0, k;
		for ( k = 0; k < count; k++ ) if ( list[k] == list[i] ) place = k + 1;
		MH_TabDriver( i, list[i], posCol, place, nameCol, out );
		MH_TabSet( i, scoreCol, out ? mhDim : place == 1 ? mhAccent : mhWhite, "%d", sc ? sc->score : 0 );
		MH_Thousands( sc ? sc->damageDealt : 0, text, sizeof( text ) );
		MH_TabSet( i, dealtCol, mhValue, "%s", text );
		MH_Thousands( sc ? sc->damageTaken : 0, text, sizeof( text ) );
		MH_TabSet( i, takenCol, mhMuted, "%s", text );
		MH_TabPing( i, pingCol, list[i] );
	}
	t->rowCount = rows;

	t->mode = cgs.gametype == GT_LCS ? "LAST CAR STANDING" : cgs.gametype == GT_DERBY ? "DERBY" : "DEATHMATCH";
	if ( cgs.fraglimit > 0 && !lastCar )
		Com_sprintf( mhTab.sub, sizeof( mhTab.sub ), "%s \xb7 FRAGLIMIT %d \xb7 %d DRIVER%s", map, cgs.fraglimit, count, count == 1 ? "" : "S" );
	else if ( lastCar )
		Com_sprintf( mhTab.sub, sizeof( mhTab.sub ), "%s \xb7 %d OF %d LEFT", map, MH_LastCarAlive( -1 ), count );
	else
		Com_sprintf( mhTab.sub, sizeof( mhTab.sub ), "%s \xb7 %d DRIVER%s", map, count, count == 1 ? "" : "S" );
	t->rightLabel = MH_MatchClock( clock, sizeof( clock ) );
	Q_strncpyz( mhTab.rightValue, clock, sizeof( mhTab.rightValue ) );
}

/* the team games: a panel a team, its score in the head, the mode's columns */
static void MH_TabTeams( const char *map ) {
	mhTabLayout_t *t = &mhTab.t;
	int sides = cgs.gametype == GT_CTF4 ? 4 : 2, side, i, k, row = 0, client, total = 0, perSide, most = 0;
	int list[MAX_CLIENTS], count;
	int nameCol, scoreCol, pingCol, col[4] = { -1, -1, -1, -1 };
	char clock[16];

	nameCol = MH_TabCol( "DRIVER", 0.0f, qfalse, MH_TAB_NAME );
	scoreCol = MH_TabCol( "SCORE", cgs.gametype == GT_AUTOBALL ? 40.0f : 44.0f, qtrue, MH_TAB_SCORE );
	switch ( cgs.gametype ) {
	case GT_CTF:
	case GT_CTF4:
		col[0] = MH_TabCol( "CAPS", 36.0f, qtrue, MH_TAB_MONO );
		col[1] = MH_TabCol( "AST", 36.0f, qtrue, MH_TAB_MONO );
		col[2] = MH_TabCol( "DEF", 36.0f, qtrue, MH_TAB_MONO );
		break;
	case GT_KOTH:
		col[0] = MH_TabCol( "HOLD", 48.0f, qtrue, MH_TAB_MONO );
		col[1] = MH_TabCol( "HK", 36.0f, qtrue, MH_TAB_MONO );
		col[2] = MH_TabCol( "DEF", 36.0f, qtrue, MH_TAB_MONO );
		break;
	case GT_AUTOBALL:
		col[0] = MH_TabCol( "G", 30.0f, qtrue, MH_TAB_MONO );
		col[1] = MH_TabCol( "A", 30.0f, qtrue, MH_TAB_MONO );
		col[2] = MH_TabCol( "SV", 30.0f, qtrue, MH_TAB_MONO );
		col[3] = MH_TabCol( "SH", 30.0f, qtrue, MH_TAB_MONO );
		break;
	default:
		col[0] = MH_TabCol( "DEALT", 56.0f, qtrue, MH_TAB_MONO );
		col[1] = MH_TabCol( "TAKEN", 56.0f, qtrue, MH_TAB_MONO );
		break;
	}
	pingCol = MH_TabCol( "PING", 40.0f, qtrue, MH_TAB_PING );
	t->iconCol = -1;

	/* how many rows a team gets: 10 (16 low ones) side by side, 8 low ones two by two */
	for ( side = 0; side < sides; side++ ) {
		for ( count = 0, i = 0; i < cg.numScores; i++ ) {
			client = cg.scores[i].client;
			if ( client >= 0 && client < MAX_CLIENTS && cgs.clientinfo[client].infoValid && MH_Team( client ) == TEAM_RED + side ) count++;
		}
		if ( count > most ) most = count;
	}
	t->compact = sides > 2 || most > 10;
	perSide = sides > 2 ? 8 : most > 10 ? 16 : 10;

	for ( side = 0; side < sides; side++ ) {
		int score = cg.teamScores[side], next = -999999, note;
		for ( count = 0, i = 0; i < cg.numScores; i++ ) {
			client = cg.scores[i].client;
			if ( client < 0 || client >= MAX_CLIENTS || !cgs.clientinfo[client].infoValid ) continue;
			if ( MH_Team( client ) != TEAM_RED + side ) continue;
			list[count++] = client;
		}
		total += count;
		count = MH_TabFit( list, count, cg.snap->ps.clientNum, perSide );
		for ( i = 0; i < count && row < MH_TAB_ROWS; i++, row++ ) {
			const score_t *sc = MH_TabScore( list[i] );
			int value[4] = { 0, 0, 0, 0 };
			MH_TabDriver( row, list[i], -1, 0, nameCol, qfalse );
			MH_TabSet( row, scoreCol, mhWhite, "%d", sc ? sc->score : 0 );
			if ( sc ) {
				switch ( cgs.gametype ) {
				case GT_CTF:
				case GT_CTF4:
					value[0] = sc->captures; value[1] = sc->assistCount; value[2] = sc->defendCount;
					break;
				case GT_KOTH:
					value[1] = sc->kothHillKills; value[2] = sc->defendCount;
					break;
				case GT_AUTOBALL:
					value[0] = sc->autoballGoals; value[1] = sc->autoballAssists;
					value[2] = sc->autoballSaves; value[3] = sc->autoballShots;
					break;
				default:
					value[0] = sc->damageDealt; value[1] = sc->damageTaken;
					break;
				}
			}
			for ( k = 0; k < 4; k++ ) {
				char text[16];
				if ( col[k] < 0 ) continue;
				if ( cgs.gametype == GT_KOTH && k == 0 ) {
					int secs = sc ? sc->kothHoldTimeMs / 1000 : 0;
					MH_TabSet( row, col[k], mhValue, "%d:%02d", secs / 60, secs % 60 );
					continue;
				}
				if ( value[k] >= 1000 ) MH_Thousands( value[k], text, sizeof( text ) );
				else Com_sprintf( text, sizeof( text ), "%d", value[k] );
				MH_TabSet( row, col[k], value[k] ? mhValue : mhTabCar, "%s", text );
			}
			MH_TabPing( row, pingCol, list[i] );
		}
		t->sideRows[side] = count;

		/* the head: the team, how it stands, the score */
		for ( k = 0; k < sides; k++ ) if ( k != side && cg.teamScores[k] > next ) next = cg.teamScores[k];
		mhTab.sideTeam[side] = TEAM_RED + side;
		Com_sprintf( mhTab.sideTitle[side], sizeof( mhTab.sideTitle[side] ), "%s TEAM", mhTabTeamNames[side] );
		Com_sprintf( mhTab.sideScore[side], sizeof( mhTab.sideScore[side] ), "%d", score );
		note = score - next;
		if ( note > 0 ) Com_sprintf( mhTab.sideNote[side], sizeof( mhTab.sideNote[side] ), "LEADS BY %d", note );
		else if ( note == 0 ) Q_strncpyz( mhTab.sideNote[side], "TIED", sizeof( mhTab.sideNote[side] ) );
		else Com_sprintf( mhTab.sideNote[side], sizeof( mhTab.sideNote[side] ), "%d DRIVER%s", t->sideRows[side], t->sideRows[side] == 1 ? "" : "S" );
		t->sideTitle[side] = mhTab.sideTitle[side];
		t->sideNote[side] = mhTab.sideNote[side];
		t->sideScore[side] = mhTab.sideScore[side];
	}
	t->sideCount = sides;
	t->rowCount = row;

	t->mode = cgs.gametype == GT_CTF || cgs.gametype == GT_CTF4 ? "CAPTURE THE FLAG" : cgs.gametype == GT_DOMINATION ? "DOMINATION" :
	          cgs.gametype == GT_KOTH ? "KING OF THE HILL" : cgs.gametype == GT_AUTOBALL ? "AUTOBALL" : "TEAM DEATHMATCH";
	if ( ( cgs.gametype == GT_CTF || cgs.gametype == GT_CTF4 ) && cgs.capturelimit > 0 )
		Com_sprintf( mhTab.sub, sizeof( mhTab.sub ), "%s \xb7 CAPTURELIMIT %d \xb7 %d DRIVERS", map, cgs.capturelimit, total );
	else if ( cgs.gametype == GT_TEAM && cgs.fraglimit > 0 )
		Com_sprintf( mhTab.sub, sizeof( mhTab.sub ), "%s \xb7 FRAGLIMIT %d \xb7 %d DRIVERS", map, cgs.fraglimit, total );
	else
		Com_sprintf( mhTab.sub, sizeof( mhTab.sub ), "%s \xb7 %d DRIVERS", map, total );
	t->rightLabel = MH_MatchClock( clock, sizeof( clock ) );
	Q_strncpyz( mhTab.rightValue, clock, sizeof( mhTab.rightValue ) );
}

static void MH_TabPrepare( void ) {
	mhTabLayout_t *t = &mhTab.t;
	const char *map, *host;
	int i, players = 0, ready = 0, humans = 0;

	memset( t, 0, sizeof( *t ) );
	memset( mhTab.cell, 0, sizeof( mhTab.cell ) );
	memset( mhTab.car, 0, sizeof( mhTab.car ) );
	memset( mhTab.rowMe, 0, sizeof( mhTab.rowMe ) );
	t->viewWidth = cgs.glconfig.vidWidth / mhScale;
	map = CG_ConfigString( CS_MESSAGE );
	if ( !map || !map[0] ) map = COM_SkipPath( cgs.mapname );

	switch ( cgs.gametype ) {
	case GT_DEATHMATCH:
	case GT_LCS:
	case GT_DERBY:
		MH_TabCombat( map );
		break;
	case GT_TEAM:
	case GT_CTF:
	case GT_CTF4:
	case GT_DOMINATION:
	case GT_KOTH:
	case GT_AUTOBALL:
		MH_TabTeams( map );
		break;
	default:
		MH_TabRace( map );
		break;
	}
	for ( i = 0; i < t->rowCount; i++ ) {
		int c;
		for ( c = 0; c < t->colCount; c++ ) t->cell[i][c] = mhTab.cell[i][c];
		t->rowCar[i] = mhTab.car[i];
	}

	/* between maps: who is ready in place of the clock */
	for ( i = 0; i < cgs.maxclients && i < MAX_CLIENTS; i++ ) {
		if ( !cgs.clientinfo[i].infoValid ) continue;
		players++;
		if ( cgs.clientinfo[i].botSkill > 0 || cgs.clientinfo[i].team == TEAM_SPECTATOR ) continue;
		humans++;
		if ( cg.snap->ps.stats[STAT_CLIENTS_READY] & ( 1 << i ) ) ready++;
	}
	if ( cg.snap->ps.pm_type == PM_INTERMISSION && humans > 0 ) {
		t->rightLabel = "READY";
		Com_sprintf( mhTab.rightValue, sizeof( mhTab.rightValue ), "%d / %d", ready, humans );
	}
	t->sub = mhTab.sub;
	t->rightValue = mhTab.rightValue;

	/* the footer: the spectators, the server and how full it is */
	mhTab.spectators[0] = '\0';
	for ( i = 0; i < cgs.maxclients && i < MAX_CLIENTS; i++ ) {
		char name[40];
		if ( !cgs.clientinfo[i].infoValid || cgs.clientinfo[i].team != TEAM_SPECTATOR ) continue;
		MH_Plain( cgs.clientinfo[i].name, name, sizeof( name ) );
		if ( mhTab.spectators[0] ) Q_strcat( mhTab.spectators, sizeof( mhTab.spectators ), " \xb7 " );
		Q_strcat( mhTab.spectators, sizeof( mhTab.spectators ), name );
	}
	t->spectators = mhTab.spectators[0] ? mhTab.spectators : NULL;
	host = Info_ValueForKey( CG_ConfigString( CS_SERVERINFO ), "sv_hostname" );
	{
		char plain[48];
		MH_Plain( host, plain, sizeof( plain ) );
		if ( plain[0] ) Com_sprintf( mhTab.server, sizeof( mhTab.server ), "%s \xb7 %d / %d", plain, players, cgs.maxclients );
		else Com_sprintf( mhTab.server, sizeof( mhTab.server ), "%d / %d", players, cgs.maxclients );
	}
	t->server = mhTab.server;
	MH_TabLayout( t );
}

/* the table on screen: with TAB, or between maps where no end screen is */
qboolean CG_ModernHUD_DrawTab( void ) {
	mhTabLayout_t *t = &mhTab.t;
	int i, c, k;
	if ( !cg.snap || cg_paused.integer ) return qfalse;
	if ( !cg.showScores && !( cg.snap->ps.pm_type == PM_INTERMISSION && !CG_ModernHUD_EndActive() ) ) return qfalse;

	if ( cg.time - mhTab.scoresAsked > 2000 || cg.time < mhTab.scoresAsked ) {
		mhTab.scoresAsked = cg.time;
		trap_SendClientCommand( "score" );
	}
	MH_Register();
	mhScale = cgs.glconfig.vidHeight / MH_DESIGN_HEIGHT;
	MH_TabPrepare();

	WS_FillRect( 0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, mhTabVeil );
	MH_Str( &t->modeT, mhAccent, 1.0f );
	MH_Str( &t->subT, mhMuted, 1.0f );
	MH_Str( &t->rightLabelT, mhMuted, 1.0f );
	MH_Str( &t->rightValueT, mhWhite, 1.0f );

	if ( t->sideCount <= 0 ) {
		MH_Panel( &t->panel[0], qtrue, 1.0f );
		for ( k = 0; k < t->teamCellCount; k++ ) {
			MH_FillRect( &t->teamCell[k], MH_TeamColor( mhTab.cellTeam[k] ), 1.0f );
			MH_Str( &t->teamCellNameT[k], mhOnTeam, 1.0f );
			MH_Str( &t->teamCellValueT[k], mhOnTeam, 1.0f );
			MH_Str( &t->teamCellPlaceT[k], mhOnTeam, 1.0f );
		}
		for ( c = 0; c < t->colCount; c++ ) {
			mhText_t h = t->headT[0][c];
			if ( t->colRight[c] ) h.box.x += h.box.w - WS_TextWidth( &h.style, h.text, strlen( h.text ) );
			MH_Str( &h, mhMuted, 1.0f );
		}
	} else {
		for ( k = 0; k < t->sideCount; k++ ) {
			const float *tc = MH_TeamColor( mhTab.sideTeam[k] );
			MH_FillRect( &t->panel[k], mhPanel, 1.0f );
			MH_Border( &t->panel[k], mhLine, 3.0f, tc, 1.0f );
			MH_Str( &t->sideTitleT[k], tc, 1.0f );
			MH_Str( &t->sideNoteT[k], mhMuted, 1.0f );
			MH_Str( &t->sideScoreT[k], mhWhite, 1.0f );
			for ( c = 0; c < t->colCount; c++ ) {
				mhText_t h = t->headT[k][c];
				if ( t->colRight[c] ) h.box.x += h.box.w - WS_TextWidth( &h.style, h.text, strlen( h.text ) );
				MH_Str( &h, mhMuted, 1.0f );
			}
		}
	}

	for ( i = 0; i < t->rowCount; i++ ) {
		if ( mhTab.rowMe[i] ) MH_FillRect( &t->row[i], mhMe, 1.0f );
		for ( c = 0; c < t->colCount; c++ ) {
			mhText_t v = t->cellT[i][c];
			const float *color = mhTab.color[i][c] ? mhTab.color[i][c] : mhWhite;
			if ( t->colKind[c] == MH_TAB_SWATCH ) {
				MH_FillRect( &v.box, color, 1.0f );
				continue;
			}
			if ( t->colRight[c] ) v.box.x += v.box.w - WS_TextWidth( &v.style, v.text, strlen( v.text ) );
			MH_Str( &v, color, 1.0f );
			if ( t->colKind[c] == MH_TAB_NAME ) MH_Str( &t->carT[i], mhTabCar, 1.0f );
		}
		if ( t->rowIcon[i] ) MH_Pic( &t->icon[i], 0.0f, mh.flag, mhWhite, 1.0f );
	}

	MH_Str( &t->specLabelT, mhDim, 1.0f );
	MH_Str( &t->specT, mhLabel, 1.0f );
	MH_Str( &t->serverT, mhDim, 1.0f );
	return qtrue;
}
