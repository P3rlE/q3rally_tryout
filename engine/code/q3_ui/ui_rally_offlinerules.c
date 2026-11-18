/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Step 2 of the offline race setup, rules and drivers, drawn like the WebUI's
#offline-rules (offline.js, options()) over the old server options menu. It
reads the model the page was fed from and acts through UI_WebOfflineAction:
4 sets a control, 5 picks a bot, 6 starts, 7 the host name, 0 goes back.
*/

#include "ui_local.h"
#include "ui_rally_offline.h"
#include "ui_rally_offlinerulelayout.h"
#include "ui_rally_library.h"
#include "ui_rally_rivalslayout.h"

#define RULES_MAX_CONTROLS	64
#define RULES_MAX_CHOICES	32
#define RULES_MAX_BOTS		128
#define RULES_POPUP_ROWS	20		/* Chrome's popup shows up to 20 options */

static const vec4_t rulesInput  = { 0x22 / 255.0f, 0x22 / 255.0f, 0x22 / 255.0f, 1.0f };
static const vec4_t rulesRing   = { 0xee / 255.0f, 0xee / 255.0f, 0xee / 255.0f, 1.0f };
static const vec4_t rulesRow    = { 0x33 / 255.0f, 0x33 / 255.0f, 0x33 / 255.0f, 1.0f };	/* border-bottom #333 */
static const vec4_t rulesMuted  = { 0xaa / 255.0f, 0xaa / 255.0f, 0xaa / 255.0f, 1.0f };
static const vec4_t rulesPopup  = { 0x1f / 255.0f, 0x1f / 255.0f, 0x1f / 255.0f, 1.0f };
static const vec4_t rulesPopupLine = { 0x3b / 255.0f, 0x3b / 255.0f, 0x3b / 255.0f, 1.0f };

static const char *rulesDragRules = "Equal vehicle performance. A match is the best of the chosen heats; the Ladder keeps your best "
	"heat. A tournament fills a knockout bracket with bots; their matches can be skipped. Selecting first before green, or "
	"less than 0.100 s after it, is a false start.";
static const char *rulesDragGrid = "Race a human or a bot, or practise solo. Hold 4000-5000 RPM in N, select first on green, "
	"shift up before the limiter: too long in the red blows the engine.";

typedef struct {
	int		index, type, value, slot, maxchars, choices;
	qboolean disabled, team;
	char	label[64], text[64];
} rulesControl_t;

typedef enum { DRAG_NONE, DRAG_OPTIONS, DRAG_PLAYERS, DRAG_PAGE } rulesDrag_t;

static struct {
	rulesLayout_t	layout;
	rulesControl_t	control[RULES_MAX_CONTROLS];
	int				controls;
	int				field[RULES_MAX_FIELDS];		/* control per field */
	char			fieldText[RULES_MAX_FIELDS][64];
	/* per row */
	int				slot[RULES_MAX_ROWS], typeCtl[RULES_MAX_ROWS], teamCtl[RULES_MAX_ROWS], bot[RULES_MAX_ROWS];
	char			rowText[RULES_MAX_ROWS][48], botText[RULES_MAX_ROWS][48];
	qhandle_t		car[RULES_MAX_ROWS], avatar;
	char			players[12][32];
	int				bots, sorted[RULES_MAX_BOTS];
	char			botName[RULES_MAX_BOTS][32], botModel[RULES_MAX_BOTS][48];
	char			modes[64];
	qboolean		dedicated;
	/* state */
	float			optionsScroll, playersScroll, pageScroll;
	rulesDrag_t		drag;
	float			dragY, dragScroll;
	int				edit;							/* the control being typed into, or -1 */
	char			editText[64];
	int				editTime;
	/* the open option list: a control's or a slot's bots */
	qboolean		popup;
	int				popupCtl, popupSlot, popupRow, popupCount, popupCurrent;
	float			popupScroll;
	wsRect_t		popupAnchor;
	float			popupSize;
} s_rules;

static const char *Rules_Clean( const char *in, char *out, int size ) {
	int n = 0;
	for ( ; *in && n < size - 1; in++ ) {
		if ( *in == '^' && in[1] >= '0' && in[1] <= '9' ) { in++; continue; }
		out[n++] = *in;
	}
	out[n] = '\0';
	return out;
}

static qhandle_t Rules_CarShader( const char *model ) {
	char vehicle[48], *paint;
	Q_strncpyz( vehicle, model, sizeof( vehicle ) );
	paint = strchr( vehicle, '/' );
	if ( !paint || !vehicle[0] ) return 0;
	*paint++ = '\0';
	return trap_R_RegisterShaderNoMip( va( "models/players/%s/icon_%s", vehicle, paint ) );
}

