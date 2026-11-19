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
//
/*
=======================================================================

MAIN MENU

=======================================================================
*/


#include "ui_local.h"
#include "ui_rally_mainlayout.h"
#include "../game/profile_shared.h"


#define ID_SINGLEPLAYER                 10
#define ID_MULTIPLAYER                  11
#define ID_SETUP                        12
#define ID_DEMOS                        13
#define ID_CINEMATICS                   14
#define ID_MODS                         16
#define ID_RIVALS                       17
#define ID_DOWNLOADS                    19

/* keyboard focus: the navigation, then the garage button below the car */
#define MAIN_FOCUS_NONE                 -1
#define MAIN_FOCUS_GARAGE               MAIN_NAV_COUNT
#define MAIN_FOCUS_COUNT                ( MAIN_NAV_COUNT + 1 )

#define MAIN_VEHICLE_STATUS             "Ready for the next rally"

typedef struct {
        menuframework_s menu;
        mainLayout_t    layout;

        playerInfo_t    playerinfo;
        char            modelskin[MAX_QPATH];
        char            rimskin[MAX_QPATH];
        char            headskin[MAX_QPATH];
        char            plateskin[MAX_QPATH];
        char            vehicleName[64];

        int             focus;          /* MAIN_FOCUS_*, shown as :focus-visible */
        qboolean        quitOpen;       /* #quit-confirm as a modal dialog */
        int             quitFocus;      /* 0 accept, 1 cancel */
        qboolean        quitFocusVisible;

        qboolean        dragging;       /* OrbitControls on #active-vehicle-3d */
        float           dragX, dragY;
} mainmenu_t;

static mainmenu_t s_main;

/* the showroom camera, kept while the player visits the other menus */
static float s_viewYaw = WS_VIEW_AZIMUTH;
static float s_viewElevation = WS_VIEW_ELEVATION;
static float s_viewDistance = WS_VIEW_DISTANCE;

#define MAIN_RANK_NAME( name, score ) name,
static const char *s_rankNames[] = { PROFILE_RANK_TABLE( MAIN_RANK_NAME ) };
#undef MAIN_RANK_NAME

/*
=================
MainMenu_UpdateModel
=================
*/
static void MainMenu_UpdateModel( void )
{
        vec3_t  viewangles;
        vec3_t  moveangles;
        char    plate[MAX_QPATH];
        char    key[MAX_QPATH];
        char   *slash;

        VectorClear( viewangles );
        VectorClear( moveangles );

        Q_strncpyz( plate, s_main.plateskin, sizeof( plate ) );
        UI_PlayerInfo_SetModel( &s_main.playerinfo, s_main.modelskin, s_main.rimskin, s_main.headskin, plate);
        UI_PlayerInfo_SetInfo( &s_main.playerinfo, LEGS_IDLE, TORSO_STAND, viewangles, moveangles, WP_NONE, qfalse );

        Q_strncpyz( key, s_main.modelskin, sizeof( key ) );
        slash = strchr( key, '/' );
        if ( slash ) {
                *slash = '\0';
        }
        WS_VehicleName( key, s_main.vehicleName, sizeof( s_main.vehicleName ) );
}

/*
=================
MainMenu_ReadActiveVehicle
=================
*/
static void MainMenu_ReadActiveVehicle( void )
{
        trap_Cvar_VariableStringBuffer( "model", s_main.modelskin, sizeof( s_main.modelskin ) );

        if ( !s_main.modelskin[0] ) {
                Com_sprintf( s_main.modelskin, sizeof( s_main.modelskin ), "%s/%s", DEFAULT_MODEL, DEFAULT_SKIN );
        } else if ( !strchr( s_main.modelskin, '/' ) ) {
                Q_strcat( s_main.modelskin, sizeof( s_main.modelskin ), "/" DEFAULT_SKIN );
        }
}


