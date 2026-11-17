/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS of the WebUI's main menu, rule for rule (see the header).
*/

#include "ui_local.h"
#include "ui_rally_mainlayout.h"

const char *mainNavLabels[MAIN_NAV_COUNT] = {
	"Offline", "Online", "Garage", "Config", "Rivals", "Demos", "Downloads", "Driver profile", "Quit"
};
const char *mainQuitLabel = "Q3RALLY / FINISH YOUR SESSION";
const char *mainQuitTitle = "Exit game?";
const char *mainQuitText = "Thanks for racing with us.";
const char *mainQuitAccept = "Show credits and exit";
const char *mainQuitCancel = "Keep racing";

#define REM				WS_REM
#define NAV_GAP			( 0.5f * REM )		/* #main-navigation { gap: .5rem } */
#define NAV_SIZE		( 1.3f * REM )		/* font-size 1.3rem, letter-spacing .06em */
#define NAV_LINE		( 1.5f * NAV_SIZE )	/* line-height stays the body's 1.5 */
#define SUMMARY_SIZE	( 0.85f * REM )		/* #profile-summary: .85rem/1.4, margin-top .3rem */
#define DIALOG_PAD		( 2.0f * REM )

/* Chrome keeps block lengths in 1/64 px (LayoutUnit), cut off, not rounded;
 * nine stacked buttons would otherwise drift by a quarter pixel. */
static float LU( float value ) {
	return floor( value * 64.0f ) / 64.0f;
}

/* #quit-confirm: width fit-content up to min(32rem, 90vw), centered */
static void MainMenu_DialogLayout( mainLayout_t *l, float width, float height ) {
	wsTextStyle_t label, body;
	float content, longest, space, y;

	WS_LabelStyle( &label );
	WS_BodyStyle( &body );
	l->dialogTitleStyle.font = WS_CONDENSED;
	l->dialogTitleStyle.size = WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM );
	l->dialogTitleStyle.spacing = 0.0f;
	l->dialogTitleStyle.lineHeight = l->dialogTitleStyle.size;
	l->dialogTitleStyle.uppercase = qtrue;

	/* max-content of the children: the label, the heading, the text and the
	 * line of buttons */
	space = WS_StyledWidth( &body, " " );
	l->accept.w = WS_ButtonWidth( mainQuitAccept );
	l->cancel.w = WS_ButtonWidth( mainQuitCancel );
	l->accept.h = l->cancel.h = WS_BUTTON_HEIGHT;
	longest = l->accept.w + space + l->cancel.w;
	if ( WS_StyledWidth( &label, mainQuitLabel ) > longest ) longest = WS_StyledWidth( &label, mainQuitLabel );
	if ( WS_StyledWidth( &l->dialogTitleStyle, mainQuitTitle ) > longest ) longest = WS_StyledWidth( &l->dialogTitleStyle, mainQuitTitle );
	if ( WS_StyledWidth( &body, mainQuitText ) > longest ) longest = WS_StyledWidth( &body, mainQuitText );
	content = ( 32.0f * REM < 0.9f * width ? 32.0f * REM : 0.9f * width ) - 2.0f * DIALOG_PAD - 2.0f;
	l->dialogW = longest < content ? longest : content;

	y = 1.0f + DIALOG_PAD;
	l->dialogLabelY = y;
	WS_Wrap( &label, mainQuitLabel, l->dialogW, &l->dialogLabelLines );
	y += l->dialogLabelLines.count * WS_LABEL_LINE + WS_HEADING_MARGIN;
	l->dialogTitleY = y;
	WS_StyledWrap( &l->dialogTitleStyle, mainQuitTitle, l->dialogW, &l->dialogTitleLines );
	y += l->dialogTitleLines.count * l->dialogTitleStyle.lineHeight + WS_BLOCK_MARGIN;	/* h2 and p margins collapse */
	l->dialogTextY = y;
	WS_Wrap( &body, mainQuitText, l->dialogW, &l->dialogTextLines );
	y += l->dialogTextLines.count * WS_BODY_LINE + WS_BLOCK_MARGIN;
	l->accept.y = y;
	if ( longest <= l->dialogW + 0.01f ) {
		l->cancel.y = y;
		l->cancel.x = l->accept.w + space;
		y += WS_BUTTON_HEIGHT;
	} else {
		l->cancel.y = y + WS_BUTTON_HEIGHT;
		l->cancel.x = 0.0f;
		y += 2.0f * WS_BUTTON_HEIGHT;
	}
	l->dialog.w = l->dialogW + 2.0f * DIALOG_PAD + 2.0f;
	l->dialog.h = y + DIALOG_PAD + 1.0f;
	/* position: fixed, inset 0, margin auto in the viewport the scrollbar leaves */
	l->dialog.x = ( l->pageWidth - l->dialog.w ) * 0.5f;
	l->dialog.y = ( height - l->dialog.h ) * 0.5f;
	l->dialogX = l->dialog.x + 1.0f + DIALOG_PAD;
	l->dialogLabelY += l->dialog.y;
	l->dialogTitleY += l->dialog.y;
	l->dialogTextY += l->dialog.y;
	l->accept.x = l->dialogX;
	l->cancel.x += l->dialogX;
	l->accept.y += l->dialog.y;
	l->cancel.y += l->dialog.y;
}

