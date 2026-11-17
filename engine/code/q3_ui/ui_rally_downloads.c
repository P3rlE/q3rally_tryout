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

DOWNLOAD MANAGER MENU

Communicates with the native engine via CVars:

  ui_dl_state     (int, read)   - 0=idle, 1=fetching index, 2=ready, 3=downloading, 4=done, 5=error
  ui_dl_progress  (float, read) - 0.0 to 100.0 during active download
  ui_dl_filename  (str,   read) - currently downloading file name
  ui_dl_error     (str,   read) - human-readable error message (state 5)
  ui_dl_action    (str,   write)- commands sent to engine:
                                   "fetch_index"        - start fetching content index
                                   "download:<id>"      - queue a specific item for download
                                   "cancel"             - cancel ongoing operation

The index is stored as a temporary file by the engine and the path is
written to ui_dl_indexpath so the UI can read it via trap_FS_FOpenFile.

=======================================================================
*/

#include "ui_local.h"
#include "ui_rally_library.h"
#include "ui_rally_frontend.h"


// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

#define DL_MAX_ITEMS            64
#define DL_STATE_IDLE           0
#define DL_STATE_FETCHING       1
#define DL_STATE_READY          2
#define DL_STATE_DOWNLOADING    3
#define DL_STATE_DONE           4
#define DL_STATE_ERROR          5

// Content types
#define DL_TYPE_ALL             0
#define DL_TYPE_TRACKS          1
#define DL_TYPE_VEHICLES        2
#define DL_TYPE_SKINS           3

// Widget IDs


// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

typedef struct {
    char    id[64];
    char    name[80];
    char    author[64];
    char    version[16];
    char    type[16];       // "track", "vehicle", "skin"
    char    preview[128];
    int     size_kb;
    qboolean installed;
} dlItem_t;

typedef struct {
    menuframework_s menu;
    wsLibrary_t     lib;            // search, list, scrollbar (ui_rally_library.c)
    int             typeFilter;     // #download-type: DL_TYPE_*
    qboolean        typeOpen;       // its option list is open
    int             matches[DL_MAX_ITEMS];  // allItems indices the search leaves
    int             matchCount;
    char            selectedId[64]; // library.js keeps the selection by id
    char            rowText[DL_MAX_ITEMS][96];

    // State
    int             activeTab;
    int             selectedItem;
    int             dlState;
    float           dlProgress;
    char            dlFilename[MAX_QPATH];
    char            dlError[256];

    // Item list (filtered)
    dlItem_t        items[DL_MAX_ITEMS];
    int             numItems;

    // All items from index
    dlItem_t        allItems[DL_MAX_ITEMS];
    int             numAllItems;
    int             catalogRevision;

    int             lastStateCheck;
    qboolean        showPopup;
    char            popupText[128];
} downloadsMenu_t;

static downloadsMenu_t s_dl;



// ---------------------------------------------------------------------------
// Minimal JSON parser for the content index
// ---------------------------------------------------------------------------

/*
=================
DL_ParseString
Extracts the string value after "key": "..." from buf starting at pos.
Returns pointer past the parsed value, or NULL on failure.
=================
*/
static const char *DL_ParseString( const char *buf, const char *key, char *out, int outSize ) {
    char    searchKey[128];
    const char *p;
    int     i;

    Com_sprintf( searchKey, sizeof( searchKey ), "\"%s\"", key );
    p = strstr( buf, searchKey );
    if ( !p ) return NULL;

    p += strlen( searchKey );

    // skip whitespace and colon
    while ( *p && ( *p == ' ' || *p == '\t' || *p == ':' || *p == '\n' || *p == '\r' ) ) p++;
    if ( *p != '"' ) return NULL;
    p++; // skip opening quote

    for ( i = 0; i < outSize - 1 && *p && *p != '"'; i++, p++ ) {
        out[i] = *p;
    }
    out[i] = '\0';

    if ( *p == '"' ) p++;
    return p;
}

