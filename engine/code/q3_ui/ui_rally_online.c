/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The server browser drawn like the WebUI's #online-screen (online.js) over the
old arena servers menu. It reads the model the page was fed from
(UI_WebOnlineNumber, UI_WebOnlineChar in ui_rally_servers.c) and acts
through UI_WebOnlineAction, the address actions with ui_webonline_address.
*/

#include "ui_local.h"
#include "ui_rally_online.h"
#include "ui_rally_onlinelayout.h"
#include "ui_rally_offline.h"
#include "ui_rally_library.h"
#include "ui_rally_rivalslayout.h"

void Online_Text( int kind, int index, char *out, int size ) {
	int i;
	for ( i = 0; i < size - 1; i++ ) {
		out[i] = (char)UI_WebOnlineChar( kind, index, i );
		if ( !out[i] ) break;
	}
	out[i] = '\0';
}

static void Online_Quoted( const char *text ) {
	char out[512];
	int n = 0;
	out[n++] = '"';
	for ( ; *text && n < (int)sizeof( out ) - 3; text++ ) {
		if ( *text == '"' || *text == '\\' ) out[n++] = '\\';
		out[n++] = *text;
	}
	out[n++] = '"';
	out[n] = '\0';
	Com_Printf( "%s", out );
}

/* the model as JSON, for tests/fixtures/online_model.json */
void Online_Dump( void ) {
	char text[256];
	int i, j, kind, count;

	Com_Printf( "ONLINEMODEL {\"filters\":[%d,%d,%d,%d,%d,%d],\"scanning\":%d,\"options\":{", UI_WebOnlineNumber( 2, 0 ),
	            UI_WebOnlineNumber( 3, 0 ), UI_WebOnlineNumber( 4, 0 ), UI_WebOnlineNumber( 5, 0 ), UI_WebOnlineNumber( 6, 0 ),
	            UI_WebOnlineNumber( 7, 0 ), UI_WebOnlineNumber( 8, 0 ) );
	for ( kind = 4; kind <= 6; kind++ ) {
		count = kind == 4 ? 8 : kind == 5 ? UI_WebOnlineNumber( 14, 0 ) : 5;
		Com_Printf( "%s\"%d\":[", kind > 4 ? "," : "", kind );
		for ( i = 0; i < count; i++ ) {
			Online_Text( kind, i, text, sizeof( text ) );
			if ( i ) Com_Printf( "," );
			Online_Quoted( text );
		}
		Com_Printf( "]" );
	}
	Com_Printf( "},\"servers\":[" );
	for ( i = 0; i < UI_WebOnlineNumber( 1, 0 ); i++ ) {
		Com_Printf( "%s[", i ? "," : "" );
		for ( j = 0; j < 4; j++ ) {
			Online_Text( j, i, text, sizeof( text ) );
			Online_Quoted( text );
			Com_Printf( "," );
		}
		Com_Printf( "%d,%d,%d,%d,%d]", UI_WebOnlineNumber( 9, i ), UI_WebOnlineNumber( 10, i ), UI_WebOnlineNumber( 11, i ),
		            UI_WebOnlineNumber( 12, i ), UI_WebOnlineNumber( 13, i ) );
	}
	Com_Printf( "]" );
	for ( i = 0; i < 3; i++ ) {
		Online_Text( 7 + i, 0, text, sizeof( text ) );
		Com_Printf( ",\"%s\":", i == 0 ? "status" : i == 1 ? "message" : "selected" );
		Online_Quoted( text );
	}
	Com_Printf( "}\n" );
}

/* checked: 16 x 16, a palette index per pixel */
static const byte onlineCheckedPalette[40][3] = { { 102, 44, 16 }, { 222, 78, 6 }, { 255, 85, 0 }, { 217, 79, 11 }, { 241, 83, 4 }, { 239, 83, 5 }, { 75, 61, 54 }, { 79, 61, 52 }, { 210, 78, 13 }, { 251, 84, 1 }, { 100, 64, 46 }, { 59, 59, 59 }, { 182, 75, 22 }, { 132, 68, 37 }, { 141, 69, 34 }, { 175, 74, 24 }, { 105, 65, 45 }, { 252, 84, 0 }, { 189, 75, 19 }, { 135, 69, 36 }, { 213, 79, 12 }, { 62, 58, 57 }, { 81, 61, 52 }, { 204, 78, 15 }, { 62, 59, 58 }, { 139, 69, 34 }, { 235, 81, 5 }, { 65, 59, 56 }, { 221, 80, 10 }, { 69, 60, 56 }, { 66, 59, 56 }, { 190, 76, 19 }, { 216, 79, 11 }, { 72, 60, 55 }, { 152, 71, 31 }, { 222, 80, 10 }, { 115, 65, 41 }, { 254, 84, 0 }, { 229, 81, 7 }, { 245, 83, 2 } };
static const char *onlineChecked[16] = {
	"0122222222222210",
	"1222222222222221",
	"2222222222234222",
	"2222222222567822",
	"2222222229abbc22",
	"222222222dbbe222",
	"22222222fbbgh222",
	"222ij22klbm42222",
	"22nobpq6brs22222",
	"228tbbubbv222222",
	"222wxbbby2222222",
	"2222z6bAB2222222",
	"22222CgD22222222",
	"2222222222222222",
	"1222222222222221",
	"0122222222222210",
};
/* unchecked: 16 x 16, a palette index per pixel */
static const byte onlineUncheckedPalette[7][3] = { { 25, 25, 25 }, { 101, 101, 101 }, { 111, 111, 111 }, { 133, 133, 133 }, { 99, 99, 99 }, { 59, 59, 59 }, { 112, 112, 112 } };
static const char *onlineUnchecked[16] = {
	"0123333333333210",
	"1455555555555541",
	"6555555555555556",
	"3555555555555553",
	"3555555555555553",
	"3555555555555553",
	"3555555555555553",
	"3555555555555553",
	"3555555555555553",
	"3555555555555553",
	"3555555555555553",
	"3555555555555553",
	"3555555555555553",
	"2555555555555552",
	"1455555555555541",
	"0123333333333210",
};

