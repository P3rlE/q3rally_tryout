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

#include "ui_local.h"
#include "ui_rally_frontend.h"
static qhandle_t UI_GenerateBotPlateShader( const char *botName, int botIndex, char *plateShaderName, int plateShaderNameSize ) {
    char          output[MAX_QPATH];
    char          shaderName[MAX_QPATH];
    qhandle_t     h;

    Com_sprintf( output, sizeof(output), "models/players/plates/player%d.tga", botIndex );
    Com_sprintf( shaderName, sizeof(shaderName), "models/players/plates/player%d", botIndex );
    CreateLicensePlateImage( "models/players/plates/usa_california.tga", output, botName, 10 );

    h = trap_R_RegisterShader( shaderName );
    if ( plateShaderName && plateShaderNameSize > 0 ) {
        Q_strncpyz( plateShaderName, va("player%d", botIndex), plateShaderNameSize );
    }
    Com_Printf( "Q3R UI Plate: bot %d (%s) -> %s (handle %d)\n", botIndex, botName, shaderName, h );
    return h;
}

/* forward declaration — defined later in this file */
static void UI_CanonicalizeWeapon( const char *in, char *out, int outSize );

/* ==========================================================
   WEAPON TEXT -> weapon_t MAPPING
   ========================================================== */

typedef struct { const char *canon; weapon_t wp; } weaponEnumMap_t;
static const weaponEnumMap_t s_weaponEnumMap[] = {
    { "GAUNTLET",        WP_GAUNTLET        },
    { "CHAINSAW",        WP_GAUNTLET        },
    { "MACHINEGUN",      WP_MACHINEGUN      },
    { "SHOTGUN",         WP_SHOTGUN         },
    { "GRENADELAUNCHER", WP_GRENADE_LAUNCHER},
    { "ROCKETLAUNCHER",  WP_ROCKET_LAUNCHER },
    { "LIGHTNING",       WP_LIGHTNING       },
    { "RAILGUN",         WP_RAILGUN         },
    { "PLASMAGUN",       WP_PLASMAGUN       },
    { "BFG10K",          WP_BFG             },
    { "FLAMETHROWER",    WP_FLAME_THROWER   },
};
#define WEAPONENUMMAP_COUNT ((int)(sizeof(s_weaponEnumMap)/sizeof(s_weaponEnumMap[0])))

static weapon_t UI_WeaponEnumFromText( const char *favoriteText ) {
    char canon[32];
    int  i;
    if ( !favoriteText || !*favoriteText ) return WP_MACHINEGUN;
    UI_CanonicalizeWeapon( favoriteText, canon, sizeof(canon) );
    for ( i = 0; i < WEAPONENUMMAP_COUNT; i++ ) {
        if ( Q_stricmp( s_weaponEnumMap[i].canon, canon ) == 0 )
            return s_weaponEnumMap[i].wp;
    }
    return WP_MACHINEGUN;   /* sensible default */
}

/* ==========================================================
   Icon helper: models/players/<model>/icon_<skin>
   - Do not pass a file extension (engine resolves .tga/.jpg)
   - Fallback order: icon_<skin> -> icon_default -> placeholder
   --------------------------------------------------------- */
static qhandle_t UI_LoadModelIconFor( const char *modelSkin ) {
    char model[MAX_QPATH] = "roadster";
    char skin [MAX_QPATH] = "default";
    const char *slash = NULL;
    char path[MAX_QPATH];
    qhandle_t h;

    if (modelSkin && *modelSkin) {
        slash = strchr(modelSkin, '/');
        if (slash) {
            int len = (int)(slash - modelSkin);
            if (len >= (int)sizeof(model)) len = (int)sizeof(model) - 1;
            Q_strncpyz(model, modelSkin, len + 1);
            Q_strncpyz(skin, slash + 1, sizeof(skin));
        } else {
            Q_strncpyz(model, modelSkin, sizeof(model));
        }
    }

    Com_sprintf(path, sizeof(path), "models/players/%s/icon_%s", model, skin);
    h = trap_R_RegisterShaderNoMip(path);
    if (h) return h;

    Com_sprintf(path, sizeof(path), "models/players/%s/icon_default", model);
    h = trap_R_RegisterShaderNoMip(path);
    if (h) return h;

    return trap_R_RegisterShaderNoMip("menu/art/unknownbot");
}

