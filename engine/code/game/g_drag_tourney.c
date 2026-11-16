/*
===========================================================================
Copyright (C) 2026 Q3Rally Team

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

g_drag_tourney.c -- Drag Race tournament (g_dragTournament 1)

A single elimination of g_dragTournamentSize drivers (4, 8 or 16): the humans
on the server, filled up with bots whose skill spreads around g_spSkill. Each
pairing is a normal best-of match (g_dragBestOf) on the current strip. The
two drivers of the pairing race; everyone else spectates. Bot against bot is
driven too, and a spectator can skip it ("dragskip", "dragskip all"): the
result is then drawn from the bots' skills.

Each match reloads the map, so the bracket lives in server cvars:
  g_dragTourneyA  "size bestOf|name,client,skill;..."  entrants 0..7
  g_dragTourneyB  "name,client,skill;..."               entrants 8..15
  g_dragTourneyR  "winner winsA winsB;..."              results in play order
client is the human's slot, -1 for a bot.
*/

#include "g_local.h"
#include "g_profile.h"

#define DT_MAX				16
#define DT_NAME				16
#define DT_LINEUP_RETRY		5000	/* ms before a lineup command is repeated */
#define DT_CHAMPION_BONUS	20		/* career points for winning the tournament */

extern void LogExit( const char *string );

typedef struct {
	char	name[DT_NAME];
	int		client;		/* human slot, -1 for a bot */
	int		skill;		/* bots: 1..5 */
} dtEntrant_t;

typedef struct {
	int		winner;		/* 0 first entrant, 1 second */
	int		winsA, winsB;
} dtResult_t;

static struct {
	qboolean	loaded;
	int			size;
	int			bestOf;
	int			played;
	dtEntrant_t	field[DT_MAX];
	dtResult_t	results[DT_MAX - 1];
	int			lineupTime;
	int			lineupMatch;
} dt;

/* ---------------------------------------------------------------- state */

static void DT_CleanName( const char *in, char *out ) {
	char clean[MAX_NETNAME];
	int i, n = 0;
	Q_strncpyz( clean, in, sizeof( clean ) );
	Q_CleanStr( clean );
	for ( i = 0; clean[i] && n < DT_NAME - 1; i++ ) {
		if ( clean[i] == ',' || clean[i] == ';' || clean[i] == '|' || clean[i] == '"' ) continue;
		out[n++] = clean[i];
	}
	out[n] = '\0';
	if ( !n ) Q_strncpyz( out, "Driver", DT_NAME );
}

static void DT_SaveEntrants( char *out, int size, int first, int last ) {
	int i;
	out[0] = '\0';
	for ( i = first; i < last && i < dt.size; i++ ) {
		Q_strcat( out, size, va( "%s%s,%d,%d", i > first ? ";" : "", dt.field[i].name,
			dt.field[i].client, dt.field[i].skill ) );
	}
}

static void DT_Save( void ) {
	char a[MAX_CVAR_VALUE_STRING], b[MAX_CVAR_VALUE_STRING], r[MAX_CVAR_VALUE_STRING];
	char entrants[MAX_CVAR_VALUE_STRING];
	int i;
	DT_SaveEntrants( entrants, sizeof( entrants ), 0, 8 );
	Com_sprintf( a, sizeof( a ), "%d %d|%s", dt.size, dt.bestOf, entrants );
	DT_SaveEntrants( b, sizeof( b ), 8, DT_MAX );
	r[0] = '\0';
	for ( i = 0; i < dt.played; i++ ) {
		Q_strcat( r, sizeof( r ), va( "%s%d %d %d", i ? ";" : "", dt.results[i].winner,
			dt.results[i].winsA, dt.results[i].winsB ) );
	}
	trap_Cvar_Set( "g_dragTourneyA", a );
	trap_Cvar_Set( "g_dragTourneyB", b );
	trap_Cvar_Set( "g_dragTourneyR", r );
}

static void DT_Clear( void ) {
	memset( &dt, 0, sizeof( dt ) );
	trap_Cvar_Set( "g_dragTourneyA", "" );
	trap_Cvar_Set( "g_dragTourneyB", "" );
	trap_Cvar_Set( "g_dragTourneyR", "" );
}

