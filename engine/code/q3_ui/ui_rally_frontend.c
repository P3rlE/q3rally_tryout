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
static vec4_t frontendProgressColor = UI_FRONTEND_COLOR_PROGRESS;
static vec4_t frontendHeroOverlayColor = UI_FRONTEND_COLOR_HERO_OVERLAY;
static const char *frontendBackgroundNames[] = {
    "gfx/ui/q3rally_frontend_bg",
    "gfx/ui/q3rally_frontend_bg_alt",
    "gfx/ui/q3rally_frontend_bg_alt2",
    "gfx/ui/q3rally_frontend_bg_alt3"
};
static qhandle_t frontendBackgroundShaders[ARRAY_LEN( frontendBackgroundNames )];
static qhandle_t frontendBackgroundShader;
static qboolean frontendBackgroundAttempted;
static menuframework_s *frontendBackgroundMenu;
static int frontendBackgroundIndex = -1;
static qhandle_t frontendFontAtlases[3];

#define FRONTEND_FONT_ATLAS_256 0
#define FRONTEND_FONT_ATLAS_512 1
#define FRONTEND_FONT_ATLAS_1024 2
#define FRONTEND_FONT_BASE_RASTER_HEIGHT 29.0f

qhandle_t Frontend_BackgroundShader( void ) {
    int i;
    int nextIndex;

    if ( !frontendBackgroundAttempted ) {
        frontendBackgroundAttempted = qtrue;
        for ( i = 0; i < ARRAY_LEN( frontendBackgroundNames ); i++ ) {
            frontendBackgroundShaders[i] = trap_R_RegisterShaderNoMip(
                frontendBackgroundNames[i] );
        }
    }

    /* Pick once when a frontend menu becomes active. This keeps the image
     * stable while a screen is open, but gives each screen a fresh backdrop. */
    if ( !frontendBackgroundShader || uis.activemenu != frontendBackgroundMenu ) {
        frontendBackgroundMenu = uis.activemenu;

        nextIndex = UI_RandomInt( ARRAY_LEN( frontendBackgroundNames ) );
        if ( ARRAY_LEN( frontendBackgroundNames ) > 1 &&
             nextIndex == frontendBackgroundIndex ) {
            nextIndex = ( nextIndex + 1 ) % ARRAY_LEN( frontendBackgroundNames );
        }
        frontendBackgroundIndex = nextIndex;
        frontendBackgroundShader = frontendBackgroundShaders[nextIndex];
    }

    return frontendBackgroundShader ? frontendBackgroundShader : uis.menuBackShader;
}

void Frontend_DrawBackground( const float *scrimColor ) {
    UI_SetColor( NULL );
    UI_DrawBackground( Frontend_BackgroundShader() );
    if ( scrimColor ) {
        UI_FillRect( -uis.bias, 0, SCREEN_WIDTH + uis.bias * 2,
                     SCREEN_HEIGHT, scrimColor );
    }
}

/* Match the in-game rally font's square glyph cells and tracking. Small and
 * regular text use the same 14/24px cells as CG_DrawIngameString; giant text
 * stays a 2x regular cell, matching the UI's established hierarchy. */
static int Frontend_TextBaseHeight( int style ) {
    if ( style & UI_SMALLFONT ) {
        return 14;
    }
    if ( style & UI_GIANTFONT ) {
        return 48;
    }
    return 24;
}

static float Frontend_TextTierHeight( frontendTextTier_t tier ) {
	switch ( tier ) {
	case FRONTEND_TEXT_TIER_MICRO:
		return 7.0f;
	case FRONTEND_TEXT_TIER_LABEL:
		return 8.5f;
	case FRONTEND_TEXT_TIER_BODY:
		return 10.0f;
	case FRONTEND_TEXT_TIER_VALUE:
		return 14.0f;
	case FRONTEND_TEXT_TIER_HEADING:
		return 18.0f;
	case FRONTEND_TEXT_TIER_DISPLAY:
	default:
		return 24.0f;
	}
}