/*
=================
MainMenu_Update
=================
*/
void MainMenu_Update( void ){

        MainMenu_ReadActiveVehicle();

        trap_Cvar_VariableStringBuffer( "rim", s_main.rimskin, sizeof( s_main.rimskin ) );
        trap_Cvar_VariableStringBuffer( "head", s_main.headskin, sizeof( s_main.headskin ) );
        trap_Cvar_VariableStringBuffer( "plate", s_main.plateskin, sizeof( s_main.plateskin ) );

        MainMenu_UpdateModel();
}

/*
=================
MainMenu_SyncActiveVehicle

The vehicle can change in the garage while this menu stays on the stack.
=================
*/
static void MainMenu_SyncActiveVehicle( void )
{
        char previousModel[MAX_QPATH];
        char rim[MAX_QPATH];
        char head[MAX_QPATH];
        char plate[MAX_QPATH];

        Q_strncpyz( previousModel, s_main.modelskin, sizeof( previousModel ) );
        MainMenu_ReadActiveVehicle();

        trap_Cvar_VariableStringBuffer( "rim", rim, sizeof( rim ) );
        trap_Cvar_VariableStringBuffer( "head", head, sizeof( head ) );
        trap_Cvar_VariableStringBuffer( "plate", plate, sizeof( plate ) );

        if ( !Q_stricmp( previousModel, s_main.modelskin ) &&
             !Q_stricmp( s_main.rimskin, rim ) &&
             !Q_stricmp( s_main.headskin, head ) &&
             !Q_stricmp( s_main.plateskin, plate ) ) {
                return;
        }

        Q_strncpyz( s_main.rimskin, rim, sizeof( s_main.rimskin ) );
        Q_strncpyz( s_main.headskin, head, sizeof( s_main.headskin ) );
        Q_strncpyz( s_main.plateskin, plate, sizeof( s_main.plateskin ) );
        MainMenu_UpdateModel();
}

/*
=================
MainMenu_Preview

ui_mainMenuPreview "name,rank,points,quit" pins the profile line and the quit
dialog, so the menu can be compared with the WebUI in a given state
(tools/ui_webstyle). Returns qfalse when the cvar is empty.
=================
*/
static qboolean MainMenu_Preview( char *name, int nameSize, int *rank, int *points, qboolean *quit ) {
        char        value[96];
        char       *field;

        trap_Cvar_VariableStringBuffer( "ui_mainMenuPreview", value, sizeof( value ) );
        if ( !value[0] || !( field = strchr( value, ',' ) ) ) {
                return qfalse;
        }
        *field++ = '\0';
        Q_strncpyz( name, value, nameSize );
        *rank = atoi( field );
        *points = 0;
        *quit = qfalse;
        if ( ( field = strchr( field, ',' ) ) != NULL ) {
                *points = atoi( ++field );
                if ( ( field = strchr( field, ',' ) ) != NULL ) {
                        *quit = atoi( field + 1 ) ? qtrue : qfalse;
                }
        }
        return qtrue;
}

/*
=================
MainMenu_ProfileSummary

#profile-summary as frontend.js writes it: name, rank and points.
=================
*/
static void MainMenu_ProfileSummary( char *out, int size ) {
        const profile_stats_t *stats = UI_Profile_GetActiveStats();
        char     name[64];
        int      rank = UI_Profile_UnlockedRank();
        int      points = stats ? stats->playerScore : 0;
        qboolean quit;

        trap_Cvar_VariableStringBuffer( "profile_active", name, sizeof( name ) );
        if ( MainMenu_Preview( name, sizeof( name ), &rank, &points, &quit ) && quit ) {
                s_main.quitOpen = qtrue;
        }
        Com_sprintf( out, size, "%s \xb7 %s \xb7 %d points", name[0] ? name : "Driver",
                     rank >= 0 && rank < (int)ARRAY_LEN( s_rankNames ) ? s_rankNames[rank] : "",
                     points );
}

/*
=================
MainMenu_ExitAction
=================
*/
static void MainMenu_ExitAction( qboolean result ) {
        if( !result ) {
                return;
        }
        UI_PopMenu();

        UI_Rally_CreditMenu();

        UI_CreditMenu();
}

