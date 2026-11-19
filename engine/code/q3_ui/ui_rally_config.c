/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

The settings drawn like the WebUI's #config-screen (config.js): the setup
page, controls, graphics, display, sound, network, game and Q3Rally options,
the renderer information, the confirmation and the race control pages
(pause, bots, teams, server information), and the wizards (offline Ladder
registration, first start, demo save). One renderer for every page, as
on the page: it reads UI_WebConfigNumber / UI_WebConfigChar and acts through
UI_WebConfigAction, like the client did for CEF.
*/

#include "ui_local.h"
#include "ui_rally_config.h"
#include "ui_rally_configlayout.h"
#include "ui_rally_library.h"
#include "ui_rally_rivalslayout.h"

#define CONFIG_LAST_PAGE	21

static int Config_Page( void ) {
	return UI_WebConfigScreen() ? UI_WebConfigNumber( 0, 0 ) : -1;
}

qboolean Config_Native( void ) {
	int page = Config_Page();
	return page >= 0 && page <= CONFIG_LAST_PAGE;
}

static void Config_Text( int kind, int index, int choice, char *out, int size ) {
	int i;
	for ( i = 0; i < size - 1; i++ ) {
		out[i] = (char)UI_WebConfigChar( kind, index, choice, i );
		if ( !out[i] ) break;
	}
	out[i] = '\0';
}

static void Config_Quoted( const char *text ) {
	static char out[4400];
	int n = 0;
	out[n++] = '"';
	for ( ; *text && n < (int)sizeof( out ) - 3; text++ ) {
		if ( *text == '"' || *text == '\\' ) out[n++] = '\\';
		if ( *text == '\n' ) { out[n++] = '\\'; out[n++] = 'n'; continue; }
		out[n++] = *text;
	}
	out[n++] = '"';
	out[n] = '\0';
	/* Com_Printf takes 1024 characters at a time */
	for ( n = 0; out[n]; n += 500 ) {
		char part[501];
		Q_strncpyz( part, out + n, sizeof( part ) );
		Com_Printf( "%s", part );
		if ( (int)strlen( out + n ) <= 500 ) break;
	}
}

/* the page as begin(), item() and choice() gave it, for tests/fixtures/config_model.json */
void Config_Dump( void ) {
	static char text[2048];
	int i, j, type, count = 0;

	Com_Printf( "CONFIGMODEL {\"page\":%d,\"title\":", UI_WebConfigNumber( 0, 0 ) );
	Config_Text( 0, 0, 0, text, sizeof( text ) );
	Config_Quoted( text );
	Com_Printf( ",\"description\":" );
	Config_Text( 4, 0, 0, text, sizeof( text ) );
	Config_Quoted( text );
	Com_Printf( ",\"confirmation\":%d,\"items\":[", UI_WebConfigNumber( 8, 0 ) );
	for ( i = 0; i < UI_WebConfigNumber( 1, 0 ); i++ ) {
		type = UI_WebConfigNumber( 2, i );
		if ( !type ) continue;
		Com_Printf( "%s[%d,%d,", count++ ? "," : "", i, type );
		Config_Text( 1, i, 0, text, sizeof( text ) );
		Config_Quoted( text );
		Com_Printf( "," );
		Config_Text( 2, i, 0, text, sizeof( text ) );
		Config_Quoted( text );
		Com_Printf( ",%d,%d,%d,%d,[", UI_WebConfigNumber( 3, i ), UI_WebConfigNumber( 4, i ), UI_WebConfigNumber( 5, i ), UI_WebConfigNumber( 6, i ) );
		for ( j = 0; j < UI_WebConfigNumber( 7, i ); j++ ) {
			if ( j ) Com_Printf( "," );
			Com_Printf( "[" );
			Config_Text( 3, i, j, text, sizeof( text ) );
			Config_Quoted( text );
			Com_Printf( "," );
			Config_Text( 5, i, j, text, sizeof( text ) );
			Config_Quoted( text );
			Com_Printf( "]" );
		}
		Com_Printf( "]]" );
	}
	Com_Printf( "]}\n" );
}

/* -------------------------------------------------------------------------
   The page as config.js renders it
   ------------------------------------------------------------------------- */

typedef enum { DRAG_NONE, DRAG_PAGE, DRAG_SLIDER, DRAG_LIST } configDrag_t;

static const vec4_t configHolder   = { 0x75 / 255.0f, 0x75 / 255.0f, 0x75 / 255.0f, 1.0f };
static const vec4_t configDim      = { 1.0f, 1.0f, 1.0f, 0.5f };	/* disabled controls */
static const vec4_t configBackdrop = { 0.0f, 0.0f, 0.0f, 0xbb / 255.0f };	/* dialog::backdrop #000b */
static const vec4_t configPopup    = { 0x1f / 255.0f, 0x1f / 255.0f, 0x1f / 255.0f, 1.0f };
static const vec4_t configPopupLine = { 0x3b / 255.0f, 0x3b / 255.0f, 0x3b / 255.0f, 1.0f };

