/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The offline race setup drawn like the WebUI's #offline-screen (offline.js),
on the model the page was fed from: UI_WebOfflineNumber, UI_WebOfflineChar
and UI_WebOfflineAction in ui_rally_startserver.c. Step 1, track and mode;
step 2 is ui_rally_offlinerules.c.
*/

#include "ui_local.h"
#include "ui_rally_offline.h"

/* a text of the model, as Web_OfflineText reads it */
void Offline_Text( int kind, int index, int choice, char *out, int size ) {
	int i, c;
	for ( i = 0; i < size - 1; i++ ) {
		c = UI_WebOfflineChar( kind, index, choice, i );
		if ( !c ) break;
		out[i] = (char)c;
	}
	out[i] = '\0';
}

static void Offline_Quoted( const char *text ) {
	char out[512];
	int n = 0;
	for ( ; *text && n < (int)sizeof( out ) - 3; text++ ) {
		if ( *text == '"' || *text == '\\' ) out[n++] = '\\';
		out[n++] = *text;
	}
	out[n] = '\0';
	Com_Printf( "\"%s\"", out );
}

/* ui_offlineDump 1: the model as one JSON line, to feed the WebUI the same
 * data when measuring it (tools/ui_webstyle/measure_webui_offline.cjs) */
void Offline_Dump( void ) {
	char text[256];
	int i;

	Com_Printf( "OFFLINEMODEL {\"mode\":%d,\"selected\":%d,\"hosting\":%d,\"modes\":[", UI_WebOfflineNumber( 1, 0 ),
	            UI_WebOfflineNumber( 2, 0 ), UI_WebOfflineNumber( 14, 0 ) );
	for ( i = 0; i < UI_WebOfflineNumber( 16, 0 ); i++ ) {
		Offline_Text( 3, i, 0, text, sizeof( text ) );
		if ( i ) Com_Printf( "," );
		Offline_Quoted( text );
	}
	Com_Printf( "],\"maps\":[" );
	for ( i = 0; i < UI_WebOfflineNumber( 0, 0 ); i++ ) {
		if ( i ) Com_Printf( "," );
		Com_Printf( "[" );
		Offline_Text( 0, i, 0, text, sizeof( text ) );
		Offline_Quoted( text );
		Com_Printf( "," );
		Offline_Text( 1, i, 0, text, sizeof( text ) );
		Offline_Quoted( text );
		Com_Printf( "]" );
	}
	Com_Printf( "],\"details\":[" );
	for ( i = 0; i < UI_WebOfflineNumber( 11, 0 ); i++ ) {
		Offline_Text( 2, i, 0, text, sizeof( text ) );
		if ( i ) Com_Printf( "," );
		Offline_Quoted( text );
	}
	Com_Printf( "]" );
	if ( UI_WebOfflineScreen() == 10 ) {
		int j, count;
		Com_Printf( ",\"allowsBots\":%d,\"dedicated\":%d,\"avatar\":", UI_WebOfflineNumber( 12, 0 ), UI_WebOfflineNumber( 15, 0 ) );
		Offline_Text( 10, 0, 0, text, sizeof( text ) );
		Offline_Quoted( text );
		Com_Printf( ",\"controls\":[" );
		for ( i = 0, count = 0; i < UI_WebOfflineNumber( 3, 0 ); i++ ) {
			if ( !UI_WebOfflineNumber( 4, i ) ) continue;
			Com_Printf( "%s[%d,%d,", count++ ? "," : "", i, UI_WebOfflineNumber( 4, i ) );
			Offline_Text( 4, i, 0, text, sizeof( text ) );
			Offline_Quoted( text );
			Com_Printf( "," );
			Offline_Text( 5, i, 0, text, sizeof( text ) );
			Offline_Quoted( text );
			Com_Printf( ",%d,%d,%d,%d,%d,[", UI_WebOfflineNumber( 5, i ), UI_WebOfflineNumber( 7, i ), UI_WebOfflineNumber( 9, i ),
			            UI_WebOfflineNumber( 10, i ), UI_WebOfflineNumber( 13, i ) );
			for ( j = 0; j < UI_WebOfflineNumber( 6, i ); j++ ) {
				Offline_Text( 6, i, j, text, sizeof( text ) );
				if ( j ) Com_Printf( "," );
				Offline_Quoted( text );
			}
			Com_Printf( "]]" );
		}
		Com_Printf( "],\"players\":[" );
		for ( i = 0; i < 12; i++ ) {
			Offline_Text( 8, i, 0, text, sizeof( text ) );
			if ( i ) Com_Printf( "," );
			Offline_Quoted( text );
		}
		Com_Printf( "],\"bots\":[" );
		for ( i = 0; i < UI_WebOfflineNumber( 8, 0 ); i++ ) {
			if ( i ) Com_Printf( "," );
			Com_Printf( "[" );
			Offline_Text( 7, i, 0, text, sizeof( text ) );
			Offline_Quoted( text );
			Com_Printf( "," );
			Offline_Text( 9, i, 0, text, sizeof( text ) );
			Offline_Quoted( text );
			Com_Printf( "]" );
		}
		Com_Printf( "]" );
	}
	Com_Printf( "}\n" );
}