static const vec4_t onlineInput   = { 0x22 / 255.0f, 0x22 / 255.0f, 0x22 / 255.0f, 1.0f };
static const vec4_t onlineRing    = { 0xee / 255.0f, 0xee / 255.0f, 0xee / 255.0f, 1.0f };
static const vec4_t onlinePressed = { 0x44 / 255.0f, 0x20 / 255.0f, 0x0e / 255.0f, 1.0f };
static const vec4_t onlineMuted   = { 0xaa / 255.0f, 0xaa / 255.0f, 0xaa / 255.0f, 1.0f };
static const vec4_t onlineSmall   = { 0xbb / 255.0f, 0xbb / 255.0f, 0xbb / 255.0f, 1.0f };
static const vec4_t onlineHolder  = { 0x75 / 255.0f, 0x75 / 255.0f, 0x75 / 255.0f, 1.0f };
static const vec4_t onlineBackdrop = { 0.0f, 0.0f, 0.0f, 0xbb / 255.0f };	/* dialog::backdrop #000b */
static const vec4_t onlinePopup   = { 0x1f / 255.0f, 0x1f / 255.0f, 0x1f / 255.0f, 1.0f };
static const vec4_t onlinePopupLine = { 0x3b / 255.0f, 0x3b / 255.0f, 0x3b / 255.0f, 1.0f };

#define ONLINE_MAX_SERVERS	ONLINE_MAX_ROWS

typedef enum { DRAG_NONE, DRAG_LIST, DRAG_PAGE } onlineDrag_t;
typedef enum { FOCUS_NONE, FOCUS_SEARCH, FOCUS_DIRECT } onlineFocus_t;

static struct {
	onlineLayout_t	layout;
	/* the model */
	int				servers, filters[6], options[3];
	qboolean		scanning;
	char			name[ONLINE_MAX_SERVERS][64], address[ONLINE_MAX_SERVERS][64], map[ONLINE_MAX_SERVERS][64], mode[ONLINE_MAX_SERVERS][32];
	char			sub[ONLINE_MAX_SERVERS][160];
	int				visible[ONLINE_MAX_ROWS];			/* server per row */
	char			status[128], message[128], selected[64], count[48];
	char			players[ONLINE_MAX_ROWS][24], ping[ONLINE_MAX_ROWS][16];
	char			detail[ONLINE_DETAILS][64];
	char			shotName[64];
	qhandle_t		shot;
	float			aspect;
	/* the page's own state */
	char			query[64], direct[64], directError[128], removal[64];
	onlineFocus_t	focus;
	int				focusTime;
	int				popup;								/* the open select + 1 */
	float			popupScroll;
	float			listScroll, pageScroll;
	onlineDrag_t	drag;
	float			dragY, dragScroll;
	onlineDialog_t	dialog;
	qboolean		dumped, pickedFavorite;
	int				previewRow, previewDialog;			/* ui_onlinePreview, once the list is there */
} s_online;

static const char *Online_Clean( const char *in, char *out, int size ) {
	int n = 0;
	for ( ; *in && n < size - 1; in++ ) {
		if ( *in == '^' && in[1] >= '0' && in[1] <= '9' ) { in++; continue; }
		out[n++] = *in;
	}
	out[n] = '\0';
	return out;
}

static void Online_Lower( const char *in, char *out, int size ) {
	char clean[256];
	Online_Clean( in, clean, sizeof( clean ) );
	Library_Lower( clean, out, size );
}

/* sends an address action (5 select, 6 connect, 7 favorite, 8 remove, 9 direct) */
static void Online_AddressAction( int action, const char *address ) {
	trap_Cvar_Set( "ui_webonline_address", address );
	UI_WebOnlineAction( action, 0, 0 );
}