static int DT_LoadEntrants( const char *text, int first ) {
	char name[DT_NAME];
	int count = 0, n, client, skill;
	while ( *text && first + count < DT_MAX ) {
		n = 0;
		while ( *text && *text != ',' && n < DT_NAME - 1 ) name[n++] = *text++;
		name[n] = '\0';
		if ( *text != ',' ) break;
		text++;
		client = atoi( text );
		while ( *text && *text != ',' ) text++;
		if ( *text != ',' ) break;
		text++;
		skill = atoi( text );
		while ( *text && *text != ';' ) text++;
		if ( *text == ';' ) text++;
		Q_strncpyz( dt.field[first + count].name, name, DT_NAME );
		dt.field[first + count].client = client;
		dt.field[first + count].skill = skill;
		count++;
	}
	return count;
}

static qboolean DT_Load( void ) {
	char a[MAX_CVAR_VALUE_STRING], b[MAX_CVAR_VALUE_STRING], r[MAX_CVAR_VALUE_STRING];
	const char *p;
	int count;
	memset( &dt, 0, sizeof( dt ) );
	trap_Cvar_VariableStringBuffer( "g_dragTourneyA", a, sizeof( a ) );
	trap_Cvar_VariableStringBuffer( "g_dragTourneyB", b, sizeof( b ) );
	trap_Cvar_VariableStringBuffer( "g_dragTourneyR", r, sizeof( r ) );
	dt.size = atoi( a );
	p = strchr( a, ' ' );
	dt.bestOf = p ? atoi( p + 1 ) : 0;
	if ( dt.size != 4 && dt.size != 8 && dt.size != 16 ) {
		memset( &dt, 0, sizeof( dt ) );
		return qfalse;
	}
	p = strchr( a, '|' );
	count = p ? DT_LoadEntrants( p + 1, 0 ) : 0;
	count += DT_LoadEntrants( b, 8 );
	if ( count != dt.size ) {
		memset( &dt, 0, sizeof( dt ) );
		return qfalse;
	}
	for ( p = r; *p && dt.played < dt.size - 1; ) {
		dtResult_t *result = &dt.results[dt.played];
		const char *field = p;
		result->winner = atoi( field );
		if ( !( field = strchr( field, ' ' ) ) ) break;
		result->winsA = atoi( ++field );
		if ( !( field = strchr( field, ' ' ) ) ) break;
		result->winsB = atoi( ++field );
		dt.played++;
		p = strchr( field, ';' );
		if ( !p ) break;
		p++;
	}
	dt.loaded = qtrue;
	return qtrue;
}

/* ---------------------------------------------------------------- bracket */

/* Match k in play order: round 0 first, then the next round, and so on. */
static int DT_Round( int k, int *indexInRound, int *roundStart ) {
	int round = 0, start = 0, count = dt.size / 2;
	while ( count && k >= start + count ) {
		start += count;
		count /= 2;
		round++;
	}
	if ( indexInRound ) *indexInRound = k - start;
	if ( roundStart ) *roundStart = start;
	return round;
}

static void DT_MatchEntrants( int k, int *a, int *b );
static void DT_Record( int winnerSide, int winsA, int winsB );

static int DT_Winner( int k ) {
	int a, b;
	if ( k < 0 || k >= dt.played ) return -1;
	DT_MatchEntrants( k, &a, &b );
	return dt.results[k].winner ? b : a;
}

static void DT_MatchEntrants( int k, int *a, int *b ) {
	int index, start, round, previousStart;
	round = DT_Round( k, &index, &start );
	if ( !round ) {
		*a = 2 * index;
		*b = 2 * index + 1;
		return;
	}
	previousStart = start - ( dt.size >> round );
	*a = DT_Winner( previousStart + 2 * index );
	*b = DT_Winner( previousStart + 2 * index + 1 );
}

static qboolean DT_IsBot( int entrant ) {
	return entrant >= 0 && dt.field[entrant].client < 0;
}

