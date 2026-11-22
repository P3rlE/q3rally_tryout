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
#define MH_KV_MAX			6		/* the drag time slip: reaction, 60 ft .. trap */
#define MH_ROWS_MAX			6
#define MH_RPM_SEGMENTS		20
#define MH_RPM_HOT			16		/* segments from here on light in the accent */
#define MH_SLOTS_MAX		3		/* front, rear, item */
#define MH_TEAMS_MAX		4
#define MH_KILLS_MAX		3		/* the kill feed under the standings */
#define MH_FLAGS_MAX		4		/* CTF: a line per flag */
#define MH_ZONES_MAX		5		/* Domination: a tile per control point */
#define MH_TREE_ROWS		6		/* Drag: PRE, STAGE, three ambers, GO */
#define MH_TACH_SEGMENTS	40		/* Drag: the tachometer's segments */
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
	float		posSize;			/* the figure under posLabel, 0: 64 */
	qboolean	headerRow;			/* posLabel and pos on one baseline row (CTF: "YOUR TEAM" .. "Green") */
	int			flagCount;			/* CTF: icon, team, state, note, then a second line */
	const char	*flagName[MH_FLAGS_MAX], *flagState[MH_FLAGS_MAX], *flagNote[MH_FLAGS_MAX];
	const char	*labelRowLeft, *labelRowRight;	/* Drag: "YOU" "LANE LEFT", two labels on a row */
	const char	*hillLabel, *hillOwner;	/* KOTH: an icon, beside it "HILL HELD BY" over "Red" */
	int			zoneCount;			/* Domination: a row of tiles (A B C ..), then a row over a bar
									   (KOTH: the row and the bar only, zoneLabel set) */
	const char	*zoneName[MH_ZONES_MAX];
	const char	*zoneLabel, *zoneValue;	/* "CAPTURING C" "65%" */
	const char	*lapLabel, *lapValue;	/* "LAP" "2/5", NULL: none */
	qboolean	progress;			/* a sprint: the course as a bar, its checkpoints as ticks */
	int			progressTicks;
	const char	*progressLeft, *progressRight;	/* "CP 4 / 7" "58%" */
	const char	*derbyStatus;		/* derby: the car from above (110x170) and beside it the
									   worst zone ("FRONT CRITICAL") over a legend; NULL: none */
	qboolean	elim;				/* elimination: who goes out next, a bar, a line under it */
	const char	*elimLabel, *elimValue, *elimCaption;	/* "NEXT OUT" "LAP 3" "2 OF 3 THROUGH" */
	int			kvCount;
	const char	*kvKey[MH_KV_MAX], *kvValue[MH_KV_MAX];
	float		kvSize[MH_KV_MAX];	/* the value's size, 0: 17 */

	/* top right: the standings */
	qboolean	standings;
	const char	*orderTitle, *orderRight;
	int			teamCount;			/* the teams' standing over the rows, in their colours */
	const char	*teamName[MH_TEAMS_MAX], *teamValue[MH_TEAMS_MAX];
	qboolean	rowSwatch;			/* a square in the driver's team colour before the name */
	qboolean	rowBars;			/* name, a bar, a value 40px wide at the right (no place) */
	int			killCount;			/* the kill feed, oldest first: "Player" "ROCKET" "Paul" */
	const char	*killA[MH_KILLS_MAX], *killWeapon[MH_KILLS_MAX], *killB[MH_KILLS_MAX];	/* killB "": none */
	int			rowCount;
	const char	*rowPos[MH_ROWS_MAX], *rowName[MH_ROWS_MAX], *rowValue[MH_ROWS_MAX];
	qboolean	tree;				/* Drag: under the standings, two lamps a row and a label */
	const char	*treeLabel[MH_TREE_ROWS];

	/* top centre: rear view and the notification lane */
	qboolean	scorebug;			/* Autoball: red cell, the clock over a line, blue cell (in place of the mirror) */
	const char	*bugLeftLabel, *bugLeft, *bugClock, *bugSub, *bugRight, *bugRightLabel;
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

	/* bottom centre: the drag tachometer (in place of minimap and cockpit) */
	qboolean	tach;
	const char	*tachRpm, *tachMidLabel, *tachMidValue, *tachGear, *tachUnit, *tachSpeed, *tachLeft, *tachRight;

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
	mhText_t	elimLabelT, elimValueT, elimCaptionT;
	wsRect_t	derbyCar, derbySwatch[3];			/* the car in the design's 110x170 view box */
	wsRect_t	flagIcon[MH_FLAGS_MAX], sep2;
	wsRect_t	zoneTile[MH_ZONES_MAX], zoneBar, hillIcon;
	wsRect_t	bugBox, bugCell[3];			/* the border, then red, clock, blue */
	mhText_t	labelRowLeftT, labelRowRightT;
	wsRect_t	treeBox, treeLamp[MH_TREE_ROWS][2];
	mhText_t	treeTitleT, treeLabelT[MH_TREE_ROWS];
	wsRect_t	tachBox, tachSeg[MH_TACH_SEGMENTS];
	mhText_t	tachRpmLabelT, tachRpmT, tachMidLabelT, tachMidValueT, tachGearLabelT, tachGearT,
				tachUnitT, tachSpeedT, tachLeftT, tachRightT;
	mhText_t	bugLeftLabelT, bugLeftT, bugClockT, bugSubT, bugRightT, bugRightLabelT;
	mhText_t	hillLabelT, hillOwnerT;
	mhText_t	zoneNameT[MH_ZONES_MAX], zoneLabelT, zoneValueT;
	mhText_t	flagNameT[MH_FLAGS_MAX], flagStateT[MH_FLAGS_MAX], flagNoteT[MH_FLAGS_MAX];
	mhText_t	derbyStatusT, derbyLegendT[3];		/* OK, WORN, CRITICAL */
	wsRect_t	elimBar;

	wsRect_t	order, row[MH_ROWS_MAX];
	mhText_t	orderTitleT, orderRightT;
	mhText_t	rowPosT[MH_ROWS_MAX], rowNameT[MH_ROWS_MAX], rowValueT[MH_ROWS_MAX];
	wsRect_t	teamCell[MH_TEAMS_MAX], rowSwatchBox[MH_ROWS_MAX], rowBar[MH_ROWS_MAX];
	mhText_t	teamNameT[MH_TEAMS_MAX], teamValueT[MH_TEAMS_MAX];
	wsRect_t	killBox[MH_KILLS_MAX];
	mhText_t	killAT[MH_KILLS_MAX], killWeaponT[MH_KILLS_MAX], killBT[MH_KILLS_MAX];

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
	float		bottom;				/* its lower edge, 0: MH_BOTTOM above the screen's (drag: over the tachometer) */

	wsRect_t	box;
	mhText_t	labelT, nameT, hint1T, hint2T;
} mhSpectatorLayout_t;

