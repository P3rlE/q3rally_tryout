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
		&& dragCountdownTime > 0 && dragClients[clientNum].participant;
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
	return !strcmp( identity, dragClients[clientNum].ladderIdentity.playerId );
}

qboolean G_DragPopulateLadderPlayer( int clientNum, ladderPlayerPayload_t *player ) {
	const dragClient_t *driver;
	int i;
	if ( !player || !G_DragLadderParticipant( clientNum ) ) return qfalse;
	driver = &dragClients[clientNum];
	*player = driver->ladderIdentity;
	player->time = level.time > driver->enterTime ? level.time - driver->enterTime : 0;
	Q_strncpyz( player->dragStatus, driver->phase == DRAG_FALSE_START ? "false_start" : "dnf", sizeof( player->dragStatus ) );
	if ( driver->phase == DRAG_FINISHED && driver->launchTime >= dragGreenTime
		&& driver->finishTime > driver->launchTime ) {
		Q_strncpyz( player->dragStatus, "finished", sizeof( player->dragStatus ) );
		player->reactionMs = driver->launchTime - dragGreenTime;
		player->elapsedMs = driver->finishTime - driver->launchTime;
		player->totalRaceMs = driver->finishTime - dragGreenTime;
		player->finishRaceTime = driver->finishTime;
		player->trapSpeedKph = driver->trapSpeed * 0.1f;
		player->position = 1;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( i != clientNum && G_DragLadderParticipant( i )
				&& dragClients[i].phase == DRAG_FINISHED
				&& dragClients[i].finishTime < driver->finishTime ) player->position++;
		}
	}
	return qtrue;
}

static qboolean G_DragActive( int clientNum ) {
	gentity_t *ent = &g_entities[clientNum];
	return ent->inuse && ent->client
		&& ent->client->pers.connected == CON_CONNECTED
		&& ent->client->sess.sessionTeam != TEAM_SPECTATOR
		&& !( ent->r.svFlags & SVF_BOT );
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
	if ( g_gametype.integer == GT_DRAG ) {
		level.winnerNumber = -1;
	}
}

qboolean G_DragCanJoin( void ) {
	return g_gametype.integer != GT_DRAG || !dragCountdownTime;
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
	} else if ( dragCountdownTime && ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
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
	if ( commandTime < ent->client->pers.cmd.serverTime ) {
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

void G_DragRunFrame( void ) {
	int i, j, count, done, winner, position;
	qboolean tied, positionChanged, resultChanged;
	if ( g_gametype.integer != GT_DRAG || !dragCountdownTime || level.intermissiontime ) return;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( dragClients[i].participant && G_DragActive( i ) ) G_DragCheckEngine( i );
	}
	G_DragStartGreen();
	if ( !level.startRaceTime ) return;
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
		if ( winner >= 0 && !tied ) {
			trap_SendServerCommand( -1, va( "print \"%s won the Drag heat!\n\"",
				g_entities[winner].client->pers.netname ) );
			trap_SendServerCommand( winner, "cp \"DRAG HEAT WON\"" );
		} else {
			trap_SendServerCommand( -1, winner >= 0 ? "cp \"DRAG HEAT TIED\"" : "cp \"NO VALID FINISH\"" );
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
	if ( g_gametype.integer != GT_DRAG ) return qfalse;
	if ( dragMapInvalid && !level.intermissionQueued && !level.intermissiontime ) {
		LogExit( "Invalid Drag Race map." );
	} else if ( dragAllDoneTime && level.time >= dragAllDoneTime + DRAG_RESULT_HOLD_MS
		&& !level.intermissionQueued && !level.intermissiontime ) {
		LogExit( "Drag heat finished." );
	}
	return qtrue;
}
