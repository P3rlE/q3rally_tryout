/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Layout of the main menu, computed from the WebUI's CSS (#frontend with
#main-navigation, and #quit-confirm) for the current screen size. Kept apart
from the drawing so a test can hold it against the positions Chrome reports.
*/

#ifndef UI_RALLY_MAINLAYOUT_H
#define UI_RALLY_MAINLAYOUT_H

#include "ui_rally_frontpage.h"

typedef enum {
	MAIN_NAV_OFFLINE,
	MAIN_NAV_ONLINE,
	MAIN_NAV_GARAGE,
	MAIN_NAV_CONFIG,
	MAIN_NAV_RIVALS,
	MAIN_NAV_DEMOS,
	MAIN_NAV_DOWNLOADS,
	MAIN_NAV_PROFILE,		/* .secondary, with the profile summary */
	MAIN_NAV_QUIT,			/* .secondary */
	MAIN_NAV_COUNT
} mainNav_t;

typedef struct {
	/* inputs */
	char		vehicle[64];		/* #active-vehicle-name */
	char		status[96];			/* #active-vehicle-status */
	char		summary[96];		/* #profile-summary */

	wsFrontPage_t page;
	float		pageWidth, pageHeight;	/* the html box */

	/* #main-navigation */
	wsTextStyle_t navStyle;
	wsRect_t	nav[MAIN_NAV_COUNT];
	float		navTextX, navTextY;	/* label offset inside a button */
	wsTextStyle_t summaryStyle;
	wsLines_t	summaryLines;
	float		summaryY;

	wsShowroom_t room;

	/* #quit-confirm, centered over a backdrop */
	wsRect_t	dialog;
	float		dialogX, dialogW;	/* content box */
	float		dialogLabelY;
	wsLines_t	dialogLabelLines;
	wsTextStyle_t dialogTitleStyle;
	wsLines_t	dialogTitleLines;
	float		dialogTitleY;
	wsLines_t	dialogTextLines;
	float		dialogTextY;
	wsRect_t	accept, cancel;
} mainLayout_t;

extern const char *mainNavLabels[MAIN_NAV_COUNT];
extern const char *mainQuitLabel;
extern const char *mainQuitTitle;
extern const char *mainQuitText;
extern const char *mainQuitAccept;
extern const char *mainQuitCancel;

/* Fills in everything below the inputs for a viewport of width x height. */
void MainMenu_Layout( mainLayout_t *layout, float width, float height );

#endif
