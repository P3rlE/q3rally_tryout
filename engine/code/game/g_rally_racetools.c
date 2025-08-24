/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

/*
 * NOTE: The Q3 VM (lcc) environment doesn't provide standard C headers like
 * <string.h>/<stdlib.h>. We therefore avoid strtok/strchr/strlen/atoi and use
 * local minimal helpers + existing Q3 string utilities (Q_stricmp, Q_strncmp,
 * Q_strncpyz, Com_sprintf, va, etc.). Everything below is C89-friendly.
 */

/* ---- minimal local helpers (C89, no libc) ---- */
static int q_len( const char *s ) {
    int n;
    if ( !s ) return 0;
    n = 0;
    while ( s[n] ) n++;
    return n;
}

static int q_atoi( const char *s ) {
    int sign, v, c;
    if ( !s ) return 0;
    sign = 1; v = 0;
    if ( *s == '-' ) { sign = -1; s++; }
    while ( (c = *s) >= '0' && c <= '9' ) { v = v*10 + (c - '0'); s++; }
    return sign * v;
}

static char *q_find_char( char *s, char ch ) {
    if ( !s ) return NULL;
    while ( *s ) { if ( *s == ch ) return s; s++; }
    return NULL;
}

/* Iterate over lines in a mutable buffer. Replaces line breaks with  . */
static char *q_next_line( char **cursor ) {
    char *p, *start;
    if ( !cursor || !*cursor ) return NULL;
    p = *cursor;
    if ( !*p ) return NULL;
    start = p;
    /* find end of line: LF(10) or CR(13) */
    while ( *p && *p != (char)10 && *p != (char)13 ) p++;
    if ( *p ) { *p = (char)0; p++; }
    /* skip subsequent CR/LF */
    while ( *p == (char)10 || *p == (char)13 ) p++;
    *cursor = p;
    return start;
}

/* ----------------------------------------------------------------------------
   Records I/O (best lap/score per map)
   ---------------------------------------------------------------------------- */
static int G_ReadBestValue( const char *mapname, const char *key ) {
    fileHandle_t f;
    char filename[MAX_QPATH];
    char buffer[1024];
    int len, klen, value;
    char *cur, *line;

    value = 0;
    klen = q_len( key );

    Com_sprintf( filename, sizeof( filename ), "records/%s.record", mapname );
    len = trap_FS_FOpenFile( filename, &f, FS_READ );
    if ( len <= 0 ) {
        return 0;
    }
    if ( len >= (int)sizeof( buffer ) ) {
        len = (int)sizeof( buffer ) - 1;
    }
    trap_FS_Read( buffer, len, f );
    buffer[len] = ' ';
    trap_FS_FCloseFile( f );

    cur = buffer;
    while ( (line = q_next_line( &cur )) != NULL ) {
        if ( !Q_strncmp( line, key, klen ) && line[klen] == '=' ) {
            value = q_atoi( line + klen + 1 );
            break;
        }
    }
    return value;
}

typedef struct {
    char key[64];
    char value[128];
} record_t;

