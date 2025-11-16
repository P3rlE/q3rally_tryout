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
#define ART_MEDAL_DRIVEN_LOCKED         "menu/achievements/medal_driven_locked"
#define ART_MEDAL_DRIVEN_UNLOCKED       "menu/achievements/medal_driven_unlocked"
#define ART_MEDAL_KILLS_LOCKED          "menu/achievements/medal_kills_locked"
#define ART_MEDAL_KILLS_UNLOCKED        "menu/achievements/medal_kills_unlocked"
#define ART_MEDAL_WINS_LOCKED           "menu/achievements/medal_wins_locked"
#define ART_MEDAL_WINS_UNLOCKED         "menu/achievements/medal_wins_unlocked"
#define ART_MEDAL_FLAGS_LOCKED          "menu/achievements/medal_flags_locked"
#define ART_MEDAL_FLAGS_UNLOCKED        "menu/achievements/medal_flags_unlocked"
#define ART_MEDAL_ASSISTS_LOCKED        "menu/achievements/medal_assists_locked"
#define ART_MEDAL_ASSISTS_UNLOCKED      "menu/achievements/medal_assists_unlocked"
#define ART_MEDAL_FUEL_LOCKED           ART_MEDAL_DRIVEN_LOCKED
#define ART_MEDAL_FUEL_UNLOCKED         ART_MEDAL_DRIVEN_UNLOCKED

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

#define ID_TAB_PROFILE		30
#define ID_TAB_VEHICLE		31
#define ID_TAB_STATS		32
#define ID_TAB_ACHIEVEMENTS	33
#define ID_GENDER		40
#define ID_BIRTH_DAY		41
#define ID_BIRTH_MONTH		42
#define ID_BIRTH_YEAR		43

#define TAB_PROFILE		0
#define TAB_VEHICLE		1
#define TAB_STATS		2
#define TAB_ACHIEVEMENTS	3

#define PLAYERSETTINGS_TAB_COUNT		4
#define PLAYERSETTINGS_TAB_WIDTH		120
#define PLAYERSETTINGS_TAB_GAP		12
#define PLAYERSETTINGS_TAB_TOP		64
#define PLAYERSETTINGS_TAB_HEIGHT	32
#define PLAYERSETTINGS_TAB_TEXT_OFFSET	8
#define PLAYERSETTINGS_CONTENT_TOP	( PLAYERSETTINGS_TAB_TOP + PLAYERSETTINGS_TAB_HEIGHT + 18 )

#define PLAYERSETTINGS_PROFILE_PANEL_LEFT		24
#define PLAYERSETTINGS_PROFILE_PANEL_TOP		112
#define PLAYERSETTINGS_PROFILE_PANEL_WIDTH		592
#define PLAYERSETTINGS_PROFILE_PANEL_INNER_MARGIN	6
#define PLAYERSETTINGS_PROFILE_PANEL_BOTTOM_EXTRA	16
#define PLAYERSETTINGS_PROFILE_FIELD_LEFT		( PLAYERSETTINGS_PROFILE_PANEL_LEFT + PLAYERSETTINGS_PROFILE_PANEL_INNER_MARGIN )
#define PLAYERSETTINGS_PROFILE_ROW_RIGHT		( PLAYERSETTINGS_PROFILE_PANEL_LEFT + PLAYERSETTINGS_PROFILE_PANEL_WIDTH - PLAYERSETTINGS_PROFILE_PANEL_INNER_MARGIN )
#define PLAYERSETTINGS_PROFILE_LABEL_OFFSET	16
#define PLAYERSETTINGS_PROFILE_VALUE_OFFSET	32
#define PLAYERSETTINGS_PROFILE_VALUE_BASELINE	18
#define PLAYERSETTINGS_PROFILE_FIELD_HEIGHT	36
#define PLAYERSETTINGS_PROFILE_ROW_HEIGHT		44

#define PLAYERSETTINGS_BACK_BUTTON_LEFT			PLAYERSETTINGS_PROFILE_FIELD_LEFT
#define PLAYERSETTINGS_BACK_BUTTON_Y			( PLAYERSETTINGS_TAB_TOP + PLAYERSETTINGS_TAB_HEIGHT + 14 )

#define PLAYERSETTINGS_MAX_ACHIEVEMENT_TIERS            8
#define PLAYERSETTINGS_ACHIEVEMENT_MEDAL_SIZE           48.0f
#define PLAYERSETTINGS_ACHIEVEMENT_ROW_GAP              16
#define PLAYERSETTINGS_ACHIEVEMENT_TITLE_OFFSET         6
#define PLAYERSETTINGS_ACHIEVEMENT_HEADER_LINE_HEIGHT   32.0f
#define PLAYERSETTINGS_ACHIEVEMENT_HEADER_GAP           8.0f
#define PLAYERSETTINGS_ACHIEVEMENT_HEADER_BLOCK_HEIGHT  ( PLAYERSETTINGS_ACHIEVEMENT_HEADER_LINE_HEIGHT + 8.0f )
#define PLAYERSETTINGS_ACHIEVEMENT_ENTRY_VERTICAL_GAP   24.0f
#define PLAYERSETTINGS_ACHIEVEMENT_COLUMN_GAP           32.0f
#define PLAYERSETTINGS_ACHIEVEMENT_TEXT_GAP             24.0f
#define PLAYERSETTINGS_ACHIEVEMENT_TEXT_LINE_HEIGHT     12.0f
#define PLAYERSETTINGS_ACHIEVEMENT_TEXT_SCALE_MULTIPLIER        0.6f
#define PLAYERSETTINGS_ACHIEVEMENT_VALUE_BASELINE       PLAYERSETTINGS_PROFILE_VALUE_BASELINE

#define PLAYERSETTINGS_STATS_ROW_HEIGHT		40
#define PLAYERSETTINGS_STATS_ROW_GAP		4
#define PLAYERSETTINGS_STATS_VALUE_OFFSET		260
#define PLAYERSETTINGS_STATS_VALUE_BASELINE		PLAYERSETTINGS_PROFILE_VALUE_BASELINE

#define PLAYERSETTINGS_ACHIEVEMENT_CATEGORY_COUNT       6
#define PLAYERSETTINGS_ACHIEVEMENT_SECTION_COUNT        PLAYERSETTINGS_ACHIEVEMENT_CATEGORY_COUNT
#define PLAYERSETTINGS_ACHIEVEMENT_ROW_COUNT            PLAYERSETTINGS_ACHIEVEMENT_SECTION_COUNT
#define PLAYERSETTINGS_ACHIEVEMENT_CONTENT_MARGIN	0.0f

#define MAX_NAMELENGTH	20
// STONELANCE
#define NUM_FAVORITES		4
#define MAX_PLAYERMODELS	256
#define BIRTH_YEAR_START	1950
#define BIRTH_YEAR_END	2100
#define BIRTH_YEAR_COUNT	( ( BIRTH_YEAR_END ) - ( BIRTH_YEAR_START ) + 1 )
#define BIRTH_DAY_MAX	31
// END


static vec4_t achievementUnlockedColor = { 0.6f, 1.0f, 0.6f, 1.0f };
static vec4_t achievementLockedColor = { 0.7f, 0.7f, 0.7f, 1.0f };
static vec4_t tabBackgroundColor = { 0.05f, 0.05f, 0.05f, 0.85f };
static vec4_t tabSelectedBackgroundColor = { 0.16f, 0.16f, 0.16f, 0.95f };
static vec4_t tabBorderColor = { 0.45f, 0.45f, 0.45f, 1.0f };
static vec4_t tabSelectedBorderColor = { 1.0f, 0.8f, 0.3f, 1.0f };
static vec4_t tabFocusBorderColor = { 0.8f, 0.8f, 0.8f, 1.0f };
static vec4_t profilePanelFillColor = { 0.03f, 0.03f, 0.03f, 0.80f };
static vec4_t profileRowEvenFillColor = { 0.10f, 0.10f, 0.10f, 0.60f };
static vec4_t profileRowOddFillColor = { 0.14f, 0.14f, 0.14f, 0.60f };
static vec4_t profileRowBorderColor = { 0.35f, 0.35f, 0.35f, 0.85f };
static vec4_t avatarImageBackgroundColor = { 0.05f, 0.05f, 0.05f, 0.80f };
static vec4_t avatarImageMissingColor = { 0.6f, 0.2f, 0.2f, 1.0f };

typedef struct {
	double		threshold;
	const char		*name;
	const char		*description;
} playersettingsAchievementTierDef_t;

static const playersettingsAchievementTierDef_t s_distanceAchievementTiers[] = {
        { 10.0, "Sunday Driver", "Cover a relaxed 10 km overall." },
        { 50.0, "Daily Commuter", "Cruise through a combined 50 km." },
        { 150.0, "Road Tripper", "Accumulate 150 km behind the wheel." },
        { 300.0, "Night Rider", "Push past 300 km of distance." },
        { 600.0, "Highway Hero", "Rack up 600 km on the odometer." },
        { 1500.0, "Endurance Ace", "Stay in the race for 1,500 km total." },
        { 5000.0, "Globetrotter", "Log an epic 5,000 km journey." },
        { 10000.0, "Legend of Asphalt", "Master the asphalt for 10,000 km." }
};

static const playersettingsAchievementTierDef_t s_killAchievementTiers[] = {
        { 10.0, "Spark Starter", "Score 10 takedowns." },
        { 25.0, "Arc Blazer", "Deliver 25 total kills." },
        { 50.0, "Demolition Driver", "Knock rivals out 50 times." },
        { 100.0, "Pit Boss", "Dominate the arena with 100 kills." },
        { 250.0, "Arena Menace", "Send 250 opponents packing." },
        { 500.0, "Road Reaper", "Leave 500 wrecks behind." },
        { 1000.0, "Overdrive Executioner", "Achieve 1,000 eliminations." },
        { 2500.0, "Apocalypse Engine", "Crush 2,500 opponents." }
};

static const playersettingsAchievementTierDef_t s_winAchievementTiers[] = {
        { 1.0, "Checkered Debut", "Win your very first race." },
        { 3.0, "Podium Regular", "Collect 3 total victories." },
        { 5.0, "Sprint Specialist", "Take home 5 wins." },
        { 10.0, "Championship Hopeful", "Secure 10 race wins." },
        { 20.0, "Series Star", "Reach 20 gold finishes." },
        { 30.0, "Circuit Royalty", "Earn 30 overall wins." },
        { 40.0, "Dynasty Driver", "Stack up 40 victories." },
        { 50.0, "Hall of Fame", "Celebrate 50 race wins." }
};

static const playersettingsAchievementTierDef_t s_flagCaptureAchievementTiers[] = {
        { 1.0, "Flag Rookie", "Capture your first flag." },
        { 5.0, "Fast Courier", "Deliver 5 flags to base." },
        { 10.0, "Relay Racer", "Bank 10 successful captures." },
        { 25.0, "Siege Runner", "Snatch 25 flags." },
        { 50.0, "Banner Bandit", "Swipe 50 flags." },
        { 75.0, "Frontline Phantom", "Steal 75 flags unnoticed." },
        { 100.0, "Flagship", "Secure 100 captures." },
        { 150.0, "Mythic Messenger", "Run home 150 flags." }
};

