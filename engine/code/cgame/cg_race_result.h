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
The race result for the end screen. The client collects what the server
already sends everybody: the start ("raceTime"), every lap ("newLapTime"),
every checkpoint ("raceSplit"), the finish ("raceFinishTime") and the
positions. From that: each driver's laps and best sector, the position at
the start, the classification and the gaps. Pure C, tested by
tests/test_race_result.py.
*/

#ifndef CG_RACE_RESULT_H
#define CG_RACE_RESULT_H

#define CG_RESULT_LAPS	64
#define CG_RESULT_CPS	32		/* checkpoints a sprint keeps, by number (1 ..) */

typedef struct {
	int		present;			/* took part in this race */
	int		startTime;			/* server time of the start */
	int		lapStart;			/* when the lap being driven began */
	int		lastCross;			/* the last checkpoint or lap line, for the sectors */
	int		laps[CG_RESULT_LAPS];
	int		lapCount;
	int		finishTime;			/* server time, 0: not finished */
	int		grid;				/* position at the start, 0: unknown */
	int		position;			/* the server's position, for those not finished */
	int		bestSector;			/* ms, 0: none */
	int		bestSectorNum;		/* the checkpoint it ended at (S1, S2 ..) */
	int		cpAt[CG_RESULT_CPS];	/* ms from the start to each checkpoint (the last pass), 0: not there */
} cgRaceResult_t;

void CG_ResultStart( cgRaceResult_t *r, int startTime );
void CG_ResultSplit( cgRaceResult_t *r, int checkpoint, int time );
void CG_ResultLap( cgRaceResult_t *r, int time );			/* a lap completed at time */
void CG_ResultFinish( cgRaceResult_t *r, int time );		/* the last lap, then done */

int  CG_ResultBestLap( const cgRaceResult_t *r );			/* 0: no lap */
int  CG_ResultTotal( const cgRaceResult_t *r );				/* finish - start, 0: not finished */

/* the classification: the finishers by time, then the others by laps done,
 * then by the server's position. Fills order[] with indexes into d[] of
 * those present; returns how many. */
int  CG_ResultOrder( const cgRaceResult_t *d, int count, int *order );

/* a sprint, one run from A to B: its sectors run from checkpoint to
 * checkpoint, the last one to the finish (sector checkpoints + 1). */
int  CG_ResultCheckpoints( const cgRaceResult_t *d, int count );	/* the highest anybody passed */
int  CG_ResultReached( const cgRaceResult_t *r );					/* the highest this one passed */
int  CG_ResultSector( const cgRaceResult_t *r, int sector, int checkpoints );	/* ms, 0: not driven */
/* the place at the end of a sector among those who got there, 0: not there */
int  CG_ResultPlaceAt( const cgRaceResult_t *d, int count, int who, int sector, int checkpoints );

/* "4:20.238" */
void CG_ResultTime( int ms, char *out, int size );
/* the time column: the winner's total, "+1.142" for the others who
 * finished, "DNF" for those who did not */
void CG_ResultGap( const cgRaceResult_t *winner, const cgRaceResult_t *d, char *out, int size );
/* a difference in time, signed: "+2.070" "-6.140" "+1:02.100" */
void CG_ResultDiff( int ms, char *out, int size );

#endif