/*
=================
DL_ParseInt
=================
*/
static int DL_ParseInt( const char *buf, const char *key ) {
    char    searchKey[128];
    const char *p;

    Com_sprintf( searchKey, sizeof( searchKey ), "\"%s\"", key );
    p = strstr( buf, searchKey );
    if ( !p ) return 0;

    p += strlen( searchKey );
    while ( *p && ( *p == ' ' || *p == '\t' || *p == ':' ) ) p++;
    return atoi( p );
}

/*
=================
DL_IsInstalled
Checks whether a .pk3 file for this item already exists in the game path.
=================
*/
static qboolean DL_IsInstalled( const char *itemId ) {
    fileHandle_t    f;
    char            path[MAX_QPATH];
    int             len;

    Com_sprintf( path, sizeof( path ), "%s.pk3", itemId );
    len = trap_FS_FOpenFile( path, &f, FS_READ );
    if ( len >= 0 ) {
        trap_FS_FCloseFile( f );
        return qtrue;
    }
    return qfalse;
}

/*
=================
DL_ParseIndex

Parses a minimal JSON content index from a buffer.
Expected format (array of objects):
[
  { "id": "...", "name": "...", "author": "...", "version": "...",
    "type": "track|vehicle|skin", "size": <bytes> },
  ...
]
=================
*/
static void DL_ParseIndex( const char *buf ) {
    const char  *p = buf;
    const char  *objEnd;
    dlItem_t    *item;

    s_dl.numAllItems = 0;
    s_dl.catalogRevision++;

    // Walk through the array looking for object starts
    while ( *p && s_dl.numAllItems < DL_MAX_ITEMS ) {
        // Find next object opening brace
        p = strchr( p, '{' );
        if ( !p ) break;

        item = &s_dl.allItems[s_dl.numAllItems];
        memset( item, 0, sizeof( *item ) );

        // Find closing brace to limit our parse scope
        objEnd = strchr( p, '}' );
        if ( !objEnd ) break;

        // We parse within this object range only
        // (crude but functional for a flat object structure)
        DL_ParseString( p, "id",      item->id,      sizeof( item->id ) );
        DL_ParseString( p, "name",    item->name,    sizeof( item->name ) );
        DL_ParseString( p, "author",  item->author,  sizeof( item->author ) );
        DL_ParseString( p, "version", item->version, sizeof( item->version ) );
        DL_ParseString( p, "type",    item->type,    sizeof( item->type ) );
        DL_ParseString( p, "preview", item->preview, sizeof( item->preview ) );
        item->size_kb = DL_ParseInt( p, "size" ) / 1024;

        if ( item->id[0] && item->name[0] && item->type[0] ) {
            item->installed = DL_IsInstalled( item->id );
            s_dl.numAllItems++;
        }

        p = objEnd + 1;
    }
}

/*
=================
DL_LoadIndex
Tries to read the cached index file written by the engine.
=================
*/
static void DL_LoadIndex( void ) {
    fileHandle_t    f;
    char            indexPath[MAX_QPATH];
    char            buf[16384];
    int             len;

    trap_Cvar_VariableStringBuffer( "ui_dl_indexpath", indexPath, sizeof( indexPath ) );
    if ( !indexPath[0] ) return;

    len = trap_FS_FOpenFile( indexPath, &f, FS_READ );
    if ( len <= 0 || len > (int)sizeof( buf ) - 1 ) {
        if ( len > 0 ) trap_FS_FCloseFile( f );
        return;
    }

    trap_FS_Read( buf, len, f );
    trap_FS_FCloseFile( f );
    buf[len] = '\0';

    DL_ParseIndex( buf );
}


// ---------------------------------------------------------------------------
// Filtering
// ---------------------------------------------------------------------------

