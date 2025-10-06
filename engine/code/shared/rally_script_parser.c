/*
===========================================================================
Shared implementation for parsing rally scripted object definitions.
===========================================================================
*/

#include "rally_script_parser.h"

#include <stdlib.h>
#include <string.h>

static void RSP_CopyToken( char *dest, size_t destSize, const char *token ) {
    if ( !dest || !destSize ) {
        return;
    }

    if ( token && token[0] ) {
        Q_strncpyz( dest, token, destSize );
    } else {
        dest[0] = '\0';
    }
}

void RSP_InitObjectDef( rallyScriptObjectDef_t *def ) {
    if ( !def ) {
        return;
    }

    memset( def, 0, sizeof( *def ) );
    def->mass = 100;
    def->elasticity = 0.1f;
}

qboolean RSP_LoadScriptText( const char *scriptName,
                            char *buffer, int bufferSize,
                            char *resolvedName, size_t resolvedSize,
                            rsp_fs_open_t fsOpen,
                            rsp_fs_read_t fsRead,
                            rsp_fs_close_t fsClose,
                            rsp_print_t printFn,
                            qboolean developerMode ) {
    fileHandle_t    fileHandle;
    int             len;
    char            filename[MAX_QPATH];

    if ( !buffer || bufferSize <= 0 || !fsOpen || !fsRead || !fsClose ) {
        return qfalse;
    }

    if ( !scriptName || !scriptName[0] ) {
        if ( printFn ) {
            printFn( "No Script file specified\n" );
        }
        return qfalse;
    }

    Q_strncpyz( filename, scriptName, sizeof( filename ) );
    if ( !strchr( filename, '.' ) ) {
        Q_strcat( filename, sizeof( filename ), ".script" );
    }

    if ( developerMode && printFn ) {
        printFn( "Attempting to load script %s\n", filename );
    }

    len = fsOpen( filename, &fileHandle, FS_READ );
    if ( !fileHandle ) {
        if ( printFn ) {
            printFn( "Could not find script %s\n", filename );
        }
        return qfalse;
    }

    if ( len >= bufferSize ) {
        len = bufferSize - 1;
    }

    fsRead( buffer, len, fileHandle );
    buffer[len] = '\0';
    fsClose( fileHandle );

    if ( resolvedName ) {
        Q_strncpyz( resolvedName, filename, resolvedSize );
    }

    return qtrue;
}

static qboolean RSP_ParseGibs( char **text_p, rallyScriptObjectDef_t *outDef, rsp_print_t warningFn, const char *filename ) {
    char    *token;
    int     current = -1;

    token = COM_Parse( text_p );
    if ( !token || !token[0] ) {
        return qfalse;
    }

    if ( Q_stricmp( token, "{" ) ) {
        // nothing to parse, treat as malformed block
        if ( warningFn ) {
            warningFn( "Warning: Malformed gibs block in %s\n", filename );
        }
        return qfalse;
    }

    while ( 1 ) {
        token = COM_Parse( text_p );

        if ( !token || !token[0] ) {
            return qfalse;
        }

        if ( !Q_stricmp( token, "}" ) ) {
            break;
        }

        if ( !Q_stricmp( token, "{" ) ) {
            // skip nested sections entirely
            if ( !RSP_SkipBracedSection( text_p ) ) {
                return qfalse;
            }
            continue;
        }

        if ( !Q_stricmp( token, "model" ) ) {
            token = COM_Parse( text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }

            if ( outDef->numGibs < RSP_MAX_GIBS ) {
                current = outDef->numGibs++;
                RSP_CopyToken( outDef->gibs[current].model, sizeof( outDef->gibs[current].model ), token );
            } else {
                current = -1;
            }
            continue;
        }

        if ( !Q_stricmp( token, "sound" ) ) {
            token = COM_Parse( text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }

            if ( current >= 0 && current < outDef->numGibs ) {
                RSP_CopyToken( outDef->gibs[current].sound, sizeof( outDef->gibs[current].sound ), token );
            }
            continue;
        }

        // Skip unknown token (and let following tokens be processed)
        if ( warningFn ) {
            warningFn( "Warning: Skipping unknown token %s in %s\n", token, filename );
        }
    }

    return qtrue;
}