/* the model as options() saw it */
static void Rules_Build( void ) {
	rulesLayout_t *l = &s_rules.layout;
	char text[256], clean[64];
	int i, j, slot, mode, selected;
	qboolean drag, allowsBots;
	static char title[96], modeName[64];

	memset( l, 0, sizeof( *l ) );
	l->optionsScroll = s_rules.optionsScroll;
	l->playersScroll = s_rules.playersScroll;
	l->pageScroll = s_rules.pageScroll;
	l->hosting = UI_WebOfflineNumber( 14, 0 ) != 0;
	s_rules.dedicated = UI_WebOfflineNumber( 15, 0 ) != 0;
	allowsBots = UI_WebOfflineNumber( 12, 0 ) != 0;
	mode = UI_WebOfflineNumber( 1, 0 );
	selected = UI_WebOfflineNumber( 2, 0 );
	Offline_Text( 3, mode, 0, modeName, sizeof( modeName ) );
	Offline_Text( 1, selected, 0, title, sizeof( title ) );
	Q_strncpyz( l->title, title[0] ? title : "Your race", sizeof( l->title ) );
	Q_strncpyz( l->modeName, modeName, sizeof( l->modeName ) );
	drag = !Q_stricmp( modeName, "Drag Race" );
	l->ruleHelp = drag ? rulesDragRules : "Zero means no limit.";
	l->gridHelp = drag ? rulesDragGrid : allowsBots ? "Choose your opponents and teams." : "This mode is driven solo. Bots are disabled.";

	/* the controls */
	s_rules.controls = 0;
	for ( i = 0; i < UI_WebOfflineNumber( 3, 0 ) && s_rules.controls < RULES_MAX_CONTROLS; i++ ) {
		rulesControl_t *c = &s_rules.control[s_rules.controls];
		c->type = UI_WebOfflineNumber( 4, i );
		if ( !c->type ) continue;
		c->index = i;
		c->value = UI_WebOfflineNumber( 5, i );
		c->choices = UI_WebOfflineNumber( 6, i );
		c->disabled = UI_WebOfflineNumber( 7, i ) != 0;
		c->slot = UI_WebOfflineNumber( 9, i );
		c->team = UI_WebOfflineNumber( 10, i ) != 0;
		c->maxchars = UI_WebOfflineNumber( 13, i );
		Offline_Text( 4, i, 0, c->label, sizeof( c->label ) );
		Offline_Text( 5, i, 0, c->text, sizeof( c->text ) );
		s_rules.controls++;
	}
	for ( i = 0; i < 12; i++ ) Offline_Text( 8, i, 0, s_rules.players[i], sizeof( s_rules.players[i] ) );
	s_rules.bots = UI_WebOfflineNumber( 8, 0 );
	if ( s_rules.bots > RULES_MAX_BOTS ) s_rules.bots = RULES_MAX_BOTS;
	for ( i = 0; i < s_rules.bots; i++ ) {
		Offline_Text( 7, i, 0, text, sizeof( text ) );
		Rules_Clean( text, s_rules.botName[i], sizeof( s_rules.botName[i] ) );
		Offline_Text( 9, i, 0, s_rules.botModel[i], sizeof( s_rules.botModel[i] ) );
		s_rules.sorted[i] = i;
	}
	/* bots.slice().sort((a, b) => a.name.localeCompare(b.name)) */
	for ( i = 1; i < s_rules.bots; i++ ) {
		int key = s_rules.sorted[i];
		for ( j = i - 1; j >= 0 && Q_stricmp( s_rules.botName[s_rules.sorted[j]], s_rules.botName[key] ) > 0; j-- ) {
			s_rules.sorted[j + 1] = s_rules.sorted[j];
		}
		s_rules.sorted[j + 1] = key;
	}
	Offline_Text( 10, 0, 0, text, sizeof( text ) );
	s_rules.avatar = text[0] ? trap_R_RegisterShaderNoMip( text ) : 0;

	/* the fields: the controls without a slot */
	for ( i = 0; i < s_rules.controls && l->fields < RULES_MAX_FIELDS; i++ ) {
		rulesControl_t *c = &s_rules.control[i];
		char *end;
		if ( c->slot >= 0 ) continue;
		Q_strncpyz( s_rules.fieldText[l->fields], c->label, sizeof( s_rules.fieldText[l->fields] ) );
		end = s_rules.fieldText[l->fields] + strlen( s_rules.fieldText[l->fields] );
		while ( end > s_rules.fieldText[l->fields] && end[-1] == ' ' ) *--end = '\0';
		if ( end > s_rules.fieldText[l->fields] && end[-1] == ':' ) *--end = '\0';
		while ( end > s_rules.fieldText[l->fields] && end[-1] == ' ' ) *--end = '\0';
		s_rules.field[l->fields] = i;
		l->fieldText[l->fields] = s_rules.fieldText[l->fields];
		l->fieldInput[l->fields] = c->type == 1 || c->type == 4;
		l->fields++;
	}

	/* the grid: a row per slot */
	for ( slot = 0; slot < 12 && l->rows < RULES_MAX_ROWS; slot++ ) {
		int type = -1, team = -1, row = l->rows, n = 0, has = 0;
		for ( i = 0; i < s_rules.controls; i++ ) {
			if ( s_rules.control[i].slot != slot ) continue;
			has = 1;
			if ( s_rules.control[i].team ) { if ( team < 0 ) team = i; }
			else if ( type < 0 ) type = i;
		}
		if ( ( !has && slot ) || ( !allowsBots && slot ) || ( drag && slot > 1 ) ) continue;
		s_rules.slot[row] = slot;
		s_rules.typeCtl[row] = type;
		s_rules.teamCtl[row] = team;
		s_rules.bot[row] = -2;		/* no bot select */
		s_rules.car[row] = 0;
		l->rowHuman[row] = slot == 0 && !s_rules.dedicated;
		if ( slot == 0 ) {
			if ( s_rules.dedicated ) Q_strncpyz( s_rules.rowText[row], "Server slot", sizeof( s_rules.rowText[row] ) );
			else Com_sprintf( s_rules.rowText[row], sizeof( s_rules.rowText[row] ), "You / %s", Rules_Clean( s_rules.players[0], clean, sizeof( clean ) ) );
		} else {
			Com_sprintf( s_rules.rowText[row], sizeof( s_rules.rowText[row] ), "Slot %d", slot + 1 );
		}
		l->rowText[row] = s_rules.rowText[row];
		l->rowKind[row][n++] = RULES_P;
		if ( l->rowHuman[row] ) {
			if ( s_rules.avatar ) l->rowKind[row][n++] = RULES_AVATAR;
		} else if ( type >= 0 ) {
			l->rowKind[row][n++] = RULES_TYPE;
		}
		if ( slot > 0 && type >= 0 && s_rules.control[type].value == 1 && allowsBots ) {
			int current = -1;
			Rules_Clean( s_rules.players[slot], clean, sizeof( clean ) );
			for ( i = 0; i < s_rules.bots; i++ ) if ( !Q_stricmp( s_rules.botName[i], clean ) ) { current = i; break; }
			s_rules.bot[row] = current;
			if ( current >= 0 ) {
				Q_strncpyz( s_rules.botText[row], s_rules.botName[current], sizeof( s_rules.botText[row] ) );
				s_rules.car[row] = Rules_CarShader( s_rules.botModel[current] );
			} else {
				Q_strncpyz( s_rules.botText[row], s_rules.players[slot][0] ? clean : "Choose a rival", sizeof( s_rules.botText[row] ) );
			}
			if ( s_rules.car[row] ) l->rowKind[row][n++] = RULES_CAR;
			l->rowKind[row][n++] = RULES_BOT;
		}
		if ( team >= 0 ) l->rowKind[row][n++] = RULES_TEAM;
		l->rowParts[row] = n;
		l->rows++;
	}
	Rules_Layout( l, WS_ViewportWidth(), WS_ViewportHeight() );
	s_rules.optionsScroll = l->optionsScroll;
	s_rules.playersScroll = l->playersScroll;
	s_rules.pageScroll = l->pageScroll;
}

