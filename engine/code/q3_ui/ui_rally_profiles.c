#include "ui_local.h"

#define MAX_PROFILE_FILES   64
#define PROFILE_STATUS_LINES 1

#define ID_PROFILE_LIST      200
#define ID_PROFILE_CREATE    201
#define ID_PROFILE_DELETE    202
#define ID_PROFILE_SELECT    203
#define ID_PROFILE_NAME      204

static vec4_t overlayBackgroundColor = { 0.0f, 0.0f, 0.0f, 0.85f };
static vec4_t statusNormalColor = { 1.0f, 1.0f, 1.0f, 1.0f };
static vec4_t statusErrorColor  = { 1.0f, 0.3f, 0.3f, 1.0f };
static vec4_t statusInfoColor   = { 1.0f, 0.8f, 0.3f, 1.0f };

static const char *emptyProfileList[] = { "No profiles", NULL };

typedef struct {
    menuframework_s menu;
    menutext_s      title;
    menulist_s      list;
    menufield_s     nameField;
    menutext_s      createButton;
    menutext_s      deleteButton;
    menutext_s      selectButton;
    menutext_s      hint;

    char            profileNames[MAX_PROFILE_FILES][PROFILE_MAX_NAME];
    const char     *listItems[MAX_PROFILE_FILES + 1];
    int             profileCount;

    char            statusLine[128];
    vec4_t          statusColor;

    qboolean        forcingSelection;
} profileOverlay_t;

static profileOverlay_t s_profileOverlay;

static void UI_ProfileOverlay_Draw( void );
static sfxHandle_t UI_ProfileOverlay_Key( int key );

static void UI_ProfileOverlay_SetStatus( const char *text, const vec4_t color ) {
    if ( text ) {
        Q_strncpyz( s_profileOverlay.statusLine, text, sizeof( s_profileOverlay.statusLine ) );
    } else {
        s_profileOverlay.statusLine[0] = '\0';
    }
    if ( color ) {
        Vector4Copy( color, s_profileOverlay.statusColor );
    } else {
        Vector4Copy( statusNormalColor, s_profileOverlay.statusColor );
    }
}

static void UI_ProfileOverlay_TrimName( char *name ) {
    char *start;
    char *end;

    if ( !name ) {
        return;
    }

    start = name;
    while ( *start && Q_IsColorString( start ) ) {
        start += 2;
    }

    while ( *start == ' ' || *start == '\t' ) {
        start++;
    }

    if ( start != name ) {
        memmove( name, start, strlen( start ) + 1 );
    }

    end = name + strlen( name );
    while ( end > name && ( end[-1] == ' ' || end[-1] == '\t' ) ) {
        *--end = '\0';
    }
}

static qboolean UI_Profile_NameIsValid( const char *name, char *error, int errorSize ) {
    int length;
    int i;

    if ( !name ) {
        if ( error && errorSize > 0 ) {
            Q_strncpyz( error, "Invalid profile name", errorSize );
        }
        return qfalse;
    }

    length = strlen( name );
    if ( length <= 0 ) {
        if ( error && errorSize > 0 ) {
            Q_strncpyz( error, "Name cannot be empty", errorSize );
        }
        return qfalse;
    }

    if ( length >= PROFILE_MAX_NAME ) {
        if ( error && errorSize > 0 ) {
            Q_strncpyz( error, "Name is too long", errorSize );
        }
        return qfalse;
    }

    for ( i = 0; i < length; ++i ) {
        char c = name[i];
        if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) ) {
            continue;
        }
        if ( c == '_' || c == '-' ) {
            continue;
        }
        if ( error && errorSize > 0 ) {
            Q_strncpyz( error, "Use letters, numbers, '-' or '_' only", errorSize );
        }
        return qfalse;
    }

    return qtrue;
}

static double UI_Profile_ParseDouble( const char *buffer, const char *key, double defaultValue ) {
    char pattern[64];
    const char *cursor;

    Com_sprintf( pattern, sizeof( pattern ), "\"%s\"", key );
    cursor = strstr( buffer, pattern );
    if ( !cursor ) {
        return defaultValue;
    }

    cursor = strchr( cursor, ':' );
    if ( !cursor ) {
        return defaultValue;
    }

    cursor++;
    while ( *cursor == ' ' || *cursor == '\t' ) {
        cursor++;
    }

    return atof( cursor );
}