/* the model as render() saw it, the list filtered by the search (list()) */
static void Online_Build( void ) {
	onlineLayout_t *l = &s_online.layout;
	char query[64], text[256], lower[256];
	int i, k, picked = -1, humans;

	UI_WebOnlineTick( uis.realtime );
	memset( l, 0, sizeof( *l ) );
	for ( i = 0; i < 6; i++ ) s_online.filters[i] = UI_WebOnlineNumber( 2 + i, 0 );
	s_online.scanning = UI_WebOnlineNumber( 8, 0 ) != 0;
	s_online.servers = s_online.scanning ? 0 : UI_WebOnlineNumber( 1, 0 );
	if ( s_online.servers > ONLINE_MAX_SERVERS ) s_online.servers = ONLINE_MAX_SERVERS;
	s_online.options[0] = 8;
	s_online.options[1] = UI_WebOnlineNumber( 14, 0 );
	s_online.options[2] = 5;
	Online_Text( 7, 0, text, sizeof( text ) );
	Online_Clean( text, s_online.status, sizeof( s_online.status ) );
	if ( s_online.scanning ) Q_strncpyz( s_online.status, s_online.status[0] ? s_online.status : "Searching for servers...", sizeof( s_online.status ) );
	else if ( !s_online.status[0] ) Com_sprintf( s_online.status, sizeof( s_online.status ), "%d servers found", s_online.servers );
	Online_Text( 8, 0, s_online.message, sizeof( s_online.message ) );
	Online_Text( 9, 0, s_online.selected, sizeof( s_online.selected ) );
	if ( s_online.dialog == ONLINE_DIRECT && s_online.message[0] ) Q_strncpyz( s_online.directError, s_online.message, sizeof( s_online.directError ) );

	Library_Lower( s_online.query, query, sizeof( query ) );
	humans = s_online.filters[5];
	for ( i = 0; i < s_online.servers; i++ ) {
		int clients = UI_WebOnlineNumber( 9, i ), max = UI_WebOnlineNumber( 10, i ), ping = UI_WebOnlineNumber( 11, i );
		qboolean favorite = UI_WebOnlineNumber( 13, i ) != 0, match = !query[0];
		Online_Text( 0, i, text, sizeof( text ) );
		Online_Clean( text, s_online.name[i], sizeof( s_online.name[i] ) );
		Online_Text( 1, i, s_online.address[i], sizeof( s_online.address[i] ) );
		Online_Text( 2, i, s_online.map[i], sizeof( s_online.map[i] ) );
		Online_Text( 3, i, s_online.mode[i], sizeof( s_online.mode[i] ) );
		if ( !Q_stricmp( s_online.address[i], s_online.selected ) ) picked = i;
		for ( k = 0; k < 4 && !match; k++ ) {
			Online_Lower( k == 0 ? s_online.name[i] : k == 1 ? s_online.map[i] : k == 2 ? s_online.address[i] : s_online.mode[i], lower, sizeof( lower ) );
			if ( strstr( lower, query ) ) match = qtrue;
		}
		if ( !match || l->rows >= ONLINE_MAX_ROWS ) continue;
		Com_sprintf( s_online.sub[i], sizeof( s_online.sub[i] ), "%s / %s%s", s_online.map[i][0] ? s_online.map[i] : "Unknown map",
		             s_online.mode[i], favorite ? " / Favorite" : "" );
		Com_sprintf( s_online.players[l->rows], sizeof( s_online.players[0] ), "%d / %d", humans ? UI_WebOnlineNumber( 12, i ) : clients, max );
		if ( ping > 0 ) Com_sprintf( s_online.ping[l->rows], sizeof( s_online.ping[0] ), "%d ms", ping );
		else Q_strncpyz( s_online.ping[l->rows], "--", sizeof( s_online.ping[0] ) );
		s_online.visible[l->rows] = i;
		l->rowName[l->rows] = s_online.name[i];
		l->rowSub[l->rows] = s_online.sub[i];
		l->rows++;
	}
	Com_sprintf( s_online.count, sizeof( s_online.count ), "%d / %d servers", l->rows, s_online.servers );
	l->status = s_online.status;
	l->count = s_online.count;
	l->empty = s_online.scanning ? "Looking for servers..." : s_online.servers ? "No servers match your search."
		: "No servers found. Try another source, refresh, or use Direct connect.";
	l->message = s_online.message;
	l->remove = s_online.filters[0] == 7;
	l->picked = picked >= 0;
	if ( picked >= 0 ) {
		int ping = UI_WebOnlineNumber( 11, picked ), humansNow = UI_WebOnlineNumber( 12, picked );
		l->name = s_online.name[picked];
		l->address = s_online.address[picked];
		Q_strncpyz( s_online.detail[0], s_online.mode[picked], sizeof( s_online.detail[0] ) );
		Q_strncpyz( s_online.detail[1], s_online.map[picked][0] ? s_online.map[picked] : "Unknown", sizeof( s_online.detail[1] ) );
		Com_sprintf( s_online.detail[2], sizeof( s_online.detail[2] ), "%d / %d", UI_WebOnlineNumber( 9, picked ), UI_WebOnlineNumber( 10, picked ) );
		/* the page's node() writes nothing for a 0 */
		if ( humansNow ) Com_sprintf( s_online.detail[3], sizeof( s_online.detail[3] ), "%d", humansNow );
		else s_online.detail[3][0] = '\0';
		if ( ping > 0 ) Com_sprintf( s_online.detail[4], sizeof( s_online.detail[4] ), "%d ms", ping );
		else Q_strncpyz( s_online.detail[4], "No response", sizeof( s_online.detail[4] ) );
		for ( i = 0; i < ONLINE_DETAILS; i++ ) l->detail[i] = s_online.detail[i];
		s_online.pickedFavorite = UI_WebOnlineNumber( 13, picked ) != 0;
		l->favorite = s_online.pickedFavorite ? "Saved favorite" : "Save favorite";
		/* the levelshot of the map, when there is one */
		Library_Lower( s_online.map[picked], lower, sizeof( lower ) );
		if ( Q_stricmp( lower, s_online.shotName ) ) {
			Q_strncpyz( s_online.shotName, lower, sizeof( s_online.shotName ) );
			s_online.shot = lower[0] ? trap_R_RegisterShaderNoMip( va( "levelshots/%s", lower ) ) : 0;
			s_online.aspect = s_online.shot ? Offline_ImageAspect( lower ) : 1.0f;
		}
		l->image = s_online.shot != 0;
	} else {
		s_online.pickedFavorite = qfalse;
		l->name = "Choose a server";
		l->address = "Select a server from the list or connect directly.";
		l->favorite = "Save favorite";
	}
	l->dialog = s_online.dialog;
	l->dialogText = s_online.dialog == ONLINE_DIRECT ? s_online.directError : s_online.removal;
	l->listScroll = s_online.listScroll;
	l->pageScroll = s_online.pageScroll;
	Online_Layout( l, WS_ViewportWidth(), WS_ViewportHeight() );
	s_online.listScroll = l->listScroll;
	s_online.pageScroll = l->pageScroll;
}

/* -------------------------------------------------------------------------
   Drawing
   ------------------------------------------------------------------------- */

static void Online_Fill( float x, float y, float w, float h, const float *color, const wsRect_t *clip ) {
	float top = y, bottom = y + h;
	if ( clip ) {
		if ( top < clip->y ) top = clip->y;
		if ( bottom > clip->y + clip->h ) bottom = clip->y + clip->h;
	}
	if ( bottom > top && w > 0.0f ) WS_FillRect( x, top, w, bottom - top, color );
}

static void Online_DrawField( const wsRect_t *r, qboolean focus ) {
	float x0 = WS_Round( r->x ), y0 = WS_Round( r->y ), x1 = WS_Round( r->x + r->w ), y1 = WS_Round( r->y + r->h );
	if ( focus ) WS_FillRect( x0 - 1, y0 - 1, x1 - x0 + 2, y1 - y0 + 2, onlineRing );
	else WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
	WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, onlineInput );
}

