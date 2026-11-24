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
The bracket of a drag tournament from what the server sends (dragTourney:
the size, the winner of each match played so far, in play order). Matches
come in play order: the rounds up to the semi-finals, then third place
(size - 2), then the final (size - 1). Pure C, tested by
tests/test_drag_bracket.py; drawn by the classic bracket and the v0.9 one.
*/

#ifndef CG_DRAG_BRACKET_H
#define CG_DRAG_BRACKET_H

#define CG_BRACKET_MAX		16
#define CG_BRACKET_ROUNDS	5

typedef struct {
	int		size, rounds, played;
	int		start[CG_BRACKET_ROUNDS];				/* first match of each round */
	int		slot[CG_BRACKET_ROUNDS][CG_BRACKET_MAX];	/* entrant in each slot, -1 not yet known */
	int		third[2];								/* the semi-final losers */
	int		winner[CG_BRACKET_MAX];					/* 0 or 1: which of the two won each match */
} cgDragBracket_t;

void		CG_BracketBuild( cgDragBracket_t *b, int size, int played, const int *winner );
/* the match of a round and pair; the last round is the final */
int			CG_BracketMatch( const cgDragBracket_t *b, int round, int pair );
/* the round a match belongs to; third place counts with the final */
int			CG_BracketRound( const cgDragBracket_t *b, int match );
/* the two entrants of a match (-1: not known yet) */
void		CG_BracketEntrants( const cgDragBracket_t *b, int match, int *a, int *c );
/* "FINAL", "THIRD PLACE", "SEMI-FINAL", "QUARTER-FINAL", "ROUND OF 16" */
const char	*CG_BracketMatchName( const cgDragBracket_t *b, int match );
/* "F", "3RD", "SF", "QF", "R16" */
const char	*CG_BracketMatchShort( const cgDragBracket_t *b, int match );
/* a column's head: "ROUND OF 16", "QUARTER-FINALS", "SEMI-FINALS", "FINAL" */
const char	*CG_BracketRoundHead( const cgDragBracket_t *b, int round );
/* the matches an entrant is in, in play order: those played, then the one it
 * waits for; a semi-final lost goes on to third place */
int			CG_BracketPath( const cgDragBracket_t *b, int entrant, int *matches, int max );
/* where an entrant ended once the final is played (1..size), 0 before */
int			CG_BracketPlace( const cgDragBracket_t *b, int entrant );

#endif
