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

/* ==========================================================
   LICENSE PLATE IMAGE GENERATION
   Ported from cg_rally_platetools.c — uses only trap_FS_*
   so it works in UI without any new syscall.
   ========================================================== */

#define UI_PLATE_POOLSIZE  (1024 * 1024)
static char   ui_plateMemPool[UI_PLATE_POOLSIZE];
static int    ui_plateAllocPoint = 0;

static void *UI_PlateAlloc( int size ) {
    char *p;
    if ( ui_plateAllocPoint + size > UI_PLATE_POOLSIZE ) {
        Com_Printf( S_COLOR_RED "UI_PlateAlloc: out of memory\n" );
        return NULL;
    }
    p = &ui_plateMemPool[ui_plateAllocPoint];
    ui_plateAllocPoint += ( size + 31 ) & ~31;
    return p;
}

typedef struct {
    byte *imageData;
    int   bpp;
    int   width;
    int   height;
} UI_PlateImage;

static qboolean UI_LoadPlateTGA( UI_PlateImage *tex, const char *filename ) {
    byte         idlen, colorMapType, imageType;
    byte         hdr[6];
    int          bytesPerPixel, imageSize, i;
    fileHandle_t f;
    byte        *dst;

    trap_FS_FOpenFile( filename, &f, FS_READ );
    if ( !f ) {
        Com_Printf( S_COLOR_YELLOW "Q3R UI: Cannot open plate TGA: %s\n", filename );
        return qfalse;
    }

    /* TGA header: idLength(1) colorMapType(1) imageType(1) colorMapSpec(5) imageSpec(10) = 18 bytes */
    trap_FS_Read( &idlen,        1, f );
    trap_FS_Read( &colorMapType, 1, f );
    trap_FS_Read( &imageType,    1, f );

    /* skip colormap spec (5 bytes) + image origin x/y (4 bytes) */
    { byte skip[9]; trap_FS_Read( skip, 9, f ); }

    trap_FS_Read( hdr, 6, f );  /* width(2) height(2) bpp(1) descriptor(1) */

    /* skip image ID field */
    if ( idlen > 0 ) {
        byte skip[256];
        trap_FS_Read( skip, idlen, f );
    }

    tex->width  = hdr[1] * 256 + hdr[0];
    tex->height = hdr[3] * 256 + hdr[2];
    tex->bpp    = hdr[4];

    /* accept type 2 (uncompressed RGB) and type 10 (RLE RGB) */
    if ( imageType != 2 && imageType != 10 ) {
        trap_FS_FCloseFile(f);
        Com_Printf( S_COLOR_YELLOW "Q3R UI: Unsupported TGA type %d: %s\n", (int)imageType, filename );
        return qfalse;
    }
    if ( tex->width <= 0 || tex->height <= 0 || (tex->bpp != 24 && tex->bpp != 32) ) {
        trap_FS_FCloseFile(f);
        Com_Printf( S_COLOR_YELLOW "Q3R UI: Invalid TGA dims/bpp: %s\n", filename );
        return qfalse;
    }

    bytesPerPixel = tex->bpp / 8;
    imageSize     = tex->width * tex->height * bytesPerPixel;
    tex->imageData = (byte *)UI_PlateAlloc( imageSize );
    if ( !tex->imageData ) {
        trap_FS_FCloseFile(f);
        return qfalse;
    }

    if ( imageType == 2 ) {
        /* uncompressed */
        trap_FS_Read( tex->imageData, imageSize, f );
    } else {
        /* RLE compressed (type 10) */
        dst = tex->imageData;
        i   = 0;
        while ( i < tex->width * tex->height ) {
            byte   packet;
            byte   pixel[4];
            int    count, j;

            trap_FS_Read( &packet, 1, f );
            count = (packet & 0x7F) + 1;

            if ( packet & 0x80 ) {
                /* run-length packet: one pixel repeated */
                trap_FS_Read( pixel, bytesPerPixel, f );
                for ( j = 0; j < count && i < tex->width * tex->height; j++, i++ ) {
                    memcpy( dst, pixel, bytesPerPixel );
                    dst += bytesPerPixel;
                }
            } else {
                /* raw packet: count distinct pixels */
                for ( j = 0; j < count && i < tex->width * tex->height; j++, i++ ) {
                    trap_FS_Read( dst, bytesPerPixel, f );
                    dst += bytesPerPixel;
                }
            }
        }
    }

    trap_FS_FCloseFile(f);
    return qtrue;
}

