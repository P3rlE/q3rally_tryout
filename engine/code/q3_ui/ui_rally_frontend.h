/*
===========================================================================
Copyright (C) 2002-2026 Q3Rally Team
===========================================================================
*/

#ifndef UI_RALLY_FRONTEND_H
#define UI_RALLY_FRONTEND_H

#include "ui_rally_theme.h"

typedef enum {
	FRONTEND_TEXT_TIER_MICRO,
	FRONTEND_TEXT_TIER_LABEL,
	FRONTEND_TEXT_TIER_BODY,
	FRONTEND_TEXT_TIER_VALUE,
	FRONTEND_TEXT_TIER_HEADING,
	FRONTEND_TEXT_TIER_DISPLAY
} frontendTextTier_t;

int Frontend_TextWidth( const char *text, int style );
int Frontend_TextVisualWidth( const char *text, int style );
int Frontend_TextHeight( int style );
float Frontend_TextDefaultScale( int style );
qhandle_t Frontend_BackgroundShader( void );
void Frontend_DrawBackground( const float *scrimColor );
void Frontend_DrawText( int x, int y, const char *text, int style,
                        const float *color );
void Frontend_DrawTextScaled( int x, int y, const char *text, int style,
                              float scale, const float *color );
void Frontend_DrawTextTier( int x, int y, const char *text, int style,
                            frontendTextTier_t tier, const float *color );
void Frontend_DrawTextFitted( int x, int y, int maxWidth, const char *text,
                              int style, const float *color );
void Frontend_DrawTextTierFitted( int x, int y, int maxWidth, const char *text,
                                  int style, frontendTextTier_t tier,
                                  const float *color );
void Frontend_DrawPanel( int x, int y, int width, int height,
                         float alpha, int style );
void Frontend_DrawCard( int x, int y, int width, int height,
                        float alpha, qboolean active );
qboolean Frontend_DrawButton( int x, int y, int width, int height,
                              const char *label, float alpha,
                              qboolean active, int textAlign );
qboolean Frontend_DrawButtonFocused( int x, int y, int width, int height,
                                     const char *label, float alpha,
                                     qboolean active, int textAlign );
qboolean Frontend_DrawNavButton( int x, int y, int width, int height,
                                 const char *label, float alpha,
                                 qboolean active, int textAlign );
void Frontend_DrawStatusChip( int x, int y, const char *label,
                              const float *statusColor, float alpha );
void Frontend_DrawSidebar( int x, int y, int width, int height,
                           const char *title, float alpha );
void Frontend_DrawVehicleHero( int x, int y, int width, int height,
                               playerInfo_t *playerInfo, int realtime,
                               float alpha );
void Frontend_DrawProgress( int x, int y, int width, int height,
                            float progress, float alpha );

#endif
