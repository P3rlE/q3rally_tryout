/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The HUD design's boxes, the way Chrome lays out its pages
(tools/ui_webstyle/hud_design/gen.py): fixed-width columns in the corners
of the screen, flex rows and columns with gaps inside them, panels with a
1px #373737 border, a 2px accent top border and 14px 16px padding.
*/

#ifdef CGAME
#include "cg_local.h"
#endif
#include "cg_hud_modernlayout.h"

/* the corners (gen.py: TL, TR, BL, BR, top_center) */
#define MH_SIDE			32.0f
#define MH_TOP			28.0f
#define MH_BOTTOM		28.0f
#define MH_CENTER_TOP	22.0f
#define MH_COLUMN_GAP	10.0f
#define MH_CARD_W		300.0f
#define MH_ORDER_W		272.0f
#define MH_COCKPIT_W	330.0f
#define MH_MIRROR_W		380.0f
#define MH_MIRROR_H		92.0f
#define MH_TOAST_W		420.0f
/* panel(): border 1px, border-top 2px accent, padding 14px 16px */
#define MH_PAD_X		16.0f
#define MH_PAD_Y		14.0f

void MH_Label( wsTextStyle_t *style, float size ) {
	style->font = WS_MONO;
	style->size = size;
	style->spacing = 0.16f * size;
	style->uppercase = qtrue;
	style->lineHeight = WS_ContentHeight( style );
}

void MH_Big( wsTextStyle_t *style, float size ) {
	style->font = WS_CONDENSED_ITALIC;
	style->size = size;
	style->spacing = 0.0f;
	style->uppercase = qfalse;
	style->lineHeight = size;
}

void MH_Mono( wsTextStyle_t *style, float size ) {
	style->font = WS_MONO;
	style->size = size;
	style->spacing = 0.0f;
	style->uppercase = qfalse;
	style->lineHeight = WS_ContentHeight( style );
}

void MH_Text( wsTextStyle_t *style, float size ) {
	style->font = WS_BODY;
	style->size = size;
	style->spacing = 0.0f;
	style->uppercase = qfalse;
	style->lineHeight = WS_ContentHeight( style );
}

/* -------------------------------------------------------------------------
   text items
   ------------------------------------------------------------------------- */

static float MH_Width( const mhText_t *t ) {
	return WS_TextWidth( &t->style, t->text, strlen( t->text ) );
}

/* white-space: nowrap; overflow: hidden; text-overflow: ellipsis */
static void MH_Fit( mhText_t *t, float width ) {
	char whole[MH_TEXT_MAX];
	/* Share Tech Mono has no "…" in its atlas: three dots, as wide as it is */
	const char *ellipsis = t->style.font == WS_MONO ? "..." : MH_ELLIPSIS;
	int length;
	if ( MH_Width( t ) <= width + 0.01f ) return;
	Q_strncpyz( whole, t->text, sizeof( whole ) );
	for ( length = strlen( whole ); length > 0; length-- ) {
		Q_strncpyz( t->text, whole, length + 1 );
		Q_strcat( t->text, sizeof( t->text ), ellipsis );
		if ( MH_Width( t ) <= width + 0.01f ) return;
	}
	Q_strncpyz( t->text, ellipsis, sizeof( t->text ) );
}

/* a block holding one line: as wide as its text (shrink to fit) */
static float MH_Set( mhText_t *t, const wsTextStyle_t *style, const char *text ) {
	t->style = *style;
	WS_Transform( style, text ? text : "", t->text, sizeof( t->text ) );
	t->box.w = MH_Width( t );
	t->box.h = style->lineHeight;
	return t->box.w;
}

static void MH_Place( mhText_t *t, float x, float y ) {
	t->box.x = x;
	t->box.y = y;
}

static float MH_Base( const mhText_t *t ) {
	return WS_Baseline( &t->style );
}

/* row(left, right): flex, space-between, align-items baseline */
static float MH_BaselineRow( mhText_t *a, mhText_t *b, float x, float y, float w ) {
	float above = MH_Base( a ), below = a->box.h - MH_Base( a );
	if ( MH_Base( b ) > above ) above = MH_Base( b );
	if ( b->box.h - MH_Base( b ) > below ) below = b->box.h - MH_Base( b );
	MH_Place( a, x, y + above - MH_Base( a ) );
	MH_Place( b, x + w - b->box.w, y + above - MH_Base( b ) );
	return above + below;
}

static float MH_PanelHeight( float content, qboolean accent, float padY ) {
	return ( accent ? 2.0f : 1.0f ) + 2.0f * padY + content + 1.0f;
}

/* -------------------------------------------------------------------------
   top left: the mode card (gen.py modecard, posline, kv)
   ------------------------------------------------------------------------- */

