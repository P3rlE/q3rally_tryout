/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The parts of the WebUI's #library-screen Rivals and Downloads share (see the
header).
*/

#include "ui_local.h"
#include "ui_rally_library.h"

static const vec4_t libSelected = { 0x3b / 255.0f, 0x23 / 255.0f, 0x17 / 255.0f, 1.0f };   /* aria-pressed */
static const vec4_t libInput    = { 0x11 / 255.0f, 0x11 / 255.0f, 0x11 / 255.0f, 1.0f };
static const vec4_t libHolder   = { 0x75 / 255.0f, 0x75 / 255.0f, 0x75 / 255.0f, 1.0f };   /* ::placeholder */
static const vec4_t libRing     = { 0xee / 255.0f, 0xee / 255.0f, 0xee / 255.0f, 1.0f };   /* focus ring */


void Library_Cursor( float *x, float *y ) {
	*x = uis.cursorx * uis.xscale + uis.bias;
	*y = uis.cursory * uis.yscale;
}

qboolean Library_Inside( const wsRect_t *r, float x, float y ) {
	return r->w > 0.0f && x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

void Library_Lower( const char *in, char *out, int size ) {
	int i;
	for ( i = 0; in[i] && i < size - 1; i++ ) out[i] = tolower( (unsigned char)in[i] );
	out[i] = '\0';
}

void Library_Query( const wsLibrary_t *lib, char *out, int size ) {
	char lower[64], *start, *end;
	Library_Lower( lib->query, lower, sizeof( lower ) );
	for ( start = lower; *start == ' '; start++ ) {
	}
	for ( end = start + strlen( start ); end > start && end[-1] == ' '; end-- ) {
	}
	*end = '\0';
	Q_strncpyz( out, start, size );
}

/* -------------------------------------------------------------------------
   Drawing
   ------------------------------------------------------------------------- */

static void Library_Fill( const wsLibrary_t *lib, float x, float y, float w, float h, const float *color ) {
	const wsRect_t *list = &lib->layout.list;
	float top = y > list->y ? y : list->y;
	float bottom = y + h < list->y + list->h ? y + h : list->y + list->h;
	if ( bottom > top ) WS_FillRect( x, top, w, bottom - top, color );
}

static void Library_FillAlpha( float x, float y, float w, float h, const float *color, float alpha ) {
	vec4_t faded;
	Vector4Copy( color, faded );
	faded[3] *= alpha;
	WS_FillRect( x, y, w, h, faded );
}

void Library_DrawButton( const wsRect_t *box, const char *label, qboolean secondary, qboolean disabled ) {
	wsTextStyle_t bold;
	vec4_t text;
	float alpha = disabled ? 0.45f : 1.0f;
	float x0 = WS_Round( box->x ), y0 = WS_Round( box->y ), x1 = WS_Round( box->x + box->w ), y1 = WS_Round( box->y + box->h );

	if ( !disabled ) {
		WS_DrawButton( box, label, secondary, qfalse );
		return;
	}
	/* opacity .45 over what is behind: border and face blend, then the text */
	Library_FillAlpha( x0, y0, x1 - x0, 1, secondary ? wsLine : wsAccent, alpha );
	Library_FillAlpha( x0, y1 - 1, x1 - x0, 1, secondary ? wsLine : wsAccent, alpha );
	Library_FillAlpha( x0, y0 + 1, 1, y1 - y0 - 2, secondary ? wsLine : wsAccent, alpha );
	Library_FillAlpha( x1 - 1, y0 + 1, 1, y1 - y0 - 2, secondary ? wsLine : wsAccent, alpha );
	Library_FillAlpha( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, secondary ? wsSurface : wsAccent, alpha );
	WS_BodyStyle( &bold );
	bold.font = WS_BODY_BOLD;
	/* the button is one group at .45: its text covers the face first, so it
	 * mixes with what lies behind the button (the dark section), not the face */
	{
		const float *t = secondary ? wsWhite : wsButtonText;
		int c;
		for ( c = 0; c < 3; c++ ) text[c] = alpha * t[c] + ( 1.0f - alpha ) * wsSurface[c];
		text[3] = 1.0f;
	}
	WS_DrawText( &bold, box->x + 1 + WS_BUTTON_PAD_X, box->y + 1 + WS_BUTTON_PAD_Y, label, strlen( label ), text );
}

void Library_DrawSearch( const wsLibrary_t *lib, const char *placeholder ) {
	const wsRect_t *r = &lib->layout.search;
	wsTextStyle_t body, cancel;
	float x0 = WS_Round( r->x ), y0 = WS_Round( r->y ), x1 = WS_Round( r->x + r->w ), y1 = WS_Round( r->y + r->h );
	float textX = r->x + 1.0f + WS_LU( 0.6f * WS_REM ), lineTop = r->y + 1.0f + WS_LU( 0.6f * WS_REM );

	WS_BodyStyle( &body );
	if ( lib->searchFocus ) {
		/* the focus ring, 2px #eee over the border */
		WS_FillRect( x0 - 1, y0 - 1, x1 - x0 + 2, y1 - y0 + 2, libRing );
	} else {
		WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
	}
	WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, libInput );
	if ( lib->query[0] ) {
		WS_DrawText( &body, textX, lineTop, lib->query, strlen( lib->query ), wsWhite );
		if ( lib->searchFocus ) {
			/* the search field's cancel button */
			cancel = body;
			cancel.font = WS_BODY_BOLD;
			cancel.size = 20.0f;
			cancel.lineHeight = WS_BODY_LINE;
			WS_DrawText( &cancel, x1 - 1.0f - WS_LU( 0.6f * WS_REM ) - 12.0f, lineTop - 1.0f, "\xd7", 1, wsWhite );
		}
	} else {
		WS_DrawText( &body, textX, lineTop, placeholder, strlen( placeholder ), libHolder );
	}
	if ( lib->searchFocus && ( ( uis.realtime - lib->searchFocusTime ) / 500 ) % 2 == 0 ) {
		float caret = WS_Round( textX + WS_TextWidth( &body, lib->query, strlen( lib->query ) ) );
		WS_FillRect( caret, WS_Round( lineTop + 3.0f ), 1, 19, wsWhite );
	}
}

