/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS of the WebUI's Rivals screen, rule for rule (see the header).
*/

#include "ui_local.h"
#include "ui_rally_rivalslayout.h"

const char *rivalsLabel = "Q3RALLY / RIVALS";
const char *rivalsBack = "Back to main menu";
const char *rivalsDetailLabel = "RIVAL PROFILE";
const char *rivalsPlaceholder = "Find a rival";

#define REM		WS_REM
#define LU		WS_LU

/* a p of body text: its lines, its height (an empty p has none) */
static float Rivals_Paragraph( const char *text, float width, wsLines_t *lines ) {
	wsTextStyle_t body;
	WS_BodyStyle( &body );
	if ( !text[0] ) {
		memset( lines, 0, sizeof( *lines ) );
		return 0.0f;
	}
	WS_Wrap( &body, text, width, lines );
	return ( lines->count > 0 ? lines->count : 1 ) * WS_BODY_LINE;
}

void Rivals_Layout( rivalsLayout_t *l, float width, float height ) {
	float top, y, leftH, rightH, column1, column2, pad, span, maxScroll;

	WS_FrontPageLabeled( &l->page, width, height, rivalsLabel, "Meet your ", "rivals.", "" );
	l->pageWidth = l->page.pageWidth;
	top = WS_PlaceHeaderButton( &l->page, rivalsBack, &l->back );

	/* .library-layout: minmax(14rem, .7fr) minmax(0, 1.5fr), gap 1.5rem */
	if ( !l->page.oneColumn ) {
		column1 = ( l->page.contentW - 1.5f * REM ) * 0.7f / 2.2f;
		if ( column1 < 14.0f * REM ) column1 = 14.0f * REM;
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

	/* left: "Search" on its line, the input (width 100%, margin .5rem 0),
	 * the count, the list (max-height 46vh, overflow auto) */
	y = 1.0f + pad;
	l->searchLabelY = y;
	y += WS_BODY_LINE + LU( 0.5f * REM );
	l->search.x = l->leftX;
	l->search.y = y;
	l->search.w = l->leftW;
	l->search.h = 2.0f * LU( 0.6f * REM ) + WS_BODY_LINE + 2.0f;
	/* the input's margin is part of its line, the p's margin adds to it */
	y += l->search.h + LU( 0.5f * REM ) + WS_BLOCK_MARGIN;
	l->countY = y;
	y += WS_BODY_LINE + WS_BLOCK_MARGIN;
	l->rowHeight = 2.0f * LU( 0.65f * REM ) + 2.0f + 2.5f * REM;	/* the 2.5rem icon is the tallest */
	l->rowGap = LU( 0.4f * REM );
	l->contentHeight = l->count > 0 ? l->count * l->rowHeight + ( l->count - 1 ) * l->rowGap : 0.0f;
	l->list.x = l->leftX;
	l->list.y = y;
	l->list.w = l->leftW;
	l->list.h = l->contentHeight < LU( 0.46f * height ) ? l->contentHeight : LU( 0.46f * height );
	l->scrollbar = l->contentHeight > l->list.h + 0.5f;
	l->buttonWidth = l->scrollbar ? l->leftW - RIVALS_SCROLLBAR : l->leftW;
	maxScroll = l->contentHeight - l->list.h;
	l->scroll = l->scrollbar ? WS_Clamp( 0.0f, floor( l->scroll + 0.5f ), floor( maxScroll + 0.5f ) ) : 0.0f;
	y += l->list.h;
	leftH = y + pad + 1.0f;

	/* right: the showroom section with the rival's profile */
	l->right.x = l->page.oneColumn ? l->page.contentX : l->page.contentX + column1 + 1.5f * REM;
	l->right.w = column2;
	l->rightX = l->right.x + 1.0f + pad;
	l->rightW = column2 - 2.0f - 2.0f * pad;
	y = 1.0f + pad;
	l->labelY = y;
	y += LU( WS_LABEL_LINE ) + WS_HEADING_MARGIN;
	l->title.font = WS_CONDENSED;
	l->title.size = LU( WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM ) );
	l->title.spacing = 0.0f;
	l->title.lineHeight = l->title.size;
	l->title.uppercase = qtrue;
	WS_StyledWrap( &l->title, l->name, l->rightW, &l->nameLines );
	l->nameY = y;
	y += ( l->nameLines.count > 0 ? l->nameLines.count : 1 ) * l->title.size + WS_HEADING_MARGIN;
	l->car.x = l->rightX;
	l->car.y = y;
	l->car.w = l->rightW;
	l->car.h = LU( WS_Clamp( 10.0f * REM, 0.32f * height, 23.0f * REM ) );
	y += l->car.h + WS_BLOCK_MARGIN;	/* the empty author line collapses into it */
	l->personalityY = y;
	span = Rivals_Paragraph( l->personality, l->rightW, &l->personalityLines );
	y += span + ( span > 0.0f ? WS_BLOCK_MARGIN : 0.0f );
	l->weaponY = y;
	span = Rivals_Paragraph( l->weapon, l->rightW, &l->weaponLines );
	y += span + ( span > 0.0f ? WS_BLOCK_MARGIN : 0.0f );
	l->descriptionY = y;
	span = Rivals_Paragraph( l->description, l->rightW, &l->descriptionLines );
	y += span + ( span > 0.0f ? WS_BLOCK_MARGIN : 0.0f );
	l->statusY = y;
	span = Rivals_Paragraph( l->status, l->rightW, &l->statusLines );
	y += span;
	rightH = y + WS_BLOCK_MARGIN + pad + 1.0f;	/* the last margin stays: the section has padding */

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
	l->searchLabelY += l->left.y;
	l->search.y += l->left.y;
	l->countY += l->left.y;
	l->list.y += l->left.y;
	l->labelY += l->right.y;
	l->nameY += l->right.y;
	l->car.y += l->right.y;
	l->personalityY += l->right.y;
	l->weaponY += l->right.y;
	l->descriptionY += l->right.y;
	l->statusY += l->right.y;
	l->pageHeight = l->right.y + l->right.h + l->page.pad;
	if ( l->left.y + l->left.h + l->page.pad > l->pageHeight ) l->pageHeight = l->left.y + l->left.h + l->page.pad;

	/* the scrollbar thumb: 9px wide, its length the visible share of the track */
	memset( &l->thumb, 0, sizeof( l->thumb ) );
	if ( l->scrollbar ) {
		float track = l->list.h - 2.0f * RIVALS_ARROW - 9.0f;
		l->thumb.h = track * l->list.h / l->contentHeight;
		if ( l->thumb.h < 9.0f ) l->thumb.h = 9.0f;
		l->thumb.w = 9.0f;
		l->thumb.x = l->list.x + l->list.w - RIVALS_SCROLLBAR + 3.0f;
		l->thumb.y = l->list.y + RIVALS_ARROW + 2.5f + ( track - l->thumb.h ) * l->scroll / maxScroll;
	}
}

void Rivals_Button( const rivalsLayout_t *l, int row, wsRect_t *button ) {
	button->x = l->list.x;
	button->w = l->buttonWidth;
	button->y = l->list.y - l->scroll + row * ( l->rowHeight + l->rowGap );
	button->h = l->rowHeight;
}
