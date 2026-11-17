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

const char *gfxStageLabels[GFX_LAYOUT_STAGES + 1] = {
	"Starting up", "Loading your car", "Preparing the setup menu", "Opening the garage",
	"Driver settings", "Reading your controls", "Server browser", "Loading the track list",
	"Ready to race"
};
const char *gfxPromptText = "A newer version of Q3Rally is available.";
const char *gfxLoadingLabel = "01 / PREPARING YOUR RALLY";
const char *gfxShowroomLabel = "YOUR ACTIVE VEHICLE";
const char *gfxButtonLabels[2] = { "Open download page", "Continue" };

static const char *gfxHeaderLabel = "Q3RALLY / IT'S DAMN FAST, BABY.";
static const char *gfxHeaderTitle = "Getting ";
static const char *gfxHeaderEm = "ready.";
static const char *gfxHeaderStatus = "Loading graphics";

#define REM				WS_REM
#define LABEL_LINE		( 1.5f * 0.8f * REM )		/* .label: .8rem/1.5 */
#define HEADING_MARGIN	( 0.4f * REM )				/* h1, h2: margin .4rem 0 */
#define BODY_LINE		( 1.5f * REM )				/* body: 16px/1.5 */
#define BLOCK_MARGIN	REM							/* p, ol: 1em */
#define BUTTON_HEIGHT	( 2.0f * 0.65f * REM + BODY_LINE + 2.0f )
#define BUTTON_PAD_X	( 0.85f * REM )

static void GFX_Style( wsTextStyle_t *style, wsFont_t font, float size, float spacing, float line, qboolean upper ) {
	style->font = font;
	style->size = size;
	style->spacing = spacing;
	style->lineHeight = line;
	style->uppercase = upper;
}

static float GFX_Width( const wsTextStyle_t *style, const char *text ) {
	char upper[256];
	WS_Transform( style, text, upper, sizeof( upper ) );
	return WS_TextWidth( style, upper, strlen( upper ) );
}

static void GFX_WrapText( const wsTextStyle_t *style, const char *text, float width, wsLines_t *lines ) {
	char upper[256];
	WS_Transform( style, text, upper, sizeof( upper ) );
	WS_Wrap( style, upper, width, lines );
}

static float GFX_ButtonWidth( int button ) {
	wsTextStyle_t style;
	GFX_Style( &style, WS_BODY_BOLD, REM, 0.0f, BODY_LINE, qfalse );
	return 2.0f * BUTTON_PAD_X + 2.0f + GFX_Width( &style, gfxButtonLabels[button] );
}

