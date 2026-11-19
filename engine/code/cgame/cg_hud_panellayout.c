/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS of the WebUI's HUD options, rule for rule (see the header).
*/

#ifdef CGAME
#include "cg_local.h"
#endif
#include "cg_hud_panellayout.h"

#define REM		WS_REM
#define LU		WS_LU

const char *hudPanelLabel = "Q3RALLY / LIVE HUD";
const char *hudPanelTitle = "Your race.";
const char *hudPanelEm = "display.";		/* after "Your " on the second line */
const char *hudPanelText = "Changes apply immediately. Settings unavailable in this mode are disabled.";
const char *hudPanelHint = "SHIFT + H / ESC TO CLOSE";

#define HUDPANEL_GAP		LU( 0.4f * REM )				/* label { display: grid; gap: .4rem } */
#define HUDPANEL_PAD		LU( 0.6f * REM )				/* select, input: padding .6rem */
#define HUDPANEL_SELECT		( 2.0f * HUDPANEL_PAD + 2.0f + 21.0f )
#define HUDPANEL_H3			( 1.17f * REM )					/* h3: 1.17em, margin 1em, line 1.5 */

void HUDPanel_Layout( hudPanelLayout_t *l, float width, float height ) {
	wsTextStyle_t body;
	float y, top;
	int i;

	WS_BodyStyle( &body );
	/* position: fixed; right: 0 (inside the page's scrollbar gutter);
	 * width: clamp(21rem, 42vw, 36rem), all of it below 40rem */
	l->panel.w = width <= 40.0f * REM ? width : WS_Clamp( 21.0f * REM, 0.42f * width, 36.0f * REM );
	if ( l->panel.w > width ) l->panel.w = width;
	l->panel.x = width - WS_SCROLLBAR_GUTTER - l->panel.w;
	if ( width <= 40.0f * REM ) l->panel.x = 0.0f;
	l->panel.y = 0.0f;
	l->panel.h = height;
	l->x = l->panel.x + 3.0f + 1.5f * REM;
	l->w = l->panel.w - 3.0f - 3.0f * REM;

	/* header (margin-bottom 1rem): the label, h2 "Your race.<br>Your display." */
	top = y = 1.5f * REM;
	l->labelY = y;
	l->h2Size = LU( WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM ) );
	l->h2Y = y + LU( WS_LABEL_LINE ) + LU( WS_HEADING_MARGIN );
	y = l->h2Y + 2.0f * l->h2Size + LU( WS_HEADING_MARGIN ) + REM;
	{
		wsTextStyle_t h2;
		h2.font = WS_CONDENSED;
		h2.size = h2.lineHeight = l->h2Size;
		h2.spacing = 0.0f;
		h2.uppercase = qtrue;
		l->emX = l->x + WS_StyledWidth( &h2, "Your " );
	}

	/* p, then .session-actions (flex-wrap, gap .6rem, margin-bottom 1.5rem) */
	l->textY = y;
	WS_Wrap( &body, hudPanelText, l->w, &l->textLines );
	y += l->textLines.count * WS_BODY_LINE + REM;
	l->close.w = WS_ButtonWidth( "Back to race" );
	l->console.w = WS_ButtonWidth( "Console" );
	l->close.x = l->x;
	l->close.y = y;
	l->close.h = l->console.h = WS_BUTTON_HEIGHT;
	if ( l->close.w + HUDPANEL_PAD + l->console.w <= l->w + 0.01f ) {
		l->console.x = l->x + l->close.w + HUDPANEL_PAD;
		l->console.y = y;
	} else {
		l->console.x = l->x;
		l->console.y = y + WS_BUTTON_HEIGHT + HUDPANEL_PAD;
	}
	y = l->console.y + WS_BUTTON_HEIGHT + 1.5f * REM;

	/* #hud-fields: a label per toggle (margin 1rem 0, collapsing): its text,
	 * then the select */
	for ( i = 0; i < l->items; i++ ) {
		if ( i ) y += REM;
		l->itemY[i] = y;
		WS_Wrap( &body, l->itemLabel[i] ? l->itemLabel[i] : "", l->w, &l->itemLines[i] );
		y += l->itemLines[i].count * WS_BODY_LINE + HUDPANEL_GAP;
		l->select[i].x = l->x;
		l->select[i].y = y;
		l->select[i].w = l->w;
		l->select[i].h = HUDPANEL_SELECT;
		y += HUDPANEL_SELECT;
	}
	if ( !l->items ) y -= 1.5f * REM - REM;

	/* h3 Mini-map, then the two range labels: text, output, range input
	 * (padding 0, 2px margins) */
	y += HUDPANEL_H3;
	l->h3Y = y;
	y += LU( 1.5f * HUDPANEL_H3 ) + HUDPANEL_H3;
	l->sizeY = y;
	l->sizeOutputY = y + WS_BODY_LINE + HUDPANEL_GAP;
	l->size.x = l->x + 2.0f;
	l->size.y = l->sizeOutputY + WS_BODY_LINE + HUDPANEL_GAP + 2.0f;
	l->size.w = l->w;
	l->size.h = 18.0f;
	y = l->size.y + l->size.h + 2.0f + REM;
	l->zoomY = y;
	l->zoomOutputY = y + WS_BODY_LINE + HUDPANEL_GAP;
	l->zoom = l->size;
	l->zoom.y = l->zoomOutputY + WS_BODY_LINE + HUDPANEL_GAP + 2.0f;
	y = l->zoom.y + l->zoom.h + 2.0f + REM;
	l->hintY = y;
	y += LU( WS_LABEL_LINE );
	l->contentHeight = y + 1.5f * REM;

	/* overflow-y: auto */
	l->scrollbar = l->contentHeight > height + 0.5f;
	l->scroll = l->scrollbar ? WS_Clamp( 0.0f, floor( l->scroll + 0.5f ), floor( l->contentHeight - height + 0.5f ) ) : 0.0f;
	memset( &l->thumb, 0, sizeof( l->thumb ) );
	if ( l->scrollbar ) {
		float dy = -l->scroll;
		WS_ScrollThumb( &l->panel, l->contentHeight, l->scroll, &l->thumb );
		l->labelY += dy; l->h2Y += dy; l->textY += dy;
		l->close.y += dy; l->console.y += dy;
		for ( i = 0; i < l->items; i++ ) { l->itemY[i] += dy; l->select[i].y += dy; }
		l->h3Y += dy; l->sizeY += dy; l->sizeOutputY += dy; l->zoomY += dy; l->zoomOutputY += dy;
		l->size.y += dy; l->zoom.y += dy; l->hintY += dy;
	}
	( void )top;
}