static const playersettingsAchievementTierDef_t s_flagAssistAchievementTiers[] = {
        { 1.0, "Helping Hand", "Assist with 1 flag capture." },
        { 5.0, "Wingman", "Support 5 flag scores." },
        { 10.0, "Shield Mate", "Help with 10 captures." },
        { 25.0, "Escort Elite", "Escort 25 flags safely." },
        { 50.0, "Guardian Angel", "Guide 50 flags home." },
        { 75.0, "Formation Leader", "Add 75 total assists." },
        { 100.0, "Tactical Anchor", "Reach 100 flag assists." },
        { 150.0, "Legendary Support", "Record 150 flag assists." }
};

static const playersettingsAchievementTierDef_t s_fuelAchievementTiers[] = {
        { 10.0, "Fuel Sipper", "Burn through 10 L of fuel." },
        { 50.0, "Tank Tipper", "Spend 50 L on the throttle." },
        { 100.0, "Octane Addict", "Consume 100 L overall." },
        { 250.0, "Combustion Captain", "Use 250 L chasing speed." },
        { 500.0, "Turbo Baron", "Torch 500 L of fuel." },
        { 1000.0, "Inferno Investor", "Pour 1,000 L into momentum." },
        { 2500.0, "Petrol Pharaoh", "Spend 2,500 L keeping pace." },
        { 5000.0, "Galaxy Guzzler", "Atomize 5,000 L in total." }
};

typedef enum {
        PLAYERSETTINGS_ACHIEVEMENT_ICON_DRIVEN = 0,
        PLAYERSETTINGS_ACHIEVEMENT_ICON_KILLS,
        PLAYERSETTINGS_ACHIEVEMENT_ICON_WINS,
        PLAYERSETTINGS_ACHIEVEMENT_ICON_FLAGS,
        PLAYERSETTINGS_ACHIEVEMENT_ICON_FLAG_ASSISTS,
        PLAYERSETTINGS_ACHIEVEMENT_ICON_FUEL,
        PLAYERSETTINGS_ACHIEVEMENT_ICON_COUNT
} playersettingsAchievementIcon_t;

typedef struct {
	int	columns;
	int	rows;
	float	columnWidth;
	float	columnGap;
} playersettingsAchievementGridLayout_t;

typedef struct playersettings_scroll_state_s {
	float	offset;
	float	targetOffset;
	qboolean	dragging;
	float	dragStartOffset;
} playersettingsScrollState_t;

static void PlayerSettings_DrawStatsLabelValue( int row, const char *label, const char *value );
static void PlayerSettings_DrawStatsMessage( int row, const char *message );
static void PlayerSettings_DrawAchievementsPanelBackground( void );
static void PlayerSettings_DrawAchievementsTab( void );
static void PlayerSettings_GetAchievementRowBounds( int row, int *top, int *bottom );
static void PlayerSettings_GetAchievementsHeaderBounds( int *top, int *bottom );
static void PlayerSettings_UpdateScrollDrag( playersettingsScrollState_t *state, float contentHeight );
static int PlayerSettings_GetAchievementSectionTierCount( int row ) {
	int	count;

	switch ( row ) {
	case 0:
		count = ARRAY_LEN( s_distanceAchievementTiers );
		break;
	case 1:
		count = ARRAY_LEN( s_killAchievementTiers );
		break;
	case 2:
		count = ARRAY_LEN( s_winAchievementTiers );
		break;
	case 3:
		count = ARRAY_LEN( s_flagCaptureAchievementTiers );
		break;
	case 4:
		count = ARRAY_LEN( s_flagAssistAchievementTiers );
		break;
	case 5:
		count = ARRAY_LEN( s_fuelAchievementTiers );
		break;
	default:
		count = 0;
		break;
	}

	if ( count > PLAYERSETTINGS_MAX_ACHIEVEMENT_TIERS ) {
		count = PLAYERSETTINGS_MAX_ACHIEVEMENT_TIERS;
	}
	if ( count < 0 ) {
		count = 0;
	}

	return count;
}

static void PlayerSettings_GetAchievementContentArea( float *left, float *right ) {
	float areaLeft;
	float areaRight;

	areaLeft = PLAYERSETTINGS_PROFILE_FIELD_LEFT + PLAYERSETTINGS_PROFILE_VALUE_OFFSET + PLAYERSETTINGS_ACHIEVEMENT_CONTENT_MARGIN;
	areaRight = PLAYERSETTINGS_PROFILE_ROW_RIGHT - PLAYERSETTINGS_ACHIEVEMENT_CONTENT_MARGIN;
	if ( areaRight <= areaLeft ) {
		areaRight = areaLeft + 1.0f;
	}

	if ( left ) {
		*left = areaLeft;
	}
	if ( right ) {
		*right = areaRight;
	}
}

static void PlayerSettings_ComputeAchievementGridLayout( int tierCount, playersettingsAchievementGridLayout_t *layout ) {
	float areaLeft;
	float areaRight;
	float availableWidth;
	int maxColumns;
	int i;
	int bestColumns;
	float bestGap;
	float bestWidth;

	if ( !layout ) {
		return;
	}

	PlayerSettings_GetAchievementContentArea( &areaLeft, &areaRight );
	availableWidth = areaRight - areaLeft;
	if ( availableWidth <= 0.0f ) {
		availableWidth = PLAYERSETTINGS_ACHIEVEMENT_MEDAL_SIZE;
	}

	if ( tierCount <= 0 ) {
		tierCount = 1;
	}

	maxColumns = tierCount;
	bestColumns = 1;
	bestGap = 0.0f;
	bestWidth = availableWidth;

	for ( i = maxColumns; i >= 1; --i ) {
		float gap;
		float width;

		gap = ( i > 1 ) ? PLAYERSETTINGS_ACHIEVEMENT_COLUMN_GAP : 0.0f;
		width = ( availableWidth - gap * ( i - 1 ) ) / i;
		if ( width >= PLAYERSETTINGS_ACHIEVEMENT_MEDAL_SIZE ) {
			bestColumns = i;
			bestGap = gap;
			bestWidth = width;
			break;
		}
		if ( i == 1 ) {
			bestColumns = 1;
			bestGap = 0.0f;
			bestWidth = width;
		}
	}

	if ( bestWidth < PLAYERSETTINGS_ACHIEVEMENT_MEDAL_SIZE ) {
		bestWidth = PLAYERSETTINGS_ACHIEVEMENT_MEDAL_SIZE;
		if ( bestColumns > 1 ) {
			bestGap = ( availableWidth - bestWidth * bestColumns ) / ( bestColumns - 1 );
			if ( bestGap < 0.0f ) {
				bestGap = 0.0f;
			}
		} else {
			bestGap = 0.0f;
		}
	}

	layout->columns = bestColumns;
	layout->columnGap = bestGap;
	layout->columnWidth = bestWidth;
	layout->rows = ( tierCount + bestColumns - 1 ) / bestColumns;
	if ( layout->rows < 1 ) {
		layout->rows = 1;
	}
}

static float PlayerSettings_GetAchievementRowHeight( int row ) {
	playersettingsAchievementGridLayout_t layout;
	float rowHeight;
	int tierCount;

	if ( row < 0 || row >= PLAYERSETTINGS_ACHIEVEMENT_ROW_COUNT ) {
		return 0.0f;
	}

	tierCount = PlayerSettings_GetAchievementSectionTierCount( row );
	if ( tierCount <= 0 ) {
		return 0.0f;
	}

	PlayerSettings_ComputeAchievementGridLayout( tierCount, &layout );
	rowHeight = PLAYERSETTINGS_ACHIEVEMENT_HEADER_LINE_HEIGHT + PLAYERSETTINGS_ACHIEVEMENT_HEADER_GAP;
	rowHeight += layout.rows * PLAYERSETTINGS_ACHIEVEMENT_MEDAL_SIZE;
	rowHeight += ( layout.rows - 1 ) * PLAYERSETTINGS_ACHIEVEMENT_ENTRY_VERTICAL_GAP;

	return rowHeight;
}

static float PlayerSettings_GetAchievementsScrollRowHeight( void ) {
	float	totalHeight;
	int	countedRows;
	int	i;

	totalHeight = 0.0f;
	countedRows = 0;
	for ( i = 0; i < PLAYERSETTINGS_ACHIEVEMENT_ROW_COUNT; ++i ) {
		float rowHeight;

		rowHeight = PlayerSettings_GetAchievementRowHeight( i );
		if ( rowHeight <= 0.0f ) {
			continue;
		}

		totalHeight += rowHeight;
		++countedRows;
	}

	if ( countedRows <= 0 ) {
		return PLAYERSETTINGS_ACHIEVEMENT_HEADER_LINE_HEIGHT + PLAYERSETTINGS_ACHIEVEMENT_HEADER_GAP + PLAYERSETTINGS_ACHIEVEMENT_MEDAL_SIZE;
	}

	return totalHeight / (float)countedRows;
}

static float PlayerSettings_GetStatsRowSpacing( void ) {
	return PLAYERSETTINGS_STATS_ROW_HEIGHT + PLAYERSETTINGS_STATS_ROW_GAP;
}

static float PlayerSettings_GetStatsContentHeight( void ) {
	if ( STATS_ROW_COUNT <= 0 ) {
		return 0.0f;
	}

	return PLAYERSETTINGS_STATS_ROW_HEIGHT * STATS_ROW_COUNT
			+ PLAYERSETTINGS_STATS_ROW_GAP * ( STATS_ROW_COUNT - 1 );
}

static float PlayerSettings_GetAchievementsContentHeight( void ) {
float totalHeight;
int i;

if ( PLAYERSETTINGS_ACHIEVEMENT_ROW_COUNT <= 0 ) {
return 0.0f;
}

totalHeight = PLAYERSETTINGS_ACHIEVEMENT_HEADER_BLOCK_HEIGHT;
for ( i = 0; i < PLAYERSETTINGS_ACHIEVEMENT_ROW_COUNT; ++i ) {
totalHeight += PlayerSettings_GetAchievementRowHeight( i );
}
if ( PLAYERSETTINGS_ACHIEVEMENT_ROW_COUNT > 1 ) {
totalHeight += PLAYERSETTINGS_ACHIEVEMENT_ROW_GAP * ( PLAYERSETTINGS_ACHIEVEMENT_ROW_COUNT - 1 );
}

return totalHeight;
}

static float PlayerSettings_GetScrollMaxOffset( float contentHeight ) {
	float contentTop;
	float viewportBottom;
	float maxOffset;

	contentTop = PlayerSettings_GetScrollContentTop();
	viewportBottom = PlayerSettings_GetScrollViewportBottom( contentHeight );
	maxOffset = contentTop + contentHeight - viewportBottom;
	if ( maxOffset < 0.0f ) {
		maxOffset = 0.0f;
	}

	return maxOffset;
}

static float PlayerSettings_ClampScrollOffset( float offset, float contentHeight ) {
	float maxOffset;

	if ( offset < 0.0f ) {
		offset = 0.0f;
	}

	maxOffset = PlayerSettings_GetScrollMaxOffset( contentHeight );
	if ( offset > maxOffset ) {
		offset = maxOffset;
	}

	return offset;
}

static void PlayerSettings_SynchronizeScrollState( playersettingsScrollState_t *state, float contentHeight ) {
	if ( !state ) {
		return;
	}

	state->targetOffset = PlayerSettings_ClampScrollOffset( state->targetOffset, contentHeight );
	state->offset = PlayerSettings_ClampScrollOffset( state->targetOffset, contentHeight );
}

