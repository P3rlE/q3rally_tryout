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

  Central HUD dispatcher:
    - Cvar registration for all HUD element toggles
    - CG_DrawHUD()          : main entry point called each frame
    - CG_DrawUpperRightHUD(): legacy right-side racing panels
    - CG_DrawLowerRightHUD(): speedometer area
    - CG_DrawLowerLeftHUD() : rear-weapon ammo area
    - CG_DrawHUDOptionsMenu(): in-game overlay to toggle elements
===========================================================================
*/

#include "cg_local.h"
#include "cg_hud_elements.h"

/* -----------------------------------------------------------------------
   Shared constants used across all HUD modules
   ----------------------------------------------------------------------- */
#define HUD_RIGHT_EDGE          636.0f
#define HUD_COLUMN_SPACING        4.0f
#define HUD_TEXT_INSET            6.0f
#define HUD_ROW_HEIGHT          ((float)TINYCHAR_HEIGHT + 4.0f)

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
vmCvar_t  cg_hudShowDistToFinish;
vmCvar_t  cg_hudShowGhostDelta;
vmCvar_t  cg_hudShowArrow;
vmCvar_t  cg_hudShowCarAheadBehind;
vmCvar_t  cg_hudShowElimTimeline;
vmCvar_t  cg_hudShowOpponentList;
vmCvar_t  cg_hudShowScores;

vmCvar_t  cg_hudShowSpeed;
vmCvar_t  cg_hudShowFuelGauge;
vmCvar_t  cg_hudShowRearView;
vmCvar_t  cg_hudShowMiniMap;