void Library_DrawList( const wsLibrary_t *lib, const qhandle_t *icons ) {
	const rivalsLayout_t *l = &lib->layout;
	wsTextStyle_t bold;
	wsRect_t b;
	int row;

	WS_BodyStyle( &bold );
	bold.font = WS_BODY_BOLD;
	for ( row = 0; row < l->count; row++ ) {
		qboolean pressed = row == lib->selectedRow;
		float x0, y0, x1, y1, inner;
		Rivals_Button( l, row, &b );
		if ( b.y >= l->list.y + l->list.h || b.y + b.h <= l->list.y ) continue;
		x0 = WS_Round( b.x ); y0 = WS_Round( b.y ); x1 = WS_Round( b.x + b.w ); y1 = WS_Round( b.y + b.h );
		Library_Fill( lib, x0, y0, x1 - x0, y1 - y0, pressed ? wsAccent : wsLine );
		Library_Fill( lib, x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, pressed ? libSelected : wsSurface );
		inner = b.h - 2.0f - 2.0f * WS_LU( WS_BUTTON_PAD_Y );	/* align-items: center */
		if ( icons && icons[row] ) {
			/* object-fit: contain in 3.5rem x 2.5rem: the square icons are 40 x 40 */
			float iconX = WS_Round( b.x + 1.0f + WS_LU( WS_BUTTON_PAD_X ) + 8.0f );
			float iconY = WS_Round( b.y + 1.0f + WS_LU( WS_BUTTON_PAD_Y ) + ( inner - 40.0f ) * 0.5f );
			float top = iconY > l->list.y ? iconY : l->list.y;
			float bottom = iconY + 40.0f < l->list.y + l->list.h ? iconY + 40.0f : l->list.y + l->list.h;
			if ( bottom > top ) {
				trap_R_SetColor( NULL );
				trap_R_DrawStretchPic( iconX, top, 40.0f, bottom - top, 0.0f, ( top - iconY ) / 40.0f,
				                       1.0f, ( bottom - iconY ) / 40.0f, icons[row] );
			}
		}
		WS_SetFade( l->list.y, l->list.h, 0.0f );
		WS_DrawLines( &bold, b.x + l->rowTextX, b.y + 1.0f + WS_LU( WS_BUTTON_PAD_Y ) +
		              ( inner - l->rowLines[row].count * WS_BODY_LINE ) * 0.5f, l->rowText[row], &l->rowLines[row], wsWhite );
		WS_SetFade( 0.0f, 0.0f, 0.0f );
	}
}

