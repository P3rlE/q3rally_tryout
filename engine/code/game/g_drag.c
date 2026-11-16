/*
 * Q3Rally v0.9 Drag Race prototype.
 * Equal cars, a neutral-to-first launch and a common finish plane.
 * Keep heat timing authoritative for the v0.9 Ladder report.
 */
#include "g_local.h"
#include "g_profile.h"

#define DRAG_HEAT_TIMEOUT_MS 60000
#define DRAG_FINISH_GRACE_MS 10000
#define DRAG_RESULT_HOLD_MS 3000
#define DRAG_NEXT_HEAT_MS 4000	/* result on screen before the next heat of a match */

typedef struct {
	dragPhase_t phase;
	qboolean spawned;
	qboolean participant;
	qboolean throttleReady;
	int launchTime;
	int finishTime;
	int trapSpeed;
	int nextNotice;
	vec3_t gridOrigin;
	float progress;
	float previousSpeed;
	ladderPlayerPayload_t ladderIdentity;
	int enterTime;
	/* Bot driver: a plan per heat, drawn from the bot's skill. */
	qboolean botPlanned;
	float botSkill;			/* 0 (skill 1) .. 1 (skill 5) */
	float botLaunchRpm;
	int botReactionMs;
	float botShiftRpm;
	int botLastShift;
} dragClient_t;

static dragClient_t dragClients[MAX_CLIENTS];
static int dragCountdownTime;
/* The scheduled green stays on the server. Clients learn the green time only
 * once it has happened: knowing it in advance would let a modified client
 * launch on the exact millisecond. */
static int dragScheduledGreenTime;
/* The server frame in which green actually happened; 0 before green. That
 * snapshot carries GO, so reaction times are measured from what drivers saw. */
static int dragGreenTime;
static int dragLastLight;
static int dragAllDoneTime;
static int dragFirstFinishTime;
static qboolean dragMapInvalid;
static qboolean dragResultAnnounced;
static vec3_t dragForward, dragRight, dragFinishOrigin;
static float dragLaneHalfWidth;

/*
 * Match: best of g_dragBestOf heats (1, 3, 5 or 7) between the same drivers,
 * without reloading the map. A duel ends when a driver has won the majority;
 * a heat without a winner (tie, both invalid) is run again, up to twice the
 * number of heats. Solo practice runs the number of heats. The Ladder gets
 * each driver's best heat; wins, losses and points count for the match.
 */
typedef struct {
	qboolean entered;		/* armed in a heat of this match */
	int wins;
	qboolean hasBest;		/* a valid finish in some heat */
	int bestRt, bestEt, bestTotal, bestTrap, bestFinish;
	dragPhase_t lastPhase;
	ladderPlayerPayload_t identity;	/* frozen at the driver's first heat */
	int enterTime;
} dragMatchDriver_t;

static dragMatchDriver_t dragMatch[MAX_CLIENTS];
static int dragMatchBestOf;
static int dragMatchHeats;		/* heats that counted */
static int dragMatchAttempts;	/* heats run, including repeated ones */
static int dragMatchWinner;
static qboolean dragMatchOver;
static qboolean dragHeatRecorded;
static int G_DragMatchEntrants( int *gone );

/* The result of one driver over the match so far, including the current heat
 * if it has not been recorded yet. */
typedef struct {
	int wins;
	qboolean hasBest;
	int rt, et, total, trap, finish;
	dragPhase_t phase;
} dragMatchResult_t;

extern void LogExit( const char *string );

/* Freeze identity at arming. A disconnected driver's slot can subsequently
 * belong to a spectator; that must not transfer a forfeit to the new owner. */
static void G_DragCaptureLadderIdentity( int clientNum ) {
	dragClient_t *driver = &dragClients[clientNum];
	gclient_t *client = g_entities[clientNum].client;
	ladderPlayerPayload_t *player = &driver->ladderIdentity;
	char userinfo[MAX_INFO_STRING];
	const char *value;

	memset( player, 0, sizeof( *player ) );
	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );
	player->clientNum = clientNum;
	player->team = TEAM_FREE;
	player->zoneActiveSigil = -1;
	player->ping = client->ps.ping;
	player->isBot = ( g_entities[clientNum].r.svFlags & SVF_BOT ) ? qtrue : qfalse;
	driver->enterTime = client->pers.enterTime;
	if ( client->pers.localClient ) {
		G_Profile_GetUUID( player->playerId, sizeof( player->playerId ) );
	}
	value = Info_ValueForKey( userinfo, "cl_uuid" );
	if ( !player->playerId[0] && value[0] ) Q_strncpyz( player->playerId, value, sizeof( player->playerId ) );
	value = Info_ValueForKey( userinfo, "cl_guid" );
	if ( value[0] ) {
		Q_strncpyz( player->guid, value, sizeof( player->guid ) );
		if ( !player->playerId[0] ) Q_strncpyz( player->playerId, value, sizeof( player->playerId ) );
	}
	value = Info_ValueForKey( userinfo, "ip" );
	if ( value[0] ) {
		if ( !player->guid[0] ) Q_strncpyz( player->guid, value, sizeof( player->guid ) );
		if ( !player->playerId[0] && !client->pers.localClient ) Q_strncpyz( player->playerId, value, sizeof( player->playerId ) );
	}
	value = Info_ValueForKey( userinfo, "name" );
	Q_strncpyz( player->name, value[0] ? value : client->pers.netname, sizeof( player->name ) );
	Q_strncpyz( player->cleanName, player->name, sizeof( player->cleanName ) );
	Q_CleanStr( player->cleanName );
	if ( !player->playerId[0] ) Q_strncpyz( player->playerId, player->cleanName, sizeof( player->playerId ) );
	if ( !player->playerId[0] ) Com_sprintf( player->playerId, sizeof( player->playerId ), "client-%i", clientNum );
	Q_strncpyz( player->model, Info_ValueForKey( userinfo, "model" ), sizeof( player->model ) );
	value = Info_ValueForKey( userinfo, "chassis" );
	if ( !value[0] ) value = Info_ValueForKey( userinfo, "vehicle" );
	Q_strncpyz( player->vehicle, value, sizeof( player->vehicle ) );
}

qboolean G_DragLadderParticipant( int clientNum ) {
	return g_gametype.integer == GT_DRAG && clientNum >= 0 && clientNum < MAX_CLIENTS
		&& dragMatch[clientNum].entered;
}

int G_DragMatchBestOf( void ) {
	return dragMatchBestOf;
}

static void G_DragMatchResultFor( int clientNum, dragMatchResult_t *out ) {
	const dragMatchDriver_t *entry = &dragMatch[clientNum];
	const dragClient_t *driver = &dragClients[clientNum];
	int total;
	memset( out, 0, sizeof( *out ) );
	out->wins = entry->wins;
	out->hasBest = entry->hasBest;
	out->rt = entry->bestRt;
	out->et = entry->bestEt;
	out->total = entry->bestTotal;
	out->trap = entry->bestTrap;
	out->finish = entry->bestFinish;
	out->phase = entry->lastPhase;
	if ( dragHeatRecorded || !driver->participant || !dragCountdownTime ) return;
	out->phase = driver->phase;
	if ( driver->phase == DRAG_FINISHED && dragGreenTime && driver->launchTime >= dragGreenTime
		&& driver->finishTime > driver->launchTime ) {
		total = driver->finishTime - dragGreenTime;
		if ( !out->hasBest || total < out->total ) {
			out->hasBest = qtrue;
			out->rt = driver->launchTime - dragGreenTime;
			out->et = driver->finishTime - driver->launchTime;
			out->total = total;
			out->trap = driver->trapSpeed;
			out->finish = driver->finishTime;
		}
	}
}

