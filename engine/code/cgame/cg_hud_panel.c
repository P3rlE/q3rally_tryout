/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The HUD options (Shift+H in a race) drawn like the WebUI's #hud-screen
(session.js): the panel at the right with a select per toggle and the
mini-map's two sliders. It reads and changes the toggles through the same
CG_WebHUDNumber / CG_WebHUDChar / CG_WebHUDAction the client used for CEF.
Layout in cg_hud_panellayout.c; the drawing is q3_ui's webstyle, built into
the cgame as well.
*/

#include "cg_local.h"
#include "../client/keycodes.h"
#include "cg_hud_panellayout.h"

static const vec4_t panelBack     = { 0x11 / 255.0f, 0x11 / 255.0f, 0x11 / 255.0f, 1.0f };	/* #hud-screen */
static const vec4_t panelPopup    = { 0x1f / 255.0f, 0x1f / 255.0f, 0x1f / 255.0f, 1.0f };
static const vec4_t panelPopupLine = { 0x3b / 255.0f, 0x3b / 255.0f, 0x3b / 255.0f, 1.0f };

typedef enum { DRAG_NONE, DRAG_THUMB, DRAG_SIZE, DRAG_ZOOM } hudPanelDrag_t;

static struct {
	hudPanelLayout_t	layout;
	char				label[HUDPANEL_MAX_ITEMS][64];
	int					value[HUDPANEL_MAX_ITEMS], choices[HUDPANEL_MAX_ITEMS];
	int					size, zoom;		/* cg_mmap_size * 100, cg_mmap_fov */
	float				scroll;
	int					popup;			/* item + 1 */
	float				popupScroll;
	hudPanelDrag_t		drag;
	int					dragValue;
	float				dragY, dragScroll;
} s_panel;

static void HUDPanel_Text( int index, int field, int choice, char *out, int size ) {
	int i;
	for ( i = 0; i < size - 1; i++ ) {
		out[i] = (char)CG_WebHUDChar( index, field, choice, i );
		if ( !out[i] ) break;
	}
	out[i] = '\0';
}

/* the pointer in screen pixels: the cgame keeps a 640x480 cursor placed
 * like the UI's (centred, reaching the sides of a wide screen) */
static void HUDPanel_Cursor( float *x, float *y ) {
	*x = cgs.cursorX * cgs.screenXScale + cgs.screenXBias;
	*y = cgs.cursorY * cgs.screenYScale + cgs.screenYBias;
}