static void DL_ApplyFilter( void ) {
    int     i;
    const char *filterType = NULL;

    switch ( s_dl.activeTab ) {
        case DL_TYPE_TRACKS:   filterType = "track";   break;
        case DL_TYPE_VEHICLES: filterType = "vehicle"; break;
        case DL_TYPE_SKINS:    filterType = "skin";    break;
        default:               filterType = NULL;      break; // ALL
    }

    s_dl.numItems = 0;
    for ( i = 0; i < s_dl.numAllItems; i++ ) {
        if ( !filterType || !Q_stricmp( s_dl.allItems[i].type, filterType ) ) {
            s_dl.items[s_dl.numItems++] = s_dl.allItems[i];
            if ( s_dl.numItems >= DL_MAX_ITEMS ) break;
        }
    }

    s_dl.selectedItem = ( s_dl.numItems > 0 ) ? 0 : -1;
}


// ---------------------------------------------------------------------------
// State polling
// ---------------------------------------------------------------------------

static void DL_PollState( void ) {
    int prevState = s_dl.dlState;
    s_dl.dlState    = (int)trap_Cvar_VariableValue( "ui_dl_state" );
    s_dl.dlProgress = trap_Cvar_VariableValue( "ui_dl_progress" );
    trap_Cvar_VariableStringBuffer( "ui_dl_filename", s_dl.dlFilename, sizeof( s_dl.dlFilename ) );
    trap_Cvar_VariableStringBuffer( "ui_dl_error",    s_dl.dlError,    sizeof( s_dl.dlError ) );

    // When the engine transitions to READY, parse the index exactly once
    if ( s_dl.dlState == DL_STATE_READY && prevState != DL_STATE_READY ) {
        DL_LoadIndex();
        DL_ApplyFilter();
    }
    // When a download finishes, reload index and show restart popup
    if ( s_dl.dlState == DL_STATE_DONE && prevState == DL_STATE_DOWNLOADING ) {
        DL_LoadIndex();
        DL_ApplyFilter();
        s_dl.showPopup = qtrue;
        Q_strncpyz( s_dl.popupText, "Restart required to use new content.", sizeof( s_dl.popupText ) );

    }
    if ( s_dl.dlState == DL_STATE_IDLE && prevState != DL_STATE_IDLE &&
         prevState != DL_STATE_FETCHING ) {
        DL_LoadIndex();
        DL_ApplyFilter();
    }
}


// ---------------------------------------------------------------------------
// Draw helpers
// ---------------------------------------------------------------------------

static qhandle_t DL_GetPreviewShader( const dlItem_t *item ) {
    qhandle_t    shader;
    char         shaderPath[MAX_QPATH];
    char         ext[8];
    const char  *dot;
    const char  *lastSlash;
    const char  *filename;

    if ( !item ) return 0;

    // 1. Check the preview cache: q3rally_previews/<id>.<ext>
    //    The engine stores images there after fetching them.
    if ( item->preview[0] ) {
        // Derive extension from the URL
        lastSlash = strrchr( item->preview, '/' );
        filename  = lastSlash ? lastSlash + 1 : item->preview;
        dot       = strrchr( filename, '.' );
        if ( dot && strlen( dot ) <= 5 ) {
            Q_strncpyz( ext, dot, sizeof( ext ) );
        } else {
            Q_strncpyz( ext, ".jpg", sizeof( ext ) );
        }

        // Strip extension for RegisterShaderNoMip (engine appends its own)
        Com_sprintf( shaderPath, sizeof( shaderPath ),
                     "q3rally_previews/%s", item->id );
        // shaderPath now ends without extension – correct for Q3 shader lookup
        shader = trap_R_RegisterShaderNoMip( shaderPath );
        if ( shader ) return shader;

        // Also try with the explicit extension path stripped
        Com_sprintf( shaderPath, sizeof( shaderPath ),
                     "q3rally_previews/%s%s", item->id, ext );
        COM_StripExtension( shaderPath, shaderPath, sizeof( shaderPath ) );
        shader = trap_R_RegisterShaderNoMip( shaderPath );
        if ( shader ) return shader;
    }

    // 2. Fallback: levelshots/<id> (already packed in a pk3)
    Com_sprintf( shaderPath, sizeof( shaderPath ), "levelshots/%s", item->id );
    shader = trap_R_RegisterShaderNoMip( shaderPath );
    if ( shader ) return shader;

    // 3. Fallback: levelshots/<basename> (strip any path prefix from id)
    {
        const char *namePart = strrchr( item->id, '/' );
        if ( namePart ) {
            Com_sprintf( shaderPath, sizeof( shaderPath ),
                         "levelshots/%s", namePart + 1 );
            shader = trap_R_RegisterShaderNoMip( shaderPath );
            if ( shader ) return shader;
        }
    }

    return 0;
}