static qboolean PlayerSettings_AdjustScrollOffset( playersettingsScrollState_t *state, float contentHeight, float delta ) {
	float newTarget;

	if ( !state ) {
		return qfalse;
	}

	newTarget = PlayerSettings_ClampScrollOffset( state->targetOffset + delta, contentHeight );
	if ( newTarget == state->targetOffset ) {
		return qfalse;
	}

	state->targetOffset = newTarget;
	state->offset = newTarget;

	return qtrue;
}

static float PlayerSettings_GetScrollPageStep( float contentHeight, float rowHeight, float rowGap ) {
	float viewportHeight;
	float rowSpacing;
	float step;

	viewportHeight = PlayerSettings_GetScrollViewportHeight( contentHeight );
	rowSpacing = rowHeight + rowGap;
	if ( rowSpacing <= 0.0f ) {
		return viewportHeight;
	}

	step = viewportHeight - rowSpacing;
	if ( step < rowSpacing ) {
		step = rowSpacing;
	}

	return step;
}

static void PlayerSettings_GetVisibleRowRange( int rowCount, float rowHeight, float rowGap, float scrollOffset, float contentHeight, float leadingOffset, int *firstRow, int *lastRow ) {
        float viewportTop;
        float viewportBottom;
        float contentTop;
        float localTop;
        float localBottom;
        float spacing;
        int start;
        int end;
        int i;

        if ( firstRow ) {
                *firstRow = 0;
        }
        if ( lastRow ) {
                *lastRow = -1;
        }
        if ( rowCount <= 0 ) {
                return;
        }

        PlayerSettings_GetScrollViewportBounds( contentHeight, &viewportTop, &viewportBottom );
        contentTop = PlayerSettings_GetScrollContentTop();
        spacing = rowHeight + rowGap;
        if ( spacing <= 0.0f ) {
                if ( firstRow ) {
                        *firstRow = 0;
                }
                if ( lastRow ) {
                        *lastRow = rowCount - 1;
                }
                return;
        }

        localTop = scrollOffset + ( viewportTop - contentTop );
        localBottom = scrollOffset + ( viewportBottom - contentTop );

        start = rowCount;
        for ( i = 0; i < rowCount; ++i ) {
                float rowTop;
                float rowBottom;

                rowTop = leadingOffset + i * spacing;
                rowBottom = rowTop + rowHeight;
                if ( rowBottom > localTop ) {
                        start = i;
                        break;
                }
        }
        if ( start >= rowCount ) {
                return;
        }

        end = start;
        for ( i = start; i < rowCount; ++i ) {
                float rowTop;

                rowTop = leadingOffset + i * spacing;
                if ( rowTop >= localBottom ) {
                        break;
                }
                end = i;
        }

        if ( firstRow ) {
                *firstRow = start;
        }
        if ( lastRow ) {
                *lastRow = end;
        }
}

static void PlayerSettings_GetScrollBarGeometry( float contentHeight, int *x, int *y, int *height ) {
	float viewportTop;
	float viewportBottom;
	int barX;
	int barY;
	int barHeight;

	PlayerSettings_GetScrollViewportBounds( contentHeight, &viewportTop, &viewportBottom );
	barX = PLAYERSETTINGS_PROFILE_PANEL_LEFT + PLAYERSETTINGS_PROFILE_PANEL_WIDTH - PLAYERSETTINGS_PROFILE_PANEL_INNER_MARGIN - SB_WIDTH;
	barY = (int)viewportTop;
	barHeight = (int)( viewportBottom - viewportTop );
	if ( barHeight < SB_WIDTH * 2 ) {
		barHeight = SB_WIDTH * 2;
	}

	if ( x ) {
		*x = barX;
	}
	if ( y ) {
		*y = barY;
	}
	if ( height ) {
		*height = barHeight;
	}
}

static void PlayerSettings_DrawScrollBar( playersettingsScrollState_t *state, float contentHeight ) {
	int x;
	int y;
	int h;
	float viewportHeight;
	float maxOffset;
	int thumbSize;
	int thumbTrack;
	int thumbPos;

	if ( !state ) {
		return;
	}

	PlayerSettings_SynchronizeScrollState( state, contentHeight );
	maxOffset = PlayerSettings_GetScrollMaxOffset( contentHeight );
	if ( maxOffset <= 0.0f ) {
		return;
	}

	PlayerSettings_GetScrollBarGeometry( contentHeight, &x, &y, &h );

	UI_FillRect( x, y + SB_WIDTH / 2, SB_WIDTH, h - SB_WIDTH, menu_back_color );

	UI_DrawHandlePic( x, y, SB_WIDTH, SB_WIDTH, uis.sb_u0 );
	if ( UI_CursorInRect( x, y, SB_WIDTH, SB_WIDTH ) && state->offset > 0.0f ) {
		UI_DrawHandlePic( x, y, SB_WIDTH, SB_WIDTH, uis.sb_u1 );
	}

	UI_DrawHandlePic( x, y + h - SB_WIDTH, SB_WIDTH, SB_WIDTH, uis.sb_d0 );
	if ( UI_CursorInRect( x, y + h - SB_WIDTH, SB_WIDTH, SB_WIDTH ) && state->offset < maxOffset ) {
		UI_DrawHandlePic( x, y + h - SB_WIDTH, SB_WIDTH, SB_WIDTH, uis.sb_d1 );
	}

	viewportHeight = PlayerSettings_GetScrollViewportHeight( contentHeight );	
	if ( viewportHeight < 1.0f ) {
		viewportHeight = 1.0f;
	}

	thumbTrack = h - 2 * SB_WIDTH;
	if ( thumbTrack < 1 ) {
		thumbTrack = 1;
	}
	thumbSize = (int)( thumbTrack * ( viewportHeight / ( viewportHeight + maxOffset ) ) );
	if ( thumbSize < SB_WIDTH ) {
		thumbSize = SB_WIDTH;
	}
	if ( thumbSize > thumbTrack ) {
		thumbSize = thumbTrack;
	}
	thumbTrack -= thumbSize;
	if ( thumbTrack < 0 ) {
		thumbTrack = 0;
	}

	thumbPos = 0;
	if ( maxOffset > 0.0f && thumbTrack > 0 ) {
		thumbPos = (int)( thumbTrack * ( state->offset / maxOffset ) );
	}

	UI_DrawHandlePic( x, y + SB_WIDTH + thumbPos, SB_WIDTH, SB_WIDTH / 2, uis.sbtop );
	if ( thumbSize > SB_WIDTH ) {
		UI_DrawHandlePic( x, y + SB_WIDTH + thumbPos + 8, SB_WIDTH, thumbSize - SB_WIDTH, uis.sbmid );
	}
	UI_DrawHandlePic( x, y + SB_WIDTH + thumbPos + ( thumbSize - 8 ), SB_WIDTH, SB_WIDTH / 2, uis.sbbot );
}

static qboolean PlayerSettings_HandleScrollKey( playersettingsScrollState_t *state, float contentHeight, float rowHeight, float rowGap, int key ) {
	float step;

	switch ( key ) {
	case K_MWHEELUP:
		return PlayerSettings_AdjustScrollOffset( state, contentHeight, -( rowHeight + rowGap ) );
	case K_MWHEELDOWN:
		return PlayerSettings_AdjustScrollOffset( state, contentHeight, rowHeight + rowGap );
	case K_PGUP:
		step = PlayerSettings_GetScrollPageStep( contentHeight, rowHeight, rowGap );
		return PlayerSettings_AdjustScrollOffset( state, contentHeight, -step );
	case K_PGDN:
		step = PlayerSettings_GetScrollPageStep( contentHeight, rowHeight, rowGap );
		return PlayerSettings_AdjustScrollOffset( state, contentHeight, step );
	default:
		break;
	}

	return qfalse;
}

static qboolean PlayerSettings_HandleScrollBarClick( playersettingsScrollState_t *state, float contentHeight, float rowHeight, float rowGap ) {
	int x;
	int y;
	int h;
	int mx;
	int my;
	float maxOffset;
	float viewportHeight;
	int thumbSize;
	int thumbTrack;
	int thumbPos;
	int thumbTop;
	int thumbBottom;
	float step;

	if ( PlayerSettings_GetScrollMaxOffset( contentHeight ) <= 0.0f ) {
		return qfalse;
	}

	PlayerSettings_GetScrollBarGeometry( contentHeight, &x, &y, &h );
	mx = uis.cursorx;
	my = uis.cursory;

	if ( mx < x || mx > x + SB_WIDTH || my < y || my > y + h ) {
		return qfalse;
	}

	if ( my < y + SB_WIDTH ) {
		state->dragging = qfalse;
		return PlayerSettings_AdjustScrollOffset( state, contentHeight, -( rowHeight + rowGap ) );
	}
	if ( my > y + h - SB_WIDTH ) {
		state->dragging = qfalse;
		return PlayerSettings_AdjustScrollOffset( state, contentHeight, rowHeight + rowGap );
	}

	maxOffset = PlayerSettings_GetScrollMaxOffset( contentHeight );
	viewportHeight = PlayerSettings_GetScrollViewportHeight( contentHeight );
	if ( viewportHeight < 1.0f ) {
		viewportHeight = 1.0f;
	}

	thumbTrack = h - 2 * SB_WIDTH;
	if ( thumbTrack < 1 ) {
		thumbTrack = 1;
	}
	thumbSize = (int)( thumbTrack * ( viewportHeight / ( viewportHeight + maxOffset ) ) );
	if ( thumbSize < SB_WIDTH ) {
		thumbSize = SB_WIDTH;
	}
	if ( thumbSize > thumbTrack ) {
		thumbSize = thumbTrack;
	}
	thumbTrack -= thumbSize;
	if ( thumbTrack < 0 ) {
		thumbTrack = 0;
	}

	thumbPos = 0;
	if ( maxOffset > 0.0f && thumbTrack > 0 ) {
		thumbPos = (int)( thumbTrack * ( state->offset / maxOffset ) );
	}

	thumbTop = y + SB_WIDTH + thumbPos;
	thumbBottom = thumbTop + thumbSize;

	if ( my >= thumbTop && my <= thumbBottom ) {
		state->dragging = qtrue;
		state->dragStartOffset = my - thumbTop;
		return qtrue;
	}

	state->dragging = qfalse;
	step = PlayerSettings_GetScrollPageStep( contentHeight, rowHeight, rowGap );
	if ( my < thumbTop ) {
		return PlayerSettings_AdjustScrollOffset( state, contentHeight, -step );
	}
	if ( my > thumbBottom ) {
		return PlayerSettings_AdjustScrollOffset( state, contentHeight, step );
	}

	return qfalse;
}

