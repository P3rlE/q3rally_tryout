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

/* the chat (gen.py chat): under the card 10px apart, each line as wide as
 * its text at the left edge, padding 4px 10px, "Sam:" bold and the message
 * 6px apart. A message too long for MH_CHAT_W breaks at a space onto a
 * second row under it (that row cut with an ellipsis). */
static float MH_Chat( mhRaceLayout_t *l, float y ) {
	wsTextStyle_t name, text;
	float x = MH_SIDE, room;
	int i;

	MH_Text( &name, 14.0f );
	name.font = WS_BODY_BOLD;
	MH_Text( &text, 14.0f );
	for ( i = 0; i < l->chatCount; i++ ) {
		mhText_t *n = &l->chatNameT[i], *m = &l->chatMessageT[i], *more = &l->chatMoreT[i];
		char who[MH_TEXT_MAX];
		float w, h;
		float gap;
		/* no name: a line of the server or the game ("Tobias entered the game") */
		if ( l->chatName[i] && l->chatName[i][0] ) Com_sprintf( who, sizeof( who ), "%s:", l->chatName[i] );
		else who[0] = '\0';
		MH_Set( n, &name, who );
		MH_Fit( n, 140.0f );
		n->box.w = MH_Width( n );
		gap = who[0] ? 6.0f : 0.0f;
		room = MH_CHAT_W - 20.0f - n->box.w - gap;
		MH_Set( m, &text, l->chatMessage[i] ? l->chatMessage[i] : "" );
		MH_Set( more, &text, "" );
		if ( m->box.w > room ) {
			/* the last space that still fits; none: cut the word */
			char whole[256];
			int cut, len;
			WS_Transform( &text, l->chatMessage[i], whole, sizeof( whole ) );
			len = strlen( whole );
			for ( cut = len; cut > 0; cut-- ) {
				if ( whole[cut] != ' ' && whole[cut] != '\0' ) continue;
				if ( WS_TextWidth( &m->style, whole, cut ) <= room ) break;
			}
			if ( cut <= 0 ) {
				for ( cut = len; cut > 1 && WS_TextWidth( &m->style, whole, cut ) > room; cut-- );
			}
			Q_strncpyz( m->text, whole, cut + 1 < (int)sizeof( m->text ) ? cut + 1 : (int)sizeof( m->text ) );
			m->box.w = MH_Width( m );
			while ( whole[cut] == ' ' ) cut++;
			Q_strncpyz( more->text, whole + cut, sizeof( more->text ) );
			MH_Fit( more, room );
			more->box.w = MH_Width( more );
		}
		w = 10.0f + n->box.w + gap + ( m->box.w > more->box.w ? m->box.w : more->box.w ) + 10.0f;
		h = 4.0f + n->box.h + ( more->text[0] ? 2.0f + more->box.h : 0.0f ) + 4.0f;
		l->chatBox[i].x = x;
		l->chatBox[i].y = y;
		l->chatBox[i].w = w;
		l->chatBox[i].h = h;
		MH_Place( n, x + 10.0f, y + 4.0f );
		MH_Place( m, x + 10.0f + n->box.w + gap, y + 4.0f + MH_Base( n ) - MH_Base( m ) );
		MH_Place( more, m->box.x, m->box.y + m->box.h + 2.0f );
		y += h + MH_COLUMN_GAP;
	}
	return y;
}

/* the line being typed, under the chat: MH_CHAT_W wide so it does not grow
 * with the text, padding 6px 10px past a 3px bar in the mode's colour, the
 * label ("SAY", "TEAM", "TO SAM") and 10px on the text, on one baseline. A
 * text too long shows the part around the caret: the end while typing at it. */