/* Match standing: more heat wins first, then the better best heat. */
static qboolean G_DragMatchAhead( const dragMatchResult_t *a, const dragMatchResult_t *b ) {
	if ( a->wins != b->wins ) return a->wins > b->wins;
	if ( a->hasBest != b->hasBest ) return a->hasBest;
	return a->hasBest && a->total < b->total;
}

qboolean G_DragLadderDuel( void ) {
	int i, count = 0;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( G_DragLadderParticipant( i ) ) count++;
	}
	return count == 2;
}

qboolean G_DragLadderIdentityMatches( int clientNum ) {
	char userinfo[MAX_INFO_STRING], identity[LADDER_MAX_PLAYER_ID];
	gclient_t *client;
	const char *value;
	if ( !G_DragLadderParticipant( clientNum ) ) return qfalse;
	client = g_entities[clientNum].client;
	if ( !client || client->pers.connected != CON_CONNECTED ) return qfalse;
	identity[0] = '\0';
	if ( client->pers.localClient ) G_Profile_GetUUID( identity, sizeof( identity ) );
	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );
	value = Info_ValueForKey( userinfo, "cl_uuid" );
	if ( !identity[0] && value[0] ) Q_strncpyz( identity, value, sizeof( identity ) );
	value = Info_ValueForKey( userinfo, "cl_guid" );
	if ( !identity[0] && value[0] ) Q_strncpyz( identity, value, sizeof( identity ) );
	value = Info_ValueForKey( userinfo, "ip" );
	if ( !identity[0] && !client->pers.localClient && value[0] ) Q_strncpyz( identity, value, sizeof( identity ) );
	if ( !identity[0] ) {
		value = Info_ValueForKey( userinfo, "name" );
		Q_strncpyz( identity, value[0] ? value : client->pers.netname, sizeof( identity ) );
		Q_CleanStr( identity );
	}
	if ( !identity[0] ) Com_sprintf( identity, sizeof( identity ), "client-%i", clientNum );
	return !strcmp( identity, dragMatch[clientNum].identity.playerId );
}

/* The Ladder gets the driver's best heat of the match; the position is the
 * match standing (heat wins, then best heat). */
qboolean G_DragPopulateLadderPlayer( int clientNum, ladderPlayerPayload_t *player ) {
	const dragMatchDriver_t *entry;
	dragMatchResult_t mine, other;
	int i;
	if ( !player || !G_DragLadderParticipant( clientNum ) ) return qfalse;
	entry = &dragMatch[clientNum];
	G_DragMatchResultFor( clientNum, &mine );
	*player = entry->identity;
	player->time = level.time > entry->enterTime ? level.time - entry->enterTime : 0;
	player->dragHeatWins = mine.wins;
	Q_strncpyz( player->dragStatus, mine.phase == DRAG_FALSE_START ? "false_start" : "dnf", sizeof( player->dragStatus ) );
	if ( mine.hasBest ) {
		Q_strncpyz( player->dragStatus, "finished", sizeof( player->dragStatus ) );
		player->reactionMs = mine.rt;
		player->elapsedMs = mine.et;
		player->totalRaceMs = mine.total;
		player->finishRaceTime = mine.finish;
		player->trapSpeedKph = mine.trap * 0.1f;
		player->position = 1;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( i == clientNum || !G_DragLadderParticipant( i ) ) continue;
			G_DragMatchResultFor( i, &other );
			if ( G_DragMatchAhead( &other, &mine ) ) player->position++;
		}
	}
	return qtrue;
}

static qboolean G_DragActive( int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	return ent->inuse && ent->client
		&& ent->client->pers.connected == CON_CONNECTED
		&& ent->client->sess.sessionTeam != TEAM_SPECTATOR;
}

static qboolean G_DragTerminal( const dragClient_t *driver ) {
	return driver->phase == DRAG_FINISHED || driver->phase == DRAG_FALSE_START
		|| driver->phase == DRAG_DNF;
}

static void G_DragPublish( int clientNum, int recipient ) {
	const dragClient_t *driver = &dragClients[clientNum];
	/* The countdown start identifies the heat and drives the red lamps.
	 * dragGreenTime stays 0 until green has happened. */
	trap_SendServerCommand( recipient, va( "dragStatus %d %d %d %d %d %d %d",
		clientNum, driver->phase, dragCountdownTime, dragGreenTime,
		driver->launchTime, driver->finishTime, driver->trapSpeed ) );
}

static void G_DragSetPhase( int clientNum, dragPhase_t phase ) {
	dragClients[clientNum].phase = phase;
	if ( phase == DRAG_COUNTDOWN && g_entities[clientNum].client ) {
		/* The heat is armed: from now on the engine can be over-revved. */
		g_entities[clientNum].client->ps.stats[STAT_DRAG_ENGINE] = DRAG_ENGINE_ARMED;
	}
	if ( phase == DRAG_RUNNING && g_entities[clientNum].client
		&& g_entities[clientNum].client->ps.commandTime < dragClients[clientNum].launchTime ) {
		/* A usercommand describes input at its timestamp. Do not integrate
		 * its new throttle backwards across the interval spent staged. */
		g_entities[clientNum].client->ps.commandTime = dragClients[clientNum].launchTime;
	}
	if ( phase == DRAG_RUNNING && g_entities[clientNum].client ) {
		/* Keep the free engine RPM: engaging first transfers the launch load
		 * through the shared drivetrain simulation. */
		g_entities[clientNum].client->car.gear = 1;
		g_entities[clientNum].client->ps.stats[STAT_GEAR] = 1;
	}
	G_DragPublish( clientNum, -1 );
}

static void G_DragDisqualify( int clientNum, dragPhase_t phase, const char *reason ) {
	gentity_t *ent = &g_entities[clientNum];
	dragClient_t *driver = &dragClients[clientNum];
	if ( G_DragTerminal( driver ) ) {
		return;
	}
	driver->participant = qtrue;
	G_LogPrintf( "DragDisqualify: %i %s\n", clientNum, reason );
	G_DragSetPhase( clientNum, phase );
	if ( ent->client ) {
		/* A non-zero finish marker also activates the existing observer delay.
		 * Comparator and HUD use phase, so this never becomes a valid time. */
		ent->client->finishRaceTime = level.time > 0 ? level.time : 1;
		ent->takedamage = qfalse;
		trap_SendServerCommand( clientNum, va( "cp \"%s\"", reason ) );
	}
}