static void G_WriteRecord( const char *mapname, const char *key, int value, const char *player ) {
    fileHandle_t f;
    char filename[MAX_QPATH];
    char buffer[1024];
    int len, count, i;
    char *cur, *line, *eq;
    record_t records[16];
    char val[64];
    char playerKey[64];

    Com_sprintf( filename, sizeof( filename ), "records/%s.record", mapname );

    /* Read existing */
    count = 0;
    len = trap_FS_FOpenFile( filename, &f, FS_READ );
    if ( len > 0 ) {
        if ( len >= (int)sizeof( buffer ) ) {
            len = (int)sizeof( buffer ) - 1;
        }
        trap_FS_Read( buffer, len, f );
        buffer[len] = ' ';
        trap_FS_FCloseFile( f );

        cur = buffer;
        while ( (line = q_next_line( &cur )) != NULL && count < (int)ARRAY_LEN( records ) ) {
            eq = q_find_char( line, '=' );
            if ( !eq ) {
                continue;
            }
            *eq = ' ';
            Q_strncpyz( records[count].key, line, sizeof( records[count].key ) );
            Q_strncpyz( records[count].value, eq + 1, sizeof( records[count].value ) );
            count++;
        }
    }

    /* update or add the value */
    Com_sprintf( val, sizeof( val ), "%d", value );
    for ( i = 0; i < count; i++ ) {
        if ( !Q_stricmp( records[i].key, key ) ) {
            Q_strncpyz( records[i].value, val, sizeof( records[i].value ) );
            break;
        }
    }
    if ( i == count && count < (int)ARRAY_LEN( records ) ) {
        Q_strncpyz( records[count].key, key, sizeof( records[count].key ) );
        Q_strncpyz( records[count].value, val, sizeof( records[count].value ) );
        count++;
    }

    /* update or add the player name for this key */
    Com_sprintf( playerKey, sizeof( playerKey ), "player_%s", key );
    for ( i = 0; i < count; i++ ) {
        if ( !Q_stricmp( records[i].key, playerKey ) ) {
            Q_strncpyz( records[i].value, player, sizeof( records[i].value ) );
            break;
        }
    }
    if ( i == count && count < (int)ARRAY_LEN( records ) ) {
        Q_strncpyz( records[count].key, playerKey, sizeof( records[count].key ) );
        Q_strncpyz( records[count].value, player, sizeof( records[count].value ) );
        count++;
    }

    trap_FS_FOpenFile( filename, &f, FS_WRITE );
    for ( i = 0; i < count; i++ ) {
        char out[256];
        char nl = (char)10; /* 
 */
        Com_sprintf( out, sizeof( out ), "%s=%s", records[i].key, records[i].value );
        trap_FS_Write( out, q_len( out ), f );
        trap_FS_Write( &nl, 1, f );
    }
    trap_FS_FCloseFile( f );
}

void G_UpdateLapRecord( gentity_t *player, int lapTime ) {
    char serverinfo[MAX_INFO_STRING];
    char mapname[MAX_QPATH];
    int best;
    const char *key;

    trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );
    Q_strncpyz( mapname, Info_ValueForKey( serverinfo, "mapname" ), sizeof( mapname ) );

    key = ( player->r.svFlags & SVF_BOT ) ? "best_lap_time_bot" : "best_lap_time_player";

    best = G_ReadBestValue( mapname, key );
    if ( player->r.svFlags & SVF_BOT ) {
        return;
    }

    if ( !best || lapTime < best ) {
        G_WriteRecord( mapname, key, lapTime, player->client->pers.netname );
    }
}

void G_UpdateScoreRecord( gentity_t *player ) {
    char serverinfo[MAX_INFO_STRING];
    char mapname[MAX_QPATH];
    int best, score;

    score = player->client->ps.persistant[PERS_SCORE];

    trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );
    Q_strncpyz( mapname, Info_ValueForKey( serverinfo, "mapname" ), sizeof( mapname ) );

    best = G_ReadBestValue( mapname, "best_score" );
    if ( !best || score > best ) {
        G_WriteRecord( mapname, "best_score", score, player->client->pers.netname );
    }
}

int GetTeamAtRank( int rank ){
    int i, j, count;
    int ranks[4];
    int counts[4];

    for (i = 0; i < 4; i++){
        counts[i] = TeamCount( -1, TEAM_RED + i );
        ranks[i] = 0;
    }

    for (i = 0; i < 4; i++){
        if (!counts[i]) continue;

        count = 0;
        for (j = 0; j < 4; j++){
            if (!counts[j]) continue;

            if (isRallyRace()){
                if (level.teamTimes[i + TEAM_RED] > level.teamTimes[j + TEAM_RED]) count++;
            }
            else if (level.teamScores[i + TEAM_RED] < level.teamScores[j + TEAM_RED]) count++;
        }

        while( count < 4 && ranks[count] ) count++; /* rank is taken so move to the next one */
        if (count < 4)
            ranks[count] = TEAM_RED + i;
    }

    if (g_gametype.integer == GT_CTF && rank > 2){
        return -1;
    }
    else {
        return ranks[rank-1];
    }
}

