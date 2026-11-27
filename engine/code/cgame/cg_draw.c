/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

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
// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"
#include "cg_hud_elements.h"
#include "../client/keycodes.h"

/* Keep the three compact notices aligned just above the telemetry strip. */
#define RALLY_HUD_OVERLAY_BOTTOM 410

#ifdef MISSIONPACK
#include "../ui/ui_shared.h"

// used for scoreboard
extern displayContextDef_t cgDC;
menuDef_t *menuScoreboard = NULL;
#else
int drawTeamOverlayModificationCount = -1;
#endif

int sortedTeamPlayers[TEAM_MAXOVERLAY];
int	numSortedTeamPlayers;

char systemChat[256];
char teamChat1[256];
char teamChat2[256];


#ifdef MISSIONPACK

int CG_Text_Width(const char *text, float scale, int limit) {
  int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  out = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[*s & 255];
				out += glyph->xSkip;
				s++;
				count++;
			}
    }
  }
  return out * useScale;
	}

int CG_Text_Height(const char *text, float scale, int limit) {
  int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  max = 0;
  if (text) {
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if ( Q_IsColorString(s) ) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[*s & 255];
	      if (max < glyph->height) {
		      max = glyph->height;
			  }
				s++;
				count++;
			}
    }
  }
  return max * useScale;
	}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
  float w, h;
  w = width * scale;
  h = height * scale;
  CG_AdjustFrom640( &x, &y, &w, &h );
  trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
  int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;
	fontInfo_t *font = &cgDC.Assets.textFont;
	if (scale <= cg_smallFont.value) {
		font = &cgDC.Assets.smallFont;
	} else if (scale > cg_bigFont.value) {
		font = &cgDC.Assets.bigFont;
	}
	useScale = scale * font->glyphScale;
  if (text) {
		const char *s = text;
		trap_R_SetColor( color );
		memcpy(&newColor[0], &color[0], sizeof(vec4_t));
    len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[*s & 255];
      //int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
      //float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					CG_Text_PaintChar(x + ofs, y - yadj + ofs, 
														glyph->imageWidth,
														glyph->imageHeight,
														useScale, 
														glyph->s,
														glyph->t,
														glyph->s2,
														glyph->t2,
														glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				CG_Text_PaintChar(x, y - yadj, 
													glyph->imageWidth,
													glyph->imageHeight,
													useScale, 
													glyph->s,
													glyph->t,
													glyph->s2,
													glyph->t2,
													glyph->glyph);
				// CG_DrawPic(x, y - yadj, scale * cgDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * cgDC.Assets.textFont.glyphs[text[i]].imageHeight, cgDC.Assets.textFont.glyphs[text[i]].glyph);
				x += (glyph->xSkip * useScale) + adjust;
				s++;
				count++;
			}
    }
	  trap_R_SetColor( NULL );
  }
}


#endif

/*
==============================================================
CG_DrawField - Draws large numbers for status bar and powerups
==============================================================
*/
#ifndef MISSIONPACK
#endif // MISSIONPACK

/*
==============
CG_Draw3DModel
==============
*/
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );
}

/*
==================
CG_DrawHealthModel
==================
*/
void CG_DrawHealthModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, qhandle_t model2, vec3_t origin, vec3_t angles, float yaw2 ) {
	refdef_t		refdef;
	refEntity_t		ent;

	if ( !cg_draw3dIcons.integer || !cg_drawIcons.integer ) {
		return;
	}

	CG_AdjustFrom640( &x, &y, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );

	memset( &ent, 0, sizeof( ent ) );
	AnglesToAxis( angles, ent.axis );
	VectorCopy( origin, ent.origin );
	ent.hModel = model;
	ent.customSkin = skin;
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );

	if ( model2 ) {
		ent.hModel = model2;
		angles[YAW] = yaw2;
		AnglesToAxis( angles, ent.axis );
		trap_R_AddRefEntityToScene( &ent );
	}

	trap_R_RenderScene( &refdef );
}

/*
==================================================================
CG_DrawFlagModel - Used for both the status bar and the scoreboard
==================================================================
*/
void CG_DrawFlagModel( float x, float y, float w, float h, int team, qboolean force2D ) {
	qhandle_t		cm;
	float			len;
	vec3_t			origin, angles;
	vec3_t			mins, maxs;
	qhandle_t		handle;

	if ( !force2D && cg_draw3dIcons.integer ) {

		VectorClear( angles );

		cm = cgs.media.redFlagModel;

		// offset the origin y and z to center the flag
		trap_R_ModelBounds( cm, mins, maxs );

		origin[2] = -0.5 * ( mins[2] + maxs[2] );
		origin[1] = 0.5 * ( mins[1] + maxs[1] );

		// calculate distance so the flag nearly fills the box
		// assume heads are taller than wide
		len = 0.5 * ( maxs[2] - mins[2] );		
		origin[0] = len / 0.268;	// len / tan( fov/2 )

		angles[YAW] = 60 * sin( cg.time / 2000.0 );;

		if( team == TEAM_RED ) {
			handle = cgs.media.redFlagModel;
		} else if( team == TEAM_BLUE ) {
			handle = cgs.media.blueFlagModel;
		} else if( team == TEAM_GREEN ) {
			handle = cgs.media.greenFlagModel;
		} else if( team == TEAM_YELLOW ) {
			handle = cgs.media.yellowFlagModel;
		} else if( team == TEAM_FREE ) {
			handle = cgs.media.neutralFlagModel;
		} else {
			return;
		}
		CG_Draw3DModel( x, y, w, h, handle, 0, origin, angles );
	} else if ( cg_drawIcons.integer ) {
		gitem_t *item;

		if( team == TEAM_RED ) {
			item = BG_FindItemForPowerup( PW_REDFLAG );
		} else if( team == TEAM_BLUE ) {
			item = BG_FindItemForPowerup( PW_BLUEFLAG );
		} else if( team == TEAM_GREEN ) {
			item = BG_FindItemForPowerup( PW_GREENFLAG );
		} else if( team == TEAM_YELLOW ) {
			item = BG_FindItemForPowerup( PW_YELLOWFLAG );
		} else if( team == TEAM_FREE ) {
			item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		} else {
			return;
		}
		if (item) {
		  CG_DrawPic( x, y, w, h, cg_items[ ITEM_INDEX(item) ].icon );
		}
	}
}

/*
=====================
CG_DrawTeamBackground
=====================
*/
void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team )
{
	vec4_t		hcolor;

	hcolor[3] = alpha;
	if ( team == TEAM_RED ) {
		hcolor[0] = 1;
		hcolor[1] = 0;
		hcolor[2] = 0;
	} else if ( team == TEAM_BLUE ) {
		hcolor[0] = 0;
		hcolor[1] = 0;
		hcolor[2] = 1;
	}
// Q3Rally Code Start
	else if ( team == TEAM_GREEN ) {
		hcolor[0] = 0;
		hcolor[1] = 1;
		hcolor[2] = 0;
	}
	else if ( team == TEAM_YELLOW ) {
		hcolor[0] = 1;
		hcolor[1] = 1;
		hcolor[2] = 0;
	}
// END
	else {
		return;
	}
	trap_R_SetColor( hcolor );
    CG_SetScreenPlacement(PLACE_STRETCH, CG_GetScreenVerticalPlacement());
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
    CG_PopScreenPlacement();
	trap_R_SetColor( NULL );
}

// Q3Rally Code Start - KOTH
static void CG_DrawKOTH_LossFlash( void ) {
	vec4_t flashColor;
	float remain;

	if ( cgs.gametype != GT_KOTH ) {
		return;
	}

	if ( cg.kothLossFlashUntil <= cg.time ) {
		return;
	}

	remain = ( cg.kothLossFlashUntil - cg.time ) / 600.0f;
	if ( remain < 0.0f ) {
		remain = 0.0f;
	}

	flashColor[0] = 0.9f;
	flashColor[1] = 0.1f;
	flashColor[2] = 0.1f;
	flashColor[3] = 0.25f * remain;

	CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );
	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, flashColor );
	CG_PopScreenPlacement();
}
// Q3Rally Code END - KOTH

#ifndef MISSIONPACK


/*
=====================
CG_DrawRallyStatusBar
=====================
*/
static void CG_ShiftHudToastQueue( void ) {
	int i;

	if ( cg.hudToastQueueCount <= 0 ) {
		cg.hudToastQueueCount = 0;
		return;
	}
	for ( i = 1; i < cg.hudToastQueueCount; i++ ) {
		cg.hudToastQueue[i - 1] = cg.hudToastQueue[i];
	}
	cg.hudToastQueueCount--;
}