static void MH_Card( mhRaceLayout_t *l ) {
	wsTextStyle_t label, big64, big26, big34, mono17;
	wsRect_t in;
	float y, h, posAbove, posBelow, left, right;
	int i;

	MH_Label( &label, 12.0f );
	MH_Big( &big64, 64.0f );
	MH_Big( &big26, 26.0f );
	MH_Big( &big34, 34.0f );
	MH_Mono( &mono17, 17.0f );

	l->card.x = MH_SIDE;
	l->card.y = MH_TOP;
	l->card.w = MH_CARD_W;
	in.x = l->card.x + 1.0f + MH_PAD_X;
	in.w = l->card.w - 2.0f - 2.0f * MH_PAD_X;
	y = l->card.y + 2.0f + MH_PAD_Y;

	MH_Set( &l->titleT, &label, l->title );
	MH_Fit( &l->titleT, in.w );
	MH_Place( &l->titleT, in.x, y );
	l->titleT.box.w = in.w;		/* a block over the card's width */
	y += l->titleT.box.h + 12.0f;

	/* posline: "P3" "/ 6" on one baseline at the left, "LAP" over "2/5" at
	 * the right, both at the bottom of the row */
	if ( l->pos || l->lapValue ) {
		left = right = 0.0f;
		posAbove = posBelow = 0.0f;
		if ( l->pos && l->posLabel ) {
			/* label over the figure, like the lap at the right */
			MH_Set( &l->posLabelT, &label, l->posLabel );
			MH_Set( &l->posT, &big64, l->pos );
			left = l->posLabelT.box.h + 4.0f + l->posT.box.h;
		} else if ( l->pos ) {
			MH_Set( &l->posT, &big64, l->pos );
			MH_Set( &l->ofT, &big26, l->of );
			posAbove = MH_Base( &l->posT );
			if ( MH_Base( &l->ofT ) > posAbove ) posAbove = MH_Base( &l->ofT );
			posBelow = l->posT.box.h - MH_Base( &l->posT );
			if ( l->ofT.box.h - MH_Base( &l->ofT ) > posBelow ) posBelow = l->ofT.box.h - MH_Base( &l->ofT );
			left = posAbove + posBelow;
		}
		if ( l->lapValue ) {
			MH_Set( &l->lapLabelT, &label, l->lapLabel );
			MH_Set( &l->lapValueT, &big34, l->lapValue );
			right = l->lapLabelT.box.h + 4.0f + l->lapValueT.box.h;
		}
		h = left > right ? left : right;
		if ( l->pos && l->posLabel ) {
			float top = y + h - left;
			MH_Place( &l->posLabelT, in.x, top );
			MH_Place( &l->posT, in.x, top + l->posLabelT.box.h + 4.0f );
			if ( l->posT.box.w > l->posLabelT.box.w ) l->posLabelT.box.w = l->posT.box.w;	/* a column's block */
		} else if ( l->pos ) {
			float top = y + h - left;
			MH_Place( &l->posT, in.x, top + posAbove - MH_Base( &l->posT ) );
			MH_Place( &l->ofT, in.x + l->posT.box.w + 6.0f, top + posAbove - MH_Base( &l->ofT ) );
		}
		if ( l->lapValue ) {
			float top = y + h - right;
			MH_Place( &l->lapLabelT, in.x + in.w - l->lapLabelT.box.w, top );
			MH_Place( &l->lapValueT, in.x + in.w - l->lapValueT.box.w, top + l->lapLabelT.box.h + 4.0f );
		}
		y += h + 12.0f;
	}
	/* sprint_progress(): margin-top 4px, 6px high; a row of two labels below */
	if ( l->progress ) {
		l->progressBar.x = in.x;
		l->progressBar.y = y + 4.0f;
		l->progressBar.w = in.w;
		l->progressBar.h = 6.0f;
		y += 4.0f + 6.0f + 12.0f;
		MH_Set( &l->progressLeftT, &label, l->progressLeft );
		MH_Set( &l->progressRightT, &label, l->progressRight );
		y += MH_BaselineRow( &l->progressLeftT, &l->progressRightT, in.x, y, in.w ) + 12.0f;
	}
	if ( l->pos || l->lapValue || l->progress ) {
		l->sep.x = in.x;
		l->sep.y = y;
		l->sep.w = in.w;
		l->sep.h = 1.0f;
		y += 1.0f + 12.0f;
	}

	/* elimination: a column 6px apart: the row, a 6px bar, a small line */
	if ( l->elim ) {
		wsTextStyle_t small;
		MH_Label( &small, 11.0f );
		MH_Set( &l->elimLabelT, &label, l->elimLabel );
		MH_Set( &l->elimValueT, &mono17, l->elimValue );
		y += MH_BaselineRow( &l->elimLabelT, &l->elimValueT, in.x, y, in.w ) + 6.0f;
		l->elimBar.x = in.x;
		l->elimBar.y = y;
		l->elimBar.w = in.w;
		l->elimBar.h = 6.0f;
		y += 6.0f + 6.0f;
		MH_Set( &l->elimCaptionT, &small, l->elimCaption );
		MH_Fit( &l->elimCaptionT, in.w );
		MH_Place( &l->elimCaptionT, in.x, y );
		l->elimCaptionT.box.w = in.w;	/* a block over the card's width */
		y += l->elimCaptionT.box.h + 12.0f;
	}

	for ( i = 0; i < l->kvCount; i++ ) {
		wsTextStyle_t key, value;
		MH_Label( &key, 12.0f );
		MH_Mono( &value, l->kvSize[i] > 0.0f ? l->kvSize[i] : 17.0f );
		MH_Set( &l->kvKeyT[i], &key, l->kvKey[i] );
		MH_Set( &l->kvValueT[i], &value, l->kvValue[i] );
		y += MH_BaselineRow( &l->kvKeyT[i], &l->kvValueT[i], in.x, y, in.w ) + 12.0f;
	}
	y -= 12.0f;
	l->card.h = y + MH_PAD_Y + 1.0f - l->card.y;
}

/* -------------------------------------------------------------------------
   top right: the standings (gen.py standings)
   ------------------------------------------------------------------------- */

