/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Native drawing in the look of the WebUI. See ui_rally_webstyle.h.

The fonts are baked by tools/ui_webstyle/build_ui_fonts.py into
menu/fonts/<variant>.dat plus one atlas per size. Advances and kerning are
in font units, so a run measures the same at every size; the glyph images
come from the next larger atlas.
*/

#include "ui_local.h"
#include "ui_rally_webstyle.h"

const vec4_t wsAccent     = { 1.0f, 85 / 255.0f, 0.0f, 1.0f };
const vec4_t wsLine       = { 55 / 255.0f, 55 / 255.0f, 55 / 255.0f, 1.0f };
const vec4_t wsSurface    = { 23 / 255.0f, 23 / 255.0f, 23 / 255.0f, 1.0f };
const vec4_t wsWhite      = { 1.0f, 1.0f, 1.0f, 1.0f };
const vec4_t wsMuted      = { 153 / 255.0f, 153 / 255.0f, 153 / 255.0f, 1.0f };
const vec4_t wsSummary    = { 187 / 255.0f, 187 / 255.0f, 187 / 255.0f, 1.0f };
const vec4_t wsButtonText = { 20 / 255.0f, 20 / 255.0f, 20 / 255.0f, 1.0f };

/* -------------------------------------------------------------------------
   Fonts
   ------------------------------------------------------------------------- */

#define WS_MAX_GLYPHS	224
#define WS_MAX_KERNS	4096
#define WS_MAX_SIZES	12
#define WS_FILE_MAX		65536

typedef struct {
	int			size100;		/* atlas pixel size * 100 */
	int			width, height;
	char		image[64];
	qhandle_t	shader;
	short		rect[WS_MAX_GLYPHS][6];	/* x y w h left top */
} wsAtlas_t;

typedef struct {
	int			state;			/* 0 not tried, 1 ready, -1 failed */
	int			upm, ascent, descent;
	int			glyphCount, kernCount, sizeCount;
	short		index[256];		/* Latin-1 code -> glyph, -1 none */
	int			advance[WS_MAX_GLYPHS];
	int			kernKey[WS_MAX_KERNS];
	short		kernValue[WS_MAX_KERNS];
	wsAtlas_t	atlas[WS_MAX_SIZES];
} wsFontData_t;

static const char *wsFontFiles[WS_NUM_FONTS] = {
	"sharetechmono", "barlow", "barlow_bold", "barlowcondensed", "barlowcondensed_italic"
};
static wsFontData_t	wsFonts[WS_NUM_FONTS];
static byte			wsFile[WS_FILE_MAX];

static int WS_ReadInt( const byte *data, int *offset ) {
	const byte *p = data + *offset;
	*offset += 4;
	return (int)( p[0] | ( p[1] << 8 ) | ( p[2] << 16 ) | ( (unsigned)p[3] << 24 ) );
}

