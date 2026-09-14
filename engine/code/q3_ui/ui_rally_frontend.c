/*
===========================================================================
Copyright (C) 2002-2026 Q3Rally Team
===========================================================================
*/

#include "ui_local.h"
#include "ui_rally_frontend.h"

static vec4_t frontendPanelColor   = UI_FRONTEND_COLOR_PANEL;
static vec4_t frontendCardColor    = UI_FRONTEND_COLOR_PANEL_ALT;
static vec4_t frontendFocusColor   = UI_FRONTEND_COLOR_FOCUS_BG;
static vec4_t frontendBorderColor  = UI_FRONTEND_COLOR_BORDER;
static vec4_t frontendAccentColor  = UI_FRONTEND_COLOR_ACCENT;
static vec4_t frontendTextColor    = UI_FRONTEND_COLOR_TEXT;
static vec4_t frontendMutedColor   = UI_FRONTEND_COLOR_MUTED;
static vec4_t frontendShadowColor  = UI_FRONTEND_COLOR_SHADOW;
static vec4_t frontendProgressColor = UI_FRONTEND_COLOR_PROGRESS;
static vec4_t frontendHeroOverlayColor = UI_FRONTEND_COLOR_HERO_OVERLAY;

/* The legacy UI text path treats every glyph as a full 8/16 pixel cell.
 * The modern screens use the same atlas, but with a tighter advance and a
 * slightly wider glyph quad. This keeps the type readable without the wide,
 * arcade-menu tracking of the original renderer. */
static int Frontend_TextHeight( int style ) {
    if ( style & UI_SMALLFONT ) {
        return SMALLCHAR_HEIGHT;
    }
    if ( style & UI_GIANTFONT ) {
        return GIANTCHAR_HEIGHT;
    }
    return BIGCHAR_HEIGHT;
}

static int Frontend_TextAdvance( int ch, int style ) {
    int advance;

    if ( style & UI_SMALLFONT ) {
        advance = 6;
    } else if ( style & UI_GIANTFONT ) {
        advance = 18;
    } else {
        advance = 12;
    }

    if ( ch == ' ' ) {
        return ( advance + 1 ) / 2;
    }

    /* Give naturally narrow glyphs less horizontal room while keeping the
     * atlas sampling simple and stable for every character. */
    if ( ch == 'I' || ch == 'i' || ch == 'l' || ch == '!' ||
         ch == '|' || ch == '.' || ch == ',' || ch == ':' || ch == ';' ) {
        return ( advance + 1 ) / 2;
    }

    return advance;
}

static int Frontend_TextQuadWidth( int style ) {
    if ( style & UI_SMALLFONT ) {
        return 10;
    }
    if ( style & UI_GIANTFONT ) {
        return 28;
    }
    return 20;
}

static int Frontend_TextWidthRaw( const char *text, int style ) {
    const char *s;
    int width;

    if ( !text ) {
        return 0;
    }

    width = 0;
    for ( s = text; *s; s++ ) {
        if ( Q_IsColorString( s ) ) {
            s++;
            continue;
        }
        width += Frontend_TextAdvance( *s & 255, style );
    }
    return width;
}

static void Frontend_DrawTextRaw( int x, int y, const char *text,
                                  int style, const float *color ) {
    const char *s;
    int charHeight;
    int quadWidth;
    int cursorX;
    vec4_t drawColor;

    if ( !text || !text[0] ) {
        return;
    }

    charHeight = Frontend_TextHeight( style );
    quadWidth = Frontend_TextQuadWidth( style );
    cursorX = x;
    Vector4Copy( color, drawColor );

    trap_R_SetColor( drawColor );
    for ( s = text; *s; s++ ) {
        int ch;
        int advance;
        float ax;
        float ay;
        float aw;
        float ah;
        float frow;
        float fcol;

        if ( Q_IsColorString( s ) ) {
            s++;
            continue;
        }

        ch = *s & 255;
        advance = Frontend_TextAdvance( ch, style );
        if ( ch == ' ' ) {
            cursorX += advance;
            continue;
        }

        ax = cursorX * uis.xscale + uis.bias;
        ay = y * uis.yscale;
        aw = quadWidth * uis.xscale;
        ah = charHeight * uis.yscale;
        frow = ( ch >> 4 ) * 0.0625f;
        fcol = ( ch & 15 ) * 0.0625f;
        trap_R_DrawStretchPic( ax, ay, aw, ah,
                               fcol, frow, fcol + 0.0625f,
                               frow + 0.0625f, uis.charset );
        cursorX += advance;
    }
    trap_R_SetColor( NULL );
}