static struct {
	configLayout_t	layout;
	char			pageKey[160];
	char			title[128];
	char			description[4096];
	char			buttonText[CONFIG_MAX_BUTTONS][64];
	int				buttonItem[CONFIG_MAX_BUTTONS];
	qboolean		buttonDisabled[CONFIG_MAX_BUTTONS];
	char			fieldLabel[CONFIG_MAX_FIELDS][96];
	char			fieldCapture[CONFIG_MAX_FIELDS][96];	/* the label as the capture dialog shows it */
	char			fieldText[CONFIG_MAX_FIELDS][1024];
	int				fieldItem[CONFIG_MAX_FIELDS];
	int				value[CONFIG_MAX_FIELDS], low[CONFIG_MAX_FIELDS], high[CONFIG_MAX_FIELDS], choices[CONFIG_MAX_FIELDS];
	qboolean		disabled[CONFIG_MAX_FIELDS];
	float			listScroll[CONFIG_MAX_FIELDS];
	char			search[64];
	int				focus;			/* the text field being edited, field + 1 */
	qboolean		backDisabled;
	char			avatarPath[CONFIG_MAX_FIELDS][MAX_QPATH];
	qhandle_t		avatarShader[CONFIG_MAX_FIELDS];
	int				focusTime;
	int				popup;			/* field + 1 */
	float			popupScroll;
	int				capture;		/* field + 1 */
	float			pageScroll;
	configDrag_t	drag;
	int				dragField, dragValue;
	float			dragY, dragScroll;
} s_config;

/* strips ^colors, like clean() in config.js */
static void Config_Clean( char *text ) {
	char *in = text, *out = text;
	while ( *in ) {
		if ( in[0] == '^' && in[1] >= '0' && in[1] <= '9' ) { in += 2; continue; }
		*out++ = *in++;
	}
	*out = '\0';
}

static void Config_Choice( int field, int choice, char *out, int size ) {
	Config_Text( 3, s_config.fieldItem[field], choice, out, size );
	Config_Clean( out );
}

/* what the field's select shows */
static void Config_SelectText( int field, char *out, int size ) {
	int type = s_config.layout.field[field].type;
	if ( type == CONFIG_CHOOSE ) Q_strncpyz( out, "Choose an option", size );
	else if ( s_config.value[field] < 0 || s_config.value[field] >= s_config.choices[field] ) Q_strncpyz( out, s_config.value[field] < 0 ? "Custom" : "", size );
	else Config_Choice( field, s_config.value[field], out, size );
}

/* String(value / 100) */
static void Config_Hundredths( int value, char *out, int size ) {
	int whole = abs( value ) / 100, part = abs( value ) % 100;
	const char *sign = value < 0 ? "-" : "";
	if ( !part ) Com_sprintf( out, size, "%s%d", sign, whole );
	else if ( part % 10 == 0 ) Com_sprintf( out, size, "%s%d.%d", sign, whole, part / 10 );
	else Com_sprintf( out, size, "%s%d.%02d", sign, whole, part );
}

