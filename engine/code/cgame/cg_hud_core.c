/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

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
===========================================================================
*/

/*
===========================================================================
  cg_hud_core.c

  HUD toggles and the per-frame HUD hook:
    - cvar registration for the HUD element toggles
    - the toggle table the HUD options panel (Shift+H) shows
    - CG_DrawHUD(): the standings and the race state, each frame
===========================================================================
*/

#include "cg_local.h"
#include "../client/keycodes.h"
#include "cg_hud_elements.h"

/* -----------------------------------------------------------------------
   Shared colour palette (also used by racing / derby / vehicle modules)
   ----------------------------------------------------------------------- */
float colors[4][4] = {
    { 1.0f, 0.69f, 0.0f, 1.0f },   /* normal            */
    { 1.0f, 0.2f,  0.2f, 1.0f },   /* low health        */
    { 0.5f, 0.5f,  0.5f, 1.0f },   /* weapon firing     */
    { 1.0f, 1.0f,  1.0f, 1.0f }    /* health > 100      */
};

/* -----------------------------------------------------------------------
   HUD element toggle cvars  (definitions – declared extern in header)
   ----------------------------------------------------------------------- */
vmCvar_t  cg_hudShowTimes;
vmCvar_t  cg_hudShowLaps;
vmCvar_t  cg_hudShowPosition;
vmCvar_t  cg_hudShowOpponentList;
vmCvar_t  cg_hudShowSlipstream;

vmCvar_t  cg_hudShowDerbyList;

/* Menu open/close toggle – registered in CG_HUD_RegisterCvars, updated each frame */
static vmCvar_t cg_hudOptionsOpen;

/* -----------------------------------------------------------------------
   CG_HUD_RegisterCvars
   Call once from CG_RegisterCvars() in cg_main.c
   ----------------------------------------------------------------------- */
void CG_HUD_RegisterCvars( void ) {
    /* Menu toggle – bind F9 "toggle cg_hudOptionsOpen" */
    trap_Cvar_Register( &cg_hudOptionsOpen,        "cg_hudOptionsOpen",        "0", CVAR_ARCHIVE );

    /* Racing */
    trap_Cvar_Register( &cg_hudShowTimes,          "cg_hudShowTimes",          "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowLaps,           "cg_hudShowLaps",           "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowPosition,       "cg_hudShowPosition",       "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowOpponentList,   "cg_hudShowOpponentList",   "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowSlipstream,     "cg_hudShowSlipstream",     "1", CVAR_ARCHIVE );

    /* Derby */
    trap_Cvar_Register( &cg_hudShowDerbyList,      "cg_hudShowDerbyList",      "1", CVAR_ARCHIVE );
}

/* =======================================================================
   HUD OPTIONS MENU
   Shift+H (or "toggle cg_hudOptionsOpen") in a race. The panel is
   cg_hud_panel.c, in the look of the WebUI's #hud-screen; the toggles it
   shows are this table.
   ======================================================================= */

#define HUDOPT_MODE_ANY                 -1
#define HUDOPT_MODE_RALLY_RACE          -3
#define HUDOPT_MODE_RACE_OR_LCS         -4
#define HUDOPT_MODE_RACE_LCS_DERBY      -5
#define HUDOPT_MODE_RACE_WITH_LAPS      -6

typedef struct {
    const char  *label;
    const char  *cvarName;
    vmCvar_t    *cvar;
    int          onValue;       /* value to set when toggling ON; also max cycle value */
    int          gameTypeOnly;  /* exact GT_* value or negative mode-group selector     */
    qboolean     isCycler;      /* qtrue: cycle 0..onValue instead of simple toggle    */
    const char *const *cycleLabels; /* state badge text when cycling                   */
} hudToggleEntry_t;

static const char *const hudGhostPlaybackLabels[] = {
    "OFF", "PERSONAL", "SERVER BASE", "LADDER", NULL
};

static const char *const hudCheckpointArrowLabels[] = {
    "OFF", "ON HUD", "ABOVE CAR", NULL
};

