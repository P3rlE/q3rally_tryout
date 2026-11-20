/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Where the in-game HUD of v0.9 puts things (the design: tools/ui_webstyle/
hud_design/gen.py, one 1600x900 board per game mode). Everything here is in
design pixels: the screen is 900 of them high, as wide as its aspect makes
it, and the drawing scales by the screen height. Kept apart from the drawing
so a test can hold it against what Chrome reports for the design
(tests/fixtures/hud_*_layout.json).
*/

#ifndef CG_HUD_MODERNLAYOUT_H
#define CG_HUD_MODERNLAYOUT_H

#include "../q3_ui/ui_rally_webstyle.h"

#define MH_DESIGN_HEIGHT	900.0f
#define MH_TEXT_MAX			96
#define MH_KV_MAX			4
#define MH_ROWS_MAX			6
#define MH_RPM_SEGMENTS		20
#define MH_RPM_HOT			16		/* segments from here on light in the accent */
#define MH_SLOTS_MAX		3		/* front, rear, item */
#define MH_TEAMS_MAX		4
#define MH_ELLIPSIS			"\x9e"	/* the body faces' "..." */

typedef struct {
	wsTextStyle_t	style;
	char			text[MH_TEXT_MAX];	/* as drawn: transformed, cut with an ellipsis */
	wsRect_t		box;				/* the element's box: its line box */
} mhText_t;

typedef struct {
	/* inputs ------------------------------------------------------------- */
	float		viewWidth;			/* the screen width in design pixels */

	/* top left: the mode card */
	const char	*title;				/* "Race · Downtown" (shown in capitals) */
	const char	*pos, *of;			/* "P3" "/ 6", NULL: no position */
	const char	*posLabel;			/* the ghost race: "VS GHOST" over "-0.84" (of unused) */
	const char	*lapLabel, *lapValue;	/* "LAP" "2/5", NULL: none */
	qboolean	progress;			/* a sprint: the course as a bar, its checkpoints as ticks */
	int			progressTicks;
	const char	*progressLeft, *progressRight;	/* "CP 4 / 7" "58%" */
	int			kvCount;
	const char	*kvKey[MH_KV_MAX], *kvValue[MH_KV_MAX];
	float		kvSize[MH_KV_MAX];	/* the value's size, 0: 17 */

	/* top right: the standings */
	qboolean	standings;
	const char	*orderTitle, *orderRight;
	int			teamCount;			/* the teams' standing over the rows, in their colours */
	const char	*teamName[MH_TEAMS_MAX], *teamValue[MH_TEAMS_MAX];
	qboolean	rowSwatch;			/* a square in the driver's team colour before the name */
	int			rowCount;
	const char	*rowPos[MH_ROWS_MAX], *rowName[MH_ROWS_MAX], *rowValue[MH_ROWS_MAX];

	/* top centre: rear view and the notification lane */
	qboolean	mirror;
	qboolean	toast, toastIcon;
	const char	*toastTag, *toastTitle, *toastSub;	/* toastSub NULL: two lines */

	/* bottom left: jukebox and the car */
	qboolean	jukebox, jukeboxOpen;
	const char	*jukeboxLine, *jukeboxTime;			/* closed: "Artist — Title" "1:42" */
	const char	*jukeboxTitle, *jukeboxArtist, *jukeboxState;	/* open */
	const char	*integrity, *armor;					/* armor NULL: not shown */
	float		vehicleWidth;						/* 0: 300 */
	int			slotCount;							/* weapons and items under a line */
	const char	*slotLabel[MH_SLOTS_MAX], *slotValue[MH_SLOTS_MAX], *slotName[MH_SLOTS_MAX];

	/* bottom right: minimap, slipstream, cockpit */
	qboolean	minimap;
	float		minimapSize;		/* its view, square (176) */
	qboolean	slipstream;
	const char	*slipValue;
	const char	*speed, *unit, *gear, *nos, *fuel;

	/* outputs ------------------------------------------------------------ */
	wsRect_t	card, sep;
	mhText_t	titleT, posLabelT, posT, ofT, lapLabelT, lapValueT;
	mhText_t	kvKeyT[MH_KV_MAX], kvValueT[MH_KV_MAX];
	wsRect_t	progressBar;		/* the ticks: drawn at i / (ticks + 1) of it, 2x12, 3px above */
	mhText_t	progressLeftT, progressRightT;

	wsRect_t	order, row[MH_ROWS_MAX];
	mhText_t	orderTitleT, orderRightT;
	mhText_t	rowPosT[MH_ROWS_MAX], rowNameT[MH_ROWS_MAX], rowValueT[MH_ROWS_MAX];
	wsRect_t	teamCell[MH_TEAMS_MAX], rowSwatchBox[MH_ROWS_MAX];
	mhText_t	teamNameT[MH_TEAMS_MAX], teamValueT[MH_TEAMS_MAX];

	wsRect_t	mirrorBox, toastBox, toastIconBox;
	mhText_t	toastTagT, toastTitleT, toastSubT;

	wsRect_t	jukeboxBox, jukeboxIcon, jukeboxBar;
	mhText_t	jukeboxLineT, jukeboxTimeT;
	mhText_t	jukeboxStateT, jukeboxTitleT, jukeboxArtistT, jukeboxModesT;
	wsRect_t	jukeboxButton[3];

	wsRect_t	vehicle, integrityBar, vehicleSep, slotIcon[MH_SLOTS_MAX];
	mhText_t	integrityLabel, integrityT, armorLabel, armorT;
	mhText_t	slotLabelT[MH_SLOTS_MAX], slotValueT[MH_SLOTS_MAX], slotNameT[MH_SLOTS_MAX];

	wsRect_t	minimapBox, minimapView;

	wsRect_t	slip, slipBar;
	mhText_t	slipLabel, slipValueT;

	wsRect_t	cockpit, gearBox, rpm, nosBar, fuelBar;
	mhText_t	speedT, unitT, gearLabel, gearT, nosLabel, nosValue, fuelLabel, fuelValue;
} mhRaceLayout_t;

