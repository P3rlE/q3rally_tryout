/*
=======================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2024 Q3Rally Team (Per Thormann - q3rally@gmail.com)

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
=======================================================================
*/

#include "ui_local.h"
#include "../game/bg_public.h"
#include "../game/q3r_profile.h"

#define PROFILES_PATH          "profiles"
#define PROFILE_EXTENSION      ".dat"

#define ID_PROFILE_LIST        110
#define ID_PROFILE_CREATE      111
#define ID_PROFILE_DELETE      112
#define ID_PROFILE_CONTINUE    113

#define PROFILE_PANEL_W        600
#define PROFILE_PANEL_H        440
#define PROFILE_PANEL_X        ( 320 - ( PROFILE_PANEL_W / 2 ) )
#define PROFILE_PANEL_Y        20

#define PROFILE_LIST_WIDTH     30
#define PROFILE_LIST_HEIGHT    9

#define PROFILE_FIELD_WIDTH    18

#define PROFILE_BUTTON_WIDTH   260
#define PROFILE_BUTTON_HEIGHT  24

#define ID_PROFILE_NAME        114

#define PROFILE_MIN_SELECTIONS 0
#define PROFILE_SLOTS_CVAR     "ui_profile_slots"

const achievement_def_t achievement_defs[ACH_MAX] = {
    { ACH_FIRST_RACE_FINISHED, "first_race", "First Finish!", "Finish your first race.", ACH_TYPE_ONE_SHOT, 1.0f },
    { ACH_10_RACES_FINISHED, "10_races", "Veteran Racer", "Finish 10 races.", ACH_TYPE_PROGRESS, 10.0f },
    { ACH_100KM_DRIVEN, "100km_driven", "Long Distance Driver", "Drive 100 km total.", ACH_TYPE_PROGRESS, 100000.0f },
    { ACH_10_WINS, "10_wins", "Champion", "Win 10 races.", ACH_TYPE_PROGRESS, 10.0f },
    { ACH_DERBY_SPECIALIST, "derby_specialist", "Derby Specialist", "Win 5 Derby matches.", ACH_TYPE_PROGRESS, 5.0f }
};

typedef struct {
    menuframework_s menu;
    menulist_s      list;
    menufield_s     nameField;
    menutext_s      create;
    menutext_s      deleteBtn;
    menutext_s      continueBtn;
    menubitmap_s    capture;

    char            profileNames[UI_MAX_PROFILE_SLOTS][MAX_QPATH];
    const char     *profileItems[UI_MAX_PROFILE_SLOTS];
    int             profileCount;
    int             lastSelection;

    char            createLabel[64];
    char            continueLabel[64];
} profileOverlay_t;

static profileOverlay_t s_profileOverlay;
static qboolean         s_profileOverlayInitialized;
static qboolean         s_profileOverlayShownThisSession;
q3r_profile_t    cg_profile;

#define MAX_NOTIFICATIONS 4
#define NOTIFICATION_DURATION 4000

typedef struct {
    achievement_id_t id;
    int startTime;
} achievement_notification_t;

static achievement_notification_t achievement_notifications[MAX_NOTIFICATIONS];
static int notification_count = 0;

static const vec4_t profileOverlayBackdrop = { 0.0f, 0.0f, 0.0f, 0.75f };
static const vec4_t profileOverlayPanel    = { 0.08f, 0.08f, 0.08f, 0.92f };

static void UI_ProfileOverlay_InitMenu( void );
static void UI_ProfileOverlay_RebuildList( void );
static void UI_ProfileOverlay_CreateFromField( void );
static void UI_ProfileOverlay_UpdateSelection( int index );
static void UI_ProfileOverlay_UpdateButtonStates( void );
static void UI_ProfileOverlay_UpdateCreateLabel( void );
static qboolean UI_ProfileOverlay_ShouldForcePrompt( void );
static void UI_ProfileOverlay_EnsureSelection( void );
static void UI_ProfileOverlay_Show( qboolean forceCreate );
static void UI_ProfileOverlay_ResetNameField( void );
static void UI_ProfileOverlay_GetSanitizedFieldText( char *output, size_t size );
static void UI_ProfileOverlay_UpdateButtonBounds( menutext_s *text );
static qboolean Q3R_Profile_Load( const char *name );
void Q3R_AchievementNotify_Push(achievement_id_t id);