// ---------------------------------------------------------------------------
// The WebUI's #library-screen in its Downloads mode (library.js mode 6)
// ---------------------------------------------------------------------------

static const char *dlTypeOptions[4] = { "All content", "Tracks", "Vehicles", "Skins" };
static const char *dlTypeValues[4] = { "", "track", "vehicle", "skin" };
static const vec4_t dlPopup     = { 0x1f / 255.0f, 0x1f / 255.0f, 0x1f / 255.0f, 1.0f };
static const vec4_t dlPopupLine = { 0x3b / 255.0f, 0x3b / 255.0f, 0x3b / 255.0f, 1.0f };

static qboolean DL_Busy( void ) {
    return s_dl.dlState == DL_STATE_FETCHING || s_dl.dlState == DL_STATE_DOWNLOADING;
}

/* renderList(): type filter, name, author and type matched; the selection
 * stays with its id while it matches, else the first match */
static void DL_Filter( void ) {
    char query[64], joined[256], lower[256];
    int i, keep = -1;

    Library_Query( &s_dl.lib, query, sizeof( query ) );
    s_dl.matchCount = 0;
    for ( i = 0; i < s_dl.numAllItems; i++ ) {
        dlItem_t *item = &s_dl.allItems[i];
        if ( s_dl.typeFilter && Q_stricmp( item->type, dlTypeValues[s_dl.typeFilter] ) ) continue;
        Com_sprintf( joined, sizeof( joined ), "%s %s %s", item->name, item->author, item->type );
        Library_Lower( joined, lower, sizeof( lower ) );
        if ( query[0] && !strstr( lower, query ) ) continue;
        if ( !strcmp( item->id, s_dl.selectedId ) ) keep = s_dl.matchCount;
        Com_sprintf( s_dl.rowText[s_dl.matchCount], sizeof( s_dl.rowText[0] ), "%s%s", item->name,
                     item->installed ? " / Installed" : "" );
        s_dl.matches[s_dl.matchCount++] = i;
    }
    s_dl.lib.selectedRow = keep >= 0 ? keep : ( s_dl.matchCount ? 0 : -1 );
    Q_strncpyz( s_dl.selectedId, s_dl.lib.selectedRow >= 0 ? s_dl.allItems[s_dl.matches[s_dl.lib.selectedRow]].id : "",
                sizeof( s_dl.selectedId ) );
}

static dlItem_t *DL_Selected( void ) {
    return s_dl.lib.selectedRow >= 0 ? &s_dl.allItems[s_dl.matches[s_dl.lib.selectedRow]] : NULL;
}

static void DL_StatusText( char *out, int size ) {
    switch ( s_dl.dlState ) {
    case DL_STATE_IDLE:     Q_strncpyz( out, "Choose content to install.", size ); break;
    case DL_STATE_FETCHING: Q_strncpyz( out, "Fetching catalog...", size ); break;
    case DL_STATE_READY:    Q_strncpyz( out, "Catalog ready.", size ); break;
    case DL_STATE_DOWNLOADING:
        Com_sprintf( out, size, "Downloading %s / %d%%", s_dl.dlFilename, (int)s_dl.dlProgress );
        break;
    case DL_STATE_DONE:     Q_strncpyz( out, "Installed. Restart Q3Rally to use the new content.", size ); break;
    case DL_STATE_ERROR:
        Q_strncpyz( out, s_dl.dlError[0] ? s_dl.dlError : "Download failed. Try refreshing the catalog.", size );
        break;
    default:                Q_strncpyz( out, "Waiting for download service.", size ); break;
    }
}

