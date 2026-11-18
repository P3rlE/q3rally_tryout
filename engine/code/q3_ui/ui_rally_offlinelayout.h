/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Layout of step 1 of the offline race setup (track and mode), computed from
the WebUI's CSS (#offline-screen in engine/webui/garage/index.html,
garage.css) for the current screen size. From 64rem x 44rem the page fills
the viewport and only the track list scrolls; below it flows. Kept apart
from the drawing so a test can hold it against what Chrome reports.
*/

#ifndef UI_RALLY_OFFLINELAYOUT_H
#define UI_RALLY_OFFLINELAYOUT_H

#include "ui_rally_frontpage.h"

#define OFFLINE_MAX_ROWS	64
#define OFFLINE_MAX_DETAILS	16

typedef struct {
	/* inputs */
	qboolean	hosting;			/* "Q3RALLY / HOST A SERVER" */
	int			count;				/* tracks in the list after the search */
	const char	*rowText[OFFLINE_MAX_ROWS];
	char		mapId[64];			/* #offline-map-id */
	char		name[96];			/* #offline-map-name */
	qboolean	image;				/* a levelshot exists */
	int			details;
	const char	*detail[OFFLINE_MAX_DETAILS];	/* trimmed */
	float		scroll;				/* #offline-map-list scrollTop, clamped here */
	float		pageScroll;			/* the page's own scroll below 64rem x 44rem, clamped here */
	qboolean	hideScrollbar;		/* lay out like a browser with hidden scrollbars (tests) */

	qboolean	desktop;			/* min-width 64rem and min-height 44rem */
	float		pageWidth, pageHeight, padX, padY, contentX, contentW;
	float		labelY, h1Y, h1Size;
	wsRect_t	back;
	float		stepY, stepRulesX;	/* the two steps, the first at contentX */
	wsRect_t	left, right;
	float		leftX, leftW, rightX, rightW;
	float		modeLabelY;
	wsRect_t	mode;				/* the select */
	float		searchLabelY;
	wsRect_t	search;
	float		countY;
	wsRect_t	list;				/* its scrollport */
	float		contentHeight;
	qboolean	scrollbar;
	wsRect_t	thumb;
	qboolean	pageScrollbar;		/* the page is taller than the viewport */
	wsRect_t	page, pageThumb;
	float		rowY[OFFLINE_MAX_ROWS], rowH[OFFLINE_MAX_ROWS], rowWidth;
	wsLines_t	rowLines[OFFLINE_MAX_ROWS];
	float		mapIdY;
	wsTextStyle_t title;			/* #offline-map-name */
	wsLines_t	nameLines;
	float		nameY;
	wsRect_t	shot;				/* #offline-map-image, cover */
	wsTextStyle_t detailStyle;
	float		detailX[OFFLINE_MAX_DETAILS], detailY[OFFLINE_MAX_DETAILS], detailW;
	wsLines_t	detailLines[OFFLINE_MAX_DETAILS];
	float		detailsY, detailsH;
	wsRect_t	next;				/* "Race setup" */
} offlineLayout_t;

void Offline_Layout( offlineLayout_t *layout, float width, float height );

#endif