static qboolean UI_SavePlateTGA( UI_PlateImage *tex, const char *filename ) {
    byte         header[18];
    int          bytesPerPixel, imageSize;
    fileHandle_t f;

    trap_FS_FOpenFile( filename, &f, FS_WRITE );
    if ( !f ) {
        Com_Printf( S_COLOR_YELLOW "Q3R UI: Cannot write plate TGA: %s\n", filename );
        return qfalse;
    }

    /* Standard 18-byte TGA header */
    memset( header, 0, sizeof(header) );
    header[0]  = 0;                      /* ID length */
    header[1]  = 0;                      /* color map type: none */
    header[2]  = 2;                      /* image type: uncompressed RGB */
    /* bytes 3-7: color map spec (all zero) */
    /* bytes 8-11: image origin x=0, y=0 */
    header[12] = tex->width  & 0xFF;
    header[13] = (tex->width  >> 8) & 0xFF;
    header[14] = tex->height & 0xFF;
    header[15] = (tex->height >> 8) & 0xFF;
    header[16] = tex->bpp;               /* bits per pixel */
    header[17] = 0;                      /* descriptor: bottom-left origin (Q3 default) */

    bytesPerPixel = tex->bpp / 8;
    imageSize     = tex->width * tex->height * bytesPerPixel;

    trap_FS_Write( header, sizeof(header), f );
    trap_FS_Write( tex->imageData, imageSize, f );
    trap_FS_FCloseFile(f);
    return qtrue;
}

#define UI_PLATE_CHARW   8    /* rendered width per character on the plate */
#define UI_PLATE_CHARH   8    /* rendered height per character on the plate */
#define UI_PLATE_ADVANCE 9    /* horizontal advance per character (px) */

static qboolean UI_WriteNameOnPlateTGA( UI_PlateImage *tex, const char *name, int maxChars ) {
    UI_PlateImage font;
    const char   *s;
    unsigned char ch;
    float         ax, ay, aw, ah;
    float         frow, fcol, fwidth, fheight;
    int           i, j, t, f, len, cnt;
    int           bppF, bppT;
    vec4_t        color;
    float         a;

    if ( !UI_LoadPlateTGA( &font, "gfx/2d/bigchars_plates.tga" ) ) {
        /* fallback: use standard charset */
        if ( !UI_LoadPlateTGA( &font, "gfx/2d/bigchars.tga" ) ) {
            Com_Printf( S_COLOR_YELLOW "Q3R UI Plate: no usable font TGA found\n" );
            return qfalse;
        }
    }

    bppF = font.bpp / 8;
    bppT = tex->bpp  / 8;

    /* clamp maxChars to how many actually fit */
    {
        int fits = (int)((tex->width - 4) / UI_PLATE_ADVANCE);
        if ( fits < maxChars ) maxChars = fits;
    }
    len = strlen(name);
    if ( len > maxChars ) len = maxChars;

    /* center the text horizontally, vertically centered in plate */
    ax = (int)((tex->width  / 2.0f) - (len * UI_PLATE_ADVANCE / 2.0f));
    ay = (int)((tex->height / 2.0f) - (UI_PLATE_CHARH / 2.0f));
    if ( ax < 0 ) ax = 0;
    ah = UI_PLATE_CHARH;

    color[0] = color[1] = color[2] = 0; color[3] = 1.0f;

    s   = name;
    cnt = 0;
    while ( *s && cnt < maxChars ) {
        if ( Q_IsColorString(s) ) {
            memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof(color) );
            s += 2;
            continue;
        }
        ch = *s & 127;
        if ( ch == ' ' ) {
            aw = UI_PLATE_ADVANCE;
        } else if ( ch > 32 && ch < 127 ) {
            frow   = (ch >> 4) * 16.0f;
            fcol   = (ch & 15) * 16.0f;
            fwidth = fheight = 16.0f;
            aw     = UI_PLATE_ADVANCE;
            for ( i = 0; i < (int)ah; i++ ) {
                t = (int)((tex->height - (ay + ah - i)) * tex->width + ax) * bppT;
                f = (int)(font.height - (frow + fheight - fheight * (i / (float)ah))) * font.width;
                for ( j = 0; j < (int)aw; j++ ) {
                    int fi = (int)((f + (int)(fcol + fwidth*(j/(float)aw)))*bppF + 3);
                    if ( fi >= 0 && fi < font.width * font.height * bppF ) {
                        a = font.imageData[fi];
                        if ( t >= 0 && t+2 < tex->width * tex->height * bppT ) {
                            tex->imageData[t]   = (byte)(tex->imageData[t]   * (1.0f-(a/255.0f)) + color[2]*a);
                            tex->imageData[t+1] = (byte)(tex->imageData[t+1] * (1.0f-(a/255.0f)) + color[1]*a);
                            tex->imageData[t+2] = (byte)(tex->imageData[t+2] * (1.0f-(a/255.0f)) + color[0]*a);
                        }
                        t += bppT;
                    }
                }
            }
        } else {
            aw = 0;
        }
        ax += aw;
        cnt++;
        s++;
    }
    return qtrue;
}