void Q3R_Achievements_Unlock(achievement_id_t id) {
    if (cg_profile.achievements[id].unlocked) return;
    cg_profile.achievements[id].unlocked = qtrue;
    cg_profile.achievements[id].unlockTime = trap_Milliseconds();
    Q3R_Profile_Save(&cg_profile);
    Q3R_AchievementNotify_Push(id);
}

void Q3R_Achievements_UpdateProgress(achievement_id_t id, float progress) {
    if (cg_profile.achievements[id].unlocked) return;
    cg_profile.achievements[id].progress += progress;
    if (cg_profile.achievements[id].progress >= achievement_defs[id].target) {
        Q3R_Achievements_Unlock(id);
    }
}

void Q3R_Profile_InitDefaults(q3r_profile_t *profile, const char *name) {
    if (!profile) return;
    Com_Memset(profile, 0, sizeof(q3r_profile_t));
    profile->version = Q3R_PROFILE_VERSION;
    Q_strncpyz(profile->playerName, name, sizeof(profile->playerName));
    // Initialize other fields to default values
}

void Q3R_AchievementNotify_Push(achievement_id_t id) {
    if (notification_count < MAX_NOTIFICATIONS) {
        achievement_notifications[notification_count].id = id;
        achievement_notifications[notification_count].startTime = trap_Milliseconds();
        notification_count++;
    }
}

void Q3R_AchievementNotify_Update(void) {
    int i = 0;
    while (i < notification_count) {
        if (trap_Milliseconds() - achievement_notifications[i].startTime > NOTIFICATION_DURATION) {
            // Remove notification
            notification_count--;
            for (int j = i; j < notification_count; j++) {
                achievement_notifications[j] = achievement_notifications[j+1];
            }
        } else {
            i++;
        }
    }
}

void Q3R_AchievementNotify_Draw(void) {
    Q3R_AchievementNotify_Update();
    for (int i = 0; i < notification_count; i++) {
        const achievement_def_t *def = &achievement_defs[achievement_notifications[i].id];
        UI_DrawProportionalString(320, 100 + i * 40, va("Achievement Unlocked: %s", def->title), UI_CENTER | UI_SMALLFONT, colorWhite);
    }
}

qboolean Q3R_Profile_Save(const q3r_profile_t *profile) {
    fileHandle_t f;
    char path[MAX_QPATH];

    if (!profile) return qfalse;

    Com_sprintf(path, sizeof(path), "%s/%s%s", PROFILES_PATH, profile->playerName, PROFILE_EXTENSION);
    if (trap_FS_FOpenFile(path, &f, FS_WRITE) < 0 || !f) {
        Com_Printf("Failed to open profile for writing: %s\n", path);
        return qfalse;
    }

    trap_FS_Write(Q3R_PROFILE_HEADER, strlen(Q3R_PROFILE_HEADER), f);
    trap_FS_Write(profile, sizeof(q3r_profile_t), f);
    trap_FS_FCloseFile(f);
    return qtrue;
}

static void UI_ProfileOverlay_Sanitize( const char *input, char *output, size_t size ) {
    size_t length = 0;

    if ( !output || !size ) {
        return;
    }

    output[0] = '\0';

    if ( !input ) {
        return;
    }

    while ( *input && length + 1 < size ) {
        char c = *input++;

        if ( ( c >= 'a' && c <= 'z' ) ||
             ( c >= 'A' && c <= 'Z' ) ||
             ( c >= '0' && c <= '9' ) ||
             c == '-' || c == '_' || c == '.' ) {
            output[length++] = c;
        }
    }

    output[length] = '\0';
}

static void UI_ProfileOverlay_BuildPlayerProfileName( char *output, size_t size ) {
    char raw[MAX_QPATH];
    char cleaned[MAX_QPATH];

    if ( !output || !size ) {
        return;
    }

    trap_Cvar_VariableStringBuffer( "name", raw, sizeof( raw ) );
    Q_strncpyz( cleaned, raw, sizeof( cleaned ) );
    Q_CleanStr( cleaned );
    UI_ProfileOverlay_Sanitize( cleaned, output, size );

    if ( !output[0] ) {
        Q_strncpyz( output, "Player", size );
    }
}

static int UI_ProfileOverlay_FindIndex( const char *name ) {
    int i;

    if ( !name || !name[0] ) {
        return -1;
    }

    for ( i = 0; i < s_profileOverlay.profileCount; ++i ) {
        if ( !Q_stricmp( s_profileOverlay.profileNames[i], name ) ) {
            return i;
        }
    }

    return -1;
}