static qboolean HUDPanel_Inside( const wsRect_t *r, float x, float y ) {
	return r->w > 0.0f && x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

static void HUDPanel_Build( void ) {
	hudPanelLayout_t *l = &s_panel.layout;
	int i, count = CG_WebHUDNumber( 0, 0 );

	if ( count > HUDPANEL_MAX_ITEMS ) count = HUDPANEL_MAX_ITEMS;
	l->items = count;
	for ( i = 0; i < count; i++ ) {
		HUDPanel_Text( i, 0, 0, s_panel.label[i], sizeof( s_panel.label[0] ) );
		l->itemLabel[i] = s_panel.label[i];
		s_panel.value[i] = CG_WebHUDNumber( 1, i );
		s_panel.choices[i] = CG_WebHUDNumber( 2, i );
	}
	trap_Cvar_Update( &cg_mmap_size );
	trap_Cvar_Update( &cg_mmap_fov );
	s_panel.size = (int)( cg_mmap_size.value * 100.0f + 0.5f );
	s_panel.zoom = (int)( cg_mmap_fov.value + 0.5f );
	l->scroll = s_panel.scroll;
	HUDPanel_Layout( l, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight );
	s_panel.scroll = l->scroll;
}

/* <input type="range" min max step 5> */
static int HUDPanel_RangeValue( const wsRect_t *r, int low, int high, float x ) {
	float w = r->w - 2.0f - 16.0f, fraction = w > 0.0f ? WS_Clamp( 0.0f, ( x - r->x - 1.0f - 8.0f ) / w, 1.0f ) : 0.0f;
	return low + (int)( fraction * ( high - low ) / 5.0f + 0.5f ) * 5;
}

static float HUDPanel_Fraction( int value, int low, int high ) {
	return WS_Clamp( 0.0f, (float)( value - low ) / ( high - low ), 1.0f );
}

/* -------------------------------------------------------------------------
   Popup of the open select
   ------------------------------------------------------------------------- */

static void HUDPanel_PopupRect( wsRect_t *r, int *rows ) {
	const wsRect_t *anchor = &s_panel.layout.select[s_panel.popup - 1];
	int count = s_panel.choices[s_panel.popup - 1];
	float below = cgs.glconfig.vidHeight - WS_Round( anchor->y + anchor->h ), above = WS_Round( anchor->y );
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
	s_panel.popupScroll = WS_Clamp( 0.0f, s_panel.popupScroll, (float)( count - *rows ) );
}

static void HUDPanel_DrawPopup( void ) {
	wsTextStyle_t body;
	wsRect_t r;
	char text[64];
	int rows, i, first, item = s_panel.popup - 1;
	if ( !s_panel.popup ) return;
	HUDPanel_PopupRect( &r, &rows );
	WS_BodyStyle( &body );
	WS_FillRect( r.x, r.y, r.w, r.h, panelPopupLine );
	WS_FillRect( r.x + 1, r.y + 1, r.w - 2, r.h - 2, panelPopup );
	first = (int)s_panel.popupScroll;
	for ( i = 0; i < rows; i++ ) {
		if ( first + i == s_panel.value[item] ) WS_FillRect( r.x + 1, r.y + 1 + i * WS_BODY_LINE, r.w - 2, WS_BODY_LINE, panelPopupLine );
		HUDPanel_Text( item, 1, first + i, text, sizeof( text ) );
		WS_DrawText( &body, r.x + 1.0f + WS_LU( 0.6f * WS_REM ), r.y + 1 + i * WS_BODY_LINE, text, strlen( text ), wsWhite );
	}
}

static void HUDPanel_PopupClick( float x, float y ) {
	wsRect_t r;
	int rows, row, item = s_panel.popup - 1;
	HUDPanel_PopupRect( &r, &rows );
	s_panel.popup = 0;
	if ( !HUDPanel_Inside( &r, x, y ) || y < r.y + 1 || y >= r.y + 1 + rows * WS_BODY_LINE ) return;
	row = (int)s_panel.popupScroll + (int)( ( y - r.y - 1 ) / WS_BODY_LINE );
	if ( row != s_panel.value[item] ) CG_WebHUDAction( 0, item, row );	/* the select's change event */
}

/* -------------------------------------------------------------------------
   Drawing
   ------------------------------------------------------------------------- */

void CG_HUDPanel_Draw( void ) {
	hudPanelLayout_t *l = &s_panel.layout;
	wsTextStyle_t body, h2, em, h3;
	char text[64];
	float x, y, track;
	int i, value;

	/* a dragged thumb or slider follows the pointer until the button is up */
	if ( s_panel.drag != DRAG_NONE ) {
		HUDPanel_Cursor( &x, &y );
		if ( s_panel.drag == DRAG_THUMB ) {
			track = l->panel.h - 2.0f * WS_SCROLLBAR_ARROW - 9.0f - l->thumb.h;
			if ( !trap_Key_IsDown( K_MOUSE1 ) ) s_panel.drag = DRAG_NONE;
			else if ( track > 0.0f ) s_panel.scroll = s_panel.dragScroll + ( y - s_panel.dragY ) * ( l->contentHeight - l->panel.h ) / track;
		} else {
			s_panel.dragValue = s_panel.drag == DRAG_SIZE ? HUDPanel_RangeValue( &l->size, 40, 200, x ) : HUDPanel_RangeValue( &l->zoom, 30, 120, x );
			if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
				/* the range input's change event */
				CG_WebHUDAction( s_panel.drag == DRAG_SIZE ? 1 : 2, 0, s_panel.dragValue );
				s_panel.drag = DRAG_NONE;
			}
		}
	}
	HUDPanel_Build();

	WS_BodyStyle( &body );
	h2.font = WS_CONDENSED;
	h2.size = h2.lineHeight = l->h2Size;
	h2.spacing = 0.0f;
	h2.uppercase = qtrue;
	em = h2;
	em.font = WS_CONDENSED_ITALIC;
	h3.font = WS_BODY_BOLD;
	h3.size = 1.17f * WS_REM;
	h3.lineHeight = WS_LU( 1.5f * h3.size );
	h3.spacing = 0.0f;
	h3.uppercase = qfalse;

	/* #hud-screen: #111, border-left 3px accent */
	WS_FillRect( WS_Round( l->panel.x ), 0, WS_Round( l->panel.x + l->panel.w ) - WS_Round( l->panel.x ), l->panel.h, panelBack );
	WS_FillRect( WS_Round( l->panel.x ), 0, 3, l->panel.h, wsAccent );

	WS_DrawLabel( l->x, l->labelY, hudPanelLabel, NULL );
	WS_Transform( &h2, hudPanelTitle, text, sizeof( text ) );
	WS_DrawText( &h2, l->x, l->h2Y, text, strlen( text ), wsWhite );
	WS_DrawText( &h2, l->x, l->h2Y + l->h2Size, "YOUR ", 5, wsWhite );
	WS_Transform( &em, hudPanelEm, text, sizeof( text ) );
	WS_DrawText( &em, l->emX, l->h2Y + l->h2Size, text, strlen( text ), wsAccent );
	WS_DrawLines( &body, l->x, l->textY, hudPanelText, &l->textLines, wsWhite );
	WS_DrawButton( &l->close, "Back to race", qfalse, qfalse );
	WS_DrawButton( &l->console, "Console", qtrue, qfalse );

	for ( i = 0; i < l->items; i++ ) {
		WS_DrawLines( &body, l->x, l->itemY[i], l->itemLabel[i], &l->itemLines[i], wsWhite );
		HUDPanel_Text( i, 1, s_panel.value[i], text, sizeof( text ) );
		WS_DrawSelect( &l->select[i], WS_LU( 0.6f * WS_REM ), text, s_panel.popup == i + 1 );
	}

	WS_DrawText( &h3, l->x, l->h3Y, "Mini-map", 8, wsWhite );
	value = s_panel.drag == DRAG_SIZE ? s_panel.dragValue : s_panel.size;
	WS_DrawText( &body, l->x, l->sizeY, "Size", 4, wsWhite );
	Com_sprintf( text, sizeof( text ), "%d.%02d", value / 100, value % 100 );	/* toFixed(2) */
	WS_DrawText( &body, l->x, l->sizeOutputY, text, strlen( text ), wsAccent );
	WS_DrawRange( &l->size, 0.0f, HUDPanel_Fraction( value, 40, 200 ), qfalse );
	value = s_panel.drag == DRAG_ZOOM ? s_panel.dragValue : s_panel.zoom;
	WS_DrawText( &body, l->x, l->zoomY, "Field of view", 13, wsWhite );
	Com_sprintf( text, sizeof( text ), "%d", value );
	WS_DrawText( &body, l->x, l->zoomOutputY, text, strlen( text ), wsAccent );
	WS_DrawRange( &l->zoom, 0.0f, HUDPanel_Fraction( value, 30, 120 ), qfalse );
	WS_DrawLabel( l->x, l->hintY, hudPanelHint, NULL );

	if ( l->scrollbar ) WS_DrawScrollbar( &l->panel, &l->thumb );
	HUDPanel_DrawPopup();

	/* the pointer */
	{
		static qhandle_t cursor;
		float size = 16.0f * cgs.glconfig.vidHeight / 480.0f;
		if ( !cursor ) cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
		HUDPanel_Cursor( &x, &y );
		trap_R_SetColor( NULL );
		trap_R_DrawStretchPic( x, y, size, size, 0, 0, 1, 1, cursor );
	}
}