/* Generate a plate TGA with the bot's name.
   Write TGA first, then register directly by full path. */
static qhandle_t UI_GenerateBotPlateShader( const char *botName, int botIndex ) {
    UI_PlateImage tga;
    char          output[MAX_QPATH];
    qhandle_t     h;

    ui_plateAllocPoint = 0;

    Com_sprintf( output, sizeof(output), "models/players/plates/uibot%d.tga", botIndex );

    if ( !UI_LoadPlateTGA( &tga, "models/players/plates/usa_california.tga" ) ) {
        Com_Printf( S_COLOR_YELLOW "Q3R UI Plate: failed to load base TGA for bot %d (%s)\n", botIndex, botName );
        return 0;
    }
    if ( !UI_WriteNameOnPlateTGA( &tga, botName, 10 ) ) {
        Com_Printf( S_COLOR_YELLOW "Q3R UI Plate: failed to write name for bot %d (%s)\n", botIndex, botName );
        return 0;
    }
    if ( !UI_SavePlateTGA( &tga, output ) ) {
        Com_Printf( S_COLOR_YELLOW "Q3R UI Plate: failed to save TGA for bot %d (%s)\n", botIndex, botName );
        return 0;
    }

    /* Register directly with full TGA path.
       Avoid remapping here so we don't preload the image with different flags. */
    h = trap_R_RegisterShaderNoMip( output );
    Com_Printf( "Q3R UI Plate: bot %d (%s) -> shader handle %d\n", botIndex, botName, h );
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
typedef struct { const char *canon; const char *icon; } weaponiconmap_t;
static const weaponiconmap_t s_weaponIconMap[] = {
    { "GAUNTLET",        "icons/iconw_gauntlet" },  /* Chainsaw alias uses same icon */
    { "CHAINSAW",        "icons/iconw_gauntlet" },
    { "MACHINEGUN",      "icons/iconw_machinegun" },
    { "SHOTGUN",         "icons/iconw_shotgun" },
    { "GRENADELAUNCHER", "icons/iconw_grenade" },
    { "ROCKETLAUNCHER",  "icons/iconw_rocket" },
    { "LIGHTNING",       "icons/iconw_lightning" },
    { "RAILGUN",         "icons/iconw_railgun" },
    { "PLASMAGUN",       "icons/iconw_plasma" },
    { "BFG10k",          "icons/iconw_bfg" },
    { "FLAMETHROWER",    "icons/iconw_flame" }
};
#define WEAPONICONMAP_COUNT ( (int)(sizeof(s_weaponIconMap)/sizeof(s_weaponIconMap[0])) )

/* Resolve icon handle from favoriteweapon text */
static qhandle_t UI_WeaponIconFromText( const char *favoriteText ) {
    char canon[32]; int i;
    if (!favoriteText || !*favoriteText) return 0;
    UI_CanonicalizeWeapon(favoriteText, canon, sizeof(canon));
    if (!canon[0]) return 0;
    for (i = 0; i < WEAPONICONMAP_COUNT; i++) {
        if (Q_stricmp(s_weaponIconMap[i].canon, canon) == 0) {
            return trap_R_RegisterShaderNoMip(s_weaponIconMap[i].icon);
        }
    }
    return 0;
}

/* simple proportional text wrapper (C89) */
static void UI_DrawWrappedProportional( int x, int y, int maxWidth, int lineHeight,
                                        const char *text, int style, vec4_t color ) {
    char line[1024];
    char word[256];
    char test[1024];
    const char *s;
    float scale;
    int wi;
    int w;

    if (!text || !*text) {
        return;
    }

    line[0] = '\0';
    s = text;
    scale = UI_ProportionalSizeScale(style);

    while (*s) {
        /* respect explicit newlines */
        if (*s == '\n') {
            if (line[0]) {
                UI_DrawProportionalString(x, y, line, style, color);
                line[0] = '\0';
            }
            y += lineHeight;
            s++;
            continue;
        }

        /* read next word */
        wi = 0;
        while (*s && *s != ' ' && *s != '\n' && wi < (int)sizeof(word) - 1) {
            word[wi++] = *s++;
        }
        word[wi] = '\0';

        /* collapse multiple spaces */
        while (*s == ' ') s++;

        if (!word[0]) continue;

        if (line[0]) {
            Q_strncpyz(test, line, sizeof(test));
            Q_strcat(test, sizeof(test), " ");
            Q_strcat(test, sizeof(test), word);
        } else {
            Q_strncpyz(test, word, sizeof(test));
        }

        w = (int)(UI_ProportionalStringWidth(test) * scale);
        if (w <= maxWidth) {
            if (line[0]) Q_strcat(line, sizeof(line), " ");
            Q_strcat(line, sizeof(line), word);
        } else {
            if (line[0]) {
                UI_DrawProportionalString(x, y, line, style, color);
                y += lineHeight;
                Q_strncpyz(line, word, sizeof(line));
            } else {
                /* extremely long single word: draw anyway */
                UI_DrawProportionalString(x, y, word, style, color);
                y += lineHeight;
            }
        }
    }

    if (line[0]) {
        UI_DrawProportionalString(x, y, line, style, color);
    }
}

#define NAME_BUFSIZE 64
#define DESC_BUFSIZE 256
#define MAX_VISIBLE_BOTS 10
#define DESC_MAXWIDTH 750
#define DESC_LINEHEIGHT 16

/* control IDs */
#define ID_BOT0  1000
#define ID_PREV  2000
#define ID_NEXT  2001
#define ID_BACK  10

/* semi-transparent background color */
static vec4_t rivals_background = { 0.0f, 0.0f, 0.0f, 0.25f };

/* Draw weapon icon to the right of the car icon; derives position from given rect (no layout change) */
static void UI_DrawWeaponIconNextTo( int x, int y, int w, int h, const char *favoriteText ) {
    qhandle_t wi;
    if (!favoriteText || !*favoriteText) return;
    wi = UI_WeaponIconFromText(favoriteText);
    if (!wi) return;
    {
#define WEAPON_ICON_DY 32
    int wx = x + w + 24;
    int wy = y + WEAPON_ICON_DY;
        UI_DrawHandlePic(wx, wy, 40, 40, wi);
    }
}

static char botNames[MAX_BOTS][NAME_BUFSIZE];
static char botModels[MAX_BOTS][NAME_BUFSIZE];
static char botAIFiles[MAX_BOTS][NAME_BUFSIZE];
static char botDescriptions[MAX_BOTS][DESC_BUFSIZE];
static char botPersonalities[MAX_BOTS][DESC_BUFSIZE];
static char botFavWeapon[MAX_BOTS][DESC_BUFSIZE];
static qhandle_t botIcons[MAX_BOTS];
static qhandle_t botPlateShaders[MAX_BOTS];
static weapon_t  botWeapons[MAX_BOTS];
static int botCount = 0;
static int botPage = 0;
static int botSelected = -1;

static menutext_s botItems[MAX_VISIBLE_BOTS];
static menutext_s nextButton;
static menutext_s prevButton;

typedef struct {
    menuframework_s menu;
    menutext_s      banner;
    menutext_s      back;
} botsmenu_t;

static botsmenu_t s_bots;
static playerInfo_t s_garagePlayerInfo;

static void UI_BotsMenu_Init(void);
static void UI_BotsMenu_DrawBotPage(void);

/* Apply model, weapon and plate for a selected rival */
static void UI_BotsMenu_SetRival( int index ) {
    vec3_t viewAngles  = { 0, 180, 0 };
    vec3_t moveAngles  = { 0,   0, 0 };
    char   plate[MAX_QPATH];
    weapon_t wp;

    if ( index < 0 || index >= botCount ) return;

    wp = botWeapons[index];

    /* plate: use USA marker for model selection when using generated bot plate shaders */
    if ( botPlateShaders[index] ) {
        Q_strncpyz( plate, "usa_california", sizeof(plate) );
    } else {
        trap_Cvar_VariableStringBuffer( "plate", plate, sizeof(plate) );
        if ( !plate[0] ) Q_strncpyz( plate, "usa_california", sizeof(plate) );
    }

    UI_PlayerInfo_SetModel( &s_garagePlayerInfo, botModels[index], NULL, NULL, plate );

    /* Override the plateShader directly with our pre-generated one */
    if ( botPlateShaders[index] ) {
        s_garagePlayerInfo.plateShader = botPlateShaders[index];
    }

    UI_PlayerInfo_SetInfo( &s_garagePlayerInfo,
        LEGS_IDLE, TORSO_STAND,
        viewAngles, moveAngles,
        wp, qfalse );
}

static void UI_BotsMenu_BackEvent(void *ptr, int event) {
    if (event != QM_ACTIVATED) return;
    UI_PopMenu();
}

static void UI_BotsMenu_BotSelectEvent(void *ptr, int event) {
    int i;
    int index;

    if (event != QM_ACTIVATED) return;

    for (i = 0; i < MAX_VISIBLE_BOTS; i++) {
        if ((void*)&botItems[i] == ptr) {
            index = botPage * MAX_VISIBLE_BOTS + i;
            if (index >= 0 && index < botCount) {
                botSelected = index;
                UI_BotsMenu_SetRival( botSelected );
                UI_BotsMenu_DrawBotPage();
            }
            break;
        }
    }
}


static void UI_BotsMenu_NextPage(void *ptr, int event) {
    int start;
    if (event != QM_ACTIVATED) return;
    if ((botPage + 1) * MAX_VISIBLE_BOTS < botCount) {
        botPage++;
        start = botPage * MAX_VISIBLE_BOTS;
        if (botSelected < start || botSelected >= start + MAX_VISIBLE_BOTS) {
            botSelected = (start < botCount) ? start : botCount - 1;
            if (botSelected >= 0) {
                UI_BotsMenu_SetRival( botSelected );
            }
        }
        UI_BotsMenu_DrawBotPage();
    }
}

static void UI_BotsMenu_PrevPage(void *ptr, int event) {
    int start;
    if (event != QM_ACTIVATED) return;
    if (botPage > 0) {
        botPage--;
        start = botPage * MAX_VISIBLE_BOTS;
        if (botSelected < start || botSelected >= start + MAX_VISIBLE_BOTS) {
            botSelected = (start < botCount) ? start : botCount - 1;
            if (botSelected >= 0) {
                UI_BotsMenu_SetRival( botSelected );
            }
        }
        UI_BotsMenu_DrawBotPage();
    }
}

static sfxHandle_t UI_BotsMenu_Key(int key) {
    return Menu_DefaultKey(&s_bots.menu, key);
}

static void UI_BotsMenu_Draw(void) {
    int boxX = 0, boxY = 100, boxW = 640, boxH = 360;
    UI_FillRect(boxX, boxY, boxW, boxH, rivals_background);
    Menu_Draw(&s_bots.menu);

    /* Pagination counter: "Page X / Y" centered above the prev/next buttons */
    if (botCount > MAX_VISIBLE_BOTS) {
        int totalPages = (botCount + MAX_VISIBLE_BOTS - 1) / MAX_VISIBLE_BOTS;
        char pageStr[32];
        Com_sprintf(pageStr, sizeof(pageStr), "PAGE %d / %d", botPage + 1, totalPages);
        UI_DrawProportionalString(320, 415, pageStr, UI_CENTER | UI_SMALLFONT, colorWhite);
    }

    if (botSelected >= 0 && botSelected < botCount) {
        if (botIcons[botSelected]) {
            UI_DrawHandlePic(330, 375, 92, 92, botIcons[botSelected]);
            UI_DrawWeaponIconNextTo(330, 375, 92, 92, botFavWeapon[botSelected][0] ? botFavWeapon[botSelected] : NULL);
        }
        UI_DrawProportionalString(20, 320, botPersonalities[botSelected], UI_LEFT | UI_SMALLFONT, colorCyan);
        {
            const char *fw = (botFavWeapon[botSelected][0]) ? botFavWeapon[botSelected] : "-";
            char fav[64];
            Com_sprintf(fav, sizeof(fav), "Favorite Weapon: %s", fw);
            UI_DrawProportionalString(20, 340, fav, UI_LEFT | UI_SMALLFONT, colorWhite);
        }
        UI_DrawWrappedProportional( 20, 360, DESC_MAXWIDTH, DESC_LINEHEIGHT, botDescriptions[botSelected], UI_LEFT | UI_SMALLFONT, colorYellow );
        UI_DrawPlayer(270, 0, 425, 425, &s_garagePlayerInfo, uis.realtime);
    } else {
        /* Empty state: no rival selected yet */
        UI_DrawProportionalString(320, 250, "SELECT A RIVAL", UI_CENTER | UI_BIGFONT, colorWhite);
    }
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
            botIcons[botCount]        = UI_LoadModelIconFor(model);
            botWeapons[botCount]      = UI_WeaponEnumFromText(favoriteweapon);
            botPlateShaders[botCount] = UI_GenerateBotPlateShader(name, botCount);
            
            botCount++;
        }
    }
}

