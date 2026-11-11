/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

q3rally source code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software Foundation,
Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
===========================================================================
*/

/*
=======================================================================

CONFIG HUB

The hub owns the modern navigation surface. The actual option screens are
still opened through their existing entry points so their cvar and binding
logic remains unchanged while each screen is migrated separately.

=======================================================================
*/

#include "ui_local.h"
#include "ui_rally_frontend.h"

#define ID_CUSTOMIZEPLAYER      10
#define ID_CUSTOMIZECONTROLS    11
#define ID_GRAPHICS             12
#define ID_SOUND                13
#define ID_NETWORK              14
#define ID_GAME                 15
#define ID_Q3ROPTIONS           16
#define ID_DEFAULTS             17
#define ID_BACK                 18
#define ID_DISPLAY              19

#define CONFIG_CATEGORY_COUNT   8
#define CONFIG_FRAME_X          24
#define CONFIG_FRAME_Y          20
#define CONFIG_FRAME_WIDTH      592
#define CONFIG_FRAME_HEIGHT     440
#define CONFIG_LIST_X           40
#define CONFIG_LIST_Y           152
#define CONFIG_LIST_WIDTH       202
#define CONFIG_LIST_HEIGHT      268
#define CONFIG_ROW_X            ( CONFIG_LIST_X + 16 )
#define CONFIG_ROW_WIDTH        ( CONFIG_LIST_WIDTH - 32 )
#define CONFIG_ROW_HEIGHT       UI_FRONTEND_BUTTON_HEIGHT
#define CONFIG_ROW_GAP          2
#define CONFIG_DETAIL_X         266
#define CONFIG_DETAIL_Y         104
#define CONFIG_DETAIL_WIDTH     334
#define CONFIG_DETAIL_HEIGHT    292
#define CONFIG_ACTION_Y         420
#define CONFIG_ACTION_HEIGHT    UI_FRONTEND_BUTTON_HEIGHT
#define CONFIG_ACTION_WIDTH     UI_FRONTEND_ACTION_WIDTH
#define CONFIG_DEFAULTS_X       ( CONFIG_FRAME_X + CONFIG_FRAME_WIDTH - 24 - CONFIG_ACTION_WIDTH )
#define CONFIG_BACK_X           UI_FRONTEND_BACK_X

typedef struct {
    menuframework_s menu;
    menutext_s      categories[CONFIG_CATEGORY_COUNT];
    menutext_s      defaults;
    menutext_s      back;
    int             selectedCategory;
} setupMenuInfo_t;

static setupMenuInfo_t setupMenuInfo;

static const int configCategoryIds[CONFIG_CATEGORY_COUNT] = {
    ID_CUSTOMIZEPLAYER,
    ID_CUSTOMIZECONTROLS,
    ID_GRAPHICS,
    ID_DISPLAY,
    ID_SOUND,
    ID_NETWORK,
    ID_GAME,
    ID_Q3ROPTIONS
};

static const char *configCategoryLabels[CONFIG_CATEGORY_COUNT] = {
    "Driver",
    "Controls",
    "Graphics",
    "Display",
    "Sound",
    "Network",
    "Game options",
    "Q3R options"
};

static const char *configCategoryDescriptions[CONFIG_CATEGORY_COUNT][2] = {
    { "Choose your driver profile and", "tune the active vehicle." },
    { "Configure bindings, mouse feel", "and input behavior." },
    { "Shape display mode, image quality", "and advanced rendering." },
    { "Tune brightness, screen size", "and display framing." },
    { "Balance music, effects and", "voice feedback." },
    { "Set connection, rate and", "server behavior." },
    { "Adjust gameplay rules and", "match preferences." },
    { "Configure Q3Rally driving, HUD", "and vehicle behavior." }
};

static vec4_t configTextColor = UI_FRONTEND_COLOR_TEXT;
static vec4_t configMutedColor = UI_FRONTEND_COLOR_MUTED;
static vec4_t configAccentColor = UI_FRONTEND_COLOR_ACCENT;