static void UI_ProfileOverlay_UpdateButtonStates( void ) {
    if ( s_profileOverlay.profileCount <= PROFILE_MIN_SELECTIONS ) {
        s_profileOverlay.deleteBtn.generic.flags |= QMF_GRAYED;
    } else {
        s_profileOverlay.deleteBtn.generic.flags &= ~QMF_GRAYED;
    }
}

static void UI_ProfileOverlay_UpdateButtonBounds( menutext_s *text ) {
    int halfWidth;

    if ( !text ) {
        return;
    }

    halfWidth = PROFILE_BUTTON_WIDTH / 2;
    text->generic.left = text->generic.x - halfWidth;
    text->generic.right = text->generic.x + halfWidth;
    text->generic.top = text->generic.y - 8;
    text->generic.bottom = text->generic.y + PROFILE_BUTTON_HEIGHT;
}

static void UI_ProfileOverlay_GetSanitizedFieldText( char *output, size_t size ) {
    char cleaned[MAX_QPATH];

    if ( !output || !size ) {
        return;
    }

    output[0] = '\0';

    Q_strncpyz( cleaned, s_profileOverlay.nameField.field.buffer, sizeof( cleaned ) );
    Q_CleanStr( cleaned );
    UI_ProfileOverlay_Sanitize( cleaned, output, size );
}

static void UI_ProfileOverlay_AddProfileName( const char *name ) {
    char sanitized[MAX_QPATH];

    if ( s_profileOverlay.profileCount >= UI_MAX_PROFILE_SLOTS ) {
        return;
    }

    UI_ProfileOverlay_Sanitize( name, sanitized, sizeof( sanitized ) );
    if ( !sanitized[0] ) {
        return;
    }

    if ( UI_ProfileOverlay_FindIndex( sanitized ) >= 0 ) {
        return;
    }

    Q_strncpyz( s_profileOverlay.profileNames[s_profileOverlay.profileCount], sanitized,
                sizeof( s_profileOverlay.profileNames[0] ) );
    s_profileOverlay.profileItems[s_profileOverlay.profileCount] =
        s_profileOverlay.profileNames[s_profileOverlay.profileCount];
    s_profileOverlay.profileCount++;
}

static void UI_ProfileOverlay_RebuildList( void ) {
    char file_list[4096];
    char *filename;
    int num_files, i;

    s_profileOverlay.profileCount = 0;
    num_files = trap_FS_GetFileList(PROFILES_PATH, PROFILE_EXTENSION, file_list, sizeof(file_list));
    filename = file_list;

    for (i = 0; i < num_files; i++) {
        int len = strlen(filename);
        if (len > 0) {
            char name[MAX_QPATH];
            Q_strncpyz(name, filename, sizeof(name));
            name[len - strlen(PROFILE_EXTENSION)] = '\0'; // Strip extension
            UI_ProfileOverlay_AddProfileName(name);
            filename += len + 1;
        }
    }

    s_profileOverlay.list.numitems = s_profileOverlay.profileCount;
    if (s_profileOverlay.list.curvalue >= s_profileOverlay.profileCount) {
        s_profileOverlay.list.curvalue = s_profileOverlay.profileCount > 0 ? s_profileOverlay.profileCount - 1 : 0;
    }

    UI_ProfileOverlay_UpdateButtonStates();
}

static void UI_ProfileOverlay_UpdateSelection( int index ) {
    if ( index < 0 || index >= s_profileOverlay.profileCount ) {
        s_profileOverlay.continueBtn.generic.flags |= QMF_GRAYED;
        Q_strncpyz( s_profileOverlay.continueLabel, "CONTINUE", sizeof( s_profileOverlay.continueLabel ) );
        UI_ProfileOverlay_UpdateButtonBounds( &s_profileOverlay.continueBtn );
        trap_Cvar_Set( "cg_profile", "" );
        trap_Cvar_Set( "profile", "" );
        trap_Cvar_Set( "ui_profileSelected", "" );
        s_profileOverlay.lastSelection = -1;
        return;
    }

    s_profileOverlay.list.curvalue = index;

    if (Q3R_Profile_Load(s_profileOverlay.profileNames[index])) {
        trap_Cvar_Set("name", cg_profile.playerName);
    }

    Q_strncpyz( s_profileOverlay.nameField.field.buffer, s_profileOverlay.profileNames[index],
                sizeof( s_profileOverlay.nameField.field.buffer ) );
    s_profileOverlay.nameField.field.cursor = strlen( s_profileOverlay.nameField.field.buffer );
    s_profileOverlay.nameField.field.scroll = 0;

    Com_sprintf( s_profileOverlay.continueLabel, sizeof( s_profileOverlay.continueLabel ),
                 "CONTINUE (%s)", s_profileOverlay.profileNames[index] );
    s_profileOverlay.continueBtn.generic.flags &= ~QMF_GRAYED;
    UI_ProfileOverlay_UpdateButtonBounds( &s_profileOverlay.continueBtn );
    s_profileOverlay.lastSelection = index;
}