static qboolean WS_LoadFont( wsFont_t id ) {
	wsFontData_t	*font = &wsFonts[id];
	fileHandle_t	file;
	char			path[MAX_QPATH];
	int				length, offset = 4, version, i, s, code;

	Com_sprintf( path, sizeof( path ), "menu/fonts/%s.dat", wsFontFiles[id] );
	length = trap_FS_FOpenFile( path, &file, FS_READ );
	if ( length <= 32 || length > WS_FILE_MAX ) {
		if ( length >= 0 ) trap_FS_FCloseFile( file );
		Com_Printf( S_COLOR_YELLOW "WARNING: menu font %s missing or too large\n", path );
		return qfalse;
	}
	trap_FS_Read( wsFile, length, file );
	trap_FS_FCloseFile( file );
	if ( memcmp( wsFile, "Q3UF", 4 ) ) return qfalse;

	version = WS_ReadInt( wsFile, &offset );
	font->upm = WS_ReadInt( wsFile, &offset );
	font->ascent = WS_ReadInt( wsFile, &offset );
	font->descent = WS_ReadInt( wsFile, &offset );
	font->glyphCount = WS_ReadInt( wsFile, &offset );
	font->kernCount = WS_ReadInt( wsFile, &offset );
	font->sizeCount = WS_ReadInt( wsFile, &offset );
	if ( version != 1 || font->upm <= 0 || font->glyphCount > WS_MAX_GLYPHS ||
	     font->kernCount > WS_MAX_KERNS || font->sizeCount < 1 || font->sizeCount > WS_MAX_SIZES ||
	     offset + font->glyphCount * 8 + font->kernCount * 8 > length ) {
		Com_Printf( S_COLOR_YELLOW "WARNING: menu font %s has an unsupported layout\n", path );
		return qfalse;
	}
	for ( i = 0; i < 256; i++ ) font->index[i] = -1;
	for ( i = 0; i < font->glyphCount; i++ ) {
		code = WS_ReadInt( wsFile, &offset );
		font->advance[i] = WS_ReadInt( wsFile, &offset );
		if ( code >= 0 && code < 256 ) font->index[code] = i;
	}
	for ( i = 0; i < font->kernCount; i++ ) {
		font->kernKey[i] = WS_ReadInt( wsFile, &offset );
		font->kernValue[i] = (short)WS_ReadInt( wsFile, &offset );
	}
	for ( s = 0; s < font->sizeCount; s++ ) {
		wsAtlas_t *atlas = &font->atlas[s];
		if ( offset + 12 + 64 + font->glyphCount * 24 > length ) return qfalse;
		atlas->size100 = WS_ReadInt( wsFile, &offset );
		atlas->width = WS_ReadInt( wsFile, &offset );
		atlas->height = WS_ReadInt( wsFile, &offset );
		Q_strncpyz( atlas->image, (const char *)wsFile + offset, sizeof( atlas->image ) );
		offset += 64;
		for ( i = 0; i < font->glyphCount; i++ ) {
			int k;
			for ( k = 0; k < 6; k++ ) atlas->rect[i][k] = (short)WS_ReadInt( wsFile, &offset );
		}
		if ( atlas->size100 <= 0 || atlas->width <= 0 || atlas->height <= 0 ) return qfalse;
	}
	return qtrue;
}

qboolean WS_FontReady( wsFont_t font ) {
	if ( font < 0 || font >= WS_NUM_FONTS ) return qfalse;
	if ( !wsFonts[font].state ) {
		wsFonts[font].state = WS_LoadFont( font ) ? 1 : -1;
	}
	return wsFonts[font].state == 1;
}

static int WS_Kern( const wsFontData_t *font, int left, int right ) {
	int key = ( left << 16 ) | right, low = 0, high = font->kernCount - 1, middle;
	while ( low <= high ) {
		middle = ( low + high ) >> 1;
		if ( font->kernKey[middle] == key ) return font->kernValue[middle];
		if ( font->kernKey[middle] < key ) low = middle + 1;
		else high = middle - 1;
	}
	return 0;
}

/* The glyph a byte shows: missing ones fall back to '?'. */
static int WS_Glyph( const wsFontData_t *font, int code ) {
	int glyph = font->index[code & 255];
	return glyph >= 0 ? glyph : font->index['?'];
}

/* -------------------------------------------------------------------------
   Metrics
   ------------------------------------------------------------------------- */

float WS_Clamp( float low, float value, float high ) {
	return value < low ? low : ( value > high ? high : value );
}

float WS_Round( float value ) {
	return (float)floor( value + 0.5f );
}

float WS_ViewportWidth( void ) { return (float)uis.glconfig.vidWidth; }
float WS_ViewportHeight( void ) { return (float)uis.glconfig.vidHeight; }

void WS_Transform( const wsTextStyle_t *style, const char *in, char *out, int size ) {
	int n = 0, c;
	for ( ; *in && n < size - 1; in++ ) {
		c = (unsigned char)*in;
		if ( style->uppercase ) {
			if ( c == 0xdf ) {		/* sharp s uppercases to "SS" */
				if ( n < size - 2 ) { out[n++] = 'S'; out[n++] = 'S'; }
				continue;
			}
			if ( c >= 'a' && c <= 'z' ) c -= 32;
			else if ( c >= 0xe0 && c <= 0xfe && c != 0xf7 ) c -= 32;
		}
		out[n++] = (char)c;
	}
	out[n] = '\0';
}

