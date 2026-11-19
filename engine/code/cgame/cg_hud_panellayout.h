/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Where the WebUI's HUD options (#hud-screen in engine/webui/garage/
index.html, session.js, garage.css) put things: a panel fixed at the right
of the race, scrolling on its own. Kept apart from the drawing so a test can
hold it against what Chrome reports.
*/

#ifndef CG_HUD_PANELLAYOUT_H
#define CG_HUD_PANELLAYOUT_H

#include "../q3_ui/ui_rally_webstyle.h"

#define HUDPANEL_MAX_ITEMS	32

extern const char *hudPanelLabel, *hudPanelTitle, *hudPanelEm, *hudPanelText, *hudPanelHint;

typedef struct {
	/* inputs */
	int				items;
	const char		*itemLabel[HUDPANEL_MAX_ITEMS];
	float			scroll;

	wsRect_t		panel;			/* #hud-screen: border-left 3px, padding 1.5rem */
	float			x, w;			/* its content */
	float			labelY, h2Y, h2Size, emX;
	float			textY;
	wsLines_t		textLines;
	wsRect_t		close, console;	/* .session-actions */
	float			itemY[HUDPANEL_MAX_ITEMS];
	wsLines_t		itemLines[HUDPANEL_MAX_ITEMS];
	wsRect_t		select[HUDPANEL_MAX_ITEMS];
	float			h3Y;			/* Mini-map */
	float			sizeY, sizeOutputY, zoomY, zoomOutputY;
	wsRect_t		size, zoom;		/* the range inputs (2px margin) */
	float			hintY;
	float			contentHeight;	/* scrollHeight */
	qboolean		scrollbar;
	wsRect_t		thumb;
} hudPanelLayout_t;

void HUDPanel_Layout( hudPanelLayout_t *layout, float width, float height );

#endif