/* Canonicalize free-text weapon names from bots.txt into tokens like "ROCKETLAUNCHER" */
static void UI_CanonicalizeWeapon( const char *in, char *out, int outSize ) {
    int i = 0, j = 0;
    char c;

    if (!out || outSize <= 0) {
        return;
    }
    out[0] = '\0';

    if (!in || !*in) {
        return;
    }

    /* Uppercase letters, drop spaces/hyphens/underscores; keep digits (rare) */
    while ((c = in[i++]) != '\0' && j < outSize - 1) {
        if (c == ' ' || c == '-' || c == '_') {
            continue;
        }
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        out[j++] = c;
    }
    out[j] = '\0';

    /* Strip leading "WEAPON" if present */
    if (!Q_stricmpn(out, "WEAPON", 6)) {
        int k = 0;
        while (out[6 + k]) { out[k] = out[6 + k]; k++; }
        out[k] = '\0';
    }
}

/* Canonical token -> icon shader (icons/iconw_*) */
#define NAME_BUFSIZE 64
#define DESC_BUFSIZE 256

static char botNames[MAX_BOTS][NAME_BUFSIZE];
static char botModels[MAX_BOTS][NAME_BUFSIZE];
static char botAIFiles[MAX_BOTS][NAME_BUFSIZE];
static char botDescriptions[MAX_BOTS][DESC_BUFSIZE];
static char botPersonalities[MAX_BOTS][DESC_BUFSIZE];
static char botFavWeapon[MAX_BOTS][DESC_BUFSIZE];
static char botPlateNames[MAX_BOTS][NAME_BUFSIZE];
static qhandle_t botIcons[MAX_BOTS];
static qhandle_t botPlateShaders[MAX_BOTS];
static weapon_t  botWeapons[MAX_BOTS];
static int botCount = 0;

#include "ui_rally_rivalslayout.h"

typedef struct {
    menuframework_s menu;
    rivalsLayout_t  layout;
    char            query[64];          /* #library-search */
    qboolean        searchFocus;
    int             searchFocusTime;
    int             matches[MAX_BOTS];  /* the rivals the search leaves, in order */
    int             matchCount;
    int             selected;           /* rival index, -1 none */
    float           scroll;             /* #library-list scrollTop */
    qboolean        dragCar, dragThumb;
    float           dragX, dragY, dragScroll;
    float           yaw, elevation, distance;
} botsmenu_t;

static botsmenu_t s_bots;
static playerInfo_t s_garagePlayerInfo;

#define RIVALS_VIEW_DISTANCE  2.1f      /* library.js: openVehicleShowroom( ..., {distance: 2.1} ) */

/* Apply model, weapon and plate for a selected rival */
static void UI_BotsMenu_SetRival( int index ) {
    vec3_t viewAngles  = { 0, 180, 0 };
    vec3_t moveAngles  = { 0,   0, 0 };
    char   plate[MAX_QPATH];
    weapon_t wp;

    if ( index < 0 || index >= botCount ) return;

    wp = botWeapons[index];
    /* The Chainsaw is represented by WP_GAUNTLET for gameplay, but its
     * hand-weapon preview is not meaningful on a car's tag_weapon mount.
     * Keep the favorite weapon in the profile text/icon and render the car
     * without an attached weapon to avoid entering that legacy path. */
    if ( wp == WP_GAUNTLET ) {
        wp = WP_NONE;
    }

    /*
     * Reload/invalidate point for RIVALS:
     * regenerate and re-register plate shader whenever a rival is (re)selected
     * so the latest generated file is always bound.
     */
    botPlateShaders[index] = 0;
    botPlateShaders[index] = UI_GenerateBotPlateShader( botNames[index], index, botPlateNames[index], sizeof(botPlateNames[index]) );

    if ( botPlateNames[index][0] ) {
        Q_strncpyz( plate, botPlateNames[index], sizeof(plate) );
    } else {
        trap_Cvar_VariableStringBuffer( "plate", plate, sizeof(plate) );
        if ( !plate[0] ) Q_strncpyz( plate, "usa_california", sizeof(plate) );
    }

    UI_PlayerInfo_SetModel( &s_garagePlayerInfo, botModels[index],
                            DEFAULT_RIM, DEFAULT_HEAD, plate );

    /* Ensure plateShader points to the exact freshly-generated shader handle. */
    if ( botPlateShaders[index] ) {
        Com_Printf( "RIVALS: Bot %d uses generated plate '%s' with marker '%s' (handle %d)\n",
                    index, botPlateNames[index], plate, botPlateShaders[index] );
        s_garagePlayerInfo.plateShader = botPlateShaders[index];
    }

    UI_PlayerInfo_SetInfo( &s_garagePlayerInfo,
        LEGS_IDLE, TORSO_STAND,
        viewAngles, moveAngles,
        wp, qfalse );
}


