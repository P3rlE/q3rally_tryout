/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The HUD options (Shift+H): a panel in the middle of the screen, where the
HUD keeps it free, so every change shows in the corners at once. Its content
is the mode's toggles (CG_HUDOptions_Build, cg_hud_core.c), its layout
MH_OptionsLayout (the design: tools/ui_webstyle/hud_design/gen.py
hud_panel), its drawing CG_ModernHUD_DrawOptions. Here: the pointer, the
clicks and the minimap's sliders while they are dragged.
*/

#include "cg_local.h"
#include "../client/keycodes.h"
#include "cg_hud_elements.h"

static struct {
	mhOptionsLayout_t	layout;
	int					entry[MH_OPT_ITEMS];	/* the toggle each item changes */
	int					value[MH_OPT_ITEMS];
	int					drag;					/* the slider dragged: item + 1 */
	int					dragValue;
} s_panel;

/* the pointer in design pixels (the screen 900 of them high): the cgame
 * keeps a 640x480 cursor placed like the UI's (centred, reaching the sides
 * of a wide screen) */
static void HUDPanel_Cursor( float *x, float *y ) {
	float scale = cgs.glconfig.vidHeight / MH_DESIGN_HEIGHT;
	*x = ( cgs.cursorX * cgs.screenXScale + cgs.screenXBias ) / scale;
	*y = ( cgs.cursorY * cgs.screenYScale + cgs.screenYBias ) / scale;
}

static qboolean HUDPanel_Inside( const wsRect_t *r, float x, float y ) {
	return r->w > 0.0f && x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

/* a slider takes the pointer over its track and thumb, a little above
 * and below them */
static qboolean HUDPanel_OnSlider( int item, float x, float y ) {
	const wsRect_t *t = &s_panel.layout.sliderTrack[item];
	return x >= t->x - 7.0f && x < t->x + t->w + 7.0f && y >= t->y - 9.0f && y < t->y + t->h + 9.0f;
}

/* the slider's value at x: its range in steps of 5 */
static int HUDPanel_SliderValue( int item, float x ) {
	const wsRect_t *t = &s_panel.layout.sliderTrack[item];
	int low = s_panel.entry[item] == HUDOPT_ITEM_SIZE ? 40 : 30;
	int high = s_panel.entry[item] == HUDOPT_ITEM_SIZE ? 200 : 120;
	float fraction = t->w > 0.0f ? WS_Clamp( 0.0f, ( x - t->x ) / t->w, 1.0f ) : 0.0f;
	return low + (int)( fraction * ( high - low ) / 5.0f + 0.5f ) * 5;
}

static void HUDPanel_Build( void ) {
	mhOptionsLayout_t *o = &s_panel.layout;
	memset( o, 0, sizeof( *o ) );
	o->viewWidth = cgs.glconfig.vidWidth / ( cgs.glconfig.vidHeight / MH_DESIGN_HEIGHT );
	CG_HUDOptions_Build( o, s_panel.entry, s_panel.value );
	/* a slider being dragged shows where it is */
	if ( s_panel.drag && s_panel.drag <= o->itemCount && !o->itemChoices[s_panel.drag - 1] ) {
		int item = s_panel.drag - 1, low = s_panel.entry[item] == HUDOPT_ITEM_SIZE ? 40 : 30;
		int high = s_panel.entry[item] == HUDOPT_ITEM_SIZE ? 200 : 120;
		static char text[16];
		if ( s_panel.entry[item] == HUDOPT_ITEM_SIZE ) Com_sprintf( text, sizeof( text ), "%d.%02d", s_panel.dragValue / 100, s_panel.dragValue % 100 );
		else Com_sprintf( text, sizeof( text ), "%d", s_panel.dragValue );
		o->itemValue[item] = text;
		o->itemFrac[item] = (float)( s_panel.dragValue - low ) / ( high - low );
	}
	MH_OptionsLayout( o );
}

/* -------------------------------------------------------------------------
   Drawing
   ------------------------------------------------------------------------- */

void CG_HUDPanel_Draw( void ) {
	mhOptionsLayout_t *o = &s_panel.layout;
	float x, y;
	int i, c, hover = -1, hoverChoice = -1;

	/* a dragged slider follows the pointer until the button is up */
	if ( s_panel.drag ) {
		HUDPanel_Cursor( &x, &y );
		s_panel.dragValue = HUDPanel_SliderValue( s_panel.drag - 1, x );
		if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
			CG_HUDOptions_Set( s_panel.entry[s_panel.drag - 1], s_panel.dragValue );
			s_panel.drag = 0;
		}
	}
	HUDPanel_Build();

	/* what the pointer is over: a choice (hover, hoverChoice), the back
	 * button (-2) or the console button (-3) */
	HUDPanel_Cursor( &x, &y );
	for ( i = 0; i < o->itemCount; i++ ) {
		for ( c = 0; c < o->itemChoices[i]; c++ ) {
			if ( HUDPanel_Inside( &o->itemButton[i][c], x, y ) ) { hover = i; hoverChoice = c; }
		}
	}
	if ( HUDPanel_Inside( &o->backBox, x, y ) ) hover = -2;
	if ( HUDPanel_Inside( &o->consoleBox, x, y ) ) hover = -3;

	CG_ModernHUD_DrawOptions( o, s_panel.value, hover, hoverChoice );

	/* the pointer */
	{
		static qhandle_t cursor;
		float size = 16.0f * cgs.glconfig.vidHeight / 480.0f;
		if ( !cursor ) cursor = trap_R_RegisterShaderNoMip( "menu/art/3_cursor2" );
		x = cgs.cursorX * cgs.screenXScale + cgs.screenXBias;
		y = cgs.cursorY * cgs.screenYScale + cgs.screenYBias;
		trap_R_SetColor( NULL );
		trap_R_DrawStretchPic( x, y, size, size, 0, 0, 1, 1, cursor );
	}
}

