/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

#include "cg_race_gap.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

const cgRaceSplit_t *CG_RaceSplitAt( const cgRaceSplitHistory_t *history, int age ) {
	int index;
	if ( age < 0 || age >= history->count ) return NULL;
	index = history->next - 1 - age;
	while ( index < 0 ) index += CG_RACE_SPLIT_HISTORY;
	return &history->splits[index];
}

void CG_RaceSplitAdd( cgRaceSplitHistory_t *history, int lap, int checkpoint, int time ) {
	cgRaceSplit_t *split;
	int age;

	for ( age = 0; age < history->count; age++ ) {
		split = (cgRaceSplit_t *)CG_RaceSplitAt( history, age );
		if ( split->lap == lap && split->checkpoint == checkpoint ) {
			split->time = time;
			return;
		}
	}
	split = &history->splits[history->next];
	split->lap = lap;
	split->checkpoint = checkpoint;
	split->time = time;
	history->next = ( history->next + 1 ) % CG_RACE_SPLIT_HISTORY;
	if ( history->count < CG_RACE_SPLIT_HISTORY ) history->count++;
}

int CG_RaceGap( const cgRaceSplitHistory_t *ahead, int aheadFinish,
                const cgRaceSplitHistory_t *behind, int behindFinish, int now, int *gapMs ) {
	const cgRaceSplit_t *b, *a, *next;
	int i, j, gap = 0, found = 0;

	/* both home: the finish times */
	if ( aheadFinish && behindFinish ) {
		*gapMs = behindFinish - aheadFinish;
		return 1;
	}
	/* home before the one "ahead": nothing sensible to show */
	if ( behindFinish ) return 0;

	/* the last checkpoint both have crossed, newest first */
	for ( i = 0; i < behind->count && !found; i++ ) {
		b = CG_RaceSplitAt( behind, i );
		for ( j = 0; j < ahead->count; j++ ) {
			a = CG_RaceSplitAt( ahead, j );
			if ( a->lap != b->lap || a->checkpoint != b->checkpoint ) continue;
			gap = b->time - a->time;
			found = 1;
			/* the one ahead has crossed the next checkpoint already: the one
			 * behind, who has not (it is his latest), is at least that long
			 * behind by now. Not when he has newer checkpoints than the one
			 * ahead: then he is the one ahead on the road. */
			next = CG_RaceSplitAt( ahead, j - 1 );
			if ( i == 0 && next && now - next->time > gap ) gap = now - next->time;
			break;
		}
	}
	/* before his first checkpoint: since the one ahead crossed his first */
	if ( !found && behind->count == 0 && ahead->count > 0 ) {
		gap = now - CG_RaceSplitAt( ahead, ahead->count - 1 )->time;
		found = 1;
	}
	/* the one ahead is home: at least since then */
	if ( aheadFinish && ( !found || now - aheadFinish > gap ) ) {
		gap = now - aheadFinish;
		found = 1;
	}
	if ( found ) *gapMs = gap;
	return found;
}