/* a big message in the middle (gen.py bigmsg): a title in the condensed face,
 * up to three lines under it in the label face, centred */
#define MH_SUB_MAX			3
#define MH_MESSAGE_TOP		300.0f

typedef struct {
	float		viewWidth;
	const char	*title;
	float		titleSize;			/* 96 countdown, 56 warnings, 44 the rest; shrunk to fit */
	int			subCount;
	const char	*sub[MH_SUB_MAX];
	float		top;				/* 0: MH_MESSAGE_TOP */

	mhText_t	titleT, subT[MH_SUB_MAX];
} mhMessageLayout_t;

/* the spectator's panel at the bottom in the middle (gen.py spectator) */
#define MH_SPECTATOR_W		300.0f

typedef struct {
	float		viewWidth;
	const char	*label, *name, *hint1, *hint2;	/* "FOLLOWING" "Sam" "FIRE NEXT" "ESC MENU" */

	wsRect_t	box;
	mhText_t	labelT, nameT, hint1T, hint2T;
} mhSpectatorLayout_t;

/* the design's text styles at a design size */
void	MH_Label( wsTextStyle_t *style, float size );	/* Share Tech Mono, .16em, capitals */
void	MH_Big( wsTextStyle_t *style, float size );		/* Barlow Condensed bold italic, line-height 1 */
void	MH_Mono( wsTextStyle_t *style, float size );		/* Share Tech Mono figures */
void	MH_Text( wsTextStyle_t *style, float size );		/* Barlow */

void	MH_RaceLayout( mhRaceLayout_t *l );
void	MH_MessageLayout( mhMessageLayout_t *m );
void	MH_SpectatorLayout( mhSpectatorLayout_t *s );

#endif
