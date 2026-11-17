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
const char *downloadsLabels[3] = { "Install content", "Refresh catalog", "Cancel transfer" };

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

static void Rivals_Rows( rivalsLayout_t *l ) {
	wsTextStyle_t bold;
	float y = 0.0f, text, minimum;
	int i;

	WS_BodyStyle( &bold );
	bold.font = WS_BODY_BOLD;
	text = l->buttonWidth - l->rowTextX - 1.0f - LU( WS_BUTTON_PAD_X );
	minimum = l->downloads ? WS_BODY_LINE : 2.5f * REM;
	if ( l->count > RIVALS_MAX ) l->count = RIVALS_MAX;
	for ( i = 0; i < l->count; i++ ) {
		float lines;
		WS_Wrap( &bold, l->rowText[i] ? l->rowText[i] : "", text, &l->rowLines[i] );
		lines = ( l->rowLines[i].count > 0 ? l->rowLines[i].count : 1 ) * WS_BODY_LINE;
		l->rowY[i] = y;
		l->rowH[i] = 2.0f * LU( WS_BUTTON_PAD_Y ) + 2.0f + ( lines > minimum ? lines : minimum );
		y += l->rowH[i] + l->rowGap;
	}
	l->contentHeight = l->count > 0 ? y - l->rowGap : 0.0f;
}

/* A block with vertical margins after the previous one: returns its top.
 * An empty block's margins collapse through it into the next. */
static float Rivals_Block( float *y, float *margin, float top, float height, float bottom ) {
	float at;
	if ( height <= 0.0f ) {
		if ( top > *margin ) *margin = top;
		if ( bottom > *margin ) *margin = bottom;
		return *y + *margin;
	}
	*y += top > *margin ? top : *margin;
	at = *y;
	*y += height;
	*margin = bottom;
	return at;
}