static void UI_ProfileOverlay_UpdateCreateLabel( void ) {
    char sanitized[MAX_QPATH];

    UI_ProfileOverlay_GetSanitizedFieldText( sanitized, sizeof( sanitized ) );

    if ( sanitized[0] ) {
        Com_sprintf( s_profileOverlay.createLabel, sizeof( s_profileOverlay.createLabel ),
                     "CREATE PROFILE (%s)", sanitized );
        s_profileOverlay.create.generic.flags &= ~QMF_GRAYED;
    } else {
        Q_strncpyz( s_profileOverlay.createLabel, "ENTER A VALID PROFILE NAME", sizeof( s_profileOverlay.createLabel ) );
        s_profileOverlay.create.generic.flags |= QMF_GRAYED;
    }

    UI_ProfileOverlay_UpdateButtonBounds( &s_profileOverlay.create );
}

qboolean Q3R_Profile_Load(const char *name) {
    fileHandle_t f;
    char path[MAX_QPATH];
    char header[10];
    int len;

    Com_sprintf(path, sizeof(path), "%s/%s%s", PROFILES_PATH, name, PROFILE_EXTENSION);
    len = trap_FS_FOpenFile(path, &f, FS_READ);
    if (!f) {
        return qfalse;
    }

    if (len < (int)(strlen(Q3R_PROFILE_HEADER) + sizeof(q3r_profile_t))) {
        trap_FS_FCloseFile(f);
        return qfalse;
    }

    trap_FS_Read(header, strlen(Q3R_PROFILE_HEADER), f);
    if (strncmp(header, Q3R_PROFILE_HEADER, strlen(Q3R_PROFILE_HEADER)) != 0) {
        trap_FS_FCloseFile(f);
        return qfalse;
    }

    trap_FS_Read(&cg_profile, sizeof(q3r_profile_t), f);
    trap_FS_FCloseFile(f);
    return qtrue;
}

static void UI_ProfileOverlay_CreateFromField( void ) {
    char sanitized[MAX_QPATH];
    int  index;

    UI_ProfileOverlay_GetSanitizedFieldText( sanitized, sizeof( sanitized ) );
    if ( !sanitized[0] ) {
        return;
    }

    index = UI_ProfileOverlay_FindIndex( sanitized );

    if ( index < 0 ) {
        q3r_profile_t new_profile;
        Q3R_Profile_InitDefaults(&new_profile, sanitized);
        if (Q3R_Profile_Save(&new_profile)) {
            UI_ProfileOverlay_AddProfileName(sanitized);
            index = UI_ProfileOverlay_FindIndex(sanitized);
            s_profileOverlay.list.numitems = s_profileOverlay.profileCount;
            UI_ProfileOverlay_UpdateButtonStates();
        }
    }

    UI_ProfileOverlay_UpdateSelection( index );
}

static void UI_ProfileOverlay_DeleteSelected( void ) {
    int i;
    int index;

    if ( s_profileOverlay.profileCount <= 0 ) {
        return;
    }

    index = s_profileOverlay.list.curvalue;
    if ( index < 0 || index >= s_profileOverlay.profileCount ) {
        return;
    }

    char path[MAX_QPATH];
    Com_sprintf(path, sizeof(path), "%s/%s%s", PROFILES_PATH, s_profileOverlay.profileNames[index], PROFILE_EXTENSION);
    trap_FS_Delete(path);

    for ( i = index; i < s_profileOverlay.profileCount - 1; ++i ) {
        Q_strncpyz( s_profileOverlay.profileNames[i], s_profileOverlay.profileNames[i + 1],
                    sizeof( s_profileOverlay.profileNames[i] ) );
        s_profileOverlay.profileItems[i] = s_profileOverlay.profileNames[i];
    }

    if ( s_profileOverlay.profileCount > 0 ) {
        s_profileOverlay.profileNames[s_profileOverlay.profileCount - 1][0] = '\0';
        s_profileOverlay.profileItems[s_profileOverlay.profileCount - 1] = NULL;
        s_profileOverlay.profileCount--;
    }

    s_profileOverlay.list.numitems = s_profileOverlay.profileCount;

    if ( s_profileOverlay.profileCount <= 0 ) {
        s_profileOverlay.list.curvalue = 0;
        UI_ProfileOverlay_UpdateButtonStates();
        UI_ProfileOverlay_UpdateSelection( -1 );
        UI_ProfileOverlay_ResetNameField();
        return;
    }

    if ( index >= s_profileOverlay.profileCount ) {
        index = s_profileOverlay.profileCount - 1;
    }

    UI_ProfileOverlay_UpdateButtonStates();
    UI_ProfileOverlay_UpdateSelection( index );
}

