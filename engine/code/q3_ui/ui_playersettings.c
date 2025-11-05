/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

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
#include "ui_local.h"

// STONELANCE
/*
#define ART_FRAMEL			"menu/art/frame2_l"
#define ART_FRAMER			"menu/art/frame1_r"
#define ART_MODEL0			"menu/art/model_0"
#define ART_MODEL1			"menu/art/model_1"
#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"
*/
#define ART_SELECT			"menu/art/menu_select"
#define ART_SELECTED		"menu/art/menu_selected"
#define ART_PORT			"menu/art/menu_port"
#define ART_LEFT0			"menu/art/arrow_l0"
#define ART_LEFT1			"menu/art/arrow_l1"
#define ART_RIGHT0			"menu/art/arrow_r0"
#define ART_RIGHT1			"menu/art/arrow_r1"
// END
#define ART_FX_BASE			"menu/art/fx_base"
#define ART_FX_BLUE			"menu/art/fx_blue"
#define ART_FX_CYAN			"menu/art/fx_cyan"
#define ART_FX_GREEN		"menu/art/fx_grn"
#define ART_FX_RED			"menu/art/fx_red"
#define ART_FX_TEAL			"menu/art/fx_teal"
#define ART_FX_WHITE		"menu/art/fx_white"
#define ART_FX_YELLOW		"menu/art/fx_yel"

#define ID_NAME			10
#define ID_HANDICAP		11
#define ID_EFFECTS		12
#define ID_BACK			13
// STONELANCE
// #define ID_MODEL		14
#define ID_CUSTOMIZE	14

#define ID_FAVORITE1	15
#define ID_FAVORITE2	16
#define ID_FAVORITE3	17
#define ID_FAVORITE4	18
#define ID_LEFT			19
#define ID_RIGHT		20
#define ID_PLATE		21
// END
#define ID_TAB_CAR              30
#define ID_TAB_STATS            31
#define ID_TAB_ACHIEVEMENTS     32
#define ID_PROFILE_LIST         33
#define ID_PROFILE_NEWNAME      34
#define ID_PROFILE_CREATE       35
#define ID_PROFILE_DELETE       36
#define ID_PROFILE_SELECT       37

#define MAX_NAMELENGTH	20
// STONELANCE
#define NUM_FAVORITES		4
#define MAX_PLAYERMODELS	256
// END

#define PLAYERSETTINGS_NUM_TABS         3
#define PLAYERSETTINGS_TAB_CAR          0
#define PLAYERSETTINGS_TAB_STATS        1
#define PLAYERSETTINGS_TAB_ACHIEVEMENTS 2

#define MAX_PROFILE_SLOTS               8
#define MAX_PROFILE_NAME_LENGTH         32

#define PROFILE_SLOTS_CVAR              "ui_profile_slots"

#define PROFILE_ACHIEVEMENT_DISTANCE_100KM      (1 << 0)
#define PROFILE_ACHIEVEMENT_DISTANCE_500KM      (1 << 1)
#define PROFILE_ACHIEVEMENT_MATCHES_10          (1 << 2)
#define PROFILE_ACHIEVEMENT_MATCHES_50          (1 << 3)

typedef struct {
        profileLifetime_t       raw;
        float                   totalDistanceMeters;
        float                   totalFuelConsumed;
        int                     sequence;
        qboolean                valid;
} playerLifetimeDisplay_t;

typedef struct {
	menuframework_s		menu;

	menutext_s			banner;
// STONELANCE
/*
	menubitmap_s		framel;
	menubitmap_s		framer;
*/
// END
	menubitmap_s		player;

	menufield_s			name;
	menulist_s			handicap;
	menulist_s			effects;

// STONELANCE
//	menubitmap_s		back;
	menutext_s			back;
	menutext_s			customize;
	menutext_s			plate;

	menubitmap_s		left;
	menutext_s			modelname;
	menubitmap_s		right;

	menubitmap_s		tabButtons[PLAYERSETTINGS_NUM_TABS];
	menutext_s			tabLabels[PLAYERSETTINGS_NUM_TABS];

	menutext_s			statsPanel;
	menutext_s			achievementsPanel;

	menulist_s			profileList;
	menufield_s		profileName;
	menutext_s			profileNameLabel;
	menutext_s			profileCreate;
	menutext_s			profileDelete;
	menutext_s			profileSelect;

	const char				*profileItems[MAX_PROFILE_SLOTS + 1];
	char				profileNames[MAX_PROFILE_SLOTS][MAX_PROFILE_NAME_LENGTH];
	int					profileCount;
	int					activeTab;
	playerLifetimeDisplay_t lifetimeDisplay;

	menutext_s			favorites;
	menubitmap_s		favpics[NUM_FAVORITES];
	menubitmap_s		favpicbuttons[NUM_FAVORITES];
	menubitmap_s		ports[NUM_FAVORITES];

	char				modelList[MAX_PLAYERMODELS][MAX_QPATH];
	int					selectedModel;
	int					numModels;
	int					allModels;

	char				modelskin[MAX_QPATH];
	char				rimskin[MAX_QPATH];
	char				headskin[MAX_QPATH];

	char				favIcons[NUM_FAVORITES][MAX_QPATH];
	qboolean			modelChanged;
// END

	qhandle_t			fxBasePic;
	qhandle_t			fxPic[7];
	playerInfo_t		playerinfo;
	int					current_fx;
	char				playerModel[MAX_QPATH];
} playersettings_t;

static playersettings_t	s_playersettings;
static int s_playersettingsInitialTab = PLAYERSETTINGS_TAB_CAR;

static int gamecodetoui[] = {4,2,3,0,5,1,6};
static int uitogamecode[] = {4,6,2,3,1,5,7};

static const char *handicap_items[] = {
        "None",
        "95",
        "90",
	"85",
	"80",
	"75",
	"70",
	"65",
	"60",
	"55",
	"50",
	"45",
	"40",
	"35",
	"30",
	"25",
	"20",
	"15",
	"10",
	"5",
        0
};


static void PlayerSettings_SetMenuItemVisible( menucommon_s *item, qboolean visible );
static void PlayerSettings_SetActiveTab( int tab );
static void PlayerSettings_UpdateTabVisibility( void );
static void PlayerSettings_UpdateTabHighlight( void );
static void PlayerSettings_DrawTabButton( void *self );
static void PlayerSettings_DrawStatsPanel( void *self );
static void PlayerSettings_DrawAchievementsPanel( void *self );
static void PlayerSettings_LoadProfileSlots( void );
static void PlayerSettings_SaveProfileSlots( void );
static void PlayerSettings_BuildProfileItems( void );
static int PlayerSettings_FindProfileIndex( const char *name );
static qboolean PlayerSettings_SanitizeProfileName( const char *input, char *output, size_t size );
static void PlayerSettings_AddProfileSlot( const char *name );
static void PlayerSettings_RemoveProfileSlot( int index );
static void PlayerSettings_SetProfileCvars( const char *profile );
static void PlayerSettings_SelectProfileByIndex( int index );
static void PlayerSettings_UpdateLifetimeData( void );

static void PlayerSettings_SetMenuItemVisible( menucommon_s *item, qboolean visible ) {
        if ( !item ) {
                return;
        }

        if ( visible ) {
                item->flags &= ~QMF_HIDDEN;
        } else {
                item->flags |= QMF_HIDDEN;
        }
}

static qboolean PlayerSettings_SanitizeProfileName( const char *input, char *output, size_t size ) {
        size_t length;

        if ( !output || size == 0 ) {
                return qfalse;
        }

        output[0] = '\0';
        if ( !input ) {
                return qfalse;
        }

        length = 0;
        while ( *input && length + 1 < size ) {
                char c = *input++;

                if ( ( c >= 'a' && c <= 'z' ) ||
                     ( c >= 'A' && c <= 'Z' ) ||
                     ( c >= '0' && c <= '9' ) ) {
                        output[length++] = c;
                } else if ( c == '-' || c == '_' || c == '.' ) {
                        output[length++] = c;
                }
        }

        output[length] = '\0';
        return length > 0;
}

static int PlayerSettings_FindProfileIndex( const char *name ) {
        int i;

        if ( !name || !name[0] ) {
                return -1;
        }

        for ( i = 0; i < s_playersettings.profileCount; i++ ) {
                if ( !Q_stricmp( s_playersettings.profileNames[i], name ) ) {
                        return i;
                }
        }

        return -1;
}

static void PlayerSettings_BuildProfileItems( void ) {
        int i;

        for ( i = 0; i < s_playersettings.profileCount && i < MAX_PROFILE_SLOTS; i++ ) {
                s_playersettings.profileItems[i] = s_playersettings.profileNames[i];
        }

        s_playersettings.profileItems[s_playersettings.profileCount] = NULL;
        s_playersettings.profileList.itemnames = s_playersettings.profileItems;
        s_playersettings.profileList.numitems = s_playersettings.profileCount;

        if ( s_playersettings.profileList.curvalue >= s_playersettings.profileCount ) {
                if ( s_playersettings.profileCount > 0 ) {
                        s_playersettings.profileList.curvalue = s_playersettings.profileCount - 1;
                } else {
                        s_playersettings.profileList.curvalue = 0;
                }
        }
        s_playersettings.profileList.oldvalue = s_playersettings.profileList.curvalue;
}

static void PlayerSettings_AddProfileSlot( const char *name ) {
        char sanitized[MAX_PROFILE_NAME_LENGTH];

        if ( s_playersettings.profileCount >= MAX_PROFILE_SLOTS ) {
                return;
        }

        if ( !PlayerSettings_SanitizeProfileName( name, sanitized, sizeof( sanitized ) ) ) {
                return;
        }

        if ( PlayerSettings_FindProfileIndex( sanitized ) >= 0 ) {
                return;
        }

        Q_strncpyz( s_playersettings.profileNames[s_playersettings.profileCount], sanitized,
                    sizeof( s_playersettings.profileNames[0] ) );
        s_playersettings.profileCount++;
}

static void PlayerSettings_RemoveProfileSlot( int index ) {
        int i;

        if ( index < 0 || index >= s_playersettings.profileCount ) {
                return;
        }

        if ( !Q_stricmp( s_playersettings.profileNames[index], PROFILE_DEFAULT_SLOT ) ) {
                return;
        }

        for ( i = index; i < s_playersettings.profileCount - 1; i++ ) {
                Q_strncpyz( s_playersettings.profileNames[i], s_playersettings.profileNames[i + 1],
                            sizeof( s_playersettings.profileNames[i] ) );
        }

        if ( s_playersettings.profileCount > 0 ) {
                s_playersettings.profileCount--;
                s_playersettings.profileNames[s_playersettings.profileCount][0] = '\0';
        }
}