static void UI_BotsMenu_ParseBots(void) {
    char *text_p;
    char *token;
    char *buffer;
    int len;
    fileHandle_t f;
    char name[NAME_BUFSIZE];
    char model[NAME_BUFSIZE];
    char aifile[NAME_BUFSIZE];
    char description[DESC_BUFSIZE];
    char personality[DESC_BUFSIZE];
    char favoriteweapon[DESC_BUFSIZE];

    len = trap_FS_FOpenFile("scripts/bots.txt", &f, FS_READ);
    if (!f) return;
    buffer = (char *)UI_Alloc(len + 1);
    trap_FS_Read(buffer, len, f);
    buffer[len] = '\0';
    trap_FS_FCloseFile(f);

    text_p = buffer;
    botCount = 0;

    while (1) {
        token = COM_ParseExt(&text_p, qtrue);
        if (!token[0]) break;
        if (token[0] != '{') continue;

        name[0] = '\0';
        model[0] = '\0';
        aifile[0] = '\0';
        description[0] = '\0';
        personality[0] = '\0';
        favoriteweapon[0] = '\0';

        while (1) {
            token = COM_ParseExt(&text_p, qtrue);
            if (!token[0]) break;
            if (token[0] == '}') break;

            if (!Q_stricmp(token, "name")) {
                token = COM_ParseExt(&text_p, qfalse);
                Q_strncpyz(name, token, NAME_BUFSIZE);
            } else if (!Q_stricmp(token, "model")) {
                token = COM_ParseExt(&text_p, qfalse);
                Q_strncpyz(model, token, NAME_BUFSIZE);
            } else if (!Q_stricmp(token, "aifile")) {
                token = COM_ParseExt(&text_p, qfalse);
                Q_strncpyz(aifile, token, NAME_BUFSIZE);
            } else if (!Q_stricmp(token, "description")) {
                token = COM_ParseExt(&text_p, qfalse);
                Q_strncpyz(description, token, DESC_BUFSIZE);
            } else if (!Q_stricmp(token, "personality")) {
                token = COM_ParseExt(&text_p, qfalse);
                Q_strncpyz(personality, token, DESC_BUFSIZE);
            } else if (!Q_stricmp(token, "favoriteweapon")) {
                token = COM_ParseExt(&text_p, qfalse);
                Q_strncpyz(favoriteweapon, token, DESC_BUFSIZE);
            } else {
                token = COM_ParseExt(&text_p, qfalse);
            }
        }

        if (botCount < MAX_BOTS) {
            Q_strncpyz(botNames[botCount], name, NAME_BUFSIZE);
            Q_strncpyz(botModels[botCount], model, NAME_BUFSIZE);
            Q_strncpyz(botAIFiles[botCount], aifile, NAME_BUFSIZE);
            Q_strncpyz(botDescriptions[botCount], description, DESC_BUFSIZE);
            Q_strncpyz(botPersonalities[botCount], personality, DESC_BUFSIZE);
            Q_strncpyz(botFavWeapon[botCount], favoriteweapon, DESC_BUFSIZE);
            botPlateNames[botCount][0] = '\0';
            botIcons[botCount]        = UI_LoadModelIconFor(model);
            botWeapons[botCount]      = UI_WeaponEnumFromText(favoriteweapon);
            botPlateShaders[botCount] = 0;
            
            botCount++;
        }
    }
}


static const vec4_t rivalsSelected = { 0x3b / 255.0f, 0x23 / 255.0f, 0x17 / 255.0f, 1.0f };   /* aria-pressed */
static const vec4_t rivalsInput    = { 0x11 / 255.0f, 0x11 / 255.0f, 0x11 / 255.0f, 1.0f };
static const vec4_t rivalsHolder   = { 0x75 / 255.0f, 0x75 / 255.0f, 0x75 / 255.0f, 1.0f };   /* ::placeholder */
static const vec4_t rivalsRing     = { 0xee / 255.0f, 0xee / 255.0f, 0xee / 255.0f, 1.0f };   /* focus ring */
static const vec4_t rivalsTrack    = { 0x2c / 255.0f, 0x2c / 255.0f, 0x2c / 255.0f, 1.0f };   /* dark scrollbar */
static const vec4_t rivalsThumb    = { 0x9f / 255.0f, 0x9f / 255.0f, 0x9f / 255.0f, 1.0f };