/* -------------------------------------------------------------------------
   Drawing
   ------------------------------------------------------------------------- */

static void Rules_Fill( float x, float y, float w, float h, const float *color, const wsRect_t *clip ) {
	float top = y, bottom = y + h;
	if ( clip ) {
		if ( top < clip->y ) top = clip->y;
		if ( bottom > clip->y + clip->h ) bottom = clip->y + clip->h;
	}
	if ( bottom > top && w > 0.0f ) WS_FillRect( x, top, w, bottom - top, color );
}

static void Rules_Mix( const float *color, float opacity, vec4_t out ) {
	int i;
	for ( i = 0; i < 3; i++ ) out[i] = color[i] * opacity + wsSurface[i] * ( 1.0f - opacity );
	out[3] = 1.0f;
}

/* an input or a select: #222, the line border, the focus ring */
static void Rules_DrawBox( const wsRect_t *r, qboolean focus, float opacity, const wsRect_t *clip ) {
	float x0 = WS_Round( r->x ), y0 = WS_Round( r->y ), x1 = WS_Round( r->x + r->w ), y1 = WS_Round( r->y + r->h );
	vec4_t line, fill;
	Rules_Mix( focus ? rulesRing : wsLine, opacity, line );
	Rules_Mix( rulesInput, opacity, fill );
	if ( focus ) Rules_Fill( x0 - 1, y0 - 1, x1 - x0 + 2, y1 - y0 + 2, line, clip );
	else Rules_Fill( x0, y0, x1 - x0, y1 - y0, line, clip );
	Rules_Fill( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, fill, clip );
}

static void Rules_SetClip( const wsRect_t *clip ) {
	if ( clip ) WS_SetFade( clip->y, clip->h, 0.0f );
	else WS_SetFade( 0.0f, 0.0f, 0.0f );
}

