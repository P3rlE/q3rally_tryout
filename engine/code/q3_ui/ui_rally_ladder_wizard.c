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
#define ID_OWNER_NAME       12
#define ID_OWNER_EMAIL      13

typedef enum { WIZARD_PAGE_CONFIRM = 0, WIZARD_PAGE_DONE } wizardPage_t;
typedef enum {
    WIZARD_RESULT_NONE = 0,
    WIZARD_RESULT_PENDING,
    WIZARD_RESULT_SUCCESS,
    WIZARD_RESULT_ERROR
} wizardResult_t;

static struct {
    menuframework_s menu;
    menutext_s      dummy;   /* invisible first item to avoid hitbox issues */
    menufield_s     ownerName;
    menufield_s     ownerEmail;
    menutext_s      btnYes;
    menutext_s      btnNo;
    wizardPage_t    page;
    wizardResult_t  result;
    char            playerName[PROFILE_MAX_NAME];
    char            serverName[64];
    char            apiKey[256];
    char            statusLine[128];
} s_wizard;

static vec4_t wizardBg     = { 0.08f, 0.08f, 0.12f, 0.97f };
static vec4_t wizardDim    = { 0.00f, 0.00f, 0.00f, 0.45f };
static vec4_t wizardBorder = { 0.35f, 0.45f, 0.75f, 0.60f };
static vec4_t wizardTitle  = { 0.72f, 0.82f, 1.00f, 1.00f };
static vec4_t wizardText   = { 0.75f, 0.78f, 0.88f, 1.00f };
static vec4_t wizardAccent = { 0.50f, 0.65f, 1.00f, 1.00f };
static vec4_t wizardError  = { 1.00f, 0.42f, 0.42f, 1.00f };
static vec4_t colorClear   = { 0.0f,  0.0f,  0.0f,  0.0f  };

static void LadderWizard_MenuEvent( void *ptr, int event );
static void LadderWizard_Draw( void );
static void LadderWizard_UpdateButtons( void );

/* ── CVar registration ───────────────────────────────────────────────────────── */

static vmCvar_t ui_ladderWizardDismissed;

static void LadderWizard_RegisterCvars( void ) {
    trap_Cvar_Register( &ui_ladderWizardDismissed,
                        "ladder_wizard_dismissed", "0",
                        CVAR_ARCHIVE | CVAR_USERINFO );
}

static qboolean LadderWizard_ValidateEmail( const char *email ) {
    const char *at;

    if ( !email || !email[0] ) {
        return qfalse;
    }

    at = strchr( email, '@' );
    if ( !at || at == email || !at[1] ) {
        return qfalse;
    }

    if ( !strchr( at + 1, '.' ) ) {
        return qfalse;
    }

    return qtrue;
}

static void LadderWizard_SanitizeArg( const char *src, char *dst, size_t dstSize ) {
    size_t i;
    size_t j;

    if ( !dst || !dstSize ) {
        return;
    }

    if ( !src ) {
        dst[0] = '\0';
        return;
    }

    for ( i = 0, j = 0; src[i] && j + 1 < dstSize; ++i ) {
        if ( src[i] == '"' || src[i] == '\\' || src[i] == '\n' || src[i] == '\r' ) {
            continue;
        }
        dst[j++] = src[i];
    }

    dst[j] = '\0';
}

