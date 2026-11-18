/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS of step 1 of the WebUI's offline race setup, rule for rule (see the
header).
*/

#include "ui_local.h"
#include "ui_rally_offlinelayout.h"
#include "ui_rally_demoslayout.h"
#include "ui_rally_rivalslayout.h"

#define REM		WS_REM
#define LU		WS_LU

static const char *offlineSteps[2] = { "01 / TRACK & MODE", "02 / RULES & DRIVERS" };

static void Offline_Rows( offlineLayout_t *l ) {
	wsTextStyle_t bold;
	float y = 0.0f, text;
	int i;

	WS_BodyStyle( &bold );
	bold.font = WS_BODY_BOLD;
	/* .offline-map: padding, the 6rem image, a 1rem gap, the title */
	text = l->rowWidth - 2.0f - 2.0f * LU( WS_BUTTON_PAD_X ) - 6.0f * REM - REM;
	for ( i = 0; i < l->count; i++ ) {
		float lines;
		WS_Wrap( &bold, l->rowText[i] ? l->rowText[i] : "", text, &l->rowLines[i] );
		lines = ( l->rowLines[i].count > 0 ? l->rowLines[i].count : 1 ) * WS_BODY_LINE;
		l->rowY[i] = y;
		l->rowH[i] = 2.0f * LU( WS_BUTTON_PAD_Y ) + 2.0f + ( lines > 3.5f * REM ? lines : 3.5f * REM );
		y += l->rowH[i] + LU( 0.5f * REM );
	}
	l->contentHeight = l->count > 0 ? y - LU( 0.5f * REM ) : WS_BODY_LINE + 2.0f * WS_BLOCK_MARGIN;
}

/* #offline-map-details: repeat(auto-fit, minmax(min(100%, 12rem), 1fr)) */
static float Offline_Details( offlineLayout_t *l, float width, float rowGap, float columnGap ) {
	int columns, i, row;
	float y = 0.0f, column, rowHeight = 0.0f;

	columns = (int)( ( width + columnGap ) / ( 12.0f * REM + columnGap ) );
	if ( columns < 1 ) columns = 1;
	if ( columns > l->details && l->details > 0 ) {
		/* auto-fit collapses the empty tracks: the items share the width */
		columns = l->details;
	}
	column = ( width - ( columns - 1 ) * columnGap ) / columns;
	l->detailW = column;
	for ( i = 0; i < l->details; i++ ) {
		row = i / columns;
		if ( i % columns == 0 && row > 0 ) {
			y += rowHeight + rowGap;
			rowHeight = 0.0f;
		}
		Demos_WrapAnywhere( &l->detailStyle, l->detail[i], column, &l->detailLines[i] );
		l->detailX[i] = ( i % columns ) * ( column + columnGap );
		l->detailY[i] = y;
		if ( l->detailLines[i].count * l->detailStyle.lineHeight > rowHeight ) {
			rowHeight = l->detailLines[i].count * l->detailStyle.lineHeight;
		}
	}
	return l->details > 0 ? y + rowHeight : 0.0f;
}

