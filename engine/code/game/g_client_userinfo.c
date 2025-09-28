#include "g_local.h"
#include "g_client_userinfo.h"

#include <string.h>

qboolean G_SetClientConfigstringIfChanged( int clientNum, const char *playerConfigString ) {
    char currentConfigString[MAX_INFO_STRING];

    trap_GetConfigstring( CS_PLAYERS + clientNum, currentConfigString, sizeof( currentConfigString ) );

    if ( strcmp( currentConfigString, playerConfigString ) == 0 ) {
        return qfalse;
    }

    trap_SetConfigstring( CS_PLAYERS + clientNum, playerConfigString );
    return qtrue;
}