static int UI_Profile_ParseInt( const char *buffer, const char *key, int defaultValue ) {
    char pattern[64];
    const char *cursor;

    Com_sprintf( pattern, sizeof( pattern ), "\"%s\"", key );
    cursor = strstr( buffer, pattern );
    if ( !cursor ) {
        return defaultValue;
    }

    cursor = strchr( cursor, ':' );
    if ( !cursor ) {
        return defaultValue;
    }

    cursor++;
    while ( *cursor == ' ' || *cursor == '\t' ) {
        cursor++;
    }

    return atoi( cursor );
}

static qboolean UI_Profile_ReadStats( const char *name, profile_stats_t *out ) {
    fileHandle_t file;
    char path[MAX_QPATH];
    char buffer[1024];
    int length;

    if ( !name || !name[0] || !out ) {
        return qfalse;
    }

    Com_sprintf( path, sizeof( path ), "profiles/%s.json", name );
    length = trap_FS_FOpenFile( path, &file, FS_READ );
    if ( length <= 0 ) {
        if ( file ) {
            trap_FS_FCloseFile( file );
        }
        return qfalse;
    }

    if ( length >= (int)sizeof( buffer ) ) {
        length = sizeof( buffer ) - 1;
    }

    trap_FS_Read( buffer, length, file );
    buffer[length] = '\0';
    trap_FS_FCloseFile( file );

    out->distanceKm = UI_Profile_ParseDouble( buffer, "distanceKm", 0.0 );
    out->fuelUsed = UI_Profile_ParseDouble( buffer, "fuelUsed", 0.0 );
    out->bestLapMs = UI_Profile_ParseInt( buffer, "bestLapMs", 0 );
    out->kills = UI_Profile_ParseInt( buffer, "kills", 0 );
    out->deaths = UI_Profile_ParseInt( buffer, "deaths", 0 );
    out->wins = UI_Profile_ParseInt( buffer, "wins", 0 );
    out->losses = UI_Profile_ParseInt( buffer, "losses", 0 );
    out->flagCaptures = UI_Profile_ParseInt( buffer, "flagCaptures", 0 );

    return qtrue;
}

static qboolean UI_Profile_WriteDefaultFile( const char *name ) {
    fileHandle_t file;
    char path[MAX_QPATH];
    char buffer[512];
    int length;

    if ( !name || !name[0] ) {
        return qfalse;
    }

    Com_sprintf( path, sizeof( path ), "profiles/%s.json", name );
    length = Com_sprintf( buffer, sizeof( buffer ),
        "{\n"
        "\t\"name\": \"%s\",\n"
        "\t\"stats\": {\n"
        "\t\t\"distanceKm\": 0.0,\n"
        "\t\t\"fuelUsed\": 0.0,\n"
        "\t\t\"bestLapMs\": 0,\n"
        "\t\t\"kills\": 0,\n"
        "\t\t\"deaths\": 0,\n"
        "\t\t\"wins\": 0,\n"
        "\t\t\"losses\": 0,\n"
        "\t\t\"flagCaptures\": 0\n"
        "\t}\n"
        "}\n",
        name );

    trap_FS_FOpenFile( path, &file, FS_WRITE );
    if ( file < 0 ) {
        return qfalse;
    }

    trap_FS_Write( buffer, length, file );
    trap_FS_FCloseFile( file );
    return qtrue;
}