static void Config_Build( void ) {
	configLayout_t *l = &s_config.layout;
	char key[160], label[128];
	int page = Config_Page(), items = UI_WebConfigNumber( 1, 0 ), i, j, type, confirm;

	Config_Text( 0, 0, 0, s_config.title, sizeof( s_config.title ) );
	Config_Clean( s_config.title );
	Config_Text( 4, 0, 0, s_config.description, sizeof( s_config.description ) );
	Config_Clean( s_config.description );
	confirm = UI_WebConfigNumber( 8, 0 );
	Com_sprintf( key, sizeof( key ), "%d:%s", page, s_config.title );
	if ( strcmp( key, s_config.pageKey ) ) {
		/* a new page: back to the top (render() in config.js) */
		Q_strncpyz( s_config.pageKey, key, sizeof( s_config.pageKey ) );
		s_config.pageScroll = 0.0f;
		s_config.popup = s_config.capture = 0;
		s_config.focus = 0;
		s_config.drag = DRAG_NONE;
		memset( s_config.listScroll, 0, sizeof( s_config.listScroll ) );
	}

	l->screen = page;
	l->title = s_config.title;
	l->description = s_config.description;
	l->buttons = l->fields = 0;
	s_config.backDisabled = qfalse;
	for ( i = 0; i < items; i++ ) {
		type = UI_WebConfigNumber( 2, i );
		if ( !type ) continue;
		Config_Text( 1, i, 0, label, sizeof( label ) );
		Config_Clean( label );
		if ( type == CONFIG_BUTTON ) {
			char trimmed[128];
			int start = 0, end = strlen( label );
			while ( label[start] == ' ' ) start++;
			while ( end > start && label[end - 1] == ' ' ) end--;
			Q_strncpyz( trimmed, label + start, end - start + 1 );
			if ( !Q_stricmp( label, "back" ) ) s_config.backDisabled = UI_WebConfigNumber( 6, i ) != 0;
			if ( !confirm && ( !Q_stricmp( trimmed, "back" ) || ( page == 10 && !Q_stricmpn( label, "resume", 6 ) ) ) ) continue;
			if ( l->buttons >= CONFIG_MAX_BUTTONS ) continue;
			Q_strncpyz( s_config.buttonText[l->buttons], label, sizeof( s_config.buttonText[0] ) );
			s_config.buttonItem[l->buttons] = i;
			s_config.buttonDisabled[l->buttons] = UI_WebConfigNumber( 6, i ) != 0;
			l->buttonLabel[l->buttons] = s_config.buttonText[l->buttons];
			l->buttons++;
			continue;
		}
		if ( l->fields >= CONFIG_MAX_FIELDS ) continue;
		j = l->fields++;
		s_config.fieldItem[j] = i;
		Q_strncpyz( s_config.fieldCapture[j], label, sizeof( s_config.fieldCapture[0] ) );
		/* the label without its colon (label.replace(/:\s*$/, '')) */
		{
			int end = strlen( label );
			while ( end > 0 && ( label[end - 1] == ' ' || label[end - 1] == '\t' ) ) end--;
			if ( end > 0 && label[end - 1] == ':' ) label[end - 1] = '\0';
		}
		Q_strncpyz( s_config.fieldLabel[j], label, sizeof( s_config.fieldLabel[0] ) );
		s_config.value[j] = UI_WebConfigNumber( 3, i );
		s_config.low[j] = UI_WebConfigNumber( 4, i );
		s_config.high[j] = UI_WebConfigNumber( 5, i );
		s_config.disabled[j] = UI_WebConfigNumber( 6, i ) != 0;
		s_config.choices[j] = UI_WebConfigNumber( 7, i );
		s_config.fieldText[j][0] = '\0';
		if ( type == CONFIG_KEY || type == CONFIG_FIELD ) {
			Config_Text( 2, i, 0, s_config.fieldText[j], sizeof( s_config.fieldText[0] ) );
			Config_Clean( s_config.fieldText[j] );
		} else if ( type == CONFIG_LIST ) {
			int c, length = 0;
			for ( c = 0; c < s_config.choices[j]; c++ ) {
				char choice[128];
				Config_Choice( j, c, choice, sizeof( choice ) );
				if ( c ) Q_strcat( s_config.fieldText[j], sizeof( s_config.fieldText[0] ), "\n" );
				Q_strcat( s_config.fieldText[j], sizeof( s_config.fieldText[0] ), choice );
				length = strlen( s_config.fieldText[j] );
			}
			( void )length;
		}
		/* the avatar select's preview: the picture of the choice, if it has one */
		l->field[j].avatar = CONFIG_NO_PREVIEW;
		if ( type == CONFIG_SPIN || type == CONFIG_RADIO ) {
			char image[MAX_QPATH];
			int c;
			for ( c = 0; c < s_config.choices[j]; c++ ) {
				Config_Text( 5, i, c, image, sizeof( image ) );
				if ( image[0] ) break;
			}
			if ( c < s_config.choices[j] ) {
				image[0] = '\0';
				if ( s_config.value[j] >= 0 && s_config.value[j] < s_config.choices[j] ) Config_Text( 5, i, s_config.value[j], image, sizeof( image ) );
				if ( strcmp( image, s_config.avatarPath[j] ) ) {
					Q_strncpyz( s_config.avatarPath[j], image, sizeof( s_config.avatarPath[0] ) );
					s_config.avatarShader[j] = image[0] ? trap_R_RegisterShaderNoMip( image ) : 0;
				}
				l->field[j].avatar = s_config.avatarShader[j] ? CONFIG_PREVIEW_IMAGE : CONFIG_PREVIEW_NOTE;
			}
		}
		l->field[j].type = type;
		l->field[j].label = s_config.fieldLabel[j];
		l->field[j].text = s_config.fieldText[j];
	}
	if ( s_config.capture > l->fields || ( s_config.capture && l->field[s_config.capture - 1].type != CONFIG_KEY ) ) s_config.capture = 0;
	if ( s_config.popup > l->fields ) s_config.popup = 0;
	if ( s_config.focus > l->fields || ( s_config.focus && l->field[s_config.focus - 1].type != CONFIG_FIELD ) ) s_config.focus = 0;
	l->dialog = confirm ? CONFIG_CONFIRM : s_config.capture ? CONFIG_CAPTURE : CONFIG_NO_DIALOG;
	l->captureAction = s_config.capture ? s_config.fieldCapture[s_config.capture - 1] : "";
	l->pageScroll = s_config.pageScroll;
	Config_Layout( l, WS_ViewportWidth(), WS_ViewportHeight() );
	s_config.pageScroll = l->pageScroll;
}

/* -------------------------------------------------------------------------
   Drawing
   ------------------------------------------------------------------------- */

static void Config_DrawInput( const wsRect_t *r, const char *text, const char *placeholder, qboolean focus ) {
	wsTextStyle_t body;
	float x = r->x + 1.0f + WS_LU( 0.65f * WS_REM ), y = r->y + 1.0f + WS_LU( 0.65f * WS_REM );
	WS_BodyStyle( &body );
	WS_DrawInputBox( r, focus );
	WS_SetClipX( r->x + 1.0f, r->x + r->w - 1.0f );
	if ( text[0] ) WS_DrawText( &body, x, y, text, strlen( text ), wsWhite );
	else if ( placeholder ) WS_DrawText( &body, x, y, placeholder, strlen( placeholder ), configHolder );
	WS_SetClipX( 0.0f, 0.0f );
	if ( focus && ( ( uis.realtime - s_config.focusTime ) / 500 ) % 2 == 0 ) {
		WS_FillRect( WS_Round( x + WS_TextWidth( &body, text, strlen( text ) ) ), WS_Round( y + 3.0f ), 1, 19, wsWhite );
	}
}

