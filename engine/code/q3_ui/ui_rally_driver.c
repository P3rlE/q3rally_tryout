/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The driver page drawn like the WebUI's #driver-screen (driver.js): the
overview, the stats and the achievements, the editor and the profile dialog.
It reads what the page was fed (the active profile, UI_WebDriverValue,
UI_WebAchievementChar) and changes profiles through UI_WebDriverAction with
ui_webdriver_payload, as the page did.
*/

#include "ui_local.h"
#include "ui_rally_driver.h"
#include "ui_rally_driverlayout.h"
#include "ui_rally_library.h"
#include "ui_rally_rivalslayout.h"

#define DRIVER_KEY_MOST_USED	18	/* driverStatKeys: "mostUsedVehicle" */

static double Driver_StatNumber( const profile_stats_t *s, int key ) {
	switch ( key ) {
	case 0: return s->gamesPlayed;
	case 1: return s->wins;
	case 2: return s->losses;
	case 3: return s->playerScore;
	case 4: return s->distanceKm;
	case 5: return s->fuelUsed;
	case 6: return s->topSpeedKph;
	case 7: return s->bestLapMs;
	case 8: return s->kills;
	case 9: return s->deaths;
	case 10: return s->damageDealt;
	case 11: return s->damageTaken;
	case 12: return s->flagCaptures;
	case 13: return s->flagAssists;
	case 14: return s->accuracyAwards;
	case 15: return s->excellentAwards;
	case 16: return s->impressiveAwards;
	case 17: return s->perfectAwards;
	case 19: return s->mostUsedVehicleTimeMs;
	case 20: return s->racingWins;
	case 21: return s->racingPodiums;
	case 22: return s->racingCompleted;
	case 23: return s->racingTotalMs;
	case 24: return s->racingDmWins;
	case 25: return s->racingDmPodiums;
	case 26: return s->racingDmCompleted;
	case 27: return s->racingDmTotalMs;
	case 28: return s->sprintWins;
	case 29: return s->sprintCompleted;
	case 30: return s->sprintBestMs;
	case 31: return s->eliminationWins;
	case 32: return s->eliminationCompleted;
	case 33: return s->eliminationTotalRoundsLasted;
	case 34: return s->lcsWins;
	case 35: return s->lcsCompleted;
	case 36: return s->lcsTotalSurvivalMs;
	case 37: return s->derbyWins;
	case 38: return s->derbyCompleted;
	case 39: return s->derbyKills;
	case 40: return s->dmWins;
	case 41: return s->dmCompleted;
	case 42: return s->dmKills;
	case 43: return s->ctfWins;
	case 44: return s->ctfCompleted;
	case 45: return s->ctfCaptures;
	case 46: return s->ctf4Wins;
	case 47: return s->ctf4Completed;
	case 48: return s->ctf4Captures;
	case 49: return s->teamWins;
	case 50: return s->teamCompleted;
	case 51: return s->teamKills;
	case 52: return s->teamRacingWins;
	case 53: return s->teamRacingCompleted;
	case 54: return s->teamRacingPodiums;
	case 55: return s->teamRacingDmWins;
	case 56: return s->teamRacingDmCompleted;
	case 57: return s->teamRacingDmPodiums;
	case 58: return s->dominationWins;
	case 59: return s->dominationCompleted;
	case 60: return s->dominationZoneHoldMs;
	case 61: return s->kothWins;
	case 62: return s->kothCompleted;
	case 63: return s->kothZoneHoldMs;
	case 64: return s->autoballWins;
	case 65: return s->autoballCompleted;
	case 66: return s->autoballGoals;
	}
	return 0.0;
}

/* a number as JSON writes it: no fraction for whole numbers, else up to six digits */
static const char *Driver_JsonNumber( double value ) {
	static char text[64];
	char *end;
	if ( value == (double)(int)value ) return va( "%d", (int)value );
	Com_sprintf( text, sizeof( text ), "%.6f", value );
	end = text + strlen( text );
	while ( end > text && end[-1] == '0' ) *--end = '\0';
	return text;
}

static void Driver_Quoted( const char *text ) {
	char out[1024];
	int n = 0;
	out[n++] = '"';
	for ( ; *text && n < (int)sizeof( out ) - 3; text++ ) {
		if ( *text == '"' || *text == '\\' ) out[n++] = '\\';
		out[n++] = *text;
	}
	out[n++] = '"';
	out[n] = '\0';
	Com_Printf( "%s", out );
}

static void Driver_AchievementText( int category, int tier, int field, char *out, int size ) {
	int i;
	for ( i = 0; i < size - 1; i++ ) {
		out[i] = (char)UI_WebAchievementChar( category, tier, field, i );
		if ( !out[i] ) break;
	}
	out[i] = '\0';
}

/* the profiles on disk, sorted (profiles/<name>.json) */
static int Driver_Available( char names[][64], int max ) {
	static char list[16384];
	char *name;
	int count = trap_FS_GetFileList( "profiles", ".json", list, sizeof( list ) ), i, n = 0, j;
	for ( i = 0, name = list; i < count && n < max; i++, name += strlen( name ) + 1 ) {
		int length = strlen( name );
		if ( length < 6 || length >= 64 ) continue;
		COM_StripExtension( name, names[n], 64 );
		for ( j = n; j > 0 && Q_stricmp( names[j - 1], names[j] ) > 0; j-- ) {
			char swap[64];
			Q_strncpyz( swap, names[j], sizeof( swap ) );
			Q_strncpyz( names[j], names[j - 1], 64 );
			Q_strncpyz( names[j - 1], swap, 64 );
		}
		n++;
	}
	return n;
}

/* what the page got, as JSON, for tests/fixtures/driver_model.json */
void Driver_Dump( void ) {
	static char names[64][64];
	const profile_stats_t *stats = UI_Profile_GetActiveStats();
	const profile_info_t *info = UI_Profile_GetActiveInfo();
	char text[512];
	int i, j, count;

	Com_Printf( "DRIVERMODEL {\"available\":[" );
	count = Driver_Available( names, 64 );
	for ( i = 0; i < count; i++ ) {
		if ( i ) Com_Printf( "," );
		Driver_Quoted( names[i] );
	}
	Com_Printf( "],\"profile\":" );
	if ( UI_WebDriverValue( 0, 0 ) && info && stats ) {
		Com_Printf( "{\"identity\":{\"name\":" );
		Driver_Quoted( UI_Profile_GetActiveName() );
		Com_Printf( ",\"uuid\":" );
		Driver_Quoted( info->uuid );
		Com_Printf( ",\"info\":{\"gender\":" );
		Driver_Quoted( info->gender );
		Com_Printf( ",\"birthDate\":" );
		Driver_Quoted( info->birthDate );
		Com_Printf( ",\"country\":" );
		Driver_Quoted( info->country );
		Com_Printf( ",\"avatar\":" );
		Driver_Quoted( info->avatar );
		Com_Printf( "}},\"progress\":{\"stats\":{" );
		for ( i = 0; i < DRIVER_STATS; i++ ) {
			Com_Printf( "%s\"%s\":", i ? "," : "", driverStatKeys[i] );
			if ( !strcmp( driverStatKeys[i], "mostUsedVehicle" ) ) Driver_Quoted( stats->mostUsedVehicle );
			else Com_Printf( "%s", Driver_JsonNumber( Driver_StatNumber( stats, i ) ) );
		}
		Com_Printf( "}}}" );
	} else {
		Com_Printf( "null" );
	}
	Com_Printf( ",\"rank\":[%d,%d,%d],\"categories\":[", UI_WebDriverValue( 1, 0 ), UI_WebDriverValue( 2, 0 ), UI_WebDriverValue( 3, 0 ) );
	for ( i = 0; i < UI_WebDriverValue( 4, 0 ); i++ ) {
		Com_Printf( "%s[", i ? "," : "" );
		Driver_AchievementText( i, -1, 0, text, sizeof( text ) );
		Driver_Quoted( text );
		Driver_AchievementText( i, -1, 4, text, sizeof( text ) );
		Com_Printf( ",%s,[", text[0] ? text : "0" );
		for ( j = 0; j < UI_WebDriverValue( 5, i ); j++ ) {
			int field;
			Com_Printf( "%s[", j ? "," : "" );
			for ( field = 1; field < 7; field++ ) {
				Driver_AchievementText( i, j, field, text, sizeof( text ) );
				if ( field == 4 ) continue;
				if ( field > 1 ) Com_Printf( "," );
				Driver_Quoted( text );
			}
			Com_Printf( "]" );
		}
		Com_Printf( "]]" );
	}
	Com_Printf( "],\"tab\":%d}\n", UI_WebDriverValue( 6, 0 ) );
}