static int UI_SetupMenu_CategoryIndex( int id ) {
    int i;

    for ( i = 0; i < CONFIG_CATEGORY_COUNT; i++ ) {
        if ( configCategoryIds[i] == id ) {
            return i;
        }
    }
    return -1;
}

static void Setup_ResetDefaults_Action( qboolean result ) {
    if ( !result ) {
        return;
    }

    trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n" );
    trap_Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n" );
    trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}

static void Setup_ResetDefaults_Draw( void ) {
    vec4_t warningColor = { 0.90f, 0.78f, 0.33f, 1.00f };

    Frontend_DrawText( 320, 222, "Reset all settings?",
                       UI_CENTER | UI_SMALLFONT, warningColor );
    Frontend_DrawText( 320, 244, "This restores the default configuration.",
                       UI_CENTER | UI_SMALLFONT, configMutedColor );
}

static void UI_SetupMenu_DrawCategory( void *self ) {
    menutext_s *item;
    int index;
    qboolean active;
    qboolean focus;

    item = (menutext_s *)self;
    index = UI_SetupMenu_CategoryIndex( item->generic.id );
    if ( index < 0 ) {
        return;
    }

    focus = ( Menu_ItemAtCursor( item->generic.parent ) == item );
    active = ( setupMenuInfo.selectedCategory == index ) ? qtrue : qfalse;
    Frontend_DrawNavButton( item->generic.left, item->generic.top,
                            item->generic.right - item->generic.left,
                            item->generic.bottom - item->generic.top,
                            item->string, 1.0f, active || focus,
                            UI_FRONTEND_TEXT_LEFT );
}

static void UI_SetupMenu_DrawAction( void *self ) {
    menutext_s *button;
    qboolean focus;
    int x;
    int y;
    int width;
    int height;

    button = (menutext_s *)self;
    focus = ( Menu_ItemAtCursor( button->generic.parent ) == button );
    x = button->generic.left;
    y = button->generic.top;
    width = button->generic.right - button->generic.left;
    height = button->generic.bottom - button->generic.top;

    Frontend_DrawButton( x, y, width, height, button->string, 1.0f,
                         focus, UI_FRONTEND_TEXT_CENTER );
}

