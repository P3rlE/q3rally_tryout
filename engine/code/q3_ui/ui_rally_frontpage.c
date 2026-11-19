/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The WebUI's frontend page (#frontend in engine/webui/garage/index.html,
garage.css) rule for rule; see the header. Lengths in px; vw/vh are the
viewport including the scrollbar, the boxes are laid out in the viewport
minus the reserved scrollbar gutter.
*/

#include "ui_local.h"
#include "ui_rally_frontpage.h"
#include "ui_rally_vehicle_names.h"
#include "../game/garage_unlocks.h"

const char *wsShowroomLabel = "YOUR ACTIVE VEHICLE";
const char *wsGarageButton = "Customize in garage";

static const char *wsHeaderLabel = "Q3RALLY / IT'S DAMN FAST, BABY.";

static void WS_H1Styles( const wsFrontPage_t *page, wsTextStyle_t *h1, wsTextStyle_t *em ) {
	h1->font = WS_CONDENSED;
	h1->size = page->h1Size;
	h1->spacing = 0.0f;
	h1->lineHeight = page->h1Size;
	h1->uppercase = qtrue;
	*em = *h1;
	em->font = WS_CONDENSED_ITALIC;
}

void WS_FrontPage( wsFrontPage_t *page, float width, float height,
                   const char *title, const char *em, const char *status ) {
	WS_FrontPageLabeled( page, width, height, wsHeaderLabel, title, em, status );
}

void WS_FrontPageLabeled( wsFrontPage_t *page, float width, float height, const char *headerLabel,
                          const char *title, const char *em, const char *status ) {
	wsTextStyle_t label, h1, h1em;
	float main, headerH, divW, statusW;

	memset( page, 0, sizeof( *page ) );
	page->label = headerLabel;
	page->width = width;
	page->height = height;
	WS_LabelStyle( &label );

	/* main { max-width: 105rem; margin: auto; padding: clamp(1rem, 4vw, 5rem) } */
	page->pageWidth = width - WS_SCROLLBAR_GUTTER;
	main = page->pageWidth < 105.0f * WS_REM ? page->pageWidth : 105.0f * WS_REM;
	page->pad = WS_Clamp( WS_REM, 0.04f * width, 5.0f * WS_REM );
	page->contentX = ( page->pageWidth - main ) * 0.5f + page->pad;
	page->contentW = main - 2.0f * page->pad;

	/* header: label and h1 (clamp(3rem, 8vw, 7rem)), status label on the right */
	page->h1Size = WS_Clamp( 3.0f * WS_REM, 0.08f * width, 7.0f * WS_REM );
	WS_H1Styles( page, &h1, &h1em );
	page->headerX = page->contentX;
	page->headerY = page->pad;
	page->h1Y = page->pad + WS_LABEL_LINE + WS_HEADING_MARGIN;
	headerH = WS_LABEL_LINE + 2.0f * WS_HEADING_MARGIN + page->h1Size;
	divW = WS_StyledWidth( &h1, title ) + WS_StyledWidth( &h1em, em );
	if ( WS_StyledWidth( &label, headerLabel ) > divW ) divW = WS_StyledWidth( &label, headerLabel );
	page->titleW = divW;
	page->titleH = headerH;
	statusW = WS_StyledWidth( &label, status );
	if ( divW + WS_REM + statusW <= page->contentW ) {
		page->statusX = page->contentX + page->contentW - statusW;
		page->statusY = page->pad;
	} else {	/* flex-wrap: the status goes to its own line */
		page->statusX = page->contentX;
		page->statusY = page->pad + headerH + WS_REM;
		headerH += WS_REM + WS_LABEL_LINE;
	}

	/* .frontend-layout: minmax(14rem, .65fr) minmax(0, 1.8fr), gap 1.5rem;
	 * one column up to 42rem */
	page->gridY = page->pad + headerH + 2.0f * WS_REM;
	page->oneColumn = width <= 42.0f * WS_REM;
	if ( !page->oneColumn ) {
		page->column1 = ( page->contentW - 1.5f * WS_REM ) * 0.65f / 2.45f;
		if ( page->column1 < 14.0f * WS_REM ) page->column1 = 14.0f * WS_REM;
		page->column2 = page->contentW - 1.5f * WS_REM - page->column1;
		if ( page->column2 < 0.0f ) page->column2 = 0.0f;
		page->column2X = page->contentX + page->column1 + 1.5f * WS_REM;
	} else {
		page->column1 = page->column2 = page->contentW;
		page->column2X = page->contentX;
	}
	page->sectionPad = WS_Clamp( WS_REM, 0.03f * width, 2.0f * WS_REM );
}

float WS_PlaceHeaderButton( const wsFrontPage_t *page, const char *label, wsRect_t *button ) {
	button->w = WS_ButtonWidth( label );
	button->h = WS_BUTTON_HEIGHT;
	if ( page->titleW + WS_REM + button->w <= page->contentW ) {
		button->x = page->contentX + page->contentW - button->w;
		button->y = page->pad;
		return page->pad + page->titleH + 2.0f * WS_REM;
	}
	button->x = page->contentX;
	button->y = page->pad + page->titleH + WS_REM;
	return button->y + button->h + 2.0f * WS_REM;
}

/* label, h2 (clamp(1.5rem, 4vw, 3rem), line-height 1), the 3D view
 * (clamp(15rem, 43vh, 32rem)), the status line and the garage button */
