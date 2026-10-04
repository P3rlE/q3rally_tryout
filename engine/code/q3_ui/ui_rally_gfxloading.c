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

Runs the UI cache stages one by one with visual feedback, shows the result
of the online version check and offers "Update" / "Skip" when a newer
release is available. "Update" opens the download page in the browser.
===========================================================================
*/

#include "ui_local.h"
#include "ui_rally_theme.h"
#include "ui_rally_frontend.h"

/* -------------------------------------------------------------------------
   Timing
   ------------------------------------------------------------------------- */

#define GFX_MIN_STAGE_TIME       450     /* minimum milliseconds per stage */
#define GFX_FINAL_DISPLAY_TIME  1200     /* time to display 100% before transition */
#define GFX_UPDATE_WAIT_TIME    2500     /* extra wait for a still-running version check */
#define GFX_MAX_DEBUG_PAUSE    15000     /* safety clamp for test-only loading pauses */
#define GFX_SMOOTH_LERP_SPEED   4.5f     /* units per second for progress smoothing */

/* Used when version.txt does not announce a download page. */
#define GFX_DEFAULT_DOWNLOAD_URL "https://www.q3rally.com/downloads"

/* -------------------------------------------------------------------------
   Layout - positions in 640x480 virtual screen space
   ------------------------------------------------------------------------- */

#define GFX_RAIL_INSET          24      /* rail distance from the viewport edge */
#define GFX_RAIL_Y              32
#define GFX_RAIL_W             210
#define GFX_RAIL_H             410
#define GFX_RAIL_GAP            16      /* gap between rail and content panel */
#define GFX_CONTENT_Y           32
#define GFX_CONTENT_H          410
#define GFX_CONTENT_INSET       16
#define GFX_PROGRESS_Y         176
#define GFX_PROGRESS_H          14
#define GFX_PROGRESS_SEG_Y     198
#define GFX_PROGRESS_SEG_H       5
#define GFX_PROGRESS_SEGMENTS   18
#define GFX_PROGRESS_SEG_GAP     3
#define GFX_STATUS_Y           226
#define GFX_UPDATE_Y           250
#define GFX_TIP_SEPARATOR_Y    324
#define GFX_TIP_LABEL_Y        340
#define GFX_TIP_TEXT_Y         358
#define GFX_FOOTER_Y           458

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
    menuframework_s  menu;
    playerInfo_t     playerinfo;            /* vehicle loaded by the player-data stage */

    /* loading progress */
    int              currentStage;
    float            loadPercent;           /* raw progress [0.0 - 1.0] */
    float            smoothProgress;        /* smoothed value used for drawing */
    int              stageStartTime;
    int              finalDisplayStartTime; /* when 100% was reached */
    int              lastDrawTime;
    qboolean         stageExecuted;
    qboolean         finalPhase;            /* all stages done, holding 100% */
    qboolean         skipRequested;         /* player skipped the final hold */
    int              tipIndex;

    /* update notice */
    gfxUpdateState_t updateState;
    qboolean         requireUpdateAck;      /* remote version newer than local */
    qboolean         updateAcked;           /* player answered the update notice */
    gfxButton_t      hoveredBtn;            /* button under the mouse cursor */
    gfxButton_t      focusedBtn;            /* button activated by Enter / pad A */
    int              lastCursorX;
    int              lastCursorY;
} gfxloading_t;

static gfxloading_t s_gfxloading;

/* -------------------------------------------------------------------------
   Stages
   ------------------------------------------------------------------------- */

typedef struct {
    const char *name;
    void      (*exec)( void );
} gfxStage_t;

static void GFXStage_PlayerData( void ) {
    char model[MAX_QPATH];
    char rim[MAX_QPATH];
    char head[MAX_QPATH];
    char plate[MAX_QPATH];

    trap_Cvar_VariableStringBuffer( "model", model, sizeof( model ) );
    trap_Cvar_VariableStringBuffer( "rim",   rim,   sizeof( rim ) );
    trap_Cvar_VariableStringBuffer( "head",  head,  sizeof( head ) );
    trap_Cvar_VariableStringBuffer( "plate", plate, sizeof( plate ) );
    UI_PlayerInfo_SetModel( &s_gfxloading.playerinfo, model, rim, head, plate );
}

