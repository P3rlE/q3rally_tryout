/*
===========================================================================
Copyright (C) 2002-2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/*
The heats of a drag match for the end screen. The server sends each car's
state ("dragStatus": phase, countdown, green, launch, finish, trap speed)
and keeps no history for the client; the client writes every heat down as
it goes, keyed by the heat's countdown start, with the splits the HUD
measures. Pure C, tested by tests/test_drag_log.py.
*/

#ifndef CG_DRAG_LOG_H
#define CG_DRAG_LOG_H

#define CG_DRAG_HEATS	16
#define CG_DRAG_SPLITS	3		/* 60 ft, 330 ft, 1/8 mile */

typedef enum {
	CG_DRAG_RUNNING,			/* not done yet */
	CG_DRAG_DONE,				/* over the line */
	CG_DRAG_FOUL,				/* a false start */
	CG_DRAG_OUT					/* did not finish */
} cgDragState_t;

typedef struct {
	int				client;		/* -1: an empty lane */
	cgDragState_t	state;
	int				rt;			/* reaction: green to launch, -1 unknown */
	int				et;			/* elapsed: launch to the line, -1 unknown */
	int				total;		/* green to the line (who was first over it), -1 */
	int				trap;		/* km/h x 10, -1 unknown */
	int				split[CG_DRAG_SPLITS];	/* ms after the launch, 0: none */
} cgDragRun_t;

typedef struct {
	int			countdown;		/* the server time its countdown began: the key */
	cgDragRun_t	run[2];
} cgDragHeat_t;

typedef struct {
	cgDragHeat_t	heat[CG_DRAG_HEATS];
	int				count;
} cgDragLog_t;

void CG_DragLogReset( cgDragLog_t *log );
/* a car's state as the server sent it (countdown 0: not armed, ignored) */
void CG_DragLogStatus( cgDragLog_t *log, int client, cgDragState_t state, int countdown,
                       int green, int launch, int finish, int trap );
void CG_DragLogSplit( cgDragLog_t *log, int client, int countdown, int index, int ms );
/* who won the heat: 0 or 1, -1 nobody (yet). margin: the gap over the line
 * in ms, or -1 when the other one fouled or did not finish */
int  CG_DragLogWinner( const cgDragHeat_t *h, int *margin );
/* the lane's car was in the heat: it launched, or the heat ended for it */
int  CG_DragLogPresent( const cgDragRun_t *r );
/* the heats a driver ran: their indexes, in order; returns how many */
int  CG_DragLogHeatsOf( const cgDragLog_t *log, int client, int *heats, int max );
const cgDragRun_t *CG_DragLogRun( const cgDragHeat_t *h, int client );

#endif