/* a select: its text in the font's size, the arrow at the right */
static void Rules_DrawSelect( const wsRect_t *r, const char *text, float size, qboolean disabled, qboolean focus, const wsRect_t *clip ) {
	wsTextStyle_t style;
	vec4_t color;
	float line = floor( size + 0.5f ) + floor( 0.2f * size + 0.5f ) + 2.0f, arrowY;
	float opacity = disabled ? 0.7f : 1.0f;		/* select:disabled { opacity: .7 } */

	WS_BodyStyle( &style );
	style.size = size;
	style.lineHeight = line;
	Rules_DrawBox( r, focus, opacity, clip );
	Rules_Mix( wsWhite, opacity, color );
	WS_SetClipX( r->x + 1.0f, r->x + r->w - 20.0f );
	Rules_SetClip( clip );
	WS_DrawText( &style, r->x + 1.0f + WS_LU( 0.65f * WS_REM ), r->y + 1.0f + WS_LU( 0.65f * WS_REM ) + ( r->h - 2.0f * WS_LU( 0.65f * WS_REM ) - 2.0f - line ) * 0.5f,
	             text, strlen( text ), color );
	Rules_SetClip( NULL );
	WS_SetClipX( 0.0f, 0.0f );
	arrowY = WS_Round( r->y + r->h * 0.5f ) - 3.0f;
	if ( !clip || ( arrowY >= clip->y && arrowY + 7.0f <= clip->y + clip->h ) ) {
		trap_R_SetColor( color );
		trap_R_DrawStretchPic( WS_Round( r->x + r->w ) - 14.0f, arrowY, 10, 7, 0, 25 / 32.0f, 10 / 16.0f, 1.0f,
		                       trap_R_RegisterShaderNoMip( "menu/ui/web_scrollbar" ) );
		trap_R_SetColor( NULL );
	}
}

static void Rules_DrawInput( const wsRect_t *r, const char *text, qboolean editing, const wsRect_t *clip ) {
	wsTextStyle_t body;
	float x = r->x + 1.0f + WS_LU( 0.65f * WS_REM ), y = r->y + 1.0f + WS_LU( 0.65f * WS_REM ) + ( r->h - 2.0f * WS_LU( 0.65f * WS_REM ) - 2.0f - WS_BODY_LINE ) * 0.5f;
	WS_BodyStyle( &body );
	Rules_DrawBox( r, editing, 1.0f, clip );
	Rules_SetClip( clip );
	WS_SetClipX( r->x + 1.0f, r->x + r->w - 1.0f );
	WS_DrawText( &body, x, y, text, strlen( text ), wsWhite );
	WS_SetClipX( 0.0f, 0.0f );
	Rules_SetClip( NULL );
	if ( editing && ( ( uis.realtime - s_rules.editTime ) / 500 ) % 2 == 0 ) {
		Rules_Fill( WS_Round( x + WS_TextWidth( &body, text, strlen( text ) ) ), WS_Round( y + 3.0f ), 1, 19, wsWhite, clip );
	}
}

/* a square picture, object-fit: contain in its box, cut to the clip */
static void Rules_DrawPicture( const wsRect_t *r, qhandle_t shader, const wsRect_t *clip ) {
	float top = r->y, bottom = r->y + r->h;
	if ( !shader || r->h <= 0.0f ) return;
	if ( clip ) {
		if ( top < clip->y ) top = clip->y;
		if ( bottom > clip->y + clip->h ) bottom = clip->y + clip->h;
	}
	if ( bottom <= top ) return;
	trap_R_SetColor( NULL );
	trap_R_DrawStretchPic( WS_Round( r->x ), top, WS_Round( r->w ), bottom - top, 0, ( top - r->y ) / r->h, 1, ( bottom - r->y ) / r->h, shader );
}

static const char *Rules_ChoiceText( const rulesControl_t *c, int choice, char *out, int size ) {
	Offline_Text( 6, c->index, choice, out, size );
	return out;
}

/* the option list's entries: a control's choices, or the bots by name (and
 * the slot's own name when it is no bot) */
static void Rules_PopupText( int item, char *out, int size ) {
	if ( s_rules.popupCtl >= 0 ) {
		Rules_ChoiceText( &s_rules.control[s_rules.popupCtl], item, out, size );
	} else if ( item < s_rules.bots ) {
		Q_strncpyz( out, s_rules.botName[s_rules.sorted[item]], size );
	} else {
		Q_strncpyz( out, s_rules.botText[s_rules.popupRow], size );
	}
}