static qhandle_t DL_BuildLayout( void ) {
    rivalsLayout_t *l = &s_dl.lib.layout;
    dlItem_t *item;
    qhandle_t preview;
    int i;

    DL_Filter();
    item = DL_Selected();
    preview = item ? DL_GetPreviewShader( item ) : 0;
    memset( l, 0, sizeof( *l ) );
    l->downloads = qtrue;
    l->imageShown = preview != 0;
    l->count = s_dl.matchCount;
    for ( i = 0; i < l->count; i++ ) l->rowText[i] = s_dl.rowText[i];
    l->scroll = s_dl.lib.scroll;
    Q_strncpyz( l->name, item ? item->name : "Select content", sizeof( l->name ) );
    if ( item ) {
        char size[16];
        Com_sprintf( l->author, sizeof( l->author ), "By %s", item->author );
        /* [type, version, size in MB, state].filter(Boolean).join(' / ') */
        Com_sprintf( size, sizeof( size ), "%.1f MB", item->size_kb / 1024.0f );
        Com_sprintf( l->status, sizeof( l->status ), "%s%s%s%s%s / %s", item->type, item->type[0] ? " / " : "",
                     item->version, item->version[0] ? " / " : "", size, item->installed ? "Installed" : "Available" );
    }
    DL_StatusText( l->downloadStatus, sizeof( l->downloadStatus ) );
    Rivals_Layout( l, WS_ViewportWidth(), WS_ViewportHeight() );
    s_dl.lib.scroll = l->scroll;
    return preview;
}

/* the <select>: its box like the search field, the dark arrow at the right */
static void DL_DrawType( void ) {
    const rivalsLayout_t *l = &s_dl.lib.layout;
    const wsRect_t *r = &l->type;
    wsTextStyle_t body;
    float x0 = WS_Round( r->x ), y0 = WS_Round( r->y ), x1 = WS_Round( r->x + r->w ), y1 = WS_Round( r->y + r->h );
    static const vec4_t input = { 0x11 / 255.0f, 0x11 / 255.0f, 0x11 / 255.0f, 1.0f };
    int i;

    WS_BodyStyle( &body );
    body.lineHeight = 21.0f;
    WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
    WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, input );
    WS_DrawText( &body, r->x + 1.0f + WS_LU( 0.6f * WS_REM ), r->y + 1.0f + WS_LU( 0.6f * WS_REM ),
                 dlTypeOptions[s_dl.typeFilter], strlen( dlTypeOptions[s_dl.typeFilter] ), wsWhite );
    trap_R_SetColor( wsWhite );
    trap_R_DrawStretchPic( x1 - 14.0f, WS_Round( r->y + r->h * 0.5f ) - 3.0f, 10, 7, 0, 25 / 32.0f, 10 / 16.0f, 1.0f,
                           trap_R_RegisterShaderNoMip( "menu/ui/web_scrollbar" ) );
    trap_R_SetColor( NULL );

    /* the option list; in CEF this is the system's popup */
    if ( !s_dl.typeOpen ) return;
    body.lineHeight = WS_BODY_LINE;
    WS_FillRect( x0, y1, x1 - x0, 4 * WS_BODY_LINE + 2, dlPopupLine );
    WS_FillRect( x0 + 1, y1 + 1, x1 - x0 - 2, 4 * WS_BODY_LINE, dlPopup );
    for ( i = 0; i < 4; i++ ) {
        if ( i == s_dl.typeFilter ) WS_FillRect( x0 + 1, y1 + 1 + i * WS_BODY_LINE, x1 - x0 - 2, WS_BODY_LINE, dlPopupLine );
        WS_DrawText( &body, r->x + 1.0f + WS_LU( 0.6f * WS_REM ), y1 + 1 + i * WS_BODY_LINE, dlTypeOptions[i],
                     strlen( dlTypeOptions[i] ), wsWhite );
    }
}