static void UI_ProfileOverlay_LoadProfiles( void ) {
    char fileBuffer[4096];
    char activeName[PROFILE_MAX_NAME];
    char *ptr;
    int total;
    int index;

    s_profileOverlay.profileCount = 0;
    trap_Cvar_VariableStringBuffer( "profile_active", activeName, sizeof( activeName ) );

    total = trap_FS_GetFileList( "profiles", ".json", fileBuffer, sizeof( fileBuffer ) );
    ptr = fileBuffer;

    for ( int i = 0; i < total && s_profileOverlay.profileCount < MAX_PROFILE_FILES; ++i ) {
        char name[MAX_QPATH];
        int len = strlen( ptr );
        int fileLen;
        fileHandle_t file;

        if ( len <= 0 ) {
            ptr++;
            continue;
        }

        COM_StripExtension( ptr, name, sizeof( name ) );
        if ( !UI_Profile_NameIsValid( name, NULL, 0 ) ) {
            ptr += len + 1;
            continue;
        }

        // ignore zero-length files (treated as deleted)
        fileLen = trap_FS_FOpenFile( va( "profiles/%s.json", name ), &file, FS_READ );
        if ( fileLen <= 0 ) {
            if ( file ) {
                trap_FS_FCloseFile( file );
            }
            ptr += len + 1;
            continue;
        }
        trap_FS_FCloseFile( file );

        Q_strncpyz( s_profileOverlay.profileNames[s_profileOverlay.profileCount], name, PROFILE_MAX_NAME );
        s_profileOverlay.listItems[s_profileOverlay.profileCount] = s_profileOverlay.profileNames[s_profileOverlay.profileCount];
        s_profileOverlay.profileCount++;
        ptr += len + 1;
    }

    s_profileOverlay.listItems[s_profileOverlay.profileCount] = NULL;

    s_profileOverlay.list.generic.flags &= ~QMF_GRAYED;
    s_profileOverlay.deleteButton.generic.flags &= ~QMF_GRAYED;
    s_profileOverlay.selectButton.generic.flags &= ~QMF_GRAYED;

    if ( s_profileOverlay.profileCount <= 0 ) {
        s_profileOverlay.list.itemnames = emptyProfileList;
        s_profileOverlay.list.numitems = 1;
        s_profileOverlay.list.curvalue = 0;
        s_profileOverlay.list.generic.flags |= QMF_GRAYED;
        s_profileOverlay.deleteButton.generic.flags |= QMF_GRAYED;
        s_profileOverlay.selectButton.generic.flags |= QMF_GRAYED;
        s_profileOverlay.forcingSelection = qtrue;
        UI_ProfileOverlay_SetStatus( "Create a new profile to continue", statusInfoColor );
        return;
    }

    s_profileOverlay.list.itemnames = s_profileOverlay.listItems;
    s_profileOverlay.list.numitems = s_profileOverlay.profileCount;

    index = 0;
    if ( activeName[0] ) {
        for ( int i = 0; i < s_profileOverlay.profileCount; ++i ) {
            if ( !Q_stricmp( s_profileOverlay.profileNames[i], activeName ) ) {
                index = i;
                break;
            }
        }
    }
    s_profileOverlay.list.curvalue = index;
    s_profileOverlay.forcingSelection = qfalse;
    UI_ProfileOverlay_SetStatus( "Select a profile to continue", statusNormalColor );
}

static void UI_ProfileOverlay_MenuEvent( void *ptr, int event );
static void UI_ProfileOverlay_HandleCreate( void );
static void UI_ProfileOverlay_HandleDelete( void );
static void UI_ProfileOverlay_HandleSelect( void );

static void UI_ProfileOverlay_SetupMenu( void ) {
    profileOverlay_t *overlay = &s_profileOverlay;

    Com_Memset( overlay, 0, sizeof( *overlay ) );

    overlay->menu.fullscreen = qtrue;
    overlay->menu.wrapAround = qfalse;
    overlay->menu.draw = UI_ProfileOverlay_Draw;
    overlay->menu.key = UI_ProfileOverlay_Key;

    overlay->title.generic.type = MTYPE_BTEXT;
    overlay->title.generic.flags = QMF_INACTIVE;
    overlay->title.generic.x = 320;
    overlay->title.generic.y = 78;
    overlay->title.string = "PROFILE SELECTION";
    overlay->title.color = text_color_normal;
    overlay->title.style = UI_CENTER | UI_BIGFONT;

    overlay->list.generic.type = MTYPE_SPINCONTROL;
    overlay->list.generic.flags = QMF_CENTER_JUSTIFY;
    overlay->list.generic.id = ID_PROFILE_LIST;
    overlay->list.generic.callback = UI_ProfileOverlay_MenuEvent;
    overlay->list.generic.x = 320;
    overlay->list.generic.y = 154;
    overlay->list.curvalue = 0;
    overlay->list.itemnames = overlay->listItems;

    overlay->hint.generic.type = MTYPE_PTEXT;
    overlay->hint.generic.flags = QMF_INACTIVE | QMF_CENTER_JUSTIFY;
    overlay->hint.generic.x = 320;
    overlay->hint.generic.y = 196;
    overlay->hint.string = "Left/Right to browse existing profiles";
    overlay->hint.style = UI_CENTER | UI_SMALLFONT;
    overlay->hint.color = text_color_normal;

    overlay->nameField.generic.type = MTYPE_FIELD;
    overlay->nameField.generic.id = ID_PROFILE_NAME;
    overlay->nameField.generic.flags = QMF_CENTER_JUSTIFY;
    overlay->nameField.generic.x = 320;
    overlay->nameField.generic.y = 228;
    overlay->nameField.field.cursor = 0;
    overlay->nameField.field.scroll = 0;
    overlay->nameField.field.widthInChars = 20;
    overlay->nameField.field.maxchars = PROFILE_MAX_NAME - 1;
    overlay->nameField.field.buffer[0] = '\0';

    overlay->createButton.generic.type = MTYPE_PTEXT;
    overlay->createButton.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    overlay->createButton.generic.id = ID_PROFILE_CREATE;
    overlay->createButton.generic.callback = UI_ProfileOverlay_MenuEvent;
    overlay->createButton.generic.x = 320;
    overlay->createButton.generic.y = 266;
    overlay->createButton.string = "CREATE";
    overlay->createButton.style = UI_CENTER | UI_SMALLFONT;
    overlay->createButton.color = text_color_normal;

    overlay->deleteButton.generic.type = MTYPE_PTEXT;
    overlay->deleteButton.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    overlay->deleteButton.generic.id = ID_PROFILE_DELETE;
    overlay->deleteButton.generic.callback = UI_ProfileOverlay_MenuEvent;
    overlay->deleteButton.generic.x = 320;
    overlay->deleteButton.generic.y = 296;
    overlay->deleteButton.string = "DELETE";
    overlay->deleteButton.style = UI_CENTER | UI_SMALLFONT;
    overlay->deleteButton.color = text_color_normal;

    overlay->selectButton.generic.type = MTYPE_PTEXT;
    overlay->selectButton.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS;
    overlay->selectButton.generic.id = ID_PROFILE_SELECT;
    overlay->selectButton.generic.callback = UI_ProfileOverlay_MenuEvent;
    overlay->selectButton.generic.x = 320;
    overlay->selectButton.generic.y = 328;
    overlay->selectButton.string = "SELECT";
    overlay->selectButton.style = UI_CENTER | UI_SMALLFONT;
    overlay->selectButton.color = text_color_normal;

    UI_ProfileOverlay_LoadProfiles();

    Menu_AddItem( &overlay->menu, &overlay->title );
    Menu_AddItem( &overlay->menu, &overlay->list );
    Menu_AddItem( &overlay->menu, &overlay->hint );
    Menu_AddItem( &overlay->menu, &overlay->nameField );
    Menu_AddItem( &overlay->menu, &overlay->createButton );
    Menu_AddItem( &overlay->menu, &overlay->deleteButton );
    Menu_AddItem( &overlay->menu, &overlay->selectButton );

    Menu_SetCursorToItem( &overlay->menu, &overlay->selectButton );
}

