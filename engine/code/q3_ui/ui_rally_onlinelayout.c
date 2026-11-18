/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS of the WebUI's server browser, rule for rule (see the header).
*/

#include "ui_local.h"
#include "ui_rally_onlinelayout.h"
#include "ui_rally_demoslayout.h"
#include "ui_rally_rivalslayout.h"

#define REM		WS_REM
#define LU		WS_LU

const char *onlineFilterText[ONLINE_FILTERS] = { "Server source", "Game mode", "Sort by", "Search servers" };
const char *onlineToggleText[ONLINE_TOGGLES] = { "Show full", "Show empty", "Count humans only" };
const char *onlineActionText[ONLINE_ACTIONS] = { "Refresh servers", "Stop search", "Direct connect", "Host a server" };
const char *onlineDetailText[ONLINE_DETAILS] = { "Game mode", "Map", "Players", "Human players", "Ping" };

#define ONLINE_GAP			LU( 0.65f * REM )		/* .online-actions: gap .65rem */
#define ONLINE_SELECT		( 2.0f * LU( 0.65f * REM ) + 2.0f + 21.0f )
#define ONLINE_INPUT		( 2.0f * LU( 0.65f * REM ) + 2.0f + WS_BODY_LINE )

/* flex-wrap: lays out boxes of the given widths from x, rows of height h;
 * returns the height used */
static float Online_Flow( wsRect_t *box, int count, float x, float y, float width, float h, float gap ) {
	float cx = x, cy = y;
	int i;
	for ( i = 0; i < count; i++ ) {
		if ( box[i].w <= 0.0f ) continue;
		if ( cx > x && cx + box[i].w > x + width + 0.01f ) {
			cx = x;
			cy += h + gap;
		}
		box[i].x = cx;
		box[i].y = cy;
		box[i].h = h;
		cx += box[i].w + gap;
	}
	return cy + h - y;
}

/* one server button: padding .85rem, the name and its line, players, ping */
static void Online_Rows( onlineLayout_t *l ) {
	wsTextStyle_t strong;
	float y = 0.0f, pad = LU( 0.85f * REM );
	int i;

	WS_BodyStyle( &strong );
	strong.font = WS_BODY_BOLD;
	l->nameW = l->rowWidth - 2.0f - 2.0f * pad - 2.0f * l->playersW - REM;
	for ( i = 0; i < l->rows; i++ ) {
		float h;
		Demos_WrapAnywhere( &strong, l->rowName[i] ? l->rowName[i] : "", l->nameW, &l->nameLines[i] );
		Demos_WrapAnywhere( &l->subStyle, l->rowSub[i] ? l->rowSub[i] : "", l->nameW, &l->subLines[i] );
		h = l->nameLines[i].count * WS_BODY_LINE + l->subLines[i].count * l->subStyle.lineHeight;
		if ( h < WS_BODY_LINE ) h = WS_BODY_LINE;
		l->rowY[i] = y;
		l->rowH[i] = 2.0f + 2.0f * pad + h;
		y += l->rowH[i] + 0.5f * REM;
	}
	if ( l->rows ) l->listHeight = y - 0.5f * REM;
	else {
		wsTextStyle_t body;
		WS_BodyStyle( &body );
		WS_Wrap( &body, l->empty ? l->empty : "", l->rowWidth, &l->emptyLines );
		l->emptyY = WS_BLOCK_MARGIN;
		l->listHeight = 2.0f * WS_BLOCK_MARGIN + l->emptyLines.count * WS_BODY_LINE;
	}
}