static void UI_SetupMenu_Draw( void ) {
    vec4_t scrimColor = UI_FRONTEND_COLOR_SCRIM;
    int selected;

    selected = setupMenuInfo.selectedCategory;
    if ( selected < 0 || selected >= CONFIG_CATEGORY_COUNT ) {
        selected = 0;
    }

    Frontend_DrawBackground( scrimColor );
    Frontend_DrawPanel( CONFIG_FRAME_X, CONFIG_FRAME_Y,
                        CONFIG_FRAME_WIDTH, CONFIG_FRAME_HEIGHT, 1.0f,
                        UI_FRONTEND_STYLE_FRAME );
    Frontend_DrawText( CONFIG_FRAME_X + 24, CONFIG_FRAME_Y + 24,
                       "Config", UI_LEFT | UI_BIGFONT, configTextColor );
    Frontend_DrawText( CONFIG_FRAME_X + 24, CONFIG_FRAME_Y + 48,
                       "Tune your ride, controls and system",
                       UI_LEFT | UI_SMALLFONT, configMutedColor );
    Frontend_DrawStatusChip( CONFIG_FRAME_X + CONFIG_FRAME_WIDTH - 104,
                             CONFIG_FRAME_Y + 26, "Settings",
                             configAccentColor, 1.0f );

    Frontend_DrawCard( CONFIG_LIST_X, CONFIG_LIST_Y - 40,
                       CONFIG_LIST_WIDTH, CONFIG_LIST_HEIGHT, 1.0f, qfalse );
    Frontend_DrawCard( CONFIG_DETAIL_X, CONFIG_DETAIL_Y,
                       CONFIG_DETAIL_WIDTH, CONFIG_DETAIL_HEIGHT, 1.0f,
                       qfalse );
    Frontend_DrawText( CONFIG_LIST_X + 16, CONFIG_LIST_Y - 24,
                       "Categories", UI_LEFT | UI_SMALLFONT,
                       configMutedColor );
    Frontend_DrawText( CONFIG_DETAIL_X + 20, CONFIG_DETAIL_Y + 18,
                       "Configuration", UI_LEFT | UI_SMALLFONT,
                       configMutedColor );

    Menu_Draw( &setupMenuInfo.menu );

    Frontend_DrawStatusChip( CONFIG_DETAIL_X + 20, CONFIG_DETAIL_Y + 54,
                             "Available", configAccentColor, 1.0f );
    Frontend_DrawText( CONFIG_DETAIL_X + 20, CONFIG_DETAIL_Y + 100,
                       configCategoryLabels[selected],
                       UI_LEFT | UI_BIGFONT, configTextColor );
    Frontend_DrawText( CONFIG_DETAIL_X + 20, CONFIG_DETAIL_Y + 140,
                       configCategoryDescriptions[selected][0],
                       UI_LEFT | UI_SMALLFONT, configMutedColor );
    Frontend_DrawText( CONFIG_DETAIL_X + 20, CONFIG_DETAIL_Y + 162,
                       configCategoryDescriptions[selected][1],
                       UI_LEFT | UI_SMALLFONT, configMutedColor );
    Frontend_DrawText( CONFIG_DETAIL_X + 20, CONFIG_DETAIL_Y + 222,
                       "Changes use the existing game settings",
                       UI_LEFT | UI_SMALLFONT, configMutedColor );
    Frontend_DrawText( CONFIG_DETAIL_X + 20, CONFIG_DETAIL_Y + 244,
                       "and apply through the original menus.",
                       UI_LEFT | UI_SMALLFONT, configMutedColor );

    Frontend_DrawText( CONFIG_FRAME_X + 24, CONFIG_FRAME_Y + 384,
                       "Select a category to continue",
                       UI_LEFT | UI_SMALLFONT, configMutedColor );
    Frontend_DrawText( CONFIG_FRAME_X + CONFIG_FRAME_WIDTH - 24,
                       CONFIG_FRAME_Y + 384,
                       "Enter open   Esc back", UI_RIGHT | UI_SMALLFONT,
                       configMutedColor );
}

static void UI_SetupMenu_OpenCategory( int id ) {
    /* Keep the hub on the menu stack. Child screens can then use their
     * existing UI_PopMenu() Back action to return here. */
    switch ( id ) {
    case ID_CUSTOMIZEPLAYER:
        UI_PlayerSettingsMenu();
        break;
    case ID_CUSTOMIZECONTROLS:
        UI_ControlsMenu();
        break;
    case ID_GRAPHICS:
        UI_GraphicsOptionsMenu();
        break;
    case ID_DISPLAY:
        UI_DisplayOptionsMenu();
        break;
    case ID_SOUND:
        UI_SoundOptionsMenu();
        break;
    case ID_NETWORK:
        UI_NetworkOptionsMenu();
        break;
    case ID_GAME:
        UI_PreferencesMenu();
        break;
    case ID_Q3ROPTIONS:
        UI_Q3ROptionsMenu();
        break;
    }
}

static void UI_SetupMenu_Event( void *ptr, int event ) {
    menucommon_s *item;
    int category;

    item = (menucommon_s *)ptr;
    category = UI_SetupMenu_CategoryIndex( item->id );
    if ( category >= 0 ) {
        setupMenuInfo.selectedCategory = category;
        if ( event == QM_ACTIVATED ) {
            UI_SetupMenu_OpenCategory( item->id );
        }
        return;
    }

    if ( event != QM_ACTIVATED ) {
        return;
    }

    switch ( item->id ) {
    case ID_DEFAULTS:
        UI_ConfirmMenu( "RESET SETTINGS?", Setup_ResetDefaults_Draw,
                        Setup_ResetDefaults_Action );
        break;
    case ID_BACK:
        UI_PopMenu();
        break;
    }
}

