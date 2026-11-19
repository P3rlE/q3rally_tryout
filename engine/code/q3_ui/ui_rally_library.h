/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

What Rivals and Downloads share, as the WebUI's #library-screen does: the
search field, the scrolling list with Chrome's dark scrollbar and the
header. The screens keep their own data and details.
*/

#ifndef UI_RALLY_LIBRARY_H
#define UI_RALLY_LIBRARY_H

#include "ui_rally_rivalslayout.h"

typedef struct {
	rivalsLayout_t	layout;
	char			query[64];		/* #library-search */
	qboolean		searchFocus;
	int				searchFocusTime;
	float			scroll;			/* #library-list scrollTop */
	qboolean		dragThumb;
	float			dragY, dragScroll;
	int				selectedRow;	/* aria-pressed, -1 none */
} wsLibrary_t;

/* the cursor in screen pixels */
void	Library_Cursor( float *x, float *y );
qboolean Library_Inside( const wsRect_t *r, float x, float y );

/* the query lowercased and trimmed, as library.js matches it */
void	Library_Query( const wsLibrary_t *lib, char *out, int size );
void	Library_Lower( const char *in, char *out, int size );

void	Library_DrawSearch( const wsLibrary_t *lib, const char *placeholder );
/* the list buttons with their texts; icons per row or NULL */
void	Library_DrawList( const wsLibrary_t *lib, const qhandle_t *icons );
void	Library_DrawScrollbar( const wsLibrary_t *lib );
/* Chrome's dark scrollbar at the right of a scrollport */
/* a button drawn at opacity .45 when disabled, as Chrome does */
void	Library_DrawButton( const wsRect_t *box, const char *label, qboolean secondary, qboolean disabled );

/* follows a dragged scrollbar thumb; call every frame */
void	Library_Track( wsLibrary_t *lib );
/* scrolls so the row shows */
void	Library_ScrollTo( wsLibrary_t *lib, int row );
/* a click on the search field or the scrollbar; returns qtrue when it took it;
 * *changed is set when the query changed (the × button) */
qboolean Library_Click( wsLibrary_t *lib, float x, float y, qboolean *changed );
/* the row under the cursor, -1 none */
int		Library_RowAt( const wsLibrary_t *lib, float x, float y );
/* typing into the focused search field; qtrue when the query changed */
qboolean Library_Char( wsLibrary_t *lib, int ch );

#endif
