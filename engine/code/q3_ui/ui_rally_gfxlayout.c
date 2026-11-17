/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS of the WebUI's loading screen, rule for rule (see the header).
Lengths in px; vw/vh are the viewport including the scrollbar, the boxes
are laid out in the viewport minus the reserved scrollbar gutter.
*/

#include "ui_local.h"
#include "ui_rally_gfxlayout.h"
#include "ui_rally_frontpage.h"

const char *gfxStageLabels[GFX_LAYOUT_STAGES + 1] = {
	"Starting up", "Loading your car", "Preparing the setup menu", "Opening the garage",
	"Driver settings", "Reading your controls", "Server browser", "Loading the track list",
	"Ready to race"
};
const char *gfxPromptText = "A newer version of Q3Rally is available.";
const char *gfxLoadingLabel = "01 / PREPARING YOUR RALLY";
const char *gfxButtonLabels[2] = { "Open download page", "Continue" };

#define REM				WS_REM
#define LABEL_LINE		WS_LABEL_LINE
#define HEADING_MARGIN	WS_HEADING_MARGIN
#define BODY_LINE		WS_BODY_LINE
#define BLOCK_MARGIN	WS_BLOCK_MARGIN

void GFX_Layout( gfxLayout_t *l, float width, float height ) {
	wsFrontPage_t page;
	wsTextStyle_t label, body;
	float leftH, rightH, top, line, y, space;
	int i;

	WS_LabelStyle( &label );
	WS_BodyStyle( &body );
	WS_FrontPage( &page, width, height, "Getting ", "ready.", "Loading graphics" );
	l->page = page;
	l->pageWidth = page.pageWidth;
	l->headerX = page.headerX;
	l->headerY = page.headerY;
	l->h1Y = page.h1Y;
	l->h1Size = page.h1Size;
	l->statusX = page.statusX;
	l->statusY = page.statusY;

	l->left.x = page.contentX;
	l->left.w = page.column1;
	l->leftX = l->left.x + 1.0f + page.sectionPad;
	l->leftW = page.column1 - 2.0f - 2.0f * page.sectionPad;

	/* left section, from its content top */
	top = 1.0f + page.sectionPad;
	l->loadingLabelY = top;
	WS_Wrap( &label, gfxLoadingLabel, l->leftW, &l->loadingLabelLines );
	l->titleStyle.font = WS_CONDENSED;
	l->titleStyle.size = WS_Clamp( 1.5f * REM, 0.025f * width, 2.0f * REM );
	l->titleStyle.spacing = 0.0f;
	l->titleStyle.lineHeight = 1.1f * l->titleStyle.size;
	l->titleStyle.uppercase = qtrue;
	WS_StyledWrap( &l->titleStyle, l->title, l->leftW, &l->titleLines );
	l->titleY = top + l->loadingLabelLines.count * LABEL_LINE + HEADING_MARGIN;
	line = l->titleLines.count * l->titleStyle.lineHeight;
	if ( line < 3.3f * l->titleStyle.size ) line = 3.3f * l->titleStyle.size;	/* min-height: 3.3em */
	y = l->titleY + line + BLOCK_MARGIN;	/* h2 and ol margins collapse to 1em */

	/* ol: padding-left 1.6rem, li padding .3rem 0 */
	l->stageX = l->leftX + 1.6f * REM;
	for ( i = 0; i < GFX_LAYOUT_STAGES; i++ ) {
		wsLines_t lines;
		l->stageY[i] = y;
		WS_Wrap( &body, gfxStageLabels[i], l->leftW - 1.6f * REM, &lines );
		y += 2.0f * 0.3f * REM + ( lines.count > 0 ? lines.count : 1 ) * BODY_LINE;
	}
	y += BLOCK_MARGIN;

	/* <progress> sits on the baseline of its line, vertical-align -0.2em */
	l->progressX = l->leftX;
	l->progressW = l->leftW;
	l->progressY = y + WS_Baseline( &body ) - ( REM - 0.2f * REM );
	y += BODY_LINE + BLOCK_MARGIN;
	l->percentY = y;
	y += BODY_LINE + BLOCK_MARGIN;

	memset( &l->promptLines, 0, sizeof( l->promptLines ) );
	memset( l->buttons, 0, sizeof( l->buttons ) );
	if ( l->prompt ) {
		l->promptY = y;
		WS_Wrap( &body, gfxPromptText, l->leftW, &l->promptLines );
		y += l->promptLines.count * BODY_LINE + BLOCK_MARGIN;
		/* inline-block buttons separated by a space, wrapping like words */
		space = WS_StyledWidth( &body, " " );
		for ( i = 0; i < 2; i++ ) {
			l->buttons[i].w = WS_ButtonWidth( gfxButtonLabels[i] );
			l->buttons[i].h = WS_BUTTON_HEIGHT;
		}
		l->buttons[0].x = l->leftX;
		l->buttons[0].y = y;
		if ( l->buttons[0].w + space + l->buttons[1].w <= l->leftW + 0.01f ) {
			l->buttons[1].x = l->leftX + l->buttons[0].w + space;
			l->buttons[1].y = y;
			y += WS_BUTTON_HEIGHT;
		} else {
			l->buttons[1].x = l->leftX;
			l->buttons[1].y = y + WS_BUTTON_HEIGHT;
			y += 2.0f * WS_BUTTON_HEIGHT;
		}
	}
	/* without the prompt the percentage's bottom margin ends the content,
	 * with it the button line does */
	leftH = y + page.sectionPad + 1.0f;

	rightH = WS_ShowroomLayout( &l->room, &page, l->vehicle, l->status, qfalse );

	/* align-items: stretch, one row; or stacked in one column */
	l->left.y = page.gridY;
	if ( !page.oneColumn ) {
		if ( leftH < rightH ) leftH = rightH;
		rightH = leftH;
		WS_PlaceShowroom( &l->room, page.gridY, rightH );
	} else {
		WS_PlaceShowroom( &l->room, page.gridY + leftH + 1.5f * REM, rightH );
	}
	l->left.h = leftH;

	/* section-relative positions to the screen */
	l->loadingLabelY += l->left.y;
	l->titleY += l->left.y;
	for ( i = 0; i < GFX_LAYOUT_STAGES; i++ ) l->stageY[i] += l->left.y;
	l->progressY += l->left.y;
	l->percentY += l->left.y;
	l->promptY += l->left.y;
	l->buttons[0].y += l->left.y;
	l->buttons[1].y += l->left.y;

	l->pageHeight = l->room.box.y + l->room.box.h + page.pad;
	if ( l->left.y + l->left.h + page.pad > l->pageHeight ) l->pageHeight = l->left.y + l->left.h + page.pad;
}