/* input[type=range] in the dark scheme with accent-color: an 8px track with
 * a grey border, the accent up to the 16px thumb */
static float Config_SliderFraction( int field, int value ) {
	int span = s_config.high[field] - s_config.low[field];
	return span > 0 ? WS_Clamp( 0.0f, (float)( value - s_config.low[field] ) / span, 1.0f ) : 0.0f;
}

static void Config_DrawLines( const wsTextStyle_t *style, float x, float y, const char *text, const configLines_t *lines, const float *color ) {
	int i;
	for ( i = 0; i < lines->count; i++ ) {
		WS_DrawText( style, x, y + i * style->lineHeight, text + lines->start[i], lines->length[i], color );
	}
}

static int Config_PopupRows( int field ) {
	int type = s_config.layout.field[field].type;
	return s_config.choices[field] + ( type == CONFIG_CHOOSE || s_config.value[field] < 0 ? 1 : 0 );
}

/* row r of the open popup: its text, and the choice it stands for (-1 none) */
static int Config_PopupChoice( int field, int row, char *out, int size ) {
	int type = s_config.layout.field[field].type;
	if ( type == CONFIG_CHOOSE || s_config.value[field] < 0 ) {
		if ( !row ) {
			Q_strncpyz( out, type == CONFIG_CHOOSE ? "Choose an option" : "Custom", size );
			return -1;
		}
		row--;
	}
	Config_Choice( field, row, out, size );
	return row;
}

static void Config_PopupRect( wsRect_t *r, int *rows ) {
	const wsRect_t *anchor = &s_config.layout.field[s_config.popup - 1].control;
	int count = Config_PopupRows( s_config.popup - 1 );
	float below = WS_ViewportHeight() - WS_Round( anchor->y + anchor->h ), above = WS_Round( anchor->y );
	*rows = count < 20 ? count : 20;
	r->x = WS_Round( anchor->x );
	r->w = WS_Round( anchor->x + anchor->w ) - r->x;
	if ( *rows * WS_BODY_LINE + 2.0f > below && above > below ) {
		while ( *rows > 1 && *rows * WS_BODY_LINE + 2.0f > above ) ( *rows )--;
		r->h = *rows * WS_BODY_LINE + 2.0f;
		r->y = above - r->h;
	} else {
		while ( *rows > 1 && *rows * WS_BODY_LINE + 2.0f > below ) ( *rows )--;
		r->h = *rows * WS_BODY_LINE + 2.0f;
		r->y = WS_Round( anchor->y + anchor->h );
	}
	s_config.popupScroll = WS_Clamp( 0.0f, s_config.popupScroll, (float)( count - *rows ) );
}

static void Config_DrawPopup( void ) {
	wsTextStyle_t body;
	wsRect_t r;
	char text[128];
	int rows, i, first, field = s_config.popup - 1, current;
	if ( !s_config.popup ) return;
	Config_PopupRect( &r, &rows );
	current = s_config.value[field] < 0 || s_config.layout.field[field].type == CONFIG_CHOOSE ? 0 : s_config.value[field];
	WS_BodyStyle( &body );
	WS_FillRect( r.x, r.y, r.w, r.h, configPopupLine );
	WS_FillRect( r.x + 1, r.y + 1, r.w - 2, r.h - 2, configPopup );
	first = (int)s_config.popupScroll;
	for ( i = 0; i < rows; i++ ) {
		int choice = Config_PopupChoice( field, first + i, text, sizeof( text ) );
		if ( ( choice < 0 && first + i == 0 && current == 0 && ( s_config.value[field] < 0 || s_config.layout.field[field].type == CONFIG_CHOOSE ) ) ||
		     ( choice >= 0 && choice == s_config.value[field] && s_config.layout.field[field].type != CONFIG_CHOOSE ) )
			WS_FillRect( r.x + 1, r.y + 1 + i * WS_BODY_LINE, r.w - 2, WS_BODY_LINE, configPopupLine );
		WS_DrawText( &body, r.x + 1.0f + WS_LU( 0.65f * WS_REM ), r.y + 1 + i * WS_BODY_LINE, text, strlen( text ),
		             choice < 0 && s_config.layout.field[field].type != CONFIG_CHOOSE ? configDim : wsWhite );
	}
}

/* config.js: primary only on the wizard pages, for these */
static qboolean Config_Secondary( const char *label ) {
	static const char *primary[] = { "next", "join ladder", "register", "retry", "ok", "done", "start racing", "save" };
	int i;
	if ( s_config.layout.screen < 16 ) return qtrue;
	for ( i = 0; i < (int)ARRAY_LEN( primary ); i++ ) {
		if ( !Q_stricmp( label, primary[i] ) ) return qfalse;
	}
	return qtrue;
}

