#include "g_local.h"

#define MAX_GHOST_RECORDS_PER_MAP 32
#define GHOST_FILE_EXTENSION ".ghost"
#define GHOST_DIRECTORY "ghosts"
#define MAX_GHOST_FILE_SIZE ( 2 * 1024 * 1024 )

static ghostRecord_t s_levelGhosts[MAX_GHOST_RECORDS_PER_MAP];
static int s_levelGhostCount = 0;
static ghostBotRoute_t s_botRoute;

static int G_Ghost_Strlen( const char *text ) {
    int len = 0;

    if ( !text ) {
        return 0;
    }

    while ( text[len] ) {
        ++len;
    }

    return len;
}

static int G_Ghost_ParseInt( const char *text ) {
    int value = 0;

    if ( !text ) {
        return 0;
    }

    while ( *text == ' ' || *text == '\t' ) {
        ++text;
    }

    while ( *text >= '0' && *text <= '9' ) {
        value = value * 10 + ( *text - '0' );
        ++text;
    }

    return value;
}

static void G_Ghost_Reset( void ) {
    Com_Memset( s_levelGhosts, 0, sizeof( s_levelGhosts ) );
    Com_Memset( &s_botRoute, 0, sizeof( s_botRoute ) );
    s_levelGhostCount = 0;
}

static char *G_Ghost_NextLine( char **cursor ) {
    char *line;
    char *end;

    if ( !cursor || !*cursor || !( **cursor ) ) {
        return NULL;
    }

    line = *cursor;

    while ( *line == ' ' || *line == '\t' ) {
        ++line;
    }

    if ( line[0] == '\xEF' && line[1] == '\xBB' && line[2] == '\xBF' ) {
        line += 3;
    }

    end = line;
    while ( *end && *end != '\n' && *end != '\r' ) {
        ++end;
    }

    if ( *end ) {
        char saved = *end;
        *end = '\0';
        *cursor = end + 1;
        if ( saved == '\r' && **cursor == '\n' ) {
            ++( *cursor );
        }
    } else {
        *cursor = end;
    }

    return line;
}

static qboolean G_Ghost_ParseHeader( char *buffer, const char *expectedMap, ghostRecord_t *outRecord ) {
    char *cursor;
    char *line;
    char mapName[MAX_QPATH] = "";
    qboolean hasTime = qfalse;

    if ( !buffer || !outRecord ) {
        return qfalse;
    }

    cursor = buffer;
    while ( ( line = G_Ghost_NextLine( &cursor ) ) != NULL ) {
        if ( line[0] == '#' || line[0] == '\0' ) {
            continue;
        }

        if ( !Q_stricmpn( line, "map", 3 ) ) {
            const char *value = line + 3;
            while ( *value == ' ' || *value == '\t' ) {
                ++value;
            }
            Q_strncpyz( mapName, value, sizeof( mapName ) );
        } else if ( !Q_stricmpn( line, "vehicle", 7 ) ) {
            const char *value = line + 7;
            while ( *value == ' ' || *value == '\t' ) {
                ++value;
            }
            Q_strncpyz( outRecord->vehicleClass, value, sizeof( outRecord->vehicleClass ) );
        } else if ( !Q_stricmpn( line, "best_time_ms", 12 ) ) {
            const char *value = line + 12;
            while ( *value == ' ' || *value == '\t' ) {
                ++value;
            }
            outRecord->bestTimeMs = G_Ghost_ParseInt( value );
            hasTime = outRecord->bestTimeMs > 0;
        } else if ( !Q_stricmpn( line, "frames", 6 ) ) {
            break;
        }
    }

    if ( expectedMap && expectedMap[0] && mapName[0] && Q_stricmp( expectedMap, mapName ) ) {
        return qfalse;
    }

    return hasTime;
}

