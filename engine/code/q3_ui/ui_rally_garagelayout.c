/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS of the WebUI's garage, rule for rule (see the header).
*/

#include "ui_local.h"
#include "ui_rally_garagelayout.h"

#define REM		WS_REM
#define LU		WS_LU

const char *garagePickerLabel[GARAGE_PICKERS] = { "Vehicle", "Paint", "Wheels", "Driver", "License plate" };

#define GARAGE_GAP			LU( 0.35f * REM )		/* .garage-carousel gap, .picker-label margin */
#define GARAGE_CARD_GAP		LU( 0.1f * REM )		/* the cards' flex gap */
#define GARAGE_CARD_MIN		LU( 5.8f * REM )		/* .carousel-current min-height */
#define GARAGE_NAME_LINE	LU( 1.5f * 0.95f * REM )
#define GARAGE_SMALL_LINE	LU( 1.5f * 0.7f * REM )
#define GARAGE_LOCK_LINE	LU( 1.5f * 0.65f * REM )
#define GARAGE_COUNT_LINE	12.0f					/* .65rem Share Tech Mono, line-height normal */

/* the flex column of a card, centred in its content box: the picture, the
 * name, "Locked", the count; returns the content's height */
static float Garage_CardContent( garageCard_t *c, float x, float w, float top, float inner, qboolean current, qboolean locked ) {
	float image = current ? 2.5f * REM : LU( 1.9f * REM ), name = current ? GARAGE_NAME_LINE : GARAGE_SMALL_LINE;
	float h = image + GARAGE_CARD_GAP + name, y;
	if ( locked ) h += GARAGE_CARD_GAP + GARAGE_LOCK_LINE;
	if ( current ) h += GARAGE_CARD_GAP + GARAGE_COUNT_LINE;
	if ( inner < 0.0f ) return h;
	y = top + ( inner - h ) * 0.5f;
	c->image.x = x;
	c->image.w = w;
	c->image.y = y;
	c->image.h = image;
	y += image + GARAGE_CARD_GAP;
	c->nameX = x;
	c->nameW = w;
	c->nameY = y;
	y += name + GARAGE_CARD_GAP;
	c->lockY = locked ? y : 0.0f;
	if ( locked ) y += GARAGE_LOCK_LINE + GARAGE_CARD_GAP;
	c->countY = current ? y : 0.0f;
	return h;
}

/* .garage-carousel: 2rem minmax(2.5rem, .5fr) minmax(5rem, 1.8fr)
 * minmax(2.5rem, .5fr) 2rem (or 2rem minmax(0, 1fr) 2rem when short);
 * returns the picker's height */
static float Garage_Picker( garagePicker_t *p, float x, float y, float width ) {
	float free, side, middle, h, innerCurrent, innerNeighbor;
	int i;

	p->labelY = y;
	y += LU( 1.5f * 0.9f * REM ) + GARAGE_GAP;
	p->box.x = x;
	p->box.y = y;
	p->box.w = width;
	p->short_ = p->count < 3;
	if ( p->short_ ) {
		side = 0.0f;
		middle = width - 4.0f * REM - 2.0f * GARAGE_GAP;
		if ( middle < 0.0f ) middle = 0.0f;
	} else {
		free = width - 4.0f * REM - 4.0f * GARAGE_GAP;
		side = 0.5f * free / 2.8f;
		middle = 1.8f * free / 2.8f;
		if ( side < 2.5f * REM ) {
			side = 2.5f * REM;
			middle = free - 2.0f * side;
		}
		if ( middle < 5.0f * REM ) {
			middle = 5.0f * REM;
			side = ( free - middle ) * 0.5f;
			if ( side < 2.5f * REM ) side = 2.5f * REM;
		}
	}
	/* the row: the tallest card, at least the current one's min-height */
	h = Garage_CardContent( &p->card[1], 0, 0, 0, -1.0f, qtrue, p->locked[1] ) + 2.0f * LU( 0.3f * REM ) + 2.0f;
	if ( h < GARAGE_CARD_MIN ) h = GARAGE_CARD_MIN;
	if ( !p->short_ ) {
		for ( i = 0; i < 3; i += 2 ) {
			float n = Garage_CardContent( &p->card[i], 0, 0, 0, -1.0f, qfalse, p->locked[i] ) + 2.0f * LU( 0.3f * REM ) + 2.0f;
			if ( n > h ) h = n;
		}
	}
	p->box.h = h;
	p->previous.x = x;
	p->previous.w = 2.0f * REM;
	p->next.w = 2.0f * REM;
	p->next.x = x + width - 2.0f * REM;
	p->previous.y = p->next.y = y;
	p->previous.h = p->next.h = h;
	memset( p->card, 0, sizeof( p->card ) );
	if ( p->short_ ) {
		p->card[1].box.x = x + 2.0f * REM + GARAGE_GAP;
	} else {
		p->card[0].box.x = x + 2.0f * REM + GARAGE_GAP;
		p->card[0].box.w = side;
		p->card[1].box.x = p->card[0].box.x + side + GARAGE_GAP;
		p->card[2].box.x = p->card[1].box.x + middle + GARAGE_GAP;
		p->card[2].box.w = side;
	}
	p->card[1].box.w = middle;
	/* .carousel-current: padding .3rem .4rem, border-bottom 2px;
	 * .carousel-neighbor: padding .3rem .15rem, a 1px border */
	innerCurrent = h - 2.0f * LU( 0.3f * REM ) - 2.0f;
	innerNeighbor = h - 2.0f * LU( 0.3f * REM ) - 2.0f;
	for ( i = 0; i < 3; i++ ) {
		garageCard_t *c = &p->card[i];
		if ( c->box.w <= 0.0f ) continue;
		c->box.y = y;
		c->box.h = h;
		if ( i == 1 ) Garage_CardContent( c, c->box.x + LU( 0.4f * REM ), c->box.w - 2.0f * LU( 0.4f * REM ), y + LU( 0.3f * REM ), innerCurrent, qtrue, p->locked[1] );
		else Garage_CardContent( c, c->box.x + 1.0f + LU( 0.15f * REM ), c->box.w - 2.0f - 2.0f * LU( 0.15f * REM ), y + 1.0f + LU( 0.3f * REM ), innerNeighbor, qfalse, p->locked[i] );
	}
	return p->box.y + h - p->labelY;
}