static void MH_Order( mhRaceLayout_t *l ) {
	wsTextStyle_t label, mono13, text15, label11, big20;
	wsRect_t in;
	float y;
	int i;

	MH_Label( &label, 12.0f );
	MH_Mono( &mono13, 13.0f );
	MH_Text( &text15, 15.0f );
	MH_Label( &label11, 11.0f );
	MH_Big( &big20, 20.0f );

	l->order.x = l->viewWidth - MH_SIDE - MH_ORDER_W;
	l->order.y = MH_TOP;
	l->order.w = MH_ORDER_W;
	in.x = l->order.x + 1.0f + MH_PAD_X;
	in.w = l->order.w - 2.0f - 2.0f * MH_PAD_X;
	y = l->order.y + 2.0f + MH_PAD_Y;

	MH_Set( &l->orderTitleT, &label, l->orderTitle );
	MH_Set( &l->orderRightT, &label, l->orderRight );
	y += MH_BaselineRow( &l->orderTitleT, &l->orderRightT, in.x, y, in.w ) + 10.0f;

	/* the teams: a grid of equal cells 4px apart, padding 5px 8px, name and
	 * figure at the ends, centred */
	if ( l->teamCount > 0 ) {
		float cellW = ( in.w - 4.0f * ( l->teamCount - 1 ) ) / l->teamCount, h = 0.0f;
		for ( i = 0; i < l->teamCount; i++ ) {
			MH_Set( &l->teamNameT[i], &label11, l->teamName[i] );
			MH_Set( &l->teamValueT[i], &big20, l->teamValue[i] );
			if ( l->teamNameT[i].box.h > h ) h = l->teamNameT[i].box.h;
			if ( l->teamValueT[i].box.h > h ) h = l->teamValueT[i].box.h;
		}
		h += 10.0f;
		for ( i = 0; i < l->teamCount; i++ ) {
			wsRect_t *c = &l->teamCell[i];
			c->x = in.x + i * ( cellW + 4.0f );
			c->y = y;
			c->w = cellW;
			c->h = h;
			MH_Place( &l->teamNameT[i], c->x + 8.0f, c->y + ( h - l->teamNameT[i].box.h ) * 0.5f );
			MH_Place( &l->teamValueT[i], c->x + c->w - 8.0f - l->teamValueT[i].box.w, c->y + ( h - l->teamValueT[i].box.h ) * 0.5f );
		}
		y += h + 10.0f;
	}

	/* rows: margin 0 -8px, 26px high, padding 0 8px, gap 10px, centred */
	for ( i = 0; i < l->rowCount; i++ ) {
		wsRect_t *r = &l->row[i];
		float x, valueW;
		r->x = in.x - 8.0f;
		r->y = y;
		r->w = in.w + 16.0f;
		r->h = 26.0f;
		x = r->x + 8.0f;
		MH_Set( &l->rowPosT[i], &mono13, l->rowPos[i] );
		MH_Place( &l->rowPosT[i], x, r->y + ( r->h - l->rowPosT[i].box.h ) * 0.5f );
		x += l->rowPosT[i].box.w + 10.0f;
		if ( l->rowSwatch ) {
			l->rowSwatchBox[i].x = x;
			l->rowSwatchBox[i].y = r->y + ( r->h - 8.0f ) * 0.5f;
			l->rowSwatchBox[i].w = l->rowSwatchBox[i].h = 8.0f;
			x += 8.0f + 10.0f;
		}
		valueW = MH_Set( &l->rowValueT[i], &mono13, l->rowValue[i] );
		MH_Place( &l->rowValueT[i], r->x + r->w - 8.0f - valueW, r->y + ( r->h - l->rowValueT[i].box.h ) * 0.5f );
		MH_Set( &l->rowNameT[i], &text15, l->rowName[i] );
		l->rowNameT[i].box.w = r->x + r->w - 8.0f - valueW - 10.0f - x;
		MH_Fit( &l->rowNameT[i], l->rowNameT[i].box.w );
		MH_Place( &l->rowNameT[i], x, r->y + ( r->h - l->rowNameT[i].box.h ) * 0.5f );
		y += r->h + 2.0f;
	}
	if ( l->rowCount ) y -= 2.0f;
	else y -= 10.0f;
	l->order.h = y + MH_PAD_Y + 1.0f - l->order.y;
}

/* -------------------------------------------------------------------------
   top centre: mirror and toast (gen.py top_center, mirror, toast)
   ------------------------------------------------------------------------- */

