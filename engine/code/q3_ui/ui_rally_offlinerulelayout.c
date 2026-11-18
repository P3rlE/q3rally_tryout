/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS of step 2 of the WebUI's offline race setup, rule for rule (see the
header).
*/

#include "ui_local.h"
#include "ui_rally_offlinerulelayout.h"
#include "ui_rally_demoslayout.h"
#include "ui_rally_rivalslayout.h"

#define REM		WS_REM
#define LU		WS_LU

/* a select's height: padding .65rem, a border, and Chrome's menulist line
 * (the font's ascent and descent, each rounded, and a pixel above and below) */
static float Rules_SelectHeight( float size ) {
	return 2.0f * LU( 0.65f * REM ) + 2.0f + floor( size + 0.5f ) + floor( 0.2f * size + 0.5f ) + 2.0f;
}

#define RULES_INPUT_HEIGHT	( 2.0f * LU( 0.65f * REM ) + 2.0f + WS_BODY_LINE )

typedef struct {
	float	grow, base, min, width, height, margin;
	qboolean frozen;
} rulesFlex_t;

/* one .offline-player: flex, wrap, align-items center */
static void Rules_Row( rulesLayout_t *l, int index, float width ) {
	rulesRow_t *row = &l->row[index];
	rulesFlex_t flex[RULES_MAX_PARTS];
	wsTextStyle_t label;
	float gap = l->head.desktop ? 8.0f : LU( 0.6f * REM ), pad = l->head.desktop ? 0.75f * REM : REM;
	float labelMin = l->head.desktop ? 5.5f * REM : 6.0f * REM, labelMargin = l->head.desktop ? 0.0f : REM;
	float y = pad, imageW = l->head.desktop ? 3.0f * REM : 4.0f * REM;
	float select = Rules_SelectHeight( 16.0f );
	char shown[96];
	int i, first, last, n = l->rowParts[index], k;

	WS_LabelStyle( &label );
	row->parts = n;
	row->text = l->rowText[index];
	for ( i = 0; i < n; i++ ) {
		rulesItem_t *part = &row->part[i];
		rulesFlex_t *f = &flex[i];
		memset( part, 0, sizeof( *part ) );
		memset( f, 0, sizeof( *f ) );
		part->kind = l->rowKind[index][i];
		switch ( part->kind ) {
		case RULES_P:
			if ( !l->head.desktop ) { f->base = width; f->min = 0.0f; }
			else if ( l->rowHuman[index] ) { f->grow = 1.0f; }
			else { f->base = 4.5f * REM; }
			f->height = LU( WS_LABEL_LINE );
			break;
		case RULES_AVATAR:
		case RULES_CAR:
			f->base = imageW;
			f->height = 3.0f * REM;
			break;
		case RULES_BOT:
			/* flex: 2, its basis 0 plus padding and border */
			f->grow = 2.0f;
			f->base = 2.0f * LU( 0.65f * REM ) + 2.0f;
			f->height = select;
			break;
		default:
			f->grow = 1.0f;
			f->min = labelMin;
			f->margin = labelMargin;
			break;
		}
	}
	/* lines: break where the hypothetical sizes overflow */
	for ( first = 0; first < n; first = last ) {
		float used = 0.0f, avail, lineH = 0.0f;
		for ( last = first; last < n; last++ ) {
			float hyp = flex[last].base > flex[last].min ? flex[last].base : flex[last].min;
			if ( last > first && used + gap + hyp > width + 0.01f ) break;
			used += ( last > first ? gap : 0.0f ) + hyp;
		}
		avail = width - gap * ( last - first - 1 );
		for ( k = 0; k < 8; k++ ) {
			float free = avail, grow = 0.0f;
			qboolean violation = qfalse;
			for ( i = first; i < last; i++ ) {
				if ( flex[i].frozen ) free -= flex[i].width;
				else { free -= flex[i].base; grow += flex[i].grow; }
			}
			for ( i = first; i < last; i++ ) {
				if ( flex[i].frozen ) continue;
				flex[i].width = flex[i].base + ( grow > 0.0f && free > 0.0f ? free * flex[i].grow / grow : 0.0f );
			}
			for ( i = first; i < last; i++ ) {
				if ( !flex[i].frozen && flex[i].width < flex[i].min ) {
					flex[i].width = flex[i].min;
					flex[i].frozen = violation = qtrue;
				}
			}
			if ( !violation ) break;
		}
		/* heights: a label is its text, a .5rem gap and its select */
		for ( i = first; i < last; i++ ) {
			rulesItem_t *part = &row->part[i];
			if ( part->kind == RULES_P ) {
				WS_Transform( &label, row->text ? row->text : "", shown, sizeof( shown ) );
				Demos_WrapAnywhere( &label, shown, flex[i].width, &part->lines );
				if ( part->lines.count > 1 ) flex[i].height = part->lines.count * LU( WS_LABEL_LINE );
			} else if ( part->kind == RULES_TYPE || part->kind == RULES_TEAM ) {
				WS_Wrap( &l->rowLabel, part->kind == RULES_TYPE ? "Slot type" : "Team", flex[i].width, &part->lines );
				flex[i].height = part->lines.count * l->rowLabel.lineHeight + 0.5f * REM + l->selectHeight;
			}
			if ( flex[i].height + flex[i].margin > lineH ) lineH = flex[i].height + flex[i].margin;
		}
		{
			float x = 0.0f;
			for ( i = first; i < last; i++ ) {
				rulesItem_t *part = &row->part[i];
				part->box.x = x;
				part->box.w = flex[i].width;
				part->box.h = flex[i].height;
				part->box.y = y + ( lineH - flex[i].height - flex[i].margin ) * 0.5f;
				if ( part->kind == RULES_AVATAR || part->kind == RULES_CAR ) {
					/* object-fit: contain, the pictures are square */
					part->control = part->box;
					part->control.w = part->box.h;
					part->control.x = part->box.x + ( part->box.w - part->box.h ) * 0.5f;
				} else if ( part->kind == RULES_TYPE || part->kind == RULES_TEAM ) {
					part->control.x = part->box.x;
					part->control.w = part->box.w;
					part->control.h = l->selectHeight;
					part->control.y = part->box.y + part->box.h - l->selectHeight;
				}
				x += flex[i].width + gap;
			}
		}
		y += lineH + ( last < n ? gap : 0.0f );
	}
	row->h = y + pad + 1.0f;
}