static void PlayerSettings_UpdateScrollDrag( playersettingsScrollState_t *state, float contentHeight ) {
	int x;
	int y;
	int h;
	float maxOffset;
	float viewportHeight;
	int thumbSize;
	int thumbTrack;
	float thumbPos;
	float thumbOffset;

	if ( !state || !state->dragging ) {
		return;
	}

	if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
		state->dragging = qfalse;
		return;
	}

	maxOffset = PlayerSettings_GetScrollMaxOffset( contentHeight );
	if ( maxOffset <= 0.0f ) {
		state->dragging = qfalse;
		PlayerSettings_SynchronizeScrollState( state, contentHeight );
		return;
	}

	PlayerSettings_GetScrollBarGeometry( contentHeight, &x, &y, &h );

	viewportHeight = PlayerSettings_GetScrollViewportHeight( contentHeight );
	if ( viewportHeight < 1.0f ) {
		viewportHeight = 1.0f;
	}

	thumbTrack = h - 2 * SB_WIDTH;
	if ( thumbTrack < 1 ) {
		thumbTrack = 1;
	}
	thumbSize = (int)( thumbTrack * ( viewportHeight / ( viewportHeight + maxOffset ) ) );
	if ( thumbSize < SB_WIDTH ) {
		thumbSize = SB_WIDTH;
	}
	if ( thumbSize > thumbTrack ) {
		thumbSize = thumbTrack;
	}
	thumbTrack -= thumbSize;
	if ( thumbTrack <= 0 ) {
		state->dragging = qfalse;
		PlayerSettings_SynchronizeScrollState( state, contentHeight );
		return;
	}

	thumbOffset = uis.cursory - ( y + SB_WIDTH ) - state->dragStartOffset;
	if ( thumbOffset < 0.0f ) {
		thumbOffset = 0.0f;
	}
	if ( thumbOffset > thumbTrack ) {
		thumbOffset = (float)thumbTrack;
	}

	thumbPos = thumbOffset / thumbTrack;
	state->targetOffset = PlayerSettings_ClampScrollOffset( thumbPos * maxOffset, contentHeight );
	state->offset = state->targetOffset;
}

static void PlayerSettings_GetStatsRowBounds( int row, int *top, int *bottom ) {
	float rowTop;
	float rowBottom;
	float spacing;
	float contentTop;

	if ( row < 0 ) {
		row = 0;
	}
	if ( row >= STATS_ROW_COUNT ) {
		row = STATS_ROW_COUNT - 1;
	}

	spacing = PlayerSettings_GetStatsRowSpacing();
	contentTop = PlayerSettings_GetScrollContentTop();
	rowTop = contentTop + row * spacing - s_playersettings.statsScroll.offset;
	rowBottom = rowTop + PLAYERSETTINGS_STATS_ROW_HEIGHT;

	if ( top ) {
		*top = (int)rowTop;
	}
	if ( bottom ) {
		*bottom = (int)rowBottom;
	}
}


static void PlayerSettings_DrawStatsPanelBackground( void ) {
	vec4_t panelColor;
	vec4_t rowColor;
	vec4_t borderColor;
	float contentHeight;
	float viewportTop;
	float viewportBottom;
	int panelTop;
	int panelBottom;
	int firstRow;
	int lastRow;
	int i;

	contentHeight = PlayerSettings_GetStatsContentHeight();
	PlayerSettings_UpdateScrollDrag( &s_playersettings.statsScroll, contentHeight );
	PlayerSettings_SynchronizeScrollState( &s_playersettings.statsScroll, contentHeight );

	panelTop = PLAYERSETTINGS_PROFILE_PANEL_TOP;
	panelBottom = (int)PlayerSettings_GetPanelBottomForContent( contentHeight );

	Vector4Copy( profilePanelFillColor, panelColor );
	panelColor[3] *= uis.tFrac;
	UI_FillRect( PLAYERSETTINGS_PROFILE_PANEL_LEFT, panelTop, PLAYERSETTINGS_PROFILE_PANEL_WIDTH, panelBottom - panelTop, panelColor );

	Vector4Copy( profileRowBorderColor, borderColor );
	borderColor[3] *= uis.tFrac;

	PlayerSettings_GetScrollViewportBounds( contentHeight, &viewportTop, &viewportBottom );
	PlayerSettings_GetVisibleRowRange( STATS_ROW_COUNT, PLAYERSETTINGS_STATS_ROW_HEIGHT, PLAYERSETTINGS_STATS_ROW_GAP, s_playersettings.statsScroll.offset, contentHeight, 0.0f, &firstRow, &lastRow );

	if ( lastRow < firstRow ) {
		return;
	}

	for ( i = firstRow; i <= lastRow; ++i ) {
		int rowTop;
		int rowBottom;

		PlayerSettings_GetStatsRowBounds( i, &rowTop, &rowBottom );

		if ( rowTop < (int)viewportTop ) {
			rowTop = (int)viewportTop;
		}
		if ( rowBottom > (int)viewportBottom ) {
			rowBottom = (int)viewportBottom;
		}
		if ( rowBottom <= rowTop ) {
			continue;
		}

		Vector4Copy( ( i & 1 ) ? profileRowOddFillColor : profileRowEvenFillColor, rowColor );
		rowColor[3] *= uis.tFrac;

		UI_FillRect(
			PLAYERSETTINGS_PROFILE_FIELD_LEFT,
			rowTop,
			PLAYERSETTINGS_PROFILE_PANEL_WIDTH - PLAYERSETTINGS_PROFILE_PANEL_INNER_MARGIN * 2,
			rowBottom - rowTop,
			rowColor );

		UI_DrawRect(
			PLAYERSETTINGS_PROFILE_FIELD_LEFT,
			rowTop,
			PLAYERSETTINGS_PROFILE_PANEL_WIDTH - PLAYERSETTINGS_PROFILE_PANEL_INNER_MARGIN * 2,
			rowBottom - rowTop,
			borderColor );
	}
}
static void PlayerSettings_DrawStatsTab( void ) {
	const profile_stats_t *stats;
	char buffer[64];

	PlayerSettings_SynchronizeScrollState( &s_playersettings.statsScroll, PlayerSettings_GetStatsContentHeight() );

	if ( !UI_Profile_HasActiveProfile() ) {
		PlayerSettings_DrawStatsMessage( STATS_ROW_DISTANCE, "No active profile selected." );
		PlayerSettings_DrawStatsMessage( STATS_ROW_FUEL, "Create or select a profile from the main menu." );
		return;
	}

	stats = UI_Profile_GetActiveStats();
	if ( !stats ) {
		PlayerSettings_DrawStatsMessage( STATS_ROW_DISTANCE, "Unable to read profile statistics." );
		return;
	}

	Com_sprintf( buffer, sizeof( buffer ), "%.2f km", stats->distanceKm );
	PlayerSettings_DrawStatsLabelValue( STATS_ROW_DISTANCE, "Distance Driven", buffer );

	Com_sprintf( buffer, sizeof( buffer ), "%.1f L", stats->fuelUsed );
	PlayerSettings_DrawStatsLabelValue( STATS_ROW_FUEL, "Fuel Used", buffer );

	if ( stats->bestLapMs > 0 ) {
		int minutes = stats->bestLapMs / 60000;
		int seconds = ( stats->bestLapMs % 60000 ) / 1000;
		int millis = stats->bestLapMs % 1000;
		Com_sprintf( buffer, sizeof( buffer ), "%02d:%02d.%03d", minutes, seconds, millis );
	} else {
		Q_strncpyz( buffer, "--", sizeof( buffer ) );
	}
	PlayerSettings_DrawStatsLabelValue( STATS_ROW_BEST_LAP, "Best Lap", buffer );

	Com_sprintf( buffer, sizeof( buffer ), "%d / %d", stats->kills, stats->deaths );
	PlayerSettings_DrawStatsLabelValue( STATS_ROW_KILLS, "Kills / Deaths", buffer );

	Com_sprintf( buffer, sizeof( buffer ), "%d / %d", stats->wins, stats->losses );
	PlayerSettings_DrawStatsLabelValue( STATS_ROW_WINS, "Wins / Losses", buffer );

	Com_sprintf( buffer, sizeof( buffer ), "%d", stats->flagCaptures );
	PlayerSettings_DrawStatsLabelValue( STATS_ROW_FLAGS_CAPTURED, "Flags Captured", buffer );

	Com_sprintf( buffer, sizeof( buffer ), "%d", stats->flagAssists );
	PlayerSettings_DrawStatsLabelValue( STATS_ROW_FLAG_ASSISTS, "Flag Assists", buffer );
}



static void PlayerSettings_DrawStatsLabelValueWithColors( int row, const char *label, const vec4_t labelColor, const char *value, const vec4_t valueColor ) {
	int rowTop;
	int rowBottom;
	int y;
	int labelX;
	int valueX;
	vec4_t mutableLabelColor;
	vec4_t mutableValueColor;
	float viewportTop;
	float viewportBottom;

        labelX = PLAYERSETTINGS_PROFILE_FIELD_LEFT + PLAYERSETTINGS_PROFILE_LABEL_OFFSET;
        valueX = PLAYERSETTINGS_PROFILE_FIELD_LEFT + PLAYERSETTINGS_STATS_VALUE_OFFSET;

	PlayerSettings_GetStatsRowBounds( row, &rowTop, &rowBottom );
	PlayerSettings_GetScrollViewportBounds( PlayerSettings_GetStatsContentHeight(), &viewportTop, &viewportBottom );
	if ( rowBottom <= (int)viewportTop || rowTop >= (int)viewportBottom ) {
		return;
	}

	y = rowTop + PLAYERSETTINGS_STATS_VALUE_BASELINE;

	Vector4Copy( labelColor, mutableLabelColor );
	Vector4Copy( valueColor, mutableValueColor );

	if ( label && label[0] ) {
		UI_DrawProportionalString( labelX, y, label, UI_LEFT | UI_SMALLFONT, mutableLabelColor );
	}

	if ( value && value[0] ) {
		UI_DrawProportionalString( valueX, y, value, UI_LEFT | UI_SMALLFONT, mutableValueColor );
	}
}


static void PlayerSettings_DrawStatsLabelValue( int row, const char *label, const char *value ) {
	PlayerSettings_DrawStatsLabelValueWithColors( row, label, text_color_highlight, value, text_color_normal );
}


static void PlayerSettings_DrawStatsMessage( int row, const char *message ) {
	int rowTop;
	int rowBottom;
	int y;
	int x;
	float viewportTop;
	float viewportBottom;

	x = PLAYERSETTINGS_PROFILE_FIELD_LEFT + PLAYERSETTINGS_PROFILE_LABEL_OFFSET;

	PlayerSettings_GetStatsRowBounds( row, &rowTop, &rowBottom );
	PlayerSettings_GetScrollViewportBounds( PlayerSettings_GetStatsContentHeight(), &viewportTop, &viewportBottom );
	if ( rowBottom <= (int)viewportTop || rowTop >= (int)viewportBottom ) {
		return;
	}

	y = rowTop + PLAYERSETTINGS_STATS_VALUE_BASELINE;

	UI_DrawProportionalString( x, y, message, UI_LEFT | UI_SMALLFONT, text_color_normal );
}



/*
=================
PlayerSettings_DrawBackShaders
=================
*/
static void PlayerSettings_DrawBackShaders( void ) {
        vec4_t color;
        vec4_t panelColor;

        Vector4Copy( menu_back_color, color );
        color[3] *= uis.tFrac;

        UI_FillRect( 24, PLAYERSETTINGS_TAB_TOP - 12, 592, PLAYERSETTINGS_TAB_HEIGHT + 24, color );

        Vector4Copy( menu_back_color, panelColor );
        panelColor[3] *= uis.tFrac;

        if ( s_playersettings.currentTab == TAB_PROFILE ) {
                PlayerSettings_DrawProfilePanelBackground();
        } else if ( s_playersettings.currentTab == TAB_VEHICLE ) {
                UI_FillRect( 124, 138, 392, 32, panelColor );
        } else if ( s_playersettings.currentTab == TAB_STATS ) {
                PlayerSettings_DrawStatsPanelBackground();
        } else if ( s_playersettings.currentTab == TAB_ACHIEVEMENTS ) {
                PlayerSettings_DrawAchievementsPanelBackground();
        }

        Menu_Draw( &s_playersettings.menu );

        if ( s_playersettings.currentTab == TAB_STATS ) {
                PlayerSettings_DrawStatsTab();
                PlayerSettings_DrawScrollBar( &s_playersettings.statsScroll, PlayerSettings_GetStatsContentHeight() );
        } else if ( s_playersettings.currentTab == TAB_ACHIEVEMENTS ) {
                PlayerSettings_DrawAchievementsTab();
                PlayerSettings_DrawScrollBar( &s_playersettings.achievementsScroll, PlayerSettings_GetAchievementsContentHeight() );
        }
}