static const hudToggleEntry_t hudToggleTable[] = {
    /* ---- race / match ---- */
    { "TIMES PANEL",         "cg_hudShowTimes",          &cg_hudShowTimes,          1, HUDOPT_MODE_RACE_LCS_DERBY,      qfalse, NULL },
    { "LAP COUNTER",         "cg_hudShowLaps",           &cg_hudShowLaps,           1, HUDOPT_MODE_RACE_WITH_LAPS,      qfalse, NULL },
    { "RACE POSITION",       "cg_hudShowPosition",       &cg_hudShowPosition,       1, HUDOPT_MODE_RALLY_RACE,           qfalse, NULL },
    { "GHOST PLAYBACK",      "cg_ghostPlayback",         &cg_ghostPlayback,         3, GT_GHOST,                         qtrue,  hudGhostPlaybackLabels },
    { "CHECKPOINT ARROW",    "cg_checkpointArrowMode",   &cg_checkpointArrowMode,   2, HUDOPT_MODE_RALLY_RACE,           qtrue,  hudCheckpointArrowLabels },
    { "RACING LINE",         "cg_racingLine",            &cg_racingLine,            1, HUDOPT_MODE_RALLY_RACE,           qfalse, NULL },
    { "OPPONENT LIST",       "cg_hudShowOpponentList",   &cg_hudShowOpponentList,   1, HUDOPT_MODE_RACE_OR_LCS,          qfalse, NULL },
    /* ---- Derby ---- */
    { "DERBY SCOREBOARD",    "cg_hudShowDerbyList",      &cg_hudShowDerbyList,      1, GT_DERBY, qfalse, NULL },
    { "DERBY HIT IMPACT",    "cg_derbyHitFxEnable",      &cg_derbyHitFxEnable,      1, GT_DERBY, qfalse, NULL },
    /* ---- car ---- */
    { "STATUS HUD",          "cg_drawStatus",            &cg_drawStatus,            1, HUDOPT_MODE_ANY, qfalse, NULL },
    { "REAR-VIEW MIRROR",    "cg_drawRearView",          &cg_drawRearView,          1, HUDOPT_MODE_ANY, qfalse, NULL },
    { "MINI-MAP",            "cg_drawMMap",              &cg_drawMMap,              1, HUDOPT_MODE_ANY, qfalse, NULL },
    { "SLIPSTREAM GAUGE",    "cg_hudShowSlipstream",     &cg_hudShowSlipstream,     1, HUDOPT_MODE_RALLY_RACE, qfalse, NULL },
};

#define HUDOPT_NUM_ENTRIES  ( (int)( sizeof(hudToggleTable) / sizeof(hudToggleTable[0]) ) )

static void HUDOpt_SetFloatCvar( const char *cvarName, vmCvar_t *cvar, float value, qboolean wholeNumber ) {
    if ( wholeNumber ) {
        trap_Cvar_Set( cvarName, va( "%.0f", value ) );
    } else {
        trap_Cvar_Set( cvarName, va( "%.2f", value ) );
    }
    trap_Cvar_Update( cvar );
}

/*
================
HUDEntry_IsUnavail
Returns qtrue when a toggle entry is not applicable in the current gametype.
  -1 is always available; -3 is course-race guidance/position, -4 opponent
  list, -5 race/LCS/Derby times, -6 race with laps. Drag has its own HUD;
  only times and an optional two-driver opponent list apply there.
  Non-negative values are exact GT_* matches.
================
*/
static qboolean HUDEntry_IsUnavail( const hudToggleEntry_t *e ) {
    if ( e->gameTypeOnly == HUDOPT_MODE_ANY )
        return qfalse;
    if ( e->gameTypeOnly == HUDOPT_MODE_RALLY_RACE )
        return ( cgs.gametype == GT_DRAG || !isRallyRace() );
    if ( e->gameTypeOnly == HUDOPT_MODE_RACE_OR_LCS )
        return ( cgs.gametype == GT_DRAG && cgs.numRacers < 2 ) ||
               ( !isRallyRace() && cgs.gametype != GT_LCS );
    if ( e->gameTypeOnly == HUDOPT_MODE_RACE_LCS_DERBY )
        return ( !isRallyRace() && cgs.gametype != GT_LCS && cgs.gametype != GT_DERBY );
    if ( e->gameTypeOnly == HUDOPT_MODE_RACE_WITH_LAPS )
        return ( !isRallyRace() || cgs.gametype == GT_ELIMINATION || cgs.gametype == GT_DRAG );
    return ( cgs.gametype != e->gameTypeOnly );
}

/*
================
HUDEntry_DoBadgeLabel
Returns the badge string for a given entry, reflecting cycler states.
================
*/
/*
================
CG_HUDOptionsIsOpen
Safe accessor for cg_main.c (cg_hudOptionsOpen is static here).
================
*/
qboolean CG_HUDOptionsIsOpen( void ) {
    return cg_hudOptionsOpen.integer ? qtrue : qfalse;
}

/*
================
CG_HUDOptions_KeyEvent / CG_HUDOptions_MouseEvent / CG_DrawHUDOptionsMenu
The panel itself is cg_hud_panel.c, in the look of the WebUI's #hud-screen.
================
*/
void CG_HUDOptions_KeyEvent( int key ) {
    if ( !cg_hudOptionsOpen.integer ) return;
    CG_HUDPanel_Key( key );
}

qboolean CG_HUDOptions_MouseEvent( int cx, int cy, qboolean clicked ) {
    ( void )cx; ( void )cy;
    if ( !cg_hudOptionsOpen.integer ) return qfalse;
    if ( clicked ) CG_HUDPanel_Key( K_MOUSE1 );
    return qtrue;   /* the menu takes all mouse input while it is open */
}

void CG_DrawHUDOptionsMenu( void ) {
    trap_Cvar_Update( &cg_hudOptionsOpen );
    if ( !cg_hudOptionsOpen.integer ) return;
    CG_HUDPanel_Draw();
}


/* =======================================================================
   MAIN HUD ENTRY POINT
   ======================================================================= */

/* -----------------------------------------------------------------------
   Shared in-game surface styling.  The frontend uses quiet dark surfaces,
   a thin steel-blue signal colour, and a very restrained amount of text.  Keep
   the same language here so the HUD feels like part of the same product.
   ----------------------------------------------------------------------- */

