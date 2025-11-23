#ifndef PROFILE_SHARED_H
#define PROFILE_SHARED_H

#include "../qcommon/q_shared.h"

#define PROFILE_MAX_NAME 32
#define PROFILE_MAX_GENDER 16
#define PROFILE_MAX_BIRTHDATE 16
#define PROFILE_MAX_AVATAR 64
#define PROFILE_MAX_COUNTRY 32
#define PROFILE_MAX_VEHICLE 64
#define PROFILE_MAX_GARAGE_SLOTS 4

typedef struct {
    char model[PROFILE_MAX_VEHICLE];
    char skin[PROFILE_MAX_VEHICLE];
    char setup[PROFILE_MAX_VEHICLE];
    char paint[PROFILE_MAX_VEHICLE];
    char tires[PROFILE_MAX_VEHICLE];
    qboolean favoriteLoadout;
} profile_garage_slot_t;

typedef struct {
    double distanceKm;
    double fuelUsed;
    int bestLapMs;
    int kills;
    int deaths;
    int wins;
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
    char gender[PROFILE_MAX_GENDER];
    char birthDate[PROFILE_MAX_BIRTHDATE];
    char avatar[PROFILE_MAX_AVATAR];
    char country[PROFILE_MAX_COUNTRY];
    int  activeGarageSlot;
    profile_garage_slot_t garageSlots[PROFILE_MAX_GARAGE_SLOTS];
} profile_info_t;

#endif /* PROFILE_SHARED_H */