/* -------------------------------------------------------------------------
   The model, loaded when the page opens and after every change
   ------------------------------------------------------------------------- */

static const char *driverCountries[][2] = {
	{ "", "Not specified" }, { "AT", "Austria" }, { "AU", "Australia" }, { "BE", "Belgium" }, { "BR", "Brazil" }, { "CA", "Canada" },
	{ "CH", "Switzerland" }, { "CN", "China" }, { "CZ", "Czech Republic" }, { "DE", "Germany" }, { "DK", "Denmark" }, { "ES", "Spain" },
	{ "FI", "Finland" }, { "FR", "France" }, { "GB", "United Kingdom" }, { "GR", "Greece" }, { "HU", "Hungary" }, { "ID", "Indonesia" },
	{ "IN", "India" }, { "IT", "Italy" }, { "JP", "Japan" }, { "KR", "South Korea" }, { "MX", "Mexico" }, { "NL", "Netherlands" },
	{ "NO", "Norway" }, { "NZ", "New Zealand" }, { "PL", "Poland" }, { "PT", "Portugal" }, { "RO", "Romania" }, { "RU", "Russia" },
	{ "SE", "Sweden" }, { "SG", "Singapore" }, { "SK", "Slovakia" }, { "TH", "Thailand" }, { "TR", "Turkey" }, { "TW", "Taiwan" },
	{ "UA", "Ukraine" }, { "US", "United States" }, { "VN", "Vietnam" }, { "ZA", "South Africa" }, { "AR", "Argentina" }, { "CL", "Chile" }
};
#define DRIVER_COUNTRIES	( sizeof( driverCountries ) / sizeof( driverCountries[0] ) )
static const char *driverGenders[5] = { "", "Female", "Male", "Non-binary", "Other" };

typedef enum { POPUP_NONE, POPUP_FILTER, POPUP_GENDER, POPUP_COUNTRY } driverPopup_t;
typedef enum { FOCUS_NONE, FOCUS_BIRTH, FOCUS_NAME } driverFocus_t;
typedef enum { DRAG_NONE, DRAG_PAGE, DRAG_DIALOG, DRAG_LIST } driverDrag_t;

static struct {
	driverLayout_t	layout;
	menuframework_s	*menu;
	/* the model */
	qboolean		profile;
	profile_info_t	info;
	char			name[64], rankName[64], country[80];
	int				rank[3];
	char			overview[4][32], progressText[64], stats[DRIVER_STATS][32];
	float			progress;
	int				medals, earnedCount;
	char			medalLabel[DRIVER_MAX_MEDALS][40], medalName[DRIVER_MAX_MEDALS][48], medalText[DRIVER_MAX_MEDALS][96];
	char			medalStatus[DRIVER_MAX_MEDALS][48], medalIcon[DRIVER_MAX_MEDALS][64];
	int				medalCategory[DRIVER_MAX_MEDALS];
	qboolean		medalEarned[DRIVER_MAX_MEDALS];
	float			medalProgress[DRIVER_MAX_MEDALS];
	int				categories;
	char			categoryTitle[16][40];
	char			count[48];
	char			available[DRIVER_MAX_PROFILES][64], profileText[DRIVER_MAX_PROFILES][80];
	int				profiles;
	char			avatars[DRIVER_AVATARS][64];
	int				avatarCount;
	int				countryOrder[DRIVER_COUNTRIES];	/* "" first, then by name */
	/* the page */
	int				tab, filter;					/* filter: a category, -1 all */
	float			pageScroll, dialogScroll, listScroll;
	driverDrag_t	drag;
	float			dragY, dragScroll;
	driverDialog_t	dialog;
	driverPopup_t	popup;
	float			popupScroll;
	wsRect_t		popupRect;						/* where it was drawn, for the clicks */
	driverFocus_t	focus;
	int				focusTime;
	/* the editor */
	int				gender, countryIndex;
	char			birth[9];						/* the date field's digits, mmddyyyy */
	char			avatar[64];
	char			newName[32];
	char			status[128];
	qboolean		pending;
	char			profileLine[80];
	char			filterText[48];
	qboolean		dumped;
} s_driver;

/* toLocaleString( 'en-US', { maximumFractionDigits: 1 } ) */
static void Driver_Format( double value, char *out, int size ) {
	char digits[32], grouped[48];
	int tenths, whole, fraction, length, i, n = 0;
	qboolean negative = value < 0.0;
	if ( negative ) value = -value;
	tenths = (int)( value * 10.0 + 0.5 );
	whole = tenths / 10;
	fraction = tenths % 10;
	Com_sprintf( digits, sizeof( digits ), "%d", whole );
	length = strlen( digits );
	if ( negative ) grouped[n++] = '-';
	for ( i = 0; i < length; i++ ) {
		if ( i && ( length - i ) % 3 == 0 ) grouped[n++] = ',';
		grouped[n++] = digits[i];
	}
	grouped[n] = '\0';
	if ( fraction ) Com_sprintf( out, size, "%s.%d", grouped, fraction );
	else Q_strncpyz( out, grouped, size );
}

/* statValue(): the em dash for no time is drawn as a bar (the fonts are Latin-1) */
#define DRIVER_EMDASH	"\x97"

static void Driver_Stat( const profile_stats_t *stats, int key, char *out, int size ) {
	const char *name = driverStatKeys[key];
	double value = Driver_StatNumber( stats, key );
	int length = strlen( name );
	if ( key == DRIVER_KEY_MOST_USED ) {
		Q_strncpyz( out, stats->mostUsedVehicle[0] ? stats->mostUsedVehicle : "None yet", size );
	} else if ( length > 2 && !strcmp( name + length - 2, "Ms" ) ) {
		int ms = value > 0.0 ? (int)value : 0;
		if ( !ms ) Q_strncpyz( out, DRIVER_EMDASH, size );
		else Com_sprintf( out, size, "%d:%02d.%03d", ms / 60000, ( ms / 1000 ) % 60, ms % 1000 );
	} else {
		char number[32];
		Driver_Format( value, number, sizeof( number ) );
		Com_sprintf( out, size, "%s%s", number, !strcmp( name, "distanceKm" ) ? " km" : !strcmp( name, "fuelUsed" ) ? " l" :
		             !strcmp( name, "topSpeedKph" ) ? " km/h" : "" );
	}
}

static const char *Driver_CountryName( const char *code ) {
	int i;
	for ( i = 0; i < (int)DRIVER_COUNTRIES; i++ ) if ( !Q_stricmp( driverCountries[i][0], code ) ) return driverCountries[i][1];
	return code;
}