float WS_TextWidth( const wsTextStyle_t *style, const char *text, int length ) {
	const wsFontData_t *font;
	float width = 0.0f, scale;
	int i, glyph, previous = -1;

	if ( !WS_FontReady( style->font ) ) return 0.0f;
	font = &wsFonts[style->font];
	scale = style->size / font->upm;
	for ( i = 0; i < length && text[i]; i++ ) {
		int code = (unsigned char)text[i];
		glyph = WS_Glyph( font, code );
		if ( previous >= 0 ) width += WS_Kern( font, previous, code ) * scale;
		if ( glyph >= 0 ) width += font->advance[glyph] * scale;
		width += style->spacing;
		previous = code;
	}
	return width;
}

/* Chrome rounds ascent and descent to whole pixels and puts the larger half
 * of the leading below the text. */
static float WS_Ascent( const wsTextStyle_t *style ) {
	const wsFontData_t *font = &wsFonts[style->font];
	return WS_Round( font->ascent * style->size / font->upm );
}

float WS_TextTop( const wsTextStyle_t *style ) {
	const wsFontData_t *font;
	float content;
	if ( !WS_FontReady( style->font ) ) return 0.0f;
	font = &wsFonts[style->font];
	content = WS_Ascent( style ) + WS_Round( font->descent * style->size / font->upm );
	return (float)floor( ( style->lineHeight - content ) * 0.5f );
}

float WS_Baseline( const wsTextStyle_t *style ) {
	if ( !WS_FontReady( style->font ) ) return 0.0f;
	return WS_TextTop( style ) + WS_Ascent( style );
}

void WS_Wrap( const wsTextStyle_t *style, const char *text, float maxWidth, wsLines_t *lines ) {
	int start = 0, end, next, length = strlen( text );

	memset( lines, 0, sizeof( *lines ) );
	while ( start < length && lines->count < WS_MAX_LINES ) {
		/* take words while they fit; a single long word stays alone */
		end = start;
		while ( end < length && text[end] != ' ' ) end++;
		while ( end < length ) {
			next = end + 1;
			while ( next < length && text[next] != ' ' ) next++;
			if ( WS_TextWidth( style, text + start, next - start ) > maxWidth + 0.01f ) break;
			end = next;
		}
		lines->start[lines->count] = start;
		lines->length[lines->count] = end - start;
		lines->width[lines->count] = WS_TextWidth( style, text + start, end - start );
		lines->count++;
		start = end;
		while ( start < length && text[start] == ' ' ) start++;
	}
}

void WS_DrawText( const wsTextStyle_t *style, float x, float lineTop, const char *text, int length, const float *color ) {
	const wsFontData_t *font;
	const wsAtlas_t *atlas;
	float pen = x, scale, k, baseline;
	int i, s, glyph, previous = -1;

	if ( !WS_FontReady( style->font ) ) return;
	font = &wsFonts[style->font];

	/* the next larger atlas, scaled down */
	atlas = &font->atlas[font->sizeCount - 1];
	for ( s = 0; s < font->sizeCount; s++ ) {
		if ( font->atlas[s].size100 >= (int)( style->size * 100.0f - 0.5f ) ) {
			atlas = &font->atlas[s];
			break;
		}
	}
	if ( !atlas->shader ) {
		( (wsAtlas_t *)atlas )->shader = trap_R_RegisterShaderNoMip( atlas->image );
	}
	scale = style->size / font->upm;
	k = style->size * 100.0f / atlas->size100;
	baseline = WS_Round( lineTop + WS_Baseline( style ) );

	trap_R_SetColor( color );
	for ( i = 0; i < length && text[i]; i++ ) {
		int code = (unsigned char)text[i];
		const short *rect;
		glyph = WS_Glyph( font, code );
		if ( previous >= 0 ) pen += WS_Kern( font, previous, code ) * scale;
		previous = code;
		if ( glyph < 0 ) {
			pen += style->spacing;
			continue;
		}
		rect = atlas->rect[glyph];
		if ( rect[2] > 0 && rect[3] > 0 ) {
			trap_R_DrawStretchPic( WS_Round( pen + rect[4] * k ), baseline - rect[5] * k, rect[2] * k, rect[3] * k,
				(float)rect[0] / atlas->width, (float)rect[1] / atlas->height,
				(float)( rect[0] + rect[2] ) / atlas->width, (float)( rect[1] + rect[3] ) / atlas->height,
				atlas->shader );
		}
		pen += font->advance[glyph] * scale + style->spacing;
	}
	trap_R_SetColor( NULL );
}