/*
===============
MainMenu_ChangeMenu
===============
*/
void MainMenu_ChangeMenu( int menuId ){

        uis.mainMenu = 0;
        s_main.dragging = qfalse;

        switch( menuId) {
        case ID_SINGLEPLAYER:
                UI_StartServerMenu( qfalse );
                break;

        case ID_MULTIPLAYER:
                UI_ArenaServersMenu();
                break;

        case ID_SETUP:
                UI_SetupMenu();
                break;

        case ID_RIVALS:
                UI_BotsMenu();
                break;

        case ID_DEMOS:
                UI_DemosMenu();
                break;

        case ID_CINEMATICS:
                UI_CinematicsMenu();
                break;

        case ID_MODS:
                UI_ModsMenu();
                break;

        case ID_DOWNLOADS:
                UI_Rally_DownloadsMenu();
                break;
        }
}

/*
===============
MainMenu_Prepare
===============
*/
void MainMenu_Prepare( void ) {

        UI_Profile_MarkStatsDirty();
        MainMenu_Update();

}

/*
===============
MainMenu_Activate

A navigation button or the garage button, clicked or confirmed by key.
===============
*/
static void MainMenu_Activate( int item ) {
        s_main.dragging = qfalse;
        switch ( item ) {
        case MAIN_NAV_OFFLINE:   MainMenu_ChangeMenu( ID_SINGLEPLAYER ); break;
        case MAIN_NAV_ONLINE:    MainMenu_ChangeMenu( ID_MULTIPLAYER ); break;
        case MAIN_NAV_CONFIG:    MainMenu_ChangeMenu( ID_SETUP ); break;
        case MAIN_NAV_RIVALS:    MainMenu_ChangeMenu( ID_RIVALS ); break;
        case MAIN_NAV_DEMOS:     MainMenu_ChangeMenu( ID_DEMOS ); break;
        case MAIN_NAV_DOWNLOADS: MainMenu_ChangeMenu( ID_DOWNLOADS ); break;
        case MAIN_NAV_GARAGE:
        case MAIN_FOCUS_GARAGE:
                uis.mainMenu = 0;
                UI_GarageMenu();
                break;
        case MAIN_NAV_PROFILE:
                uis.mainMenu = 0;
                /* the driver page is native now, like the WebUI's */
                UI_WebDriverMenu();
                break;
        case MAIN_NAV_QUIT:
                s_main.quitOpen = qtrue;
                s_main.quitFocus = 1;   /* #quit-cancel has autofocus */
                break;
        }
}

/*
===============
MainMenu_Cursor / MainMenu_Inside

The cursor in screen pixels, the units of the layout.
===============
*/
static void MainMenu_Cursor( float *x, float *y ) {
        *x = uis.cursorx * uis.xscale + uis.bias;
        *y = uis.cursory * uis.yscale;
}

static qboolean MainMenu_Inside( const wsRect_t *r, float x, float y ) {
        return r->w > 0.0f && x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

static int MainMenu_ItemAt( float x, float y ) {
        int i;

        for ( i = 0; i < MAIN_NAV_COUNT; i++ ) {
                if ( MainMenu_Inside( &s_main.layout.nav[i], x, y ) ) return i;
        }
        if ( MainMenu_Inside( &s_main.layout.room.garage, x, y ) ) return MAIN_FOCUS_GARAGE;
        return MAIN_FOCUS_NONE;
}

/*
===============
MainMenu_Orbit

OrbitControls: a drag turns the camera by 2 pi per element height, the
elevation stays between 67.5 and -10 degrees.
===============
*/
static void MainMenu_Orbit( void ) {
        const wsRect_t *car = &s_main.layout.room.car;
        float x, y;

        if ( !s_main.dragging ) return;
        if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
                s_main.dragging = qfalse;
                return;
        }
        MainMenu_Cursor( &x, &y );
        if ( car->h > 0.0f ) {
                s_viewYaw = AngleNormalize360( s_viewYaw + 360.0f * ( x - s_main.dragX ) / car->h );
                s_viewElevation = WS_Clamp( WS_VIEW_MIN_ELEVATION,
                                            s_viewElevation + 360.0f * ( y - s_main.dragY ) / car->h,
                                            WS_VIEW_MAX_ELEVATION );
        }
        s_main.dragX = x;
        s_main.dragY = y;
}