static qboolean G_Ghost_LoadBotRouteFromFile( const ghostRecord_t *record, ghostBotRoute_t *outRoute ) {
    fileHandle_t f;
    int length;
    static char buffer[MAX_GHOST_FILE_SIZE + 1];
    char *cursor;
    char *line;
    char mapName[MAX_QPATH] = "";

    if ( !record || !record->path[0] || !outRoute ) {
        return qfalse;
    }

    length = trap_FS_FOpenFile( record->path, &f, FS_READ );
    if ( length <= 0 ) {
        G_Printf( "G_Ghost: could not open %s for bot route\n", record->path );
        return qfalse;
    }

    if ( length > MAX_GHOST_FILE_SIZE ) {
        trap_FS_FCloseFile( f );
        G_Printf( "G_Ghost: %s too large for bot route (%d bytes)\n", record->path, length );
        return qfalse;
    }

    trap_FS_Read( buffer, length, f );
    trap_FS_FCloseFile( f );
    buffer[length] = '\0';

    Com_Memset( outRoute, 0, sizeof( *outRoute ) );
    Q_strncpyz( outRoute->path, record->path, sizeof( outRoute->path ) );

    cursor = buffer;
    while ( ( line = G_Ghost_NextLine( &cursor ) ) != NULL ) {
        if ( line[0] == '#' || line[0] == '\0' ) {
            continue;
        }

        if ( !Q_stricmpn( line, "map", 3 ) ) {
            const char *value = line + 3;
            while ( *value == ' ' || *value == '\t' ) {
                ++value;
            }
            Q_strncpyz( mapName, value, sizeof( mapName ) );
            continue;
        }

        if ( !Q_stricmpn( line, "vehicle", 7 ) ) {
            const char *value = line + 7;
            while ( *value == ' ' || *value == '\t' ) {
                ++value;
            }
            Q_strncpyz( outRoute->vehicleClass, value, sizeof( outRoute->vehicleClass ) );
            continue;
        }

        if ( !Q_stricmpn( line, "best_time_ms", 12 ) ) {
            const char *value = line + 12;
            while ( *value == ' ' || *value == '\t' ) {
                ++value;
            }
            outRoute->bestTimeMs = G_Ghost_ParseInt( value );
            continue;
        }

        if ( !Q_stricmpn( line, "frames", 6 ) ) {
            continue;
        }

        if ( outRoute->numWaypoints < MAX_GHOST_BOT_WAYPOINTS ) {
            ghostWaypoint_t *wp = &outRoute->waypoints[outRoute->numWaypoints];
            float ox, oy, oz;
            int parsed;

            parsed = sscanf( line, "%d %f %f %f", &wp->timeOffset, &ox, &oy, &oz );
            if ( parsed == 4 ) {
                wp->origin[0] = ox;
                wp->origin[1] = oy;
                wp->origin[2] = oz;
                outRoute->numWaypoints++;
            }
        }
    }

    if ( outRoute->numWaypoints < 2 ) {
        G_Printf( "G_Ghost: %s has no usable bot waypoints\n", record->path );
        Com_Memset( outRoute, 0, sizeof( *outRoute ) );
        return qfalse;
    }

    outRoute->valid = qtrue;
    G_Printf( "G_Ghost: Bot route ready from %s (%d waypoints, vehicle=%s, map=%s)\n",
        record->path,
        outRoute->numWaypoints,
        outRoute->vehicleClass[0] ? outRoute->vehicleClass : "any",
        mapName[0] ? mapName : "unknown" );

    return qtrue;
}

