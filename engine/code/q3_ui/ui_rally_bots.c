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

#include "ui_rally_library.h"

typedef struct {
    menuframework_s menu;
    wsLibrary_t     lib;                /* search, list, scrollbar */
    int             matches[MAX_BOTS];  /* the rivals the search leaves, in order */
    qhandle_t       icons[RIVALS_MAX];  /* per row */
    int             matchCount;
    int             selected;           /* rival index, -1 none */
    qboolean        dragCar;
    float           dragX, dragY;
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


/* -------------------------------------------------------------------------
   Search and selection, as library.js does it
   ------------------------------------------------------------------------- */

/* name, model, description and personality joined, matched case-insensitively */
static qboolean Rivals_Matches( int index, const char *query ) {
    char joined[2 * DESC_BUFSIZE + 2 * NAME_BUFSIZE + 4], lower[sizeof( joined )];
    if ( !query[0] ) return qtrue;
    Com_sprintf( joined, sizeof( joined ), "%s %s %s %s", botNames[index], botModels[index],
                 botDescriptions[index], botPersonalities[index] );
    Library_Lower( joined, lower, sizeof( lower ) );
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

static int Rivals_Row( int index ) {
    int i;
    for ( i = 0; i < s_bots.matchCount; i++ ) {
        if ( s_bots.matches[i] == index ) return i;
    }
    return -1;
}

/* renderList(): filter, keep the selected rival if it still matches, else the first */
static void Rivals_Filter( void ) {
    char query[64];
    int i, keep = -1;

    Library_Query( &s_bots.lib, query, sizeof( query ) );
    s_bots.matchCount = 0;
    for ( i = 0; i < botCount && s_bots.matchCount < RIVALS_MAX; i++ ) {
        if ( !Rivals_Matches( i, query ) ) continue;
        if ( i == s_bots.selected ) keep = i;
        s_bots.icons[s_bots.matchCount] = botIcons[i];
        s_bots.matches[s_bots.matchCount++] = i;
    }
    Rivals_Select( keep >= 0 ? keep : ( s_bots.matchCount ? s_bots.matches[0] : -1 ) );
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
    rivalsLayout_t *l = &s_bots.lib.layout;
    int index = s_bots.selected, i;
    char weapon[DESC_BUFSIZE];

    memset( l, 0, sizeof( *l ) );
    l->count = s_bots.matchCount;
    for ( i = 0; i < l->count; i++ ) l->rowText[i] = botNames[s_bots.matches[i]];
    l->scroll = s_bots.lib.scroll;
    Q_strncpyz( l->name, index >= 0 ? botNames[index] : "No rivals found", sizeof( l->name ) );
    Q_strncpyz( l->personality, index >= 0 ? botPersonalities[index] : "", sizeof( l->personality ) );
    if ( index >= 0 ) {
        Rivals_Clean( botFavWeapon[index], weapon, sizeof( weapon ) );
        if ( weapon[0] ) Com_sprintf( l->weapon, sizeof( l->weapon ), "Favorite weapon / %s", weapon );
    }
    Q_strncpyz( l->description, index >= 0 ? botDescriptions[index] : "", sizeof( l->description ) );
    Q_strncpyz( l->status, index >= 0 ? "Drag to rotate / scroll to zoom" : "", sizeof( l->status ) );
    Rivals_Layout( l, WS_ViewportWidth(), WS_ViewportHeight() );
    s_bots.lib.scroll = l->scroll;
    s_bots.lib.selectedRow = Rivals_Row( index );
}

/* -------------------------------------------------------------------------
   Drawing
   ------------------------------------------------------------------------- */

static void Rivals_Orbit( void ) {
    const wsRect_t *car = &s_bots.lib.layout.car;
    float x, y;

    Library_Cursor( &x, &y );
    if ( s_bots.dragCar ) {
        if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
            s_bots.dragCar = qfalse;
        } else if ( car->h > 0.0f ) {
            s_bots.yaw = AngleNormalize360( s_bots.yaw + 360.0f * ( x - s_bots.dragX ) / car->h );
            s_bots.elevation = WS_Clamp( WS_VIEW_MIN_ELEVATION, s_bots.elevation + 360.0f * ( y - s_bots.dragY ) / car->h,
                                         WS_VIEW_MAX_ELEVATION );
        }
    }
    s_bots.dragX = x;
    s_bots.dragY = y;
}

static void UI_BotsMenu_Draw( void ) {
    rivalsLayout_t *l = &s_bots.lib.layout;
    wsTextStyle_t body;
    char count[32];

    Library_Track( &s_bots.lib );
    Rivals_Orbit();
    Rivals_BuildLayout();
    WS_BodyStyle( &body );

    WS_DrawBackground( l->pageWidth, l->pageHeight );
    WS_DrawFrontHeader( &l->page, "Meet your ", "rivals.", "" );
    WS_DrawButton( &l->back, rivalsBack, qtrue, qfalse );

    /* left: search, count, list */
    WS_DrawSection( &l->left, qfalse );
    WS_DrawText( &body, l->leftX, l->searchLabelY, "Search", 6, wsWhite );
    Library_DrawSearch( &s_bots.lib, rivalsPlaceholder );
    Com_sprintf( count, sizeof( count ), "%d rivals", s_bots.matchCount );
    WS_DrawText( &body, l->leftX, l->countY, count, strlen( count ), wsWhite );
    Library_DrawList( &s_bots.lib, s_bots.icons );
    Library_DrawScrollbar( &s_bots.lib );

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

static void Rivals_Click( float x, float y ) {
    const rivalsLayout_t *l = &s_bots.lib.layout;
    qboolean changed;
    int row;

    if ( Library_Inside( &l->back, x, y ) ) {
        UI_PopMenu();
        return;
    }
    if ( Library_Click( &s_bots.lib, x, y, &changed ) ) {
        if ( changed ) Rivals_Filter();
        return;
    }
    row = Library_RowAt( &s_bots.lib, x, y );
    if ( row >= 0 ) {
        Rivals_Select( s_bots.matches[row] );
        return;
    }
    if ( s_bots.selected >= 0 && Library_Inside( &l->car, x, y ) ) {
        s_bots.dragCar = qtrue;
        s_bots.dragX = x;
        s_bots.dragY = y;
    }
}

static sfxHandle_t UI_BotsMenu_Key( int key ) {
    const rivalsLayout_t *l = &s_bots.lib.layout;
    float x, y;
    int row;

    Library_Cursor( &x, &y );
    if ( key & K_CHAR_FLAG ) {
        if ( Library_Char( &s_bots.lib, key & ~K_CHAR_FLAG ) ) Rivals_Filter();
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
        if ( Library_Inside( &l->list, x, y ) ) {
            s_bots.lib.scroll += key == K_MWHEELUP ? -100.0f : 100.0f;
        } else if ( s_bots.selected >= 0 && Library_Inside( &l->car, x, y ) ) {
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
        Library_ScrollTo( &s_bots.lib, row );
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
        Q_strncpyz( s_bots.lib.query, preview, sizeof( s_bots.lib.query ) );
        Rivals_Filter();
        if ( field ) {
            s_bots.lib.scroll = atof( field );
            field = strchr( field, ',' );
            if ( field && field[1] != '-' && atoi( field + 1 ) < botCount ) {
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
