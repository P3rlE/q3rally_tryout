#ifndef G_PROFILE_H
#define G_PROFILE_H

#define PROFILE_LIFETIME_DISTANCE_SCALE 100
#define PROFILE_LIFETIME_FUEL_SCALE 100

typedef struct profileLifetime_s {
        int version;
        int matchesPlayed;
        int wins;
        int losses;
        int finishes;
        int dnfs;
        int bestPosition;
        int bestLapMs;
        int bestTotalRaceMs;
        int totalRaceTimeMs;
        int totalScore;
        int totalKills;
        int totalDeaths;
        int totalDamageDealt;
        int totalDamageTaken;
        int totalDistanceScaled;
        int totalFuelConsumedScaled;
        int achievements;
} profileLifetime_t;

#endif // G_PROFILE_H