/* -------------------------------------------------------------------------
   Step 1: track and mode
   ------------------------------------------------------------------------- */

#include "ui_rally_offlinelayout.h"
#include "ui_rally_library.h"

static const vec4_t offlineInput   = { 0x22 / 255.0f, 0x22 / 255.0f, 0x22 / 255.0f, 1.0f };
static const vec4_t offlinePressed = { 0x44 / 255.0f, 0x20 / 255.0f, 0x0e / 255.0f, 1.0f };	/* aria-pressed */
static const vec4_t offlineMuted   = { 0xaa / 255.0f, 0xaa / 255.0f, 0xaa / 255.0f, 1.0f };
static const vec4_t offlineHolder  = { 0x75 / 255.0f, 0x75 / 255.0f, 0x75 / 255.0f, 1.0f };
static const vec4_t offlineRing    = { 0xee / 255.0f, 0xee / 255.0f, 0xee / 255.0f, 1.0f };
static const vec4_t offlinePopup   = { 0x1f / 255.0f, 0x1f / 255.0f, 0x1f / 255.0f, 1.0f };
static const vec4_t offlinePopupLine = { 0x3b / 255.0f, 0x3b / 255.0f, 0x3b / 255.0f, 1.0f };

#define OFFLINE_MAX_MAPS	64

static struct {
	offlineLayout_t	layout;
	char			query[64];			/* #offline-search */
	qboolean		searchFocus;
	int				searchFocusTime;
	qboolean		modeOpen;			/* the select's option list */
	float			scroll, pageScroll;
	qboolean		dragThumb, dragPage;
	float			dragY, dragScroll;
	int				rows[OFFLINE_MAX_ROWS];	/* map index per row */
	char			ids[OFFLINE_MAX_MAPS][32], titles[OFFLINE_MAX_MAPS][64];
	char			details[OFFLINE_MAX_DETAILS][128];
	qhandle_t		shots[OFFLINE_MAX_MAPS];
	float			aspects[OFFLINE_MAX_MAPS];	/* width / height of the levelshot */
	char			shotNames[OFFLINE_MAX_MAPS][32];
	int				maps;
} s_offline;

static const char *Offline_Trimmed( const char *text ) {
	while ( *text == ' ' ) text++;
	return text;
}

/* the levelshot's size from its file header: object-fit cover needs the
 * picture's own aspect (most are 16:9, a few are square) */