/*
===============
MainMenu_DrawNav

#main-navigation: primary buttons in the accent color, the profile and
quit buttons secondary.
===============
*/
static void MainMenu_DrawNav( const mainLayout_t *l ) {
        char shown[64];
        int  i;

        for ( i = 0; i < MAIN_NAV_COUNT; i++ ) {
                const wsRect_t *b = &l->nav[i];
                qboolean secondary = i >= MAIN_NAV_PROFILE;
                float x0 = WS_Round( b->x ), y0 = WS_Round( b->y );
                float x1 = WS_Round( b->x + b->w ), y1 = WS_Round( b->y + b->h );

                WS_FillRect( x0, y0, x1 - x0, y1 - y0, secondary ? wsLine : wsAccent );
                WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, secondary ? wsSurface : wsAccent );
                WS_Transform( &l->navStyle, mainNavLabels[i], shown, sizeof( shown ) );
                WS_DrawText( &l->navStyle, b->x + l->navTextX, b->y + l->navTextY, shown, strlen( shown ),
                             secondary ? wsWhite : wsButtonText );
                if ( s_main.focus == i ) {
                        /* button:focus-visible { outline: 2px solid #fff; outline-offset: 3px } */
                        WS_FillRect( x0 - 5, y0 - 5, x1 - x0 + 10, 2, wsWhite );
                        WS_FillRect( x0 - 5, y1 + 3, x1 - x0 + 10, 2, wsWhite );
                        WS_FillRect( x0 - 5, y0 - 3, 2, y1 - y0 + 6, wsWhite );
                        WS_FillRect( x1 + 3, y0 - 3, 2, y1 - y0 + 6, wsWhite );
                }
        }
        WS_DrawLines( &l->summaryStyle, l->nav[MAIN_NAV_PROFILE].x + l->navTextX, l->summaryY,
                      l->summary, &l->summaryLines, wsSummary );
}

/*
===============
MainMenu_DrawQuit

#quit-confirm over its ::backdrop (#000b).
===============
*/
static void MainMenu_DrawQuit( const mainLayout_t *l ) {
        static const float backdrop[4] = { 0.0f, 0.0f, 0.0f, 0xbb / 255.0f };
        wsTextStyle_t body;
        float x0 = WS_Round( l->dialog.x ), y0 = WS_Round( l->dialog.y );
        float x1 = WS_Round( l->dialog.x + l->dialog.w ), y1 = WS_Round( l->dialog.y + l->dialog.h );

        WS_BodyStyle( &body );
        WS_FillRect( 0, 0, l->page.width, l->page.height, backdrop );
        WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsAccent );
        WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, wsSurface );
        WS_DrawLabel( l->dialogX, l->dialogLabelY, mainQuitLabel, &l->dialogLabelLines );
        WS_DrawLines( &l->dialogTitleStyle, l->dialogX, l->dialogTitleY, mainQuitTitle, &l->dialogTitleLines, wsWhite );
        WS_DrawLines( &body, l->dialogX, l->dialogTextY, mainQuitText, &l->dialogTextLines, wsWhite );
        WS_DrawButton( &l->accept, mainQuitAccept, qfalse, s_main.quitFocusVisible && s_main.quitFocus == 0 );
        WS_DrawButton( &l->cancel, mainQuitCancel, qtrue, s_main.quitFocusVisible && s_main.quitFocus == 1 );
}

