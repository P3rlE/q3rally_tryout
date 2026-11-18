/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The CSS of the WebUI's settings, rule for rule (see the header).
*/

#include "ui_local.h"
#include "ui_rally_configlayout.h"
#include "ui_rally_rivalslayout.h"

#define REM		WS_REM
#define LU		WS_LU

const char *configCaptureHelp = "Press a key, mouse button, mouse wheel or controller input. Escape cancels.";

#define CONFIG_GAP			LU( 0.65f * REM )		/* .config-actions: gap .65rem */
#define CONFIG_SELECT		( 2.0f * LU( 0.65f * REM ) + 2.0f + 21.0f )
#define CONFIG_INPUT		( 2.0f * LU( 0.65f * REM ) + 2.0f + WS_BODY_LINE )
#define CONFIG_RANGE		( 2.0f * 0.5f * REM + 2.0f + 16.0f )	/* padding .5rem 0, a 16px track */

/* flex-wrap from x, rows of height h; returns the height used */
static float Config_Flow( wsRect_t *box, int count, float x, float y, float width, float h, float gap ) {
	float cx = x, cy = y;
	int i;
	if ( !count ) return 0.0f;
	for ( i = 0; i < count; i++ ) {
		if ( cx > x && cx + box[i].w > x + width + 0.01f ) {
			cx = x;
			cy += h + gap;
		}
		box[i].x = cx;
		box[i].y = cy;
		box[i].h = h;
		cx += box[i].w + gap;
	}
	return cy + h - y;
}

/* white-space: pre-wrap; overflow-wrap: anywhere. Lines end at line feeds;
 * a line too long breaks after its last space (or a hyphen before a
 * letter), and where there is none, after the last character that fits. */
void Config_WrapPre( const wsTextStyle_t *style, const char *text, float width, configLines_t *lines ) {
	int start = 0, length = strlen( text );

	memset( lines, 0, sizeof( *lines ) );
	while ( start < length && lines->count < CONFIG_MAX_LINES ) {
		int end = start, fit = start, chance = -1, next, i;
		while ( end < length && text[end] != '\n' ) end++;
		/* the longest piece that fits; spaces at its end hang */
		for ( i = start + 1; i <= end; i++ ) {
			if ( text[i - 1] != ' ' && WS_TextWidth( style, text + start, i - start ) > width + 0.01f ) break;
			fit = i;
			if ( i < end && text[i - 1] == ' ' && text[i] != ' ' ) chance = i;
			if ( i < end && i - 1 > start && text[i - 1] == '-' && isalnum( (unsigned char)text[i - 2] ) && isalpha( (unsigned char)text[i] ) ) chance = i;
		}
		if ( fit >= end ) next = end;
		else if ( chance > start ) next = chance;
		else next = fit > start ? fit : start + 1;
		lines->start[lines->count] = start;
		lines->length[lines->count] = next - start;
		lines->count++;
		start = next;
		if ( start < length && text[start] == '\n' ) {
			start++;
			/* an empty line between two line feeds */
			if ( start < length && text[start] == '\n' && lines->count < CONFIG_MAX_LINES ) {
				lines->start[lines->count] = start;
				lines->length[lines->count] = 0;
				lines->count++;
			}
		}
	}
}

/* one .config-field: the label (margin-bottom .5rem) and its control;
 * positions relative to the field's top */
static float Config_Field( configLayout_t *l, configField_t *f, float width ) {
	wsTextStyle_t body;
	float y;
	WS_BodyStyle( &body );
	WS_Wrap( &body, f->label ? f->label : "", width, &f->labelLines );
	y = f->labelLines.count * WS_BODY_LINE;
	memset( &f->control, 0, sizeof( f->control ) );
	f->control.w = width;
	switch ( f->type ) {
	case CONFIG_SLIDER:
		/* output (display: block), then the range input in a line box: 2px
		 * margins, its baseline at the bottom of the margin box */
		f->outputY = y + 0.5f * REM;
		f->control.x = 2.0f;
		f->control.y = f->outputY + WS_BODY_LINE + 2.0f;
		f->control.h = CONFIG_RANGE;
		/* the line's strut reaches 4px below the baseline (as Chrome measures it) */
		return f->outputY + WS_BODY_LINE + CONFIG_RANGE + 4.0f + 4.0f;
	case CONFIG_FIELD:
		f->control.y = y + 0.5f * REM;
		f->control.h = CONFIG_INPUT;
		return f->control.y + f->control.h;
	case CONFIG_KEY:
		/* div.config-actions: margin 1rem 0 (collapsing with the label's .5rem) */
		f->control.y = y + REM;
		f->rebind.w = WS_ButtonWidth( f->text && f->text[0] ? f->text : "Unbound" );
		f->clear.w = WS_ButtonWidth( "Clear" );
		{
			wsRect_t pair[2];
			pair[0] = f->rebind;
			pair[1] = f->clear;
			f->control.h = Config_Flow( pair, 2, 0.0f, f->control.y, width, WS_BUTTON_HEIGHT, CONFIG_GAP );
			f->rebind = pair[0];
			f->clear = pair[1];
		}
		return f->control.y + f->control.h + REM;
	case CONFIG_LIST:
		/* pre: margin 1em 0, max-height 26rem, scrolls */
		Config_WrapPre( &body, f->text ? f->text : "", width, &f->lines );
		f->listHeight = f->lines.count * WS_BODY_LINE;
		f->control.y = y + REM;
		f->control.h = f->listHeight < 26.0f * REM ? f->listHeight : 26.0f * REM;
		return f->control.y + f->control.h + REM;
	default:	/* the selects */
		f->control.y = y + 0.5f * REM;
		f->control.h = CONFIG_SELECT;
		return f->control.y + f->control.h;
	}
}