static void UI_SetupMenu_InitAction( menutext_s *item, int id,
                                     const char *label, int x ) {
    item->generic.type = MTYPE_PTEXT;
    item->generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS |
                          QMF_NODEFAULTINIT;
    item->generic.id = id;
    item->generic.callback = UI_SetupMenu_Event;
    item->generic.x = x + CONFIG_ACTION_WIDTH / 2;
    item->generic.y = CONFIG_ACTION_Y;
    item->generic.left = x;
    item->generic.top = CONFIG_ACTION_Y;
    item->generic.right = x + CONFIG_ACTION_WIDTH;
    item->generic.bottom = CONFIG_ACTION_Y + CONFIG_ACTION_HEIGHT;
    item->string = (char *)label;
    item->color = configTextColor;
    item->style = UI_CENTER | UI_SMALLFONT;
    item->generic.ownerdraw = UI_SetupMenu_DrawAction;
}

static void UI_SetupMenu_Init( void ) {
    int i;
    int y;

    memset( &setupMenuInfo, 0, sizeof( setupMenuInfo ) );
    setupMenuInfo.menu.draw = UI_SetupMenu_Draw;
    setupMenuInfo.menu.fullscreen = qtrue;
    setupMenuInfo.menu.wrapAround = qtrue;
    setupMenuInfo.selectedCategory = 0;

    for ( i = 0; i < CONFIG_CATEGORY_COUNT; i++ ) {
        menutext_s *item = &setupMenuInfo.categories[i];

        y = CONFIG_LIST_Y + i * ( CONFIG_ROW_HEIGHT + CONFIG_ROW_GAP );
        item->generic.type = MTYPE_PTEXT;
        item->generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS |
                              QMF_NODEFAULTINIT;
        item->generic.id = configCategoryIds[i];
        item->generic.callback = UI_SetupMenu_Event;
        item->generic.x = CONFIG_ROW_X;
        item->generic.y = y;
        item->generic.left = CONFIG_ROW_X;
        item->generic.top = y;
        item->generic.right = CONFIG_ROW_X + CONFIG_ROW_WIDTH;
        item->generic.bottom = y + CONFIG_ROW_HEIGHT;
        item->string = (char *)configCategoryLabels[i];
        item->color = configTextColor;
        item->style = UI_LEFT | UI_SMALLFONT;
        item->generic.ownerdraw = UI_SetupMenu_DrawCategory;
        Menu_AddItem( &setupMenuInfo.menu, item );
    }

    UI_SetupMenu_InitAction( &setupMenuInfo.defaults, ID_DEFAULTS,
                             "Reset defaults", CONFIG_DEFAULTS_X );
    UI_SetupMenu_InitAction( &setupMenuInfo.back, ID_BACK,
                             "Back", CONFIG_BACK_X );
    Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.defaults );
    Menu_AddItem( &setupMenuInfo.menu, &setupMenuInfo.back );

    if ( trap_Cvar_VariableValue( "cl_paused" ) ) {
        setupMenuInfo.defaults.generic.flags |= QMF_GRAYED;
    }
}

void UI_SetupMenu_Cache( void ) {
    /* The Config hub uses the shared frontend components. */
}

void UI_SetupMenu( void ) {
    UI_SetupMenu_Init();
    Frontend_SetBackButton( &setupMenuInfo.back.generic );
    UI_PushMenu( &setupMenuInfo.menu );
}