static void PlayerSettings_SaveProfileSlots( void ) {
        char buffer[MAX_STRING_CHARS];
        int i;

        buffer[0] = '\0';

        for ( i = 0; i < s_playersettings.profileCount; i++ ) {
                if ( buffer[0] ) {
                        Q_strcat( buffer, sizeof( buffer ), " " );
                }
                Q_strcat( buffer, sizeof( buffer ), s_playersettings.profileNames[i] );
        }

        trap_Cvar_Set( PROFILE_SLOTS_CVAR, buffer );
}

static void PlayerSettings_LoadProfileSlots( void ) {
        char buffer[MAX_STRING_CHARS];
        char *token;
        char currentProfileRaw[MAX_STRING_CHARS];
        char sanitizedCurrent[MAX_PROFILE_NAME_LENGTH];

        s_playersettings.profileCount = 0;

        trap_Cvar_VariableStringBuffer( PROFILE_SLOTS_CVAR, buffer, sizeof( buffer ) );
        token = strtok( buffer, " " );
        while ( token ) {
                PlayerSettings_AddProfileSlot( token );
                if ( s_playersettings.profileCount >= MAX_PROFILE_SLOTS ) {
                        break;
                }
                token = strtok( NULL, " " );
        }

        PlayerSettings_AddProfileSlot( PROFILE_DEFAULT_SLOT );

        trap_Cvar_VariableStringBuffer( "cg_profile", currentProfileRaw, sizeof( currentProfileRaw ) );
        if ( !PlayerSettings_SanitizeProfileName( currentProfileRaw, sanitizedCurrent, sizeof( sanitizedCurrent ) ) ) {
                Q_strncpyz( sanitizedCurrent, PROFILE_DEFAULT_SLOT, sizeof( sanitizedCurrent ) );
        }
        PlayerSettings_AddProfileSlot( sanitizedCurrent );

        PlayerSettings_BuildProfileItems();

        s_playersettings.profileList.curvalue = PlayerSettings_FindProfileIndex( sanitizedCurrent );
        if ( s_playersettings.profileList.curvalue < 0 ) {
                s_playersettings.profileList.curvalue = 0;
        }
        s_playersettings.profileList.oldvalue = s_playersettings.profileList.curvalue;

        Q_strncpyz( s_playersettings.profileName.field.buffer, sanitizedCurrent,
                    sizeof( s_playersettings.profileName.field.buffer ) );
}

static void PlayerSettings_SetProfileCvars( const char *profile ) {
        char sanitized[MAX_PROFILE_NAME_LENGTH];

        if ( !PlayerSettings_SanitizeProfileName( profile, sanitized, sizeof( sanitized ) ) ) {
                Q_strncpyz( sanitized, PROFILE_DEFAULT_SLOT, sizeof( sanitized ) );
        }

        trap_Cvar_Set( "cg_profile", sanitized );
        trap_Cvar_Set( "profile", sanitized );

        s_playersettings.lifetimeDisplay.valid = qfalse;
        s_playersettings.lifetimeDisplay.sequence = -1;
}

static void PlayerSettings_SelectProfileByIndex( int index ) {
        if ( index < 0 || index >= s_playersettings.profileCount ) {
                return;
        }

        PlayerSettings_SetProfileCvars( s_playersettings.profileNames[index] );
        s_playersettings.profileList.curvalue = index;
        s_playersettings.profileList.oldvalue = index;
        Q_strncpyz( s_playersettings.profileName.field.buffer, s_playersettings.profileNames[index],
                    sizeof( s_playersettings.profileName.field.buffer ) );
        PlayerSettings_SaveProfileSlots();
}

static void PlayerSettings_UpdateTabHighlight( void ) {
        int i;

        for ( i = 0; i < PLAYERSETTINGS_NUM_TABS; i++ ) {
                if ( s_playersettings.activeTab == i ) {
                        s_playersettings.tabLabels[i].color = text_color_highlight;
                } else {
                        s_playersettings.tabLabels[i].color = uis.text_color;
                }
        }
}

static void PlayerSettings_UpdateTabVisibility( void ) {
        int i;
        qboolean showCar;
        qboolean showStats;
        qboolean showAchievements;

        showCar = ( s_playersettings.activeTab == PLAYERSETTINGS_TAB_CAR );
        showStats = ( s_playersettings.activeTab == PLAYERSETTINGS_TAB_STATS );
        showAchievements = ( s_playersettings.activeTab == PLAYERSETTINGS_TAB_ACHIEVEMENTS );

        PlayerSettings_SetMenuItemVisible( &s_playersettings.name.generic, showCar );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.handicap.generic, showCar );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.effects.generic, showCar );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.customize.generic, showCar );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.plate.generic, showCar );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.left.generic, showCar );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.modelname.generic, showCar );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.right.generic, showCar );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.player.generic, showCar );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.favorites.generic, showCar );
        for ( i = 0; i < NUM_FAVORITES; i++ ) {
                PlayerSettings_SetMenuItemVisible( &s_playersettings.favpics[i].generic, showCar );
                PlayerSettings_SetMenuItemVisible( &s_playersettings.favpicbuttons[i].generic, showCar );
                PlayerSettings_SetMenuItemVisible( &s_playersettings.ports[i].generic, showCar );
        }

        PlayerSettings_SetMenuItemVisible( &s_playersettings.statsPanel.generic, showStats );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.profileList.generic, showStats );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.profileName.generic, showStats );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.profileNameLabel.generic, showStats );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.profileCreate.generic, showStats );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.profileDelete.generic, showStats );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.profileSelect.generic, showStats );

        PlayerSettings_SetMenuItemVisible( &s_playersettings.achievementsPanel.generic, showAchievements );
}

static void PlayerSettings_SetActiveTab( int tab ) {
        if ( tab < 0 || tab >= PLAYERSETTINGS_NUM_TABS ) {
                tab = PLAYERSETTINGS_TAB_CAR;
        }

        if ( s_playersettings.activeTab == tab ) {
                return;
        }

        s_playersettings.activeTab = tab;
        PlayerSettings_UpdateTabVisibility();
        PlayerSettings_UpdateTabHighlight();
}

static void PlayerSettings_DrawTabButton( void *self ) {
        menubitmap_s *b;
        int tabIndex;
        vec4_t activeColor = { 0.18f, 0.18f, 0.18f, 0.85f };
        vec4_t inactiveColor = { 0.05f, 0.05f, 0.05f, 0.65f };
        vec4_t borderColor = { 0.3f, 0.3f, 0.3f, 0.9f };

        b = (menubitmap_s *)self;

        switch ( b->generic.id ) {
        default:
        case ID_TAB_CAR:
                tabIndex = PLAYERSETTINGS_TAB_CAR;
                break;
        case ID_TAB_STATS:
                tabIndex = PLAYERSETTINGS_TAB_STATS;
                break;
        case ID_TAB_ACHIEVEMENTS:
                tabIndex = PLAYERSETTINGS_TAB_ACHIEVEMENTS;
                break;
        }

        if ( s_playersettings.activeTab != tabIndex ) {
                UI_FillRect( b->generic.x, b->generic.y, b->width, b->height, inactiveColor );
        } else {
                UI_FillRect( b->generic.x, b->generic.y, b->width, b->height, activeColor );
        }
        UI_DrawRect( b->generic.x, b->generic.y, b->width, b->height, borderColor );
}

static void PlayerSettings_UpdateLifetimeData( void ) {
        playerLifetimeDisplay_t *display;
        char buffer[64];
        int sequence;

        display = &s_playersettings.lifetimeDisplay;

        sequence = (int)trap_Cvar_VariableValue( "ui_profile_sequence" );
        if ( display->valid && display->sequence == sequence ) {
                return;
        }

        display->sequence = sequence;
        display->valid = qfalse;

        display->raw.version = (int)trap_Cvar_VariableValue( "ui_profile_version" );
        display->raw.matchesPlayed = (int)trap_Cvar_VariableValue( "ui_profile_matches" );
        display->raw.wins = (int)trap_Cvar_VariableValue( "ui_profile_wins" );
        display->raw.losses = (int)trap_Cvar_VariableValue( "ui_profile_losses" );
        display->raw.finishes = (int)trap_Cvar_VariableValue( "ui_profile_finishes" );
        display->raw.dnfs = (int)trap_Cvar_VariableValue( "ui_profile_dnfs" );
        display->raw.bestPosition = (int)trap_Cvar_VariableValue( "ui_profile_bestPosition" );
        display->raw.bestLapMs = (int)trap_Cvar_VariableValue( "ui_profile_bestLapMs" );
        display->raw.bestTotalRaceMs = (int)trap_Cvar_VariableValue( "ui_profile_bestTotalRaceMs" );
        display->raw.totalRaceTimeMs = (int)trap_Cvar_VariableValue( "ui_profile_totalRaceTimeMs" );
        display->raw.totalScore = (int)trap_Cvar_VariableValue( "ui_profile_totalScore" );
        display->raw.totalKills = (int)trap_Cvar_VariableValue( "ui_profile_totalKills" );
        display->raw.totalDeaths = (int)trap_Cvar_VariableValue( "ui_profile_totalDeaths" );
        display->raw.totalDamageDealt = (int)trap_Cvar_VariableValue( "ui_profile_totalDamageDealt" );
        display->raw.totalDamageTaken = (int)trap_Cvar_VariableValue( "ui_profile_totalDamageTaken" );
        display->raw.achievements = (int)trap_Cvar_VariableValue( "ui_profile_achievements" );

        trap_Cvar_VariableStringBuffer( "ui_profile_totalDistance", buffer, sizeof( buffer ) );
        display->totalDistanceMeters = atof( buffer );

        trap_Cvar_VariableStringBuffer( "ui_profile_totalFuel", buffer, sizeof( buffer ) );
        display->totalFuelConsumed = atof( buffer );

        if ( display->raw.version > 0 ) {
                display->valid = qtrue;
        }
}