static cgHudToast_t *CG_AddHudToastEntry( cgHudToastType_t type, int duration ) {
	cgHudToast_t *toast;
	int startTime;
	int previousEnd;
	int previousDuration;

	if ( !cg_drawStatus.integer ) {
		return NULL;
	}
	while ( cg.hudToastQueueCount > 0 ) {
		toast = &cg.hudToastQueue[0];
		if ( cg.time < toast->startTime + toast->duration ) {
			break;
		}
		CG_ShiftHudToastQueue();
	}
	if ( cg.hudToastQueueCount >= HUD_TOAST_QUEUE_SIZE ) {
		return NULL;
	}

	startTime = cg.time;
	if ( cg.hudToastQueueCount > 0 ) {
		toast = &cg.hudToastQueue[cg.hudToastQueueCount - 1];
		previousDuration = toast->duration > 0 ? toast->duration : HUD_TOAST_DISPLAY_TIME;
		previousEnd = toast->startTime + previousDuration;
		if ( previousEnd > startTime ) {
			startTime = previousEnd;
		}
	}

	toast = &cg.hudToastQueue[cg.hudToastQueueCount++];
	Com_Memset( toast, 0, sizeof( *toast ) );
	toast->type = type;
	toast->startTime = startTime;
	toast->eventTime = cg.time;
	toast->duration = duration;
	return toast;
}

void CG_QueueHudToast( int itemNum ) {
	cgHudToastType_t type;
	cgHudToast_t *toast;
	int i;
	int delta;

	if ( itemNum == HUD_TOAST_CLEAN_SECTOR_ITEM ) {
		type = CG_HUD_TOAST_CLEAN_SECTOR;
	} else {
		if ( itemNum < 1 || itemNum >= bg_numItems ||
		     cg_drawPickups.value <= 0.0f ) {
			return;
		}
		type = CG_HUD_TOAST_PICKUP;
	}

	/* Ignore the duplicate local/global event copy, but retain later pickups. */
	for ( i = 0; i < cg.hudToastQueueCount; i++ ) {
		if ( cg.hudToastQueue[i].type == type &&
		     cg.hudToastQueue[i].itemNum == itemNum ) {
			delta = cg.time - cg.hudToastQueue[i].eventTime;
			if ( delta < 0 ) delta = -delta;
			if ( delta <= 200 ) {
				return;
			}
		}
	}

	toast = CG_AddHudToastEntry( type, HUD_TOAST_DISPLAY_TIME );
	if ( toast ) {
		toast->itemNum = itemNum;
	}
}

void CG_QueueRewardToast( qhandle_t icon, sfxHandle_t sound, int rewardCount ) {
	cgHudToast_t *toast;

	if ( !cg_drawRewards.integer ) {
		return;
	}
	toast = CG_AddHudToastEntry( CG_HUD_TOAST_REWARD,
	                             HUD_TOAST_ANNOUNCEMENT_TIME );
	if ( toast ) {
		toast->iconShader = icon;
		toast->sound = sound;
		toast->rewardCount = rewardCount;
	}
}

void CG_QueueAchievementToast( bgAchievementCategory_t category, int tierIndex ) {
	cgHudToast_t *toast;

	if ( category < 0 || category >= BG_ACHIEVEMENT_CATEGORY_COUNT ||
	     !BG_AchievementGetTier( category, tierIndex ) ) {
		return;
	}
	toast = CG_AddHudToastEntry( CG_HUD_TOAST_ACHIEVEMENT,
	                             HUD_TOAST_ANNOUNCEMENT_TIME );
	if ( toast ) {
		toast->achievementCategory = category;
		toast->achievementTier = tierIndex;
		toast->sound = cgs.media.achievementUnlockSound;
	}
}

void CG_QueueRankToast( int rankIndex, const char *name, qboolean rankUp ) {
	cgHudToast_t *toast;

	if ( rankIndex < 0 || !name || !name[0] ) {
		return;
	}
	toast = CG_AddHudToastEntry( CG_HUD_TOAST_RANK,
	                             HUD_TOAST_ANNOUNCEMENT_TIME );
	if ( toast ) {
		toast->rankIndex = rankIndex;
		toast->rankUp = rankUp;
		Q_strncpyz( toast->name, name, sizeof( toast->name ) );
	}
}

void CG_QueueMessageToast( const char *message ) {
	cgHudToast_t *toast;
	int i;

	if ( !message || !message[0] ) {
		return;
	}
	toast = CG_AddHudToastEntry( CG_HUD_TOAST_MESSAGE,
	                             HUD_TOAST_DISPLAY_TIME + 300 );
	if ( toast ) {
		Q_strncpyz( toast->message, message, sizeof( toast->message ) );
		for ( i = 0; toast->message[i]; i++ ) {
			if ( toast->message[i] == '\n' || toast->message[i] == '\r' ) {
				toast->message[i] = ' ';
			}
		}
	}
}

/* drag: a new heat of a match, "title\nsub" like a message */
void CG_QueueHeatToast( const char *title, const char *sub ) {
	cgHudToast_t *toast = CG_AddHudToastEntry( CG_HUD_TOAST_HEAT, HUD_TOAST_DISPLAY_TIME + 300 );
	if ( toast ) Com_sprintf( toast->message, sizeof( toast->message ), "%s\n%s", title, sub );
}

/* "Rear hit on Josh": the zone of his car, the damage dealt. Hits on the
 * same car while its card is still up add up on that card. */
void CG_QueueDerbyHitToast( int client, int zone, int damage, qboolean bonus ) {
	cgHudToast_t *toast;
	int shown;

	if ( client < 0 || client >= MAX_CLIENTS || damage <= 0 ) {
		return;
	}
	if ( cg.hudToastQueueCount > 0 ) {
		toast = &cg.hudToastQueue[cg.hudToastQueueCount - 1];
		if ( toast->type == CG_HUD_TOAST_DERBY_HIT && toast->hitClient == client &&
		     cg.time < toast->startTime + toast->duration - HUD_TOAST_FADE_TIME ) {
			toast->hitZone = zone;
			toast->hitDamage += damage;
			if ( bonus ) toast->hitBonus = qtrue;
			/* the card showing: stays up a while from this hit on */
			shown = cg.time - toast->startTime;
			if ( shown > 0 ) toast->duration = shown + HUD_TOAST_DISPLAY_TIME + 300;
			return;
		}
	}
	toast = CG_AddHudToastEntry( CG_HUD_TOAST_DERBY_HIT, HUD_TOAST_DISPLAY_TIME + 300 );
	if ( !toast ) {
		return;
	}
	toast->hitClient = client;
	toast->hitZone = zone;
	toast->hitDamage = damage;
	toast->hitBonus = bonus;
}

static const char *CG_DerbyHitZoneWord( int zone ) {
	switch ( zone ) {
	case CAR_HIT_ZONE_FRONT: return "Front hit";
	case CAR_HIT_ZONE_REAR: return "Rear hit";
	case CAR_HIT_ZONE_LEFT: return "Left side hit";
	case CAR_HIT_ZONE_RIGHT: return "Right side hit";
	case CAR_HIT_ZONE_ROOF: return "Roof hit";
	case CAR_HIT_ZONE_UNDERBODY: return "Underbody hit";
	default: return "Hit";
	}
}

static const char *CG_GetRewardToastLabel( qhandle_t icon ) {
	if ( icon == cgs.media.medalImpressive ) return "IMPRESSIVE";
	if ( icon == cgs.media.medalImpressiveTelefrag ) return "TELEFRAG";
	if ( icon == cgs.media.medalExcellent ) return "EXCELLENT";
	if ( icon == cgs.media.medalGauntlet ) return "HUMILIATION";
	if ( icon == cgs.media.medalDefend ) return "DEFEND";
	if ( icon == cgs.media.medalAssist ) return "ASSIST";
	if ( icon == cgs.media.medalCapture ) return "CAPTURE";
	if ( icon == cgs.media.medalKothCapture ) return "HILL CAPTURE";
	if ( icon == cgs.media.medalKothDefend ) return "HILL DEFEND";
	return "MEDAL EARNED";
}

static qhandle_t CG_GetAchievementTierMedal( bgAchievementIcon_t icon, int tierIndex ) {
	qhandle_t *handle;

	if ( icon < 0 || icon >= BG_ACHIEVEMENT_ICON_COUNT ||
	     tierIndex < 0 || tierIndex >= BG_ACHIEVEMENT_MAX_TIERS ) {
		return 0;
	}

	handle = &cgs.media.achievementMedalTiers[icon][tierIndex];
	if ( !*handle ) {
		*handle = trap_R_RegisterShaderNoMip( bg_achievementMedalTierPaths[icon][tierIndex] );
	}

	return *handle;
}

/*
=================
CG_SlipstreamGaugeFactor

Share of air drag the slipstream currently removes, and how visible the
gauge is (0 = not drawn). Shared by the gauge and the toast lane, which
moves up out of the gauge slot while the gauge is shown.
=================
*/
#define CG_SLIPSTREAM_GAUGE_FULL    0.25f
#define CG_SLIPSTREAM_GAUGE_W       96.0f
/* "EXCELLENT" -> "Excellent" */
static void CG_HudToastTitleCase( const char *in, char *out, int size ) {
	int i;
	qboolean start = qtrue;
	for ( i = 0; in[i] && i < size - 1; i++ ) {
		char c = in[i];
		if ( start && c >= 'a' && c <= 'z' ) c -= 32;
		else if ( !start && c >= 'A' && c <= 'Z' ) c += 32;
		start = ( c == ' ' );
		out[i] = c;
	}
	out[i] = '\0';
}

