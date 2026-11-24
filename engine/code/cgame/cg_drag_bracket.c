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

/* cg_drag_bracket.c -- the drag tournament's bracket */
#include "cg_drag_bracket.h"

int CG_BracketMatch( const cgDragBracket_t *b, int round, int pair ) {
	return round == b->rounds - 1 ? b->size - 1 : b->start[round] + pair;
}

void CG_BracketBuild( cgDragBracket_t *b, int size, int played, const int *winner ) {
	int r, j, m, semi;
	if ( size > CG_BRACKET_MAX ) size = CG_BRACKET_MAX;
	b->size = size;
	b->played = played;
	for ( b->rounds = 0; ( 1 << b->rounds ) < size; b->rounds++ ) {
	}
	for ( r = 0; r < CG_BRACKET_ROUNDS; r++ ) {
		b->start[r] = 0;
		for ( j = 0; j < CG_BRACKET_MAX; j++ ) b->slot[r][j] = -1;
	}
	for ( j = 0; j < CG_BRACKET_MAX; j++ ) b->winner[j] = j < played && winner ? ( winner[j] ? 1 : 0 ) : 0;
	for ( j = 0; j < size; j++ ) b->slot[0][j] = j;
	for ( r = 1; r <= b->rounds && r < CG_BRACKET_ROUNDS; r++ ) {
		b->start[r] = b->start[r - 1] + ( size >> r );
		for ( j = 0; j < size >> r; j++ ) {
			m = CG_BracketMatch( b, r - 1, j );
			b->slot[r][j] = m < played && b->slot[r - 1][2 * j] >= 0 && b->slot[r - 1][2 * j + 1] >= 0
				? b->slot[r - 1][2 * j + b->winner[m]] : -1;
		}
	}
	b->third[0] = b->third[1] = -1;
	if ( b->rounds < 2 ) return;
	for ( j = 0; j < 2; j++ ) {
		semi = size - 4 + j;
		b->third[j] = semi < played && b->slot[b->rounds - 2][2 * j] >= 0
			? b->slot[b->rounds - 2][2 * j + !b->winner[semi]] : -1;
	}
}

int CG_BracketRound( const cgDragBracket_t *b, int match ) {
	int r;
	if ( match >= b->size - 2 ) return b->rounds - 1;
	for ( r = 0; r < b->rounds - 1 && match >= b->start[r] + ( b->size >> ( r + 1 ) ); r++ ) {
	}
	return r;
}

void CG_BracketEntrants( const cgDragBracket_t *b, int match, int *a, int *c ) {
	int r, pair;
	*a = *c = -1;
	if ( match < 0 || match >= b->size ) return;
	if ( match == b->size - 2 && b->rounds >= 2 ) {
		*a = b->third[0];
		*c = b->third[1];
		return;
	}
	r = CG_BracketRound( b, match );
	pair = match == b->size - 1 ? 0 : match - b->start[r];
	*a = b->slot[r][2 * pair];
	*c = b->slot[r][2 * pair + 1];
}

const char *CG_BracketMatchName( const cgDragBracket_t *b, int match ) {
	int left;
	if ( match >= b->size - 1 ) return "FINAL";
	if ( match == b->size - 2 ) return "THIRD PLACE";
	left = b->size >> CG_BracketRound( b, match );
	if ( left == 4 ) return "SEMI-FINAL";
	if ( left == 8 ) return "QUARTER-FINAL";
	return "ROUND OF 16";
}

const char *CG_BracketMatchShort( const cgDragBracket_t *b, int match ) {
	int left;
	if ( match >= b->size - 1 ) return "F";
	if ( match == b->size - 2 ) return "3RD";
	left = b->size >> CG_BracketRound( b, match );
	if ( left == 4 ) return "SF";
	if ( left == 8 ) return "QF";
	return "R16";
}

const char *CG_BracketRoundHead( const cgDragBracket_t *b, int round ) {
	int left = b->size >> round;
	if ( left <= 2 ) return "FINAL";
	if ( left == 4 ) return "SEMI-FINALS";
	if ( left == 8 ) return "QUARTER-FINALS";
	return "ROUND OF 16";
}

int CG_BracketPath( const cgDragBracket_t *b, int entrant, int *matches, int max ) {
	int r, j, m, n = 0, a, c;
	if ( entrant < 0 || entrant >= b->size ) return 0;
	for ( r = 0; r < b->rounds && n < max; r++ ) {
		for ( j = 0; j < b->size >> r && b->slot[r][j] != entrant; j++ ) {
		}
		if ( j >= b->size >> r ) break;
		m = CG_BracketMatch( b, r, j / 2 );
		matches[n++] = m;
		if ( m >= b->played ) return n;	/* the one it waits for */
		if ( b->slot[r][2 * ( j / 2 ) + b->winner[m]] == entrant ) continue;
		/* out: a semi-final lost goes on to third place */
		if ( r == b->rounds - 2 && n < max ) {
			CG_BracketEntrants( b, b->size - 2, &a, &c );
			if ( a == entrant || c == entrant ) matches[n++] = b->size - 2;
		}
		return n;
	}
	return n;
}

int CG_BracketPlace( const cgDragBracket_t *b, int entrant ) {
	int path[CG_BRACKET_ROUNDS + 1], n, last, a, c, won;
	if ( b->played < b->size || entrant < 0 || entrant >= b->size ) return 0;
	n = CG_BracketPath( b, entrant, path, CG_BRACKET_ROUNDS + 1 );
	if ( n <= 0 ) return 0;
	last = path[n - 1];
	CG_BracketEntrants( b, last, &a, &c );
	won = ( b->winner[last] ? c : a ) == entrant;
	if ( last == b->size - 1 ) return won ? 1 : 2;
	if ( last == b->size - 2 ) return won ? 3 : 4;
	return ( b->size >> ( CG_BracketRound( b, last ) + 1 ) ) + 1;
}
