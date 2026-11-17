/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS of the WebUI's demo archive, rule for rule (see the header).
*/

#include "ui_local.h"
#include "ui_rally_demoslayout.h"
#include "ui_rally_rivalslayout.h"

const char *demosTransport[5] = { "Restart", "-10 s", "Pause", "+10 s", "Stop" };

#define REM		WS_REM
#define LU		WS_LU

void Demos_WrapAnywhere( const wsTextStyle_t *style, const char *text, float width, wsLines_t *lines ) {
	int start = 0, length = strlen( text );

	memset( lines, 0, sizeof( *lines ) );
	while ( start < length && lines->count < WS_MAX_LINES ) {
		int end = start, lastSpace = -1, i;
		/* the longest run that fits, ending at a space if one fits */
		for ( i = start + 1; i <= length; i++ ) {
			if ( WS_TextWidth( style, text + start, i - start ) > width + 0.01f ) break;
			end = i;
			if ( i < length && text[i] == ' ' ) lastSpace = i;
		}
		if ( end < length && lastSpace > start ) end = lastSpace;
		if ( end <= start ) end = start + 1;
		lines->start[lines->count] = start;
		lines->length[lines->count] = end - start;
		lines->width[lines->count] = WS_TextWidth( style, text + start, end - start );
		lines->count++;
		start = end;
		while ( start < length && text[start] == ' ' ) start++;
	}
}