/* The client driving an entrant right now, or -1. */
static int DT_ClientOf( int entrant ) {
	char name[DT_NAME];
	gclient_t *client;
	int i;
	if ( entrant < 0 || entrant >= dt.size ) return -1;
	if ( !DT_IsBot( entrant ) ) {
		i = dt.field[entrant].client;
		if ( i < 0 || i >= level.maxclients ) return -1;
		client = &level.clients[i];
		return client->pers.connected != CON_DISCONNECTED && !( g_entities[i].r.svFlags & SVF_BOT ) ? i : -1;
	}
	for ( i = 0; i < level.maxclients; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected == CON_DISCONNECTED || !( g_entities[i].r.svFlags & SVF_BOT ) ) continue;
		DT_CleanName( client->pers.netname, name );
		if ( !Q_stricmp( name, dt.field[entrant].name ) ) return i;
	}
	return -1;
}

static int DT_EntrantOfClient( int clientNum ) {
	int a, b;
	if ( !G_DragTourneyActive() ) return -1;
	DT_MatchEntrants( dt.played, &a, &b );
	if ( DT_ClientOf( a ) == clientNum ) return a;
	if ( DT_ClientOf( b ) == clientNum ) return b;
	return -1;
}

static const char *DT_RoundName( int round ) {
	int left = dt.size >> round;
	if ( left == 2 ) return "FINAL";
	if ( left == 4 ) return "SEMI-FINAL";
	if ( left == 8 ) return "QUARTER-FINAL";
	return "ROUND OF 16";
}

/* ---------------------------------------------------------------- public */

qboolean G_DragTourneyActive( void ) {
	return g_gametype.integer == GT_DRAG && g_dragTournament.integer && dt.loaded && dt.played < dt.size - 1;
}

/* dragTourney <size> <bestOf> <played> <a> <b> { "name" <human> }x size
 *             { <winner> <winsA> <winsB> }x played */
void G_DragTourneyPublish( int recipient ) {
	char text[MAX_STRING_CHARS];
	int i, a = -1, b = -1;
	if ( g_gametype.integer != GT_DRAG ) return;
	if ( !dt.loaded ) {
		if ( recipient >= 0 ) trap_SendServerCommand( recipient, "dragTourney 0" );
		return;
	}
	if ( dt.played < dt.size - 1 ) DT_MatchEntrants( dt.played, &a, &b );
	Com_sprintf( text, sizeof( text ), "dragTourney %d %d %d %d %d", dt.size, dt.bestOf, dt.played, a, b );
	for ( i = 0; i < dt.size; i++ ) {
		Q_strcat( text, sizeof( text ), va( " \"%s\" %d", dt.field[i].name, DT_IsBot( i ) ? 0 : 1 ) );
	}
	for ( i = 0; i < dt.played; i++ ) {
		Q_strcat( text, sizeof( text ), va( " %d %d %d", dt.results[i].winner, dt.results[i].winsA, dt.results[i].winsB ) );
	}
	trap_SendServerCommand( recipient, text );
}

/* A new map (not a restart of this one) starts over. */
void G_DragTourneyReset( void ) {
	DT_Clear();
}

/* Each map load. A finished tournament makes room for the next one. */
void G_DragTourneyInit( void ) {
	if ( g_gametype.integer != GT_DRAG || !g_dragTournament.integer ) {
		memset( &dt, 0, sizeof( dt ) );
		return;
	}
	if ( DT_Load() && dt.played >= dt.size - 1 ) {
		DT_Clear();
	}
	dt.lineupTime = 0;
	dt.lineupMatch = -1;
}

