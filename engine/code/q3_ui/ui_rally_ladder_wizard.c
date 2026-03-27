/*
===========================================================================
Copyright (C) 2002-2026 Q3Rally Team
===========================================================================
*/
// ui_rally_ladder_wizard.c – Offline tracking registration wizard

#include "ui_local.h"

#define WIZARD_SCREEN_W     640
#define WIZARD_SCREEN_H     480
#define WIZARD_PANEL_W      460
#define WIZARD_PANEL_H      260
#define WIZARD_PANEL_X      ( ( WIZARD_SCREEN_W - WIZARD_PANEL_W ) / 2 )
#define WIZARD_PANEL_Y      ( ( WIZARD_SCREEN_H - WIZARD_PANEL_H ) / 2 )
#define WIZARD_BTN_Y        ( WIZARD_PANEL_Y + WIZARD_PANEL_H - 48 )

#define ID_WIZARD_YES       10
#define ID_WIZARD_NO        11

typedef enum { WIZARD_PAGE_CONFIRM = 0, WIZARD_PAGE_DONE } wizardPage_t;

static struct {
    menuframework_s menu;
    menutext_s      dummy;   /* invisible first item to avoid hitbox issues */
    menutext_s      btnYes;
    menutext_s      btnNo;
    wizardPage_t    page;
    char            playerName[PROFILE_MAX_NAME];
    char            serverName[64];
} s_wizard;

static vec4_t wizardBg     = { 0.08f, 0.08f, 0.12f, 0.97f };
static vec4_t wizardBorder = { 0.35f, 0.45f, 0.75f, 0.60f };
static vec4_t wizardTitle  = { 0.72f, 0.82f, 1.00f, 1.00f };
static vec4_t wizardText   = { 0.75f, 0.78f, 0.88f, 1.00f };
static vec4_t wizardAccent = { 0.50f, 0.65f, 1.00f, 1.00f };
static vec4_t colorClear   = { 0.0f,  0.0f,  0.0f,  0.0f  };

static void LadderWizard_MenuEvent( void *ptr, int event );
static void LadderWizard_Draw( void );

/* ── CVar registration ───────────────────────────────────────────────────────── */

static vmCvar_t ui_ladderWizardDismissed;

static void LadderWizard_RegisterCvars( void ) {
    trap_Cvar_Register( &ui_ladderWizardDismissed,
                        "ladder_wizard_dismissed", "0",
                        CVAR_ARCHIVE | CVAR_USERINFO );
}

/* ── MaybeShow ───────────────────────────────────────────────────────────────── */

void UI_LadderWizard_MaybeShow( void ) {
    char profileName[PROFILE_MAX_NAME];
    char apiKey[256];

    LadderWizard_RegisterCvars();
    trap_Cvar_Update( &ui_ladderWizardDismissed );

    trap_Cvar_VariableStringBuffer( "profile_active", profileName, sizeof( profileName ) );
    trap_Cvar_VariableStringBuffer( "sv_ladderApiKey", apiKey,     sizeof( apiKey ) );

    if ( !profileName[0] ) return;
    if ( apiKey[0] )       return;
    if ( ui_ladderWizardDismissed.integer != 0 ) return;

    UI_LadderWizardMenu();
}

/* ── Menu init ───────────────────────────────────────────────────────────────── */

void UI_LadderWizardMenu( void ) {
    memset( &s_wizard, 0, sizeof( s_wizard ) );

    trap_Cvar_VariableStringBuffer( "profile_active",
                                    s_wizard.playerName,
                                    sizeof( s_wizard.playerName ) );
    if ( !s_wizard.playerName[0] ) return;

    Com_sprintf( s_wizard.serverName, sizeof( s_wizard.serverName ),
                 "%s_OFFLINE", s_wizard.playerName );
    s_wizard.page = WIZARD_PAGE_CONFIRM;

    s_wizard.menu.draw       = LadderWizard_Draw;
    s_wizard.menu.fullscreen = qfalse;
    s_wizard.menu.wrapAround = qtrue;
    s_wizard.menu.showlogo   = qfalse;

    /* Invisible dummy – absorbs the automatic first-item cursor focus */
    s_wizard.dummy.generic.type  = MTYPE_PTEXT;
    s_wizard.dummy.generic.flags = QMF_INACTIVE;
    s_wizard.dummy.generic.x     = -9999;
    s_wizard.dummy.generic.y     = -9999;
    s_wizard.dummy.string        = " ";
    s_wizard.dummy.style         = UI_LEFT | UI_SMALLFONT;
    s_wizard.dummy.color         = colorClear;

    /* YES */
    s_wizard.btnYes.generic.type     = MTYPE_PTEXT;
    s_wizard.btnYes.generic.flags    = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    s_wizard.btnYes.generic.id       = ID_WIZARD_YES;
    s_wizard.btnYes.generic.callback = LadderWizard_MenuEvent;
    s_wizard.btnYes.generic.x        = WIZARD_SCREEN_W / 2 - 70;
    s_wizard.btnYes.generic.y        = WIZARD_BTN_Y;
    s_wizard.btnYes.string           = "YES";
    s_wizard.btnYes.style            = UI_CENTER | UI_SMALLFONT;
    s_wizard.btnYes.color            = wizardAccent;

    /* NO */
    s_wizard.btnNo.generic.type     = MTYPE_PTEXT;
    s_wizard.btnNo.generic.flags    = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    s_wizard.btnNo.generic.id       = ID_WIZARD_NO;
    s_wizard.btnNo.generic.callback = LadderWizard_MenuEvent;
    s_wizard.btnNo.generic.x        = WIZARD_SCREEN_W / 2 + 70;
    s_wizard.btnNo.generic.y        = WIZARD_BTN_Y;
    s_wizard.btnNo.string           = "NO";
    s_wizard.btnNo.style            = UI_CENTER | UI_SMALLFONT;
    s_wizard.btnNo.color            = wizardText;

    Menu_AddItem( &s_wizard.menu, &s_wizard.dummy );
    Menu_AddItem( &s_wizard.menu, &s_wizard.btnYes );
    Menu_AddItem( &s_wizard.menu, &s_wizard.btnNo );

    uis.transitionIn  = 0;
    uis.transitionOut = 0;

    UI_PushMenu( &s_wizard.menu );
}