/* UPDATE - send as command string instead? */
void Cmd_RacePositions_f( void ) {
    char entry[1024];
    char string[1400];
    gentity_t *player;
    int i, count, j, stringlength;

    string[0] = 0;
    stringlength = 0;

    for(i = 0, count = 0; i < level.maxclients; i++){
        player = &g_entities[i];
        if (!player->inuse) continue;
        if (!player->client) continue;

        Com_sprintf (entry, sizeof(entry)," %i %i", player->s.clientNum, player->client->ps.stats[STAT_POSITION]);
        j = q_len(entry);
        if (stringlength + j > 1024)
            break;
        strcpy (string + stringlength, entry);
        stringlength += j;

        count++;
    }

    G_LogPrintf("%s", va("positions %i%s", count, string));
    trap_SendServerCommand( -1, va("positions %i%s", count, string) );
}

void Cmd_Times_f( gentity_t *ent ) {
/* original code intentionally disabled here */
}

/* --------------------------------------------------------------------------
   GetDistanceToMarker
   -------------------------------------------------------------------------- */
float GetDistanceToMarker( gentity_t *player, float markerNumber )
{
    gentity_t *ent;
    vec3_t dist;

    ent = NULL;

    if ( !markerNumber )
        return 1<<30;

    while ( (ent = G_Find (ent, FOFS(classname), "rally_checkpoint")) != NULL )
    {
        if( ent->number == markerNumber )
            break;
    }

    if ( ent )
    {
        VectorSubtract(player->r.currentOrigin, ent->s.origin, dist);
        return VectorLength(dist);
    }
    else
        return 1<<30;
}

/* --------------------------------------------------------------------------
   IsCarAhead
   -------------------------------------------------------------------------- */
qboolean IsCarAhead(gentity_t *one, gentity_t *two){
    float dist1, dist2;
    int time1, time2;

    if (one->client->finishRaceTime && two->client->finishRaceTime){
        time1 = one->client->finishRaceTime - level.startRaceTime;
        if (one->client->ps.persistant[PERS_SCORE] > 0 && !isRallyNonDMRace()){
            time1 -= (one->client->ps.persistant[PERS_SCORE] * TIME_BONUS_PER_FRAG);
        }

        time2 = two->client->finishRaceTime - level.startRaceTime;
        if (two->client->ps.persistant[PERS_SCORE] > 0 && !isRallyNonDMRace()){
            time2 -= (two->client->ps.persistant[PERS_SCORE] * TIME_BONUS_PER_FRAG);
        }

        if (time1 < time2){
            return qtrue;
        }
        else {
            return qfalse;
        }
    }
    else if (one->client->finishRaceTime){
        return qtrue;
    }
    else if (two->client->finishRaceTime){
        return qfalse;
    }
    else if (one->currentLap < two->currentLap){
        return qfalse;
    }
    else if (one->currentLap == two->currentLap && one->number < two->number){
        return qfalse;
    }
    else if (one->currentLap == two->currentLap && one->number == two->number){
        dist1 = GetDistanceToMarker( one, one->number );
        dist2 = GetDistanceToMarker( two, two->number );

        if (dist1 > dist2){
            return qfalse;
        }
    }

    return qtrue;
}

/* --------------------------------------------------------------------------
   CalculatePlayerPositions
   -------------------------------------------------------------------------- */