static const gfxStage_t gfxStages[] = {
    { "Initializing System...",        NULL                },
    { "Loading Setup Menu...",         UI_SetupMenu_Cache  },
    { "Caching Player Models...",      PlayerModel_Cache   },
    { "Applying Player Settings...",   PlayerSettings_Cache },
    { "Loading Control Bindings...",   Controls_Cache      },
    { "Building Arena Server List...", ArenaServers_Cache  },
    { "Configuring Player Data...",    GFXStage_PlayerData },
    { "Finalizing User Interface...",  StartServer_Cache   },
};

#define GFX_NUM_STAGES  ( (int)ARRAY_LEN( gfxStages ) )
#define GFX_READY_TEXT  "Ready to Start!"

/* -------------------------------------------------------------------------
   Loading tips
   ------------------------------------------------------------------------- */

static const char * const loadingTips[] = {
    /* basics */
    "Use the handbrake to drift through tight corners.",
    "Keep your speed up when hitting jumps.",
    "Ramming opponents can knock them off the track.",
    "Collect power-ups to gain an edge on rivals.",
    "Watch for shortcuts to shave off lap times.",

    /* driving technique */
    "Tap the brakes before a corner, not during - you'll carry more speed.",
    "Handbrake turns work best at medium speed - too fast and you'll spin out.",
    "Countersteering after a drift keeps your car pointed the right way.",
    "Land jumps with a flat car to avoid losing control on impact.",

    /* tactics & racing */
    "The inside line isn't always fastest - sometimes the outside gives better exit speed.",
    "Ramming from the side is more effective than from behind.",
    "Save your power-ups for the last lap - that's when they matter most.",
    "Watch the minimap: knowing where rivals are is half the battle.",
    "Block the racing line on the final straight to deny an overtake.",

    /* tracks & shortcuts */
    "Every track has at least one shortcut - explore before you race.",
    "Wet surfaces reduce grip earlier than you'd expect - brake sooner.",
    "Jumps are faster if you hit the ramp dead center.",
    "Cutting corners too aggressively can launch you off the track entirely.",

    /* general */
    "First place isn't safe until you cross the finish line.",
    "A well-placed ram can knock two opponents off course at once.",
    "Sometimes slowing down slightly lets you set up a much faster corner exit.",
    "Race the ladder ghosts in Ghost Race to learn the fastest lines.",
};

/* -------------------------------------------------------------------------
   Colors
   ------------------------------------------------------------------------- */

static const vec4_t gfxSeparatorColor   = UI_FRONTEND_COLOR_BORDER;
static const vec4_t gfxHeaderColor      = UI_FRONTEND_COLOR_TEXT;
static const vec4_t gfxBodyTextColor    = UI_FRONTEND_COLOR_TEXT;
static const vec4_t gfxMutedTextColor   = UI_FRONTEND_COLOR_MUTED;
static const vec4_t gfxAccentColor      = UI_FRONTEND_COLOR_ACCENT;
static const vec4_t gfxSuccessColor     = UI_THEME_COLOR_SUCCESS;
static const vec4_t gfxErrorColor       = UI_THEME_COLOR_ERROR;
static const vec4_t gfxWarningColor     = UI_FRONTEND_COLOR_STATUS;
static const vec4_t gfxProgressTrackColor = UI_FRONTEND_COLOR_PROGRESS;
static const vec4_t gfxBackdropColor    = UI_FRONTEND_COLOR_SCRIM;
static const vec4_t gfxPanelColor       = UI_FRONTEND_COLOR_PANEL;
static const vec4_t gfxHeroOverlayColor = UI_FRONTEND_COLOR_HERO_OVERLAY;

/* -------------------------------------------------------------------------
   Layout helpers (widescreen aware)
   ------------------------------------------------------------------------- */