static void PlayerSettings_GetAchievementRowBounds( int row, int *top, int *bottom ) {
	float	rowTop;
	float	rowBottom;
	float	rowHeight;
	float	contentTop;
	int		i;

	if ( PLAYERSETTINGS_ACHIEVEMENT_ROW_COUNT <= 0 ) {
		if ( top ) {
			*top = 0;
		}
		if ( bottom ) {
			*bottom = 0;
		}
		return;
	}

	if ( row < 0 ) {
		row = 0;
	}
	if ( row >= PLAYERSETTINGS_ACHIEVEMENT_ROW_COUNT ) {
		row = PLAYERSETTINGS_ACHIEVEMENT_ROW_COUNT - 1;
	}

	contentTop = PlayerSettings_GetScrollContentTop();
	rowTop = contentTop + PLAYERSETTINGS_ACHIEVEMENT_HEADER_BLOCK_HEIGHT - s_playersettings.achievementsScroll.offset;
	for ( i = 0; i < row; ++i ) {
		rowTop += PlayerSettings_GetAchievementRowHeight( i );
		rowTop += PLAYERSETTINGS_ACHIEVEMENT_ROW_GAP;
	}
	rowHeight = PlayerSettings_GetAchievementRowHeight( row );
	rowBottom = rowTop + rowHeight;

	if ( top ) {
		*top = (int)rowTop;
	}
	if ( bottom ) {
		*bottom = (int)rowBottom;
	}
}

static void PlayerSettings_GetAchievementsHeaderBounds( int *top, int *bottom ) {
        float headerTop;
        float headerBottom;

        headerTop = PlayerSettings_GetScrollContentTop() - s_playersettings.achievementsScroll.offset;
        headerBottom = headerTop + PLAYERSETTINGS_ACHIEVEMENT_HEADER_BLOCK_HEIGHT;

        if ( top ) {
                *top = (int)headerTop;
        }
        if ( bottom ) {
                *bottom = (int)headerBottom;
        }
}

static void PlayerSettings_DrawAchievementsPanelBackground( void ) {
	vec4_t panelColor;
	vec4_t rowColor;
	vec4_t borderColor;
	float contentHeight;
	float viewportTop;
	float viewportBottom;
	int panelTop;
	int panelBottom;
	int firstRow;
	int lastRow;
	int i;

	contentHeight = PlayerSettings_GetAchievementsContentHeight();
	PlayerSettings_UpdateScrollDrag( &s_playersettings.achievementsScroll, contentHeight );
	PlayerSettings_SynchronizeScrollState( &s_playersettings.achievementsScroll, contentHeight );

	panelTop = PLAYERSETTINGS_PROFILE_PANEL_TOP;
	panelBottom = (int)PlayerSettings_GetPanelBottomForContent( contentHeight );

	Vector4Copy( profilePanelFillColor, panelColor );
	panelColor[3] *= uis.tFrac;
	UI_FillRect( PLAYERSETTINGS_PROFILE_PANEL_LEFT, panelTop, PLAYERSETTINGS_PROFILE_PANEL_WIDTH, panelBottom - panelTop, panelColor );

	Vector4Copy( profileRowBorderColor, borderColor );
	borderColor[3] *= uis.tFrac;

	PlayerSettings_GetScrollViewportBounds( contentHeight, &viewportTop, &viewportBottom );
	firstRow = -1;
	lastRow = -1;
	for ( i = 0; i < PLAYERSETTINGS_ACHIEVEMENT_ROW_COUNT; ++i ) {
		int tempTop;
		int tempBottom;

		PlayerSettings_GetAchievementRowBounds( i, &tempTop, &tempBottom );
		if ( tempBottom <= (int)viewportTop || tempTop >= (int)viewportBottom ) {
			continue;
		}

		if ( firstRow < 0 ) {
			firstRow = i;
		}
		lastRow = i;
	}

	if ( firstRow < 0 ) {
		return;
	}

	for ( i = firstRow; i <= lastRow; ++i ) {
		int rowTop;
		int rowBottom;

		PlayerSettings_GetAchievementRowBounds( i, &rowTop, &rowBottom );

		if ( rowTop < (int)viewportTop ) {
			rowTop = (int)viewportTop;
		}
		if ( rowBottom > (int)viewportBottom ) {
			rowBottom = (int)viewportBottom;
		}
		if ( rowBottom <= rowTop ) {
			continue;
		}

		Vector4Copy( ( i & 1 ) ? profileRowOddFillColor : profileRowEvenFillColor, rowColor );
		rowColor[3] *= uis.tFrac;

		UI_FillRect(
			PLAYERSETTINGS_PROFILE_FIELD_LEFT,
			rowTop,
			PLAYERSETTINGS_PROFILE_PANEL_WIDTH - PLAYERSETTINGS_PROFILE_PANEL_INNER_MARGIN * 2,
			rowBottom - rowTop,
			rowColor );

		UI_DrawRect(
			PLAYERSETTINGS_PROFILE_FIELD_LEFT,
			rowTop,
			PLAYERSETTINGS_PROFILE_PANEL_WIDTH - PLAYERSETTINGS_PROFILE_PANEL_INNER_MARGIN * 2,
			rowBottom - rowTop,
			borderColor );
	}
}

static int PlayerSettings_DrawScaledProportionalString_Wrapped(
        int x,
        int y,
        float maxWidth,
        float lineHeight,
        const char *text,
        int style,
        vec4_t color,
        float textScale,
        int maxLines ) {
        char buffer[1024];
        char *s1;
        char *s2;
        char *s3;
        char c_bcp;
        float width;
        int linesDrawn;

        if ( !text || !text[0] ) {
                return 0;
        }

        if ( maxWidth <= 0.0f ) {
                UI_DrawScaledProportionalString( x, y, text, style, color, textScale );
                return 1;
        }

        Q_strncpyz( buffer, text, sizeof( buffer ) );
        s1 = buffer;
        s2 = buffer;
        s3 = buffer;
        linesDrawn = 0;

        while ( qtrue ) {
                do {
                        s3++;
                } while ( *s3 != ' ' && *s3 != '\0' );

                c_bcp = *s3;
                *s3 = '\0';
                width = UI_ProportionalStringWidth( s1 ) * textScale;
                *s3 = c_bcp;

                if ( width > maxWidth ) {
                        if ( s1 == s2 ) {
                                s2 = s3;
                        }

                        *s2 = '\0';
                        UI_DrawScaledProportionalString( x, y, s1, style, color, textScale );
                        ++linesDrawn;
                        if ( maxLines > 0 && linesDrawn >= maxLines ) {
                                return linesDrawn;
                        }

                        y += lineHeight;

                        if ( c_bcp == '\0' ) {
                                s2++;
                                if ( *s2 != '\0' ) {
                                        UI_DrawScaledProportionalString( x, y, s2, style, color, textScale );
                                        ++linesDrawn;
                                }
                                return linesDrawn;
                        }

                        s2++;
                        s1 = s2;
                        s3 = s2;
                } else {
                        s2 = s3;
                        if ( c_bcp == '\0' ) {
                                UI_DrawScaledProportionalString( x, y, s1, style, color, textScale );
                                ++linesDrawn;
                                return linesDrawn;
                        }
                }
        }

        return linesDrawn;
}

static int PlayerSettings_DrawAchievementSection( int row, const char *title, const playersettingsAchievementTierDef_t *tiers, int count, double progress, playersettingsAchievementIcon_t iconIndex ) {
        playersettingsAchievementGridLayout_t gridLayout;
        int columnCount;
        int i;
        int rowTop;
        int rowBottom;
        int titleX;
        int titleY;
        float        areaLeft;
        float        areaRight;
        float        columnWidth;
        float        columnGap;
        float gridTop;
        int unlockedCount;
        qboolean entryUnlocked[PLAYERSETTINGS_MAX_ACHIEVEMENT_TIERS];
        qhandle_t lockedIconHandle;
        qhandle_t unlockedIconHandle;
        float viewportTop;
        float viewportBottom;
        qboolean visible;

        if ( count > PLAYERSETTINGS_MAX_ACHIEVEMENT_TIERS ) {
                count = PLAYERSETTINGS_MAX_ACHIEVEMENT_TIERS;
        }
        if ( count <= 0 || !tiers ) {
                return 0;
        }

        lockedIconHandle = 0;
        unlockedIconHandle = 0;
        if ( iconIndex >= 0 && iconIndex < PLAYERSETTINGS_ACHIEVEMENT_ICON_COUNT ) {
                lockedIconHandle = s_playersettings.achievementMedalLocked[iconIndex];
                unlockedIconHandle = s_playersettings.achievementMedalUnlocked[iconIndex];
        }

        unlockedCount = 0;
        for ( i = 0; i < count; ++i ) {
                entryUnlocked[i] = ( progress >= tiers[i].threshold );
                if ( entryUnlocked[i] ) {
                        ++unlockedCount;
                }
        }

        PlayerSettings_GetAchievementRowBounds( row, &rowTop, &rowBottom );
        PlayerSettings_GetScrollViewportBounds( PlayerSettings_GetAchievementsContentHeight(), &viewportTop, &viewportBottom );
        visible = ( rowBottom > (int)viewportTop && rowTop < (int)viewportBottom );

        titleX = PLAYERSETTINGS_PROFILE_FIELD_LEFT + PLAYERSETTINGS_PROFILE_LABEL_OFFSET;
        titleY = rowTop + PLAYERSETTINGS_ACHIEVEMENT_TITLE_OFFSET;

        if ( visible ) {
                UI_DrawProportionalString( titleX, titleY, title, UI_LEFT | UI_SMALLFONT, text_color_highlight );
        }

        PlayerSettings_GetAchievementContentArea( &areaLeft, &areaRight );
        PlayerSettings_ComputeAchievementGridLayout( count, &gridLayout );
        columnWidth = gridLayout.columnWidth;
        columnGap = gridLayout.columnGap;
        columnCount = gridLayout.columns;
        if ( columnCount < 1 ) {
                columnCount = 1;
        }

        gridTop = rowTop + PLAYERSETTINGS_ACHIEVEMENT_TITLE_OFFSET + PLAYERSETTINGS_ACHIEVEMENT_HEADER_LINE_HEIGHT + PLAYERSETTINGS_ACHIEVEMENT_HEADER_GAP;

        if ( visible ) {
                const float textScale = UI_ProportionalSizeScale( UI_SMALLFONT ) * PLAYERSETTINGS_ACHIEVEMENT_TEXT_SCALE_MULTIPLIER;
                const float textLineHeight = PLAYERSETTINGS_ACHIEVEMENT_TEXT_LINE_HEIGHT * PLAYERSETTINGS_ACHIEVEMENT_TEXT_SCALE_MULTIPLIER;
                for ( i = 0; i < count; ++i ) {
                        int column;
                        int tierRow;
                        float entryLeft;
                        float entryTop;
                        float textX;
                        float textMaxWidth;
                        float nameY;
                        float descriptionY;
                        qhandle_t iconHandle;
                        float iconWidth;
                        const char *name;
                        const char *description;

                        column = i % columnCount;
                        tierRow = i / columnCount;
                        entryLeft = areaLeft + column * ( columnWidth + columnGap );
                        entryTop = gridTop + tierRow * ( PLAYERSETTINGS_ACHIEVEMENT_MEDAL_SIZE + PLAYERSETTINGS_ACHIEVEMENT_ENTRY_VERTICAL_GAP );

                        iconHandle = entryUnlocked[i] ? unlockedIconHandle : lockedIconHandle;
                        iconWidth = ( iconHandle != 0 ) ? PLAYERSETTINGS_ACHIEVEMENT_MEDAL_SIZE : 0.0f;
                        if ( iconWidth > 0.0f ) {
                                UI_DrawHandlePic( (int)entryLeft, (int)entryTop, PLAYERSETTINGS_ACHIEVEMENT_MEDAL_SIZE, PLAYERSETTINGS_ACHIEVEMENT_MEDAL_SIZE, iconHandle );
                        }

                        textX = entryLeft;
                        if ( iconWidth > 0.0f ) {
                                textX += iconWidth + PLAYERSETTINGS_ACHIEVEMENT_TEXT_GAP;
                        }

                        textMaxWidth = entryLeft + columnWidth - textX;
                        if ( textMaxWidth <= 0.0f ) {
                                textMaxWidth = columnWidth;
                        }

                        name = tiers[i].name;
                        description = tiers[i].description;
                        nameY = entryTop + 12.0f;
                        descriptionY = nameY + textLineHeight;

                        if ( name && name[0] ) {
                                UI_DrawScaledProportionalString(
                                        ( int )textX,
                                        ( int )nameY,
                                        name,
                                        UI_LEFT | UI_SMALLFONT,
                                        entryUnlocked[i] ? text_color_highlight : achievementLockedColor,
                                        textScale );
                        }
                        if ( description && description[0] ) {
                                PlayerSettings_DrawScaledProportionalString_Wrapped(
                                        ( int )textX,
                                        ( int )descriptionY,
                                        textMaxWidth,
                                        textLineHeight,
                                        description,
                                        UI_LEFT | UI_SMALLFONT,
                                        entryUnlocked[i] ? achievementUnlockedColor : achievementLockedColor,
                                        textScale,
                                        2 );
                        }
                }
        }

        return unlockedCount;
}