static void Config_Dialog( configLayout_t *l, float width, float height ) {
	wsTextStyle_t body, label;
	float y, top, dy;
	int i;

	WS_BodyStyle( &body );
	WS_LabelStyle( &label );
	/* width: min(42rem, 100vw - 2rem); padding 2rem; border-top 3px; centred */
	l->dialogBox.w = width - 2.0f * REM < 42.0f * REM ? width - 2.0f * REM : 42.0f * REM;
	l->dialogBox.x = ( l->page.pageWidth - l->dialogBox.w ) * 0.5f;
	l->dialogX = l->dialogBox.x + 1.0f + 2.0f * REM;
	l->dialogW = l->dialogBox.w - 2.0f - 4.0f * REM;
	y = 3.0f + 2.0f * REM;
	l->dialogLabel = l->dialog == CONFIG_CONFIRM ? "Q3RALLY / CONFIRM CHANGE" : "Q3RALLY / CONTROLS";
	l->dialogTitle = l->dialog == CONFIG_CONFIRM ? ( l->description && l->description[0] ? l->description : "Confirm change" ) : "Choose a key";
	l->dialogLabelY = y;
	/* h2: margin .75rem 0 */
	l->dialogH2Y = y + LU( WS_LABEL_LINE ) + 0.75f * REM;
	WS_StyledWrap( &l->h2, l->dialogTitle, l->dialogW, &l->dialogH2Lines );
	y = l->dialogH2Y + l->dialogH2Lines.count * l->h2.size + REM;
	if ( l->dialog == CONFIG_CONFIRM ) {
		/* the empty message's margins collapse into the buttons' */
		for ( i = 0; i < l->buttons; i++ ) l->button[i].w = WS_ButtonWidth( l->buttonLabel[i] );
		y += Config_Flow( l->button, l->buttons, l->dialogX, y, l->dialogW, WS_BUTTON_HEIGHT, CONFIG_GAP );
	} else {
		l->dialogActionY = y;
		WS_Wrap( &body, l->captureAction ? l->captureAction : "", l->dialogW, &l->dialogActionLines );
		if ( l->dialogActionLines.count ) y += l->dialogActionLines.count * WS_BODY_LINE + REM;
		l->dialogHelpY = y;
		WS_Wrap( &body, configCaptureHelp, l->dialogW, &l->dialogHelpLines );
		y += l->dialogHelpLines.count * WS_BODY_LINE + REM;
		l->cancel.w = WS_ButtonWidth( "Cancel" );
		y += Config_Flow( &l->cancel, 1, l->dialogX, y, l->dialogW, WS_BUTTON_HEIGHT, CONFIG_GAP );
	}
	l->dialogBox.h = y + REM + 2.0f * REM + 1.0f;
	if ( l->dialogBox.h > height - 3.0f * REM ) l->dialogBox.h = height - 3.0f * REM;
	/* centred in the viewport */
	top = ( height - l->dialogBox.h ) * 0.5f;
	if ( top < 0.0f ) top = 0.0f;
	dy = top;
	l->dialogBox.y = top;
	l->dialogLabelY += dy; l->dialogH2Y += dy; l->dialogActionY += dy; l->dialogHelpY += dy;
	l->cancel.y += dy;
	for ( i = 0; i < l->buttons && l->dialog == CONFIG_CONFIRM; i++ ) l->button[i].y += dy;
}

