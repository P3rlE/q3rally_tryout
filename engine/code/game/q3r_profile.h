/*
=======================================================================
Copyright (C) 2024 Q3Rally Team (Per Thormann - q3rally@gmail.com)

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
=======================================================================
*/

#ifndef Q3R_PROFILE_H
#define Q3R_PROFILE_H

#include "../qcommon/q_platform.h"

#define Q3R_PROFILE_VERSION 1
#define Q3R_PROFILE_HEADER "Q3RPROF"
#define MAX_ACHIEVEMENTS 32
#define Q3R_NUM_FAVORITE_SLOTS 4

typedef enum {
    ACH_FIRST_RACE_FINISHED,
    ACH_10_RACES_FINISHED,
    ACH_100KM_DRIVEN,
    ACH_10_WINS,
    ACH_DERBY_SPECIALIST,
    // Add more achievement IDs here
    ACH_MAX
} achievement_id_t;

typedef enum {
    ACH_TYPE_ONE_SHOT,
    ACH_TYPE_PROGRESS
} achievement_type_t;

typedef struct {
    int carId;
    int paintId;
    int wheelId;
    int tuningId;
} q3r_favorite_slot_t;

typedef struct {
    int version;
    char playerName[MAX_QPATH];
    int totalRacesStarted;
    int totalRacesFinished;
    int totalRacesWon;
    int totalDerbyMatches;
    int totalDerbyWins;
    int totalPlayTimeSeconds;
    double totalDistanceMeters;
    int totalCrashes;
    int totalCarsDestroyed;
    int totalFlagsCaptured;
    int totalLapsCompleted;
    int lastCarId;
    int favoriteCarId;
    q3r_favorite_slot_t favoriteSlots[Q3R_NUM_FAVORITE_SLOTS];
    struct {
        int id;
        qboolean unlocked;
        int unlockTime;
        float progress;
        float target;
    } achievements[MAX_ACHIEVEMENTS];
} q3r_profile_t;

typedef struct {
    achievement_id_t id;
    const char *key;
    const char *title;
    const char *description;
    achievement_type_t type;
    float target;
} achievement_def_t;

static achievement_def_t achievement_defs[ACH_MAX] = {
    { ACH_FIRST_RACE_FINISHED, "first_race", "First Finish!", "Finish your first race.", ACH_TYPE_ONE_SHOT, 1.0f },
    { ACH_10_RACES_FINISHED, "10_races", "Veteran Racer", "Finish 10 races.", ACH_TYPE_PROGRESS, 10.0f },
    { ACH_100KM_DRIVEN, "100km_driven", "Long Distance Driver", "Drive 100 km total.", ACH_TYPE_PROGRESS, 100000.0f },
    { ACH_10_WINS, "10_wins", "Champion", "Win 10 races.", ACH_TYPE_PROGRESS, 10.0f },
    { ACH_DERBY_SPECIALIST, "derby_specialist", "Derby Specialist", "Win 5 Derby matches.", ACH_TYPE_PROGRESS, 5.0f }
};

#endif // Q3R_PROFILE_H