static menuframework_s *WebConfig_Menu( int *page ) {
    menuframework_s *menu;
    if ( (menu = UI_WebLadderWizardMenu()) ) { *page = 16; return menu; }
    if ( (menu = UI_WebDemoSaveMenu()) ) { *page = 21; return menu; }
    if ( (menu = UI_WebProfileWizardMenu()) ) { *page = 17 + UI_WebProfileWizardPage(); return menu; }
    if ( uis.activemenu == &setupMenuInfo.menu ) { *page = 0; return &setupMenuInfo.menu; }
    if ( (menu = UI_WebControlsMenu()) ) { *page = 1; return menu; }
    if ( (menu = UI_WebGraphicsMenu()) ) { *page = 2; return menu; }
    if ( (menu = UI_WebDisplayMenu()) ) { *page = 3; return menu; }
    if ( (menu = UI_WebSoundMenu()) ) { *page = 4; return menu; }
    if ( (menu = UI_WebNetworkMenu()) ) { *page = 5; return menu; }
    if ( (menu = UI_WebPreferencesMenu()) ) { *page = 6; return menu; }
    if ( (menu = UI_WebRallyOptionsMenu()) ) { *page = 7; return menu; }
    if ( (menu = UI_WebDriverInfoMenu()) ) { *page = 8; return menu; }
    if ( (menu = UI_WebPauseMenu()) ) { *page = 10; return menu; }
    if ( (menu = UI_WebAddBotsMenu()) ) { *page = 11; return menu; }
    if ( (menu = UI_WebRemoveBotsMenu()) ) { *page = 12; return menu; }
    if ( (menu = UI_WebTeamMenu()) ) { *page = 13; return menu; }
    if ( (menu = UI_WebTeamOrdersMenu()) ) { *page = 14; return menu; }
    if ( (menu = UI_WebServerInfoMenu()) ) { *page = 15; return menu; }
    if ( (menu = UI_WebConfirmMenu()) ) {
        *page = 9; return menu;
    }
    *page = -1; return NULL;
}
int UI_WebConfigScreen( void ) { int page; return WebConfig_Menu( &page ) != NULL; }
static menucommon_s *WebConfig_Item( int index, int *page ) {
    menuframework_s *menu = WebConfig_Menu( page );
    menucommon_s *item;
    if ( !menu || index < 0 || index >= menu->nitems ) return NULL;
    item = menu->items[index];
    if ( item->flags & QMF_HIDDEN ) return NULL;
    return item;
}
static int WebConfig_Choices( menucommon_s *item ) {
    int count = 0; const char **names;
    if ( !item ) return 0;
    if ( item->type == MTYPE_RADIOBUTTON ) return 2;
    if ( item->type == MTYPE_SCROLLLIST ) return ((menulist_s *)item)->numitems;
    if ( item->type != MTYPE_SPINCONTROL && item->type != MTYPE_SCROLLLIST ) return 0;
    names = ((menulist_s *)item)->itemnames;
    if ( names ) while ( count < 256 && names[count] ) count++;
    return count;
}
int UI_WebConfigNumber( int field, int index ) {
    int page; menuframework_s *menu = WebConfig_Menu( &page );
    menucommon_s *item = WebConfig_Item( index, &page );
    if ( !menu ) return 0;
    switch ( field ) {
    case 0: return page;
    case 1: return menu->nitems;
    case 2:
        if ( !item ) return 0;
        if ( item->type == MTYPE_SPINCONTROL ) return 1;
        if ( item->type == MTYPE_RADIOBUTTON ) return 2;
    if ( item->type == MTYPE_SCROLLLIST ) return ((menulist_s *)item)->numitems;
        if ( item->type == MTYPE_SLIDER ) return 3;
        if ( item->type == MTYPE_FIELD ) return 4;
        if ( item->type == MTYPE_ACTION && UI_WebControlText( index, 0 )[0] ) return 5;
        if ( (item->type == MTYPE_PTEXT || item->type == MTYPE_BITMAP) && item->callback ) return 6;
        if ( item->type == MTYPE_SCROLLLIST ) return page == 14 ? 8 : 7;
        return 0;
    case 3:
        if ( !item ) return 0;
        if ( item->type == MTYPE_SLIDER ) return (int)(((menuslider_s *)item)->curvalue * 100 + 0.5f);
        if ( item->type == MTYPE_RADIOBUTTON ) return ((menuradiobutton_s *)item)->curvalue;
        if ( item->type == MTYPE_SPINCONTROL ) return ((menulist_s *)item)->curvalue;
        return 0;
    case 4: return item && item->type == MTYPE_SLIDER ? (int)(((menuslider_s *)item)->minvalue * 100) : 0;
    case 5:
        if ( item && item->type == MTYPE_FIELD ) return ((menufield_s *)item)->field.maxchars;
        return item && item->type == MTYPE_SLIDER ? (int)(((menuslider_s *)item)->maxvalue * 100) : 0;
    case 6: return item && (item->flags & (QMF_INACTIVE|QMF_GRAYED)) ? 1 : 0;
    case 7: return WebConfig_Choices( item );
    case 8: return page == 9 || UI_WebControlsConfirm();
    case 9:
        if ( page >= 16 ) {
            const char *text = page == 21 ? UI_WebDemoSaveDescription() : page == 16 ? UI_WebLadderWizardDescription() : UI_WebProfileWizardDescription();
            unsigned hash = page; int i;
            while ( *text ) hash = hash * 33u + (unsigned char)*text++;
            for ( i = 0; i < menu->nitems; i++ ) hash = hash * 33u + ((menucommon_s *)menu->items[i])->flags;
            return hash & 0x7fffffff;
        }
        return 0;
    }
    return 0;
}
int UI_WebConfigChar( int kind, int index, int choice, int character ) {
    static const char *titles[] = {"Configuration","Controls","Graphics","Display","Sound","Network","Game options","Q3Rally options","Renderer information","Confirm change","Race control","Add bots","Remove bots","Choose a team","Team orders","Server information"};
    int page; menucommon_s *item = WebConfig_Item( index, &page );
    const char *text = ""; char info[2048];
    if ( page < 0 ) return 0;
    switch ( kind ) {
    case 0:
        if ( page == 21 ) text = "Save demo recording";
        else if ( page == 16 ) text = "Register offline profile";
        else if ( page >= 17 ) { static const char *steps[] = {"Create your driver", "Make it yours", "Offline Ladder", "Ready to race"}; text = steps[page - 17]; }
        else text = page == 2 ? UI_WebGraphicsTitle() : titles[page];
        break;
    case 1:
        if ( item ) {
            if ( item->type == MTYPE_BITMAP ) text = strstr( item->name ? item->name : "", "down" ) ? "Next bots" : strstr( item->name ? item->name : "", "up" ) ? "Previous bots" : "Back";
            else if ( item->type == MTYPE_PTEXT ) text = ((menutext_s *)item)->string;
            else if ( item->type == MTYPE_ACTION ) text = UI_WebControlText( index, 0 );
            else text = item->name ? item->name : "Search controls";
            if ( page == 11 && UI_WebAddBotsLabel( index )[0] ) text = UI_WebAddBotsLabel( index );
            if ( page == 12 && UI_WebRemoveBotsLabel( index )[0] ) text = UI_WebRemoveBotsLabel( index );
            if ( page == 7 && item->id >= 21 && item->id <= 25 ) { Com_sprintf( info, sizeof( info ), "Rear view: %s", text ); text = info; }
            if ( page == 7 && item->id >= 30 && item->id <= 34 ) { Com_sprintf( info, sizeof( info ), "Main view: %s", text ); text = info; }
        }
        break;
    case 2:
        if ( item && item->type == MTYPE_FIELD ) text = ((menufield_s *)item)->field.buffer;
        else if ( item && item->type == MTYPE_ACTION ) text = UI_WebControlText( index, 1 );
        break;
    case 3:
        if ( item && choice >= 0 && choice < WebConfig_Choices( item ) ) text = item->type == MTYPE_RADIOBUTTON ? (choice ? "On" : "Off") : ((menulist_s *)item)->itemnames[choice];
        break;
    case 4:
        if ( page >= 16 ) text = page == 21 ? UI_WebDemoSaveDescription() : page == 16 ? UI_WebLadderWizardDescription() : UI_WebProfileWizardDescription();
        else if ( page == 9 || UI_WebControlsConfirm() ) {
            text = UI_WebConfirmQuestion();
            if ( UI_WebControlsPrompt()[0] ) text = UI_WebControlsPrompt();
        } else if ( page == 15 ) { text = UI_WebServerInfo();
        } else if ( page == 10 ) { text = "Race control and session settings. Online races continue while this menu is open.";
        } else if ( page == 8 ) {
            Com_sprintf( info, sizeof( info ), "Vendor: %s\nRenderer: %s\nVersion: %s\nColor / depth / stencil: %d / %d / %d bits", uis.glconfig.vendor_string, uis.glconfig.renderer_string, uis.glconfig.version_string, uis.glconfig.colorBits, uis.glconfig.depthBits, uis.glconfig.stencilBits ); text = info;
        } else if ( page == 2 ) text = "Apply may restart the renderer. The embedded UI returns after the restart.";
        else if ( page == 4 ) text = "Volume changes are applied with Apply. Changing the sound system or quality may restart audio.";
        else if ( page == 1 ) text = "Choose an action to rebind it. Save or discard your changes when leaving Controls.";
        else text = "Settings use the existing Q3Rally configuration and apply rules.";
        break;
    }
    if ( kind == 5 && page == 18 ) text = UI_WebProfileWizardAvatar( item, choice );
    if ( !text || character < 0 || character >= (int)strlen( text ) ) return 0;
    return (unsigned char)text[character];
}
void UI_WebConfigAction( int action, int index, int value ) {
    int page, count; menucommon_s *item = WebConfig_Item( index, &page );
    menuframework_s *menu = WebConfig_Menu( &page );
    if ( !menu ) return;
    uis.transitionIn = uis.transitionOut = 0;
    if ( action == 6 ) { if ( page >= 17 && page <= 20 ) UI_WebProfileWizardBack(); else if ( menu->key ) menu->key( K_ESCAPE ); else Menu_DefaultKey( menu, K_ESCAPE ); return; }
    if ( !item || item->flags & (QMF_HIDDEN|QMF_INACTIVE|QMF_GRAYED) ) return;
    if ( action == 0 && (item->type == MTYPE_PTEXT || item->type == MTYPE_BITMAP) && item->callback ) {
        if ( page == 0 && item->id == ID_CUSTOMIZEPLAYER ) UI_WebDriverMenu();
        else item->callback( item, QM_ACTIVATED );
    } else if ( action == 7 && page == 14 && item->type == MTYPE_SCROLLLIST ) {
        if ( value < 0 || value >= WebConfig_Choices( item ) ) return;
        ((menulist_s *)item)->curvalue = value;
        if ( item->callback ) item->callback( item, QM_ACTIVATED );
    } else if ( action == 1 && (item->type == MTYPE_SPINCONTROL || item->type == MTYPE_RADIOBUTTON) ) {
        count = WebConfig_Choices( item ); if ( value < 0 || value >= count ) return;
        if ( item->type == MTYPE_RADIOBUTTON ) ((menuradiobutton_s *)item)->curvalue = value;
        else ((menulist_s *)item)->curvalue = value;
        if ( item->callback ) item->callback( item, QM_ACTIVATED );
    } else if ( action == 2 && item->type == MTYPE_SLIDER ) {
        menuslider_s *slider = (menuslider_s *)item; float next = value / 100.0f;
        if ( next < slider->minvalue || next > slider->maxvalue ) return;
        slider->curvalue = next; if ( item->callback ) item->callback( item, QM_ACTIVATED );
    } else if ( (action == 3 || action == 5) && item->type == MTYPE_ACTION ) UI_WebControlsBind( index, action == 5 ? -1 : value );
    else if ( action == 4 && page == 1 && item->type == MTYPE_FIELD ) {
        char text[128]; trap_Cvar_VariableStringBuffer( "ui_webconfig_text", text, sizeof( text ) ); UI_WebControlsSearch( text );
    } else if ( action == 4 && page >= 16 && item->type == MTYPE_FIELD ) {
        menufield_s *field = (menufield_s *)item;
        char text[128]; int limit = field->field.maxchars + 1;
        trap_Cvar_VariableStringBuffer( "ui_webconfig_text", text, sizeof( text ) );
        if ( limit > sizeof( field->field.buffer ) ) limit = sizeof( field->field.buffer );
        if ( limit < 1 ) return;
        Q_strncpyz( field->field.buffer, text, limit );
        field->field.cursor = strlen( field->field.buffer ); field->field.scroll = 0;
    }
}