void G_DragInit( void ) {
	memset( dragClients, 0, sizeof( dragClients ) );
	dragCountdownTime = dragScheduledGreenTime = dragGreenTime = 0;
	dragAllDoneTime = dragFirstFinishTime = 0;
	dragLastLight = -1;
	dragMapInvalid = dragResultAnnounced = qfalse;
	VectorClear( dragForward );
	VectorClear( dragRight );
	VectorClear( dragFinishOrigin );
	dragLaneHalfWidth = 0.0f;
	memset( dragMatch, 0, sizeof( dragMatch ) );
	dragMatchBestOf = g_dragBestOf.integer;
	if ( dragMatchBestOf < 1 ) dragMatchBestOf = 1;
	if ( dragMatchBestOf > 7 ) dragMatchBestOf = 7;
	if ( !( dragMatchBestOf & 1 ) ) dragMatchBestOf++;
	dragMatchHeats = dragMatchAttempts = 0;
	dragMatchWinner = -1;
	dragMatchOver = dragHeatRecorded = qfalse;
	if ( g_gametype.integer == GT_DRAG ) {
		level.winnerNumber = -1;
	}
	G_DragTourneyInit();
}

/* New drivers join before the first heat of a match, or once it is over. */
qboolean G_DragCanJoin( void ) {
	return g_gametype.integer != GT_DRAG || dragMatchOver
		|| ( !dragCountdownTime && !dragMatchAttempts );
}

/* dragMatch <bestOf> <heat> <over> <winner> <count> { <client> <wins> } */
static void G_DragPublishMatch( int recipient ) {
	char text[256];
	int i, count = 0;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( dragMatch[i].entered ) count++;
	}
	Com_sprintf( text, sizeof( text ), "dragMatch %d %d %d %d %d", dragMatchBestOf,
		dragMatchOver ? dragMatchHeats : dragMatchHeats + 1, dragMatchOver ? 1 : 0, dragMatchWinner, count );
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( dragMatch[i].entered ) {
			Q_strcat( text, sizeof( text ), va( " %d %d", i, dragMatch[i].wins ) );
		}
	}
	trap_SendServerCommand( recipient, text );
}

void G_DragClientSpawn( gentity_t *ent ) {
	int i, clientNum;
	dragClient_t *driver;
	if ( g_gametype.integer != GT_DRAG || !ent || !ent->client ) {
		return;
	}
	clientNum = ent->s.number;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return;
	}
	driver = &dragClients[clientNum];
	if ( dragCountdownTime && driver->participant
		&& ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		/* SetTeam also reaches ClientSpawn. Retain the timed participant:
		 * leaving the driving field is a forfeit, not a way to erase a heat. */
		G_DragClientDisconnect( clientNum );
	} else if ( dragCountdownTime && !dragMatchOver && ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		/* A respawn/reset must not create a new timed attempt in this heat. */
		G_DragDisqualify( clientNum, DRAG_DNF, "DNF - no resets during a Drag heat" );
	} else {
		memset( driver, 0, sizeof( *driver ) );
		driver->spawned = qtrue;
		VectorCopy( ent->client->ps.origin, driver->gridOrigin );
		ent->client->car.gear = 0;
		ent->client->ps.stats[STAT_GEAR] = 0;
		ent->client->ps.stats[STAT_DRAG_ENGINE] = 0;
		ent->ready = qfalse;
		G_DragPublish( clientNum, -1 );
	}
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( dragClients[i].spawned || dragClients[i].participant ) {
			G_DragPublish( i, clientNum );
		}
	}
	G_DragPublishMatch( clientNum );
	G_DragTourneyPublish( clientNum );
	/* Late spectators also need the current red stage for the countdown
	 * text; the lamps themselves follow the published countdown start. */
	if ( dragCountdownTime && !level.startRaceTime
		&& dragLastLight >= 1 && dragLastLight <= 3 ) {
		trap_SendServerCommand( clientNum, va( "rc \"%d - HOLD RPM IN N\" %d",
			dragLastLight, dragLastLight ) );
	} else if ( dragGreenTime > 0 && level.time >= dragGreenTime
		&& level.time < dragGreenTime + 4000 ) {
		trap_SendServerCommand( clientNum, "rc \"GO!\" 0" );
	}
}

void G_DragClientDisconnect( int clientNum ) {
	int i;
	if ( g_gametype.integer != GT_DRAG || clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		return;
	}
	if ( dragCountdownTime && !level.startRaceTime ) {
		/* Cancel the armed start so a disconnect never gives the remaining
		 * driver an unexpected green or an automatic walkover. */
		dragCountdownTime = dragScheduledGreenTime = dragGreenTime = 0;
		dragLastLight = -1;
		level.raceState = RACE_STATE_NONE;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			dragClients[i].participant = qfalse;
			dragClients[i].launchTime = 0;
			dragClients[i].finishTime = 0;
			if ( G_DragActive( i ) && i != clientNum ) {
				dragClients[i].phase = dragClients[i].throttleReady ? DRAG_STAGED : DRAG_WAITING;
				g_entities[i].client->car.gear = 0;
				g_entities[i].client->ps.stats[STAT_GEAR] = 0;
				g_entities[i].client->ps.stats[STAT_DRAG_ENGINE] = 0;
				g_entities[i].client->finishRaceTime = 0;
				G_DragPublish( i, -1 );
			}
		}
		trap_SendServerCommand( -1, "rc \"START CANCELLED - RESTAGE\" 0" );
		if ( !dragMatchAttempts ) {
			/* The match had not begun: nobody has entered it yet. */
			memset( dragMatch, 0, sizeof( dragMatch ) );
		}
		memset( &dragClients[clientNum], 0, sizeof( dragClients[clientNum] ) );
		G_DragPublish( clientNum, -1 );
		return;
	}
	if ( dragClients[clientNum].participant && !G_DragTerminal( &dragClients[clientNum] ) ) {
		G_DragDisqualify( clientNum, DRAG_DNF, "DNF - driver disconnected" );
	} else if ( !dragCountdownTime ) {
		memset( &dragClients[clientNum], 0, sizeof( dragClients[clientNum] ) );
		G_DragPublish( clientNum, -1 );
	}
}

static qboolean G_DragConfigureTrack( void ) {
	vec3_t startOrigin;
	float width;
	int i;
	if ( !level.startEnt || !level.finishEnt || level.startEnt == level.finishEnt ) {
		return qfalse;
	}
	for ( i = 0; i < 3; i++ ) {
		startOrigin[i] = ( level.startEnt->r.absmin[i] + level.startEnt->r.absmax[i] ) * 0.5f;
		dragFinishOrigin[i] = ( level.finishEnt->r.absmin[i] + level.finishEnt->r.absmax[i] ) * 0.5f;
	}
	VectorSubtract( dragFinishOrigin, startOrigin, dragForward );
	dragForward[2] = 0.0f;
	if ( VectorNormalize( dragForward ) < CAR_LENGTH * 4.0f ) {
		return qfalse;
	}
	dragRight[0] = -dragForward[1];
	dragRight[1] = dragForward[0];
	dragRight[2] = 0.0f;
	width = fabs( dragRight[0] ) * ( level.finishEnt->r.absmax[0] - level.finishEnt->r.absmin[0] )
		+ fabs( dragRight[1] ) * ( level.finishEnt->r.absmax[1] - level.finishEnt->r.absmin[1] );
	dragLaneHalfWidth = width * 0.25f;
	return dragLaneHalfWidth > CAR_WIDTH;
}

/* Call only from the server frame (starter think and G_DragRunFrame), never
 * while processing a client command between frames: the snapshot of this
 * frame is the first one that shows green. */
