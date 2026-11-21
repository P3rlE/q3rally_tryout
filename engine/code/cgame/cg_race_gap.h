/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The gaps in the race order. The server sends every checkpoint a driver
crosses ("raceSplit client lap checkpoint time", server time); the gap of a
driver to the one ahead is taken at the last checkpoint both have crossed,
and grows while the one ahead has already crossed the next one. Kept free
of the rest of cgame so tests/test_race_gap.py can run it.
*/

#ifndef CG_RACE_GAP_H
#define CG_RACE_GAP_H

#define CG_RACE_SPLIT_HISTORY 32

typedef struct {
	int lap;
	int checkpoint;
	int time;
} cgRaceSplit_t;

typedef struct {
	cgRaceSplit_t splits[CG_RACE_SPLIT_HISTORY];
	int next;
	int count;
} cgRaceSplitHistory_t;

/* the split crossed `age` splits ago (0: the latest) */
const cgRaceSplit_t *CG_RaceSplitAt( const cgRaceSplitHistory_t *history, int age );

/* keeps a crossing; the same lap and checkpoint again replaces the time */
void CG_RaceSplitAdd( cgRaceSplitHistory_t *history, int lap, int checkpoint, int time );

/* how far `behind` is behind `ahead` at server time `now`, in ms. A finish
 * time of 0: still racing. Returns 0 when there is nothing to measure yet;
 * the gap can be negative when `behind` was ahead at the last checkpoint. */
int CG_RaceGap( const cgRaceSplitHistory_t *ahead, int aheadFinish,
                const cgRaceSplitHistory_t *behind, int behindFinish, int now, int *gapMs );

#endif