static void PlayerSettings_DrawStatsPanel( void *self ) {
        menucommon_s *item;
        playerLifetimeDisplay_t *display;
        float x;
        float y;
        char buffer[64];
        float distanceKm;
        float fuelConsumed;
        int minutes;
        int seconds;
        int millis;

        if ( s_playersettings.activeTab != PLAYERSETTINGS_TAB_STATS ) {
                return;
        }

        PlayerSettings_UpdateLifetimeData();

        item = (menucommon_s *)self;
        display = &s_playersettings.lifetimeDisplay;

        x = item->x;
        y = item->y;

        if ( !display->valid ) {
                UI_DrawString( x, y, "Lifetime profile data unavailable", UI_LEFT | UI_SMALLFONT, text_color_disabled );
                return;
        }

        distanceKm = display->totalDistanceMeters / 1000.0f;
        fuelConsumed = display->totalFuelConsumed;

        Com_sprintf( buffer, sizeof( buffer ), "Total distance: %.2f km", distanceKm );
        UI_DrawString( x, y, buffer, UI_LEFT | UI_SMALLFONT, uis.text_color );
        y += SMALLCHAR_HEIGHT + 2;

        Com_sprintf( buffer, sizeof( buffer ), "Total fuel used: %.2f L", fuelConsumed );
        UI_DrawString( x, y, buffer, UI_LEFT | UI_SMALLFONT, uis.text_color );
        y += SMALLCHAR_HEIGHT + 8;

        Com_sprintf( buffer, sizeof( buffer ), "Matches played: %d", display->raw.matchesPlayed );
        UI_DrawString( x, y, buffer, UI_LEFT | UI_SMALLFONT, uis.text_color );
        y += SMALLCHAR_HEIGHT + 2;

        Com_sprintf( buffer, sizeof( buffer ), "Wins / Losses: %d / %d", display->raw.wins, display->raw.losses );
        UI_DrawString( x, y, buffer, UI_LEFT | UI_SMALLFONT, uis.text_color );
        y += SMALLCHAR_HEIGHT + 2;

        Com_sprintf( buffer, sizeof( buffer ), "Finishes / DNFs: %d / %d", display->raw.finishes, display->raw.dnfs );
        UI_DrawString( x, y, buffer, UI_LEFT | UI_SMALLFONT, uis.text_color );
        y += SMALLCHAR_HEIGHT + 8;

        if ( display->raw.bestPosition > 0 ) {
                Com_sprintf( buffer, sizeof( buffer ), "Best position: #%d", display->raw.bestPosition );
        } else {
                Q_strncpyz( buffer, "Best position: --", sizeof( buffer ) );
        }
        UI_DrawString( x, y, buffer, UI_LEFT | UI_SMALLFONT, uis.text_color );
        y += SMALLCHAR_HEIGHT + 2;

        if ( display->raw.bestLapMs > 0 ) {
                minutes = display->raw.bestLapMs / 60000;
                seconds = ( display->raw.bestLapMs % 60000 ) / 1000;
                millis = display->raw.bestLapMs % 1000;
                Com_sprintf( buffer, sizeof( buffer ), "Best lap: %02d:%02d.%03d", minutes, seconds, millis );
        } else {
                Q_strncpyz( buffer, "Best lap: --", sizeof( buffer ) );
        }
        UI_DrawString( x, y, buffer, UI_LEFT | UI_SMALLFONT, uis.text_color );
        y += SMALLCHAR_HEIGHT + 2;

        if ( display->raw.bestTotalRaceMs > 0 ) {
                minutes = display->raw.bestTotalRaceMs / 60000;
                seconds = ( display->raw.bestTotalRaceMs % 60000 ) / 1000;
                millis = display->raw.bestTotalRaceMs % 1000;
                Com_sprintf( buffer, sizeof( buffer ), "Best race: %02d:%02d.%03d", minutes, seconds, millis );
        } else {
                Q_strncpyz( buffer, "Best race: --", sizeof( buffer ) );
        }
        UI_DrawString( x, y, buffer, UI_LEFT | UI_SMALLFONT, uis.text_color );
        y += SMALLCHAR_HEIGHT + 2;

        if ( display->raw.totalRaceTimeMs > 0 ) {
                int hours = display->raw.totalRaceTimeMs / 3600000;
                minutes = ( display->raw.totalRaceTimeMs % 3600000 ) / 60000;
                seconds = ( display->raw.totalRaceTimeMs % 60000 ) / 1000;
                Com_sprintf( buffer, sizeof( buffer ), "Total race time: %d:%02d:%02d", hours, minutes, seconds );
        } else {
                Q_strncpyz( buffer, "Total race time: --", sizeof( buffer ) );
        }
        UI_DrawString( x, y, buffer, UI_LEFT | UI_SMALLFONT, uis.text_color );
}

static void PlayerSettings_DrawAchievementsPanel( void *self ) {
        menucommon_s *item;
        playerLifetimeDisplay_t *display;
        float x;
        float y;
        int i;
        static const struct {
                int bit;
                const char *label;
        } achievementMap[] = {
                { PROFILE_ACHIEVEMENT_DISTANCE_100KM, "Road Trip (100 km)" },
                { PROFILE_ACHIEVEMENT_DISTANCE_500KM, "Long Haul (500 km)" },
                { PROFILE_ACHIEVEMENT_MATCHES_10,     "Rookie Driver (10 races)" },
                { PROFILE_ACHIEVEMENT_MATCHES_50,     "Endurance Racer (50 races)" }
        };

        if ( s_playersettings.activeTab != PLAYERSETTINGS_TAB_ACHIEVEMENTS ) {
                return;
        }

        PlayerSettings_UpdateLifetimeData();

        item = (menucommon_s *)self;
        display = &s_playersettings.lifetimeDisplay;

        x = item->x;
        y = item->y;

        if ( !display->valid ) {
                UI_DrawString( x, y, "Achievement data unavailable", UI_LEFT | UI_SMALLFONT, text_color_disabled );
                return;
        }

        for ( i = 0; i < ARRAY_LEN( achievementMap ); i++ ) {
                const float *color;
                if ( display->raw.achievements & achievementMap[i].bit ) {
                        color = text_color_highlight;
                } else {
                        color = uis.text_color;
                }
                UI_DrawString( x, y, achievementMap[i].label, UI_LEFT | UI_SMALLFONT, color );
                y += SMALLCHAR_HEIGHT + 4;
        }
}