static float GFX_ViewportLeft( void ) {
    if ( uis.xscale <= 0.0f ) {
        return 0.0f;
    }
    return -uis.bias / uis.xscale;
}

static float GFX_ViewportRight( void ) {
    return SCREEN_WIDTH - GFX_ViewportLeft();
}

static float GFX_RailX( void ) {
    return GFX_ViewportLeft() + GFX_RAIL_INSET;
}

static float GFX_ContentX( void ) {
    return GFX_RailX() + GFX_RAIL_W + GFX_RAIL_GAP;
}

static float GFX_ContentRight( void ) {
    return GFX_ViewportRight() - GFX_RAIL_INSET;
}

/* -------------------------------------------------------------------------
   Drawing helpers
   ------------------------------------------------------------------------- */

static void GFX_DrawSeparator( float x, float width, int y ) {
    UI_FillRect( x, y, width, 1, gfxSeparatorColor );
}

static void GFX_LerpColor( vec4_t out, const vec4_t from, const vec4_t to, float t ) {
    out[0] = from[0] + ( to[0] - from[0] ) * t;
    out[1] = from[1] + ( to[1] - from[1] ) * t;
    out[2] = from[2] + ( to[2] - from[2] ) * t;
    out[3] = from[3] + ( to[3] - from[3] ) * t;
}

static void GFX_DrawProgressSegments( int x, int y, int width, int height, float progress ) {
    int    usableW = width - ( GFX_PROGRESS_SEGMENTS - 1 ) * GFX_PROGRESS_SEG_GAP;
    int    i;
    vec4_t color;

    for ( i = 0; i < GFX_PROGRESS_SEGMENTS; i++ ) {
        int   segW      = usableW / GFX_PROGRESS_SEGMENTS + ( i < usableW % GFX_PROGRESS_SEGMENTS ? 1 : 0 );
        float threshold = (float)( i + 1 ) / (float)GFX_PROGRESS_SEGMENTS;

        if ( progress >= threshold ) {
            GFX_LerpColor( color, gfxAccentColor, gfxSuccessColor, threshold );
            color[3] = 0.95f;
        } else {
            Vector4Copy( gfxProgressTrackColor, color );
            color[3] = 0.58f;
        }
        UI_FillRect( x, y, segW, height, color );
        x += segW + GFX_PROGRESS_SEG_GAP;
    }
}

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
            s_gfxloading.hoveredBtn       = GFX_BTN_NONE;
            s_gfxloading.focusedBtn       = GFX_BTN_UPDATE;
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

static void GFX_ExecuteStage( void ) {
    if ( s_gfxloading.stageExecuted ) {
        return;
    }
    if ( s_gfxloading.currentStage < GFX_NUM_STAGES &&
         gfxStages[s_gfxloading.currentStage].exec ) {
        gfxStages[s_gfxloading.currentStage].exec();
    }
    s_gfxloading.stageExecuted = qtrue;
}