static void DownloadsMenu_Draw( void ) {
    rivalsLayout_t *l = &s_dl.lib.layout;
    wsTextStyle_t body;
    qhandle_t preview;
    dlItem_t *item;
    char count[32];

    DL_PollState();
    Library_Track( &s_dl.lib );
    preview = DL_BuildLayout();
    item = DL_Selected();
    WS_BodyStyle( &body );

    WS_DrawBackground( l->pageWidth, l->pageHeight );
    WS_DrawFrontHeader( &l->page, "Expand your ", "rally.", "" );
    WS_DrawButton( &l->back, rivalsBack, qtrue, qfalse );

    /* left: search, type, count, list */
    WS_DrawSection( &l->left, qfalse );
    WS_DrawText( &body, l->leftX, l->searchLabelY, "Search", 6, wsWhite );
    Library_DrawSearch( &s_dl.lib, "Find content" );
    WS_DrawText( &body, l->leftX, l->typeLabelY, "Content type", 12, wsWhite );
    Com_sprintf( count, sizeof( count ), "%d items", s_dl.matchCount );
    WS_DrawText( &body, l->leftX, l->countY, count, strlen( count ), wsWhite );
    Library_DrawList( &s_dl.lib, NULL );
    Library_DrawScrollbar( &s_dl.lib );

    /* right: the content details and the transfer controls */
    WS_DrawSection( &l->right, qtrue );
    WS_DrawLabel( l->rightX, l->labelY, "CONTENT DETAILS", NULL );
    WS_DrawLines( &l->title, l->rightX, l->nameY, l->name, &l->nameLines, wsWhite );
    if ( preview ) {
        /* object-fit: contain; the previews are 4:3 levelshots */
        float h = l->car.h, w = h * 4.0f / 3.0f;
        if ( w > l->car.w ) { w = l->car.w; h = w * 0.75f; }
        trap_R_SetColor( NULL );
        trap_R_DrawStretchPic( l->car.x + ( l->car.w - w ) * 0.5f, l->car.y + ( l->car.h - h ) * 0.5f, w, h, 0, 0, 1, 1, preview );
    }
    WS_DrawLines( &body, l->rightX, l->authorY, l->author, &l->authorLines, wsSummary );
    WS_DrawLines( &body, l->rightX, l->statusY, l->status, &l->statusLines, wsSummary );
    Library_DrawButton( &l->install, downloadsLabels[0], qfalse, DL_Busy() || !item || item->installed );
    Library_DrawButton( &l->refresh, downloadsLabels[1], qtrue, DL_Busy() );
    Library_DrawButton( &l->cancel, downloadsLabels[2], qtrue, !DL_Busy() );
    WS_DrawProgress( l->progress.x, l->progress.y, l->progress.w, WS_Clamp( 0.0f, s_dl.dlProgress, 100.0f ) / 100.0f );
    WS_DrawLines( &body, l->rightX, l->downloadStatusY, l->downloadStatus, &l->downloadStatusLines, wsWhite );

    DL_DrawType();
    Menu_Draw( &s_dl.menu );
}