static void G_DragStartGreen( void ) {
	int i;
	if ( !dragScheduledGreenTime || level.startRaceTime || level.time < dragScheduledGreenTime ) {
		return;
	}
	dragGreenTime = level.time;
	level.startRaceTime = dragGreenTime;
	level.raceState = RACE_STATE_RUNNING;
	level.raceIntroEndTime = 0;
	trap_SendServerCommand( -1, va( "raceTime %d", dragGreenTime ) );
	trap_SendServerCommand( -1, "rc \"GO!\" 0" );
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !dragClients[i].participant || !G_DragActive( i ) ) {
			continue;
		}
		g_entities[i].client->lapStartTime = dragGreenTime;
		g_entities[i].client->lastCheckpointTime = dragGreenTime;
		/* Every request before this frame was a false start, so no driver is
		 * launched here; publish the green time with each driver's state. */
		G_DragPublish( i, -1 );
	}
}

void G_DragStarterThink( gentity_t *starter ) {
	int i, count, light;
	qboolean staged;
	if ( g_gametype.integer != GT_DRAG ) {
		return;
	}
	starter->nextthink = level.time + 50;
	if ( level.intermissiontime || level.intermissionQueued || dragMapInvalid ) {
		return;
	}
	if ( !G_DragConfigureTrack() ) {
		dragMapInvalid = qtrue;
		trap_SendServerCommand( -1,
			"cp \"Drag Race needs separate start/finish gates and two equal lanes.\"" );
		return;
	}
	if ( !dragCountdownTime ) {
		/* Tournament: the pairing's two drivers on the strip first. */
		if ( !G_DragTourneyLineup() ) {
			return;
		}
		count = 0;
		staged = qtrue;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( !G_DragActive( i ) ) {
				continue;
			}
			count++;
			if ( !dragClients[i].spawned || !dragClients[i].throttleReady ||
				g_entities[i].client->car.gear != 0 ) {
				staged = qfalse;
			}
		}
		if ( !count || !staged || count > 2 ) {
			return;
		}
		/* No new heat once the match is decided or a driver of it has left. */
		if ( dragMatchOver || ( dragMatchAttempts && G_DragMatchEntrants( &i ) && i >= 0 ) ) {
			return;
		}
		dragCountdownTime = level.time;
		/* Vary only the final wait, and keep it secret until green. */
		dragScheduledGreenTime = level.time + 3000 + ( rand() % 1001 );
		dragGreenTime = 0;
		level.raceState = RACE_STATE_COUNTDOWN;
		level.raceIntroEndTime = 0;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( G_DragActive( i ) ) {
				dragClients[i].participant = qtrue;
				G_DragCaptureLadderIdentity( i );
				if ( !dragMatch[i].entered ) {
					dragMatch[i].entered = qtrue;
					dragMatch[i].identity = dragClients[i].ladderIdentity;
					dragMatch[i].enterTime = dragClients[i].enterTime;
				}
				dragClients[i].launchTime = 0;
				G_DragSetPhase( i, DRAG_COUNTDOWN );
			}
		}
	}
	G_DragStartGreen();
	if ( !level.startRaceTime ) {
		light = 3 - ( level.time - dragCountdownTime ) / 1000;
		if ( light < 1 ) light = 1;
		if ( light != dragLastLight ) {
			dragLastLight = light;
			trap_SendServerCommand( -1, va( "rc \"%d - HOLD RPM IN N\" %d", light, light ) );
		}
	}
}

void G_DragFilterCommand( gentity_t *ent, usercmd_t *cmd ) {
	dragClient_t *driver;
	qboolean throttleReady;
	int clientNum;
	if ( g_gametype.integer != GT_DRAG || !ent || !ent->client || !cmd ) {
		return;
	}
	clientNum = ent->s.number;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) return;
	driver = &dragClients[clientNum];
	ent->client->pers.transmissionMode = TR_MANUAL;
	ent->client->pers.manualShift = qtrue;
	ent->client->ps.stats[STAT_HOLDABLE_ITEM] = 0;
	ent->client->ps.stats[STAT_WEAPONS] = 0;
	ent->client->ps.stats[STAT_SLIPSTREAM] = 0;
	memset( ent->client->ps.powerups, 0, sizeof( ent->client->ps.powerups ) );
	ent->client->ps.weapon = ent->s.weapon = WP_NONE;
	ent->takedamage = qfalse;
	cmd->buttons &= ~( BUTTON_ATTACK | BUTTON_USE_HOLDABLE );
	cmd->upmove = 0;
	cmd->weapon = WP_NONE;
	if ( !driver->spawned ) {
		G_DragClientSpawn( ent );
	}
	if ( !dragCountdownTime ) {
		/* Neutral keeps the car on the grid while the engine responds to gas.
		 * A positive pedal input is the driver's explicit staging action. */
		ent->client->car.gear = 0;
		ent->client->ps.stats[STAT_GEAR] = 0;
		throttleReady = cmd->forwardmove > 0 ? qtrue : qfalse;
		if ( driver->throttleReady != throttleReady ) {
			G_DragSetPhase( clientNum, throttleReady ? DRAG_STAGED : DRAG_WAITING );
		}
		driver->throttleReady = throttleReady;
		ent->ready = throttleReady;
		if ( level.time >= driver->nextNotice ) {
			trap_SendServerCommand( clientNum,
				"cp \"DRAG RACE: apply gas in N to stage.\nHold your launch RPM, select FIRST on GREEN.\nManual gears: gearUp / gearDown\"" );
			driver->nextNotice = level.time + 4000;
		}
	} else {
		driver->throttleReady = !G_DragTerminal( driver ) && cmd->forwardmove > 0 ? qtrue : qfalse;
	}
	if ( !driver->participant || driver->phase != DRAG_RUNNING || !level.startRaceTime ) {
		cmd->buttons = BUTTON_HANDBRAKE;
		cmd->rightmove = cmd->upmove = 0;
		ent->client->car.gear = 0;
		ent->client->ps.stats[STAT_GEAR] = 0;
		/* Free revving stays available before launch; invalid/finished runs
		 * are stopped instead of continuing to rev in the result screen. */
		if ( G_DragTerminal( driver ) || cmd->forwardmove < 0 ) {
			cmd->forwardmove = 0;
		}
	}
}