static void GFX_UpdateProgress( int currentTime ) {
    int stageElapsed = currentTime - s_gfxloading.stageStartTime;
    int stageTime    = GFX_GetPause( "ui_gfxLoadingStagePause", GFX_MIN_STAGE_TIME );

    GFX_ExecuteStage();

    if ( s_gfxloading.currentStage >= GFX_NUM_STAGES ) {
        return;
    }

    if ( stageElapsed >= stageTime ) {
        s_gfxloading.currentStage++;
        s_gfxloading.loadPercent    = (float)s_gfxloading.currentStage / (float)GFX_NUM_STAGES;
        s_gfxloading.stageStartTime = currentTime;
        s_gfxloading.stageExecuted  = qfalse;

        if ( s_gfxloading.currentStage >= GFX_NUM_STAGES ) {
            s_gfxloading.loadPercent           = 1.0f;
            s_gfxloading.finalPhase            = qtrue;
            s_gfxloading.finalDisplayStartTime = currentTime;
        }
    } else {
        float stageProgress = (float)stageElapsed / (float)stageTime;
        s_gfxloading.loadPercent =
            ( (float)s_gfxloading.currentStage + stageProgress ) / (float)GFX_NUM_STAGES;
    }

    if ( s_gfxloading.loadPercent > 1.0f ) {
        s_gfxloading.loadPercent = 1.0f;
    }
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

/*
 * Leave for the main menu once 100% has been shown long enough. A pending
 * version check gets a short grace period so its notice is not lost, and an
 * open update notice always waits for an answer.
 */
static qboolean GFX_ReadyForMainMenu( int currentTime ) {
    int holdTime;
    int elapsed;

    if ( !s_gfxloading.finalPhase || GFX_UpdatePromptActive() ) {
        return qfalse;
    }
    if ( s_gfxloading.skipRequested ) {
        return qtrue;
    }

    holdTime = GFX_GetPause( "ui_gfxLoadingFinalPause", GFX_FINAL_DISPLAY_TIME );
    elapsed  = currentTime - s_gfxloading.finalDisplayStartTime;

    if ( elapsed < holdTime || s_gfxloading.smoothProgress < 0.98f ) {
        return qfalse;
    }
    if ( GFX_UpdateCheckPending() && elapsed < holdTime + GFX_UPDATE_WAIT_TIME ) {
        return qfalse;
    }
    return qtrue;
}

/* -------------------------------------------------------------------------
   Update status card
   ------------------------------------------------------------------------- */

static void GFX_DrawStatusCard( int x, int y, int width, int height, const float *barColor ) {
    Frontend_DrawCard( x, y - 10, width, height, 1.0f, qfalse );
    UI_FillRect( x, y - 10, 3, height, barColor );
}

static void GFX_DrawUpdateButtons( int x, int y, int width ) {
    int         buttonY  = y + 43;
    int         skipX    = x + width - 16 - UI_FRONTEND_ACTION_WIDTH;
    int         updateX  = skipX - 8 - UI_FRONTEND_ACTION_WIDTH;
    qboolean    hoverUpdate;
    qboolean    hoverSkip;
    gfxButton_t hovered;

    hoverUpdate = Frontend_DrawButtonFocused( updateX, buttonY, UI_FRONTEND_ACTION_WIDTH,
                                              UI_FRONTEND_BUTTON_HEIGHT, "Update", 1.0f,
                                              s_gfxloading.focusedBtn == GFX_BTN_UPDATE,
                                              UI_FRONTEND_TEXT_CENTER );
    hoverSkip   = Frontend_DrawButtonFocused( skipX, buttonY, UI_FRONTEND_ACTION_WIDTH,
                                              UI_FRONTEND_BUTTON_HEIGHT, "Skip", 1.0f,
                                              s_gfxloading.focusedBtn == GFX_BTN_SKIP,
                                              UI_FRONTEND_TEXT_CENTER );

    hovered = hoverUpdate ? GFX_BTN_UPDATE : ( hoverSkip ? GFX_BTN_SKIP : GFX_BTN_NONE );
    s_gfxloading.hoveredBtn = hovered;

    /* The mouse only takes over the focus when it actually moves, so a
     * resting cursor does not fight keyboard / gamepad navigation. */
    if ( hovered != GFX_BTN_NONE &&
         ( uis.cursorx != s_gfxloading.lastCursorX || uis.cursory != s_gfxloading.lastCursorY ) ) {
        s_gfxloading.focusedBtn = hovered;
    }
    s_gfxloading.lastCursorX = uis.cursorx;
    s_gfxloading.lastCursorY = uis.cursory;
}

static void GFX_DrawUpdateCard( int x, int y, int width, int currentTime ) {
    static const char * const dots[] = { "", ".", "..", "..." };
    char remoteVersion[64];
    char remoteDate[64];
    char errorMsg[128];
    char buf[256];

    trap_Cvar_VariableStringBuffer( "cl_updateRemote", remoteVersion, sizeof( remoteVersion ) );

    switch ( s_gfxloading.updateState ) {
    case GFX_UPD_OUTDATED:
        trap_Cvar_VariableStringBuffer( "cl_updateDate", remoteDate, sizeof( remoteDate ) );
        GFX_DrawStatusCard( x, y, width, 78, gfxErrorColor );
        Frontend_DrawText( x + 14, y, "Update available", UI_LEFT | UI_SMALLFONT, gfxErrorColor );

        if ( remoteVersion[0] && remoteDate[0] ) {
            Com_sprintf( buf, sizeof( buf ), "Installed %s  /  latest %s (%s)",
                         PRODUCT_VERSION, remoteVersion, remoteDate );
        } else if ( remoteVersion[0] ) {
            Com_sprintf( buf, sizeof( buf ), "Installed %s  /  latest %s",
                         PRODUCT_VERSION, remoteVersion );
        } else {
            Com_sprintf( buf, sizeof( buf ), "Installed %s  /  latest unknown", PRODUCT_VERSION );
        }
        Frontend_DrawText( x + 14, y + 18, buf, UI_LEFT | UI_SMALLFONT, gfxWarningColor );

        if ( GFX_UpdatePromptActive() ) {
            GFX_DrawUpdateButtons( x, y, width );
        } else {
            Frontend_DrawText( x + 14, y + 43, "Update acknowledged / continuing",
                               UI_LEFT | UI_SMALLFONT, gfxSuccessColor );
        }
        break;

    case GFX_UPD_CURRENT:
        GFX_DrawStatusCard( x, y, width, 46, gfxAccentColor );
        Frontend_DrawText( x + 14, y, "System check / up to date",
                           UI_LEFT | UI_SMALLFONT, gfxSuccessColor );
        Com_sprintf( buf, sizeof( buf ), "BUILD %s%s%s", PRODUCT_VERSION,
                     remoteVersion[0] ? "  /  LATEST " : "", remoteVersion );
        Frontend_DrawText( x + 14, y + 18, buf, UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
        break;

    case GFX_UPD_AHEAD:
        GFX_DrawStatusCard( x, y, width, 46, gfxAccentColor );
        Frontend_DrawText( x + 14, y, "Development build / newer than latest release",
                           UI_LEFT | UI_SMALLFONT, gfxAccentColor );
        Com_sprintf( buf, sizeof( buf ), "BUILD %s  /  LATEST %s", PRODUCT_VERSION, remoteVersion );
        Frontend_DrawText( x + 14, y + 18, buf, UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
        break;

    case GFX_UPD_OFFLINE:
    case GFX_UPD_FAILED:
        trap_Cvar_VariableStringBuffer( "cl_updateError", errorMsg, sizeof( errorMsg ) );
        if ( !errorMsg[0] ) {
            Q_strncpyz( errorMsg,
                        s_gfxloading.updateState == GFX_UPD_OFFLINE ? "UPDATE SERVICE OFFLINE"
                                                                    : "UPDATE CHECK FAILED",
                        sizeof( errorMsg ) );
        }
        GFX_DrawStatusCard( x, y, width, 46, gfxWarningColor );
        Frontend_DrawText( x + 14, y, errorMsg, UI_LEFT | UI_SMALLFONT, gfxWarningColor );
        Frontend_DrawText( x + 14, y + 18, "Continuing without update data",
                           UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
        break;

    case GFX_UPD_UNAVAILABLE:
        GFX_DrawStatusCard( x, y, width, 46, gfxMutedTextColor );
        Frontend_DrawText( x + 14, y, "Update check unavailable",
                           UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
        Frontend_DrawText( x + 14, y + 18, "This build has no online update support",
                           UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
        break;

    case GFX_UPD_IDLE:
    case GFX_UPD_CHECKING:
    default:
        GFX_DrawStatusCard( x, y, width, 46, gfxMutedTextColor );
        Frontend_DrawText( x + 14, y, "Checking for updates",
                           UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
        Frontend_DrawText( x + 14, y + 18,
                           va( "Contacting update service%s", dots[( currentTime / 400 ) % ARRAY_LEN( dots )] ),
                           UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
        break;
    }
}

/* -------------------------------------------------------------------------
   Main draw function
   ------------------------------------------------------------------------- */

static void UI_GFX_Loading_MenuDraw( void ) {
    int         currentTime  = trap_Milliseconds();
    float       railX        = GFX_RailX();
    float       contentX     = GFX_ContentX();
    float       contentRight = GFX_ContentRight();
    float       contentWidth = contentRight - contentX;
    int         innerX       = (int)contentX + GFX_CONTENT_INSET;
    int         innerW       = (int)contentWidth - GFX_CONTENT_INSET * 2;
    int         innerRight   = (int)contentRight - GFX_CONTENT_INSET;
    int         stageNumber;
    const char *stageName;
    qboolean    ready;
    vec4_t      color;

    GFX_RefreshUpdateState();
    GFX_UpdateProgress( currentTime );
    GFX_UpdateSmoothProgress( currentTime );

    if ( s_gfxloading.currentStage < GFX_NUM_STAGES ) {
        stageNumber = s_gfxloading.currentStage + 1;
        stageName   = gfxStages[s_gfxloading.currentStage].name;
    } else {
        stageNumber = GFX_NUM_STAGES;
        stageName   = GFX_READY_TEXT;
    }
    ready = ( s_gfxloading.smoothProgress >= 1.0f ) ? qtrue : qfalse;

    /* Same full-width background, rail and hero-panel language as the main menu. */
    Vector4Copy( gfxBackdropColor, color );
    Frontend_DrawBackground( color );

    Frontend_DrawPanel( (int)railX, GFX_RAIL_Y, GFX_RAIL_W, GFX_RAIL_H,
                        1.0f, UI_FRONTEND_STYLE_SURFACE );

    UI_FillRect( contentX, GFX_CONTENT_Y, contentWidth, GFX_CONTENT_H, gfxPanelColor );
    UI_SetColor( NULL );
    UI_DrawHandlePic( contentX, GFX_CONTENT_Y, contentWidth, GFX_CONTENT_H,
                      Frontend_BackgroundShader() );
    UI_FillRect( contentX, GFX_CONTENT_Y, contentWidth, GFX_CONTENT_H, gfxHeroOverlayColor );
    Frontend_DrawPanel( (int)contentX, GFX_CONTENT_Y, (int)contentWidth,
                        GFX_CONTENT_H, 1.0f, UI_FRONTEND_STYLE_FRAME );

    /* Brand and status rail. */
    UI_FillRect( railX + 22, 50, 6, 6, gfxAccentColor );
    Frontend_DrawText( (int)railX + 38, 48, "Q3RALLY",
                       UI_LEFT | UI_BIGFONT | UI_DROPSHADOW, gfxHeaderColor );
    Frontend_DrawText( (int)railX + 22, 94, "System boot",
                       UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );

    Frontend_DrawCard( (int)railX + 14, 112, 182, 76, 1.0f, qfalse );
    Frontend_DrawText( (int)railX + 28, 124, "Startup sequence",
                       UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
    Frontend_DrawText( (int)railX + 28, 145,
                       va( "Stage %02d / %02d", stageNumber, GFX_NUM_STAGES ),
                       UI_LEFT | UI_SMALLFONT, gfxBodyTextColor );
    Frontend_DrawText( (int)railX + 28, 165,
                       va( "%.0f%% ready", s_gfxloading.smoothProgress * 100.0f ),
                       UI_LEFT | UI_SMALLFONT, gfxAccentColor );

    Frontend_DrawCard( (int)railX + 14, 348, 182, 64, 1.0f, qfalse );
    UI_FillRect( railX + 28, 363, 6, 6, ready ? gfxAccentColor : gfxWarningColor );
    Frontend_DrawText( (int)railX + 44, 358, "Frontend",
                       UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
    Frontend_DrawText( (int)railX + 44, 376, ready ? "System ready" : "Boot sequence",
                       UI_LEFT | UI_SMALLFONT, gfxBodyTextColor );
    Frontend_DrawText( (int)railX + 44, 394, "Q3Rally  -  2002-2026",
                       UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );

    /* Workspace header and progress. */
    Frontend_DrawText( innerX, 52, "System / GFX loading",
                       UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
    Frontend_DrawText( innerRight, 52, ready ? "Ready" : "Loading",
                       UI_RIGHT | UI_SMALLFONT, gfxAccentColor );
    UI_FillRect( contentRight - 10, 48, 6, 6, gfxAccentColor );

    Frontend_DrawText( innerX, 100, "Resource cache",
                       UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
    Frontend_DrawText( innerX, 122, stageName,
                       UI_LEFT | UI_SMALLFONT | UI_DROPSHADOW, gfxBodyTextColor );
    Frontend_DrawText( innerRight, 122, va( "%02d / %02d", stageNumber, GFX_NUM_STAGES ),
                       UI_RIGHT | UI_SMALLFONT, gfxMutedTextColor );
    Frontend_DrawText( innerX, GFX_STATUS_Y, "Cache progress",
                       UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
    Frontend_DrawText( innerRight, GFX_STATUS_Y,
                       va( "%.0f%%", s_gfxloading.smoothProgress * 100.0f ),
                       UI_RIGHT | UI_SMALLFONT, gfxAccentColor );

    Frontend_DrawProgress( innerX, GFX_PROGRESS_Y, innerW, GFX_PROGRESS_H,
                           s_gfxloading.smoothProgress, 1.0f );
    GFX_DrawProgressSegments( innerX, GFX_PROGRESS_SEG_Y, innerW, GFX_PROGRESS_SEG_H,
                              s_gfxloading.smoothProgress );

    GFX_DrawUpdateCard( innerX, GFX_UPDATE_Y, innerW, currentTime );

    GFX_DrawSeparator( innerX, innerW, GFX_TIP_SEPARATOR_Y );
    Frontend_DrawText( innerX, GFX_TIP_LABEL_Y, "Drive tip",
                       UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
    Frontend_DrawText( innerX, GFX_TIP_TEXT_Y, loadingTips[s_gfxloading.tipIndex],
                       UI_LEFT | UI_SMALLFONT, gfxBodyTextColor );

    Frontend_DrawText( innerX, GFX_FOOTER_Y, "Q3Rally  -  system initialization",
                       UI_LEFT | UI_SMALLFONT, gfxMutedTextColor );
    Frontend_DrawText( innerRight, GFX_FOOTER_Y, va( "BUILD %s", PRODUCT_VERSION ),
                       UI_RIGHT | UI_SMALLFONT, gfxMutedTextColor );

    Menu_Draw( &s_gfxloading.menu );

    if ( GFX_ReadyForMainMenu( currentTime ) ) {
        UI_PopMenu();
        UI_MainMenu();
        /* s_gfxloading is no longer the active menu - draw nothing more. */
    }
}

/* -------------------------------------------------------------------------
   Key / mouse handler

   Keys are never forwarded to Menu_DefaultKey: ESC there would pop the only
   menu on the stack, abort the remaining cache stages and skip the update
   notice.
   ------------------------------------------------------------------------- */

static sfxHandle_t UI_GFX_Loading_Key( int key ) {
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
            if ( s_gfxloading.hoveredBtn == GFX_BTN_NONE ) {
                return 0;
            }
            GFX_ActivateButton( s_gfxloading.hoveredBtn );
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

    /* Once everything is cached, confirm / back skips the remaining hold. */
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
    /* The draw function paints its own full-width background; fullscreen
     * only tells the engine that nothing of the game shows through. */
    s_gfxloading.menu.fullscreen = qtrue;

    uis.menusp = 0;
    UI_PushMenu( &s_gfxloading.menu );
    m_entersound = qfalse;

    s_gfxloading.stageStartTime = trap_Milliseconds();
    s_gfxloading.tipIndex       = UI_RandomInt( ARRAY_LEN( loadingTips ) );
    s_gfxloading.hoveredBtn     = GFX_BTN_NONE;
    s_gfxloading.focusedBtn     = GFX_BTN_UPDATE;
    s_gfxloading.lastCursorX    = uis.cursorx;
    s_gfxloading.lastCursorY    = uis.cursory;
    s_gfxloading.updateState    = GFX_ReadUpdateState();
}
