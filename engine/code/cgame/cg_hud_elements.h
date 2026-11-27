/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifndef __CG_HUD_ELEMENTS_H__
#define __CG_HUD_ELEMENTS_H__

/*
===========================================================================
  HUD ELEMENT TOGGLE CVARS
  All cvars default to 1 (enabled). Set to 0 to hide an element.
  Accessible via the HUD Options menu (bound to a key) or console.
===========================================================================
*/

/* --- Racing HUD --- */
extern vmCvar_t  cg_hudShowTimes;          /* Lap/total/best times panel      */
extern vmCvar_t  cg_hudShowLaps;           /* Lap counter                      */
extern vmCvar_t  cg_hudShowPosition;       /* Current race position            */
extern vmCvar_t  cg_hudShowOpponentList;   /* Racing/elimination order list    */

/* --- Vehicle HUD --- */
extern vmCvar_t  cg_hudShowSlipstream;     /* Slipstream gauge above the strip */

/* --- Derby HUD --- */
extern vmCvar_t  cg_hudShowDerbyList;      /* Derby order                      */

/*
===========================================================================
  CVAR REGISTRATION - call once from CG_RegisterCvars()
===========================================================================
*/
void CG_HUD_RegisterCvars( void );

/*
===========================================================================
  HUD DRAW ENTRY POINTS
  Declared here so cg_hud_core.c can call across translation units.
===========================================================================
*/

/* cg_hud_racing.c */
void  CG_DrawArrowToCheckpoint( void );

/* cg_hud_derby.c */
void  CG_DrawHUD_DerbyHitImpact( void );

/* cg_hud_vehicle.c */
void  CG_AddObjectsToScene( int renderLevel );
void  CG_AddKOTHHillIndicatorToScene( qboolean minimapPass );

/* cg_hud_core.c */
void     CG_DrawHUDOptionsMenu( void );
qboolean CG_HUDOptions_MouseEvent( int cx, int cy, qboolean clicked );
void     CG_HUDOptions_KeyEvent( int key );
/* cg_hud_panel.c */
void     CG_HUDPanel_Draw( void );
qboolean CG_HUDPanel_Key( int key );
void     CG_HUDPanel_Reset( void );
qboolean CG_HUDOptionsIsOpen( void );
void     CG_HUD_RegisterCvars( void );
qboolean CG_DrawHUD( void );

#endif /* __CG_HUD_ELEMENTS_H__ */