void Config_Layout( configLayout_t *l, float width, float height ) {
	wsTextStyle_t body;
	float top, y, pad, column;
	int i, row;

	WS_BodyStyle( &body );
	l->headerLabel = l->screen >= 10 ? "Q3RALLY / IN RACE" : "Q3RALLY / CONFIGURATION";
	l->backLabel = l->screen == 10 ? "Resume race" : "Back";
	WS_FrontPageLabeled( &l->page, width, height, l->headerLabel, l->title ? l->title : "", "", "" );
	top = WS_PlaceHeaderButton( &l->page, l->backLabel, &l->back );
	pad = l->page.sectionPad;
	l->h2.font = WS_CONDENSED;
	l->h2.size = l->h2.lineHeight = LU( WS_Clamp( 1.5f * REM, 0.04f * width, 3.0f * REM ) );
	l->h2.spacing = 0.0f;
	l->h2.uppercase = qtrue;

	/* p#config-description (pre-wrap), then nav.config-actions (a flex box:
	 * margin 1rem 0, its own margins never collapse together) */
	Config_WrapPre( &body, l->description ? l->description : "", l->page.contentW, &l->descriptionLines );
	l->descriptionY = top;
	y = top;
	if ( l->descriptionLines.count ) y += l->descriptionLines.count * WS_BODY_LINE + REM;
	l->nav.x = l->page.contentX;
	l->nav.w = l->page.contentW;
	l->nav.y = y;
	l->nav.h = 0.0f;
	if ( l->dialog != CONFIG_CONFIRM ) {
		for ( i = 0; i < l->buttons; i++ ) l->button[i].w = WS_ButtonWidth( l->buttonLabel[i] );
		l->nav.h = Config_Flow( l->button, l->buttons, l->nav.x, y, l->nav.w, WS_BUTTON_HEIGHT, CONFIG_GAP );
	}
	y = l->nav.y + l->nav.h + REM;

	/* section#config-settings > .config-fields:
	 * repeat(auto-fit, minmax(min(100%, 19rem), 1fr)), gap 1.5rem; auto-fit
	 * drops the empty columns */
	l->settings = l->fields > 0 && l->dialog != CONFIG_CONFIRM;
	l->section.x = l->page.contentX;
	l->section.w = l->page.contentW;
	l->section.y = y;
	l->section.h = 0.0f;
	l->fieldsX = l->section.x + 1.0f + pad;
	l->fieldsW = l->section.w - 2.0f - 2.0f * pad;
	l->columns = (int)( ( l->fieldsW + 1.5f * REM ) / ( 19.0f * REM + 1.5f * REM ) );
	if ( l->columns < 1 ) l->columns = 1;
	if ( l->columns > l->fields ) l->columns = l->fields;
	if ( l->settings ) {
		float rowTop = y + 1.0f + pad, h[CONFIG_MAX_FIELDS];
		column = ( l->fieldsW - ( l->columns - 1 ) * 1.5f * REM ) / l->columns;
		for ( row = 0; row * l->columns < l->fields; row++ ) {
			float rowH = 0.0f;
			for ( i = row * l->columns; i < l->fields && i < ( row + 1 ) * l->columns; i++ ) {
				h[i] = Config_Field( l, &l->field[i], column );
				if ( h[i] > rowH ) rowH = h[i];
			}
			for ( i = row * l->columns; i < l->fields && i < ( row + 1 ) * l->columns; i++ ) {
				configField_t *f = &l->field[i];
				f->box.x = l->fieldsX + ( i - row * l->columns ) * ( column + 1.5f * REM );
				f->box.y = rowTop;
				f->box.w = column;
				f->box.h = rowH;
				f->control.x += f->box.x;
				f->control.y += rowTop;
				f->outputY += rowTop;
				f->rebind.x += f->box.x; f->clear.x += f->box.x;
				f->rebind.y += rowTop; f->clear.y += rowTop;
			}
			rowTop += rowH + 1.5f * REM;
		}
		l->section.h = rowTop - 1.5f * REM + pad + 1.0f - y;
		y += l->section.h;
	}
	l->pageHeight = y + l->page.pad;

	/* the page's scroll */
	l->viewport.x = 0.0f; l->viewport.y = 0.0f; l->viewport.w = width; l->viewport.h = height;
	l->pageBar = l->pageHeight > height + 0.5f;
	l->pageScroll = l->pageBar ? WS_Clamp( 0.0f, floor( l->pageScroll + 0.5f ), floor( l->pageHeight - height + 0.5f ) ) : 0.0f;
	memset( &l->pageThumb, 0, sizeof( l->pageThumb ) );
	if ( l->pageBar ) {
		float dy = -l->pageScroll;
		WS_ScrollThumb( &l->viewport, l->pageHeight, l->pageScroll, &l->pageThumb );
		l->page.headerY += dy; l->page.h1Y += dy; l->back.y += dy;
		l->descriptionY += dy;
		l->nav.y += dy;
		if ( l->dialog != CONFIG_CONFIRM ) for ( i = 0; i < l->buttons; i++ ) l->button[i].y += dy;
		l->section.y += dy;
		for ( i = 0; i < l->fields && l->settings; i++ ) {
			configField_t *f = &l->field[i];
			f->box.y += dy; f->control.y += dy; f->outputY += dy; f->rebind.y += dy; f->clear.y += dy;
		}
	}
	if ( l->dialog != CONFIG_NO_DIALOG ) Config_Dialog( l, width, height );
}