/* flex-wrap buttons (gap .5rem); returns the height */
static float Garage_Row( wsRect_t *buttons, int count, float x, float y, float width ) {
	float cx = x, cy = y;
	int i;
	for ( i = 0; i < count; i++ ) {
		if ( buttons[i].w <= 0.0f ) continue;
		if ( cx > x && cx + buttons[i].w > x + width + 0.01f ) {
			cx = x;
			cy += WS_BUTTON_HEIGHT + 0.5f * REM;
		}
		buttons[i].x = cx;
		buttons[i].y = cy;
		buttons[i].h = WS_BUTTON_HEIGHT;
		cx += buttons[i].w + 0.5f * REM;
	}
	return cy + WS_BUTTON_HEIGHT - y;
}

/* a button as wide as its column, its label wrapping inside */
static float Garage_WideButton( wsRect_t *button, wsLines_t *lines, const char *label, float x, float y, float width ) {
	wsTextStyle_t bold;
	WS_BodyStyle( &bold );
	bold.font = WS_BODY_BOLD;
	WS_StyledWrap( &bold, label, width - 2.0f - 2.0f * WS_BUTTON_PAD_X, lines );
	button->x = x;
	button->y = y;
	button->w = width;
	button->h = 2.0f * WS_LU( WS_BUTTON_PAD_Y ) + 2.0f + ( lines->count > 0 ? lines->count : 1 ) * WS_BODY_LINE;
	return button->h;
}