static void Config_DrawDialog( void ) {
	configLayout_t *l = &s_config.layout;
	wsTextStyle_t body;
	wsRect_t *box = &l->dialogBox;
	char shown[256];
	float x0, y0, x1, y1;
	int i;

	if ( l->dialog == CONFIG_NO_DIALOG ) return;
	WS_BodyStyle( &body );
	/* the backdrop leaves the scrollbar gutter alone */
	WS_FillRect( 0, 0, s_config.layout.page.pageWidth, WS_ViewportHeight(), configBackdrop );
	x0 = WS_Round( box->x ); y0 = WS_Round( box->y ); x1 = WS_Round( box->x + box->w ); y1 = WS_Round( box->y + box->h );
	WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
	WS_FillRect( x0, y0, x1 - x0, 3, wsAccent );
	WS_FillRect( x0 + 1, y0 + 3, x1 - x0 - 2, y1 - y0 - 4, wsSurface );
	WS_DrawLabel( l->dialogX, l->dialogLabelY, l->dialogLabel, NULL );
	WS_Transform( &l->h2, l->dialogTitle, shown, sizeof( shown ) );
	WS_DrawLines( &l->h2, l->dialogX, l->dialogH2Y, shown, &l->dialogH2Lines, wsWhite );
	if ( l->dialog == CONFIG_CONFIRM ) {
		for ( i = 0; i < l->buttons; i++ ) Library_DrawButton( &l->button[i], l->buttonLabel[i], Config_Secondary( l->buttonLabel[i] ), s_config.buttonDisabled[i] );
	} else {
		WS_DrawLines( &body, l->dialogX, l->dialogActionY, l->captureAction, &l->dialogActionLines, wsWhite );
		WS_DrawLines( &body, l->dialogX, l->dialogHelpY, configCaptureHelp, &l->dialogHelpLines, wsWhite );
		Library_DrawButton( &l->cancel, "Cancel", qtrue, qfalse );
	}
}

/* .config-avatar-preview: #111 with a border, the picture (object-fit:
 * contain) or a note in #aaa at .9rem */
static void Config_DrawPreview( const configField_t *f, int field ) {
	static const vec4_t back = { 0x11 / 255.0f, 0x11 / 255.0f, 0x11 / 255.0f, 1.0f };
	static const vec4_t note = { 0xaa / 255.0f, 0xaa / 255.0f, 0xaa / 255.0f, 1.0f };
	float x0 = WS_Round( f->preview.x ), y0 = WS_Round( f->preview.y ), x1 = WS_Round( f->preview.x + f->preview.w ), y1 = WS_Round( f->preview.y + f->preview.h );
	WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
	WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, back );
	if ( f->avatar == CONFIG_PREVIEW_IMAGE ) {
		trap_R_SetColor( NULL );
		trap_R_DrawStretchPic( WS_Round( f->image.x ), WS_Round( f->image.y ), f->image.w, f->image.h, 0, 0, 1, 1, s_config.avatarShader[field] );
	} else {
		wsTextStyle_t small;
		const char *text = s_config.avatarPath[field][0] ? "Preview unavailable" : "No avatar selected";
		WS_BodyStyle( &small );
		small.size = 0.9f * WS_REM;
		small.lineHeight = WS_LU( 1.5f * 0.9f * WS_REM );
		WS_DrawText( &small, f->preview.x + ( f->preview.w - WS_TextWidth( &small, text, strlen( text ) ) ) * 0.5f,
		             f->preview.y + ( f->preview.h - small.lineHeight ) * 0.5f, text, strlen( text ), note );
	}
}