/* A new field: the humans here, filled up with bots. */
static qboolean DT_Build( void ) {
	char names[DT_MAX][DT_NAME];
	const char *bot;
	dtEntrant_t swap;
	int i, j, size, count = 0, base;
	size = g_dragTournamentSize.integer;
	size = size <= 4 ? 4 : ( size <= 8 ? 8 : 16 );
	memset( &dt, 0, sizeof( dt ) );
	dt.size = size;
	dt.bestOf = G_DragMatchBestOf();
	for ( i = 0; i < level.maxclients && count < size; i++ ) {
		if ( level.clients[i].pers.connected != CON_CONNECTED || ( g_entities[i].r.svFlags & SVF_BOT ) ) continue;
		DT_CleanName( level.clients[i].pers.netname, dt.field[count].name );
		dt.field[count].client = i;
		count++;
	}
	if ( !count ) return qfalse;
	base = (int)trap_Cvar_VariableValue( "g_spSkill" );
	if ( base < 1 || base > 5 ) base = 3;
	for ( i = 0; count < size && i < 64; i++ ) {
		bot = G_DragTourneyBotName( i );
		if ( !bot ) break;
		DT_CleanName( bot, names[0] );
		for ( j = 0; j < count; j++ ) {
			if ( !Q_stricmp( dt.field[j].name, names[0] ) ) break;
		}
		if ( j < count ) continue;
		Q_strncpyz( dt.field[count].name, names[0], DT_NAME );
		dt.field[count].client = -1;
		dt.field[count].skill = base + ( rand() % 3 ) - 1;
		if ( dt.field[count].skill < 1 ) dt.field[count].skill = 1;
		if ( dt.field[count].skill > 5 ) dt.field[count].skill = 5;
		count++;
	}
	if ( count < size ) {
		memset( &dt, 0, sizeof( dt ) );
		return qfalse;
	}
	for ( i = size - 1; i > 0; i-- ) {
		j = rand() % ( i + 1 );
		swap = dt.field[i];
		dt.field[i] = dt.field[j];
		dt.field[j] = swap;
	}
	dt.loaded = qtrue;
	dt.lineupMatch = -1;
	DT_Save();
	G_LogPrintf( "DragTourney: new field of %d, best of %d\n", dt.size, dt.bestOf );
	trap_SendServerCommand( -1, va( "print \"Drag tournament: %d drivers, best of %d.\n\"", dt.size, dt.bestOf ) );
	return qtrue;
}

/* Before a match can be armed: the two drivers of the pairing on the strip,
 * everyone else watching. Returns qtrue once the lineup stands. */
qboolean G_DragTourneyLineup( void ) {
	gclient_t *client;
	int a, b, ca, cb, i;
	qboolean ready, retry;

	if ( g_gametype.integer != GT_DRAG || !g_dragTournament.integer ) return qtrue;
	if ( !dt.loaded && !DT_Build() ) return qfalse;
	if ( dt.played >= dt.size - 1 ) return qfalse;
	DT_MatchEntrants( dt.played, &a, &b );
	ca = DT_ClientOf( a );
	cb = DT_ClientOf( b );
	/* A human who left the server forfeits; the bracket goes on. */
	if ( ( !DT_IsBot( a ) && ca < 0 ) || ( !DT_IsBot( b ) && cb < 0 ) ) {
		int need = dt.bestOf / 2 + 1, side;
		if ( !DT_IsBot( a ) && ca < 0 && !DT_IsBot( b ) && cb < 0 ) side = rand() & 1;
		else side = !DT_IsBot( a ) && ca < 0;
		trap_SendServerCommand( -1, va( "print \"%s left the tournament.\n\"",
			dt.field[side ? a : b].name ) );
		DT_Record( side, side ? 0 : need, side ? need : 0 );
		dt.lineupMatch = -1;
		if ( !G_DragTourneyActive() ) {
			LogExit( "Drag tournament: final forfeited." );
		}
		return qfalse;
	}
	retry = dt.lineupMatch != dt.played || level.time >= dt.lineupTime + DT_LINEUP_RETRY;

	for ( i = 0; i < level.maxclients; i++ ) {
		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) continue;
		if ( i == ca || i == cb ) {
			/* only once the lane is free, kicked bots leave a frame later */
			if ( client->sess.sessionTeam == TEAM_SPECTATOR && !( g_entities[i].r.svFlags & SVF_BOT )
				&& TeamCount( i, TEAM_FREE ) < 2 ) {
				SetTeam( &g_entities[i], "free" );
			}
			continue;
		}
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
			if ( retry ) trap_SendConsoleCommand( EXEC_APPEND, va( "clientkick %d\n", i ) );
		} else if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			SetTeam( &g_entities[i], "follow1" );
		}
	}
	if ( retry ) {
		if ( DT_IsBot( a ) && ca < 0 ) {
			trap_SendConsoleCommand( EXEC_APPEND, va( "addbot %s %d free\n", dt.field[a].name, dt.field[a].skill ) );
		}
		if ( DT_IsBot( b ) && cb < 0 ) {
			trap_SendConsoleCommand( EXEC_APPEND, va( "addbot %s %d free\n", dt.field[b].name, dt.field[b].skill ) );
		}
		if ( dt.lineupMatch != dt.played ) {
			trap_SendServerCommand( -1, va( "cp \"%s\n%s vs %s%s\"", DT_RoundName( DT_Round( dt.played, NULL, NULL ) ),
				dt.field[a].name, dt.field[b].name,
				DT_IsBot( a ) && DT_IsBot( b ) ? "\n\n/dragskip to skip, /dragskip all" : "" ) );
			G_DragTourneyPublish( -1 );
		}
		dt.lineupMatch = dt.played;
		dt.lineupTime = level.time;
	}
	ready = ca >= 0 && cb >= 0
		&& level.clients[ca].sess.sessionTeam == TEAM_FREE
		&& level.clients[cb].sess.sessionTeam == TEAM_FREE;
	return ready;
}