void G_Ghost_InitForMap( const char *mapname ) {
    char fileList[2048];
    int fileCount;
    int offset;
    int i;

    G_Ghost_Reset();

    if ( !mapname || !mapname[0] ) {
        G_Printf( "G_Ghost: No mapname provided, skipping ghost discovery\n" );
        return;
    }

    fileCount = trap_FS_GetFileList( GHOST_DIRECTORY, GHOST_FILE_EXTENSION, fileList, sizeof( fileList ) );
    if ( fileCount <= 0 ) {
        G_Printf( "G_Ghost: No ghost files found for map %s\n", mapname );
        return;
    }

    offset = 0;
    for ( i = 0; i < fileCount && s_levelGhostCount < MAX_GHOST_RECORDS_PER_MAP; i++ ) {
        const char *filename = fileList + offset;
        char cleanName[MAX_QPATH];
        fileHandle_t f;
        int length;
        char buffer[16 * 1024 + 1];

        offset += G_Ghost_Strlen( filename ) + 1;

        if ( !filename[0] ) {
            continue;
        }

        Q_strncpyz( cleanName, filename, sizeof( cleanName ) );
        COM_StripExtension( cleanName, cleanName, sizeof( cleanName ) );
        if ( Q_stricmp( cleanName, mapname ) &&
             ( Q_stricmpn( cleanName, mapname, G_Ghost_Strlen( mapname ) ) || cleanName[G_Ghost_Strlen( mapname )] != '_' ) ) {
            continue;
        }

        length = trap_FS_FOpenFile( va( "%s/%s", GHOST_DIRECTORY, filename ), &f, FS_READ );
        if ( length <= 0 ) {
            continue;
        }

        if ( length >= sizeof( buffer ) ) {
            trap_FS_FCloseFile( f );
            continue;
        }

        trap_FS_Read( buffer, length, f );
        buffer[length] = '\0';
        trap_FS_FCloseFile( f );

        if ( G_Ghost_ParseHeader( buffer, mapname, &s_levelGhosts[s_levelGhostCount] ) ) {
            Q_strncpyz( s_levelGhosts[s_levelGhostCount].path, va( "%s/%s", GHOST_DIRECTORY, filename ), sizeof( s_levelGhosts[s_levelGhostCount].path ) );
            ++s_levelGhostCount;
        }
    }

    if ( s_levelGhostCount == 0 ) {
        G_Printf( "G_Ghost: No matching ghost files for map %s\n", mapname );
    } else {
        const ghostRecord_t *best;

        G_Printf( "G_Ghost: Loaded %d ghost record(s) for %s\n", s_levelGhostCount, mapname );

        best = G_Ghost_FindBestRecord();
        if ( best && G_Ghost_LoadBotRouteFromFile( best, &s_botRoute ) ) {
            G_Printf( "G_Ghost: Bot route source set to %s\n", s_botRoute.path );
        } else {
            G_Printf( "G_Ghost: Bot route unavailable for map %s\n", mapname );
        }
    }
}

const ghostRecord_t *G_Ghost_FindBestRecord( void ) {
    int i;
    const ghostRecord_t *best = NULL;

    for ( i = 0; i < s_levelGhostCount; ++i ) {
        if ( !best || ( s_levelGhosts[i].bestTimeMs > 0 && s_levelGhosts[i].bestTimeMs < best->bestTimeMs ) ) {
            best = &s_levelGhosts[i];
        }
    }

    return best;
}

<<<<<<< codex/implement-racing-bot-waypoint-system-using-ghost-files-ju0s9c
qboolean G_Ghost_GetBotRoute( const ghostBotRoute_t **outRoute ) {
=======
qboolean G_Ghost_GetBotRoute( ghostBotRoute_t *outRoute ) {
>>>>>>> master
    if ( !outRoute || !s_botRoute.valid ) {
        return qfalse;
    }

<<<<<<< codex/implement-racing-bot-waypoint-system-using-ghost-files-ju0s9c
    *outRoute = &s_botRoute;
=======
    *outRoute = s_botRoute;
>>>>>>> master
    return qtrue;
}

void G_Ghost_AnnounceForClient( gentity_t *ent ) {
    const ghostRecord_t *record;

    if ( !ent || !ent->client || ent->client->pers.connected != CON_CONNECTED ) {
        return;
    }

    record = G_Ghost_FindBestRecord();

    if ( record ) {
        trap_SendServerCommand( ent - g_entities, va( "ghostmeta %s %d %s", record->vehicleClass[0] ? record->vehicleClass : "any", record->bestTimeMs, record->path ) );
    } else {
        trap_SendServerCommand( ent - g_entities, "ghostmeta none 0" );
    }
}
