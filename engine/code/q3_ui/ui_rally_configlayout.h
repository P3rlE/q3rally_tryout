/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Where the WebUI's settings (#config-screen in engine/webui/garage/index.html,
config.js, garage.css) put things for the current screen size: the header,
the description, the buttons (nav.config-actions), the fields in their grid
and the two dialogs (confirm, key capture). The page flows and scrolls as a
whole. Kept apart from the drawing so a test can hold it against what Chrome
reports.
*/

#ifndef UI_RALLY_CONFIGLAYOUT_H
#define UI_RALLY_CONFIGLAYOUT_H

#include "ui_rally_frontpage.h"

#define CONFIG_MAX_FIELDS	48
#define CONFIG_MAX_BUTTONS	24
#define CONFIG_MAX_LINES	48

/* the item types of UI_WebConfigNumber( 2, item ) */
#define CONFIG_SPIN		1
#define CONFIG_RADIO	2
#define CONFIG_SLIDER	3
#define CONFIG_FIELD	4
#define CONFIG_KEY		5
#define CONFIG_BUTTON	6
#define CONFIG_LIST		7		/* a text list, shown as <pre> */
#define CONFIG_CHOOSE	8		/* a list to pick from, a <select> */

/* white-space: pre-wrap; overflow-wrap: anywhere, with more lines than wsLines_t */
typedef struct {
	int		count;
	int		start[CONFIG_MAX_LINES];
	int		length[CONFIG_MAX_LINES];
} configLines_t;

typedef enum { CONFIG_NO_DIALOG, CONFIG_CONFIRM, CONFIG_CAPTURE } configDialog_t;

typedef struct {
	/* inputs */
	int				type;
	const char		*label;			/* without the trailing colon */
	const char		*text;			/* the list's lines (CONFIG_LIST), the key (CONFIG_KEY) */

	wsRect_t		box;			/* .config-field, stretched to its grid row */
	wsLines_t		labelLines;
	wsRect_t		control;		/* select, input, range input (2px margin) or pre */
	float			outputY;		/* the slider's value */
	wsRect_t		rebind, clear;	/* CONFIG_KEY */
	configLines_t	lines;			/* CONFIG_LIST */
	float			listHeight;		/* the pre's content */
} configField_t;

typedef struct {
	/* inputs */
	int				screen;			/* the config page (0-15) */
	const char		*title, *description;
	int				buttons;		/* the button items config.js shows (not Back, not Resume race) */
	const char		*buttonLabel[CONFIG_MAX_BUTTONS];
	int				fields;
	configField_t	field[CONFIG_MAX_FIELDS];
	configDialog_t	dialog;
	const char		*captureAction;	/* the action being bound */
	float			pageScroll;

	wsFrontPage_t	page;
	const char		*headerLabel, *backLabel;
	wsRect_t		back;
	float			descriptionY;
	configLines_t	descriptionLines;
	wsRect_t		nav;
	wsRect_t		button[CONFIG_MAX_BUTTONS];		/* in nav, or in the confirm dialog */
	qboolean		settings;
	wsRect_t		section;
	float			fieldsX, fieldsW;
	int				columns;
	float			pageHeight;
	qboolean		pageBar;
	wsRect_t		viewport, pageThumb;
	/* the open dialog, in the viewport */
	wsRect_t		dialogBox;
	float			dialogX, dialogW, dialogLabelY, dialogH2Y, dialogActionY, dialogHelpY;
	wsTextStyle_t	h2;
	const char		*dialogLabel, *dialogTitle;
	wsLines_t		dialogH2Lines, dialogActionLines, dialogHelpLines;
	wsRect_t		cancel;			/* the capture dialog's button */
} configLayout_t;

extern const char *configCaptureHelp;

void Config_WrapPre( const wsTextStyle_t *style, const char *text, float width, configLines_t *lines );
void Config_Layout( configLayout_t *layout, float width, float height );

#endif
