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
#include "../game/q3r_profile.h"
#include "../game/g_profile.h"

#ifndef INT_MAX
#define INT_MAX 0x7fffffff
#endif

#ifndef INT_MIN
#define INT_MIN (-INT_MAX - 1)
#endif

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
#define ID_PROFILE_REFRESH      34

#define MAX_NAMELENGTH	20
// STONELANCE
#define NUM_FAVORITES		4
#define MAX_PLAYERMODELS	256
#define MAX_RIMMODELS		256
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
        char                    identifier[MAX_QPATH];
} playerLifetimeDisplay_t;

typedef enum {
    TAB_VEHICLE,
    TAB_STATS,
    TAB_ACHIEVEMENTS,
    NUM_TABS
} profile_tab_t;

typedef struct {
	menuframework_s		menu;
    profile_tab_t   current_tab;

    menutext_s      tabs[NUM_TABS];

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
	menutext_s			profileRefresh;
	menutext_s			achievementsPanel;

	menulist_s			profileList;
	menutext_s			profileNameLabel;

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

	int					numRims;
	char				rimList[MAX_RIMMODELS][MAX_QPATH];
// END

	qhandle_t			fxBasePic;
	qhandle_t			fxPic[7];
	playerInfo_t		playerinfo;
	int					current_fx;
	char				playerModel[MAX_QPATH];

    // Stats
    menutext_s      stats_races_started;
    menutext_s      stats_races_finished;
    menutext_s      stats_races_won;
    menutext_s      stats_derby_matches;
    menutext_s      stats_derby_wins;
    menutext_s      stats_play_time;
    menutext_s      stats_distance_driven;

    // Achievements
    menulist_s      achievements_filter;
    menutext_s      achievements_list[MAX_ACHIEVEMENTS];

    // Vehicle & Player
    menutext_s      player_name;
} playersettings_t;

static playersettings_t	s_playersettings;
static int s_playersettingsInitialTab = PLAYERSETTINGS_TAB_CAR;

static int gamecodetoui[] = {4,2,3,0,5,1,6};
static int uitogamecode[] = {4,6,2,3,1,5,7};

static void PlayerSettings_SetTab(profile_tab_t tab) {
    s_playersettings.current_tab = tab;
    for (int i = 0; i < NUM_TABS; i++) {
        s_playersettings.tabs[i].color = (i == tab) ? text_color_highlight : text_color_normal;
    }

    // Vehicle & Player
    s_playersettings.name.generic.flags = (tab == TAB_VEHICLE) ? QMF_NODEFAULTINIT | QMF_INACTIVE : QMF_HIDDEN;
    s_playersettings.handicap.generic.flags = (tab == TAB_VEHICLE) ? QMF_NODEFAULTINIT : QMF_HIDDEN;
    s_playersettings.effects.generic.flags = (tab == TAB_VEHICLE) ? QMF_NODEFAULTINIT : QMF_HIDDEN;
    s_playersettings.player_name.generic.flags = (tab == TAB_VEHICLE) ? QMF_LEFT_JUSTIFY : QMF_HIDDEN;
    // ... hide/show other controls for this tab

    // Stats
    s_playersettings.stats_races_started.generic.flags = (tab == TAB_STATS) ? QMF_LEFT_JUSTIFY : QMF_HIDDEN;
    s_playersettings.stats_races_finished.generic.flags = (tab == TAB_STATS) ? QMF_LEFT_JUSTIFY : QMF_HIDDEN;
    s_playersettings.stats_races_won.generic.flags = (tab == TAB_STATS) ? QMF_LEFT_JUSTIFY : QMF_HIDDEN;
    s_playersettings.stats_derby_matches.generic.flags = (tab == TAB_STATS) ? QMF_LEFT_JUSTIFY : QMF_HIDDEN;
    s_playersettings.stats_derby_wins.generic.flags = (tab == TAB_STATS) ? QMF_LEFT_JUSTIFY : QMF_HIDDEN;
    s_playersettings.stats_play_time.generic.flags = (tab == TAB_STATS) ? QMF_LEFT_JUSTIFY : QMF_HIDDEN;
    s_playersettings.stats_distance_driven.generic.flags = (tab == TAB_STATS) ? QMF_LEFT_JUSTIFY : QMF_HIDDEN;

    // Achievements
    s_playersettings.achievements_filter.generic.flags = (tab == TAB_ACHIEVEMENTS) ? QMF_NODEFAULTINIT : QMF_HIDDEN;
    for (int i = 0; i < ACH_MAX; i++) {
        s_playersettings.achievements_list[i].generic.flags = (tab == TAB_ACHIEVEMENTS) ? QMF_LEFT_JUSTIFY : QMF_HIDDEN;
    }
}

static void PlayerSettings_TabEvent(void* ptr, int event) {
    if (event == QM_ACTIVATED) {
        PlayerSettings_SetTab((profile_tab_t)(((menucommon_s*)ptr)->id - ID_TAB_CAR));
    }
}

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
static void PlayerSettings_UpdateFavorites( void );
static void PlayerSettings_LoadProfileSlots( void );
static void PlayerSettings_BuildProfileItems( void );
static int PlayerSettings_FindProfileIndex( const char *name );
static qboolean PlayerSettings_SanitizeProfileName( const char *input, char *output, size_t size );
static qboolean PlayerSettings_ProfileFileExists( const char *prefix, const char *slot );
static qboolean PlayerSettings_FindProfileIdentifierForSlot( const char *slot, char *identifier, size_t size );

static void PlayerSettings_UpdateVehicleCvarsFromLifetime( const profileLifetime_t *lifetime ) {
	if ( !lifetime ) {
		trap_Cvar_Set( "ui_profile_model", "" );
		trap_Cvar_Set( "ui_profile_head", "" );
		trap_Cvar_Set( "ui_profile_rim", "" );
		trap_Cvar_Set( "ui_profile_plate", "" );
		return;
	}

	trap_Cvar_Set( "ui_profile_model", lifetime->vehicleModel );
	trap_Cvar_Set( "ui_profile_head", lifetime->vehicleHead );
	trap_Cvar_Set( "ui_profile_rim", lifetime->vehicleRim );
	trap_Cvar_Set( "ui_profile_plate", lifetime->vehiclePlate );

	if ( lifetime->vehicleModel[0] ) {
		trap_Cvar_Set( "model", lifetime->vehicleModel );
	}
	if ( lifetime->vehicleHead[0] ) {
		trap_Cvar_Set( "head", lifetime->vehicleHead );
	}
	if ( lifetime->vehicleRim[0] ) {
		trap_Cvar_Set( "rim", lifetime->vehicleRim );
	}
	if ( lifetime->vehiclePlate[0] ) {
		trap_Cvar_Set( "plate", lifetime->vehiclePlate );
	}
}

