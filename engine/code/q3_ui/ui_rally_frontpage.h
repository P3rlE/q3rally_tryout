/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The page the loading screen and the main menu share in the WebUI
(<main id="frontend">): header with label, h1 and status label, a two column
grid, and the showroom section with the active vehicle. Lengths follow
garage.css; see docs/native-webstyle.md.
*/

#ifndef UI_RALLY_FRONTPAGE_H
#define UI_RALLY_FRONTPAGE_H

#include "ui_rally_webstyle.h"


/* The showroom camera the WebUI opens with (frontend.js: yaw 135, distance
 * 2.6) and its 18 degree elevation; OrbitControls limits for dragging. */
#define WS_VIEW_AZIMUTH		135.0f
#define WS_VIEW_ELEVATION	18.0f
#define WS_VIEW_DISTANCE	2.6f
#define WS_VIEW_MIN_ELEVATION	-10.0f		/* maxPolarAngle PI / 1.8 */
#define WS_VIEW_MAX_ELEVATION	67.5f		/* minPolarAngle PI / 8 */
#define WS_VIEW_MIN_DISTANCE	2.1f
#define WS_VIEW_MAX_DISTANCE	7.0f

typedef struct {
	float		width, height;		/* viewport */
	float		pageWidth;			/* html box: the viewport minus the scrollbar gutter */
	float		pad;				/* main padding */
	float		contentX, contentW;	/* main content box */
	/* header */
	float		headerX, headerY, h1Y, h1Size, statusX, statusY;
	float		titleW, titleH;		/* the header's first child: label and h1 */
	const char	*label;				/* the label above the h1 */
	/* .frontend-layout */
	float		gridY;
	qboolean	oneColumn;			/* up to 42rem */
	float		column1, column2, column2X;
	float		sectionPad;			/* section padding */
} wsFrontPage_t;

typedef struct {
	wsRect_t		box;			/* border box, y relative until placed */
	float			x, w;			/* content box */
	float			labelY;
	wsLines_t		labelLines;
	wsTextStyle_t	vehicleStyle;
	wsLines_t		vehicleLines;
	float			vehicleY;
	wsRect_t		car;			/* #active-vehicle-3d */
	wsLines_t		statusLines;
	float			statusY;
	wsRect_t		garage;			/* "Customize in garage", w 0 when hidden */
} wsShowroom_t;

extern const char *wsShowroomLabel;
extern const char *wsGarageButton;


/* main, header and grid of the page; title + em is the h1 */
void	WS_FrontPageLabeled( wsFrontPage_t *page, float width, float height, const char *label,
							 const char *title, const char *em, const char *status );
/* A secondary button at the header's right ("Back to main menu"), wrapping
 * below the title like flex-wrap does; returns the top of what follows. */
float	WS_PlaceHeaderButton( const wsFrontPage_t *page, const char *label, wsRect_t *button );
void	WS_FrontPage( wsFrontPage_t *page, float width, float height,
					  const char *title, const char *em, const char *status );
/* The showroom section in the second column; returns its natural height.
 * Positions are relative to the section's top until WS_PlaceShowroom. */
float	WS_ShowroomLayout( wsShowroom_t *room, const wsFrontPage_t *page,
						   const char *vehicle, const char *status, qboolean garageButton );
void	WS_PlaceShowroom( wsShowroom_t *room, float top, float height );

/* The name the WebUI catalog shows for a vehicle folder. */
void	WS_VehicleName( const char *key, char *out, int size );

void	WS_DrawFrontHeader( const wsFrontPage_t *page, const char *title, const char *em, const char *status );
void	WS_DrawShowroom( const wsShowroom_t *room, const char *vehicle, const char *status,
						 playerInfo_t *car, float azimuth, float elevation, float distance );

#endif