static void UI_ProfileManager_DrawInfo(void) {
    int y = PROFILE_PANEL_Y + 180;
    UI_DrawProportionalString(480, y, va("Races: %d", cg_profile.totalRacesStarted), UI_LEFT | UI_SMALLFONT, colorWhite);
    y += 20;
    UI_DrawProportionalString(480, y, va("Wins: %d", cg_profile.totalRacesWon), UI_LEFT | UI_SMALLFONT, colorWhite);
    y += 20;
    UI_DrawProportionalString(480, y, va("Play Time: %d hours", cg_profile.totalPlayTimeSeconds / 3600), UI_LEFT | UI_SMALLFONT, colorWhite);
}

static void UI_ProfileOverlay_Draw( void ) {
    char playerName[MAX_QPATH];
    char cleaned[MAX_QPATH];
    int  listLeft;
    int  listRight;

    UI_FillRect( -uis.bias, 0, SCREEN_WIDTH + 2 * uis.bias, SCREEN_HEIGHT, profileOverlayBackdrop );
    UI_FillRect( PROFILE_PANEL_X, PROFILE_PANEL_Y, PROFILE_PANEL_W, PROFILE_PANEL_H, profileOverlayPanel );

    trap_Cvar_VariableStringBuffer( "name", playerName, sizeof( playerName ) );
    Q_strncpyz( cleaned, playerName, sizeof( cleaned ) );
    Q_CleanStr( cleaned );
    if ( !cleaned[0] ) {
        Q_strncpyz( cleaned, "Player", sizeof( cleaned ) );
    }

    UI_DrawProportionalString( 320, PROFILE_PANEL_Y + 24, "PROFILE SELECTION", UI_CENTER | UI_BIGFONT, text_color_normal );
    UI_DrawProportionalString( 320, PROFILE_PANEL_Y + 74, va( "PLAYER NAME: %s", cleaned ),
                               UI_CENTER | UI_SMALLFONT, text_color_normal );
    UI_DrawProportionalString( 320, PROFILE_PANEL_Y + 102,
                               "SELECT A PROFILE OR CREATE ONE FROM YOUR PLAYER NAME",
                               UI_CENTER | UI_SMALLFONT, text_color_normal );
    UI_DrawProportionalString( 320, PROFILE_PANEL_Y + 134,
                               "NEW PROFILE NAME",
                               UI_CENTER | UI_SMALLFONT, text_color_normal );

    listLeft = s_profileOverlay.list.generic.left;
    listRight = s_profileOverlay.list.generic.right;
    if ( listRight > listLeft ) {
        UI_DrawRect( listLeft - 4, s_profileOverlay.list.generic.top - 4,
                     ( listRight - listLeft ) + 8,
                     ( s_profileOverlay.list.generic.bottom - s_profileOverlay.list.generic.top ) + 8,
                     colorMdGrey );
    }

    UI_ProfileOverlay_UpdateCreateLabel();
    Menu_Draw( &s_profileOverlay.menu );
	UI_ProfileManager_DrawInfo();
}

