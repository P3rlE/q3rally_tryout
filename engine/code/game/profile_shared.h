#ifndef PROFILE_SHARED_H
#define PROFILE_SHARED_H

#include "../qcommon/q_shared.h"

#define PROFILE_MAX_NAME 32
#define PROFILE_MAX_GENDER 16
#define PROFILE_MAX_BIRTHDATE 16
#define PROFILE_MAX_AVATAR 64
#define PROFILE_MAX_COUNTRY 32
#define PROFILE_MAX_VEHICLE 64
#define PROFILE_MAX_FAVORITE_CARS 4
#define PROFILE_MAX_FAVORITE_FIELD MAX_QPATH

typedef struct {
    const char *name;
    int minimumScore;
} profile_rank_def_t;

#define PROFILE_RANK_TABLE( ENTRY ) \
    ENTRY( "Rank 1", 0 )           \
    ENTRY( "Rank 2", 50 )          \
    ENTRY( "Rank 3", 150 )         \
    ENTRY( "Rank 4", 300 )         \
    ENTRY( "Rank 5", 600 )         \
    ENTRY( "Rank 6", 1000 )        \
    ENTRY( "Rank 7", 1600 )        \
    ENTRY( "Rank 8", 2400 )        \
    ENTRY( "Rank 9", 3500 )        \
    ENTRY( "Rank 10", 5000 )       \
    ENTRY( "Rank 11", 7000 )       \
    ENTRY( "Rank 12", 10000 )      \
    ENTRY( "Rank 13", 14000 )      \
    ENTRY( "Rank 14", 20000 )      \
    ENTRY( "Rank 15", 30000 )

typedef struct {
    double distanceKm;
    double fuelUsed;
    int bestLapMs;
    int kills;
    int deaths;
    int wins;
    int playerScore;
    int sprintWins;
    int losses;
    int flagCaptures;
    int flagAssists;
    int accuracyAwards;
    int excellentAwards;
    int impressiveAwards;
    int perfectAwards;
    double topSpeedKph;
    int damageDealt;
    int damageTaken;
    char mostUsedVehicle[PROFILE_MAX_VEHICLE];
    int mostUsedVehicleTimeMs;
} profile_stats_t;

typedef struct {
    char model[PROFILE_MAX_FAVORITE_FIELD];
    char skin[PROFILE_MAX_FAVORITE_FIELD];
    char rim[PROFILE_MAX_FAVORITE_FIELD];
    char head[PROFILE_MAX_FAVORITE_FIELD];
} profile_favorite_car_t;

typedef struct {
    char name[PROFILE_MAX_NAME];
    char gender[PROFILE_MAX_GENDER];
    char birthDate[PROFILE_MAX_BIRTHDATE];
    char avatar[PROFILE_MAX_AVATAR];
    char country[PROFILE_MAX_COUNTRY];
    int  currentRank;
    int  highestRank;
    profile_favorite_car_t favoriteCars[PROFILE_MAX_FAVORITE_CARS];
} profile_info_t;

typedef struct {
    int index;
    const profile_rank_def_t *current;
    const profile_rank_def_t *next;
} profile_rank_t;

static ID_INLINE qboolean Profile_GetRankForScore( const profile_stats_t *stats,
                                                  const profile_rank_def_t *rankDefs,
                                                  int rankDefCount,
                                                  profile_rank_t *outRank ) {
    int i;
    const profile_rank_def_t *current;
    const profile_rank_def_t *next;
    int currentIndex;

    if ( !stats || !rankDefs || rankDefCount <= 0 || !outRank ) {
        return qfalse;
    }

    current = &rankDefs[0];
    next = NULL;
    currentIndex = 0;

    for ( i = 0; i < rankDefCount; ++i ) {
        if ( stats->playerScore >= rankDefs[i].minimumScore ) {
            current = &rankDefs[i];
            currentIndex = i;
        } else {
            next = &rankDefs[i];
            break;
        }
    }

    outRank->index = currentIndex;
    outRank->current = current;
    outRank->next = next;

    return qtrue;
}

#endif /* PROFILE_SHARED_H */
