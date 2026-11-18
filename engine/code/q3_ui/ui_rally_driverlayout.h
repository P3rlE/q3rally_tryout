/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Layout of the driver page, computed from the WebUI's CSS (#driver-screen and
#driver-dialog in engine/webui/garage/garage.css, built by driver.js) for the
current screen size: the tabs, the overview, the stats, the achievements
(cards with the medal floated right) and the two dialogs. The page flows and
scrolls as a whole; a dialog taller than the viewport scrolls itself. Kept
apart from the drawing so a test can hold it against what Chrome reports.
*/

#ifndef UI_RALLY_DRIVERLAYOUT_H
#define UI_RALLY_DRIVERLAYOUT_H

#include "ui_rally_frontpage.h"

#define DRIVER_STATS		67
#define DRIVER_GROUPS		16
#define DRIVER_MAX_MEDALS	128
#define DRIVER_MAX_PROFILES	64
#define DRIVER_AVATARS		32

extern const char *driverGroups[DRIVER_GROUPS];
extern const int driverGroupSize[DRIVER_GROUPS];
extern const char *driverStatKeys[DRIVER_STATS];
extern const char *driverStatLabels[DRIVER_STATS];
extern const char *driverOverviewLabels[4];

typedef enum { DRIVER_NO_DIALOG, DRIVER_EDIT, DRIVER_SWITCH } driverDialog_t;

/* a metric: p.label and the strong value */
typedef struct {
	wsRect_t	box;
	wsLines_t	label, value;
} driverCard_t;

/* an achievement */
typedef struct {
	wsRect_t	box;
	float		top;				/* the content's top (3px border when earned) */
	wsRect_t	medal;				/* floated right, 4rem */
	float		labelY, nameY, textY, progressY, statusY;
	wsLines_t	label, name, text, status;
} driverMedal_t;

typedef struct {
	/* inputs */
	int			tab;
	qboolean	profile;			/* an active profile */
	qboolean	avatar;
	const char	*name, *rank, *country;		/* country: "Country: Germany" or "" */
	const char	*overview[4];		/* the four values */
	const char	*progressText;
	const char	*stats[DRIVER_STATS];
	const char	*filterText;		/* the widest option, for the select's width */
	const char	*count;				/* "35 / 88 achievements earned" */
	int			medals;
	qboolean	earned[DRIVER_MAX_MEDALS];
	const char	*medalLabel[DRIVER_MAX_MEDALS], *medalName[DRIVER_MAX_MEDALS], *medalText[DRIVER_MAX_MEDALS], *medalStatus[DRIVER_MAX_MEDALS];
	driverDialog_t dialog;
	const char	*profileLine;		/* "Profile: Per" */
	int			avatars;
	int			profiles;
	const char	*profileName[DRIVER_MAX_PROFILES];	/* "Per · Active" */
	const char	*status;			/* the dialog's status line */
	float		pageScroll, dialogScroll, listScroll;
	qboolean	hideScrollbar;		/* lay out like a browser with hidden scrollbars (tests) */

	wsFrontPage_t page;
	wsRect_t	back;
	float		pageHeight;
	wsRect_t	tabs[3];
	float		panelY;
	/* overview */
	wsRect_t	identity, career;
	wsRect_t	avatarBox;
	float		identityX, identityY, identityW;
	float		identityLabelY, nameY, rankY, countryY;
	wsTextStyle_t h2;
	wsLines_t	nameLines, rankLines, countryLines;
	float		careerX, careerW, careerH2Y;
	driverCard_t overviewCard[4];
	float		progressY, progressTextY;
	wsRect_t	actions[3];
	/* stats */
	wsRect_t	group[DRIVER_GROUPS];
	float		groupH2Y[DRIVER_GROUPS];
	driverCard_t statCard[DRIVER_STATS];
	/* achievements */
	float		categoryY;
	wsRect_t	filter;
	float		countY;
	driverMedal_t medal[DRIVER_MAX_MEDALS];
	/* the dialog, in the viewport; its content scrolls */
	wsRect_t	dialogBox, dialogPort;
	float		dialogHeight;
	qboolean	dialogBar;
	wsRect_t	dialogThumb;
	float		dialogX, dialogW;
	float		dialogLabelY, dialogH2Y;
	float		profileLineY;
	float		fieldLabelY[3];
	wsRect_t	field[3];			/* gender, birth date, country */
	float		avatarLabelY;
	wsRect_t	avatarChoice[DRIVER_AVATARS];
	wsRect_t	list;				/* the profile list's scrollport */
	float		listHeight;
	qboolean	listBar;
	wsRect_t	listThumb;
	wsRect_t	profileButton[DRIVER_MAX_PROFILES];	/* relative to the list's content */
	float		createY, nameLabelY;
	wsRect_t	nameInput;
	float		helpY;
	wsLines_t	helpLines;
	wsRect_t	submit;
	float		statusY;
	wsLines_t	statusLines;
	wsRect_t	close;
	qboolean	pageBar;
	wsRect_t	viewport, pageThumb;
} driverLayout_t;

void Driver_Layout( driverLayout_t *layout, float width, float height );
/* the strong values: Barlow bold 1.6rem */
void Driver_ValueStyle( wsTextStyle_t *style );
/* the cards' h3: Barlow Condensed 1.5rem */
void Driver_MedalStyle( wsTextStyle_t *style );

#endif