int Frontend_TextWidth( const char *text, int style ) {
    return Frontend_TextWidthRaw( text, style );
}

void Frontend_DrawText( int x, int y, const char *text, int style,
                        const float *color ) {
    int width;
    int format;
    vec4_t drawColor;
    vec4_t dropColor;

    if ( !text || !color ) {
        return;
    }

    Vector4Copy( color, drawColor );
    if ( style & UI_PULSE ) {
        vec4_t lowlight;

        lowlight[0] = drawColor[0] * 0.8f;
        lowlight[1] = drawColor[1] * 0.8f;
        lowlight[2] = drawColor[2] * 0.8f;
        lowlight[3] = drawColor[3] * 0.8f;
        UI_LerpColor( drawColor, lowlight, drawColor,
                      0.5f + 0.5f * sin( uis.realtime / PULSE_DIVISOR ) );
    }

    width = Frontend_TextWidthRaw( text, style );
    format = style & UI_FORMATMASK;
    if ( format == UI_CENTER ) {
        x -= width / 2;
    } else if ( format == UI_RIGHT ) {
        x -= width;
    }

    if ( style & UI_DROPSHADOW ) {
        dropColor[0] = 0.0f;
        dropColor[1] = 0.0f;
        dropColor[2] = 0.0f;
        dropColor[3] = drawColor[3];
        Frontend_DrawTextRaw( x + 2, y + 2, text, style, dropColor );
    }
    Frontend_DrawTextRaw( x, y, text, style, drawColor );
}

static void Frontend_ColorWithAlpha( vec4_t out, const float *baseColor,
                                     float alpha ) {
    out[0] = baseColor[0];
    out[1] = baseColor[1];
    out[2] = baseColor[2];
    out[3] = baseColor[3] * alpha;
}

void Frontend_DrawPanel( int x, int y, int width, int height,
                         float alpha, int style ) {
    vec4_t fillColor;
    vec4_t borderColor;
    vec4_t accentColor;
    vec4_t shadowColor;

    Frontend_ColorWithAlpha( borderColor, frontendBorderColor, alpha );
    Frontend_ColorWithAlpha( accentColor, frontendAccentColor, alpha );

    if ( style == UI_FRONTEND_STYLE_FRAME ) {
        UI_DrawRect( x, y, width, height, borderColor );
        UI_FillRect( x, y, width, UI_FRONTEND_PANEL_TOPBAR, accentColor );
        return;
    }

    if ( style == UI_FRONTEND_STYLE_ACTIVE ) {
        Frontend_ColorWithAlpha( fillColor, frontendFocusColor, alpha );
    } else if ( style == UI_FRONTEND_STYLE_CARD ) {
        Frontend_ColorWithAlpha( fillColor, frontendCardColor, alpha );
    } else {
        Frontend_ColorWithAlpha( fillColor, frontendPanelColor, alpha );
    }

    if ( style != UI_FRONTEND_STYLE_SURFACE ) {
        Frontend_ColorWithAlpha( shadowColor, frontendShadowColor, alpha );
        UI_FillRect( x + UI_FRONTEND_PANEL_SHADOW,
                     y + UI_FRONTEND_PANEL_SHADOW,
                     width, height, shadowColor );
    }

    UI_FillRect( x, y, width, height, fillColor );
    UI_FillRect( x, y, width, UI_FRONTEND_PANEL_TOPBAR, borderColor );

    if ( style == UI_FRONTEND_STYLE_ACTIVE ) {
        UI_FillRect( x, y, UI_FRONTEND_PANEL_TOPBAR, height, accentColor );
    }
}

void Frontend_DrawCard( int x, int y, int width, int height,
                        float alpha, qboolean active ) {
    Frontend_DrawPanel( x, y, width, height, alpha,
                        active ? UI_FRONTEND_STYLE_ACTIVE : UI_FRONTEND_STYLE_CARD );
}

qboolean Frontend_DrawButton( int x, int y, int width, int height,
                              const char *label, float alpha,
                              qboolean active, int textAlign ) {
    vec4_t textColor;
    qboolean hovered;
    qboolean highlighted;

    hovered = ( uis.cursorx >= x && uis.cursorx <= x + width &&
                uis.cursory >= y && uis.cursory <= y + height ) ? qtrue : qfalse;
    highlighted = ( active || hovered ) ? qtrue : qfalse;

    Frontend_DrawPanel( x, y, width, height, alpha,
                        highlighted ? UI_FRONTEND_STYLE_ACTIVE : UI_FRONTEND_STYLE_CARD );

    if ( highlighted ) {
        Frontend_ColorWithAlpha( textColor, frontendAccentColor, alpha );
    } else {
        Frontend_ColorWithAlpha( textColor, frontendTextColor, alpha );
    }

    Frontend_DrawText( x + ( textAlign == UI_CENTER ? width / 2 : UI_FRONTEND_SPACE_MD ),
                       y + ( height - SMALLCHAR_HEIGHT ) / 2,
                       label, textAlign | UI_SMALLFONT | UI_DROPSHADOW, textColor );

    return hovered;
}