void MainMenu_Layout( mainLayout_t *l, float width, float height ) {
	float navH, roomH, y, rowH, navY;
	int i;

	WS_FrontPage( &l->page, width, height, "Ready to ", "race.", "Main menu" );
	l->pageWidth = l->page.pageWidth;

	/* #main-navigation: buttons stretched to the column, the condensed face */
	l->navStyle.font = WS_CONDENSED;
	l->navStyle.size = NAV_SIZE;
	l->navStyle.spacing = 0.06f * NAV_SIZE;
	l->navStyle.lineHeight = NAV_LINE;
	l->navStyle.uppercase = qtrue;
	l->summaryStyle.font = WS_BODY;
	l->summaryStyle.size = SUMMARY_SIZE;
	l->summaryStyle.spacing = 0.0f;
	l->summaryStyle.lineHeight = 1.4f * SUMMARY_SIZE;
	l->summaryStyle.uppercase = qfalse;
	l->navTextX = 1.0f + LU( WS_BUTTON_PAD_X );
	l->navTextY = 1.0f + LU( WS_BUTTON_PAD_Y );
	WS_Wrap( &l->summaryStyle, l->summary, l->page.column1 - 2.0f - 2.0f * WS_BUTTON_PAD_X, &l->summaryLines );

	y = 0.0f;
	for ( i = 0; i < MAIN_NAV_COUNT; i++ ) {
		l->nav[i].x = l->page.contentX;
		l->nav[i].w = l->page.column1;
		l->nav[i].y = y;
		l->nav[i].h = 2.0f * LU( WS_BUTTON_PAD_Y ) + 2.0f + LU( NAV_LINE );
		if ( i == MAIN_NAV_PROFILE ) {
			l->summaryY = y + l->navTextY + LU( NAV_LINE ) + LU( 0.3f * REM );
			l->nav[i].h += LU( 0.3f * REM ) + ( l->summaryLines.count > 0 ? l->summaryLines.count : 1 ) * LU( l->summaryStyle.lineHeight );
		}
		y += l->nav[i].h + ( i + 1 < MAIN_NAV_COUNT ? NAV_GAP : 0.0f );
	}
	navH = y;

	roomH = WS_ShowroomLayout( &l->room, &l->page, l->vehicle, l->status, qtrue );

	/* one row as high as the taller column; or stacked in one column */
	navY = l->page.gridY;
	if ( !l->page.oneColumn ) {
		rowH = navH > roomH ? navH : roomH;
		WS_PlaceShowroom( &l->room, l->page.gridY, rowH );
		l->pageHeight = l->page.gridY + rowH + l->page.pad;
	} else {
		WS_PlaceShowroom( &l->room, l->page.gridY + navH + 1.5f * REM, roomH );
		l->pageHeight = l->room.box.y + roomH + l->page.pad;
	}
	for ( i = 0; i < MAIN_NAV_COUNT; i++ ) l->nav[i].y += navY;
	l->summaryY += navY;

	MainMenu_DialogLayout( l, width, height );
}