/* -------------------------------------------------------------------------
   Input
   ------------------------------------------------------------------------- */

/* the panel was closed (Escape, Shift+H): nothing stays dragged */
void CG_HUDPanel_Reset( void ) {
	s_panel.drag = 0;
}

static void HUDPanel_Close( void ) {
	CG_HUDPanel_Reset();
	trap_Cvar_Set( "cg_hudOptionsOpen", "0" );
	trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_CGAME );
}

static void HUDPanel_Click( float x, float y ) {
	mhOptionsLayout_t *o = &s_panel.layout;
	int i, c;

	if ( HUDPanel_Inside( &o->backBox, x, y ) ) { HUDPanel_Close(); return; }
	if ( HUDPanel_Inside( &o->consoleBox, x, y ) ) { trap_SendConsoleCommand( "toggleconsole\n" ); return; }
	for ( i = 0; i < o->itemCount; i++ ) {
		if ( !o->itemChoices[i] ) {
			if ( HUDPanel_OnSlider( i, x, y ) ) {
				s_panel.drag = i + 1;
				s_panel.dragValue = HUDPanel_SliderValue( i, x );
				return;
			}
			continue;
		}
		for ( c = 0; c < o->itemChoices[i]; c++ ) {
			if ( HUDPanel_Inside( &o->itemButton[i][c], x, y ) ) {
				if ( c != s_panel.value[i] ) CG_HUDOptions_Set( s_panel.entry[i], c );
				return;
			}
		}
	}
}

/* a key while the panel is open; qfalse for Escape, which closes it */
qboolean CG_HUDPanel_Key( int key ) {
	float x, y;

	switch ( key ) {
	case K_ESCAPE:
	case K_PAD0_B:
		return qfalse;
	case K_MOUSE1:
		HUDPanel_Build();
		HUDPanel_Cursor( &x, &y );
		HUDPanel_Click( x, y );
		return qtrue;
	}
	return qtrue;
}