/* a select: Chrome's menulist starts its text 4px inside the padding */
static void Online_DrawSelect( const wsRect_t *r, const char *text, qboolean open ) {
	wsTextStyle_t style;
	WS_BodyStyle( &style );
	style.lineHeight = 21.0f;
	Online_DrawField( r, open );
	WS_SetClipX( r->x + 1.0f, r->x + r->w - 20.0f );
	WS_DrawText( &style, r->x + 1.0f + WS_LU( 0.65f * WS_REM ) + 4.0f, r->y + 1.0f + WS_LU( 0.65f * WS_REM ) + ( r->h - 2.0f * WS_LU( 0.65f * WS_REM ) - 23.0f ) * 0.5f,
	             text, strlen( text ), wsWhite );
	WS_SetClipX( 0.0f, 0.0f );
	trap_R_SetColor( wsWhite );
	trap_R_DrawStretchPic( WS_Round( r->x + r->w ) - 14.0f, WS_Round( r->y + r->h * 0.5f ) - 3.0f, 10, 7, 0, 25 / 32.0f, 10 / 16.0f, 1.0f,
	                       trap_R_RegisterShaderNoMip( "menu/ui/web_scrollbar" ) );
	trap_R_SetColor( NULL );
}

static void Online_DrawInput( const wsRect_t *r, const char *text, const char *placeholder, qboolean focus ) {
	wsTextStyle_t body;
	float x = r->x + 1.0f + WS_LU( 0.65f * WS_REM ), y = r->y + 1.0f + WS_LU( 0.65f * WS_REM );
	WS_BodyStyle( &body );
	Online_DrawField( r, focus );
	WS_SetClipX( r->x + 1.0f, r->x + r->w - 1.0f );
	if ( text[0] ) WS_DrawText( &body, x, y, text, strlen( text ), wsWhite );
	else if ( placeholder ) WS_DrawText( &body, x, y, placeholder, strlen( placeholder ), onlineHolder );
	WS_SetClipX( 0.0f, 0.0f );
	if ( focus && ( ( uis.realtime - s_online.focusTime ) / 500 ) % 2 == 0 ) {
		WS_FillRect( WS_Round( x + WS_TextWidth( &body, text, strlen( text ) ) ), WS_Round( y + 3.0f ), 1, 19, wsWhite );
	}
}

/* Chrome's checkbox in the dark scheme with accent-color, as it draws it */
static void Online_DrawCheckbox( float x, float y, qboolean checked ) {
	const char **pixels = checked ? onlineChecked : onlineUnchecked;
	const byte ( *palette )[3] = checked ? onlineCheckedPalette : onlineUncheckedPalette;
	static const char *digits = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i, j, run;
	x = WS_Round( x );
	y = WS_Round( y );
	for ( j = 0; j < 16; j++ ) {
		for ( i = 0; i < 16; i += run ) {
			const char *index = strchr( digits, pixels[j][i] );
			vec4_t color;
			for ( run = 1; i + run < 16 && pixels[j][i + run] == pixels[j][i]; run++ ) {}
			if ( !index ) continue;
			color[0] = palette[index - digits][0] / 255.0f;
			color[1] = palette[index - digits][1] / 255.0f;
			color[2] = palette[index - digits][2] / 255.0f;
			color[3] = 1.0f;
			WS_FillRect( x + i, y + j, run, 1, color );
		}
	}
}

/* a levelshot with object-fit: cover */
static void Online_DrawShot( const wsRect_t *r ) {
	float s0 = 0.0f, s1 = 1.0f, t0 = 0.0f, t1 = 1.0f, box;
	if ( !s_online.shot || r->w <= 0.0f || r->h <= 0.0f ) return;
	box = r->w / r->h;
	if ( box > s_online.aspect ) { t0 = 0.5f - 0.5f * s_online.aspect / box; t1 = 0.5f + 0.5f * s_online.aspect / box; }
	else { s0 = 0.5f - 0.5f * box / s_online.aspect; s1 = 0.5f + 0.5f * box / s_online.aspect; }
	trap_R_SetColor( NULL );
	trap_R_DrawStretchPic( WS_Round( r->x ), WS_Round( r->y ), WS_Round( r->x + r->w ) - WS_Round( r->x ), WS_Round( r->y + r->h ) - WS_Round( r->y ),
	                       s0, t0, s1, t1, s_online.shot );
}

static void Online_SelectText( int select, int index, char *out, int size ) {
	Online_Text( 4 + select, index, out, size );
}

static void Online_PopupRect( wsRect_t *r, int *rows ) {
	const wsRect_t *anchor = &s_online.layout.filterControl[s_online.popup - 1];
	int count = s_online.options[s_online.popup - 1];
	float below = WS_ViewportHeight() - WS_Round( anchor->y + anchor->h ), above = WS_Round( anchor->y );
	*rows = count < 20 ? count : 20;
	r->x = WS_Round( anchor->x );
	r->w = WS_Round( anchor->x + anchor->w ) - r->x;
	if ( *rows * WS_BODY_LINE + 2.0f > below && above > below ) {
		while ( *rows > 1 && *rows * WS_BODY_LINE + 2.0f > above ) ( *rows )--;
		r->h = *rows * WS_BODY_LINE + 2.0f;
		r->y = above - r->h;
	} else {
		while ( *rows > 1 && *rows * WS_BODY_LINE + 2.0f > below ) ( *rows )--;
		r->h = *rows * WS_BODY_LINE + 2.0f;
		r->y = WS_Round( anchor->y + anchor->h );
	}
	s_online.popupScroll = WS_Clamp( 0.0f, s_online.popupScroll, (float)( count - *rows ) );
}