/* -------------------------------------------------------------------------
   Boxes
   ------------------------------------------------------------------------- */

static qhandle_t wsStripes, wsGlow, wsProgressTrack, wsProgressFill;

static void WS_RegisterArt( void ) {
	if ( wsStripes ) return;
	wsStripes = trap_R_RegisterShaderNoMip( "menu/ui/web_stripes" );
	wsGlow = trap_R_RegisterShaderNoMip( "menu/ui/web_showroom" );
	wsProgressTrack = trap_R_RegisterShaderNoMip( "menu/ui/web_progress_track" );
	wsProgressFill = trap_R_RegisterShaderNoMip( "menu/ui/web_progress_fill" );
}

/* Boxes snap to whole pixels the way Chrome paints them. */
void WS_FillRect( float x, float y, float w, float h, const float *color ) {
	float x0 = WS_Round( x ), y0 = WS_Round( y ), x1 = WS_Round( x + w ), y1 = WS_Round( y + h );
	if ( x1 <= x0 || y1 <= y0 ) return;
	trap_R_SetColor( color );
	trap_R_DrawStretchPic( x0, y0, x1 - x0, y1 - y0, 0, 0, 0, 0, uis.whiteShader );
	trap_R_SetColor( NULL );
}

void WS_DrawSection( const wsRect_t *box, qboolean showroom ) {
	float x0 = WS_Round( box->x ), y0 = WS_Round( box->y );
	float x1 = WS_Round( box->x + box->w ), y1 = WS_Round( box->y + box->h );

	WS_RegisterArt();
	if ( showroom ) {
		/* radial-gradient(ellipse at center, #38302c, #141414 75%) over the padding box */
		trap_R_DrawStretchPic( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, 0, 0, 1, 1, wsGlow );
	} else {
		WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsSurface );
	}
	WS_FillRect( x0, y0 + 1, 1, y1 - y0 - 1, wsLine );
	WS_FillRect( x1 - 1, y0 + 1, 1, y1 - y0 - 1, wsLine );
	WS_FillRect( x0 + 1, y1 - 1, x1 - x0 - 2, 1, wsLine );
	WS_FillRect( x0, y0, x1 - x0, 1, wsAccent );
}

/*
 * repeating-linear-gradient(132deg, #0b0b0b 0 12px, #171717 12px 13px) on
 * the body, which Chrome paints across the canvas in tiles the size of the
 * html box. The shader turns the pixel position into the distance along the
 * gradient line (tcMod transform); s here is the position in periods.
 */
#define WS_STRIPE_PERIOD	13.0f
#define WS_STRIPE_DX		0.7431448f		/* sin 132 */
#define WS_STRIPE_DY		0.6691306f		/* -cos 132 */

