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
   HUD OPTIONS
   Shift+H (or "toggle cg_hudOptionsOpen"). The panel is cg_hud_panel.c,
   laid out by MH_OptionsLayout (the design: gen.py hud_panel); it shows the
   toggles of this table that do something in the mode, in their groups.
   ======================================================================= */

#define HUDOPT_MODE_ANY                 -1
#define HUDOPT_MODE_COURSE              -3	/* a race on a course: guidance, slipstream */
#define HUDOPT_MODE_ORDER               -4	/* the order at the top right (not Derby's) */
#define HUDOPT_MODE_TIMES               -5	/* the times in the card */
#define HUDOPT_MODE_LAPS                -6	/* a race with laps */
#define HUDOPT_MODE_POSITION            -7	/* the place in the card (the ghost race has the gap) */
#define HUDOPT_MODE_VIEWS               -8	/* rear view and minimap (Drag: the tachometer) */

/* the groups, in the panel's order; the left column first */
typedef enum { HUDG_CARD, HUDG_TRACK, HUDG_ORDER, HUDG_FX, HUDG_CAR, HUDG_MAP, HUDG_COUNT } hudGroup_t;

static const struct {
    const char  *title;
    int          region;    /* mhOptionsRegion_t */
    int          column;    /* the car's goes left in a mode without the card's toggles */
} hudGroups[HUDG_COUNT] = {
    { "Card",            MH_OPT_CARD,  0 },
    { "On track",        MH_OPT_TRACK, 0 },
    { "Order",           MH_OPT_ORDER, 0 },
    { "Effects",         MH_OPT_FX,    0 },
    { "Car and cockpit", MH_OPT_CAR,   1 },
    { "Mini-map",        MH_OPT_MAP,   1 },
};

typedef struct {
    const char  *label;         /* NULL: the HUD's own name for its order */
    const char  *cvarName;
    vmCvar_t    *cvar;
    int          onValue;       /* the highest value: 0..onValue are the choices */
    int          gameTypeOnly;  /* exact GT_* value or negative mode-group selector     */
    int          group;         /* hudGroup_t */
    const char  *hint;          /* under the label; NULL: none */
    const char *const *choiceLabels;	/* NULL: Off, On */
    qboolean     wide;          /* the choices under the label */
} hudToggleEntry_t;

static const char *const hudGhostPlaybackLabels[] = {
    "Off", "Personal", "Server", "Ladder", NULL
};

static const char *const hudCheckpointArrowLabels[] = {
    "Off", "On HUD", "Above car", NULL
};