/* -------------------------------------------------------------------------
   Search and selection, as library.js does it
   ------------------------------------------------------------------------- */

static void Rivals_Lower( const char *in, char *out, int size ) {
    int i;
    for ( i = 0; in[i] && i < size - 1; i++ ) out[i] = tolower( (unsigned char)in[i] );
    out[i] = '\0';
}

/* name, model, description and personality joined, matched case-insensitively */
static qboolean Rivals_Matches( int index, const char *query ) {
    char joined[2 * DESC_BUFSIZE + 2 * NAME_BUFSIZE + 4], lower[sizeof( joined )];
    if ( !query[0] ) return qtrue;
    Com_sprintf( joined, sizeof( joined ), "%s %s %s %s", botNames[index], botModels[index],
                 botDescriptions[index], botPersonalities[index] );
    Rivals_Lower( joined, lower, sizeof( lower ) );
    return strstr( lower, query ) != NULL;
}

static void Rivals_Select( int index ) {
    if ( index == s_bots.selected ) return;
    s_bots.selected = index;
    /* a new showroom per rival: the camera starts over */
    s_bots.yaw = WS_VIEW_AZIMUTH;
    s_bots.elevation = WS_VIEW_ELEVATION;
    s_bots.distance = RIVALS_VIEW_DISTANCE;
    s_bots.dragCar = qfalse;
    if ( index >= 0 ) UI_BotsMenu_SetRival( index );
}

/* renderList(): filter, keep the selected rival if it still matches, else the first */
static void Rivals_Filter( void ) {
    char query[64], *start, *end;
    int i, keep = -1;

    Rivals_Lower( s_bots.query, query, sizeof( query ) );
    for ( start = query; *start == ' '; start++ ) {
    }
    for ( end = start + strlen( start ); end > start && end[-1] == ' '; end-- ) {
    }
    *end = '\0';
    s_bots.matchCount = 0;
    for ( i = 0; i < botCount; i++ ) {
        if ( !Rivals_Matches( i, start ) ) continue;
        if ( i == s_bots.selected ) keep = i;
        s_bots.matches[s_bots.matchCount++] = i;
    }
    Rivals_Select( keep >= 0 ? keep : ( s_bots.matchCount ? s_bots.matches[0] : -1 ) );
}

static int Rivals_Row( int index ) {
    int i;
    for ( i = 0; i < s_bots.matchCount; i++ ) {
        if ( s_bots.matches[i] == index ) return i;
    }
    return -1;
}

/* -------------------------------------------------------------------------
   Layout
   ------------------------------------------------------------------------- */

static void Rivals_Clean( const char *in, char *out, int size ) {
    /* cleanWeapon(): color codes out, trimmed */
    int n = 0;
    for ( ; *in && n < size - 1; in++ ) {
        if ( *in == '^' && in[1] >= '0' && in[1] <= '9' ) { in++; continue; }
        out[n++] = *in;
    }
    out[n] = '\0';
    while ( n > 0 && out[n - 1] == ' ' ) out[--n] = '\0';
    for ( n = 0; out[n] == ' '; n++ ) {
    }
    if ( n ) memmove( out, out + n, strlen( out + n ) + 1 );
}

static void Rivals_BuildLayout( void ) {
    rivalsLayout_t *l = &s_bots.layout;
    int index = s_bots.selected;
    char weapon[DESC_BUFSIZE];

    l->count = s_bots.matchCount;
    l->scroll = s_bots.scroll;
    Q_strncpyz( l->name, index >= 0 ? botNames[index] : "No rivals found", sizeof( l->name ) );
    Q_strncpyz( l->personality, index >= 0 ? botPersonalities[index] : "", sizeof( l->personality ) );
    l->weapon[0] = '\0';
    if ( index >= 0 ) {
        Rivals_Clean( botFavWeapon[index], weapon, sizeof( weapon ) );
        if ( weapon[0] ) Com_sprintf( l->weapon, sizeof( l->weapon ), "Favorite weapon / %s", weapon );
    }
    Q_strncpyz( l->description, index >= 0 ? botDescriptions[index] : "", sizeof( l->description ) );
    Q_strncpyz( l->status, index >= 0 ? "Drag to rotate / scroll to zoom" : "", sizeof( l->status ) );
    Rivals_Layout( l, WS_ViewportWidth(), WS_ViewportHeight() );
    s_bots.scroll = l->scroll;
}

