/*
===========================================================================
Shared utilities for parsing rally scripted object definitions.
===========================================================================
*/

#ifndef RALLY_SCRIPT_PARSER_H
#define RALLY_SCRIPT_PARSER_H

#include <stddef.h>

#include "../qcommon/q_shared.h"

#define RSP_MAX_SCRIPT_TEXT     8192

#ifndef RSP_MAX_GIBS
#define RSP_MAX_GIBS            5
#endif

typedef struct {
    char    model[MAX_QPATH];
    char    sound[MAX_QPATH];
} rallyScriptGibDef_t;

typedef struct {
    char    scriptFile[MAX_QPATH];

    int     type;
    qboolean        hasType;

    qboolean        moveable;
    qboolean        hasMoveable;

    float   elasticity;
    qboolean        hasElasticity;

    int     mass;
    qboolean        hasMass;

    int     health;
    qboolean        hasHealth;

    vec3_t  mins;
    qboolean        hasMins;

    vec3_t  maxs;
    qboolean        hasMaxs;

    char    model[MAX_QPATH];
    qboolean        hasModel;

    char    deadmodel[MAX_QPATH];
    qboolean        hasDeadModel;

    char    hitSound[MAX_QPATH];
    qboolean        hasHitSound;

    char    preSound[MAX_QPATH];
    qboolean        hasPreSound;

    char    postSound[MAX_QPATH];
    qboolean        hasPostSound;

    char    destroySound[MAX_QPATH];
    qboolean        hasDestroySound;

    int     numGibs;
    rallyScriptGibDef_t gibs[RSP_MAX_GIBS];
} rallyScriptObjectDef_t;

typedef void (QDECL *rsp_print_t)( const char *fmt, ... );
typedef int (*rsp_fs_open_t)( const char *qpath, fileHandle_t *f, fsMode_t mode );
typedef void (*rsp_fs_read_t)( void *buffer, int len, fileHandle_t f );
typedef void (*rsp_fs_close_t)( fileHandle_t f );

void RSP_InitObjectDef( rallyScriptObjectDef_t *def );

qboolean RSP_LoadScriptText( const char *scriptName,
                            char *buffer, int bufferSize,
                            char *resolvedName, size_t resolvedSize,
                            rsp_fs_open_t fsOpen,
                            rsp_fs_read_t fsRead,
                            rsp_fs_close_t fsClose,
                            rsp_print_t printFn,
                            qboolean developerMode );

qboolean RSP_ParseScriptedObject( char *text, const char *filename,
                                  rallyScriptObjectDef_t *outDef,
                                  rsp_print_t warningFn );

qboolean RSP_SeekToSection( char **pointer, const char *section );
qboolean RSP_ParseTokenVec3( char **text_p, vec3_t out );
qboolean RSP_SkipBracedSection( char **text_p );

#endif /* RALLY_SCRIPT_PARSER_H */
