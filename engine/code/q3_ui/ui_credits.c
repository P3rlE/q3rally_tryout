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
Q3Rally Credits Screen

Scrolling credits combining Q3Rally team and original iD Software credits,
drawn like the WebUI's #credits-screen (ui_rally_creditslayout.c,
docs/native-webstyle.md).
- Rolls up through a window at 38 px/s, faded in at its bottom and out at
  its top, then quits the game
- Music volume is raised to 0.5 if below, then restored on exit
- Any key or the "Exit game" button exits and quits the game
===========================================================================
*/

#include "ui_local.h"
#include "ui_rally_frontend.h"
#include "ui_rally_creditslayout.h"

/* -------------------------------------------------------------------------
   Colours: they only tell the line kinds apart (UI_WebCreditKind)
   ------------------------------------------------------------------------- */

static vec4_t color_header     = UI_FRONTEND_COLOR_ACCENT;
static vec4_t color_name       = UI_FRONTEND_COLOR_TEXT;
static vec4_t color_id_header  = UI_FRONTEND_COLOR_MUTED;
static vec4_t color_id_name    = UI_FRONTEND_COLOR_MUTED;
static vec4_t color_title      = UI_FRONTEND_COLOR_TEXT;

/* -------------------------------------------------------------------------
   Credit line structure
   ------------------------------------------------------------------------- */

typedef struct {
    const char *string;     /* NULL marks end of array; "" is a spacer line */
    int         style;
    vec4_t     *colour;
} cr_line_t;

/* -------------------------------------------------------------------------
   Credits data
   ------------------------------------------------------------------------- */

static cr_line_t credits[] = {

    /* ----- Title ----- */
    { "Thank you for playing!", UI_CENTER|UI_BIGFONT|UI_DROPSHADOW,  &color_title  },
    { "",                       UI_CENTER|UI_SMALLFONT,              &color_name   },
    { "Q3Rally Team",           UI_CENTER|UI_BIGFONT|UI_DROPSHADOW,  &color_title  },
    { "",                       UI_CENTER|UI_SMALLFONT,              &color_name   },
    { "",                       UI_CENTER|UI_SMALLFONT,              &color_name   },

    /* ----- Programming ----- */
    { "Programming:",                     UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_header },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "Steven 'Stonelance' Heijsters",    UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Per 'P3rlE' Thormann",             UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Eddy Valdez aka. 'TheBigBuu'",     UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "ZTurtleMan from TurtleArena",      UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Eraser from EntityPlus",           UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Noire",                            UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },

    /* ----- Mapping ----- */
    { "Mapping:",                         UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_header },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "Jeff 'Stecki' Garstecki",          UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Jim 'gout' Bahe",                  UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Simon 'System Krash' Batty",       UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Jonathan 'Amphetamine' Garrod",    UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Michael 'Cyberdemon' Kaminsky",    UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Per 'P3rlE' Thormann",             UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "'MysteriousPoetd' aka 'Poet'",     UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "'OliverV'",                        UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Thomas aka. 'To-mos'",             UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Eddy Valdez aka. 'thebigbuu'",     UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "'ailmanki' aka. 'peyote'",         UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Denis 'insellium' Manylov",        UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },

    /* ----- Art and Models ----- */
    { "Art and Models:",                  UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_header },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "Jeff 'Stecki' Garstecki",          UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Per 'P3rlE' Thormann",             UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Denis 'insellium' Manylov",        UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "'Steel Painter'",                  UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Thomas aka. 'To-mos'",             UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },

    /* ----- Sound and Music ----- */
    { "Sound Design and Music:",          UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_header },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "P.Andersson",                      UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "F.Segerfalk",                      UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Dale Wilson",                      UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Thomas aka. 'To-mos'",             UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },

    /* ----- Texture Design ----- */
    { "Texture Design:",                  UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_header },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "Melanie aka. 'Toxicity'",          UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Eddy Valdez aka. 'thebigbuu'",     UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Denis 'insellium' Manylov",        UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },

    /* ----- Document Design ----- */
    { "Document Design and Layout:",      UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_header },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "Richard Smith",                    UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Per 'P3rlE' Thormann",             UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Mirco 'PaniC' Herrmann",           UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },

    /* ----- Beta Testing ----- */
    { "BETA Testing:",                    UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_header },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "To-Mos",                           UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Eddy Valdez aka. 'thebigbuu'",     UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Per 'P3rlE' Thormann",             UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Mirco 'PaniC' Herrmann",           UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Jeremiah aka. 'BETAMONKEY'",       UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "'Onai'",                           UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "Denis 'insellium' Manylov",        UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },

    /* ----- Special Thanks ----- */
    { "Special Thanks:",                  UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_header },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "Cyberdemon, Killaz and skw|d",     UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },

    /* ----- Footer ----- */
    { PRODUCT_VERSION " | 2002 - 2026 | www.q3rally.com",
                                          UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_header },
    { "It's damn fast baby!",             UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_title  },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name   },

    /* ----- iD Software ----- */
    { "Based on Quake III Arena",         UI_CENTER|UI_BIGFONT|UI_DROPSHADOW,   &color_id_header },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name      },
    { "John Carmack, John Cash",          UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_id_name   },
    { "Adrian Carmack, Kevin Cloud,",     UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_id_name   },
    { "Paul Steed, Kenneth Scott",        UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_id_name   },
    { "Graeme Devine, Tim Willits,",      UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_id_name   },
    { "Christian Antkow, Paul Jaquays",   UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_id_name   },
    { "Todd Hollenshead",                 UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_id_name   },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name      },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name      },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name      },
    { "",                                 UI_CENTER|UI_SMALLFONT,               &color_name      },

    { NULL } /* end marker */
};