void WS_DrawBackground( float pageWidth, float pageHeight ) {
	float width = WS_ViewportWidth(), height = WS_ViewportHeight();
	float tileX, tileY, w, h, phase;

	WS_RegisterArt();
	if ( pageWidth < 1.0f || pageHeight < 1.0f ) return;
	phase = ( pageWidth * WS_STRIPE_DX + pageHeight * WS_STRIPE_DY ) * 0.5f
		- pageWidth * 0.5f * WS_STRIPE_DX - pageHeight * 0.5f * WS_STRIPE_DY;
	for ( tileY = 0.0f; tileY < height; tileY += pageHeight ) {
		h = tileY + pageHeight > height ? height - tileY : pageHeight;
		for ( tileX = 0.0f; tileX < width; tileX += pageWidth ) {
			float s0 = phase / ( WS_STRIPE_PERIOD * WS_STRIPE_DX );
			w = tileX + pageWidth > width ? width - tileX : pageWidth;
			trap_R_DrawStretchPic( tileX, tileY, w, h,
				s0, 0.0f, s0 + w / WS_STRIPE_PERIOD, h / WS_STRIPE_PERIOD, wsStripes );
		}
	}
}

/*
 * Chrome's dark progress bar: an 8px pill centered in the 16px box, a 1px
 * #858585 rim around #3b3b3b, the value in the accent color. The art holds
 * the caps as Chrome draws them on a #171717 section: columns 0-3 left
 * cap, 4-11 middle, 12-15 right cap.
 */
#define WS_PROGRESS_ART		16.0f

static void WS_ProgressPart( float x, float y, float w, float s0, float s1, qhandle_t shader ) {
	if ( w <= 0.0f ) return;
	trap_R_DrawStretchPic( x, y, w, 8, s0 / WS_PROGRESS_ART, 0, s1 / WS_PROGRESS_ART, 1, shader );
}

void WS_DrawProgress( float x, float y, float w, float fraction ) {
	float left = WS_Round( x ), right = WS_Round( x + w ), top = WS_Round( y ) + 4, fill;

	WS_RegisterArt();
	fraction = WS_Clamp( 0.0f, fraction, 1.0f );
	WS_ProgressPart( left, top, 4, 0, 4, wsProgressTrack );
	WS_ProgressPart( left + 4, top, right - left - 8, 4.5f, 4.5f, wsProgressTrack );
	WS_ProgressPart( right - 4, top, 4, 12, 16, fraction >= 1.0f ? wsProgressFill : wsProgressTrack );
	if ( fraction <= 0.0f ) return;
	fill = WS_Round( x + w * fraction );
	if ( fill > right - 4 || fraction >= 1.0f ) fill = right - 4;
	WS_ProgressPart( left, top, fill - left < 4 ? fill - left : 4, 0, fill - left < 4 ? fill - left : 4, wsProgressFill );
	WS_ProgressPart( left + 4, top, fill - left - 4, 4.5f, 4.5f, wsProgressFill );
}

void WS_DrawButton( const wsRect_t *box, const char *label, qboolean secondary, qboolean focused ) {
	wsTextStyle_t style = { WS_BODY_BOLD, WS_REM, 0.0f, 24.0f, qfalse };
	float x0 = WS_Round( box->x ), y0 = WS_Round( box->y );
	float x1 = WS_Round( box->x + box->w ), y1 = WS_Round( box->y + box->h );
	const float *border = secondary ? wsLine : wsAccent;

	WS_FillRect( x0, y0, x1 - x0, y1 - y0, border );
	WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, secondary ? wsSurface : wsAccent );
	WS_DrawText( &style, box->x + 1 + 0.85f * WS_REM, box->y + 1 + 0.65f * WS_REM, label, strlen( label ),
		secondary ? wsWhite : wsButtonText );
	if ( focused ) {
		/* button:focus-visible { outline: 2px solid #fff; outline-offset: 3px } */
		WS_FillRect( x0 - 5, y0 - 5, x1 - x0 + 10, 2, wsWhite );
		WS_FillRect( x0 - 5, y1 + 3, x1 - x0 + 10, 2, wsWhite );
		WS_FillRect( x0 - 5, y0 - 3, 2, y1 - y0 + 6, wsWhite );
		WS_FillRect( x1 + 3, y0 - 3, 2, y1 - y0 + 6, wsWhite );
	}
}