static void CG_HudToastStripColors( char *text ) {
	char *out = text;
	for ( ; *text; text++ ) {
		if ( Q_IsColorString( text ) ) {
			text++;
			continue;
		}
		*out++ = *text;
	}
	*out = '\0';
}

/*
=================
CG_HudToastView

The notification lane's current card for the v0.9 HUD: the same queue,
sound and timing as CG_DrawHudToast, told as a tag, a title and a line.
=================
*/
qboolean CG_HudToastView( cgHudToastView_t *view ) {
	cgHudToast_t *toast;
	gitem_t *item;
	const bgAchievementCategoryDef_t *category;
	const bgAchievementTierDef_t *tier;
	int elapsed, duration, itemNum;

	memset( view, 0, sizeof( *view ) );
	while ( cg.hudToastQueueCount > 0 ) {
		toast = &cg.hudToastQueue[0];
		duration = toast->duration > 0 ? toast->duration : HUD_TOAST_DISPLAY_TIME;
		elapsed = cg.time - toast->startTime;
		if ( elapsed < duration &&
		     !( toast->type == CG_HUD_TOAST_PICKUP && cg_drawPickups.value <= 0.0f ) ) {
			break;
		}
		CG_ShiftHudToastQueue();
	}
	if ( cg.hudToastQueueCount <= 0 ) {
		return qfalse;
	}
	toast = &cg.hudToastQueue[0];
	duration = toast->duration > 0 ? toast->duration : HUD_TOAST_DISPLAY_TIME;
	elapsed = cg.time - toast->startTime;
	if ( elapsed < 0 ) {
		return qfalse;
	}
	if ( toast->sound && !toast->soundPlayed ) {
		trap_S_StartLocalSound( toast->sound, CHAN_ANNOUNCER );
		toast->soundPlayed = qtrue;
	}
	view->alpha = 1.0f;
	if ( elapsed > duration - HUD_TOAST_FADE_TIME ) {
		view->alpha = (float)( duration - elapsed ) / HUD_TOAST_FADE_TIME;
	}
	if ( view->alpha < 0.0f ) view->alpha = 0.0f;
	itemNum = toast->itemNum;

	switch ( toast->type ) {
	case CG_HUD_TOAST_CLEAN_SECTOR:
		Q_strncpyz( view->tag, "Clean sector", sizeof( view->tag ) );
		Q_strncpyz( view->title, "+0.5 s NOS", sizeof( view->title ) );
		item = BG_FindItemForPowerup( PW_TURBO );
		if ( item ) itemNum = item - bg_itemlist;
		break;
	case CG_HUD_TOAST_PICKUP:
		if ( itemNum < 1 || itemNum >= bg_numItems ) {
			CG_ShiftHudToastQueue();
			return qfalse;
		}
		item = &bg_itemlist[itemNum];
		Q_strncpyz( view->tag, "Pickup", sizeof( view->tag ) );
		Q_strncpyz( view->title, item->pickup_name ? item->pickup_name : "Pickup", sizeof( view->title ) );
		break;
	case CG_HUD_TOAST_REWARD:
		Q_strncpyz( view->tag, "Reward", sizeof( view->tag ) );
		CG_HudToastTitleCase( CG_GetRewardToastLabel( toast->iconShader ), view->title, sizeof( view->title ) );
		if ( toast->rewardCount > 1 ) {
			Com_sprintf( view->sub, sizeof( view->sub ), "X%d", toast->rewardCount );
		}
		view->icon = toast->iconShader;
		break;
	case CG_HUD_TOAST_ACHIEVEMENT:
		category = BG_AchievementGetCategory( toast->achievementCategory );
		tier = BG_AchievementGetTier( toast->achievementCategory, toast->achievementTier );
		if ( !category || !tier ) {
			CG_ShiftHudToastQueue();
			return qfalse;
		}
		Q_strncpyz( view->tag, "Achievement", sizeof( view->tag ) );
		Q_strncpyz( view->title, tier->name, sizeof( view->title ) );
		if ( category->title ) Q_strncpyz( view->sub, category->title, sizeof( view->sub ) );
		if ( category->icon >= 0 && category->icon < BG_ACHIEVEMENT_ICON_COUNT ) {
			view->icon = ( elapsed < ACHIEVEMENT_LOCKED_TIME ) ?
			             cgs.media.achievementMedalLocked[category->icon] :
			             CG_GetAchievementTierMedal( category->icon, toast->achievementTier );
			if ( !view->icon ) {
				view->icon = cgs.media.achievementMedalUnlocked[category->icon];
			}
		}
		break;
	case CG_HUD_TOAST_RANK:
		Q_strncpyz( view->tag, toast->rankUp ? "Rank up" : "Rank down", sizeof( view->tag ) );
		Q_strncpyz( view->title, toast->name, sizeof( view->title ) );
		view->tone = toast->rankUp ? 1 : 0;
		break;
	case CG_HUD_TOAST_MESSAGE:
	case CG_HUD_TOAST_HEAT:
		Q_strncpyz( view->tag, toast->type == CG_HUD_TOAST_HEAT ? "Heat" : "Message", sizeof( view->tag ) );
		Q_strncpyz( view->title, toast->message, sizeof( view->title ) );
		/* "You fragged X\n2nd place with 3": the second line below */
		{
			char *line = strchr( view->title, '\n' );
			if ( line ) {
				*line++ = '\0';
				Q_strncpyz( view->sub, line, sizeof( view->sub ) );
				line = strchr( view->sub, '\n' );
				if ( line ) *line = '\0';
			}
		}
		view->tone = toast->type == CG_HUD_TOAST_HEAT ? 3 : 2;	/* 3: the message icon in blue */
		break;
	case CG_HUD_TOAST_DERBY_HIT:
		Q_strncpyz( view->tag, "Hit", sizeof( view->tag ) );
		if ( cgs.clientinfo[toast->hitClient].infoValid )
			Com_sprintf( view->title, sizeof( view->title ), "%s on %s", CG_DerbyHitZoneWord( toast->hitZone ),
			             cgs.clientinfo[toast->hitClient].name );
		else
			Q_strncpyz( view->title, CG_DerbyHitZoneWord( toast->hitZone ), sizeof( view->title ) );
		Com_sprintf( view->sub, sizeof( view->sub ), "%d DAMAGE%s", toast->hitDamage, toast->hitBonus ? " \xb7 RAM BONUS" : "" );
		break;
	default:
		CG_ShiftHudToastQueue();
		return qfalse;
	}
	if ( !view->icon && itemNum > 0 && itemNum < bg_numItems &&
	     ( toast->type == CG_HUD_TOAST_PICKUP || toast->type == CG_HUD_TOAST_CLEAN_SECTOR ) ) {
		CG_RegisterItemVisuals( itemNum );
		view->icon = cg_items[itemNum].icon;
	}
	/* the v0.9 type draws plain text: no ^7 or ^4 out of player names */
	CG_HudToastStripColors( view->title );
	CG_HudToastStripColors( view->sub );
	return qtrue;
}

static void CG_DrawRallyStatusBar( void ) {
	if ( cg_drawStatus.integer ) {
		CG_ModernHUD_Draw();
	}
}
#endif



/*
===========================================================================================

 UPPER RIGHT CORNER

===========================================================================================
*/

/*
================
CG_DrawAttacker
================
*/
static float CG_DrawAttacker( float y ) {
	int			t;
	float		size;
// Q3Rally Code
	//vec3_t		angles;
// Q3Rally Code END
	const char	*info;
	const char	*name;
	int			clientNum;

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return y;
	}

	if ( !cg.attackerTime ) {
		return y;
	}

	clientNum = cg.predictedPlayerState.persistant[PERS_ATTACKER];
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || clientNum == cg.snap->ps.clientNum ) {
		return y;
	}

	if ( !cgs.clientinfo[clientNum].infoValid ) {
		cg.attackerTime = 0;
		return y;
	}

	t = cg.time - cg.attackerTime;
	if ( t > ATTACKER_HEAD_TIME ) {
		cg.attackerTime = 0;
		return y;
	}

	size = ICON_SIZE * 1.25;

// Q3Rally Code ( removed function )
//	angles[PITCH] = 0;
//	angles[YAW] = 180;
//	angles[ROLL] = 0;
//	CG_DrawHead( 640 - size, y, size, size, clientNum, angles );
// Q3Rally Code END

	info = CG_ConfigString( CS_PLAYERS + clientNum );
	name = Info_ValueForKey(  info, "n" );
	y += size;
	CG_DrawBigString( 640 - ( Q_PrintStrlen( name ) * BIGCHAR_WIDTH), y, name, 0.5 );

	return y + BIGCHAR_HEIGHT + 2;
}