float Frontend_TextDefaultScale( int style ) {
	frontendTextTier_t tier;

	if ( style & UI_SMALLFONT ) {
		tier = FRONTEND_TEXT_TIER_BODY;
	} else if ( style & UI_GIANTFONT ) {
		tier = FRONTEND_TEXT_TIER_DISPLAY;
	} else {
		tier = FRONTEND_TEXT_TIER_HEADING;
	}
	return Frontend_TextTierHeight( tier ) /
	       (float)Frontend_TextBaseHeight( style );
}

int Frontend_TextHeight( int style ) {
	return (int)( Frontend_TextBaseHeight( style ) *
	              Frontend_TextDefaultScale( style ) + 0.5f );
}

static float Frontend_TextTierScale( int style, frontendTextTier_t tier ) {
	return Frontend_TextTierHeight( tier ) /
	       (float)Frontend_TextBaseHeight( style );
}

static int Frontend_TextAdvance( int ch, int style ) {
    int advance;

    if ( style & UI_SMALLFONT ) {
        advance = 14;
    } else if ( style & UI_GIANTFONT ) {
        advance = 48;
    } else {
        advance = 24;
    }

    if ( ch == ' ' ) {
        return ( advance + 1 ) / 2;
    }
    return advance;
}

static int Frontend_TextQuadWidth( int ch, int style ) {
    (void)ch;
    if ( style & UI_SMALLFONT ) {
        return 14;
    }
    if ( style & UI_GIANTFONT ) {
        return 48;
    }
    return 24;
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

static qhandle_t Frontend_FontAtlas( float scale, int style ) {
	float glyphHeight;
	int tier;
	qhandle_t shader;
	char shaderName[MAX_QPATH];

	if ( !frontendFontAtlases[FRONTEND_FONT_ATLAS_512] ) {
		frontendFontAtlases[FRONTEND_FONT_ATLAS_512] = uis.charset;
	}

	glyphHeight = Frontend_TextBaseHeight( style ) * scale * uis.yscale;
	tier = FRONTEND_FONT_ATLAS_512;
	if ( glyphHeight < FRONTEND_FONT_BASE_RASTER_HEIGHT * 0.70710678f ) {
		tier = FRONTEND_FONT_ATLAS_256;
	} else if ( glyphHeight > FRONTEND_FONT_BASE_RASTER_HEIGHT * 1.41421356f ) {
		tier = FRONTEND_FONT_ATLAS_1024;
	}

	shader = frontendFontAtlases[tier];
	if ( !shader && tier != FRONTEND_FONT_ATLAS_512 ) {
		Com_sprintf( shaderName, sizeof( shaderName ),
		             "gfx/ui/ingame_charset_%d.png",
		             tier == FRONTEND_FONT_ATLAS_256 ? 256 : 1024 );
		shader = trap_R_RegisterShaderNoMip( shaderName );
		frontendFontAtlases[tier] = shader;
	}
	return shader ? shader : frontendFontAtlases[FRONTEND_FONT_ATLAS_512];
}

static int Frontend_TextVisualWidthRaw( const char *text, int style ) {
    const char *s;
    int cursorX;
    int visualWidth;

    if ( !text ) {
        return 0;
    }

    cursorX = 0;
    visualWidth = 0;
    for ( s = text; *s; s++ ) {
        int ch;
        int glyphRight;

        if ( Q_IsColorString( s ) ) {
            s++;
            continue;
        }

        ch = *s & 255;
        if ( ch != ' ' ) {
            glyphRight = cursorX + Frontend_TextQuadWidth( ch, style );
            if ( glyphRight > visualWidth ) {
                visualWidth = glyphRight;
            }
        }
        cursorX += Frontend_TextAdvance( ch, style );
    }

    if ( cursorX > visualWidth ) {
        visualWidth = cursorX;
    }
    return visualWidth;
}

static void Frontend_DrawTextRaw( float x, int y, const char *text,
                                  int style, float scale,
                                  const float *color ) {
    const char *s;
    float charHeight;
    float cursorX;
    qhandle_t charset;
    vec4_t drawColor;

    if ( !text || !text[0] ) {
        return;
    }

    charHeight = Frontend_TextBaseHeight( style ) * scale;
    charset = Frontend_FontAtlas( scale, style );
    if ( !charset ) {
        return;
    }
    cursorX = x;
    Vector4Copy( color, drawColor );

    trap_R_SetColor( drawColor );
    for ( s = text; *s; s++ ) {
        int ch;
        float advance;
        float quadWidth;
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
        advance = Frontend_TextAdvance( ch, style ) * scale;
        quadWidth = Frontend_TextQuadWidth( ch, style ) * scale;
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
                               frow + 0.0625f, charset );
        cursorX += advance;
    }
    trap_R_SetColor( NULL );
}