static void UI_BotsMenu_DrawBotPage(void) {
    int start, i, index;
    int tw;
    float sscale;

    start = botPage * MAX_VISIBLE_BOTS;
    for (i = 0; i < MAX_VISIBLE_BOTS; i++) {
        index = start + i;
        if (index < botCount) {
            botItems[i].string = botNames[index];
            botItems[i].color  = (index == botSelected) ? color_yellow : color_white;
            botItems[i].generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS | QMF_MOUSEONLY;
            sscale = UI_ProportionalSizeScale(botItems[i].style);
            tw = (int)(UI_ProportionalStringWidth(botItems[i].string) * sscale) + 8;
            botItems[i].generic.left   = botItems[i].generic.x;
            botItems[i].generic.top    = botItems[i].generic.y - 10;
            botItems[i].generic.right  = botItems[i].generic.x + tw;
            botItems[i].generic.bottom = botItems[i].generic.y + 10;
        } else {
            botItems[i].generic.flags = QMF_INACTIVE;
            botItems[i].generic.left = botItems[i].generic.right = botItems[i].generic.x;
            botItems[i].generic.top = botItems[i].generic.bottom = botItems[i].generic.y;
            botItems[i].string = "";
            botItems[i].color  = color_white;
        }
    }
    prevButton.generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
    nextButton.generic.flags = QMF_RIGHT_JUSTIFY | QMF_PULSEIFFOCUS;
}