static void Driver_Load( void ) {
	static profile_stats_t empty;
	const profile_stats_t *stats = UI_Profile_GetActiveStats();
	const profile_info_t *info = UI_Profile_GetActiveInfo();
	char text[256], number[32], number2[32];
	int i, j, k;
	double progress;

	s_driver.profile = UI_WebDriverValue( 0, 0 ) && info && stats;
	if ( !s_driver.profile ) stats = &empty;
	if ( s_driver.profile ) s_driver.info = *info;
	else memset( &s_driver.info, 0, sizeof( s_driver.info ) );
	for ( i = 0; i < 3; i++ ) s_driver.rank[i] = UI_WebDriverValue( 1 + i, 0 );
	if ( s_driver.profile ) {
		profile_rank_t rank;
		const char *name = UI_Profile_GetActiveName();
		int n = 0;
		for ( ; *name && n < (int)sizeof( s_driver.name ) - 1; name++ ) {
			if ( *name == '^' && name[1] >= '0' && name[1] <= '9' ) { name++; continue; }
			s_driver.name[n++] = *name;
		}
		s_driver.name[n] = '\0';
		Q_strncpyz( s_driver.rankName, UI_Profile_GetRank( stats, &rank ) && rank.current ? rank.current->name : "Driver", sizeof( s_driver.rankName ) );
		if ( s_driver.info.country[0] ) Com_sprintf( s_driver.country, sizeof( s_driver.country ), "Country: %s", Driver_CountryName( s_driver.info.country ) );
		else s_driver.country[0] = '\0';
	} else {
		Q_strncpyz( s_driver.name, "Choose your driver", sizeof( s_driver.name ) );
		Q_strncpyz( s_driver.rankName, "Create or select a profile to begin.", sizeof( s_driver.rankName ) );
		s_driver.country[0] = '\0';
	}
	Driver_Format( stats->playerScore, s_driver.overview[0], sizeof( s_driver.overview[0] ) );
	Driver_Format( stats->gamesPlayed, s_driver.overview[1], sizeof( s_driver.overview[1] ) );
	Driver_Format( stats->wins, s_driver.overview[2], sizeof( s_driver.overview[2] ) );
	Driver_Format( stats->distanceKm, number, sizeof( number ) );
	Com_sprintf( s_driver.overview[3], sizeof( s_driver.overview[3] ), "%s km", number );
	if ( !s_driver.profile ) {
		s_driver.progress = 0.0f;
		Q_strncpyz( s_driver.progressText, "No active profile", sizeof( s_driver.progressText ) );
	} else if ( s_driver.rank[2] > s_driver.rank[1] ) {
		s_driver.progress = WS_Clamp( 0.0f, (float)( stats->playerScore - s_driver.rank[1] ) / ( s_driver.rank[2] - s_driver.rank[1] ), 1.0f );
		Driver_Format( s_driver.rank[2] - stats->playerScore > 0 ? s_driver.rank[2] - stats->playerScore : 0, number, sizeof( number ) );
		Com_sprintf( s_driver.progressText, sizeof( s_driver.progressText ), "%s points to the next rank", number );
	} else {
		s_driver.progress = 1.0f;
		Q_strncpyz( s_driver.progressText, "Highest rank reached", sizeof( s_driver.progressText ) );
	}
	for ( i = 0; i < DRIVER_STATS; i++ ) Driver_Stat( stats, i, s_driver.stats[i], sizeof( s_driver.stats[i] ) );

	/* the achievements */
	s_driver.medals = s_driver.earnedCount = 0;
	s_driver.categories = UI_WebDriverValue( 4, 0 );
	if ( s_driver.categories > 16 ) s_driver.categories = 16;
	Q_strncpyz( s_driver.filterText, "All achievements", sizeof( s_driver.filterText ) );
	for ( i = 0; i < s_driver.categories; i++ ) {
		Driver_AchievementText( i, -1, 0, s_driver.categoryTitle[i], sizeof( s_driver.categoryTitle[i] ) );
		Driver_AchievementText( i, -1, 4, text, sizeof( text ) );
		progress = atof( text );
		if ( strlen( s_driver.categoryTitle[i] ) > strlen( s_driver.filterText ) ) Q_strncpyz( s_driver.filterText, s_driver.categoryTitle[i], sizeof( s_driver.filterText ) );
		for ( j = 0; j < UI_WebDriverValue( 5, i ) && s_driver.medals < DRIVER_MAX_MEDALS; j++ ) {
			int m = s_driver.medals++;
			double threshold;
			s_driver.medalCategory[m] = i;
			Q_strncpyz( s_driver.medalLabel[m], s_driver.categoryTitle[i], sizeof( s_driver.medalLabel[m] ) );
			Driver_AchievementText( i, j, 1, s_driver.medalName[m], sizeof( s_driver.medalName[m] ) );
			Driver_AchievementText( i, j, 2, s_driver.medalText[m], sizeof( s_driver.medalText[m] ) );
			Driver_AchievementText( i, j, 3, s_driver.medalIcon[m], sizeof( s_driver.medalIcon[m] ) );
			Driver_AchievementText( i, j, 5, text, sizeof( text ) );
			threshold = atof( text );
			Driver_AchievementText( i, j, 6, text, sizeof( text ) );
			s_driver.medalEarned[m] = text[0] == '1';
			if ( s_driver.medalEarned[m] ) s_driver.earnedCount++;
			s_driver.medalProgress[m] = threshold > 0.0 ? WS_Clamp( 0.0f, (float)( progress / threshold ), 1.0f ) : 1.0f;
			if ( s_driver.medalEarned[m] ) Q_strncpyz( s_driver.medalStatus[m], "Earned", sizeof( s_driver.medalStatus[m] ) );
			else {
				Driver_Format( progress, number, sizeof( number ) );
				Driver_Format( threshold, number2, sizeof( number2 ) );
				Com_sprintf( s_driver.medalStatus[m], sizeof( s_driver.medalStatus[m] ), "Locked \xb7 %s / %s", number, number2 );
			}
		}
	}
	Com_sprintf( s_driver.count, sizeof( s_driver.count ), "%d / %d achievements earned", s_driver.earnedCount, s_driver.medals );

	/* the profiles and the avatars */
	s_driver.profiles = Driver_Available( s_driver.available, DRIVER_MAX_PROFILES );
	for ( i = 0; i < s_driver.profiles; i++ ) {
		Com_sprintf( s_driver.profileText[i], sizeof( s_driver.profileText[i] ), "%s%s", s_driver.available[i],
		             s_driver.profile && !strcmp( s_driver.available[i], UI_Profile_GetActiveName() ) ? " \xb7 Active" : "" );
	}
	{
		static char list[4096];
		char *file;
		int count = trap_FS_GetFileList( "gfx/avatars/preset", "png", list, sizeof( list ) );
		s_driver.avatarCount = 0;
		for ( i = 0, file = list; i < count && s_driver.avatarCount < DRIVER_AVATARS; i++, file += strlen( file ) + 1 ) {
			char base[64];
			COM_StripExtension( file, base, sizeof( base ) );
			Com_sprintf( s_driver.avatars[s_driver.avatarCount], 64, "gfx/avatars/preset/%s", base );
			for ( k = s_driver.avatarCount; k > 0 && Q_stricmp( s_driver.avatars[k - 1], s_driver.avatars[k] ) > 0; k-- ) {
				char swap[64];
				Q_strncpyz( swap, s_driver.avatars[k], 64 );
				Q_strncpyz( s_driver.avatars[k], s_driver.avatars[k - 1], 64 );
				Q_strncpyz( s_driver.avatars[k - 1], swap, 64 );
			}
			s_driver.avatarCount++;
		}
	}
	Com_sprintf( s_driver.profileLine, sizeof( s_driver.profileLine ), "Profile: %s", s_driver.profile ? UI_Profile_GetActiveName() : "" );
}

/* openEditor(): the editor's fields from the profile */
static void Driver_OpenDialog( driverDialog_t dialog ) {
	int i;
	s_driver.dialog = dialog;
	s_driver.dialogScroll = s_driver.listScroll = 0.0f;
	s_driver.popup = POPUP_NONE;
	s_driver.focus = FOCUS_NONE;
	s_driver.pending = qfalse;
	s_driver.status[0] = '\0';
	s_driver.newName[0] = '\0';
	s_driver.gender = 0;
	for ( i = 1; i < 5; i++ ) if ( !strcmp( s_driver.info.gender, driverGenders[i] ) ) s_driver.gender = i;
	s_driver.countryIndex = 0;
	for ( i = 0; i < (int)DRIVER_COUNTRIES; i++ ) if ( !Q_stricmp( driverCountries[s_driver.countryOrder[i]][0], s_driver.info.country ) ) s_driver.countryIndex = i;
	/* YYYY-MM-DD shows as mm/dd/yyyy */
	s_driver.birth[0] = '\0';
	if ( strlen( s_driver.info.birthDate ) == 10 ) {
		const char *b = s_driver.info.birthDate;
		Com_sprintf( s_driver.birth, sizeof( s_driver.birth ), "%c%c%c%c%c%c%c%c", b[5], b[6], b[8], b[9], b[0], b[1], b[2], b[3] );
	}
	Q_strncpyz( s_driver.avatar, s_driver.info.avatar[0] ? s_driver.info.avatar : s_driver.avatarCount ? s_driver.avatars[0] : "", sizeof( s_driver.avatar ) );
}

/* the change, through the same path the page used (ui_webdriver_payload) */
static void Driver_Mutate( int action, const char *payload ) {
	char message[128];
	driverDialog_t dialog = s_driver.dialog;
	s_driver.pending = qtrue;
	trap_Cvar_Set( "ui_webdriver_payload", payload );
	UI_WebDriverAction( action );
	s_driver.pending = qfalse;
	trap_Cvar_VariableStringBuffer( "ui_webdriver_message", message, sizeof( message ) );
	if ( trap_Cvar_VariableValue( "ui_webdriver_ok" ) ) {
		Driver_Load();
		Driver_OpenDialog( dialog );
	}
	Q_strncpyz( s_driver.status, message, sizeof( s_driver.status ) );
}