static float Offline_ImageAspect( const char *name ) {
	static byte data[65536];
	static const char *extensions[3] = { "jpg", "tga", "png" };
	fileHandle_t f;
	int length, i, e;
	int w = 0, h = 0;

	for ( e = 0; e < 3 && !w; e++ ) {
		length = trap_FS_FOpenFile( va( "levelshots/%s.%s", name, extensions[e] ), &f, FS_READ );
		if ( !f ) continue;
		if ( length > (int)sizeof( data ) ) length = sizeof( data );
		trap_FS_Read( data, length, f );
		trap_FS_FCloseFile( f );
		if ( e == 0 ) {
			/* JPEG: walk the markers to the frame header (SOF0-SOF15 but DHT, JPG, DAC) */
			for ( i = 2; i + 9 < length; ) {
				int marker, size;
				if ( data[i] != 0xff ) break;
				marker = data[i + 1];
				size = ( data[i + 2] << 8 ) | data[i + 3];
				if ( marker >= 0xc0 && marker <= 0xcf && marker != 0xc4 && marker != 0xc8 && marker != 0xcc ) {
					h = ( data[i + 5] << 8 ) | data[i + 6];
					w = ( data[i + 7] << 8 ) | data[i + 8];
					break;
				}
				i += 2 + size;
			}
		} else if ( e == 1 && length >= 16 ) {
			w = data[12] | ( data[13] << 8 );
			h = data[14] | ( data[15] << 8 );
		} else if ( e == 2 && length >= 24 ) {
			w = ( data[16] << 24 ) | ( data[17] << 16 ) | ( data[18] << 8 ) | data[19];
			h = ( data[20] << 24 ) | ( data[21] << 16 ) | ( data[22] << 8 ) | data[23];
		}
	}
	return w > 0 && h > 0 ? (float)w / h : 1.0f;
}

static qhandle_t Offline_Shot( int map ) {
	if ( map < 0 || map >= OFFLINE_MAX_MAPS ) return 0;
	if ( Q_stricmp( s_offline.shotNames[map], s_offline.ids[map] ) ) {
		Q_strncpyz( s_offline.shotNames[map], s_offline.ids[map], sizeof( s_offline.shotNames[map] ) );
		s_offline.shots[map] = s_offline.ids[map][0] ? trap_R_RegisterShaderNoMip( va( "levelshots/%s", s_offline.ids[map] ) ) : 0;
		s_offline.aspects[map] = s_offline.shots[map] ? Offline_ImageAspect( s_offline.ids[map] ) : 1.0f;
	}
	return s_offline.shots[map];
}

/* the model as the page got it, the list filtered by the search (mapList()) */
static void Offline_Build( void ) {
	offlineLayout_t *l = &s_offline.layout;
	char query[64], joined[128], lower[128];
	int i, selected = UI_WebOfflineNumber( 2, 0 );

	memset( l, 0, sizeof( *l ) );
	l->hosting = UI_WebOfflineNumber( 14, 0 ) != 0;
	s_offline.maps = UI_WebOfflineNumber( 0, 0 );
	if ( s_offline.maps > OFFLINE_MAX_MAPS ) s_offline.maps = OFFLINE_MAX_MAPS;
	Library_Lower( s_offline.query, query, sizeof( query ) );
	for ( i = 0; i < s_offline.maps; i++ ) {
		Offline_Text( 0, i, 0, s_offline.ids[i], sizeof( s_offline.ids[i] ) );
		Offline_Text( 1, i, 0, s_offline.titles[i], sizeof( s_offline.titles[i] ) );
		Com_sprintf( joined, sizeof( joined ), "%s %s", s_offline.titles[i], s_offline.ids[i] );
		Library_Lower( joined, lower, sizeof( lower ) );
		if ( query[0] && !strstr( lower, query ) ) continue;
		if ( l->count < OFFLINE_MAX_ROWS ) {
			s_offline.rows[l->count] = i;
			l->rowText[l->count++] = s_offline.titles[i];
		}
	}
	if ( selected >= 0 && selected < s_offline.maps ) {
		Q_strncpyz( l->mapId, s_offline.ids[selected], sizeof( l->mapId ) );
		Q_strncpyz( l->name, s_offline.titles[selected], sizeof( l->name ) );
		l->image = Offline_Shot( selected ) != 0;
	} else {
		Q_strncpyz( l->mapId, "TRY ANOTHER GAME MODE", sizeof( l->mapId ) );
		Q_strncpyz( l->name, "No compatible tracks", sizeof( l->name ) );
	}
	l->details = UI_WebOfflineNumber( 11, 0 );
	if ( l->details > OFFLINE_MAX_DETAILS ) l->details = OFFLINE_MAX_DETAILS;
	for ( i = 0; i < l->details; i++ ) {
		Offline_Text( 2, i, 0, s_offline.details[i], sizeof( s_offline.details[i] ) );
		l->detail[i] = Offline_Trimmed( s_offline.details[i] );
	}
	l->scroll = s_offline.scroll;
	l->pageScroll = s_offline.pageScroll;
	Offline_Layout( l, WS_ViewportWidth(), WS_ViewportHeight() );
	s_offline.scroll = l->scroll;
	s_offline.pageScroll = l->pageScroll;
}

