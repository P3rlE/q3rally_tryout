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
//
#include "ui_local.h"
#include "ui_rally_maploadlayout.h"

/*
===============================================================================

CONNECTION SCREEN

===============================================================================
*/

qboolean	passwordNeeded = qtrue;
menufield_s passwordField;

/* the map loading screen of the WebUI (#map-loading-screen, map-loading.js) */
static const vec4_t mapLoadSurface = { 0x16 / 255.0f, 0x16 / 255.0f, 0x16 / 255.0f, 1.0f };
static const vec4_t mapLoadLine    = { 0x33 / 255.0f, 0x33 / 255.0f, 0x33 / 255.0f, 1.0f };
static const vec4_t mapLoadAccent  = { 1.0f, 0x5a / 255.0f, 0.0f, 1.0f };

static qhandle_t connectMapShader;
static char connectMapShaderName[MAX_QPATH];

/* the levelshot, or nothing: the page hides the image without one */
static qhandle_t UI_ConnectMapShader( const char *mapname ) {
	char imageName[MAX_QPATH];

	if ( !mapname || !mapname[0] ) return 0;
	Com_sprintf( imageName, sizeof( imageName ), "levelshots/%s", mapname );
	if ( Q_stricmp( imageName, connectMapShaderName ) ) {
		Q_strncpyz( connectMapShaderName, imageName, sizeof( connectMapShaderName ) );
		connectMapShader = trap_R_RegisterShaderNoMip( imageName );
	}
	return connectMapShader;
}

/* clean(): color codes out */
static void UI_ConnectClean( const char *in, char *out, int size ) {
	int n = 0;
	for ( ; *in && n < size - 1; in++ ) {
		if ( *in == '^' && in[1] >= '0' && in[1] <= '9' ) { in++; continue; }
		out[n++] = *in;
	}
	out[n] = '\0';
}

static void UI_ConnectSection( const wsRect_t *box ) {
	float x0 = WS_Round( box->x ), y0 = WS_Round( box->y ), x1 = WS_Round( box->x + box->w ), y1 = WS_Round( box->y + box->h );
	WS_FillRect( x0, y0, x1 - x0, y1 - y0, mapLoadLine );
	WS_FillRect( x0, y0, x1 - x0, 2, mapLoadAccent );
	WS_FillRect( x0 + 1, y0 + 2, x1 - x0 - 2, y1 - y0 - 3, mapLoadSurface );
}