static void UI_ProfileOverlay_MenuEvent( void *ptr, int event ) {
    menucommon_s *item;

    if ( event != QM_ACTIVATED ) {
        return;
    }

    item = (menucommon_s *)ptr;
    switch ( item->id ) {
        case ID_PROFILE_CREATE:
            UI_ProfileOverlay_HandleCreate();
            break;
        case ID_PROFILE_DELETE:
            UI_ProfileOverlay_HandleDelete();
            break;
        case ID_PROFILE_SELECT:
            UI_ProfileOverlay_HandleSelect();
            break;
    }
}

static void UI_ProfileOverlay_HandleCreate( void ) {
    char name[PROFILE_MAX_NAME];
    char error[64];
    int i;

    Q_strncpyz( name, s_profileOverlay.nameField.field.buffer, sizeof( name ) );
    UI_ProfileOverlay_TrimName( name );

    if ( !UI_Profile_NameIsValid( name, error, sizeof( error ) ) ) {
        UI_ProfileOverlay_SetStatus( error, statusErrorColor );
        return;
    }

    for ( i = 0; i < s_profileOverlay.profileCount; ++i ) {
        if ( !Q_stricmp( s_profileOverlay.profileNames[i], name ) ) {
            UI_ProfileOverlay_SetStatus( "Profile already exists", statusErrorColor );
            return;
        }
    }

    if ( !UI_Profile_WriteDefaultFile( name ) ) {
        UI_ProfileOverlay_SetStatus( "Failed to create profile", statusErrorColor );
        return;
    }

    s_profileOverlay.nameField.field.buffer[0] = '\0';
    s_profileOverlay.nameField.field.cursor = 0;
    s_profileOverlay.nameField.field.scroll = 0;
    UI_ProfileOverlay_LoadProfiles();

    for ( i = 0; i < s_profileOverlay.profileCount; ++i ) {
        if ( !Q_stricmp( s_profileOverlay.profileNames[i], name ) ) {
            s_profileOverlay.list.curvalue = i;
            break;
        }
    }

    UI_ProfileOverlay_SetStatus( "Profile created", statusInfoColor );
}