static void PlayerSettings_RegisterProfileCvars( void );
static void PlayerSettings_AddProfileSlot( const char *name );
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

static qboolean PlayerSettings_ProfileFileExists( const char *prefix, const char *slot ) {
        char path[MAX_QPATH];
        fileHandle_t file;
        int length;

        if ( !prefix || !prefix[0] || !slot || !slot[0] ) {
                return qfalse;
        }

        Com_sprintf( path, sizeof( path ), "%s/%s/%s%s", PROFILE_DIRECTORY, prefix, slot, PROFILE_EXTENSION );
        length = trap_FS_FOpenFile( path, &file, FS_READ );
        if ( length > 0 && file ) {
                trap_FS_FCloseFile( file );
                return qtrue;
        }

        if ( file ) {
                trap_FS_FCloseFile( file );
        }

        return qfalse;
}

static qboolean PlayerSettings_FindProfileIdentifierForSlot( const char *slot, char *identifier, size_t size ) {
        char dirList[2048];
        char sanitized[MAX_QPATH];
        char path[MAX_QPATH];
        char *dirPtr;
        int dirCount;
        int dirLen;
        int i;

        if ( !slot || !slot[0] || !identifier || size == 0 ) {
                return qfalse;
        }

        dirCount = trap_FS_GetFileList( PROFILE_DIRECTORY, "/", dirList, sizeof( dirList ) );
        dirPtr = dirList;

        for ( i = 0; i < dirCount && dirPtr && *dirPtr; i++ ) {
                fileHandle_t file;
                int length;
                char *next;

                dirLen = strlen( dirPtr );
                next = dirPtr + dirLen + 1;

                if ( dirLen <= 0 ) {
                        dirPtr = next;
                        continue;
                }

                if ( dirPtr[dirLen - 1] == '/' ) {
                        dirPtr[dirLen - 1] = '\0';
                        dirLen--;
                        if ( dirLen <= 0 ) {
                                dirPtr = next;
                                continue;
                        }
                }

                if ( !PlayerSettings_SanitizeProfileName( dirPtr, sanitized, sizeof( sanitized ) ) ) {
                        dirPtr = next;
                        continue;
                }

                Com_sprintf( path, sizeof( path ), "%s/%s/%s%s", PROFILE_DIRECTORY, sanitized, slot, PROFILE_EXTENSION );
                length = trap_FS_FOpenFile( path, &file, FS_READ );
                if ( length > 0 && file ) {
                        trap_FS_FCloseFile( file );
                        Com_sprintf( identifier, size, "%s/%s", sanitized, slot );
                        trap_Cvar_Set( "ui_profile_identifier", identifier );
                        return qtrue;
                }

                if ( file ) {
                        trap_FS_FCloseFile( file );
                }

                dirPtr = next;
        }

        return qfalse;
}

typedef struct {
        int     version;
        int     matchesPlayed;
        int     wins;
        int     losses;
        int     finishes;
        int     dnfs;
        int     bestPosition;
        int     bestLapMs;
        int     bestTotalRaceMs;
        int     totalRaceTimeMs;
        int     totalScore;
        int     totalKills;
        int     totalDeaths;
        int     totalDamageDealt;
        int     totalDamageTaken;
        float   totalDistanceMeters;
        float   totalFuelConsumed;
        int     achievements;
        char    vehicleModel[MAX_QPATH];
        char    vehicleHead[MAX_QPATH];
        char    vehicleRim[MAX_QPATH];
        char    vehiclePlate[MAX_QPATH];
} playerSettingsProfileDisk_t;

typedef struct {
        int     version;
        int     matchesPlayed;
        int     wins;
        int     losses;
        int     finishes;
        int     dnfs;
        int     bestPosition;
        int     bestLapMs;
        int     bestTotalRaceMs;
        int     totalRaceTimeMs;
        int     totalScore;
        int     totalKills;
        int     totalDeaths;
        int     totalDamageDealt;
        int     totalDamageTaken;
        float   totalDistanceMeters;
        float   totalFuelConsumed;
        int     achievements;
} playerSettingsProfileDiskV2_t;

typedef struct {
        int     version;
        int     matchesPlayed;
        int     wins;
        int     losses;
        int     finishes;
        int     dnfs;
        int     bestPosition;
        int     bestLapMs;
        int     bestTotalRaceMs;
        int     totalRaceTimeMs;
        int     totalScore;
        int     totalKills;
        int     totalDeaths;
        int     totalDamageDealt;
        int     totalDamageTaken;
        float   totalDistanceMeters;
        float   totalFuelConsumed;
} playerSettingsProfileDiskV1_t;

static int PlayerSettings_EncodeScaledFloat( float value, float scale ) {
        double scaled;

        scaled = (double)value * (double)scale;
        if ( scaled > (double)INT_MAX ) {
                return INT_MAX;
        }
        if ( scaled < (double)INT_MIN ) {
                return INT_MIN;
        }
        if ( scaled >= 0.0 ) {
                return (int)( scaled + 0.5 );
        }
        return (int)( scaled - 0.5 );
}

