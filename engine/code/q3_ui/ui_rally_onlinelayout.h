/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Layout of the server browser, computed from the WebUI's CSS (#online-screen
in engine/webui/garage/index.html, garage.css) for the current screen size:
the toolbar (filters, toggles, buttons, status), the server list and the
destination, and the two dialogs. The page flows and scrolls as a whole; the
list scrolls from 38rem. Kept apart from the drawing so a test can hold it
against what Chrome reports.
*/

#ifndef UI_RALLY_ONLINELAYOUT_H
#define UI_RALLY_ONLINELAYOUT_H

#include "ui_rally_frontpage.h"

#define ONLINE_MAX_ROWS		128
#define ONLINE_DETAILS		5
#define ONLINE_FILTERS		4
#define ONLINE_TOGGLES		3
#define ONLINE_ACTIONS		4
#define ONLINE_BUTTONS		3

extern const char *onlineFilterText[ONLINE_FILTERS];
extern const char *onlineToggleText[ONLINE_TOGGLES];
extern const char *onlineActionText[ONLINE_ACTIONS];
extern const char *onlineDetailText[ONLINE_DETAILS];

typedef enum { ONLINE_NO_DIALOG, ONLINE_DIRECT, ONLINE_REMOVE } onlineDialog_t;

typedef struct {
	/* inputs */
	int			rows;
	const char	*rowName[ONLINE_MAX_ROWS], *rowSub[ONLINE_MAX_ROWS];
	const char	*status, *count, *empty, *message;
	qboolean	picked;					/* a server is selected */
	const char	*name, *address;		/* #online-name, #online-address */
	qboolean	image;
	const char	*detail[ONLINE_DETAILS];	/* the values, "" when empty */
	const char	*favorite;				/* "Save favorite" or "Saved favorite" */
	qboolean	remove;					/* the favorites source shows "Remove favorite" */
	onlineDialog_t dialog;
	const char	*dialogText;			/* the direct dialog's error, the address to remove */
	float		listScroll, pageScroll;
	qboolean	hideScrollbar;			/* lay out like a browser with hidden scrollbars (tests) */

	wsFrontPage_t page;
	wsRect_t	back;
	float		pageHeight;
	wsRect_t	toolbar;
	float		innerX, innerW;			/* the toolbar's content */
	wsRect_t	filterBox[ONLINE_FILTERS], filterControl[ONLINE_FILTERS];
	wsLines_t	filterLines[ONLINE_FILTERS];
	wsRect_t	toggle[ONLINE_TOGGLES];	/* the labels; the box is 4px in, 16 x 16 */
	wsRect_t	action[ONLINE_ACTIONS];
	float		statusY;
	wsLines_t	statusLines;
	wsRect_t	listSection, detailSection;
	float		listX, listW, detailX, detailW;
	float		headingY, headingPlayersX, headingPingX, playersW;
	float		countY;
	wsRect_t	list;					/* scrollport */
	float		listHeight, rowWidth;
	qboolean	listBar;
	wsRect_t	listThumb;
	float		rowY[ONLINE_MAX_ROWS], rowH[ONLINE_MAX_ROWS], nameW;	/* relative to the list's content */
	wsLines_t	nameLines[ONLINE_MAX_ROWS], subLines[ONLINE_MAX_ROWS];
	wsTextStyle_t subStyle;
	float		emptyY;
	wsLines_t	emptyLines;
	float		labelY, nameY;
	wsTextStyle_t h2;
	wsLines_t	nameLinesDetail;
	float		addressY;
	wsLines_t	addressLines;
	wsRect_t	shot;
	float		detailsY, detailsH;
	wsRect_t	card[ONLINE_DETAILS];
	float		cardValueY[ONLINE_DETAILS];
	wsLines_t	cardLabelLines[ONLINE_DETAILS], cardValueLines[ONLINE_DETAILS];
	wsRect_t	button[ONLINE_BUTTONS];	/* Connect, favorite, remove (w 0 when hidden) */
	float		messageY;
	wsLines_t	messageLines;
	qboolean	pageBar;
	wsRect_t	viewport, pageThumb;
	/* the open dialog, in the viewport */
	wsRect_t	dialogBox;
	float		dialogX, dialogW, dialogLabelY, dialogH2Y;
	wsLines_t	dialogH2Lines;
	float		dialogFieldLabelY;
	wsRect_t	dialogInput;
	float		dialogHelpY, dialogErrorY, dialogAddressY;
	wsLines_t	dialogHelpLines, dialogErrorLines, dialogAddressLines;
	wsRect_t	dialogButton[2];
} onlineLayout_t;

void Online_Layout( onlineLayout_t *layout, float width, float height );

#endif
