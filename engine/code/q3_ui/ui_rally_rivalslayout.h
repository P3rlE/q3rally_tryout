/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Layout of the Rivals screen, computed from the WebUI's CSS (#library-screen
in engine/webui/garage/index.html with library.js in its rivals mode,
garage.css) for the current screen size. Kept apart from the drawing so a
test can hold it against the positions Chrome reports.
*/

#ifndef UI_RALLY_RIVALSLAYOUT_H
#define UI_RALLY_RIVALSLAYOUT_H

#include "ui_rally_frontpage.h"

#define RIVALS_MAX			64
#define RIVALS_SCROLLBAR	15.0f	/* Chrome's classic scrollbar, as CEF shows it */
#define RIVALS_ARROW		15.0f	/* its arrow buttons */

typedef struct {
	/* inputs */
	qboolean	downloads;			/* library.js mode 6: Downloads, else Rivals */
	qboolean	imageShown;			/* Downloads: a preview image exists */
	qboolean	hideScrollbar;		/* lay out like a browser with hidden scrollbars (tests) */
	char		author[96];			/* Downloads: "By ..." */
	int			count;				/* rivals in the list, after the search */
	const char	*rowText[RIVALS_MAX];	/* each button's text */
	char		name[64];			/* the selected rival */
	char		personality[256];
	char		weapon[256];		/* "Favorite weapon / ..." or empty */
	char		description[256];
	char		status[64];
	float		scroll;				/* #library-list scrollTop, clamped here */

	wsFrontPage_t page;
	float		pageWidth, pageHeight;
	wsRect_t	back;				/* #library-back */
	wsRect_t	left, right;		/* the two sections */
	float		leftX, leftW, rightX, rightW;	/* content boxes */

	float		searchLabelY;		/* "Search" */
	wsRect_t	search;				/* #library-search */
	float		typeLabelY;			/* Downloads: "Content type" */
	wsRect_t	type;				/* Downloads: #download-type */
	float		countY;				/* #library-count */
	wsRect_t	list;				/* #library-list, its scrollport */
	float		contentHeight;		/* scrollHeight */
	qboolean	scrollbar;			/* the list overflows */
	float		rowGap;				/* buttons in the list */
	float		rowY[RIVALS_MAX];	/* relative to the list content */
	float		rowH[RIVALS_MAX];
	wsLines_t	rowLines[RIVALS_MAX];
	float		rowTextX;			/* from the button's left */
	float		buttonWidth;		/* the list's width without the scrollbar */
	wsRect_t	thumb;				/* the scrollbar thumb, when there is one */

	float		labelY;				/* RIVAL PROFILE */
	wsTextStyle_t title;			/* the h2 */
	wsLines_t	nameLines;
	float		nameY;
	wsRect_t	car;				/* #rival-3d, or #library-image for Downloads */
	wsLines_t	authorLines, personalityLines, weaponLines, descriptionLines, statusLines;
	float		authorY, personalityY, weaponY, descriptionY, statusY;

	/* Downloads: #download-controls */
	wsRect_t	install, refresh, cancel;
	wsRect_t	progress;			/* the <progress> box, 16px high */
	wsLines_t	downloadStatusLines;
	float		downloadStatusY;
	char		downloadStatus[256];	/* input */
} rivalsLayout_t;

extern const char *rivalsLabel;
extern const char *rivalsBack;
extern const char *rivalsDetailLabel;
extern const char *rivalsPlaceholder;
extern const char *downloadsLabels[3];		/* Install, Refresh, Cancel */

void Rivals_Layout( rivalsLayout_t *layout, float width, float height );
/* the thumb of Chrome's scrollbar for a scrollport, content height and scroll */
/* the box of a list button, on screen (scrolled), before clipping to the list */
void Rivals_Button( const rivalsLayout_t *layout, int row, wsRect_t *button );

#endif