/* Chrome's dark classic scrollbar: track, arrow buttons, rounded thumb */

void Library_DrawScrollbar( const wsLibrary_t *lib ) {
	if ( lib->layout.scrollbar ) WS_DrawScrollbar( &lib->layout.list, &lib->layout.thumb );
}

/* -------------------------------------------------------------------------
   Input
   ------------------------------------------------------------------------- */

void Library_Track( wsLibrary_t *lib ) {
	const rivalsLayout_t *l = &lib->layout;
	float x, y, track;

	if ( !lib->dragThumb ) return;
	if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
		lib->dragThumb = qfalse;
		return;
	}
	Library_Cursor( &x, &y );
	track = l->list.h - 2.0f * RIVALS_ARROW - 9.0f - l->thumb.h;
	if ( track > 0.0f ) {
		lib->scroll = lib->dragScroll + ( y - lib->dragY ) * ( l->contentHeight - l->list.h ) / track;
	}
}

void Library_ScrollTo( wsLibrary_t *lib, int row ) {
	const rivalsLayout_t *l = &lib->layout;
	if ( row < 0 || row >= l->count ) return;
	if ( l->rowY[row] < lib->scroll ) lib->scroll = l->rowY[row];
	if ( l->rowY[row] + l->rowH[row] > lib->scroll + l->list.h ) lib->scroll = l->rowY[row] + l->rowH[row] - l->list.h;
}

qboolean Library_Click( wsLibrary_t *lib, float x, float y, qboolean *changed ) {
	const rivalsLayout_t *l = &lib->layout;
	wsRect_t cancel;

	*changed = qfalse;
	lib->searchFocus = qfalse;
	if ( Library_Inside( &l->search, x, y ) ) {
		cancel = l->search;
		cancel.x = l->search.x + l->search.w - 1.0f - WS_LU( 0.6f * WS_REM ) - 16.0f;
		cancel.w = 16.0f + WS_LU( 0.6f * WS_REM );
		if ( lib->query[0] && Library_Inside( &cancel, x, y ) ) {
			lib->query[0] = '\0';
			*changed = qtrue;
		}
		lib->searchFocus = qtrue;
		lib->searchFocusTime = uis.realtime;
		return qtrue;
	}
	if ( l->scrollbar && Library_Inside( &l->list, x, y ) && x >= l->list.x + l->list.w - RIVALS_SCROLLBAR ) {
		if ( y < l->list.y + RIVALS_ARROW ) lib->scroll -= 40.0f;
		else if ( y >= l->list.y + l->list.h - RIVALS_ARROW ) lib->scroll += 40.0f;
		else if ( y < l->thumb.y ) lib->scroll -= 0.875f * l->list.h;
		else if ( y >= l->thumb.y + l->thumb.h ) lib->scroll += 0.875f * l->list.h;
		else {
			lib->dragThumb = qtrue;
			lib->dragY = y;
			lib->dragScroll = lib->scroll;
		}
		return qtrue;
	}
	return qfalse;
}

int Library_RowAt( const wsLibrary_t *lib, float x, float y ) {
	const rivalsLayout_t *l = &lib->layout;
	wsRect_t b;
	int row;

	if ( !Library_Inside( &l->list, x, y ) ) return -1;
	for ( row = 0; row < l->count; row++ ) {
		Rivals_Button( l, row, &b );
		if ( Library_Inside( &b, x, y ) ) return row;
	}
	return -1;
}

qboolean Library_Char( wsLibrary_t *lib, int ch ) {
	int length = strlen( lib->query );
	if ( !lib->searchFocus ) return qfalse;
	if ( ch == 8 ) {
		if ( !length ) return qfalse;
		lib->query[length - 1] = '\0';
	} else if ( ch >= 32 && ch != 127 && ch < 256 && length < (int)sizeof( lib->query ) - 1 ) {
		lib->query[length] = (char)ch;
		lib->query[length + 1] = '\0';
	} else {
		return qfalse;
	}
	lib->searchFocusTime = uis.realtime;
	return qtrue;
}