static void MH_TopCentre( mhRaceLayout_t *l ) {
	wsTextStyle_t label11, text19, mono12;
	float y = MH_CENTER_TOP, x, textH, content, textW;

	if ( l->mirror ) {
		l->mirrorBox.x = ( l->viewWidth - MH_MIRROR_W ) * 0.5f;
		l->mirrorBox.y = y;
		l->mirrorBox.w = MH_MIRROR_W;
		l->mirrorBox.h = MH_MIRROR_H;
		y += MH_MIRROR_H + 12.0f;
	}
	if ( !l->toast ) return;

	MH_Label( &label11, 11.0f );
	MH_Text( &text19, 19.0f );
	MH_Mono( &mono12, 12.0f );
	l->toastBox.x = ( l->viewWidth - MH_TOAST_W ) * 0.5f;
	l->toastBox.y = y;
	l->toastBox.w = MH_TOAST_W;
	x = l->toastBox.x + 1.0f + MH_PAD_X;
	textW = l->toastBox.w - 2.0f - 2.0f * MH_PAD_X;

	/* padding 12px 16px; an icon box of 44px + border, gap 14px */
	MH_Set( &l->toastTagT, &label11, l->toastTag );
	MH_Set( &l->toastTitleT, &text19, l->toastTitle );
	textH = l->toastTagT.box.h + 4.0f + l->toastTitleT.box.h;
	if ( l->toastSub ) {
		MH_Set( &l->toastSubT, &mono12, l->toastSub );
		textH += 4.0f + l->toastSubT.box.h;
	}
	content = textH;
	if ( l->toastIcon && content < 46.0f ) content = 46.0f;
	y = l->toastBox.y + 2.0f + 12.0f;
	if ( l->toastIcon ) {
		l->toastIconBox.x = x;
		l->toastIconBox.y = y + ( content - 46.0f ) * 0.5f;
		l->toastIconBox.w = l->toastIconBox.h = 46.0f;
		x += 46.0f + 14.0f;
		textW -= 46.0f + 14.0f;
	}
	y += ( content - textH ) * 0.5f;
	MH_Fit( &l->toastTagT, textW );
	MH_Fit( &l->toastTitleT, textW );
	MH_Place( &l->toastTagT, x, y );
	y += l->toastTagT.box.h + 4.0f;
	MH_Place( &l->toastTitleT, x, y );
	y += l->toastTitleT.box.h + 4.0f;
	if ( l->toastSub ) {
		MH_Fit( &l->toastSubT, textW );
		MH_Place( &l->toastSubT, x, y );
	}
	/* the lines stretch to the widest of them */
	{
		float widest = l->toastTagT.box.w > l->toastTitleT.box.w ? l->toastTagT.box.w : l->toastTitleT.box.w;
		if ( l->toastSub && l->toastSubT.box.w > widest ) widest = l->toastSubT.box.w;
		if ( widest > textW ) widest = textW;
		l->toastTagT.box.w = l->toastTitleT.box.w = widest;
		if ( l->toastSub ) l->toastSubT.box.w = widest;
	}
	l->toastBox.h = 2.0f + 12.0f + content + 12.0f + 1.0f;
}

/* -------------------------------------------------------------------------
   bottom left: jukebox (gen.py jukebox) and the car (gen.py vehicle)
   ------------------------------------------------------------------------- */

static float MH_Vehicle( mhRaceLayout_t *l, float bottom ) {
	wsTextStyle_t label, big40, big28, label11, big24, text12;
	float width = l->vehicleWidth > 0.0f ? l->vehicleWidth : MH_CARD_W;
	float x = MH_SIDE + 1.0f + MH_PAD_X, w = width - 2.0f - 2.0f * MH_PAD_X;
	float leftH, rightH, headH, y, slotsH = 0.0f, slotW;
	int i;

	MH_Label( &label, 12.0f );
	MH_Big( &big40, 40.0f );
	MH_Big( &big28, 28.0f );
	MH_Set( &l->integrityLabel, &label, "INTEGRITY" );
	MH_Set( &l->integrityT, &big40, l->integrity );
	leftH = l->integrityLabel.box.h + 4.0f + l->integrityT.box.h;
	rightH = 0.0f;
	if ( l->armor ) {
		MH_Set( &l->armorLabel, &label, "ARMOR" );
		MH_Set( &l->armorT, &big28, l->armor );
		rightH = l->armorLabel.box.h + 4.0f + l->armorT.box.h;
	}
	headH = leftH > rightH ? leftH : rightH;
	/* the integrity column stretches its lines to the widest; the armor's
	 * (align-items: flex-end) keeps them at their own width, at the right */
	if ( l->integrityT.box.w < l->integrityLabel.box.w ) l->integrityT.box.w = l->integrityLabel.box.w;
	else l->integrityLabel.box.w = l->integrityT.box.w;

	/* the weapons and items: a line, then a grid of slots with gaps of 12px,
	 * each its label, the icon (20px) beside the figure, the name below */
	MH_Label( &label11, 11.0f );
	MH_Big( &big24, 24.0f );
	MH_Text( &text12, 12.0f );
	slotW = l->slotCount ? ( w - ( l->slotCount - 1 ) * 12.0f ) / l->slotCount : 0.0f;
	for ( i = 0; i < l->slotCount; i++ ) {
		MH_Set( &l->slotLabelT[i], &label11, l->slotLabel[i] );
		MH_Set( &l->slotValueT[i], &big24, l->slotValue[i] );
		MH_Set( &l->slotNameT[i], &text12, l->slotName[i] );
		MH_Fit( &l->slotLabelT[i], slotW );
		MH_Fit( &l->slotNameT[i], slotW );
		slotsH = l->slotLabelT[i].box.h + 6.0f + 24.0f + 6.0f + l->slotNameT[i].box.h;
	}

	l->vehicle.x = MH_SIDE;
	l->vehicle.w = width;
	l->vehicle.h = MH_PanelHeight( headH + 10.0f + 4.0f + ( l->slotCount ? 10.0f + 1.0f + 10.0f + slotsH : 0.0f ),
	                               qtrue, MH_PAD_Y );
	l->vehicle.y = bottom - l->vehicle.h;
	y = l->vehicle.y + 2.0f + MH_PAD_Y;
	MH_Place( &l->integrityLabel, x, y + headH - leftH );
	MH_Place( &l->integrityT, x, y + headH - leftH + l->integrityLabel.box.h + 4.0f );
	if ( l->armor ) {
		MH_Place( &l->armorLabel, x + w - l->armorLabel.box.w, y + headH - rightH );
		MH_Place( &l->armorT, x + w - l->armorT.box.w, y + headH - rightH + l->armorLabel.box.h + 4.0f );
	}
	l->integrityBar.x = x;
	l->integrityBar.y = y + headH + 10.0f;
	l->integrityBar.w = w;
	l->integrityBar.h = 4.0f;
	if ( l->slotCount ) {
		y = l->integrityBar.y + 4.0f + 10.0f;
		l->vehicleSep.x = x;
		l->vehicleSep.y = y;
		l->vehicleSep.w = w;
		l->vehicleSep.h = 1.0f;
		y += 1.0f + 10.0f;
		for ( i = 0; i < l->slotCount; i++ ) {
			float sx = x + i * ( slotW + 12.0f ), rowY = y + l->slotLabelT[i].box.h + 6.0f;
			MH_Place( &l->slotLabelT[i], sx, y );
			l->slotLabelT[i].box.w = slotW;
			l->slotIcon[i].x = sx;
			l->slotIcon[i].y = rowY + 2.0f;
			l->slotIcon[i].w = l->slotIcon[i].h = 20.0f;
			MH_Place( &l->slotValueT[i], sx + 20.0f + 8.0f, rowY );
			MH_Place( &l->slotNameT[i], sx, rowY + 24.0f + 6.0f );
			l->slotNameT[i].box.w = slotW;
		}
	}
	return l->vehicle.y;
}

