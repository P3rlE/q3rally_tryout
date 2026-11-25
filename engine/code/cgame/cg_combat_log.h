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
A combat match for the end screen (Deathmatch, Team Deathmatch, Derby, Last
Car Standing). The server sends the scores (score, damage) and every kill
as an obituary, and keeps no history for the client; the client writes the
match down as it goes: who killed whom and when, the streaks, who went out
when, and at the end of every minute each driver's score and damage and
the teams' scores. Pure C, tested by tests/test_combat_log.py.
*/

#ifndef CG_COMBAT_LOG_H
#define CG_COMBAT_LOG_H

#define CG_COMBAT_CLIENTS	64
#define CG_COMBAT_MINUTES	30		/* longer matches: the last minute keeps counting */
#define CG_COMBAT_TEAMS		4
#define CG_COMBAT_NONE		-99999	/* a minute a driver was not there */

typedef struct {
	int		start;									/* the match's start (cg.time) */
	int		kills[CG_COMBAT_CLIENTS], deaths[CG_COMBAT_CLIENTS];
	int		streak[CG_COMBAT_CLIENTS], bestStreak[CG_COMBAT_CLIENTS];
	unsigned char killedBy[CG_COMBAT_CLIENTS][CG_COMBAT_CLIENTS];	/* [victim][attacker] */
	int		outAt[CG_COMBAT_CLIENTS];				/* the last death, ms into the match; 0: never */
	int		outBy[CG_COMBAT_CLIENTS];				/* who did it, -1: the world or themselves */
	int		minuteKills[CG_COMBAT_MINUTES][CG_COMBAT_CLIENTS];
	/* at the end of each minute: the last scores seen in it */
	int		minutes;								/* the minutes seen */
	int		score[CG_COMBAT_MINUTES][CG_COMBAT_CLIENTS];	/* CG_COMBAT_NONE: not there */
	int		damage[CG_COMBAT_MINUTES][CG_COMBAT_CLIENTS];
	int		team[CG_COMBAT_MINUTES][CG_COMBAT_TEAMS];
} cgCombatLog_t;

void CG_CombatReset( cgCombatLog_t *log, int start );
/* a kill at time (cg.time); attacker -1 or the target itself: no kill counted */
void CG_CombatKill( cgCombatLog_t *log, int attacker, int target, int time );
/* the scores at time: score and damage of each client (score CG_COMBAT_NONE: not
 * there) and the teams' scores */
void CG_CombatSample( cgCombatLog_t *log, int time, const int *score, const int *damage, const int *team );

int  CG_CombatMinute( const cgCombatLog_t *log, int time );	/* 0 .. CG_COMBAT_MINUTES - 1 */
/* who killed this one most, -1: nobody; *times how often */
int  CG_CombatNemesis( const cgCombatLog_t *log, int victim, int *times );
/* the place at the end of a minute by score, 0: not there */
int  CG_CombatPlaceAt( const cgCombatLog_t *log, int client, int minute );
/* how many were still in at the end of a minute: there and not out by then */
int  CG_CombatLeftAt( const cgCombatLog_t *log, int minute );

#endif