static void Config_DrawPage( void ) {
	configLayout_t *l = &s_config.layout;
	wsTextStyle_t body, accent;
	char text[128];
	int i;

	WS_BodyStyle( &body );
	WS_DrawBackground( l->page.pageWidth, l->pageHeight );
	WS_DrawFrontHeader( &l->page, l->title, "", "" );
	if ( !l->backHidden ) Library_DrawButton( &l->back, l->backLabel, qtrue, l->screen >= 17 && s_config.backDisabled );
	Config_DrawLines( &l->descriptionStyle, l->page.contentX, l->descriptionY, l->description, &l->descriptionLines, wsWhite );
	if ( l->dialog != CONFIG_CONFIRM ) {
		for ( i = 0; i < l->buttons; i++ ) Library_DrawButton( &l->button[i], l->buttonLabel[i], Config_Secondary( l->buttonLabel[i] ), s_config.buttonDisabled[i] );
	}
	if ( !l->settings ) return;
	if ( l->registration ) {
		/* .registration-screen #config-settings: border-top 2px */
		float x0 = WS_Round( l->section.x ), y0 = WS_Round( l->section.y ), x1 = WS_Round( l->section.x + l->section.w ), y1 = WS_Round( l->section.y + l->section.h );
		WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
		WS_FillRect( x0, y0, x1 - x0, 2, wsAccent );
		WS_FillRect( x0 + 1, y0 + 2, x1 - x0 - 2, y1 - y0 - 3, wsSurface );
	} else {
		WS_DrawSection( &l->section, qfalse );
	}
	accent = body;
	for ( i = 0; i < l->fields; i++ ) {
		configField_t *f = &l->field[i];
		WS_DrawLines( &body, f->box.x, f->box.y, f->label, &f->labelLines, wsWhite );
		switch ( f->type ) {
		case CONFIG_SLIDER: {
			int value = s_config.drag == DRAG_SLIDER && s_config.dragField == i ? s_config.dragValue : s_config.value[i];
			Config_Hundredths( value, text, sizeof( text ) );
			WS_DrawText( &accent, f->box.x, f->outputY, text, strlen( text ), wsAccent );
			WS_DrawRange( &f->control, 0.5f * WS_REM, Config_SliderFraction( i, value ), s_config.disabled[i] );
			break;
		}
		case CONFIG_FIELD:
			Config_DrawInput( &f->control, s_config.focus == i + 1 ? s_config.search : f->text, l->screen == 1 ? "Search controls" : NULL, s_config.focus == i + 1 );
			break;
		case CONFIG_KEY:
			Library_DrawButton( &f->rebind, f->text[0] ? f->text : "Unbound", qtrue, s_config.disabled[i] );
			Library_DrawButton( &f->clear, "Clear", qtrue, s_config.disabled[i] );
			break;
		case CONFIG_LIST: {
			float scroll = s_config.listScroll[i], top = f->control.y;
			WS_SetFade( top, f->control.h, 0.0f );
			Config_DrawLines( &body, f->control.x, top - scroll, f->text, &f->lines, wsWhite );
			WS_SetFade( 0.0f, 0.0f, 0.0f );
			if ( f->listHeight > f->control.h + 0.5f ) {
				wsRect_t thumb;
				WS_ScrollThumb( &f->control, f->listHeight, scroll, &thumb );
				WS_DrawScrollbar( &f->control, &thumb );
			}
			break;
		}
		default:
			Config_SelectText( i, text, sizeof( text ) );
			WS_DrawSelect( &f->control, WS_LU( 0.65f * WS_REM ), text, s_config.popup == i + 1 );
			if ( f->avatar != CONFIG_NO_PREVIEW ) Config_DrawPreview( f, i );
			break;
		}
	}
}

/* -------------------------------------------------------------------------
   Input
   ------------------------------------------------------------------------- */

static int Config_SliderValue( int field, float x ) {
	const wsRect_t *r = &s_config.layout.field[field].control;
	int low = s_config.low[field], high = s_config.high[field], step = high - low <= 100 ? 1 : 100, value;
	float w = r->w - 2.0f - 16.0f, fraction = w > 0.0f ? WS_Clamp( 0.0f, ( x - r->x - 1.0f - 8.0f ) / w, 1.0f ) : 0.0f;
	value = low + (int)( fraction * ( high - low ) / step + 0.5f ) * step;
	return value > high ? high : value;
}

/* the edited text goes to the field as the page's input event did */
static void Config_Search( void ) {
	trap_Cvar_Set( "ui_webconfig_text", s_config.search );
	if ( s_config.focus ) UI_WebConfigAction( 4, s_config.fieldItem[s_config.focus - 1], 0 );
}

static void Config_Bind( int key ) {
	int field = s_config.capture - 1;
	s_config.capture = 0;
	UI_WebConfigAction( 3, s_config.fieldItem[field], key );
}

static void Config_PopupClick( float x, float y ) {
	wsRect_t r;
	char text[128];
	int rows, row, choice, field = s_config.popup - 1;
	Config_PopupRect( &r, &rows );
	s_config.popup = 0;
	if ( !Library_Inside( &r, x, y ) || y < r.y + 1 || y >= r.y + 1 + rows * WS_BODY_LINE ) return;
	row = (int)s_config.popupScroll + (int)( ( y - r.y - 1 ) / WS_BODY_LINE );
	choice = Config_PopupChoice( field, row, text, sizeof( text ) );
	if ( choice < 0 ) return;
	if ( s_config.layout.field[field].type == CONFIG_CHOOSE ) UI_WebConfigAction( 7, s_config.fieldItem[field], choice );
	else if ( choice != s_config.value[field] ) UI_WebConfigAction( 1, s_config.fieldItem[field], choice );
}

static qboolean Config_ScrollbarClick( const wsRect_t *port, const wsRect_t *thumb, float *scroll, configDrag_t drag, int field, float x, float y ) {
	if ( x < port->x + port->w - RIVALS_SCROLLBAR || !Library_Inside( port, x, y ) ) return qfalse;
	if ( y < port->y + RIVALS_ARROW ) *scroll -= 40.0f;
	else if ( y >= port->y + port->h - RIVALS_ARROW ) *scroll += 40.0f;
	else if ( y < thumb->y ) *scroll -= 0.875f * port->h;
	else if ( y >= thumb->y + thumb->h ) *scroll += 0.875f * port->h;
	else {
		s_config.drag = drag;
		s_config.dragField = field;
		s_config.dragY = y;
		s_config.dragScroll = *scroll;
	}
	return qtrue;
}

