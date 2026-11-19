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
Q3Rally Graphics Loading Screen

Runs the UI cache stages one by one. It looks exactly like the WebUI's
"Getting ready." screen (screen 1 of engine/webui/garage): the layout comes
from ui_rally_gfxlayout.c, which follows the WebUI's CSS, and the drawing
from ui_rally_webstyle.c. The WebUI used to draw this screen over the game;
now the game draws it on every platform.

The online version check opens the update prompt with "Open download page"
and "Continue".
===========================================================================
*/

#include "ui_local.h"
#include "ui_rally_gfxlayout.h"
#include "ui_rally_frontpage.h"

/* -------------------------------------------------------------------------
   Timing (milliseconds)
   ------------------------------------------------------------------------- */

/* Deliberately unhurried: on a fast PC the cache work takes a few dozen
 * milliseconds, these minimums let the stages play out. Slower machines
 * simply take longer per stage. */
#define GFX_STAGE_TIME          550     /* minimum time per stage */
#define GFX_GO_TIME            1200     /* from 100% until "Ready to race" hands over */
#define GFX_UPDATE_WAIT_TIME   2500     /* extra wait for a still-running version check */
#define GFX_GO_HOLD             800     /* "go" until the main menu takes over */
#define GFX_MAX_DEBUG_PAUSE   15000     /* safety clamp for the test cvars */
#define GFX_SMOOTH_LERP_SPEED  4.5f     /* progress smoothing, units per second */

/* Used when version.txt does not announce a download page. */
#define GFX_DEFAULT_DOWNLOAD_URL "https://www.q3rally.com/downloads"
#define GFX_GO_SOUND             "sound/rally/race/go.ogg"

/* -------------------------------------------------------------------------
   Types
   ------------------------------------------------------------------------- */

typedef enum {
    GFX_BTN_NONE = -1,
    GFX_BTN_UPDATE,
    GFX_BTN_SKIP
} gfxButton_t;

/* Mirrors the cl_updateState values written by client/cl_update.c. */
typedef enum {
    GFX_UPD_IDLE,
    GFX_UPD_CHECKING,
    GFX_UPD_CURRENT,
    GFX_UPD_AHEAD,
    GFX_UPD_OUTDATED,
    GFX_UPD_OFFLINE,
    GFX_UPD_FAILED,
    GFX_UPD_UNAVAILABLE
} gfxUpdateState_t;

typedef struct {
    void      (*exec)( void );
} gfxStage_t;

#define GFX_MAX_STAGES  16

typedef struct {
    menuframework_s  menu;

    /* the player's car, loaded by the vehicle stage */
    playerInfo_t     playerinfo;
    qboolean         carLoaded;
    char             vehicleName[64];

    /* stages */
    int              startTime;
    int              currentStage;
    int              stageStartTime;
    qboolean         stageShown;            /* drawn at least once - run it next frame */
    qboolean         stageExecuted;
    float            loadPercent;           /* raw progress [0.0 - 1.0] */
    float            smoothProgress;        /* smoothed value used for drawing */
    int              lastDrawTime;

    /* final phase and hand-off */
    qboolean         finalPhase;            /* all stages done */
    int              finalStartTime;
    int              goTime;                /* hand-off started, 0 until then */
    qboolean         skipRequested;
    qboolean         musicStarted;
    sfxHandle_t      goSound;

    /* update notice */
    gfxUpdateState_t updateState;
    qboolean         requireUpdateAck;      /* remote version newer than local */
    qboolean         updateAcked;           /* player answered the notice */
    gfxButton_t      ackedWith;
    gfxButton_t      focusedBtn;            /* keyboard / pad focus, NONE until used */

    gfxLayout_t      layout;                /* of the last frame, for the mouse */
} gfxloading_t;

static gfxloading_t s_gfxloading;

/* -------------------------------------------------------------------------
   Stages
   ------------------------------------------------------------------------- */

static void GFX_ReadVehicle( void ) {
    char  model[MAX_QPATH];
    char *slash;

    trap_Cvar_VariableStringBuffer( "model", model, sizeof( model ) );
    if ( !model[0] ) {
        Q_strncpyz( model, DEFAULT_MODEL, sizeof( model ) );
    }
    slash = strchr( model, '/' );
    if ( slash ) {
        *slash = '\0';
    }
    WS_VehicleName( model, s_gfxloading.vehicleName, sizeof( s_gfxloading.vehicleName ) );
}