static void DL_Click( float x, float y ) {
    const rivalsLayout_t *l = &s_dl.lib.layout;
    qboolean changed;
    dlItem_t *item = DL_Selected();
    int row;

    if ( s_dl.typeOpen ) {
        s_dl.typeOpen = qfalse;
        if ( x >= l->type.x && x < l->type.x + l->type.w && y >= l->type.y + l->type.h &&
             y < l->type.y + l->type.h + 1 + 4 * WS_BODY_LINE ) {
            s_dl.typeFilter = (int)( ( y - l->type.y - l->type.h - 1 ) / WS_BODY_LINE );
            if ( s_dl.typeFilter < 0 ) s_dl.typeFilter = 0;
            if ( s_dl.typeFilter > 3 ) s_dl.typeFilter = 3;
        }
        return;
    }
    if ( Library_Inside( &l->back, x, y ) ) {
        UI_PopMenu();
        return;
    }
    if ( Library_Inside( &l->type, x, y ) ) {
        s_dl.lib.searchFocus = qfalse;
        s_dl.typeOpen = qtrue;
        return;
    }
    if ( Library_Click( &s_dl.lib, x, y, &changed ) ) return;
    row = Library_RowAt( &s_dl.lib, x, y );
    if ( row >= 0 ) {
        Q_strncpyz( s_dl.selectedId, s_dl.allItems[s_dl.matches[row]].id, sizeof( s_dl.selectedId ) );
        return;
    }
    if ( Library_Inside( &l->install, x, y ) && item ) UI_WebCatalogAction( 6, 3, s_dl.matches[s_dl.lib.selectedRow] );
    else if ( Library_Inside( &l->refresh, x, y ) ) UI_WebCatalogAction( 6, 1, -1 );
    else if ( Library_Inside( &l->cancel, x, y ) ) UI_WebCatalogAction( 6, 2, -1 );
}

static sfxHandle_t DownloadsMenu_Key( int key ) {
    const rivalsLayout_t *l = &s_dl.lib.layout;
    float x, y;
    int row;

    Library_Cursor( &x, &y );
    if ( key & K_CHAR_FLAG ) {
        Library_Char( &s_dl.lib, key & ~K_CHAR_FLAG );
        return 0;
    }
    switch ( key ) {
    case K_ESCAPE:
    case K_PAD0_B:
        if ( s_dl.typeOpen ) s_dl.typeOpen = qfalse;
        else UI_PopMenu();
        return 0;
    case K_MOUSE1:
        DL_Click( x, y );
        return 0;
    case K_MWHEELUP:
    case K_MWHEELDOWN:
        if ( Library_Inside( &l->list, x, y ) ) s_dl.lib.scroll += key == K_MWHEELUP ? -100.0f : 100.0f;
        return 0;
    case K_UPARROW:
    case K_KP_UPARROW:
    case K_DOWNARROW:
    case K_KP_DOWNARROW:
    case K_PAD0_DPAD_UP:
    case K_PAD0_DPAD_DOWN:
        if ( !s_dl.matchCount ) return 0;
        row = s_dl.lib.selectedRow;
        if ( key == K_UPARROW || key == K_KP_UPARROW || key == K_PAD0_DPAD_UP ) row = row > 0 ? row - 1 : 0;
        else row = row < s_dl.matchCount - 1 ? row + 1 : s_dl.matchCount - 1;
        Q_strncpyz( s_dl.selectedId, s_dl.allItems[s_dl.matches[row]].id, sizeof( s_dl.selectedId ) );
        s_dl.lib.selectedRow = row;
        Library_ScrollTo( &s_dl.lib, row );
        return 0;
    }
    return 0;
}

