/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS of the WebUI's map loading screen, rule for rule (see the header).
Unlike the other pages it is written in px.
*/

#include "ui_local.h"
#include "ui_rally_maploadlayout.h"
#include "ui_rally_demoslayout.h"

const char *mapLoadNote = "Your race starts when the track and vehicles are ready.";

#define REM		WS_REM
#define LU		WS_LU

void MapLoad_Layout( mapLoadLayout_t *l, float width, float height ) {
	wsTextStyle_t label, h1, h1em, body;
	float main, y, column1, column2, previewH, statusH, top, lineBelow, title, labelW;
	char shown[128];

	WS_LabelStyle( &label );
	WS_BodyStyle( &body );
	l->width = width;
	l->height = height;

	/* #map-loading-screen: max-width 1600px, padding clamp(24px, 5vw, 80px),
	 * min-height 100vh */
	l->pageWidth = width - WS_SCROLLBAR_GUTTER;
	main = l->pageWidth < 1600.0f ? l->pageWidth : 1600.0f;
	l->pad = WS_Clamp( 24.0f, 0.05f * width, 80.0f );
	l->contentX = ( l->pageWidth - main ) * 0.5f + l->pad;
	l->contentW = main - 2.0f * l->pad;

	/* header (flex, space-between, wrap): the label and the h1 side by side */
	l->h1Size = WS_Clamp( 3.0f * REM, 0.08f * width, 7.0f * REM );
	h1.font = WS_CONDENSED; h1.size = l->h1Size; h1.spacing = 0.0f; h1.lineHeight = l->h1Size; h1.uppercase = qtrue;
	h1em = h1; h1em.font = WS_CONDENSED_ITALIC;
	title = WS_StyledWidth( &h1, "Ready to " ) + WS_StyledWidth( &h1em, "race." );
	labelW = WS_StyledWidth( &label, "Q3RALLY / NEXT STAGE" );
	l->labelY = l->pad;
	if ( labelW + REM + title <= l->contentW ) {
		l->h1X = l->contentX + l->contentW - title;
		l->h1Y = l->pad + WS_HEADING_MARGIN;
		top = l->pad + 2.0f * WS_HEADING_MARGIN + l->h1Size;
	} else {
		l->h1X = l->contentX;
		l->h1Y = l->pad + LU( WS_LABEL_LINE ) + REM + WS_HEADING_MARGIN;
		top = l->h1Y + l->h1Size + WS_HEADING_MARGIN;
	}
	top += 2.0f * REM;	/* margin-bottom 2rem, collapsed with the grid's 32px */

	/* .map-loading-grid: 1.2fr 1fr, gap 24px; one column up to 760px */
	l->oneColumn = width <= 760.0f;
	if ( !l->oneColumn ) {
		column1 = ( l->contentW - 24.0f ) * 1.2f / 2.2f;
		column2 = l->contentW - 24.0f - column1;
	} else {
		column1 = column2 = l->contentW;
	}
	l->sectionPad = WS_Clamp( 20.0f, 0.03f * width, 40.0f );
	l->preview.x = l->contentX;
	l->preview.w = column1;
	l->status.x = l->oneColumn ? l->contentX : l->contentX + column1 + 24.0f;
	l->status.w = column2;
	l->previewX = l->preview.x + 1.0f + l->sectionPad;
	l->previewW = column1 - 2.0f - 2.0f * l->sectionPad;
	l->statusX = l->status.x + 1.0f + l->sectionPad;
	l->statusW = column2 - 2.0f - 2.0f * l->sectionPad;

	l->title.font = WS_CONDENSED;
	l->title.size = LU( WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM ) );
	l->title.spacing = 0.0f;
	l->title.lineHeight = l->title.size;
	l->title.uppercase = qtrue;
	/* below the baseline of a line of body text: where an inline box ends */
	lineBelow = WS_BODY_LINE - WS_Baseline( &body );

	/* preview: the levelshot (an inline image, 16:9, margin-bottom 24px), the
	 * name, the server */
	y = 2.0f + l->sectionPad;
	memset( &l->shot, 0, sizeof( l->shot ) );
	if ( l->image ) {
		l->shot.x = l->previewX;
		l->shot.y = y;
		l->shot.w = l->previewW;
		l->shot.h = LU( l->previewW * 9.0f / 16.0f );
		y += l->shot.h + 24.0f + lineBelow;
	}
	WS_Transform( &l->title, l->name, shown, sizeof( shown ) );
	Demos_WrapAnywhere( &l->title, shown, l->previewW, &l->nameLines );
	l->nameY = y + WS_HEADING_MARGIN;
	y = l->nameY + ( l->nameLines.count > 0 ? l->nameLines.count : 1 ) * l->title.size + WS_BLOCK_MARGIN;
	l->serverY = y;
	y += l->server[0] ? WS_BODY_LINE + WS_BLOCK_MARGIN : 0.0f;
	previewH = y + l->sectionPad + 1.0f;

	/* status: label, the detail h2, the progress line (margin 28px 0 12px,
	 * vertical-align -.2em), the transfer, the note, Cancel */
	y = 2.0f + l->sectionPad;
	l->statusLabelY = y;
	y += LU( WS_LABEL_LINE ) + WS_HEADING_MARGIN;
	l->detailY = y;
	WS_Transform( &l->title, l->detail, shown, sizeof( shown ) );
	Demos_WrapAnywhere( &l->title, shown, l->statusW, &l->detailLines );
	y += ( l->detailLines.count > 0 ? l->detailLines.count : 1 ) * l->title.size + WS_HEADING_MARGIN;
	l->progress.x = l->statusX;
	l->progress.y = y + 28.0f;
	l->progress.w = l->statusW;
	l->progress.h = REM;
	y += 28.0f + REM + 12.0f - 0.2f * REM + lineBelow;
	Demos_WrapAnywhere( &body, l->transfer, l->statusW, &l->transferLines );
	l->transferY = y + WS_BLOCK_MARGIN;
	if ( l->transfer[0] ) y = l->transferY + l->transferLines.count * WS_BODY_LINE;
	l->noteY = y + WS_BLOCK_MARGIN;
	Demos_WrapAnywhere( &body, mapLoadNote, l->statusW, &l->noteLines );
	y = l->noteY + l->noteLines.count * WS_BODY_LINE + WS_BLOCK_MARGIN;
	l->cancel.x = l->statusX;
	l->cancel.y = y;
	l->cancel.w = WS_ButtonWidth( "Cancel" );
	l->cancel.h = WS_BUTTON_HEIGHT;
	statusH = y + WS_BUTTON_HEIGHT + l->sectionPad + 1.0f;

	/* one row as high as the taller section; or stacked */
	l->preview.y = top;
	if ( !l->oneColumn ) {
		if ( previewH < statusH ) previewH = statusH;
		statusH = previewH;
		l->status.y = top;
	} else {
		l->status.y = top + previewH + 24.0f;
	}
	l->preview.h = previewH;
	l->status.h = statusH;
	l->shot.y += l->preview.y;
	l->nameY += l->preview.y;
	l->serverY += l->preview.y;
	l->statusLabelY += l->status.y;
	l->detailY += l->status.y;
	l->progress.y += l->status.y;
	l->transferY += l->status.y;
	l->noteY += l->status.y;
	l->cancel.y += l->status.y;
	l->pageHeight = l->status.y + l->status.h + l->pad;
	if ( l->pageHeight < height ) l->pageHeight = height;
}