/* a select or an input: #222 with the line border, the focus ring when focused */
static void Offline_DrawField( const wsRect_t *r, qboolean focus ) {
	float x0 = WS_Round( r->x ), y0 = WS_Round( r->y ), x1 = WS_Round( r->x + r->w ), y1 = WS_Round( r->y + r->h );
	if ( focus ) WS_FillRect( x0 - 1, y0 - 1, x1 - x0 + 2, y1 - y0 + 2, offlineRing );
	else WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
	WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, offlineInput );
}

/* a square levelshot, object-fit: cover in a box */
static void Offline_DrawCover( float x, float y, float w, float h, float clipTop, float clipBottom, int map ) {
	float s0 = 0.0f, s1 = 1.0f, t0 = 0.0f, t1 = 1.0f, top, bottom, box, aspect;
	qhandle_t shader = Offline_Shot( map );
	if ( w <= 0.0f || h <= 0.0f || !shader ) return;
	box = w / h;
	aspect = s_offline.aspects[map];
	if ( box > aspect ) { t0 = 0.5f - 0.5f * aspect / box; t1 = 0.5f + 0.5f * aspect / box; }
	else { s0 = 0.5f - 0.5f * box / aspect; s1 = 0.5f + 0.5f * box / aspect; }
	top = y > clipTop ? y : clipTop;
	bottom = y + h < clipBottom ? y + h : clipBottom;
	if ( bottom <= top ) return;
	trap_R_SetColor( NULL );
	trap_R_DrawStretchPic( x, top, w, bottom - top, s0, t0 + ( t1 - t0 ) * ( top - y ) / h, s1,
	                       t0 + ( t1 - t0 ) * ( bottom - y ) / h, shader );
}

static void Offline_Fill( float x, float y, float w, float h, const float *color ) {
	const wsRect_t *list = &s_offline.layout.list;
	float top = y > list->y ? y : list->y, bottom = y + h < list->y + list->h ? y + h : list->y + list->h;
	if ( bottom > top ) WS_FillRect( x, top, w, bottom - top, color );
}