void Rivals_Layout( rivalsLayout_t *l, float width, float height ) {
	float top, y, leftH, rightH, column1, column2, pad, margin, maxScroll;

	if ( l->downloads ) {
		WS_FrontPageLabeled( &l->page, width, height, "Q3RALLY / DOWNLOADS", "Expand your ", "rally.", "" );
	} else {
		WS_FrontPageLabeled( &l->page, width, height, rivalsLabel, "Meet your ", "rivals.", "" );
	}
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
	y += l->search.h + LU( 0.5f * REM );
	memset( &l->type, 0, sizeof( l->type ) );
	if ( l->downloads ) {
		/* #download-filters: "Content type", then the select like the input */
		l->typeLabelY = y;
		y += WS_BODY_LINE + LU( 0.5f * REM );
		l->type.x = l->leftX;
		l->type.y = y;
		l->type.w = l->leftW;
		l->type.h = 2.0f * LU( 0.6f * REM ) + 2.0f + 21.0f;	/* a select's line is 21px */
		y += l->type.h + LU( 0.5f * REM );
	}
	y += WS_BLOCK_MARGIN;
	l->countY = y;
	y += WS_BODY_LINE + WS_BLOCK_MARGIN;
	/* the buttons: the 2.5rem icon or the wrapped name, whichever is taller;
	 * the list's scrollbar takes 15px from them where it overflows */
	l->rowGap = LU( 0.4f * REM );
	l->rowTextX = 1.0f + LU( WS_BUTTON_PAD_X ) + ( l->downloads ? 0.0f : 3.5f * REM + LU( 0.7f * REM ) );
	l->list.x = l->leftX;
	l->list.y = y;
	l->list.w = l->leftW;
	l->buttonWidth = l->leftW;
	Rivals_Rows( l );
	l->list.h = l->contentHeight < LU( 0.46f * height ) ? l->contentHeight : LU( 0.46f * height );
	l->scrollbar = l->contentHeight > l->list.h + 0.5f;
	if ( l->scrollbar && !l->hideScrollbar ) {
		l->buttonWidth = l->leftW - RIVALS_SCROLLBAR;
		Rivals_Rows( l );
	}
	maxScroll = l->contentHeight - l->list.h;
	l->scroll = l->scrollbar ? WS_Clamp( 0.0f, floor( l->scroll + 0.5f ), floor( maxScroll + 0.5f ) ) : 0.0f;
	y += l->list.h;
	leftH = y + pad + 1.0f;

	/* right: the showroom section with the rival's profile */
	l->right.x = l->page.oneColumn ? l->page.contentX : l->page.contentX + column1 + 1.5f * REM;
	l->right.w = column2;
	l->rightX = l->right.x + 1.0f + pad;
	l->rightW = column2 - 2.0f - 2.0f * pad;
	/* block by block; adjoining vertical margins collapse, empty paragraphs
	 * let theirs collapse through */
	y = 1.0f + pad;
	margin = 0.0f;
	l->labelY = y;
	y += LU( WS_LABEL_LINE );
	l->title.font = WS_CONDENSED;
	l->title.size = LU( WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM ) );
	l->title.spacing = 0.0f;
	l->title.lineHeight = l->title.size;
	l->title.uppercase = qtrue;
	WS_StyledWrap( &l->title, l->name, l->rightW, &l->nameLines );
	l->nameY = Rivals_Block( &y, &margin, WS_HEADING_MARGIN,
		( l->nameLines.count > 0 ? l->nameLines.count : 1 ) * l->title.size, WS_HEADING_MARGIN );
	memset( &l->car, 0, sizeof( l->car ) );
	if ( !l->downloads || l->imageShown ) {
		l->car.x = l->rightX;
		l->car.w = l->rightW;
		l->car.h = LU( WS_Clamp( 10.0f * REM, 0.32f * height, 23.0f * REM ) );
		l->car.y = Rivals_Block( &y, &margin, 0.0f, l->car.h, 0.0f );
	}
	l->authorY = Rivals_Block( &y, &margin, WS_BLOCK_MARGIN, Rivals_Paragraph( l->author, l->rightW, &l->authorLines ), WS_BLOCK_MARGIN );
	l->personalityY = Rivals_Block( &y, &margin, WS_BLOCK_MARGIN,
		Rivals_Paragraph( l->personality, l->rightW, &l->personalityLines ), WS_BLOCK_MARGIN );
	if ( l->weapon[0] ) {	/* hidden without a weapon */
		l->weaponY = Rivals_Block( &y, &margin, WS_BLOCK_MARGIN, Rivals_Paragraph( l->weapon, l->rightW, &l->weaponLines ),
			WS_BLOCK_MARGIN );
	} else {
		memset( &l->weaponLines, 0, sizeof( l->weaponLines ) );
	}
	l->descriptionY = Rivals_Block( &y, &margin, WS_BLOCK_MARGIN,
		Rivals_Paragraph( l->description, l->rightW, &l->descriptionLines ), WS_BLOCK_MARGIN );
	l->statusY = Rivals_Block( &y, &margin, WS_BLOCK_MARGIN, Rivals_Paragraph( l->status, l->rightW, &l->statusLines ),
		WS_BLOCK_MARGIN );
	if ( l->downloads ) {
		/* #download-controls: flex-wrap, gap .5rem: three buttons, then the
		 * progress and the status at full width (flex margins stay) */
		wsRect_t *buttons[3];
		float x, line, gap = LU( 0.5f * REM );
		int b;
		buttons[0] = &l->install; buttons[1] = &l->refresh; buttons[2] = &l->cancel;
		y += margin;	/* the div has no margin of its own */
		margin = 0.0f;
		x = l->rightX;
		line = y;
		for ( b = 0; b < 3; b++ ) {
			buttons[b]->w = WS_ButtonWidth( downloadsLabels[b] );
			buttons[b]->h = WS_BUTTON_HEIGHT;
			if ( x > l->rightX && x + buttons[b]->w > l->rightX + l->rightW + 0.01f ) {
				x = l->rightX;
				line += WS_BUTTON_HEIGHT + gap;
			}
			buttons[b]->x = x;
			buttons[b]->y = line;
			x += buttons[b]->w + gap;
		}
		y = line + WS_BUTTON_HEIGHT + gap;
		l->progress.x = l->rightX;
		l->progress.y = y;
		l->progress.w = l->rightW;
		l->progress.h = REM;
		y += REM + gap + WS_BLOCK_MARGIN;
		l->downloadStatusY = y;
		y += Rivals_Paragraph( l->downloadStatus, l->rightW, &l->downloadStatusLines ) + WS_BLOCK_MARGIN;
	}
	rightH = y + margin + pad + 1.0f;	/* the last margin stays: the section has padding */

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
	l->typeLabelY += l->left.y;
	l->type.y += l->left.y;
	l->labelY += l->right.y;
	l->nameY += l->right.y;
	l->car.y += l->right.y;
	l->authorY += l->right.y;
	l->install.y += l->right.y;
	l->refresh.y += l->right.y;
	l->cancel.y += l->right.y;
	l->progress.y += l->right.y;
	l->downloadStatusY += l->right.y;
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
	button->y = l->list.y - l->scroll + l->rowY[row];
	button->h = l->rowH[row];
}

void WS_ScrollThumb( const wsRect_t *port, float content, float scroll, wsRect_t *thumb ) {
	float track = port->h - 2.0f * RIVALS_ARROW - 9.0f;
	thumb->h = content > 0.0f ? track * port->h / content : track;
	if ( thumb->h < 9.0f ) thumb->h = 9.0f;
	thumb->w = 9.0f;
	thumb->x = port->x + port->w - RIVALS_SCROLLBAR + 3.0f;
	thumb->y = port->y + RIVALS_ARROW + 2.5f +
		( content > port->h ? ( track - thumb->h ) * scroll / ( content - port->h ) : 0.0f );
}