vmCvar_t  cg_hudShowDerbyVehicle;
vmCvar_t  cg_hudShowDerbyList;
vmCvar_t  cg_hudShowDerbyHitImpact;

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
    trap_Cvar_Register( &cg_hudShowDistToFinish,   "cg_hudShowDistToFinish",   "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowGhostDelta,     "cg_hudShowGhostDelta",     "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowArrow,          "cg_hudShowArrow",          "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowCarAheadBehind, "cg_hudShowCarAheadBehind", "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowElimTimeline,   "cg_hudShowElimTimeline",   "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowOpponentList,   "cg_hudShowOpponentList",   "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowScores,         "cg_hudShowScores",         "1", CVAR_ARCHIVE );

    /* Vehicle */
    trap_Cvar_Register( &cg_hudShowSpeed,          "cg_hudShowSpeed",          "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowFuelGauge,      "cg_hudShowFuelGauge",      "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowRearView,       "cg_hudShowRearView",       "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowMiniMap,        "cg_hudShowMiniMap",        "1", CVAR_ARCHIVE );

    /* Derby */
    trap_Cvar_Register( &cg_hudShowDerbyVehicle,   "cg_hudShowDerbyVehicle",   "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowDerbyList,      "cg_hudShowDerbyList",      "1", CVAR_ARCHIVE );
    trap_Cvar_Register( &cg_hudShowDerbyHitImpact, "cg_hudShowDerbyHitImpact", "1", CVAR_ARCHIVE );
}

/* -----------------------------------------------------------------------
   CG_GetEliminationColumnWidth  (shared utility, stays in core)
   ----------------------------------------------------------------------- */
float CG_GetEliminationColumnWidth( void ) {
    static float columnWidth = 0.0f;

    if ( columnWidth <= 0.0f ) {
        const float charWidth  = (float)TINYCHAR_WIDTH;
        const float insetWidth = HUD_TEXT_INSET * 2.0f;
        float       maxWidth   = insetWidth + charWidth * CG_DrawStrlen( "T: 00:00.000" );
        float       candidate;

#define CHECK_CANDIDATE(str) \
        candidate = insetWidth + charWidth * CG_DrawStrlen( str ); \
        if ( candidate > maxWidth ) { maxWidth = candidate; }

        CHECK_CANDIDATE( "L: 00:00.000" )
        CHECK_CANDIDATE( "B: 00:00.000" )
        CHECK_CANDIDATE( "LAP: 00/00" )
        CHECK_CANDIDATE( "POS: 00/00" )
        CHECK_CANDIDATE( "DIST: 0000m" )
        CHECK_CANDIDATE( "DIST: 100.0%" )
        CHECK_CANDIDATE( "D: +00.000" )
        CHECK_CANDIDATE( "PLAYERS LEFT: 000" )
        CHECK_CANDIDATE( "R99 LEFT63 Name (99)" )
#undef CHECK_CANDIDATE

        columnWidth = maxWidth;
    }

    return columnWidth;
}


/* =======================================================================
   HUD OPTIONS MENU
   A lightweight in-game overlay rendered when cg.showHUDOptions is set.
   Bind a key in the .cfg:   bind F9 "toggle cg_hudOptionsOpen"
   ======================================================================= */

/*
 * Layout:  A translucent panel centred on screen.
 * Each row shows a label and a [ON]/[OFF] toggle drawn as coloured text.
 * Clicking is handled by tracking mouse position against row bounds –
 * the engine passes cursor data via CG_MouseEvent() which you hook below.
 *
 * NOTE: This uses the existing Q3 UI drawing primitives only (no new trap
 * calls needed), keeping it compatible with the ioq3 / OpenArena VM ABI.
 */

#define HUDOPT_X            160.0f
#define HUDOPT_Y             60.0f
#define HUDOPT_W            320.0f
#define HUDOPT_ROW_H         18.0f
#define HUDOPT_PADDING        6.0f
#define HUDOPT_TITLE_H       22.0f

typedef struct {
    const char  *label;
    const char  *cvarName;
    vmCvar_t    *cvar;
} hudToggleEntry_t;

static const hudToggleEntry_t hudToggleTable[] = {
    /* ---- Racing ---- */
    { "Times Panel",         "cg_hudShowTimes",          &cg_hudShowTimes          },
    { "Lap Counter",         "cg_hudShowLaps",           &cg_hudShowLaps           },
    { "Race Position",       "cg_hudShowPosition",       &cg_hudShowPosition       },
    { "Distance to Finish",  "cg_hudShowDistToFinish",   &cg_hudShowDistToFinish   },
    { "Ghost Delta",         "cg_hudShowGhostDelta",     &cg_hudShowGhostDelta     },
    { "Checkpoint Arrow",    "cg_hudShowArrow",          &cg_hudShowArrow          },
    { "Cars Ahead/Behind",   "cg_hudShowCarAheadBehind", &cg_hudShowCarAheadBehind },
    { "Elim. Timeline",      "cg_hudShowElimTimeline",   &cg_hudShowElimTimeline   },
    { "Opponent List",       "cg_hudShowOpponentList",   &cg_hudShowOpponentList   },
    { "Scores Panel",        "cg_hudShowScores",         &cg_hudShowScores         },
    /* ---- Vehicle ---- */
    { "Speedometer",         "cg_hudShowSpeed",          &cg_hudShowSpeed          },
    { "Fuel Gauge",          "cg_hudShowFuelGauge",      &cg_hudShowFuelGauge      },
    { "Rear-View Mirror",    "cg_hudShowRearView",       &cg_hudShowRearView       },
    { "Mini-Map",            "cg_hudShowMiniMap",        &cg_hudShowMiniMap        },
    /* ---- Derby ---- */
    { "Derby Vehicle State", "cg_hudShowDerbyVehicle",   &cg_hudShowDerbyVehicle   },
    { "Derby Scoreboard",    "cg_hudShowDerbyList",      &cg_hudShowDerbyList      },
    { "Derby Hit Impact",    "cg_hudShowDerbyHitImpact", &cg_hudShowDerbyHitImpact },
};

#define HUDOPT_NUM_ENTRIES  ( (int)( sizeof(hudToggleTable) / sizeof(hudToggleTable[0]) ) )

/* Track which row the cursor hovers over (-1 = none) */
static int      g_hudOptHoverRow  = -1;

/*
================
CG_HUDOptions_MouseEvent
Forward raw mouse deltas here from CG_MouseEvent().
cx/cy are *absolute* cursor coordinates in 640x480 space.
Returns qtrue if the menu consumed the click.
================
*/
qboolean CG_HUDOptions_MouseEvent( int cx, int cy, qboolean clicked ) {
    int     i;
    float   rowY;

    if ( !cg_hudOptionsOpen.integer ) {
        return qfalse;
    }

    g_hudOptHoverRow = -1;

    rowY = HUDOPT_Y + HUDOPT_TITLE_H + HUDOPT_PADDING;
    for ( i = 0; i < HUDOPT_NUM_ENTRIES; i++ ) {
        float rowTop    = rowY;
        float rowBottom = rowY + HUDOPT_ROW_H;

        if ( cy >= rowTop && cy < rowBottom &&
             cx >= HUDOPT_X && cx < HUDOPT_X + HUDOPT_W ) {
            g_hudOptHoverRow = i;
            if ( clicked ) {
                /* Toggle the cvar */
                int cur = hudToggleTable[i].cvar->integer;
                trap_Cvar_Set( hudToggleTable[i].cvarName, cur ? "0" : "1" );
                trap_Cvar_Update( hudToggleTable[i].cvar );
            }
            return qtrue;
        }
        rowY += HUDOPT_ROW_H;
    }

    return qtrue; /* consume all mouse input while menu is open */
}

/*
================
CG_DrawHUDOptionsMenu
Renders the toggle overlay when cg_hudOptionsOpen is set.
Call from CG_DrawActive() *after* the main HUD pass.
================
*/
void CG_DrawHUDOptionsMenu( void ) {
    int     i;
    float   rowY;
    float   totalH;
    float   bgColor[4]     = { 0.0f, 0.0f, 0.0f, 0.75f };
    float   titleColor[4]  = { 1.0f, 0.69f, 0.0f, 1.0f };
    float   onColor[4]     = { 0.2f, 1.0f, 0.2f, 1.0f };
    float   offColor[4]    = { 1.0f, 0.3f, 0.3f, 1.0f };
    float   hoverColor[4]  = { 1.0f, 1.0f, 0.3f, 0.25f };
    float   labelColor[4]  = { 0.9f, 0.9f, 0.9f, 1.0f };

    if ( !cg_hudOptionsOpen.integer ) {
        return;
    }

    /* Refresh all toggle cvar values from engine */
    trap_Cvar_Update( &cg_hudOptionsOpen );
    trap_Cvar_Update( &cg_hudShowTimes );
    trap_Cvar_Update( &cg_hudShowLaps );
    trap_Cvar_Update( &cg_hudShowPosition );
    trap_Cvar_Update( &cg_hudShowDistToFinish );
    trap_Cvar_Update( &cg_hudShowGhostDelta );
    trap_Cvar_Update( &cg_hudShowArrow );
    trap_Cvar_Update( &cg_hudShowCarAheadBehind );
    trap_Cvar_Update( &cg_hudShowElimTimeline );
    trap_Cvar_Update( &cg_hudShowOpponentList );
    trap_Cvar_Update( &cg_hudShowScores );
    trap_Cvar_Update( &cg_hudShowSpeed );
    trap_Cvar_Update( &cg_hudShowFuelGauge );
    trap_Cvar_Update( &cg_hudShowRearView );
    trap_Cvar_Update( &cg_hudShowMiniMap );
    trap_Cvar_Update( &cg_hudShowDerbyVehicle );
    trap_Cvar_Update( &cg_hudShowDerbyList );
    trap_Cvar_Update( &cg_hudShowDerbyHitImpact );

    /* Force neutral placement so coordinates are screen-absolute */
    CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );

    totalH = HUDOPT_TITLE_H + HUDOPT_PADDING
             + HUDOPT_NUM_ENTRIES * HUDOPT_ROW_H
             + HUDOPT_PADDING;

    /* Background panel */
    CG_FillRect( HUDOPT_X - HUDOPT_PADDING,
                 HUDOPT_Y - HUDOPT_PADDING,
                 HUDOPT_W + HUDOPT_PADDING * 2.0f,
                 totalH   + HUDOPT_PADDING * 2.0f,
                 bgColor );

    /* Title */
    CG_DrawStringExt( (int)( HUDOPT_X + HUDOPT_W * 0.5f
                              - CG_DrawStrlen( "HUD ELEMENTS" ) * SMALLCHAR_WIDTH * 0.5f ),
                      (int)HUDOPT_Y,
                      "HUD ELEMENTS",
                      titleColor, qfalse, qtrue,
                      SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0 );

    /* Hint */
    {
        const char *hint = "Click row to toggle  |  F9 to close";
        CG_DrawStringExt( (int)( HUDOPT_X + HUDOPT_W * 0.5f
                                  - CG_DrawStrlen( hint ) * TINYCHAR_WIDTH * 0.5f ),
                          (int)( HUDOPT_Y + SMALLCHAR_HEIGHT + 2 ),
                          hint,
                          labelColor, qfalse, qtrue,
                          TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
    }

    rowY = HUDOPT_Y + HUDOPT_TITLE_H + HUDOPT_PADDING;

    for ( i = 0; i < HUDOPT_NUM_ENTRIES; i++ ) {
        qboolean enabled = ( hudToggleTable[i].cvar->integer != 0 );

        /* Hover highlight */
        if ( i == g_hudOptHoverRow ) {
            CG_FillRect( HUDOPT_X, rowY, HUDOPT_W, HUDOPT_ROW_H, hoverColor );
        }

        /* Label */
        CG_DrawStringExt( (int)( HUDOPT_X + 4 ),
                          (int)( rowY + 2 ),
                          hudToggleTable[i].label,
                          labelColor, qfalse, qfalse,
                          TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );

        /* ON / OFF badge */
        {
            const char  *badge     = enabled ? "[ON] " : "[OFF]";
            float       *badgeClr  = enabled ? onColor : offColor;
            int          badgeX    = (int)( HUDOPT_X + HUDOPT_W - CG_DrawStrlen( badge ) * TINYCHAR_WIDTH - 4 );

            CG_DrawStringExt( badgeX,
                              (int)( rowY + 2 ),
                              badge,
                              badgeClr, qfalse, qfalse,
                              TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
        }

        rowY += HUDOPT_ROW_H;
    }

    CG_PopScreenPlacement();
}


/* =======================================================================
   MAIN HUD ENTRY POINT
   ======================================================================= */

/*
================
CG_DrawUpperRightHUD
Draws the legacy right-side racing info stack.
================
*/
float CG_DrawUpperRightHUD( float y ) {
    int i;

    /* FIXME: move racer count update somewhere more appropriate */
    cgs.numRacers = 0;
    for ( i = 0; i < cgs.maxclients; i++ ) {
        if ( !cgs.clientinfo[i].infoValid )                      continue;
        if (  cgs.clientinfo[i].team == TEAM_SPECTATOR )         continue;
        if (  cg.scores[i].ping == -1 )                          continue;
        cgs.numRacers++;
    }

    if ( cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR ) {
        if ( isRallyRace() ) {
            float timesStart;
            float timesY;

            if ( cg_hudShowArrow.integer ) {
                y = CG_DrawArrowToCheckpoint( y );
            }

            timesStart = y;
            timesY     = y;

            CG_UpdateGhostSplitDelta();

            if ( cg_hudShowTimes.integer )         timesY = CG_DrawTimes( timesY );
            if ( cg_hudShowGhostDelta.integer )    timesY = CG_DrawGhostSplitDelta( timesY );
            if ( cg_hudShowLaps.integer )          timesY = CG_DrawLaps( timesY );
            if ( cg_hudShowDistToFinish.integer )  timesY = CG_DrawDistanceToFinish( timesY );
            if ( cg_hudShowElimTimeline.integer )  timesY = CG_DrawEliminationTimeline( timesY );

            if ( cg_hudShowPosition.integer )      CG_DrawCurrentPosition( timesStart );
            if ( cg_hudShowCarAheadBehind.integer) y = CG_DrawCarAheadAndBehind( timesY );
            else                                   y = timesY;

        } else if ( cgs.gametype == GT_DERBY || cgs.gametype == GT_LCS ) {
            float timesStart = y;
            if ( cg_hudShowTimes.integer ) y = CG_DrawTimes( y );
            if ( cg_hudShowPosition.integer && cgs.gametype == GT_LCS ) {
                CG_DrawCurrentPosition( timesStart );
            }
        }
    }

    if ( !isRallyNonDMRace()
         && cgs.gametype != GT_DERBY
         && cgs.gametype != GT_LCS
         && cg_hudShowScores.integer ) {
        y = CG_DrawScores( 636, y );
    }

    return y;
}


/*
================
CG_DrawLowerRightHUD
================
*/
float CG_DrawLowerRightHUD( float y ) {
    if ( cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR ) {
        if ( cg_hudShowSpeed.integer ) {
            y = CG_DrawSpeed( y );
        }
    }
    return y;
}


/*
================
CG_DrawLowerLeftHUD
================
*/
float CG_DrawLowerLeftHUD( float y ) {
    int i;

    y += 36;
    for ( i = RWP_SMOKE; i < WP_NUM_WEAPONS; i++ ) {
        if ( cg.snap->ps.stats[STAT_WEAPONS] & ( 1u << i ) ) {
            if ( cg.snap->ps.ammo[i] ) {
                y -= 36;
                break;
            }
        }
    }
    return y;
}


/*
================================
CG_DrawHUD
Main HUD dispatcher, called each frame from CG_DrawActive().
================================
*/
qboolean CG_DrawHUD( void ) {
    /* Update all HUD toggle cvars from engine each frame */
    trap_Cvar_Update( &cg_hudOptionsOpen );
    trap_Cvar_Update( &cg_hudShowTimes );
    trap_Cvar_Update( &cg_hudShowLaps );
    trap_Cvar_Update( &cg_hudShowPosition );
    trap_Cvar_Update( &cg_hudShowDistToFinish );
    trap_Cvar_Update( &cg_hudShowGhostDelta );
    trap_Cvar_Update( &cg_hudShowArrow );
    trap_Cvar_Update( &cg_hudShowCarAheadBehind );
    trap_Cvar_Update( &cg_hudShowElimTimeline );
    trap_Cvar_Update( &cg_hudShowOpponentList );
    trap_Cvar_Update( &cg_hudShowScores );
    trap_Cvar_Update( &cg_hudShowSpeed );
    trap_Cvar_Update( &cg_hudShowFuelGauge );
    trap_Cvar_Update( &cg_hudShowRearView );
    trap_Cvar_Update( &cg_hudShowMiniMap );
    trap_Cvar_Update( &cg_hudShowDerbyVehicle );
    trap_Cvar_Update( &cg_hudShowDerbyList );
    trap_Cvar_Update( &cg_hudShowDerbyHitImpact );

    if ( cg_paused.integer ) {
        return qfalse;
    }

    if ( !cg.showHUD ) {
        if ( cgs.gametype == GT_DERBY
             && cg_hudShowDerbyVehicle.integer ) {
            CG_DrawHUD_DerbyVehicleState();
        }
        return qfalse;
    }

    /* Keep scores fresh for accurate team DM times */
    if ( cg.scoresRequestTime + 2000 < cg.time ) {
        cg.scoresRequestTime = cg.time;
        trap_SendClientCommand( "score" );
    }

    switch ( cgs.gametype ) {

    default:
    case GT_RACING:
    case GT_SPRINT:
    case GT_TEAM_RACING:
        if ( cg_hudShowTimes.integer )    CG_DrawHUD_Times( 0, 112 );
        if ( cg_hudShowPosition.integer ) CG_DrawHUD_Positions( 0, 228 );
        if ( cg_hudShowLaps.integer )     CG_DrawHUD_Laps( 0, 304 );
        break;

    case GT_ELIMINATION:
        if ( cg_hudShowTimes.integer )         CG_DrawHUD_Times( 0, 112 );
        if ( cg_hudShowPosition.integer )      CG_DrawHUD_Positions( 0, 228 );
        if ( cg_hudShowLaps.integer )          CG_DrawHUD_Laps( 0, 304 );
        if ( cg_hudShowOpponentList.integer )  CG_DrawHUD_OpponentList( 440, 130 );
        break;

    case GT_RACING_DM:
    case GT_TEAM_RACING_DM:
        if ( cg_hudShowTimes.integer )    CG_DrawHUD_Times( 0, 112 );
        if ( cg_hudShowPosition.integer ) CG_DrawHUD_Positions( 0, 228 );
        if ( cg_hudShowLaps.integer )     CG_DrawHUD_Laps( 0, 304 );
        if ( cg_hudShowScores.integer )   CG_DrawHUD_Scores( 264, 130 );
        break;

    case GT_DEATHMATCH:
    case GT_TEAM:
    case GT_CTF:
    case GT_DOMINATION:
        if ( cg_hudShowScores.integer )   CG_DrawHUD_Scores( 264, 130 );
        break;

    case GT_DERBY:
        if ( cg_hudShowDerbyVehicle.integer )   CG_DrawHUD_DerbyVehicleState();
        if ( cg_hudShowDerbyList.integer )      CG_DrawHUD_DerbyList( 440, 130 );
        if ( cg_hudShowDerbyHitImpact.integer ) CG_DrawHUD_DerbyHitImpact();
        break;

    case GT_LCS:
        if ( cg_hudShowOpponentList.integer )   CG_DrawHUD_OpponentList( 440, 130 );
        break;
    }

    return qtrue;
}