static void Rules_PopupRect( wsRect_t *r, int *rows ) {
	float below, above;
	*rows = s_rules.popupCount < RULES_POPUP_ROWS ? s_rules.popupCount : RULES_POPUP_ROWS;
	r->x = WS_Round( s_rules.popupAnchor.x );
	r->w = WS_Round( s_rules.popupAnchor.x + s_rules.popupAnchor.w ) - r->x;
	below = WS_ViewportHeight() - WS_Round( s_rules.popupAnchor.y + s_rules.popupAnchor.h );
	above = WS_Round( s_rules.popupAnchor.y );
	if ( *rows * s_rules.popupSize + 2.0f > below && above > below ) {
		while ( *rows > 1 && *rows * s_rules.popupSize + 2.0f > above ) ( *rows )--;
		r->h = *rows * s_rules.popupSize + 2.0f;
		r->y = above - r->h;
	} else {
		while ( *rows > 1 && *rows * s_rules.popupSize + 2.0f > below ) ( *rows )--;
		r->h = *rows * s_rules.popupSize + 2.0f;
		r->y = WS_Round( s_rules.popupAnchor.y + s_rules.popupAnchor.h );
	}
	s_rules.popupScroll = WS_Clamp( 0.0f, s_rules.popupScroll, (float)( s_rules.popupCount - *rows ) );
}

static void Rules_DrawPopup( void ) {
	wsTextStyle_t body;
	wsRect_t r;
	char text[64];
	int rows, i, first;

	if ( !s_rules.popup ) return;
	Rules_PopupRect( &r, &rows );
	WS_BodyStyle( &body );
	WS_FillRect( r.x, r.y, r.w, r.h, rulesPopupLine );
	WS_FillRect( r.x + 1, r.y + 1, r.w - 2, r.h - 2, rulesPopup );
	first = (int)s_rules.popupScroll;
	WS_SetClipX( r.x + 1, r.x + r.w - 1 );
	for ( i = 0; i < rows && first + i < s_rules.popupCount; i++ ) {
		if ( first + i == s_rules.popupCurrent ) WS_FillRect( r.x + 1, r.y + 1 + i * s_rules.popupSize, r.w - 2, s_rules.popupSize, rulesPopupLine );
		Rules_PopupText( first + i, text, sizeof( text ) );
		WS_DrawText( &body, r.x + 1.0f + WS_LU( 0.65f * WS_REM ), r.y + 1 + i * s_rules.popupSize, text, strlen( text ), wsWhite );
	}
	WS_SetClipX( 0.0f, 0.0f );
}

