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

#include "cg_race_result.h"

static void CG_ResultCopy( char *out, int size, const char *text ) {
	int i;
	if ( size <= 0 ) return;
	for ( i = 0; i < size - 1 && text[i]; i++ ) out[i] = text[i];
	out[i] = '\0';
}

/* digits into out, right-aligned to width with zeros; returns the end */
static char *CG_ResultDigits( char *out, int value, int width ) {
	char tmp[12];
	int n = 0;
	do {
		tmp[n++] = (char)( '0' + value % 10 );
		value /= 10;
	} while ( value > 0 && n < 11 );
	while ( n < width && n < 11 ) tmp[n++] = '0';
	while ( n > 0 ) *out++ = tmp[--n];
	return out;
}

void CG_ResultStart( cgRaceResult_t *r, int startTime ) {
	int grid = r->grid;
	int i;
	char *p = (char *)r;
	for ( i = 0; i < (int)sizeof( *r ); i++ ) p[i] = 0;
	r->present = 1;
	r->startTime = r->lapStart = r->lastCross = startTime;
	r->grid = grid;
}

void CG_ResultSplit( cgRaceResult_t *r, int checkpoint, int time ) {
	int sector;
	if ( !r->present || r->finishTime || time <= r->lastCross ) return;
	sector = time - r->lastCross;
	if ( !r->bestSector || sector < r->bestSector ) {
		r->bestSector = sector;
		r->bestSectorNum = checkpoint;
	}
	r->lastCross = time;
}

void CG_ResultLap( cgRaceResult_t *r, int time ) {
	if ( !r->present || r->finishTime || time <= r->lapStart ) return;
	if ( r->lapCount < CG_RESULT_LAPS ) r->laps[r->lapCount] = time - r->lapStart;
	r->lapCount++;
	r->lapStart = time;
	if ( time > r->lastCross ) r->lastCross = time;
}

void CG_ResultFinish( cgRaceResult_t *r, int time ) {
	if ( !r->present || r->finishTime ) return;
	CG_ResultLap( r, time );
	r->finishTime = time;
}

int CG_ResultBestLap( const cgRaceResult_t *r ) {
	int i, best = 0, n = r->lapCount < CG_RESULT_LAPS ? r->lapCount : CG_RESULT_LAPS;
	for ( i = 0; i < n; i++ ) {
		if ( r->laps[i] > 0 && ( !best || r->laps[i] < best ) ) best = r->laps[i];
	}
	return best;
}

int CG_ResultTotal( const cgRaceResult_t *r ) {
	return r->finishTime ? r->finishTime - r->startTime : 0;
}

/* a before b in the classification */
static int CG_ResultAhead( const cgRaceResult_t *a, const cgRaceResult_t *b ) {
	if ( a->finishTime && b->finishTime ) return a->finishTime < b->finishTime;
	if ( a->finishTime != 0 ) return 1;
	if ( b->finishTime != 0 ) return 0;
	if ( a->lapCount != b->lapCount ) return a->lapCount > b->lapCount;
	if ( a->position && b->position ) return a->position < b->position;
	return a->position != 0;
}

int CG_ResultOrder( const cgRaceResult_t *d, int count, int *order ) {
	int n = 0, i, k, swap;
	for ( i = 0; i < count; i++ ) {
		if ( d[i].present ) order[n++] = i;
	}
	/* insertion sort: stable, the lists are short */
	for ( i = 1; i < n; i++ ) {
		for ( k = i; k > 0 && CG_ResultAhead( &d[order[k]], &d[order[k - 1]] ); k-- ) {
			swap = order[k];
			order[k] = order[k - 1];
			order[k - 1] = swap;
		}
	}
	return n;
}

void CG_ResultTime( int ms, char *out, int size ) {
	char buf[24], *p = buf;
	if ( ms < 0 ) ms = 0;
	p = CG_ResultDigits( p, ms / 60000, 1 );
	*p++ = ':';
	p = CG_ResultDigits( p, ( ms / 1000 ) % 60, 2 );
	*p++ = '.';
	p = CG_ResultDigits( p, ms % 1000, 3 );
	*p = '\0';
	CG_ResultCopy( out, size, buf );
}

void CG_ResultGap( const cgRaceResult_t *winner, const cgRaceResult_t *d, char *out, int size ) {
	int gap;
	if ( !d->finishTime || !winner->finishTime ) {
		CG_ResultCopy( out, size, "DNF" );
		return;
	}
	if ( d == winner ) {
		CG_ResultTime( CG_ResultTotal( d ), out, size );
		return;
	}
	gap = d->finishTime - winner->finishTime;
	CG_ResultDiff( gap < 0 ? 0 : gap, out, size );
}

void CG_ResultDiff( int ms, char *out, int size ) {
	char buf[24], *p = buf;
	*p++ = ms < 0 ? '-' : '+';
	if ( ms < 0 ) ms = -ms;
	if ( ms >= 60000 ) {
		CG_ResultTime( ms, p, (int)sizeof( buf ) - 1 );
	} else {
		p = CG_ResultDigits( p, ms / 1000, 1 );
		*p++ = '.';
		p = CG_ResultDigits( p, ms % 1000, 3 );
		*p = '\0';
	}
	CG_ResultCopy( out, size, buf );
}