void CalculatePlayerPositions( void )
{
    gentity_t *ent, *leader, *cur, *last;
    int position;
    qboolean positionChanged;

    if (!isRallyRace()){
        return;
    }

    positionChanged = qfalse;
    leader = NULL;
    ent = NULL;
    last = NULL;
    while ( (ent = G_Find (ent, FOFS(classname), "player")) != NULL )
    {
        if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) continue;

        ent->carBehind = NULL;

        if ( leader == NULL )
        {
            leader = ent;
            continue;
        }

        cur = leader;
        if ( IsCarAhead( ent, cur ) )
        {
            ent->carBehind = cur;
            leader = ent;
            continue;
        }

        while ( cur->carBehind != NULL )
        {
            if ( IsCarAhead( ent, cur->carBehind ) )
            {
                last = cur;
                cur = cur->carBehind;
                break;
            }

            last = cur;
            cur = cur->carBehind;
        }

        if ( IsCarAhead( ent, cur ) )
        {
            ent->carBehind = cur;
            if (last) {
                last->carBehind = ent;
            }
        }
        else {
            cur->carBehind = ent;
            ent->carBehind = NULL;
        }
    }

    if ( leader == NULL )
        return;

    cur = leader;
    position = 1;

    while( cur->carBehind != NULL )
    {
        if ( position != cur->client->ps.stats[STAT_POSITION] && cur->client ){
            cur->client->ps.stats[STAT_POSITION] = position;
            positionChanged = qtrue;
        }

        cur = cur->carBehind;
        position++;
    }

    if ( position != cur->client->ps.stats[STAT_POSITION] && cur->client ){
        cur->client->ps.stats[STAT_POSITION] = position;
        positionChanged = qtrue;
    }

    if ( positionChanged )
    {
        Cmd_RacePositions_f();
        CalculateRanks();
    }
}

void RallyRace_Think( gentity_t *ent ){
    ent->nextthink = level.time + 200;
    CalculatePlayerPositions();
}

void RaceCountdown( char *s, int secondsLeft ){
    trap_SendServerCommand( -1, va("rc \"%s\" %d", s, secondsLeft) );
}

void RallyStarter_Think( gentity_t *ent ){
    gentity_t *player, *t;
    int i, count;
    qboolean start;

    if (level.startRaceTime){
        return;
    }

    /* if no checkpoints dont do start sequence */
    if (isRallyRace()){
        t = NULL;
        t = G_Find (t, FOFS(classname), "rally_checkpoint");
        if (t == NULL){
            /* start race right away */
            level.startRaceTime = level.time;
            trap_SendServerCommand( -1, va("raceTime %i", level.startRaceTime) );
            CenterPrint_All("GO..");

            for ( i = 0; i < level.maxclients; i++ ) {
                player = &g_entities[i];
                if ( !player->inuse || !player->client ) continue;
                player->client->startLapTime = level.startRaceTime;
                player->client->bestLapTime = 0;
            }

            G_FreeEntity( ent );
            return;
        }
    }
    ent->nextthink = level.time + 1000;
    t = NULL;

    if ( ent->number == 0 ){

        if( level.time - level.startTime < 7500 )
            return;

        start = qtrue;
        for (i = 0, count = 0; i < MAX_CLIENTS; i++){
            player = &g_entities[i];
            if (!player->inuse) continue;
            if (!player->client) continue;
            if (player->client->sess.sessionTeam == TEAM_SPECTATOR) continue;
            /* bots are always ready */

            count++;

            if (player->r.svFlags & SVF_BOT) continue;

            if ( !player->ready ){
                start = qfalse;
                break;
            }
        }

        if ( !count ){
            return;
        }
        else if ( start && count ){
            ent->number = 3;
        }
        else if ( level.time >= level.startTime + (g_forceEngineStart.integer * 1000) ) {
            ent->number = 3; /* force race start */
        }
        else if (ent->number == 0 && level.time > level.startTime + (g_forceEngineStart.integer * 1000) - 10000){
            CenterPrint_All( va("Forced engine start in %i...", 10 - ((level.time - (level.startTime + (g_forceEngineStart.integer * 1000) - 10000)) / 1000)) );
            return;
        }
        else {
            return;
        }
    }

    if ( ent->pain_debounce_time == 0 )
        ent->pain_debounce_time = level.time;

    if ( level.time > ent->pain_debounce_time + 5000 ){
            level.startRaceTime = level.time;

            for ( i = 0; i < level.maxclients; i++ ) {
                    player = &g_entities[i];
                    if ( !player->inuse || !player->client ) continue;
                    player->client->startLapTime = level.startRaceTime;
                    player->client->bestLapTime = 0;
            }

            trap_SendServerCommand( -1, va("raceTime %i", level.startRaceTime) );
            RaceCountdown("GO!", 0);

            Rally_Sound( ent, EV_GLOBAL_SOUND, CHAN_ANNOUNCER, G_SoundIndex("sound/rally/race/go.wav") );

            if (g_gametype.integer != GT_DERBY)
                    ent->think = RallyRace_Think;
    }
    else if ( level.time > ent->pain_debounce_time + 4000 ){
        RaceCountdown("1", 1);

        Rally_Sound( ent, EV_GLOBAL_SOUND, CHAN_ANNOUNCER, G_SoundIndex("sound/rally/race/one.wav") );
        ent->number = -1;
    }
    else if ( level.time > ent->pain_debounce_time + 3000 ){
        RaceCountdown("2", 2);

        Rally_Sound( ent, EV_GLOBAL_SOUND, CHAN_ANNOUNCER, G_SoundIndex("sound/rally/race/two.wav") );
        ent->number = 1;
    }
    else if ( level.time > ent->pain_debounce_time + 2000 ){
        RaceCountdown("3", 3);

        Rally_Sound( ent, EV_GLOBAL_SOUND, CHAN_ANNOUNCER, G_SoundIndex("sound/rally/race/three.wav") );
        ent->number = 2;
    }
    else {
        CenterPrint_All("Starting Race...");
    }
}