/* #offline-options: two columns on the desktop, auto-fit 12rem below */
static float Rules_Fields( rulesLayout_t *l, float width ) {
	wsTextStyle_t body;
	int columns, i, j, rows;
	float column, gapX = REM, gapY = l->head.desktop ? 0.75f * REM : REM, margin = l->head.desktop ? 0.0f : REM, y = 0.0f;

	WS_BodyStyle( &body );
	if ( l->head.desktop ) columns = 2;
	else {
		columns = (int)( ( width + gapX ) / ( 12.0f * REM + gapX ) );
		if ( columns < 1 ) columns = 1;
		if ( columns > l->fields && l->fields > 0 ) columns = l->fields;
	}
	column = ( width - ( columns - 1 ) * gapX ) / columns;
	rows = ( l->fields + columns - 1 ) / columns;
	for ( j = 0; j < rows; j++ ) {
		float rowH = 0.0f, own[8], text[8];
		for ( i = j * columns; i < l->fields && i < ( j + 1 ) * columns; i++ ) {
			float control = l->fieldInput[i] ? RULES_INPUT_HEIGHT : Rules_SelectHeight( 16.0f );
			WS_Wrap( &body, l->fieldText[i], column, &l->fieldLines[i] );
			text[i - j * columns] = ( l->fieldLines[i].count > 0 ? l->fieldLines[i].count : 1 ) * WS_BODY_LINE;
			own[i - j * columns] = text[i - j * columns] + 0.5f * REM + control;
			l->fieldControl[i].h = control;
			if ( own[i - j * columns] + margin > rowH ) rowH = own[i - j * columns] + margin;
		}
		for ( i = j * columns; i < l->fields && i < ( j + 1 ) * columns; i++ ) {
			/* stretched: the extra height goes half to the text, half to the control */
			float extra = rowH - margin - own[i - j * columns];
			l->fieldBox[i].x = ( i - j * columns ) * ( column + gapX );
			l->fieldBox[i].y = y;
			l->fieldBox[i].w = column;
			l->fieldBox[i].h = rowH - margin;
			l->fieldControl[i].x = l->fieldBox[i].x;
			l->fieldControl[i].w = column;
			l->fieldControl[i].y = y + text[i - j * columns] + 0.5f * extra + 0.5f * REM;
			l->fieldControl[i].h += 0.5f * extra;
		}
		y += rowH + ( j < rows - 1 ? gapY : 0.0f );
	}
	return y;
}

static void Rules_Bar( const wsRect_t *port, float content, float *scroll, qboolean *bar, wsRect_t *thumb ) {
	*bar = content > port->h + 0.5f;
	*scroll = *bar ? WS_Clamp( 0.0f, floor( *scroll + 0.5f ), floor( content - port->h + 0.5f ) ) : 0.0f;
	memset( thumb, 0, sizeof( *thumb ) );
	if ( *bar ) WS_ScrollThumb( port, content, *scroll, thumb );
}