static qboolean PlayerSettings_BuildProfileIdentifier( char *identifier, size_t size ) {
        char prefix[MAX_QPATH];
        char slot[MAX_QPATH];
        char buffer[MAX_CVAR_VALUE_STRING];
        char identifierBuffer[MAX_QPATH];
        char identifierSlot[MAX_QPATH];
        const char *separator;

        if ( !identifier || size == 0 ) {
                return qfalse;
        }

        identifier[0] = '\0';

        trap_Cvar_VariableStringBuffer( "cg_profile", buffer, sizeof( buffer ) );
        if ( !PlayerSettings_SanitizeProfileName( buffer, slot, sizeof( slot ) ) ) {
                slot[0] = '\0';
        }

        trap_Cvar_VariableStringBuffer( "ui_profile_identifier", identifierBuffer, sizeof( identifierBuffer ) );
        separator = strchr( identifierBuffer, '/' );
        if ( separator ) {
                size_t prefixLength;

                prefixLength = separator - identifierBuffer;
                if ( prefixLength >= sizeof( buffer ) ) {
                        prefixLength = sizeof( buffer ) - 1;
                }

                Q_strncpyz( buffer, identifierBuffer, prefixLength + 1 );

                if ( PlayerSettings_SanitizeProfileName( buffer, prefix, sizeof( prefix ) ) &&
                     PlayerSettings_SanitizeProfileName( separator + 1, identifierSlot, sizeof( identifierSlot ) ) ) {
                        if ( !slot[0] || !Q_stricmp( identifierSlot, slot ) ) {
                                Com_sprintf( identifier, size, "%s/%s", prefix, identifierSlot );
                                return qtrue;
                        }
                }
        }

        trap_Cvar_VariableStringBuffer( "ui_profile_identifier", identifierBuffer, sizeof( identifierBuffer ) );
        separator = strchr( identifierBuffer, '/' );
        if ( separator ) {
                size_t prefixLength;
                char slotSource[MAX_QPATH];

                prefixLength = separator - identifierBuffer;
                if ( prefixLength >= sizeof( buffer ) ) {
                        prefixLength = sizeof( buffer ) - 1;
                }

                Q_strncpyz( buffer, identifierBuffer, prefixLength + 1 );
                Q_strncpyz( slotSource, separator + 1, sizeof( slotSource ) );

                if ( PlayerSettings_SanitizeProfileName( buffer, prefix, sizeof( prefix ) ) &&
                     PlayerSettings_SanitizeProfileName( slotSource, slot, sizeof( slot ) ) ) {
                        Com_sprintf( identifier, size, "%s/%s", prefix, slot );
                        return qtrue;
                }
        }

        trap_Cvar_VariableStringBuffer( "cl_guid", buffer, sizeof( buffer ) );
        if ( !PlayerSettings_SanitizeProfileName( buffer, prefix, sizeof( prefix ) ) ) {
                trap_Cvar_VariableStringBuffer( "name", buffer, sizeof( buffer ) );
                Q_CleanStr( buffer );
                PlayerSettings_SanitizeProfileName( buffer, prefix, sizeof( prefix ) );
        }

        if ( !prefix[0] ) {
                Q_strncpyz( prefix, "client", sizeof( prefix ) );
        }

        if ( slot[0] && PlayerSettings_ProfileFileExists( prefix, slot ) ) {
                Com_sprintf( identifier, size, "%s/%s", prefix, slot );
                trap_Cvar_Set( "ui_profile_identifier", identifier );
                return qtrue;
        }

        if ( slot[0] && PlayerSettings_FindProfileIdentifierForSlot( slot, identifier, size ) ) {
                return qtrue;
        }

        if ( !slot[0] ) {
                return qfalse;
        }

        Com_sprintf( identifier, size, "%s/%s", prefix, slot );
        return qtrue;
}