static void Offline_TrackDraw( void ) {
	offlineLayout_t *l;
	wsTextStyle_t body, bold, mono, h1, h1em, select;
	char text[96], shown[96];
	int i, mode, modes, selected;

	if ( s_offline.dragThumb || s_offline.dragPage ) {
		float x, y, track;
		if ( !trap_Key_IsDown( K_MOUSE1 ) ) s_offline.dragThumb = s_offline.dragPage = qfalse;
		else if ( s_offline.dragThumb ) {
			Library_Cursor( &x, &y );
			track = s_offline.layout.list.h - 2.0f * RIVALS_ARROW - 9.0f - s_offline.layout.thumb.h;
			if ( track > 0.0f ) s_offline.scroll = s_offline.dragScroll + ( y - s_offline.dragY ) *
				( s_offline.layout.contentHeight - s_offline.layout.list.h ) / track;
		} else {
			Library_Cursor( &x, &y );
			track = s_offline.layout.page.h - 2.0f * RIVALS_ARROW - 9.0f - s_offline.layout.pageThumb.h;
			if ( track > 0.0f ) s_offline.pageScroll = s_offline.dragScroll + ( y - s_offline.dragY ) *
				( s_offline.layout.pageHeight - s_offline.layout.page.h ) / track;
		}
	}
	Offline_Build();
	l = &s_offline.layout;
	mode = UI_WebOfflineNumber( 1, 0 );
	modes = UI_WebOfflineNumber( 16, 0 );
	selected = UI_WebOfflineNumber( 2, 0 );
	WS_BodyStyle( &body );
	bold = body; bold.font = WS_BODY_BOLD;
	select = body; select.lineHeight = 21.0f;
	mono.font = WS_MONO; mono.size = 0.9f * WS_REM; mono.spacing = 0.0f; mono.lineHeight = 16.0f; mono.uppercase = qfalse;
	h1.font = WS_CONDENSED; h1.size = l->h1Size; h1.spacing = 0.0f; h1.lineHeight = l->h1Size; h1.uppercase = qtrue;
	h1em = h1; h1em.font = WS_CONDENSED_ITALIC;

	WS_DrawBackground( l->pageWidth, l->pageHeight );
	WS_DrawLabel( l->contentX, l->labelY, l->hosting ? "Q3RALLY / HOST A SERVER" : "Q3RALLY / OFFLINE", NULL );
	WS_DrawText( &h1, l->contentX, l->h1Y, "YOUR NEXT ", 10, wsWhite );
	WS_DrawText( &h1em, l->contentX + WS_StyledWidth( &h1, "Your next " ), l->h1Y, "RACE.", 5, wsAccent );
	WS_DrawButton( &l->back, l->hosting ? "Back to server browser" : "Back to main menu", qtrue, qfalse );
	WS_DrawText( &mono, l->contentX, l->stepY, "01 / TRACK & MODE", 17, wsAccent );
	WS_DrawText( &mono, l->stepRulesX, l->stepY, "02 / RULES & DRIVERS", 20, offlineMuted );

	/* left: mode, search, count, list */
	WS_DrawSection( &l->left, qfalse );
	WS_DrawText( &body, l->leftX, l->modeLabelY, "Game mode", 9, wsWhite );
	Offline_DrawField( &l->mode, qfalse );
	Offline_Text( 3, mode, 0, text, sizeof( text ) );
	WS_DrawText( &select, l->mode.x + 1.0f + WS_LU( 0.65f * WS_REM ), l->mode.y + 1.0f + WS_LU( 0.65f * WS_REM ), text, strlen( text ), wsWhite );
	trap_R_SetColor( wsWhite );
	trap_R_DrawStretchPic( WS_Round( l->mode.x + l->mode.w ) - 14.0f, WS_Round( l->mode.y + l->mode.h * 0.5f ) - 3.0f, 10, 7,
	                       0, 25 / 32.0f, 10 / 16.0f, 1.0f, trap_R_RegisterShaderNoMip( "menu/ui/web_scrollbar" ) );
	trap_R_SetColor( NULL );
	WS_DrawText( &body, l->leftX, l->searchLabelY, "Find a track", 12, wsWhite );
	Offline_DrawField( &l->search, s_offline.searchFocus );
	if ( s_offline.query[0] ) {
		WS_DrawText( &body, l->search.x + 1.0f + WS_LU( 0.65f * WS_REM ), l->search.y + 1.0f + WS_LU( 0.65f * WS_REM ),
		             s_offline.query, strlen( s_offline.query ), wsWhite );
	} else {
		WS_DrawText( &body, l->search.x + 1.0f + WS_LU( 0.65f * WS_REM ), l->search.y + 1.0f + WS_LU( 0.65f * WS_REM ),
		             "Search tracks", 13, offlineHolder );
	}
	if ( s_offline.searchFocus && ( ( uis.realtime - s_offline.searchFocusTime ) / 500 ) % 2 == 0 ) {
		WS_FillRect( WS_Round( l->search.x + 1.0f + WS_LU( 0.65f * WS_REM ) +
		             WS_TextWidth( &body, s_offline.query, strlen( s_offline.query ) ) ),
		             WS_Round( l->search.y + 1.0f + WS_LU( 0.65f * WS_REM ) + 3.0f ), 1, 19, wsWhite );
	}
	Com_sprintf( text, sizeof( text ), "%d / %d tracks", l->count, s_offline.maps );
	WS_DrawText( &body, l->leftX, l->countY, text, strlen( text ), wsWhite );
	if ( !l->count ) {
		const char *empty = s_offline.maps ? "No tracks match your search." : "No tracks support this mode. Choose another game mode.";
		wsLines_t lines;
		WS_Wrap( &body, empty, l->list.w, &lines );
		WS_DrawLines( &body, l->list.x, l->list.y + WS_BLOCK_MARGIN, empty, &lines, wsWhite );
	}
	for ( i = 0; i < l->count; i++ ) {
		int map = s_offline.rows[i];
		float y = l->list.y - l->scroll + l->rowY[i], h = l->rowH[i];
		float x0 = WS_Round( l->list.x ), x1 = WS_Round( l->list.x + l->rowWidth ), y0 = WS_Round( y ), y1 = WS_Round( y + h );
		float inner = h - 2.0f - 2.0f * WS_LU( WS_BUTTON_PAD_Y );
		if ( y >= l->list.y + l->list.h || y + h <= l->list.y ) continue;
		Offline_Fill( x0, y0, x1 - x0, y1 - y0, map == selected ? wsAccent : wsLine );
		Offline_Fill( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, map == selected ? offlinePressed : wsSurface );
		Offline_DrawCover( WS_Round( l->list.x + 1.0f + WS_LU( WS_BUTTON_PAD_X ) ),
		                   WS_Round( y + 1.0f + WS_LU( WS_BUTTON_PAD_Y ) + ( inner - 3.5f * WS_REM ) * 0.5f ),
		                   6.0f * WS_REM, 3.5f * WS_REM, l->list.y, l->list.y + l->list.h, map );
		WS_SetFade( l->list.y, l->list.h, 0.0f );
		WS_DrawLines( &bold, l->list.x + 1.0f + WS_LU( WS_BUTTON_PAD_X ) + 7.0f * WS_REM,
		              y + 1.0f + WS_LU( WS_BUTTON_PAD_Y ) + ( inner - l->rowLines[i].count * WS_BODY_LINE ) * 0.5f,
		              l->rowText[i], &l->rowLines[i], wsWhite );
		WS_SetFade( 0.0f, 0.0f, 0.0f );
	}
	if ( l->scrollbar ) WS_DrawScrollbar( &l->list, &l->thumb );

	/* right: the track */
	WS_DrawSection( &l->right, qtrue );
	WS_DrawLabel( l->rightX, l->mapIdY, l->mapId, NULL );
	WS_Transform( &l->title, l->name, shown, sizeof( shown ) );
	WS_DrawLines( &l->title, l->rightX, l->nameY, shown, &l->nameLines, wsWhite );
	if ( l->image ) Offline_DrawCover( WS_Round( l->shot.x ), WS_Round( l->shot.y ), WS_Round( l->shot.w ), WS_Round( l->shot.h ),
	                                   0.0f, 100000.0f, selected );
	for ( i = 0; i < l->details; i++ ) {
		WS_DrawLines( &l->detailStyle, l->detailX[i], l->detailY[i], l->detail[i], &l->detailLines[i], wsWhite );
	}
	Library_DrawButton( &l->next, "Race setup", qfalse, s_offline.maps <= 0 || selected < 0 );
	if ( l->pageScrollbar ) WS_DrawScrollbar( &l->page, &l->pageThumb );

	/* the mode's option list; in CEF the system's popup */
	if ( s_offline.modeOpen ) {
		float x0 = WS_Round( l->mode.x ), x1 = WS_Round( l->mode.x + l->mode.w ), y1 = WS_Round( l->mode.y + l->mode.h );
		float h = modes * WS_BODY_LINE + 2.0f;
		if ( y1 + h > WS_ViewportHeight() ) y1 = WS_Round( l->mode.y ) - h;
		WS_FillRect( x0, y1, x1 - x0, h, offlinePopupLine );
		WS_FillRect( x0 + 1, y1 + 1, x1 - x0 - 2, h - 2, offlinePopup );
		for ( i = 0; i < modes; i++ ) {
			if ( i == mode ) WS_FillRect( x0 + 1, y1 + 1 + i * WS_BODY_LINE, x1 - x0 - 2, WS_BODY_LINE, offlinePopupLine );
			Offline_Text( 3, i, 0, text, sizeof( text ) );
			WS_DrawText( &body, l->mode.x + 1.0f + WS_LU( 0.65f * WS_REM ), y1 + 1 + i * WS_BODY_LINE, text, strlen( text ), wsWhite );
		}
	}
}