/*
=================
PlayerSettings_DrawName
=================
*/
static void PlayerSettings_DrawName( void *self ) {
	menufield_s		*f;
	qboolean		focus;
	int				style;
	char			*txt;
	char			c;
	float			*color;
	int				n;
	int				basex, x, y;
// STONELANCE
//	char			name[32];
// END

	f = (menufield_s*)self;
	basex = f->generic.x;
	y = f->generic.y;
	focus = (f->generic.parent->cursor == f->generic.menuPosition);

	style = UI_LEFT|UI_SMALLFONT;
// STONELANCE
/*
	color = text_color_normal;
	if( focus ) {
		style |= UI_PULSE;
		color = text_color_highlight;
	}
	UI_DrawProportionalString( basex, y, "Name", style, color );
*/
	color = uis.text_color;
	if( focus ) {
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	UI_DrawProportionalString( basex + 16, y, "Name", style, color );
// END

	// draw the actual name
	basex += 64;
// STONELANCE
//	y += PROP_HEIGHT;
	y += 18;
// END
	txt = f->field.buffer;
// STONELANCE
//	color = g_color_table[ColorIndex(COLOR_WHITE)];
// END
	x = basex;
	while ( (c = *txt) != 0 ) {
		if ( !focus && Q_IsColorString( txt ) ) {
			n = ColorIndex( *(txt+1) );
			if( n == 0 ) {
				n = 7;
			}
			color = g_color_table[n];
			txt += 2;
			continue;
		}
		UI_DrawChar( x, y, c, style, color );
		txt++;
		x += SMALLCHAR_WIDTH;
	}

	// draw cursor if we have focus
	if( focus ) {
		if ( trap_Key_GetOverstrikeMode() ) {
			c = 11;
		} else {
			c = 10;
		}

		style &= ~UI_PULSE;
		style |= UI_BLINK;

		UI_DrawChar( basex + f->field.cursor * SMALLCHAR_WIDTH, y, c, style, color_white );
	}

// STONELANCE
/*
	// draw at bottom also using proportional font
	Q_strncpyz( name, f->field.buffer, sizeof(name) );
	Q_CleanStr( name );
	UI_DrawProportionalString( 320, 440, name, UI_CENTER|UI_BIGFONT, text_color_normal );
*/
// END
}


/*
=================
PlayerSettings_DrawHandicap
=================
*/
static void PlayerSettings_DrawHandicap( void *self ) {
	menulist_s		*item;
	qboolean		focus;
	int				style;
	float			*color;

	item = (menulist_s *)self;
	focus = (item->generic.parent->cursor == item->generic.menuPosition);

	style = UI_LEFT|UI_SMALLFONT;
// STONELANCE
/*
	color = text_color_normal;
	if( focus ) {
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	UI_DrawProportionalString( item->generic.x, item->generic.y, "Handicap", style, color );
	UI_DrawProportionalString( item->generic.x + 64, item->generic.y + PROP_HEIGHT, handicap_items[item->curvalue], style, color );
*/
	color = uis.text_color;
	if( focus && !(uis.transitionIn || uis.transitionOut)) {
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	UI_DrawProportionalString( item->generic.x + 16, item->generic.y, "Handicap", style, color );
	UI_DrawString( item->generic.x + 64, item->generic.y + 18, handicap_items[item->curvalue], style, color );
// END
}


/*
=================
PlayerSettings_DrawEffects
=================
*/
static void PlayerSettings_DrawEffects( void *self ) {
	menulist_s		*item;
	qboolean		focus;
	int				style;
	float			*color;

	item = (menulist_s *)self;
	focus = (item->generic.parent->cursor == item->generic.menuPosition);

	style = UI_LEFT|UI_SMALLFONT;
// STONELANCE
/*
	color = text_color_normal;
	if( focus ) {
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	UI_DrawProportionalString( item->generic.x, item->generic.y, "Effects", style, color );

	UI_DrawHandlePic( item->generic.x + 64, item->generic.y + PROP_HEIGHT + 8, 128, 8, s_playersettings.fxBasePic );
	UI_DrawHandlePic( item->generic.x + 64 + item->curvalue * 16 + 8, item->generic.y + PROP_HEIGHT + 6, 16, 12, s_playersettings.fxPic[item->curvalue] );
*/

	color = uis.text_color;
	if( focus && !(uis.transitionIn || uis.transitionOut)) {
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	UI_DrawProportionalString( item->generic.x + 16, item->generic.y, "Effects", style, color );

	UI_DrawHandlePic( item->generic.x + 18, item->generic.y + 20, 128, 16, s_playersettings.fxBasePic );
	UI_DrawHandlePic( item->generic.x + 23 + item->curvalue * 17, item->generic.y + 20, 16, 16, s_playersettings.fxPic[item->curvalue] );
// END
}


// STONELANCE
/*
=================
PlayerSettings_DrawCustomize
=================
*/
static void PlayerSettings_DrawCustomize( void *self ) {
	menulist_s		*item;
	qboolean		focus;
	int				style;
	float			*color;

	item = (menulist_s *)self;
	focus = (item->generic.parent->cursor == item->generic.menuPosition);

	style = UI_RIGHT | UI_SMALLFONT;
	color = uis.text_color;
	if( focus && !(uis.transitionIn || uis.transitionOut)) {
		style |= UI_PULSE;
		color = text_color_highlight;
	}

	UI_DrawProportionalString( item->generic.x, item->generic.y, "CUSTOMIZE", style, color );
	UI_DrawProportionalString( item->generic.x, item->generic.y + 20, "THIS CAR >", style, color );
}


/*
=================
PlayerSettings_DrawBackShaders
=================
*/
static void PlayerSettings_DrawBackShaders( void ) {
	vec4_t	color;

	Vector4Copy(menu_back_color, color);
	color[3] *= uis.tFrac;

	UI_FillRect( 24, 80, 592, 48, color);
	UI_FillRect( 124, 138, 392, 32, menu_back_color);

	Menu_Draw( &s_playersettings.menu );
}


/*
=================
PlayerSettings_UpdateModel
=================
*/
static void PlayerSettings_UpdateModel( void )
{
	vec3_t	viewangles;
	vec3_t	moveangles;
	char	plate[MAX_QPATH];

	memset( &s_playersettings.playerinfo, 0, sizeof(playerInfo_t) );
	
	VectorClear( viewangles );
	VectorClear( moveangles );

	trap_Cvar_VariableStringBuffer( "plate", plate, sizeof( plate ) );
	UI_PlayerInfo_SetModel( &s_playersettings.playerinfo, s_playersettings.modelskin, s_playersettings.rimskin, s_playersettings.headskin, plate);
	UI_PlayerInfo_SetInfo( &s_playersettings.playerinfo, LEGS_IDLE, TORSO_STAND, viewangles, moveangles, WP_NONE, qfalse );
}
// END


/*
=================
PlayerSettings_DrawPlayer
=================
*/
static void PlayerSettings_DrawPlayer( void *self ) {
	menubitmap_s	*b;
// STONELANCE
/*
	vec3_t			viewangles;
	char			buf[MAX_QPATH];

	trap_Cvar_VariableStringBuffer( "model", buf, sizeof( buf ) );
	if ( strcmp( buf, s_playersettings.playerModel ) != 0 ) {
		UI_PlayerInfo_SetModel( &s_playersettings.playerinfo, buf );
		strcpy( s_playersettings.playerModel, buf );

		viewangles[YAW]   = 180 - 30;
		viewangles[PITCH] = 0;
		viewangles[ROLL]  = 0;
		UI_PlayerInfo_SetInfo( &s_playersettings.playerinfo, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MACHINEGUN, qfalse );
	}
*/
// END
	b = (menubitmap_s*) self;
	UI_DrawPlayer( b->generic.x, b->generic.y, b->width, b->height, &s_playersettings.playerinfo, uis.realtime );
}


// STONELANCE (new function)
/*
=================
LoadFavorite

=================
*/
static void LoadFavorite( const char *favorite ) {
	char		modelName[MAX_QPATH];
	char		skinName[MAX_QPATH];
	char		rimName[MAX_QPATH];
	char		headName[MAX_QPATH];
	int			i;
	qboolean	carFound;

	GetValuesFromFavorite(favorite, modelName, skinName, rimName, headName);

	// find model in our list
	carFound = qfalse;
	for (i = 0; i < s_playersettings.allModels; i++)
	{
		if (!Q_stricmp( modelName, s_playersettings.modelList[i] ))
		{
			// found pic, set selection here
			s_playersettings.selectedModel = i;
			s_playersettings.modelname.string = s_playersettings.modelList[s_playersettings.selectedModel];
			carFound = qtrue;
			break;
		}
	}

	if (!carFound){
		s_playersettings.selectedModel = 0;

		// get model
		Com_sprintf(s_playersettings.modelskin, sizeof(s_playersettings.modelskin), "%s/%s", s_playersettings.modelList[s_playersettings.selectedModel], DEFAULT_SKIN);

		s_playersettings.modelname.string = s_playersettings.modelList[s_playersettings.selectedModel];

		// FIXME: check to see if these exist
		Q_strncpyz(s_playersettings.rimskin, DEFAULT_RIM, sizeof(s_playersettings.rimskin));
		Q_strncpyz(s_playersettings.headskin, DEFAULT_HEAD, sizeof(s_playersettings.headskin));

		s_playersettings.modelChanged = qtrue;
	}
	else {
		Com_sprintf(s_playersettings.modelskin, sizeof(s_playersettings.modelskin), "%s/%s", modelName, skinName);
		Q_strncpyz(s_playersettings.rimskin, rimName, sizeof(s_playersettings.rimskin));
		Q_strncpyz(s_playersettings.headskin, headName, sizeof(s_playersettings.headskin));

		trap_Cvar_Set( "model", s_playersettings.modelskin );
		trap_Cvar_Set( "rim", rimName );
		trap_Cvar_Set( "head", headName );

		s_playersettings.modelChanged = qtrue;
	}
}

/*
=================
PlayerSettings_UpdateFavorites

=================
*/
static void PlayerSettings_UpdateFavorites( void ) {
	int			i;
	char		buf[MAX_QPATH];
	char		modelName[MAX_QPATH];
	char		skinName[MAX_QPATH];
	qboolean	error;
	
	for (i=0; i < NUM_FAVORITES; i++){
		Com_sprintf(buf, sizeof(buf), "favoritecar%i", (i+1));
		error = GetValuesFromFavorite(buf, modelName, skinName, NULL, NULL);

		if (!error){
			Com_sprintf(s_playersettings.favIcons[i], sizeof(s_playersettings.favIcons[i]), "models/players/%s/icon_%s", modelName, skinName);
			s_playersettings.favpics[i].generic.name = s_playersettings.favIcons[i];
			s_playersettings.favpicbuttons[i].generic.flags &= ~QMF_INACTIVE;
		}
		else{
			s_playersettings.favpics[i].generic.name = NULL;
			s_playersettings.favpicbuttons[i].generic.flags |= QMF_INACTIVE;
		}

		s_playersettings.favpics[i].shader = 0;
	}
}

/*
=================
PlayerSettings_Update
=================
*/
void PlayerSettings_Update( void ){
	trap_Cvar_VariableStringBuffer( "rim", s_playersettings.rimskin, sizeof( s_playersettings.rimskin ) );
	trap_Cvar_VariableStringBuffer( "head", s_playersettings.headskin, sizeof( s_playersettings.headskin ) );
	trap_Cvar_VariableStringBuffer( "model", s_playersettings.modelskin, sizeof( s_playersettings.modelskin ) );
	
	PlayerSettings_UpdateFavorites();
	PlayerSettings_UpdateModel();
}
// END


/*
=================
PlayerSettings_SaveChanges
=================
*/
static void PlayerSettings_SaveChanges( void ) {
	// name
	trap_Cvar_Set( "name", s_playersettings.name.field.buffer );

// STONELANCE
	if (s_playersettings.modelChanged){
		trap_Cvar_Set( "model", s_playersettings.modelskin );
	}
// END

	// handicap
	trap_Cvar_SetValue( "handicap", 100 - s_playersettings.handicap.curvalue * 5 );

	// effects color
	trap_Cvar_SetValue( "color1", uitogamecode[s_playersettings.effects.curvalue] );
}


/*
=================
PlayerSettings_MenuKey
=================
*/
static sfxHandle_t PlayerSettings_MenuKey( int key ) {
	if( key == K_MOUSE2 || key == K_ESCAPE ) {
// STONELANCE
//		PlayerSettings_SaveChanges();
		s_playersettings.menu.transitionMenu = ID_BACK;
		uis.transitionOut = uis.realtime;
		return 0;
// END
	}
	return Menu_DefaultKey( &s_playersettings.menu, key );
}


/*
=================
PlayerSettings_SetMenuItems
=================
*/
static void PlayerSettings_SetMenuItems( void ) {
//	vec3_t	viewangles;
	int		c;
	int		h;

// STONELANCE
	int			i;
	char		modelName[MAX_QPATH];
	char		*slash;
	qboolean	carFound;

	trap_Cvar_VariableStringBuffer( "rim", s_playersettings.rimskin, sizeof( s_playersettings.rimskin ) );
	trap_Cvar_VariableStringBuffer( "head", s_playersettings.headskin, sizeof( s_playersettings.headskin ) );
	trap_Cvar_VariableStringBuffer( "model", s_playersettings.modelskin, sizeof( s_playersettings.modelskin ) );
// END

	// name
	Q_strncpyz( s_playersettings.name.field.buffer, UI_Cvar_VariableString("name"), sizeof(s_playersettings.name.field.buffer) );

	// effects color
	c = trap_Cvar_VariableValue( "color1" ) - 1;
	if( c < 0 || c > 6 ) {
		c = 6;
	}
	s_playersettings.effects.curvalue = gamecodetoui[c];

	// model/skin
	memset( &s_playersettings.playerinfo, 0, sizeof(playerInfo_t) );
/*
	viewangles[YAW]   = 180 - 30;
	viewangles[PITCH] = 0;
	viewangles[ROLL]  = 0;
*/
// STONELANCE
	Q_strncpyz( modelName, s_playersettings.modelskin, sizeof( modelName ) );
	slash = strchr( modelName, '/' );
	if ( slash ) {
		*slash = 0;
	}

	s_playersettings.modelChanged = qfalse;

	// find model in our list
	carFound = qfalse;
	for (i = 0; i < s_playersettings.allModels; i++)
	{
		if (!Q_stricmp( modelName, s_playersettings.modelList[i] )){
			// found pic, set selection here
			s_playersettings.selectedModel = i;
			s_playersettings.modelname.string = s_playersettings.modelList[s_playersettings.selectedModel];
			carFound = qtrue;
			break;
		}
	}

	if (!carFound){
		s_playersettings.selectedModel = 0;

		// get model
		Com_sprintf( s_playersettings.modelskin, sizeof(s_playersettings.modelskin), "%s/%s", s_playersettings.modelList[s_playersettings.selectedModel], DEFAULT_SKIN);
		s_playersettings.modelname.string = s_playersettings.modelList[s_playersettings.selectedModel];
		s_playersettings.modelChanged = qtrue;
	}

/*
	UI_PlayerInfo_SetModel( &s_playersettings.playerinfo, UI_Cvar_VariableString( "model" ) );
	UI_PlayerInfo_SetInfo( &s_playersettings.playerinfo, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MACHINEGUN, qfalse );
*/

	PlayerSettings_UpdateModel();
	PlayerSettings_UpdateFavorites();
// END

	// handicap
	h = Com_Clamp( 5, 100, trap_Cvar_VariableValue("handicap") );
	s_playersettings.handicap.curvalue = 20 - h / 5;

	PlayerSettings_LoadProfileSlots();
	PlayerSettings_UpdateTabHighlight();
	PlayerSettings_UpdateTabVisibility();
}


// STONELANCE
/*
=================
PlayerSettings_PicEvent
=================
*/
static void PlayerSettings_PicEvent( void* ptr, int event )
{
	if (event != QM_ACTIVATED)
		return;

	switch(((menucommon_s*)ptr)->id){
	case ID_FAVORITE1:
		LoadFavorite("favoritecar1");
		break;

	case ID_FAVORITE2:
		LoadFavorite("favoritecar2");
		break;

	case ID_FAVORITE3:
		LoadFavorite("favoritecar3");
		break;

	case ID_FAVORITE4:
		LoadFavorite("favoritecar4");
		break;
	}

	PlayerSettings_UpdateModel();
}
// END


/*
=================
PlayerSettings_MenuEvent
=================
*/
static void PlayerSettings_MenuEvent( void* ptr, int event ) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_HANDICAP:
		trap_Cvar_Set( "handicap", va( "%i", 100 - 25 * s_playersettings.handicap.curvalue ) );
		break;

// STONELANCE
/*
	case ID_MODEL:
		PlayerSettings_SaveChanges();
		UI_PlayerModelMenu();
		break;

	case ID_BACK:
		PlayerSettings_SaveChanges();
		UI_PopMenu();
		break;
*/

	case ID_CUSTOMIZE:
	case ID_BACK:
		s_playersettings.menu.transitionMenu = ((menucommon_s*)ptr)->id;
		uis.transitionOut = uis.realtime;
		break;

	case ID_TAB_CAR:
		PlayerSettings_SetActiveTab( PLAYERSETTINGS_TAB_CAR );
		break;

	case ID_TAB_STATS:
		PlayerSettings_SetActiveTab( PLAYERSETTINGS_TAB_STATS );
		break;

	case ID_TAB_ACHIEVEMENTS:
		PlayerSettings_SetActiveTab( PLAYERSETTINGS_TAB_ACHIEVEMENTS );
		break;

	case ID_PROFILE_LIST:
		if ( s_playersettings.profileList.curvalue >= 0 &&
		     s_playersettings.profileList.curvalue < s_playersettings.profileCount ) {
			Q_strncpyz( s_playersettings.profileName.field.buffer,
			        s_playersettings.profileNames[s_playersettings.profileList.curvalue],
			        sizeof( s_playersettings.profileName.field.buffer ) );
		}
		break;

	case ID_PROFILE_CREATE:
	{
		int newIndex;
		char sanitized[MAX_PROFILE_NAME_LENGTH];

		if ( PlayerSettings_SanitizeProfileName( s_playersettings.profileName.field.buffer,
		        sanitized, sizeof( sanitized ) ) ) {
			PlayerSettings_AddProfileSlot( sanitized );
			PlayerSettings_BuildProfileItems();
			PlayerSettings_SaveProfileSlots();
			newIndex = PlayerSettings_FindProfileIndex( sanitized );
			if ( newIndex >= 0 ) {
				PlayerSettings_SelectProfileByIndex( newIndex );
			}
		}
	}
		break;

	case ID_PROFILE_DELETE:
	{
		int index = s_playersettings.profileList.curvalue;
		PlayerSettings_RemoveProfileSlot( index );
		PlayerSettings_BuildProfileItems();
		PlayerSettings_SaveProfileSlots();
		if ( s_playersettings.profileCount <= 0 ) {
			PlayerSettings_LoadProfileSlots();
		} else {
			if ( s_playersettings.profileList.curvalue >= s_playersettings.profileCount ) {
				s_playersettings.profileList.curvalue = s_playersettings.profileCount - 1;
			}
			if ( s_playersettings.profileList.curvalue < 0 ) {
				s_playersettings.profileList.curvalue = 0;
			}
			PlayerSettings_SelectProfileByIndex( s_playersettings.profileList.curvalue );
		}
	}
		break;

	case ID_PROFILE_SELECT:
		PlayerSettings_SelectProfileByIndex( s_playersettings.profileList.curvalue );
		break;

	case ID_PLATE:
		UI_PlateSelectionMenu();
		break;

	case ID_LEFT:
		//Com_Printf("Clicked car selection LEFT\n");
		if (s_playersettings.selectedModel > 0)
		{
			s_playersettings.selectedModel--;

			//Com_Printf("PS: Car selected, %i\n", s_playersettings.selectedmodel);

			// get model
			Com_sprintf(s_playersettings.modelskin, sizeof(s_playersettings.modelskin), "%s/%s", s_playersettings.modelList[s_playersettings.selectedModel], DEFAULT_SKIN);

			//Com_Printf("PS: modelskin set to: %s\n", s_playersettings.modelskin);

			s_playersettings.modelname.string = s_playersettings.modelList[s_playersettings.selectedModel];

			//Com_Printf("PS: modelname set to: %s\n", s_playersettings.modelname.string);

			s_playersettings.modelChanged = qtrue;

			PlayerSettings_UpdateModel();
		}
		break;

	case ID_RIGHT:
		//Com_Printf("Clicked car selection RIGHT\n");
		if (s_playersettings.selectedModel < s_playersettings.numModels - 1 )
		{
			s_playersettings.selectedModel++;

			//Com_Printf("PS: Car selected, %i\n", s_playersettings.selectedmodel);

			// get model
			Com_sprintf(s_playersettings.modelskin, sizeof(s_playersettings.modelskin), "%s/%s", s_playersettings.modelList[s_playersettings.selectedModel], DEFAULT_SKIN);

			//Com_Printf("PS: modelskin set to: %s\n", s_playersettings.modelskin);

			s_playersettings.modelname.string = s_playersettings.modelList[s_playersettings.selectedModel];

			//Com_Printf("PS: modelname set to: %s\n", s_playersettings.modelname.string);

			s_playersettings.modelChanged = qtrue;

			PlayerSettings_UpdateModel();
		}
		break;
// END
	}
}


// STONELANCE
/*
=================
PlayerSettigns_ChangeMenu
=================
*/
void PlayerSettigns_ChangeMenu( int menuID ){

	switch(menuID){
	case ID_CUSTOMIZE:
		PlayerSettings_SaveChanges();
		s_playersettings.modelChanged = qfalse;
		UI_PlayerModelMenu( s_playersettings.modelname.string );
		break;

	case ID_BACK:
		PlayerSettings_SaveChanges();
		s_playersettings.modelChanged = qfalse;
//		uis.transitionIn = uis.realtime;
		UI_PopMenu();
		break;
	}
}


/*
=================
PlayerSettings_RunTransition
=================
*/
void PlayerSettings_RunTransition(float frac){
	int		i, y;

	uis.text_color[0] = text_color_normal[0];
	uis.text_color[1] = text_color_normal[1];
	uis.text_color[2] = text_color_normal[2];
	uis.text_color[3] = text_color_normal[3] * frac;

	s_playersettings.banner.color = uis.text_color;

	s_playersettings.customize.color = uis.text_color;
	s_playersettings.favorites.color = uis.text_color;
	s_playersettings.modelname.color = uis.text_color;
	s_playersettings.plate.color = uis.text_color;

	if (s_playersettings.menu.transitionMenu != ID_CUSTOMIZE){
		y = 403 + (int)(77 * (1 - frac));
		for (i=0; i<NUM_FAVORITES; i++){
			s_playersettings.ports[i].generic.y = y;
			s_playersettings.favpics[i].generic.y = y;
			s_playersettings.favpicbuttons[i].generic.y = y;
		}
	}
}


/*
=================
PlayerSettings_BuildList
=================
*/
static void PlayerSettings_BuildList( void ){
	// get car list
	s_playersettings.numModels = UI_BuildFileList("models/players", "md3", "body", qtrue, qtrue, BL_EXCLUDE, 0, s_playersettings.modelList);
	s_playersettings.allModels = UI_BuildFileList("models/players", "md3", "body", qtrue, qtrue, BL_ONLY, s_playersettings.numModels, s_playersettings.modelList);
}
// END


/*
=================
PlayerSettings_MenuInit
=================
*/
static void PlayerSettings_MenuInit( void ) {
	int		y;
// STONELANCE
	int		i, j, x;
	static char	modelname[32];
// END

	memset(&s_playersettings,0,sizeof(playersettings_t));
	s_playersettings.activeTab = -1;

	PlayerSettings_Cache();

	s_playersettings.menu.key        = PlayerSettings_MenuKey;
	s_playersettings.menu.wrapAround = qtrue;
	s_playersettings.menu.fullscreen = qtrue;
// STONELANCE
	s_playersettings.menu.draw		 = PlayerSettings_DrawBackShaders;
	s_playersettings.menu.transition = PlayerSettings_RunTransition;
	s_playersettings.menu.changeMenu = PlayerSettigns_ChangeMenu;
// END

	s_playersettings.banner.generic.type  = MTYPE_BTEXT;
	s_playersettings.banner.generic.x     = 320;
// STONELANCE
	s_playersettings.banner.generic.y     = 17;
// END
	s_playersettings.banner.string        = "PLAYER SETTINGS";
// STONELANCE
	s_playersettings.banner.color         = text_color_normal;
// END
	s_playersettings.banner.style         = UI_CENTER;

	{
		static const int tabIds[PLAYERSETTINGS_NUM_TABS] = { ID_TAB_CAR, ID_TAB_STATS, ID_TAB_ACHIEVEMENTS };
		static const char *tabTexts[PLAYERSETTINGS_NUM_TABS] = { "CAR", "STATS", "ACHIEVEMENTS" };
		int tab;
		int tabX = 64;
		int tabY = 64;
		int tabWidth = 160;
		int tabHeight = 28;

		for ( tab = 0; tab < PLAYERSETTINGS_NUM_TABS; tab++ ) {
			menubitmap_s *button = &s_playersettings.tabButtons[tab];
			menutext_s *label = &s_playersettings.tabLabels[tab];
			int x = tabX + tab * ( tabWidth + 8 );

			button->generic.type = MTYPE_BITMAP;
			button->generic.flags = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
			button->generic.id = tabIds[tab];
			button->generic.callback = PlayerSettings_MenuEvent;
			button->generic.ownerdraw = PlayerSettings_DrawTabButton;
			button->generic.x = x;
			button->generic.y = tabY;
			button->generic.left = x;
			button->generic.top = tabY;
			button->generic.right = x + tabWidth;
			button->generic.bottom = tabY + tabHeight;
			button->width = tabWidth;
			button->height = tabHeight;

			label->generic.type = MTYPE_PTEXT;
			label->generic.flags = QMF_CENTER_JUSTIFY|QMF_INACTIVE;
			label->generic.x = x + tabWidth / 2;
			label->generic.y = tabY + 6;
			label->generic.id = tabIds[tab];
			label->string = tabTexts[tab];
			label->style = UI_CENTER|UI_SMALLFONT;
			label->color = uis.text_color;
		}
	}

// STONELANCE
/*
	s_playersettings.framel.generic.type  = MTYPE_BITMAP;
	s_playersettings.framel.generic.name  = ART_FRAMEL;
	s_playersettings.framel.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_playersettings.framel.generic.x     = 0;
	s_playersettings.framel.generic.y     = 78;
	s_playersettings.framel.width         = 256;
	s_playersettings.framel.height        = 329;

	s_playersettings.framer.generic.type  = MTYPE_BITMAP;
	s_playersettings.framer.generic.name  = ART_FRAMER;
	s_playersettings.framer.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_playersettings.framer.generic.x     = 376;
	s_playersettings.framer.generic.y     = 76;
	s_playersettings.framer.width         = 256;
	s_playersettings.framer.height        = 334;
*/

//	y = 144;
	y = 86;
// END
	s_playersettings.name.generic.type			= MTYPE_FIELD;
	s_playersettings.name.generic.flags			= QMF_NODEFAULTINIT;
	s_playersettings.name.generic.ownerdraw		= PlayerSettings_DrawName;
	s_playersettings.name.field.widthInChars	= MAX_NAMELENGTH;
	s_playersettings.name.field.maxchars		= MAX_NAMELENGTH;
// STONELANCE
/*
	s_playersettings.name.generic.x				= 192;
	s_playersettings.name.generic.y				= y;
	s_playersettings.name.generic.left			= 192 - 8;
	s_playersettings.name.generic.top			= y - 8;
	s_playersettings.name.generic.right			= 192 + 200;
	s_playersettings.name.generic.bottom		= y + 2 * PROP_HEIGHT;
*/
	s_playersettings.name.generic.x				= 30;
	s_playersettings.name.generic.y				= y;
	s_playersettings.name.generic.left			= 30;
	s_playersettings.name.generic.top			= y;
	s_playersettings.name.generic.right			= 30 + 203;
	s_playersettings.name.generic.bottom		= y + 36;

//	y += 3 * PROP_HEIGHT;
// END
	s_playersettings.handicap.generic.type		= MTYPE_SPINCONTROL;
	s_playersettings.handicap.generic.flags		= QMF_NODEFAULTINIT;
	s_playersettings.handicap.generic.id		= ID_HANDICAP;
	s_playersettings.handicap.generic.ownerdraw	= PlayerSettings_DrawHandicap;
// STONELANCE
/*
	s_playersettings.handicap.generic.x			= 192;
	s_playersettings.handicap.generic.y			= y;
	s_playersettings.handicap.generic.left		= 192 - 8;
	s_playersettings.handicap.generic.top		= y - 8;
	s_playersettings.handicap.generic.right		= 192 + 200;
	s_playersettings.handicap.generic.bottom	= y + 2 * PROP_HEIGHT;
*/
	s_playersettings.handicap.generic.x			= 262;
	s_playersettings.handicap.generic.y			= y;
	s_playersettings.handicap.generic.left		= 262;
	s_playersettings.handicap.generic.top		= y;
	s_playersettings.handicap.generic.right		= 262 + 194;
	s_playersettings.handicap.generic.bottom	= y + 36;
// END
	s_playersettings.handicap.numitems			= 20;

// STONELANCE
//	y += 3 * PROP_HEIGHT;
// END
	s_playersettings.effects.generic.type		= MTYPE_SPINCONTROL;
	s_playersettings.effects.generic.flags		= QMF_NODEFAULTINIT;
	s_playersettings.effects.generic.id			= ID_EFFECTS;
	s_playersettings.effects.generic.ownerdraw	= PlayerSettings_DrawEffects;
// STONELANCE
/*
	s_playersettings.effects.generic.x			= 192;
	s_playersettings.effects.generic.y			= y;
	s_playersettings.effects.generic.left		= 192 - 8;
	s_playersettings.effects.generic.top		= y - 8;
	s_playersettings.effects.generic.right		= 192 + 200;
	s_playersettings.effects.generic.bottom		= y + 2* PROP_HEIGHT;
*/
	s_playersettings.effects.generic.x			= 463;
	s_playersettings.effects.generic.y			= y;
	s_playersettings.effects.generic.left		= 463;
	s_playersettings.effects.generic.top		= y;
	s_playersettings.effects.generic.right		= 463 + 147;
	s_playersettings.effects.generic.bottom		= y + 36;
// END
	s_playersettings.effects.numitems			= 7;

// STONELANCE
/*
	s_playersettings.model.generic.type			= MTYPE_BITMAP;
	s_playersettings.model.generic.name			= ART_MODEL0;
	s_playersettings.model.generic.flags		= QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.model.generic.id			= ID_MODEL;
	s_playersettings.model.generic.callback		= PlayerSettings_MenuEvent;
	s_playersettings.model.generic.x			= 640;
	s_playersettings.model.generic.y			= 480-64;
	s_playersettings.model.width				= 128;
	s_playersettings.model.height				= 64;
	s_playersettings.model.focuspic				= ART_MODEL1;
*/
	s_playersettings.customize.generic.type		= MTYPE_PTEXT;
	s_playersettings.customize.generic.flags	= QMF_NODEFAULTINIT;
	s_playersettings.customize.generic.id		= ID_CUSTOMIZE;
	s_playersettings.customize.generic.ownerdraw= PlayerSettings_DrawCustomize;
	s_playersettings.customize.generic.x		= 640 - 20;
	s_playersettings.customize.generic.y		= 480 - 60;
	s_playersettings.customize.generic.left		= 640 - 20 - 100;
	s_playersettings.customize.generic.top		= 480 - 60;
	s_playersettings.customize.generic.right	= 640 - 20;
	s_playersettings.customize.generic.bottom	= 480 - 20;
	s_playersettings.customize.generic.callback	= PlayerSettings_MenuEvent; 
	s_playersettings.customize.color			= text_color_normal;
	s_playersettings.customize.style			= UI_RIGHT;
// END

	s_playersettings.statsPanel.generic.type = MTYPE_PTEXT;
	s_playersettings.statsPanel.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE|QMF_SMALLFONT;
	s_playersettings.statsPanel.generic.x = 72;
	s_playersettings.statsPanel.generic.y = 144;
	s_playersettings.statsPanel.generic.left = 72;
	s_playersettings.statsPanel.generic.top = 144;
	s_playersettings.statsPanel.generic.right = 72 + 240;
	s_playersettings.statsPanel.generic.bottom = 144 + 220;
	s_playersettings.statsPanel.generic.ownerdraw = PlayerSettings_DrawStatsPanel;
	s_playersettings.statsPanel.color = text_color_normal;
	s_playersettings.statsPanel.style = UI_LEFT|UI_SMALLFONT;

	s_playersettings.achievementsPanel.generic.type = MTYPE_PTEXT;
	s_playersettings.achievementsPanel.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE|QMF_SMALLFONT;
	s_playersettings.achievementsPanel.generic.x = 72;
	s_playersettings.achievementsPanel.generic.y = 144;
	s_playersettings.achievementsPanel.generic.left = 72;
	s_playersettings.achievementsPanel.generic.top = 144;
	s_playersettings.achievementsPanel.generic.right = 72 + 320;
	s_playersettings.achievementsPanel.generic.bottom = 144 + 220;
	s_playersettings.achievementsPanel.generic.ownerdraw = PlayerSettings_DrawAchievementsPanel;
	s_playersettings.achievementsPanel.color = text_color_normal;
	s_playersettings.achievementsPanel.style = UI_LEFT|UI_SMALLFONT;

	s_playersettings.profileNameLabel.generic.type = MTYPE_PTEXT;
	s_playersettings.profileNameLabel.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE;
	s_playersettings.profileNameLabel.generic.x = 360;
	s_playersettings.profileNameLabel.generic.y = 140;
	s_playersettings.profileNameLabel.generic.left = 360;
	s_playersettings.profileNameLabel.generic.top = 140;
	s_playersettings.profileNameLabel.generic.right = 360 + 220;
	s_playersettings.profileNameLabel.generic.bottom = 140 + SMALLCHAR_HEIGHT;
	s_playersettings.profileNameLabel.string = "PROFILE NAME";
	s_playersettings.profileNameLabel.style = UI_LEFT|UI_SMALLFONT;
	s_playersettings.profileNameLabel.color = text_color_normal;

	s_playersettings.profileList.generic.type = MTYPE_SPINCONTROL;
	s_playersettings.profileList.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.profileList.generic.id = ID_PROFILE_LIST;
	s_playersettings.profileList.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.profileList.generic.x = 360;
	s_playersettings.profileList.generic.y = 170;
	s_playersettings.profileList.generic.left = 360;
	s_playersettings.profileList.generic.top = 170;
	s_playersettings.profileList.generic.right = 360 + 220;
	s_playersettings.profileList.generic.bottom = 170 + SMALLCHAR_HEIGHT;
	s_playersettings.profileList.width = 220;
	s_playersettings.profileList.height = SMALLCHAR_HEIGHT;
	s_playersettings.profileList.columns = 1;
	s_playersettings.profileList.separation = 0;

	s_playersettings.profileName.generic.type = MTYPE_FIELD;
	s_playersettings.profileName.generic.flags = QMF_NODEFAULTINIT|QMF_SMALLFONT;
	s_playersettings.profileName.generic.id = ID_PROFILE_NEWNAME;
	s_playersettings.profileName.generic.x = 360;
	s_playersettings.profileName.generic.y = 210;
	s_playersettings.profileName.generic.left = 360;
	s_playersettings.profileName.generic.top = 210;
	s_playersettings.profileName.generic.right = 360 + 220;
	s_playersettings.profileName.generic.bottom = 210 + SMALLCHAR_HEIGHT;
	s_playersettings.profileName.field.widthInChars = 20;
	s_playersettings.profileName.field.maxchars = MAX_PROFILE_NAME_LENGTH - 1;

	s_playersettings.profileCreate.generic.type = MTYPE_PTEXT;
	s_playersettings.profileCreate.generic.flags = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.profileCreate.generic.id = ID_PROFILE_CREATE;
	s_playersettings.profileCreate.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.profileCreate.generic.x = 360;
	s_playersettings.profileCreate.generic.y = 244;
	s_playersettings.profileCreate.generic.left = 360;
	s_playersettings.profileCreate.generic.top = 244 - SMALLCHAR_HEIGHT;
	s_playersettings.profileCreate.generic.right = 360 + 120;
	s_playersettings.profileCreate.generic.bottom = 244 + SMALLCHAR_HEIGHT;
	s_playersettings.profileCreate.string = "CREATE";
	s_playersettings.profileCreate.style = UI_LEFT|UI_SMALLFONT;
	s_playersettings.profileCreate.color = text_color_normal;

	s_playersettings.profileDelete.generic.type = MTYPE_PTEXT;
	s_playersettings.profileDelete.generic.flags = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.profileDelete.generic.id = ID_PROFILE_DELETE;
	s_playersettings.profileDelete.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.profileDelete.generic.x = 360;
	s_playersettings.profileDelete.generic.y = 268;
	s_playersettings.profileDelete.generic.left = 360;
	s_playersettings.profileDelete.generic.top = 268 - SMALLCHAR_HEIGHT;
	s_playersettings.profileDelete.generic.right = 360 + 120;
	s_playersettings.profileDelete.generic.bottom = 268 + SMALLCHAR_HEIGHT;
	s_playersettings.profileDelete.string = "DELETE";
	s_playersettings.profileDelete.style = UI_LEFT|UI_SMALLFONT;
	s_playersettings.profileDelete.color = text_color_normal;

	s_playersettings.profileSelect.generic.type = MTYPE_PTEXT;
	s_playersettings.profileSelect.generic.flags = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.profileSelect.generic.id = ID_PROFILE_SELECT;
	s_playersettings.profileSelect.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.profileSelect.generic.x = 360;
	s_playersettings.profileSelect.generic.y = 292;
	s_playersettings.profileSelect.generic.left = 360;
	s_playersettings.profileSelect.generic.top = 292 - SMALLCHAR_HEIGHT;
	s_playersettings.profileSelect.generic.right = 360 + 120;
	s_playersettings.profileSelect.generic.bottom = 292 + SMALLCHAR_HEIGHT;
	s_playersettings.profileSelect.string = "LOAD";
	s_playersettings.profileSelect.style = UI_LEFT|UI_SMALLFONT;
	s_playersettings.profileSelect.color = text_color_normal;

	s_playersettings.player.generic.type		= MTYPE_BITMAP;
	s_playersettings.player.generic.flags		= QMF_INACTIVE;
	s_playersettings.player.generic.ownerdraw	= PlayerSettings_DrawPlayer;
// STONELANCE
/*
	s_playersettings.player.generic.x			= 400;
	s_playersettings.player.generic.y			= -40;
	s_playersettings.player.width				= 32*10;
	s_playersettings.player.height				= 56*10;
*/
	s_playersettings.player.generic.x	       = 40;
	s_playersettings.player.generic.y	       = 0;
	s_playersettings.player.width	           = 560;
	s_playersettings.player.height             = 480;


	y = 138;
	s_playersettings.modelname.generic.type   = MTYPE_PTEXT;
	s_playersettings.modelname.generic.flags  = QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_playersettings.modelname.generic.x	  = 320;
	s_playersettings.modelname.generic.y	  = y + 4;
	s_playersettings.modelname.string	      = modelname;
	s_playersettings.modelname.style		  = UI_CENTER;
	s_playersettings.modelname.color          = text_color_normal;

	s_playersettings.left.generic.type			= MTYPE_BITMAP;
	s_playersettings.left.generic.name			= ART_LEFT0;
	s_playersettings.left.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.left.generic.callback		= PlayerSettings_MenuEvent;
	s_playersettings.left.generic.id			= ID_LEFT;
	s_playersettings.left.generic.x				= 124 - 16;
	s_playersettings.left.generic.y				= y;
	s_playersettings.left.width  				= 32;
	s_playersettings.left.height  				= 32;
	s_playersettings.left.focuspic				= ART_LEFT1;
	
	s_playersettings.right.generic.type			= MTYPE_BITMAP;
	s_playersettings.right.generic.name			= ART_RIGHT0;
	s_playersettings.right.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.right.generic.callback		= PlayerSettings_MenuEvent;
	s_playersettings.right.generic.id			= ID_RIGHT;
	s_playersettings.right.generic.x			= 124 + 392 - 16;
	s_playersettings.right.generic.y			= y;
	s_playersettings.right.width  				= 32;
	s_playersettings.right.height  				= 32;
	s_playersettings.right.focuspic				= ART_RIGHT1;

	s_playersettings.favorites.generic.type   = MTYPE_PTEXT;
	s_playersettings.favorites.generic.flags  = QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_playersettings.favorites.generic.x	  = 320;
	s_playersettings.favorites.generic.y	  = 378;
	s_playersettings.favorites.string	      = "LOAD FAVORITE";
	s_playersettings.favorites.style		  = UI_CENTER|UI_SMALLFONT;
	s_playersettings.favorites.color          = text_color_normal;

	x =	183;
	y = 403;
	for (j=0; j<NUM_FAVORITES; j++)
	{
		s_playersettings.ports[j].generic.type		= MTYPE_BITMAP;
		s_playersettings.ports[j].generic.name		= ART_PORT;
		s_playersettings.ports[j].generic.flags		= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
		s_playersettings.ports[j].generic.x			= x;
		s_playersettings.ports[j].generic.y			= y;
		s_playersettings.ports[j].width  			= 64;
		s_playersettings.ports[j].height  			= 64;

		s_playersettings.favpics[j].generic.type	= MTYPE_BITMAP;
		s_playersettings.favpics[j].generic.flags	= QMF_LEFT_JUSTIFY|QMF_INACTIVE;
		s_playersettings.favpics[j].generic.x		= x;
		s_playersettings.favpics[j].generic.y		= y;
		s_playersettings.favpics[j].width  			= 64;
		s_playersettings.favpics[j].height  		= 64;
		s_playersettings.favpics[j].focuspic        = ART_SELECTED;
		s_playersettings.favpics[j].focuscolor      = text_color_highlight;

		s_playersettings.favpicbuttons[j].generic.type		= MTYPE_BITMAP;
		s_playersettings.favpicbuttons[j].generic.flags		= QMF_LEFT_JUSTIFY|QMF_NODEFAULTINIT|QMF_PULSEIFFOCUS;
		s_playersettings.favpicbuttons[j].generic.id	    = ID_FAVORITE1 + j;
		s_playersettings.favpicbuttons[j].generic.callback	= PlayerSettings_PicEvent;
		s_playersettings.favpicbuttons[j].generic.x    		= x;
		s_playersettings.favpicbuttons[j].generic.y			= y;
		s_playersettings.favpicbuttons[j].generic.left		= x;
		s_playersettings.favpicbuttons[j].generic.top		= y;
		s_playersettings.favpicbuttons[j].generic.right		= x + 64;
		s_playersettings.favpicbuttons[j].generic.bottom	= y + 64;
		s_playersettings.favpicbuttons[j].width  		    = 64;
		s_playersettings.favpicbuttons[j].height  			= 64;
		s_playersettings.favpicbuttons[j].focuspic  		= ART_SELECT;
		s_playersettings.favpicbuttons[j].focuscolor  		= text_color_highlight;

		x += 64+6;
	}

	s_playersettings.plate.generic.type				= MTYPE_PTEXT;
	s_playersettings.plate.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.plate.generic.x				= 640 - 140;
	s_playersettings.plate.generic.y				= 378;
	s_playersettings.plate.generic.id				= ID_PLATE;
	s_playersettings.plate.generic.callback			= PlayerSettings_MenuEvent; 
	s_playersettings.plate.string					= "CHANGE PLATE";
	s_playersettings.plate.color					= text_color_normal;
	s_playersettings.plate.style					= UI_LEFT | UI_SMALLFONT;


	s_playersettings.back.generic.type				= MTYPE_PTEXT;
	s_playersettings.back.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.back.generic.x					= 20;
	s_playersettings.back.generic.y					= 480 - 50;
	s_playersettings.back.generic.id				= ID_BACK;
	s_playersettings.back.generic.callback			= PlayerSettings_MenuEvent; 
	s_playersettings.back.string					= "< BACK";
	s_playersettings.back.color						= text_color_normal;
	s_playersettings.back.style						= UI_LEFT | UI_SMALLFONT;

/*
	s_playersettings.back.generic.type			= MTYPE_BITMAP;
	s_playersettings.back.generic.name			= ART_BACK0;
	s_playersettings.back.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_playersettings.back.generic.id			= ID_BACK;
	s_playersettings.back.generic.callback		= PlayerSettings_MenuEvent;
	s_playersettings.back.generic.x				= 0;
	s_playersettings.back.generic.y				= 480-64;
	s_playersettings.back.width					= 128;
	s_playersettings.back.height				= 64;
	s_playersettings.back.focuspic				= ART_BACK1;

	s_playersettings.item_null.generic.type		= MTYPE_BITMAP;
	s_playersettings.item_null.generic.flags	= QMF_LEFT_JUSTIFY|QMF_MOUSEONLY|QMF_SILENT;
	s_playersettings.item_null.generic.x		= 0;
	s_playersettings.item_null.generic.y		= 0;
	s_playersettings.item_null.width			= 640;
	s_playersettings.item_null.height			= 480;
*/
// END

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.banner );
	for ( i = 0; i < PLAYERSETTINGS_NUM_TABS; i++ ) {
		Menu_AddItem( &s_playersettings.menu, &s_playersettings.tabButtons[i] );
		Menu_AddItem( &s_playersettings.menu, &s_playersettings.tabLabels[i] );
	}
// STONELANCE
/*
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.framel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.framer );
*/
// END

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.name );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.handicap );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.effects );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.statsPanel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.profileNameLabel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.profileList );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.profileName );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.profileCreate );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.profileDelete );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.profileSelect );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.achievementsPanel );