int Frontend_TextWidth( const char *text, int style ) {
    return (int)( Frontend_TextWidthRaw( text, style ) *
                  Frontend_TextDefaultScale( style ) + 0.5f );
}

int Frontend_TextVisualWidth( const char *text, int style ) {
    return (int)( Frontend_TextVisualWidthRaw( text, style ) *
                  Frontend_TextDefaultScale( style ) + 0.5f );
}

void Frontend_DrawTextScaled( int x, int y, const char *text, int style,
                              float scale, const float *color ) {
    float width;
    int format;
    float drawX;
    vec4_t drawColor;
    vec4_t dropColor;

    if ( !text || !color || scale <= 0.0f ) {
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

    width = Frontend_TextWidthRaw( text, style ) * scale;
    format = style & UI_FORMATMASK;
    drawX = (float)x;
    if ( format == UI_CENTER ) {
        drawX -= width * 0.5f;
    } else if ( format == UI_RIGHT ) {
        drawX -= width;
    }

    if ( style & UI_DROPSHADOW ) {
        dropColor[0] = 0.0f;
        dropColor[1] = 0.0f;
        dropColor[2] = 0.0f;
        dropColor[3] = drawColor[3];
        Frontend_DrawTextRaw( drawX + 2.0f, y + 2, text, style,
                              scale, dropColor );
    }
    Frontend_DrawTextRaw( drawX, y, text, style, scale, drawColor );
}

void Frontend_DrawText( int x, int y, const char *text, int style,
                        const float *color ) {
    Frontend_DrawTextScaled( x, y, text, style,
                             Frontend_TextDefaultScale( style ), color );
}

void Frontend_DrawTextTier( int x, int y, const char *text, int style,
                            frontendTextTier_t tier, const float *color ) {
	Frontend_DrawTextScaled( x, y, text, style,
	                         Frontend_TextTierScale( style, tier ), color );
}

void Frontend_DrawTextFitted( int x, int y, int maxWidth, const char *text,
                              int style, const float *color ) {
	float scale;
	int textWidth;

	if ( !text || !text[0] || maxWidth <= 0 ) {
		return;
	}

	scale = Frontend_TextDefaultScale( style );
	textWidth = Frontend_TextWidth( text, style );
	if ( textWidth > maxWidth ) {
		scale *= (float)maxWidth / (float)textWidth;
	}
	Frontend_DrawTextScaled( x, y, text, style, scale, color );
}

void Frontend_DrawTextTierFitted( int x, int y, int maxWidth, const char *text,
                                  int style, frontendTextTier_t tier,
                                  const float *color ) {
	float scale;
	int textWidth;

	if ( !text || !text[0] || maxWidth <= 0 ) {
		return;
	}

	scale = Frontend_TextTierScale( style, tier );
	textWidth = (int)( Frontend_TextWidthRaw( text, style ) * scale + 0.5f );
	if ( textWidth > maxWidth ) {
		scale *= (float)maxWidth / (float)textWidth;
	}
	Frontend_DrawTextScaled( x, y, text, style, scale, color );
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

    Frontend_ColorWithAlpha( borderColor, frontendBorderColor, alpha );
    Frontend_ColorWithAlpha( accentColor, frontendAccentColor, alpha );

    if ( style == UI_FRONTEND_STYLE_FRAME ) {
        /* Frames are intentionally open: one signal line gives the surface
         * an edge without turning it into another boxed window. */
        UI_FillRect( x, y, width, UI_FRONTEND_PANEL_TOPBAR, accentColor );
        UI_FillRect( x, y + height - 1, width, 1, borderColor );
        return;
    }

    if ( style == UI_FRONTEND_STYLE_ACTIVE ) {
        Frontend_ColorWithAlpha( fillColor, frontendFocusColor, alpha );
    } else if ( style == UI_FRONTEND_STYLE_CARD ) {
        Frontend_ColorWithAlpha( fillColor, frontendCardColor, alpha );
    } else {
        Frontend_ColorWithAlpha( fillColor, frontendPanelColor, alpha );
    }

    UI_FillRect( x, y, width, height, fillColor );

    if ( style == UI_FRONTEND_STYLE_ACTIVE ) {
        UI_FillRect( x, y, 2, height, accentColor );
    } else if ( style == UI_FRONTEND_STYLE_CARD ) {
        /* Cards get one quiet hairline instead of a complete border. */
        borderColor[3] *= 0.65f;
        UI_FillRect( x, y, width, 1, borderColor );
    }
}

void Frontend_DrawCard( int x, int y, int width, int height,
                        float alpha, qboolean active ) {
    Frontend_DrawPanel( x, y, width, height, alpha,
                        active ? UI_FRONTEND_STYLE_ACTIVE : UI_FRONTEND_STYLE_CARD );
}

static qboolean Frontend_DrawButtonInternal( int x, int y, int width, int height,
                                             const char *label, float alpha,
                                             qboolean active, qboolean allowHover,
                                             int textAlign ) {
    vec4_t buttonColor;
    vec4_t borderColor;
    vec4_t accentColor;
    vec4_t textColor;
    qboolean hovered;
    qboolean highlighted;
    int textWidth;
    int textHeight;
    int textX;
    int textY;
    int availableWidth;
    float textScale;

    hovered = ( uis.cursorx >= x && uis.cursorx <= x + width &&
                uis.cursory >= y && uis.cursory <= y + height ) ? qtrue : qfalse;
    highlighted = ( active || ( allowHover && hovered ) ) ? qtrue : qfalse;

    Frontend_ColorWithAlpha( buttonColor, frontendFocusColor, alpha );
    Frontend_ColorWithAlpha( borderColor, frontendBorderColor, alpha * 0.70f );
    Frontend_ColorWithAlpha( accentColor, frontendAccentColor, alpha );

    /* Flat controls are transparent at rest. Interaction is communicated by
     * a soft wash and an underline, rather than a raised rectangle. */
    if ( highlighted ) {
        buttonColor[3] *= 0.58f;
        UI_FillRect( x, y, width, height, buttonColor );
    }
    UI_FillRect( x, y + height - 1, width, 1, borderColor );
    if ( highlighted ) {
        UI_FillRect( x, y + height - 2, width, 2, accentColor );
    }

    if ( highlighted ) {
        Frontend_ColorWithAlpha( textColor, frontendAccentColor, alpha );
    } else {
        Frontend_ColorWithAlpha( textColor, frontendTextColor, alpha );
    }

    availableWidth = width - 2 * UI_FRONTEND_SPACE_MD;
    if ( availableWidth < 1 ) {
        availableWidth = 1;
    }
    textWidth = Frontend_TextVisualWidth( label, UI_SMALLFONT );
    textScale = Frontend_TextDefaultScale( UI_SMALLFONT );
    if ( textWidth > availableWidth ) {
        textScale *= (float)availableWidth / (float)textWidth;
    }
    textHeight = (int)( Frontend_TextBaseHeight( UI_SMALLFONT ) * textScale + 0.5f );
    textX = ( textAlign == UI_CENTER ) ? x + width / 2 : x + UI_FRONTEND_SPACE_MD;
    textY = y + ( height - textHeight ) / 2;
    Frontend_DrawTextScaled( textX, textY, label, textAlign | UI_SMALLFONT,
                             textScale, textColor );

    return hovered;
}

qboolean Frontend_DrawButton( int x, int y, int width, int height,
                              const char *label, float alpha,
                              qboolean active, int textAlign ) {
    return Frontend_DrawButtonInternal( x, y, width, height, label, alpha,
                                        active, qtrue, textAlign );
}

qboolean Frontend_DrawButtonFocused( int x, int y, int width, int height,
                                     const char *label, float alpha,
                                     qboolean active, int textAlign ) {
    return Frontend_DrawButtonInternal( x, y, width, height, label, alpha,
                                        active, qfalse, textAlign );
}

qboolean Frontend_DrawNavButton( int x, int y, int width, int height,
                                 const char *label, float alpha,
                                 qboolean active, int textAlign ) {
    vec4_t textColor;
    qboolean hovered;
    qboolean highlighted;
    int textWidth;
    int textHeight;
    int textX;
    int textY;
    int availableWidth;
    float textScale;

    hovered = ( uis.cursorx >= x && uis.cursorx <= x + width &&
                uis.cursory >= y && uis.cursory <= y + height ) ? qtrue : qfalse;
    highlighted = ( active || hovered ) ? qtrue : qfalse;

    /* Navigation stays visually quiet until it is selected. */
    if ( highlighted ) {
        Frontend_ColorWithAlpha( textColor, frontendFocusColor, alpha * 0.55f );
        UI_FillRect( x, y, width, height, textColor );
        Frontend_ColorWithAlpha( textColor, frontendAccentColor, alpha );
        UI_FillRect( x, y + height - 2, width, 2, textColor );
    }

    if ( highlighted ) {
        Frontend_ColorWithAlpha( textColor, frontendAccentColor, alpha );
    } else {
        Frontend_ColorWithAlpha( textColor, frontendMutedColor, alpha );
    }

    availableWidth = width - 2 * UI_FRONTEND_SPACE_MD;
    if ( availableWidth < 1 ) {
        availableWidth = 1;
    }
    textWidth = Frontend_TextVisualWidth( label, UI_SMALLFONT );
    textScale = Frontend_TextDefaultScale( UI_SMALLFONT );
    if ( textWidth > availableWidth ) {
        textScale *= (float)availableWidth / (float)textWidth;
    }
    textHeight = (int)( Frontend_TextBaseHeight( UI_SMALLFONT ) * textScale + 0.5f );
    textX = ( textAlign == UI_CENTER ) ? x + width / 2 : x + UI_FRONTEND_SPACE_MD;
    textY = y + ( height - textHeight ) / 2;
    Frontend_DrawTextScaled( textX, textY, label, textAlign | UI_SMALLFONT,
                             textScale, textColor );
    return hovered;
}

void Frontend_DrawStatusChip( int x, int y, const char *label,
                              const float *statusColor, float alpha ) {
    vec4_t dotColor;
    vec4_t textColor;

    Frontend_ColorWithAlpha( dotColor, statusColor, alpha );
    Frontend_ColorWithAlpha( textColor, frontendAccentColor, alpha );
    UI_FillRect( x, y + 3, UI_FRONTEND_STATUS_DOT, UI_FRONTEND_STATUS_DOT, dotColor );
    Frontend_DrawTextTier( x + UI_FRONTEND_STATUS_DOT + UI_FRONTEND_SPACE_SM,
                           y, label, UI_LEFT | UI_SMALLFONT,
                           FRONTEND_TEXT_TIER_LABEL, textColor );
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