/* -------------------------------------------------------------------------
   State
   ------------------------------------------------------------------------- */

typedef struct {
    menuframework_s menu;
    int             startTime;   /* uis.realtime when credits began          */
    float           mvolume;     /* original music volume, restored on exit  */
    qboolean        exiting;
    int             kinds[CREDITS_MAX_LINES];
    creditsLayout_t layout;
} creditsmenu_t;

static creditsmenu_t s_credits;

/* -------------------------------------------------------------------------
   Exit helper - restores music volume and quits
   ------------------------------------------------------------------------- */

static void Credits_Exit( void ) {
    if ( s_credits.exiting ) return;
    s_credits.exiting = qtrue;
    trap_Cmd_ExecuteText( EXEC_APPEND,
                          va( "s_musicvolume %f; quit\n", s_credits.mvolume ) );
}

/* Seconds into the roll; ui_creditsPreview pins it for screenshots
 * (tools/ui_webstyle). */
static float Credits_Seconds( qboolean *pinned ) {
    char value[16];

    trap_Cvar_VariableStringBuffer( "ui_creditsPreview", value, sizeof( value ) );
    *pinned = value[0] != '\0';
    if ( *pinned ) return atof( value );
    return ( uis.realtime - s_credits.startTime ) / 1000.0f;
}

/* -------------------------------------------------------------------------
   Key handler: like the page, any key exits; the mouse only on the button.
   ------------------------------------------------------------------------- */

static sfxHandle_t UI_CreditMenu_Key( int key ) {
    const wsRect_t *b = &s_credits.layout.exit;
    float x, y;

    if ( key & K_CHAR_FLAG )
        return 0;
    if ( key == K_MOUSE1 ) {
        x = uis.cursorx * uis.xscale + uis.bias;
        y = uis.cursory * uis.yscale;
        if ( x >= b->x && x < b->x + b->w && y >= b->y && y < b->y + b->h ) {
            Credits_Exit();
        }
        return 0;
    }
    if ( key >= K_MOUSE2 && key <= K_MOUSE5 ) return 0;
    if ( key == K_MWHEELUP || key == K_MWHEELDOWN ) return 0;

    Credits_Exit();
    return 0;
}

/* -------------------------------------------------------------------------
   Draw function: #credits-screen of the WebUI
   ------------------------------------------------------------------------- */