/* -------------------------------------------------------------------------
   Input
   ------------------------------------------------------------------------- */

/* the panel was closed (Escape, Shift+H): nothing stays open in it */
void CG_HUDPanel_Reset( void ) {
	s_panel.popup = 0;
	s_panel.drag = DRAG_NONE;
}

static void HUDPanel_Close( void ) {
	CG_HUDPanel_Reset();
	trap_Cvar_Set( "cg_hudOptionsOpen", "0" );
	trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_CGAME );
}

static void HUDPanel_Click( float x, float y ) {
	hudPanelLayout_t *l = &s_panel.layout;
	int i;

	if ( s_panel.popup ) { HUDPanel_PopupClick( x, y ); return; }
	if ( l->scrollbar && x >= l->panel.x + l->panel.w - WS_SCROLLBAR_GUTTER && HUDPanel_Inside( &l->panel, x, y ) ) {
		if ( y < WS_SCROLLBAR_ARROW ) s_panel.scroll -= 40.0f;
		else if ( y >= l->panel.h - WS_SCROLLBAR_ARROW ) s_panel.scroll += 40.0f;
		else if ( y < l->thumb.y ) s_panel.scroll -= 0.875f * l->panel.h;
		else if ( y >= l->thumb.y + l->thumb.h ) s_panel.scroll += 0.875f * l->panel.h;
		else {
			s_panel.drag = DRAG_THUMB;
			s_panel.dragY = y;
			s_panel.dragScroll = s_panel.scroll;
		}
		return;
	}
	if ( HUDPanel_Inside( &l->close, x, y ) ) { HUDPanel_Close(); return; }
	if ( HUDPanel_Inside( &l->console, x, y ) ) { trap_SendConsoleCommand( "toggleconsole\n" ); return; }
	for ( i = 0; i < l->items; i++ ) {
		if ( HUDPanel_Inside( &l->select[i], x, y ) ) {
			if ( CG_WebHUDNumber( 3, i ) ) return;	/* disabled */
			s_panel.popup = i + 1;
			s_panel.popupScroll = s_panel.value[i] - 10;
			return;
		}
	}
	if ( HUDPanel_Inside( &l->size, x, y ) ) { s_panel.drag = DRAG_SIZE; s_panel.dragValue = HUDPanel_RangeValue( &l->size, 40, 200, x ); return; }
	if ( HUDPanel_Inside( &l->zoom, x, y ) ) { s_panel.drag = DRAG_ZOOM; s_panel.dragValue = HUDPanel_RangeValue( &l->zoom, 30, 120, x ); }
}