static void MH_JukeboxClosed( mhRaceLayout_t *l, float bottom ) {
	wsTextStyle_t text14, mono12;
	float x, rowH;

	MH_Text( &text14, 14.0f );
	MH_Mono( &mono12, 12.0f );
	MH_Set( &l->jukeboxLineT, &text14, l->jukeboxLine );
	/* max-width 190px, cut with an ellipsis */
	if ( l->jukeboxLineT.box.w > 190.0f ) {
		MH_Fit( &l->jukeboxLineT, 190.0f );
		l->jukeboxLineT.box.w = 190.0f;
	}
	MH_Set( &l->jukeboxTimeT, &mono12, l->jukeboxTime );

	/* a row with padding 8px 12px, a 16px icon, gaps of 10px; a 2px bar below */
	rowH = 16.0f;
	if ( l->jukeboxLineT.box.h > rowH ) rowH = l->jukeboxLineT.box.h;
	if ( l->jukeboxTimeT.box.h > rowH ) rowH = l->jukeboxTimeT.box.h;
	rowH += 16.0f;
	l->jukeboxBox.x = MH_SIDE;
	l->jukeboxBox.w = 2.0f + 12.0f + 16.0f + 10.0f + l->jukeboxLineT.box.w + 10.0f + l->jukeboxTimeT.box.w + 12.0f;
	l->jukeboxBox.h = 2.0f + rowH + 2.0f;
	l->jukeboxBox.y = bottom - l->jukeboxBox.h;
	x = l->jukeboxBox.x + 1.0f + 12.0f;
	l->jukeboxIcon.x = x;
	l->jukeboxIcon.y = l->jukeboxBox.y + 1.0f + ( rowH - 16.0f ) * 0.5f;
	l->jukeboxIcon.w = l->jukeboxIcon.h = 16.0f;
	x += 16.0f + 10.0f;
	MH_Place( &l->jukeboxLineT, x, l->jukeboxBox.y + 1.0f + ( rowH - l->jukeboxLineT.box.h ) * 0.5f );
	x += l->jukeboxLineT.box.w + 10.0f;
	MH_Place( &l->jukeboxTimeT, x, l->jukeboxBox.y + 1.0f + ( rowH - l->jukeboxTimeT.box.h ) * 0.5f );
	l->jukeboxBar.x = l->jukeboxBox.x + 1.0f;
	l->jukeboxBar.y = l->jukeboxBox.y + 1.0f + rowH;
	l->jukeboxBar.w = l->jukeboxBox.w - 2.0f;
	l->jukeboxBar.h = 2.0f;
}

