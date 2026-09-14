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

    UI_DrawString( x + ( textAlign == UI_CENTER ? width / 2 : UI_FRONTEND_SPACE_MD ),
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
    UI_DrawString( x + UI_FRONTEND_STATUS_DOT + UI_FRONTEND_SPACE_SM, y,
                   label, UI_LEFT | UI_SMALLFONT, textColor );
}

void Frontend_DrawSidebar( int x, int y, int width, int height,
                           const char *title, float alpha ) {
    vec4_t titleColor;

    Frontend_DrawPanel( x, y, width, height, alpha, UI_FRONTEND_STYLE_SURFACE );
    Frontend_ColorWithAlpha( titleColor, frontendMutedColor, alpha );
    UI_DrawString( x + UI_FRONTEND_SPACE_LG, y + 44, title,
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