static void Online_DrawPopup( void ) {
	wsTextStyle_t body;
	wsRect_t r;
	char text[64];
	int rows, i, first, current;
	if ( !s_online.popup ) return;
	Online_PopupRect( &r, &rows );
	current = s_online.filters[s_online.popup - 1];
	WS_BodyStyle( &body );
	WS_FillRect( r.x, r.y, r.w, r.h, onlinePopupLine );
	WS_FillRect( r.x + 1, r.y + 1, r.w - 2, r.h - 2, onlinePopup );
	first = (int)s_online.popupScroll;
	for ( i = 0; i < rows; i++ ) {
		if ( first + i == current ) WS_FillRect( r.x + 1, r.y + 1 + i * WS_BODY_LINE, r.w - 2, WS_BODY_LINE, onlinePopupLine );
		Online_SelectText( s_online.popup - 1, first + i, text, sizeof( text ) );
		WS_DrawText( &body, r.x + 1.0f + WS_LU( 0.65f * WS_REM ), r.y + 1 + i * WS_BODY_LINE, text, strlen( text ), wsWhite );
	}
}

static void Online_DrawDialog( void ) {
	onlineLayout_t *l = &s_online.layout;
	wsTextStyle_t body, em;
	wsRect_t *box = &l->dialogBox;
	float x0, y0, x1, y1;

	if ( l->dialog == ONLINE_NO_DIALOG ) return;
	WS_BodyStyle( &body );
	WS_FillRect( 0, 0, WS_ViewportWidth(), WS_ViewportHeight(), onlineBackdrop );
	x0 = WS_Round( box->x ); y0 = WS_Round( box->y ); x1 = WS_Round( box->x + box->w ); y1 = WS_Round( box->y + box->h );
	WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
	WS_FillRect( x0, y0, x1 - x0, 3, wsAccent );
	WS_FillRect( x0 + 1, y0 + 3, x1 - x0 - 2, y1 - y0 - 4, wsSurface );
	em = l->h2;
	em.font = WS_CONDENSED_ITALIC;
	if ( l->dialog == ONLINE_DIRECT ) {
		WS_DrawLabel( l->dialogX, l->dialogLabelY, "Q3RALLY / DIRECT CONNECTION", NULL );
		WS_DrawText( &l->h2, l->dialogX, l->dialogH2Y, "KNOW THE ", 9, wsWhite );
		WS_DrawText( &em, l->dialogX + WS_StyledWidth( &l->h2, "Know the " ), l->dialogH2Y, "ADDRESS?", 8, wsAccent );
		WS_DrawText( &body, l->dialogX, l->dialogFieldLabelY, "Server address", 14, wsWhite );
		Online_DrawInput( &l->dialogInput, s_online.direct, "server.example.com:27960", s_online.focus == FOCUS_DIRECT );
		WS_DrawLines( &body, l->dialogX, l->dialogHelpY, "Hostname, IPv4 or IPv6. The default port is 27960.", &l->dialogHelpLines, wsWhite );
		WS_DrawLines( &body, l->dialogX, l->dialogErrorY, s_online.directError, &l->dialogErrorLines, wsWhite );
		Library_DrawButton( &l->dialogButton[0], "Connect", qfalse, qfalse );
		Library_DrawButton( &l->dialogButton[1], "Cancel", qtrue, qfalse );
	} else {
		WS_DrawText( &l->h2, l->dialogX, l->dialogH2Y, "REMOVE FAVORITE?", 16, wsWhite );
		WS_DrawLines( &body, l->dialogX, l->dialogAddressY, s_online.removal, &l->dialogAddressLines, wsWhite );
		Library_DrawButton( &l->dialogButton[0], "Remove favorite", qfalse, qfalse );
		Library_DrawButton( &l->dialogButton[1], "Keep favorite", qtrue, qfalse );
	}
}