static void Driver_Save( void ) {
	char payload[512], birth[16];
	if ( strlen( s_driver.birth ) == 8 ) {
		const char *b = s_driver.birth;
		Com_sprintf( birth, sizeof( birth ), "%c%c%c%c-%c%c-%c%c", b[4], b[5], b[6], b[7], b[0], b[1], b[2], b[3] );
	} else birth[0] = '\0';
	Com_sprintf( payload, sizeof( payload ), "{\"uuid\":\"%s\",\"gender\":\"%s\",\"birthDate\":\"%s\",\"country\":\"%s\",\"avatar\":\"%s\"}",
	             s_driver.info.uuid, driverGenders[s_driver.gender], birth, driverCountries[s_driver.countryOrder[s_driver.countryIndex]][0], s_driver.avatar );
	Driver_Mutate( 4, payload );
}

static void Driver_Create( void ) {
	const char *c;
	if ( !s_driver.newName[0] ) { Q_strncpyz( s_driver.status, "Please fill out this field.", sizeof( s_driver.status ) ); return; }
	for ( c = s_driver.newName; *c; c++ ) {
		if ( !( ( *c >= 'a' && *c <= 'z' ) || ( *c >= 'A' && *c <= 'Z' ) || ( *c >= '0' && *c <= '9' ) || *c == '_' || *c == '-' ) ) {
			Q_strncpyz( s_driver.status, "Please match the requested format.", sizeof( s_driver.status ) );
			return;
		}
	}
	Driver_Mutate( 5, va( "{\"name\":\"%s\"}", s_driver.newName ) );
}

/* -------------------------------------------------------------------------
   Drawing
   ------------------------------------------------------------------------- */

static const vec4_t driverPressed = { 0x44 / 255.0f, 0x20 / 255.0f, 0x0e / 255.0f, 1.0f };
static const vec4_t driverInput   = { 0x22 / 255.0f, 0x22 / 255.0f, 0x22 / 255.0f, 1.0f };
static const vec4_t driverRing    = { 0xee / 255.0f, 0xee / 255.0f, 0xee / 255.0f, 1.0f };
static const vec4_t driverHolder  = { 0x75 / 255.0f, 0x75 / 255.0f, 0x75 / 255.0f, 1.0f };
static const vec4_t driverSystem  = { 0x3b / 255.0f, 0x3b / 255.0f, 0x3b / 255.0f, 1.0f };	/* Chrome's dark select */
static const vec4_t driverSystemLine = { 0x85 / 255.0f, 0x85 / 255.0f, 0x85 / 255.0f, 1.0f };
static const vec4_t driverBackdrop = { 0.0f, 0.0f, 0.0f, 0xbb / 255.0f };
static const vec4_t driverPopup   = { 0x1f / 255.0f, 0x1f / 255.0f, 0x1f / 255.0f, 1.0f };
static const vec4_t driverPopupLine = { 0x3b / 255.0f, 0x3b / 255.0f, 0x3b / 255.0f, 1.0f };
static const vec4_t driverLocked  = { 1.0f, 1.0f, 1.0f, 0.45f };

/* what is drawn inside the dialog is cut to its scrollport */
static const wsRect_t *driverClip;

static void Driver_Fill( float x, float y, float w, float h, const float *color ) {
	float top = y, bottom = y + h;
	if ( driverClip ) {
		if ( top < driverClip->y ) top = driverClip->y;
		if ( bottom > driverClip->y + driverClip->h ) bottom = driverClip->y + driverClip->h;
	}
	if ( bottom > top && w > 0.0f ) WS_FillRect( x, top, w, bottom - top, color );
}

static void Driver_ClipText( qboolean on ) {
	if ( on && driverClip ) WS_SetFade( driverClip->y, driverClip->h, 0.0f );
	else WS_SetFade( 0.0f, 0.0f, 0.0f );
}

static void Driver_Picture( float x, float y, float w, float h, qhandle_t shader, const float *color ) {
	float top = y, bottom = y + h;
	if ( !shader || h <= 0.0f ) return;
	if ( driverClip ) {
		if ( top < driverClip->y ) top = driverClip->y;
		if ( bottom > driverClip->y + driverClip->h ) bottom = driverClip->y + driverClip->h;
	}
	if ( bottom <= top ) return;
	trap_R_SetColor( color );
	trap_R_DrawStretchPic( WS_Round( x ), top, WS_Round( x + w ) - WS_Round( x ), bottom - top, 0, ( top - y ) / h, 1, ( bottom - y ) / h, shader );
	trap_R_SetColor( NULL );
}

/* a button, its text centred (stretched) or at its padding */
static void Driver_Button( const wsRect_t *r, const char *label, qboolean secondary, qboolean disabled, qboolean pressed, qboolean center ) {
	wsTextStyle_t style = { WS_BODY_BOLD, WS_REM, 0.0f, 24.0f, qfalse };
	float x0 = WS_Round( r->x ), y0 = WS_Round( r->y ), x1 = WS_Round( r->x + r->w ), y1 = WS_Round( r->y + r->h );
	vec4_t border, fill, text;
	int i;
	if ( !driverClip && !pressed && !center ) { Library_DrawButton( r, label, secondary, disabled ); return; }
	for ( i = 0; i < 4; i++ ) {
		border[i] = pressed ? wsAccent[i] : secondary ? wsLine[i] : wsAccent[i];
		fill[i] = pressed ? driverPressed[i] : secondary ? wsSurface[i] : wsAccent[i];
		text[i] = secondary || pressed ? wsWhite[i] : wsButtonText[i];
		if ( disabled && i < 3 ) {
			border[i] = border[i] * 0.45f + wsSurface[i] * 0.55f;
			fill[i] = fill[i] * 0.45f + wsSurface[i] * 0.55f;
			text[i] = text[i] * 0.45f + wsSurface[i] * 0.55f;
		}
	}
	Driver_Fill( x0, y0, x1 - x0, y1 - y0, border );
	Driver_Fill( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, fill );
	Driver_ClipText( qtrue );
	WS_DrawText( &style, center ? r->x + ( r->w - WS_TextWidth( &style, label, strlen( label ) ) ) * 0.5f : r->x + 1 + 0.85f * WS_REM,
	             r->y + 1 + 0.65f * WS_REM, label, strlen( label ), text );
	Driver_ClipText( qfalse );
}

static void Driver_Field( const wsRect_t *r, qboolean focus ) {
	float x0 = WS_Round( r->x ), y0 = WS_Round( r->y ), x1 = WS_Round( r->x + r->w ), y1 = WS_Round( r->y + r->h );
	if ( focus ) Driver_Fill( x0 - 1, y0 - 1, x1 - x0 + 2, y1 - y0 + 2, driverRing );
	else Driver_Fill( x0, y0, x1 - x0, y1 - y0, wsLine );
	Driver_Fill( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, driverInput );
}

static void Driver_Arrow( const wsRect_t *r, float right ) {
	float y = WS_Round( r->y + r->h * 0.5f ) - 3.0f;
	if ( driverClip && ( y < driverClip->y || y + 7.0f > driverClip->y + driverClip->h ) ) return;
	trap_R_SetColor( wsWhite );
	trap_R_DrawStretchPic( WS_Round( r->x + r->w ) - right, y, 10, 7, 0, 25 / 32.0f, 10 / 16.0f, 1.0f, trap_R_RegisterShaderNoMip( "menu/ui/web_scrollbar" ) );
	trap_R_SetColor( NULL );
}

static void Driver_Select( const wsRect_t *r, const char *text, qboolean open ) {
	wsTextStyle_t style;
	WS_BodyStyle( &style );
	style.lineHeight = 21.0f;
	Driver_Field( r, open );
	Driver_ClipText( qtrue );
	WS_SetClipX( r->x + 1.0f, r->x + r->w - 20.0f );
	WS_DrawText( &style, r->x + 1.0f + WS_LU( 0.65f * WS_REM ) + 4.0f, r->y + 1.0f + WS_LU( 0.65f * WS_REM ) + 1.0f, text, strlen( text ), wsWhite );
	WS_SetClipX( 0.0f, 0.0f );
	Driver_ClipText( qfalse );
	Driver_Arrow( r, 14.0f );
}

static void Driver_Input( const wsRect_t *r, const char *text, const char *placeholder, qboolean focus, float inner ) {
	wsTextStyle_t body;
	float x = r->x + 1.0f + WS_LU( 0.65f * WS_REM ), y = r->y + 1.0f + WS_LU( 0.65f * WS_REM ) + ( inner - WS_BODY_LINE ) * 0.5f;
	WS_BodyStyle( &body );
	Driver_Field( r, focus );
	Driver_ClipText( qtrue );
	if ( text[0] ) WS_DrawText( &body, x, y, text, strlen( text ), wsWhite );
	else if ( placeholder ) WS_DrawText( &body, x, y, placeholder, strlen( placeholder ), focus ? wsWhite : driverHolder );
	Driver_ClipText( qfalse );
	if ( focus && ( ( uis.realtime - s_driver.focusTime ) / 500 ) % 2 == 0 && ( !placeholder || text[0] ) ) {
		Driver_Fill( WS_Round( x + WS_TextWidth( &body, text, strlen( text ) ) ), WS_Round( y + 3.0f ), 1, 19, wsWhite );
	}
}