/*
===============
Main_MenuDraw
===============
*/
static void Main_MenuDraw( void ) {
        mainLayout_t *l = &s_main.layout;

        uis.mainMenu = 1;
        MainMenu_SyncActiveVehicle();
        MainMenu_Orbit();

        Q_strncpyz( l->vehicle, s_main.vehicleName, sizeof( l->vehicle ) );
        Q_strncpyz( l->status, MAIN_VEHICLE_STATUS, sizeof( l->status ) );
        MainMenu_ProfileSummary( l->summary, sizeof( l->summary ) );
        MainMenu_Layout( l, WS_ViewportWidth(), WS_ViewportHeight() );

        WS_DrawBackground( l->pageWidth, l->pageHeight );
        WS_DrawFrontHeader( &l->page, "Ready to ", "race.", "Main menu" );
        MainMenu_DrawNav( l );
        WS_DrawShowroom( &l->room, l->vehicle, l->status, &s_main.playerinfo,
                         s_viewYaw, s_viewElevation, s_viewDistance );
        if ( s_main.focus == MAIN_FOCUS_GARAGE ) {
                WS_DrawButton( &l->room.garage, wsGarageButton, qtrue, qtrue );
        }
        if ( s_main.quitOpen ) {
                MainMenu_DrawQuit( l );
        }

        Menu_Draw( &s_main.menu );
}

/*
===============
MainMenu_QuitKey

The modal dialog: Tab and the arrows move between its two buttons, Escape
cancels it like the browser does.
===============
*/
static sfxHandle_t MainMenu_QuitKey( int key ) {
        const mainLayout_t *l = &s_main.layout;
        float x, y;

        switch ( key ) {
        case K_ESCAPE:
        case K_MOUSE2:
        case K_PAD0_B:
                s_main.quitOpen = qfalse;
                return 0;

        case K_TAB:
        case K_LEFTARROW:
        case K_RIGHTARROW:
        case K_KP_LEFTARROW:
        case K_KP_RIGHTARROW:
        case K_PAD0_DPAD_LEFT:
        case K_PAD0_DPAD_RIGHT:
        case K_PAD0_LEFTSTICK_LEFT:
        case K_PAD0_LEFTSTICK_RIGHT:
                s_main.quitFocus = !s_main.quitFocus;
                s_main.quitFocusVisible = qtrue;
                return 0;

        case K_MOUSE1:
                MainMenu_Cursor( &x, &y );
                s_main.quitFocusVisible = qfalse;
                if ( MainMenu_Inside( &l->accept, x, y ) ) {
                        s_main.quitOpen = qfalse;
                        MainMenu_ExitAction( qtrue );
                } else if ( MainMenu_Inside( &l->cancel, x, y ) ) {
                        s_main.quitOpen = qfalse;
                }
                return 0;

        case K_ENTER:
        case K_KP_ENTER:
        case K_SPACE:
        case K_JOY1:
        case K_PAD0_A:
                s_main.quitOpen = qfalse;
                if ( s_main.quitFocus == 0 ) {
                        MainMenu_ExitAction( qtrue );
                }
                return 0;
        }
        return 0;
}

/*
===============
Main_MenuKey

Buttons react to a click, Tab and the arrows move a visible focus, Enter
presses the focused button. Escape does nothing here: this is the top menu.
===============
*/
static sfxHandle_t Main_MenuKey( int key ) {
        const wsRect_t *car = &s_main.layout.room.car;
        float x, y;
        int   item;

        if ( s_main.quitOpen ) {
                return MainMenu_QuitKey( key );
        }

        switch ( key ) {
        case K_MOUSE1:
                MainMenu_Cursor( &x, &y );
                s_main.focus = MAIN_FOCUS_NONE;
                item = MainMenu_ItemAt( x, y );
                if ( item != MAIN_FOCUS_NONE ) {
                        MainMenu_Activate( item );
                } else if ( MainMenu_Inside( car, x, y ) ) {
                        s_main.dragging = qtrue;
                        s_main.dragX = x;
                        s_main.dragY = y;
                }
                return 0;

        case K_MWHEELUP:
        case K_MWHEELDOWN:
                /* OrbitControls zoom: 0.95 per notch, 2.1 to 7 radii */
                MainMenu_Cursor( &x, &y );
                if ( MainMenu_Inside( car, x, y ) ) {
                        s_viewDistance = WS_Clamp( WS_VIEW_MIN_DISTANCE,
                                                   key == K_MWHEELUP ? s_viewDistance * 0.95f : s_viewDistance / 0.95f,
                                                   WS_VIEW_MAX_DISTANCE );
                }
                return 0;

        case K_TAB:
        case K_DOWNARROW:
        case K_KP_DOWNARROW:
        case K_PAD0_DPAD_DOWN:
        case K_PAD0_LEFTSTICK_DOWN:
                s_main.focus = ( s_main.focus + 1 ) % MAIN_FOCUS_COUNT;
                return 0;

        case K_UPARROW:
        case K_KP_UPARROW:
        case K_PAD0_DPAD_UP:
        case K_PAD0_LEFTSTICK_UP:
                s_main.focus = s_main.focus <= 0 ? MAIN_FOCUS_COUNT - 1 : s_main.focus - 1;
                return 0;

        case K_ENTER:
        case K_KP_ENTER:
        case K_SPACE:
        case K_JOY1:
        case K_PAD0_A:
                if ( s_main.focus != MAIN_FOCUS_NONE ) {
                        item = s_main.focus;
                        if ( item == MAIN_NAV_QUIT ) {
                                s_main.quitFocusVisible = qtrue;
                        }
                        MainMenu_Activate( item );
                }
                return 0;
        }
        return 0;
}