static void UI_ProfileOverlay_Event( void *ptr, int event ) {
    menucommon_s *item = (menucommon_s*)ptr;

    if ( item->id == ID_PROFILE_LIST ) {
        if ( event == QM_GOTFOCUS ) {
            if ( s_profileOverlay.list.curvalue != s_profileOverlay.lastSelection ) {
                UI_ProfileOverlay_UpdateSelection( s_profileOverlay.list.curvalue );
            }
        } else if ( event == QM_ACTIVATED ) {
            if ( s_profileOverlay.list.curvalue != s_profileOverlay.lastSelection ) {
                UI_ProfileOverlay_UpdateSelection( s_profileOverlay.list.curvalue );
            }
            if ( s_profileOverlay.profileCount > 0 ) {
                UI_PopMenu();
            }
        }
        return;
    }

    if ( event != QM_ACTIVATED ) {
        return;
    }

    switch ( item->id ) {
    case ID_PROFILE_CREATE:
        UI_ProfileOverlay_CreateFromField();
        break;
    case ID_PROFILE_DELETE:
        UI_ProfileOverlay_DeleteSelected();
        break;
    case ID_PROFILE_CONTINUE:
        if ( s_profileOverlay.profileCount > 0 ) {
            UI_PopMenu();
        }
        break;
    }
}

static sfxHandle_t UI_ProfileOverlay_Key( int key ) {
    if ( key == K_ESCAPE ) {
        return menu_null_sound;
    }

    return Menu_DefaultKey( &s_profileOverlay.menu, key );
}