static float MH_ChatInput( mhRaceLayout_t *l, float y ) {
	wsTextStyle_t label, text;
	mhText_t *lt = &l->chatInputLabelT, *tt = &l->chatInputTextT;
	const char *all = l->chatInputText ? l->chatInputText : "";
	float x = MH_SIDE, room;
	int len = strlen( all ), cursor = l->chatInputCursor, s, e;

	MH_Label( &label, 11.0f );
	MH_Text( &text, 14.0f );
	if ( cursor < 0 ) cursor = 0;
	if ( cursor > len ) cursor = len;
	l->chatInputBox.x = x;
	l->chatInputBox.y = y;
	l->chatInputBox.w = MH_CHAT_W;
	l->chatInputBox.h = 6.0f + text.lineHeight + 6.0f;
	l->chatInputBar = l->chatInputBox;
	l->chatInputBar.w = 3.0f;
	MH_Set( lt, &label, l->chatInputLabel ? l->chatInputLabel : "" );
	MH_Fit( lt, 140.0f );
	lt->box.w = MH_Width( lt );
	room = MH_CHAT_W - 3.0f - 10.0f - lt->box.w - 10.0f - 10.0f;
	/* the most of the end that fits, moved back to the caret if it is before it */
	for ( s = 0; s < len && WS_TextWidth( &text, all + s, len - s ) > room; s++ );
	if ( cursor < s ) s = cursor;
	for ( e = len; e > s && WS_TextWidth( &text, all + s, e - s ) > room; e-- );
	if ( e - s > MH_TEXT_MAX - 1 ) {
		if ( cursor - s > MH_TEXT_MAX - 1 ) s = cursor - ( MH_TEXT_MAX - 1 );
		e = s + MH_TEXT_MAX - 1;
	}
	tt->style = text;
	Q_strncpyz( tt->text, all + s, e - s + 1 );
	tt->box.w = MH_Width( tt );
	tt->box.h = text.lineHeight;
	MH_Place( lt, x + 3.0f + 10.0f, 0.0f );
	MH_Place( tt, lt->box.x + lt->box.w + 10.0f, y + 6.0f );
	lt->box.y = tt->box.y + MH_Base( tt ) - MH_Base( lt );
	l->chatInputCaret.x = tt->box.x + WS_TextWidth( &text, all + s, cursor - s );
	l->chatInputCaret.y = tt->box.y;
	l->chatInputCaret.w = 2.0f;
	l->chatInputCaret.h = tt->box.h;
	return l->chatInputBox.h;
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
	if ( l->chatCount || l->chatInput ) {
		float y = MH_Chat( l, l->card.h > 0.0f ? l->card.y + l->card.h + MH_COLUMN_GAP : MH_TOP );
		if ( l->chatInput ) MH_ChatInput( l, y );
	}
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
/* a tournament driver's way (gen.py path_line): YOUR WAY over a line a
 * round, 26 high and 4 apart: the round in 112, the match 12 after it, the
 * tag at the right, on one baseline (a round still to come: at the top) */
static float MH_PathRows( mhText_t *title, mhText_t *round, mhText_t *what, mhText_t *tag,
                          const char **rounds, const char **whats, const char **tags, int count, float x, float y, float w ) {
	wsTextStyle_t label12, label11, text15;
	int i;
	MH_Label( &label12, 12.0f );
	MH_Label( &label11, 11.0f );
	MH_Text( &text15, 15.0f );
	MH_Set( title, &label12, "YOUR WAY" );
	MH_Place( title, x, y );
	title->box.w = w;
	y += title->box.h + 4.0f;
	for ( i = 0; i < count && i < MH_PATH_ROWS; i++ ) {
		float base;
		MH_Set( &round[i], &label11, rounds[i] ? rounds[i] : "" );
		MH_Set( &what[i], &text15, whats[i] ? whats[i] : "" );
		MH_Set( &tag[i], &label11, tags[i] ? tags[i] : "" );
		/* the line's baseline: the match's, the labels on it */
		base = MH_Base( &what[i] ) > MH_Base( &round[i] ) ? MH_Base( &what[i] ) : MH_Base( &round[i] );
		if ( !what[i].text[0] ) base = MH_Base( &round[i] );
		MH_Place( &what[i], x + 112.0f + 12.0f, y + base - MH_Base( &what[i] ) );
		MH_Place( &round[i], x, y + base - MH_Base( &round[i] ) );
		round[i].box.w = 112.0f;
		MH_Place( &tag[i], x + w - tag[i].box.w, y + base - MH_Base( &tag[i] ) );
		MH_Fit( &what[i], tag[i].box.x - 12.0f - what[i].box.x );
		y += 26.0f + ( i < count - 1 ? 4.0f : 0.0f );
	}
	return y;
}

static void MH_EndLeft( mhEndLayout_t *e ) {
	wsTextStyle_t big168, big48, big44, big30, label12, label11, label10, mono11;
	float x = MH_END_MARGIN, y = e->top + MH_END_TOP, w = MH_END_LEFT_W, h, slot;
	int i;

	MH_Big( &big168, e->posSize > 0.0f ? e->posSize : 168.0f );
	MH_Big( &big48, 48.0f );
	MH_Big( &big44, 44.0f );
	MH_Big( &big30, 30.0f );
	MH_Label( &label12, 12.0f );
	MH_Label( &label11, 11.0f );
	MH_Label( &label10, 10.0f );
	MH_Mono( &mono11, 11.0f );

	/* "P2" and "/ 8" on one baseline, 14px apart; a ghost race's margin
	 * smaller, the best lap 10px under it */
	MH_Set( &e->posT, &big168, e->pos );
	MH_Set( &e->ofT, &big48, e->of ? e->of : "" );
	MH_Place( &e->posT, x, y );
	MH_Place( &e->ofT, x + e->posT.box.w + 14.0f, y + MH_Base( &e->posT ) - MH_Base( &e->ofT ) );
	y += e->posT.box.h + 22.0f - ( e->posSize > 0.0f ? 10.0f : 18.0f );

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
	if ( e->pathCount > 0 ) {
		MH_PathRows( &e->pathTitleT, e->pathRoundT, e->pathWhatT, e->pathTagT, e->pathRound, e->pathWhat, e->pathTag,
		             e->pathCount, x, y, w );
		return;
	}
	MH_Set( &e->lapsTitleT, &label12, e->chartTitle ? e->chartTitle : "YOUR LAPS" );
	MH_Set( &e->lapsNoteT, &label11, e->chartNote ? e->chartNote : "LOST TO YOUR BEST" );
	y += MH_BaselineRow( &e->lapsTitleT, &e->lapsNoteT, x, y, w ) + 12.0f;

	/* Autoball: a goal a row 22 high, 2 apart: the minute (30), the square
	 * (10), the scorer, the shot (70) and the score (44) at the right, 10
	 * apart, each in the row's middle */
	for ( i = 0; i < e->goalCount && i < MH_END_GOALS; i++ ) {
		wsTextStyle_t mono12, mono13, text14;
		MH_Mono( &mono12, 12.0f );
		MH_Mono( &mono13, 13.0f );
		MH_Text( &text14, 14.0f );
		MH_Set( &e->goalMinuteT[i], &mono12, e->goalMinute[i] ? e->goalMinute[i] : "" );
		MH_Set( &e->goalNameT[i], &text14, e->goalName[i] ? e->goalName[i] : "" );
		MH_Set( &e->goalShotT[i], &mono12, e->goalShot[i] ? e->goalShot[i] : "" );
		MH_Set( &e->goalScoreT[i], &mono13, e->goalScore[i] ? e->goalScore[i] : "" );
		MH_Fit( &e->goalNameT[i], w - 194.0f );
		MH_Place( &e->goalMinuteT[i], x, y + ( 22.0f - e->goalMinuteT[i].box.h ) * 0.5f );
		e->goalMinuteT[i].box.w = 30.0f;	/* the column */
		e->goalSwatch[i].x = x + 40.0f;
		e->goalSwatch[i].y = y + 6.0f;
		e->goalSwatch[i].w = e->goalSwatch[i].h = 10.0f;
		MH_Place( &e->goalNameT[i], x + 60.0f, y + ( 22.0f - e->goalNameT[i].box.h ) * 0.5f );
		e->goalNameT[i].box.w = w - 194.0f;
		/* the shot and the score: their columns (the text at the right of them) */
		MH_Place( &e->goalShotT[i], x + w - 124.0f, y + ( 22.0f - e->goalShotT[i].box.h ) * 0.5f );
		e->goalShotT[i].box.w = 70.0f;
		MH_Place( &e->goalScoreT[i], x + w - 44.0f, y + ( 22.0f - e->goalScoreT[i].box.h ) * 0.5f );
		e->goalScoreT[i].box.w = 44.0f;
		y += 22.0f + 2.0f;
	}

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
		if ( e->lapDiverge ) {
			/* 27 up, a 2px line, 27 down: gained up from it, lost down */
			bh = (float)(int)( 26.0f * ( e->lapFrac[i] < 0.0f ? -e->lapFrac[i] : e->lapFrac[i] ) );	/* whole pixels */
			if ( bh < 2.0f ) bh = 2.0f;
			e->lapAxis[i].x = sx;
			e->lapAxis[i].y = y + 27.0f;
			e->lapAxis[i].w = slot;
			e->lapAxis[i].h = 2.0f;
			e->lapBar[i].h = bh;
			e->lapBar[i].y = e->lapFrac[i] < 0.0f ? y + 27.0f - bh : y + 29.0f;
		}
		MH_Set( &e->lapValueT[i], &mono11, e->lapValue[i] );
		MH_Set( &e->lapNameT[i], &label10, e->lapName[i] );
		MH_Place( &e->lapValueT[i], sx + ( slot - e->lapValueT[i].box.w ) * 0.5f, y + 56.0f + 6.0f );
		MH_Place( &e->lapNameT[i], sx + ( slot - e->lapNameT[i].box.w ) * 0.5f,
		          e->lapValueT[i].box.y + e->lapValueT[i].box.h + 6.0f );
		e->lapSlot[i].h = e->lapNameT[i].box.y + e->lapNameT[i].box.h - y;
		if ( e->lapPlace[i] ) {
			/* a sprint: 6px, a 1px rule, 6px, the place */
			wsTextStyle_t mono12;
			MH_Mono( &mono12, 12.0f );
			e->lapRule[i].x = sx;
			e->lapRule[i].y = e->lapNameT[i].box.y + e->lapNameT[i].box.h + 6.0f;
			e->lapRule[i].w = slot;
			e->lapRule[i].h = 1.0f;
			MH_Set( &e->lapPlaceT[i], &mono12, e->lapPlace[i] );
			MH_Place( &e->lapPlaceT[i], sx + ( slot - e->lapPlaceT[i].box.w ) * 0.5f, e->lapRule[i].y + 1.0f + 6.0f );
			e->lapSlot[i].h = e->lapPlaceT[i].box.y + e->lapPlaceT[i].box.h - y;
		}
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

/* a ghost race: you and the ghost, cards 268 wide 64 apart from the top,
 * the car 250 high; VS in the gap. A card: the car, 12px, the tag, 6px,
 * the name, 6px, the lap with the note on its baseline, 14px */
static void MH_EndDuel( mhEndLayout_t *e, float x, float top ) {
	wsTextStyle_t label11, text20, big34, mono13;
	int k;

	MH_Label( &label11, 11.0f );
	MH_Text( &text20, 20.0f );
	MH_Big( &big34, 34.0f );
	MH_Mono( &mono13, 13.0f );
	for ( k = 0; k < 2; k++ ) {
		float cx = x + k * ( 268.0f + 64.0f ), inX = cx + 1.0f + 14.0f, inW = 268.0f - 2.0f - 28.0f, y;
		MH_Set( &e->duelTagT[k], &label11, e->duelTag[k] ? e->duelTag[k] : "" );
		MH_Set( &e->duelNameT[k], &text20, e->duelName[k] ? e->duelName[k] : "" );
		MH_Set( &e->duelLapT[k], &big34, e->duelLap[k] ? e->duelLap[k] : "" );
		MH_Set( &e->duelNoteT[k], &mono13, e->duelNote[k] ? e->duelNote[k] : "" );
		e->duelCard[k].x = cx;
		e->duelCard[k].y = top;
		e->duelCard[k].w = 268.0f;
		e->duelCar[k].x = cx + 1.0f;
		e->duelCar[k].y = top + 2.0f;
		e->duelCar[k].w = 266.0f;
		e->duelCar[k].h = 250.0f;
		y = top + 2.0f + 250.0f + 12.0f;
		MH_Place( &e->duelTagT[k], inX, y );
		e->duelTagT[k].box.w = inW;
		y += e->duelTagT[k].box.h + 6.0f;
		MH_Fit( &e->duelNameT[k], inW );
		MH_Place( &e->duelNameT[k], inX, y );
		e->duelNameT[k].box.w = inW;
		y += e->duelNameT[k].box.h + 6.0f;
		y += MH_BaselineRow( &e->duelLapT[k], &e->duelNoteT[k], inX, y, inW );
		e->duelCard[k].h = y + 14.0f + 1.0f - top;
	}
	MH_Set( &e->vsT, &big34, "VS" );
	MH_Place( &e->vsT, x + 268.0f + ( 64.0f - e->vsT.box.w ) * 0.5f, top + 104.0f );
}

/* a ghost race: the ladder, a panel 16px under the classification; the
 * title row, 12px, rows 30 high 2px apart: 34, the name, 82, 40, 10 apart */
static void MH_EndLadder( mhEndLayout_t *e ) {
	wsTextStyle_t label12, label10, mono13, text15;
	float inX, inW, y, colX[4], colW[4];
	int i;

	if ( e->ladderCount <= 0 ) return;
	MH_Label( &label12, 12.0f );
	MH_Label( &label10, 10.0f );
	MH_Mono( &mono13, 13.0f );
	MH_Text( &text15, 15.0f );
	e->ladder.x = e->list.x;
	e->ladder.y = e->list.y + e->list.h + 16.0f;
	e->ladder.w = e->list.w;
	inX = e->ladder.x + 1.0f + MH_PAD_X;
	inW = e->ladder.w - 2.0f - 2.0f * MH_PAD_X;
	y = e->ladder.y + 2.0f + MH_PAD_Y;
	MH_Set( &e->ladderTitleT, &label12, e->ladderTitle ? e->ladderTitle : "GHOST LADDER" );
	MH_Set( &e->ladderRightT, &label12, e->ladderRight ? e->ladderRight : "" );
	y += MH_BaselineRow( &e->ladderTitleT, &e->ladderRightT, inX, y, inW ) + 12.0f;
	colW[0] = 34.0f;
	colW[2] = 82.0f;
	colW[3] = 40.0f;
	colW[1] = inW - colW[0] - colW[2] - colW[3] - 30.0f;
	colX[0] = 0.0f;
	for ( i = 1; i < 4; i++ ) colX[i] = colX[i - 1] + colW[i - 1] + 10.0f;
	for ( i = 0; i < e->ladderCount && i < MH_END_LADDER; i++ ) {
		wsRect_t *r = &e->ladderRow[i];
		r->x = inX - 8.0f;
		r->w = inW + 16.0f;
		r->y = y;
		r->h = 30.0f;
		MH_Set( &e->ladderPosT[i], &mono13, e->ladderPos[i] );
		MH_Set( &e->ladderNameT[i], &text15, e->ladderName[i] );
		MH_Set( &e->ladderLapT[i], &mono13, e->ladderLap[i] );
		MH_Set( &e->ladderTagT[i], &label10, e->ladderTag[i] ? e->ladderTag[i] : "" );
		MH_Fit( &e->ladderNameT[i], colW[1] );
		MH_Place( &e->ladderPosT[i], inX + colX[0], y + ( 30.0f - e->ladderPosT[i].box.h ) * 0.5f );
		MH_Place( &e->ladderNameT[i], inX + colX[1], y + ( 30.0f - e->ladderNameT[i].box.h ) * 0.5f );
		MH_Place( &e->ladderLapT[i], inX + colX[2], y + ( 30.0f - e->ladderLapT[i].box.h ) * 0.5f );
		MH_Place( &e->ladderTagT[i], inX + colX[3], y + ( 30.0f - e->ladderTagT[i].box.h ) * 0.5f );
		e->ladderPosT[i].box.w = colW[0];
		e->ladderNameT[i].box.w = colW[1];
		e->ladderLapT[i].box.w = colW[2];
		e->ladderTagT[i].box.w = colW[3];
		y += 30.0f + ( i < e->ladderCount - 1 ? 2.0f : 0.0f );
	}
	e->ladder.h = y + MH_PAD_Y + 1.0f - e->ladder.y;
}

/* the middle: the podium (second, winner, third, 12px apart, on one line),
 * 28px under it the highlights */
/* a drag duel: the heat over a time slip a lane (gen.py time_slip), the
 * lines 22 high over a dashed rule; returns where they end */
static float MH_EndSlips( mhEndLayout_t *e, float x, float top ) {
	wsTextStyle_t label12, label11, text20, mono14;
	float y, bottom = top;
	int k, i, n = e->slipCount < 2 ? e->slipCount : 2;
	MH_Label( &label12, 12.0f );
	MH_Label( &label11, 11.0f );
	MH_Text( &text20, 20.0f );
	MH_Mono( &mono14, 14.0f );
	MH_Set( &e->slipTitleT, &label12, e->slipTitle );
	MH_Set( &e->slipNoteT, &label11, e->slipNote ? e->slipNote : "" );
	y = top + MH_BaselineRow( &e->slipTitleT, &e->slipNoteT, x, top, MH_END_PODIUM_W ) + 10.0f;
	for ( k = 0; k < n; k++ ) {
		float sx = n == 1 ? x + ( MH_END_PODIUM_W - 294.0f ) * 0.5f : x + k * ( 294.0f + 12.0f );
		float inX = sx + 1.0f + 14.0f, inW = 294.0f - 2.0f - 28.0f, ry, right;
		e->slip[k].x = sx;
		e->slip[k].y = y;
		e->slip[k].w = 294.0f;
		e->slipCar[k].x = sx + 1.0f;
		e->slipCar[k].y = y + 2.0f;
		e->slipCar[k].w = 292.0f;
		e->slipCar[k].h = 176.0f;
		ry = y + 2.0f + 176.0f + 12.0f;
		/* the lane at the left, YOU and WIN at the right */
		MH_Set( &e->slipLaneT[k], &label11, e->slipLane[k] );
		MH_Set( &e->slipTagT[k], &label11, e->slipWin[k] ? "WIN" : "" );
		MH_Set( &e->slipYouT[k], &label11, e->slipMe[k] ? "YOU" : "" );
		MH_Place( &e->slipLaneT[k], inX, ry );
		right = inX + inW;
		MH_Place( &e->slipTagT[k], right - e->slipTagT[k].box.w, ry );
		if ( e->slipTagT[k].box.w > 0.0f ) right -= e->slipTagT[k].box.w + 10.0f;
		MH_Place( &e->slipYouT[k], right - e->slipYouT[k].box.w, ry );
		ry += e->slipLaneT[k].box.h + 8.0f;
		MH_Set( &e->slipNameT[k], &text20, e->slipName[k] );
		MH_Place( &e->slipNameT[k], inX, ry );
		MH_Fit( &e->slipNameT[k], inW );
		e->slipNameT[k].box.w = inW;
		ry += e->slipNameT[k].box.h + 8.0f;
		for ( i = 0; i < e->slipRows && i < MH_END_SLIP_ROWS; i++ ) {
			mhText_t *lab = &e->slipLabelT[k][i], *val = &e->slipValueT[k][i];
			MH_Set( lab, &label11, e->slipLabel[i] );
			MH_Set( val, &mono14, e->slipValue[k][i] ? e->slipValue[k][i] : "" );
			MH_BaselineRow( lab, val, inX, ry, inW );
			e->slipRule[k][i].x = inX;
			e->slipRule[k][i].y = ry + 22.0f;
			e->slipRule[k][i].w = inW;
			e->slipRule[k][i].h = 1.0f;
			ry += 23.0f;
		}
		e->slip[k].h = ry + 14.0f + 1.0f - y;
		if ( y + e->slip[k].h > bottom ) bottom = y + e->slip[k].h;
	}
	return bottom;
}

static void MH_EndMiddle( mhEndLayout_t *e ) {
	wsTextStyle_t label12, label11, text16, mono15;
	float x = MH_END_PODIUM_X + ( e->viewWidth - 1600.0f ) * 0.5f, top = e->top + MH_END_TOP;
	float bottom = top + MH_END_PODIUM_H, y;
	int i;

	if ( e->slipCount > 0 ) bottom = MH_EndSlips( e, x, top );
	if ( e->duel ) MH_EndDuel( e, x, top );

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
	wsTextStyle_t label12, label11, mono14, text16, mono12, text14, mono13, text15;
	float x = e->viewWidth - MH_END_MARGIN - MH_END_LIST_W, inX, inW, y, colX[4], colW[4], rowH;
	int i, c;

	MH_Label( &label12, 12.0f );
	MH_Label( &label11, 11.0f );
	MH_Mono( &mono14, 14.0f );
	MH_Text( &text16, 16.0f );
	MH_Mono( &mono12, 12.0f );
	MH_Text( &text14, 14.0f );
	MH_Mono( &mono13, 13.0f );
	MH_Text( &text15, 15.0f );
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
	colW[1] = inW - colW[0] - colW[2] - colW[3] - 30.0f - ( e->rowSwatch ? 20.0f : 0.0f );
	if ( e->listMatches ) {	/* a tournament's matches: 34, the winner, 44, the one beaten */
		colW[2] = 44.0f;
		colW[1] = colW[3] = ( inW - colW[0] - colW[2] - 30.0f ) * 0.5f;
	}
	colX[0] = 0.0f;
	colX[1] = colX[0] + colW[0] + 10.0f + ( e->rowSwatch ? 20.0f : 0.0f );
	for ( c = 2; c < 4; c++ ) colX[c] = colX[c - 1] + colW[c - 1] + 10.0f;
	for ( c = 0; c < 4; c++ ) {
		MH_Set( &e->headT[c], &label11, e->listHead[c] ? e->listHead[c] : heads[c] );
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
		if ( e->listMatches ) {	/* the round smaller, both names as names */
			MH_Set( &e->rowPosT[i], e->compact ? &mono12 : &mono13, e->rowPos[i] );
			MH_Set( &e->rowNameT[i], e->compact ? &text14 : &text15, e->rowName[i] );
			MH_Set( &e->rowBestT[i], e->compact ? &text14 : &text15, e->rowBest[i] );
			MH_Fit( &e->rowBestT[i], colW[3] );
		}
		MH_Fit( &e->rowNameT[i], colW[1] );
		MH_Place( &e->rowPosT[i], inX + colX[0], y + ( rowH - e->rowPosT[i].box.h ) * 0.5f );
		MH_Place( &e->rowNameT[i], inX + colX[1], y + ( rowH - e->rowNameT[i].box.h ) * 0.5f );
		MH_Place( &e->rowTimeT[i], inX + colX[2], y + ( rowH - e->rowTimeT[i].box.h ) * 0.5f );
		MH_Place( &e->rowBestT[i], inX + colX[3], y + ( rowH - e->rowBestT[i].box.h ) * 0.5f );
		if ( e->rowSwatch ) {
			e->rowSwatchR[i].x = inX + colX[0] + colW[0] + 10.0f;
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
	MH_EndLadder( e );
	MH_EndFooter( e );
}

/* -------------------------------------------------------------------------
   the TAB table (gen.py tab_header, tab_table, team_side, tab_footer)
   ------------------------------------------------------------------------- */

/* the rows of a table from y: the columns 'gap' apart inside 8px of the
 * row, the row as wide as the panel's inside and 8px either side */
static float MH_TabRows( mhTabLayout_t *t, int first, int count, float inX, float inW, float y, float gap, float scoreSize ) {
	float colX[MH_TAB_COLS], colW[MH_TAB_COLS], fixed = 0.0f, rowH = t->compact ? 24.0f : 34.0f;
	int c, i, rest = -1;
	wsTextStyle_t mono14, mono13, label11, label10, text16, score;
	MH_Mono( &mono14, 14.0f );
	MH_Mono( &mono13, 13.0f );
	MH_Label( &label11, 11.0f );
	MH_Label( &label10, 10.0f );
	MH_Text( &text16, 16.0f );
	MH_Big( &score, scoreSize );

	for ( c = 0; c < t->colCount; c++ ) {
		if ( t->colW[c] > 0.0f ) fixed += t->colW[c];
		else rest = c;
	}
	for ( c = 0; c < t->colCount; c++ ) {
		colW[c] = c == rest ? inW - fixed - gap * ( t->colCount - 1 ) : t->colW[c];
		colX[c] = c == 0 ? inX : colX[c - 1] + colW[c - 1] + gap;
	}
	for ( i = first; i < first + count && i < MH_TAB_ROWS; i++ ) {
		wsRect_t *r = &t->row[i];
		r->x = inX - 8.0f;
		r->y = y;
		r->w = inW + 16.0f;
		r->h = rowH;
		for ( c = 0; c < t->colCount; c++ ) {
			mhText_t *ct = &t->cellT[i][c];
			const char *v = t->cell[i][c] ? t->cell[i][c] : "";
			switch ( t->colKind[c] ) {
			case MH_TAB_NAME: {
				mhText_t *car = &t->carT[i];
				float carW;
				MH_Set( ct, &text16, v );
				MH_Set( car, &label10, t->rowCar[i] ? t->rowCar[i] : "" );
				carW = car->text[0] ? car->box.w + 10.0f : 0.0f;
				if ( ct->box.w + carW > colW[c] ) {	/* the car goes first, then the name is cut */
					car->text[0] = '\0';
					car->box.w = carW = 0.0f;
					MH_Fit( ct, colW[c] );
					ct->box.w = MH_Width( ct );
				}
				MH_Place( ct, colX[c], y + ( rowH - ct->box.h ) * 0.5f );
				MH_Place( car, colX[c] + ct->box.w + 10.0f, ct->box.y + MH_Base( ct ) - MH_Base( car ) );
				break;
			}
			case MH_TAB_SWATCH:
				ct->text[0] = '\0';
				ct->box.x = colX[c];
				ct->box.y = y + ( rowH - 10.0f ) * 0.5f;
				ct->box.w = ct->box.h = 10.0f;
				break;
			case MH_TAB_SCORE:
				MH_Set( ct, &score, v );
				break;
			case MH_TAB_PING:
				MH_Set( ct, Q_stricmp( v, "BOT" ) ? &mono13 : &label11, v );
				break;
			default:
				MH_Set( ct, &mono14, v );
				break;
			}
			if ( t->colKind[c] == MH_TAB_NAME || t->colKind[c] == MH_TAB_SWATCH ) continue;
			MH_Fit( ct, colW[c] );
			ct->box.w = colW[c];	/* the column's box; drawn at its right where colRight */
			MH_Place( ct, colX[c], y + ( rowH - ct->box.h ) * 0.5f );
		}
		if ( t->iconCol >= 0 && t->iconCol < t->colCount && t->rowIcon[i] ) {
			const mhText_t *v = &t->cellT[i][t->iconCol];
			t->icon[i].w = t->icon[i].h = 14.0f;
			t->icon[i].x = v->box.x + v->box.w - MH_Width( v ) - 6.0f - 14.0f;
			t->icon[i].y = y + ( rowH - 14.0f ) * 0.5f;
		}
		y += rowH + 2.0f;
	}
	return count > 0 ? y - 2.0f : y;
}

/* the heads over the rows, on the rows' grid */
static float MH_TabHead( mhTabLayout_t *t, int side, float inX, float inW, float y, float gap ) {
	float colX = inX, fixed = 0.0f, w;
	int c, rest = -1;
	wsTextStyle_t label11;
	MH_Label( &label11, 11.0f );
	for ( c = 0; c < t->colCount; c++ ) {
		if ( t->colW[c] > 0.0f ) fixed += t->colW[c];
		else rest = c;
	}
	for ( c = 0; c < t->colCount; c++ ) {
		mhText_t *h = &t->headT[side][c];
		w = c == rest ? inW - fixed - gap * ( t->colCount - 1 ) : t->colW[c];
		MH_Set( h, &label11, t->colHead[c] ? t->colHead[c] : "" );
		MH_Place( h, colX, y );
		h->box.w = w;
		colX += w + gap;
	}
	return y + label11.lineHeight;
}

void MH_TabLayout( mhTabLayout_t *t ) {
	wsTextStyle_t label13, label12, label11, label10, mono13, big34, big26, big56, text14;
	float x = ( t->viewWidth - MH_TAB_W ) * 0.5f, y, bottom, h;
	int k;
	MH_Label( &label13, 13.0f );
	MH_Label( &label12, 12.0f );
	MH_Label( &label11, 11.0f );
	MH_Label( &label10, 10.0f );
	MH_Mono( &mono13, 13.0f );
	MH_Big( &big34, 34.0f );
	MH_Big( &big26, 26.0f );
	MH_Big( &big56, 56.0f );
	MH_Text( &text14, 14.0f );

	/* the header: the mode over the map at the left, the time at the right,
	 * both standing on a line 33px over the table */
	bottom = 150.0f - 33.0f;
	MH_Set( &t->modeT, &label13, t->mode );
	MH_Set( &t->subT, &label12, t->sub );
	MH_Place( &t->subT, x, bottom - t->subT.box.h );
	MH_Place( &t->modeT, x, t->subT.box.y - 8.0f - t->modeT.box.h );
	MH_Fit( &t->subT, MH_TAB_W - 240.0f );
	t->modeT.box.w = t->subT.box.w = t->modeT.box.w > t->subT.box.w ? t->modeT.box.w : t->subT.box.w;
	MH_Set( &t->rightValueT, &big34, t->rightValue );
	MH_Set( &t->rightLabelT, &label11, t->rightLabel );
	MH_Place( &t->rightValueT, x + MH_TAB_W - t->rightValueT.box.w, bottom - t->rightValueT.box.h );
	MH_Place( &t->rightLabelT, x + MH_TAB_W - t->rightLabelT.box.w, t->rightValueT.box.y - 6.0f - t->rightLabelT.box.h );

	if ( t->sideCount <= 0 ) {
		/* one table: 2px top, inside 16 18 14 */
		float inX = x + 1.0f + 18.0f, inW = MH_TAB_W - 2.0f - 36.0f;
		y = 150.0f + 2.0f + 16.0f;
		if ( t->teamCellCount > 0 ) {
			float cw = ( inW - 2.0f * ( t->teamCellCount - 1 ) ) / t->teamCellCount;
			for ( k = 0; k < t->teamCellCount && k < MH_TAB_SIDES; k++ ) {
				wsRect_t *c = &t->teamCell[k];
				c->x = inX + k * ( cw + 2.0f );
				c->y = y;
				c->w = cw;
				c->h = 40.0f;
				MH_Set( &t->teamCellNameT[k], &label10, t->teamCellName[k] );
				MH_Set( &t->teamCellValueT[k], &mono13, t->teamCellValue[k] );
				MH_Set( &t->teamCellPlaceT[k], &big26, t->teamCellPlace[k] );
				MH_Place( &t->teamCellNameT[k], c->x + 10.0f, y + 6.0f );
				MH_Place( &t->teamCellValueT[k], c->x + 10.0f, t->teamCellNameT[k].box.y + t->teamCellNameT[k].box.h + 2.0f );
				MH_Place( &t->teamCellPlaceT[k], c->x + cw - 10.0f - t->teamCellPlaceT[k].box.w, y + ( 40.0f - t->teamCellPlaceT[k].box.h ) * 0.5f );
				MH_Fit( &t->teamCellValueT[k], cw - 20.0f - t->teamCellPlaceT[k].box.w - 8.0f );
				t->teamCellValueT[k].box.w = MH_Width( &t->teamCellValueT[k] );
				if ( t->teamCellValueT[k].box.w > t->teamCellNameT[k].box.w ) t->teamCellNameT[k].box.w = t->teamCellValueT[k].box.w;
				else t->teamCellValueT[k].box.w = t->teamCellNameT[k].box.w;
			}
			y += 40.0f + 12.0f;
		}
		y = MH_TabHead( t, 0, inX, inW, y, 10.0f ) + 12.0f;
		y = MH_TabRows( t, 0, t->rowCount, inX, inW, y, 10.0f, 24.0f );
		h = y + 14.0f + 1.0f - 150.0f;
		t->panel[0].x = x;
		t->panel[0].y = 150.0f;
		t->panel[0].w = MH_TAB_W;
		t->panel[0].h = h;
		bottom = 150.0f + h;
	} else {
		/* a panel a team: 3px top in its colour, inside 16 18 14; two side by
		 * side 16 apart, four two by two */
		float w = ( MH_TAB_W - 16.0f ) * 0.5f, inW = w - 2.0f - 36.0f, rowTop = 150.0f;
		int first = 0;
		bottom = 150.0f;
		for ( k = 0; k < t->sideCount && k < MH_TAB_SIDES; k++ ) {
			float px = x + ( k % 2 ) * ( w + 16.0f ), inX = px + 1.0f + 18.0f, top;
			float above;
			if ( k == 2 ) rowTop = bottom + 16.0f;
			top = rowTop + 3.0f + 16.0f;
			MH_Set( &t->sideTitleT[k], &label13, t->sideTitle[k] );
			MH_Set( &t->sideNoteT[k], &label11, t->sideNote[k] );
			MH_Set( &t->sideScoreT[k], &big56, t->sideScore[k] );
			/* the title over the note, standing on the score's foot */
			above = t->sideTitleT[k].box.h + 6.0f + t->sideNoteT[k].box.h;
			MH_Place( &t->sideScoreT[k], inX + inW - t->sideScoreT[k].box.w, top );
			MH_Place( &t->sideTitleT[k], inX, top + t->sideScoreT[k].box.h - above );
			MH_Place( &t->sideNoteT[k], inX, t->sideTitleT[k].box.y + t->sideTitleT[k].box.h + 6.0f );
			if ( t->sideNoteT[k].box.w > t->sideTitleT[k].box.w ) t->sideTitleT[k].box.w = t->sideNoteT[k].box.w;
			else t->sideNoteT[k].box.w = t->sideTitleT[k].box.w;
			y = top + t->sideScoreT[k].box.h + 12.0f;
			y = MH_TabHead( t, k, inX, inW, y, 8.0f ) + 12.0f;
			y = MH_TabRows( t, first, t->sideRows[k], inX, inW, y, 8.0f, 22.0f );
			first += t->sideRows[k];
			t->panel[k].x = px;
			t->panel[k].y = rowTop;
			t->panel[k].w = w;
			t->panel[k].h = y + 14.0f + 1.0f - rowTop;
			if ( t->panel[k].y + t->panel[k].h > bottom ) bottom = t->panel[k].y + t->panel[k].h;
		}
		/* the panels of a row as high as the highest */
		for ( k = 0; k < t->sideCount && k < MH_TAB_SIDES; k += 2 ) {
			if ( k + 1 >= t->sideCount ) break;
			h = t->panel[k].h > t->panel[k + 1].h ? t->panel[k].h : t->panel[k + 1].h;
			t->panel[k].h = t->panel[k + 1].h = h;
		}
	}

	/* the footer, 14 under: the spectators at the left, the server at the right */
	y = bottom + 14.0f;
	MH_Set( &t->specLabelT, &label11, t->spectators ? "SPECTATORS" : "" );
	MH_Set( &t->specT, &text14, t->spectators ? t->spectators : "" );
	MH_Set( &t->serverT, &label11, t->server ? t->server : "" );
	MH_Place( &t->specT, x + 2.0f + t->specLabelT.box.w + 12.0f, y );
	MH_Place( &t->specLabelT, x + 2.0f, y + MH_Base( &t->specT ) - MH_Base( &t->specLabelT ) );
	MH_Place( &t->serverT, x + MH_TAB_W - 2.0f - t->serverT.box.w, y + MH_Base( &t->specT ) - MH_Base( &t->serverT ) );
	MH_Fit( &t->specT, t->serverT.box.x - 24.0f - t->specT.box.x );
}

/* -------------------------------------------------------------------------
   the drag tournament's bracket (gen.py tourney_left, bracket)
   ------------------------------------------------------------------------- */

/* the frame only: the header and the footer of the end screen */
static void MH_EndFrame( mhEndLayout_t *e ) {
	wsTextStyle_t label13, label12;
	MH_Label( &label13, 13.0f );
	MH_Label( &label12, 12.0f );
	MH_Set( &e->titleT, &label13, e->title );
	MH_Set( &e->subtitleT, &label12, e->subtitle ? e->subtitle : "" );
	MH_Place( &e->titleT, MH_END_MARGIN, e->top + 48.0f );
	MH_Place( &e->subtitleT, MH_END_MARGIN + e->titleT.box.w + 16.0f,
	          e->titleT.box.y + MH_Base( &e->titleT ) - MH_Base( &e->subtitleT ) );
	MH_EndFooter( e );
}

/* a line of a card: the name (flexible), BOT, the heats won, 8 apart inside 10 */
static void MH_BracketLine( const mhBracketEntrant_t *in, mhText_t *name, mhText_t *bot, mhText_t *wins,
                            wsRect_t *row, float x, float y, float w, float rowH ) {
	wsTextStyle_t text15, text13, label9, mono14;
	float right = x + w - 10.0f;
	MH_Text( &text15, 15.0f );
	MH_Text( &text13, 13.0f );
	MH_Label( &label9, 9.0f );
	MH_Mono( &mono14, 14.0f );
	row->x = x;
	row->y = y;
	row->w = w;
	row->h = rowH;
	MH_Set( wins, &mono14, in->name && in->wins ? in->wins : "" );
	MH_Set( bot, &label9, in->name && in->bot ? "BOT" : "" );
	MH_Set( name, rowH >= 26.0f ? &text15 : &text13, in->name ? in->name : "-" );
	if ( wins->text[0] ) {
		MH_Place( wins, right - wins->box.w, y + ( rowH - wins->box.h ) * 0.5f );
		right = wins->box.x - 8.0f;
	}
	if ( bot->text[0] ) {
		MH_Place( bot, right - bot->box.w, y + ( rowH - bot->box.h ) * 0.5f );
		right = bot->box.x - 8.0f;
	}
	MH_Place( name, x + 10.0f, y + ( rowH - name->box.h ) * 0.5f );
	MH_Fit( name, right - name->box.x );
	name->box.w = right - name->box.x;
}

/* a card: 1px around, two lines, a rule between */
static void MH_BracketCard( mhBracketLayout_t *b, wsRect_t *card, wsRect_t *divider, const mhBracketEntrant_t *e2,
                            mhText_t *names, mhText_t *bots, mhText_t *wins, wsRect_t *rows, float x, float cy, float w, float rowH ) {
	float h = 2.0f * rowH + 1.0f, top = cy - h * 0.5f;
	card->x = x;
	card->y = top;
	card->w = w;
	card->h = h + 2.0f;
	MH_BracketLine( &e2[0], &names[0], &bots[0], &wins[0], &rows[0], x + 1.0f, top + 1.0f, w - 2.0f, rowH );
	divider->x = x + 1.0f;
	divider->y = top + 1.0f + rowH;
	divider->w = w - 2.0f;
	divider->h = 1.0f;
	MH_BracketLine( &e2[1], &names[1], &bots[1], &wins[1], &rows[1], x + 1.0f, top + 2.0f + rowH, w - 2.0f, rowH );
	(void)b;
}

static void MH_BracketLineRect( mhBracketLayout_t *b, float x, float y, float w, float h ) {
	if ( b->lineCount >= (int)( sizeof( b->line ) / sizeof( b->line[0] ) ) ) return;
	b->line[b->lineCount].x = x;
	b->line[b->lineCount].y = y;
	b->line[b->lineCount].w = w;
	b->line[b->lineCount].h = h;
	b->lineCount++;
}

void MH_BracketLayout( mhBracketLayout_t *b ) {
	mhEndLayout_t *e = &b->frame;
	wsTextStyle_t label12, label11, label10, big64, text22, text14, text16;
	float x = MH_END_MARGIN, y = e->top + MH_END_TOP, w = MH_END_LEFT_W;
	float x0 = MH_END_MARGIN + MH_END_LEFT_W + 40.0f, total, top = e->top + 172.0f, bottom = e->top + 760.0f;
	float cardW, rowH, gap, pitch, centre[MH_BRACKET_ROUNDS][MH_BRACKET_SLOTS / 2], cx, cy;
	int r, j, k, cols, first;

	MH_Label( &label12, 12.0f );
	MH_Label( &label11, 11.0f );
	MH_Label( &label10, 10.0f );
	MH_Big( &big64, 64.0f );
	MH_Text( &text22, 22.0f );
	MH_Text( &text14, 14.0f );
	MH_Text( &text16, 16.0f );
	MH_EndFrame( e );

	/* the left: the match up next, big; who rides it; your way; a hint */
	MH_Set( &b->upLabelT, &label12, b->upLabel ? b->upLabel : "" );
	MH_Set( &b->upNameT, &big64, b->upName ? b->upName : "" );
	MH_Place( &b->upLabelT, x, y );
	b->upLabelT.box.w = w;
	MH_Place( &b->upNameT, x, y + b->upLabelT.box.h + 8.0f );
	MH_Fit( &b->upNameT, w );
	b->upNameT.box.w = w;
	y = b->upNameT.box.y + b->upNameT.box.h + 22.0f;
	for ( k = 0; k < 2; k++ ) {
		MH_Set( &b->vsNameT[k], &text22, b->vsName[k] ? b->vsName[k] : "-" );
		MH_Set( &b->vsTagT[k], &label11, b->vsTag[k] ? b->vsTag[k] : "" );
		MH_Place( &b->vsNameT[k], x, y );
		MH_Fit( &b->vsNameT[k], w - 60.0f );
		MH_Place( &b->vsTagT[k], x + b->vsNameT[k].box.w + 10.0f, y + MH_Base( &b->vsNameT[k] ) - MH_Base( &b->vsTagT[k] ) );
		y += b->vsNameT[k].box.h + 6.0f;
		if ( k == 0 ) {
			MH_Set( &b->againstT, &label11, "AGAINST" );
			MH_Place( &b->againstT, x, y );
			b->againstT.box.w = w;
			y += b->againstT.box.h + 6.0f;
		}
	}
	y += 22.0f - 6.0f;
	b->sep[0].x = x;
	b->sep[0].y = y;
	b->sep[0].w = w;
	b->sep[0].h = 1.0f;
	y += 1.0f + 22.0f;
	if ( b->pathCount > 0 ) {
		y = MH_PathRows( &b->pathTitleT, b->pathRoundT, b->pathWhatT, b->pathTagT, b->pathRound, b->pathWhat, b->pathTag,
		                 b->pathCount, x, y, w );
	} else {
		MH_Set( &b->pathTitleT, &label12, "" );
	}
	if ( b->hintText ) {
		y += 22.0f;
		b->sep[1].x = x;
		b->sep[1].y = y;
		b->sep[1].w = w;
		b->sep[1].h = 1.0f;
		y += 1.0f + 22.0f;
		MH_Set( &b->hintLabelT, &label11, b->hintLabel ? b->hintLabel : "" );
		MH_Set( &b->hintTextT, &text14, b->hintText );
		MH_Place( &b->hintLabelT, x, y );
		b->hintLabelT.box.w = w;
		MH_Place( &b->hintTextT, x, y + b->hintLabelT.box.h + 6.0f );
		MH_Fit( &b->hintTextT, w );
		b->hintTextT.box.w = w;
	} else {
		memset( &b->sep[1], 0, sizeof( b->sep[1] ) );
		MH_Set( &b->hintLabelT, &label11, "" );
		MH_Set( &b->hintTextT, &text14, "" );
	}

	/* the bracket: a column a round and one for the winner, the cards
	 * evenly spaced in the first, each later one between the two it comes from */
	for ( b->rounds = 0; ( 2 << b->rounds ) <= b->size && b->rounds < MH_BRACKET_ROUNDS; b->rounds++ ) {
	}
	cols = b->rounds + 1;
	total = e->viewWidth - MH_END_MARGIN - x0;
	cardW = b->size >= 16 ? 176.0f : b->size >= 8 ? 210.0f : 240.0f;
	rowH = b->size >= 16 ? 22.0f : 30.0f;
	gap = ( total - cols * cardW ) / ( cols - 1 );
	first = b->size / 2;
	pitch = ( bottom - top ) / first;
	b->lineCount = 0;
	MH_Set( &b->upNextT, &label10, "" );
	for ( r = 0; r < b->rounds; r++ ) {
		cx = x0 + r * ( cardW + gap );
		for ( j = 0; j < b->size >> ( r + 1 ); j++ ) {
			cy = r == 0 ? top + ( j + 0.5f ) * pitch : ( centre[r - 1][2 * j] + centre[r - 1][2 * j + 1] ) * 0.5f;
			centre[r][j] = cy;
			MH_BracketCard( b, &b->card[r][j], &b->divider[r][j], &b->slot[r][2 * j], &b->nameT[r][2 * j], &b->botT[r][2 * j],
			                &b->winsT[r][2 * j], &b->row[r][2 * j], cx, cy, cardW, rowH );
			if ( r == b->nextRound && j == b->nextPair && !b->nextThird ) {
				MH_Set( &b->upNextT, &label10, "UP NEXT" );
				MH_Place( &b->upNextT, cx, b->card[r][j].y - 18.0f );
			}
			if ( r > 0 ) {	/* in from the two before: out of each, down the middle, in */
				float px = cx - gap, lo = centre[r - 1][2 * j], hi = centre[r - 1][2 * j + 1];
				MH_BracketLineRect( b, px, lo, gap * 0.5f, 1.0f );
				MH_BracketLineRect( b, px, hi, gap * 0.5f, 1.0f );
				MH_BracketLineRect( b, px + gap * 0.5f, lo, 1.0f, hi - lo );
				MH_BracketLineRect( b, px + gap * 0.5f, cy, gap * 0.5f, 1.0f );
			}
		}
	}
	for ( r = 0; r <= b->rounds; r++ ) {
		MH_Set( &b->headT[r], &label11, b->head[r] ? b->head[r] : "" );
		MH_Place( &b->headT[r], x0 + r * ( cardW + gap ), top - 40.0f );
	}

	/* the winner: out of the final to a card of its own, 56 high */
	cx = x0 + b->rounds * ( cardW + gap );
	cy = centre[b->rounds - 1][0];
	MH_BracketLineRect( b, cx - gap, cy, gap, 1.0f );
	b->winnerCard.x = cx;
	b->winnerCard.y = cy - 28.0f;
	b->winnerCard.w = cardW;
	b->winnerCard.h = 56.0f;
	b->trophy.x = cx + 1.0f + 12.0f;
	b->trophy.w = b->trophy.h = 20.0f;
	b->trophy.y = b->winnerCard.y + 2.0f + ( 56.0f - 3.0f - 20.0f ) * 0.5f;
	MH_Set( &b->winnerT, &text16, b->champion ? b->champion : "?" );
	MH_Place( &b->winnerT, b->trophy.x + 20.0f + 10.0f, b->winnerCard.y + 2.0f + ( 56.0f - 3.0f - b->winnerT.box.h ) * 0.5f );
	MH_Fit( &b->winnerT, cx + cardW - 13.0f - b->winnerT.box.x );

	/* third place under it */
	MH_Set( &b->thirdLabelT, &label10, "THIRD PLACE" );
	MH_Place( &b->thirdLabelT, cx, cy + 90.0f - 18.0f );
	MH_BracketCard( b, &b->thirdCard, &b->thirdDivider, b->third, b->thirdNameT, b->thirdBotT, b->thirdWinsT, b->thirdRow,
	                cx, cy + 90.0f + ( 2.0f * rowH + 1.0f ) * 0.5f, cardW, rowH );
}
