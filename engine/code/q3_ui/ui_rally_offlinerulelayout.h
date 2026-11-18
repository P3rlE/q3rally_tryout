/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Layout of step 2 of the offline race setup (rules and drivers), computed from
the WebUI's CSS (#offline-rules in engine/webui/garage/index.html,
garage.css) for the current screen size: the race summary with "Start race",
the rules (a grid of fields) and the starting grid (a row per slot, laid out
as the page's flex rows). From 64rem x 44rem the page fills the viewport and
the fields and the grid scroll; below it flows. Kept apart from the drawing
so a test can hold it against what Chrome reports.
*/

#ifndef UI_RALLY_OFFLINERULELAYOUT_H
#define UI_RALLY_OFFLINERULELAYOUT_H

#include "ui_rally_offlinelayout.h"

#define RULES_MAX_FIELDS	16
#define RULES_MAX_ROWS		12
#define RULES_MAX_PARTS		6

typedef enum {
	RULES_P,			/* "You / name", "Slot 2" */
	RULES_AVATAR,		/* the driver's avatar (the human's row) */
	RULES_TYPE,			/* label "Slot type" with its select */
	RULES_CAR,			/* the bot's car */
	RULES_BOT,			/* the bot's select */
	RULES_TEAM			/* label "Team" with its select */
} rulesPart_t;

typedef struct {
	rulesPart_t	kind;
	wsRect_t	box;			/* relative to the grid's content */
	wsRect_t	control;		/* labels: their select */
	wsLines_t	lines;			/* the p's or the label's text */
} rulesItem_t;

typedef struct {
	int			parts;
	rulesItem_t	part[RULES_MAX_PARTS];
	float		y, h;			/* relative to the grid's content */
	const char	*text;			/* the p */
} rulesRow_t;

typedef struct {
	/* inputs */
	qboolean	hosting;
	char		title[96];			/* #offline-race-title */
	char		modeName[64];		/* #offline-race-mode */
	const char	*ruleHelp, *gridHelp;
	int			fields;
	const char	*fieldText[RULES_MAX_FIELDS];	/* without the colon */
	qboolean	fieldInput[RULES_MAX_FIELDS];	/* an input, else a select */
	int			rows;
	/* per row: the parts in the page's order (RULES_P first) */
	int			rowParts[RULES_MAX_ROWS];
	rulesPart_t	rowKind[RULES_MAX_ROWS][RULES_MAX_PARTS];
	const char	*rowText[RULES_MAX_ROWS];
	qboolean	rowHuman[RULES_MAX_ROWS];
	float		optionsScroll, playersScroll, pageScroll;
	qboolean	hideScrollbar;		/* lay out like a browser with hidden scrollbars (tests) */

	offlineHeader_t head;
	float		pageHeight;
	wsRect_t	summary;
	float		summaryLabelY, titleY, modeY;
	wsTextStyle_t titleStyle, h2Style;
	wsLines_t	titleLines;
	wsRect_t	start;
	wsRect_t	left, right;
	float		leftX, leftW, rightX, rightW;
	float		rulesH2Y, ruleHelpY, gridH2Y, gridHelpY;
	wsLines_t	ruleHelpLines, gridHelpLines;
	wsRect_t	options;			/* scrollport */
	float		optionsHeight;
	qboolean	optionsBar;
	wsRect_t	optionsThumb;
	wsRect_t	fieldBox[RULES_MAX_FIELDS], fieldControl[RULES_MAX_FIELDS];	/* relative to the options' content */
	wsLines_t	fieldLines[RULES_MAX_FIELDS];
	wsRect_t	players;			/* scrollport */
	float		playersHeight, rowWidth;
	qboolean	playersBar;
	wsRect_t	playersThumb;
	rulesRow_t	row[RULES_MAX_ROWS];
	wsTextStyle_t rowLabel;		/* the labels' text in a row */
	float		selectHeight;		/* a select in a row's label */
	qboolean	pageBar;
	wsRect_t	page, pageThumb;
} rulesLayout_t;

void Rules_Layout( rulesLayout_t *layout, float width, float height );

#endif