static void Online_Draw( void ) {
	onlineLayout_t *l;
	wsTextStyle_t body, bold, mono, label;
	char text[64];
	int i, picked;

	if ( s_online.drag != DRAG_NONE ) {
		float x, y, track;
		l = &s_online.layout;
		Library_Cursor( &x, &y );
		if ( !trap_Key_IsDown( K_MOUSE1 ) ) s_online.drag = DRAG_NONE;
		else if ( s_online.drag == DRAG_LIST ) {
			track = l->list.h - 2.0f * RIVALS_ARROW - 9.0f - l->listThumb.h;
			if ( track > 0.0f ) s_online.listScroll = s_online.dragScroll + ( y - s_online.dragY ) * ( l->listHeight - l->list.h ) / track;
		} else {
			track = l->viewport.h - 2.0f * RIVALS_ARROW - 9.0f - l->pageThumb.h;
			if ( track > 0.0f ) s_online.pageScroll = s_online.dragScroll + ( y - s_online.dragY ) * ( l->pageHeight - l->viewport.h ) / track;
		}
	}
	Online_Build();
	if ( s_online.previewRow >= 0 && !s_online.scanning && s_online.previewRow < s_online.servers ) {
		Online_AddressAction( 5, s_online.address[s_online.previewRow] );
		if ( s_online.previewDialog == 2 ) {
			Q_strncpyz( s_online.removal, s_online.address[s_online.previewRow], sizeof( s_online.removal ) );
			s_online.dialog = ONLINE_REMOVE;
		}
		s_online.previewRow = -1;
		Online_Build();
	}
	if ( !s_online.dumped && trap_Cvar_VariableValue( "ui_onlineDump" ) && !s_online.scanning && s_online.servers > 0 ) {
		s_online.dumped = qtrue;
		Online_Dump();
	}
	l = &s_online.layout;
	WS_BodyStyle( &body );
	bold = body;
	bold.font = WS_BODY_BOLD;
	WS_LabelStyle( &label );
	mono.font = WS_MONO; mono.size = 0.8f * WS_REM; mono.spacing = 0.0f; mono.lineHeight = 14.0f; mono.uppercase = qfalse;

	WS_DrawBackground( l->page.pageWidth, l->pageHeight );
	WS_DrawFrontHeader( &l->page, "Join the ", "grid.", "" );
	WS_DrawButton( &l->back, "Back to main menu", qtrue, qfalse );

	/* the toolbar */
	WS_DrawSection( &l->toolbar, qfalse );
	for ( i = 0; i < ONLINE_FILTERS; i++ ) {
		WS_DrawLines( &body, l->filterBox[i].x, l->filterBox[i].y + ( l->filterControl[i].y - l->filterBox[i].y - 0.5f * WS_REM - l->filterLines[i].count * WS_BODY_LINE ) * 0.5f,
		              onlineFilterText[i], &l->filterLines[i], wsWhite );
		if ( i < 3 ) {
			Online_SelectText( i, s_online.filters[i], text, sizeof( text ) );
			Online_DrawSelect( &l->filterControl[i], text, s_online.popup == i + 1 );
		} else {
			Online_DrawInput( &l->filterControl[i], s_online.query, "Name, map or address", s_online.focus == FOCUS_SEARCH );
		}
	}
	for ( i = 0; i < ONLINE_TOGGLES; i++ ) {
		Online_DrawCheckbox( l->toggle[i].x + 4.0f, l->toggle[i].y + 4.0f, s_online.filters[3 + i] != 0 );
		WS_DrawText( &body, l->toggle[i].x + 23.0f + 0.5f * WS_REM, l->toggle[i].y, onlineToggleText[i], strlen( onlineToggleText[i] ), wsWhite );
	}
	Library_DrawButton( &l->action[0], onlineActionText[0], qfalse, s_online.scanning );
	Library_DrawButton( &l->action[1], onlineActionText[1], qtrue, !s_online.scanning );
	Library_DrawButton( &l->action[2], onlineActionText[2], qtrue, qfalse );
	Library_DrawButton( &l->action[3], onlineActionText[3], qtrue, qfalse );
	WS_DrawLines( &body, l->innerX, l->statusY, s_online.status, &l->statusLines, wsWhite );

	/* the list */
	WS_DrawSection( &l->listSection, qfalse );
	WS_DrawText( &mono, l->listX, l->headingY, "SERVER / MAP", 12, onlineMuted );
	WS_DrawText( &mono, l->headingPlayersX, l->headingY, "PLAYERS", 7, onlineMuted );
	WS_DrawText( &mono, l->headingPingX, l->headingY, "PING", 4, onlineMuted );
	WS_DrawText( &body, l->listX, l->countY, s_online.count, strlen( s_online.count ), wsWhite );
	if ( !l->rows ) WS_DrawLines( &body, l->list.x, l->list.y + l->emptyY, l->empty, &l->emptyLines, wsWhite );
	picked = -1;
	for ( i = 0; i < l->rows; i++ ) {
		int server = s_online.visible[i];
		float y = l->list.y - l->listScroll + l->rowY[i], h = l->rowH[i], pad = WS_LU( 0.85f * WS_REM ), inner, nameH, x;
		float x0 = WS_Round( l->list.x ), x1 = WS_Round( l->list.x + l->rowWidth ), y0 = WS_Round( y ), y1 = WS_Round( y + h );
		qboolean pressed = !Q_stricmp( s_online.address[server], s_online.selected );
		float players = l->headingPingX - l->headingPlayersX - 0.5f * WS_REM, pingW = l->list.x + l->listW - l->headingPingX;
		if ( y >= l->list.y + l->list.h || y + h <= l->list.y ) continue;
		if ( pressed ) picked = i;
		Online_Fill( x0, y0, x1 - x0, y1 - y0, pressed ? wsAccent : wsLine, &l->list );
		Online_Fill( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, pressed ? onlinePressed : wsSurface, &l->list );
		inner = h - 2.0f - 2.0f * pad;
		nameH = l->nameLines[i].count * WS_BODY_LINE + l->subLines[i].count * l->subStyle.lineHeight;
		WS_SetFade( l->list.y, l->list.h, 0.0f );
		x = l->list.x + 1.0f + pad;
		WS_DrawLines( &bold, x, y + 1.0f + pad + ( inner - nameH ) * 0.5f, l->rowName[i], &l->nameLines[i], wsWhite );
		WS_DrawLines( &l->subStyle, x, y + 1.0f + pad + ( inner - nameH ) * 0.5f + l->nameLines[i].count * WS_BODY_LINE, l->rowSub[i], &l->subLines[i], onlineSmall );
		/* players and ping: right-aligned in their columns (5rem, 3.5rem / 4rem below 48rem) */
		{
			float right = l->list.x + l->rowWidth - 1.0f - pad;
			float pingX = right - pingW, playersX = pingX - 0.5f * WS_REM - players;
			WS_DrawText( &bold, playersX + players - WS_TextWidth( &bold, s_online.players[i], strlen( s_online.players[i] ) ), y + ( h - WS_BODY_LINE ) * 0.5f,
			             s_online.players[i], strlen( s_online.players[i] ), wsWhite );
			WS_DrawText( &bold, right - WS_TextWidth( &bold, s_online.ping[i], strlen( s_online.ping[i] ) ), y + ( h - WS_BODY_LINE ) * 0.5f,
			             s_online.ping[i], strlen( s_online.ping[i] ), wsWhite );
		}
		WS_SetFade( 0.0f, 0.0f, 0.0f );
	}
	if ( l->listBar ) WS_DrawScrollbar( &l->list, &l->listThumb );

	/* the destination */
	WS_DrawSection( &l->detailSection, qtrue );
	WS_DrawLabel( l->detailX, l->labelY, "YOUR DESTINATION", NULL );
	{
		char shown[128];
		WS_Transform( &l->h2, l->name, shown, sizeof( shown ) );
		WS_DrawLines( &l->h2, l->detailX, l->nameY, shown, &l->nameLinesDetail, wsWhite );
	}
	WS_DrawLines( &body, l->detailX, l->addressY, l->address, &l->addressLines, wsWhite );
	if ( l->image ) Online_DrawShot( &l->shot );
	for ( i = 0; i < ONLINE_DETAILS && l->picked; i++ ) {
		char shown[64];
		WS_Transform( &label, onlineDetailText[i], shown, sizeof( shown ) );
		WS_DrawLines( &label, l->card[i].x, l->card[i].y + 0.25f * WS_REM, shown, &l->cardLabelLines[i], wsAccent );
		WS_DrawLines( &body, l->card[i].x, l->cardValueY[i], l->detail[i], &l->cardValueLines[i], wsWhite );
	}
	Library_DrawButton( &l->button[0], "Connect", qfalse, !l->picked );
	Library_DrawButton( &l->button[1], l->favorite, qtrue, !l->picked || !Q_stricmp( l->favorite, "Saved favorite" ) );
	if ( l->remove ) Library_DrawButton( &l->button[2], "Remove favorite", qtrue, !l->picked || !s_online.pickedFavorite );
	WS_DrawLines( &body, l->detailX, l->messageY, s_online.message, &l->messageLines, wsWhite );
	if ( l->pageBar ) WS_DrawScrollbar( &l->viewport, &l->pageThumb );
	Online_DrawPopup();
	Online_DrawDialog();
	( void )picked;
}