/* ── Draw ────────────────────────────────────────────────────────────────────── */

static void LadderWizard_Draw( void ) {
    int cx = WIZARD_SCREEN_W / 2;
    int ty = WIZARD_PANEL_Y + 60;

    UI_FillRect( WIZARD_PANEL_X, WIZARD_PANEL_Y,
                 WIZARD_PANEL_W, WIZARD_PANEL_H, wizardBg );
    UI_DrawRect( WIZARD_PANEL_X, WIZARD_PANEL_Y,
                 WIZARD_PANEL_W, WIZARD_PANEL_H, wizardBorder );

    UI_DrawProportionalString( cx, WIZARD_PANEL_Y + 18,
                               "Q3RALLY LADDER",
                               UI_CENTER | UI_SMALLFONT, wizardTitle );

    Menu_Draw( &s_wizard.menu );

    if ( s_wizard.page == WIZARD_PAGE_CONFIRM ) {
        UI_DrawString( cx, ty,
            "Would you like to track your offline matches",
            UI_CENTER | UI_SMALLFONT, wizardText );
        UI_DrawString( cx, ty + 14,
            "on the Q3Rally Ladder?",
            UI_CENTER | UI_SMALLFONT, wizardText );
        UI_DrawString( cx, ty + 38,
            va( "Server name: %s", s_wizard.serverName ),
            UI_CENTER | UI_SMALLFONT, wizardAccent );
        UI_DrawString( cx, ty + 58,
            "Register at: ladder.q3rally.com/register.php",
            UI_CENTER | UI_SMALLFONT, wizardText );
    } else {
        UI_DrawString( cx, ty,
            "CVars prepared. Complete registration at:",
            UI_CENTER | UI_SMALLFONT, wizardText );
        UI_DrawString( cx, ty + 16,
            "ladder.q3rally.com/register.php",
            UI_CENTER | UI_SMALLFONT, wizardAccent );
        UI_DrawString( cx, ty + 38,
            "Then add to autoexec.cfg:",
            UI_CENTER | UI_SMALLFONT, wizardText );
        UI_DrawString( cx, ty + 54,
            va( "set sv_hostname \"%s\"", s_wizard.serverName ),
            UI_CENTER | UI_SMALLFONT, wizardAccent );
        UI_DrawString( cx, ty + 68,
            "set sv_ladderEnabled \"1\"",
            UI_CENTER | UI_SMALLFONT, wizardAccent );
        UI_DrawString( cx, ty + 82,
            "set sv_ladderApiKey \"your-key-here\"",
            UI_CENTER | UI_SMALLFONT, wizardAccent );
    }
}

/* ── Event handler ───────────────────────────────────────────────────────────── */

static void LadderWizard_MenuEvent( void *ptr, int event ) {
    if ( event != QM_ACTIVATED ) return;

    switch ( ( (menucommon_s *)ptr )->id ) {
    case ID_WIZARD_YES:
        if ( s_wizard.page == WIZARD_PAGE_CONFIRM ) {
            trap_Cvar_Set( "sv_ladderEnabled", "1" );
            trap_Cvar_Set( "sv_ladderUrl",
                           "https://ladder.q3rally.com/index.php/matches" );
            trap_Cvar_Set( "sv_hostname", s_wizard.serverName );
            s_wizard.page          = WIZARD_PAGE_DONE;
            s_wizard.btnYes.string = "OK";
            s_wizard.btnNo.string  = "";
        } else {
            UI_PopMenu();
        }
        break;
    case ID_WIZARD_NO:
        trap_Cvar_SetValue( "ladder_wizard_dismissed", 1 );
        trap_Cvar_Update( &ui_ladderWizardDismissed );
        UI_PopMenu();
        break;
    }
}

/* Called by engine when trap_LadderRegister succeeds (v0.8) */
void UI_LadderWizard_OnSuccess( const char *key ) {
    if ( !key || !key[0] ) return;
    trap_Cvar_Set( "sv_ladderEnabled", "1" );
    trap_Cvar_Set( "sv_ladderUrl",
                   "https://ladder.q3rally.com/index.php/matches" );
    trap_Cvar_Set( "sv_ladderApiKey", key );
    trap_Cvar_Set( "sv_hostname",     s_wizard.serverName );
}

void UI_LadderWizard_OnError( const char *msg ) {
    (void)msg;
}