/* opened for a while after a change of track: a panel 300px wide */
static void MH_JukeboxOpen( mhRaceLayout_t *l, float bottom ) {
	wsTextStyle_t label, label11, mono12, text20, text14;
	float x = MH_SIDE + 1.0f + MH_PAD_X, w = 300.0f - 2.0f - 2.0f * MH_PAD_X, y, h;
	int i;

	MH_Label( &label, 12.0f );
	MH_Label( &label11, 11.0f );
	MH_Mono( &mono12, 12.0f );
	MH_Text( &text20, 20.0f );
	MH_Text( &text14, 14.0f );
	MH_Set( &l->jukeboxStateT, &label, l->jukeboxState );
	MH_Set( &l->jukeboxTimeT, &mono12, l->jukeboxTime );
	MH_Set( &l->jukeboxTitleT, &text20, l->jukeboxTitle );
	MH_Fit( &l->jukeboxTitleT, w );
	MH_Set( &l->jukeboxArtistT, &text14, l->jukeboxArtist );
	MH_Fit( &l->jukeboxArtistT, w );
	MH_Set( &l->jukeboxModesT, &label11, l->jukeboxLine );

	/* row, title and artist (gap 2), bar 3, buttons 34x30 with the modes */
	h = ( l->jukeboxStateT.box.h > l->jukeboxTimeT.box.h ? l->jukeboxStateT.box.h : l->jukeboxTimeT.box.h )
		+ 10.0f + l->jukeboxTitleT.box.h + 2.0f + l->jukeboxArtistT.box.h + 10.0f + 3.0f + 10.0f + 30.0f;
	l->jukeboxBox.x = MH_SIDE;
	l->jukeboxBox.w = 300.0f;
	l->jukeboxBox.h = MH_PanelHeight( h, qtrue, MH_PAD_Y );
	l->jukeboxBox.y = bottom - l->jukeboxBox.h;
	y = l->jukeboxBox.y + 2.0f + MH_PAD_Y;
	y += MH_BaselineRow( &l->jukeboxStateT, &l->jukeboxTimeT, x, y, w ) + 10.0f;
	/* the title and the artist stretch over the column */
	l->jukeboxTitleT.box.w = l->jukeboxArtistT.box.w = w;
	MH_Place( &l->jukeboxTitleT, x, y );
	y += l->jukeboxTitleT.box.h + 2.0f;
	MH_Place( &l->jukeboxArtistT, x, y );
	y += l->jukeboxArtistT.box.h + 10.0f;
	l->jukeboxBar.x = x;
	l->jukeboxBar.y = y;
	l->jukeboxBar.w = w;
	l->jukeboxBar.h = 3.0f;
	y += 3.0f + 10.0f;
	for ( i = 0; i < 3; i++ ) {
		l->jukeboxButton[i].x = x + i * ( 34.0f + 6.0f );
		l->jukeboxButton[i].y = y;
		l->jukeboxButton[i].w = 34.0f;
		l->jukeboxButton[i].h = 30.0f;
	}
	/* space-between with the three buttons (114px): no overlap when it is too long */
	{
		float modesX = x + w - l->jukeboxModesT.box.w;
		if ( modesX < x + 114.0f ) modesX = x + 114.0f;
		MH_Place( &l->jukeboxModesT, modesX, y + ( 30.0f - l->jukeboxModesT.box.h ) * 0.5f );
	}
}

/* -------------------------------------------------------------------------
   bottom right: minimap, slipstream, cockpit (gen.py minimap, slipstream, cockpit)
   ------------------------------------------------------------------------- */

static float MH_Cockpit( mhRaceLayout_t *l, float bottom ) {
	wsTextStyle_t big74, big40, label13, label11, label, mono13;
	wsRect_t in;
	float speedAbove, speedBelow, speedH, gearH, topH, y, colW, ry;

	MH_Big( &big74, 74.0f );
	MH_Big( &big40, 40.0f );
	MH_Label( &label13, 13.0f );
	MH_Label( &label11, 11.0f );
	MH_Label( &label, 12.0f );
	MH_Mono( &mono13, 13.0f );

	l->cockpit.w = MH_COCKPIT_W;
	l->cockpit.x = l->viewWidth - MH_SIDE - MH_COCKPIT_W;
	in.x = l->cockpit.x + 1.0f + MH_PAD_X;
	in.w = l->cockpit.w - 2.0f - 2.0f * MH_PAD_X;

	/* speed and unit on one baseline, gap 8px; the gear in a 58x52 box */
	MH_Set( &l->speedT, &big74, l->speed );
	MH_Set( &l->unitT, &label13, l->unit );
	speedAbove = MH_Base( &l->speedT ) > MH_Base( &l->unitT ) ? MH_Base( &l->speedT ) : MH_Base( &l->unitT );
	speedBelow = l->speedT.box.h - MH_Base( &l->speedT );
	if ( l->unitT.box.h - MH_Base( &l->unitT ) > speedBelow ) speedBelow = l->unitT.box.h - MH_Base( &l->unitT );
	speedH = speedAbove + speedBelow;
	MH_Set( &l->gearLabel, &label11, "GEAR" );
	MH_Set( &l->gearT, &big40, l->gear );
	gearH = l->gearLabel.box.h + 4.0f + 54.0f;
	topH = speedH > gearH ? speedH : gearH;

	l->cockpit.h = MH_PanelHeight( topH + 12.0f + 10.0f + 12.0f, qtrue, MH_PAD_Y );
	MH_Set( &l->nosLabel, &label, "NOS" );
	MH_Set( &l->nosValue, &mono13, l->nos );
	MH_Set( &l->fuelLabel, &label, "FUEL" );
	MH_Set( &l->fuelValue, &mono13, l->fuel );
	/* the gauges' rows, measured before the box is placed */
	{
		mhText_t a = l->nosLabel, b = l->nosValue;
		ry = MH_BaselineRow( &a, &b, 0.0f, 0.0f, 100.0f );
	}
	l->cockpit.h += ry + 6.0f + 4.0f;
	l->cockpit.y = bottom - l->cockpit.h;

	y = l->cockpit.y + 2.0f + MH_PAD_Y;
	MH_Place( &l->speedT, in.x, y + topH - speedH + speedAbove - MH_Base( &l->speedT ) );
	MH_Place( &l->unitT, in.x + l->speedT.box.w + 8.0f, y + topH - speedH + speedAbove - MH_Base( &l->unitT ) );
	l->gearBox.w = 60.0f;
	l->gearBox.h = 54.0f;
	l->gearBox.x = in.x + in.w - 60.0f;
	l->gearBox.y = y + topH - 54.0f;
	MH_Place( &l->gearLabel, l->gearBox.x + ( 60.0f - l->gearLabel.box.w ) * 0.5f, l->gearBox.y - 4.0f - l->gearLabel.box.h );
	MH_Place( &l->gearT, l->gearBox.x + 1.0f + ( 58.0f - l->gearT.box.w ) * 0.5f, l->gearBox.y + 1.0f + ( 52.0f - l->gearT.box.h ) * 0.5f );
	y += topH + 12.0f;

	l->rpm.x = in.x;
	l->rpm.y = y;
	l->rpm.w = in.w;
	l->rpm.h = 10.0f;
	y += 10.0f + 12.0f;

	/* two columns, gap 16px: label and value on a baseline, 6px, a 4px bar */
	colW = ( in.w - 16.0f ) * 0.5f;
	MH_BaselineRow( &l->nosLabel, &l->nosValue, in.x, y, colW );
	MH_BaselineRow( &l->fuelLabel, &l->fuelValue, in.x + colW + 16.0f, y, colW );
	l->nosBar.x = in.x;
	l->fuelBar.x = in.x + colW + 16.0f;
	l->nosBar.y = l->fuelBar.y = y + ry + 6.0f;
	l->nosBar.w = l->fuelBar.w = colW;
	l->nosBar.h = l->fuelBar.h = 4.0f;
	return l->cockpit.y;
}