/* -------------------------------------------------------------------------
   Input
   ------------------------------------------------------------------------- */

static qboolean Online_ScrollbarClick( const wsRect_t *port, const wsRect_t *thumb, float *scroll, onlineDrag_t drag, float x, float y ) {
	if ( x < port->x + port->w - RIVALS_SCROLLBAR || !Library_Inside( port, x, y ) ) return qfalse;
	if ( y < port->y + RIVALS_ARROW ) *scroll -= 40.0f;
	else if ( y >= port->y + port->h - RIVALS_ARROW ) *scroll += 40.0f;
	else if ( y < thumb->y ) *scroll -= 0.875f * port->h;
	else if ( y >= thumb->y + thumb->h ) *scroll += 0.875f * port->h;
	else {
		s_online.drag = drag;
		s_online.dragY = y;
		s_online.dragScroll = *scroll;
	}
	return qtrue;
}

static void Online_OpenDirect( void ) {
	s_online.dialog = ONLINE_DIRECT;
	s_online.directError[0] = '\0';
	s_online.focus = FOCUS_DIRECT;
	s_online.focusTime = uis.realtime;
	s_online.popup = 0;
}

/* the direct dialog's submit */
static void Online_Direct( void ) {
	char address[64], *start = s_online.direct, *end;
	while ( *start == ' ' ) start++;
	Q_strncpyz( address, start, sizeof( address ) );
	end = address + strlen( address );
	while ( end > address && end[-1] == ' ' ) *--end = '\0';
	if ( !address[0] ) {
		Q_strncpyz( s_online.directError, "Enter a server address.", sizeof( s_online.directError ) );
		return;
	}
	Q_strncpyz( s_online.directError, "Connecting...", sizeof( s_online.directError ) );
	Online_AddressAction( 9, address );
}

static void Online_DialogClick( float x, float y ) {
	onlineLayout_t *l = &s_online.layout;
	if ( l->dialog == ONLINE_DIRECT ) {
		if ( Library_Inside( &l->dialogInput, x, y ) ) { s_online.focus = FOCUS_DIRECT; s_online.focusTime = uis.realtime; return; }
		if ( Library_Inside( &l->dialogButton[0], x, y ) ) { Online_Direct(); return; }
		if ( Library_Inside( &l->dialogButton[1], x, y ) ) { s_online.dialog = ONLINE_NO_DIALOG; s_online.focus = FOCUS_NONE; return; }
		s_online.focus = FOCUS_NONE;
	} else {
		if ( Library_Inside( &l->dialogButton[0], x, y ) ) {
			s_online.dialog = ONLINE_NO_DIALOG;
			if ( s_online.removal[0] ) Online_AddressAction( 8, s_online.removal );
			s_online.removal[0] = '\0';
		} else if ( Library_Inside( &l->dialogButton[1], x, y ) ) {
			s_online.dialog = ONLINE_NO_DIALOG;
			s_online.removal[0] = '\0';
		}
	}
}

static void Online_Click( float x, float y ) {
	onlineLayout_t *l = &s_online.layout;
	int i;

	if ( s_online.popup ) {
		wsRect_t r;
		int rows, item, select = s_online.popup - 1;
		Online_PopupRect( &r, &rows );
		s_online.popup = 0;
		if ( !Library_Inside( &r, x, y ) || y < r.y + 1 || y >= r.y + 1 + rows * WS_BODY_LINE ) return;
		item = (int)s_online.popupScroll + (int)( ( y - r.y - 1 ) / WS_BODY_LINE );
		if ( item < 0 || item >= s_online.options[select] || item == s_online.filters[select] ) return;
		if ( select == 0 ) s_online.listScroll = 0.0f;
		UI_WebOnlineAction( 1 + select, item, 0 );
		return;
	}
	if ( l->dialog != ONLINE_NO_DIALOG ) { Online_DialogClick( x, y ); return; }
	s_online.focus = FOCUS_NONE;
	if ( l->pageBar && Online_ScrollbarClick( &l->viewport, &l->pageThumb, &s_online.pageScroll, DRAG_PAGE, x, y ) ) return;
	if ( Library_Inside( &l->back, x, y ) ) { UI_WebOnlineAction( 0, 0, 0 ); return; }
	for ( i = 0; i < 3; i++ ) {
		if ( Library_Inside( &l->filterControl[i], x, y ) ) {
			s_online.popup = i + 1;
			s_online.popupScroll = s_online.filters[i] - 10;
			return;
		}
	}
	if ( Library_Inside( &l->filterControl[3], x, y ) ) { s_online.focus = FOCUS_SEARCH; s_online.focusTime = uis.realtime; return; }
	for ( i = 0; i < ONLINE_TOGGLES; i++ ) {
		if ( Library_Inside( &l->toggle[i], x, y ) ) { UI_WebOnlineAction( 4, i, s_online.filters[3 + i] ? 0 : 1 ); return; }
	}
	if ( Library_Inside( &l->action[0], x, y ) ) { if ( !s_online.scanning ) UI_WebOnlineAction( 10, 0, 0 ); return; }
	if ( Library_Inside( &l->action[1], x, y ) ) { if ( s_online.scanning ) UI_WebOnlineAction( 11, 0, 0 ); return; }
	if ( Library_Inside( &l->action[2], x, y ) ) { Online_OpenDirect(); return; }
	if ( Library_Inside( &l->action[3], x, y ) ) { UI_WebOnlineAction( 12, 0, 0 ); return; }
	if ( Library_Inside( &l->button[0], x, y ) ) { if ( l->picked ) Online_AddressAction( 6, s_online.selected ); return; }
	if ( Library_Inside( &l->button[1], x, y ) ) { if ( l->picked && !s_online.pickedFavorite ) Online_AddressAction( 7, s_online.selected ); return; }
	if ( l->remove && Library_Inside( &l->button[2], x, y ) ) {
		if ( l->picked && s_online.pickedFavorite ) {
			Q_strncpyz( s_online.removal, s_online.selected, sizeof( s_online.removal ) );
			s_online.dialog = ONLINE_REMOVE;
		}
		return;
	}
	if ( !Library_Inside( &l->list, x, y ) ) return;
	if ( l->listBar && Online_ScrollbarClick( &l->list, &l->listThumb, &s_online.listScroll, DRAG_LIST, x, y ) ) return;
	for ( i = 0; i < l->rows; i++ ) {
		float top = l->list.y - l->listScroll + l->rowY[i];
		if ( y >= top && y < top + l->rowH[i] && x < l->list.x + l->rowWidth ) {
			Online_AddressAction( 5, s_online.address[s_online.visible[i]] );
			return;
		}
	}
}