/* the date field: mm/dd/yyyy, the digits typed so far, and the calendar */
static void Driver_Date( const wsRect_t *r ) {
	char shown[16];
	const char *mask = "mm/dd/yyyy";
	int i, d = 0;
	float cx, cy;
	for ( i = 0; i < 10; i++ ) {
		if ( mask[i] == '/' ) shown[i] = '/';
		else shown[i] = s_driver.birth[d] ? s_driver.birth[d++] : mask[i];
	}
	shown[10] = '\0';
	Driver_Input( r, shown, NULL, s_driver.focus == FOCUS_BIRTH, 26.0f );
	/* the picker's icon: a small calendar in white */
	cx = WS_Round( r->x + r->w ) - 1.0f - WS_LU( 0.65f * WS_REM ) - 14.0f;
	cy = WS_Round( r->y + r->h * 0.5f ) - 7.0f;
	if ( !driverClip || ( cy >= driverClip->y && cy + 14.0f <= driverClip->y + driverClip->h ) ) {
		WS_FillRect( cx, cy + 1, 13, 1, wsWhite );
		WS_FillRect( cx, cy + 13, 13, 1, wsWhite );
		WS_FillRect( cx, cy + 1, 1, 13, wsWhite );
		WS_FillRect( cx + 12, cy + 1, 1, 13, wsWhite );
		WS_FillRect( cx, cy + 4, 13, 1, wsWhite );
		WS_FillRect( cx + 3, cy - 1, 1, 3, wsWhite );
		WS_FillRect( cx + 9, cy - 1, 1, 3, wsWhite );
	}
}

/* a value that may be the em dash */
static void Driver_Value( const wsTextStyle_t *style, float x, float y, const char *text, const wsLines_t *lines ) {
	if ( !strcmp( text, DRIVER_EMDASH ) ) {
		/* Barlow's em dash: .039 to .646 of the em, .252 to .314 above the baseline */
		float baseline = WS_Round( y + WS_Baseline( style ) );
		WS_FillRect( WS_Round( x + 0.039f * style->size ), baseline - WS_Round( 0.314f * style->size ) - 1.0f,
		             WS_Round( 0.607f * style->size ) + 1.0f, WS_Round( 0.062f * style->size ) + 1.0f, wsWhite );
		return;
	}
	WS_DrawLines( style, x, y, text, lines, wsWhite );
}

static void Driver_DrawCards( const driverCard_t *cards, const char **labels, const char **values, int count ) {
	wsTextStyle_t value;
	int i;
	Driver_ValueStyle( &value );
	for ( i = 0; i < count; i++ ) {
		const driverCard_t *c = &cards[i];
		if ( c->box.y > WS_ViewportHeight() || c->box.y + c->box.h < 0.0f ) continue;
		WS_DrawLabel( c->box.x, c->box.y, labels[i], &c->label );
		Driver_Value( &value, c->box.x, c->box.y + c->label.count * WS_LU( WS_LABEL_LINE ), values[i], &c->value );
	}
}

/* the layout from the model */
static void Driver_Build( void ) {
	driverLayout_t *l = &s_driver.layout;
	int i;
	memset( l, 0, sizeof( *l ) );
	l->tab = s_driver.tab;
	l->profile = s_driver.profile;
	l->avatar = s_driver.profile && s_driver.info.avatar[0] && trap_R_RegisterShaderNoMip( s_driver.info.avatar );
	l->name = s_driver.name;
	l->rank = s_driver.rankName;
	l->country = s_driver.country;
	for ( i = 0; i < 4; i++ ) l->overview[i] = s_driver.overview[i];
	l->progressText = s_driver.progressText;
	for ( i = 0; i < DRIVER_STATS; i++ ) l->stats[i] = s_driver.stats[i];
	l->filterText = s_driver.filterText;
	l->count = s_driver.count;
	for ( i = 0; i < s_driver.medals; i++ ) {
		if ( s_driver.filter >= 0 && s_driver.medalCategory[i] != s_driver.filter ) continue;
		l->earned[l->medals] = s_driver.medalEarned[i];
		l->medalLabel[l->medals] = s_driver.medalLabel[i];
		l->medalName[l->medals] = s_driver.medalName[i];
		l->medalText[l->medals] = s_driver.medalText[i];
		l->medalStatus[l->medals] = s_driver.medalStatus[i];
		l->medals++;
	}
	l->dialog = s_driver.dialog;
	l->profileLine = s_driver.profileLine;
	l->avatars = s_driver.avatarCount;
	l->profiles = s_driver.profiles;
	for ( i = 0; i < s_driver.profiles; i++ ) l->profileName[i] = s_driver.profileText[i];
	l->status = s_driver.status;
	l->pageScroll = s_driver.pageScroll;
	l->dialogScroll = s_driver.dialogScroll;
	l->listScroll = s_driver.listScroll;
	Driver_Layout( l, WS_ViewportWidth(), WS_ViewportHeight() );
	s_driver.pageScroll = l->pageScroll;
	s_driver.dialogScroll = l->dialogScroll;
	s_driver.listScroll = l->listScroll;
}

/* the medals on screen: the filtered list's index to the model's */
static int Driver_MedalIndex( int shown ) {
	int i, n = 0;
	for ( i = 0; i < s_driver.medals; i++ ) {
		if ( s_driver.filter >= 0 && s_driver.medalCategory[i] != s_driver.filter ) continue;
		if ( n++ == shown ) return i;
	}
	return 0;
}

static void Driver_DrawMedals( void ) {
	driverLayout_t *l = &s_driver.layout;
	wsTextStyle_t body, label, h3, system;
	int i;
	WS_BodyStyle( &body );
	WS_LabelStyle( &label );
	Driver_MedalStyle( &h3 );
	system = body;
	system.size = 13.333333f;
	system.lineHeight = 17.0f;
	WS_DrawText( &body, l->page.contentX, l->categoryY, "Category", 8, wsWhite );
	/* the filter: Chrome's own select in the dark scheme */
	{
		float x0 = WS_Round( l->filter.x ), y0 = WS_Round( l->filter.y ), x1 = WS_Round( l->filter.x + l->filter.w ), y1 = WS_Round( l->filter.y + l->filter.h );
		const char *text = s_driver.filter >= 0 ? s_driver.categoryTitle[s_driver.filter] : "All achievements";
		WS_FillRect( x0, y0, x1 - x0, y1 - y0, s_driver.popup == POPUP_FILTER ? driverRing : driverSystemLine );
		WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, driverSystem );
		WS_DrawText( &system, l->filter.x + 1.0f + WS_LU( 0.65f * WS_REM ) + 4.0f, l->filter.y + 1.0f + WS_LU( 0.65f * WS_REM ), text, strlen( text ), wsWhite );
		Driver_Arrow( &l->filter, 20.0f );
	}
	WS_DrawText( &body, l->page.contentX, l->countY, s_driver.count, strlen( s_driver.count ), wsWhite );
	for ( i = 0; i < l->medals; i++ ) {
		driverMedal_t *m = &l->medal[i];
		int model = Driver_MedalIndex( i );
		float x0, y0, x1, y1, inner;
		char icon[80], shown[64];
		if ( m->box.y > WS_ViewportHeight() || m->box.y + m->box.h < 0.0f ) continue;
		x0 = WS_Round( m->box.x ); y0 = WS_Round( m->box.y ); x1 = WS_Round( m->box.x + m->box.w ); y1 = WS_Round( m->box.y + m->box.h );
		WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
		if ( l->earned[i] ) WS_FillRect( x0, y0, x1 - x0, 3, wsAccent );
		WS_FillRect( x0 + 1, y0 + ( l->earned[i] ? 3 : 1 ), x1 - x0 - 2, y1 - y0 - ( l->earned[i] ? 4 : 2 ), wsSurface );
		/* the medal; a locked one in grey at .45 (the page filters it to grayscale) */
		Q_strncpyz( icon, s_driver.medalIcon[model], sizeof( icon ) );
		if ( !l->earned[i] ) {
			char *tier = strstr( icon, "_t" );
			if ( tier ) Q_strncpyz( tier, "_locked", sizeof( icon ) - ( tier - icon ) );
		}
		Driver_Picture( m->medal.x, m->medal.y, m->medal.w, m->medal.h, trap_R_RegisterShaderNoMip( icon ), l->earned[i] ? NULL : driverLocked );
		inner = m->box.x + 1.0f + 1.25f * WS_REM;
		WS_DrawLabel( inner, m->labelY, l->medalLabel[i], &m->label );
		WS_Transform( &h3, l->medalName[i], shown, sizeof( shown ) );
		WS_DrawLines( &h3, inner, m->nameY, shown, &m->name, wsWhite );
		WS_DrawLines( &body, inner, m->textY, l->medalText[i], &m->text, wsWhite );
		WS_DrawProgress( inner, m->progressY, m->box.w - 2.0f - 2.5f * WS_REM, s_driver.medalProgress[model] );
		WS_DrawLines( &body, inner, m->statusY, l->medalStatus[i], &m->status, wsWhite );
	}
}