static void PlayerSettings_DrawAchievementsTab( void ) {
        const profile_stats_t *stats;
        int unlockedAchievements;
        int displayTotalAchievements;
        char progressBuffer[32];
        char headerBuffer[64];
        int headerTop;
        int headerBottom;
        int headerY;
        int row;
        float viewportTop;
        float viewportBottom;

        PlayerSettings_SynchronizeScrollState( &s_playersettings.achievementsScroll, PlayerSettings_GetAchievementsContentHeight() );

        if ( !UI_Profile_HasActiveProfile() ) {
                int messageX;

                messageX = PLAYERSETTINGS_PROFILE_FIELD_LEFT + PLAYERSETTINGS_PROFILE_LABEL_OFFSET;
                UI_DrawProportionalString( messageX, 208, "No active profile selected.", UI_LEFT | UI_SMALLFONT, text_color_normal );
                UI_DrawProportionalString( messageX, 236, "Create or select a profile from the main menu.", UI_LEFT | UI_SMALLFONT, text_color_normal );
                return;
        }

        stats = UI_Profile_GetActiveStats();
        if ( !stats ) {
                int messageX;

                messageX = PLAYERSETTINGS_PROFILE_FIELD_LEFT + PLAYERSETTINGS_PROFILE_LABEL_OFFSET;
                UI_DrawProportionalString( messageX, 220, "Unable to read profile statistics.", UI_LEFT | UI_SMALLFONT, text_color_normal );
                return;
        }

        unlockedAchievements = 0;
        displayTotalAchievements = PLAYERSETTINGS_DISPLAY_ACHIEVEMENT_TOTAL;

	row = 0;
        unlockedAchievements += PlayerSettings_DrawAchievementSection( row++, "Distance Driven", s_distanceAchievementTiers, ARRAY_LEN( s_distanceAchievementTiers ), stats->distanceKm, PLAYERSETTINGS_ACHIEVEMENT_ICON_DRIVEN );
        unlockedAchievements += PlayerSettings_DrawAchievementSection( row++, "Kills", s_killAchievementTiers, ARRAY_LEN( s_killAchievementTiers ), (double)stats->kills, PLAYERSETTINGS_ACHIEVEMENT_ICON_KILLS );
        unlockedAchievements += PlayerSettings_DrawAchievementSection( row++, "Races Won", s_winAchievementTiers, ARRAY_LEN( s_winAchievementTiers ), (double)stats->wins, PLAYERSETTINGS_ACHIEVEMENT_ICON_WINS );
        unlockedAchievements += PlayerSettings_DrawAchievementSection( row++, "Flags Captured", s_flagCaptureAchievementTiers, ARRAY_LEN( s_flagCaptureAchievementTiers ), (double)stats->flagCaptures, PLAYERSETTINGS_ACHIEVEMENT_ICON_FLAGS );
        unlockedAchievements += PlayerSettings_DrawAchievementSection( row++, "Flag Assists", s_flagAssistAchievementTiers, ARRAY_LEN( s_flagAssistAchievementTiers ), (double)stats->flagAssists, PLAYERSETTINGS_ACHIEVEMENT_ICON_FLAG_ASSISTS );
        unlockedAchievements += PlayerSettings_DrawAchievementSection( row++, "Fuel Consumed", s_fuelAchievementTiers, ARRAY_LEN( s_fuelAchievementTiers ), stats->fuelUsed, PLAYERSETTINGS_ACHIEVEMENT_ICON_FUEL );

        if ( unlockedAchievements > displayTotalAchievements ) {
                unlockedAchievements = displayTotalAchievements;
        }

        Com_sprintf( progressBuffer, sizeof( progressBuffer ), "%d/%d", unlockedAchievements, displayTotalAchievements );
        Com_sprintf( headerBuffer, sizeof( headerBuffer ), "Achievements %s", progressBuffer );

	PlayerSettings_GetAchievementsHeaderBounds( &headerTop, &headerBottom );
        PlayerSettings_GetScrollViewportBounds( PlayerSettings_GetAchievementsContentHeight(), &viewportTop, &viewportBottom );
        if ( headerBottom > (int)viewportTop && headerTop < (int)viewportBottom ) {
                headerY = headerTop + PLAYERSETTINGS_ACHIEVEMENT_VALUE_BASELINE;
                UI_DrawProportionalString(
                        PLAYERSETTINGS_PROFILE_FIELD_LEFT + PLAYERSETTINGS_PROFILE_LABEL_OFFSET,
                        headerY,
                        headerBuffer,
                        UI_LEFT | UI_SMALLFONT,
                        text_color_highlight );
        }
}