/* -------------------------------------------------------------------------
   Drawing
   ------------------------------------------------------------------------- */

/* a rectangle cut to the list's scrollport */
static void Rivals_FillClipped( float x, float y, float w, float h, const float *color ) {
    const wsRect_t *list = &s_bots.layout.list;
    float top = y > list->y ? y : list->y;
    float bottom = y + h < list->y + list->h ? y + h : list->y + list->h;
    if ( bottom > top ) WS_FillRect( x, top, w, bottom - top, color );
}

static void Rivals_DrawButton( const wsRect_t *b, int index ) {
    const rivalsLayout_t *l = &s_bots.layout;
    wsTextStyle_t bold;
    qboolean pressed = index == s_bots.selected;
    float x0 = WS_Round( b->x ), y0 = WS_Round( b->y ), x1 = WS_Round( b->x + b->w ), y1 = WS_Round( b->y + b->h );
    float iconX, iconY, top, bottom;

    Rivals_FillClipped( x0, y0, x1 - x0, y1 - y0, pressed ? wsAccent : wsLine );
    Rivals_FillClipped( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, pressed ? rivalsSelected : wsSurface );

    /* the icon, object-fit: contain in 3.5rem x 2.5rem: the square icons are 40 x 40 */
    iconX = WS_Round( b->x + 1.0f + WS_LU( WS_BUTTON_PAD_X ) + 8.0f );
    iconY = WS_Round( b->y + 1.0f + WS_LU( WS_BUTTON_PAD_Y ) );
    top = iconY > l->list.y ? iconY : l->list.y;
    bottom = iconY + 40.0f < l->list.y + l->list.h ? iconY + 40.0f : l->list.y + l->list.h;
    if ( bottom > top && botIcons[index] ) {
        trap_R_SetColor( NULL );
        trap_R_DrawStretchPic( iconX, top, 40.0f, bottom - top, 0.0f, ( top - iconY ) / 40.0f,
                               1.0f, ( bottom - iconY ) / 40.0f, botIcons[index] );
    }

    WS_BodyStyle( &bold );
    bold.font = WS_BODY_BOLD;
    WS_SetFade( l->list.y, l->list.h, 0.0f );
    WS_DrawText( &bold, b->x + 1.0f + WS_LU( WS_BUTTON_PAD_X ) + 3.5f * WS_REM + WS_LU( 0.7f * WS_REM ),
                 b->y + 1.0f + WS_LU( WS_BUTTON_PAD_Y ) + 8.0f, botNames[index], strlen( botNames[index] ), wsWhite );
    WS_SetFade( 0.0f, 0.0f, 0.0f );
}

/* Chrome's dark classic scrollbar: track, arrow buttons, rounded thumb */
static void Rivals_DrawScrollbar( void ) {
    static qhandle_t art;
    const rivalsLayout_t *l = &s_bots.layout;
    float x = WS_Round( l->list.x + l->list.w - RIVALS_SCROLLBAR ), top = WS_Round( l->list.y );
    float bottom = WS_Round( l->list.y + l->list.h ), y0, y1;

    if ( !l->scrollbar ) return;
    if ( !art ) art = trap_R_RegisterShaderNoMip( "menu/ui/web_scrollbar" );
    WS_FillRect( x, top, RIVALS_SCROLLBAR, bottom - top, rivalsTrack );
    trap_R_SetColor( rivalsThumb );
    trap_R_DrawStretchPic( x, top, 15, 15, 0, 0, 15 / 16.0f, 15 / 32.0f, art );
    trap_R_DrawStretchPic( x, bottom - 15, 15, 15, 0, 15 / 32.0f, 15 / 16.0f, 0, art );
    y0 = WS_Round( l->thumb.y );
    y1 = floor( l->thumb.y + l->thumb.h ) + 1.0f;
    trap_R_DrawStretchPic( x, y0, 15, 5, 0, 15 / 32.0f, 15 / 16.0f, 20 / 32.0f, art );
    trap_R_DrawStretchPic( x, y1 - 5, 15, 5, 0, 20 / 32.0f, 15 / 16.0f, 25 / 32.0f, art );
    trap_R_SetColor( NULL );
    if ( y1 - 5 > y0 + 5 ) WS_FillRect( x + 3, y0 + 5, 9, y1 - y0 - 10, rivalsThumb );
}