void Garage_Layout( garageLayout_t *l, float width, float height ) {
	wsTextStyle_t body, label;
	float top, pad, col1, col2, y, rowBottom, showroomBottom, configBottom;
	int i;

	WS_BodyStyle( &body );
	WS_LabelStyle( &label );
	WS_FrontPageLabeled( &l->page, width, height, "Q3RALLY / EMBEDDED GARAGE", "Your ", "garage.", "" );
	top = WS_PlaceHeaderButton( &l->page, "Back to game menu", &l->back );
	pad = l->page.sectionPad;
	l->h2.font = WS_CONDENSED;
	l->h2.size = l->h2.lineHeight = LU( WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM ) );
	l->h2.spacing = 0.0f;
	l->h2.uppercase = qtrue;

	/* .layout: minmax(0, 1.3fr) minmax(0, 1fr), gap 1rem; one column up to 42rem */
	if ( width <= 42.0f * REM ) {
		col1 = col2 = l->page.contentW;
	} else {
		col1 = ( l->page.contentW - REM ) * 1.3f / 2.3f;
		col2 = l->page.contentW - REM - col1;
	}
	l->showroom.x = l->page.contentX;
	l->showroom.y = top;
	l->showroom.w = col1;
	l->showroomX = l->showroom.x + 1.0f + pad;
	l->showroomW = col1 - 2.0f - 2.0f * pad;

	/* the showroom: label, h2, the picture (or the 3D view), the actions */
	y = top + 1.0f + pad;
	l->showroomLabelY = y;
	l->vehicleY = y + LU( WS_LABEL_LINE ) + LU( WS_HEADING_MARGIN );
	WS_StyledWrap( &l->h2, l->vehicle ? l->vehicle : "", l->showroomW, &l->vehicleLines );
	y = l->vehicleY + ( l->vehicleLines.count ? l->vehicleLines.count : 1 ) * l->h2.size + LU( WS_HEADING_MARGIN );
	l->image.x = l->showroomX;
	l->image.y = y;
	l->image.w = l->showroomW;
	l->image.h = l->view3d ? LU( WS_Clamp( 14.0f * REM, 0.33f * width, 27.0f * REM ) ) : LU( WS_Clamp( 10.0f * REM, 0.27f * width, 22.0f * REM ) );
	y += l->image.h + 0.75f * REM;
	l->toggle.w = WS_ButtonWidth( l->view3d ? "Photo view" : "3D view" );
	l->reset.w = l->view3d ? WS_ButtonWidth( "Reset view" ) : 0.0f;
	{
		wsRect_t row[2];
		row[0] = l->toggle;
		row[1] = l->reset;
		y += Garage_Row( row, 2, l->showroomX, y, l->showroomW );
		l->toggle = row[0];
		l->reset = row[1];
	}
	l->viewStatusX = ( l->reset.w > 0.0f ? l->reset.x + l->reset.w : l->toggle.x + l->toggle.w ) + 0.5f * REM;
	l->viewStatusY = l->toggle.y;
	showroomBottom = y + pad + 1.0f;

	/* the configuration: label, h2, the vehicle's carousel, the other four
	 * (.setup-controls: grid gap 1rem, margin-bottom 1rem), Apply, the web
	 * profile buttons, the two status lines */
	if ( width <= 42.0f * REM ) {
		l->config.x = l->page.contentX;
		l->config.y = 0.0f;	/* placed below the showroom */
	} else {
		l->config.x = l->page.contentX + col1 + REM;
		l->config.y = top;
	}
	l->config.w = col2;
	l->configX = l->config.x + 1.0f + pad;
	l->configW = col2 - 2.0f - 2.0f * pad;
	y = 1.0f + pad;
	l->configLabelY = y;
	l->configH2Y = y + LU( WS_LABEL_LINE ) + LU( WS_HEADING_MARGIN );
	y = l->configH2Y + l->h2.size + LU( WS_HEADING_MARGIN );
	for ( i = 0; i < GARAGE_PICKERS; i++ ) {
		y += Garage_Picker( &l->picker[i], l->configX, y, l->configW );
		y += REM;	/* the grid's gap, or the controls' margin */
	}
	l->apply.w = WS_ButtonWidth( "Apply setup" );
	l->apply.h = WS_BUTTON_HEIGHT;
	l->apply.x = l->configX;
	l->apply.y = y;
	y += WS_BUTTON_HEIGHT + 0.75f * REM;
	l->link.w = WS_ButtonWidth( "Link web profile" );
	l->pull.w = WS_ButtonWidth( "Load web setup and favorites" );
	{
		wsRect_t row[2];
		row[0] = l->link;
		row[1] = l->pull;
		y += Garage_Row( row, 2, l->configX, y, l->configW );
		l->link = row[0];
		l->pull = row[1];
	}
	/* p#web-profile-status (empty: its margins fold into the next p's),
	 * p#status: min-height 1.5em, both margin 1em 0 */
	y += REM;
	l->profileStatusY = y;
	WS_Wrap( &body, l->profileStatus ? l->profileStatus : "", l->configW, &l->profileStatusLines );
	if ( l->profileStatusLines.count ) y += l->profileStatusLines.count * WS_BODY_LINE + REM;
	l->statusY = y;
	WS_Wrap( &body, l->status ? l->status : "", l->configW, &l->statusLines );
	y += ( l->statusLines.count ? l->statusLines.count : 1 ) * WS_BODY_LINE + REM;
	configBottom = y + pad + 1.0f;

	/* the grid row: both sections as tall as the taller one */
	if ( width <= 42.0f * REM ) {
		l->showroom.h = showroomBottom - top;
		l->config.y = top + l->showroom.h + REM;
		l->config.h = configBottom;
		rowBottom = l->config.y + l->config.h;
	} else {
		float h = showroomBottom - top > configBottom ? showroomBottom - top : configBottom;
		l->showroom.h = l->config.h = h;
		rowBottom = top + h;
	}
	{
		float dy = l->config.y;
		l->configLabelY += dy; l->configH2Y += dy;
		for ( i = 0; i < GARAGE_PICKERS; i++ ) {
			garagePicker_t *p = &l->picker[i];
			int c;
			p->labelY += dy; p->box.y += dy; p->previous.y += dy; p->next.y += dy;
			for ( c = 0; c < 3; c++ ) {
				p->card[c].box.y += dy; p->card[c].image.y += dy; p->card[c].nameY += dy;
				if ( p->card[c].lockY ) p->card[c].lockY += dy;
				if ( p->card[c].countY ) p->card[c].countY += dy;
			}
		}
		l->apply.y += dy; l->link.y += dy; l->pull.y += dy; l->profileStatusY += dy; l->statusY += dy;
	}

	/* .favorite-panel: margin-top 1.5rem; label, h2, p, the slots
	 * (repeat(4, minmax(0, 1fr)), gap 1rem; two columns up to 42rem) */
	l->favorites.x = l->page.contentX;
	l->favorites.y = rowBottom + 1.5f * REM;
	l->favorites.w = l->page.contentW;
	l->favoritesX = l->favorites.x + 1.0f + pad;
	l->favoritesW = l->favorites.w - 2.0f - 2.0f * pad;
	y = l->favorites.y + 1.0f + pad;
	l->favoritesLabelY = y;
	l->favoritesH2Y = y + LU( WS_LABEL_LINE ) + LU( WS_HEADING_MARGIN );
	y = l->favoritesH2Y + l->h2.size + REM;
	l->favoritesTextY = y;
	WS_Wrap( &body, "Preview a saved setup or save your current selection in one of four slots.", l->favoritesW, &l->favoritesTextLines );
	y += l->favoritesTextLines.count * WS_BODY_LINE + REM;
	{
		int columns = width <= 42.0f * REM ? 2 : 4, row;
		float column = ( l->favoritesW - ( columns - 1 ) * REM ) / columns, rowTop = y;
		for ( row = 0; row * columns < GARAGE_SLOTS; row++ ) {
			float rowH = 0.0f;
			for ( i = row * columns; i < GARAGE_SLOTS && i < ( row + 1 ) * columns; i++ ) {
				garageSlot_t *s = &l->slot[i];
				float sy;
				s->box.x = l->favoritesX + ( i - row * columns ) * ( column + REM );
				s->box.y = rowTop;
				s->box.w = column;
				/* a grid (gap .5rem) of: p "Slot n" (margin 1em 0), the
				 * picture (5rem), the three buttons */
				s->labelY = rowTop + REM;
				sy = s->labelY + WS_BODY_LINE + REM + 0.5f * REM;
				memset( &s->picture, 0, sizeof( s->picture ) );
				if ( s->image ) {
					s->picture.x = s->box.x;
					s->picture.y = sy;
					s->picture.w = column;
					s->picture.h = 5.0f * REM;
					sy += 5.0f * REM + 0.5f * REM;
				}
				sy += Garage_WideButton( &s->loadButton, &s->loadLines, s->load ? s->load : "Empty", s->box.x, sy, column ) + 0.5f * REM;
				{
					wsLines_t saveLines;
					sy += Garage_WideButton( &s->save, &saveLines, "Save here", s->box.x, sy, column ) + 0.5f * REM;
				}
				sy += Garage_WideButton( &s->upload, &s->uploadLines, "Upload to web profile", s->box.x, sy, column );
				s->box.h = sy - rowTop;
				if ( s->box.h > rowH ) rowH = s->box.h;
			}
			for ( i = row * columns; i < GARAGE_SLOTS && i < ( row + 1 ) * columns; i++ ) l->slot[i].box.h = rowH;
			rowTop += rowH + REM;
		}
		y = rowTop - REM;
	}
	l->favorites.h = y + pad + 1.0f - l->favorites.y;
	l->pageHeight = l->favorites.y + l->favorites.h + l->page.pad;

	/* the page's scroll */
	l->viewport.x = 0.0f; l->viewport.y = 0.0f; l->viewport.w = width; l->viewport.h = height;
	l->pageBar = l->pageHeight > height + 0.5f;
	l->scroll = l->pageBar ? WS_Clamp( 0.0f, floor( l->scroll + 0.5f ), floor( l->pageHeight - height + 0.5f ) ) : 0.0f;
	memset( &l->pageThumb, 0, sizeof( l->pageThumb ) );
	if ( l->pageBar ) WS_ScrollThumb( &l->viewport, l->pageHeight, l->scroll, &l->pageThumb );
	( void )label;
}
