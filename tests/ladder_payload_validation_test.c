#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "server.h"

cvar_t *sv_ladderUrl = NULL;
cvar_t *sv_ladderApiKey = NULL;
cvar_t *sv_ladderEnabled = NULL;
cvar_t *sv_telemetryMaxBatch = NULL;

#include "sv_ladder_for_test.c"

static void InitPayload( ladderMatchPayload_t *payload, int gametype ) {
    Com_Memset( payload, 0, sizeof( *payload ) );
    payload->valid = qtrue;
    payload->gametype = gametype;
    Q_strncpyz( payload->matchId, "match-1", sizeof( payload->matchId ) );
    Q_strncpyz( payload->mode, "mode", sizeof( payload->mode ) );
    Q_strncpyz( payload->mapName, "q3r_test", sizeof( payload->mapName ) );
    payload->playerCount = 1;
    payload->players[0].team = TEAM_FREE;
}

int main( void ) {
    ladderMatchPayload_t payload;

    InitPayload( &payload, GT_RACING );
    payload.numberOfLaps = 3;
    payload.players[0].lapCount = 2;
    payload.players[0].lapTimes[0] = 12000;
    payload.players[0].lapTimes[1] = 11000;
    assert( G_LadderValidatePayload( &payload, qtrue ) == qtrue );
    assert( payload.validationErrors == 0 );

    InitPayload( &payload, GT_RACING );
    payload.numberOfLaps = 0;
    assert( G_LadderValidatePayload( &payload, qtrue ) == qfalse );
    assert( payload.valid == qfalse );
    assert( payload.validationErrors & LADDER_PAYLOAD_ERR_MISSING_REQUIRED );

    InitPayload( &payload, GT_DEATHMATCH );
    payload.players[0].kills = 10;
    payload.players[0].deaths = 0;
    payload.players[0].kdRatio = 0.0f;
    assert( G_LadderValidatePayload( &payload, qtrue ) == qtrue );
    assert( payload.validationWarnings & LADDER_PAYLOAD_WARN_KD_RATIO_REPAIRED );
    assert( payload.players[0].kdRatio == 10.0f );

    InitPayload( &payload, GT_TEAM );
    payload.players[0].team = 77;
    assert( G_LadderValidatePayload( &payload, qtrue ) == qfalse );
    assert( payload.validationErrors & LADDER_PAYLOAD_ERR_VALUE_RANGE );

    InitPayload( &payload, GT_CTF );
    payload.players[0].lapCount = 1;
    payload.players[0].lapTimes[0] = 0;
    assert( G_LadderValidatePayload( &payload, qtrue ) == qfalse );
    assert( payload.validationErrors & LADDER_PAYLOAD_ERR_INTERNAL_CONSISTENCY );

    InitPayload( &payload, GT_DRAG );
    payload.winnerClientNum = -1;
    Q_strncpyz( payload.mode, "GT_DRAG", sizeof( payload.mode ) );
    Q_strncpyz( payload.players[0].dragStatus, "finished", sizeof( payload.players[0].dragStatus ) );
    payload.players[0].reactionMs = 200;
    payload.players[0].elapsedMs = 12500;
    payload.players[0].totalRaceMs = 12700;
    payload.players[0].finishRaceTime = 22700;
    payload.players[0].position = 1;
    payload.players[0].trapSpeedKph = 151.2f;
    assert( G_LadderValidatePayload( &payload, qtrue ) == qtrue );
    payload.players[0].totalRaceMs++;
    assert( G_LadderValidatePayload( &payload, qtrue ) == qfalse );
    payload.players[0].totalRaceMs--;
    payload.valid = qtrue;
    payload.players[0].reactionMs = DRAG_MIN_REACTION_MS; /* the fastest legal reaction */
    payload.players[0].totalRaceMs = payload.players[0].elapsedMs + DRAG_MIN_REACTION_MS;
    assert( G_LadderValidatePayload( &payload, qtrue ) == qtrue );
    payload.players[0].reactionMs--; /* faster is a false start, never a finish */
    payload.players[0].totalRaceMs--;
    assert( G_LadderValidatePayload( &payload, qtrue ) == qfalse );
    payload.players[0].reactionMs = 0;
    payload.players[0].totalRaceMs = payload.players[0].elapsedMs;
    payload.valid = qtrue;
    assert( G_LadderValidatePayload( &payload, qtrue ) == qfalse );
    payload.players[0].reactionMs = DRAG_MIN_REACTION_MS;
    payload.players[0].totalRaceMs = payload.players[0].elapsedMs + DRAG_MIN_REACTION_MS;
    payload.valid = qtrue;
    payload.players[0].trapSpeedKph = NAN;
    assert( G_LadderValidatePayload( &payload, qtrue ) == qfalse );

    InitPayload( &payload, GT_DRAG );
    payload.winnerClientNum = -1;
    Q_strncpyz( payload.players[0].dragStatus, "false_start", sizeof( payload.players[0].dragStatus ) );
    assert( G_LadderValidatePayload( &payload, qtrue ) == qtrue );
    payload.players[0].finishRaceTime = 10000; /* observer delay is not a finish */
    assert( G_LadderValidatePayload( &payload, qtrue ) == qfalse );
    payload.players[0].finishRaceTime = 0;
    payload.valid = qtrue;
    payload.winnerClientNum = 0; /* solo practice cannot be a competitive win */
    assert( G_LadderValidatePayload( &payload, qtrue ) == qfalse );

    InitPayload( &payload, GT_DRAG );
    payload.playerCount = 2;
    payload.dragDuel = qtrue;
    payload.winnerClientNum = -1;
    Q_strncpyz( payload.players[0].dragStatus, "dnf", sizeof( payload.players[0].dragStatus ) );
    Q_strncpyz( payload.players[1].dragStatus, "dnf", sizeof( payload.players[1].dragStatus ) );
    assert( G_LadderValidatePayload( &payload, qtrue ) == qtrue );
    payload.dragDuel = qfalse;
    assert( G_LadderValidatePayload( &payload, qtrue ) == qfalse );

    puts( "ok" );
    return 0;
}
