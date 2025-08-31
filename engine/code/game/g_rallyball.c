/*
=============================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2025 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
=============================================================================
*/

#include "g_local.h"

// forward declarations for functions from other modules
void InitTrigger( gentity_t *self );
void LogExit( const char *string );

static void G_RallyBall_SpawnBall( void );
static qboolean G_RallyBall_ScoresTied( void );

static qboolean rbRoundActive;
static int      rbRoundStartTime;
static int      rbBallRespawnTime;
static int      rbLastCountdown;

/*
=================
G_RallyBall_Init

Called when the level is initialized for the rallyball gametype.
=================
*/
void G_RallyBall_Init( void ) {
    // default spawn origin at world origin; maps can override via rallyball_spawn entity
    VectorClear( level.rallyballSpawn );
    level.rallyball = NULL;
    rbRoundActive = qfalse;
    rbLastCountdown = -1;
    rbRoundStartTime = level.time + 5000;
    rbBallRespawnTime = rbRoundStartTime;

    if ( g_timelimit.integer ) {
        level.rallyballEndTime = level.time + g_timelimit.integer * 60000;
    } else {
        level.rallyballEndTime = 0;
    }
}

/*
=================
G_RallyBall_SpawnBall

Spawns a new ball at the stored spawn origin.
=================
*/
static void G_RallyBall_SpawnBall( void ) {
    if ( level.rallyball ) {
        G_FreeEntity( level.rallyball );
        level.rallyball = NULL;
    }
    level.rallyball = G_SpawnRallyBall( level.rallyballSpawn );
}

static qboolean G_RallyBall_ScoresTied( void ) {
    int i;
    int top = -1;
    int second = -1;

    for ( i = TEAM_RED; i <= TEAM_YELLOW; i++ ) {
        if ( level.teamScores[i] > top ) {
            second = top;
            top = level.teamScores[i];
        } else if ( level.teamScores[i] > second ) {
            second = level.teamScores[i];
        }
    }

    return top == second;
}

/*
=================
G_RallyBall_RunFrame

Per-frame updates for rallyball.
=================
*/
void G_RallyBall_RunFrame( void ) {
    if ( !rbRoundActive ) {
        int secs = ( rbRoundStartTime - level.time + 999 ) / 1000;
        if ( secs > 0 ) {
            if ( secs != rbLastCountdown ) {
                trap_SendServerCommand( -1, va( "cp \"%i\"", secs ) );
                rbLastCountdown = secs;
            }
            return;
        }
        rbRoundActive = qtrue;
        trap_SendServerCommand( -1, "cp \"GO!\"" );
    }

    if ( !level.rallyball && level.time >= rbBallRespawnTime ) {
        G_RallyBall_SpawnBall();
    }

    if ( level.rallyballEndTime && level.time >= level.rallyballEndTime ) {
        if ( G_RallyBall_ScoresTied() ) {
            level.rallyballEndTime = level.time + 60000;
            trap_SendServerCommand( -1, "cp \"Overtime!\"" );
        } else {
            LogExit( "Rallyball time limit hit." );
        }
    }
}

/*
=================
G_RallyBall_GoalTouch

Called when the rallyball touches a goal trigger.
=================
*/
static void G_RallyBall_GoalTouch( gentity_t *self, gentity_t *other, trace_t *trace ) {
    int team = TEAM_FREE;

    if ( other->s.eType != ET_RALLYBALL ) {
        return;
    }

    if ( self->spawnflags & 1 ) {
        team = TEAM_RED;
    } else if ( self->spawnflags & 2 ) {
        team = TEAM_BLUE;
    } else if ( self->spawnflags & 4 ) {
        team = TEAM_GREEN;
    } else if ( self->spawnflags & 8 ) {
        team = TEAM_YELLOW;
    }

    if ( team != TEAM_FREE ) {
        level.teamScores[ team ]++;
        switch ( team ) {
            case TEAM_RED:
                trap_SetConfigstring( CS_SCORES1, va("%i", level.teamScores[TEAM_RED]) );
                break;
            case TEAM_BLUE:
                trap_SetConfigstring( CS_SCORES2, va("%i", level.teamScores[TEAM_BLUE]) );
                break;
            case TEAM_GREEN:
                trap_SetConfigstring( CS_SCORES3, va("%i", level.teamScores[TEAM_GREEN]) );
                break;
            case TEAM_YELLOW:
                trap_SetConfigstring( CS_SCORES4, va("%i", level.teamScores[TEAM_YELLOW]) );
                break;
        }

        {
            gentity_t *te = G_TempEntity(other->s.pos.trBase, EV_GLOBAL_TEAM_SOUND);
            te->r.svFlags |= SVF_BROADCAST;
            te->s.otherEntityNum = team;
            if (team == TEAM_RED || team == TEAM_GREEN) {
                te->s.eventParm = GTS_REDTEAM_SCORED;
            } else {
                te->s.eventParm = GTS_BLUETEAM_SCORED;
            }
        }
    }

    // remove old ball and spawn a new one
    if ( other == level.rallyball ) {
        level.rallyball = NULL;
    }
    G_FreeEntity( other );
    rbBallRespawnTime = level.time + 3000;
    CalculateRanks();
}

/*QUAKED trigger_rallyball_goal (.5 .5 .5) ? RED_GOAL BLUE_GOAL GREEN_GOAL YELLOW_GOAL
A trigger volume that scores for the specified team when the rallyball touches it.
*/
void SP_trigger_rallyball_goal( gentity_t *ent ) {
    ent->touch = G_RallyBall_GoalTouch;
    InitTrigger( ent );
    trap_LinkEntity( ent );
}

/*QUAKED rallyball_spawn (0 0.5 0.5) (-8 -8 -8) (8 8 8)
Defines the spawn location for the rallyball.
*/
void SP_rallyball_spawn( gentity_t *ent ) {
    VectorCopy( ent->s.origin, level.rallyballSpawn );
    G_FreeEntity( ent );
}