/*
===============
CG_DrawSnapshot
===============
*/
static float CG_DrawSnapshot( float y ) {
	char		*s;
	int			w;

	s = va( "time:%i snap:%i cmd:%i", cg.snap->serverTime, 
		cg.latestSnapshotNum, cgs.serverCommandSequence );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==========
CG_DrawFPS
==========
*/
#define	FPS_FRAMES	4
static float CG_DrawFPS( float y ) {
	char		*s;
	static int	previousTimes[FPS_FRAMES];
	static int	index;
	int		i, total;
	int		fps;
	static	int	previous;
	int		t, frameTime;
	screenPlacement_e savedHorizontalPlacement;
	screenPlacement_e savedVerticalPlacement;
	vec4_t fpsColor = { 0.480f, 0.610f, 0.650f, 0.90f };

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t = trap_Milliseconds();
	frameTime = t - previous;
	previous = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if ( index > FPS_FRAMES ) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va( "FPS %i", fps );
		savedHorizontalPlacement = CG_GetScreenHorizontalPlacement();
		savedVerticalPlacement = CG_GetScreenVerticalPlacement();
		/* Keep the small readout in the upper-right margin, above the driver
		 * order panel, and clear of the bottom telemetry strip. */
		CG_SetScreenPlacement( PLACE_RIGHT, PLACE_TOP );
		CG_DrawIngameString( 632, 0, s, UI_SMALLFONT | UI_RIGHT,
		                     0.42f, fpsColor );
		CG_SetScreenPlacement( savedHorizontalPlacement, savedVerticalPlacement );
	}

	return y + BIGCHAR_HEIGHT + 4;
}

/*
============
CG_DrawTimer
============
*/
static float CG_DrawTimer( float y ) {
	char		*s;
	int			w;
	int			mins, seconds, tens;
	int			msec;

	msec = cg.time - cgs.levelStartTime;
	if ( msec < 0 )
		msec = 0;	/* Autoball: the match clock starts at the first whistle */

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	s = va( "%i:%i%i", mins, tens, seconds );
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;

	CG_DrawBigString( 635 - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}


/*
==================
CG_DrawTeamOverlay
==================
*/

static float CG_DrawTeamOverlay( float y, qboolean right, qboolean upper ) {
	int x, w, h, xx;
	int i, j, len;
	const char *p;
	vec4_t		hcolor;
	int pwidth, lwidth;
	int plyrs;
	char st[16];
	clientInfo_t *ci;
	gitem_t	*item;
	int ret_y, count;

	if ( !cg_drawTeamOverlay.integer ) {
		return y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_RED && cg.snap->ps.persistant[PERS_TEAM] != TEAM_BLUE ) {
		return y; // Not on any team
	}

	plyrs = 0;

	// max player name width
	pwidth = 0;
	count = (numSortedTeamPlayers > 8) ? 8 : numSortedTeamPlayers;
	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {
			plyrs++;
			len = CG_DrawStrlen(ci->name);
			if (len > pwidth)
				pwidth = len;
		}
	}

	if (!plyrs)
		return y;

	if (pwidth > TEAM_OVERLAY_MAXNAME_WIDTH)
		pwidth = TEAM_OVERLAY_MAXNAME_WIDTH;

	// max location name width
	lwidth = 0;
	for (i = 1; i < MAX_LOCATIONS; i++) {
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && *p) {
			len = CG_DrawStrlen(p);
			if (len > lwidth)
				lwidth = len;
		}
	}

	if (lwidth > TEAM_OVERLAY_MAXLOCATION_WIDTH)
		lwidth = TEAM_OVERLAY_MAXLOCATION_WIDTH;

	w = (pwidth + lwidth + 4 + 7) * TINYCHAR_WIDTH;

	if ( right )
		x = 640 - w;
	else
		x = 0;

	h = plyrs * TINYCHAR_HEIGHT;

	if ( upper ) {
		ret_y = y + h;
	} else {
		y -= h;
		ret_y = y;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		hcolor[0] = 1.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 0.0f;
		hcolor[3] = 0.33f;
	} else { // if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE )
		hcolor[0] = 0.0f;
		hcolor[1] = 0.0f;
		hcolor[2] = 1.0f;
		hcolor[3] = 0.33f;
	}
	trap_R_SetColor( hcolor );
	CG_DrawPic( x, y, w, h, cgs.media.teamStatusBar );
	trap_R_SetColor( NULL );

	for (i = 0; i < count; i++) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM]) {

			hcolor[0] = hcolor[1] = hcolor[2] = hcolor[3] = 1.0;

			xx = x + TINYCHAR_WIDTH;

			CG_DrawStringExt( xx, y,
				ci->name, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, TEAM_OVERLAY_MAXNAME_WIDTH);

			if (lwidth) {
				p = CG_ConfigString(CS_LOCATIONS + ci->location);
				if (!p || !*p)
					p = "unknown";
//				len = CG_DrawStrlen(p);
//				if (len > lwidth)
//					len = lwidth;

//				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth + 
//					((lwidth/2 - len/2) * TINYCHAR_WIDTH);
				xx = x + TINYCHAR_WIDTH * 2 + TINYCHAR_WIDTH * pwidth;
				CG_DrawStringExt( xx, y,
					p, hcolor, qfalse, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT,
					TEAM_OVERLAY_MAXLOCATION_WIDTH);
			}

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );

			Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);

			xx = x + TINYCHAR_WIDTH * 3 + 
				TINYCHAR_WIDTH * pwidth + TINYCHAR_WIDTH * lwidth;

			CG_DrawStringExt( xx, y,
				st, hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );

			// draw weapon icon
			xx += TINYCHAR_WIDTH * 3;

			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
					cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
					cgs.media.deferShader );
			}

			// Draw powerup icons
			if (right) {
				xx = x;
			} else {
				xx = x + w - TINYCHAR_WIDTH;
			}
			for (j = 0; j <= PW_NUM_POWERUPS; j++) {
				if (ci->powerups & (1 << j)) {

					item = BG_FindItemForPowerup( j );

					if (item) {
						CG_DrawPic( xx, y, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 
						trap_R_RegisterShader( item->icon ) );
						if (right) {
							xx -= TINYCHAR_WIDTH;
						} else {
							xx += TINYCHAR_WIDTH;
						}
					}
				}
			}

			y += TINYCHAR_HEIGHT;
		}
	}

	return ret_y;
//#endif
}


/*
=================
CG_DrawUpperRight
=================
*/
static void CG_DrawUpperRight(stereoFrame_t stereoFrame)
{
	float	y;

	y = 0;
    
    CG_SetScreenPlacement(PLACE_RIGHT, PLACE_TOP);

	if ( cgs.gametype != GT_CTF && cgs.gametype != GT_CTF4 &&
	     cgs.gametype != GT_DOMINATION &&
	     cgs.gametype != GT_KOTH &&
	     cgs.gametype != GT_AUTOBALL &&
	     cgs.gametype > GT_TEAM &&
	     cg_drawTeamOverlay.integer == 1 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qtrue );
	} 
	if ( cg_drawSnapshot.integer ) {
		y = CG_DrawSnapshot( y );
	}
	if (cg_drawFPS.integer && (stereoFrame == STEREO_CENTER || stereoFrame == STEREO_RIGHT)) {
		y = CG_DrawFPS( y );
	}
	if ( cg_drawTimer.integer ) {
		y = CG_DrawTimer( y );
	}
	if ( cg_drawAttacker.integer ) {
		CG_DrawAttacker( y );
	}

}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

/*
=================
CG_DrawLowerRight
=================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerRight( void ) {
	float	y;

	y = 470;

	CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);

	if ( isRaceObserver( cg.snap->ps.clientNum ) )
		return;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 2 ) {
		y = CG_DrawTeamOverlay( y, qtrue, qfalse );
	} 

}
#endif // MISSIONPACK

/*
================
CG_DrawLowerLeft
================
*/
#ifndef MISSIONPACK
static void CG_DrawLowerLeft( void ) {
	float	y;

	y = 480;
    
    CG_SetScreenPlacement(PLACE_LEFT, PLACE_BOTTOM);

	if (y > 404)
		y = 404;

	if ( cgs.gametype >= GT_TEAM && cg_drawTeamOverlay.integer == 3 ) {
		CG_DrawTeamOverlay( y, qfalse, qfalse );
	}
}
#endif // MISSIONPACK


//===========================================================================================