/* the design's text styles at a design size */
void	MH_Label( wsTextStyle_t *style, float size );	/* Share Tech Mono, .16em, capitals */
void	MH_Big( wsTextStyle_t *style, float size );		/* Barlow Condensed bold italic, line-height 1 */
void	MH_Mono( wsTextStyle_t *style, float size );		/* Share Tech Mono figures */
void	MH_Text( wsTextStyle_t *style, float size );		/* Barlow */

/* Domination: a control point in the scene, its letter on a tile in its
 * owner's colour and the distance under it */
typedef struct {
	float		x, y;				/* inputs: the tile's centre */
	const char	*letter, *distance;
	wsRect_t	tile;				/* outputs */
	mhText_t	letterT, distanceT;
} mhZoneMarker_t;

#define MH_MARKER_SIZE	26.0f

/* the end screen of a race (gen.py EndRacing): your result at the left, the
 * podium with the cars and the highlights in the middle, the classification
 * at the right, the keys and who is ready at the bottom. Laid out on a
 * 1600 x 900 page at least; a narrower screen scales the page down. */
#define MH_END_STATS		4
#define MH_END_LAPS			12		/* the lap chart: the last ones */
#define MH_END_ROWS			16		/* the classification; more: yours stays in */
#define MH_END_AWARDS		3
#define MH_END_TEAMS		4		/* team races: up to four teams */
#define MH_END_DOTS			16		/* a team's drivers on the time axis */
#define MH_END_MEMBERS		4		/* a team's drivers listed on its podium card */

