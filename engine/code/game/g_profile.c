/*
===========================================================================
Copyright (C) 2024 Q3Rally Team

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

#include "g_local.h"
//#include <limits.h>

#ifndef INT_MAX
#define INT_MAX 0x7fffffff
#endif

#ifndef INT_MIN
#define INT_MIN (-INT_MAX - 1)
#endif

#ifdef Q3_VM
#include "bg_lib.h"
#else
#if defined(__has_include)
#if __has_include(<limits.h>)
#include <limits.h>
#endif
#else
#include <limits.h>
#endif
#endif

#ifndef INT_MAX
#define INT_MAX 0x7fffffff
#endif

#ifndef INT_MIN
#define INT_MIN (-INT_MAX - 1)
#endif

#define PROFILE_FILE_VERSION            2
#define PROFILE_DIRECTORY               "profiles"
#define PROFILE_EXTENSION               ".profile"

#define PROFILE_DISTANCE_100KM_METERS   100000.0f
#define PROFILE_DISTANCE_500KM_METERS   500000.0f
#define PROFILE_MATCHES_10              10
#define PROFILE_MATCHES_50              50

#define PROFILE_NOTIFY_DISTANCE_100KM   "achv_distance_100km"
#define PROFILE_NOTIFY_DISTANCE_500KM   "achv_distance_500km"
#define PROFILE_NOTIFY_MATCHES_10       "achv_races_10"
#define PROFILE_NOTIFY_MATCHES_50       "achv_races_50"


typedef enum {
        PROFILE_ACHIEVEMENT_DISTANCE_100KM        = 1 << 0,
        PROFILE_ACHIEVEMENT_DISTANCE_500KM        = 1 << 1,
        PROFILE_ACHIEVEMENT_MATCHES_10            = 1 << 2,
        PROFILE_ACHIEVEMENT_MATCHES_50            = 1 << 3
} profileAchievementBits_t;


typedef struct {
        int                     client;
        int                     score;
        int                     ping;
        int                     time;
        int                     scoreFlags;
        int                     powerUps;
        int                     accuracy;
        int                     impressiveCount;
        int                     impressiveTelefragCount;
        int                     excellentCount;
        int                     gauntletCount;
        int                     defendCount;
        int                     assistCount;
        int                     captures;
        qboolean        perfect;
        int                     team;
        int                     damageDealt;
        int                     damageTaken;
        int                     position;
} score_t;

typedef struct {
        int                     version;
        int                     matchesPlayed;
        int                     wins;
        int                     losses;
        int                     finishes;
        int                     dnfs;
        int                     bestPosition;
        int                     bestLapMs;
        int                     bestTotalRaceMs;
        int                     totalRaceTimeMs;
        int                     totalScore;
        int                     totalKills;
        int                     totalDeaths;
        int                     totalDamageDealt;
        int                     totalDamageTaken;
        float           totalDistanceMeters;
        float           totalFuelConsumed;
        int                     achievements;
} profileData_t;

typedef struct {
        int                     version;
        int                     matchesPlayed;
        int                     wins;
        int                     losses;
        int                     finishes;
        int                     dnfs;
        int                     bestPosition;
        int                     bestLapMs;
        int                     bestTotalRaceMs;
        int                     totalRaceTimeMs;
        int                     totalScore;
        int                     totalKills;
        int                     totalDeaths;
        int                     totalDamageDealt;
        int                     totalDamageTaken;
        float           totalDistanceMeters;
        float           totalFuelConsumed;
        int                     achievements;
} profileDisk_t;

typedef struct {
        int                     version;
        int                     matchesPlayed;
        int                     wins;
        int                     losses;
        int                     finishes;
        int                     dnfs;
        int                     bestPosition;
        int                     bestLapMs;
        int                     bestTotalRaceMs;
        int                     totalRaceTimeMs;
        int                     totalScore;
        int                     totalKills;
        int                     totalDeaths;
        int                     totalDamageDealt;
        int                     totalDamageTaken;
        float           totalDistanceMeters;
        float           totalFuelConsumed;
} profileDiskV1_t;

static int G_ProfileRoundPositiveFloat( float value ) {
        if ( value <= 0.0f ) {
                return 0;
        }

        if ( value >= (float)0x7fffffff ) {
                return 0x7fffffff;
        }

        return (int)( value + 0.5f );
}

static void G_ProfileSendLifetimeCommand( int clientNum, const profileData_t *profile ) {
        char command[MAX_STRING_CHARS];
        char identifier[MAX_QPATH];
        gclient_t *client;
        int distanceMeters;
        int fuelUnits;

        if ( clientNum < 0 || clientNum >= level.maxclients ) {
                return;
        }

        if ( !profile ) {
                return;
        }

        client = &level.clients[clientNum];
        if ( !client ) {
                return;
        }

        if ( !G_ProfileBuildIdentifier( client, identifier, sizeof( identifier ) ) ) {
                identifier[0] = '\0';
        }

        distanceMeters = G_ProfileRoundPositiveFloat( profile->totalDistanceMeters );
        fuelUnits = G_ProfileRoundPositiveFloat( profile->totalFuelConsumed );

        Com_sprintf( command, sizeof( command ),
                "profileLifetime \"%s\" %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
                identifier,
                profile->matchesPlayed,
                profile->wins,
                profile->losses,
                profile->finishes,
                profile->dnfs,
                profile->bestPosition,
                profile->bestLapMs,
                profile->bestTotalRaceMs,
                profile->totalRaceTimeMs,
                profile->totalScore,
                profile->totalKills,
                profile->totalDeaths,
                profile->totalDamageDealt,
                profile->totalDamageTaken,
                distanceMeters,
                fuelUnits,
                profile->achievements );

        trap_SendServerCommand( clientNum, command );
}

static void G_ProfileSetDefaults( profileData_t *profile ) {
        if ( !profile ) {
                return;
        }

        Com_Memset( profile, 0, sizeof( *profile ) );
        profile->version = PROFILE_FILE_VERSION;
}

static void G_ProfileSanitizeComponent( const char *input, char *output, size_t size ) {
        size_t length = 0;

        if ( !output || size == 0 ) {
                return;
        }

        output[0] = '\0';

        if ( !input ) {
                return;
        }

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
}

static qboolean G_ProfileBuildIdentifier( gclient_t *client, char *buffer, size_t size ) {
	char userinfo[MAX_INFO_STRING];
	const char *value;
	char prefix[MAX_QPATH];
	char slot[MAX_QPATH];
	char keepPath[MAX_QPATH];
	int clientNum;
	fileHandle_t file;

	if ( !client || !buffer || size == 0 ) {
		return qfalse;
	}

	clientNum = client - level.clients;
	if ( clientNum < 0 || clientNum >= level.maxclients ) {
		return qfalse;
	}

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	value = Info_ValueForKey( userinfo, "cl_guid" );
	if ( !value || !value[0] ) {
		value = Info_ValueForKey( userinfo, "ip" );
	}
	if ( !value || !value[0] ) {
		value = Info_ValueForKey( userinfo, "name" );
	}
	if ( !value || !value[0] ) {
		value = client->pers.netname;
	}

	G_ProfileSanitizeComponent( value, prefix, sizeof( prefix ) );
	if ( !prefix[0] ) {
		Com_sprintf( prefix, sizeof( prefix ), "client-%i", clientNum );
	}

	G_ProfileSanitizeComponent( client->profileId, slot, sizeof( slot ) );
	if ( !slot[0] ) {
		Q_strncpyz( slot, PROFILE_DEFAULT_SLOT, sizeof( slot ) );
	}

	if ( prefix[0] ) {
		file = 0;
		Com_sprintf( keepPath, sizeof( keepPath ), "%s/%s/.keep", PROFILE_DIRECTORY, prefix );
		trap_FS_FOpenFile( keepPath, &file, FS_APPEND );
		if ( file ) {
			trap_FS_FCloseFile( file );
		}
	}

        Com_sprintf( buffer, size, "%s/%s", prefix, slot );
        return qtrue;
}

static qboolean G_ProfileLoad( const char *identifier, profileData_t *profile );

static int G_ProfileEncodeScaledFloat( float value, float scale ) {
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

static void G_ProfileFillLifetime( const profileData_t *profile, profileLifetime_t *lifetime ) {
        if ( !profile || !lifetime ) {
                return;
        }

        lifetime->version = profile->version;
        lifetime->matchesPlayed = profile->matchesPlayed;
        lifetime->wins = profile->wins;
        lifetime->losses = profile->losses;
        lifetime->finishes = profile->finishes;
        lifetime->dnfs = profile->dnfs;
        lifetime->bestPosition = profile->bestPosition;
        lifetime->bestLapMs = profile->bestLapMs;
        lifetime->bestTotalRaceMs = profile->bestTotalRaceMs;
        lifetime->totalRaceTimeMs = profile->totalRaceTimeMs;
        lifetime->totalScore = profile->totalScore;
        lifetime->totalKills = profile->totalKills;
        lifetime->totalDeaths = profile->totalDeaths;
        lifetime->totalDamageDealt = profile->totalDamageDealt;
        lifetime->totalDamageTaken = profile->totalDamageTaken;
        lifetime->totalDistanceScaled = G_ProfileEncodeScaledFloat( profile->totalDistanceMeters, PROFILE_LIFETIME_DISTANCE_SCALE );
        lifetime->totalFuelConsumedScaled = G_ProfileEncodeScaledFloat( profile->totalFuelConsumed, PROFILE_LIFETIME_FUEL_SCALE );
        lifetime->achievements = profile->achievements;
}

        value = Info_ValueForKey( userinfo, "profile" );
        if ( value && value[0] ) {
                G_ProfileSanitizeComponent( value, sanitized, sizeof( sanitized ) );
                if ( sanitized[0] ) {
                        Q_strncpyz( buffer, sanitized, size );
                        return qtrue;
                }
        }

        value = Info_ValueForKey( userinfo, "profile" );
        if ( value && value[0] ) {
                G_ProfileSanitizeComponent( value, sanitized, sizeof( sanitized ) );
                if ( sanitized[0] ) {
                        Q_strncpyz( buffer, sanitized, size );
                        return qtrue;
                }
        }

        value = Info_ValueForKey( userinfo, "cl_guid" );
        if ( !value || !value[0] ) {
                value = Info_ValueForKey( userinfo, "ip" );
        }

        Com_Memset( lifetime, 0, sizeof( *lifetime ) );

        if ( !client ) {
                return qfalse;
        }

        if ( !G_ProfileBuildIdentifier( client, identifier, sizeof( identifier ) ) ) {
                lifetime->version = PROFILE_FILE_VERSION;
                return qfalse;
        }

        Com_Memset( &profile, 0, sizeof( profile ) );
        G_ProfileLoad( identifier, &profile );
        G_ProfileFillLifetime( &profile, lifetime );
        return qtrue;
}

void G_ProfileSendLifetimeCommand( int clientNum, const profileLifetime_t *lifetime ) {
        char command[MAX_STRING_CHARS];
        gentity_t *ent;
        gclient_t *client;

        value = Info_ValueForKey( userinfo, "profile" );
        if ( value && value[0] ) {
                G_ProfileSanitizeComponent( value, sanitized, sizeof( sanitized ) );
                if ( sanitized[0] ) {
                        Q_strncpyz( buffer, sanitized, size );
                        return qtrue;
                }
        }

        value = Info_ValueForKey( userinfo, "cl_guid" );
        if ( !value || !value[0] ) {
                value = Info_ValueForKey( userinfo, "ip" );
        }

        if ( clientNum < 0 || clientNum >= level.maxclients ) {
                return;
        }

        ent = &g_entities[clientNum];
        client = &level.clients[clientNum];

        if ( !ent || !ent->inuse ) {
                return;
        }

        if ( ent->r.svFlags & SVF_BOT ) {
                return;
        }

        if ( client->pers.connected != CON_CONNECTED ) {
                return;
        }

        Com_sprintf( command, sizeof( command ),
                     "profileStats %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                     lifetime->version,
                     lifetime->matchesPlayed,
                     lifetime->wins,
                     lifetime->losses,
                     lifetime->finishes,
                     lifetime->dnfs,
                     lifetime->bestPosition,
                     lifetime->bestLapMs,
                     lifetime->bestTotalRaceMs,
                     lifetime->totalRaceTimeMs,
                     lifetime->totalScore,
                     lifetime->totalKills,
                     lifetime->totalDeaths,
                     lifetime->totalDamageDealt,
                     lifetime->totalDamageTaken,
                     lifetime->totalDistanceScaled,
                     lifetime->totalFuelConsumedScaled,
                     lifetime->achievements );
        trap_SendServerCommand( clientNum, command );
}

static void G_ProfileSerialize( const profileData_t *profile, profileDisk_t *disk ) {
        if ( !profile || !disk ) {
                return;
        }

        disk->version = LittleLong( profile->version );
        disk->matchesPlayed = LittleLong( profile->matchesPlayed );
        disk->wins = LittleLong( profile->wins );
        disk->losses = LittleLong( profile->losses );
        disk->finishes = LittleLong( profile->finishes );
        disk->dnfs = LittleLong( profile->dnfs );
        disk->bestPosition = LittleLong( profile->bestPosition );
        disk->bestLapMs = LittleLong( profile->bestLapMs );
        disk->bestTotalRaceMs = LittleLong( profile->bestTotalRaceMs );
        disk->totalRaceTimeMs = LittleLong( profile->totalRaceTimeMs );
        disk->totalScore = LittleLong( profile->totalScore );
        disk->totalKills = LittleLong( profile->totalKills );
        disk->totalDeaths = LittleLong( profile->totalDeaths );
        disk->totalDamageDealt = LittleLong( profile->totalDamageDealt );
        disk->totalDamageTaken = LittleLong( profile->totalDamageTaken );
        disk->totalDistanceMeters = LittleFloat( profile->totalDistanceMeters );
        disk->totalFuelConsumed = LittleFloat( profile->totalFuelConsumed );
        disk->achievements = LittleLong( profile->achievements );
}

static void G_ProfileDeserialize( profileData_t *profile, const profileDisk_t *disk ) {
        if ( !profile || !disk ) {
                return;
        }

        profile->version = LittleLong( disk->version );
        profile->matchesPlayed = LittleLong( disk->matchesPlayed );
        profile->wins = LittleLong( disk->wins );
        profile->losses = LittleLong( disk->losses );
        profile->finishes = LittleLong( disk->finishes );
        profile->dnfs = LittleLong( disk->dnfs );
        profile->bestPosition = LittleLong( disk->bestPosition );
        profile->bestLapMs = LittleLong( disk->bestLapMs );
        profile->bestTotalRaceMs = LittleLong( disk->bestTotalRaceMs );
        profile->totalRaceTimeMs = LittleLong( disk->totalRaceTimeMs );
        profile->totalScore = LittleLong( disk->totalScore );
        profile->totalKills = LittleLong( disk->totalKills );
        profile->totalDeaths = LittleLong( disk->totalDeaths );
        profile->totalDamageDealt = LittleLong( disk->totalDamageDealt );
        profile->totalDamageTaken = LittleLong( disk->totalDamageTaken );
        profile->totalDistanceMeters = LittleFloat( disk->totalDistanceMeters );
        profile->totalFuelConsumed = LittleFloat( disk->totalFuelConsumed );
        profile->achievements = LittleLong( disk->achievements );
}

static void G_ProfileDeserializeV1( profileData_t *profile, const profileDiskV1_t *disk ) {
        if ( !profile || !disk ) {
                return;
        }

        profile->version = LittleLong( disk->version );
        profile->matchesPlayed = LittleLong( disk->matchesPlayed );
        profile->wins = LittleLong( disk->wins );
        profile->losses = LittleLong( disk->losses );
        profile->finishes = LittleLong( disk->finishes );
        profile->dnfs = LittleLong( disk->dnfs );
        profile->bestPosition = LittleLong( disk->bestPosition );
        profile->bestLapMs = LittleLong( disk->bestLapMs );
        profile->bestTotalRaceMs = LittleLong( disk->bestTotalRaceMs );
        profile->totalRaceTimeMs = LittleLong( disk->totalRaceTimeMs );
        profile->totalScore = LittleLong( disk->totalScore );
        profile->totalKills = LittleLong( disk->totalKills );
        profile->totalDeaths = LittleLong( disk->totalDeaths );
        profile->totalDamageDealt = LittleLong( disk->totalDamageDealt );
        profile->totalDamageTaken = LittleLong( disk->totalDamageTaken );
        profile->totalDistanceMeters = LittleFloat( disk->totalDistanceMeters );
        profile->totalFuelConsumed = LittleFloat( disk->totalFuelConsumed );
        profile->achievements = 0;
}

static void G_ProfileSendAchievementEvent( int clientNum, const char *identifier ) {
        if ( !identifier || !identifier[0] ) {
                return;
        }

        if ( clientNum < 0 || clientNum >= level.maxclients ) {
                return;
        }

        trap_SendServerCommand( clientNum, va( "notify \"%s\"", identifier ) );
}

static void G_ProfileProcessAchievements( gclient_t *client, int clientNum, profileData_t *profile, int previousAchievements ) {
        int unlocked;

        if ( !client || !profile ) {
                return;
        }

        if ( profile->totalDistanceMeters >= PROFILE_DISTANCE_500KM_METERS ) {
                profile->achievements |= PROFILE_ACHIEVEMENT_DISTANCE_500KM;
        }
        if ( profile->totalDistanceMeters >= PROFILE_DISTANCE_100KM_METERS ) {
                profile->achievements |= PROFILE_ACHIEVEMENT_DISTANCE_100KM;
        }
        if ( profile->matchesPlayed >= PROFILE_MATCHES_50 ) {
                profile->achievements |= PROFILE_ACHIEVEMENT_MATCHES_50;
        }
        if ( profile->matchesPlayed >= PROFILE_MATCHES_10 ) {
                profile->achievements |= PROFILE_ACHIEVEMENT_MATCHES_10;
        }

        unlocked = profile->achievements & ~previousAchievements;
        if ( !unlocked ) {
                return;
        }

        if ( unlocked & PROFILE_ACHIEVEMENT_DISTANCE_100KM ) {
                G_ProfileSendAchievementEvent( clientNum, PROFILE_NOTIFY_DISTANCE_100KM );
        }
        if ( unlocked & PROFILE_ACHIEVEMENT_DISTANCE_500KM ) {
                G_ProfileSendAchievementEvent( clientNum, PROFILE_NOTIFY_DISTANCE_500KM );
        }
        if ( unlocked & PROFILE_ACHIEVEMENT_MATCHES_10 ) {
                G_ProfileSendAchievementEvent( clientNum, PROFILE_NOTIFY_MATCHES_10 );
        }
        if ( unlocked & PROFILE_ACHIEVEMENT_MATCHES_50 ) {
                G_ProfileSendAchievementEvent( clientNum, PROFILE_NOTIFY_MATCHES_50 );
        }
}

static void G_ProfileBuildPath( const char *identifier, char *path, size_t size ) {
        if ( !path || size == 0 ) {
                return;
        }

        if ( !identifier || !identifier[0] ) {
                Q_strncpyz( path, PROFILE_DIRECTORY "/unknown" PROFILE_EXTENSION, size );
                return;
        }

        Com_sprintf( path, size, "%s/%s%s", PROFILE_DIRECTORY, identifier, PROFILE_EXTENSION );
}

static qboolean G_ProfileLoad( const char *identifier, profileData_t *profile ) {
        fileHandle_t file;
        int length;
        char path[MAX_QPATH];
        qboolean loaded = qfalse;

        if ( !profile ) {
                return qfalse;
        }

        G_ProfileSetDefaults( profile );

        if ( !identifier || !identifier[0] ) {
                return qfalse;
        }

        G_ProfileBuildPath( identifier, path, sizeof( path ) );

        length = trap_FS_FOpenFile( path, &file, FS_READ );
        if ( !file ) {
                return qfalse;
        }

        if ( length == sizeof( profileDisk_t ) ) {
                profileDisk_t disk;

                Com_Memset( &disk, 0, sizeof( disk ) );
                trap_FS_Read( &disk, sizeof( disk ), file );
                G_ProfileDeserialize( profile, &disk );
                loaded = qtrue;
        } else if ( length == sizeof( profileDiskV1_t ) ) {
                profileDiskV1_t diskV1;

                Com_Memset( &diskV1, 0, sizeof( diskV1 ) );
                trap_FS_Read( &diskV1, sizeof( diskV1 ), file );
                G_ProfileDeserializeV1( profile, &diskV1 );
                loaded = qtrue;
        } else {
                trap_FS_FCloseFile( file );
                Com_Printf( "Profile: ignoring '%s' with unexpected size (%i)\n", path, length );
                return qfalse;
        }

        trap_FS_FCloseFile( file );

        if ( profile->version != PROFILE_FILE_VERSION ) {
                if ( profile->version == 1 ) {
                        profile->version = PROFILE_FILE_VERSION;
                } else {
                        Com_Printf( "Profile: ignoring '%s' with unsupported version %i\n", path, profile->version );
                        G_ProfileSetDefaults( profile );
                        return qfalse;
                }
        }

        if ( loaded ) {
                profile->version = PROFILE_FILE_VERSION;
        }

        return loaded;
}

static qboolean G_ProfileSave( const char *identifier, const profileData_t *profile ) {
        profileDisk_t disk;
        profileData_t temp;
        fileHandle_t file;
        char path[MAX_QPATH];

        if ( !profile || !identifier || !identifier[0] ) {
                return qfalse;
        }

        G_ProfileBuildPath( identifier, path, sizeof( path ) );

        file = 0;
        trap_FS_FOpenFile( path, &file, FS_WRITE );
        if ( !file ) {
                Com_Printf( "Profile: failed to open '%s' for writing\n", path );
                return qfalse;
        }

        Com_Memcpy( &temp, profile, sizeof( temp ) );
        temp.version = PROFILE_FILE_VERSION;
        G_ProfileSerialize( &temp, &disk );
        trap_FS_Write( &disk, sizeof( disk ), file );
        trap_FS_FCloseFile( file );
        return qtrue;
}

static void G_ProfileBuildScore( int clientNum, score_t *score ) {
        gclient_t *cl;
        gentity_t *ent;

        if ( !score ) {
                return;
        }

        Com_Memset( score, 0, sizeof( *score ) );

        if ( clientNum < 0 || clientNum >= level.maxclients ) {
                return;
        }

        cl = &level.clients[clientNum];
        ent = &g_entities[clientNum];

        score->client = clientNum;
        score->score = cl->ps.persistant[PERS_SCORE];
        if ( cl->pers.connected == CON_CONNECTING ) {
                score->ping = -1;
        } else {
                score->ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
        }

        if ( isRallyRace() || g_gametype.integer == GT_DERBY || g_gametype.integer == GT_LCS ) {
                score->time = level.startRaceTime;
        } else {
                score->time = cl->pers.enterTime;
        }

        score->scoreFlags = cl->ps.persistant[PERS_PLAYEREVENTS];
        score->powerUps = ent->s.powerups;
        if ( cl->accuracy_shots > 0 ) {
                score->accuracy = ( cl->accuracy_hits * 100 ) / cl->accuracy_shots;
        }
        score->impressiveCount = cl->ps.persistant[PERS_IMPRESSIVE_COUNT];
        score->impressiveTelefragCount = cl->ps.persistant[PERS_IMPRESSIVETELEFRAG_COUNT];
        score->excellentCount = cl->ps.persistant[PERS_EXCELLENT_COUNT];
        score->gauntletCount = cl->ps.persistant[PERS_GAUNTLET_FRAG_COUNT];
        score->defendCount = cl->ps.persistant[PERS_DEFEND_COUNT];
        score->assistCount = cl->ps.persistant[PERS_ASSIST_COUNT];
        score->captures = cl->ps.persistant[PERS_CAPTURES];
        score->perfect = ( cl->ps.persistant[PERS_RANK] == 0 && cl->ps.persistant[PERS_KILLED] == 0 ) ? qtrue : qfalse;
        score->team = cl->sess.sessionTeam;
        score->damageDealt = cl->ps.stats[STAT_DAMAGE_DEALT];
        score->damageTaken = cl->ps.stats[STAT_DAMAGE_TAKEN];
        score->position = cl->ps.stats[STAT_POSITION];
}

static qboolean G_ProfileIsRaceGametype( void ) {
        return ( isRallyRace() ||
                 g_gametype.integer == GT_DERBY ||
                 g_gametype.integer == GT_LCS ||
                 g_gametype.integer == GT_ELIMINATION );
}

static float G_ProfileComputeDistance( gclient_t *client ) {
        float trackMeters;
        float totalMeters;
        float remain;

        if ( !client ) {
                return 0.0f;
        }

        if ( client->profileDistanceAccum > 0.0f ) {
                return client->profileDistanceAccum;
        }

        if ( level.trackLength <= 0.0f ) {
                return 0.0f;
        }

        trackMeters = level.trackLength / CP_M_2_QU;
        if ( level.numberOfLaps > 0 ) {
                totalMeters = trackMeters * level.numberOfLaps;
                remain = (float)client->ps.stats[STAT_DISTANCE_REMAIN];
                if ( remain < 0.0f ) {
                        remain = 0.0f;
                }
                if ( remain > totalMeters ) {
                        remain = totalMeters;
                }
                return totalMeters - remain;
        }

        if ( client->recordedLapCount > 0 ) {
                return trackMeters * client->recordedLapCount;
        }

        return 0.0f;
}

static float G_ProfileComputeFuelConsumed( gclient_t *client ) {
        float consumed;

        if ( !client ) {
                return 0.0f;
        }

        if ( client->profileFuelUsedAccum > 0.0f ) {
                return client->profileFuelUsedAccum;
        }

        consumed = client->car.maxFuel - client->car.fuel;
        if ( consumed < 0.0f ) {
                consumed = 0.0f;
        }
        return consumed;
}

static int G_ProfileComputeRaceTime( gclient_t *client ) {
        if ( !client ) {
                return 0;
        }

        if ( level.startRaceTime <= 0 ) {
                return 0;
        }

        if ( client->finishRaceTime > level.startRaceTime ) {
                return client->finishRaceTime - level.startRaceTime;
        }

        if ( level.finishRaceTime > level.startRaceTime ) {
                return level.finishRaceTime - level.startRaceTime;
        }

        return 0;
}

static qboolean G_ProfileDidParticipate( gclient_t *client ) {
        if ( !client ) {
                return qfalse;
        }

        if ( client->pers.connected != CON_CONNECTED ) {
                return qfalse;
        }

        if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
                return qfalse;
        }

        return qtrue;
}

static qboolean G_ProfileDidFinishRace( gclient_t *client ) {
        if ( !client ) {
                return qfalse;
        }

        if ( level.startRaceTime <= 0 ) {
                return qtrue;
        }

        if ( client->finishRaceTime > level.startRaceTime ) {
                return qtrue;
        }

        return qfalse;
}

static qboolean G_ProfileIsWinner( int clientNum, const score_t *score ) {
        if ( score && score->position == 1 ) {
                return qtrue;
        }

        if ( level.winnerNumber == clientNum && clientNum >= 0 ) {
                return qtrue;
        }

        if ( level.numNonSpectatorClients > 0 && level.sortedClients[0] == clientNum ) {
                return qtrue;
        }

        return qfalse;
}

static void G_ProfileApplyMatchStats( gclient_t *client, int clientNum, const score_t *score, profileData_t *profile ) {
        qboolean raceGametype;
        qboolean participated;
        qboolean finished = qtrue;
        qboolean winner = qfalse;
        int totalRaceMs = 0;
        int kills;
        int deaths;
        float distanceMeters = 0.0f;
        float fuelConsumed = 0.0f;

        if ( !client || !profile ) {
                return;
        }

        participated = G_ProfileDidParticipate( client );
        if ( !participated ) {
                return;
        }

        raceGametype = G_ProfileIsRaceGametype();

        profile->matchesPlayed++;

        if ( raceGametype ) {
                finished = G_ProfileDidFinishRace( client );
                if ( finished ) {
                        profile->finishes++;
                } else {
                        profile->dnfs++;
                }

                distanceMeters = G_ProfileComputeDistance( client );
                fuelConsumed = G_ProfileComputeFuelConsumed( client );

                if ( finished ) {
                        totalRaceMs = G_ProfileComputeRaceTime( client );
                        if ( totalRaceMs > 0 ) {
                                profile->totalRaceTimeMs += totalRaceMs;
                                if ( profile->bestTotalRaceMs == 0 || totalRaceMs < profile->bestTotalRaceMs ) {
                                        profile->bestTotalRaceMs = totalRaceMs;
                                }
                        }
                }

                if ( client->bestLapMs > 0 && ( profile->bestLapMs == 0 || client->bestLapMs < profile->bestLapMs ) ) {
                        profile->bestLapMs = client->bestLapMs;
                }
        } else {
                profile->finishes++;
        }

        if ( score ) {
                if ( score->position > 0 && ( profile->bestPosition == 0 || score->position < profile->bestPosition ) ) {
                        profile->bestPosition = score->position;
                }
                profile->totalScore += score->score;
                profile->totalDamageDealt += score->damageDealt;
                profile->totalDamageTaken += score->damageTaken;
        }

        profile->totalDistanceMeters += distanceMeters;
        profile->totalFuelConsumed += fuelConsumed;

        kills = client->ps.persistant[PERS_SCORE];
        deaths = client->ps.persistant[PERS_KILLED];
        profile->totalKills += kills;
        profile->totalDeaths += deaths;

        winner = G_ProfileIsWinner( clientNum, score );
        if ( winner ) {
                profile->wins++;
        } else {
                profile->losses++;
        }
}

void G_ProfileUpdateForClient( gclient_t *client ) {
        profileData_t profile;
        score_t score;
        char identifier[MAX_QPATH];
        int clientNum;
        gentity_t *ent;
        int previousAchievements;

        if ( !client ) {
                return;
        }

        clientNum = client - level.clients;
        if ( clientNum < 0 || clientNum >= level.maxclients ) {
                return;
        }

        ent = &g_entities[clientNum];
        if ( !ent || !ent->inuse || ent->r.svFlags & SVF_BOT ) {
                return;
        }

        if ( !G_ProfileBuildIdentifier( client, identifier, sizeof( identifier ) ) ) {
                return;
        }

        G_ProfileLoad( identifier, &profile );
        G_ProfileFillLifetime( &profile, &client->profileLifetime );
        previousAchievements = profile.achievements;
        G_ProfileBuildScore( clientNum, &score );
        G_ProfileApplyMatchStats( client, clientNum, &score, &profile );
        G_ProfileFillLifetime( &profile, &client->profileLifetime );
        G_ProfileProcessAchievements( client, clientNum, &profile, previousAchievements );
        G_ProfileSendLifetimeCommand( clientNum, &profile );

        if ( !G_ProfileSave( identifier, &profile ) ) {
                Com_Printf( "Profile: failed to persist statistics for '%s'\n", identifier );
        }

        G_ProfileSendLifetimeCommand( clientNum, &client->profileLifetime );
}