static float MH_Slipstream( mhRaceLayout_t *l, float bottom ) {
	wsTextStyle_t label, mono13;
	float x, w, rowH;

	MH_Label( &label, 12.0f );
	MH_Mono( &mono13, 13.0f );
	MH_Set( &l->slipLabel, &label, "SLIPSTREAM" );
	MH_Set( &l->slipValueT, &mono13, l->slipValue );
	l->slip.w = MH_COCKPIT_W;
	l->slip.x = l->viewWidth - MH_SIDE - MH_COCKPIT_W;
	x = l->slip.x + 1.0f + MH_PAD_X;
	w = l->slip.w - 2.0f - 2.0f * MH_PAD_X;
	{
		mhText_t a = l->slipLabel, b = l->slipValueT;
		rowH = MH_BaselineRow( &a, &b, 0.0f, 0.0f, 100.0f );
	}
	/* padding 10px 16px, no accent */
	l->slip.h = MH_PanelHeight( rowH + 6.0f + 4.0f, qfalse, 10.0f );
	l->slip.y = bottom - l->slip.h;
	MH_BaselineRow( &l->slipLabel, &l->slipValueT, x, l->slip.y + 1.0f + 10.0f, w );
	l->slipBar.x = x;
	l->slipBar.y = l->slip.y + 1.0f + 10.0f + rowH + 6.0f;
	l->slipBar.w = w;
	l->slipBar.h = 4.0f;
	return l->slip.y;
}

static float MH_Minimap( mhRaceLayout_t *l, float bottom ) {
	/* padding 6px, border 1px, at the right of the column */
	float side = l->minimapSize + 14.0f;
	l->minimapBox.x = l->viewWidth - MH_SIDE - side;
	l->minimapBox.y = bottom - side;
	l->minimapBox.w = l->minimapBox.h = side;
	l->minimapView.x = l->minimapBox.x + 7.0f;
	l->minimapView.y = l->minimapBox.y + 7.0f;
	l->minimapView.w = l->minimapView.h = l->minimapSize;
	return l->minimapBox.y;
}

/* ------------------------------------------------------------------------- */

/* the kill feed (gen.py killfeed): under the standings 8px apart, each line
 * at the right edge, padding 4px 10px, the three parts 8px apart on one
 * baseline */
static void MH_KillFeed( mhRaceLayout_t *l, float y ) {
	wsTextStyle_t name, weapon;
	float right = l->viewWidth - MH_SIDE;
	int i;

	MH_Text( &name, 14.0f );
	MH_Mono( &weapon, 11.0f );
	for ( i = 0; i < l->killCount; i++ ) {
		mhText_t *a = &l->killAT[i], *w = &l->killWeaponT[i], *b = &l->killBT[i];
		qboolean target = l->killB[i] && l->killB[i][0];
		float above, below, x;
		MH_Set( a, &name, l->killA[i] );
		MH_Fit( a, 110.0f );
		a->box.w = MH_Width( a );
		MH_Set( w, &weapon, l->killWeapon[i] );
		MH_Set( b, &name, target ? l->killB[i] : "" );
		MH_Fit( b, 110.0f );
		b->box.w = MH_Width( b );
		above = MH_Base( a ) > MH_Base( w ) ? MH_Base( a ) : MH_Base( w );
		below = a->box.h - MH_Base( a ) > w->box.h - MH_Base( w ) ? a->box.h - MH_Base( a ) : w->box.h - MH_Base( w );
		l->killBox[i].w = 10.0f + a->box.w + 8.0f + w->box.w + ( target ? 8.0f + b->box.w : 0.0f ) + 10.0f;
		l->killBox[i].h = 4.0f + above + below + 4.0f;
		l->killBox[i].x = right - l->killBox[i].w;
		l->killBox[i].y = y;
		x = l->killBox[i].x + 10.0f;
		MH_Place( a, x, y + 4.0f + above - MH_Base( a ) );
		x += a->box.w + 8.0f;
		MH_Place( w, x, y + 4.0f + above - MH_Base( w ) );
		x += w->box.w + 8.0f;
		MH_Place( b, x, y + 4.0f + above - MH_Base( b ) );
		y += l->killBox[i].h + 8.0f;
	}
}