static void Rules_Draw( void ) {
	rulesLayout_t *l;
	offlineHeader_t *h;
	wsTextStyle_t body, h1, h1em, mono;
	char text[96];
	const wsRect_t *clip;
	int i, j;

	if ( s_rules.drag != DRAG_NONE ) {
		float x, y, track, content;
		const wsRect_t *port, *thumb;
		float *scroll;
		Library_Cursor( &x, &y );
		l = &s_rules.layout;
		if ( !trap_Key_IsDown( K_MOUSE1 ) ) s_rules.drag = DRAG_NONE;
		else {
			if ( s_rules.drag == DRAG_OPTIONS ) { port = &l->options; thumb = &l->optionsThumb; content = l->optionsHeight; scroll = &s_rules.optionsScroll; }
			else if ( s_rules.drag == DRAG_PLAYERS ) { port = &l->players; thumb = &l->playersThumb; content = l->playersHeight; scroll = &s_rules.playersScroll; }
			else { port = &l->page; thumb = &l->pageThumb; content = l->pageHeight; scroll = &s_rules.pageScroll; }
			track = port->h - 2.0f * RIVALS_ARROW - 9.0f - thumb->h;
			if ( track > 0.0f ) *scroll = s_rules.dragScroll + ( y - s_rules.dragY ) * ( content - port->h ) / track;
		}
	}
	Rules_Build();
	l = &s_rules.layout;
	h = &l->head;
	WS_BodyStyle( &body );
	mono.font = WS_MONO; mono.size = 0.9f * WS_REM; mono.spacing = 0.0f; mono.lineHeight = 16.0f; mono.uppercase = qfalse;
	h1.font = WS_CONDENSED; h1.size = h->h1Size; h1.spacing = 0.0f; h1.lineHeight = h->h1Size; h1.uppercase = qtrue;
	h1em = h1; h1em.font = WS_CONDENSED_ITALIC;

	WS_DrawBackground( h->pageWidth, l->pageHeight );
	WS_DrawLabel( h->contentX, h->labelY, l->hosting ? "Q3RALLY / HOST A SERVER" : "Q3RALLY / OFFLINE", NULL );
	WS_DrawText( &h1, h->contentX, h->h1Y, "YOUR NEXT ", 10, wsWhite );
	WS_DrawText( &h1em, h->contentX + WS_StyledWidth( &h1, "Your next " ), h->h1Y, "RACE.", 5, wsAccent );
	WS_DrawButton( &h->back, "Back to tracks", qtrue, qfalse );
	WS_DrawText( &mono, h->contentX, h->stepY, "01 / TRACK & MODE", 17, rulesMuted );
	WS_DrawText( &mono, h->stepRulesX, h->stepY, "02 / RULES & DRIVERS", 20, wsAccent );

	/* the summary */
	WS_DrawSection( &l->summary, qfalse );
	/* the summary's padding is the sections' at the sides */
	WS_DrawLabel( l->leftX, l->summaryLabelY, "YOUR RACE", NULL );
	WS_Transform( &l->titleStyle, l->title, text, sizeof( text ) );
	WS_DrawLines( &l->titleStyle, l->leftX, l->titleY, text, &l->titleLines, wsWhite );
	WS_DrawText( &body, l->leftX, l->modeY, l->modeName, strlen( l->modeName ), wsWhite );
	Library_DrawButton( &l->start, l->hosting ? "Start server" : "Start race", qfalse, UI_WebOfflineNumber( 0, 0 ) <= 0 );

	/* the rules */
	WS_DrawSection( &l->left, qfalse );
	WS_DrawText( &l->h2Style, l->leftX, l->rulesH2Y, "RACE RULES", 10, wsWhite );
	WS_DrawLines( &body, l->leftX, l->ruleHelpY, l->ruleHelp, &l->ruleHelpLines, wsWhite );
	clip = h->desktop ? &l->options : NULL;
	for ( i = 0; i < l->fields; i++ ) {
		rulesControl_t *c = &s_rules.control[s_rules.field[i]];
		wsRect_t box = l->fieldBox[i], control = l->fieldControl[i];
		float dy = l->options.y - l->optionsScroll;
		box.x += l->options.x; box.y += dy;
		control.x += l->options.x; control.y += dy;
		if ( box.y >= l->options.y + l->options.h || box.y + box.h <= l->options.y ) continue;
		Rules_SetClip( clip );
		WS_DrawLines( &body, box.x, box.y + ( control.y - box.y - 0.5f * WS_REM - l->fieldLines[i].count * WS_BODY_LINE ) * 0.5f,
		              l->fieldText[i], &l->fieldLines[i], wsWhite );
		Rules_SetClip( NULL );
		if ( c->type == 1 || c->type == 4 ) {
			qboolean editing = s_rules.edit == s_rules.field[i];
			Rules_DrawInput( &control, editing ? s_rules.editText : c->text, editing, clip );
		} else {
			Rules_DrawSelect( &control, Rules_ChoiceText( c, c->value, text, sizeof( text ) ), 16.0f, c->disabled,
			                  s_rules.popup && s_rules.popupCtl == s_rules.field[i], clip );
		}
	}
	if ( l->optionsBar ) WS_DrawScrollbar( &l->options, &l->optionsThumb );

	/* the starting grid */
	WS_DrawSection( &l->right, qfalse );
	WS_DrawText( &l->h2Style, l->rightX, l->gridH2Y, "STARTING GRID", 13, wsWhite );
	WS_DrawLines( &body, l->rightX, l->gridHelpY, l->gridHelp, &l->gridHelpLines, wsWhite );
	clip = h->desktop ? &l->players : NULL;
	for ( i = 0; i < l->rows; i++ ) {
		rulesRow_t *row = &l->row[i];
		float ox = l->players.x, oy = l->players.y - l->playersScroll + row->y;
		if ( oy >= l->players.y + l->players.h || oy + row->h <= l->players.y ) continue;
		Rules_Fill( ox, oy + row->h - 1.0f, l->rowWidth, 1.0f, rulesRow, clip );
		for ( j = 0; j < row->parts; j++ ) {
			rulesItem_t *part = &row->part[j];
			wsRect_t box = part->box, control = part->control;
			int ctl;
			box.x += ox; box.y += oy;
			control.x += ox; control.y += oy;
			switch ( part->kind ) {
			case RULES_P: {
				wsTextStyle_t label;
				WS_LabelStyle( &label );
				Rules_SetClip( clip );
				WS_Transform( &label, row->text, text, sizeof( text ) );
				WS_DrawLines( &label, box.x, box.y, text, &part->lines, wsAccent );
				Rules_SetClip( NULL );
				break;
			}
			case RULES_AVATAR:
				Rules_DrawPicture( &control, s_rules.avatar, clip );
				break;
			case RULES_CAR:
				Rules_DrawPicture( &control, s_rules.car[i], clip );
				break;
			case RULES_BOT:
				Rules_DrawSelect( &box, s_rules.botText[i], 16.0f, qfalse, s_rules.popup && s_rules.popupCtl < 0 && s_rules.popupRow == i, clip );
				break;
			default:
				ctl = part->kind == RULES_TYPE ? s_rules.typeCtl[i] : s_rules.teamCtl[i];
				Rules_SetClip( clip );
				WS_DrawLines( &l->rowLabel, box.x, box.y, part->kind == RULES_TYPE ? "Slot type" : "Team", &part->lines, wsWhite );
				Rules_SetClip( NULL );
				if ( ctl >= 0 ) {
					rulesControl_t *c = &s_rules.control[ctl];
					Rules_DrawSelect( &control, Rules_ChoiceText( c, c->value, text, sizeof( text ) ), l->rowLabel.size, c->disabled,
					                  s_rules.popup && s_rules.popupCtl == ctl, clip );
				}
				break;
			}
		}
	}
	if ( l->playersBar ) WS_DrawScrollbar( &l->players, &l->playersThumb );
	if ( l->pageBar ) WS_DrawScrollbar( &l->page, &l->pageThumb );
	Rules_DrawPopup();
}

