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

#ifdef CGAME
/* the cgame draws the HUD options in this look too (cgame/cg_hud_panel.c) */
#include "../cgame/cg_local.h"
#define WS_GLCONFIG		cgs.glconfig
#define WS_WHITE_SHADER	cgs.media.whiteShader
#else
#include "ui_local.h"
#define WS_GLCONFIG		uis.glconfig
#define WS_WHITE_SHADER	uis.whiteShader
#endif
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
#define WS_MAX_LIGS		16
#define WS_FILE_MAX		131072	/* the condensed face with its lowercase: 84 KB */

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
	int			ligCount;		/* the font's standard ligatures, in its order */
	int			ligCode[WS_MAX_LIGS];
	char		ligText[WS_MAX_LIGS][5];
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
	/* version 1 has no ligatures: older data still shows its text */
	if ( ( version != 1 && version != 2 ) || font->upm <= 0 || font->glyphCount > WS_MAX_GLYPHS ||
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
	font->ligCount = 0;
	if ( version >= 2 ) {
		if ( offset + 4 > length ) return qfalse;
		font->ligCount = WS_ReadInt( wsFile, &offset );
	}
	if ( font->ligCount < 0 || font->ligCount > WS_MAX_LIGS || offset + font->ligCount * 8 > length ) return qfalse;
	for ( i = 0; i < font->ligCount; i++ ) {
		font->ligCode[i] = WS_ReadInt( wsFile, &offset );
		memcpy( font->ligText[i], wsFile + offset, 4 );
		font->ligText[i][4] = '\0';
		offset += 4;
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
/* The code of the next glyph at text[i]: one of the font's standard
 * ligatures where it starts there, as Chrome shapes text without
 * letter-spacing, else the character. *used gets the characters it takes. */
static int WS_NextCode( const wsFontData_t *font, const wsTextStyle_t *style, const char *text, int i,
                        int length, int *used ) {
	int l, n;
	*used = 1;
	if ( style->spacing == 0.0f ) {
		for ( l = 0; l < font->ligCount; l++ ) {
			const char *c = font->ligText[l];
			for ( n = 0; c[n] && i + n < length && text[i + n] == c[n]; n++ ) {
			}
			if ( !c[n] ) {
				*used = n;
				return font->ligCode[l];
			}
		}
	}
	return (unsigned char)text[i];
}

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

float WS_ViewportWidth( void ) { return (float)WS_GLCONFIG.vidWidth; }
float WS_ViewportHeight( void ) { return (float)WS_GLCONFIG.vidHeight; }

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
	int i, glyph, previous = -1, used;

	if ( !WS_FontReady( style->font ) ) return 0.0f;
	font = &wsFonts[style->font];
	scale = style->size / font->upm;
	for ( i = 0; i < length && text[i]; i += used ) {
		int code = WS_NextCode( font, style, text, i, length, &used );
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

/* the end of the next piece that may end a line: before a space, or after a
 * hyphen between letters or digits ("4000-5000", as Chrome breaks it) */
static int WS_NextBreak( const char *text, int from, int length ) {
	int p;
	for ( p = from; p < length; p++ ) {
		if ( text[p] == ' ' ) return p;
		if ( text[p] == '-' && p > from && p + 1 < length && isalnum( (unsigned char)text[p - 1] ) &&
		     isalnum( (unsigned char)text[p + 1] ) ) return p + 1;
	}
	return length;
}

void WS_Wrap( const wsTextStyle_t *style, const char *text, float maxWidth, wsLines_t *lines ) {
	int start = 0, end, next, length = strlen( text );

	memset( lines, 0, sizeof( *lines ) );
	while ( start < length && lines->count < WS_MAX_LINES ) {
		/* take words while they fit; a single long word stays alone */
		end = WS_NextBreak( text, start, length );
		while ( end < length ) {
			next = end;
			while ( next < length && text[next] == ' ' ) next++;
			next = WS_NextBreak( text, next, length );
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

/* -------------------------------------------------------------------------
   Vertical fade mask

   mask-image: linear-gradient(transparent, #000 F, #000 1-F, transparent)
   on a box: text inside is cut to the box and faded in over the top and
   out over the bottom fraction F of its height. A glyph in a fade zone is
   drawn in one pixel rows, each with the mask's alpha at its center.
   ------------------------------------------------------------------------- */

static struct {
	float top, height, fraction;
} wsFade;

/* overflow: hidden at the sides, for text in a narrower box */
static float wsClipLeft, wsClipRight;

void WS_SetClipX( float left, float right ) {
	wsClipLeft = left;
	wsClipRight = right;
}

void WS_SetFade( float top, float height, float fraction ) {
	wsFade.top = top;
	wsFade.height = height;
	wsFade.fraction = fraction;
}

float WS_FadeAlpha( float y ) {
	float t, zone;
	if ( wsFade.height <= 0.0f ) return 1.0f;
	t = ( y - wsFade.top ) / wsFade.height;
	zone = wsFade.fraction;
	if ( t <= 0.0f || t >= 1.0f ) return 0.0f;
	if ( zone > 0.0f && t < zone ) return t / zone;
	if ( zone > 0.0f && t > 1.0f - zone ) return ( 1.0f - t ) / zone;
	return 1.0f;
}

static void WS_DrawFadedGlyph( float x, float y, float w, float h, float s0, float t0, float s1, float t1,
                               qhandle_t shader, const float *color ) {
	float top = wsFade.top, bottom = wsFade.top + wsFade.height;
	float solidTop = top + wsFade.fraction * wsFade.height, solidBottom = bottom - wsFade.fraction * wsFade.height;
	float row, next, a, b;
	vec4_t faded;

	if ( y >= bottom || y + h <= top ) return;
	/* the part between the fade zones in one piece */
	a = y > solidTop ? y : solidTop;
	b = y + h < solidBottom ? y + h : solidBottom;
	if ( b > a ) {
		trap_R_SetColor( color );
		trap_R_DrawStretchPic( x, a, w, b - a, s0, t0 + ( t1 - t0 ) * ( a - y ) / h, s1,
		                       t0 + ( t1 - t0 ) * ( b - y ) / h, shader );
	}
	/* the rest row by row; the solid part is skipped */
	a = y > top ? y : top;
	b = y + h < bottom ? y + h : bottom;
	for ( row = a; row < b; row = next ) {
		next = floor( row ) + 1.0f;
		if ( next > b ) next = b;
		if ( row >= solidTop && row < solidBottom ) {
			next = solidBottom;		/* drawn in one piece above */
			continue;
		}
		if ( row < solidTop && next > solidTop ) next = solidTop;
		if ( color ) Vector4Copy( color, faded );
		else { faded[0] = faded[1] = faded[2] = faded[3] = 1.0f; }
		faded[3] *= WS_FadeAlpha( 0.5f * ( row + next ) );
		if ( faded[3] <= 0.0f ) continue;
		trap_R_SetColor( faded );
		trap_R_DrawStretchPic( x, row, w, next - row, s0, t0 + ( t1 - t0 ) * ( row - y ) / h, s1,
		                       t0 + ( t1 - t0 ) * ( next - y ) / h, shader );
	}
	trap_R_SetColor( color );
}

float WS_LU( float value ) {
	return floor( value * 64.0f ) / 64.0f;
}

void WS_DrawText( const wsTextStyle_t *style, float x, float lineTop, const char *text, int length, const float *color ) {
	const wsFontData_t *font;
	const wsAtlas_t *atlas;
	float pen = x, scale, k, baseline;
	int i, s, glyph, previous = -1, used;

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
	for ( i = 0; i < length && text[i]; i += used ) {
		int code = WS_NextCode( font, style, text, i, length, &used );
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
			float gx = WS_Round( pen + rect[4] * k ), gy = baseline - rect[5] * k;
			float s0 = (float)rect[0] / atlas->width, t0 = (float)rect[1] / atlas->height;
			float s1 = (float)( rect[0] + rect[2] ) / atlas->width, t1 = (float)( rect[1] + rect[3] ) / atlas->height;
			float gw = rect[2] * k;
			if ( wsClipRight > wsClipLeft ) {
				if ( gx >= wsClipRight || gx + gw <= wsClipLeft ) {
					pen += font->advance[glyph] * scale + style->spacing;
					continue;
				}
				if ( gx + gw > wsClipRight ) {
					s1 = s0 + ( s1 - s0 ) * ( wsClipRight - gx ) / gw;
					gw = wsClipRight - gx;
				}
				if ( gx < wsClipLeft ) {
					s0 = s1 - ( s1 - s0 ) * ( gx + gw - wsClipLeft ) / gw;
					gw = gx + gw - wsClipLeft;
					gx = wsClipLeft;
				}
			}
			if ( wsFade.height > 0.0f ) {
				WS_DrawFadedGlyph( gx, gy, gw, rect[3] * k, s0, t0, s1, t1, atlas->shader, color );
			} else {
				trap_R_DrawStretchPic( gx, gy, gw, rect[3] * k, s0, t0, s1, t1, atlas->shader );
			}
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
	trap_R_DrawStretchPic( x0, y0, x1 - x0, y1 - y0, 0, 0, 0, 0, WS_WHITE_SHADER );
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

/* An indeterminate <progress>: the track with a moving piece of fill from
 * one fraction to another (both clamped to the bar) */
void WS_DrawProgressRange( float x, float y, float w, float from, float to ) {
	float left = WS_Round( x ), right = WS_Round( x + w ), top = WS_Round( y ) + 4, a, b;

	WS_RegisterArt();
	WS_ProgressPart( left, top, 4, 0, 4, wsProgressTrack );
	WS_ProgressPart( left + 4, top, right - left - 8, 4.5f, 4.5f, wsProgressTrack );
	WS_ProgressPart( right - 4, top, 4, 12, 16, wsProgressTrack );
	a = WS_Round( x + w * WS_Clamp( 0.0f, from, 1.0f ) );
	b = WS_Round( x + w * WS_Clamp( 0.0f, to, 1.0f ) );
	if ( a < left + 4 ) a = left + 4;
	if ( b > right - 4 ) b = right - 4;
	if ( b > a ) WS_ProgressPart( a, top, b - a, 4.5f, 4.5f, wsProgressFill );
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

/* -------------------------------------------------------------------------
   Text styles
   ------------------------------------------------------------------------- */

void WS_LabelStyle( wsTextStyle_t *style ) {
	style->font = WS_MONO;
	style->size = 0.8f * WS_REM;
	style->spacing = 0.14f * 0.8f * WS_REM;
	style->lineHeight = WS_LABEL_LINE;
	style->uppercase = qfalse;
}

void WS_BodyStyle( wsTextStyle_t *style ) {
	style->font = WS_BODY;
	style->size = WS_REM;
	style->spacing = 0.0f;
	style->lineHeight = WS_BODY_LINE;
	style->uppercase = qfalse;
}

float WS_StyledWidth( const wsTextStyle_t *style, const char *text ) {
	char shown[256];
	WS_Transform( style, text, shown, sizeof( shown ) );
	return WS_TextWidth( style, shown, strlen( shown ) );
}

void WS_StyledWrap( const wsTextStyle_t *style, const char *text, float width, wsLines_t *lines ) {
	char shown[256];
	WS_Transform( style, text, shown, sizeof( shown ) );
	WS_Wrap( style, shown, width, lines );
}

float WS_ButtonWidth( const char *label ) {
	wsTextStyle_t style;
	WS_BodyStyle( &style );
	style.font = WS_BODY_BOLD;
	return 2.0f * WS_BUTTON_PAD_X + 2.0f + WS_StyledWidth( &style, label );
}

void WS_DrawLines( const wsTextStyle_t *style, float x, float y, const char *text,
                   const wsLines_t *lines, const float *color ) {
	char shown[256];
	int  i;

	WS_Transform( style, text, shown, sizeof( shown ) );
	for ( i = 0; i < lines->count; i++ ) {
		WS_DrawText( style, x, y + i * style->lineHeight, shown + lines->start[i], lines->length[i], color );
	}
}

void WS_DrawLabel( float x, float y, const char *text, const wsLines_t *lines ) {
	wsTextStyle_t label;
	wsLines_t     single;

	WS_LabelStyle( &label );
	if ( !lines ) {
		WS_Wrap( &label, text, 1.0e6f, &single );
		lines = &single;
	}
	WS_DrawLines( &label, x, y, text, lines, wsAccent );
}

/* -------------------------------------------------------------------------
   Form controls and Chrome's classic scrollbar
   ------------------------------------------------------------------------- */

static const vec4_t wsInput     = { 0x22 / 255.0f, 0x22 / 255.0f, 0x22 / 255.0f, 1.0f };
static const vec4_t wsRing      = { 0xee / 255.0f, 0xee / 255.0f, 0xee / 255.0f, 1.0f };
static const vec4_t wsTrack     = { 0x3b / 255.0f, 0x3b / 255.0f, 0x3b / 255.0f, 1.0f };	/* range, dark scheme */
static const vec4_t wsTrackLine = { 0x85 / 255.0f, 0x85 / 255.0f, 0x85 / 255.0f, 1.0f };
static const vec4_t wsBarTrack  = { 0x2c / 255.0f, 0x2c / 255.0f, 0x2c / 255.0f, 1.0f };	/* dark scrollbar */
static const vec4_t wsBarThumb  = { 0x9f / 255.0f, 0x9f / 255.0f, 0x9f / 255.0f, 1.0f };

static qhandle_t WS_ScrollbarArt( void ) {
	static qhandle_t art;
	if ( !art ) art = trap_R_RegisterShaderNoMip( "menu/ui/web_scrollbar" );
	return art;
}

void WS_DrawInputBox( const wsRect_t *r, qboolean focus ) {
	float x0 = WS_Round( r->x ), y0 = WS_Round( r->y ), x1 = WS_Round( r->x + r->w ), y1 = WS_Round( r->y + r->h );
	if ( focus ) WS_FillRect( x0 - 1, y0 - 1, x1 - x0 + 2, y1 - y0 + 2, wsRing );
	else WS_FillRect( x0, y0, x1 - x0, y1 - y0, wsLine );
	WS_FillRect( x0 + 1, y0 + 1, x1 - x0 - 2, y1 - y0 - 2, wsInput );
}

/* Chrome's menulist starts its text 4px inside the padding; a disabled one
 * looks the same (the pages set its colours) */
void WS_DrawSelect( const wsRect_t *r, float padding, const char *text, qboolean open ) {
	wsTextStyle_t style;
	WS_BodyStyle( &style );
	style.lineHeight = 21.0f;
	WS_DrawInputBox( r, open );
	WS_SetClipX( r->x + 1.0f, r->x + r->w - 20.0f );
	WS_DrawText( &style, r->x + 1.0f + padding + 4.0f, r->y + 1.0f + padding + ( r->h - 2.0f * padding - 23.0f ) * 0.5f,
	             text, strlen( text ), wsWhite );
	WS_SetClipX( 0.0f, 0.0f );
	trap_R_SetColor( wsWhite );
	trap_R_DrawStretchPic( WS_Round( r->x + r->w ) - 14.0f, WS_Round( r->y + r->h * 0.5f ) - 3.0f, 10, 7, 0, 25 / 32.0f, 10 / 16.0f, 1.0f,
	                       WS_ScrollbarArt() );
	trap_R_SetColor( NULL );
}

/* input[type=range] in the dark scheme with accent-color: an 8px track with
 * a grey border, the accent up to the 16px thumb, in the content box below
 * the vertical padding */
void WS_DrawRange( const wsRect_t *r, float padding, float fraction, qboolean disabled ) {
	float x0 = r->x + 1.0f, w = r->w - 2.0f, top = r->y + 1.0f + padding, cx, cy, radius = 8.0f;
	float tx0 = WS_Round( x0 ), tx1 = WS_Round( x0 + w ), ty = WS_Round( top + 4.0f ), fill;
	vec4_t accent;
	int j;
	Vector4Copy( wsAccent, accent );
	if ( disabled ) accent[3] = 0.5f;
	cx = x0 + radius + fraction * ( w - 2.0f * radius );
	cy = top + radius;
	fill = WS_Round( cx );
	/* the track: rounded ends, 1px border */
	WS_FillRect( tx0 + 2, ty, tx1 - tx0 - 4, 1, wsTrackLine );
	WS_FillRect( tx0 + 2, ty + 7, tx1 - tx0 - 4, 1, wsTrackLine );
	WS_FillRect( tx0, ty + 2, 1, 4, wsTrackLine );
	WS_FillRect( tx1 - 1, ty + 2, 1, 4, wsTrackLine );
	WS_FillRect( tx0 + 1, ty + 1, 1, 1, wsTrackLine );
	WS_FillRect( tx0 + 1, ty + 6, 1, 1, wsTrackLine );
	WS_FillRect( tx1 - 2, ty + 1, 1, 1, wsTrackLine );
	WS_FillRect( tx1 - 2, ty + 6, 1, 1, wsTrackLine );
	WS_FillRect( tx0 + 1, ty + 2, 1, 4, accent );
	WS_FillRect( tx0 + 2, ty + 1, fill - tx0 - 2, 6, accent );
	WS_FillRect( fill, ty + 1, tx1 - 2 - fill, 6, wsTrack );
	WS_FillRect( tx1 - 2, ty + 2, 1, 4, wsTrack );
	/* the thumb: a 16px disc */
	for ( j = 0; j < 16; j++ ) {
		float dy = j + 0.5f - radius, half = sqrt( radius * radius - dy * dy );
		float a = WS_Round( cx - half ), b = WS_Round( cx + half );
		if ( b > a ) WS_FillRect( a, WS_Round( cy - radius ) + j, b - a, 1, accent );
	}
}

void WS_ScrollThumb( const wsRect_t *port, float content, float scroll, wsRect_t *thumb ) {
	float track = port->h - 2.0f * WS_SCROLLBAR_ARROW - 9.0f;
	thumb->h = content > 0.0f ? track * port->h / content : track;
	if ( thumb->h < 9.0f ) thumb->h = 9.0f;
	thumb->w = 9.0f;
	thumb->x = port->x + port->w - WS_SCROLLBAR_GUTTER + 3.0f;
	thumb->y = port->y + WS_SCROLLBAR_ARROW + 2.5f +
		( content > port->h ? ( track - thumb->h ) * scroll / ( content - port->h ) : 0.0f );
}

void WS_DrawScrollbar( const wsRect_t *port, const wsRect_t *thumb ) {
	float x = WS_Round( port->x + port->w - WS_SCROLLBAR_GUTTER ), top = WS_Round( port->y );
	float bottom = WS_Round( port->y + port->h ), y0, y1;
	qhandle_t art = WS_ScrollbarArt();

	WS_FillRect( x, top, WS_SCROLLBAR_GUTTER, bottom - top, wsBarTrack );
	trap_R_SetColor( wsBarThumb );
	trap_R_DrawStretchPic( x, top, 15, 15, 0, 0, 15 / 16.0f, 15 / 32.0f, art );
	trap_R_DrawStretchPic( x, bottom - 15, 15, 15, 0, 15 / 32.0f, 15 / 16.0f, 0, art );
	y0 = WS_Round( thumb->y );
	y1 = floor( thumb->y + thumb->h ) + 1.0f;
	trap_R_DrawStretchPic( x, y0, 15, 5, 0, 15 / 32.0f, 15 / 16.0f, 20 / 32.0f, art );
	trap_R_DrawStretchPic( x, y1 - 5, 15, 5, 0, 20 / 32.0f, 15 / 16.0f, 25 / 32.0f, art );
	trap_R_SetColor( NULL );
	if ( y1 - 5 > y0 + 5 ) WS_FillRect( x + 3, y0 + 5, 9, y1 - y0 - 10, wsBarThumb );
}