static void LadderWizard_StartRegistration( void ) {
    char ownerName[64];
    char ownerEmail[128];
    char serverName[64];
    char cmd[384];

    Q_strncpyz( ownerName, s_wizard.ownerName.field.buffer, sizeof( ownerName ) );
    Q_strncpyz( ownerEmail, s_wizard.ownerEmail.field.buffer, sizeof( ownerEmail ) );

    if ( !ownerName[0] ) {
        Q_strncpyz( s_wizard.statusLine, "Please enter an owner name.", sizeof( s_wizard.statusLine ) );
        return;
    }

    if ( !LadderWizard_ValidateEmail( ownerEmail ) ) {
        Q_strncpyz( s_wizard.statusLine, "Please enter a valid owner email.", sizeof( s_wizard.statusLine ) );
        return;
    }

    LadderWizard_SanitizeArg( ownerName, ownerName, sizeof( ownerName ) );
    LadderWizard_SanitizeArg( ownerEmail, ownerEmail, sizeof( ownerEmail ) );
    LadderWizard_SanitizeArg( s_wizard.serverName, serverName, sizeof( serverName ) );

    if ( !ownerName[0] || !ownerEmail[0] || !serverName[0] ) {
        Q_strncpyz( s_wizard.statusLine, "Invalid registration data.", sizeof( s_wizard.statusLine ) );
        return;
    }

    trap_Cvar_Set( "sv_ladderEnabled", "1" );
    trap_Cvar_Set( "sv_ladderUrl", "https://ladder.q3rally.com/index.php/matches" );
    trap_Cvar_Set( "sv_hostname", s_wizard.serverName );

    Com_sprintf( cmd, sizeof( cmd ),
                 "ladder_register \"%s\" \"%s\" \"%s\"\n",
                 ownerName, ownerEmail, serverName );
    trap_Cmd_ExecuteText( EXEC_APPEND, cmd );

    s_wizard.result = WIZARD_RESULT_PENDING;
    Q_strncpyz( s_wizard.statusLine,
                "Submitting registration...",
                sizeof( s_wizard.statusLine ) );
    LadderWizard_UpdateButtons();
}

static void LadderWizard_UpdateButtons( void ) {
    if ( s_wizard.page == WIZARD_PAGE_CONFIRM ) {
        s_wizard.btnYes.string = "REGISTER";
        s_wizard.btnNo.string  = "CANCEL";

        s_wizard.ownerName.generic.flags = ( s_wizard.result == WIZARD_RESULT_PENDING )
            ? QMF_INACTIVE
            : 0;
        s_wizard.ownerEmail.generic.flags = ( s_wizard.result == WIZARD_RESULT_PENDING )
            ? QMF_INACTIVE
            : 0;

        s_wizard.btnYes.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
        if ( s_wizard.result == WIZARD_RESULT_PENDING ) {
            s_wizard.btnYes.generic.flags |= QMF_INACTIVE;
            s_wizard.btnNo.string = "ABORT";
        }
        s_wizard.btnNo.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
        return;
    }

    if ( s_wizard.result == WIZARD_RESULT_SUCCESS ) {
        s_wizard.btnYes.string = "OK";
        s_wizard.btnNo.string  = "";
        s_wizard.btnNo.generic.flags = QMF_INACTIVE | QMF_HIDDEN;
    } else {
        s_wizard.btnYes.string = "RETRY";
        s_wizard.btnNo.string  = "BACK";
        s_wizard.btnNo.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    }

    s_wizard.btnYes.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
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
    s_wizard.result = WIZARD_RESULT_NONE;

    Q_strncpyz( s_wizard.ownerName.field.buffer, s_wizard.playerName,
                sizeof( s_wizard.ownerName.field.buffer ) );

    s_wizard.menu.draw       = LadderWizard_Draw;
    s_wizard.menu.transparent = qtrue;
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

    s_wizard.ownerName.generic.type = MTYPE_FIELD;
    s_wizard.ownerName.generic.flags = 0;
    s_wizard.ownerName.generic.id = ID_OWNER_NAME;
    s_wizard.ownerName.generic.x = WIZARD_PANEL_X + 172;
    s_wizard.ownerName.generic.y = WIZARD_PANEL_Y + 112;
    s_wizard.ownerName.field.widthInChars = 22;
    s_wizard.ownerName.field.maxchars = 32;

    s_wizard.ownerEmail.generic.type = MTYPE_FIELD;
    s_wizard.ownerEmail.generic.flags = 0;
    s_wizard.ownerEmail.generic.id = ID_OWNER_EMAIL;
    s_wizard.ownerEmail.generic.x = WIZARD_PANEL_X + 172;
    s_wizard.ownerEmail.generic.y = WIZARD_PANEL_Y + 134;
    s_wizard.ownerEmail.field.widthInChars = 22;
    s_wizard.ownerEmail.field.maxchars = 64;

    /* YES */
    s_wizard.btnYes.generic.type     = MTYPE_PTEXT;
    s_wizard.btnYes.generic.flags    = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    s_wizard.btnYes.generic.id       = ID_WIZARD_YES;
    s_wizard.btnYes.generic.callback = LadderWizard_MenuEvent;
    s_wizard.btnYes.generic.x        = WIZARD_SCREEN_W / 2 - 70;
    s_wizard.btnYes.generic.y        = WIZARD_BTN_Y;
    s_wizard.btnYes.string           = "REGISTER";
    s_wizard.btnYes.style            = UI_CENTER | UI_SMALLFONT;
    s_wizard.btnYes.color            = wizardAccent;

    /* NO */
    s_wizard.btnNo.generic.type     = MTYPE_PTEXT;
    s_wizard.btnNo.generic.flags    = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    s_wizard.btnNo.generic.id       = ID_WIZARD_NO;
    s_wizard.btnNo.generic.callback = LadderWizard_MenuEvent;
    s_wizard.btnNo.generic.x        = WIZARD_SCREEN_W / 2 + 70;
    s_wizard.btnNo.generic.y        = WIZARD_BTN_Y;
    s_wizard.btnNo.string           = "CANCEL";
    s_wizard.btnNo.style            = UI_CENTER | UI_SMALLFONT;
    s_wizard.btnNo.color            = wizardText;

    Menu_AddItem( &s_wizard.menu, &s_wizard.dummy );
    Menu_AddItem( &s_wizard.menu, &s_wizard.ownerName );
    Menu_AddItem( &s_wizard.menu, &s_wizard.ownerEmail );
    Menu_AddItem( &s_wizard.menu, &s_wizard.btnYes );
    Menu_AddItem( &s_wizard.menu, &s_wizard.btnNo );

    LadderWizard_UpdateButtons();

    uis.transitionIn  = 0;
    uis.transitionOut = 0;

    UI_PushMenu( &s_wizard.menu );
}