/* -------------------------------------------------------------------------
   Input
   ------------------------------------------------------------------------- */

/* the change event of an input: a number, or the host name */
static void Rules_Commit( void ) {
	rulesControl_t *c;
	int i;
	if ( s_rules.edit < 0 ) return;
	c = &s_rules.control[s_rules.edit];
	if ( c->type == 4 ) {
		trap_Cvar_Set( "ui_weboffline_text", s_rules.editText[0] ? s_rules.editText : "Q3Rally" );
		UI_WebOfflineAction( 7, c->index, 0 );
	} else {
		int value = 0;
		for ( i = 0; s_rules.editText[i]; i++ ) value = value * 10 + ( s_rules.editText[i] - '0' );
		UI_WebOfflineAction( 4, c->index, value );
	}
	s_rules.edit = -1;
}

static void Rules_OpenPopup( int ctl, int row, const wsRect_t *anchor, float size ) {
	s_rules.popup = qtrue;
	s_rules.popupCtl = ctl;
	s_rules.popupRow = row;
	s_rules.popupAnchor = *anchor;
	s_rules.popupSize = WS_BODY_LINE;
	( void )size;
	if ( ctl >= 0 ) {
		s_rules.popupCount = s_rules.control[ctl].choices;
		s_rules.popupCurrent = s_rules.control[ctl].value;
	} else {
		int i;
		s_rules.popupCount = s_rules.bots + ( s_rules.bot[row] < 0 ? 1 : 0 );
		s_rules.popupCurrent = s_rules.bots;
		for ( i = 0; i < s_rules.bots; i++ ) if ( s_rules.sorted[i] == s_rules.bot[row] ) s_rules.popupCurrent = i;
	}
	s_rules.popupScroll = s_rules.popupCurrent - RULES_POPUP_ROWS / 2;
}

static void Rules_PopupClick( float x, float y ) {
	wsRect_t r;
	int rows, item;
	Rules_PopupRect( &r, &rows );
	s_rules.popup = qfalse;
	if ( !Library_Inside( &r, x, y ) || y < r.y + 1 || y >= r.y + 1 + rows * s_rules.popupSize ) return;
	item = (int)s_rules.popupScroll + (int)( ( y - r.y - 1 ) / s_rules.popupSize );
	if ( item < 0 || item >= s_rules.popupCount || item == s_rules.popupCurrent ) return;
	if ( s_rules.popupCtl >= 0 ) UI_WebOfflineAction( 4, s_rules.control[s_rules.popupCtl].index, item );
	else if ( item < s_rules.bots ) UI_WebOfflineAction( 5, s_rules.slot[s_rules.popupRow], s_rules.sorted[item] );
}

static qboolean Rules_ScrollbarClick( const wsRect_t *port, const wsRect_t *thumb, float *scroll, rulesDrag_t drag, float x, float y ) {
	if ( x < port->x + port->w - RIVALS_SCROLLBAR || !Library_Inside( port, x, y ) ) return qfalse;
	if ( y < port->y + RIVALS_ARROW ) *scroll -= 40.0f;
	else if ( y >= port->y + port->h - RIVALS_ARROW ) *scroll += 40.0f;
	else if ( y < thumb->y ) *scroll -= 0.875f * port->h;
	else if ( y >= thumb->y + thumb->h ) *scroll += 0.875f * port->h;
	else {
		s_rules.drag = drag;
		s_rules.dragY = y;
		s_rules.dragScroll = *scroll;
	}
	return qtrue;
}