qboolean G_DragTryGearChange( gentity_t *ent, int direction, int commandTime ) {
	dragClient_t *driver;
	int clientNum, reactionMs;
	char reason[96];
	if ( g_gametype.integer != GT_DRAG ) return qfalse;
	if ( !ent || !ent->client ) return qtrue;
	clientNum = ent->s.number;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || !G_DragActive( clientNum ) ) return qtrue;
	driver = &dragClients[clientNum];
	if ( driver->phase == DRAG_RUNNING ) return qfalse;
	if ( G_DragTerminal( driver ) || direction <= 0 || driver->launchTime ) return qtrue;
	if ( !dragCountdownTime || !driver->participant ) {
		trap_SendServerCommand( clientNum, "cp \"Apply gas in N to stage, then select FIRST on GREEN.\"" );
		return qtrue;
	}
	/* Use the same accepted input-time bounds as movement. Keeping an early
	 * timestamp is necessary: delayed delivery must not turn a false start
	 * into a legal launch. The first gear request is final. */
	if ( commandTime <= 0 ) commandTime = ent->client->pers.cmd.serverTime;
	if ( commandTime < level.time - 1000 ) commandTime = level.time - 1000;
	if ( commandTime > level.time + 200 ) commandTime = level.time + 200;
	/* The client stamps gearUp with the time of the frame it was pressed in,
	 * and sends it together with that frame's movement. It can therefore never
	 * be older than the movement already received: an older stamp is a
	 * backdated launch, and is moved up to the last accepted movement. */
	if ( commandTime < ent->client->pers.cmd.serverTime && !( ent->r.svFlags & SVF_BOT ) ) {
		commandTime = ent->client->pers.cmd.serverTime;
	}
	driver->launchTime = commandTime;
	/* Before the green frame, no request is legal, whatever its timestamp. */
	if ( !level.startRaceTime || !dragGreenTime || commandTime < dragGreenTime ) {
		G_DragDisqualify( clientNum, DRAG_FALSE_START, "FALSE START - selected FIRST before GREEN" );
		return qtrue;
	}
	reactionMs = commandTime - dragGreenTime;
	if ( reactionMs < DRAG_MIN_REACTION_MS ) {
		/* Not va(): G_DragDisqualify formats with va() itself. */
		Com_sprintf( reason, sizeof( reason ), "FALSE START - reaction %d.%03d s, minimum %d.%03d s",
			reactionMs / 1000, reactionMs % 1000, DRAG_MIN_REACTION_MS / 1000, DRAG_MIN_REACTION_MS % 1000 );
		G_DragDisqualify( clientNum, DRAG_FALSE_START, reason );
		return qtrue;
	}
	if ( ent->r.svFlags & SVF_BOT ) {
		/* The server drives bots between frames at their exact reaction
		 * time. Until then the car stood in N, so integrating first gear
		 * from that moment is the same as having launched then. */
		ent->client->ps.commandTime = commandTime;
	}
	if ( driver->phase != DRAG_RUNNING ) {
		G_DragSetPhase( clientNum, DRAG_RUNNING );
	}
	return qtrue;
}

void G_DragPostMove( gentity_t *ent, const vec3_t oldOrigin, int oldTime ) {
	dragClient_t *driver;
	vec3_t delta, oldDelta, crossing;
	float lateral, before, after, fraction, speed;
	int i, timestamp, clientNum;
	if ( g_gametype.integer != GT_DRAG || !ent || !ent->client || !level.startRaceTime ) {
		return;
	}
	clientNum = ent->s.number;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) return;
	driver = &dragClients[clientNum];
	if ( driver->phase != DRAG_RUNNING ) return;
	VectorSubtract( ent->client->ps.origin, driver->gridOrigin, delta );
	driver->progress = DotProduct( delta, dragForward );
	lateral = fabs( DotProduct( delta, dragRight ) );
	if ( lateral > dragLaneHalfWidth - CAR_WIDTH * 0.5f ) {
		G_DragDisqualify( clientNum, DRAG_DNF, "DNF - left your lane" );
		return;
	}
	VectorSubtract( oldOrigin, dragFinishOrigin, oldDelta );
	VectorSubtract( ent->client->ps.origin, dragFinishOrigin, delta );
	before = DotProduct( oldDelta, dragForward );
	after = DotProduct( delta, dragForward );
	speed = VectorLength( ent->client->ps.velocity );
	if ( before < 0.0f && after >= 0.0f && after > before
		&& ent->client->ps.commandTime > oldTime ) {
		fraction = -before / ( after - before );
		for ( i = 0; i < 3; i++ ) {
			crossing[i] = oldOrigin[i] + fraction * ( ent->client->ps.origin[i] - oldOrigin[i] );
			if ( crossing[i] < level.finishEnt->r.absmin[i] - 0.1f
				|| crossing[i] > level.finishEnt->r.absmax[i] + 0.1f ) {
				G_DragDisqualify( clientNum, DRAG_DNF, "DNF - missed the finish gate" );
				return;
			}
		}
		if ( ent->number != level.finishEnt->number ) {
			G_DragDisqualify( clientNum, DRAG_DNF, "DNF - missed a checkpoint" );
			return;
		}
		timestamp = oldTime + (int)( fraction * ( ent->client->ps.commandTime - oldTime ) + 0.5f );
		if ( timestamp < driver->launchTime ) timestamp = driver->launchTime;
		driver->finishTime = timestamp;
		driver->trapSpeed = (int)( ( driver->previousSpeed + fraction * ( speed - driver->previousSpeed ) )
			* 36.0f / CP_M_2_QU + 0.5f );
		ent->client->finishRaceTime = timestamp;
		ent->client->bestLapMs = timestamp - level.startRaceTime;
		ent->client->lastCheckpointTime = timestamp;
		G_DragSetPhase( clientNum, DRAG_FINISHED );
		trap_SendServerCommand( -1, va( "raceFinishTime %d %d", clientNum, timestamp ) );
	}
	driver->previousSpeed = speed;
}

int G_DragCompare( int clientA, int clientB ) {
	const dragClient_t *a, *b;
	qboolean invalidA, invalidB;
	if ( clientA < 0 || clientA >= MAX_CLIENTS || clientB < 0 || clientB >= MAX_CLIENTS ) return 0;
	a = &dragClients[clientA];
	b = &dragClients[clientB];
	invalidA = a->phase == DRAG_FALSE_START || a->phase == DRAG_DNF;
	invalidB = b->phase == DRAG_FALSE_START || b->phase == DRAG_DNF;
	if ( invalidA != invalidB ) return invalidA ? 1 : -1;
	if ( invalidA ) return 0;
	if ( a->phase == DRAG_FINISHED && b->phase == DRAG_FINISHED ) {
		if ( a->finishTime == b->finishTime ) return 0;
		return a->finishTime < b->finishTime ? -1 : 1;
	}
	if ( a->phase == DRAG_FINISHED ) return -1;
	if ( b->phase == DRAG_FINISHED ) return 1;
	if ( a->progress == b->progress ) return 0;
	return a->progress > b->progress ? -1 : 1;
}

/*
 * Bots. A bot draws a plan per heat from its skill: the revs it holds in N,
 * its reaction time and its shift points. The server then drives it on the
 * frame: launch at green + reaction (a rookie now and then jumps the light,
 * which is a false start under the minimum reaction rule), shift at the
 * planned revs, and steer along the own lane. Skill 1 bogs or smokes its
 * launches and reacts in about 0.36 s; skill 5 launches in the band and
 * reacts in about 0.16 s.
 */
#define DRAG_BOT_AIM_AHEAD		1600.0f
#define DRAG_BOT_SHIFT_SETTLE	250		/* ms after a gear change before the next */

static float G_DragGauss( void ) {
	/* sum of four uniforms: close enough to a unit normal for driver error */
	return ( random() + random() + random() + random() - 2.0f ) * 1.732f;
}

static float G_DragClamp( float value, float low, float high ) {
	return value < low ? low : ( value > high ? high : value );
}

static void G_DragBotRollShift( dragClient_t *driver ) {
	driver->botShiftRpm = G_DragClamp( 6020.0f + G_DragGauss() * ( 40.0f + 300.0f * ( 1.0f - driver->botSkill ) ),
		5300.0f, 6200.0f );
}