static int Offline_ModeAt( float x, float y ) {
	const offlineLayout_t *l = &s_offline.layout;
	int modes = UI_WebOfflineNumber( 16, 0 );
	float y1 = WS_Round( l->mode.y + l->mode.h ), h = modes * WS_BODY_LINE + 2.0f;
	if ( y1 + h > WS_ViewportHeight() ) y1 = WS_Round( l->mode.y ) - h;
	if ( x < l->mode.x || x >= l->mode.x + l->mode.w || y < y1 + 1 || y >= y1 + 1 + modes * WS_BODY_LINE ) return -1;
	return (int)( ( y - y1 - 1 ) / WS_BODY_LINE );
}

static void Offline_TrackClick( float x, float y ) {
	const offlineLayout_t *l = &s_offline.layout;
	int i, mode;

	if ( s_offline.modeOpen ) {
		s_offline.modeOpen = qfalse;
		mode = Offline_ModeAt( x, y );
		if ( mode >= 0 && mode != UI_WebOfflineNumber( 1, 0 ) ) {
			s_offline.query[0] = '\0';
			s_offline.scroll = 0.0f;
			UI_WebOfflineAction( 1, mode, 0 );
		}
		return;
	}
	s_offline.searchFocus = qfalse;
	if ( l->pageScrollbar && x >= l->page.w - RIVALS_SCROLLBAR ) {
		if ( y < RIVALS_ARROW ) s_offline.pageScroll -= 40.0f;
		else if ( y >= l->page.h - RIVALS_ARROW ) s_offline.pageScroll += 40.0f;
		else if ( y < l->pageThumb.y ) s_offline.pageScroll -= 0.875f * l->page.h;
		else if ( y >= l->pageThumb.y + l->pageThumb.h ) s_offline.pageScroll += 0.875f * l->page.h;
		else {
			s_offline.dragPage = qtrue;
			s_offline.dragY = y;
			s_offline.dragScroll = s_offline.pageScroll;
		}
		return;
	}
	if ( Library_Inside( &l->back, x, y ) ) { UI_WebOfflineAction( 0, 0, 0 ); return; }
	if ( Library_Inside( &l->mode, x, y ) ) { s_offline.modeOpen = qtrue; return; }
	if ( Library_Inside( &l->search, x, y ) ) {
		s_offline.searchFocus = qtrue;
		s_offline.searchFocusTime = uis.realtime;
		return;
	}
	if ( Library_Inside( &l->next, x, y ) ) {
		if ( s_offline.maps > 0 ) UI_WebOfflineAction( 3, 0, 0 );
		return;
	}
	if ( !Library_Inside( &l->list, x, y ) ) return;
	if ( l->scrollbar && x >= l->list.x + l->list.w - RIVALS_SCROLLBAR ) {
		if ( y < l->list.y + RIVALS_ARROW ) s_offline.scroll -= 40.0f;
		else if ( y >= l->list.y + l->list.h - RIVALS_ARROW ) s_offline.scroll += 40.0f;
		else if ( y < l->thumb.y ) s_offline.scroll -= 0.875f * l->list.h;
		else if ( y >= l->thumb.y + l->thumb.h ) s_offline.scroll += 0.875f * l->list.h;
		else {
			s_offline.dragThumb = qtrue;
			s_offline.dragY = y;
			s_offline.dragScroll = s_offline.scroll;
		}
		return;
	}
	for ( i = 0; i < l->count; i++ ) {
		float top = l->list.y - l->scroll + l->rowY[i];
		if ( y >= top && y < top + l->rowH[i] && x < l->list.x + l->rowWidth ) {
			UI_WebOfflineAction( 2, s_offline.rows[i], 0 );
			return;
		}
	}
}

