/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Layout of the demo archive, computed from the WebUI's CSS (#demos-screen in
engine/webui/garage/index.html, garage.css; archive.js) for the current
screen size. Kept apart from the drawing so a test can hold it against the
positions Chrome reports.
*/

#ifndef UI_RALLY_DEMOSLAYOUT_H
#define UI_RALLY_DEMOSLAYOUT_H

#include "ui_rally_frontpage.h"

#define DEMOS_LAYOUT_MAX	256
#define DEMOS_DETAIL_LINE	21.609375f	/* .9rem/1.5 as Chrome lays it out */

typedef struct {
	/* inputs */
	int			count;					/* recordings in the list */
	float		scroll;					/* #demo-list scrollTop, clamped here */
	char		details[160];			/* #demo-details */
	qboolean	placeholder;			/* #demo-placeholder shown */
	char		placeholderText[96];
	char		clock[16];				/* #demo-clock */
	char		message[96];			/* #demo-message */
	char		pause[16];				/* "Pause" or "Resume" */
	qboolean	hideScrollbar;			/* lay out like a browser with hidden scrollbars (tests) */

	wsFrontPage_t page;
	float		pageWidth, pageHeight;
	wsRect_t	back;					/* #demos-back */
	wsRect_t	left, right;
	float		leftX, leftW, rightX, rightW;

	float		recordingsY;			/* RECORDINGS */
	float		chooseY;				/* "Choose a recording" */
	wsRect_t	list;					/* #demo-list, its border box */
	wsRect_t	port;					/* inside the border: the scrollport */
	float		optionHeight, optionWidth, contentHeight;
	qboolean	scrollbar;
	wsRect_t	thumb;
	wsLines_t	detailLines;
	float		detailsY;
	wsRect_t	play;					/* #demo-play */

	float		previewY;				/* PREVIEW */
	wsRect_t	viewport;				/* #demo-viewport, where the demo renders */
	float		placeholderX, placeholderY;
	wsRect_t	transport[5];			/* Restart, -10 s, Pause, +10 s, Stop */
	float		clockX, clockY, messageX;
} demosLayout_t;

extern const char *demosTransport[5];

void Demos_Layout( demosLayout_t *layout, float width, float height );
/* overflow-wrap: anywhere: at spaces, inside a word only where it has to */
void Demos_WrapAnywhere( const wsTextStyle_t *style, const char *text, float width, wsLines_t *lines );

#endif
