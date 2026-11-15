#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "server.h"

cvar_t *sv_ladderUrl = NULL;
cvar_t *sv_ladderApiKey = NULL;
cvar_t *sv_ladderEnabled = NULL;
cvar_t *sv_telemetryMaxBatch = NULL;

#include "sv_ladder_for_test.c"

int main( void ) {
        ladderMatchPayload_t payload;
        char *json;
        size_t length;

        Com_Memset( &payload, 0, sizeof( payload ) );
        payload.valid = qtrue;
        payload.playerCount = 1;
        payload.players[0].clientNum = 7;
        payload.players[0].team = TEAM_RED;
        payload.players[0].lapCount = 2;
        payload.players[0].lapTimes[0] = 1234;
        payload.players[0].lapTimes[1] = 1100;
        payload.players[0].bestLapMs = 1100;
        Q_strncpyz( payload.players[0].name, "Tester", sizeof( payload.players[0].name ) );

        json = SV_LadderSerializeMatch( &payload, &length );
        assert( json != NULL );
        assert( strstr( json, "\"bestLapMs\":1100" ) != NULL );
        assert( strstr( json, "\"lapTimes\":[1234,1100]" ) != NULL );
        /* no weather filled in: a dry match on a map without weather */
        assert( strstr( json, "\"weather\":{\"condition\":\"dry\",\"mode\":0,\"wetPct\":0,\"snowPct\":0,\"map\":\"none\"}" ) != NULL );
        assert( strstr( json, "\"teamHoldMs\"" ) == NULL );
        assert( strstr( json, "\"kothContestTimeMs\"" ) == NULL );
        assert( strstr( json, "\"autoballGoals\"" ) == NULL );
        assert( strstr( json, "\"dragStatus\"" ) == NULL );
        assert( strstr( json, "\"dragDuel\"" ) == NULL );

        Z_Free( json );

        payload.gametype = GT_KOTH;
        payload.teamHoldMs[TEAM_RED] = 125000;
        payload.teamHoldMs[TEAM_BLUE] = 93000;
        payload.players[0].zoneHoldMs = 42000;
        payload.players[0].kothContestTimeMs = 7000;
        json = SV_LadderSerializeMatch( &payload, &length );
        assert( json != NULL );
        assert( strstr( json, "\"teamHoldMs\":[0,125000,93000,0,0,0]" ) != NULL );
        assert( strstr( json, "\"zoneHoldMs\":42000" ) != NULL );
        assert( strstr( json, "\"kothContestTimeMs\":7000" ) != NULL );
        assert( strstr( json, "\"autoballGoals\"" ) == NULL );
        Z_Free( json );

        /* Autoball counters are serialized only for GT_AUTOBALL. */
        payload.players[0].autoballGoals = 3;
        payload.players[0].autoballAssists = 2;
        payload.players[0].autoballSaves = 4;
        payload.players[0].autoballShots = 9;
        payload.players[0].autoballDemos = 1;
        json = SV_LadderSerializeMatch( &payload, &length );
        assert( json != NULL );
        assert( strstr( json, "\"autoballGoals\"" ) == NULL );
        assert( strstr( json, "\"autoballDemos\"" ) == NULL );
        Z_Free( json );

        payload.gametype = GT_AUTOBALL;
        payload.teamScores[TEAM_RED] = 5;
        payload.teamScores[TEAM_BLUE] = 3;
        json = SV_LadderSerializeMatch( &payload, &length );
        assert( json != NULL );
        assert( strstr( json, "\"autoballGoals\":3" ) != NULL );
        assert( strstr( json, "\"autoballAssists\":2" ) != NULL );
        assert( strstr( json, "\"autoballSaves\":4" ) != NULL );
        assert( strstr( json, "\"autoballShots\":9" ) != NULL );
        assert( strstr( json, "\"autoballDemos\":1" ) != NULL );
        assert( strstr( json, "\"teamScores\":[0,5,3,0,0,0]" ) != NULL );
        assert( strstr( json, "\"kothContestTimeMs\"" ) == NULL );
        assert( strstr( json, "\"teamHoldMs\"" ) == NULL );
        Z_Free( json );

        payload.gametype = GT_DRAG;
        payload.dragDuel = qtrue;
        payload.players[0].reactionMs = 200;
        payload.players[0].elapsedMs = 12500;
        payload.players[0].totalRaceMs = 12700;
        payload.players[0].trapSpeedKph = 151.2f;
        Q_strncpyz( payload.players[0].dragStatus, "finished", sizeof( payload.players[0].dragStatus ) );
        payload.players[0].profileAttached = qtrue;
        payload.players[0].profile.valid = qtrue;
        payload.players[0].profile.dragCompleted = 4;
        payload.players[0].profile.dragBestReactionMs = 0;
        json = SV_LadderSerializeMatch( &payload, &length );
        assert( json != NULL );
        assert( strstr( json, "\"dragDuel\":true" ) != NULL );
        assert( strstr( json, "\"dragStatus\":\"finished\"" ) != NULL );
        assert( strstr( json, "\"reactionMs\":200" ) != NULL );
        assert( strstr( json, "\"elapsedMs\":12500" ) != NULL );
        assert( strstr( json, "\"totalRaceMs\":12700" ) != NULL );
        assert( strstr( json, "\"trapSpeedKph\":151.2" ) != NULL );
        assert( strstr( json, "\"dragCompleted\":4" ) != NULL );
        assert( strstr( json, "\"dragBestReactionMs\":0" ) != NULL );
        Z_Free( json );

        puts( "ok" );
        return 0;
}
