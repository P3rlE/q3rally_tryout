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

DEMOS MENU

=======================================================================
*/

#include "ui_local.h"
#include "ui_rally_demoslayout.h"
#include "ui_rally_library.h"
#include "ui_rally_frontend.h"
#include "../qcommon/demo_layout.h"

#define NAMEBUFSIZE             ( MAX_DEMOS * 32 )

#define ID_BACK                 10
#define ID_GO                   11
#define ID_LIST                 12
#define ID_RIGHT                13
#define ID_LEFT                 14
#define ID_PAUSE 15
#define ID_STOP 16
#define ID_RESTART 17
#define ID_REWIND 18
#define ID_FORWARD 19

/* The virtual UI is 640x480. Keep the demo screen aligned with the other
 * modern frontend surfaces and leave the image itself open behind them. */

typedef struct {
    menuframework_s menu;
    menulist_s      list;
    menutext_s      back;
    menutext_s      previous;
    menutext_s      next;
    menutext_s      go;
    menutext_s pause, stop, restart, rewind, forward;
    char playing[MAX_QPATH];
    int infoSelection, infoBytes;
    demosLayout_t   layout;
    float           scroll;             /* #demo-list scrollTop */
    qboolean        dragThumb;
    float           dragY, dragScroll;
    int             lastClick, lastClickRow;

    int             numDemos;
    char            names[NAMEBUFSIZE];
    char            *demolist[MAX_DEMOS];
} demos_t;

static demos_t s_demos;
static vec4_t demosTextColor = UI_FRONTEND_COLOR_TEXT;		/* the save dialog */
static vec4_t demosMutedColor = UI_FRONTEND_COLOR_MUTED;


static void Demos_PlaySelected( void ) {
    char cleanName[MAX_QPATH];

    if ( s_demos.numDemos <= 0 ||
         s_demos.list.curvalue < 0 ||
         s_demos.list.curvalue >= s_demos.numDemos ) {
        return;
    }

    COM_SanitizeFileName( s_demos.list.itemnames[s_demos.list.curvalue],
                          cleanName, sizeof( cleanName ) );
    Q_strncpyz( s_demos.playing, cleanName, sizeof(s_demos.playing) );
    trap_Cvar_Set("ui_demoPlayerName", cleanName);
    trap_Cvar_Set( "cl_freezeDemo", "0" );
    trap_Cmd_ExecuteText( EXEC_APPEND, va( "demopreview \"%s\"\n", cleanName ) );
}

static void Demos_MenuEvent( void *ptr, int event ) {
    menucommon_s *item;

    if ( event != QM_ACTIVATED && event != QM_GOTFOCUS ) {
        return;
    }

    item = (menucommon_s *)ptr;
    switch ( item->id ) {
    case ID_GO:
        if ( event == QM_ACTIVATED ) {
            Demos_PlaySelected();
        }
        break;

    case ID_REWIND:
    case ID_FORWARD:
        if (event == QM_ACTIVATED)
            trap_Cmd_ExecuteText(EXEC_APPEND, item->id == ID_REWIND ? "demoseek -10\n" : "demoseek 10\n");
        break;
    case ID_PAUSE:
        if ( event == QM_ACTIVATED && trap_Cvar_VariableValue("cl_demoPlayer") )
            trap_Cvar_Set("cl_freezeDemo", trap_Cvar_VariableValue("cl_freezeDemo") ? "0" : "1");
        break;
    case ID_STOP:
        if ( event == QM_ACTIVATED ) trap_Cmd_ExecuteText(EXEC_APPEND, "demopreviewstop\n");
        break;
    case ID_RESTART:
        if ( event == QM_ACTIVATED && s_demos.playing[0] ) {
            trap_Cvar_Set("cl_freezeDemo", "0");
            trap_Cmd_ExecuteText(EXEC_APPEND, va("demopreview \"%s\"\n", s_demos.playing));
        }
        break;
    case ID_BACK:
        if ( event == QM_ACTIVATED ) {
            trap_Cmd_ExecuteText(EXEC_APPEND, "demopreviewstop\n");
            UI_PopMenu();
        }
        break;

    }
}

/* -------------------------------------------------------------------------
   The WebUI's #demos-screen (archive.js)
   ------------------------------------------------------------------------- */

static const vec4_t demosList     = { 0x15 / 255.0f, 0x15 / 255.0f, 0x15 / 255.0f, 1.0f };
static const vec4_t demosChecked  = { 0x71 / 255.0f, 0x31 / 255.0f, 0x10 / 255.0f, 1.0f };   /* option:checked */
static const vec4_t demosViewport = { 0x08 / 255.0f, 0x08 / 255.0f, 0x08 / 255.0f, 1.0f };