void Demos_Layout( demosLayout_t *l, float width, float height ) {
	wsTextStyle_t body, detail;
	float top, y, column1, column2, pad, leftH, rightH, maxScroll, x, space;
	int i;

	WS_BodyStyle( &body );
	detail = body;
	detail.size = 0.9f * REM;
	detail.lineHeight = DEMOS_DETAIL_LINE;
	WS_FrontPageLabeled( &l->page, width, height, "Q3RALLY / DEMO ARCHIVE", "Your ", "replays.", "" );
	l->pageWidth = l->page.pageWidth;
	top = WS_PlaceHeaderButton( &l->page, "Back to main menu", &l->back );

	/* .archive-layout: minmax(12rem, .6fr) minmax(0, 1.6fr), gap 1.5rem */
	if ( !l->page.oneColumn ) {
		column1 = ( l->page.contentW - 1.5f * REM ) * 0.6f / 2.2f;
		if ( column1 < 12.0f * REM ) column1 = 12.0f * REM;
		column2 = l->page.contentW - 1.5f * REM - column1;
		if ( column2 < 0.0f ) column2 = 0.0f;
	} else {
		column1 = column2 = l->page.contentW;
	}
	pad = l->page.sectionPad;
	l->left.x = l->page.contentX;
	l->left.w = column1;
	l->leftX = l->left.x + 1.0f + pad;
	l->leftW = column1 - 2.0f - 2.0f * pad;

	/* left: label, "Choose a recording", the listbox (margin-top .7rem,
	 * height clamp(10rem, 32vh, 24rem)), the details (.9rem), Play */
	y = 1.0f + pad;
	l->recordingsY = y;
	y += LU( WS_LABEL_LINE );
	l->chooseY = y;
	y += WS_BODY_LINE + LU( 0.7f * REM );
	l->list.x = l->leftX;
	l->list.y = y;
	l->list.w = l->leftW;
	l->list.h = LU( WS_Clamp( 10.0f * REM, 0.32f * height, 24.0f * REM ) );
	l->port.x = l->list.x + 1.0f;
	l->port.y = l->list.y + 1.0f;
	l->port.w = l->list.w - 2.0f;
	l->port.h = l->list.h - 2.0f;
	l->optionHeight = 2.0f * LU( 0.4f * REM ) + 19.0f;	/* padding .4rem, the option's normal line */
	l->contentHeight = l->count * l->optionHeight;
	l->scrollbar = l->contentHeight > l->port.h + 0.5f;
	l->optionWidth = l->scrollbar && !l->hideScrollbar ? l->port.w - 15.0f : l->port.w;
	maxScroll = l->contentHeight - l->port.h;
	l->scroll = l->scrollbar ? WS_Clamp( 0.0f, floor( l->scroll + 0.5f ), floor( maxScroll + 0.5f ) ) : 0.0f;
	memset( &l->thumb, 0, sizeof( l->thumb ) );
	y += l->list.h + LU( 0.9f * REM );
	l->detailsY = y;
	Demos_WrapAnywhere( &detail, l->details, l->leftW, &l->detailLines );
	y += ( l->detailLines.count > 0 ? l->detailLines.count : 1 ) * DEMOS_DETAIL_LINE + LU( 0.9f * REM );
	l->play.x = l->leftX;
	l->play.y = y;
	l->play.w = WS_ButtonWidth( "Play demo" );
	l->play.h = WS_BUTTON_HEIGHT;
	y += WS_BUTTON_HEIGHT;
	leftH = y + pad + 1.0f;

	/* right: label, the viewport (clamp(10rem, 42vh, 28rem), margin .75rem 0),
	 * the transport (flex-wrap, gap .5rem), clock and message */
	l->right.x = l->page.oneColumn ? l->page.contentX : l->page.contentX + column1 + 1.5f * REM;
	l->right.w = column2;
	l->rightX = l->right.x + 1.0f + pad;
	l->rightW = column2 - 2.0f - 2.0f * pad;
	y = 1.0f + pad;
	l->previewY = y;
	y += LU( WS_LABEL_LINE ) + LU( 0.75f * REM );
	l->viewport.x = l->rightX;
	l->viewport.y = y;
	l->viewport.w = l->rightW;
	l->viewport.h = LU( WS_Clamp( 10.0f * REM, 0.42f * height, 28.0f * REM ) );
	l->placeholderX = l->viewport.x + ( l->viewport.w - WS_TextWidth( &body, l->placeholderText, strlen( l->placeholderText ) ) ) * 0.5f;
	l->placeholderY = l->viewport.y + ( l->viewport.h - WS_BODY_LINE ) * 0.5f;
	y += l->viewport.h + LU( 0.75f * REM );
	x = l->rightX;
	for ( i = 0; i < 5; i++ ) {
		l->transport[i].w = WS_ButtonWidth( i == 2 ? l->pause : demosTransport[i] );
		l->transport[i].h = WS_BUTTON_HEIGHT;
		if ( x > l->rightX && x + l->transport[i].w > l->rightX + l->rightW + 0.01f ) {
			x = l->rightX;
			y += WS_BUTTON_HEIGHT + LU( 0.5f * REM );
		}
		l->transport[i].x = x;
		l->transport[i].y = y;
		x += l->transport[i].w + LU( 0.5f * REM );
	}
	y += WS_BUTTON_HEIGHT + WS_BLOCK_MARGIN;
	l->clockY = y;
	l->clockX = l->rightX;
	space = WS_TextWidth( &body, " ", 1 );
	l->messageX = l->clockX + WS_TextWidth( &body, l->clock, strlen( l->clock ) ) + space + LU( 0.8f * REM );
	y += WS_BODY_LINE + WS_BLOCK_MARGIN;
	rightH = y + pad + 1.0f;

	/* one row as high as the taller section; or stacked */
	l->left.y = top;
	if ( !l->page.oneColumn ) {
		if ( leftH < rightH ) leftH = rightH;
		rightH = leftH;
		l->right.y = top;
	} else {
		l->right.y = top + leftH + 1.5f * REM;
	}
	l->left.h = leftH;
	l->right.h = rightH;
	l->recordingsY += l->left.y;
	l->chooseY += l->left.y;
	l->list.y += l->left.y;
	l->port.y += l->left.y;
	l->detailsY += l->left.y;
	l->play.y += l->left.y;
	l->previewY += l->right.y;
	l->viewport.y += l->right.y;
	l->placeholderY += l->right.y;
	for ( i = 0; i < 5; i++ ) l->transport[i].y += l->right.y;
	l->clockY += l->right.y;
	l->pageHeight = l->right.y + l->right.h + l->page.pad;
	if ( l->left.y + l->left.h + l->page.pad > l->pageHeight ) l->pageHeight = l->left.y + l->left.h + l->page.pad;
	if ( l->scrollbar ) WS_ScrollThumb( &l->port, l->contentHeight, l->scroll, &l->thumb );
}