static void Rules_Click( float x, float y ) {
	rulesLayout_t *l = &s_rules.layout;
	int i, j;

	if ( s_rules.popup ) { Rules_PopupClick( x, y ); return; }
	Rules_Commit();
	if ( l->pageBar && Rules_ScrollbarClick( &l->page, &l->pageThumb, &s_rules.pageScroll, DRAG_PAGE, x, y ) ) return;
	if ( Library_Inside( &l->head.back, x, y ) ) { UI_WebOfflineAction( 0, 0, 0 ); return; }
	if ( Library_Inside( &l->start, x, y ) ) {
		if ( UI_WebOfflineNumber( 0, 0 ) > 0 ) UI_WebOfflineAction( 6, 0, 0 );
		return;
	}
	if ( Library_Inside( &l->options, x, y ) ) {
		if ( l->optionsBar && Rules_ScrollbarClick( &l->options, &l->optionsThumb, &s_rules.optionsScroll, DRAG_OPTIONS, x, y ) ) return;
		for ( i = 0; i < l->fields; i++ ) {
			wsRect_t control = l->fieldControl[i];
			rulesControl_t *c = &s_rules.control[s_rules.field[i]];
			control.x += l->options.x;
			control.y += l->options.y - l->optionsScroll;
			if ( !Library_Inside( &control, x, y ) || c->disabled ) continue;
			if ( c->type == 1 || c->type == 4 ) {
				s_rules.edit = s_rules.field[i];
				Q_strncpyz( s_rules.editText, c->text, sizeof( s_rules.editText ) );
				s_rules.editTime = uis.realtime;
			} else {
				Rules_OpenPopup( s_rules.field[i], -1, &control, 16.0f );
			}
			return;
		}
		return;
	}
	if ( !Library_Inside( &l->players, x, y ) ) return;
	if ( l->playersBar && Rules_ScrollbarClick( &l->players, &l->playersThumb, &s_rules.playersScroll, DRAG_PLAYERS, x, y ) ) return;
	for ( i = 0; i < l->rows; i++ ) {
		rulesRow_t *row = &l->row[i];
		float ox = l->players.x, oy = l->players.y - l->playersScroll + row->y;
		for ( j = 0; j < row->parts; j++ ) {
			rulesItem_t *part = &row->part[j];
			wsRect_t r = part->kind == RULES_BOT ? part->box : part->control;
			int ctl;
			r.x += ox; r.y += oy;
			if ( !Library_Inside( &r, x, y ) ) continue;
			if ( part->kind == RULES_BOT ) { Rules_OpenPopup( -1, i, &r, 16.0f ); return; }
			if ( part->kind != RULES_TYPE && part->kind != RULES_TEAM ) continue;
			ctl = part->kind == RULES_TYPE ? s_rules.typeCtl[i] : s_rules.teamCtl[i];
			if ( ctl >= 0 && !s_rules.control[ctl].disabled ) Rules_OpenPopup( ctl, i, &r, l->rowLabel.size );
			return;
		}
	}
}

static sfxHandle_t Rules_Key( int key ) {
	rulesLayout_t *l = &s_rules.layout;
	float x, y;
	int length;

	Library_Cursor( &x, &y );
	if ( key & K_CHAR_FLAG ) {
		int ch = key & ~K_CHAR_FLAG, max;
		rulesControl_t *c;
		if ( s_rules.edit < 0 ) return 0;
		c = &s_rules.control[s_rules.edit];
		length = strlen( s_rules.editText );
		max = c->maxchars > 0 ? c->maxchars : ( c->type == 1 ? 3 : 63 );
		if ( max > (int)sizeof( s_rules.editText ) - 1 ) max = sizeof( s_rules.editText ) - 1;
		if ( ch == 8 ) { if ( length ) s_rules.editText[length - 1] = '\0'; }
		else if ( ch >= 32 && ch < 127 && length < max && ( c->type != 1 || ( ch >= '0' && ch <= '9' ) ) ) {
			s_rules.editText[length] = (char)ch;
			s_rules.editText[length + 1] = '\0';
		}
		s_rules.editTime = uis.realtime;
		return 0;
	}
	switch ( key ) {
	case K_ESCAPE:
	case K_PAD0_B:
		if ( s_rules.popup ) s_rules.popup = qfalse;
		else if ( s_rules.edit >= 0 ) Rules_Commit();
		else UI_WebOfflineAction( 0, 0, 0 );
		return 0;
	case K_MOUSE1:
		Rules_Click( x, y );
		return 0;
	case K_MWHEELUP:
	case K_MWHEELDOWN: {
		float step = key == K_MWHEELUP ? -100.0f : 100.0f;
		if ( s_rules.popup ) { s_rules.popupScroll += key == K_MWHEELUP ? -3.0f : 3.0f; return 0; }
		if ( Library_Inside( &l->options, x, y ) && l->optionsBar ) s_rules.optionsScroll += step;
		else if ( Library_Inside( &l->players, x, y ) && l->playersBar ) s_rules.playersScroll += step;
		else s_rules.pageScroll += step;
		return 0;
	}
	case K_ENTER:
	case K_KP_ENTER:
		if ( s_rules.popup ) s_rules.popup = qfalse;
		else if ( s_rules.edit >= 0 ) Rules_Commit();
		else if ( UI_WebOfflineNumber( 0, 0 ) > 0 ) UI_WebOfflineAction( 6, 0, 0 );
		return 0;
	}
	return 0;
}

void Offline_InstallRules( menuframework_s *menu ) {
	char preview[64];

	memset( &s_rules, 0, sizeof( s_rules ) );
	s_rules.edit = -1;
	menu->draw = Rules_Draw;
	menu->key = Rules_Key;
	/* ui_offlinePreview "mode,search,track,scroll,pagescroll,2": the grid's scroll and the page's */
	trap_Cvar_VariableStringBuffer( "ui_offlinePreview", preview, sizeof( preview ) );
	if ( preview[0] ) {
		char *field = preview;
		int n;
		for ( n = 0; n < 3 && field; n++ ) {
			field = strchr( field, ',' );
			if ( field ) field++;
		}
		if ( field ) {
			s_rules.playersScroll = atof( field );
			field = strchr( field, ',' );
			if ( field ) s_rules.pageScroll = atof( field + 1 );
		}
	}
}