static void Driver_DrawPopup( void ) {
	driverLayout_t *l = &s_driver.layout;
	wsTextStyle_t body;
	const wsRect_t *anchor;
	float below, above, x, y, w, h;
	int count, current, rows, i, first;
	char text[64];

	if ( s_driver.popup == POPUP_NONE ) return;
	anchor = s_driver.popup == POPUP_FILTER ? &l->filter : s_driver.popup == POPUP_GENDER ? &l->field[0] : &l->field[2];
	count = s_driver.popup == POPUP_FILTER ? s_driver.categories + 1 : s_driver.popup == POPUP_GENDER ? 5 : (int)DRIVER_COUNTRIES;
	current = s_driver.popup == POPUP_FILTER ? s_driver.filter + 1 : s_driver.popup == POPUP_GENDER ? s_driver.gender : s_driver.countryIndex;
	rows = count < 20 ? count : 20;
	below = WS_ViewportHeight() - WS_Round( anchor->y + anchor->h );
	above = WS_Round( anchor->y );
	x = WS_Round( anchor->x );
	w = WS_Round( anchor->x + anchor->w ) - x;
	if ( s_driver.popup == POPUP_FILTER && w < 220.0f ) w = 220.0f;
	if ( rows * WS_BODY_LINE + 2.0f > below && above > below ) {
		while ( rows > 1 && rows * WS_BODY_LINE + 2.0f > above ) rows--;
		h = rows * WS_BODY_LINE + 2.0f;
		y = above - h;
	} else {
		while ( rows > 1 && rows * WS_BODY_LINE + 2.0f > below ) rows--;
		h = rows * WS_BODY_LINE + 2.0f;
		y = WS_Round( anchor->y + anchor->h );
	}
	s_driver.popupScroll = WS_Clamp( 0.0f, s_driver.popupScroll, (float)( count - rows ) );
	first = (int)s_driver.popupScroll;
	WS_BodyStyle( &body );
	WS_FillRect( x, y, w, h, driverPopupLine );
	WS_FillRect( x + 1, y + 1, w - 2, h - 2, driverPopup );
	for ( i = 0; i < rows; i++ ) {
		int item = first + i;
		if ( item == current ) WS_FillRect( x + 1, y + 1 + i * WS_BODY_LINE, w - 2, WS_BODY_LINE, driverPopupLine );
		if ( s_driver.popup == POPUP_FILTER ) Q_strncpyz( text, item ? s_driver.categoryTitle[item - 1] : "All achievements", sizeof( text ) );
		else if ( s_driver.popup == POPUP_GENDER ) Q_strncpyz( text, item ? driverGenders[item] : "Unspecified", sizeof( text ) );
		else Q_strncpyz( text, driverCountries[s_driver.countryOrder[item]][1], sizeof( text ) );
		WS_DrawText( &body, x + 1.0f + WS_LU( 0.65f * WS_REM ), y + 1 + i * WS_BODY_LINE, text, strlen( text ), wsWhite );
	}
	s_driver.popupRect.x = x;
	s_driver.popupRect.y = y;
	s_driver.popupRect.w = w;
	s_driver.popupRect.h = h;
}

static void Driver_DrawDialog( void ) {
	driverLayout_t *l = &s_driver.layout;
	wsTextStyle_t body, h3;
	const wsRect_t *box = &l->dialogBox;
	float x0 = WS_Round( box->x ), y0 = WS_Round( box->y ), x1 = WS_Round( box->x + box->w ), y1 = WS_Round( box->y + box->h ), H = WS_ViewportHeight();
	char shown[64];
	int i;

	if ( l->dialog == DRIVER_NO_DIALOG ) return;
	WS_BodyStyle( &body );
	/* the backdrop around the dialog, the dialog, its content cut to the scrollport */
	WS_FillRect( 0, 0, WS_ViewportWidth(), y0, driverBackdrop );
	WS_FillRect( 0, y1, WS_ViewportWidth(), H - y1, driverBackdrop );
	WS_FillRect( 0, y0, x0, y1 - y0, driverBackdrop );
	WS_FillRect( x1, y0, WS_ViewportWidth() - x1, y1 - y0, driverBackdrop );
	WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
	WS_FillRect( x0, y0, x1 - x0, 3, wsAccent );
	WS_FillRect( x0 + 1, y0 + 3, x1 - x0 - 2, y1 - y0 - 4, wsSurface );
	driverClip = &l->dialogPort;
	Driver_ClipText( qtrue );
	WS_DrawLabel( l->dialogX, l->dialogLabelY, "Q3RALLY / YOUR IDENTITY", NULL );
	WS_Transform( &l->h2, l->dialog == DRIVER_EDIT ? "Edit driver" : "Your drivers", shown, sizeof( shown ) );
	WS_DrawLines( &l->h2, l->dialogX, l->dialogH2Y, shown, &l->nameLines, wsWhite );
	Driver_ClipText( qfalse );
	if ( l->dialog == DRIVER_EDIT ) {
		static const char *labels[3] = { "Gender (optional)", "Birth date (optional)", "Country (optional)" };
		Driver_ClipText( qtrue );
		WS_DrawText( &body, l->dialogX, l->profileLineY, s_driver.profileLine, strlen( s_driver.profileLine ), wsWhite );
		for ( i = 0; i < 3; i++ ) WS_DrawText( &body, l->dialogX, l->fieldLabelY[i], labels[i], strlen( labels[i] ), wsWhite );
		WS_DrawLabel( l->dialogX, l->avatarLabelY, "Avatar", NULL );
		Driver_ClipText( qfalse );
		Driver_Select( &l->field[0], s_driver.gender ? driverGenders[s_driver.gender] : "Unspecified", s_driver.popup == POPUP_GENDER );
		Driver_Date( &l->field[1] );
		Driver_Select( &l->field[2], driverCountries[s_driver.countryOrder[s_driver.countryIndex]][1], s_driver.popup == POPUP_COUNTRY );
		for ( i = 0; i < l->avatars; i++ ) {
			wsRect_t *r = &l->avatarChoice[i];
			float pad = 1.0f + WS_LU( 0.35f * WS_REM );
			Driver_Button( r, "", qtrue, qfalse, !strcmp( s_driver.avatar, s_driver.avatars[i] ), qfalse );
			Driver_Picture( r->x + pad, r->y + pad, r->w - 2.0f * pad, r->h - 2.0f * pad, trap_R_RegisterShaderNoMip( s_driver.avatars[i] ), NULL );
		}
		Driver_Button( &l->submit, "Save driver", qfalse, s_driver.pending, qfalse, qtrue );
	} else {
		const wsRect_t *list = &l->list;
		const wsRect_t *outer = driverClip;
		wsRect_t cut = *list;
		/* the list scrolls inside the dialog */
		if ( cut.y < outer->y ) { cut.h -= outer->y - cut.y; cut.y = outer->y; }
		if ( cut.y + cut.h > outer->y + outer->h ) cut.h = outer->y + outer->h - cut.y;
		driverClip = &cut;
		if ( !l->profiles ) {
			Driver_ClipText( qtrue );
			WS_DrawText( &body, list->x, list->y + WS_REM, "No local profiles yet. Create your first driver below.", 54, wsWhite );
			Driver_ClipText( qfalse );
		}
		for ( i = 0; i < l->profiles; i++ ) {
			wsRect_t r = l->profileButton[i];
			r.x += list->x;
			r.y += list->y - l->listScroll;
			if ( r.y > cut.y + cut.h || r.y + r.h < cut.y ) continue;
			Driver_Button( &r, l->profileName[i], qtrue, s_driver.pending, qfalse, qfalse );
		}
		if ( l->listBar ) WS_DrawScrollbar( list, &l->listThumb );
		driverClip = outer;
		h3 = body;
		h3.font = WS_BODY_BOLD;
		h3.size = 1.17f * WS_REM;
		h3.lineHeight = 1.5f * h3.size;
		Driver_ClipText( qtrue );
		WS_DrawText( &h3, l->dialogX, l->createY, "Create a driver", 15, wsWhite );
		WS_DrawText( &body, l->dialogX, l->nameLabelY, "Profile name", 12, wsWhite );
		WS_DrawLines( &body, l->dialogX, l->helpY, "Use letters, numbers, a dash or an underscore. A new profile starts with its own progress.", &l->helpLines, wsWhite );
		Driver_ClipText( qfalse );
		Driver_Input( &l->nameInput, s_driver.newName, "", s_driver.focus == FOCUS_NAME, WS_BODY_LINE );
		Driver_Button( &l->submit, "Create & select", qfalse, s_driver.pending, qfalse, qtrue );
	}
	Driver_ClipText( qtrue );
	WS_DrawLines( &body, l->dialogX, l->statusY, s_driver.status, &l->statusLines, wsWhite );
	Driver_ClipText( qfalse );
	Driver_Button( &l->close, "Close", qtrue, s_driver.pending, qfalse, qfalse );
	driverClip = NULL;
	if ( l->dialogBar ) WS_DrawScrollbar( &l->dialogPort, &l->dialogThumb );
}