static void Online_Dialog( onlineLayout_t *l, float width, float height ) {
	wsTextStyle_t body;
	float y;

	WS_BodyStyle( &body );
	l->dialogBox.w = width - 2.0f * REM < 40.0f * REM ? width - 2.0f * REM : 40.0f * REM;
	/* fixed, centred in the viewport without the scrollbar gutter */
	l->dialogBox.x = ( l->page.pageWidth - l->dialogBox.w ) * 0.5f;
	l->dialogX = l->dialogBox.x + 1.0f + 2.0f * REM;
	l->dialogW = l->dialogBox.w - 2.0f - 4.0f * REM;
	y = 3.0f + 2.0f * REM;
	if ( l->dialog == ONLINE_DIRECT ) {
		l->dialogLabelY = y;
		l->dialogH2Y = y + LU( WS_LABEL_LINE ) + WS_HEADING_MARGIN;
		WS_StyledWrap( &l->h2, "Know the address?", l->dialogW, &l->dialogH2Lines );
		y = l->dialogH2Y + l->dialogH2Lines.count * l->h2.size + 1.5f * REM;
		l->dialogFieldLabelY = y;
		l->dialogInput.x = l->dialogX;
		l->dialogInput.w = l->dialogW;
		l->dialogInput.y = y + WS_BODY_LINE + 0.5f * REM;
		l->dialogInput.h = ONLINE_INPUT;
		y = l->dialogInput.y + l->dialogInput.h + WS_BLOCK_MARGIN;
		WS_Wrap( &body, "Hostname, IPv4 or IPv6. The default port is 27960.", l->dialogW, &l->dialogHelpLines );
		l->dialogHelpY = y;
		y += l->dialogHelpLines.count * WS_BODY_LINE + WS_BLOCK_MARGIN;
		WS_Wrap( &body, l->dialogText ? l->dialogText : "", l->dialogW, &l->dialogErrorLines );
		l->dialogErrorY = y;
		if ( l->dialogErrorLines.count ) y += l->dialogErrorLines.count * WS_BODY_LINE + WS_BLOCK_MARGIN;
		l->dialogButton[0].w = WS_ButtonWidth( "Connect" );
		l->dialogButton[1].w = WS_ButtonWidth( "Cancel" );
	} else {
		l->dialogH2Y = y + WS_HEADING_MARGIN;
		WS_StyledWrap( &l->h2, "Remove favorite?", l->dialogW, &l->dialogH2Lines );
		y = l->dialogH2Y + l->dialogH2Lines.count * l->h2.size + 1.5f * REM;
		WS_Wrap( &body, l->dialogText ? l->dialogText : "", l->dialogW, &l->dialogAddressLines );
		l->dialogAddressY = y;
		y += l->dialogAddressLines.count * WS_BODY_LINE + WS_BLOCK_MARGIN;
		l->dialogButton[0].w = WS_ButtonWidth( "Remove favorite" );
		l->dialogButton[1].w = WS_ButtonWidth( "Keep favorite" );
	}
	y += Online_Flow( l->dialogButton, 2, l->dialogX, y, l->dialogW, WS_BUTTON_HEIGHT, ONLINE_GAP );
	l->dialogBox.h = y + 2.0f * REM + 1.0f;
	{
		/* centred in the viewport */
		float top = ( height - l->dialogBox.h ) * 0.5f, dy;
		if ( top < 0.0f ) top = 0.0f;
		dy = top;
		l->dialogBox.y = top;
		l->dialogLabelY += dy; l->dialogH2Y += dy; l->dialogFieldLabelY += dy; l->dialogInput.y += dy;
		l->dialogHelpY += dy; l->dialogErrorY += dy; l->dialogAddressY += dy;
		l->dialogButton[0].y += dy; l->dialogButton[1].y += dy;
	}
}