/*
=================
CG_DrawTeamInfo
=================
*/
#ifndef MISSIONPACK
static void CG_DrawTeamInfo( void ) {
	int h;
	int i;
	vec4_t		hcolor;
	int		chatHeight;

#define CHATLOC_Y 420 // bottom end
#define CHATLOC_X 0

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT)
		chatHeight = cg_teamChatHeight.integer;
	else
		chatHeight = TEAMCHAT_HEIGHT;
	if (chatHeight <= 0)
		return; // disabled

    CG_SetScreenPlacement( PLACE_LEFT, PLACE_BOTTOM );

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		h = (cgs.teamChatPos - cgs.teamLastChatPos) * TINYCHAR_HEIGHT;

		if ( cgs.clientinfo[cg.clientNum].team == TEAM_RED ) {
			hcolor[0] = 1.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		} else if ( cgs.clientinfo[cg.clientNum].team == TEAM_BLUE ) {
			hcolor[0] = 0.0f;
			hcolor[1] = 0.0f;
			hcolor[2] = 1.0f;
			hcolor[3] = 0.33f;
		} else {
			hcolor[0] = 0.0f;
			hcolor[1] = 1.0f;
			hcolor[2] = 0.0f;
			hcolor[3] = 0.33f;
		}

		trap_R_SetColor( hcolor );
		CG_DrawPic( CHATLOC_X, CHATLOC_Y - h, 640, h, cgs.media.teamStatusBar );
		trap_R_SetColor( NULL );

		hcolor[0] = hcolor[1] = hcolor[2] = 1.0f;
		hcolor[3] = 1.0f;

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
			CG_DrawStringExt( CHATLOC_X + TINYCHAR_WIDTH, 
				CHATLOC_Y - (cgs.teamChatPos - i)*TINYCHAR_HEIGHT, 
				cgs.teamChatMsgs[i % chatHeight], hcolor, qfalse, qfalse,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0 );
		}
	}
}
#endif // MISSIONPACK

/*
===================
CG_DrawHoldableItem
===================
*/
#ifndef MISSIONPACK
/* Stable positions match the numbered keyboard shortcuts. */
static const float trunkDirection[TRUNK_ITEM_COUNT][2] = {
	{ 0, -1 }, { 0.866f, 0.5f }, { -0.866f, 0.5f }
};
static const char *const trunkLabel[TRUNK_ITEM_COUNT] = { "FUEL CAN", "TELEPORTER", "REPAIR KIT" };

qboolean CG_TrunkKeyEvent( int key ) {
	int mask, slot, step, i;
	if ( !cg.trunkOpen ) return qfalse;
	if ( key == K_ESCAPE ) {
		cg.trunkOpen = qfalse;
		return qtrue;
	}
	if ( key != K_MWHEELUP && key != K_MWHEELDOWN ) return qfalse;
	mask = cg.snap ? BG_TrunkMask( cg.snap->ps.stats[STAT_HOLDABLE_ITEM] ) : 0;
	slot = cg.trunkSelected;
	step = key == K_MWHEELUP ? TRUNK_ITEM_COUNT - 1 : 1;
	for ( i = 0; i < TRUNK_ITEM_COUNT; i++ ) {
		slot = ( slot + step ) % TRUNK_ITEM_COUNT;
		if ( mask & ( 1 << slot ) ) {
			cg.trunkSelected = slot;
			break;
		}
	}
	return qtrue;
}

/*
 * The wheel uses the telemetry HUD language: dark translucent panels, a thin
 * cool-grey outline and a bright rim on the active slot.  The round shapes come
 * from anti-aliased masks (tools/generate_trunk_wheel.py) that are tinted here;
 * every mask covers the whole TRUNK_WHEEL_BOX square around the centre.
 */
#define TRUNK_WHEEL_X			590.0f
#define TRUNK_WHEEL_Y			( RALLY_HUD_OVERLAY_BOTTOM - 68.0f )
#define TRUNK_WHEEL_BOX			72.0f
#define TRUNK_WHEEL_ICON_RADIUS	22.5f
#define TRUNK_WHEEL_FADE_TIME	100

static void CG_DrawTrunkMask( qhandle_t mask, const float *color ) {
	if ( !mask ) return;
	trap_R_SetColor( color );
	CG_DrawPic( TRUNK_WHEEL_X - TRUNK_WHEEL_BOX * 0.5f, TRUNK_WHEEL_Y - TRUNK_WHEEL_BOX * 0.5f,
		TRUNK_WHEEL_BOX, TRUNK_WHEEL_BOX, mask );
}

static void CG_DrawTrunkWheel( int mask ) {
	vec4_t fillColor = { 0.008f, 0.012f, 0.016f, 0.74f };
	vec4_t emptyColor = { 0.008f, 0.012f, 0.016f, 0.36f };
	vec4_t selectedColor = { 0.070f, 0.120f, 0.160f, 0.90f };
	vec4_t outlineColor = { 0.240f, 0.340f, 0.360f, 0.78f };
	vec4_t edgeColor = { 0.380f, 0.820f, 1.000f, 1.00f };
	vec4_t iconColor = { 1.00f, 1.00f, 1.00f, 0.80f };
	vec4_t iconSelectedColor = { 1.00f, 1.00f, 1.00f, 1.00f };
	vec4_t iconEmptyColor = { 0.480f, 0.610f, 0.650f, 0.30f };
	vec4_t labelPanelColor = { 0.008f, 0.012f, 0.016f, 0.76f };
	vec4_t accentColor = { Q3RALLY_ACCENT_RED, Q3RALLY_ACCENT_GREEN,
	                        Q3RALLY_ACCENT_BLUE, 0.78f };
	vec4_t textColor = { 0.88f, 0.95f, 0.97f, 1.00f };
	const char *label;
	float alpha, size, x, y;
	int i, width;

	alpha = (float)( cg.time - cg.trunkOpenTime ) / TRUNK_WHEEL_FADE_TIME;
	if ( alpha > 1.0f ) alpha = 1.0f;
	if ( alpha < 0.0f ) alpha = 0.0f;
	fillColor[3] *= alpha;
	emptyColor[3] *= alpha;
	selectedColor[3] *= alpha;
	outlineColor[3] *= alpha;
	edgeColor[3] *= alpha;
	iconColor[3] *= alpha;
	iconSelectedColor[3] *= alpha;
	iconEmptyColor[3] *= alpha;
	labelPanelColor[3] *= alpha;
	accentColor[3] *= alpha;
	textColor[3] *= alpha;

	for ( i = 0; i < TRUNK_ITEM_COUNT; i++ ) {
		qboolean owned = ( mask & ( 1 << i ) ) != 0;
		CG_DrawTrunkMask( cgs.media.trunkSectorShader[i],
			!owned ? emptyColor : ( i == cg.trunkSelected ? selectedColor : fillColor ) );
	}
	CG_DrawTrunkMask( cgs.media.trunkOutlineShader, outlineColor );
	CG_DrawTrunkMask( cgs.media.trunkEdgeShader[cg.trunkSelected], edgeColor );

	for ( i = 0; i < TRUNK_ITEM_COUNT; i++ ) {
		gitem_t *item = BG_FindItemForHoldable( bg_trunkItems[i] );
		int index = item - bg_itemlist;
		qboolean owned = ( mask & ( 1 << i ) ) != 0;
		size = owned && i == cg.trunkSelected ? 16.0f : 13.0f;
		x = TRUNK_WHEEL_X + trunkDirection[i][0] * TRUNK_WHEEL_ICON_RADIUS;
		y = TRUNK_WHEEL_Y + trunkDirection[i][1] * TRUNK_WHEEL_ICON_RADIUS;
		CG_RegisterItemVisuals( index );
		trap_R_SetColor( !owned ? iconEmptyColor :
			( i == cg.trunkSelected ? iconSelectedColor : iconColor ) );
		CG_DrawPic( x - size * 0.5f, y - size * 0.5f, size, size, cg_items[index].icon );
	}
	trap_R_SetColor( NULL );

	/* Name tag in the toast style, between the wheel and the trunk strip. */
	label = trunkLabel[cg.trunkSelected];
	width = CG_IngameStringWidth( label, UI_SMALLFONT, 0.46f ) + 12;
	x = TRUNK_WHEEL_X - width * 0.5f;
	y = RALLY_HUD_OVERLAY_BOTTOM - 33.0f;
	CG_FillRect( x, y, width, 12, labelPanelColor );
	CG_FillRect( x, y, width, 1, accentColor );
	CG_DrawIngameString( (int)TRUNK_WHEEL_X, (int)y + 3, label,
		UI_CENTER | UI_SMALLFONT, 0.46f, textColor );
}