static qboolean PlayerSettings_LoadLifetimeFromProfile( playerLifetimeDisplay_t *display, const char *identifier ) {
	char path[MAX_QPATH];
	fileHandle_t file;
	int length;
	playerSettingsProfileDisk_t disk;
	playerSettingsProfileDiskV2_t diskV2;
	playerSettingsProfileDiskV1_t diskV1;
	float distanceMeters;
	float fuelConsumed;

	if ( !display || !identifier || !identifier[0] ) {
		return qfalse;
	}

	if ( display->valid && display->sequence == 0 && !Q_stricmp( display->identifier, identifier ) ) {
		return qtrue;
	}

	Com_sprintf( path, sizeof( path ), "%s/%s%s", PROFILE_DIRECTORY, identifier, PROFILE_EXTENSION );

	length = trap_FS_FOpenFile( path, &file, FS_READ );
	if ( length <= 0 || !file ) {
		return qfalse;
	}

	display->raw.vehicleModel[0] = '\0';
	display->raw.vehicleHead[0] = '\0';
	display->raw.vehicleRim[0] = '\0';
	display->raw.vehiclePlate[0] = '\0';
	distanceMeters = 0.0f;
	fuelConsumed = 0.0f;

	if ( length == sizeof( disk ) ) {
		trap_FS_Read( &disk, sizeof( disk ), file );
		trap_FS_FCloseFile( file );

		display->raw.version = LittleLong( disk.version );
		display->raw.matchesPlayed = LittleLong( disk.matchesPlayed );
		display->raw.wins = LittleLong( disk.wins );
		display->raw.losses = LittleLong( disk.losses );
		display->raw.finishes = LittleLong( disk.finishes );
		display->raw.dnfs = LittleLong( disk.dnfs );
		display->raw.bestPosition = LittleLong( disk.bestPosition );
		display->raw.bestLapMs = LittleLong( disk.bestLapMs );
		display->raw.bestTotalRaceMs = LittleLong( disk.bestTotalRaceMs );
		display->raw.totalRaceTimeMs = LittleLong( disk.totalRaceTimeMs );
		display->raw.totalScore = LittleLong( disk.totalScore );
		display->raw.totalKills = LittleLong( disk.totalKills );
		display->raw.totalDeaths = LittleLong( disk.totalDeaths );
		display->raw.totalDamageDealt = LittleLong( disk.totalDamageDealt );
		display->raw.totalDamageTaken = LittleLong( disk.totalDamageTaken );
		distanceMeters = LittleFloat( disk.totalDistanceMeters );
		fuelConsumed = LittleFloat( disk.totalFuelConsumed );
		display->raw.totalDistanceScaled = PlayerSettings_EncodeScaledFloat( distanceMeters, PROFILE_LIFETIME_DISTANCE_SCALE );
		display->raw.totalFuelConsumedScaled = PlayerSettings_EncodeScaledFloat( fuelConsumed, PROFILE_LIFETIME_FUEL_SCALE );
		display->raw.achievements = LittleLong( disk.achievements );
		Q_strncpyz( display->raw.vehicleModel, disk.vehicleModel, sizeof( display->raw.vehicleModel ) );
		Q_strncpyz( display->raw.vehicleHead, disk.vehicleHead, sizeof( display->raw.vehicleHead ) );
		Q_strncpyz( display->raw.vehicleRim, disk.vehicleRim, sizeof( display->raw.vehicleRim ) );
		Q_strncpyz( display->raw.vehiclePlate, disk.vehiclePlate, sizeof( display->raw.vehiclePlate ) );
	} else if ( length == sizeof( diskV2 ) ) {
		trap_FS_Read( &diskV2, sizeof( diskV2 ), file );
		trap_FS_FCloseFile( file );

		display->raw.version = LittleLong( diskV2.version );
		display->raw.matchesPlayed = LittleLong( diskV2.matchesPlayed );
		display->raw.wins = LittleLong( diskV2.wins );
		display->raw.losses = LittleLong( diskV2.losses );
		display->raw.finishes = LittleLong( diskV2.finishes );
		display->raw.dnfs = LittleLong( diskV2.dnfs );
		display->raw.bestPosition = LittleLong( diskV2.bestPosition );
		display->raw.bestLapMs = LittleLong( diskV2.bestLapMs );
		display->raw.bestTotalRaceMs = LittleLong( diskV2.bestTotalRaceMs );
		display->raw.totalRaceTimeMs = LittleLong( diskV2.totalRaceTimeMs );
		display->raw.totalScore = LittleLong( diskV2.totalScore );
		display->raw.totalKills = LittleLong( diskV2.totalKills );
		display->raw.totalDeaths = LittleLong( diskV2.totalDeaths );
		display->raw.totalDamageDealt = LittleLong( diskV2.totalDamageDealt );
		display->raw.totalDamageTaken = LittleLong( diskV2.totalDamageTaken );
		distanceMeters = LittleFloat( diskV2.totalDistanceMeters );
		fuelConsumed = LittleFloat( diskV2.totalFuelConsumed );
		display->raw.totalDistanceScaled = PlayerSettings_EncodeScaledFloat( distanceMeters, PROFILE_LIFETIME_DISTANCE_SCALE );
		display->raw.totalFuelConsumedScaled = PlayerSettings_EncodeScaledFloat( fuelConsumed, PROFILE_LIFETIME_FUEL_SCALE );
		display->raw.achievements = LittleLong( diskV2.achievements );
	} else if ( length == sizeof( diskV1 ) ) {
		trap_FS_Read( &diskV1, sizeof( diskV1 ), file );
		trap_FS_FCloseFile( file );

		display->raw.version = LittleLong( diskV1.version );
		display->raw.matchesPlayed = LittleLong( diskV1.matchesPlayed );
		display->raw.wins = LittleLong( diskV1.wins );
		display->raw.losses = LittleLong( diskV1.losses );
		display->raw.finishes = LittleLong( diskV1.finishes );
		display->raw.dnfs = LittleLong( diskV1.dnfs );
		display->raw.bestPosition = LittleLong( diskV1.bestPosition );
		display->raw.bestLapMs = LittleLong( diskV1.bestLapMs );
		display->raw.bestTotalRaceMs = LittleLong( diskV1.bestTotalRaceMs );
		display->raw.totalRaceTimeMs = LittleLong( diskV1.totalRaceTimeMs );
		display->raw.totalScore = LittleLong( diskV1.totalScore );
		display->raw.totalKills = LittleLong( diskV1.totalKills );
		display->raw.totalDeaths = LittleLong( diskV1.totalDeaths );
		display->raw.totalDamageDealt = LittleLong( diskV1.totalDamageDealt );
		display->raw.totalDamageTaken = LittleLong( diskV1.totalDamageTaken );
		distanceMeters = LittleFloat( diskV1.totalDistanceMeters );
		fuelConsumed = LittleFloat( diskV1.totalFuelConsumed );
		display->raw.totalDistanceScaled = PlayerSettings_EncodeScaledFloat( distanceMeters, PROFILE_LIFETIME_DISTANCE_SCALE );
		display->raw.totalFuelConsumedScaled = PlayerSettings_EncodeScaledFloat( fuelConsumed, PROFILE_LIFETIME_FUEL_SCALE );
		display->raw.achievements = 0;
	} else {
		trap_FS_FCloseFile( file );
		return qfalse;
	}

	if ( display->raw.version != PROFILE_FILE_VERSION && display->raw.version != 2 && display->raw.version != 1 ) {
		display->identifier[0] = '\0';
		return qfalse;
	}

	display->raw.version = PROFILE_FILE_VERSION;
	display->totalDistanceMeters = distanceMeters;
	display->totalFuelConsumed = fuelConsumed;
	display->sequence = 0;
	display->valid = qtrue;
	Q_strncpyz( display->identifier, identifier, sizeof( display->identifier ) );
	return qtrue;
}