void Online_Layout( onlineLayout_t *l, float width, float height ) {
	wsTextStyle_t body, label;
	float top, pad, y, column, rowH, column1, column2, listBottom, detailBottom;
	int columns, i, rows;

	WS_BodyStyle( &body );
	WS_LabelStyle( &label );
	WS_FrontPageLabeled( &l->page, width, height, "Q3RALLY / ONLINE", "Join the ", "grid.", "" );
	top = WS_PlaceHeaderButton( &l->page, "Back to main menu", &l->back );
	pad = l->page.sectionPad;
	l->subStyle = body;
	l->subStyle.size = 16.0f / 1.2f;		/* small: font-size smaller */
	l->subStyle.lineHeight = LU( 1.5f * 16.0f / 1.2f );
	l->h2.font = WS_CONDENSED;
	l->h2.size = l->h2.lineHeight = LU( WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM ) );
	l->h2.spacing = 0.0f;
	l->h2.uppercase = qtrue;

	/* the toolbar */
	l->toolbar.x = l->page.contentX;
	l->toolbar.w = l->page.contentW;
	l->toolbar.y = top;
	l->innerX = l->toolbar.x + 1.0f + pad;
	l->innerW = l->toolbar.w - 2.0f - 2.0f * pad;
	y = top + 1.0f + pad;
	/* .online-filters: repeat(auto-fit, minmax(min(100%, 12rem), 1fr)), gap 1rem */
	columns = (int)( ( l->innerW + REM ) / ( 12.0f * REM + REM ) );
	if ( columns < 1 ) columns = 1;
	if ( columns > ONLINE_FILTERS ) columns = ONLINE_FILTERS;
	column = ( l->innerW - ( columns - 1 ) * REM ) / columns;
	rows = ( ONLINE_FILTERS + columns - 1 ) / columns;
	for ( i = 0; i < rows; i++ ) {
		int j;
		float own[ONLINE_FILTERS], text[ONLINE_FILTERS];
		rowH = 0.0f;
		for ( j = i * columns; j < ONLINE_FILTERS && j < ( i + 1 ) * columns; j++ ) {
			WS_Wrap( &body, onlineFilterText[j], column, &l->filterLines[j] );
			text[j] = l->filterLines[j].count * WS_BODY_LINE;
			l->filterControl[j].h = j < 3 ? ONLINE_SELECT : ONLINE_INPUT;
			own[j] = text[j] + 0.5f * REM + l->filterControl[j].h;
			if ( own[j] > rowH ) rowH = own[j];
		}
		for ( j = i * columns; j < ONLINE_FILTERS && j < ( i + 1 ) * columns; j++ ) {
			/* stretched: the extra height goes half to the text, half to the control */
			float extra = rowH - own[j];
			l->filterBox[j].x = l->innerX + ( j - i * columns ) * ( column + REM );
			l->filterBox[j].y = y;
			l->filterBox[j].w = column;
			l->filterBox[j].h = rowH;
			l->filterControl[j].x = l->filterBox[j].x;
			l->filterControl[j].w = column;
			l->filterControl[j].y = y + text[j] + 0.5f * extra + 0.5f * REM;
			l->filterControl[j].h += 0.5f * extra;
		}
		y += rowH + ( i < rows - 1 ? REM : 0.0f );
	}
	/* the toggles: a 16px box with its margins (4px left, 3px right), .5rem, the text */
	y += REM;
	for ( i = 0; i < ONLINE_TOGGLES; i++ ) l->toggle[i].w = 4.0f + 16.0f + 3.0f + 0.5f * REM + WS_TextWidth( &body, onlineToggleText[i], strlen( onlineToggleText[i] ) );
	y += Online_Flow( l->toggle, ONLINE_TOGGLES, l->innerX, y, l->innerW, WS_BODY_LINE, ONLINE_GAP ) + REM;
	for ( i = 0; i < ONLINE_ACTIONS; i++ ) l->action[i].w = WS_ButtonWidth( onlineActionText[i] );
	y += Online_Flow( l->action, ONLINE_ACTIONS, l->innerX, y, l->innerW, WS_BUTTON_HEIGHT, ONLINE_GAP ) + WS_BLOCK_MARGIN;
	l->statusY = y;
	WS_Wrap( &body, l->status ? l->status : "", l->innerW, &l->statusLines );
	y += l->statusLines.count * WS_BODY_LINE + WS_BLOCK_MARGIN;
	l->toolbar.h = y + pad + 1.0f - top;
	top = l->toolbar.y + l->toolbar.h + REM;

	/* .online-layout: minmax(0, 1.65fr) minmax(0, 1fr), gap 1rem; one column up to 48rem */
	if ( width > 48.0f * REM ) {
		column1 = ( l->page.contentW - REM ) * 1.65f / 2.65f;
		column2 = l->page.contentW - REM - column1;
		l->playersW = 5.0f * REM;
	} else {
		column1 = column2 = l->page.contentW;
		l->playersW = 0.0f;
	}
	l->listSection.x = l->page.contentX;
	l->listSection.w = column1;
	l->listSection.y = top;
	l->detailSection.x = width > 48.0f * REM ? l->page.contentX + column1 + REM : l->page.contentX;
	l->detailSection.w = column2;
	l->listX = l->listSection.x + 1.0f + pad;
	l->listW = column1 - 2.0f - 2.0f * pad;
	l->detailX = l->detailSection.x + 1.0f + pad;
	l->detailW = column2 - 2.0f - 2.0f * pad;

	/* the list: heading (minmax(0, 1fr) 5rem 5rem, 3.5rem 4rem below 48rem), count, list */
	{
		float players = width > 48.0f * REM ? 5.0f * REM : 3.5f * REM, ping = width > 48.0f * REM ? 5.0f * REM : 4.0f * REM;
		l->headingY = top + 1.0f + pad;
		l->headingPingX = l->listX + l->listW - ping;
		l->headingPlayersX = l->headingPingX - 0.5f * REM - players;
		l->countY = l->headingY + 14.0f + WS_BLOCK_MARGIN;
		l->list.x = l->listX;
		l->list.w = l->listW;
		l->list.y = l->countY + WS_BODY_LINE + WS_BLOCK_MARGIN;
		l->playersW = 0.5f * ( players + ping );	/* both columns, for the name's width */
		l->rowWidth = l->listW;
		Online_Rows( l );
		l->list.h = l->listHeight < 38.0f * REM ? l->listHeight : 38.0f * REM;
		if ( l->listHeight > l->list.h + 0.5f && !l->hideScrollbar ) {
			l->rowWidth = l->listW - RIVALS_SCROLLBAR;
			Online_Rows( l );
		}
		listBottom = l->list.y + l->list.h + pad + 1.0f;
	}

	/* the destination: label, h2, address, image, details, buttons, message */
	{
		float detailTop = top;
		l->labelY = detailTop + 1.0f + pad;
		l->nameY = l->labelY + LU( WS_LABEL_LINE ) + WS_HEADING_MARGIN;
		WS_StyledWrap( &l->h2, l->name ? l->name : "", l->detailW, &l->nameLinesDetail );
		l->addressY = l->nameY + l->nameLinesDetail.count * l->h2.size + WS_BLOCK_MARGIN;
		Demos_WrapAnywhere( &body, l->address ? l->address : "", l->detailW, &l->addressLines );
		y = l->addressY + l->addressLines.count * WS_BODY_LINE + WS_BLOCK_MARGIN;
		l->shot.x = l->detailX;
		l->shot.w = l->detailW;
		l->shot.y = y;
		l->shot.h = 0.0f;
		if ( l->image ) {
			l->shot.h = LU( WS_Clamp( 10.0f * REM, 0.2f * width, 20.0f * REM ) );
			y += l->shot.h + REM;
		}
		/* #online-details: repeat(auto-fit, minmax(7rem, 1fr)), gap .75rem */
		l->detailsY = y;
		l->detailsH = 0.0f;
		if ( l->picked ) {
			float cardW, rowTop = y;
			int cols = (int)( ( l->detailW + 0.75f * REM ) / ( 7.0f * REM + 0.75f * REM ) );
			if ( cols < 1 ) cols = 1;
			if ( cols > ONLINE_DETAILS ) cols = ONLINE_DETAILS;
			cardW = ( l->detailW - ( cols - 1 ) * 0.75f * REM ) / cols;
			for ( i = 0; i < ONLINE_DETAILS; i += cols ) {
				int j;
				rowH = 0.0f;
				for ( j = i; j < i + cols && j < ONLINE_DETAILS; j++ ) {
					float h;
					WS_StyledWrap( &label, onlineDetailText[j], cardW, &l->cardLabelLines[j] );
					WS_Wrap( &body, l->detail[j] ? l->detail[j] : "", cardW, &l->cardValueLines[j] );
					/* p { margin: .25rem 0 }: the empty value's margins collapse */
					h = 0.25f * REM + l->cardLabelLines[j].count * LU( WS_LABEL_LINE ) + 0.25f * REM +
					    ( l->cardValueLines[j].count ? l->cardValueLines[j].count * WS_BODY_LINE + 0.25f * REM : 0.0f );
					if ( h > rowH ) rowH = h;
				}
				for ( j = i; j < i + cols && j < ONLINE_DETAILS; j++ ) {
					l->card[j].x = l->detailX + ( j - i ) * ( cardW + 0.75f * REM );
					l->card[j].y = rowTop;
					l->card[j].w = cardW;
					l->card[j].h = rowH;
					l->cardValueY[j] = rowTop + 0.25f * REM + l->cardLabelLines[j].count * LU( WS_LABEL_LINE ) + 0.25f * REM;
				}
				rowTop += rowH + 0.75f * REM;
			}
			l->detailsH = rowTop - 0.75f * REM - y;
		}
		y = l->detailsY + l->detailsH + REM;
		l->button[0].w = WS_ButtonWidth( "Connect" );
		l->button[1].w = WS_ButtonWidth( l->favorite ? l->favorite : "Save favorite" );
		l->button[2].w = l->remove ? WS_ButtonWidth( "Remove favorite" ) : 0.0f;
		y += Online_Flow( l->button, ONLINE_BUTTONS, l->detailX, y, l->detailW, WS_BUTTON_HEIGHT, ONLINE_GAP ) + WS_BLOCK_MARGIN;
		l->messageY = y;
		WS_Wrap( &body, l->message ? l->message : "", l->detailW, &l->messageLines );
		if ( l->messageLines.count ) y += l->messageLines.count * WS_BODY_LINE + WS_BLOCK_MARGIN;
		detailBottom = y + pad + 1.0f;
	}

	if ( width > 48.0f * REM ) {
		float h = ( listBottom > detailBottom ? listBottom : detailBottom ) - top;
		l->listSection.h = l->detailSection.h = h;
		l->detailSection.y = top;
		l->pageHeight = top + h + l->page.pad;
	} else {
		/* one column: the destination below the list */
		float dy = listBottom + REM - top;
		l->listSection.h = listBottom - top;
		l->detailSection.y = top + dy;
		l->detailSection.h = detailBottom - top;
		l->labelY += dy; l->nameY += dy; l->addressY += dy; l->shot.y += dy; l->detailsY += dy; l->messageY += dy;
		for ( i = 0; i < ONLINE_DETAILS; i++ ) { l->card[i].y += dy; l->cardValueY[i] += dy; }
		for ( i = 0; i < ONLINE_BUTTONS; i++ ) l->button[i].y += dy;
		l->pageHeight = l->detailSection.y + l->detailSection.h + l->page.pad;
	}

	/* the list's own scroll, then the page's */
	l->listBar = l->listHeight > l->list.h + 0.5f;
	l->listScroll = l->listBar ? WS_Clamp( 0.0f, floor( l->listScroll + 0.5f ), floor( l->listHeight - l->list.h + 0.5f ) ) : 0.0f;
	l->viewport.x = 0.0f; l->viewport.y = 0.0f; l->viewport.w = width; l->viewport.h = height;
	l->pageBar = l->pageHeight > height + 0.5f;
	l->pageScroll = l->pageBar ? WS_Clamp( 0.0f, floor( l->pageScroll + 0.5f ), floor( l->pageHeight - height + 0.5f ) ) : 0.0f;
	memset( &l->pageThumb, 0, sizeof( l->pageThumb ) );
	if ( l->pageBar ) {
		float dy = -l->pageScroll;
		WS_ScrollThumb( &l->viewport, l->pageHeight, l->pageScroll, &l->pageThumb );
		l->page.headerY += dy; l->page.h1Y += dy; l->back.y += dy;
		l->toolbar.y += dy;
		for ( i = 0; i < ONLINE_FILTERS; i++ ) { l->filterBox[i].y += dy; l->filterControl[i].y += dy; }
		for ( i = 0; i < ONLINE_TOGGLES; i++ ) l->toggle[i].y += dy;
		for ( i = 0; i < ONLINE_ACTIONS; i++ ) l->action[i].y += dy;
		l->statusY += dy;
		l->listSection.y += dy; l->detailSection.y += dy;
		l->headingY += dy; l->countY += dy; l->list.y += dy;
		l->labelY += dy; l->nameY += dy; l->addressY += dy; l->shot.y += dy; l->detailsY += dy; l->messageY += dy;
		for ( i = 0; i < ONLINE_DETAILS; i++ ) { l->card[i].y += dy; l->cardValueY[i] += dy; }
		for ( i = 0; i < ONLINE_BUTTONS; i++ ) l->button[i].y += dy;
	}
	memset( &l->listThumb, 0, sizeof( l->listThumb ) );
	if ( l->listBar ) WS_ScrollThumb( &l->list, l->listHeight, l->listScroll, &l->listThumb );
	if ( l->dialog != ONLINE_NO_DIALOG ) Online_Dialog( l, width, height );
}