static void UI_ProfileOverlay_InitMenu( void ) {
    int listWidthPixels;
    if ( s_profileOverlayInitialized ) {
        return;
    }

    Com_Memset( &s_profileOverlay, 0, sizeof( s_profileOverlay ) );
    s_profileOverlay.lastSelection = -1;

    s_profileOverlay.menu.wrapAround = qfalse;
    s_profileOverlay.menu.fullscreen = qtrue;
    s_profileOverlay.menu.transparent = qtrue;
    s_profileOverlay.menu.draw = UI_ProfileOverlay_Draw;
    s_profileOverlay.menu.key = UI_ProfileOverlay_Key;

    listWidthPixels = PROFILE_LIST_WIDTH * SMALLCHAR_WIDTH + SB_WIDTH;

    s_profileOverlay.list.generic.type = MTYPE_LISTBOX;
    s_profileOverlay.list.generic.flags = QMF_HIGHLIGHT_IF_FOCUS;
    s_profileOverlay.list.generic.id = ID_PROFILE_LIST;
    s_profileOverlay.list.generic.callback = UI_ProfileOverlay_Event;
    s_profileOverlay.list.generic.x = PROFILE_PANEL_X + ( PROFILE_PANEL_W / 2 ) - ( listWidthPixels / 2 );
    s_profileOverlay.list.generic.y = PROFILE_PANEL_Y + 176;
    s_profileOverlay.list.width = PROFILE_LIST_WIDTH;
    s_profileOverlay.list.height = PROFILE_LIST_HEIGHT;
    s_profileOverlay.list.itemnames = (const char **)s_profileOverlay.profileItems;
    s_profileOverlay.list.scrollbarAlignment = SB_RIGHT;
    s_profileOverlay.list.generic.left = s_profileOverlay.list.generic.x;
    s_profileOverlay.list.generic.top = s_profileOverlay.list.generic.y;
    s_profileOverlay.list.generic.right = s_profileOverlay.list.generic.x + listWidthPixels;
    s_profileOverlay.list.generic.bottom = s_profileOverlay.list.generic.y + PROFILE_LIST_HEIGHT * SMALLCHAR_HEIGHT;

    s_profileOverlay.nameField.generic.type = MTYPE_FIELD;
    s_profileOverlay.nameField.generic.flags = QMF_NODEFAULTINIT;
    s_profileOverlay.nameField.generic.id = ID_PROFILE_NAME;
    s_profileOverlay.nameField.generic.x = PROFILE_PANEL_X + ( PROFILE_PANEL_W / 2 ) - ( PROFILE_FIELD_WIDTH * SMALLCHAR_WIDTH ) / 2;
    s_profileOverlay.nameField.generic.y = PROFILE_PANEL_Y + 152;
    s_profileOverlay.nameField.generic.left = s_profileOverlay.nameField.generic.x - 8;
    s_profileOverlay.nameField.generic.top = s_profileOverlay.nameField.generic.y - 4;
    s_profileOverlay.nameField.generic.right = s_profileOverlay.nameField.generic.x + PROFILE_FIELD_WIDTH * SMALLCHAR_WIDTH + 8;
    s_profileOverlay.nameField.generic.bottom = s_profileOverlay.nameField.generic.y + SMALLCHAR_HEIGHT + 4;
    s_profileOverlay.nameField.field.widthInChars = PROFILE_FIELD_WIDTH;
    s_profileOverlay.nameField.field.maxchars = MAX_QPATH - 1;
    MenuField_Init( &s_profileOverlay.nameField );

    s_profileOverlay.create.generic.type = MTYPE_PTEXT;
    s_profileOverlay.create.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    s_profileOverlay.create.generic.id = ID_PROFILE_CREATE;
    s_profileOverlay.create.generic.callback = UI_ProfileOverlay_Event;
    s_profileOverlay.create.generic.x = 320;
    s_profileOverlay.create.generic.y = PROFILE_PANEL_Y + PROFILE_PANEL_H - 96;
    s_profileOverlay.create.generic.left = s_profileOverlay.create.generic.x - ( PROFILE_BUTTON_WIDTH / 2 );
    s_profileOverlay.create.generic.top = s_profileOverlay.create.generic.y - 8;
    s_profileOverlay.create.generic.right = s_profileOverlay.create.generic.x + ( PROFILE_BUTTON_WIDTH / 2 );
    s_profileOverlay.create.generic.bottom = s_profileOverlay.create.generic.y + PROFILE_BUTTON_HEIGHT;
    s_profileOverlay.create.style = UI_CENTER | UI_SMALLFONT;
    s_profileOverlay.create.color = text_color_normal;
    s_profileOverlay.create.string = s_profileOverlay.createLabel;

    s_profileOverlay.deleteBtn.generic.type = MTYPE_PTEXT;
    s_profileOverlay.deleteBtn.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    s_profileOverlay.deleteBtn.generic.id = ID_PROFILE_DELETE;
    s_profileOverlay.deleteBtn.generic.callback = UI_ProfileOverlay_Event;
    s_profileOverlay.deleteBtn.generic.x = 320;
    s_profileOverlay.deleteBtn.generic.y = PROFILE_PANEL_Y + PROFILE_PANEL_H - 64;
    s_profileOverlay.deleteBtn.generic.left = s_profileOverlay.deleteBtn.generic.x - ( PROFILE_BUTTON_WIDTH / 2 );
    s_profileOverlay.deleteBtn.generic.top = s_profileOverlay.deleteBtn.generic.y - 8;
    s_profileOverlay.deleteBtn.generic.right = s_profileOverlay.deleteBtn.generic.x + ( PROFILE_BUTTON_WIDTH / 2 );
    s_profileOverlay.deleteBtn.generic.bottom = s_profileOverlay.deleteBtn.generic.y + PROFILE_BUTTON_HEIGHT;
    s_profileOverlay.deleteBtn.style = UI_CENTER | UI_SMALLFONT;
    s_profileOverlay.deleteBtn.color = text_color_normal;
    s_profileOverlay.deleteBtn.string = "DELETE PROFILE";

    s_profileOverlay.continueBtn.generic.type = MTYPE_PTEXT;
    s_profileOverlay.continueBtn.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    s_profileOverlay.continueBtn.generic.id = ID_PROFILE_CONTINUE;
    s_profileOverlay.continueBtn.generic.callback = UI_ProfileOverlay_Event;
    s_profileOverlay.continueBtn.generic.x = 320;
    s_profileOverlay.continueBtn.generic.y = PROFILE_PANEL_Y + PROFILE_PANEL_H - 32;
    s_profileOverlay.continueBtn.generic.left = s_profileOverlay.continueBtn.generic.x - ( PROFILE_BUTTON_WIDTH / 2 );
    s_profileOverlay.continueBtn.generic.top = s_profileOverlay.continueBtn.generic.y - 8;
    s_profileOverlay.continueBtn.generic.right = s_profileOverlay.continueBtn.generic.x + ( PROFILE_BUTTON_WIDTH / 2 );
    s_profileOverlay.continueBtn.generic.bottom = s_profileOverlay.continueBtn.generic.y + PROFILE_BUTTON_HEIGHT;
    s_profileOverlay.continueBtn.style = UI_CENTER | UI_SMALLFONT;
    s_profileOverlay.continueBtn.color = text_color_normal;
    s_profileOverlay.continueBtn.string = s_profileOverlay.continueLabel;
    s_profileOverlay.continueBtn.generic.flags |= QMF_GRAYED;

    s_profileOverlay.capture.generic.type = MTYPE_BITMAP;
    s_profileOverlay.capture.generic.flags = QMF_LEFT_JUSTIFY | QMF_INACTIVE;
    s_profileOverlay.capture.generic.x = PROFILE_PANEL_X;
    s_profileOverlay.capture.generic.y = PROFILE_PANEL_Y;
    s_profileOverlay.capture.width = PROFILE_PANEL_W;
    s_profileOverlay.capture.height = PROFILE_PANEL_H;

    Menu_AddItem( &s_profileOverlay.menu, &s_profileOverlay.capture );
    Menu_AddItem( &s_profileOverlay.menu, &s_profileOverlay.nameField );
    Menu_AddItem( &s_profileOverlay.menu, &s_profileOverlay.list );
    Menu_AddItem( &s_profileOverlay.menu, &s_profileOverlay.create );
    Menu_AddItem( &s_profileOverlay.menu, &s_profileOverlay.deleteBtn );
    Menu_AddItem( &s_profileOverlay.menu, &s_profileOverlay.continueBtn );

    UI_ProfileOverlay_UpdateButtonBounds( &s_profileOverlay.create );
    UI_ProfileOverlay_UpdateButtonBounds( &s_profileOverlay.deleteBtn );
    UI_ProfileOverlay_UpdateButtonBounds( &s_profileOverlay.continueBtn );

    s_profileOverlayInitialized = qtrue;
}