/* Load the active car the same way the main menu does. */
static void GFXStage_Vehicle( void ) {
    char   model[MAX_QPATH];
    char   rim[MAX_QPATH];
    char   head[MAX_QPATH];
    char   plate[MAX_QPATH];
    vec3_t viewangles;
    vec3_t moveangles;

    trap_Cvar_VariableStringBuffer( "model", model, sizeof( model ) );
    if ( !model[0] ) {
        Com_sprintf( model, sizeof( model ), "%s/%s", DEFAULT_MODEL, DEFAULT_SKIN );
    } else if ( !strchr( model, '/' ) ) {
        Q_strcat( model, sizeof( model ), "/" DEFAULT_SKIN );
    }
    trap_Cvar_VariableStringBuffer( "rim",   rim,   sizeof( rim ) );
    trap_Cvar_VariableStringBuffer( "head",  head,  sizeof( head ) );
    trap_Cvar_VariableStringBuffer( "plate", plate, sizeof( plate ) );

    VectorClear( viewangles );
    VectorClear( moveangles );

    UI_PlayerInfo_SetModel( &s_gfxloading.playerinfo, model, rim, head, plate );
    UI_PlayerInfo_SetInfo( &s_gfxloading.playerinfo, LEGS_IDLE, TORSO_STAND,
                           viewangles, moveangles, WP_NONE, qfalse );
    s_gfxloading.carLoaded = qtrue;
}

/* gfxStageLabels (ui_rally_gfxlayout.c) names them in this order */
static const gfxStage_t gfxStages[GFX_LAYOUT_STAGES] = {
    { NULL                 },
    { GFXStage_Vehicle     },
    { UI_SetupMenu_Cache   },
    { PlayerModel_Cache    },
    { PlayerSettings_Cache },
    { Controls_Cache       },
    { ArenaServers_Cache   },
    { StartServer_Cache    },
};

#define GFX_NUM_STAGES  GFX_LAYOUT_STAGES

/* -------------------------------------------------------------------------
   Update check state
   ------------------------------------------------------------------------- */

static gfxUpdateState_t GFX_ReadUpdateState( void ) {
    char state[32];

    trap_Cvar_VariableStringBuffer( "cl_updateState", state, sizeof( state ) );

    if ( !Q_stricmp( state, "checking" ) )    return GFX_UPD_CHECKING;
    if ( !Q_stricmp( state, "current" ) )     return GFX_UPD_CURRENT;
    if ( !Q_stricmp( state, "ahead" ) )       return GFX_UPD_AHEAD;
    if ( !Q_stricmp( state, "outdated" ) )    return GFX_UPD_OUTDATED;
    if ( !Q_stricmp( state, "offline" ) )     return GFX_UPD_OFFLINE;
    if ( !Q_stricmp( state, "failed" ) )      return GFX_UPD_FAILED;
    if ( !Q_stricmp( state, "unavailable" ) ) return GFX_UPD_UNAVAILABLE;
    return GFX_UPD_IDLE;
}

static qboolean GFX_UpdateCheckPending( void ) {
    return ( s_gfxloading.updateState == GFX_UPD_IDLE ||
             s_gfxloading.updateState == GFX_UPD_CHECKING ) ? qtrue : qfalse;
}

static qboolean GFX_UpdatePromptActive( void ) {
    return ( s_gfxloading.requireUpdateAck && !s_gfxloading.updateAcked ) ? qtrue : qfalse;
}

static void GFX_RefreshUpdateState( void ) {
    s_gfxloading.updateState = GFX_ReadUpdateState();

    if ( s_gfxloading.updateState == GFX_UPD_OUTDATED ) {
        if ( !s_gfxloading.requireUpdateAck ) {
            s_gfxloading.requireUpdateAck = qtrue;
            s_gfxloading.updateAcked      = qfalse;
            s_gfxloading.focusedBtn       = GFX_BTN_NONE;
        }
    } else {
        s_gfxloading.requireUpdateAck = qfalse;
        s_gfxloading.updateAcked      = qfalse;
    }
}