void GFX_Layout( gfxLayout_t *l, float width, float height ) {
	wsTextStyle_t label, h1, h1em, body;
	float main, mainX, pad, contentX, contentW, y, headerH, divW, statusW;
	float sectionPad, column1, column2, gridY, leftH, rightH, top, line;
	float space;
	int i;


	GFX_Style( &label, WS_MONO, 0.8f * REM, 0.14f * 0.8f * REM, LABEL_LINE, qfalse );
	GFX_Style( &body, WS_BODY, REM, 0.0f, BODY_LINE, qfalse );

	/* main { max-width: 105rem; margin: auto; padding: clamp(1rem, 4vw, 5rem) } */
	l->pageWidth = width - WS_SCROLLBAR_GUTTER;
	main = l->pageWidth < 105.0f * REM ? l->pageWidth : 105.0f * REM;
	mainX = ( l->pageWidth - main ) * 0.5f;
	pad = WS_Clamp( REM, 0.04f * width, 5.0f * REM );
	contentX = mainX + pad;
	contentW = main - 2.0f * pad;
	y = pad;

	/* header: label and h1 (clamp(3rem, 8vw, 7rem)), status label on the right */
	l->h1Size = WS_Clamp( 3.0f * REM, 0.08f * width, 7.0f * REM );
	GFX_Style( &h1, WS_CONDENSED, l->h1Size, 0.0f, l->h1Size, qtrue );
	h1em = h1;
	h1em.font = WS_CONDENSED_ITALIC;
	l->headerX = contentX;
	l->headerY = y;
	l->h1Y = y + LABEL_LINE + HEADING_MARGIN;
	headerH = LABEL_LINE + 2.0f * HEADING_MARGIN + l->h1Size;
	divW = GFX_Width( &h1, gfxHeaderTitle ) + GFX_Width( &h1em, gfxHeaderEm );
	if ( GFX_Width( &label, gfxHeaderLabel ) > divW ) divW = GFX_Width( &label, gfxHeaderLabel );
	statusW = GFX_Width( &label, gfxHeaderStatus );
	if ( divW + REM + statusW <= contentW ) {
		l->statusX = contentX + contentW - statusW;
		l->statusY = y;
	} else {	/* flex-wrap: the status goes to its own line */
		l->statusX = contentX;
		l->statusY = y + headerH + REM;
		headerH += REM + LABEL_LINE;
	}

	/* .frontend-layout: minmax(14rem, .65fr) minmax(0, 1.8fr), gap 1.5rem;
	 * one column up to 42rem */
	gridY = y + headerH + 2.0f * REM;
	if ( width > 42.0f * REM ) {
		column1 = ( contentW - 1.5f * REM ) * 0.65f / 2.45f;
		if ( column1 < 14.0f * REM ) column1 = 14.0f * REM;
		column2 = contentW - 1.5f * REM - column1;
		if ( column2 < 0.0f ) column2 = 0.0f;
	} else {
		column1 = column2 = contentW;
	}
	sectionPad = WS_Clamp( REM, 0.03f * width, 2.0f * REM );
	l->left.x = contentX;
	l->left.w = column1;
	l->right.x = width > 42.0f * REM ? contentX + column1 + 1.5f * REM : contentX;
	l->right.w = column2;
	l->leftX = l->left.x + 1.0f + sectionPad;
	l->leftW = column1 - 2.0f - 2.0f * sectionPad;
	l->rightX = l->right.x + 1.0f + sectionPad;
	l->rightW = column2 - 2.0f - 2.0f * sectionPad;

	/* left section, from its content top */
	top = 1.0f + sectionPad;
	l->loadingLabelY = top;
	WS_Wrap( &label, gfxLoadingLabel, l->leftW, &l->loadingLabelLines );
	GFX_Style( &l->titleStyle, WS_CONDENSED, WS_Clamp( 1.5f * REM, 0.025f * width, 2.0f * REM ), 0.0f, 0.0f, qtrue );
	l->titleStyle.lineHeight = 1.1f * l->titleStyle.size;
	GFX_WrapText( &l->titleStyle, l->title, l->leftW, &l->titleLines );
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
		space = GFX_Width( &body, " " );
		l->buttons[0].x = l->leftX;
		l->buttons[0].y = y;
		l->buttons[0].w = GFX_ButtonWidth( 0 );
		l->buttons[0].h = BUTTON_HEIGHT;
		l->buttons[1].w = GFX_ButtonWidth( 1 );
		l->buttons[1].h = BUTTON_HEIGHT;
		if ( l->buttons[0].w + space + l->buttons[1].w <= l->leftW + 0.01f ) {
			l->buttons[1].x = l->leftX + l->buttons[0].w + space;
			l->buttons[1].y = y;
			y += BUTTON_HEIGHT;
		} else {
			l->buttons[1].x = l->leftX;
			l->buttons[1].y = y + BUTTON_HEIGHT;
			y += 2.0f * BUTTON_HEIGHT;
		}
	}
	/* without the prompt the percentage's bottom margin ends the content,
	 * with it the button line does */
	leftH = y + sectionPad + 1.0f;

	/* right section: label, h2 (clamp(1.5rem, 4vw, 3rem), line-height 1),
	 * the 3D view (clamp(15rem, 43vh, 32rem)), the status line */
	l->showroomLabelY = top;
	WS_Wrap( &label, gfxShowroomLabel, l->rightW, &l->showroomLabelLines );
	GFX_Style( &l->vehicleStyle, WS_CONDENSED, WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM ), 0.0f, 0.0f, qtrue );
	l->vehicleStyle.lineHeight = l->vehicleStyle.size;
	GFX_WrapText( &l->vehicleStyle, l->vehicle, l->rightW, &l->vehicleLines );
	l->vehicleY = top + l->showroomLabelLines.count * LABEL_LINE + HEADING_MARGIN;
	y = l->vehicleY + ( l->vehicleLines.count > 0 ? l->vehicleLines.count : 1 ) * l->vehicleStyle.lineHeight + HEADING_MARGIN;
	l->car.x = l->rightX;
	l->car.y = y;
	l->car.w = l->rightW;
	l->car.h = WS_Clamp( 15.0f * REM, 0.43f * height, 32.0f * REM );
	y += l->car.h + BLOCK_MARGIN;
	l->statusTextY = y;
	WS_Wrap( &body, l->status, l->rightW, &l->statusLines );
	y += ( l->statusLines.count > 0 ? l->statusLines.count : 1 ) * BODY_LINE + BLOCK_MARGIN;
	rightH = y + sectionPad + 1.0f;

	/* align-items: stretch, one row; or stacked in one column */
	l->left.y = gridY;
	if ( width > 42.0f * REM ) {
		if ( leftH < rightH ) leftH = rightH;
		rightH = leftH;
		l->right.y = gridY;
	} else {
		l->right.y = gridY + leftH + 1.5f * REM;
	}
	l->left.h = leftH;
	l->right.h = rightH;

	/* section-relative positions to the screen */
	l->loadingLabelY += l->left.y;
	l->titleY += l->left.y;
	for ( i = 0; i < GFX_LAYOUT_STAGES; i++ ) l->stageY[i] += l->left.y;
	l->progressY += l->left.y;
	l->percentY += l->left.y;
	l->promptY += l->left.y;
	l->buttons[0].y += l->left.y;
	l->buttons[1].y += l->left.y;
	l->showroomLabelY += l->right.y;
	l->vehicleY += l->right.y;
	l->car.y += l->right.y;
	l->statusTextY += l->right.y;

	l->pageHeight = l->right.y + l->right.h + pad;
	if ( l->left.y + l->left.h + pad > l->pageHeight ) l->pageHeight = l->left.y + l->left.h + pad;
}