/*
================================
CG_DrawHUD
Main HUD dispatcher, called each frame from CG_DrawActive().
================================
*/
qboolean CG_DrawHUD( void ) {
    int i;

    /* drivers in the match (not spectators, not disconnected) */
    cgs.numRacers = 0;
    for ( i = 0; i < cgs.maxclients; i++ ) {
        if ( !cgs.clientinfo[i].infoValid )              continue;
        if ( cgs.clientinfo[i].team == TEAM_SPECTATOR )  continue;
        if ( cg.scores[i].ping == -1 )                   continue;
        cgs.numRacers++;
    }

    /* Update all HUD toggle cvars from engine each frame */
    trap_Cvar_Update( &cg_hudOptionsOpen );
    trap_Cvar_Update( &cg_hudShowTimes );
    trap_Cvar_Update( &cg_hudShowLaps );
    trap_Cvar_Update( &cg_hudShowPosition );
    trap_Cvar_Update( &cg_ghostPlayback );
    trap_Cvar_Update( &cg_checkpointArrowMode );

    trap_Cvar_Update( &cg_hudShowOpponentList );
    trap_Cvar_Update( &cg_hudShowSlipstream );
    trap_Cvar_Update( &cg_drawStatus );
    trap_Cvar_Update( &cg_drawRearView );
    trap_Cvar_Update( &cg_drawMMap );
    trap_Cvar_Update( &cg_hudShowDerbyList );

    if ( cg_paused.integer || !cg.showHUD ) {
        return qfalse;
    }

    /* Keep scores fresh for accurate team DM times */
    if ( cg.scoresRequestTime + 2000 < cg.time ) {
        cg.scoresRequestTime = cg.time;
        trap_SendClientCommand( "score" );
    }

    /* the race state the HUD reads: wrong way, the checkpoint arrow
       (cg_checkpointArrowMode 1), the ghost split */
    if ( cgs.gametype == GT_DRAG ) {
        cg.wrongWayStartTime = cg.wrongWayTime = 0;
    } else if ( isRallyRace() &&
                cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR &&
                !CG_IntroCam_IsActive() ) {
        if ( cg_checkpointArrowMode.integer ) {
            CG_DrawArrowToCheckpoint();
        } else {
            cg.wrongWayStartTime = 0;
        }
        CG_UpdateGhostSplitDelta();
    }

    CG_ModernHUD_DrawStandings();

    if ( cgs.gametype == GT_DERBY && cg_derbyHitFxEnable.integer ) {
        CG_DrawHUD_DerbyHitImpact();
    }

    // CG_DrawHUD draws overlays but is not the scoreboard visibility gate.
    return qfalse;
}


int CG_WebHUDNumber( int field, int index ) {
    const hudToggleEntry_t *entry;
    if ( field == 0 ) return HUDOPT_NUM_ENTRIES;
    if ( index < 0 || index >= HUDOPT_NUM_ENTRIES ) return 0;
    entry = &hudToggleTable[index]; trap_Cvar_Update( entry->cvar );
    if ( field == 1 ) return entry->cvar->integer;
    if ( field == 2 ) return entry->onValue + 1;
    if ( field == 3 ) return HUDEntry_IsUnavail( entry );
    return 0;
}
int CG_WebHUDChar( int index, int field, int choice, int character ) {
    const hudToggleEntry_t *entry;
    const char *text = "";
    if ( index < 0 || index >= HUDOPT_NUM_ENTRIES ) return 0;
    entry = &hudToggleTable[index];
    if ( field == 0 ) text = entry->label;
    else if ( field == 1 && choice >= 0 && choice <= entry->onValue ) {
        if ( entry->cycleLabels ) text = entry->cycleLabels[choice];
        else text = choice ? "On" : "Off";
        if ( entry->cvar == &cg_ghostPlayback && cgs.gametype == GT_GHOST && choice == 0 ) text = "Auto";
    }
    if ( !text || character < 0 || character >= (int)strlen( text ) ) return 0;
    return (unsigned char)text[character];
}
void CG_WebHUDAction( int action, int index, int value ) {
    const hudToggleEntry_t *entry;
    trap_Cvar_Update( &cg_hudOptionsOpen );
    if ( !CG_HUDOptionsIsOpen() ) return;
    if ( action == 0 ) {
        if ( index < 0 || index >= HUDOPT_NUM_ENTRIES ) return;
        entry = &hudToggleTable[index];
        if ( HUDEntry_IsUnavail( entry ) || value < 0 || value > entry->onValue ) return;
        trap_Cvar_Set( entry->cvarName, va( "%d", value ) ); trap_Cvar_Update( entry->cvar );
    } else if ( action == 1 && value >= 40 && value <= 200 && value % 5 == 0 ) HUDOpt_SetFloatCvar( "cg_mmap_size", &cg_mmap_size, value / 100.0f, qfalse );
    else if ( action == 2 && value >= 30 && value <= 120 && value % 5 == 0 ) HUDOpt_SetFloatCvar( "cg_mmap_fov", &cg_mmap_fov, value, qtrue );
}
