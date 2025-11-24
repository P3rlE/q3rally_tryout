#ifndef BG_ACHIEVEMENTS_H
#define BG_ACHIEVEMENTS_H

#include "../qcommon/q_shared.h"

// Icons shared by achievements
typedef enum {
    BG_ACHIEVEMENT_ICON_DRIVEN = 0,
    BG_ACHIEVEMENT_ICON_KILLS,
    BG_ACHIEVEMENT_ICON_WINS,
    BG_ACHIEVEMENT_ICON_FLAGS,
    BG_ACHIEVEMENT_ICON_FLAG_ASSISTS,
    BG_ACHIEVEMENT_ICON_FUEL,
    BG_ACHIEVEMENT_ICON_ACCURACY,
    BG_ACHIEVEMENT_ICON_EXCELLENT,
    BG_ACHIEVEMENT_ICON_IMPRESSIVE,
    BG_ACHIEVEMENT_ICON_PERFECT,
    BG_ACHIEVEMENT_ICON_COUNT
} bgAchievementIcon_t;

typedef struct {
    double threshold;
    const char *name;
    const char *description;
} bgAchievementTierDef_t;

typedef struct {
    const char *title;
    const bgAchievementTierDef_t *tiers;
    int tierCount;
    bgAchievementIcon_t icon;
} bgAchievementCategoryDef_t;

typedef enum {
    BG_ACHIEVEMENT_DISTANCE = 0,
    BG_ACHIEVEMENT_KILLS,
    BG_ACHIEVEMENT_WINS,
    BG_ACHIEVEMENT_FLAG_CAPTURES,
    BG_ACHIEVEMENT_FLAG_ASSISTS,
    BG_ACHIEVEMENT_FUEL,
    BG_ACHIEVEMENT_ACCURACY,
    BG_ACHIEVEMENT_EXCELLENT,
    BG_ACHIEVEMENT_IMPRESSIVE,
    BG_ACHIEVEMENT_PERFECT,
    BG_ACHIEVEMENT_CATEGORY_COUNT
} bgAchievementCategory_t;

#define BG_ACHIEVEMENT_MAX_TIERS 8
#define BG_ACHIEVEMENT_TOTAL_COUNT (BG_ACHIEVEMENT_CATEGORY_COUNT * BG_ACHIEVEMENT_MAX_TIERS)

extern const bgAchievementTierDef_t bg_distanceAchievementTiers[];
extern const bgAchievementTierDef_t bg_killAchievementTiers[];
extern const bgAchievementTierDef_t bg_winAchievementTiers[];
extern const bgAchievementTierDef_t bg_flagCaptureAchievementTiers[];
extern const bgAchievementTierDef_t bg_flagAssistAchievementTiers[];
extern const bgAchievementTierDef_t bg_fuelAchievementTiers[];
extern const bgAchievementTierDef_t bg_accuracyAchievementTiers[];
extern const bgAchievementTierDef_t bg_excellentAchievementTiers[];
extern const bgAchievementTierDef_t bg_impressiveAchievementTiers[];
extern const bgAchievementTierDef_t bg_perfectAchievementTiers[];

extern const char *const bg_achievementMedalLockedPaths[BG_ACHIEVEMENT_ICON_COUNT];
extern const char *const bg_achievementMedalUnlockedPaths[BG_ACHIEVEMENT_ICON_COUNT];

extern const bgAchievementCategoryDef_t bg_achievementCategories[BG_ACHIEVEMENT_CATEGORY_COUNT];

static ID_INLINE int BG_AchievementCategoryCount( void ) {
    return BG_ACHIEVEMENT_CATEGORY_COUNT;
}

static ID_INLINE const bgAchievementCategoryDef_t *BG_AchievementGetCategory( int index ) {
    if ( index < 0 || index >= BG_ACHIEVEMENT_CATEGORY_COUNT ) {
        return NULL;
    }
    return &bg_achievementCategories[index];
}

static ID_INLINE int BG_AchievementTierCount( int categoryIndex ) {
    const bgAchievementCategoryDef_t *category = BG_AchievementGetCategory( categoryIndex );
    return category ? category->tierCount : 0;
}

static ID_INLINE const bgAchievementTierDef_t *BG_AchievementGetTier( int categoryIndex, int tierIndex ) {
    const bgAchievementCategoryDef_t *category = BG_AchievementGetCategory( categoryIndex );
    if ( !category || tierIndex < 0 || tierIndex >= category->tierCount ) {
        return NULL;
    }
    return &category->tiers[tierIndex];
}

static ID_INLINE int BG_AchievementUnlockedTiers( const bgAchievementCategoryDef_t *category, double progress ) {
    int unlocked = 0;
    int i;

    if ( !category ) {
        return 0;
    }

    for ( i = 0; i < category->tierCount; ++i ) {
        if ( progress >= category->tiers[i].threshold ) {
            unlocked = i + 1;
        }
    }

    return unlocked;
}

#endif // BG_ACHIEVEMENTS_H