/* A heat plan from skill (0 rookie .. 1 best): launch revs and reaction. */
static void G_DragBotPlanFor( float skill, float *launchRpm, int *reactionMs ) {
	float reaction;
	/* Even a rookie stays out of the red zone while it waits for green. */
	*launchRpm = G_DragClamp( 4300.0f + G_DragGauss() * ( 150.0f + 1100.0f * ( 1.0f - skill ) ),
		1500.0f, DRAG_ENGINE_REDLINE_RPM - 50.0f );
	if ( random() < 0.04f * ( 1.0f - skill ) ) {
		reaction = 40.0f + random() * 55.0f;		/* anticipated the green */
	} else {
		reaction = 360.0f - 200.0f * skill + G_DragGauss() * ( 25.0f + 55.0f * ( 1.0f - skill ) );
		if ( reaction < 120.0f ) reaction = 120.0f + random() * 20.0f;
	}
	*reactionMs = (int)reaction;
}

static void G_DragBotPlan( int clientNum ) {
	dragClient_t *driver = &dragClients[clientNum];
	char userinfo[MAX_INFO_STRING];
	float skill;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );
	skill = atof( Info_ValueForKey( userinfo, "skill" ) );
	driver->botSkill = G_DragClamp( ( skill - 1.0f ) / 4.0f, 0.0f, 1.0f );
	G_DragBotPlanFor( driver->botSkill, &driver->botLaunchRpm, &driver->botReactionMs );
	G_DragBotRollShift( driver );
	driver->botPlanned = qtrue;
}

/* Time lost by the launch revs, from the calibration (docs/drag-launch-rpm-calibration.md). */
static float G_DragLaunchLoss( float rpm ) {
	static const float rpms[] = { 1000, 2000, 3000, 3500, 4000, 4500, 5000, 5500, 6000 };
	static const float loss[] = { 0.61f, 0.36f, 0.14f, 0.03f, 0.0f, 0.01f, 0.03f, 0.07f, 0.12f };
	int i;
	if ( rpm <= rpms[0] ) return loss[0];
	for ( i = 1; i < (int)( sizeof( rpms ) / sizeof( rpms[0] ) ); i++ ) {
		if ( rpm <= rpms[i] ) {
			return loss[i - 1] + ( loss[i] - loss[i - 1] ) * ( rpm - rpms[i - 1] ) / ( rpms[i] - rpms[i - 1] );
		}
	}
	return loss[sizeof( loss ) / sizeof( loss[0] ) - 1];
}

/* One heat between two bots without driving it: 0 or 1 wins, -1 nobody. */
static int G_DragSimulateHeat( int skillA, int skillB ) {
	float total[2], skill, rpm;
	int i, reaction;
	qboolean valid[2];
	for ( i = 0; i < 2; i++ ) {
		skill = G_DragClamp( ( ( i ? skillB : skillA ) - 1.0f ) / 4.0f, 0.0f, 1.0f );
		G_DragBotPlanFor( skill, &rpm, &reaction );
		valid[i] = reaction >= DRAG_MIN_REACTION_MS;
		/* shift points add a little spread, more for rookies */
		total[i] = reaction / 1000.0f + G_DragLaunchLoss( rpm ) + G_DragGauss() * ( 0.01f + 0.03f * ( 1.0f - skill ) );
	}
	if ( !valid[0] && !valid[1] ) return -1;
	if ( valid[0] != valid[1] ) return valid[0] ? 0 : 1;
	return total[1] < total[0];
}

/* A whole best-of match between two bots; a knockout always has a winner. */
void G_DragSimulateMatch( int skillA, int skillB, int bestOf, int *winsA, int *winsB ) {
	int need, attempts = 0, heat;
	if ( bestOf < 1 ) bestOf = 1;
	need = bestOf / 2 + 1;
	*winsA = *winsB = 0;
	while ( *winsA < need && *winsB < need && attempts < 2 * bestOf ) {
		heat = G_DragSimulateHeat( skillA, skillB );
		if ( heat == 0 ) ( *winsA )++;
		else if ( heat == 1 ) ( *winsB )++;
		attempts++;
	}
	if ( *winsA == *winsB ) {
		if ( random() < 0.5f ) ( *winsA )++; else ( *winsB )++;
	}
}

/* Called for every bot usercmd (ai_main.c): throttle and steering. */
qboolean G_DragBotInput( int clientNum, usercmd_t *cmd ) {
	gentity_t *ent;
	dragClient_t *driver;
	vec3_t aim, dir, angles;
	float yaw, steering;
	int i;

	if ( g_gametype.integer != GT_DRAG || clientNum < 0 || clientNum >= MAX_CLIENTS || !cmd ) return qfalse;
	ent = &g_entities[clientNum];
	if ( !ent->client || !( ent->r.svFlags & SVF_BOT ) ) return qfalse;
	driver = &dragClients[clientNum];
	if ( !driver->botPlanned ) G_DragBotPlan( clientNum );

	cmd->buttons = 0;
	cmd->upmove = cmd->rightmove = 0;
	cmd->weapon = WP_NONE;
	if ( G_DragTerminal( driver ) ) {
		cmd->forwardmove = 0;
	} else if ( driver->phase == DRAG_RUNNING ) {
		cmd->forwardmove = 127;
	} else {
		/* In N the pedal position sets the revs: hold the planned ones. */
		i = (int)( 127.0f * ( driver->botLaunchRpm - CP_RPM_MIN ) / ( CP_RPM_MAX - CP_RPM_MIN ) + 0.5f );
		cmd->forwardmove = (signed char)( i < 1 ? 1 : ( i > 127 ? 127 : i ) );
	}

	/* Steer at a point on the own lane's centre line ahead. */
	if ( dragForward[0] == 0.0f && dragForward[1] == 0.0f ) return qtrue;
	VectorMA( driver->gridOrigin, driver->progress + DRAG_BOT_AIM_AHEAD, dragForward, aim );
	VectorSubtract( aim, ent->client->ps.origin, dir );
	yaw = vectoyaw( dir );
	if ( ent->client->pers.controlMode == CT_MOUSE ) {
		VectorSet( angles, 0, yaw, 0 );
		for ( i = 0; i < 3; i++ ) cmd->angles[i] = ANGLE2SHORT( angles[i] ) - ent->client->ps.delta_angles[i];
	} else {
		steering = WheelAngle( ent->client->ps.viewangles[YAW], yaw );
		cmd->rightmove = (signed char)( 127.0f * G_DragClamp( steering / 20.0f, -1.0f, 1.0f ) );
	}
	return qtrue;
}

/* Every server frame once green has happened: launch and shift on time. */
static void G_DragBotFrame( int clientNum ) {
	dragClient_t *driver = &dragClients[clientNum];
	gentity_t *ent = &g_entities[clientNum];
	car_t *car = &ent->client->car;

	if ( !driver->botPlanned ) G_DragBotPlan( clientNum );
	if ( driver->phase == DRAG_COUNTDOWN && dragGreenTime > 0
		&& level.time >= dragGreenTime + driver->botReactionMs ) {
		G_DragTryGearChange( ent, 1, dragGreenTime + driver->botReactionMs );
		driver->botLastShift = level.time;
		return;
	}
	if ( driver->phase == DRAG_RUNNING && car->gear >= 1 && car->gear < 6
		&& level.time >= driver->botLastShift + DRAG_BOT_SHIFT_SETTLE && car->rpm >= driver->botShiftRpm ) {
		car->gear++;
		driver->botLastShift = level.time;
		G_DragBotRollShift( driver );
	}
}