static qboolean Demos_Playing( void ) {
    return trap_Cvar_VariableValue( "cl_demoPlayer" ) != 0.0f;
}

static qboolean Demos_Seeking( void ) {
    return trap_Cvar_VariableValue( "cl_demoSeeking" ) != 0.0f;
}

/* selectRecording(): name and size of the selected recording */
static void Demos_Details( char *out, int size ) {
    int index = s_demos.list.curvalue;
    fileHandle_t f;

    if ( s_demos.numDemos <= 0 || index < 0 || index >= s_demos.numDemos ) {
        Q_strncpyz( out, "No recordings found. Record a demo during a game to add it here.", size );
        return;
    }
    if ( s_demos.infoSelection != index ) {
        s_demos.infoSelection = index;
        s_demos.infoBytes = trap_FS_FOpenFile( va( "demos/%s", s_demos.demolist[index] ), &f, FS_READ );
        if ( s_demos.infoBytes >= 0 ) trap_FS_FCloseFile( f );
    }
    Com_sprintf( out, size, "%s / %.1f MB", s_demos.demolist[index],
                 ( s_demos.infoBytes > 0 ? s_demos.infoBytes : 0 ) / 1048576.0f );
}

static void Demos_BuildLayout( void ) {
    demosLayout_t *l = &s_demos.layout;
    char message[96];
    int seconds;

    memset( l, 0, sizeof( *l ) );
    l->count = s_demos.numDemos;
    l->scroll = s_demos.scroll;
    Demos_Details( l->details, sizeof( l->details ) );
    trap_Cvar_VariableStringBuffer( "cl_demoError", message, sizeof( message ) );
    if ( !message[0] ) trap_Cvar_VariableStringBuffer( "cl_demoNotice", message, sizeof( message ) );
    /* playback(): the placeholder, the clock, the message and the pause label */
    l->placeholder = !Demos_Playing() || Demos_Seeking();
    Q_strncpyz( l->placeholderText, Demos_Seeking() ? "Seeking..." : message[0] ? message : "Select a recording to preview",
                sizeof( l->placeholderText ) );
    seconds = (int)trap_Cvar_VariableValue( "cl_demoTime" );
    if ( seconds < 0 ) seconds = 0;
    Com_sprintf( l->clock, sizeof( l->clock ), "%d:%02d", seconds / 60, seconds % 60 );
    if ( message[0] ) Q_strncpyz( l->message, message, sizeof( l->message ) );
    else if ( Demos_Seeking() ) Q_strncpyz( l->message, "Seeking...", sizeof( l->message ) );
    else if ( Demos_Playing() ) Q_strncpyz( l->message, trap_Cvar_VariableValue( "cl_freezeDemo" ) ? "Paused" : "Playing",
                                            sizeof( l->message ) );
    Q_strncpyz( l->pause, trap_Cvar_VariableValue( "cl_freezeDemo" ) ? "Resume" : "Pause", sizeof( l->pause ) );
    Demos_Layout( l, WS_ViewportWidth(), WS_ViewportHeight() );
    s_demos.scroll = l->scroll;
}

static void Demos_TrackThumb( void ) {
    const demosLayout_t *l = &s_demos.layout;
    float x, y, track;

    if ( !s_demos.dragThumb ) return;
    if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
        s_demos.dragThumb = qfalse;
        return;
    }
    Library_Cursor( &x, &y );
    track = l->port.h - 2.0f * RIVALS_ARROW - 9.0f - l->thumb.h;
    if ( track > 0.0f ) s_demos.scroll = s_demos.dragScroll + ( y - s_demos.dragY ) * ( l->contentHeight - l->port.h ) / track;
}