static void PlayerSettings_RegisterProfileCvars( void ) {
        static qboolean registered = qfalse;
        const int flags = CVAR_ARCHIVE;

        if ( registered ) {
                return;
        }

        registered = qtrue;

        trap_Cvar_Register( NULL, "ui_profile_identifier", "", flags );
        trap_Cvar_Register( NULL, "ui_profile_sequence", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_version", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_matches", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_wins", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_losses", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_finishes", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_dnfs", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_bestPosition", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_bestLapMs", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_bestTotalRaceMs", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_totalRaceTimeMs", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_totalScore", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_totalKills", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_totalDeaths", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_totalDamageDealt", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_totalDamageTaken", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_totalDistance", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_totalFuel", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_achievements", "0", flags );
        trap_Cvar_Register( NULL, "ui_profile_model", "", flags );
        trap_Cvar_Register( NULL, "ui_profile_head", "", flags );
        trap_Cvar_Register( NULL, "ui_profile_rim", "", flags );
        trap_Cvar_Register( NULL, "ui_profile_plate", "", flags );
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

static char *PlayerSettings_NextProfileToken( char **cursor ) {
        char *c;
        char *token;

        if ( !cursor || !*cursor ) {
                return NULL;
        }

        c = *cursor;

        while ( *c == ' ' ) {
                c++;
        }

        if ( !*c ) {
                *cursor = c;
                return NULL;
        }

        token = c;

        while ( *c && *c != ' ' ) {
                c++;
        }

        if ( *c ) {
                *c = '\0';
                c++;
        }

        *cursor = c;

        return token;
}

static void PlayerSettings_LoadProfileSlots( void ) {
        char buffer[MAX_STRING_CHARS];
        char *cursor;
        char *token;
        char currentProfileRaw[MAX_STRING_CHARS];
        char sanitizedCurrent[MAX_PROFILE_NAME_LENGTH];
        char varName[32];
        char value[MAX_QPATH];
        int i;

        s_playersettings.profileCount = 0;

        trap_Cvar_VariableStringBuffer( PROFILE_SLOTS_CVAR, buffer, sizeof( buffer ) );
        cursor = buffer;
        token = PlayerSettings_NextProfileToken( &cursor );
        while ( token ) {
                PlayerSettings_AddProfileSlot( token );
                if ( s_playersettings.profileCount >= MAX_PROFILE_SLOTS ) {
                        break;
                }
                token = PlayerSettings_NextProfileToken( &cursor );
        }

        for ( i = 0; i < UI_MAX_PROFILE_SLOTS && s_playersettings.profileCount < MAX_PROFILE_SLOTS; ++i ) {
                Com_sprintf( varName, sizeof( varName ), "ui_profileSlot%d", i );
                trap_Cvar_VariableStringBuffer( varName, value, sizeof( value ) );
                PlayerSettings_AddProfileSlot( value );
        }

        trap_Cvar_VariableStringBuffer( "cg_profile", currentProfileRaw, sizeof( currentProfileRaw ) );
        if ( !PlayerSettings_SanitizeProfileName( currentProfileRaw, sanitizedCurrent, sizeof( sanitizedCurrent ) ) ) {
                sanitizedCurrent[0] = '\0';
        }
        PlayerSettings_AddProfileSlot( sanitizedCurrent );

        PlayerSettings_BuildProfileItems();

        s_playersettings.profileList.curvalue = PlayerSettings_FindProfileIndex( sanitizedCurrent );
        if ( s_playersettings.profileList.curvalue < 0 ) {
                s_playersettings.profileList.curvalue = 0;
        }
        s_playersettings.profileList.oldvalue = s_playersettings.profileList.curvalue;
}

static void PlayerSettings_SetProfileCvars( const char *profile ) {
        char sanitized[MAX_PROFILE_NAME_LENGTH];

        if ( PlayerSettings_SanitizeProfileName( profile, sanitized, sizeof( sanitized ) ) ) {
                trap_Cvar_Set( "cg_profile", sanitized );
                trap_Cvar_Set( "profile", sanitized );
                trap_Cvar_Set( "ui_profileSelected", sanitized );
        } else {
                trap_Cvar_Set( "cg_profile", "" );
                trap_Cvar_Set( "profile", "" );
                trap_Cvar_Set( "ui_profileSelected", "" );
        }

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
        PlayerSettings_SetMenuItemVisible( &s_playersettings.profileRefresh.generic, showStats );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.profileList.generic, qfalse );
        PlayerSettings_SetMenuItemVisible( &s_playersettings.profileNameLabel.generic, qfalse );

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
        char identifier[MAX_QPATH];
        int sequence;
        qboolean haveIdentifier;

        PlayerSettings_RegisterProfileCvars();

        PlayerSettings_RegisterProfileCvars();

        display = &s_playersettings.lifetimeDisplay;

        sequence = (int)trap_Cvar_VariableValue( "ui_profile_sequence" );
        haveIdentifier = PlayerSettings_BuildProfileIdentifier( identifier, sizeof( identifier ) );

        if ( sequence > 0 ) {
                if ( display->valid && display->sequence == sequence ) {
                        return;
                }
        } else if ( display->valid && haveIdentifier && !Q_stricmp( display->identifier, identifier ) ) {
                return;
        }

        display->sequence = sequence;
        display->valid = qfalse;
        PlayerSettings_UpdateVehicleCvarsFromLifetime( NULL );

        if ( sequence > 0 ) {
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

                trap_Cvar_VariableStringBuffer( "ui_profile_model", display->raw.vehicleModel, sizeof( display->raw.vehicleModel ) );
                trap_Cvar_VariableStringBuffer( "ui_profile_head", display->raw.vehicleHead, sizeof( display->raw.vehicleHead ) );
                trap_Cvar_VariableStringBuffer( "ui_profile_rim", display->raw.vehicleRim, sizeof( display->raw.vehicleRim ) );
                trap_Cvar_VariableStringBuffer( "ui_profile_plate", display->raw.vehiclePlate, sizeof( display->raw.vehiclePlate ) );

                trap_Cvar_VariableStringBuffer( "ui_profile_totalDistance", buffer, sizeof( buffer ) );
                display->totalDistanceMeters = atof( buffer );

                trap_Cvar_VariableStringBuffer( "ui_profile_totalFuel", buffer, sizeof( buffer ) );
                display->totalFuelConsumed = atof( buffer );

                if ( display->raw.version > 0 || sequence > 0 ) {
                        display->valid = qtrue;
                        display->identifier[0] = '\0';
                        PlayerSettings_UpdateVehicleCvarsFromLifetime( &display->raw );
                        return;
                }
        }

        if ( haveIdentifier && PlayerSettings_LoadLifetimeFromProfile( display, identifier ) ) {
                PlayerSettings_UpdateVehicleCvarsFromLifetime( &display->raw );
                return;
        }

        display->identifier[0] = '\0';
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
        qboolean unlockedAny = qfalse;
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

        for ( i = 0; i < ACH_MAX; i++ ) {
            qboolean unlocked = cg_profile.achievements[i].unlocked;
            if (s_playersettings.achievements_filter.curvalue == 1 && !unlocked) continue;
            if (s_playersettings.achievements_filter.curvalue == 2 && unlocked) continue;

            float *color = unlocked ? color_white : text_color_disabled;
            UI_DrawString( x, y, s_playersettings.achievements_list[i].string, UI_LEFT | UI_SMALLFONT, color );
            y += SMALLCHAR_HEIGHT + 4;
            unlockedAny = unlockedAny || unlocked;
        }

        if ( !unlockedAny ) {
                UI_DrawString( x, y, "No achievements unlocked yet", UI_LEFT | UI_SMALLFONT, text_color_disabled );
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
	focus = (f->generic.parent->cursor == f->generic.menuPosition) && !(f->generic.flags & QMF_INACTIVE);

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

	UI_DrawProportionalString( basex + 16, y, "AKTIVE PROFILE", style, color );
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

static int PlayerSettings_FindPaintId(const char* paintName) {
    char cleanPaintName[MAX_QPATH];
    Q_strncpyz(cleanPaintName, paintName, sizeof(cleanPaintName));
    COM_StripExtension(cleanPaintName, cleanPaintName, sizeof(cleanPaintName));

    for (int i = 0; i < s_playersettings.numModels; i++) {
        if (Q_stricmp(s_playersettings.modelList[i], cleanPaintName) == 0) {
            return i;
        }
    }
    return -1;
}

static int PlayerSettings_FindRimId(const char* rimName) {
    char cleanRimName[MAX_QPATH];
    Q_strncpyz(cleanRimName, rimName, sizeof(cleanRimName));
    COM_StripExtension(cleanRimName, cleanRimName, sizeof(cleanRimName));

    for (int i = 0; i < s_playersettings.numRims; i++) {
        if (Q_stricmp(s_playersettings.rimList[i], cleanRimName) == 0) {
            return i;
        }
    }
    return -1;
}

static void PlayerSettings_SaveFavorite(int favoriteIndex) {
    if (favoriteIndex < 0 || favoriteIndex >= Q3R_NUM_FAVORITE_SLOTS) {
        return;
    }

    q3r_favorite_slot_t* slot = &cg_profile.favoriteSlots[favoriteIndex];
    char currentPaint[MAX_QPATH];
    char currentRim[MAX_QPATH];
    char* slash;

    slot->carId = s_playersettings.selectedModel;

    trap_Cvar_VariableStringBuffer("model", currentPaint, sizeof(currentPaint));
    slash = strrchr(currentPaint, '/');
    if (slash) {
        slot->paintId = PlayerSettings_FindPaintId(slash + 1);
    } else {
        slot->paintId = -1;
    }

    trap_Cvar_VariableStringBuffer("rim", currentRim, sizeof(currentRim));
    slot->wheelId = PlayerSettings_FindRimId(currentRim);
    slot->tuningId = 0;

    Q3R_Profile_Save(&cg_profile);
    PlayerSettings_UpdateFavorites();
}

static void PlayerSettings_LoadFavorite(int favoriteIndex) {
    if (favoriteIndex < 0 || favoriteIndex >= Q3R_NUM_FAVORITE_SLOTS) {
        return;
    }

    q3r_favorite_slot_t* slot = &cg_profile.favoriteSlots[favoriteIndex];
    if (slot->carId < 0 || slot->carId >= s_playersettings.allModels) {
        return;
    }

    s_playersettings.selectedModel = slot->carId;
    s_playersettings.modelname.string = s_playersettings.modelList[s_playersettings.selectedModel];

    if (slot->paintId >= 0 && slot->paintId < s_playersettings.numModels) {
        Com_sprintf(s_playersettings.modelskin, sizeof(s_playersettings.modelskin), "%s/%s", s_playersettings.modelList[s_playersettings.selectedModel], s_playersettings.modelList[slot->paintId]);
    } else {
        Com_sprintf(s_playersettings.modelskin, sizeof(s_playersettings.modelskin), "%s/%s", s_playersettings.modelList[s_playersettings.selectedModel], DEFAULT_SKIN);
    }

    if (slot->wheelId >= 0 && slot->wheelId < s_playersettings.numRims) {
        trap_Cvar_Set("rim", s_playersettings.rimList[slot->wheelId]);
    }

    s_playersettings.modelChanged = qtrue;
    PlayerSettings_UpdateModel();
}


/*
=================
PlayerSettings_UpdateFavorites

=================
*/
static void PlayerSettings_UpdateFavorites( void ) {
    for (int i = 0; i < Q3R_NUM_FAVORITE_SLOTS; i++) {
        q3r_favorite_slot_t* slot = &cg_profile.favoriteSlots[i];
        if (slot->carId >= 0 && slot->carId < s_playersettings.allModels) {
            const char* carModel = s_playersettings.modelList[slot->carId];

            // Re-build the paint list for the specific car model of the favorite slot
            int numPaints = UI_BuildFileList(va("models/players/%s", carModel), "skin", "", qtrue, qfalse, qtrue, 0, s_playersettings.modelList);

            const char* paintName = (slot->paintId >= 0 && slot->paintId < numPaints) ? s_playersettings.modelList[slot->paintId] : DEFAULT_SKIN;

            Com_sprintf(s_playersettings.favIcons[i], sizeof(s_playersettings.favIcons[i]),
                        "models/players/%s/icon_%s", carModel, paintName);

            s_playersettings.favpics[i].generic.name = s_playersettings.favIcons[i];
            s_playersettings.favpicbuttons[i].generic.flags &= ~QMF_INACTIVE;
        } else {
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

	// All changes to favorite cars are stored directly in the cg_profile struct,
	// so we just need to save it to disk.
	Q3R_Profile_Save(&cg_profile);
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

	// The favorite car data is already in cg_profile, which is loaded when the profile is.
	// No extra loading is needed here. The UI will read from it directly.

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

    int favoriteIndex = ((menucommon_s*)ptr)->id - ID_FAVORITE1;
    if (favoriteIndex < 0 || favoriteIndex >= Q3R_NUM_FAVORITE_SLOTS) {
        return;
    }

    if (trap_Key_IsDown( K_SHIFT )) {
        PlayerSettings_SaveFavorite(favoriteIndex);
    } else {
        PlayerSettings_LoadFavorite(favoriteIndex);
    }
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
                PlayerSettings_SelectProfileByIndex( s_playersettings.profileList.curvalue );
                break;

	case ID_PROFILE_REFRESH:
                {
                        uiClientState_t cs;
                        char identifier[MAX_QPATH];

                        s_playersettings.lifetimeDisplay.valid = qfalse;
                        s_playersettings.lifetimeDisplay.sequence = -1;
                        s_playersettings.lifetimeDisplay.identifier[0] = '\0';

                        trap_GetClientState( &cs );
                        if ( cs.connState >= CA_CONNECTED ) {
                                trap_Cmd_ExecuteText( EXEC_APPEND, "profileRequest\n" );
                        } else if ( PlayerSettings_BuildProfileIdentifier( identifier, sizeof( identifier ) ) ) {
                                PlayerSettings_LoadLifetimeFromProfile( &s_playersettings.lifetimeDisplay, identifier );
                        }
                }
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
	s_playersettings.profileRefresh.color = uis.text_color;

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
	s_playersettings.numRims = UI_BuildFileList("models/players/wheels", "skin", "", qtrue, qfalse, qtrue, 0, s_playersettings.rimList);
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
	s_playersettings.banner.string        = "PLAYER PROFILE";
// STONELANCE
	s_playersettings.banner.color         = text_color_normal;
// END
	s_playersettings.banner.style         = UI_CENTER;

    s_playersettings.stats_races_started.generic.type = MTYPE_PTEXT;
    s_playersettings.stats_races_started.generic.flags = QMF_LEFT_JUSTIFY;
    s_playersettings.stats_races_started.generic.x = 100;
    s_playersettings.stats_races_started.generic.y = 100;
    s_playersettings.stats_races_started.string = va("Races Started: %d", cg_profile.totalRacesStarted);
    s_playersettings.stats_races_started.style = UI_LEFT | UI_SMALLFONT;
    s_playersettings.stats_races_started.color = color_white;

    s_playersettings.stats_races_finished.generic.type = MTYPE_PTEXT;
    s_playersettings.stats_races_finished.generic.flags = QMF_LEFT_JUSTIFY;
    s_playersettings.stats_races_finished.generic.x = 100;
    s_playersettings.stats_races_finished.generic.y = 120;
    s_playersettings.stats_races_finished.string = va("Races Finished: %d", cg_profile.totalRacesFinished);
    s_playersettings.stats_races_finished.style = UI_LEFT | UI_SMALLFONT;
    s_playersettings.stats_races_finished.color = color_white;

    s_playersettings.stats_races_won.generic.type = MTYPE_PTEXT;
    s_playersettings.stats_races_won.generic.flags = QMF_LEFT_JUSTIFY;
    s_playersettings.stats_races_won.generic.x = 100;
    s_playersettings.stats_races_won.generic.y = 140;
    s_playersettings.stats_races_won.string = va("Races Won: %d", cg_profile.totalRacesWon);
    s_playersettings.stats_races_won.style = UI_LEFT | UI_SMALLFONT;
    s_playersettings.stats_races_won.color = color_white;

    s_playersettings.stats_derby_matches.generic.type = MTYPE_PTEXT;
    s_playersettings.stats_derby_matches.generic.flags = QMF_LEFT_JUSTIFY;
    s_playersettings.stats_derby_matches.generic.x = 100;
    s_playersettings.stats_derby_matches.generic.y = 160;
    s_playersettings.stats_derby_matches.string = va("Derby Matches: %d", cg_profile.totalDerbyMatches);
    s_playersettings.stats_derby_matches.style = UI_LEFT | UI_SMALLFONT;
    s_playersettings.stats_derby_matches.color = color_white;

    s_playersettings.stats_derby_wins.generic.type = MTYPE_PTEXT;
    s_playersettings.stats_derby_wins.generic.flags = QMF_LEFT_JUSTIFY;
    s_playersettings.stats_derby_wins.generic.x = 100;
    s_playersettings.stats_derby_wins.generic.y = 180;
    s_playersettings.stats_derby_wins.string = va("Derby Wins: %d", cg_profile.totalDerbyWins);
    s_playersettings.stats_derby_wins.style = UI_LEFT | UI_SMALLFONT;
    s_playersettings.stats_derby_wins.color = color_white;

    s_playersettings.stats_play_time.generic.type = MTYPE_PTEXT;
    s_playersettings.stats_play_time.generic.flags = QMF_LEFT_JUSTIFY;
    s_playersettings.stats_play_time.generic.x = 100;
    s_playersettings.stats_play_time.generic.y = 200;
    s_playersettings.stats_play_time.string = va("Play Time: %d hours", cg_profile.totalPlayTimeSeconds / 3600);
    s_playersettings.stats_play_time.style = UI_LEFT | UI_SMALLFONT;
    s_playersettings.stats_play_time.color = color_white;

    s_playersettings.stats_distance_driven.generic.type = MTYPE_PTEXT;
    s_playersettings.stats_distance_driven.generic.flags = QMF_LEFT_JUSTIFY;
    s_playersettings.stats_distance_driven.generic.x = 100;
    s_playersettings.stats_distance_driven.generic.y = 220;
    s_playersettings.stats_distance_driven.string = va("Distance Driven: %.2f km", cg_profile.totalDistanceMeters / 1000.0);
    s_playersettings.stats_distance_driven.style = UI_LEFT | UI_SMALLFONT;
    s_playersettings.stats_distance_driven.color = color_white;

    static const char *achievement_filter_items[] = {"All", "Unlocked", "Locked", 0};
    s_playersettings.achievements_filter.generic.type = MTYPE_SPINCONTROL;
    s_playersettings.achievements_filter.generic.flags = QMF_NODEFAULTINIT;
    s_playersettings.achievements_filter.generic.x = 100;
    s_playersettings.achievements_filter.generic.y = 80;
    s_playersettings.achievements_filter.itemnames = achievement_filter_items;
    s_playersettings.achievements_filter.numitems = 3;

    for (int i = 0; i < ACH_MAX; i++) {
        s_playersettings.achievements_list[i].generic.type = MTYPE_PTEXT;
        s_playersettings.achievements_list[i].generic.flags = QMF_LEFT_JUSTIFY;
        s_playersettings.achievements_list[i].generic.x = 100;
        s_playersettings.achievements_list[i].generic.y = 100 + i * 20;

        if (achievement_defs[i].type == ACH_TYPE_PROGRESS) {
            s_playersettings.achievements_list[i].string = va("%s: %.0f/%.0f", achievement_defs[i].title, cg_profile.achievements[i].progress, achievement_defs[i].target);
        } else {
            s_playersettings.achievements_list[i].string = va("%s: %s", achievement_defs[i].title, cg_profile.achievements[i].unlocked ? "Unlocked" : "Locked");
        }

        s_playersettings.achievements_list[i].style = UI_LEFT | UI_SMALLFONT;
        s_playersettings.achievements_list[i].color = cg_profile.achievements[i].unlocked ? color_white : text_color_disabled;
    }

    s_playersettings.player_name.generic.type = MTYPE_PTEXT;
    s_playersettings.player_name.generic.flags = QMF_LEFT_JUSTIFY;
    s_playersettings.player_name.generic.x = 100;
    s_playersettings.player_name.generic.y = 80;
    s_playersettings.player_name.string = va("Player Name: %s", cg_profile.playerName);
    s_playersettings.player_name.style = UI_LEFT | UI_SMALLFONT;
    s_playersettings.player_name.color = color_white;

    s_playersettings.tabs[0].generic.type = MTYPE_PTEXT;
    s_playersettings.tabs[0].generic.flags = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_playersettings.tabs[0].generic.x = 100;
    s_playersettings.tabs[0].generic.y = 60;
    s_playersettings.tabs[0].generic.id = ID_TAB_CAR;
    s_playersettings.tabs[0].generic.callback = PlayerSettings_TabEvent;
    s_playersettings.tabs[0].string = "Vehicle & Player";
    s_playersettings.tabs[0].style = UI_LEFT | UI_SMALLFONT;

    s_playersettings.tabs[1].generic.type = MTYPE_PTEXT;
    s_playersettings.tabs[1].generic.flags = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_playersettings.tabs[1].generic.x = 250;
    s_playersettings.tabs[1].generic.y = 60;
    s_playersettings.tabs[1].generic.id = ID_TAB_STATS;
    s_playersettings.tabs[1].generic.callback = PlayerSettings_TabEvent;
    s_playersettings.tabs[1].string = "Stats";
    s_playersettings.tabs[1].style = UI_LEFT | UI_SMALLFONT;

    s_playersettings.tabs[2].generic.type = MTYPE_PTEXT;
    s_playersettings.tabs[2].generic.flags = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
    s_playersettings.tabs[2].generic.x = 350;
    s_playersettings.tabs[2].generic.y = 60;
    s_playersettings.tabs[2].generic.id = ID_TAB_ACHIEVEMENTS;
    s_playersettings.tabs[2].generic.callback = PlayerSettings_TabEvent;
    s_playersettings.tabs[2].string = "Achievements";
    s_playersettings.tabs[2].style = UI_LEFT | UI_SMALLFONT;

	{
		static const int tabIds[PLAYERSETTINGS_NUM_TABS] = { ID_TAB_CAR, ID_TAB_STATS, ID_TAB_ACHIEVEMENTS };
		static char tabTexts[PLAYERSETTINGS_NUM_TABS][16] = { "CAR", "STATS", "ACHIEVEMENTS" };
		int tab;
		int tabX = 64;
                int tabY = 48;
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
	s_playersettings.name.generic.flags			= QMF_NODEFAULTINIT | QMF_INACTIVE;
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

	s_playersettings.profileRefresh.generic.type = MTYPE_PTEXT;
	s_playersettings.profileRefresh.generic.flags = QMF_RIGHT_JUSTIFY|QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_playersettings.profileRefresh.generic.x = s_playersettings.statsPanel.generic.x + 240;
	s_playersettings.profileRefresh.generic.y = s_playersettings.statsPanel.generic.y - ( SMALLCHAR_HEIGHT + 4 );
	s_playersettings.profileRefresh.generic.left = s_playersettings.statsPanel.generic.x;
	s_playersettings.profileRefresh.generic.top = s_playersettings.profileRefresh.generic.y - 2;
	s_playersettings.profileRefresh.generic.right = s_playersettings.statsPanel.generic.x + 240;
	s_playersettings.profileRefresh.generic.bottom = s_playersettings.profileRefresh.generic.y + SMALLCHAR_HEIGHT + 2;
	s_playersettings.profileRefresh.generic.id = ID_PROFILE_REFRESH;
	s_playersettings.profileRefresh.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.profileRefresh.string = "REFRESH STATS";
	s_playersettings.profileRefresh.color = text_color_normal;
	s_playersettings.profileRefresh.style = UI_RIGHT|UI_SMALLFONT;

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
	s_playersettings.profileNameLabel.generic.flags = QMF_LEFT_JUSTIFY|QMF_INACTIVE|QMF_HIDDEN;
	s_playersettings.profileNameLabel.generic.x = 360;
	s_playersettings.profileNameLabel.generic.y = 140;
	s_playersettings.profileNameLabel.generic.left = 360;
	s_playersettings.profileNameLabel.generic.top = 140;
	s_playersettings.profileNameLabel.generic.right = 360 + 220;
	s_playersettings.profileNameLabel.generic.bottom = 140 + SMALLCHAR_HEIGHT;
	s_playersettings.profileNameLabel.string = "AKTIVE PROFILE";
	s_playersettings.profileNameLabel.style = UI_LEFT|UI_SMALLFONT;
	s_playersettings.profileNameLabel.color = text_color_normal;

	s_playersettings.profileList.generic.type = MTYPE_SPINCONTROL;
	s_playersettings.profileList.generic.flags = QMF_PULSEIFFOCUS|QMF_SMALLFONT|QMF_INACTIVE|QMF_HIDDEN;
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
	s_playersettings.favorites.string	      = "FAVORITES (SHIFT+CLICK TO SAVE)";
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
    for (int i = 0; i < NUM_TABS; i++) {
        Menu_AddItem(&s_playersettings.menu, (void*)&s_playersettings.tabs[i]);
    }

    PlayerSettings_SetTab(TAB_VEHICLE);
// STONELANCE
/*
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.framel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.framer );
*/
// END

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.name );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.handicap );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.effects );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.profileRefresh );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.statsPanel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.profileNameLabel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.profileList );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.achievementsPanel );

    Menu_AddItem(&s_playersettings.menu, (void*)&s_playersettings.stats_races_started);
    Menu_AddItem(&s_playersettings.menu, (void*)&s_playersettings.stats_races_finished);
    Menu_AddItem(&s_playersettings.menu, (void*)&s_playersettings.stats_races_won);
    Menu_AddItem(&s_playersettings.menu, (void*)&s_playersettings.stats_derby_matches);
    Menu_AddItem(&s_playersettings.menu, (void*)&s_playersettings.stats_derby_wins);
    Menu_AddItem(&s_playersettings.menu, (void*)&s_playersettings.stats_play_time);
    Menu_AddItem(&s_playersettings.menu, (void*)&s_playersettings.stats_distance_driven);

    Menu_AddItem(&s_playersettings.menu, (void*)&s_playersettings.achievements_filter);
    for (int i = 0; i < ACH_MAX; i++) {
        Menu_AddItem(&s_playersettings.menu, (void*)&s_playersettings.achievements_list[i]);
    }

    Menu_AddItem(&s_playersettings.menu, (void*)&s_playersettings.player_name);

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
        s_playersettingsInitialTab = PLAYERSETTINGS_TAB_CAR;
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