qboolean Frontend_DrawNavButton( int x, int y, int width, int height,
                                 const char *label, float alpha,
                                 qboolean active, int textAlign ) {
    vec4_t textColor;
    qboolean hovered;
    qboolean highlighted;

    hovered = ( uis.cursorx >= x && uis.cursorx <= x + width &&
                uis.cursory >= y && uis.cursory <= y + height ) ? qtrue : qfalse;
    highlighted = ( active || hovered ) ? qtrue : qfalse;

    /* Navigation stays visually quiet until it is selected. */
    if ( highlighted ) {
        Frontend_DrawPanel( x, y, width, height, alpha,
                            UI_FRONTEND_STYLE_ACTIVE );
    }

    if ( highlighted ) {
        Frontend_ColorWithAlpha( textColor, frontendAccentColor, alpha );
    } else {
        Frontend_ColorWithAlpha( textColor, frontendMutedColor, alpha );
    }

    Frontend_DrawText( x + ( textAlign == UI_CENTER ? width / 2 : UI_FRONTEND_SPACE_MD ),
                       y + ( height - SMALLCHAR_HEIGHT ) / 2,
                       label, textAlign | UI_SMALLFONT | UI_DROPSHADOW, textColor );
    return hovered;
}

void Frontend_DrawStatusChip( int x, int y, const char *label,
                              const float *statusColor, float alpha ) {
    vec4_t dotColor;
    vec4_t textColor;

    Frontend_ColorWithAlpha( dotColor, statusColor, alpha );
    Frontend_ColorWithAlpha( textColor, frontendAccentColor, alpha );
    UI_FillRect( x, y + 3, UI_FRONTEND_STATUS_DOT, UI_FRONTEND_STATUS_DOT, dotColor );
    Frontend_DrawText( x + UI_FRONTEND_STATUS_DOT + UI_FRONTEND_SPACE_SM, y,
                       label, UI_LEFT | UI_SMALLFONT, textColor );
}

void Frontend_DrawSidebar( int x, int y, int width, int height,
                           const char *title, float alpha ) {
    vec4_t titleColor;

    Frontend_DrawPanel( x, y, width, height, alpha, UI_FRONTEND_STYLE_SURFACE );
    Frontend_ColorWithAlpha( titleColor, frontendMutedColor, alpha );
    Frontend_DrawText( x + UI_FRONTEND_SPACE_LG, y + 44, title,
                       UI_LEFT | UI_SMALLFONT, titleColor );
}

void Frontend_DrawVehicleHero( int x, int y, int width, int height,
                               playerInfo_t *playerInfo, int realtime,
                               float alpha ) {
    vec4_t overlayColor;

    Frontend_ColorWithAlpha( overlayColor, frontendHeroOverlayColor, alpha );
    UI_FillRect( x, y, width, height, overlayColor );
    Frontend_DrawPanel( x, y, width, height, alpha, UI_FRONTEND_STYLE_FRAME );
    UI_DrawPlayer( x, y, width, height, playerInfo, realtime );
}

void Frontend_DrawProgress( int x, int y, int width, int height,
                            float progress, float alpha ) {
    vec4_t trackColor;
    vec4_t fillColor;
    vec4_t borderColor;
    int fillWidth;

    if ( progress < 0.0f ) {
        progress = 0.0f;
    } else if ( progress > 1.0f ) {
        progress = 1.0f;
    }

    Frontend_ColorWithAlpha( trackColor, frontendProgressColor, alpha );
    Frontend_ColorWithAlpha( fillColor, frontendAccentColor, alpha );
    Frontend_ColorWithAlpha( borderColor, frontendBorderColor, alpha );

    UI_FillRect( x, y, width, height, trackColor );
    fillWidth = (int)( width * progress );
    if ( fillWidth > 0 ) {
        UI_FillRect( x, y, fillWidth, height, fillColor );
        UI_FillRect( x, y, fillWidth, UI_FRONTEND_PANEL_TOPBAR, borderColor );
    }
    UI_DrawRect( x, y, width, height, borderColor );
}