static void Demos_DrawOptions( void ) {
    const demosLayout_t *l = &s_demos.layout;
    wsTextStyle_t body;
    float x0 = WS_Round( l->list.x ), y0 = WS_Round( l->list.y ), x1 = WS_Round( l->list.x + l->list.w ), y1 = WS_Round( l->list.y + l->list.h );
    float top = l->port.y, bottom = l->port.y + l->port.h;
    int i;

    WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
    WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, demosList );
    WS_BodyStyle( &body );
    body.lineHeight = 19.0f;	/* an option's normal line */
    WS_SetFade( top, bottom - top, 0.0f );
    WS_SetClipX( l->port.x, l->port.x + l->optionWidth );
    for ( i = 0; i < s_demos.numDemos; i++ ) {
        float y = l->port.y - s_demos.scroll + i * l->optionHeight;
        if ( y >= bottom || y + l->optionHeight <= top ) continue;
        if ( i == s_demos.list.curvalue ) {
            float a = y > top ? y : top, b = y + l->optionHeight < bottom ? y + l->optionHeight : bottom;
            WS_FillRect( WS_Round( l->port.x ), WS_Round( a ), WS_Round( l->optionWidth ), WS_Round( b ) - WS_Round( a ), demosChecked );
        }
        WS_DrawText( &body, l->port.x + WS_LU( 0.6f * WS_REM ), y + WS_LU( 0.4f * WS_REM ), s_demos.demolist[i],
                     strlen( s_demos.demolist[i] ), wsWhite );
    }
    WS_SetFade( 0.0f, 0.0f, 0.0f );
    WS_SetClipX( 0.0f, 0.0f );
    if ( l->scrollbar ) WS_DrawScrollbar( &l->port, &l->thumb );
}

static void Demos_Draw( void ) {
    demosLayout_t *l;
    wsTextStyle_t body, detail;
    qboolean playing, idle;
    int i;

    Demos_TrackThumb();
    Demos_BuildLayout();
    l = &s_demos.layout;
    playing = Demos_Playing();
    idle = !playing || Demos_Seeking();
    WS_BodyStyle( &body );
    detail = body;
    detail.size = 0.9f * WS_REM;
    detail.lineHeight = DEMOS_DETAIL_LINE;

    /* where the cgame renders the preview (cg_view.c) */
    trap_Cvar_Set( "ui_demoRect", va( "%d %d %d %d", (int)WS_Round( l->viewport.x ), (int)WS_Round( l->viewport.y ),
                                      (int)WS_Round( l->viewport.w ), (int)WS_Round( l->viewport.h ) ) );

    WS_DrawBackground( l->pageWidth, l->pageHeight );
    WS_DrawFrontHeader( &l->page, "Your ", "replays.", "" );
    WS_DrawButton( &l->back, "Back to main menu", qtrue, qfalse );

    /* left: the recordings */
    WS_DrawSection( &l->left, qfalse );
    WS_DrawLabel( l->leftX, l->recordingsY, "RECORDINGS", NULL );
    WS_DrawText( &body, l->leftX, l->chooseY, "Choose a recording", 18, wsWhite );
    Demos_DrawOptions();
    WS_DrawLines( &detail, l->leftX, l->detailsY, l->details, &l->detailLines, wsSummary );
    Library_DrawButton( &l->play, "Play demo", qfalse, s_demos.numDemos <= 0 );

    /* right: the preview and the transport */
    WS_DrawSection( &l->right, qfalse );
    WS_DrawLabel( l->rightX, l->previewY, "PREVIEW", NULL );
    WS_FillRect( WS_Round( l->viewport.x ), WS_Round( l->viewport.y ), WS_Round( l->viewport.w ), WS_Round( l->viewport.h ), demosViewport );
    if ( l->placeholder ) {
        WS_DrawText( &body, l->placeholderX, l->placeholderY, l->placeholderText, strlen( l->placeholderText ), wsWhite );
    }
    for ( i = 0; i < 5; i++ ) {
        Library_DrawButton( &l->transport[i], i == 2 ? l->pause : demosTransport[i], i != 2, idle );
    }
    WS_DrawText( &body, l->clockX, l->clockY, l->clock, strlen( l->clock ), wsWhite );
    if ( l->message[0] ) WS_DrawText( &body, l->messageX, l->clockY, l->message, strlen( l->message ), wsSummary );
}

static void Demos_Action( int id ) {
    menucommon_s item;
    item.id = id;
    Demos_MenuEvent( &item, QM_ACTIVATED );
}