/*
===============
UI_StartMenuMusic

Plays one of music/menumusic*.ogg at random.
===============
*/
void UI_StartMenuMusic( void ) {
        int numMusicFiles;
        int selectedMusic;
        char musicFiles[256][MAX_QPATH];
        char musicCommand[MAX_QPATH];

        numMusicFiles = UI_BuildFileList("music", "ogg", "menumusic", qtrue, qfalse, qfalse, 0, musicFiles);

        if (numMusicFiles > 0) {
                selectedMusic = UI_RandomInt( numMusicFiles );
                Com_sprintf(musicCommand, sizeof(musicCommand), "music music/menumusic%s\n", musicFiles[selectedMusic]);
                trap_Cmd_ExecuteText(EXEC_APPEND, musicCommand);
        }
}

/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu( void ) {
        qboolean returnToConfig;

        returnToConfig = trap_Cvar_VariableValue( "q3r_ui_return_config" ) != 0.0f;
        if ( returnToConfig ) {
                trap_Cvar_Set( "q3r_ui_return_config", "0" );
        }

        if ( uis.menuMusicCarryOver ) {
                /* started by the loading screen - let it play on */
                uis.menuMusicCarryOver = qfalse;
        } else {
                UI_StartMenuMusic();
        }

	trap_Cvar_Set( "sv_killserver", "1" );

	if( !uis.demoversion && !ui_cdkeychecked.integer ) {
		char	key[17];

		trap_GetCDKey( key, sizeof(key) );
		if( trap_VerifyCDKey( key, NULL ) == qfalse ) {
			UI_CDKeyMenu();
			return;
		}
	}

	memset( &s_main, 0 ,sizeof(mainmenu_t) );
        s_main.focus = MAIN_FOCUS_NONE;
        s_viewYaw = WS_VIEW_AZIMUTH;
        s_viewElevation = WS_VIEW_ELEVATION;
        s_viewDistance = WS_VIEW_DISTANCE;
        uis.frontendHandoff = qfalse;

        MainMenu_Prepare();

        /* The draw function paints the whole page and handles every key
         * itself; the framework only draws the cursor. */
        s_main.menu.draw = Main_MenuDraw;
        s_main.menu.key = Main_MenuKey;
        s_main.menu.fullscreen = qtrue;

        trap_Key_SetCatcher( KEYCATCH_UI );
        uis.menusp = 0;

        UI_PushMenu ( &s_main.menu );
        m_entersound = qfalse;

        {
                /* pinned states for screenshots (tools/ui_webstyle), no wizards */
                char preview[32];
                trap_Cvar_VariableStringBuffer( "ui_offlinePreview", preview, sizeof( preview ) );
                if ( preview[0] ) {
                        UI_StartServerMenu( qfalse );
                        return;
                }
                trap_Cvar_VariableStringBuffer( "ui_configPreview", preview, sizeof( preview ) );
                if ( !Q_stricmp( preview, "demosave" ) ) {
                        UI_DemoSaveMenu();
                        return;
                }
                if ( !Q_stricmp( preview, "garage" ) ) {
                        UI_GarageMenu();
                        return;
                }
                if ( preview[0] ) {
                        /* the setup page, then the buttons with these item indices ("2,3") */
                        char *press = preview;
                        UI_SetupMenu();
                        while ( press && *press ) {
                                if ( atoi( press ) > 0 || press != preview ) UI_WebConfigAction( 0, atoi( press ), 0 );
                                press = strchr( press, ',' );
                                if ( press ) press++;
                        }
                        return;
                }
                trap_Cvar_VariableStringBuffer( "ui_driverPreview", preview, sizeof( preview ) );
                if ( preview[0] ) {
                        UI_WebDriverMenu();
                        return;
                }
                trap_Cvar_VariableStringBuffer( "ui_onlinePreview", preview, sizeof( preview ) );
                if ( preview[0] ) {
                        UI_ArenaServersMenu();
                        return;
                }
                trap_Cvar_VariableStringBuffer( "ui_demosPreview", preview, sizeof( preview ) );
                if ( preview[0] ) {
                        UI_DemosMenu();
                        return;
                }
                trap_Cvar_VariableStringBuffer( "ui_downloadsPreview", preview, sizeof( preview ) );
                if ( preview[0] ) {
                        UI_Rally_DownloadsMenu();
                        return;
                }
                trap_Cvar_VariableStringBuffer( "ui_rivalsPreview", preview, sizeof( preview ) );
                if ( preview[0] ) {
                        UI_BotsMenu();
                        return;
                }
                trap_Cvar_VariableStringBuffer( "ui_creditsPreview", preview, sizeof( preview ) );
                if ( preview[0] ) {
                        UI_CreditMenu();
                        return;
                }
                trap_Cvar_VariableStringBuffer( "ui_mainMenuPreview", preview, sizeof( preview ) );
                if ( preview[0] ) {
                        return;
                }
        }

        /* Profile wizard — must run first. If no active profile exists, this
         * pushes the creation wizard on top of the main menu and blocks all
         * other auto-show logic until the player completes it.
         * UI_ProfileOverlay_MaybeShow and UI_LadderWizard_MaybeShow both
         * check for an active profile internally and will no-op when called
         * from here while the wizard is on screen. */
        UI_ProfileWizard_MaybeShow();

        UI_ProfileOverlay_MaybeShow();

        /* Q3RALLY LADDER: show offline tracking wizard for existing players
         * who update to v0.8 and have not yet registered. */
        UI_LadderWizard_MaybeShow();

        if ( returnToConfig && uis.activemenu == &s_main.menu ) {
                UI_SetupMenu();
        }
}

int UI_WebMainScreen( void ) {
        return uis.activemenu == &s_main.menu;
}

void UI_WebMainAction( int action ) {
        if ( !UI_WebMainScreen() ) return;
        /* a child menu must not start in the middle of a transition */
        uis.transitionIn = 0;
        uis.transitionOut = 0;
        uis.tFrac = 1.0f;
        switch ( action ) {
        case 1: MainMenu_ChangeMenu( ID_SINGLEPLAYER ); break;
        case 2: MainMenu_ChangeMenu( ID_MULTIPLAYER ); break;
        case 3: MainMenu_ChangeMenu( ID_SETUP ); break;
        case 4: MainMenu_ChangeMenu( ID_RIVALS ); break;
        case 5: MainMenu_ChangeMenu( ID_DEMOS ); break;
        case 6: MainMenu_ChangeMenu( ID_DOWNLOADS ); break;
        case 7: UI_WebDriverMenu(); break;
        case 8: if ( UI_Profile_HasActiveProfile() ) UI_PlayerStatsMenu(); break;
        case 9: MainMenu_Activate( MAIN_NAV_QUIT ); break;
        case 13: MainMenu_ExitAction( qtrue ); break;
        }
}