static void Config_Click( float x, float y ) {
	configLayout_t *l = &s_config.layout;
	int i;

	if ( s_config.popup ) { Config_PopupClick( x, y ); return; }
	if ( l->dialog == CONFIG_CAPTURE ) {
		/* the dialog takes any mouse button but its own Cancel */
		if ( Library_Inside( &l->cancel, x, y ) ) s_config.capture = 0;
		else Config_Bind( K_MOUSE1 );
		return;
	}
	if ( l->dialog == CONFIG_CONFIRM ) {
		for ( i = 0; i < l->buttons; i++ ) {
			if ( Library_Inside( &l->button[i], x, y ) ) {
				if ( !s_config.buttonDisabled[i] ) UI_WebConfigAction( 0, s_config.buttonItem[i], 0 );
				return;
			}
		}
		return;
	}
	s_config.focus = 0;
	if ( l->pageBar && Config_ScrollbarClick( &l->viewport, &l->pageThumb, &s_config.pageScroll, DRAG_PAGE, 0, x, y ) ) return;
	if ( Library_Inside( &l->back, x, y ) ) {
		if ( !l->backHidden && !( l->screen >= 17 && s_config.backDisabled ) ) UI_WebConfigAction( 6, 0, 0 );
		return;
	}
	for ( i = 0; i < l->buttons; i++ ) {
		if ( Library_Inside( &l->button[i], x, y ) ) {
			if ( !s_config.buttonDisabled[i] ) UI_WebConfigAction( 0, s_config.buttonItem[i], 0 );
			return;
		}
	}
	for ( i = 0; i < l->fields && l->settings; i++ ) {
		configField_t *f = &l->field[i];
		if ( f->type == CONFIG_KEY ) {
			if ( s_config.disabled[i] ) continue;
			if ( Library_Inside( &f->rebind, x, y ) ) { s_config.capture = i + 1; return; }
			if ( Library_Inside( &f->clear, x, y ) ) { UI_WebConfigAction( 5, s_config.fieldItem[i], 0 ); return; }
			continue;
		}
		if ( !Library_Inside( &f->control, x, y ) || s_config.disabled[i] ) continue;
		switch ( f->type ) {
		case CONFIG_SLIDER:
			s_config.drag = DRAG_SLIDER;
			s_config.dragField = i;
			s_config.dragValue = Config_SliderValue( i, x );
			return;
		case CONFIG_FIELD:
			s_config.focus = i + 1;
			s_config.focusTime = uis.realtime;
			Q_strncpyz( s_config.search, s_config.fieldText[i], sizeof( s_config.search ) );
			return;
		case CONFIG_LIST:
			if ( f->listHeight > f->control.h + 0.5f ) {
				wsRect_t thumb;
				WS_ScrollThumb( &f->control, f->listHeight, s_config.listScroll[i], &thumb );
				Config_ScrollbarClick( &f->control, &thumb, &s_config.listScroll[i], DRAG_LIST, i, x, y );
			}
			return;
		default:
			s_config.popup = i + 1;
			s_config.popupScroll = ( s_config.value[i] > 0 ? s_config.value[i] : 0 ) - 10;
			return;
		}
	}
}

static void Config_Type( int ch ) {
	int length = strlen( s_config.search ), limit = sizeof( s_config.search ) - 1;
	if ( s_config.focus && s_config.high[s_config.focus - 1] > 0 && s_config.high[s_config.focus - 1] < limit ) limit = s_config.high[s_config.focus - 1];
	if ( ch == 8 ) { if ( length ) s_config.search[length - 1] = '\0'; else return; }
	else if ( ch >= 32 && ch < 127 && length < limit ) {
		s_config.search[length] = (char)ch;
		s_config.search[length + 1] = '\0';
	} else return;
	s_config.focusTime = uis.realtime;
	Config_Search();
}

static int configDumped = -1;