static void Rivals_DrawSearch( void ) {
    const rivalsLayout_t *l = &s_bots.layout;
    const wsRect_t *r = &l->search;
    wsTextStyle_t body, cancel;
    float x0 = WS_Round( r->x ), y0 = WS_Round( r->y ), x1 = WS_Round( r->x + r->w ), y1 = WS_Round( r->y + r->h );
    float textX = r->x + 1.0f + WS_LU( 0.6f * WS_REM ), lineTop = r->y + 1.0f + WS_LU( 0.6f * WS_REM );

    WS_BodyStyle( &body );
    if ( s_bots.searchFocus ) {
        /* the focus ring, 2px #eee over the border */
        WS_FillRect( x0 - 1, y0 - 1, x1 - x0 + 2, y1 - y0 + 2, rivalsRing );
        WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, rivalsInput );
    } else {
        WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
        WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, rivalsInput );
    }
    if ( s_bots.query[0] ) {
        WS_DrawText( &body, textX, lineTop, s_bots.query, strlen( s_bots.query ), wsWhite );
        if ( s_bots.searchFocus ) {
            /* the search field's cancel button */
            cancel = body;
            cancel.font = WS_BODY_BOLD;
            cancel.size = 20.0f;
            cancel.lineHeight = WS_BODY_LINE;
            WS_DrawText( &cancel, x1 - 1.0f - WS_LU( 0.6f * WS_REM ) - 12.0f, lineTop - 1.0f, "\xd7", 1, wsWhite );
        }
    } else {
        WS_DrawText( &body, textX, lineTop, rivalsPlaceholder, strlen( rivalsPlaceholder ), rivalsHolder );
    }
    if ( s_bots.searchFocus && ( ( uis.realtime - s_bots.searchFocusTime ) / 500 ) % 2 == 0 ) {
        float caret = WS_Round( textX + WS_TextWidth( &body, s_bots.query, strlen( s_bots.query ) ) );
        WS_FillRect( caret, WS_Round( lineTop + 3.0f ), 1, 19, wsWhite );
    }
}

static void Rivals_Orbit( void ) {
    const wsRect_t *car = &s_bots.layout.car;
    float x = uis.cursorx * uis.xscale + uis.bias, y = uis.cursory * uis.yscale;

    if ( s_bots.dragCar ) {
        if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
            s_bots.dragCar = qfalse;
        } else if ( car->h > 0.0f ) {
            s_bots.yaw = AngleNormalize360( s_bots.yaw + 360.0f * ( x - s_bots.dragX ) / car->h );
            s_bots.elevation = WS_Clamp( WS_VIEW_MIN_ELEVATION, s_bots.elevation + 360.0f * ( y - s_bots.dragY ) / car->h,
                                         WS_VIEW_MAX_ELEVATION );
        }
    }
    if ( s_bots.dragThumb ) {
        const rivalsLayout_t *l = &s_bots.layout;
        float track = l->list.h - 2.0f * RIVALS_ARROW - 9.0f - l->thumb.h;
        if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
            s_bots.dragThumb = qfalse;
        } else if ( track > 0.0f ) {
            s_bots.scroll = s_bots.dragScroll + ( y - s_bots.dragY ) * ( l->contentHeight - l->list.h ) / track;
        }
        return;
    }
    s_bots.dragX = x;
    s_bots.dragY = y;
}