void CreateRallyStarter( void ) {
    gentity_t *ent;

    ent = G_Spawn();

    ent->think = RallyStarter_Think;
    ent->nextthink = level.time + 2000;
    ent->number = 0;
    ent->classname = "rally_starter";
}

/* --------------------------------------------------------------------------
   SelectLastMarkerForSpawn
   -------------------------------------------------------------------------- */
gentity_t *SelectLastMarkerForSpawn( gentity_t *ent, vec3_t origin, vec3_t angles, qboolean isbot ) {
    gentity_t *spot;
    int lastMarker;

    spot = NULL;
    lastMarker = ent->number - 1;
    if (lastMarker <= 0){
        lastMarker = level.numCheckpoints;
    }

    while ((spot = G_Find (spot, FOFS(classname), "rally_checkpoint")) != NULL) {
        if ( spot->number == lastMarker) {
            break;
        }
    }

    if ( !spot ) {
        return SelectSpawnPoint( vec3_origin, origin, angles, isbot );
    }

    /* spawn at last checkpoint */
    VectorCopy (spot->s.origin, origin);
    VectorCopy (spot->s.angles, angles);

    return spot;
}

/* --------------------------------------------------------------------------
   SelectGridPositionSpawn
   -------------------------------------------------------------------------- */
gentity_t *SelectGridPositionSpawn( gentity_t *ent, vec3_t origin, vec3_t angles, qboolean isbot ) {
    gentity_t *spot;
    int gridPosition;

    spot = NULL;
    gridPosition = 1;
    while ((spot = G_Find (spot, FOFS(classname), "info_player_start")) != NULL) {
        if ( (spot->number == gridPosition || !spot->number) && !SpotWouldTelefrag( spot )) {
            break;
        }
        else if (spot->number == gridPosition){
            spot = NULL; /* found spawn but someone is already there so restart search */
            gridPosition++;
        }
    }

    if ( !spot || SpotWouldTelefrag( spot ) ) {
        /* FIXME: put into spectator mode instead? */
        G_Printf("Warning: No info_player_start found for race spawn, trying info_player_deathmatch");
        return SelectSpawnPoint( vec3_origin, origin, angles, isbot );
    }

    VectorCopy (spot->s.origin, origin);
    origin[2] += 9;
    VectorCopy (spot->s.angles, angles);

    return spot;
}