static const char *GFX_DownloadURL( void ) {
    static char url[256];

    /* cl_updateUrl is validated by the client (official site only); the quote
     * check keeps the openURL command below well-formed regardless. */
    trap_Cvar_VariableStringBuffer( "cl_updateUrl", url, sizeof( url ) );
    if ( !url[0] || strchr( url, '"' ) ) {
        return GFX_DEFAULT_DOWNLOAD_URL;
    }
    return url;
}

static void GFX_ActivateButton( gfxButton_t button ) {
    if ( button == GFX_BTN_UPDATE ) {
        trap_Cmd_ExecuteText( EXEC_APPEND, va( "openURL \"%s\"\n", GFX_DownloadURL() ) );
    }
    /* both buttons acknowledge the notice so the game continues */
    s_gfxloading.updateAcked = qtrue;
    s_gfxloading.ackedWith   = button;
}

/* -------------------------------------------------------------------------
   Stage execution and progress
   ------------------------------------------------------------------------- */

static int GFX_GetPause( const char *cvarName, int fallback ) {
    int value = (int)trap_Cvar_VariableValue( cvarName );

    if ( value <= 0 ) {
        return fallback;
    }
    if ( value > GFX_MAX_DEBUG_PAUSE ) {
        return GFX_MAX_DEBUG_PAUSE;
    }
    return value;
}

/*
 * A stage runs in the frame after its name was first drawn, so a slow cache
 * call never freezes the screen on the previous stage.
 */
static void GFX_ExecuteStage( void ) {
    const gfxStage_t *stage;

    if ( s_gfxloading.stageExecuted || !s_gfxloading.stageShown ||
         s_gfxloading.currentStage >= GFX_NUM_STAGES ) {
        return;
    }

    stage = &gfxStages[s_gfxloading.currentStage];
    if ( stage->exec ) {
        stage->exec();
    }
    s_gfxloading.stageExecuted = qtrue;
}

static void GFX_UpdateProgress( int currentTime ) {
    int   stageTime = GFX_GetPause( "ui_gfxLoadingStagePause", GFX_STAGE_TIME );
    int   elapsed;
    float stageProgress;

    GFX_ExecuteStage();

    if ( s_gfxloading.currentStage >= GFX_NUM_STAGES ) {
        return;
    }

    elapsed = currentTime - s_gfxloading.stageStartTime;
    if ( elapsed >= stageTime && s_gfxloading.stageExecuted ) {
        s_gfxloading.currentStage++;
        s_gfxloading.stageStartTime = currentTime;
        s_gfxloading.stageShown     = qfalse;
        s_gfxloading.stageExecuted  = qfalse;
        s_gfxloading.loadPercent    = (float)s_gfxloading.currentStage / (float)GFX_NUM_STAGES;

        if ( s_gfxloading.currentStage >= GFX_NUM_STAGES ) {
            s_gfxloading.loadPercent    = 1.0f;
            s_gfxloading.finalPhase     = qtrue;
            s_gfxloading.finalStartTime = currentTime;
        }
        return;
    }

    stageProgress = (float)elapsed / (float)stageTime;
    if ( stageProgress > 1.0f ) {
        stageProgress = 1.0f;
    }
    s_gfxloading.loadPercent =
        ( (float)s_gfxloading.currentStage + stageProgress ) / (float)GFX_NUM_STAGES;
}

static void GFX_UpdateSmoothProgress( int currentTime ) {
    float deltaTime;

    deltaTime = ( s_gfxloading.lastDrawTime > 0 )
                ? (float)( currentTime - s_gfxloading.lastDrawTime ) * 0.001f
                : 0.016f;
    if ( deltaTime > 0.1f ) {
        deltaTime = 0.1f;   /* avoid a jump after a hitch */
    }
    s_gfxloading.lastDrawTime = currentTime;

    s_gfxloading.smoothProgress +=
        ( s_gfxloading.loadPercent - s_gfxloading.smoothProgress ) * ( GFX_SMOOTH_LERP_SPEED * deltaTime );
    if ( s_gfxloading.loadPercent >= 1.0f && s_gfxloading.smoothProgress > 0.995f ) {
        s_gfxloading.smoothProgress = 1.0f;
    }
}

/* -------------------------------------------------------------------------
   Hand-off
   ------------------------------------------------------------------------- */