void MH_RaceLayout( mhRaceLayout_t *l ) {
	float bottom;

	MH_Card( l );
	if ( l->standings ) MH_Order( l );
	if ( l->killCount ) MH_KillFeed( l, l->standings ? l->order.y + l->order.h + 8.0f : MH_TOP );
	MH_TopCentre( l );

	bottom = MH_Vehicle( l, MH_DESIGN_HEIGHT - MH_BOTTOM ) - MH_COLUMN_GAP;
	if ( l->jukebox ) {
		if ( l->jukeboxOpen ) MH_JukeboxOpen( l, bottom );
		else MH_JukeboxClosed( l, bottom );
	}

	bottom = MH_Cockpit( l, MH_DESIGN_HEIGHT - MH_BOTTOM ) - MH_COLUMN_GAP;
	if ( l->slipstream ) bottom = MH_Slipstream( l, bottom ) - MH_COLUMN_GAP;
	if ( l->minimap ) MH_Minimap( l, bottom );
}

/* -------------------------------------------------------------------------
   big messages (gen.py bigmsg): a column centred on the screen, gap 6px
   ------------------------------------------------------------------------- */

void MH_MessageLayout( mhMessageLayout_t *m ) {
	wsTextStyle_t title, sub;
	float size = m->titleSize > 0.0f ? m->titleSize : 44.0f;
	float maxW = m->viewWidth - 2.0f * 96.0f, y;
	int i;

	if ( maxW > 960.0f ) maxW = 960.0f;
	/* a long centerprint: smaller, down to 28px, then cut */
	for ( ;; ) {
		MH_Big( &title, size );
		title.uppercase = qtrue;
		MH_Set( &m->titleT, &title, m->title );
		if ( m->titleT.box.w <= maxW || size <= 28.0f ) break;
		size -= 2.0f;
	}
	MH_Fit( &m->titleT, maxW );
	m->titleT.box.w = MH_Width( &m->titleT );
	y = m->top > 0.0f ? m->top : MH_MESSAGE_TOP;
	MH_Place( &m->titleT, ( m->viewWidth - m->titleT.box.w ) * 0.5f, y );
	y += m->titleT.box.h;

	MH_Label( &sub, size >= 56.0f ? 14.0f : 13.0f );
	for ( i = 0; i < m->subCount && i < MH_SUB_MAX; i++ ) {
		MH_Set( &m->subT[i], &sub, m->sub[i] );
		MH_Fit( &m->subT[i], maxW );
		m->subT[i].box.w = MH_Width( &m->subT[i] );
		MH_Place( &m->subT[i], ( m->viewWidth - m->subT[i].box.w ) * 0.5f, y + 6.0f );
		y += 6.0f + m->subT[i].box.h;
	}
}

/* -------------------------------------------------------------------------
   the spectator (gen.py spectator): a panel, at the left the label over the
   name, at the right two hints, the columns on their first baselines
   ------------------------------------------------------------------------- */

void MH_SpectatorLayout( mhSpectatorLayout_t *s ) {
	wsTextStyle_t label, name, hint;
	float x, y, w, leftH, rightH, above, h;

	MH_Label( &label, 12.0f );
	MH_Text( &name, 22.0f );
	MH_Label( &hint, 11.0f );
	MH_Set( &s->labelT, &label, s->label );
	MH_Set( &s->nameT, &name, s->name );
	MH_Set( &s->hint1T, &hint, s->hint1 );
	MH_Set( &s->hint2T, &hint, s->hint2 );

	s->box.w = MH_SPECTATOR_W;
	s->box.x = ( s->viewWidth - s->box.w ) * 0.5f;
	x = s->box.x + 1.0f + MH_PAD_X;
	w = s->box.w - 2.0f - 2.0f * MH_PAD_X;

	/* the right column takes what it needs, the name the rest (12px apart) */
	rightH = s->hint1T.box.w > s->hint2T.box.w ? s->hint1T.box.w : s->hint2T.box.w;
	MH_Fit( &s->nameT, w - rightH - 12.0f );
	s->nameT.box.w = MH_Width( &s->nameT );
	if ( s->labelT.box.w > s->nameT.box.w ) s->nameT.box.w = s->labelT.box.w;
	else s->labelT.box.w = s->nameT.box.w;		/* a column's blocks */

	leftH = s->labelT.box.h + 4.0f + s->nameT.box.h;
	rightH = s->hint1T.box.h + 4.0f + s->hint2T.box.h;
	above = MH_Base( &s->labelT ) > MH_Base( &s->hint1T ) ? MH_Base( &s->labelT ) : MH_Base( &s->hint1T );
	h = above - MH_Base( &s->labelT ) + leftH;
	if ( above - MH_Base( &s->hint1T ) + rightH > h ) h = above - MH_Base( &s->hint1T ) + rightH;

	s->box.h = 2.0f + MH_PAD_Y + h + MH_PAD_Y + 1.0f;
	s->box.y = MH_DESIGN_HEIGHT - MH_BOTTOM - s->box.h;
	y = s->box.y + 2.0f + MH_PAD_Y;
	MH_Place( &s->labelT, x, y + above - MH_Base( &s->labelT ) );
	MH_Place( &s->nameT, x, s->labelT.box.y + s->labelT.box.h + 4.0f );
	MH_Place( &s->hint1T, x + w - s->hint1T.box.w, y + above - MH_Base( &s->hint1T ) );
	MH_Place( &s->hint2T, x + w - s->hint2T.box.w, s->hint1T.box.y + s->hint1T.box.h + 4.0f );
}