static void UI_BotsMenu_Draw( void ) {
    rivalsLayout_t *l = &s_bots.layout;
    wsTextStyle_t body, label;
    wsRect_t b;
    int row;

    Rivals_Orbit();
    Rivals_BuildLayout();
    WS_BodyStyle( &body );
    WS_LabelStyle( &label );

    WS_DrawBackground( l->pageWidth, l->pageHeight );
    WS_DrawFrontHeader( &l->page, "Meet your ", "rivals.", "" );
    WS_DrawButton( &l->back, rivalsBack, qtrue, qfalse );

    /* left: search, count, list */
    WS_DrawSection( &l->left, qfalse );
    WS_DrawText( &body, l->leftX, l->searchLabelY, "Search", 6, wsWhite );
    Rivals_DrawSearch();
    {
        char count[32];
        Com_sprintf( count, sizeof( count ), "%d rivals", s_bots.matchCount );
        WS_DrawText( &body, l->leftX, l->countY, count, strlen( count ), wsWhite );
    }
    for ( row = 0; row < s_bots.matchCount; row++ ) {
        Rivals_Button( l, row, &b );
        if ( b.y >= l->list.y + l->list.h || b.y + b.h <= l->list.y ) continue;
        Rivals_DrawButton( &b, s_bots.matches[row] );
    }
    Rivals_DrawScrollbar();

    /* right: the rival's profile */
    WS_DrawSection( &l->right, qtrue );
    WS_DrawLabel( l->rightX, l->labelY, rivalsDetailLabel, NULL );
    WS_DrawLines( &l->title, l->rightX, l->nameY, l->name, &l->nameLines, wsWhite );
    if ( s_bots.selected >= 0 ) {
        UI_DrawVehicleWebView( l->car.x, l->car.y, l->car.w, l->car.h, &s_garagePlayerInfo, uis.realtime,
                               s_bots.yaw, s_bots.elevation, s_bots.distance );
    }
    WS_DrawLines( &body, l->rightX, l->personalityY, l->personality, &l->personalityLines, wsSummary );
    WS_DrawLines( &body, l->rightX, l->weaponY, l->weapon, &l->weaponLines, wsAccent );
    WS_DrawLines( &body, l->rightX, l->descriptionY, l->description, &l->descriptionLines, wsSummary );
    WS_DrawLines( &body, l->rightX, l->statusY, l->status, &l->statusLines, wsSummary );

    Menu_Draw( &s_bots.menu );
}

/* -------------------------------------------------------------------------
   Input
   ------------------------------------------------------------------------- */