/*
 * "Ready to race" stays up for a moment at 100%. A pending version check
 * gets a short grace period so its notice is not lost, and an open update
 * notice always waits for an answer.
 */
static void GFX_UpdateFinish( int currentTime ) {
    int since;
    int goDelay;

    if ( !s_gfxloading.finalPhase || GFX_UpdatePromptActive() || s_gfxloading.goTime ) {
        return;
    }

    since   = currentTime - s_gfxloading.finalStartTime;
    goDelay = GFX_GetPause( "ui_gfxLoadingFinalPause", GFX_GO_TIME );

    if ( !s_gfxloading.skipRequested ) {
        if ( since < goDelay || s_gfxloading.smoothProgress < 0.98f ) {
            return;
        }
        if ( GFX_UpdateCheckPending() && since < goDelay + GFX_UPDATE_WAIT_TIME ) {
            return;
        }
    }

    s_gfxloading.goTime = currentTime > 0 ? currentTime : 1;
    if ( s_gfxloading.skipRequested ) {
        s_gfxloading.goTime -= GFX_GO_HOLD;
    } else if ( s_gfxloading.goSound ) {
        trap_S_StartLocalSound( s_gfxloading.goSound, CHAN_LOCAL_SOUND );
    }
}

static qboolean GFX_HandoffDue( int currentTime ) {
    return s_gfxloading.goTime && currentTime >= s_gfxloading.goTime + GFX_GO_HOLD;
}

static void GFX_HandOffToMainMenu( void ) {
    /* the main menu keeps the music playing; UI_MainMenu resets the stack */
    uis.menuMusicCarryOver = s_gfxloading.musicStarted;
    UI_MainMenu();
}

/* -------------------------------------------------------------------------
   Drawing
   ------------------------------------------------------------------------- */

static int GFX_Stage( void ) {
    return s_gfxloading.finalPhase ? GFX_NUM_STAGES : s_gfxloading.currentStage;
}

/* ui_gfxLoadingPreview "stage,percent,prompt" pins what the screen shows, so
 * it can be compared with the WebUI in a given state (tools/ui_webstyle). */
static qboolean GFX_Preview( int *stage, int *percent, qboolean *prompt ) {
    char        value[32];
    const char *field;
    int         s, p, u = 0;

    trap_Cvar_VariableStringBuffer( "ui_gfxLoadingPreview", value, sizeof( value ) );
    if ( !value[0] || !( field = strchr( value, ',' ) ) ) {
        return qfalse;
    }
    s = atoi( value );
    p = atoi( ++field );
    if ( ( field = strchr( field, ',' ) ) != NULL ) {
        u = atoi( field + 1 );
    }
    *stage = s < 0 ? 0 : ( s > GFX_NUM_STAGES ? GFX_NUM_STAGES : s );
    *percent = p < 0 ? 0 : ( p > 100 ? 100 : p );
    *prompt = u ? qtrue : qfalse;
    if ( *stage > 1 && !s_gfxloading.carLoaded ) {
        GFXStage_Vehicle();
    }
    return qtrue;
}

/* <ol style="list-style: decimal-leading-zero"> with an outside marker */
static void GFX_DrawStages( const gfxLayout_t *l, int stage ) {
    wsTextStyle_t body = { WS_BODY, WS_REM, 0.0f, 1.5f * WS_REM, qfalse };
    char          marker[8];
    wsLines_t     lines;
    const float  *color;
    int           i;

    for ( i = 0; i < GFX_NUM_STAGES; i++ ) {
        color = i < stage ? wsWhite : ( i == stage ? wsAccent : wsMuted );
        /* ::marker uses tabular figures, which the body font carries at 0x80 */
        Com_sprintf( marker, sizeof( marker ), "%c%c. ", WS_TABULAR_DIGIT + ( i + 1 ) / 10,
                     WS_TABULAR_DIGIT + ( i + 1 ) % 10 );
        WS_DrawText( &body, l->stageX - WS_TextWidth( &body, marker, strlen( marker ) ),
                     l->stageY[i] + 0.3f * WS_REM, marker, strlen( marker ), color );
        WS_Wrap( &body, gfxStageLabels[i], l->leftW - 1.6f * WS_REM, &lines );
        WS_DrawLines( &body, l->stageX, l->stageY[i] + 0.3f * WS_REM, gfxStageLabels[i], &lines, color );
    }
}