typedef struct {
	/* inputs */
	float		viewWidth;			/* at least 1600 */
	float		top;				/* the page's offset (a scaled-down page sits in the middle) */
	const char	*title, *subtitle;	/* "RACE RESULT" "DOWNTOWN · 5 LAPS · 8 DRIVERS" */
	const char	*pos, *of;			/* "P2" "/ 8" */
	const char	*doneLabel, *doneTime, *doneNote;	/* "FINISHED" "4:21.380" "+1.142 TO THE WINNER" */
	int			statCount;
	const char	*statLabel[MH_END_STATS], *statValue[MH_END_STATS], *statUnit[MH_END_STATS];
	const char	*chartTitle, *chartNote;	/* NULL: "YOUR LAPS" "LOST TO YOUR BEST" */
	int			lapCount;			/* the chart: the bars in 0..1, the line under each */
	float		lapFrac[MH_END_LAPS];
	const char	*lapValue[MH_END_LAPS], *lapName[MH_END_LAPS];
	int			spreadCount;		/* team races, in place of the lap chart: a row per team, */
	int			spreadDots[MH_END_TEAMS];	/* its drivers on the time axis (0..1), */
	float		spreadDot[MH_END_TEAMS][MH_END_DOTS];
	float		spreadTick[MH_END_TEAMS];	/* its average, the gap at the right */
	const char	*spreadValue[MH_END_TEAMS];
	qboolean	teamPodium;			/* the podium of teams: a 3px top, lower cars, the drivers under */
	const char	*podiumYou;			/* NULL: "YOU" */
	int			podiumMembers[3];
	const char	*podiumMemberPos[3][MH_END_MEMBERS], *podiumMemberName[3][MH_END_MEMBERS];
	int			teamCellCount;		/* the classification: the teams over the drivers */
	const char	*teamCellName[MH_END_TEAMS], *teamCellValue[MH_END_TEAMS];
	qboolean	rowSwatch;			/* a square in the team's colour before each name */
	int			podiumCount;		/* places 1..3 there are */
	const char	*podiumName[3], *podiumTime[3], *podiumGap[3], *podiumTag[3];
	qboolean	podiumMe[3];
	int			awardCount;
	const char	*awardLabel[MH_END_AWARDS], *awardName[MH_END_AWARDS], *awardValue[MH_END_AWARDS];
	const char	*listTitle, *listRight;	/* "FINAL CLASSIFICATION" "5 LAPS" */
	int			rowCount;
	qboolean	compact;			/* more than 8: lower rows */
	const char	*rowPos[MH_END_ROWS], *rowName[MH_END_ROWS], *rowTime[MH_END_ROWS], *rowBest[MH_END_ROWS];
	const char	*keyName[3], *keyAction[3];
	const char	*readyLabel, *readyValue;	/* "WAITING FOR 5 DRIVERS" "3 / 8 READY" */

	/* outputs, by place: [0] the winner, [1] second, [2] third */
	mhText_t	titleT, subtitleT, posT, ofT, doneLabelT, doneTimeT, doneNoteT;
	wsRect_t	sep[2];
	mhText_t	statLabelT[MH_END_STATS], statValueT[MH_END_STATS], statUnitT[MH_END_STATS];
	mhText_t	lapsTitleT, lapsNoteT, lapValueT[MH_END_LAPS], lapNameT[MH_END_LAPS];
	wsRect_t	lapSlot[MH_END_LAPS], lapBar[MH_END_LAPS];
	wsRect_t	podium[3], podiumCar[3], podiumRule[3];
	mhText_t	podiumMemberPosT[3][MH_END_MEMBERS], podiumMemberNameT[3][MH_END_MEMBERS];
	wsRect_t	spreadSwatch[MH_END_TEAMS], spreadTrack[MH_END_TEAMS], spreadDotR[MH_END_TEAMS][MH_END_DOTS], spreadTickR[MH_END_TEAMS];
	mhText_t	spreadValueT[MH_END_TEAMS];
	wsRect_t	teamCell[MH_END_TEAMS], rowSwatchR[MH_END_ROWS];
	mhText_t	teamCellNameT[MH_END_TEAMS], teamCellValueT[MH_END_TEAMS];
	mhText_t	podiumPlaceT[3], podiumTagT[3], podiumYouT[3], podiumNameT[3], podiumTimeT[3], podiumGapT[3];
	mhText_t	awardsTitleT;
	wsRect_t	award[MH_END_AWARDS], awardIcon[MH_END_AWARDS];
	mhText_t	awardLabelT[MH_END_AWARDS], awardNameT[MH_END_AWARDS], awardValueT[MH_END_AWARDS];
	wsRect_t	list, row[MH_END_ROWS];
	mhText_t	listTitleT, listRightT, headT[4];
	mhText_t	rowPosT[MH_END_ROWS], rowNameT[MH_END_ROWS], rowTimeT[MH_END_ROWS], rowBestT[MH_END_ROWS];
	mhText_t	keyNameT[3], keyActionT[3], readyLabelT, readyValueT;
	wsRect_t	readyBar;
} mhEndLayout_t;

void	MH_EndLayout( mhEndLayout_t *e );
void	MH_ZoneMarkerLayout( mhZoneMarker_t *m );
void	MH_RaceLayout( mhRaceLayout_t *l );
void	MH_MessageLayout( mhMessageLayout_t *m );
void	MH_SpectatorLayout( mhSpectatorLayout_t *s );

#endif
