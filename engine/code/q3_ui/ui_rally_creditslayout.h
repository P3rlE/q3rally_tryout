/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Layout of the end credits, computed from the WebUI's CSS (#credits-screen in
engine/webui/garage/index.html, garage.css) and its roll animation
(archive.js) for the current screen size. Kept apart from the drawing so a
test can hold it against the positions Chrome reports.
*/

#ifndef UI_RALLY_CREDITSLAYOUT_H
#define UI_RALLY_CREDITSLAYOUT_H

#include "ui_rally_frontpage.h"

#define CREDITS_MAX_LINES	128
#define CREDITS_SPEED		38.0f	/* px per second */
#define CREDITS_FADE		0.12f	/* mask: transparent, #000 12%, #000 88%, transparent */

/* the kinds the client publishes for a credit line */
#define CREDITS_TEXT		0		/* p */
#define CREDITS_HEADING		1		/* p.label */
#define CREDITS_TITLE		2		/* h2 */

typedef struct {
	wsFrontPage_t page;
	float		pageWidth, pageHeight;	/* the html box */
	wsRect_t	exit;				/* #credits-exit */
	wsRect_t	window;				/* #credits-window */
	float		hintY;				/* PRESS ANY KEY TO EXIT */
	wsTextStyle_t title;			/* #credits-roll h2 */
	int			count;
	float		lineY[CREDITS_MAX_LINES];	/* tops, relative to the roll at rest */
	float		lineH[CREDITS_MAX_LINES];
	float		rollTop;			/* the first line's top in the window at rest */
	float		rollHeight;			/* scrollHeight */
	float		duration;			/* seconds from entering at the bottom to leaving at the top */
} creditsLayout_t;

extern const char *creditsLabel;
extern const char *creditsExitLabel;
extern const char *creditsHint;

/* kinds[count]: CREDITS_* of every line */
void Credits_Layout( creditsLayout_t *layout, float width, float height, const int *kinds, int count );
/* the top of a line on screen, seconds into the roll */
float Credits_LineTop( const creditsLayout_t *layout, int line, float seconds );

#endif
