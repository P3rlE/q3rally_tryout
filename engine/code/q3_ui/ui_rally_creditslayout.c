/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS and the roll animation of the WebUI's end credits (see the header).
*/

#include "ui_local.h"
#include "ui_rally_creditslayout.h"

const char *creditsLabel = "Q3RALLY / THE PEOPLE BEHIND THE RALLY";
const char *creditsExitLabel = "Exit game";
const char *creditsHint = "PRESS ANY KEY TO EXIT";

#define REM		WS_REM
#define LU		WS_LU

void Credits_Layout( creditsLayout_t *l, float width, float height, const int *kinds, int count ) {
	float y, margin, previous;
	int i;

	WS_FrontPageLabeled( &l->page, width, height, creditsLabel, "Thank you for ", "playing.", "" );
	l->pageWidth = l->page.pageWidth;

	/* header: the title block and the exit button, space-between; the button
	 * wraps below when both do not fit with the 1rem gap */
	l->exit.w = WS_ButtonWidth( creditsExitLabel );
	l->exit.h = WS_BUTTON_HEIGHT;
	if ( l->page.titleW + REM + l->exit.w <= l->page.contentW ) {
		l->exit.x = l->page.contentX + l->page.contentW - l->exit.w;
		l->exit.y = l->page.pad;
		y = l->page.pad + l->page.titleH;
	} else {
		l->exit.x = l->page.contentX;
		l->exit.y = l->page.pad + l->page.titleH + REM;
		y = l->exit.y + l->exit.h;
	}

	/* #credits-window: 60vh, then the hint label right below */
	l->window.x = l->page.contentX;
	l->window.w = l->page.contentW;
	l->window.y = y + 2.0f * REM;
	l->window.h = LU( 0.6f * height );
	l->hintY = l->window.y + l->window.h;
	l->pageHeight = l->hintY + LU( WS_LABEL_LINE ) + l->page.pad;

	/* #credits-roll: p (.6rem margins), p.label, h2 (1.3rem margins, line
	 * height 1); adjoining margins collapse, the first one into the roll */
	l->title.font = WS_CONDENSED;
	l->title.size = LU( WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM ) );
	l->title.spacing = 0.0f;
	l->title.lineHeight = l->title.size;
	l->title.uppercase = qtrue;
	if ( count > CREDITS_MAX_LINES ) count = CREDITS_MAX_LINES;
	l->count = count;
	y = 0.0f;
	previous = 0.0f;
	l->rollTop = 0.0f;
	for ( i = 0; i < count; i++ ) {
		margin = kinds[i] == CREDITS_TITLE ? LU( 1.3f * REM ) : LU( 0.6f * REM );
		if ( i == 0 ) {
			l->rollTop = margin;
		} else {
			y += previous > margin ? previous : margin;
		}
		l->lineY[i] = y;
		l->lineH[i] = kinds[i] == CREDITS_TITLE ? l->title.size :
		              kinds[i] == CREDITS_HEADING ? LU( WS_LABEL_LINE ) : WS_BODY_LINE;
		y += l->lineH[i];
		previous = margin;
	}
	l->rollHeight = floor( y + 0.5f );
	/* translateY(window height) to translateY(-scrollHeight) at 38 px/s */
	l->duration = ( l->window.h + l->rollHeight ) / CREDITS_SPEED;
}

float Credits_LineTop( const creditsLayout_t *l, int line, float seconds ) {
	return l->window.y + l->rollTop + l->window.h - CREDITS_SPEED * seconds + l->lineY[line];
}