static void Driver_Draw( void ) {
	driverLayout_t *l;
	wsTextStyle_t body;
	static const char *tabText[3] = { "Overview", "Stats", "Achievements" };
	char shown[96];
	int i, first;

	if ( s_driver.drag != DRAG_NONE ) {
		float x, y, track, content;
		const wsRect_t *port, *thumb;
		float *scroll;
		l = &s_driver.layout;
		Library_Cursor( &x, &y );
		if ( !trap_Key_IsDown( K_MOUSE1 ) ) s_driver.drag = DRAG_NONE;
		else {
			if ( s_driver.drag == DRAG_PAGE ) { port = &l->viewport; thumb = &l->pageThumb; content = l->pageHeight; scroll = &s_driver.pageScroll; }
			else if ( s_driver.drag == DRAG_DIALOG ) { port = &l->dialogPort; thumb = &l->dialogThumb; content = l->dialogHeight - 4.0f; scroll = &s_driver.dialogScroll; }
			else { port = &l->list; thumb = &l->listThumb; content = l->listHeight; scroll = &s_driver.listScroll; }
			track = port->h - 2.0f * RIVALS_ARROW - 9.0f - thumb->h;
			if ( track > 0.0f ) *scroll = s_driver.dragScroll + ( y - s_driver.dragY ) * ( content - port->h ) / track;
		}
	}
	if ( !s_driver.dumped && trap_Cvar_VariableValue( "ui_driverDump" ) ) {
		s_driver.dumped = qtrue;
		Driver_Dump();
	}
	Driver_Build();
	l = &s_driver.layout;
	WS_BodyStyle( &body );

	WS_DrawBackground( l->page.pageWidth, l->pageHeight );
	WS_DrawFrontHeader( &l->page, "Driver ", "profile.", "" );
	WS_DrawButton( &l->back, "Back", qtrue, qfalse );
	for ( i = 0; i < 3; i++ ) WS_DrawButton( &l->tabs[i], tabText[i], i != s_driver.tab, qfalse );

	if ( s_driver.tab == 0 ) {
		WS_DrawSection( &l->identity, qfalse );
		if ( l->avatar ) Driver_Picture( l->avatarBox.x, l->avatarBox.y, l->avatarBox.w, l->avatarBox.h, trap_R_RegisterShaderNoMip( s_driver.info.avatar ), NULL );
		WS_DrawLabel( l->identityX, l->identityLabelY, "YOUR DRIVER", NULL );
		WS_Transform( &l->h2, l->name, shown, sizeof( shown ) );
		WS_DrawLines( &l->h2, l->identityX, l->nameY, shown, &l->nameLines, wsWhite );
		WS_DrawLines( &body, l->identityX, l->rankY, l->rank, &l->rankLines, wsWhite );
		if ( l->country[0] ) WS_DrawLines( &body, l->identityX, l->countryY, l->country, &l->countryLines, wsWhite );
		WS_DrawSection( &l->career, qfalse );
		WS_DrawText( &l->h2, l->careerX, l->careerH2Y, "YOUR PROGRESSION", 16, wsWhite );
		Driver_DrawCards( l->overviewCard, driverOverviewLabels, l->overview, 4 );
		WS_DrawProgress( l->careerX, l->progressY, l->careerW, s_driver.progress );
		WS_DrawText( &body, l->careerX, l->progressTextY, l->progressText, strlen( l->progressText ), wsWhite );
		Library_DrawButton( &l->actions[0], "Edit driver", qtrue, !s_driver.profile );
		Library_DrawButton( &l->actions[1], "Select / create profile", qtrue, qfalse );
		Library_DrawButton( &l->actions[2], "Web profile & garage", qtrue, qfalse );
	} else if ( s_driver.tab == 1 ) {
		for ( i = 0, first = 0; i < DRIVER_GROUPS; first += driverGroupSize[i], i++ ) {
			if ( l->group[i].y > WS_ViewportHeight() || l->group[i].y + l->group[i].h < 0.0f ) continue;
			WS_DrawSection( &l->group[i], qfalse );
			WS_Transform( &l->h2, driverGroups[i], shown, sizeof( shown ) );
			WS_DrawText( &l->h2, l->page.contentX + 1.0f + l->page.sectionPad, l->groupH2Y[i], shown, strlen( shown ), wsWhite );
			Driver_DrawCards( &l->statCard[first], &driverStatLabels[first], &l->stats[first], driverGroupSize[i] );
		}
	} else {
		Driver_DrawMedals();
	}
	if ( l->pageBar ) WS_DrawScrollbar( &l->viewport, &l->pageThumb );
	Driver_DrawDialog();
	Driver_DrawPopup();
}

/* -------------------------------------------------------------------------
   Input
   ------------------------------------------------------------------------- */

static qboolean Driver_Scrollbar( const wsRect_t *port, const wsRect_t *thumb, float *scroll, driverDrag_t drag, float x, float y ) {
	if ( x < port->x + port->w - RIVALS_SCROLLBAR || !Library_Inside( port, x, y ) ) return qfalse;
	if ( y < port->y + RIVALS_ARROW ) *scroll -= 40.0f;
	else if ( y >= port->y + port->h - RIVALS_ARROW ) *scroll += 40.0f;
	else if ( y < thumb->y ) *scroll -= 0.875f * port->h;
	else if ( y >= thumb->y + thumb->h ) *scroll += 0.875f * port->h;
	else {
		s_driver.drag = drag;
		s_driver.dragY = y;
		s_driver.dragScroll = *scroll;
	}
	return qtrue;
}

static void Driver_PopupClick( float x, float y ) {
	const wsRect_t *r = &s_driver.popupRect;
	driverPopup_t popup = s_driver.popup;
	int item;
	s_driver.popup = POPUP_NONE;
	if ( !Library_Inside( r, x, y ) || y < r->y + 1 || y >= r->y + r->h - 1 ) return;
	item = (int)s_driver.popupScroll + (int)( ( y - r->y - 1 ) / WS_BODY_LINE );
	if ( popup == POPUP_FILTER ) {
		if ( item <= s_driver.categories ) s_driver.filter = item - 1;
	} else if ( popup == POPUP_GENDER ) {
		if ( item < 5 ) s_driver.gender = item;
	} else if ( item < (int)DRIVER_COUNTRIES ) {
		s_driver.countryIndex = item;
	}
}