/* An engine blown by pmove ends the attempt: smoke, a bang, and a DNF. */
static void G_DragCheckEngine( int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	int state;
	if ( !ent->client || G_DragTerminal( &dragClients[clientNum] ) ) return;
	state = ent->client->ps.stats[STAT_DRAG_ENGINE];
	if ( !( state & DRAG_ENGINE_BLOWN ) ) return;
	ent->client->ps.eFlags |= EF_SMOKE_DARK;
	G_Sound( ent, CHAN_AUTO, G_SoundIndex( "sound/feedback/drag_engine_blow.wav" ) );
	G_DragDisqualify( clientNum, DRAG_DNF, ( state & DRAG_ENGINE_OVERREV )
		? "ENGINE BLOWN - over-revved, shift up in time"
		: "ENGINE BLOWN - too long on the limiter" );
}

/* Every armed driver who left the field during the match forfeits it. */
static int G_DragMatchEntrants( int *gone ) {
	int i, count = 0;
	if ( gone ) *gone = -1;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !dragMatch[i].entered ) continue;
		count++;
		if ( gone && !G_DragActive( i ) ) *gone = i;
	}
	return count;
}

static void G_DragUpdateMatch( void ) {
	int i, entrants, gone, need, best = -1, bestWins = -1;
	qboolean shared = qfalse;
	entrants = G_DragMatchEntrants( &gone );
	need = dragMatchBestOf / 2 + 1;
	dragMatchWinner = -1;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !dragMatch[i].entered ) continue;
		if ( dragMatch[i].wins > bestWins ) {
			bestWins = dragMatch[i].wins;
			best = i;
			shared = qfalse;
		} else if ( dragMatch[i].wins == bestWins ) {
			shared = qtrue;
		}
	}
	if ( entrants <= 1 ) {
		dragMatchOver = dragMatchHeats >= dragMatchBestOf || gone >= 0;
		return;
	}
	if ( gone >= 0 ) {
		/* A forfeit ends the match on the heats won so far: the driver who
		 * stayed wins it if ahead, as the Ladder counts it. */
		dragMatchOver = qtrue;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( dragMatch[i].entered && i != gone && G_DragActive( i )
				&& dragMatch[i].wins > dragMatch[gone].wins ) dragMatchWinner = i;
		}
		return;
	}
	dragMatchOver = bestWins >= need || dragMatchAttempts >= 2 * dragMatchBestOf;
	if ( dragMatchOver && !shared ) dragMatchWinner = best;
}

/* Fold a resolved heat into the match. A duel heat without a winner is run
 * again; solo practice counts every heat. */
static void G_DragRecordHeat( int heatDrivers, int heatWinner ) {
	int i, total;
	if ( dragHeatRecorded ) return;
	dragHeatRecorded = qtrue;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		const dragClient_t *driver = &dragClients[i];
		dragMatchDriver_t *entry = &dragMatch[i];
		if ( !driver->participant || !entry->entered ) continue;
		entry->lastPhase = driver->phase;
		if ( driver->phase != DRAG_FINISHED || !dragGreenTime || driver->launchTime < dragGreenTime
			|| driver->finishTime <= driver->launchTime ) continue;
		total = driver->finishTime - dragGreenTime;
		if ( entry->hasBest && total >= entry->bestTotal ) continue;
		entry->hasBest = qtrue;
		entry->bestRt = driver->launchTime - dragGreenTime;
		entry->bestEt = driver->finishTime - driver->launchTime;
		entry->bestTotal = total;
		entry->bestTrap = driver->trapSpeed;
		entry->bestFinish = driver->finishTime;
	}
	if ( heatWinner >= 0 && heatWinner < MAX_CLIENTS && dragMatch[heatWinner].entered ) {
		dragMatch[heatWinner].wins++;
	}
	dragMatchAttempts++;
	if ( heatDrivers <= 1 || heatWinner >= 0 ) dragMatchHeats++;
	G_DragUpdateMatch();
	G_DragPublishMatch( -1 );
	G_LogPrintf( "DragMatch: heat=%d attempts=%d bestOf=%d over=%d winner=%d\n",
		dragMatchHeats, dragMatchAttempts, dragMatchBestOf, dragMatchOver ? 1 : 0, dragMatchWinner );
}

static const char *G_DragShortName( int clientNum ) {
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || !g_entities[clientNum].client ) return "-";
	return dragMatch[clientNum].identity.cleanName[0] ? dragMatch[clientNum].identity.cleanName
		: g_entities[clientNum].client->pers.netname;
}

/* "HEAT 2 OF 3: CARLA" / "MATCH 1 - 1" or the match result. */
static void G_DragAnnounceMatch( int heatWinner ) {
	char score[64];
	int i, a = -1, b = -1;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !dragMatch[i].entered ) continue;
		if ( a < 0 ) a = i; else if ( b < 0 ) b = i;
	}
	if ( b >= 0 ) {
		Com_sprintf( score, sizeof( score ), "%s %d - %d %s", G_DragShortName( a ), dragMatch[a].wins,
			dragMatch[b].wins, G_DragShortName( b ) );
	} else {
		Com_sprintf( score, sizeof( score ), "heat %d of %d", dragMatchHeats, dragMatchBestOf );
	}
	if ( dragMatchOver ) {
		trap_SendServerCommand( -1, dragMatchWinner >= 0
			? va( "cp \"%s WINS THE MATCH\n%s\"", G_DragShortName( dragMatchWinner ), score )
			: va( "cp \"MATCH OVER\n%s\"", score ) );
	} else if ( heatWinner >= 0 ) {
		trap_SendServerCommand( -1, va( "cp \"HEAT TO %s\n%s\"", G_DragShortName( heatWinner ), score ) );
	} else {
		trap_SendServerCommand( -1, va( "cp \"%s\n%s\"", b >= 0 ? "NO WINNER - HEAT AGAIN" : "HEAT DONE", score ) );
	}
}

/* Back to the grid for the next heat of the match, without a map restart. */
static void G_DragNextHeat( void ) {
	gentity_t *ent;
	int i;
	dragCountdownTime = dragScheduledGreenTime = dragGreenTime = 0;
	dragAllDoneTime = dragFirstFinishTime = 0;
	dragLastLight = -1;
	dragResultAnnounced = dragHeatRecorded = qfalse;
	level.startRaceTime = level.finishRaceTime = 0;
	level.raceState = RACE_STATE_NONE;
	level.winnerNumber = -1;
	memset( dragClients, 0, sizeof( dragClients ) );
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !G_DragActive( i ) ) continue;
		ent = &g_entities[i];
		ent->client->finishRaceTime = 0;
		ent->client->lapStartTime = ent->client->lastCheckpointTime = 0;
		ent->client->ps.stats[STAT_POSITION] = 0;
		/* A driver who was out of the last heat (blown engine, false start,
		 * finished) became a race observer, maybe following the rival. Only
		 * ClientBegin clears that, so take him out of it here, or ClientSpawn
		 * puts him back into the observer camera instead of his car. */
		ent->raceObserver = qfalse;
		ent->client->sess.spectatorState = SPECTATOR_NOT;
		ent->client->ps.pm_flags &= ~( PMF_FOLLOW | PMF_OBSERVE );
		ClientSpawn( ent );
	}
	G_DragPublishMatch( -1 );
}