qboolean RSP_ParseScriptedObject( char *text, const char *filename,
                                  rallyScriptObjectDef_t *outDef,
                                  rsp_print_t warningFn ) {
    char    *text_p;
    char    *token;

    if ( !text || !outDef ) {
        return qfalse;
    }

    RSP_InitObjectDef( outDef );
    if ( filename ) {
        Q_strncpyz( outDef->scriptFile, filename, sizeof( outDef->scriptFile ) );
    }

    text_p = text;
    if ( !RSP_SeekToSection( &text_p, "rally_scripted_object" ) ) {
        if ( warningFn ) {
            warningFn( "Script file '%s' did not contain rally_scripted_object\n", filename ? filename : "<unknown>" );
        }
        return qfalse;
    }

    while ( 1 ) {
        token = COM_Parse( &text_p );

        if ( !token || !token[0] || !Q_stricmp( token, "}" ) ) {
            break;
        }

        if ( !Q_stricmp( token, "{" ) ) {
            continue;
        }

        if ( !Q_stricmp( token, "type" ) ) {
            token = COM_Parse( &text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }
            outDef->type = atoi( token );
            outDef->hasType = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "model" ) ) {
            token = COM_Parse( &text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }
            RSP_CopyToken( outDef->model, sizeof( outDef->model ), token );
            outDef->hasModel = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "deadmodel" ) ) {
            token = COM_Parse( &text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }
            RSP_CopyToken( outDef->deadmodel, sizeof( outDef->deadmodel ), token );
            outDef->hasDeadModel = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "moveable" ) ) {
            token = COM_Parse( &text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }
            outDef->moveable = ( atoi( token ) != 0 );
            outDef->hasMoveable = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "elasticity" ) ) {
            token = COM_Parse( &text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }
            outDef->elasticity = (float)atof( token );
            outDef->hasElasticity = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "mass" ) ) {
            token = COM_Parse( &text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }
            outDef->mass = atoi( token );
            outDef->hasMass = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "frames" ) ) {
            // skip the next two tokens (start and end frames)
            COM_Parse( &text_p );
            COM_Parse( &text_p );
            continue;
        }

        if ( !Q_stricmp( token, "health" ) ) {
            token = COM_Parse( &text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }
            outDef->health = atoi( token );
            outDef->hasHealth = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "mins" ) ) {
            if ( !RSP_ParseTokenVec3( &text_p, outDef->mins ) ) {
                return qfalse;
            }
            outDef->hasMins = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "maxs" ) ) {
            if ( !RSP_ParseTokenVec3( &text_p, outDef->maxs ) ) {
                return qfalse;
            }
            outDef->hasMaxs = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "hitsound" ) ) {
            token = COM_Parse( &text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }
            RSP_CopyToken( outDef->hitSound, sizeof( outDef->hitSound ), token );
            outDef->hasHitSound = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "presound" ) ) {
            token = COM_Parse( &text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }
            RSP_CopyToken( outDef->preSound, sizeof( outDef->preSound ), token );
            outDef->hasPreSound = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "postsound" ) ) {
            token = COM_Parse( &text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }
            RSP_CopyToken( outDef->postSound, sizeof( outDef->postSound ), token );
            outDef->hasPostSound = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "destroysound" ) ) {
            token = COM_Parse( &text_p );
            if ( !token || !token[0] ) {
                return qfalse;
            }
            RSP_CopyToken( outDef->destroySound, sizeof( outDef->destroySound ), token );
            outDef->hasDestroySound = qtrue;
            continue;
        }

        if ( !Q_stricmp( token, "gibs" ) ) {
            if ( !RSP_ParseGibs( &text_p, outDef, warningFn, filename ) ) {
                return qfalse;
            }
            continue;
        }

        if ( warningFn ) {
            warningFn( "Warning: Skipping unknown token %s in %s\n", token, filename ? filename : "<unknown>" );
        }
    }

    return qtrue;
}

qboolean RSP_SeekToSection( char **pointer, const char *section ) {
    char    *token;

    if ( !pointer || !*pointer || !section ) {
        return qfalse;
    }

    while ( 1 ) {
        token = COM_Parse( pointer );

        if ( !token || !token[0] ) {
            return qfalse;
        }

        if ( !Q_stricmp( token, "{" ) ) {
            if ( !RSP_SkipBracedSection( pointer ) ) {
                return qfalse;
            }
            continue;
        }

        if ( !Q_stricmp( token, section ) ) {
            break;
        }
    }

    return qtrue;
}

qboolean RSP_ParseTokenVec3( char **text_p, vec3_t out ) {
    int i;
    char *token;

    if ( !text_p || !out ) {
        return qfalse;
    }

    for ( i = 0; i < 3; i++ ) {
        token = COM_Parse( text_p );
        if ( !token || !token[0] ) {
            return qfalse;
        }

        out[i] = (float)atof( token );
    }

    return qtrue;
}

qboolean RSP_SkipBracedSection( char **text_p ) {
    char    *token;
    int     depth = 0;

    if ( !text_p ) {
        return qfalse;
    }

    while ( 1 ) {
        token = COM_Parse( text_p );

        if ( !token || !token[0] ) {
            return qfalse;
        }

        if ( !Q_stricmp( token, "{" ) ) {
            depth++;
            continue;
        }

        if ( !Q_stricmp( token, "}" ) ) {
            if ( depth == 0 ) {
                break;
            }

            depth--;
            if ( depth < 0 ) {
                depth = 0;
            }
        }
    }

    return qtrue;
}