float Offline_Header( offlineHeader_t *h, float width, float height, qboolean hosting, const char *back ) {
	wsTextStyle_t label, h1, h1em, mono = { WS_MONO, 0.9f * REM, 0.0f, 16.0f, qfalse };
	float main, titleW, top;
	const char *labelText = hosting ? "Q3RALLY / HOST A SERVER" : "Q3RALLY / OFFLINE";

	h->desktop = width >= 64.0f * REM && height >= 44.0f * REM;
	WS_LabelStyle( &label );
	h->pageWidth = width - WS_SCROLLBAR_GUTTER;
	main = h->pageWidth < 105.0f * REM ? h->pageWidth : 105.0f * REM;
	h->padX = WS_Clamp( REM, 0.04f * width, 5.0f * REM );
	h->padY = h->desktop ? WS_Clamp( REM, 0.03f * height, 2.0f * REM ) : h->padX;
	h->contentX = ( h->pageWidth - main ) * 0.5f + h->padX;
	h->contentW = main - 2.0f * h->padX;

	/* the label and the h1 at the left, Back at the right */
	h->h1Size = h->desktop ? WS_Clamp( 3.0f * REM, 0.06f * width, 5.5f * REM ) : WS_Clamp( 3.0f * REM, 0.08f * width, 7.0f * REM );
	h1.font = WS_CONDENSED; h1.size = h->h1Size; h1.spacing = 0.0f; h1.lineHeight = h->h1Size; h1.uppercase = qtrue;
	h1em = h1; h1em.font = WS_CONDENSED_ITALIC;
	titleW = WS_StyledWidth( &h1, "Your next " ) + WS_StyledWidth( &h1em, "race." );
	if ( WS_StyledWidth( &label, labelText ) > titleW ) titleW = WS_StyledWidth( &label, labelText );
	h->labelY = h->padY;
	h->h1Y = h->padY + LU( WS_LABEL_LINE ) + WS_HEADING_MARGIN;
	top = h->h1Y + h->h1Size + WS_HEADING_MARGIN;
	h->back.w = WS_ButtonWidth( back );
	h->back.h = WS_BUTTON_HEIGHT;
	if ( titleW + REM + h->back.w <= h->contentW ) {
		h->back.x = h->contentX + h->contentW - h->back.w;
		h->back.y = h->padY;
	} else {
		h->back.x = h->contentX;
		h->back.y = top + REM;
		top = h->back.y + h->back.h;
	}
	top += h->desktop ? REM : 2.0f * REM;

	/* .offline-steps: flex, gap 2rem, .9rem Share Tech Mono (line-height normal) */
	h->stepY = top;
	h->stepRulesX = h->contentX + WS_TextWidth( &mono, offlineSteps[0], strlen( offlineSteps[0] ) ) + 2.0f * REM;
	return top + 16.0f + ( h->desktop ? REM : 1.5f * REM );
}

