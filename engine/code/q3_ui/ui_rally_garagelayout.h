/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Where the WebUI's garage (#garage in engine/webui/garage/index.html,
garage.js, garage.css) puts things for the current screen size: the header,
the showroom, the five carousels of "Make it yours", the buttons and status
lines, and the four favorite slots. The page scrolls as a whole. Kept apart
from the drawing so a test can hold it against what Chrome reports.
*/

#ifndef UI_RALLY_GARAGELAYOUT_H
#define UI_RALLY_GARAGELAYOUT_H

#include "ui_rally_frontpage.h"

#define GARAGE_PICKERS		5		/* vehicle, paint, wheels, driver, license plate */
#define GARAGE_SLOTS		4

extern const char *garagePickerLabel[GARAGE_PICKERS];

/* a card of a carousel: its box and what it shows, top to bottom */
typedef struct {
	wsRect_t	box;
	wsRect_t	image;		/* the img box (object-fit: contain inside) */
	float		nameX, nameW, nameY, lockY, countY;
} garageCard_t;

typedef struct {
	/* inputs */
	int				count, index;
	qboolean		locked[3];		/* before, current, after */

	float			labelY;
	wsRect_t		box;			/* .garage-carousel */
	wsRect_t		previous, next;	/* the arrows */
	qboolean		short_;			/* fewer than three: no neighbors */
	garageCard_t	card[3];		/* before, current, after */
} garagePicker_t;

typedef struct {
	/* inputs */
	qboolean		used, image;
	const char		*load;			/* "Alpine / Blue", "Empty" */

	wsRect_t		box;
	float			labelY;
	wsRect_t		picture;
	wsRect_t		loadButton, save, upload;
	wsLines_t		loadLines, uploadLines;
} garageSlot_t;

typedef struct {
	/* inputs */
	const char		*vehicle;		/* the showroom's h2 */
	qboolean		view3d;
	const char		*viewStatus;
	const char		*status, *profileStatus;
	garagePicker_t	picker[GARAGE_PICKERS];
	garageSlot_t	slot[GARAGE_SLOTS];
	float			scroll;

	wsFrontPage_t	page;
	wsRect_t		back;
	wsRect_t		showroom, config, favorites;	/* the sections */
	float			showroomX, showroomW, configX, configW, favoritesX, favoritesW;
	float			showroomLabelY, vehicleY, configLabelY, configH2Y, favoritesLabelY, favoritesH2Y, favoritesTextY;
	wsTextStyle_t	h2;
	wsLines_t		vehicleLines;
	wsRect_t		image;			/* #vehicle-image, or #vehicle-3d */
	wsRect_t		toggle, reset;
	float			viewStatusX, viewStatusY;
	wsRect_t		apply, link, pull;
	float			profileStatusY, statusY;
	wsLines_t		profileStatusLines, statusLines, favoritesTextLines;
	float			pageHeight;
	qboolean		pageBar;
	wsRect_t		viewport, pageThumb;
} garageLayout_t;

void Garage_Layout( garageLayout_t *layout, float width, float height );

#endif