static void Demos_Click( float x, float y ) {
    const demosLayout_t *l = &s_demos.layout;
    static const int transport[5] = { ID_RESTART, ID_REWIND, ID_PAUSE, ID_FORWARD, ID_STOP };
    int i, row;

    if ( Library_Inside( &l->back, x, y ) ) { Demos_Action( ID_BACK ); return; }
    if ( Library_Inside( &l->play, x, y ) ) { Demos_PlaySelected(); return; }
    if ( Library_Inside( &l->port, x, y ) ) {
        if ( l->scrollbar && x >= l->port.x + l->port.w - RIVALS_SCROLLBAR ) {
            if ( y < l->port.y + RIVALS_ARROW ) s_demos.scroll -= 40.0f;
            else if ( y >= l->port.y + l->port.h - RIVALS_ARROW ) s_demos.scroll += 40.0f;
            else if ( y < l->thumb.y ) s_demos.scroll -= 0.875f * l->port.h;
            else if ( y >= l->thumb.y + l->thumb.h ) s_demos.scroll += 0.875f * l->port.h;
            else {
                s_demos.dragThumb = qtrue;
                s_demos.dragY = y;
                s_demos.dragScroll = s_demos.scroll;
            }
            return;
        }
        row = (int)( ( y - l->port.y + s_demos.scroll ) / l->optionHeight );
        if ( row >= 0 && row < s_demos.numDemos ) {
            /* a double click plays it, as the page's dblclick */
            if ( row == s_demos.lastClickRow && uis.realtime - s_demos.lastClick < 500 ) Demos_PlaySelected();
            s_demos.list.curvalue = row;
            s_demos.lastClickRow = row;
            s_demos.lastClick = uis.realtime;
        }
        return;
    }
    if ( Demos_Playing() && !Demos_Seeking() ) {
        for ( i = 0; i < 5; i++ ) {
            if ( Library_Inside( &l->transport[i], x, y ) ) { Demos_Action( transport[i] ); return; }
        }
    }
}

static sfxHandle_t Demos_MenuKey( int key ) {
    const demosLayout_t *l = &s_demos.layout;
    qboolean active = Demos_Playing();
    float x, y;
    int row;

    if ( key & K_CHAR_FLAG ) return 0;
    Library_Cursor( &x, &y );
    switch ( key ) {
    case K_ESCAPE:
    case K_PAD0_B:
        Demos_Action( ID_BACK );
        return 0;
    case K_MOUSE1:
        Demos_Click( x, y );
        return 0;
    case K_MWHEELUP:
    case K_MWHEELDOWN:
        if ( Library_Inside( &l->port, x, y ) ) s_demos.scroll += key == K_MWHEELUP ? -100.0f : 100.0f;
        return 0;
    case K_SPACE:
        if ( active ) Demos_Action( ID_PAUSE );
        return 0;
    case K_LEFTARROW:
    case K_KP_LEFTARROW:
        if ( active ) Demos_Action( ID_REWIND );
        return 0;
    case K_RIGHTARROW:
    case K_KP_RIGHTARROW:
        if ( active ) Demos_Action( ID_FORWARD );
        return 0;
    case K_UPARROW:
    case K_KP_UPARROW:
    case K_DOWNARROW:
    case K_KP_DOWNARROW:
        if ( s_demos.numDemos <= 0 ) return 0;
        row = s_demos.list.curvalue + ( key == K_UPARROW || key == K_KP_UPARROW ? -1 : 1 );
        if ( row < 0 ) row = 0;
        if ( row >= s_demos.numDemos ) row = s_demos.numDemos - 1;
        s_demos.list.curvalue = row;
        if ( row * l->optionHeight < s_demos.scroll ) s_demos.scroll = row * l->optionHeight;
        if ( ( row + 1 ) * l->optionHeight > s_demos.scroll + l->port.h ) s_demos.scroll = ( row + 1 ) * l->optionHeight - l->port.h;
        return 0;
    case K_ENTER:
    case K_KP_ENTER:
        Demos_PlaySelected();
        return 0;
    }
    return 0;
}

static void Demos_MenuInit( void ) {
    char preview[64];

    memset( &s_demos, 0, sizeof( s_demos ) );
    s_demos.infoSelection = -1;
    s_demos.lastClickRow = -1;
    trap_Cvar_VariableStringBuffer( "ui_demoPlayerName", s_demos.playing, sizeof( s_demos.playing ) );

    s_demos.menu.draw = Demos_Draw;
    s_demos.menu.key = Demos_MenuKey;
    s_demos.menu.fullscreen = qtrue;

    s_demos.list.itemnames = (const char **)s_demos.demolist;
    s_demos.numDemos = UI_FetchDemoList( s_demos.names, sizeof( s_demos.names ),
                                         (const char **)s_demos.demolist, MAX_DEMOS, NULL );
    s_demos.list.numitems = s_demos.numDemos;
    s_demos.list.curvalue = 0;	/* demosEnd(): the first recording */

    /* ui_demosPreview "selected,scroll" pins a state for screenshots (tools/ui_webstyle) */
    trap_Cvar_VariableStringBuffer( "ui_demosPreview", preview, sizeof( preview ) );
    if ( preview[0] ) {
        char *field = strchr( preview, ',' );
        s_demos.list.curvalue = atoi( preview );
        if ( field ) s_demos.scroll = atof( field + 1 );
    }
}