static void Online_Type( char *buffer, int size, int ch ) {
	int length = strlen( buffer );
	if ( ch == 8 ) { if ( length ) buffer[length - 1] = '\0'; }
	else if ( ch >= 32 && ch < 127 && length < size - 1 ) {
		buffer[length] = (char)ch;
		buffer[length + 1] = '\0';
	}
	s_online.focusTime = uis.realtime;
}

static sfxHandle_t Online_Key( int key ) {
	onlineLayout_t *l = &s_online.layout;
	float x, y;

	Library_Cursor( &x, &y );
	if ( key & K_CHAR_FLAG ) {
		int ch = key & ~K_CHAR_FLAG;
		if ( s_online.focus == FOCUS_SEARCH ) Online_Type( s_online.query, sizeof( s_online.query ), ch );
		else if ( s_online.focus == FOCUS_DIRECT ) Online_Type( s_online.direct, 64, ch );
		return 0;
	}
	switch ( key ) {
	case K_ESCAPE:
	case K_PAD0_B:
		if ( s_online.popup ) s_online.popup = 0;
		else if ( s_online.dialog != ONLINE_NO_DIALOG ) { s_online.dialog = ONLINE_NO_DIALOG; s_online.focus = FOCUS_NONE; s_online.removal[0] = '\0'; }
		else if ( s_online.focus != FOCUS_NONE ) s_online.focus = FOCUS_NONE;
		else UI_WebOnlineAction( 0, 0, 0 );
		return 0;
	case K_MOUSE1:
		Online_Click( x, y );
		return 0;
	case K_MWHEELUP:
	case K_MWHEELDOWN:
		if ( s_online.popup ) s_online.popupScroll += key == K_MWHEELUP ? -3.0f : 3.0f;
		else if ( l->dialog != ONLINE_NO_DIALOG ) return 0;
		else if ( Library_Inside( &l->list, x, y ) && l->listBar ) s_online.listScroll += key == K_MWHEELUP ? -100.0f : 100.0f;
		else s_online.pageScroll += key == K_MWHEELUP ? -100.0f : 100.0f;
		return 0;
	case K_ENTER:
	case K_KP_ENTER:
		if ( s_online.popup ) s_online.popup = 0;
		else if ( l->dialog == ONLINE_DIRECT ) Online_Direct();
		else if ( l->dialog == ONLINE_NO_DIALOG && s_online.focus == FOCUS_NONE && l->picked ) Online_AddressAction( 6, s_online.selected );
		return 0;
	case K_F5:
		if ( !s_online.scanning && l->dialog == ONLINE_NO_DIALOG ) UI_WebOnlineAction( 10, 0, 0 );
		return 0;
	}
	return 0;
}

void Online_Install( menuframework_s *menu ) {
	char preview[96];

	memset( &s_online, 0, sizeof( s_online ) );
	menu->draw = Online_Draw;
	menu->key = Online_Key;
	/* ui_onlinePreview "row,search,list scroll,page scroll,dialog" pins a state for screenshots:
	 * the row to select (-1 none), dialog 1 direct, 2 remove */
	trap_Cvar_VariableStringBuffer( "ui_onlinePreview", preview, sizeof( preview ) );
	if ( preview[0] ) {
		char *field[5];
		int n;
		field[0] = preview;
		for ( n = 1; n < 5; n++ ) {
			field[n] = field[n - 1] ? strchr( field[n - 1], ',' ) : NULL;
			if ( field[n] ) *field[n]++ = '\0';
		}
		if ( field[1] ) Q_strncpyz( s_online.query, field[1], sizeof( s_online.query ) );
		if ( field[2] ) s_online.listScroll = atof( field[2] );
		if ( field[3] ) s_online.pageScroll = atof( field[3] );
		if ( field[4] && atoi( field[4] ) == 1 ) {
			Online_OpenDirect();
			Q_strncpyz( s_online.direct, "abc", sizeof( s_online.direct ) );
			Q_strncpyz( s_online.directError, "Enter a valid server address and port (1-65535).", sizeof( s_online.directError ) );
			s_online.focus = FOCUS_NONE;
		}
		s_online.previewRow = atoi( field[0] );
		s_online.previewDialog = field[4] ? atoi( field[4] ) : 0;
	} else {
		s_online.previewRow = -1;
	}
}