static void CG_TrunkFrame( void ) {
	usercmd_t cmd;
	qboolean held = qfalse, pressed, available;
	int mask, i, catcher;
	screenPlacement_e horizontal, vertical;

	if ( trap_GetUserCmd( trap_GetCurrentCmdNumber(), &cmd ) ) {
		held = ( cmd.buttons & BUTTON_USE_HOLDABLE ) != 0;
	}
	pressed = held && !cg.trunkUseHeld;
	cg.trunkUseHeld = held;
	catcher = trap_Key_GetCatcher();
	mask = BG_TrunkMask( cg.snap->ps.stats[STAT_HOLDABLE_ITEM] );
	available = mask && cg_draw2D.integer && !cg.levelShot && !cg.demoPlayback &&
		!cg.showScores && cg.snap->ps.pm_type == PM_NORMAL && cg.snap->ps.stats[STAT_HEALTH] > 0 &&
		cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR && !( cg.snap->ps.pm_flags & PMF_FOLLOW ) &&
		!isRaceObserver( cg.snap->ps.clientNum ) && !CG_IntroCam_IsActive() && !CG_Autoball_IntroActive();
	if ( available && ( isRallyRace() || cgs.gametype == GT_DERBY || cgs.gametype == GT_LCS ) &&
	     !cg_entities[cg.snap->ps.clientNum].startRaceTime ) {
		available = qfalse;
	}
	if ( !available || catcher ) {
		cg.trunkOpen = qfalse;
		return;
	}
	if ( pressed && cg.trunkOpen ) {
		if ( mask & ( 1 << cg.trunkSelected ) ) {
			trap_SendClientCommand( va( "trunkuse %d", cg.trunkSelected + 1 ) );
		}
		cg.trunkOpen = qfalse;
		return;
	}
	if ( pressed ) {
		cg.trunkOpen = qtrue;
		cg.trunkOpenTime = cg.time;
	}
	if ( !cg.trunkOpen ) return;
	if ( !( mask & ( 1 << cg.trunkSelected ) ) ) {
		for ( i = 0; i < TRUNK_ITEM_COUNT; i++ ) {
			if ( mask & ( 1 << i ) ) break;
		}
		cg.trunkSelected = i;
	}

	horizontal = CG_GetScreenHorizontalPlacement();
	vertical = CG_GetScreenVerticalPlacement();
	/* Same placement as the trunk strip below, so both line up on widescreen. */
	CG_SetScreenPlacement( PLACE_CENTER, PLACE_BOTTOM );
	CG_DrawTrunkWheel( mask );
	CG_SetScreenPlacement( horizontal, vertical );
}

#endif // MISSIONPACK




/*
=========
LAGOMETER
=========
*/

#define	LAG_SAMPLES		128


typedef struct {
	int		frameSamples[LAG_SAMPLES];
	int		frameCount;
	int		snapshotFlags[LAG_SAMPLES];
	int		snapshotSamples[LAG_SAMPLES];
	int		snapshotCount;
} lagometer_t;

lagometer_t		lagometer;

/*
========================================================================================
CG_AddLagometerFrameInfo - Adds the current interpolate / extrapolate bar for this frame
========================================================================================
*/
void CG_AddLagometerFrameInfo( void ) {
	int			offset;

	offset = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[ lagometer.frameCount & ( LAG_SAMPLES - 1) ] = offset;
	lagometer.frameCount++;
}

/*
=======================================================
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
=======================================================
*/
void CG_AddLagometerSnapshotInfo( snapshot_t *snap ) {
	// dropped packet
	if ( !snap ) {
		lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->ping;
	lagometer.snapshotFlags[ lagometer.snapshotCount & ( LAG_SAMPLES - 1) ] = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==================================================================================
CG_DrawDisconnect - Should we draw something differnet for long lag vs no packets?
==================================================================================
*/
static void CG_DrawDisconnect( void ) {
	float		x, y;
	int			cmdNum;
	usercmd_t	cmd;
	const char		*s;
	int			w;

	/* The intro camera is an intentional pre-race pause, not a connection loss. */
	if ( CG_IntroCam_IsRaceIntroPending() ) {
		return;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &cmd );
	if ( cmd.serverTime <= cg.snap->ps.commandTime
		|| cmd.serverTime > cg.time ) {	// special check for map_restart
		return;
	}

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	// also add text in center of screen
	s = "Connection Interrupted";
	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w/2, 100, s, 1.0F);

	// blink the icon
	if ( ( cg.time >> 9 ) & 1 ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);

#ifdef MISSIONPACK
	x = 640 - 48;
	y = 480 - 144;
#else
	x = 640 - 48;
	y = 480 - 48;
#endif

	CG_DrawPic( x, y, 48, 48, trap_R_RegisterShader("gfx/2d/net.tga" ) );
}


#define	MAX_LAGOMETER_PING	900
#define	MAX_LAGOMETER_RANGE	300

/*
================
CG_DrawLagometer
================
*/
static void CG_DrawLagometer( void ) {
	int		a, x, y, i;
	float	v;
	float	ax, ay, aw, ah, mid, range;
	int		color;
	float	vscale;

	if ( !cg_lagometer.integer || cgs.localServer ) {
		CG_DrawDisconnect();
		return;
	}

    CG_SetScreenPlacement(PLACE_RIGHT, PLACE_BOTTOM);

	//
	// draw the graph
	//
#ifdef MISSIONPACK
	x = 640 - 48;
	y = 480 - 144;
#else
	x = 640 - 48;
	y = 480 - 48;
#endif

	trap_R_SetColor( NULL );
	CG_DrawPic( x, y, 48, 48, cgs.media.lagometerShader );

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640( &ax, &ay, &aw, &ah );

	color = -1;
	range = ah / 3;
	mid = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.frameCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.frameSamples[i];
		v *= vscale;
		if ( v > 0 ) {
			if ( color != 1 ) {
				color = 1;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
			}
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic ( ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 2 ) {
				color = 2;
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_BLUE)] );
			}
			v = -v;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	// draw the snapshot latency / drop graph
	range = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for ( a = 0 ; a < aw ; a++ ) {
		i = ( lagometer.snapshotCount - 1 - a ) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if ( v > 0 ) {
			if ( lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED ) {
				if ( color != 5 ) {
					color = 5;	// YELLOW for rate delay
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_YELLOW)] );
				}
			} else {
				if ( color != 3 ) {
					color = 3;
					trap_R_SetColor( g_color_table[ColorIndex(COLOR_GREEN)] );
				}
			}
			v = v * vscale;
			if ( v > range ) {
				v = range;
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader );
		} else if ( v < 0 ) {
			if ( color != 4 ) {
				color = 4;		// RED for dropped snapshots
				trap_R_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
			}
			trap_R_DrawStretchPic( ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader );
		}
	}

	trap_R_SetColor( NULL );

	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_DrawBigString( x, y, "snc", 1.0 );
	}

	CG_DrawDisconnect();
}



/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth ) {
	char	*s;

	Q_strncpyz( cg.centerPrint, str, sizeof(cg.centerPrint) );

	cg.centerPrintTime = cg.time;
	cg.centerPrintY = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s = cg.centerPrint;
	while( *s ) {
		if (*s == '\n')
			cg.centerPrintLines++;
		s++;
	}
}