// STONELANCE
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.favorites );
	for (i=0; i<NUM_FAVORITES; i++)
	{
		Menu_AddItem( &s_playersettings.menu, &s_playersettings.ports[i] );
		Menu_AddItem( &s_playersettings.menu, &s_playersettings.favpicbuttons[i] );
		Menu_AddItem( &s_playersettings.menu, &s_playersettings.favpics[i] );
	}

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.player );

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.left );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.right );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.modelname );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.customize );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.plate );
// END

// STONELANCE
//	Menu_AddItem( &s_playersettings.menu, &s_playersettings.model );
// END

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.back );

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.player );

// STONELANCE
//	Menu_AddItem( &s_playersettings.menu, &s_playersettings.item_null );
// END

	PlayerSettings_SetMenuItems();
	PlayerSettings_SetActiveTab( s_playersettingsInitialTab );

// STONELANCE
	uis.transitionIn = uis.realtime;
// END
}


/*
=================
PlayerSettings_Cache
=================
*/
void PlayerSettings_Cache( void ) {
// STONELANCE
/*
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_MODEL0 );
	trap_R_RegisterShaderNoMip( ART_MODEL1 );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
*/
// END

	s_playersettings.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
	s_playersettings.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	s_playersettings.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	s_playersettings.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	s_playersettings.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	s_playersettings.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	s_playersettings.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	s_playersettings.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );

// STONELANCE
	PlayerSettings_BuildList();
// END
}


/*
=================
UI_PlayerSettingsMenu
=================
*/
void UI_PlayerSettingsMenu( void ) {
	PlayerSettings_MenuInit();
	UI_PushMenu( &s_playersettings.menu );
	s_playersettingsInitialTab = PLAYERSETTINGS_TAB_CAR;
}

void UI_PlayerProfileMenu( void ) {
	s_playersettingsInitialTab = PLAYERSETTINGS_TAB_STATS;
	UI_PlayerSettingsMenu();
}

// STONELANCE
/*****************************************************

  Plate Selection

*****************************************************/


#define		ID_LIST			1
#define		ID_CANCEL		2
#define		ID_ACCEPT		3


#define		MAX_PLATEMODELS		256

typedef struct {
	menuframework_s		menu;

	menulist_s			list;

	menutext_s			cancel;
	menutext_s			accept;

	char				plateList[MAX_PLATEMODELS][MAX_QPATH];
	char*				items[MAX_PLATEMODELS];
	int					numPlates;

	char				plateSkin[MAX_QPATH];
} plateSelection_t;

static plateSelection_t	s_plateSelection;


/*
=================
PlateSelection_Event
=================
*/
static void PlateSelection_Event( void* ptr, int event ) {
	int		id;

	id = ((menucommon_s*)ptr)->id;

	if( event != QM_ACTIVATED && id != ID_LIST ) {
		return;
	}

	switch( id ) {
	case ID_LIST:
		// update plateSkin
		Q_strncpyz( s_plateSelection.plateSkin, s_plateSelection.plateList[s_plateSelection.list.curvalue], sizeof(s_plateSelection.plateSkin) );
		break;

	case ID_CANCEL:
		UI_PopMenu();
		uis.transitionIn = 0;
		break;

	case ID_ACCEPT:
		trap_Cvar_Set( "plate", s_plateSelection.plateSkin );
		PlayerSettings_UpdateModel();
		UI_PopMenu();
		uis.transitionIn = 0;
		break;
	}
}


