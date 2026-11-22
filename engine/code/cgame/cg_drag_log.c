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

/* cg_drag_log.c -- the heats of a drag match, written down as they run */
#include "cg_drag_log.h"

#ifndef NULL
#define NULL ( (void *)0 )
#endif

static void CG_DragLogClear( cgDragRun_t *r ) {
	int i;
	r->client = -1;
	r->state = CG_DRAG_RUNNING;
	r->rt = r->et = r->total = r->trap = -1;
	for ( i = 0; i < CG_DRAG_SPLITS; i++ ) r->split[i] = 0;
}

void CG_DragLogReset( cgDragLog_t *log ) {
	log->count = 0;
}

static cgDragHeat_t *CG_DragLogHeat( cgDragLog_t *log, int countdown, int create ) {
	cgDragHeat_t *h;
	int i;
	for ( i = log->count - 1; i >= 0; i-- ) {
		if ( log->heat[i].countdown == countdown ) return &log->heat[i];
	}
	if ( !create ) return NULL;
	if ( log->count >= CG_DRAG_HEATS ) {	/* the oldest goes */
		for ( i = 1; i < CG_DRAG_HEATS; i++ ) log->heat[i - 1] = log->heat[i];
		log->count--;
	}
	h = &log->heat[log->count++];
	h->countdown = countdown;
	CG_DragLogClear( &h->run[0] );
	CG_DragLogClear( &h->run[1] );
	return h;
}

static cgDragRun_t *CG_DragLogLane( cgDragHeat_t *h, int client, int create ) {
	int i;
	for ( i = 0; i < 2; i++ ) if ( h->run[i].client == client ) return &h->run[i];
	if ( !create ) return NULL;
	for ( i = 0; i < 2; i++ ) {
		if ( h->run[i].client < 0 ) {
			h->run[i].client = client;
			return &h->run[i];
		}
	}
	return NULL;	/* a third car in a duel: not ours to keep */
}

const cgDragRun_t *CG_DragLogRun( const cgDragHeat_t *h, int client ) {
	int i;
	for ( i = 0; i < 2; i++ ) if ( h->run[i].client == client ) return &h->run[i];
	return NULL;
}

void CG_DragLogStatus( cgDragLog_t *log, int client, cgDragState_t state, int countdown,
                       int green, int launch, int finish, int trap ) {
	cgDragHeat_t *h;
	cgDragRun_t *r;
	if ( countdown <= 0 || client < 0 ) return;
	h = CG_DragLogHeat( log, countdown, 1 );
	r = CG_DragLogLane( h, client, 1 );
	if ( !r ) return;
	r->state = state;
	if ( green > 0 && launch >= green ) r->rt = launch - green;
	if ( state == CG_DRAG_DONE && launch > 0 && finish > launch ) {
		r->et = finish - launch;
		if ( green > 0 ) r->total = finish - green;
		if ( trap >= 0 ) r->trap = trap;
	}
}

void CG_DragLogSplit( cgDragLog_t *log, int client, int countdown, int index, int ms ) {
	cgDragHeat_t *h = CG_DragLogHeat( log, countdown, 0 );
	cgDragRun_t *r;
	if ( !h || index < 0 || index >= CG_DRAG_SPLITS ) return;
	r = CG_DragLogLane( h, client, 0 );
	if ( r && !r->split[index] ) r->split[index] = ms;
}

/* a lane counts once its car launched or the heat ended for it: a car
 * staged and gone before green was never in the heat */
int CG_DragLogPresent( const cgDragRun_t *r ) {
	return r->client >= 0 && ( r->state != CG_DRAG_RUNNING || r->rt >= 0 );
}

int CG_DragLogWinner( const cgDragHeat_t *h, int *margin ) {
	const cgDragRun_t *a = &h->run[0], *b = &h->run[1];
	int thereA = CG_DragLogPresent( a ), thereB = CG_DragLogPresent( b );
	int inA = thereA && a->state == CG_DRAG_DONE && a->total >= 0;
	int inB = thereB && b->state == CG_DRAG_DONE && b->total >= 0;
	*margin = -1;
	if ( thereA && thereB ) {
		if ( inA && inB ) {
			if ( a->total == b->total ) return -1;	/* a dead heat: run again */
			*margin = a->total < b->total ? b->total - a->total : a->total - b->total;
			return a->total < b->total ? 0 : 1;
		}
		/* over the line against a foul or a car that did not finish */
		if ( inA && b->state != CG_DRAG_RUNNING ) return 0;
		if ( inB && a->state != CG_DRAG_RUNNING ) return 1;
		return -1;
	}
	/* alone on the strip: the run counts if it finished */
	if ( thereA ) return inA ? 0 : -1;
	if ( thereB ) return inB ? 1 : -1;
	return -1;
}

int CG_DragLogHeatsOf( const cgDragLog_t *log, int client, int *heats, int max ) {
	int i, n = 0;
	for ( i = 0; i < log->count && n < max; i++ ) {
		if ( CG_DragLogRun( &log->heat[i], client ) ) heats[n++] = i;
	}
	return n;
}