static sfxHandle_t Offline_TrackKey( int key ) {
	const offlineLayout_t *l = &s_offline.layout;
	float x, y;
	int i, row = -1, selected = UI_WebOfflineNumber( 2, 0 ), length;

	Library_Cursor( &x, &y );
	if ( key & K_CHAR_FLAG ) {
		int ch = key & ~K_CHAR_FLAG;
		if ( !s_offline.searchFocus ) return 0;
		length = strlen( s_offline.query );
		if ( ch == 8 ) { if ( length ) s_offline.query[length - 1] = '\0'; }
		else if ( ch >= 32 && ch != 127 && ch < 256 && length < (int)sizeof( s_offline.query ) - 1 ) {
			s_offline.query[length] = (char)ch;
			s_offline.query[length + 1] = '\0';
		}
		s_offline.searchFocusTime = uis.realtime;
		return 0;
	}
	switch ( key ) {
	case K_ESCAPE:
	case K_PAD0_B:
		if ( s_offline.modeOpen ) s_offline.modeOpen = qfalse;
		else UI_WebOfflineAction( 0, 0, 0 );
		return 0;
	case K_MOUSE1:
		Offline_TrackClick( x, y );
		return 0;
	case K_MWHEELUP:
	case K_MWHEELDOWN:
		if ( Library_Inside( &l->list, x, y ) && l->scrollbar ) s_offline.scroll += key == K_MWHEELUP ? -100.0f : 100.0f;
		else s_offline.pageScroll += key == K_MWHEELUP ? -100.0f : 100.0f;
		return 0;
	case K_ENTER:
	case K_KP_ENTER:
		if ( !s_offline.modeOpen && s_offline.maps > 0 ) UI_WebOfflineAction( 3, 0, 0 );
		return 0;
	case K_UPARROW:
	case K_KP_UPARROW:
	case K_DOWNARROW:
	case K_KP_DOWNARROW:
	case K_PAD0_DPAD_UP:
	case K_PAD0_DPAD_DOWN:
		for ( i = 0; i < l->count; i++ ) if ( s_offline.rows[i] == selected ) row = i;
		if ( !l->count ) return 0;
		row += key == K_UPARROW || key == K_KP_UPARROW || key == K_PAD0_DPAD_UP ? -1 : 1;
		if ( row < 0 ) row = 0;
		if ( row >= l->count ) row = l->count - 1;
		UI_WebOfflineAction( 2, s_offline.rows[row], 0 );
		if ( l->rowY[row] < s_offline.scroll ) s_offline.scroll = l->rowY[row];
		if ( l->rowY[row] + l->rowH[row] > s_offline.scroll + l->list.h ) s_offline.scroll = l->rowY[row] + l->rowH[row] - l->list.h;
		return 0;
	}
	return 0;
}