static void PlayerSettings_SetTab( int tab ) {
	int i;
	qboolean showProfile;
	qboolean showVehicle;

	if ( tab < TAB_PROFILE || tab > TAB_ACHIEVEMENTS ) {
		tab = TAB_PROFILE;
	}

	s_playersettings.currentTab = tab;

	s_playersettings.statsScroll.dragging = qfalse;
	s_playersettings.achievementsScroll.dragging = qfalse;

	PlayerSettings_SynchronizeScrollState( &s_playersettings.statsScroll, PlayerSettings_GetStatsContentHeight() );
	PlayerSettings_SynchronizeScrollState( &s_playersettings.achievementsScroll, PlayerSettings_GetAchievementsContentHeight() );

	showProfile = ( tab == TAB_PROFILE );
	showVehicle = ( tab == TAB_VEHICLE );

	PlayerSettings_SetWidgetVisible( &s_playersettings.name.generic, showProfile );
	PlayerSettings_SetWidgetVisible( &s_playersettings.gender.generic, showProfile );
	PlayerSettings_SetWidgetVisible( &s_playersettings.birthDateLabel.generic, showProfile );
	PlayerSettings_SetWidgetVisible( &s_playersettings.birthDay.generic, showProfile );
	PlayerSettings_SetWidgetVisible( &s_playersettings.birthMonth.generic, showProfile );
	PlayerSettings_SetWidgetVisible( &s_playersettings.birthYear.generic, showProfile );
	PlayerSettings_SetWidgetVisible( &s_playersettings.avatar.generic, showProfile );
	PlayerSettings_SetWidgetVisible( &s_playersettings.country.generic, showProfile );
	PlayerSettings_SetWidgetVisible( &s_playersettings.handicap.generic, showProfile );
	PlayerSettings_SetWidgetVisible( &s_playersettings.effects.generic, showProfile );
	PlayerSettings_SetWidgetVisible( &s_playersettings.favorites.generic, showVehicle );

	for ( i = 0; i < NUM_FAVORITES; ++i ) {
		PlayerSettings_SetWidgetVisible( &s_playersettings.ports[i].generic, showVehicle );
		PlayerSettings_SetWidgetVisible( &s_playersettings.favpicbuttons[i].generic, showVehicle );
		PlayerSettings_SetWidgetVisible( &s_playersettings.favpics[i].generic, showVehicle );
	}

	PlayerSettings_SetWidgetVisible( &s_playersettings.player.generic, showVehicle );
	PlayerSettings_SetWidgetVisible( &s_playersettings.left.generic, showVehicle );
	PlayerSettings_SetWidgetVisible( &s_playersettings.right.generic, showVehicle );
	PlayerSettings_SetWidgetVisible( &s_playersettings.modelname.generic, showVehicle );
	PlayerSettings_SetWidgetVisible( &s_playersettings.customize.generic, showVehicle );
	PlayerSettings_SetWidgetVisible( &s_playersettings.plate.generic, showVehicle );

	s_playersettings.tabProfile.color = ( tab == TAB_PROFILE ) ? text_color_highlight : uis.text_color;
	s_playersettings.tabVehicle.color = ( tab == TAB_VEHICLE ) ? text_color_highlight : uis.text_color;
	s_playersettings.tabStats.color = ( tab == TAB_STATS ) ? text_color_highlight : uis.text_color;
	s_playersettings.tabAchievements.color = ( tab == TAB_ACHIEVEMENTS ) ? text_color_highlight : uis.text_color;

	switch ( tab ) {
	case TAB_PROFILE:
		Menu_SetCursorToItem( &s_playersettings.menu, &s_playersettings.tabProfile );
		break;
	case TAB_STATS:
		Menu_SetCursorToItem( &s_playersettings.menu, &s_playersettings.tabStats );
		break;
	case TAB_ACHIEVEMENTS:
		Menu_SetCursorToItem( &s_playersettings.menu, &s_playersettings.tabAchievements );
		break;
	default:
		Menu_SetCursorToItem( &s_playersettings.menu, &s_playersettings.tabVehicle );
		break;
	}
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

// STONELANCE
	if (s_playersettings.modelChanged){
		trap_Cvar_Set( "model", s_playersettings.modelskin );
	}
// END

	if ( UI_Profile_HasActiveProfile() ) {
		profile_info_t info;
		int birthYear;
		int birthMonth;
		int birthDay;
		int maxDay;
		const char *genderValue;

		Com_Memset( &info, 0, sizeof( info ) );

		genderValue = PlayerSettings_GetGenderValue( s_playersettings.gender.curvalue );
		Q_strncpyz( info.gender, genderValue, sizeof( info.gender ) );

		birthYear = PlayerSettings_GetBirthYearFromIndex( s_playersettings.birthYear.curvalue );
		birthMonth = s_playersettings.birthMonth.curvalue;
		birthDay = s_playersettings.birthDay.curvalue;
		if ( birthYear > 0 && birthMonth > 0 && birthDay > 0 ) {
			maxDay = PlayerSettings_GetDaysInMonth( birthYear, birthMonth );
			if ( birthDay > maxDay ) {
				birthDay = maxDay;
			}
			Com_sprintf( info.birthDate, sizeof( info.birthDate ), "%04d-%02d-%02d", birthYear, birthMonth, birthDay );
		}

		if ( s_playersettings.avatarProfileName[0] ) {
			Com_sprintf( info.avatar, sizeof( info.avatar ), "gfx/avatars/%s", s_playersettings.avatarProfileName );
		} else {
			info.avatar[0] = '\0';
		}
		Q_strncpyz( info.country, s_playersettings.country.field.buffer, sizeof( info.country ) );
		UI_Profile_SaveActiveInfo( &info );
	}

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
	if ( key == ( K_MOUSE1 | K_CHAR_FLAG ) ) {
		s_playersettings.statsScroll.dragging = qfalse;
		s_playersettings.achievementsScroll.dragging = qfalse;
		return 0;
	}
	if( key == K_MOUSE2 || key == K_ESCAPE ) {
// STONELANCE
//		PlayerSettings_SaveChanges();
		s_playersettings.menu.transitionMenu = ID_BACK;
		uis.transitionOut = uis.realtime;
		return 0;
// END
	}
	if ( s_playersettings.currentTab == TAB_STATS ) {
		float contentHeight;

		contentHeight = PlayerSettings_GetStatsContentHeight();
		if ( key == K_MOUSE1 ) {
			if ( PlayerSettings_HandleScrollBarClick( &s_playersettings.statsScroll, contentHeight, PLAYERSETTINGS_STATS_ROW_HEIGHT, PLAYERSETTINGS_STATS_ROW_GAP ) ) {
				return menu_move_sound;
			}
		} else if ( PlayerSettings_HandleScrollKey( &s_playersettings.statsScroll, contentHeight, PLAYERSETTINGS_STATS_ROW_HEIGHT, PLAYERSETTINGS_STATS_ROW_GAP, key ) ) {
			return menu_move_sound;
		}
	} else if ( s_playersettings.currentTab == TAB_ACHIEVEMENTS ) {
		float contentHeight;

		contentHeight = PlayerSettings_GetAchievementsContentHeight();
		if ( key == K_MOUSE1 ) {
			if ( PlayerSettings_HandleScrollBarClick( &s_playersettings.achievementsScroll, contentHeight, PlayerSettings_GetAchievementsScrollRowHeight(), PLAYERSETTINGS_ACHIEVEMENT_ROW_GAP ) ) {
				return menu_move_sound;
			}
		} else if ( PlayerSettings_HandleScrollKey( &s_playersettings.achievementsScroll, contentHeight, PlayerSettings_GetAchievementsScrollRowHeight(), PLAYERSETTINGS_ACHIEVEMENT_ROW_GAP, key ) ) {
			return menu_move_sound;
		}
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

	if ( UI_Profile_HasActiveProfile() ) {
		const profile_info_t *info = UI_Profile_GetActiveInfo();
		int parsedYear = 0;
		int parsedMonth = 0;
		int parsedDay = 0;

		if ( info ) {
			s_playersettings.profileInfo = *info;
		} else {
			Com_Memset( &s_playersettings.profileInfo, 0, sizeof( s_playersettings.profileInfo ) );
		}

		s_playersettings.gender.curvalue = PlayerSettings_FindGenderIndex( s_playersettings.profileInfo.gender );

		if ( PlayerSettings_ParseBirthDate( s_playersettings.profileInfo.birthDate, &parsedYear, &parsedMonth, &parsedDay ) ) {
			s_playersettings.birthYear.curvalue = PlayerSettings_GetBirthYearIndex( parsedYear );
			s_playersettings.birthMonth.curvalue = parsedMonth;
			s_playersettings.birthDay.curvalue = parsedDay;
		} else {
			s_playersettings.birthYear.curvalue = 0;
			s_playersettings.birthMonth.curvalue = 0;
			s_playersettings.birthDay.curvalue = 0;
		}
		PlayerSettings_UpdateBirthDateDayItems();

		s_playersettings.avatar.field.buffer[0] = '\0';
		PlayerSettings_SetAvatarProfileName( UI_Profile_GetActiveName() );
		PlayerSettings_EnsureAvatarShader();
		Q_strncpyz( s_playersettings.country.field.buffer, s_playersettings.profileInfo.country, sizeof( s_playersettings.country.field.buffer ) );

		s_playersettings.gender.generic.flags &= ~( QMF_GRAYED | QMF_INACTIVE );
		s_playersettings.birthDay.generic.flags &= ~( QMF_GRAYED | QMF_INACTIVE );
		s_playersettings.birthMonth.generic.flags &= ~( QMF_GRAYED | QMF_INACTIVE );
		s_playersettings.birthYear.generic.flags &= ~( QMF_GRAYED | QMF_INACTIVE );
		s_playersettings.avatar.generic.flags &= ~QMF_GRAYED;
		s_playersettings.avatar.generic.flags |= QMF_INACTIVE;
		s_playersettings.country.generic.flags &= ~( QMF_GRAYED | QMF_INACTIVE );
		s_playersettings.birthDateLabel.color = uis.text_color;
	} else {
		Com_Memset( &s_playersettings.profileInfo, 0, sizeof( s_playersettings.profileInfo ) );
		s_playersettings.gender.curvalue = 0;
		s_playersettings.birthYear.curvalue = 0;
		s_playersettings.birthMonth.curvalue = 0;
		s_playersettings.birthDay.curvalue = 0;
		PlayerSettings_UpdateBirthDateDayItems();
		s_playersettings.avatar.field.buffer[0] = '\0';
		PlayerSettings_SetAvatarProfileName( "" );
		PlayerSettings_EnsureAvatarShader();
		s_playersettings.country.field.buffer[0] = '\0';

		s_playersettings.gender.generic.flags |= QMF_GRAYED | QMF_INACTIVE;
		s_playersettings.birthDay.generic.flags |= QMF_GRAYED | QMF_INACTIVE;
		s_playersettings.birthMonth.generic.flags |= QMF_GRAYED | QMF_INACTIVE;
		s_playersettings.birthYear.generic.flags |= QMF_GRAYED | QMF_INACTIVE;
		s_playersettings.avatar.generic.flags |= QMF_GRAYED | QMF_INACTIVE;
		s_playersettings.country.generic.flags |= QMF_GRAYED | QMF_INACTIVE;
		s_playersettings.birthDateLabel.color = text_color_disabled;
	}


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
	case ID_TAB_PROFILE:
		PlayerSettings_SetTab( TAB_PROFILE );
		break;

	case ID_TAB_VEHICLE:
		PlayerSettings_SetTab( TAB_VEHICLE );
		break;

	case ID_TAB_STATS:
		PlayerSettings_SetTab( TAB_STATS );
		break;

	case ID_TAB_ACHIEVEMENTS:
		PlayerSettings_SetTab( TAB_ACHIEVEMENTS );
		break;

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
	int		profileY;
// STONELANCE
	int		i, j, x;
	static char	modelname[32];
// END

	memset(&s_playersettings,0,sizeof(playersettings_t));

	PlayerSettings_Cache();
	PlayerSettings_InitBirthDateLists();

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

	s_playersettings.tabProfile.generic.type = MTYPE_PTEXT;
	s_playersettings.tabProfile.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_NODEFAULTINIT;
	s_playersettings.tabProfile.generic.id = ID_TAB_PROFILE;
	s_playersettings.tabProfile.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.tabProfile.generic.ownerdraw = PlayerSettings_DrawTabItem;
	PlayerSettings_ConfigureTab( &s_playersettings.tabProfile, TAB_PROFILE );
	s_playersettings.tabProfile.string = "PROFILE";
	s_playersettings.tabProfile.style = UI_CENTER | UI_SMALLFONT;
	s_playersettings.tabProfile.color = uis.text_color;

	s_playersettings.tabVehicle.generic.type = MTYPE_PTEXT;
	s_playersettings.tabVehicle.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_NODEFAULTINIT;
	s_playersettings.tabVehicle.generic.id = ID_TAB_VEHICLE;
	s_playersettings.tabVehicle.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.tabVehicle.generic.ownerdraw = PlayerSettings_DrawTabItem;
	PlayerSettings_ConfigureTab( &s_playersettings.tabVehicle, TAB_VEHICLE );
	s_playersettings.tabVehicle.string = "CAR";
	s_playersettings.tabVehicle.style = UI_CENTER | UI_SMALLFONT;
	s_playersettings.tabVehicle.color = uis.text_color;

	s_playersettings.tabStats.generic.type = MTYPE_PTEXT;
	s_playersettings.tabStats.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_NODEFAULTINIT;
	s_playersettings.tabStats.generic.id = ID_TAB_STATS;
	s_playersettings.tabStats.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.tabStats.generic.ownerdraw = PlayerSettings_DrawTabItem;
	PlayerSettings_ConfigureTab( &s_playersettings.tabStats, TAB_STATS );
	s_playersettings.tabStats.string = "STATS";
	s_playersettings.tabStats.style = UI_CENTER | UI_SMALLFONT;
	s_playersettings.tabStats.color = uis.text_color;

	s_playersettings.tabAchievements.generic.type = MTYPE_PTEXT;
	s_playersettings.tabAchievements.generic.flags = QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_NODEFAULTINIT;
	s_playersettings.tabAchievements.generic.id = ID_TAB_ACHIEVEMENTS;
	s_playersettings.tabAchievements.generic.callback = PlayerSettings_MenuEvent;
	s_playersettings.tabAchievements.generic.ownerdraw = PlayerSettings_DrawTabItem;
	PlayerSettings_ConfigureTab( &s_playersettings.tabAchievements, TAB_ACHIEVEMENTS );
	s_playersettings.tabAchievements.string = "ACHIEV.";
	s_playersettings.tabAchievements.style = UI_CENTER | UI_SMALLFONT;
	s_playersettings.tabAchievements.color = uis.text_color;

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
	y = PLAYERSETTINGS_CONTENT_TOP;
	profileY = PLAYERSETTINGS_PROFILE_PANEL_TOP + PLAYERSETTINGS_PROFILE_PANEL_INNER_MARGIN + 2;
// END
	s_playersettings.name.generic.type			= MTYPE_FIELD;
	s_playersettings.name.generic.flags			= QMF_NODEFAULTINIT;
	s_playersettings.name.generic.ownerdraw		= PlayerSettings_DrawName;
	s_playersettings.name.field.widthInChars	= MAX_NAMELENGTH;
	s_playersettings.name.field.maxchars		= MAX_NAMELENGTH;
			s_playersettings.name.generic.x				= PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.name.generic.y				= profileY;
	s_playersettings.name.generic.left			= PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.name.generic.top			= profileY;
	s_playersettings.name.generic.right		= PLAYERSETTINGS_PROFILE_ROW_RIGHT;
	s_playersettings.name.generic.bottom		= profileY + PLAYERSETTINGS_PROFILE_FIELD_HEIGHT;
	s_playersettings.name.generic.flags |= QMF_INACTIVE | QMF_GRAYED;

	profileY += PLAYERSETTINGS_PROFILE_ROW_HEIGHT;

	s_playersettings.gender.generic.type = MTYPE_SPINCONTROL;
	s_playersettings.gender.generic.flags = QMF_NODEFAULTINIT;
	s_playersettings.gender.generic.ownerdraw = PlayerSettings_DrawProfileList;
	s_playersettings.gender.generic.id = ID_GENDER;
	s_playersettings.gender.generic.name = "Gender";
	s_playersettings.gender.generic.x = PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.gender.generic.y = profileY;
	s_playersettings.gender.generic.left = PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.gender.generic.top = profileY;
	s_playersettings.gender.generic.right = PLAYERSETTINGS_PROFILE_ROW_RIGHT;
	s_playersettings.gender.generic.bottom = profileY + PLAYERSETTINGS_PROFILE_FIELD_HEIGHT;
	s_playersettings.gender.itemnames = (const char **)s_genderItems;
	s_playersettings.gender.numitems = 0;
	while ( s_genderItems[s_playersettings.gender.numitems] ) {
		s_playersettings.gender.numitems++;
	}

	profileY += PLAYERSETTINGS_PROFILE_ROW_HEIGHT;

	s_playersettings.birthDateLabel.generic.type = MTYPE_TEXT;
	s_playersettings.birthDateLabel.generic.flags = QMF_INACTIVE | QMF_NODEFAULTINIT;
	s_playersettings.birthDateLabel.generic.x = PLAYERSETTINGS_PROFILE_FIELD_LEFT + PLAYERSETTINGS_PROFILE_LABEL_OFFSET;
	s_playersettings.birthDateLabel.generic.y = profileY;
	s_playersettings.birthDateLabel.generic.left = PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.birthDateLabel.generic.top = profileY;
	s_playersettings.birthDateLabel.generic.right = PLAYERSETTINGS_PROFILE_ROW_RIGHT;
	s_playersettings.birthDateLabel.generic.bottom = profileY + PLAYERSETTINGS_PROFILE_FIELD_HEIGHT;
	s_playersettings.birthDateLabel.string = "Birth date";
	s_playersettings.birthDateLabel.style = UI_LEFT | UI_SMALLFONT;
	s_playersettings.birthDateLabel.color = uis.text_color;

	{
		const int birthDayWidth = 120;
		const int birthMonthWidth = 210;
		const int birthYearWidth = 150;
		const int birthColumnGap = 18;
		const int birthDayX = PLAYERSETTINGS_PROFILE_FIELD_LEFT;
		const int birthMonthX = birthDayX + birthDayWidth + birthColumnGap;
		const int birthYearX = birthMonthX + birthMonthWidth + birthColumnGap;
		const int birthRowY = profileY + PLAYERSETTINGS_PROFILE_VALUE_BASELINE;

		s_playersettings.birthDay.generic.type = MTYPE_SPINCONTROL;
		s_playersettings.birthDay.generic.flags = QMF_NODEFAULTINIT;
		s_playersettings.birthDay.generic.ownerdraw = PlayerSettings_DrawBirthDateComponent;
		s_playersettings.birthDay.generic.id = ID_BIRTH_DAY;
		s_playersettings.birthDay.generic.name = "Day";
		s_playersettings.birthDay.generic.x = birthDayX;
		s_playersettings.birthDay.generic.y = birthRowY;
		s_playersettings.birthDay.generic.left = birthDayX;
		s_playersettings.birthDay.generic.top = profileY;
		s_playersettings.birthDay.generic.right = birthDayX + birthDayWidth;
		s_playersettings.birthDay.generic.bottom = profileY + PLAYERSETTINGS_PROFILE_FIELD_HEIGHT;
		s_playersettings.birthDay.generic.callback = PlayerSettings_BirthDateChanged;
		s_playersettings.birthDay.itemnames = s_birthDayItems;
		s_playersettings.birthDay.numitems = BIRTH_DAY_MAX + 1;

		s_playersettings.birthMonth.generic.type = MTYPE_SPINCONTROL;
		s_playersettings.birthMonth.generic.flags = QMF_NODEFAULTINIT;
		s_playersettings.birthMonth.generic.ownerdraw = PlayerSettings_DrawBirthDateComponent;
		s_playersettings.birthMonth.generic.id = ID_BIRTH_MONTH;
		s_playersettings.birthMonth.generic.name = "Month";
		s_playersettings.birthMonth.generic.x = birthMonthX;
		s_playersettings.birthMonth.generic.y = birthRowY;
		s_playersettings.birthMonth.generic.left = birthMonthX;
		s_playersettings.birthMonth.generic.top = profileY;
		s_playersettings.birthMonth.generic.right = birthMonthX + birthMonthWidth;
		s_playersettings.birthMonth.generic.bottom = profileY + PLAYERSETTINGS_PROFILE_FIELD_HEIGHT;
		s_playersettings.birthMonth.generic.callback = PlayerSettings_BirthDateChanged;
		s_playersettings.birthMonth.itemnames = (const char **)s_birthMonthItems;
		s_playersettings.birthMonth.numitems = 0;
		for ( s_playersettings.birthMonth.numitems = 0; s_birthMonthItems[s_playersettings.birthMonth.numitems]; ++s_playersettings.birthMonth.numitems ) {
		}

		s_playersettings.birthYear.generic.type = MTYPE_SPINCONTROL;
		s_playersettings.birthYear.generic.flags = QMF_NODEFAULTINIT;
		s_playersettings.birthYear.generic.ownerdraw = PlayerSettings_DrawBirthDateComponent;
		s_playersettings.birthYear.generic.id = ID_BIRTH_YEAR;
		s_playersettings.birthYear.generic.name = "Year";
		s_playersettings.birthYear.generic.x = birthYearX;
		s_playersettings.birthYear.generic.y = birthRowY;
		s_playersettings.birthYear.generic.left = birthYearX;
		s_playersettings.birthYear.generic.top = profileY;
		s_playersettings.birthYear.generic.right = birthYearX + birthYearWidth;
		s_playersettings.birthYear.generic.bottom = profileY + PLAYERSETTINGS_PROFILE_FIELD_HEIGHT;
		s_playersettings.birthYear.generic.callback = PlayerSettings_BirthDateChanged;
		s_playersettings.birthYear.itemnames = s_birthYearItems;
		s_playersettings.birthYear.numitems = BIRTH_YEAR_COUNT + 1;
	}

	profileY += PLAYERSETTINGS_PROFILE_ROW_HEIGHT;

	s_playersettings.avatar.generic.type = MTYPE_FIELD;
	s_playersettings.avatar.generic.flags = QMF_NODEFAULTINIT;
	s_playersettings.avatar.generic.ownerdraw = PlayerSettings_DrawAvatarImage;
	s_playersettings.avatar.generic.name = "Avatar";
	s_playersettings.avatar.field.widthInChars = PROFILE_MAX_AVATAR - 1;
	s_playersettings.avatar.field.maxchars = PROFILE_MAX_AVATAR - 1;
	s_playersettings.avatar.generic.x = PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.avatar.generic.y = profileY;
	s_playersettings.avatar.generic.left = PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.avatar.generic.top = profileY;
	s_playersettings.avatar.generic.right = PLAYERSETTINGS_PROFILE_ROW_RIGHT;
	s_playersettings.avatar.generic.bottom = profileY + PLAYERSETTINGS_PROFILE_FIELD_HEIGHT;

	profileY += PLAYERSETTINGS_PROFILE_ROW_HEIGHT;

	s_playersettings.country.generic.type = MTYPE_FIELD;
	s_playersettings.country.generic.flags = QMF_NODEFAULTINIT;
	s_playersettings.country.generic.ownerdraw = PlayerSettings_DrawProfileField;
	s_playersettings.country.generic.name = "Country";
	s_playersettings.country.field.widthInChars = PROFILE_MAX_COUNTRY - 1;
	s_playersettings.country.field.maxchars = PROFILE_MAX_COUNTRY - 1;
	s_playersettings.country.generic.x = PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.country.generic.y = profileY;
	s_playersettings.country.generic.left = PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.country.generic.top = profileY;
	s_playersettings.country.generic.right = PLAYERSETTINGS_PROFILE_ROW_RIGHT;
	s_playersettings.country.generic.bottom = profileY + PLAYERSETTINGS_PROFILE_FIELD_HEIGHT;

	profileY += PLAYERSETTINGS_PROFILE_ROW_HEIGHT;

//	 y += 3 * PROP_HEIGHT;
// END
	s_playersettings.handicap.generic.type			= MTYPE_SPINCONTROL;
	s_playersettings.handicap.generic.flags		= QMF_NODEFAULTINIT;
	s_playersettings.handicap.generic.id			= ID_HANDICAP;
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
	s_playersettings.handicap.generic.x			= PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.handicap.generic.y			= profileY;
	s_playersettings.handicap.generic.left		= PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.handicap.generic.top		= profileY;
	s_playersettings.handicap.generic.right		= PLAYERSETTINGS_PROFILE_ROW_RIGHT;
	s_playersettings.handicap.generic.bottom	= profileY + PLAYERSETTINGS_PROFILE_FIELD_HEIGHT;
// END
	s_playersettings.handicap.numitems			= 20;

	profileY += PLAYERSETTINGS_PROFILE_ROW_HEIGHT;

// STONELANCE
//	 y += 3 * PROP_HEIGHT;
// END
	s_playersettings.effects.generic.type			= MTYPE_SPINCONTROL;
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
	s_playersettings.effects.generic.bottom	= y + 2* PROP_HEIGHT;
*/
	s_playersettings.effects.generic.x			= PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.effects.generic.y			= profileY;
	s_playersettings.effects.generic.left		= PLAYERSETTINGS_PROFILE_FIELD_LEFT;
	s_playersettings.effects.generic.top		= profileY;
	s_playersettings.effects.generic.right		= PLAYERSETTINGS_PROFILE_ROW_RIGHT;
	s_playersettings.effects.generic.bottom	= profileY + PLAYERSETTINGS_PROFILE_FIELD_HEIGHT;
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
	s_playersettings.back.generic.x					= 25;
	s_playersettings.back.generic.y					= 480 - 40;
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
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.tabProfile );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.tabVehicle );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.tabStats );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.tabAchievements );
// STONELANCE
/*
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.framel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.framer );
*/
// END

	Menu_AddItem( &s_playersettings.menu, &s_playersettings.name );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.gender );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.birthDateLabel );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.birthDay );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.birthMonth );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.birthYear );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.avatar );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.country );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.handicap );
	Menu_AddItem( &s_playersettings.menu, &s_playersettings.effects );

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

	PlayerSettings_SetTab( TAB_PROFILE );
	PlayerSettings_SetMenuItems();

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
        int i;
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

        for ( i = 0; i < PLAYERSETTINGS_ACHIEVEMENT_ICON_COUNT; ++i ) {
                s_playersettings.achievementMedalLocked[i] = 0;
                s_playersettings.achievementMedalUnlocked[i] = 0;
        }
        for ( i = 0; i < PLAYERSETTINGS_ACHIEVEMENT_ICON_COUNT; ++i ) {
                s_playersettings.achievementMedalLocked[i] = PlayerSettings_RegisterAchievementMedal( s_achievementMedalLockedPaths[i] );
                s_playersettings.achievementMedalUnlocked[i] = PlayerSettings_RegisterAchievementMedal( s_achievementMedalUnlockedPaths[i] );
        }

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