/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void ) {
	char	*start;
	int		l;
	int y;
#ifdef MISSIONPACK
	int x, w, h;
#endif
	float	*color;

	if ( !cg.centerPrintTime ) {
		return;
	}
	/* Ghost Race picker open: the picker shows the text in its panel. */
	if ( CG_LadderGhost_PickerIsOpen() ) {
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );
	if ( !color ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * 20 / 2;

	while ( 1 ) {
		char linebuffer[1024];

		for ( l = 0; l < 50; l++ ) {
			if ( !start[l] || start[l] == '\n' ) {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

#ifdef MISSIONPACK
		w = CG_Text_Width(linebuffer, 0.5, 0);
		h = CG_Text_Height(linebuffer, 0.5, 0);
		x = (SCREEN_WIDTH - w) / 2;
		CG_Text_Paint(x, y + h, 0.5, color, linebuffer, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		y += h + 6;
#else
        CG_DrawIngameString(SCREEN_WIDTH / 2, y, linebuffer,
            UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, 1.0f, color);
        y += 20;
#endif
		while ( *start && ( *start != '\n' ) ) {
			start++;
		}
		if ( !*start ) {
			break;
		}
		start++;
	}

	trap_R_SetColor( NULL );
}



/*
================================================================================

CROSSHAIR

================================================================================
*/


/*
=================
CG_DrawCrosshair
=================

static void CG_DrawCrosshair(void)
{
	float		w, h;
	qhandle_t	hShader;
	float		f;
	float		x, y;
	int			ca;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	// set color based on health
	if ( cg_crosshairHealth.integer ) {
		vec4_t		hcolor;

		CG_ColorForHealth( hcolor );
		trap_R_SetColor( hcolor );
	} else {
		trap_R_SetColor( NULL );
	}

	w = h = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
		h *= ( 1 + f );
	}

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	CG_DrawPic( ((SCREEN_WIDTH-w)*0.5f)+x, ((SCREEN_HEIGHT-h)*0.5f)+y, w, h, hShader );

	trap_R_SetColor( NULL );
}
*/
/*
==================
CG_DrawCrosshair3D
==================

static void CG_DrawCrosshair3D(void)
{
	float		w;
	qhandle_t	hShader;
	float		f;
	int			ca;

	trace_t trace;
	vec3_t endpos;
	float stereoSep, zProj, maxdist, xmax;
	char rendererinfos[128];
	refEntity_t ent;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if ( cg.renderingThirdPerson ) {
		return;
	}

	w = cg_crosshairSize.value;

	// pulse the size of the crosshair when picking up items
	f = cg.time - cg.itemPickupBlendTime;
	if ( f > 0 && f < ITEM_BLOB_TIME ) {
		f /= ITEM_BLOB_TIME;
		w *= ( 1 + f );
	}

	ca = cg_drawCrosshair.integer;
	if (ca < 0) {
		ca = 0;
	}
	hShader = cgs.media.crosshairShader[ ca % NUM_CROSSHAIRS ];

	// Use a different method rendering the crosshair so players don't see two of them when
	// focusing their eyes at distant objects with high stereo separation
	// We are going to trace to the next shootable object and place the crosshair in front of it.

	// first get all the important renderer information
	trap_Cvar_VariableStringBuffer("r_zProj", rendererinfos, sizeof(rendererinfos));
	zProj = atof(rendererinfos);
	trap_Cvar_VariableStringBuffer("r_stereoSeparation", rendererinfos, sizeof(rendererinfos));
	stereoSep = zProj / atof(rendererinfos);
	
	xmax = zProj * tan(cg.refdef.fov_x * M_PI / 360.0f);
	
	// let the trace run through until a change in stereo separation of the crosshair becomes less than one pixel.
	maxdist = cgs.glconfig.vidWidth * stereoSep * zProj / (2 * xmax);
	VectorMA(cg.refdef.vieworg, maxdist, cg.refdef.viewaxis[0], endpos);
	CG_Trace(&trace, cg.refdef.vieworg, NULL, NULL, endpos, 0, MASK_SHOT);
	
	memset(&ent, 0, sizeof(ent));
	ent.reType = RT_SPRITE;
	ent.renderfx = RF_DEPTHHACK | RF_CROSSHAIR;
	
	VectorCopy(trace.endpos, ent.origin);
	
	// scale the crosshair so it appears the same size for all distances
	ent.radius = w / 640 * xmax * trace.fraction * maxdist / zProj;
	ent.customShader = hShader;

	trap_R_AddRefEntityToScene(&ent);
}

*/

/*
=========================
CG_ScanForCrosshairEntity
=========================
*/
static void CG_ScanForCrosshairEntity( void ) {
	trace_t		trace;
	vec3_t		start, end;
	int			content;

	VectorCopy( cg.refdef.vieworg, start );
	VectorMA( start, 131072, cg.refdef.viewaxis[0], end );

	CG_Trace( &trace, start, vec3_origin, vec3_origin, end, 
		cg.snap->ps.clientNum, CONTENTS_SOLID|CONTENTS_BODY );
	if ( trace.entityNum >= MAX_CLIENTS ) {
		return;
	}

	// if the player is in fog, don't show it
	content = CG_PointContents( trace.endpos, 0 );
	if ( content & CONTENTS_FOG ) {
		return;
	}

	// if the player is invisible, don't show it
	if ( cg_entities[ trace.entityNum ].currentState.powerups & ( 1 << PW_INVIS ) ) {
		return;
	}

	// update the fade timer
	cg.crosshairClientNum = trace.entityNum;
	cg.crosshairClientTime = cg.time;
}


/*
=====================
CG_DrawCrosshairNames
=====================
*/
static void CG_DrawCrosshairNames( void ) {
	float		*color;
	char		*name;
	float		w;

	if ( !cg_drawCrosshair.integer ) {
		return;
	}

	// only draw crosshair in hood view
	if (cg_thirdPersonAngle.value != 0 || cg_thirdPersonRange.value != -20 || cg_thirdPersonHeight.value != 10) {
		return;
	}

	if ( !cg_drawCrosshairNames.integer ) {
		return;
	}
	if ( cg.renderingThirdPerson ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	// scan the known entities to see if the crosshair is sighted on one
	CG_ScanForCrosshairEntity();

	// draw the name of the player being looked at
	color = CG_FadeColor( cg.crosshairClientTime, 1000 );
	if ( !color ) {
		trap_R_SetColor( NULL );
		return;
	}

	name = cgs.clientinfo[ cg.crosshairClientNum ].name;
#ifdef MISSIONPACK
	color[3] *= 0.5f;
	w = CG_Text_Width(name, 0.3f, 0);
	CG_Text_Paint( 320 - w / 2, 190, 0.3f, color, name, 0, 0, ITEM_TEXTSTYLE_SHADOWED);
#else
	w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;
	CG_DrawBigString( 320 - w / 2, 170, name, color[3] * 0.5f );
#endif
	trap_R_SetColor( NULL );
}


//==============================================================================

/*
=================
CG_DrawSpectator
=================
*/
static void CG_DrawSpectator(void) {
	/* following a driver: CG_DrawFollow has it */
	if ( !( cg.snap->ps.pm_flags & PMF_FOLLOW ) ) CG_ModernHUD_DrawSpectator();
}

/*
===========
CG_DrawVote
===========
*/
static void CG_DrawVote(void) {
	char	*s;
	int		sec;

	if ( !cgs.voteTime ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);

	// play a talk beep whenever it is modified
	if ( cgs.voteModified ) {
		cgs.voteModified = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
#ifdef MISSIONPACK
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo);
	CG_DrawSmallString( 0, 58, s, 1.0F );
	s = "or press ESC then click Vote";
	CG_DrawSmallString( 0, 58 + SMALLCHAR_HEIGHT + 2, s, 1.0F );
#else
	s = va("VOTE(%i):%s yes:%i no:%i", sec, cgs.voteString, cgs.voteYes, cgs.voteNo );
	CG_DrawSmallString( 0, 58, s, 1.0F );
#endif
}

/*
===============
CG_DrawTeamVote
===============
*/
static void CG_DrawTeamVote(void) {
	char	*s;
	int		sec, cs_offset;

	if ( cgs.clientinfo[cg.clientNum].team == TEAM_RED )
		cs_offset = 0;
	else if ( cgs.clientinfo[cg.clientNum].team == TEAM_BLUE )
		cs_offset = 1;
	else
		return;

	if ( !cgs.teamVoteTime[cs_offset] ) {
		return;
	}

    CG_SetScreenPlacement(PLACE_LEFT, PLACE_TOP);

	// play a talk beep whenever it is modified
	if ( cgs.teamVoteModified[cs_offset] ) {
		cgs.teamVoteModified[cs_offset] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.teamVoteTime[cs_offset] ) ) / 1000;
	if ( sec < 0 ) {
		sec = 0;
	}
	s = va("TEAMVOTE(%i):%s yes:%i no:%i", sec, cgs.teamVoteString[cs_offset],
							cgs.teamVoteYes[cs_offset], cgs.teamVoteNo[cs_offset] );
	CG_DrawSmallString( 0, 90, s, 1.0F );
}


static qboolean CG_DrawScoreboard( void ) {
#ifdef MISSIONPACK
	static qboolean firstTime = qtrue;

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);

	if (menuScoreboard) {
		menuScoreboard->window.flags &= ~WINDOW_FORCED;
	}
	if (cg_paused.integer) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// should never happen in Team Arena
	if (cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		firstTime = qtrue;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD || cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
	} else {
		if ( !CG_FadeColor( cg.scoreFadeTime, FADE_TIME ) ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			firstTime = qtrue;
			return qfalse;
		}
	}

	if (menuScoreboard == NULL) {
		if ( cgs.gametype >= GT_TEAM ) {
			menuScoreboard = Menus_FindByName("teamscore_menu");
		} else {
			menuScoreboard = Menus_FindByName("score_menu");
		}
	}

	if (menuScoreboard) {
		if (firstTime) {
			CG_SetScoreSelection(menuScoreboard);
			firstTime = qfalse;
		}
		Menu_Paint(menuScoreboard, qtrue);
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 10 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
#else
	/* the v0.9 table: only with TAB (not when wrecked), and between maps */
	return CG_ModernHUD_DrawTab();
#endif
}

/*
CG_DrawIntermission
===================
*/
static void CG_DrawIntermission( stereoFrame_t stereoFrame ) {
//	int key;
#ifdef MISSIONPACK
	//if (cg_singlePlayer.integer) {
	//	CG_DrawCenterString();
	//	return;
	//}
#else
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		CG_DrawCenterString();
		return;
	}
#endif
	cg.scoreFadeTime = cg.time;

	/* a drag tournament: the bracket (after the final: the podium) once the
	 * match's own result had its moment */
	if ( CG_ModernHUD_EndActive() && cgs.gametype == GT_DRAG && CG_DragTourneyDue() ) {
		CG_ModernHUD_DrawTourney();
		cg.scoreBoardShowing = qtrue;
		if ( stereoFrame == STEREO_CENTER ) CG_JukeboxFrame();
		return;
	}
	/* the v0.9 end screen; TAB shows the scoreboard */
	if ( CG_ModernHUD_EndActive() && !cg.showScores ) {
		CG_ModernHUD_DrawEnd();
		cg.scoreBoardShowing = qtrue;
		if ( stereoFrame == STEREO_CENTER ) CG_JukeboxFrame();
		return;
	}

	/* TAB, or a mode without an end screen: the table */
	cg.scoreBoardShowing = CG_DrawScoreboard();
	CG_DrawHUDOptionsMenu();
	if ( stereoFrame == STEREO_CENTER ) {
		CG_JukeboxFrame();
	}
}

/*
=============
CG_DrawFollow
=============
*/
static qboolean CG_DrawFollow( void ) {
	if ( !(cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		return qfalse;
	}

	/* Don't show "following <player>" during the intro route preview. */
	if ( CG_IntroCam_IsActive() ) {
		return qfalse;
	}
	CG_ModernHUD_DrawSpectator();
	return qtrue;
}



/*
==================
CG_DrawAmmoWarning
==================
*/
static void CG_DrawAmmoWarning( void ) {
	const char	*s;

// Q3Rally Code Start
	if (isRallyNonDMRace() || cgs.gametype == GT_DERBY){
		return;
	}
	/* Autoball is played without weapons by default: nothing to run out of */
	if ( cgs.gametype == GT_AUTOBALL && !cg.snap->ps.stats[STAT_WEAPONS] ) {
		return;
	}
// Q3Rally Code END

	if ( cg_drawAmmoWarning.integer == 0 ) {
		return;
	}

	if ( !cg.lowAmmoWarning ) {
		return;
	}

	if ( cg.lowAmmoWarning == 2 ) {
		s = "OUT OF AMMO";
	} else {
		s = "LOW AMMO WARNING";
	}
	CG_ModernHUD_DrawAmmoWarning( s, cg.lowAmmoWarning == 2 );
}


#ifdef MISSIONPACK
/*
==================
CG_DrawProxWarning
==================
*/
static void CG_DrawProxWarning( void ) {
	char s [32];
	int			w;
  static int proxTime;
  int proxTick;

	if( !(cg.snap->ps.eFlags & EF_TICKING ) ) {
    proxTime = 0;
		return;
	}

    CG_SetScreenPlacement(PLACE_CENTER, PLACE_TOP);

  if (proxTime == 0) {
    proxTime = cg.time;
  }

  proxTick = 10 - ((cg.time - proxTime) / 1000);

  if (proxTick > 0 && proxTick <= 5) {
    Com_sprintf(s, sizeof(s), "INTERNAL COMBUSTION IN: %i", proxTick);
  } else {
    Com_sprintf(s, sizeof(s), "YOU HAVE BEEN MINED");
  }

	w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
	CG_DrawBigStringColor( 320 - w / 2, 64 + BIGCHAR_HEIGHT, s, g_color_table[ColorIndex(COLOR_RED)] );
}
#endif

//==================================================================================
#ifdef MISSIONPACK
/* 
=================
CG_DrawTimedMenus
=================
*/
void CG_DrawTimedMenus( void ) {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if ( t > 2500 ) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}
#endif
/*
=========
CG_Draw2D
=========
*/
static void CG_DrawIntroCamOverlay( void ) {
	static const vec4_t accentColor = { Q3RALLY_ACCENT_RED, Q3RALLY_ACCENT_GREEN,
	                                    Q3RALLY_ACCENT_BLUE, 0.96f };
	static const vec4_t mutedColor = { 0.63f, 0.80f, 0.84f, 0.92f };
	int secondsLeft;

	secondsLeft = CG_IntroCam_RemainingSeconds();
	CG_DrawIngameString( 320, 38, "TRACK PREVIEW",
		UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, 0.82f, accentColor );
	if ( secondsLeft > 0 ) {
		CG_DrawIngameString( 320, 57, va( "RACE STARTS IN %02d", secondsLeft ),
			UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, 0.52f, mutedColor );
	}
	CG_DrawIngameString( 320, 434, "PRESS FIRE TO SKIP",
		UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, 0.68f, mutedColor );
}

static void CG_DrawIntroCamFade( void ) {
	vec4_t fadeColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	float alpha = CG_IntroCam_FadeAlpha();

	if ( alpha <= 0.0f ) {
		return;
	}

	fadeColor[3] = alpha;
	trap_R_SetColor( fadeColor );
	trap_R_DrawStretchPic( 0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight,
		0, 0, 1, 1, cgs.media.whiteShader );
	trap_R_SetColor( NULL );
}

static void CG_Draw2D(stereoFrame_t stereoFrame)
{
#ifdef MISSIONPACK
	if (cgs.orderPending && cg.time > cgs.orderTime) {
		CG_CheckOrderPending();
	}
#endif
	// if we are taking a levelshot for the menu, don't draw anything
	if ( cg.levelShot ) {
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		CG_DrawIntermission( stereoFrame );
		return;
	}

	/* Autoball kick-off flight: no HUD over the camera */
	if ( CG_Autoball_IntroActive() ) {
		return;
	}

	/* During the cinematic, replace the entire gameplay HUD with two
	 * unobtrusive custom-font prompts. */
	if ( CG_IntroCam_IsActive() ) {
		if ( stereoFrame == STEREO_CENTER ) {
			CG_DrawIntroCamOverlay();
		}
		/* Ghost Race: the ladder ghost is picked while the intro runs; the
		 * race usually starts right when the intro ends. */
		CG_LadderGhost_DrawPicker();
		return;
	}

	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR 
		|| isRaceObserver( cg.snap->ps.clientNum ) ) {

		CG_DrawSpectator();

		if(stereoFrame == STEREO_CENTER)

		CG_DrawCrosshairNames();
	} else {
		// don't draw any status if dead or the scoreboard is being explicitly shown

		if ( !cg.showScores && cg.snap->ps.stats[STAT_HEALTH] > 0 ) {

#ifdef MISSIONPACK
			if ( cg_drawStatus.integer ) {
            
            CG_SetScreenPlacement(PLACE_CENTER, PLACE_BOTTOM);
            
				Menu_PaintAll();
				CG_DrawTimedMenus();
			}
#else

			CG_DrawRallyStatusBar();

#endif
      
			CG_DrawAmmoWarning();

#ifdef MISSIONPACK
			CG_DrawProxWarning();
#endif      
			if(stereoFrame == STEREO_CENTER)
			CG_DrawCrosshairNames();
			CG_DrawWeaponSelect();

                        /* Rewards, achievements and rank changes draw from the
                         * same serialized HUD toast lane as pickups. */
                }
        }

	CG_DrawKOTH_LossFlash();
	CG_Autoball_Draw2D();
	CG_ModernHUD_DrawChat();	/* the chat under the card */

	if ( cgs.gametype >= GT_TEAM ) {
#ifndef MISSIONPACK
		CG_DrawTeamInfo();
#endif
	}

	CG_DrawVote();
	CG_DrawTeamVote();

	CG_DrawLagometer();

	if ( stereoFrame == STEREO_CENTER ) {
		CG_JukeboxFrame();
	}

#ifdef MISSIONPACK
	if (!cg_paused.integer) {
		CG_DrawUpperRight(stereoFrame);
	}
#else
	CG_DrawUpperRight(stereoFrame);
#endif

#ifndef MISSIONPACK
	CG_DrawLowerRight();
	CG_DrawLowerLeft();
#endif

	CG_DrawFollow();

	cg.scoreBoardShowing = CG_DrawHUD();
	CG_LadderGhost_DrawPicker();
	CG_GhostRace_DrawResultBanner();

	// don't draw center string if scoreboard is up

	if (!cg.scoreBoardShowing)

		cg.scoreBoardShowing = CG_DrawScoreboard();

	if ( !cg.scoreBoardShowing) {
		CG_ModernHUD_DrawMessages();
	}

	/* the HUD options over everything, the big messages too */
	CG_DrawHUDOptionsMenu();
}


/*
========================================================================
CG_DrawActive - Perform all drawing needed to completely fill the screen
========================================================================
*/
void CG_DrawActive( stereoFrame_t stereoView ) {
    char seeking[8];
    trap_Cvar_VariableStringBuffer("cl_demoSeeking", seeking, sizeof(seeking));
    if (cg.demoPlayback && atoi(seeking)) return;
	// optionally draw the info screen instead
	if ( !cg.snap ) {
		CG_DrawInformation();
		CG_DrawIntroCamFade();
		return;
	}

    if ( cg.demoPlayback && CG_DemoPlayerActive() ) {
        trap_R_RenderScene( &cg.refdef );
        return;
    }
	// clear around the rendered view if sized down
	CG_TileClear();

	if(stereoView != STEREO_CENTER)
//		CG_DrawCrosshair3D();
	// apply earthquake effect
	CG_Earthquake();

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

// Q3Rally Code Start
	CG_ModernHUD_DrawViews();	/* the rear view and the minimap */

// Q3Rally Code END

	// draw status bar and other floating elements
 	CG_Draw2D(stereoView);
#ifndef MISSIONPACK
	CG_TrunkFrame();
#endif
	CG_DrawIntroCamFade();
}