void Demos_Cache( void ) {
    /* Keep this legacy entry point for existing callers. */
}

void UI_DemosMenu( void ) {
    Demos_MenuInit();
    UI_PushMenu( &s_demos.menu );
}

static void Demos_DrawAction( void *self ) {
    menutext_s *button;
    qboolean focus;
    qboolean disabled;
    vec4_t disabledColor;
    int x;
    int y;
    int width;
    int height;

    button = (menutext_s *)self;
    focus = ( Menu_ItemAtCursor( button->generic.parent ) == button );
    disabled = ( button->generic.flags & QMF_GRAYED ) ? qtrue : qfalse;
    x = button->generic.left;
    y = button->generic.top;
    width = button->generic.right - button->generic.left;
    height = button->generic.bottom - button->generic.top;

    if ( disabled ) {
        Vector4Copy( demosMutedColor, disabledColor );
        disabledColor[3] = 0.35f;
        Frontend_DrawText( x + width / 2, y + 4, button->string,
                           UI_CENTER | UI_SMALLFONT, disabledColor );
        return;
    }

    Frontend_DrawButton( x, y, width, height, button->string, 1.0f,
                         focus, UI_FRONTEND_TEXT_CENTER );
}

/* Naming is offered after the recording has safely been closed. Escape keeps
 * the automatically assigned name, so dismissing this menu never loses a demo. */
static struct {
    menuframework_s menu;
    menufield_s name;
    menutext_s save, keep;
    char original[MAX_QPATH];
    char error[80];
} demoSave;

static void DemoSave_Event(void *ptr, int event) {
    char clean[MAX_QPATH];
    char target[MAX_QPATH];
    const char *ext;
    fileHandle_t file;
    if (event != QM_ACTIVATED) return;
    if (((menucommon_s *)ptr)->id == 1) { UI_PopMenu(); return; }
    COM_SanitizeFileName(demoSave.name.field.buffer, clean, sizeof(clean));
    if (!clean[0] || strcmp(clean, demoSave.name.field.buffer) || strchr(clean, '/') || strchr(clean, 92) || strstr(clean, "..")) {
        Q_strncpyz(demoSave.error, "Please enter a valid filename", sizeof(demoSave.error));
        return;
    }
    ext = strrchr(demoSave.original, '.');
    Com_sprintf(target, sizeof(target), "%s%s", clean, ext ? ext : "");
    if (strcmp(target, demoSave.original)) {
        trap_FS_FOpenFile(va("demos/%s", target), &file, FS_READ);
        if (file) {
            trap_FS_FCloseFile(file);
            Q_strncpyz(demoSave.error, "That filename already exists", sizeof(demoSave.error));
            return;
        }
        trap_Cmd_ExecuteText(EXEC_APPEND, va("demosavename \"%s\"\n", clean));
    }
    UI_PopMenu();
}

static void DemoSave_DrawName(void *self) {
    menufield_s *field = (menufield_s *)self;
    char visible[33], before[33];
    int cursor = field->field.cursor - field->field.scroll;
    Q_strncpyz(visible, field->field.buffer + field->field.scroll, sizeof(visible));
    Frontend_DrawText(132, 224, visible, UI_LEFT | UI_SMALLFONT, demosTextColor);
    if (Menu_ItemAtCursor(&demoSave.menu) == field && (uis.realtime / 500) % 2 == 0) {
        if (cursor < 0) cursor = 0;
        if (cursor > 32) cursor = 32;
        Q_strncpyz(before, visible, cursor + 1);
        Frontend_DrawText(132 + Frontend_TextWidth(before, UI_SMALLFONT), 224,
            "_", UI_LEFT | UI_SMALLFONT, demosTextColor);
    }
}

static void DemoSave_Draw(void) {
    vec4_t scrim = UI_FRONTEND_COLOR_SCRIM;
    Frontend_DrawBackground(scrim);
    Frontend_DrawPanel(96, 120, 448, 240, 1, UI_FRONTEND_STYLE_FRAME);
    Frontend_DrawText(128, 144, "Save demo recording", UI_LEFT | UI_BIGFONT, demosTextColor);
    Frontend_DrawText(128, 188, "Filename (without extension)", UI_LEFT | UI_SMALLFONT, demosMutedColor);
    UI_FillRect(128, 220, 376, 24, colorBlack);
    Frontend_DrawText(128, 266, demoSave.error, UI_LEFT | UI_SMALLFONT, colorRed);
    Menu_Draw(&demoSave.menu);
}