/* ── Draw ────────────────────────────────────────────────────────────────────── */

static void LadderWizard_Draw( void ) {
    int cx = WIZARD_SCREEN_W / 2;
    int ty = WIZARD_PANEL_Y + 60;

    /* keep underlying main menu visible, but dim it for focus */
    UI_FillRect( 0, 0, WIZARD_SCREEN_W, WIZARD_SCREEN_H, wizardDim );

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
            "Moechtest du Offline-Matches auf der Ladder tracken?",
            UI_CENTER | UI_SMALLFONT, wizardText );
        UI_DrawString( cx, ty + 16,
            va( "Server name: %s", s_wizard.serverName ),
            UI_CENTER | UI_SMALLFONT, wizardAccent );

        UI_DrawString( WIZARD_PANEL_X + 78, WIZARD_PANEL_Y + 120,
            "Owner:", UI_LEFT | UI_SMALLFONT, wizardText );
        UI_DrawString( WIZARD_PANEL_X + 78, WIZARD_PANEL_Y + 142,
            "Email:", UI_LEFT | UI_SMALLFONT, wizardText );

        if ( s_wizard.statusLine[0] ) {
            UI_DrawString( cx, ty + 96,
                s_wizard.statusLine,
                UI_CENTER | UI_SMALLFONT,
                s_wizard.result == WIZARD_RESULT_PENDING ? wizardText : wizardError );
        }
    } else if ( s_wizard.result == WIZARD_RESULT_SUCCESS ) {
        UI_DrawString( cx, ty,
            "Registrierung erfolgreich.",
            UI_CENTER | UI_SMALLFONT, wizardAccent );
        UI_DrawString( cx, ty + 16,
            "API-Key wurde gespeichert (sv_ladderApiKey).",
            UI_CENTER | UI_SMALLFONT, wizardText );
        UI_DrawString( cx, ty + 38,
            "Naechste Schritte:",
            UI_CENTER | UI_SMALLFONT, wizardText );
        UI_DrawString( cx, ty + 54,
            va( "set sv_hostname \"%s\"", s_wizard.serverName ),
            UI_CENTER | UI_SMALLFONT, wizardAccent );
        UI_DrawString( cx, ty + 68,
            "set sv_ladderEnabled \"1\"",
            UI_CENTER | UI_SMALLFONT, wizardAccent );
        if ( s_wizard.apiKey[0] ) {
            UI_DrawString( cx, ty + 82,
                va( "Key: %.28s...", s_wizard.apiKey ),
                UI_CENTER | UI_SMALLFONT, wizardText );
        }
    } else {
        UI_DrawString( cx, ty,
            "Registrierung fehlgeschlagen.",
            UI_CENTER | UI_SMALLFONT, wizardError );
        if ( s_wizard.statusLine[0] ) {
            UI_DrawString( cx, ty + 16,
                s_wizard.statusLine,
                UI_CENTER | UI_SMALLFONT, wizardText );
        }
        UI_DrawString( cx, ty + 46,
            "Bitte pruefe Daten/Verbindung und versuche es erneut.",
            UI_CENTER | UI_SMALLFONT, wizardText );
    }
}

