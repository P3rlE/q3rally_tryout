/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Layout of the graphics loading screen, computed from the WebUI's CSS
(#frontend in engine/webui/garage/index.html and garage.css) for the
current screen size. Kept apart from the drawing so a test can hold it
against the positions Chrome reports.
*/

#ifndef UI_RALLY_GFXLAYOUT_H
#define UI_RALLY_GFXLAYOUT_H

#include "ui_rally_webstyle.h"

#define GFX_LAYOUT_STAGES	8

typedef struct {
	/* inputs */
	char		title[64];			/* #loading-title, before text-transform */
	char		vehicle[64];		/* #active-vehicle-name */
	char		status[96];			/* #active-vehicle-status */
	qboolean	prompt;				/* #update-prompt shown */

	/* the html box: the body background tiles at this size */
	float		pageWidth, pageHeight;

	/* header */
	float		headerX, headerY;	/* .label, then h1 below it */
	float		h1Y, h1Size;
	float		statusX, statusY;	/* #frontend-status */

	/* sections, border boxes */
	wsRect_t	left, right;
	float		leftX, rightX;		/* content box x */
	float		leftW, rightW;		/* content box width */

	/* left: loading panel */
	float		loadingLabelY;
	wsLines_t	loadingLabelLines;
	wsTextStyle_t titleStyle;
	wsLines_t	titleLines;
	float		titleY;
	float		stageX;				/* li content x */
	float		stageY[GFX_LAYOUT_STAGES];
	float		progressX, progressY, progressW;
	float		percentY;
	wsLines_t	promptLines;
	float		promptY;
	wsRect_t	buttons[2];			/* Open download page, Continue */

	/* right: showroom */
	float		showroomLabelY;
	wsLines_t	showroomLabelLines;
	wsTextStyle_t vehicleStyle;
	wsLines_t	vehicleLines;
	float		vehicleY;
	wsRect_t	car;				/* #active-vehicle-3d */
	wsLines_t	statusLines;
	float		statusTextY;
} gfxLayout_t;

extern const char *gfxStageLabels[GFX_LAYOUT_STAGES + 1];
extern const char *gfxPromptText;
extern const char *gfxLoadingLabel;
extern const char *gfxShowroomLabel;
extern const char *gfxButtonLabels[2];

/* Fills in everything below the inputs for a viewport of width x height. */
void GFX_Layout( gfxLayout_t *layout, float width, float height );

#endif