void Offline_Layout( offlineLayout_t *l, float width, float height ) {
	offlineHeader_t head;
	float y, column1, column2, pad, top, bottom, maxScroll, detailsTop;
	char shown[96];
	int i;

	top = Offline_Header( &head, width, height, l->hosting, l->hosting ? "Back to server browser" : "Back to main menu" );
	l->desktop = head.desktop;
	l->pageWidth = head.pageWidth;
	l->padX = head.padX;
	l->padY = head.padY;
	l->contentX = head.contentX;
	l->contentW = head.contentW;
	l->labelY = head.labelY;
	l->h1Y = head.h1Y;
	l->h1Size = head.h1Size;
	l->back = head.back;
	l->stepY = head.stepY;
	l->stepRulesX = head.stepRulesX;

	/* .offline-track-layout: minmax(0, 1fr) minmax(0, 1.4fr), gap 1rem; one column up to 48rem */
	if ( width > 48.0f * REM ) {
		column1 = ( l->contentW - REM ) / 2.4f;
		column2 = l->contentW - REM - column1;
	} else {
		column1 = column2 = l->contentW;
	}
	pad = l->desktop ? 1.25f * REM : WS_Clamp( REM, 0.03f * width, 2.0f * REM );
	l->left.x = l->contentX;
	l->left.w = column1;
	l->right.x = width > 48.0f * REM ? l->contentX + column1 + REM : l->contentX;
	l->right.w = column2;
	l->leftX = l->left.x + 1.0f + pad;
	l->leftW = column1 - 2.0f - 2.0f * pad;
	l->rightX = l->right.x + 1.0f + pad;
	l->rightW = column2 - 2.0f - 2.0f * pad;

	/* left: "Game mode" (a grid label, margin-bottom 1rem), the select, "Find a
	 * track", the search, the count, the list */
	y = 1.0f + pad;
	l->modeLabelY = y;
	y += WS_BODY_LINE + REM;
	l->mode.x = l->leftX; l->mode.y = y; l->mode.w = l->leftW; l->mode.h = 2.0f * LU( 0.65f * REM ) + 2.0f + 21.0f;
	y += l->mode.h;
	l->searchLabelY = y;
	y += WS_BODY_LINE + REM;
	l->search.x = l->leftX; l->search.y = y; l->search.w = l->leftW; l->search.h = 2.0f * LU( 0.65f * REM ) + 2.0f + WS_BODY_LINE;
	y += l->search.h + WS_BLOCK_MARGIN;
	l->countY = y;
	y += WS_BODY_LINE + WS_BLOCK_MARGIN;
	l->list.x = l->leftX;
	l->list.y = y;
	l->list.w = l->leftW;
	l->rowWidth = l->leftW;
	Offline_Rows( l );

	/* right: the id label, the name, the image, the details, Race setup */
	l->title.font = WS_CONDENSED;
	l->title.size = l->desktop ? 2.0f * REM : LU( WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM ) );
	l->title.spacing = 0.0f;
	l->title.lineHeight = l->title.size;
	l->title.uppercase = qtrue;
	WS_Transform( &l->title, l->name, shown, sizeof( shown ) );
	Demos_WrapAnywhere( &l->title, shown, l->rightW, &l->nameLines );
	WS_BodyStyle( &l->detailStyle );
	if ( l->desktop ) {
		l->detailStyle.size = 0.9f * REM;
		l->detailStyle.lineHeight = LU( 1.4f * 0.9f * REM );
	} else {
		l->detailStyle.lineHeight = LU( 1.4f * REM );
	}
	l->mapIdY = 1.0f + pad;
	l->nameY = l->mapIdY + LU( WS_LABEL_LINE ) + WS_HEADING_MARGIN;
	y = l->nameY + l->nameLines.count * l->title.size;

	if ( l->desktop ) {
		/* the page fills the viewport: the sections take the rest of it */
		float sectionH = height - l->padY - top, gap;
		l->left.y = l->right.y = top;
		l->left.h = l->right.h = sectionH;
		bottom = sectionH - 1.0f - pad;
		/* the list takes what the left column leaves */
		l->list.h = bottom - l->list.y;
		/* right, from the bottom: Race setup, the details (.5rem .75rem),
		 * the image (flex 1 1 0, min 6rem, margin-block .5rem) */
		l->next.x = l->rightX;
		l->next.w = WS_ButtonWidth( "Race setup" );
		l->next.h = WS_BUTTON_HEIGHT;
		l->next.y = bottom - WS_BUTTON_HEIGHT;
		l->detailsH = Offline_Details( l, l->rightW, LU( 0.3f * REM ), REM );
		l->detailsY = l->next.y - 0.75f * REM - l->detailsH;
		gap = WS_HEADING_MARGIN + 0.5f * REM;	/* h2 margin, image margin */
		l->shot.x = l->rightX;
		l->shot.w = l->rightW;
		if ( l->image ) {
			l->shot.y = y + gap;
			l->shot.h = l->detailsY - 0.5f * REM - 0.5f * REM - l->shot.y;
			if ( l->shot.h < 6.0f * REM ) l->shot.h = 6.0f * REM;
		} else {
			l->shot.y = y + WS_HEADING_MARGIN;
			l->shot.h = 0.0f;
		}
		l->pageHeight = height;
	} else {
		/* in flow: the list up to 33rem (18rem below 48rem), the image
		 * clamp(9rem, 24vh, 16rem) with .75rem margins collapsing with the h2's */
		float max = width > 48.0f * REM ? 33.0f * REM : 18.0f * REM, leftH, rightH, margin;
		l->list.h = l->contentHeight < max ? l->contentHeight : max;
		leftH = l->list.y + l->list.h + pad + 1.0f;
		margin = WS_HEADING_MARGIN;
		if ( l->image ) {
			l->shot.x = l->rightX;
			l->shot.w = l->rightW;
			l->shot.y = y + 0.75f * REM;
			l->shot.h = LU( WS_Clamp( 9.0f * REM, 0.24f * height, 16.0f * REM ) );
			y = l->shot.y + l->shot.h;
			margin = 0.75f * REM;
		}
		detailsTop = y + ( margin > 0.5f * REM ? margin : 0.5f * REM );
		l->detailsY = detailsTop;
		l->detailsH = Offline_Details( l, l->rightW, LU( 0.4f * REM ), 1.25f * REM );
		y = detailsTop + l->detailsH + REM;
		l->next.x = l->rightX;
		l->next.y = y;
		l->next.w = WS_ButtonWidth( "Race setup" );
		l->next.h = WS_BUTTON_HEIGHT;
		rightH = y + WS_BUTTON_HEIGHT + pad + 1.0f;
		l->left.y = top;
		if ( width > 48.0f * REM ) {
			if ( leftH < rightH ) leftH = rightH;
			rightH = leftH;
			l->right.y = top;
		} else {
			l->right.y = top + leftH + REM;
		}
		l->left.h = leftH;
		l->right.h = rightH;
		/* the empty status line's margins, then the page's padding */
		l->pageHeight = l->right.y + l->right.h + WS_BLOCK_MARGIN + l->padY;
	}

	/* the list's scrollbar, as CEF shows it */
	l->scrollbar = l->contentHeight > l->list.h + 0.5f;
	if ( l->scrollbar && !l->hideScrollbar ) {
		l->rowWidth = l->leftW - RIVALS_SCROLLBAR;
		Offline_Rows( l );
	}
	maxScroll = l->contentHeight - l->list.h;
	l->scroll = l->scrollbar ? WS_Clamp( 0.0f, floor( l->scroll + 0.5f ), floor( maxScroll + 0.5f ) ) : 0.0f;

	/* section-relative to the screen */
	l->modeLabelY += l->left.y;
	l->mode.y += l->left.y;
	l->searchLabelY += l->left.y;
	l->search.y += l->left.y;
	l->countY += l->left.y;
	l->list.y += l->left.y;
	l->mapIdY += l->right.y;
	l->nameY += l->right.y;
	l->shot.y += l->right.y;
	l->detailsY += l->right.y;
	l->next.y += l->right.y;
	for ( i = 0; i < l->details; i++ ) {
		l->detailX[i] += l->rightX;
		l->detailY[i] += l->detailsY;
	}
	/* the page scrolls as a whole when it is taller than the viewport */
	l->pageScrollbar = l->pageHeight > height + 0.5f;
	l->pageScroll = l->pageScrollbar ? WS_Clamp( 0.0f, floor( l->pageScroll + 0.5f ), floor( l->pageHeight - height + 0.5f ) ) : 0.0f;
	l->page.x = 0.0f; l->page.y = 0.0f; l->page.w = width; l->page.h = height;
	memset( &l->pageThumb, 0, sizeof( l->pageThumb ) );
	if ( l->pageScrollbar ) {
		float dy = -l->pageScroll;
		WS_ScrollThumb( &l->page, l->pageHeight, l->pageScroll, &l->pageThumb );
		l->labelY += dy; l->h1Y += dy; l->back.y += dy; l->stepY += dy;
		l->left.y += dy; l->right.y += dy;
		l->modeLabelY += dy; l->mode.y += dy; l->searchLabelY += dy; l->search.y += dy;
		l->countY += dy; l->list.y += dy;
		l->mapIdY += dy; l->nameY += dy; l->shot.y += dy; l->detailsY += dy; l->next.y += dy;
		for ( i = 0; i < l->details; i++ ) l->detailY[i] += dy;
	}
	memset( &l->thumb, 0, sizeof( l->thumb ) );
	if ( l->scrollbar ) WS_ScrollThumb( &l->list, l->contentHeight, l->scroll, &l->thumb );
}