/* Fold a result into the bracket; the final crowns the champion. */
static void DT_Record( int winnerSide, int winsA, int winsB ) {
	int a, b, champion;
	if ( !G_DragTourneyActive() ) return;
	DT_MatchEntrants( dt.played, &a, &b );
	dt.results[dt.played].winner = winnerSide ? 1 : 0;
	dt.results[dt.played].winsA = winsA;
	dt.results[dt.played].winsB = winsB;
	dt.played++;
	G_LogPrintf( "DragTourney: match %d %s %d - %d %s\n", dt.played, dt.field[a].name, winsA, winsB, dt.field[b].name );
	if ( dt.played >= dt.size - 1 ) {
		champion = winnerSide ? b : a;
		G_LogPrintf( "DragTourney: champion %s\n", dt.field[champion].name );
		trap_SendServerCommand( -1, va( "print \"%s wins the Drag tournament!\n\"", dt.field[champion].name ) );
		if ( !DT_IsBot( champion ) && dt.field[champion].client >= 0
			&& level.clients[dt.field[champion].client].pers.localClient ) {
			G_Profile_AddScore( DT_CHAMPION_BONUS );
		}
	}
	DT_Save();
	G_DragTourneyPublish( -1 );
}

/* A finished match of g_drag.c: the winner's client and the heats won. */
void G_DragTourneyMatchOver( int winnerClient, const int *winsByClient ) {
	int a, b, ca, cb, winsA, winsB, side;
	if ( !G_DragTourneyActive() ) return;
	DT_MatchEntrants( dt.played, &a, &b );
	ca = DT_ClientOf( a );
	cb = DT_ClientOf( b );
	/* Not the bracket's pairing (skipped meanwhile), or a driver who left:
	 * the lineup settles that one on the next load. */
	if ( ca < 0 || cb < 0 ) return;
	winsA = winsByClient[ca];
	winsB = winsByClient[cb];
	if ( winnerClient >= 0 && winnerClient == ca ) side = 0;
	else if ( winnerClient >= 0 && winnerClient == cb ) side = 1;
	else if ( winsA != winsB ) side = winsB > winsA;
	else side = rand() & 1;		/* a knockout needs a winner: a drawn match goes to a coin */
	DT_Record( side, winsA, winsB );
}

/* Skip the bot match being watched, or all bot matches up to the next one
 * with a human, by drawing their results from the bots' skills. */
void Cmd_DragSkip_f( gentity_t *ent ) {
	char arg[16];
	int a, b, winsA, winsB, skipped = 0;
	qboolean all;
	if ( !G_DragTourneyActive() ) {
		trap_SendServerCommand( ent - g_entities, "print \"No Drag tournament is running.\n\"" );
		return;
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	all = !Q_stricmp( arg, "all" );
	while ( G_DragTourneyActive() ) {
		DT_MatchEntrants( dt.played, &a, &b );
		if ( !DT_IsBot( a ) || !DT_IsBot( b ) ) break;
		G_DragSimulateMatch( dt.field[a].skill, dt.field[b].skill, dt.bestOf, &winsA, &winsB );
		DT_Record( winsB > winsA, winsA, winsB );
		skipped++;
		if ( !all ) break;
	}
	if ( !skipped ) {
		trap_SendServerCommand( ent - g_entities, "print \"Only a match between two bots can be skipped.\n\"" );
		return;
	}
	if ( !level.intermissionQueued && !level.intermissiontime ) {
		LogExit( "Drag tournament: bot match skipped." );
	}
}

/* For g_drag.c: in a tournament, does this client race the current match? */
qboolean G_DragTourneyDriver( int clientNum ) {
	return !G_DragTourneyActive() || DT_EntrantOfClient( clientNum ) >= 0;
}