static void UI_ProfileOverlay_HandleDelete( void ) {
    const char *name;
    char path[MAX_QPATH];
    fileHandle_t file;

    if ( s_profileOverlay.profileCount <= 0 ) {
        UI_ProfileOverlay_SetStatus( "Nothing to delete", statusErrorColor );
        return;
    }

    name = s_profileOverlay.profileNames[ s_profileOverlay.list.curvalue ];
    if ( !name || !name[0] ) {
        UI_ProfileOverlay_SetStatus( "Invalid selection", statusErrorColor );
        return;
    }

    Com_sprintf( path, sizeof( path ), "profiles/%s.json", name );
    trap_FS_FOpenFile( path, &file, FS_WRITE );
    if ( file < 0 ) {
        UI_ProfileOverlay_SetStatus( "Unable to remove profile", statusErrorColor );
        return;
    }
    trap_FS_FCloseFile( file );

    if ( !Q_stricmp( uis.activeProfile, name ) ) {
        uis.activeProfile[0] = '\0';
        trap_Cvar_Set( "profile_active", "" );
        trap_Cvar_Update( &ui_profileActive );
        UI_Profile_MarkStatsDirty();
    }

    UI_ProfileOverlay_LoadProfiles();
    UI_ProfileOverlay_SetStatus( "Profile deleted", statusInfoColor );
}

static void UI_ProfileOverlay_HandleSelect( void ) {
    const char *name;

    if ( s_profileOverlay.profileCount <= 0 ) {
        UI_ProfileOverlay_SetStatus( "Create a profile first", statusErrorColor );
        return;
    }

    name = s_profileOverlay.profileNames[ s_profileOverlay.list.curvalue ];
    if ( !name || !name[0] ) {
        UI_ProfileOverlay_SetStatus( "Invalid profile", statusErrorColor );
        return;
    }

    trap_Cvar_Set( "profile_active", name );
    trap_Cvar_Update( &ui_profileActive );
    Q_strncpyz( uis.activeProfile, name, sizeof( uis.activeProfile ) );
    uis.profileOverlayShown = qtrue;
    UI_Profile_MarkStatsDirty();

    UI_PopMenu();
}

static void UI_ProfileOverlay_Draw( void ) {
    trap_R_SetColor( overlayBackgroundColor );
    UI_FillRect( 0, 0, 640, 480, overlayBackgroundColor );
    trap_R_SetColor( NULL );

    Menu_Draw( &s_profileOverlay.menu );

    if ( s_profileOverlay.statusLine[0] ) {
        UI_DrawProportionalString( 320, 370, s_profileOverlay.statusLine, UI_CENTER | UI_SMALLFONT, s_profileOverlay.statusColor );
    }

    UI_DrawProportionalString( 320, 410, "Enter a new name and press CREATE", UI_CENTER | UI_SMALLFONT, text_color_normal );
}

static sfxHandle_t UI_ProfileOverlay_Key( int key ) {
    if ( key == K_ESCAPE ) {
        return 0;
    }
    return Menu_DefaultKey( &s_profileOverlay.menu, key );
}

void UI_ProfileOverlay_InitSession( void ) {
    uis.profileOverlayShown = qfalse;
    uis.activeProfile[0] = '\0';
    uis.activeProfileStatsValid = qfalse;
    uis.activeProfileLastRead = 0;

    trap_Cvar_Update( &ui_profileActive );
    if ( ui_profileActive.string && ui_profileActive.string[0] ) {
        Q_strncpyz( uis.activeProfile, ui_profileActive.string, sizeof( uis.activeProfile ) );
    } else {
        trap_Cvar_VariableStringBuffer( "profile_active", uis.activeProfile, sizeof( uis.activeProfile ) );
    }
}

void UI_ProfileOverlay_ClearState( void ) {
    Com_Memset( &s_profileOverlay, 0, sizeof( s_profileOverlay ) );
}

void UI_ProfileOverlay_MaybeShow( void ) {
    if ( uis.profileOverlayShown ) {
        return;
    }

    UI_ProfileOverlay_SetupMenu();
    uis.profileOverlayShown = qtrue;
    UI_PushMenu( &s_profileOverlay.menu );
}

void UI_Profile_MarkStatsDirty( void ) {
    uis.activeProfileStatsValid = qfalse;
}

const char *UI_Profile_GetActiveName( void ) {
    return uis.activeProfile;
}

qboolean UI_Profile_HasActiveProfile( void ) {
    return ( qboolean )( uis.activeProfile[0] != '\0' );
}

const profile_stats_t *UI_Profile_GetActiveStats( void ) {
    if ( !uis.activeProfile[0] ) {
        return NULL;
    }

    if ( !uis.activeProfileStatsValid || uis.realtime - uis.activeProfileLastRead > 1000 ) {
        if ( UI_Profile_ReadStats( uis.activeProfile, &uis.activeProfileStats ) ) {
            uis.activeProfileStatsValid = qtrue;
            uis.activeProfileLastRead = uis.realtime;
        } else {
            return NULL;
        }
    }

    return &uis.activeProfileStats;
}
