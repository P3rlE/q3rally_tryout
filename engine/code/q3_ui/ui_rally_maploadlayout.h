/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Layout of the map loading screen, computed from the WebUI's CSS
(#map-loading-screen in engine/webui/garage/index.html, garage.css;
map-loading.js) for the current screen size. Kept apart from the drawing so
a test can hold it against the positions Chrome reports.
*/

#ifndef UI_RALLY_MAPLOADLAYOUT_H
#define UI_RALLY_MAPLOADLAYOUT_H

#include "ui_rally_frontpage.h"

typedef struct {
	/* inputs */
	qboolean	image;				/* a levelshot exists */
	char		name[96];			/* #map-loading-name */
	char		server[96];
	char		detail[160];		/* #map-loading-detail */
	char		transfer[96];		/* #map-loading-transfer, may be empty */

	float		width, height, pageWidth, pageHeight;
	float		pad, contentX, contentW;
	float		labelY;				/* Q3RALLY / NEXT STAGE, at the left */
	float		h1X, h1Y, h1Size;	/* "Ready to race." at the right */
	qboolean	oneColumn;
	wsRect_t	preview, status;	/* the two sections */
	float		sectionPad;
	float		previewX, previewW, statusX, statusW;	/* content boxes */
	wsRect_t	shot;				/* the levelshot, 16:9 */
	wsTextStyle_t title;			/* the sections' h2 */
	wsLines_t	nameLines, detailLines, noteLines, transferLines;
	float		nameY, serverY;
	float		statusLabelY, detailY;
	wsRect_t	progress;			/* the <progress> box */
	float		transferY, noteY;
	wsRect_t	cancel;
} mapLoadLayout_t;

extern const char *mapLoadNote;

void MapLoad_Layout( mapLoadLayout_t *layout, float width, float height );

#endif