float WS_ShowroomLayout( wsShowroom_t *room, const wsFrontPage_t *page,
                         const char *vehicle, const char *status, qboolean garageButton ) {
	wsTextStyle_t label, body;
	float top = 1.0f + page->sectionPad, y;

	memset( room, 0, sizeof( *room ) );
	WS_LabelStyle( &label );
	WS_BodyStyle( &body );
	room->box.x = page->column2X;
	room->box.w = page->column2;
	room->x = room->box.x + 1.0f + page->sectionPad;
	room->w = page->column2 - 2.0f - 2.0f * page->sectionPad;

	room->labelY = top;
	WS_Wrap( &label, wsShowroomLabel, room->w, &room->labelLines );
	room->vehicleStyle.font = WS_CONDENSED;
	room->vehicleStyle.size = WS_Clamp( 1.5f * WS_REM, 0.04f * page->width, 3.0f * WS_REM );
	room->vehicleStyle.lineHeight = room->vehicleStyle.size;
	room->vehicleStyle.uppercase = qtrue;
	WS_StyledWrap( &room->vehicleStyle, vehicle, room->w, &room->vehicleLines );
	room->vehicleY = top + room->labelLines.count * WS_LABEL_LINE + WS_HEADING_MARGIN;
	y = room->vehicleY + ( room->vehicleLines.count > 0 ? room->vehicleLines.count : 1 ) * room->vehicleStyle.lineHeight
		+ WS_HEADING_MARGIN;
	room->car.x = room->x;
	room->car.y = y;
	room->car.w = room->w;
	room->car.h = WS_Clamp( 15.0f * WS_REM, 0.43f * page->height, 32.0f * WS_REM );
	y += room->car.h + WS_BLOCK_MARGIN;
	room->statusY = y;
	WS_Wrap( &body, status, room->w, &room->statusLines );
	y += ( room->statusLines.count > 0 ? room->statusLines.count : 1 ) * WS_BODY_LINE + WS_BLOCK_MARGIN;
	if ( garageButton ) {
		/* an inline-block on its own line after the paragraph */
		room->garage.x = room->x;
		room->garage.y = y;
		room->garage.w = WS_ButtonWidth( wsGarageButton );
		room->garage.h = WS_BUTTON_HEIGHT;
		y += WS_BUTTON_HEIGHT;
	}
	return y + page->sectionPad + 1.0f;
}

void WS_PlaceShowroom( wsShowroom_t *room, float top, float height ) {
	room->box.y = top;
	room->box.h = height;
	room->labelY += top;
	room->vehicleY += top;
	room->car.y += top;
	room->statusY += top;
	room->garage.y += top;
}

void WS_VehicleName( const char *key, char *out, int size ) {
#define WS_UNLOCK_NAME( id, name, rank ) if ( !Q_stricmp( key, id ) ) { Q_strncpyz( out, name, size ); return; }
#define WS_LABEL_NAME( id, name ) if ( !Q_stricmp( key, id ) ) { Q_strncpyz( out, name, size ); return; }
	char *c;
	qboolean start = qtrue;

	GARAGE_CAR_UNLOCKS( WS_UNLOCK_NAME )
	UI_VEHICLE_LABELS( WS_LABEL_NAME )
	/* everything else: the folder in title case (build_showroom.py) */
	Q_strncpyz( out, key, size );
	for ( c = out; *c; c++ ) {
		if ( *c == '_' ) *c = ' ';
		*c = start ? toupper( (unsigned char)*c ) : tolower( (unsigned char)*c );
		start = !isalpha( (unsigned char)*c );
	}
#undef WS_UNLOCK_NAME
#undef WS_LABEL_NAME
}

/* -------------------------------------------------------------------------
   Drawing
   ------------------------------------------------------------------------- */

void WS_DrawFrontHeader( const wsFrontPage_t *page, const char *title, const char *em, const char *status ) {
	wsTextStyle_t h1, h1em;
	char          shown[64];

	WS_DrawLabel( page->headerX, page->headerY, page->label, NULL );
	WS_H1Styles( page, &h1, &h1em );
	WS_Transform( &h1, title, shown, sizeof( shown ) );
	WS_DrawText( &h1, page->headerX, page->h1Y, shown, strlen( shown ), wsWhite );
	WS_Transform( &h1em, em, shown, sizeof( shown ) );
	WS_DrawText( &h1em, page->headerX + WS_StyledWidth( &h1, title ), page->h1Y, shown, strlen( shown ), wsAccent );
	if ( status && status[0] ) {
		WS_DrawLabel( page->statusX, page->statusY, status, NULL );
	}
}

void WS_DrawShowroom( const wsShowroom_t *room, const char *vehicle, const char *status,
                      playerInfo_t *car, float azimuth, float elevation, float distance ) {
	wsTextStyle_t body;

	WS_BodyStyle( &body );
	WS_DrawSection( &room->box, qtrue );
	WS_DrawLabel( room->x, room->labelY, wsShowroomLabel, &room->labelLines );
	WS_DrawLines( &room->vehicleStyle, room->x, room->vehicleY, vehicle, &room->vehicleLines, wsWhite );
	WS_DrawLines( &body, room->x, room->statusY, status, &room->statusLines, wsWhite );
	if ( room->garage.w > 0.0f ) {
		WS_DrawButton( &room->garage, wsGarageButton, qtrue, qfalse );
	}
	if ( car ) {
		UI_DrawVehicleWebView( room->car.x, room->car.y, room->car.w, room->car.h, car, uis.realtime,
		                       azimuth, elevation, distance );
	}
}
