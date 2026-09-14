/*
===========================================================================
Copyright (C) 2002-2026 Q3Rally Team
===========================================================================
*/

#ifndef UI_RALLY_FRONTEND_H
#define UI_RALLY_FRONTEND_H

#include "ui_rally_theme.h"

void Frontend_DrawPanel( int x, int y, int width, int height,
                         float alpha, int style );
void Frontend_DrawCard( int x, int y, int width, int height,
                        float alpha, qboolean active );
qboolean Frontend_DrawButton( int x, int y, int width, int height,
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