/*
========================
UI_DrawConnectScreen

Connecting, loading and the downloads in between, drawn like the WebUI's
map loading screen. During loading this is drawn over the cgame's own
loading screen.
========================
*/
void UI_DrawConnectScreen( qboolean overlay ) {
	mapLoadLayout_t l;
	uiClientState_t cstate;
	wsTextStyle_t label, body, h1, h1em;
	char info[MAX_INFO_STRING], map[MAX_QPATH], title[MAX_INFO_VALUE], detail[128], download[MAX_QPATH];
	char shown[160];
	qhandle_t shot;
	int count, total;

	Menu_Cache();
	trap_GetClientState( &cstate );
	memset( &l, 0, sizeof( l ) );

	/* Web_PublishMapLoading(): map, its title, server, state, download */
	map[0] = title[0] = info[0] = '\0';
	if ( trap_GetConfigString( CS_SERVERINFO, info, sizeof( info ) ) ) {
		Q_strncpyz( map, Info_ValueForKey( info, "mapname" ), sizeof( map ) );
	}
	trap_GetConfigString( CS_MESSAGE, title, sizeof( title ) );
	trap_Cvar_VariableStringBuffer( "cl_downloadName", download, sizeof( download ) );
	count = (int)trap_Cvar_VariableValue( "cl_downloadCount" );
	total = (int)trap_Cvar_VariableValue( "cl_downloadSize" );
	switch ( cstate.connState ) {
	case CA_CONNECTING:  Q_strncpyz( detail, "Awaiting challenge", sizeof( detail ) ); break;
	case CA_CHALLENGING: Q_strncpyz( detail, "Awaiting connection", sizeof( detail ) ); break;
	case CA_CONNECTED:   Q_strncpyz( detail, "Receiving game state", sizeof( detail ) ); break;
	case CA_LOADING:     trap_Cvar_VariableStringBuffer( "cl_mapLoadingDetail", detail, sizeof( detail ) ); break;
	default:             Q_strncpyz( detail, "Starting race", sizeof( detail ) ); break;
	}

	/* state(): what the page shows of it */
	UI_ConnectClean( title[0] ? title : map[0] ? map : "Preparing your race", l.name, sizeof( l.name ) );
	if ( !Q_stricmp( cstate.servername, "localhost" ) ) Q_strncpyz( l.server, "Local race", sizeof( l.server ) );
	else UI_ConnectClean( cstate.servername, l.server, sizeof( l.server ) );
	if ( download[0] ) {
		UI_ConnectClean( download, shown, sizeof( shown ) );
		Com_sprintf( l.detail, sizeof( l.detail ), "Downloading %s", shown );
		if ( total > 0 ) {
			Com_sprintf( l.transfer, sizeof( l.transfer ), "%d%% \xb7 %.1f / %.1f MB",
			             (int)WS_Clamp( 0.0f, count * 100.0f / total, 100.0f ), count / 1048576.0f, total / 1048576.0f );
		} else {
			Com_sprintf( l.transfer, sizeof( l.transfer ), "%.1f MB received", count / 1048576.0f );
		}
	} else {
		UI_ConnectClean( detail, l.detail, sizeof( l.detail ) );
		if ( !l.detail[0] ) Q_strncpyz( l.detail, "Preparing track resources", sizeof( l.detail ) );
		if ( cstate.connectPacketCount > 0 && strstr( detail, "Awaiting" ) ) {
			Com_sprintf( l.transfer, sizeof( l.transfer ), "Connection attempt %d", cstate.connectPacketCount );
		}
	}
	shot = UI_ConnectMapShader( map );
	l.image = shot != 0;
	MapLoad_Layout( &l, WS_ViewportWidth(), WS_ViewportHeight() );

	WS_LabelStyle( &label );
	WS_BodyStyle( &body );
	h1.font = WS_CONDENSED; h1.size = l.h1Size; h1.spacing = 0.0f; h1.lineHeight = l.h1Size; h1.uppercase = qtrue;
	h1em = h1; h1em.font = WS_CONDENSED_ITALIC;

	WS_DrawBackground( l.pageWidth, l.pageHeight );
	WS_DrawLabel( l.contentX, l.labelY, "Q3RALLY / NEXT STAGE", NULL );
	WS_Transform( &h1, "Ready to ", shown, sizeof( shown ) );
	WS_DrawText( &h1, l.h1X, l.h1Y, shown, strlen( shown ), wsWhite );
	WS_DrawText( &h1em, l.h1X + WS_StyledWidth( &h1, "Ready to " ), l.h1Y, "RACE.", 5, wsAccent );

	/* the track */
	UI_ConnectSection( &l.preview );
	if ( shot ) {
		/* object-fit: cover, 16:9 from the square levelshot */
		trap_R_SetColor( NULL );
		trap_R_DrawStretchPic( WS_Round( l.shot.x ), WS_Round( l.shot.y ), WS_Round( l.shot.w ), WS_Round( l.shot.h ),
		                       0.0f, 0.21875f, 1.0f, 0.78125f, shot );
	}
	WS_Transform( &l.title, l.name, shown, sizeof( shown ) );
	WS_DrawLines( &l.title, l.previewX, l.nameY, shown, &l.nameLines, wsWhite );
	if ( l.server[0] ) WS_DrawText( &body, l.previewX, l.serverY, l.server, strlen( l.server ), wsWhite );

	/* the connection */
	UI_ConnectSection( &l.status );
	WS_DrawLabel( l.statusX, l.statusLabelY, "CONNECTION / LOADING", NULL );
	WS_Transform( &l.title, l.detail, shown, sizeof( shown ) );
	WS_DrawLines( &l.title, l.statusX, l.detailY, shown, &l.detailLines, wsWhite );
	if ( download[0] && total > 0 ) {
		WS_DrawProgress( l.progress.x, l.progress.y, l.progress.w, WS_Clamp( 0.0f, (float)count / total, 1.0f ) );
	} else {
		/* no value: Chrome's indeterminate bar, a piece of fill moving across */
		float phase = ( uis.realtime % 2000 ) / 2000.0f;
		WS_DrawProgressRange( l.progress.x, l.progress.y, l.progress.w, phase * 1.2f - 0.2f, phase * 1.2f );
	}
	if ( l.transfer[0] ) WS_DrawLines( &body, l.statusX, l.transferY, l.transfer, &l.transferLines, wsWhite );
	WS_DrawLines( &body, l.statusX, l.noteY, mapLoadNote, &l.noteLines, wsWhite );
	WS_DrawButton( &l.cancel, "Cancel", qtrue, qfalse );	/* Escape cancels (UI_KeyConnect) */
}


/*
===================
UI_KeyConnect
===================
*/
void UI_KeyConnect( int key ) {
	if ( key == K_ESCAPE ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
		return;
	}
}
