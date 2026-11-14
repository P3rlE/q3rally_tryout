/*
 * Q3Rally v0.9 Drag Race prototype.
 * Equal cars, a deliberate handbrake release and a common finish plane.
 * Keep heat state private: none of these results enter the v0.8 profiles.
 */
#include "g_local.h"

#define DRAG_HEAT_TIMEOUT_MS 60000
#define DRAG_FINISH_GRACE_MS 10000
#define DRAG_RESULT_HOLD_MS 3000

typedef struct {
	dragPhase_t phase;
	qboolean spawned;
	qboolean participant;
	qboolean brakeHeld;
	int releaseTime;
	int finishTime;
	int trapSpeed;
	int nextNotice;
	vec3_t gridOrigin;
	float progress;
	float previousSpeed;
} dragClient_t;

static dragClient_t dragClients[MAX_CLIENTS];
static int dragCountdownTime;
static int dragGreenTime;
static int dragLastLight;
static int dragAllDoneTime;
static int dragFirstFinishTime;
static qboolean dragMapInvalid;
static qboolean dragResultAnnounced;
static vec3_t dragForward, dragRight, dragFinishOrigin;
static float dragLaneHalfWidth;

extern void LogExit( const char *string );

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
	trap_SendServerCommand( recipient, va( "dragStatus %d %d %d %d %d %d",
		clientNum, driver->phase, dragGreenTime, driver->releaseTime,
		driver->finishTime, driver->trapSpeed ) );
}

static void G_DragSetPhase( int clientNum, dragPhase_t phase ) {
	dragClients[clientNum].phase = phase;
	if ( phase == DRAG_RUNNING && g_entities[clientNum].client
		&& g_entities[clientNum].client->ps.commandTime < dragClients[clientNum].releaseTime ) {
		/* A usercommand describes input at its timestamp. Do not integrate
		 * its new throttle backwards across the interval spent staged. */
		g_entities[clientNum].client->ps.commandTime = dragClients[clientNum].releaseTime;
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
	dragCountdownTime = dragGreenTime = dragAllDoneTime = dragFirstFinishTime = 0;
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
		ent->ready = qfalse;
		G_DragPublish( clientNum, -1 );
	}
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( dragClients[i].spawned || dragClients[i].participant ) {
			G_DragPublish( i, clientNum );
		}
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
		dragCountdownTime = dragGreenTime = 0;
		dragLastLight = -1;
		level.raceState = RACE_STATE_NONE;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			dragClients[i].participant = qfalse;
			dragClients[i].releaseTime = 0;
			dragClients[i].finishTime = 0;
			if ( G_DragActive( i ) && i != clientNum ) {
				dragClients[i].phase = dragClients[i].brakeHeld ? DRAG_STAGED : DRAG_WAITING;
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

static void G_DragStartGreen( void ) {
	int i;
	if ( !dragGreenTime || level.startRaceTime || level.time < dragGreenTime ) {
		return;
	}
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
		if ( !G_DragTerminal( &dragClients[i] ) && dragClients[i].releaseTime ) {
			G_DragSetPhase( i, DRAG_RUNNING );
		} else {
			G_DragPublish( i, -1 );
		}
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
			if ( !dragClients[i].spawned || !dragClients[i].brakeHeld ) {
				staged = qfalse;
			}
		}
		if ( !count || !staged || count > 2 ) {
			return;
		}
		dragCountdownTime = level.time;
		/* Vary only the final wait. The common scheduled timestamp is the
		 * authority for all release commands, including delayed packets. */
		dragGreenTime = level.time + 3000 + ( rand() % 1001 );
		level.raceState = RACE_STATE_COUNTDOWN;
		level.raceIntroEndTime = 0;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( G_DragActive( i ) ) {
				dragClients[i].participant = qtrue;
				dragClients[i].releaseTime = 0;
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
			trap_SendServerCommand( -1, va( "rc \"%d - HOLD HANDBRAKE\" %d", light, light ) );
		}
	}
}

void G_DragFilterCommand( gentity_t *ent, usercmd_t *cmd ) {
	dragClient_t *driver;
	qboolean brakeHeld;
	int clientNum;
	if ( g_gametype.integer != GT_DRAG || !ent || !ent->client || !cmd ) {
		return;
	}
	clientNum = ent->s.number;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) return;
	driver = &dragClients[clientNum];
	brakeHeld = ( cmd->buttons & BUTTON_HANDBRAKE ) ? qtrue : qfalse;
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
		if ( driver->brakeHeld != brakeHeld ) {
			G_DragSetPhase( clientNum, brakeHeld ? DRAG_STAGED : DRAG_WAITING );
		}
		driver->brakeHeld = brakeHeld;
		ent->ready = brakeHeld;
		if ( level.time >= driver->nextNotice ) {
			trap_SendServerCommand( clientNum,
				"cp \"DRAG RACE: hold HANDBRAKE to stage.\nKeep holding until GREEN, then release.\nManual gears: gearUp / gearDown\"" );
			driver->nextNotice = level.time + 4000;
		}
	} else if ( driver->participant && !G_DragTerminal( driver ) ) {
		/* The first release is final: reapplying the brake cannot erase a
		 * false start, and pre-held throttle cannot launch a staged car. */
		if ( driver->brakeHeld && !brakeHeld && !driver->releaseTime ) {
			driver->releaseTime = cmd->serverTime;
			if ( cmd->serverTime < dragGreenTime ) {
				G_DragDisqualify( clientNum, DRAG_FALSE_START, "FALSE START" );
			} else if ( level.startRaceTime ) {
				G_DragSetPhase( clientNum, DRAG_RUNNING );
			}
		}
		driver->brakeHeld = brakeHeld;
	}
	G_DragStartGreen();
	if ( !driver->participant || driver->phase != DRAG_RUNNING || !level.startRaceTime ) {
		cmd->buttons = BUTTON_HANDBRAKE;
		cmd->forwardmove = cmd->rightmove = cmd->upmove = 0;
	}
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
		if ( timestamp < driver->releaseTime ) timestamp = driver->releaseTime;
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

void G_DragRunFrame( void ) {
	int i, j, count, done, winner, position;
	qboolean tied, positionChanged, resultChanged;
	if ( g_gametype.integer != GT_DRAG || !dragCountdownTime || level.intermissiontime ) return;
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
