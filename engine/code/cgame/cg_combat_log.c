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

/* A combat match written down for the end screen; see cg_combat_log.h. */

#include "cg_combat_log.h"

static void CG_CombatClear( int *a, int n, int v ) {
	int i;
	for ( i = 0; i < n; i++ ) a[i] = v;
}

void CG_CombatReset( cgCombatLog_t *log, int start ) {
	unsigned char *p = (unsigned char *)log;
	int i;
	for ( i = 0; i < (int)sizeof( *log ); i++ ) p[i] = 0;
	log->start = start;
	CG_CombatClear( log->outBy, CG_COMBAT_CLIENTS, -1 );
	CG_CombatClear( log->owner, CG_COMBAT_ZONES, -1 );
	log->longestZone = log->longestTeam = -1;
	for ( i = 0; i < CG_COMBAT_MINUTES; i++ ) CG_CombatClear( log->score[i], CG_COMBAT_CLIENTS, CG_COMBAT_NONE );
}

int CG_CombatMinute( const cgCombatLog_t *log, int time ) {
	int m = ( time - log->start ) / 60000;
	if ( m < 0 ) m = 0;
	if ( m >= CG_COMBAT_MINUTES ) m = CG_COMBAT_MINUTES - 1;
	return m;
}

void CG_CombatKill( cgCombatLog_t *log, int attacker, int target, int time ) {
	int at = time - log->start;
	if ( target < 0 || target >= CG_COMBAT_CLIENTS ) return;
	log->deaths[target]++;
	log->streak[target] = 0;
	log->outAt[target] = at > 0 ? at : 1;
	log->outBy[target] = -1;
	if ( attacker < 0 || attacker >= CG_COMBAT_CLIENTS || attacker == target ) return;
	log->outBy[target] = attacker;
	log->kills[attacker]++;
	if ( ++log->streak[attacker] > log->bestStreak[attacker] ) log->bestStreak[attacker] = log->streak[attacker];
	if ( log->killedBy[target][attacker] < 255 ) log->killedBy[target][attacker]++;
	log->minuteKills[CG_CombatMinute( log, time )][attacker]++;
}

void CG_CombatSample( cgCombatLog_t *log, int time, const int *score, const int *damage, const int *team ) {
	int m = CG_CombatMinute( log, time ), i, k;
	/* a minute without scores: what the one before ended with */
	for ( k = log->sampled; k < m; k++ ) {
		for ( i = 0; i < CG_COMBAT_CLIENTS; i++ ) {
			log->score[k][i] = k > 0 ? log->score[k - 1][i] : CG_COMBAT_NONE;
			log->damage[k][i] = k > 0 ? log->damage[k - 1][i] : 0;
		}
		for ( i = 0; i < CG_COMBAT_TEAMS; i++ ) log->team[k][i] = k > 0 ? log->team[k - 1][i] : 0;
	}
	for ( i = 0; i < CG_COMBAT_CLIENTS; i++ ) {
		log->score[m][i] = score[i];
		log->damage[m][i] = damage[i];
	}
	for ( i = 0; i < CG_COMBAT_TEAMS; i++ ) log->team[m][i] = team[i];
	if ( m + 1 > log->sampled ) log->sampled = m + 1;
	if ( m + 1 > log->minutes ) log->minutes = m + 1;
}

int CG_CombatNemesis( const cgCombatLog_t *log, int victim, int *times ) {
	int i, best = -1, most = 0;
	*times = 0;
	if ( victim < 0 || victim >= CG_COMBAT_CLIENTS ) return -1;
	for ( i = 0; i < CG_COMBAT_CLIENTS; i++ ) {
		if ( log->killedBy[victim][i] > most ) {
			most = log->killedBy[victim][i];
			best = i;
		}
	}
	*times = most;
	return best;
}