static void UI_ProfileOverlay_EnsureSelection( void ) {
    char buffer[MAX_QPATH];
    char sanitized[MAX_QPATH];
    int  index;

    trap_Cvar_VariableStringBuffer( "profile", buffer, sizeof( buffer ) );
    UI_ProfileOverlay_Sanitize( buffer, sanitized, sizeof( sanitized ) );
    index = UI_ProfileOverlay_FindIndex( sanitized );

    if ( index < 0 ) {
        trap_Cvar_VariableStringBuffer( "ui_profileSelected", buffer, sizeof( buffer ) );
        UI_ProfileOverlay_Sanitize( buffer, sanitized, sizeof( sanitized ) );
        index = UI_ProfileOverlay_FindIndex( sanitized );
    }

    if ( s_profileOverlay.profileCount == 0 ) {
        UI_ProfileOverlay_UpdateSelection( -1 );
        return;
    }

    if ( index < 0 ) {
        index = 0;
    }

    UI_ProfileOverlay_UpdateSelection( index );
}

static qboolean UI_ProfileOverlay_ShouldForcePrompt( void ) {
    return s_profileOverlay.profileCount == 0;
}

static void UI_ProfileOverlay_Show( qboolean forceCreate ) {
    if ( uis.activemenu == &s_profileOverlay.menu ) {
        return;
    }

    UI_ProfileOverlay_InitMenu();
    UI_ProfileOverlay_RebuildList();

    if ( s_profileOverlay.profileCount > 0 ) {
        UI_ProfileOverlay_EnsureSelection();
    } else {
        UI_ProfileOverlay_UpdateSelection( -1 );
    }

    if ( forceCreate || !s_profileOverlay.nameField.field.buffer[0] ) {
        UI_ProfileOverlay_ResetNameField();
    }

    UI_ProfileOverlay_UpdateButtonStates();
    UI_ProfileOverlay_UpdateCreateLabel();

    Menu_SetCursorToItem( &s_profileOverlay.menu, &s_profileOverlay.list );
    UI_PushMenu( &s_profileOverlay.menu );
    s_profileOverlayShownThisSession = qtrue;
    trap_Cvar_SetValue( "ui_profilePromptShown", 1 );
}

void UI_ProfileOverlay_ResetSession( void ) {
    s_profileOverlayShownThisSession = qfalse;
}

static void UI_ProfileOverlay_ResetNameField( void ) {
    char sanitized[MAX_QPATH];

    UI_ProfileOverlay_BuildPlayerProfileName( sanitized, sizeof( sanitized ) );
    Q_strncpyz( s_profileOverlay.nameField.field.buffer, sanitized, sizeof( s_profileOverlay.nameField.field.buffer ) );
    s_profileOverlay.nameField.field.cursor = strlen( s_profileOverlay.nameField.field.buffer );
    s_profileOverlay.nameField.field.scroll = 0;
}

void UI_ProfileOverlay_MaybeShow( void ) {
    qboolean forcePrompt;

    UI_ProfileOverlay_RebuildList();
    forcePrompt = UI_ProfileOverlay_ShouldForcePrompt();

    if ( forcePrompt ) {
        UI_ProfileOverlay_Show( qtrue );
        return;
    }

    if ( !s_profileOverlayShownThisSession ) {
        UI_ProfileOverlay_Show( qfalse );
    }
}