static void Driver_DialogClick( float x, float y ) {
	driverLayout_t *l = &s_driver.layout;
	int i;
	if ( !Library_Inside( &l->dialogBox, x, y ) ) return;
	if ( l->dialogBar && Driver_Scrollbar( &l->dialogPort, &l->dialogThumb, &s_driver.dialogScroll, DRAG_DIALOG, x, y ) ) return;
	if ( y < l->dialogPort.y || y >= l->dialogPort.y + l->dialogPort.h ) return;
	s_driver.focus = FOCUS_NONE;
	if ( s_driver.pending ) return;
	if ( Library_Inside( &l->close, x, y ) ) { s_driver.dialog = DRIVER_NO_DIALOG; return; }
	if ( l->dialog == DRIVER_EDIT ) {
		if ( Library_Inside( &l->field[0], x, y ) ) { s_driver.popup = POPUP_GENDER; s_driver.popupScroll = 0.0f; return; }
		if ( Library_Inside( &l->field[1], x, y ) ) { s_driver.focus = FOCUS_BIRTH; s_driver.focusTime = uis.realtime; return; }
		if ( Library_Inside( &l->field[2], x, y ) ) { s_driver.popup = POPUP_COUNTRY; s_driver.popupScroll = s_driver.countryIndex - 10; return; }
		for ( i = 0; i < l->avatars; i++ ) {
			if ( Library_Inside( &l->avatarChoice[i], x, y ) ) { Q_strncpyz( s_driver.avatar, s_driver.avatars[i], sizeof( s_driver.avatar ) ); return; }
		}
		if ( Library_Inside( &l->submit, x, y ) ) Driver_Save();
	} else {
		if ( Library_Inside( &l->list, x, y ) ) {
			if ( l->listBar && Driver_Scrollbar( &l->list, &l->listThumb, &s_driver.listScroll, DRAG_LIST, x, y ) ) return;
			for ( i = 0; i < l->profiles; i++ ) {
				wsRect_t r = l->profileButton[i];
				r.x += l->list.x;
				r.y += l->list.y - l->listScroll;
				if ( Library_Inside( &r, x, y ) ) {
					Driver_Mutate( 6, va( "{\"name\":\"%s\"}", s_driver.available[i] ) );
					return;
				}
			}
			return;
		}
		if ( Library_Inside( &l->nameInput, x, y ) ) { s_driver.focus = FOCUS_NAME; s_driver.focusTime = uis.realtime; return; }
		if ( Library_Inside( &l->submit, x, y ) ) Driver_Create();
	}
}

static void Driver_Click( float x, float y ) {
	driverLayout_t *l = &s_driver.layout;
	int i;
	if ( s_driver.popup != POPUP_NONE ) { Driver_PopupClick( x, y ); return; }
	if ( s_driver.dialog != DRIVER_NO_DIALOG ) { Driver_DialogClick( x, y ); return; }
	if ( l->pageBar && Driver_Scrollbar( &l->viewport, &l->pageThumb, &s_driver.pageScroll, DRAG_PAGE, x, y ) ) return;
	if ( Library_Inside( &l->back, x, y ) ) { UI_WebDriverAction( 0 ); return; }
	for ( i = 0; i < 3; i++ ) {
		if ( Library_Inside( &l->tabs[i], x, y ) ) {
			if ( s_driver.tab != i ) s_driver.pageScroll = 0.0f;
			s_driver.tab = i;
			return;
		}
	}
	if ( s_driver.tab == 0 ) {
		if ( Library_Inside( &l->actions[0], x, y ) && s_driver.profile ) Driver_OpenDialog( DRIVER_EDIT );
		else if ( Library_Inside( &l->actions[1], x, y ) ) Driver_OpenDialog( DRIVER_SWITCH );
		else if ( Library_Inside( &l->actions[2], x, y ) ) UI_WebDriverAction( 3 );
	} else if ( s_driver.tab == 2 && Library_Inside( &l->filter, x, y ) ) {
		s_driver.popup = POPUP_FILTER;
		s_driver.popupScroll = 0.0f;
	}
}

/* the date field takes digits: mm, dd, yyyy */
static void Driver_TypeDate( int ch ) {
	int length = strlen( s_driver.birth );
	if ( ch == 8 ) { if ( length ) s_driver.birth[length - 1] = '\0'; }
	else if ( ch >= '0' && ch <= '9' && length < 8 ) { s_driver.birth[length] = (char)ch; s_driver.birth[length + 1] = '\0'; }
	s_driver.focusTime = uis.realtime;
}

static sfxHandle_t Driver_Key( int key ) {
	driverLayout_t *l = &s_driver.layout;
	float x, y;

	Library_Cursor( &x, &y );
	if ( key & K_CHAR_FLAG ) {
		int ch = key & ~K_CHAR_FLAG, length = strlen( s_driver.newName );
		if ( s_driver.focus == FOCUS_BIRTH ) Driver_TypeDate( ch );
		else if ( s_driver.focus == FOCUS_NAME ) {
			/* maxlength 31 */
			if ( ch == 8 ) { if ( length ) s_driver.newName[length - 1] = '\0'; }
			else if ( ch > 32 && ch < 127 && length < 31 ) { s_driver.newName[length] = (char)ch; s_driver.newName[length + 1] = '\0'; }
			s_driver.focusTime = uis.realtime;
		}
		return 0;
	}
	switch ( key ) {
	case K_ESCAPE:
	case K_PAD0_B:
		if ( s_driver.popup != POPUP_NONE ) s_driver.popup = POPUP_NONE;
		else if ( s_driver.dialog != DRIVER_NO_DIALOG ) { if ( !s_driver.pending ) s_driver.dialog = DRIVER_NO_DIALOG; }
		else UI_WebDriverAction( 0 );
		return 0;
	case K_MOUSE1:
		Driver_Click( x, y );
		return 0;
	case K_MWHEELUP:
	case K_MWHEELDOWN: {
		float step = key == K_MWHEELUP ? -100.0f : 100.0f;
		if ( s_driver.popup != POPUP_NONE ) s_driver.popupScroll += key == K_MWHEELUP ? -3.0f : 3.0f;
		else if ( s_driver.dialog != DRIVER_NO_DIALOG ) {
			if ( l->listBar && Library_Inside( &l->list, x, y ) ) s_driver.listScroll += step;
			else s_driver.dialogScroll += step;
		} else s_driver.pageScroll += step;
		return 0;
	}
	case K_ENTER:
	case K_KP_ENTER:
		if ( s_driver.popup != POPUP_NONE ) s_driver.popup = POPUP_NONE;
		else if ( s_driver.dialog == DRIVER_EDIT && !s_driver.pending ) Driver_Save();
		else if ( s_driver.dialog == DRIVER_SWITCH && s_driver.focus == FOCUS_NAME && !s_driver.pending ) Driver_Create();
		return 0;
	case K_LEFTARROW:
	case K_RIGHTARROW:
		/* the tabs: arrows move between them */
		if ( s_driver.dialog == DRIVER_NO_DIALOG && s_driver.popup == POPUP_NONE ) {
			s_driver.tab = ( s_driver.tab + ( key == K_RIGHTARROW ? 1 : 2 ) ) % 3;
			s_driver.pageScroll = 0.0f;
		}
		return 0;
	case K_PGUP:
	case K_PGDN:
		if ( s_driver.dialog == DRIVER_NO_DIALOG ) s_driver.pageScroll += ( key == K_PGUP ? -0.875f : 0.875f ) * WS_ViewportHeight();
		return 0;
	}
	return 0;
}

void Driver_Install( menuframework_s *menu ) {
	char preview[64];
	int i, j;

	memset( &s_driver, 0, sizeof( s_driver ) );
	s_driver.menu = menu;
	s_driver.filter = -1;
	/* the countries: "Not specified" first, then by name */
	for ( i = 0; i < (int)DRIVER_COUNTRIES; i++ ) {
		s_driver.countryOrder[i] = i;
		for ( j = i; j > 1 && Q_stricmp( driverCountries[s_driver.countryOrder[j - 1]][1], driverCountries[s_driver.countryOrder[j]][1] ) > 0; j-- ) {
			int swap = s_driver.countryOrder[j];
			s_driver.countryOrder[j] = s_driver.countryOrder[j - 1];
			s_driver.countryOrder[j - 1] = swap;
		}
	}
	Driver_Load();
	s_driver.tab = UI_WebDriverValue( 6, 0 );
	menu->draw = Driver_Draw;
	menu->key = Driver_Key;
	/* ui_driverPreview "tab,filter,page scroll,dialog" pins a state for screenshots:
	 * filter -1 all, dialog 1 the editor, 2 the profiles */
	trap_Cvar_VariableStringBuffer( "ui_driverPreview", preview, sizeof( preview ) );
	if ( preview[0] ) {
		char *field[4];
		int n;
		field[0] = preview;
		for ( n = 1; n < 4; n++ ) {
			field[n] = field[n - 1] ? strchr( field[n - 1], ',' ) : NULL;
			if ( field[n] ) *field[n]++ = '\0';
		}
		s_driver.tab = atoi( field[0] );
		if ( field[1] ) s_driver.filter = atoi( field[1] );
		if ( field[2] ) s_driver.pageScroll = atof( field[2] );
		if ( field[3] && atoi( field[3] ) ) Driver_OpenDialog( atoi( field[3] ) == 1 ? DRIVER_EDIT : DRIVER_SWITCH );
	}
}