int CG_CombatPlaceAt( const cgCombatLog_t *log, int client, int minute ) {
	int i, place = 1, s;
	if ( minute < 0 || minute >= log->minutes || client < 0 || client >= CG_COMBAT_CLIENTS ) return 0;
	s = log->score[minute][client];
	if ( s == CG_COMBAT_NONE ) return 0;
	for ( i = 0; i < CG_COMBAT_CLIENTS; i++ ) {
		if ( i != client && log->score[minute][i] != CG_COMBAT_NONE && log->score[minute][i] > s ) place++;
	}
	return place;
}

int CG_CombatLeftAt( const cgCombatLog_t *log, int minute ) {
	int i, left = 0, end = ( minute + 1 ) * 60000;
	if ( minute < 0 || minute >= log->minutes ) return 0;
	for ( i = 0; i < CG_COMBAT_CLIENTS; i++ ) {
		if ( log->score[minute][i] == CG_COMBAT_NONE ) continue;
		if ( !log->outAt[i] || log->outAt[i] > end ) left++;
	}
	return left;
}

/* ms from..to for a team, split at the minutes */
static void CG_CombatHold( cgCombatLog_t *log, int team, int from, int to ) {
	while ( from < to ) {
		int m = CG_CombatMinute( log, from ), end = log->start + ( m + 1 ) * 60000;
		if ( m == CG_COMBAT_MINUTES - 1 || end > to ) end = to;	/* the last minute keeps counting */
		log->held[m][team] += end - from;
		if ( m + 1 > log->minutes ) log->minutes = m + 1;
		from = end;
	}
}

void CG_CombatZones( cgCombatLog_t *log, int time, const int *owner, int zones ) {
	int z;
	if ( zones > CG_COMBAT_ZONES ) zones = CG_COMBAT_ZONES;
	if ( zones > log->zones ) log->zones = zones;
	if ( log->heldTo && time > log->heldTo ) {
		for ( z = 0; z < log->zones; z++ ) {
			if ( log->owner[z] >= 0 ) CG_CombatHold( log, log->owner[z], log->heldTo, time );
		}
	}
	for ( z = 0; z < log->zones; z++ ) {
		int now = z < zones && owner[z] >= 0 && owner[z] < CG_COMBAT_TEAMS ? owner[z] : -1;
		if ( now == log->owner[z] ) continue;
		/* a hold over */
		if ( log->owner[z] >= 0 && time - log->since[z] > log->longestMs ) {
			log->longestMs = time - log->since[z];
			log->longestZone = z;
			log->longestTeam = log->owner[z];
		}
		log->owner[z] = now;
		log->since[z] = time;
	}
	log->heldTo = time;
}

void CG_CombatZonesPause( cgCombatLog_t *log ) {
	log->heldTo = 0;
}

int CG_CombatLongest( const cgCombatLog_t *log, int time, int *zone, int *team ) {
	int z, best = log->longestMs;
	*zone = log->longestZone;
	*team = log->longestTeam;
	for ( z = 0; z < log->zones; z++ ) {
		if ( log->owner[z] >= 0 && time - log->since[z] > best ) {
			best = time - log->since[z];
			*zone = z;
			*team = log->owner[z];
		}
	}
	return *team >= 0 ? best : 0;
}

void CG_CombatGoal( cgCombatLog_t *log, int time, int team, int scorer, int speed ) {
	int i, k = log->goals;
	if ( team < 0 || team >= CG_COMBAT_TEAMS ) return;
	if ( k >= CG_COMBAT_GOALS ) {
		/* the last ones stay */
		for ( i = 0; i < CG_COMBAT_GOALS - 1; i++ ) {
			log->goalAt[i] = log->goalAt[i + 1];
			log->goalTeam[i] = log->goalTeam[i + 1];
			log->goalScorer[i] = log->goalScorer[i + 1];
			log->goalSpeed[i] = log->goalSpeed[i + 1];
		}
		k = CG_COMBAT_GOALS - 1;
	}
	log->goalAt[k] = time - log->start > 0 ? time - log->start : 0;
	log->goalTeam[k] = team;
	log->goalScorer[k] = scorer;
	log->goalSpeed[k] = speed;
	log->goals++;
}