/* ── Event handler ───────────────────────────────────────────────────────────── */

static void LadderWizard_MenuEvent( void *ptr, int event ) {
    if ( event != QM_ACTIVATED ) return;

    switch ( ( (menucommon_s *)ptr )->id ) {
    case ID_WIZARD_YES:
        if ( s_wizard.page == WIZARD_PAGE_CONFIRM ) {
            if ( s_wizard.result != WIZARD_RESULT_PENDING ) {
                LadderWizard_StartRegistration();
            }
        } else if ( s_wizard.result == WIZARD_RESULT_SUCCESS ) {
            UI_PopMenu();
        } else {
            s_wizard.page = WIZARD_PAGE_CONFIRM;
            s_wizard.result = WIZARD_RESULT_NONE;
            s_wizard.statusLine[0] = '\0';
            LadderWizard_UpdateButtons();
        }
        break;

    case ID_WIZARD_NO:
        if ( s_wizard.page == WIZARD_PAGE_CONFIRM ) {
            trap_Cvar_SetValue( "ladder_wizard_dismissed", 1 );
            trap_Cvar_Update( &ui_ladderWizardDismissed );
        }
        UI_PopMenu();
        break;
    }
}

/* Called by engine when ladder_register succeeds */
void UI_LadderWizard_OnSuccess( const char *key ) {
    if ( !key || !key[0] ) {
        UI_LadderWizard_OnError( "Server returned no API key." );
        return;
    }

    trap_Cvar_Set( "sv_ladderEnabled", "1" );
    trap_Cvar_Set( "sv_ladderUrl", "https://ladder.q3rally.com/index.php/matches" );
    trap_Cvar_Set( "sv_ladderApiKey", key );
    trap_Cvar_Set( "sv_hostname", s_wizard.serverName );

    Q_strncpyz( s_wizard.apiKey, key, sizeof( s_wizard.apiKey ) );
    Q_strncpyz( s_wizard.statusLine, "", sizeof( s_wizard.statusLine ) );
    s_wizard.page = WIZARD_PAGE_DONE;
    s_wizard.result = WIZARD_RESULT_SUCCESS;
    LadderWizard_UpdateButtons();
    Menu_SetCursorToItem( &s_wizard.menu, &s_wizard.btnYes );
}

void UI_LadderWizard_OnError( const char *msg ) {
    if ( msg && msg[0] ) {
        Q_strncpyz( s_wizard.statusLine, msg, sizeof( s_wizard.statusLine ) );
    } else {
        Q_strncpyz( s_wizard.statusLine,
                    "Registration request failed.",
                    sizeof( s_wizard.statusLine ) );
    }

    s_wizard.page = WIZARD_PAGE_DONE;
    s_wizard.result = WIZARD_RESULT_ERROR;
    LadderWizard_UpdateButtons();
    Menu_SetCursorToItem( &s_wizard.menu, &s_wizard.btnYes );
}