void Rules_Layout( rulesLayout_t *l, float width, float height ) {
	offlineHeader_t *h = &l->head;
	wsTextStyle_t body;
	float top, pad, column1, column2, y, divW, divH, inner, titleW, gap;
	char shown[96];
	int i;

	top = Offline_Header( h, width, height, l->hosting, "Back to tracks" );
	WS_BodyStyle( &body );
	l->rowLabel = body;
	if ( h->desktop ) {
		l->rowLabel.size = 0.85f * REM;
		l->rowLabel.lineHeight = LU( 1.5f * 0.85f * REM );
	}
	l->selectHeight = Rules_SelectHeight( l->rowLabel.size );
	pad = h->desktop ? 1.25f * REM : WS_Clamp( REM, 0.03f * width, 2.0f * REM );

	/* .offline-summary: a section, flex, wrap, centred, space-between, gap 1rem */
	l->summary.x = h->contentX;
	l->summary.w = h->contentW;
	l->summary.y = top;
	inner = h->contentW - 2.0f - 2.0f * pad;
	l->titleStyle.font = WS_CONDENSED;
	l->titleStyle.size = h->desktop ? 2.0f * REM : LU( WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM ) );
	l->titleStyle.spacing = 0.0f;
	l->titleStyle.lineHeight = l->titleStyle.size;
	l->titleStyle.uppercase = qtrue;
	WS_Transform( &l->titleStyle, l->title, shown, sizeof( shown ) );
	titleW = WS_TextWidth( &l->titleStyle, shown, strlen( shown ) );
	divW = titleW;
	{
		wsTextStyle_t label;
		WS_LabelStyle( &label );
		if ( WS_StyledWidth( &label, "YOUR RACE" ) > divW ) divW = WS_StyledWidth( &label, "YOUR RACE" );
		if ( WS_TextWidth( &body, l->modeName, strlen( l->modeName ) ) > divW ) divW = WS_TextWidth( &body, l->modeName, strlen( l->modeName ) );
	}
	if ( divW > inner ) divW = inner;
	l->start.w = WS_ButtonWidth( l->hosting ? "Start server" : "Start race" );
	l->start.h = WS_BUTTON_HEIGHT;
	WS_Wrap( &l->titleStyle, shown, divW + 0.01f, &l->titleLines );
	l->summaryLabelY = top + 1.0f + ( h->desktop ? REM : pad );
	l->titleY = l->summaryLabelY + LU( WS_LABEL_LINE ) + WS_HEADING_MARGIN;
	l->modeY = l->titleY + l->titleLines.count * l->titleStyle.size + ( h->desktop ? WS_HEADING_MARGIN : REM );
	divH = l->modeY + WS_BODY_LINE + ( h->desktop ? 0.0f : REM ) - l->summaryLabelY;
	l->start.x = l->summary.x + l->summary.w - 1.0f - pad - l->start.w;
	if ( divW + REM + l->start.w <= inner ) {
		l->start.y = l->summaryLabelY + ( divH - l->start.h ) * 0.5f;
		l->summary.h = 2.0f + 2.0f * ( h->desktop ? REM : pad ) + divH;
	} else {
		/* the button wraps below, at the start of its line */
		l->start.x = l->summary.x + 1.0f + pad;
		l->start.y = l->summaryLabelY + divH + REM;
		l->summary.h = 2.0f + 2.0f * ( h->desktop ? REM : pad ) + divH + REM + l->start.h;
	}
	top = l->summary.y + l->summary.h + REM;

	/* .offline-rules-layout: minmax(0, 1fr) minmax(0, 1.5fr), gap 1rem; one column up to 48rem */
	if ( width > 48.0f * REM ) {
		column1 = ( h->contentW - REM ) / 2.5f;
		column2 = h->contentW - REM - column1;
	} else {
		column1 = column2 = h->contentW;
	}
	l->left.x = h->contentX;
	l->left.w = column1;
	l->right.x = width > 48.0f * REM ? h->contentX + column1 + REM : h->contentX;
	l->right.w = column2;
	l->leftX = l->left.x + 1.0f + pad;
	l->leftW = column1 - 2.0f - 2.0f * pad;
	l->rightX = l->right.x + 1.0f + pad;
	l->rightW = column2 - 2.0f - 2.0f * pad;

	/* both columns: the h2 (1.75rem on the desktop), the help, the content */
	l->h2Style = l->titleStyle;
	l->h2Style.size = l->h2Style.lineHeight = h->desktop ? 1.75f * REM : l->titleStyle.size;
	gap = h->desktop ? WS_HEADING_MARGIN + 0.5f * REM : REM;	/* flex: the margins add up */
	l->rulesH2Y = l->gridH2Y = 1.0f + pad + WS_HEADING_MARGIN;
	l->ruleHelpY = l->gridHelpY = l->rulesH2Y + l->h2Style.size + gap;
	WS_Wrap( &body, l->ruleHelp ? l->ruleHelp : "", l->leftW, &l->ruleHelpLines );
	WS_Wrap( &body, l->gridHelp ? l->gridHelp : "", l->rightW, &l->gridHelpLines );
	l->options.x = l->leftX;
	l->options.w = l->leftW;
	l->options.y = l->ruleHelpY + l->ruleHelpLines.count * WS_BODY_LINE + REM;
	l->players.x = l->rightX;
	l->players.w = l->rightW;
	l->players.y = l->gridHelpY + l->gridHelpLines.count * WS_BODY_LINE + REM;
	l->optionsHeight = Rules_Fields( l, l->leftW );

	/* on the desktop the avatar comes first (order: -1) */
	for ( i = 0; i < l->rows && h->desktop; i++ ) {
		int j;
		for ( j = 1; j < l->rowParts[i]; j++ ) {
			if ( l->rowKind[i][j] == RULES_AVATAR ) {
				l->rowKind[i][j] = l->rowKind[i][0];
				l->rowKind[i][0] = RULES_AVATAR;
			}
		}
	}
	l->rowWidth = l->rightW;
	for ( i = 0, y = 0.0f; i < l->rows; i++ ) {
		Rules_Row( l, i, l->rowWidth );
		l->row[i].y = y;
		y += l->row[i].h;
	}
	l->playersHeight = y;

	if ( h->desktop ) {
		float sectionH = height - h->padY - top, bottom = sectionH - 1.0f - pad;
		l->left.y = l->right.y = top;
		l->left.h = l->right.h = sectionH;
		l->options.h = bottom - l->options.y;
		if ( l->options.h > l->optionsHeight ) l->options.h = l->optionsHeight;
		if ( l->options.h < 0.0f ) l->options.h = 0.0f;
		l->players.h = bottom - l->players.y;
		if ( l->players.h < 0.0f ) l->players.h = 0.0f;
		if ( l->playersHeight > l->players.h + 0.5f && !l->hideScrollbar ) {
			/* the grid's scrollbar takes its room from the rows */
			l->rowWidth = l->rightW - RIVALS_SCROLLBAR;
			for ( i = 0, y = 0.0f; i < l->rows; i++ ) {
				Rules_Row( l, i, l->rowWidth );
				l->row[i].y = y;
				y += l->row[i].h;
			}
			l->playersHeight = y;
		}
		l->pageHeight = height;
	} else {
		float leftH, rightH;
		l->options.h = l->optionsHeight;
		l->players.h = l->playersHeight;
		leftH = l->options.y + l->options.h + pad + 1.0f;
		rightH = l->players.y + l->players.h + pad + 1.0f;
		l->left.y = top;
		if ( width > 48.0f * REM ) {
			if ( leftH < rightH ) leftH = rightH;
			rightH = leftH;
			l->right.y = top;
		} else {
			l->right.y = top + leftH + REM;
		}
		l->left.h = leftH;
		l->right.h = rightH;
		/* the empty status line's margins, then the page's padding */
		l->pageHeight = l->right.y + l->right.h + WS_BLOCK_MARGIN + h->padY;
	}

	/* section-relative to the screen, then the page's own scroll */
	l->rulesH2Y += l->left.y;
	l->ruleHelpY += l->left.y;
	l->options.y += l->left.y;
	l->gridH2Y += l->right.y;
	l->gridHelpY += l->right.y;
	l->players.y += l->right.y;
	l->pageBar = l->pageHeight > height + 0.5f;
	l->pageScroll = l->pageBar ? WS_Clamp( 0.0f, floor( l->pageScroll + 0.5f ), floor( l->pageHeight - height + 0.5f ) ) : 0.0f;
	l->page.x = 0.0f; l->page.y = 0.0f; l->page.w = width; l->page.h = height;
	memset( &l->pageThumb, 0, sizeof( l->pageThumb ) );
	if ( l->pageBar ) {
		float dy = -l->pageScroll;
		WS_ScrollThumb( &l->page, l->pageHeight, l->pageScroll, &l->pageThumb );
		h->labelY += dy; h->h1Y += dy; h->back.y += dy; h->stepY += dy;
		l->summary.y += dy; l->summaryLabelY += dy; l->titleY += dy; l->modeY += dy; l->start.y += dy;
		l->left.y += dy; l->right.y += dy;
		l->rulesH2Y += dy; l->ruleHelpY += dy; l->options.y += dy;
		l->gridH2Y += dy; l->gridHelpY += dy; l->players.y += dy;
	}
	Rules_Bar( &l->options, l->optionsHeight, &l->optionsScroll, &l->optionsBar, &l->optionsThumb );
	Rules_Bar( &l->players, l->playersHeight, &l->playersScroll, &l->playersBar, &l->playersThumb );
}