/*
=================
PlateSelection_DrawMenu
=================
*/
static void PlateSelection_DrawMenu( void ) {
	refdef_t		refdef;
	refEntity_t		ent;
	vec3_t			origin;
	vec3_t			angles;
	float			x, y, w, h;

	// setup the refdef

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	x = 150;
	y = 115;
	w = 328;
	h = 232;
	UI_AdjustFrom640( &x, &y, &w, &h );
	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.fov_x = 180;
	refdef.fov_y = 180;

	refdef.time = uis.realtime;

	origin[0] = 300;
	origin[1] = 0;
	origin[2] = 0;

	trap_R_ClearScene();

	// draw license plate with selected skin

	memset( &ent, 0, sizeof(ent) );

	VectorSet( angles, 45, 45, 45 );
	AnglesToAxis( angles, ent.axis );

	if (strstr(s_plateSelection.plateSkin, "usa_"))
		ent.hModel = trap_R_RegisterModel("models/players/plates/plate_usa.md3");
	else
		ent.hModel = trap_R_RegisterModel("models/players/plates/plate_eu.md3");
	ent.customShader = trap_R_RegisterShaderNoMip( va("models/players/plates/%s", s_plateSelection.plateSkin) );

	VectorCopy( origin, ent.origin );
	VectorCopy( origin, ent.lightingOrigin );
	ent.renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;
	VectorCopy( ent.origin, ent.oldorigin );

	trap_R_AddRefEntityToScene( &ent );

	trap_R_RenderScene( &refdef );

/*
	qhandle_t	plate;
	plate = trap_R_RegisterShaderNoMip( va("models/players/plates/%s", s_plateSelection.plateSkin) );

	if (strstr(s_plateSelection.plateSkin, "usa_"))
		UI_DrawHandlePic(250+32, 215, 64, 32, plate);
	else
		UI_DrawHandlePic(250, 215, 128, 32, plate);
*/

	Menu_Draw( &s_plateSelection.menu );
}