static const hudToggleEntry_t hudToggleTable[] = {
    { "Times",            "cg_hudShowTimes",        &cg_hudShowTimes,        1, HUDOPT_MODE_TIMES,    HUDG_CARD,  NULL, NULL, qfalse },
    { "Lap counter",      "cg_hudShowLaps",         &cg_hudShowLaps,         1, HUDOPT_MODE_LAPS,     HUDG_CARD,  NULL, NULL, qfalse },
    { "Race position",    "cg_hudShowPosition",     &cg_hudShowPosition,     1, HUDOPT_MODE_POSITION, HUDG_CARD,  NULL, NULL, qfalse },
    { "Ghost",            "cg_ghostPlayback",       &cg_ghostPlayback,       3, GT_GHOST,             HUDG_TRACK, "The car you race against", hudGhostPlaybackLabels, qtrue },
    { "Checkpoint arrow", "cg_checkpointArrowMode", &cg_checkpointArrowMode, 2, HUDOPT_MODE_COURSE,   HUDG_TRACK, "On HUD: at the top of the screen", hudCheckpointArrowLabels, qtrue },
    { "Racing line",      "cg_racingLine",          &cg_racingLine,          1, HUDOPT_MODE_COURSE,   HUDG_TRACK, NULL, NULL, qfalse },
    { NULL,               "cg_hudShowOpponentList", &cg_hudShowOpponentList, 1, HUDOPT_MODE_ORDER,    HUDG_ORDER, NULL, NULL, qfalse },
    { NULL,               "cg_hudShowDerbyList",    &cg_hudShowDerbyList,    1, GT_DERBY,             HUDG_ORDER, NULL, NULL, qfalse },
    { "Hit flash",        "cg_derbyHitFxEnable",    &cg_derbyHitFxEnable,    1, GT_DERBY,             HUDG_FX,    "Red flash when you are hit", NULL, qfalse },
    { "HUD",              "cg_drawStatus",          &cg_drawStatus,          1, HUDOPT_MODE_ANY,      HUDG_CAR,   "Off hides every panel", NULL, qfalse },
    { "Rear view",        "cg_drawRearView",        &cg_drawRearView,        1, HUDOPT_MODE_VIEWS,    HUDG_CAR,   NULL, NULL, qfalse },
    { "Slipstream gauge", "cg_hudShowSlipstream",   &cg_hudShowSlipstream,   1, HUDOPT_MODE_COURSE,   HUDG_CAR,   NULL, NULL, qfalse },
    { "Mini-map",         "cg_drawMMap",            &cg_drawMMap,            1, HUDOPT_MODE_VIEWS,    HUDG_MAP,   NULL, NULL, qfalse },
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
qtrue when a toggle does nothing in this mode (the panel leaves it out).
Negative values are the HUDOPT_MODE_* groups, others exact GT_* matches.
Drag has its own HUD: the times, and the order in a duel.
================
*/
static qboolean HUDOpt_Unavail( int mode ) {
    if ( mode == HUDOPT_MODE_ANY )
        return qfalse;
    if ( mode == HUDOPT_MODE_COURSE )
        return ( cgs.gametype == GT_DRAG || !isRallyRace() );
    if ( mode == HUDOPT_MODE_POSITION )
        return ( cgs.gametype == GT_DRAG || cgs.gametype == GT_GHOST || !isRallyRace() );
    if ( mode == HUDOPT_MODE_ORDER )
        return ( cgs.gametype == GT_DRAG && cgs.numRacers < 2 ) || cgs.gametype == GT_DERBY;
    if ( mode == HUDOPT_MODE_TIMES )
        return ( !isRallyRace() && cgs.gametype != GT_LCS );
    if ( mode == HUDOPT_MODE_LAPS )
        return ( !isRallyRace() || cgs.gametype == GT_ELIMINATION || cgs.gametype == GT_DRAG || CG_IsSprintTrack() );
    if ( mode == HUDOPT_MODE_VIEWS )
        return ( cgs.gametype == GT_DRAG );
    return ( cgs.gametype != mode );
}

static qboolean HUDEntry_IsUnavail( const hudToggleEntry_t *e ) {
    return HUDOpt_Unavail( e->gameTypeOnly );
}

/* a toggle's choice as the panel shows it */
static const char *HUDEntry_Choice( const hudToggleEntry_t *e, int choice ) {
    if ( e->cvar == &cg_ghostPlayback && cgs.gametype == GT_GHOST && choice == 0 ) return "Auto";
    if ( e->choiceLabels ) return e->choiceLabels[choice];
    return choice ? "On" : "Off";
}

/* the minimap's sliders: size (cg_mmap_size x 100) and field of view */
#define HUDOPT_SIZE_MIN     40
#define HUDOPT_SIZE_MAX     200
#define HUDOPT_FOV_MIN      30
#define HUDOPT_FOV_MAX      120

/*
================
CG_HUDOptions_Build
The panel's content for this mode: the groups that have a toggle, the
toggles, the minimap's sliders under it while it is on. entry[] gets each
item's toggle (an index into the table), or HUDOPT_ITEM_SIZE / _FOV.
================
*/
void CG_HUDOptions_Build( mhOptionsLayout_t *o, int *entry, int *value ) {
    static char crumb[96], sizeText[16], fovText[16];
    const char *title, *order;
    int g, i, groupOf[HUDG_COUNT];
    int size, fov;

    CG_ModernHUD_Names( &title, &order );
    Com_sprintf( crumb, sizeof( crumb ), "Live HUD \xb7 %s", title && title[0] ? title : "HUD" );
    o->crumb = crumb;
    if ( isRallyRace() ) {
        o->noun = "race";
        o->back = "Back to race";
    } else {
        o->noun = cgs.gametype == GT_DERBY ? "derby" : "match";
        o->back = "Back to match";
    }

    o->groupCount = o->itemCount = 0;
    for ( g = 0; g < HUDG_COUNT; g++ ) groupOf[g] = -1;
    for ( g = 0; g < HUDG_COUNT; g++ ) {
        for ( i = 0; i < HUDOPT_NUM_ENTRIES; i++ ) {
            const hudToggleEntry_t *e = &hudToggleTable[i];
            int item, c;
            if ( e->group != g || HUDEntry_IsUnavail( e ) || o->itemCount >= MH_OPT_ITEMS ) continue;
            if ( groupOf[g] < 0 ) {
                groupOf[g] = o->groupCount++;
                o->groupTitle[groupOf[g]] = hudGroups[g].title;
                o->groupRegion[groupOf[g]] = hudGroups[g].region;
                o->groupColumn[groupOf[g]] = hudGroups[g].column;
                /* without the card's toggles the left column is short: the car's join it */
                if ( g == HUDG_CAR && groupOf[HUDG_CARD] < 0 ) o->groupColumn[groupOf[g]] = 0;
            }
            trap_Cvar_Update( e->cvar );
            item = o->itemCount++;
            o->itemGroup[item] = groupOf[g];
            o->itemLabel[item] = e->label ? e->label : ( order && order[0] ? order : "Order" );
            o->itemHint[item] = e->hint;
            if ( e->cvar == &cg_hudShowTimes ) {
                o->itemHint[item] = cgs.gametype == GT_LCS ? "Match time" :
                                    HUDOpt_Unavail( HUDOPT_MODE_LAPS ) ? NULL : "Lap, best, gap";
            }
            o->itemChoices[item] = e->onValue + 1;
            for ( c = 0; c <= e->onValue && c < MH_OPT_CHOICES; c++ ) o->itemChoice[item][c] = HUDEntry_Choice( e, c );
            o->itemWide[item] = e->wide;
            entry[item] = i;
            value[item] = e->cvar->integer;

            /* the minimap's size and field of view, while it is on */
            if ( e->cvar == &cg_drawMMap && e->cvar->integer && o->itemCount + 2 <= MH_OPT_ITEMS ) {
                trap_Cvar_Update( &cg_mmap_size );
                trap_Cvar_Update( &cg_mmap_fov );
                size = (int)( cg_mmap_size.value * 100.0f + 0.5f );
                fov = (int)( cg_mmap_fov.value + 0.5f );
                Com_sprintf( sizeText, sizeof( sizeText ), "%d.%02d", size / 100, size % 100 );
                Com_sprintf( fovText, sizeof( fovText ), "%d", fov );
                item = o->itemCount++;
                o->itemGroup[item] = groupOf[g];
                o->itemLabel[item] = "Size";
                o->itemChoices[item] = 0;
                o->itemValue[item] = sizeText;
                o->itemFrac[item] = (float)( size - HUDOPT_SIZE_MIN ) / ( HUDOPT_SIZE_MAX - HUDOPT_SIZE_MIN );
                entry[item] = HUDOPT_ITEM_SIZE;
                value[item] = size;
                item = o->itemCount++;
                o->itemGroup[item] = groupOf[g];
                o->itemLabel[item] = "Field of view";
                o->itemChoices[item] = 0;
                o->itemValue[item] = fovText;
                o->itemFrac[item] = (float)( fov - HUDOPT_FOV_MIN ) / ( HUDOPT_FOV_MAX - HUDOPT_FOV_MIN );
                entry[item] = HUDOPT_ITEM_FOV;
                value[item] = fov;
            }
        }
    }
}

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


/*
================
CG_HUDOptions_Set
A choice in the panel (entry: the toggle; HUDOPT_ITEM_SIZE / _FOV: a
slider's value, size x 100 or degrees, in steps of 5). Only while the
panel is open, only toggles of this mode, only values they have.
================
*/
void CG_HUDOptions_Set( int entry, int value ) {
    const hudToggleEntry_t *e;
    trap_Cvar_Update( &cg_hudOptionsOpen );
    if ( !CG_HUDOptionsIsOpen() ) return;
    if ( entry == HUDOPT_ITEM_SIZE ) {
        if ( value >= HUDOPT_SIZE_MIN && value <= HUDOPT_SIZE_MAX && value % 5 == 0 )
            HUDOpt_SetFloatCvar( "cg_mmap_size", &cg_mmap_size, value / 100.0f, qfalse );
    } else if ( entry == HUDOPT_ITEM_FOV ) {
        if ( value >= HUDOPT_FOV_MIN && value <= HUDOPT_FOV_MAX && value % 5 == 0 )
            HUDOpt_SetFloatCvar( "cg_mmap_fov", &cg_mmap_fov, value, qtrue );
    } else if ( entry >= 0 && entry < HUDOPT_NUM_ENTRIES ) {
        e = &hudToggleTable[entry];
        if ( HUDEntry_IsUnavail( e ) || value < 0 || value > e->onValue ) return;
        trap_Cvar_Set( e->cvarName, va( "%d", value ) );
        trap_Cvar_Update( e->cvar );
    }
}
