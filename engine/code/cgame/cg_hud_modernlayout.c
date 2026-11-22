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

	/* drag: two labels on a row */
	if ( l->labelRowLeft ) {
		MH_Set( &l->labelRowLeftT, &label, l->labelRowLeft );
		MH_Set( &l->labelRowRightT, &label, l->labelRowRight ? l->labelRowRight : "" );
		y += MH_BaselineRow( &l->labelRowLeftT, &l->labelRowRightT, in.x, y, in.w ) + 12.0f;
	}

	/* posline: "P3" "/ 6" on one baseline at the left, "LAP" over "2/5" at
	 * the right, both at the bottom of the row */
	if ( l->headerRow && l->pos ) {
		/* one row: the label at the left, the figure at the right, one baseline */
		wsTextStyle_t figure;
		MH_Big( &figure, l->posSize > 0.0f ? l->posSize : 30.0f );
		MH_Set( &l->posLabelT, &label, l->posLabel );
		MH_Set( &l->posT, &figure, l->pos );
		y += MH_BaselineRow( &l->posLabelT, &l->posT, in.x, y, in.w ) + 12.0f;
	} else if ( l->pos || l->lapValue ) {
		left = right = 0.0f;
		posAbove = posBelow = 0.0f;
		if ( l->pos && l->posLabel ) {
			/* label over the figure, like the lap at the right */
			wsTextStyle_t figure;
			MH_Big( &figure, l->posSize > 0.0f ? l->posSize : 64.0f );
			MH_Set( &l->posLabelT, &label, l->posLabel );
			MH_Set( &l->posT, &figure, l->pos );
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
			/* a column's blocks: as wide as the wider one */
			if ( l->posT.box.w > l->posLabelT.box.w ) l->posLabelT.box.w = l->posT.box.w;
			else l->posT.box.w = l->posLabelT.box.w;
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
	/* KOTH: the hill icon (34), 12px to its right a column 4px apart: the
	 * label over the owner (big 40), centred on each other */
	if ( l->hillOwner ) {
		wsTextStyle_t big40;
		float colH;
		MH_Big( &big40, 40.0f );
		MH_Set( &l->hillLabelT, &label, l->hillLabel );
		MH_Set( &l->hillOwnerT, &big40, l->hillOwner );
		colH = l->hillLabelT.box.h + 4.0f + l->hillOwnerT.box.h;
		l->hillIcon.x = in.x;
		l->hillIcon.y = y + ( colH - 34.0f ) * 0.5f;
		l->hillIcon.w = l->hillIcon.h = 34.0f;
		MH_Place( &l->hillLabelT, in.x + 34.0f + 12.0f, y );
		MH_Place( &l->hillOwnerT, in.x + 34.0f + 12.0f, y + l->hillLabelT.box.h + 4.0f );
		/* a column's blocks: as wide as the wider one */
		if ( l->hillOwnerT.box.w > l->hillLabelT.box.w ) l->hillLabelT.box.w = l->hillOwnerT.box.w;
		else l->hillOwnerT.box.w = l->hillLabelT.box.w;
		y += colH + 12.0f;
	}

	/* zones(): tiles 40px high, 4px apart, sharing the width; 12px below a
	 * column 6px apart: the row (label, mono 13), a 6px bar */
	if ( l->zoneCount > 0 || l->zoneLabel ) {
		wsTextStyle_t letter, mono13;
		float tileW = l->zoneCount > 0 ? ( in.w - 4.0f * ( l->zoneCount - 1 ) ) / l->zoneCount : 0.0f;
		MH_Big( &letter, 22.0f );
		MH_Mono( &mono13, 13.0f );
		for ( i = 0; i < l->zoneCount; i++ ) {
			l->zoneTile[i].x = in.x + i * ( tileW + 4.0f );
			l->zoneTile[i].y = y;
			l->zoneTile[i].w = tileW;
			l->zoneTile[i].h = 40.0f;
			MH_Set( &l->zoneNameT[i], &letter, l->zoneName[i] );
			MH_Place( &l->zoneNameT[i], l->zoneTile[i].x + ( tileW - l->zoneNameT[i].box.w ) * 0.5f,
			          y + ( 40.0f - l->zoneNameT[i].box.h ) * 0.5f );
		}
		if ( l->zoneCount > 0 ) y += 40.0f + 12.0f;
		MH_Set( &l->zoneLabelT, &label, l->zoneLabel ? l->zoneLabel : "" );
		MH_Set( &l->zoneValueT, &mono13, l->zoneValue ? l->zoneValue : "" );
		y += MH_BaselineRow( &l->zoneLabelT, &l->zoneValueT, in.x, y, in.w ) + 6.0f;
		l->zoneBar.x = in.x;
		l->zoneBar.y = y;
		l->zoneBar.w = in.w;
		l->zoneBar.h = 6.0f;
		y += 6.0f + 12.0f;
	}
	if ( l->pos || l->lapValue || l->progress || l->zoneCount || l->zoneLabel || l->labelRowLeft ) {
		l->sep.x = in.x;
		l->sep.y = y;
		l->sep.w = in.w;
		l->sep.h = 1.0f;
		y += 1.0f + 12.0f;
	}

	/* CTF: a line per flag, 8px apart: icon 18, team 60px, state, note at the
	 * right; then a second rule */
	if ( l->flagCount > 0 ) {
		wsTextStyle_t name, state, note;
		MH_Text( &name, 15.0f );
		MH_Mono( &state, 13.0f );
		MH_Mono( &note, 12.0f );
		for ( i = 0; i < l->flagCount; i++ ) {
			float rowH;
			MH_Set( &l->flagNameT[i], &name, l->flagName[i] );
			MH_Set( &l->flagStateT[i], &state, l->flagState[i] );
			MH_Set( &l->flagNoteT[i], &note, l->flagNote[i] ? l->flagNote[i] : "" );
			MH_Fit( &l->flagNoteT[i], 90.0f );
			l->flagNoteT[i].box.w = MH_Width( &l->flagNoteT[i] );
			rowH = l->flagNameT[i].box.h > 18.0f ? l->flagNameT[i].box.h : 18.0f;
			l->flagIcon[i].x = in.x;
			l->flagIcon[i].y = y + ( rowH - 18.0f ) * 0.5f;
			l->flagIcon[i].w = l->flagIcon[i].h = 18.0f;
			l->flagNameT[i].box.w = 60.0f;
			MH_Place( &l->flagNameT[i], in.x + 28.0f, y + ( rowH - l->flagNameT[i].box.h ) * 0.5f );
			l->flagStateT[i].box.w = in.w - 98.0f - 10.0f - l->flagNoteT[i].box.w;
			MH_Place( &l->flagStateT[i], in.x + 98.0f, y + ( rowH - l->flagStateT[i].box.h ) * 0.5f );
			MH_Place( &l->flagNoteT[i], in.x + in.w - l->flagNoteT[i].box.w, y + ( rowH - l->flagNoteT[i].box.h ) * 0.5f );
			y += rowH + ( i < l->flagCount - 1 ? 8.0f : 12.0f );
		}
		l->sep2.x = in.x;
		l->sep2.y = y;
		l->sep2.w = in.w;
		l->sep2.h = 1.0f;
		y += 1.0f + 12.0f;
	}

	/* derby: the car, and 18px to its right a column 10px apart, centred on it */
	if ( l->derbyStatus ) {
		static const char *legend[3] = { "OK", "WORN", "CRITICAL" };
		wsTextStyle_t small;
		float x = in.x + 110.0f + 18.0f, colH, cy;
		MH_Label( &small, 11.0f );
		l->derbyCar.x = in.x;
		l->derbyCar.y = y;
		l->derbyCar.w = 110.0f;
		l->derbyCar.h = 170.0f;
		MH_Set( &l->derbyStatusT, &label, l->derbyStatus );
		colH = l->derbyStatusT.box.h;
		for ( i = 0; i < 3; i++ ) {
			MH_Set( &l->derbyLegendT[i], &small, legend[i] );
			colH += 10.0f + l->derbyLegendT[i].box.h;
		}
		cy = y + ( 170.0f - colH ) * 0.5f;
		MH_Place( &l->derbyStatusT, x, cy );
		cy += l->derbyStatusT.box.h + 10.0f;
		for ( i = 0; i < 3; i++ ) {
			l->derbySwatch[i].x = x;
			l->derbySwatch[i].y = cy + ( l->derbyLegendT[i].box.h - 10.0f ) * 0.5f;
			l->derbySwatch[i].w = l->derbySwatch[i].h = 10.0f;
			MH_Place( &l->derbyLegendT[i], x + 10.0f + 8.0f, cy );
			cy += l->derbyLegendT[i].box.h + 10.0f;
		}
		y += 170.0f + 12.0f;
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

void MH_ZoneMarkerLayout( mhZoneMarker_t *m ) {
	wsTextStyle_t letter, mono12;
	MH_Big( &letter, 18.0f );
	MH_Mono( &mono12, 12.0f );
	MH_Set( &m->letterT, &letter, m->letter );
	/* a letter on a square; a word ("BALL") gets 6px either side */
	m->tile.w = m->letterT.box.w + 12.0f > MH_MARKER_SIZE ? m->letterT.box.w + 12.0f : MH_MARKER_SIZE;
	m->tile.h = MH_MARKER_SIZE;
	m->tile.x = m->x - m->tile.w * 0.5f;
	m->tile.y = m->y - MH_MARKER_SIZE * 0.5f;
	MH_Place( &m->letterT, m->x - m->letterT.box.w * 0.5f, m->y - m->letterT.box.h * 0.5f );
	MH_Set( &m->distanceT, &mono12, m->distance );
	MH_Place( &m->distanceT, m->x - m->distanceT.box.w * 0.5f, m->tile.y + MH_MARKER_SIZE + 4.0f );
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
		if ( l->rowBars ) {
			/* name 92px, the bar, the value in a 40px column, all centred */
			wsTextStyle_t mono12;
			MH_Mono( &mono12, 12.0f );
			MH_Set( &l->rowNameT[i], &text15, l->rowName[i] );
			MH_Fit( &l->rowNameT[i], 92.0f );
			l->rowNameT[i].box.w = 92.0f;
			MH_Place( &l->rowNameT[i], x, r->y + ( r->h - l->rowNameT[i].box.h ) * 0.5f );
			MH_Set( &l->rowValueT[i], &mono12, l->rowValue[i] );
			l->rowValueT[i].box.w = 40.0f;	/* the column; the text sits at its right */
			MH_Place( &l->rowValueT[i], r->x + r->w - 8.0f - 40.0f, r->y + ( r->h - l->rowValueT[i].box.h ) * 0.5f );
			l->rowBar[i].x = x + 92.0f + 10.0f;
			l->rowBar[i].w = l->rowValueT[i].box.x - 10.0f - l->rowBar[i].x;
			l->rowBar[i].h = 4.0f;
			l->rowBar[i].y = r->y + ( r->h - 4.0f ) * 0.5f;
			y += r->h + 2.0f;
			continue;
		}
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

	/* scorebug(): a 1px border around three cells 52 high, 92 120 92 wide;
	 * label and score side by side 10px apart, the clock over the line 2px
	 * apart, each centred in its cell */
	if ( l->scorebug ) {
		wsTextStyle_t label11, label10, big34, mono22;
		float cx[3] = { 92.0f, 120.0f, 92.0f }, cy = y + 1.0f + 26.0f, gx, colH;
		int i;
		MH_Label( &label11, 11.0f );
		MH_Label( &label10, 10.0f );
		MH_Big( &big34, 34.0f );
		MH_Mono( &mono22, 22.0f );
		l->bugBox.w = 92.0f + 120.0f + 92.0f + 2.0f;
		l->bugBox.h = 54.0f;
		l->bugBox.x = ( l->viewWidth - l->bugBox.w ) * 0.5f;
		l->bugBox.y = y;
		x = l->bugBox.x + 1.0f;
		for ( i = 0; i < 3; i++ ) {
			l->bugCell[i].x = x;
			l->bugCell[i].y = y + 1.0f;
			l->bugCell[i].w = cx[i];
			l->bugCell[i].h = 52.0f;
			x += cx[i];
		}
		MH_Set( &l->bugLeftLabelT, &label11, l->bugLeftLabel );
		MH_Set( &l->bugLeftT, &big34, l->bugLeft );
		gx = l->bugCell[0].x + ( 92.0f - l->bugLeftLabelT.box.w - 10.0f - l->bugLeftT.box.w ) * 0.5f;
		MH_Place( &l->bugLeftLabelT, gx, cy - l->bugLeftLabelT.box.h * 0.5f );
		MH_Place( &l->bugLeftT, gx + l->bugLeftLabelT.box.w + 10.0f, cy - l->bugLeftT.box.h * 0.5f );
		MH_Set( &l->bugRightT, &big34, l->bugRight );
		MH_Set( &l->bugRightLabelT, &label11, l->bugRightLabel );
		gx = l->bugCell[2].x + ( 92.0f - l->bugRightT.box.w - 10.0f - l->bugRightLabelT.box.w ) * 0.5f;
		MH_Place( &l->bugRightT, gx, cy - l->bugRightT.box.h * 0.5f );
		MH_Place( &l->bugRightLabelT, gx + l->bugRightT.box.w + 10.0f, cy - l->bugRightLabelT.box.h * 0.5f );
		MH_Set( &l->bugClockT, &mono22, l->bugClock );
		MH_Set( &l->bugSubT, &label10, l->bugSub ? l->bugSub : "" );
		colH = l->bugClockT.box.h + ( l->bugSubT.text[0] ? 2.0f + l->bugSubT.box.h : 0.0f );
		MH_Place( &l->bugClockT, l->bugCell[1].x + ( 120.0f - l->bugClockT.box.w ) * 0.5f, cy - colH * 0.5f );
		MH_Place( &l->bugSubT, l->bugCell[1].x + ( 120.0f - l->bugSubT.box.w ) * 0.5f,
		          cy - colH * 0.5f + l->bugClockT.box.h + 2.0f );
		y += 54.0f + 12.0f;
	}
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

/* drag: the tree under the standings (gen.py tree): the label, 12px, rows
 * 8px apart of two 20px lamps 10px apart and a label 10px on */
static void MH_Tree( mhRaceLayout_t *l, float top ) {
	wsTextStyle_t label, label11;
	float x, y;
	int i;

	MH_Label( &label, 12.0f );
	MH_Label( &label11, 11.0f );
	l->treeBox.x = l->viewWidth - MH_SIDE - MH_ORDER_W;
	l->treeBox.y = top;
	l->treeBox.w = MH_ORDER_W;
	x = l->treeBox.x + 1.0f + MH_PAD_X;
	y = top + 2.0f + MH_PAD_Y;
	MH_Set( &l->treeTitleT, &label, "TREE" );
	MH_Place( &l->treeTitleT, x, y );
	l->treeTitleT.box.w = l->treeBox.w - 2.0f - 2.0f * MH_PAD_X;	/* a block over the panel's width */
	y += l->treeTitleT.box.h + 12.0f;
	for ( i = 0; i < MH_TREE_ROWS; i++ ) {
		l->treeLamp[i][0].x = x;
		l->treeLamp[i][1].x = x + 30.0f;
		l->treeLamp[i][0].y = l->treeLamp[i][1].y = y;
		l->treeLamp[i][0].w = l->treeLamp[i][0].h = l->treeLamp[i][1].w = l->treeLamp[i][1].h = 20.0f;
		MH_Set( &l->treeLabelT[i], &label11, l->treeLabel[i] ? l->treeLabel[i] : "" );
		MH_Place( &l->treeLabelT[i], x + 60.0f, y + ( 20.0f - l->treeLabelT[i].box.h ) * 0.5f );
		y += 20.0f + ( i < MH_TREE_ROWS - 1 ? 8.0f : 0.0f );
	}
	l->treeBox.h = y + MH_PAD_Y + 1.0f - top;
}

/* drag: the tachometer across the bottom, 400px in from either side
 * (gen.py drag_tach): RPM over the figure at the left, the launch window in
 * the middle, gear and speed at the right, all on the bottom line; then 40
 * segments 3px apart, 26 high; then the two ends of the scale */
static void MH_Tach( mhRaceLayout_t *l ) {
	wsTextStyle_t label, label11, big44, mono15;
	float x, w, y, top, rowH, rightX, segW;
	int i;

	MH_Label( &label, 12.0f );
	MH_Label( &label11, 11.0f );
	MH_Big( &big44, 44.0f );
	MH_Mono( &mono15, 15.0f );
	l->tachBox.x = 400.0f;
	l->tachBox.w = l->viewWidth - 800.0f;
	x = l->tachBox.x + 1.0f + MH_PAD_X;
	w = l->tachBox.w - 2.0f - 2.0f * MH_PAD_X;

	MH_Set( &l->tachRpmLabelT, &label, "RPM" );
	MH_Set( &l->tachRpmT, &big44, l->tachRpm );
	MH_Set( &l->tachGearLabelT, &label, "GEAR" );
	MH_Set( &l->tachGearT, &big44, l->tachGear );
	MH_Set( &l->tachUnitT, &label, l->tachUnit );
	MH_Set( &l->tachSpeedT, &big44, l->tachSpeed );
	MH_Set( &l->tachMidLabelT, &label, l->tachMidLabel ? l->tachMidLabel : "" );
	MH_Set( &l->tachMidValueT, &mono15, l->tachMidValue ? l->tachMidValue : "" );
	rowH = l->tachRpmLabelT.box.h + 4.0f + l->tachRpmT.box.h;
	MH_Set( &l->tachLeftT, &label11, l->tachLeft ? l->tachLeft : "" );
	MH_Set( &l->tachRightT, &label11, l->tachRight ? l->tachRight : "" );

	/* bottom up: the panel ends MH_BOTTOM above the screen's edge */
	l->tachBox.h = 2.0f + MH_PAD_Y + rowH + 12.0f + 26.0f + 12.0f + l->tachLeftT.box.h + MH_PAD_Y + 1.0f;
	l->tachBox.y = MH_DESIGN_HEIGHT - MH_BOTTOM - l->tachBox.h;
	top = y = l->tachBox.y + 2.0f + MH_PAD_Y;

	/* the left column: as wide as the wider block */
	MH_Place( &l->tachRpmLabelT, x, y );
	MH_Place( &l->tachRpmT, x, y + l->tachRpmLabelT.box.h + 4.0f );
	if ( l->tachRpmT.box.w > l->tachRpmLabelT.box.w ) l->tachRpmLabelT.box.w = l->tachRpmT.box.w;
	else l->tachRpmT.box.w = l->tachRpmLabelT.box.w;
	/* the right: speed right-aligned, gear centred over its figure 22px before */
	{
		float speedW = l->tachSpeedT.box.w > l->tachUnitT.box.w ? l->tachSpeedT.box.w : l->tachUnitT.box.w;
		float gearW = l->tachGearT.box.w > l->tachGearLabelT.box.w ? l->tachGearT.box.w : l->tachGearLabelT.box.w;
		float sx = x + w - speedW, gx = sx - 22.0f - gearW;
		MH_Place( &l->tachUnitT, x + w - l->tachUnitT.box.w, y );
		MH_Place( &l->tachSpeedT, x + w - l->tachSpeedT.box.w, y + l->tachUnitT.box.h + 4.0f );
		l->tachUnitT.box.x = sx;	/* the labels' blocks: the column's width */
		l->tachUnitT.box.w = speedW;
		MH_Place( &l->tachGearLabelT, gx + ( gearW - l->tachGearLabelT.box.w ) * 0.5f, y );
		MH_Place( &l->tachGearT, gx + ( gearW - l->tachGearT.box.w ) * 0.5f, y + l->tachGearLabelT.box.h + 4.0f );
		l->tachGearLabelT.box.x = gx;
		l->tachGearLabelT.box.w = gearW;
		rightX = gx;
	}
	/* the middle: between the two sides, at the bottom of the row */
	{
		float midW = l->tachMidLabelT.box.w > l->tachMidValueT.box.w ? l->tachMidLabelT.box.w : l->tachMidValueT.box.w;
		float mx = ( x + l->tachRpmT.box.w + rightX ) * 0.5f;
		float midH = l->tachMidLabelT.box.h + 4.0f + l->tachMidValueT.box.h;
		MH_Place( &l->tachMidLabelT, mx - l->tachMidLabelT.box.w * 0.5f, y + rowH - midH );
		MH_Place( &l->tachMidValueT, mx - l->tachMidValueT.box.w * 0.5f, y + rowH - l->tachMidValueT.box.h );
		l->tachMidLabelT.box.x = mx - midW * 0.5f;
		l->tachMidLabelT.box.w = midW;
	}
	y = top + rowH + 12.0f;
	segW = ( w - 3.0f * ( MH_TACH_SEGMENTS - 1 ) ) / MH_TACH_SEGMENTS;
	for ( i = 0; i < MH_TACH_SEGMENTS; i++ ) {
		l->tachSeg[i].x = x + i * ( segW + 3.0f );
		l->tachSeg[i].y = y;
		l->tachSeg[i].w = segW;
		l->tachSeg[i].h = 26.0f;
	}
	y += 26.0f + 12.0f;
	y += MH_BaselineRow( &l->tachLeftT, &l->tachRightT, x, y, w );
}

void MH_RaceLayout( mhRaceLayout_t *l ) {
	float bottom;

	MH_Card( l );
	if ( l->standings ) MH_Order( l );
	if ( l->tree ) MH_Tree( l, l->standings ? l->order.y + l->order.h + MH_COLUMN_GAP : MH_TOP );
	if ( l->killCount ) MH_KillFeed( l, l->standings ? l->order.y + l->order.h + 8.0f : MH_TOP );
	MH_TopCentre( l );

	bottom = MH_Vehicle( l, MH_DESIGN_HEIGHT - MH_BOTTOM ) - MH_COLUMN_GAP;
	if ( l->jukebox ) {
		if ( l->jukeboxOpen ) MH_JukeboxOpen( l, bottom );
		else MH_JukeboxClosed( l, bottom );
	}

	if ( l->tach ) {
		MH_Tach( l );	/* in place of minimap, slipstream and cockpit */
		return;
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
	s->box.y = ( s->bottom > 0.0f ? s->bottom : MH_DESIGN_HEIGHT - MH_BOTTOM ) - s->box.h;
	y = s->box.y + 2.0f + MH_PAD_Y;
	MH_Place( &s->labelT, x, y + above - MH_Base( &s->labelT ) );
	MH_Place( &s->nameT, x, s->labelT.box.y + s->labelT.box.h + 4.0f );
	MH_Place( &s->hint1T, x + w - s->hint1T.box.w, y + above - MH_Base( &s->hint1T ) );
	MH_Place( &s->hint2T, x + w - s->hint2T.box.w, s->hint1T.box.y + s->hint1T.box.h + 4.0f );
}

/* -------------------------------------------------------------------------
   the end screen of a race (gen.py EndRacing)
   ------------------------------------------------------------------------- */

#define MH_END_MARGIN		56.0f
#define MH_END_TOP			92.0f
#define MH_END_LEFT_W		380.0f
#define MH_END_PODIUM_X		476.0f
#define MH_END_PODIUM_W		600.0f
#define MH_END_PODIUM_H		410.0f
#define MH_END_LIST_W		420.0f

/* a label over a figure: the column as wide as w */
static float MH_EndStat( mhText_t *label, mhText_t *value, mhText_t *unit, float x, float y, float w ) {
	MH_Place( label, x, y );
	label->box.w = w;
	MH_Place( value, x, y + label->box.h + 4.0f );
	if ( unit->text[0] ) MH_Place( unit, x + MH_Width( value ) + 6.0f, value->box.y + MH_Base( value ) - MH_Base( unit ) );
	value->box.w = w;
	return label->box.h + 4.0f + value->box.h;
}

/* the left column: the place, the time, four figures, your laps */
static void MH_EndLeft( mhEndLayout_t *e ) {
	wsTextStyle_t big168, big48, big44, big30, label12, label11, label10, mono11;
	float x = MH_END_MARGIN, y = e->top + MH_END_TOP, w = MH_END_LEFT_W, h, slot;
	int i;

	MH_Big( &big168, 168.0f );
	MH_Big( &big48, 48.0f );
	MH_Big( &big44, 44.0f );
	MH_Big( &big30, 30.0f );
	MH_Label( &label12, 12.0f );
	MH_Label( &label11, 11.0f );
	MH_Label( &label10, 10.0f );
	MH_Mono( &mono11, 11.0f );

	/* "P2" and "/ 8" on one baseline, 14px apart */
	MH_Set( &e->posT, &big168, e->pos );
	MH_Set( &e->ofT, &big48, e->of ? e->of : "" );
	MH_Place( &e->posT, x, y );
	MH_Place( &e->ofT, x + e->posT.box.w + 14.0f, y + MH_Base( &e->posT ) - MH_Base( &e->ofT ) );
	y += e->posT.box.h + 22.0f - 18.0f;

	/* finished: label, time, note, 6px apart, blocks over the column */
	MH_Set( &e->doneLabelT, &label12, e->doneLabel );
	MH_Set( &e->doneTimeT, &big44, e->doneTime );
	MH_Set( &e->doneNoteT, &label12, e->doneNote ? e->doneNote : "" );
	MH_Place( &e->doneLabelT, x, y );
	MH_Place( &e->doneTimeT, x, y + e->doneLabelT.box.h + 6.0f );
	MH_Place( &e->doneNoteT, x, e->doneTimeT.box.y + e->doneTimeT.box.h + 6.0f );
	e->doneLabelT.box.w = e->doneTimeT.box.w = e->doneNoteT.box.w = w;
	y = e->doneNoteT.box.y + e->doneNoteT.box.h + 22.0f;

	e->sep[0].x = x;
	e->sep[0].y = y;
	e->sep[0].w = w;
	e->sep[0].h = 1.0f;
	y += 1.0f + 22.0f;

	/* the figures: two columns 24px apart, rows 18px apart */
	h = 0.0f;
	for ( i = 0; i < e->statCount && i < MH_END_STATS; i++ ) {
		float cw = ( w - 24.0f ) * 0.5f, cx = x + ( i & 1 ) * ( cw + 24.0f ), ch;
		MH_Set( &e->statLabelT[i], &label11, e->statLabel[i] );
		MH_Set( &e->statValueT[i], &big30, e->statValue[i] );
		MH_Set( &e->statUnitT[i], &label11, e->statUnit[i] ? e->statUnit[i] : "" );
		ch = MH_EndStat( &e->statLabelT[i], &e->statValueT[i], &e->statUnitT[i], cx, y, cw );
		if ( ch > h ) h = ch;
		if ( ( i & 1 ) || i == e->statCount - 1 ) {
			y += h + ( i < e->statCount - 1 ? 18.0f : 0.0f );
			h = 0.0f;
		}
	}
	y += 22.0f;
	e->sep[1].x = x;
	e->sep[1].y = y;
	e->sep[1].w = w;
	e->sep[1].h = 1.0f;
	y += 1.0f + 22.0f;

	/* your laps: the row, 12px, the chart: slots 8px apart, a 56px bar
	 * area, 6px, the value, 6px, the lap */
	MH_Set( &e->lapsTitleT, &label12, e->chartTitle ? e->chartTitle : "YOUR LAPS" );
	MH_Set( &e->lapsNoteT, &label11, e->chartNote ? e->chartNote : "LOST TO YOUR BEST" );
	y += MH_BaselineRow( &e->lapsTitleT, &e->lapsNoteT, x, y, w ) + 12.0f;

	/* team races: a row a team 20 high, 10 apart: the square (in 22), the
	 * axis, the gap (62 wide), 10px apart */
	for ( i = 0; i < e->spreadCount && i < MH_END_TEAMS; i++ ) {
		wsTextStyle_t mono12;
		float tx = x + 22.0f + 10.0f, tw = w - 22.0f - 62.0f - 20.0f;
		int k;
		MH_Mono( &mono12, 12.0f );
		e->spreadSwatch[i].x = x;
		e->spreadSwatch[i].y = y + 5.0f;
		e->spreadSwatch[i].w = e->spreadSwatch[i].h = 10.0f;
		e->spreadTrack[i].x = tx;
		e->spreadTrack[i].y = y + 9.0f;
		e->spreadTrack[i].w = tw;
		e->spreadTrack[i].h = 2.0f;
		for ( k = 0; k < e->spreadDots[i] && k < MH_END_DOTS; k++ ) {
			e->spreadDotR[i][k].x = tx + e->spreadDot[i][k] * tw - 5.0f;
			e->spreadDotR[i][k].y = y + 5.0f;
			e->spreadDotR[i][k].w = e->spreadDotR[i][k].h = 10.0f;
		}
		e->spreadTickR[i].x = tx + e->spreadTick[i] * tw - 1.0f;
		e->spreadTickR[i].y = y;
		e->spreadTickR[i].w = 2.0f;
		e->spreadTickR[i].h = 20.0f;
		MH_Set( &e->spreadValueT[i], &mono12, e->spreadValue[i] ? e->spreadValue[i] : "" );
		MH_Place( &e->spreadValueT[i], x + w - 62.0f, y + ( 20.0f - e->spreadValueT[i].box.h ) * 0.5f );
		e->spreadValueT[i].box.w = 62.0f;	/* at the right of it */
		y += 20.0f + 10.0f;
	}
	if ( e->lapCount <= 0 ) return;
	slot = ( w - 8.0f * ( e->lapCount - 1 ) ) / e->lapCount;
	for ( i = 0; i < e->lapCount && i < MH_END_LAPS; i++ ) {
		float sx = x + i * ( slot + 8.0f ), bh = 56.0f * e->lapFrac[i];
		if ( bh < 3.0f ) bh = 3.0f;
		e->lapSlot[i].x = sx;
		e->lapSlot[i].y = y;
		e->lapSlot[i].w = slot;
		e->lapBar[i].x = sx;
		e->lapBar[i].w = slot;
		e->lapBar[i].h = bh;
		e->lapBar[i].y = y + 56.0f - bh;
		MH_Set( &e->lapValueT[i], &mono11, e->lapValue[i] );
		MH_Set( &e->lapNameT[i], &label10, e->lapName[i] );
		MH_Place( &e->lapValueT[i], sx + ( slot - e->lapValueT[i].box.w ) * 0.5f, y + 56.0f + 6.0f );
		MH_Place( &e->lapNameT[i], sx + ( slot - e->lapNameT[i].box.w ) * 0.5f,
		          e->lapValueT[i].box.y + e->lapValueT[i].box.h + 6.0f );
		e->lapSlot[i].h = e->lapNameT[i].box.y + e->lapNameT[i].box.h - y;
	}
}

/* a card on the podium: [place - 1]; the car's view carH high; cards stand
 * on one line */
static void MH_EndCard( mhEndLayout_t *e, int k, float x, float w, float carH, float bottom ) {
	wsTextStyle_t big, label11, text20, mono15, mono13;
	float h, inX, inW, rowH, colH, y;

	MH_Big( &big, k == 0 ? 64.0f : 52.0f );
	MH_Label( &label11, 11.0f );
	MH_Text( &text20, 20.0f );
	MH_Mono( &mono15, 15.0f );
	MH_Mono( &mono13, 13.0f );
	MH_Set( &e->podiumPlaceT[k], &big, k == 0 ? "1" : k == 1 ? "2" : "3" );
	MH_Set( &e->podiumTagT[k], &label11, e->podiumTag[k] );
	MH_Set( &e->podiumYouT[k], &label11, e->podiumMe[k] ? ( e->podiumYou ? e->podiumYou : "YOU" ) : "" );
	MH_Set( &e->podiumNameT[k], &text20, e->podiumName[k] );
	MH_Set( &e->podiumTimeT[k], &mono15, e->podiumTime[k] );
	MH_Set( &e->podiumGapT[k], &mono13, e->podiumGap[k] ? e->podiumGap[k] : "" );
	colH = e->podiumTagT[k].box.h + ( e->podiumMe[k] ? 4.0f + e->podiumYouT[k].box.h : 0.0f );
	rowH = e->podiumPlaceT[k].box.h > colH ? e->podiumPlaceT[k].box.h : colH;
	h = 2.0f + carH + 12.0f + rowH + 6.0f + e->podiumNameT[k].box.h + 6.0f + e->podiumTimeT[k].box.h + 14.0f + 1.0f;
	if ( e->teamPodium ) {
		/* a 3px top; a rule 2px either side, then the drivers 3px apart */
		int m, n = e->podiumMembers[k] < MH_END_MEMBERS ? e->podiumMembers[k] : MH_END_MEMBERS;
		wsTextStyle_t mono12, text14;
		MH_Mono( &mono12, 12.0f );
		MH_Text( &text14, 14.0f );
		h += 1.0f;
		for ( m = 0; m < n; m++ ) {
			MH_Set( &e->podiumMemberPosT[k][m], &mono12, e->podiumMemberPos[k][m] );
			MH_Set( &e->podiumMemberNameT[k][m], &text14, e->podiumMemberName[k][m] );
		}
		if ( n > 0 ) h += 6.0f + 2.0f + 1.0f + 2.0f + 6.0f + n * e->podiumMemberNameT[k][0].box.h + ( n - 1 ) * 3.0f;
	}

	e->podium[k].x = x;
	e->podium[k].w = w;
	e->podium[k].h = h;
	e->podium[k].y = bottom - h;
	e->podiumCar[k].x = x + 1.0f;
	e->podiumCar[k].y = e->podium[k].y + ( e->teamPodium ? 3.0f : 2.0f );
	e->podiumCar[k].w = w - 2.0f;
	e->podiumCar[k].h = carH;
	inX = x + 1.0f + 14.0f;
	inW = w - 2.0f - 28.0f;
	y = e->podiumCar[k].y + carH + 12.0f;
	/* the place at the left, YOU over the tag at the right, on the row's bottom */
	MH_Place( &e->podiumPlaceT[k], inX, y + rowH - e->podiumPlaceT[k].box.h );
	MH_Place( &e->podiumTagT[k], inX + inW - e->podiumTagT[k].box.w, y + rowH - e->podiumTagT[k].box.h );
	MH_Place( &e->podiumYouT[k], inX + inW - e->podiumYouT[k].box.w, e->podiumTagT[k].box.y - 4.0f - e->podiumYouT[k].box.h );
	y += rowH + 6.0f;
	MH_Fit( &e->podiumNameT[k], inW );
	MH_Place( &e->podiumNameT[k], inX, y );
	e->podiumNameT[k].box.w = inW;
	y += e->podiumNameT[k].box.h + 6.0f;
	y += MH_BaselineRow( &e->podiumTimeT[k], &e->podiumGapT[k], inX, y, inW );
	if ( e->teamPodium && e->podiumMembers[k] > 0 ) {
		int m, n = e->podiumMembers[k] < MH_END_MEMBERS ? e->podiumMembers[k] : MH_END_MEMBERS;
		y += 6.0f + 2.0f;
		e->podiumRule[k].x = inX;
		e->podiumRule[k].y = y;
		e->podiumRule[k].w = inW;
		e->podiumRule[k].h = 1.0f;
		y += 1.0f + 2.0f + 6.0f;
		for ( m = 0; m < n; m++ ) {
			mhText_t *pos = &e->podiumMemberPosT[k][m], *name = &e->podiumMemberNameT[k][m];
			MH_Place( name, inX + pos->box.w + 6.0f, y );
			MH_Fit( name, inW - pos->box.w - 6.0f );
			MH_Place( pos, inX, y + MH_Base( name ) - MH_Base( pos ) );
			y += name->box.h + 3.0f;
		}
	}
}

/* the middle: the podium (second, winner, third, 12px apart, on one line),
 * 28px under it the highlights */
static void MH_EndMiddle( mhEndLayout_t *e ) {
	wsTextStyle_t label12, label11, text16, mono15;
	float x = MH_END_PODIUM_X + ( e->viewWidth - 1600.0f ) * 0.5f, top = e->top + MH_END_TOP;
	float bottom = top + MH_END_PODIUM_H, y;
	int i;

	/* the cars: lower on a team's card, the drivers stand under them */
	if ( e->podiumCount >= 2 ) MH_EndCard( e, 1, x, 192.0f, e->teamPodium ? 128.0f : 176.0f, bottom );
	if ( e->podiumCount >= 1 ) MH_EndCard( e, 0, x + 192.0f + 12.0f, 210.0f, e->teamPodium ? 172.0f : 236.0f, bottom );
	if ( e->podiumCount >= 3 ) MH_EndCard( e, 2, x + 192.0f + 12.0f + 210.0f + 12.0f, 192.0f, e->teamPodium ? 92.0f : 140.0f, bottom );

	MH_Label( &label12, 12.0f );
	MH_Label( &label11, 11.0f );
	MH_Text( &text16, 16.0f );
	MH_Mono( &mono15, 15.0f );
	y = bottom + 28.0f;
	if ( e->awardCount <= 0 ) return;
	MH_Set( &e->awardsTitleT, &label12, "HIGHLIGHTS" );
	MH_Place( &e->awardsTitleT, x, y );
	e->awardsTitleT.box.w = MH_END_PODIUM_W;
	y += e->awardsTitleT.box.h + 10.0f;
	for ( i = 0; i < e->awardCount && i < MH_END_AWARDS; i++ ) {
		float colH, cx;
		MH_Set( &e->awardLabelT[i], &label11, e->awardLabel[i] );
		MH_Set( &e->awardNameT[i], &text16, e->awardName[i] );
		MH_Set( &e->awardValueT[i], &mono15, e->awardValue[i] );
		colH = e->awardLabelT[i].box.h + 4.0f + e->awardNameT[i].box.h;
		e->award[i].x = x;
		e->award[i].y = y;
		e->award[i].w = MH_END_PODIUM_W;
		e->award[i].h = 1.0f + 12.0f + colH + 12.0f + 1.0f;
		e->awardIcon[i].x = x + 3.0f + 14.0f;
		e->awardIcon[i].y = y + ( e->award[i].h - 22.0f ) * 0.5f;
		e->awardIcon[i].w = e->awardIcon[i].h = 22.0f;
		cx = e->awardIcon[i].x + 22.0f + 12.0f;
		MH_Place( &e->awardLabelT[i], cx, y + 1.0f + 12.0f );
		MH_Place( &e->awardNameT[i], cx, e->awardLabelT[i].box.y + e->awardLabelT[i].box.h + 4.0f );
		MH_Fit( &e->awardNameT[i], MH_END_PODIUM_W - 3.0f - 14.0f - 34.0f - 120.0f );
		if ( e->awardNameT[i].box.w > e->awardLabelT[i].box.w ) e->awardLabelT[i].box.w = e->awardNameT[i].box.w;
		else e->awardNameT[i].box.w = e->awardLabelT[i].box.w;
		MH_Place( &e->awardValueT[i], x + MH_END_PODIUM_W - 1.0f - 14.0f - e->awardValueT[i].box.w,
		          y + ( e->award[i].h - e->awardValueT[i].box.h ) * 0.5f );
		y += e->award[i].h + 10.0f;
	}
}

/* the classification: a panel; the title row, 12px, the heads, 12px, the
 * rows (34 high, 24 compact; 2px apart) 8px wider than the panel's inside */
static void MH_EndList( mhEndLayout_t *e ) {
	static const char *heads[4] = { "POS", "DRIVER", "TIME", "BEST" };
	wsTextStyle_t label12, label11, mono14, text16, mono12, text14;
	float x = e->viewWidth - MH_END_MARGIN - MH_END_LIST_W, inX, inW, y, colX[4], colW[4], rowH;
	int i, c;

	MH_Label( &label12, 12.0f );
	MH_Label( &label11, 11.0f );
	MH_Mono( &mono14, 14.0f );
	MH_Text( &text16, 16.0f );
	MH_Mono( &mono12, 12.0f );
	MH_Text( &text14, 14.0f );
	e->list.x = x;
	e->list.y = e->top + MH_END_TOP;
	e->list.w = MH_END_LIST_W;
	inX = x + 1.0f + MH_PAD_X;
	inW = MH_END_LIST_W - 2.0f - 2.0f * MH_PAD_X;
	y = e->list.y + 2.0f + MH_PAD_Y;
	MH_Set( &e->listTitleT, &label12, e->listTitle );
	MH_Set( &e->listRightT, &label12, e->listRight ? e->listRight : "" );
	y += MH_BaselineRow( &e->listTitleT, &e->listRightT, inX, y, inW ) + 12.0f;

	/* the teams: cells 2px apart, 40 high; the name over the time, 6 8 in */
	if ( e->teamCellCount > 0 ) {
		wsTextStyle_t label10, mono13;
		float cw = ( inW - 2.0f * ( e->teamCellCount - 1 ) ) / e->teamCellCount;
		MH_Label( &label10, 10.0f );
		MH_Mono( &mono13, 13.0f );
		for ( c = 0; c < e->teamCellCount && c < MH_END_TEAMS; c++ ) {
			e->teamCell[c].x = inX + c * ( cw + 2.0f );
			e->teamCell[c].y = y;
			e->teamCell[c].w = cw;
			e->teamCell[c].h = 40.0f;
			MH_Set( &e->teamCellNameT[c], &label10, e->teamCellName[c] );
			MH_Set( &e->teamCellValueT[c], &mono13, e->teamCellValue[c] );
			MH_Place( &e->teamCellNameT[c], e->teamCell[c].x + 8.0f, y + 6.0f );
			MH_Place( &e->teamCellValueT[c], e->teamCell[c].x + 8.0f, e->teamCellNameT[c].box.y + e->teamCellNameT[c].box.h + 2.0f );
			e->teamCellNameT[c].box.w = e->teamCellValueT[c].box.w = cw - 16.0f;
		}
		y += 40.0f + 12.0f;
	}

	/* the columns: 34, (a 10px square,) the rest, 82, 70, 10px apart, inside 8px */
	colW[0] = 34.0f;
	colW[2] = 82.0f;
	colW[3] = 70.0f;
	colW[1] = inW - 16.0f - colW[0] - colW[2] - colW[3] - 30.0f - ( e->rowSwatch ? 20.0f : 0.0f );
	colX[0] = 8.0f;
	colX[1] = colX[0] + colW[0] + 10.0f + ( e->rowSwatch ? 20.0f : 0.0f );
	for ( c = 2; c < 4; c++ ) colX[c] = colX[c - 1] + colW[c - 1] + 10.0f;
	for ( c = 0; c < 4; c++ ) {
		MH_Set( &e->headT[c], &label11, heads[c] );
		MH_Place( &e->headT[c], inX + colX[c], y );
		e->headT[c].box.w = colW[c];
	}
	y += e->headT[0].box.h + 12.0f;

	rowH = e->compact ? 24.0f : 34.0f;
	for ( i = 0; i < e->rowCount && i < MH_END_ROWS; i++ ) {
		wsRect_t *r = &e->row[i];
		r->x = inX - 8.0f;
		r->w = inW + 16.0f;
		r->y = y;
		r->h = rowH;
		MH_Set( &e->rowPosT[i], e->compact ? &mono12 : &mono14, e->rowPos[i] );
		MH_Set( &e->rowNameT[i], e->compact ? &text14 : &text16, e->rowName[i] );
		MH_Set( &e->rowTimeT[i], e->compact ? &mono12 : &mono14, e->rowTime[i] );
		MH_Set( &e->rowBestT[i], e->compact ? &mono12 : &mono14, e->rowBest[i] );
		MH_Fit( &e->rowNameT[i], colW[1] );
		MH_Place( &e->rowPosT[i], r->x + colX[0], y + ( rowH - e->rowPosT[i].box.h ) * 0.5f );
		MH_Place( &e->rowNameT[i], r->x + colX[1], y + ( rowH - e->rowNameT[i].box.h ) * 0.5f );
		MH_Place( &e->rowTimeT[i], r->x + colX[2], y + ( rowH - e->rowTimeT[i].box.h ) * 0.5f );
		MH_Place( &e->rowBestT[i], r->x + colX[3], y + ( rowH - e->rowBestT[i].box.h ) * 0.5f );
		if ( e->rowSwatch ) {
			e->rowSwatchR[i].x = r->x + colX[0] + colW[0] + 10.0f;
			e->rowSwatchR[i].y = y + ( rowH - 10.0f ) * 0.5f;
			e->rowSwatchR[i].w = e->rowSwatchR[i].h = 10.0f;
		}
		/* the cells: the columns' width; time and best at the right */
		e->rowPosT[i].box.w = colW[0];
		e->rowNameT[i].box.w = colW[1];
		e->rowTimeT[i].box.w = colW[2];
		e->rowBestT[i].box.w = colW[3];
		y += rowH + ( i < e->rowCount - 1 ? 2.0f : 0.0f );
	}
	e->list.h = y + MH_PAD_Y + 1.0f - e->list.y;
}

/* the bottom: the keys at the left, who is ready at the right */
static void MH_EndFooter( mhEndLayout_t *e ) {
	wsTextStyle_t label12, label11, mono13;
	float bottom = e->top + MH_DESIGN_HEIGHT - 36.0f, x = MH_END_MARGIN, rx, y, rowH;
	int i;

	MH_Label( &label12, 12.0f );
	MH_Label( &label11, 11.0f );
	MH_Mono( &mono13, 13.0f );
	rx = e->viewWidth - MH_END_MARGIN - 360.0f;
	MH_Set( &e->readyLabelT, &label11, e->readyLabel ? e->readyLabel : "" );
	MH_Set( &e->readyValueT, &mono13, e->readyValue ? e->readyValue : "" );
	rowH = MH_Base( &e->readyValueT ) > MH_Base( &e->readyLabelT ) ? e->readyValueT.box.h : e->readyLabelT.box.h;
	y = bottom - 3.0f - 6.0f - rowH;
	MH_BaselineRow( &e->readyLabelT, &e->readyValueT, rx, y, 360.0f );
	e->readyBar.x = rx;
	e->readyBar.y = bottom - 3.0f;
	e->readyBar.w = 360.0f;
	e->readyBar.h = 3.0f;

	/* the keys, centred on the block's height */
	for ( i = 0; i < 3; i++ ) {
		MH_Set( &e->keyNameT[i], &label12, e->keyName[i] ? e->keyName[i] : "" );
		MH_Set( &e->keyActionT[i], &label12, e->keyAction[i] ? e->keyAction[i] : "" );
		MH_Place( &e->keyNameT[i], x, bottom - ( 3.0f + 6.0f + rowH ) * 0.5f - e->keyNameT[i].box.h * 0.5f );
		MH_Place( &e->keyActionT[i], x + e->keyNameT[i].box.w + 8.0f, e->keyNameT[i].box.y );
		x = e->keyActionT[i].box.x + e->keyActionT[i].box.w + 28.0f;
	}
}

void MH_EndLayout( mhEndLayout_t *e ) {
	wsTextStyle_t label13, label12;

	MH_Label( &label13, 13.0f );
	MH_Label( &label12, 12.0f );
	MH_Set( &e->titleT, &label13, e->title );
	MH_Set( &e->subtitleT, &label12, e->subtitle ? e->subtitle : "" );
	MH_Place( &e->titleT, MH_END_MARGIN, e->top + 48.0f );
	MH_Place( &e->subtitleT, MH_END_MARGIN + e->titleT.box.w + 16.0f,
	          e->titleT.box.y + MH_Base( &e->titleT ) - MH_Base( &e->subtitleT ) );

	MH_EndLeft( e );
	MH_EndMiddle( e );
	MH_EndList( e );
	MH_EndFooter( e );
}