static void GFX_BuildLayout( gfxLayout_t *l, int stage, qboolean prompt ) {
    memset( l, 0, sizeof( *l ) );
    l->prompt = prompt;
    Q_strncpyz( l->title, l->prompt ? "Update available" : gfxStageLabels[stage], sizeof( l->title ) );
    Q_strncpyz( l->vehicle, s_gfxloading.vehicleName, sizeof( l->vehicle ) );
    Q_strncpyz( l->status, "Ready for the next rally", sizeof( l->status ) );
    GFX_Layout( l, WS_ViewportWidth(), WS_ViewportHeight() );
}

static void UI_GFX_Loading_MenuDraw( void ) {
    gfxLayout_t  *l = &s_gfxloading.layout;
    wsTextStyle_t body;
    char          text[32];
    int           stage, percent;
    qboolean      prompt;

    UI_WebLoadingTick( uis.realtime );
    if ( !UI_WebLoadingScreen() ) return;

    if ( !GFX_Preview( &stage, &percent, &prompt ) ) {
        stage = GFX_Stage();
        percent = UI_WebLoadingProgress();
        prompt = GFX_UpdatePromptActive();
    }
    GFX_BuildLayout( l, stage, prompt );
    WS_DrawBackground( l->pageWidth, l->pageHeight );

    WS_DrawFrontHeader( &l->page, "Getting ", "ready.", "Loading graphics" );

    /* left: the loading panel */
    WS_DrawSection( &l->left, qfalse );
    WS_DrawLabel( l->leftX, l->loadingLabelY, gfxLoadingLabel, &l->loadingLabelLines );
    WS_DrawLines( &l->titleStyle, l->leftX, l->titleY, l->title, &l->titleLines, wsWhite );
    GFX_DrawStages( l, stage );
    WS_DrawProgress( l->progressX, l->progressY, l->progressW, percent / 100.0f );
    WS_BodyStyle( &body );
    Com_sprintf( text, sizeof( text ), "%d%%", percent );
    WS_DrawText( &body, l->leftX, l->percentY, text, strlen( text ), wsWhite );
    if ( l->prompt ) {
        WS_DrawLines( &body, l->leftX, l->promptY, gfxPromptText, &l->promptLines, wsWhite );
        WS_DrawButton( &l->buttons[0], gfxButtonLabels[0], qfalse, s_gfxloading.focusedBtn == GFX_BTN_UPDATE );
        WS_DrawButton( &l->buttons[1], gfxButtonLabels[1], qtrue, s_gfxloading.focusedBtn == GFX_BTN_SKIP );
    }

    /* right: the showroom with the active vehicle */
    WS_DrawShowroom( &l->room, l->vehicle, l->status, s_gfxloading.carLoaded ? &s_gfxloading.playerinfo : NULL,
                     WS_VIEW_AZIMUTH, WS_VIEW_ELEVATION, WS_VIEW_DISTANCE );

    Menu_Draw( &s_gfxloading.menu );
    s_gfxloading.stageShown = qtrue;
}

/* -------------------------------------------------------------------------
   Key / mouse handler

   Keys are never forwarded to Menu_DefaultKey: ESC there would pop the only
   menu on the stack, abort the remaining cache stages and skip the update
   notice.
   ------------------------------------------------------------------------- */

static gfxButton_t GFX_ButtonAtCursor( void ) {
    const gfxLayout_t *l = &s_gfxloading.layout;
    float x = uis.cursorx * uis.xscale + uis.bias;
    float y = uis.cursory * uis.yscale;
    int   i;

    for ( i = 0; i < 2; i++ ) {
        const wsRect_t *b = &l->buttons[i];
        if ( x >= b->x && x < b->x + b->w && y >= b->y && y < b->y + b->h ) {
            return (gfxButton_t)i;
        }
    }
    return GFX_BTN_NONE;
}