static qboolean Rivals_Inside( const wsRect_t *r, float x, float y ) {
    return r->w > 0.0f && x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

static void Rivals_ScrollTo( int row ) {
    const rivalsLayout_t *l = &s_bots.layout;
    float top = row * ( l->rowHeight + l->rowGap );
    if ( top < s_bots.scroll ) s_bots.scroll = top;
    if ( top + l->rowHeight > s_bots.scroll + l->list.h ) s_bots.scroll = top + l->rowHeight - l->list.h;
}

static void Rivals_Click( float x, float y ) {
    const rivalsLayout_t *l = &s_bots.layout;
    wsRect_t b, cancel;
    int row;

    s_bots.searchFocus = qfalse;
    if ( Rivals_Inside( &l->back, x, y ) ) {
        UI_PopMenu();
        return;
    }
    if ( Rivals_Inside( &l->search, x, y ) ) {
        cancel = l->search;
        cancel.x = l->search.x + l->search.w - 1.0f - WS_LU( 0.6f * WS_REM ) - 16.0f;
        cancel.w = 16.0f + WS_LU( 0.6f * WS_REM );
        if ( s_bots.query[0] && Rivals_Inside( &cancel, x, y ) ) {
            s_bots.query[0] = '\0';
            Rivals_Filter();
        }
        s_bots.searchFocus = qtrue;
        s_bots.searchFocusTime = uis.realtime;
        return;
    }
    if ( Rivals_Inside( &l->list, x, y ) ) {
        if ( l->scrollbar && x >= l->list.x + l->list.w - RIVALS_SCROLLBAR ) {
            if ( y < l->list.y + RIVALS_ARROW ) s_bots.scroll -= 40.0f;
            else if ( y >= l->list.y + l->list.h - RIVALS_ARROW ) s_bots.scroll += 40.0f;
            else if ( y < l->thumb.y ) s_bots.scroll -= 0.875f * l->list.h;
            else if ( y >= l->thumb.y + l->thumb.h ) s_bots.scroll += 0.875f * l->list.h;
            else {
                s_bots.dragThumb = qtrue;
                s_bots.dragY = y;
                s_bots.dragScroll = s_bots.scroll;
            }
            return;
        }
        for ( row = 0; row < s_bots.matchCount; row++ ) {
            Rivals_Button( l, row, &b );
            if ( Rivals_Inside( &b, x, y ) ) {
                Rivals_Select( s_bots.matches[row] );
                return;
            }
        }
        return;
    }
    if ( s_bots.selected >= 0 && Rivals_Inside( &l->car, x, y ) ) {
        s_bots.dragCar = qtrue;
        s_bots.dragX = x;
        s_bots.dragY = y;
    }
}

static sfxHandle_t UI_BotsMenu_Key( int key ) {
    const rivalsLayout_t *l = &s_bots.layout;
    float x = uis.cursorx * uis.xscale + uis.bias, y = uis.cursory * uis.yscale;
    int row, length;

    if ( key & K_CHAR_FLAG ) {
        int ch = key & ~K_CHAR_FLAG;
        if ( !s_bots.searchFocus ) return 0;
        length = strlen( s_bots.query );
        if ( ch == 8 ) {
            if ( length ) s_bots.query[length - 1] = '\0';
        } else if ( ch >= 32 && ch != 127 && ch < 256 && length < (int)sizeof( s_bots.query ) - 1 ) {
            s_bots.query[length] = (char)ch;
            s_bots.query[length + 1] = '\0';
        } else {
            return 0;
        }
        s_bots.searchFocusTime = uis.realtime;
        Rivals_Filter();
        return 0;
    }

    switch ( key ) {
    case K_ESCAPE:
    case K_PAD0_B:
        UI_PopMenu();
        return 0;
    case K_MOUSE1:
        Rivals_Click( x, y );
        return 0;
    case K_MWHEELUP:
    case K_MWHEELDOWN:
        if ( Rivals_Inside( &l->list, x, y ) ) {
            s_bots.scroll += key == K_MWHEELUP ? -100.0f : 100.0f;
        } else if ( s_bots.selected >= 0 && Rivals_Inside( &l->car, x, y ) ) {
            s_bots.distance = WS_Clamp( WS_VIEW_MIN_DISTANCE,
                                        key == K_MWHEELUP ? s_bots.distance * 0.95f : s_bots.distance / 0.95f,
                                        WS_VIEW_MAX_DISTANCE );
        }
        return 0;
    case K_UPARROW:
    case K_KP_UPARROW:
    case K_DOWNARROW:
    case K_KP_DOWNARROW:
    case K_PAD0_DPAD_UP:
    case K_PAD0_DPAD_DOWN:
        if ( !s_bots.matchCount ) return 0;
        row = Rivals_Row( s_bots.selected );
        if ( key == K_UPARROW || key == K_KP_UPARROW || key == K_PAD0_DPAD_UP ) row = row > 0 ? row - 1 : 0;
        else row = row < s_bots.matchCount - 1 ? row + 1 : s_bots.matchCount - 1;
        Rivals_Select( s_bots.matches[row] );
        Rivals_ScrollTo( row );
        return 0;
    }
    return 0;
}

/*
=================
UI_BotsMenu
=================
*/
void UI_BotsMenu( void ) {
    char preview[64];

    memset( &s_bots, 0, sizeof( s_bots ) );
    s_bots.menu.key        = UI_BotsMenu_Key;
    s_bots.menu.draw       = UI_BotsMenu_Draw;
    s_bots.menu.fullscreen = qtrue;
    s_bots.selected = -1;

    UI_BotsMenu_ParseBots();
    /* ui_rivalsPreview "search,scroll,rival" pins a state for screenshots
     * (tools/ui_webstyle) */
    trap_Cvar_VariableStringBuffer( "ui_rivalsPreview", preview, sizeof( preview ) );
    if ( preview[0] ) {
        char *field = strchr( preview, ',' );
        if ( field ) *field++ = '\0';
        Q_strncpyz( s_bots.query, preview, sizeof( s_bots.query ) );
        Rivals_Filter();
        if ( field ) {
            s_bots.scroll = atof( field );
            field = strchr( field, ',' );
            if ( field && atoi( field + 1 ) >= 0 && atoi( field + 1 ) < botCount && field[1] != '-' ) {
                Rivals_Select( atoi( field + 1 ) );
            }
        }
    } else {
        Rivals_Filter();
    }
    UI_PushMenu( &s_bots.menu );
}


int UI_WebRivalsScreen( void ) { return uis.activemenu == &s_bots.menu ? 5 : 0; }
int UI_WebRivalCount( void ) { return botCount; }
int UI_WebRivalChar( int index, int field, int character ) {
    const char *value;
    if ( index < 0 || index >= botCount ) return 0;
    switch ( field ) {
    case 0: value = botNames[index]; break;
    case 1: value = botModels[index]; break;
    case 2: value = botDescriptions[index]; break;
    case 3: value = botPersonalities[index]; break;
    case 4: value = botFavWeapon[index]; break;
    default: return 0;
    }
    if ( character < 0 || character >= (int)strlen( value ) ) return 0;
    return (unsigned char)value[character];
}
void UI_WebRivalsBack( void ) {
    if ( UI_WebRivalsScreen() ) UI_PopMenu();
}