void UI_DemoSaveMenu(void) {
    memset(&demoSave, 0, sizeof(demoSave));
    trap_Cvar_VariableStringBuffer("cl_lastDemoName", demoSave.original, sizeof(demoSave.original));
    COM_StripExtension(demoSave.original, demoSave.name.field.buffer, sizeof(demoSave.name.field.buffer));
    demoSave.menu.fullscreen = qtrue;
    demoSave.menu.wrapAround = qtrue;
    demoSave.menu.draw = DemoSave_Draw;
    demoSave.name.generic.type = MTYPE_FIELD;
    demoSave.name.generic.name = "Filename (without extension)";
    demoSave.name.generic.ownerdraw = DemoSave_DrawName;
    demoSave.name.generic.flags = QMF_NODEFAULTINIT;
    demoSave.name.generic.x = demoSave.name.generic.left = 128;
    demoSave.name.generic.y = demoSave.name.generic.top = 220;
    demoSave.name.generic.right = 504;
    demoSave.name.generic.bottom = 244;
    demoSave.name.field.widthInChars = 32;
    demoSave.name.field.maxchars = 40;
    demoSave.name.field.cursor = strlen(demoSave.name.field.buffer);
    demoSave.save.generic.type = demoSave.keep.generic.type = MTYPE_PTEXT;
    demoSave.save.generic.x = 180; demoSave.keep.generic.x = 390;
    demoSave.save.generic.y = demoSave.keep.generic.y = 308;
    demoSave.save.generic.callback = demoSave.keep.generic.callback = DemoSave_Event;
    demoSave.keep.generic.id = 1;
    demoSave.save.string = "Save"; demoSave.keep.string = "Keep name";
    demoSave.save.color = demoSave.keep.color = demosTextColor;
    demoSave.save.style = demoSave.keep.style = UI_CENTER | UI_SMALLFONT;
    demoSave.save.generic.flags = demoSave.keep.generic.flags = QMF_NODEFAULTINIT | QMF_PULSEIFFOCUS;
    demoSave.save.generic.left = 128;
    demoSave.save.generic.right = 300;
    demoSave.keep.generic.left = 324;
    demoSave.keep.generic.right = 504;
    demoSave.save.generic.top = demoSave.keep.generic.top = 302;
    demoSave.save.generic.bottom = demoSave.keep.generic.bottom = 334;
    demoSave.save.generic.ownerdraw = demoSave.keep.generic.ownerdraw = Demos_DrawAction;
    Menu_AddItem(&demoSave.menu, &demoSave.name);
    Menu_AddItem(&demoSave.menu, &demoSave.save);
    Menu_AddItem(&demoSave.menu, &demoSave.keep);
    UI_PushMenu(&demoSave.menu);
}

int UI_WebDemosScreen( void ) {
    return uis.activemenu == &s_demos.menu ? 4 : 0;
}
menuframework_s *UI_WebDemoSaveMenu( void ) {
    return uis.activemenu == &demoSave.menu ? &demoSave.menu : NULL;
}
const char *UI_WebDemoSaveDescription( void ) {
    return demoSave.error[0] ? demoSave.error : "Choose a filename for your recorded demo, or keep its current name.";
}
int UI_WebDemoCount( void ) { return s_demos.numDemos; }
int UI_WebDemoChar( int index, int character ) {
    if ( index < 0 || index >= s_demos.numDemos || character < 0 ||
         character >= (int)strlen( s_demos.demolist[index] ) ) return 0;
    return (unsigned char)s_demos.demolist[index][character];
}
void UI_WebDemoAction( int action, int index ) {
    menucommon_s item;
    if ( !UI_WebDemosScreen() ) return;
    if ( index >= 0 && index < s_demos.numDemos ) s_demos.list.curvalue = index;
    switch ( action ) {
    case 0: Demos_PlaySelected(); return;
    case 1: item.id = ID_PAUSE; break;
    case 2: item.id = ID_REWIND; break;
    case 3: item.id = ID_FORWARD; break;
    case 4: item.id = ID_RESTART; break;
    case 5: item.id = ID_STOP; break;
    case 6: item.id = ID_BACK; break;
    default: return;
    }
    Demos_MenuEvent( &item, QM_ACTIVATED );
}