/* a key while the panel is open; qfalse for Escape with nothing to close
 * but the panel itself */
qboolean CG_HUDPanel_Key( int key ) {
	float x, y;

	HUDPanel_Cursor( &x, &y );
	switch ( key ) {
	case K_ESCAPE:
	case K_PAD0_B:
		if ( !s_panel.popup ) return qfalse;
		s_panel.popup = 0;
		return qtrue;
	case K_MOUSE1:
		HUDPanel_Click( x, y );
		return qtrue;
	case K_MOUSE2:
		s_panel.popup = 0;
		return qtrue;
	case K_MWHEELUP:
	case K_MWHEELDOWN:
		if ( s_panel.popup ) s_panel.popupScroll += key == K_MWHEELUP ? -3.0f : 3.0f;
		else s_panel.scroll += key == K_MWHEELUP ? -100.0f : 100.0f;
		return qtrue;
	case K_UPARROW:
	case K_PGUP:
		s_panel.scroll -= key == K_PGUP ? 0.875f * cgs.glconfig.vidHeight : 40.0f;
		return qtrue;
	case K_DOWNARROW:
	case K_PGDN:
		s_panel.scroll += key == K_PGDN ? 0.875f * cgs.glconfig.vidHeight : 40.0f;
		return qtrue;
	}
	return qtrue;
}