void Offline_InstallTrack( menuframework_s *menu ) {
	char preview[64];

	memset( &s_offline, 0, sizeof( s_offline ) );
	menu->draw = Offline_TrackDraw;
	menu->key = Offline_TrackKey;
	/* ui_offlinePreview "mode,search,selected,scroll[,pagescroll[,step]]" pins a state for
	 * screenshots; step 2 goes on to the rules and drivers */
	trap_Cvar_VariableStringBuffer( "ui_offlinePreview", preview, sizeof( preview ) );
	if ( preview[0] ) {
		char *field[6];
		int n;
		field[0] = preview;
		for ( n = 1; n < 6; n++ ) {
			field[n] = field[n - 1] ? strchr( field[n - 1], ',' ) : NULL;
			if ( field[n] ) *field[n]++ = '\0';
		}
		if ( field[1] ) Q_strncpyz( s_offline.query, field[1], sizeof( s_offline.query ) );
		if ( field[3] ) s_offline.scroll = atof( field[3] );
		if ( field[4] ) s_offline.pageScroll = atof( field[4] );
		uis.activemenu = menu;	/* the model answers for the active menu */
		if ( atoi( field[0] ) != UI_WebOfflineNumber( 1, 0 ) ) UI_WebOfflineAction( 1, atoi( field[0] ), 0 );
		if ( field[2] ) UI_WebOfflineAction( 2, atoi( field[2] ), 0 );
		if ( field[5] && atoi( field[5] ) == 2 ) {
			UI_WebOfflineAction( 3, 0, 0 );
			if ( trap_Cvar_VariableValue( "ui_offlineDump" ) ) Offline_Dump();
		}
	}
}