void UI_BotsMenu(void) {
    memset(&s_bots, 0, sizeof(s_bots));
    s_bots.menu.key        = UI_BotsMenu_Key;
    s_bots.menu.draw       = UI_BotsMenu_Draw;
    s_bots.menu.fullscreen = qtrue;
    UI_BotsMenu_Init();
    UI_PushMenu(&s_bots.menu);
}

static void UI_BotsMenu_Init(void) {
    int i;
    trap_Cvar_Set("cg_viewsize", "100");
    s_bots.menu.wrapAround = qtrue;
    s_bots.menu.fullscreen = qtrue;

    s_bots.banner.generic.type  = MTYPE_BTEXT;
    s_bots.banner.generic.x     = 320;
    s_bots.banner.generic.y     = 40;
    s_bots.banner.string        = "RIVALS";
    s_bots.banner.color         = color_white;
    s_bots.banner.style         = UI_CENTER;

    UI_BotsMenu_ParseBots();
    UI_PlayerInfo_SetModel(&s_garagePlayerInfo, "roadster/blue", NULL, NULL, NULL);

    Menu_AddItem(&s_bots.menu, &s_bots.banner);

    for (i = 0; i < MAX_VISIBLE_BOTS; i++) {
        botItems[i].generic.type = MTYPE_PTEXT;
        botItems[i].generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS | QMF_MOUSEONLY;
        botItems[i].generic.x = 15;
        botItems[i].generic.y = 140 + i * 16;
        botItems[i].generic.id = ID_BOT0 + i;
        botItems[i].generic.callback = UI_BotsMenu_BotSelectEvent;
        botItems[i].string = "";
        botItems[i].style = UI_LEFT | UI_SMALLFONT;
        botItems[i].color = color_white;
        Menu_AddItem(&s_bots.menu, &botItems[i]);
    }

    prevButton.generic.type = MTYPE_PTEXT;
    prevButton.generic.flags = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
    prevButton.generic.x = 95;
    prevButton.generic.y = 415;
    prevButton.generic.id = ID_PREV;
    prevButton.generic.callback = UI_BotsMenu_PrevPage;
    prevButton.string = "< PREV";
    prevButton.color = color_white;
    prevButton.style = UI_LEFT | UI_SMALLFONT;
    Menu_AddItem(&s_bots.menu, &prevButton);

    nextButton.generic.type = MTYPE_PTEXT;
    nextButton.generic.flags = QMF_RIGHT_JUSTIFY | QMF_PULSEIFFOCUS;
    nextButton.generic.x = 620;
    nextButton.generic.y = 415;
    nextButton.generic.id = ID_NEXT;
    nextButton.generic.callback = UI_BotsMenu_NextPage;
    nextButton.string = "NEXT >";
    nextButton.color = color_white;
    nextButton.style = UI_RIGHT | UI_SMALLFONT;
    Menu_AddItem(&s_bots.menu, &nextButton);

    s_bots.back.generic.type     = MTYPE_PTEXT;
    s_bots.back.generic.flags    = QMF_LEFT_JUSTIFY | QMF_PULSEIFFOCUS;
    s_bots.back.generic.x        = 15;
    s_bots.back.generic.y        = 415;
    s_bots.back.generic.id       = ID_BACK;
    s_bots.back.generic.callback = UI_BotsMenu_BackEvent;
    s_bots.back.string           = "< BACK";
    s_bots.back.color            = color_white;
    s_bots.back.style            = UI_LEFT | UI_SMALLFONT;
    Menu_AddItem(&s_bots.menu, &s_bots.back);

    if (botCount > 0) {
        botSelected = 0;
        UI_BotsMenu_SetRival( 0 );
    }
    UI_BotsMenu_DrawBotPage();
}