/*
=================
PlateSelection_SetMenuItems
=================
*/
static void PlateSelection_SetMenuItems( void ) {
	int		i;

	trap_Cvar_VariableStringBuffer( "plate", s_plateSelection.plateSkin, sizeof( s_plateSelection.plateSkin ) );

	if (!s_plateSelection.numPlates)
		return;

	// find model in our list
	for (i = 0; i < s_plateSelection.numPlates; i++)
	{
		if (!Q_stricmp( s_plateSelection.plateSkin, s_plateSelection.plateList[i] )){
			// found pic, set selection here
			s_plateSelection.list.curvalue = i;
			if (s_plateSelection.list.top + s_plateSelection.list.height > s_plateSelection.numPlates)
				s_plateSelection.list.top = s_plateSelection.numPlates - s_plateSelection.list.height;

			if (s_plateSelection.list.top < 0)
				s_plateSelection.list.top = 0;
			else
				s_plateSelection.list.top = i;

			return;
		}
	}

	s_plateSelection.list.curvalue = 0;
	s_plateSelection.list.top = 0;
	Q_strncpyz(s_plateSelection.plateSkin, s_plateSelection.plateList[0], sizeof(s_plateSelection.plateSkin));
}


/*
=================
PlateSelection_Cache
=================
*/
void PlateSelection_Cache( void ) {
	// get car list
	s_plateSelection.numPlates = UI_BuildFileList("models/players/plates", "tga", "*usa_", qtrue, qfalse, qtrue, 0, s_plateSelection.plateList);
	s_plateSelection.numPlates = UI_BuildFileList("models/players/plates", "tga", "*eu_", qtrue, qfalse, qtrue, s_plateSelection.numPlates, s_plateSelection.plateList);
}


/*
=================
PlateSelection_MenuInit
=================
*/
static void PlateSelection_MenuInit( void ) {
	int		i;

	memset(&s_plateSelection, 0, sizeof(plateSelection_t));

	PlateSelection_Cache();

	s_plateSelection.menu.wrapAround = qtrue;
	s_plateSelection.menu.transparent = qtrue;
	s_plateSelection.menu.fullscreen = qtrue;
	s_plateSelection.menu.draw		 = PlateSelection_DrawMenu;


	s_plateSelection.list.generic.type			= MTYPE_LISTBOX;
	s_plateSelection.list.scrollbarAlignment	= SB_RIGHT;
	s_plateSelection.list.generic.flags			= QMF_LEFT_JUSTIFY|QMF_HIGHLIGHT_IF_FOCUS;
	s_plateSelection.list.generic.id			= ID_LIST;
	s_plateSelection.list.generic.callback		= PlateSelection_Event;
	s_plateSelection.list.generic.x				= 50;
	s_plateSelection.list.generic.y				= 175;
	s_plateSelection.list.width					= 25;
	s_plateSelection.list.height				= 11;
	s_plateSelection.list.itemnames				= (const char **)s_plateSelection.items;
	s_plateSelection.list.numitems				= s_plateSelection.numPlates;
	for( i = 0; i < MAX_PLATEMODELS; i++ ) {
		s_plateSelection.items[i] = s_plateSelection.plateList[i];
	}

	s_plateSelection.cancel.generic.type				= MTYPE_PTEXT;
	s_plateSelection.cancel.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_plateSelection.cancel.generic.x					= 250;
	s_plateSelection.cancel.generic.y					= 350;
	s_plateSelection.cancel.generic.id					= ID_CANCEL;
	s_plateSelection.cancel.generic.callback			= PlateSelection_Event; 
	s_plateSelection.cancel.string						= "Cancel";
	s_plateSelection.cancel.color						= text_color_normal;
	s_plateSelection.cancel.style						= UI_LEFT | UI_SMALLFONT;

	s_plateSelection.accept.generic.type				= MTYPE_PTEXT;
	s_plateSelection.accept.generic.flags				= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_plateSelection.accept.generic.x					= 350;
	s_plateSelection.accept.generic.y					= 350;
	s_plateSelection.accept.generic.id					= ID_ACCEPT;
	s_plateSelection.accept.generic.callback			= PlateSelection_Event; 
	s_plateSelection.accept.string						= "Accept";
	s_plateSelection.accept.color						= text_color_normal;
	s_plateSelection.accept.style						= UI_LEFT | UI_SMALLFONT;


	Menu_AddItem( &s_plateSelection.menu, &s_plateSelection.list );
	Menu_AddItem( &s_plateSelection.menu, &s_plateSelection.cancel );
	Menu_AddItem( &s_plateSelection.menu, &s_plateSelection.accept );


	PlateSelection_SetMenuItems();
}


/*
=================
UI_PlateSelectionMenu
=================
*/
void UI_PlateSelectionMenu( void ) {
	PlateSelection_MenuInit();
	UI_PushMenu( &s_plateSelection.menu );
}
// END