static sfxHandle_t UI_GFX_Loading_Key( int key ) {
    gfxButton_t button;

    if ( GFX_UpdatePromptActive() ) {
        switch ( key ) {
        case K_ESCAPE:
        case K_MOUSE2:
        case K_PAD0_B:
            GFX_ActivateButton( GFX_BTN_SKIP );
            return menu_out_sound;

        case K_LEFTARROW:
        case K_KP_LEFTARROW:
        case K_PAD0_DPAD_LEFT:
        case K_PAD0_LEFTSTICK_LEFT:
            s_gfxloading.focusedBtn = GFX_BTN_UPDATE;
            return menu_move_sound;

        case K_RIGHTARROW:
        case K_KP_RIGHTARROW:
        case K_PAD0_DPAD_RIGHT:
        case K_PAD0_LEFTSTICK_RIGHT:
            s_gfxloading.focusedBtn = GFX_BTN_SKIP;
            return menu_move_sound;

        case K_TAB:
            s_gfxloading.focusedBtn = ( s_gfxloading.focusedBtn == GFX_BTN_UPDATE )
                                      ? GFX_BTN_SKIP : GFX_BTN_UPDATE;
            return menu_move_sound;

        case K_MOUSE1:
            /* only clicks on a button count - a stray click does not dismiss */
            button = GFX_ButtonAtCursor();
            if ( button == GFX_BTN_NONE ) {
                return 0;
            }
            GFX_ActivateButton( button );
            return menu_out_sound;

        case K_ENTER:
        case K_KP_ENTER:
        case K_JOY1:
        case K_PAD0_A:
            GFX_ActivateButton( s_gfxloading.focusedBtn == GFX_BTN_NONE
                                ? GFX_BTN_UPDATE : s_gfxloading.focusedBtn );
            return menu_out_sound;

        default:
            return 0;
        }
    }

    /* Once everything is cached, confirm / back hands over at once. */
    if ( s_gfxloading.finalPhase ) {
        switch ( key ) {
        case K_ESCAPE:
        case K_ENTER:
        case K_KP_ENTER:
        case K_SPACE:
        case K_MOUSE1:
        case K_MOUSE2:
        case K_JOY1:
        case K_PAD0_A:
        case K_PAD0_B:
            s_gfxloading.skipRequested = qtrue;
            return 0;
        default:
            break;
        }
    }

    return 0;
}

/* -------------------------------------------------------------------------
   Public entry point
   ------------------------------------------------------------------------- */

void UI_GFX_Loading( void ) {
    memset( &s_gfxloading, 0, sizeof( s_gfxloading ) );

    s_gfxloading.menu.draw       = UI_GFX_Loading_MenuDraw;
    s_gfxloading.menu.key        = UI_GFX_Loading_Key;
    /* The draw function paints its own full-screen background; fullscreen
     * only tells the engine that nothing of the game shows through. */
    s_gfxloading.menu.fullscreen = qtrue;

    uis.menusp = 0;
    UI_PushMenu( &s_gfxloading.menu );
    m_entersound = qfalse;

    s_gfxloading.startTime      = trap_Milliseconds();
    s_gfxloading.stageStartTime = s_gfxloading.startTime;
    s_gfxloading.focusedBtn     = GFX_BTN_NONE;
    s_gfxloading.ackedWith      = GFX_BTN_NONE;
    s_gfxloading.updateState    = GFX_ReadUpdateState();
    s_gfxloading.goSound        = trap_S_RegisterSound( GFX_GO_SOUND, qfalse );

    GFX_ReadVehicle();

    /* the main menu takes this music over instead of starting its own */
    UI_StartMenuMusic();
    s_gfxloading.musicStarted = qtrue;
}


int UI_WebLoadingScreen( void ) {
    return uis.activemenu == &s_gfxloading.menu ? 1 : 0;
}

void UI_WebLoadingTick( int realtime ) {
    int currentTime;
    if ( !UI_WebLoadingScreen() ) return;
    uis.frametime = realtime - uis.realtime;
    uis.realtime = realtime;
    currentTime = trap_Milliseconds();
    GFX_RefreshUpdateState();
    GFX_UpdateProgress( currentTime );
    GFX_UpdateSmoothProgress( currentTime );
    GFX_UpdateFinish( currentTime );
    s_gfxloading.stageShown = qtrue;
    if ( GFX_HandoffDue( currentTime ) ) GFX_HandOffToMainMenu();
}

int UI_WebLoadingProgress( void ) {
    return (int)(s_gfxloading.smoothProgress * 100.0f);
}