/*
=================
UI_Rally_DownloadsMenu
=================
*/
void UI_Rally_DownloadsMenu( void ) {
    char preview[96];

    memset( &s_dl, 0, sizeof( s_dl ) );
    s_dl.menu.draw       = DownloadsMenu_Draw;
    s_dl.menu.key        = DownloadsMenu_Key;
    s_dl.menu.fullscreen = qtrue;
    s_dl.activeTab   = DL_TYPE_ALL;
    s_dl.selectedItem = -1;

    /* ui_downloadsPreview "state,progress,type,search,item" pins a state for
     * screenshots (tools/ui_webstyle); the catalog is ui_dl_indexpath */
    trap_Cvar_VariableStringBuffer( "ui_downloadsPreview", preview, sizeof( preview ) );
    if ( preview[0] ) {
        char *field[5];
        int n;
        field[0] = preview;
        for ( n = 1; n < 5; n++ ) {
            field[n] = field[n - 1] ? strchr( field[n - 1], ',' ) : NULL;
            if ( field[n] ) *field[n]++ = '\0';
        }
        DL_LoadIndex();
        trap_Cvar_Set( "ui_dl_state", field[0] );
        if ( field[1] ) trap_Cvar_Set( "ui_dl_progress", field[1] );
        trap_Cvar_Set( "ui_dl_filename", "trk_night_city_x.pk3" );
        DL_PollState();
        if ( field[2] ) s_dl.typeFilter = atoi( field[2] ) & 3;
        if ( field[3] ) Q_strncpyz( s_dl.lib.query, field[3], sizeof( s_dl.lib.query ) );
        if ( field[4] && atoi( field[4] ) >= 0 && atoi( field[4] ) < s_dl.numAllItems && field[4][0] != '-' )
            Q_strncpyz( s_dl.selectedId, s_dl.allItems[atoi( field[4] )].id, sizeof( s_dl.selectedId ) );
        UI_PushMenu( &s_dl.menu );
        return;
    }

    // Poll initial state
    DL_PollState();

    // If we already have a cached index, load it immediately
    if ( s_dl.dlState != DL_STATE_FETCHING && s_dl.dlState != DL_STATE_DOWNLOADING &&
         s_dl.numAllItems == 0 ) {
        DL_LoadIndex();
        DL_ApplyFilter();
    }

    // Auto-fetch index when opening the menu fresh
    if ( s_dl.dlState == DL_STATE_IDLE ) {
        trap_Cvar_Set( "ui_dl_action", "fetch_index" );
    }

    trap_Key_SetCatcher( KEYCATCH_UI );
    UI_PushMenu( &s_dl.menu );
}

int UI_WebDownloadsScreen( void ) { return uis.activemenu == &s_dl.menu ? 6 : 0; }
int UI_WebDownloadCount( void ) { return s_dl.numAllItems; }
int UI_WebDownloadChar( int index, int field, int character ) {
    const char *value;
    dlItem_t *item;
    if ( index < 0 || index >= s_dl.numAllItems ) return 0;
    item = &s_dl.allItems[index];
    switch ( field ) {
    case 0: value = item->name; break;
    case 1: value = item->author; break;
    case 2: value = item->type; break;
    case 3: value = item->version; break;
    case 4: value = item->id; break;
    default: return 0;
    }
    if ( character < 0 || character >= (int)strlen( value ) ) return 0;
    return (unsigned char)value[character];
}
int UI_WebDownloadNumber( int index, int field ) {
    if ( index == -1 && field == 0 ) return s_dl.catalogRevision;
    if ( index < 0 || index >= s_dl.numAllItems ) return 0;
    return field == 0 ? s_dl.allItems[index].size_kb : s_dl.allItems[index].installed;
}
void UI_WebDownloadTick( void ) {
    if ( UI_WebDownloadsScreen() ) DL_PollState();
}
void UI_WebCatalogAction( int screen, int action, int index ) {
    if ( screen == 5 && action == 0 ) {
        UI_WebRivalsBack();
        return;
    }
    if ( screen != 6 || !UI_WebDownloadsScreen() ) return;
    switch ( action ) {
    case 0: UI_PopMenu(); break;
    case 1:
        if ( s_dl.dlState != DL_STATE_FETCHING && s_dl.dlState != DL_STATE_DOWNLOADING )
            trap_Cvar_Set( "ui_dl_action", "fetch_index" );
        break;
    case 2:
        if ( s_dl.dlState == DL_STATE_FETCHING || s_dl.dlState == DL_STATE_DOWNLOADING )
            trap_Cvar_Set( "ui_dl_action", "cancel" );
        break;
    case 3:
        if ( index >= 0 && index < s_dl.numAllItems &&
             s_dl.dlState != DL_STATE_FETCHING && s_dl.dlState != DL_STATE_DOWNLOADING &&
             !s_dl.allItems[index].installed )
            trap_Cvar_Set( "ui_dl_action", va( "download:%s", s_dl.allItems[index].id ) );
        break;
    }
}