/* After a match against a bot, the next match is against another bot. */
static void G_DragNextRival( void ) {
	char userinfo[MAX_INFO_STRING];
	const char *next, *skill;
	int i, humans = 0;
	if ( g_dragTournament.integer ) return;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( dragMatch[i].entered && G_DragActive( i ) && !( g_entities[i].r.svFlags & SVF_BOT ) ) humans++;
	}
	if ( !humans ) return;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !dragMatch[i].entered || !G_DragActive( i ) || !( g_entities[i].r.svFlags & SVF_BOT ) ) continue;
		next = G_DragNextRivalName( g_entities[i].client->pers.netname );
		if ( !next ) continue;
		trap_GetUserinfo( i, userinfo, sizeof( userinfo ) );
		skill = Info_ValueForKey( userinfo, "skill" );
		trap_SendConsoleCommand( EXEC_APPEND, va( "clientkick %d\naddbot %s %s free\n", i, next,
			skill[0] ? skill : "3" ) );
	}
}

static void G_DragFinishMatch( const char *reason ) {
	int wins[MAX_CLIENTS], i;
	level.winnerNumber = dragMatchWinner;
	for ( i = 0; i < MAX_CLIENTS; i++ ) wins[i] = dragMatch[i].wins;
	G_DragTourneyMatchOver( dragMatchWinner, wins );
	G_DragNextRival();
	LogExit( reason );
}

void G_DragRunFrame( void ) {
	int i, j, count, done, winner, position;
	qboolean tied, positionChanged, resultChanged;
	if ( g_gametype.integer != GT_DRAG || !dragCountdownTime || level.intermissiontime ) return;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( dragClients[i].participant && G_DragActive( i ) ) G_DragCheckEngine( i );
	}
	G_DragStartGreen();
	if ( !level.startRaceTime ) return;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( dragClients[i].participant && G_DragActive( i ) && ( g_entities[i].r.svFlags & SVF_BOT ) ) {
			G_DragBotFrame( i );
		}
	}
	count = done = 0;
	winner = -1;
	tied = qfalse;
	positionChanged = resultChanged = qfalse;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		dragClient_t *driver = &dragClients[i];
		if ( !driver->participant ) continue;
		count++;
		if ( !G_DragTerminal( driver ) && ( !G_DragActive( i )
			|| level.time >= dragGreenTime + DRAG_HEAT_TIMEOUT_MS
			|| ( dragFirstFinishTime && level.time >= dragFirstFinishTime + DRAG_FINISH_GRACE_MS ) ) ) {
			G_DragDisqualify( i, DRAG_DNF, "DNF - heat ended" );
		}
		if ( G_DragTerminal( driver ) ) done++;
		if ( driver->phase == DRAG_FINISHED ) {
			if ( !dragFirstFinishTime || driver->finishTime < dragFirstFinishTime ) {
				dragFirstFinishTime = driver->finishTime;
			}
			if ( winner < 0 || G_DragCompare( i, winner ) < 0 ) {
				winner = i;
				tied = qfalse;
			} else if ( G_DragCompare( i, winner ) == 0 ) {
				tied = qtrue;
			}
		}
		if ( G_DragActive( i ) ) {
			position = 1;
			for ( j = 0; j < MAX_CLIENTS; j++ ) {
				if ( j != i && dragClients[j].participant && G_DragCompare( j, i ) < 0 ) position++;
			}
			if ( g_entities[i].client->ps.stats[STAT_POSITION] != position ) {
				g_entities[i].client->ps.stats[STAT_POSITION] = position;
				positionChanged = qtrue;
			}
		}
	}
	/* Resolve after every client's movement for this server frame; entity
	 * iteration order must never decide two close finish-plane crossings. */
	if ( winner >= 0 ) {
		level.finishRaceTime = dragFirstFinishTime;
		level.winnerNumber = tied ? -1 : winner;
	}
	if ( count && done == count && !dragAllDoneTime ) {
		dragAllDoneTime = level.time;
	}
	if ( dragAllDoneTime && !dragResultAnnounced ) {
		dragResultAnnounced = qtrue;
		resultChanged = qtrue;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			const dragClient_t *driver = &dragClients[i];
			if ( !driver->participant ) continue;
			if ( driver->phase == DRAG_FINISHED ) {
				G_LogPrintf( "DragResult: %i finished rt=%d et=%d total=%d trap=%d.%d bot=%i %s\n", i,
					driver->launchTime - dragGreenTime, driver->finishTime - driver->launchTime,
					driver->finishTime - dragGreenTime, driver->trapSpeed / 10, driver->trapSpeed % 10,
					( g_entities[i].r.svFlags & SVF_BOT ) ? 1 : 0, driver->ladderIdentity.cleanName );
			} else {
				G_LogPrintf( "DragResult: %i %s bot=%i %s\n", i,
					driver->phase == DRAG_FALSE_START ? "false_start" : "dnf",
					( g_entities[i].r.svFlags & SVF_BOT ) ? 1 : 0, driver->ladderIdentity.cleanName );
			}
		}
		if ( winner >= 0 && !tied ) {
			trap_SendServerCommand( -1, va( "print \"%s won the Drag heat!\n\"",
				g_entities[winner].client->pers.netname ) );
		}
		G_DragRecordHeat( count, tied ? -1 : winner );
		if ( dragMatchBestOf <= 1 ) {
			if ( winner >= 0 && !tied ) {
				trap_SendServerCommand( winner, "cp \"DRAG HEAT WON\"" );
			} else {
				trap_SendServerCommand( -1, winner >= 0 ? "cp \"DRAG HEAT TIED\"" : "cp \"NO VALID FINISH\"" );
			}
		} else {
			G_DragAnnounceMatch( tied ? -1 : winner );
		}
	}
	if ( positionChanged ) {
		Cmd_RacePositions_f();
	}
	if ( positionChanged || resultChanged ) {
		CalculateRanks();
	}
}

qboolean G_DragCheckExit( void ) {
	int gone;
	if ( g_gametype.integer != GT_DRAG ) return qfalse;
	if ( level.intermissionQueued || level.intermissiontime ) return qtrue;
	if ( dragMapInvalid ) {
		LogExit( "Invalid Drag Race map." );
	} else if ( dragAllDoneTime && dragMatchOver && level.time >= dragAllDoneTime + DRAG_RESULT_HOLD_MS ) {
		G_DragFinishMatch( dragMatchBestOf > 1 ? "Drag match finished." : "Drag heat finished." );
	} else if ( dragAllDoneTime && !dragMatchOver && level.time >= dragAllDoneTime + DRAG_NEXT_HEAT_MS ) {
		G_DragNextHeat();
	} else if ( !dragCountdownTime && dragMatchAttempts && !dragMatchOver ) {
		/* Between heats, a driver of the match left: forfeit. */
		G_DragMatchEntrants( &gone );
		if ( gone >= 0 ) {
			G_DragUpdateMatch();
			G_DragPublishMatch( -1 );
			G_DragAnnounceMatch( -1 );
			G_DragFinishMatch( "Drag match forfeited." );
		}
	}
	return qtrue;
}
