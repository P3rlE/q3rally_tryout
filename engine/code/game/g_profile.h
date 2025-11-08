#ifndef G_PROFILE_H
#define G_PROFILE_H

#define PROFILE_FILE_VERSION            3

#define PROFILE_DIRECTORY               "profiles"
#define PROFILE_EXTENSION               ".profile"

#define PROFILE_LIFETIME_DISTANCE_SCALE 100
#define PROFILE_LIFETIME_FUEL_SCALE     100

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
        char vehicleModel[MAX_QPATH];
        char vehicleHead[MAX_QPATH];
        char vehicleRim[MAX_QPATH];
        char vehiclePlate[MAX_QPATH];
} profileLifetime_t;

#endif // G_PROFILE_H
