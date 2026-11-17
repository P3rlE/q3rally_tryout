/*
===========================================================================
Copyright (C) 2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================

Native drawing in the look of the WebUI (engine/webui/garage/garage.css).

Everything here works in real screen pixels, which are CSS pixels for the
WebUI at a device pixel ratio of 1, so a native screen can follow the CSS
rules number for number. Text is laid out like Chrome: linear advances, the
font's pair kerning, letter-spacing after every glyph, line boxes with the
half-leading rounded down.
*/

#ifndef UI_RALLY_WEBSTYLE_H
#define UI_RALLY_WEBSTYLE_H

typedef enum {
	WS_MONO,				/* Share Tech Mono, .label */
	WS_BODY,				/* Barlow 400, body text */
	WS_BODY_BOLD,			/* Barlow at weight 750 (synthesised, like Chrome) */
	WS_CONDENSED,			/* Barlow Condensed 700, h1/h2, uppercase only */
	WS_CONDENSED_ITALIC,	/* the same, synthesised italic for <em> */
	WS_NUM_FONTS
} wsFont_t;

/* The WebUI reserves room for a scrollbar (html { scrollbar-gutter: stable }). */
#define WS_SCROLLBAR_GUTTER		15.0f
#define WS_REM					16.0f
/* WS_BODY's tabular figures 0-9 (font-variant-numeric: tabular-nums) */
#define WS_TABULAR_DIGIT		0x80

/* palette (garage.css) */
extern const vec4_t wsAccent;		/* --accent #ff5500 */
extern const vec4_t wsLine;			/* --line #373737 */
extern const vec4_t wsSurface;		/* section #171717 */
extern const vec4_t wsWhite;
extern const vec4_t wsMuted;		/* #999 */
extern const vec4_t wsSummary;		/* #bbb, #profile-summary */
extern const vec4_t wsButtonText;	/* #141414 */

typedef struct {
	float	x, y, w, h;
} wsRect_t;

/* A run of text in one font, as Chrome places it. */
typedef struct {
	wsFont_t	font;
	float		size;			/* px */
	float		spacing;		/* letter-spacing, px */
	float		lineHeight;		/* px */
	qboolean	uppercase;		/* text-transform: uppercase */
} wsTextStyle_t;

#define WS_MAX_LINES	8

typedef struct {
	int		count;
	int		start[WS_MAX_LINES];	/* offsets into the (transformed) text */
	int		length[WS_MAX_LINES];
	float	width[WS_MAX_LINES];
} wsLines_t;

float	WS_Clamp( float low, float value, float high );
float	WS_Round( float value );
float	WS_ViewportWidth( void );
float	WS_ViewportHeight( void );

qboolean WS_FontReady( wsFont_t font );
void	WS_Transform( const wsTextStyle_t *style, const char *in, char *out, int size );
float	WS_TextWidth( const wsTextStyle_t *style, const char *text, int length );
/* Content area and baseline of a line box, relative to the line top. */
float	WS_TextTop( const wsTextStyle_t *style );
float	WS_Baseline( const wsTextStyle_t *style );
/* Greedy line breaking at spaces, like a block of normal white-space. */
void	WS_Wrap( const wsTextStyle_t *style, const char *text, float maxWidth, wsLines_t *lines );
/* Chrome's LayoutUnit: lengths cut to 1/64 px */
float	WS_LU( float value );
/* A vertical fade mask over a box for the text drawn after it (fraction
 * 0.12 for 12% fades at top and bottom); a height of 0 turns it off. */
void	WS_SetFade( float top, float height, float fraction );
float	WS_FadeAlpha( float y );
void	WS_DrawText( const wsTextStyle_t *style, float x, float lineTop, const char *text, int length, const float *color );

void	WS_FillRect( float x, float y, float w, float h, const float *color );
/* A CSS section: background, 1px border, accent top border. */
void	WS_DrawSection( const wsRect_t *box, qboolean showroom );
/* body background, tiled like Chrome at the size of the html box */
void	WS_DrawBackground( float pageWidth, float pageHeight );
/* <progress> in the dark color scheme, 16px box */
void	WS_DrawProgress( float x, float y, float w, float fraction );
/* primary or .secondary button, box given, label centered on its line */
void	WS_DrawButton( const wsRect_t *box, const char *label, qboolean secondary, qboolean focused );

#endif