static void UI_CreditMenu_Draw( void ) {
    creditsLayout_t *l = &s_credits.layout;
    wsTextStyle_t    body, label;
    const wsTextStyle_t *style;
    const float     *color;
    char             shown[128];
    float            seconds, top, width;
    qboolean         pinned;
    int              n;

    seconds = Credits_Seconds( &pinned );
    Credits_Layout( l, WS_ViewportWidth(), WS_ViewportHeight(), s_credits.kinds, UI_WebCreditCount() );

    WS_DrawBackground( l->pageWidth, l->pageHeight );
    WS_DrawFrontHeader( &l->page, "Thank you for ", "playing.", "" );
    WS_DrawButton( &l->exit, creditsExitLabel, qtrue, qfalse );

    /* the roll, cut to its window and faded at both ends */
    WS_BodyStyle( &body );
    WS_LabelStyle( &label );
    WS_SetFade( l->window.y, l->window.h, CREDITS_FADE );
    for ( n = 0; n < l->count; n++ ) {
        top = Credits_LineTop( l, n, seconds );
        if ( top >= l->window.y + l->window.h || top + l->lineH[n] <= l->window.y ) continue;
        if ( !credits[n].string[0] ) continue;
        if ( s_credits.kinds[n] == CREDITS_TITLE ) {
            style = &l->title; color = wsWhite;
        } else if ( s_credits.kinds[n] == CREDITS_HEADING ) {
            style = &label; color = wsAccent;
        } else {
            style = &body; color = wsWhite;
        }
        WS_Transform( style, credits[n].string, shown, sizeof( shown ) );
        width = WS_TextWidth( style, shown, strlen( shown ) );
        WS_DrawText( style, l->window.x + ( l->window.w - width ) * 0.5f, top, shown, strlen( shown ), color );
    }
    WS_SetFade( 0.0f, 0.0f, 0.0f );

    WS_DrawLabel( l->page.contentX, l->hintY, creditsHint, NULL );

    /* the animation's end: the last line has left the window */
    if ( !pinned && seconds >= l->duration ) {
        Credits_Exit();
    }
}

/* -------------------------------------------------------------------------
   Public entry point
   ------------------------------------------------------------------------- */

void UI_CreditMenu( void ) {
    int n;

    memset( &s_credits, 0, sizeof( s_credits ) );

    s_credits.menu.draw       = UI_CreditMenu_Draw;
    s_credits.menu.key        = UI_CreditMenu_Key;
    s_credits.menu.fullscreen = qtrue;

    s_credits.startTime   = uis.realtime;
    for ( n = 0; n < UI_WebCreditCount() && n < CREDITS_MAX_LINES; n++ ) {
        s_credits.kinds[n] = UI_WebCreditKind( n );
    }

    s_credits.mvolume = trap_Cvar_VariableValue( "s_musicvolume" );
    if ( s_credits.mvolume < 0.5f )
        trap_Cmd_ExecuteText( EXEC_APPEND, "s_musicvolume 0.5\n" );
    trap_Cmd_ExecuteText( EXEC_APPEND, "music music/credits\n" );

    uis.transitionIn = 0;
    UI_PushMenu( &s_credits.menu );
    m_entersound = qfalse;
}

int UI_WebCreditsScreen( void ) {
    return uis.activemenu == &s_credits.menu ? 3 : 0;
}
int UI_WebCreditCount( void ) { return ARRAY_LEN( credits ) - 1; }
int UI_WebCreditChar( int index, int character ) {
    if ( index < 0 || index >= UI_WebCreditCount() || character < 0 ||
         character >= (int)strlen( credits[index].string ) ) return 0;
    return (unsigned char)credits[index].string[character];
}
int UI_WebCreditKind( int index ) {
    if ( index < 0 || index >= UI_WebCreditCount() ) return 0;
    if ( credits[index].style & UI_BIGFONT ) return 2;
    return credits[index].colour == &color_header ? 1 : 0;
}
void UI_WebCreditsExit( void ) {
    if ( UI_WebCreditsScreen() ) Credits_Exit();
}