void Config_Draw( void ) {
	int page = Config_Page();
	char press[64];
	/* ui_configPress <item>: presses a button, for scripted checks (tools/ui_webstyle) */
	trap_Cvar_VariableStringBuffer( "ui_configPress", press, sizeof( press ) );
	if ( press[0] ) {
		trap_Cvar_Set( "ui_configPress", "" );
		/* "action:item:value" for any other action (4: the text in ui_webconfig_text) */
		if ( strchr( press, ':' ) ) {
			char *item = strchr( press, ':' ) + 1, *value = strchr( item, ':' );
			UI_WebConfigAction( atoi( press ), atoi( item ), value ? atoi( value + 1 ) : 0 );
		} else UI_WebConfigAction( 0, atoi( press ), 0 );
		page = Config_Page();
		if ( !Config_Native() ) return;
	}
	/* ui_configCapture <item>: opens the key capture for that action, for screenshots */
	trap_Cvar_VariableStringBuffer( "ui_configCapture", press, sizeof( press ) );
	if ( press[0] ) {
		int i;
		trap_Cvar_Set( "ui_configCapture", "" );
		Config_Build();
		for ( i = 0; i < s_config.layout.fields; i++ ) {
			if ( s_config.fieldItem[i] == atoi( press ) && s_config.layout.field[i].type == CONFIG_KEY ) s_config.capture = i + 1;
		}
	}
	if ( trap_Cvar_VariableValue( "ui_configDump" ) && page != configDumped ) {
		configDumped = page;
		Config_Dump();
	}
	if ( s_config.drag != DRAG_NONE ) {
		configLayout_t *l = &s_config.layout;
		float x, y, track;
		Library_Cursor( &x, &y );
		if ( s_config.drag == DRAG_SLIDER ) {
			if ( s_config.dragField < l->fields ) s_config.dragValue = Config_SliderValue( s_config.dragField, x );
			if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
				/* the range input's change event */
				s_config.drag = DRAG_NONE;
				if ( s_config.dragField < l->fields && s_config.dragValue != s_config.value[s_config.dragField] )
					UI_WebConfigAction( 2, s_config.fieldItem[s_config.dragField], s_config.dragValue );
			}
		} else if ( !trap_Key_IsDown( K_MOUSE1 ) ) {
			s_config.drag = DRAG_NONE;
		} else if ( s_config.drag == DRAG_PAGE ) {
			track = l->viewport.h - 2.0f * RIVALS_ARROW - 9.0f - l->pageThumb.h;
			if ( track > 0.0f ) s_config.pageScroll = s_config.dragScroll + ( y - s_config.dragY ) * ( l->pageHeight - l->viewport.h ) / track;
		} else if ( s_config.dragField < l->fields ) {
			configField_t *f = &l->field[s_config.dragField];
			wsRect_t thumb;
			WS_ScrollThumb( &f->control, f->listHeight, s_config.listScroll[s_config.dragField], &thumb );
			track = f->control.h - 2.0f * RIVALS_ARROW - 9.0f - thumb.h;
			if ( track > 0.0f ) s_config.listScroll[s_config.dragField] = s_config.dragScroll + ( y - s_config.dragY ) * ( f->listHeight - f->control.h ) / track;
		}
	}
	Config_Build();
	{
		int i;
		for ( i = 0; i < s_config.layout.fields; i++ ) {
			configField_t *f = &s_config.layout.field[i];
			if ( f->type == CONFIG_LIST ) s_config.listScroll[i] = WS_Clamp( 0.0f, s_config.listScroll[i], f->listHeight > f->control.h ? f->listHeight - f->control.h : 0.0f );
		}
	}
	Config_DrawPage();
	if ( s_config.layout.pageBar ) WS_DrawScrollbar( &s_config.layout.viewport, &s_config.layout.pageThumb );
	Config_DrawPopup();
	Config_DrawDialog();
}

sfxHandle_t Config_Key( int key ) {
	configLayout_t *l = &s_config.layout;
	float x, y;
	int i;

	Library_Cursor( &x, &y );
	if ( key & K_CHAR_FLAG ) {
		if ( s_config.focus && l->dialog == CONFIG_NO_DIALOG ) Config_Type( key & ~K_CHAR_FLAG );
		return 0;
	}
	if ( l->dialog == CONFIG_CAPTURE ) {
		/* the next key, mouse button, wheel or pad input; Escape cancels */
		if ( key == K_ESCAPE ) s_config.capture = 0;
		else if ( key == K_MOUSE1 ) Config_Click( x, y );
		else if ( key != '`' && key != '~' && key != K_CONSOLE ) Config_Bind( key );
		return 0;
	}
	switch ( key ) {
	case K_ESCAPE:
	case K_PAD0_B:
	case K_MOUSE2:
		if ( s_config.popup ) s_config.popup = 0;
		else if ( key != K_MOUSE2 ) UI_WebConfigAction( 6, 0, 0 );
		return 0;
	case K_MOUSE1:
		Config_Click( x, y );
		return 0;
	case K_MWHEELUP:
	case K_MWHEELDOWN:
		if ( s_config.popup ) { s_config.popupScroll += key == K_MWHEELUP ? -3.0f : 3.0f; return 0; }
		if ( l->dialog != CONFIG_NO_DIALOG ) return 0;
		for ( i = 0; i < l->fields && l->settings; i++ ) {
			configField_t *f = &l->field[i];
			if ( f->type == CONFIG_LIST && f->listHeight > f->control.h + 0.5f && Library_Inside( &f->control, x, y ) ) {
				s_config.listScroll[i] += key == K_MWHEELUP ? -100.0f : 100.0f;
				return 0;
			}
		}
		s_config.pageScroll += key == K_MWHEELUP ? -100.0f : 100.0f;
		return 0;
	case K_BACKSPACE:
		if ( s_config.focus ) Config_Type( 8 );
		return 0;
	case K_ENTER:
	case K_KP_ENTER:
		s_config.popup = 0;
		s_config.focus = 0;
		return 0;
	case K_UPARROW:
	case K_PGUP:
		if ( !s_config.focus ) s_config.pageScroll -= key == K_PGUP ? 0.875f * l->viewport.h : 40.0f;
		return 0;
	case K_DOWNARROW:
	case K_PGDN:
		if ( !s_config.focus ) s_config.pageScroll += key == K_PGDN ? 0.875f * l->viewport.h : 40.0f;
		return 0;
	}
	return 0;
}
