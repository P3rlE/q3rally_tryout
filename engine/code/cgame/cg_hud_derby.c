/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.
===========================================================================
*/

/*
===========================================================================
  cg_hud_derby.c

  Derby: CG_DrawHUD_DerbyHitImpact, the fullscreen colour flash on a
  collision (cg_derbyHitFxEnable). The rest of the Derby HUD (the car and
  its zones, the order) is the v0.9 HUD's, cg_hud_modern.c.
===========================================================================
*/

#include "cg_local.h"
#include "cg_hud_elements.h"


/*
========================
CG_DrawHUD_DerbyHitImpact
Fullscreen coloured overlay that flashes when the player takes a hit.
========================
*/
void CG_DrawHUD_DerbyHitImpact( void ) {
	vec4_t  color;
	float   alpha, frac, overlayScale, damageBoost;
	int     elapsed, duration;

	if ( cgs.gametype != GT_DERBY || !cg_derbyHitFxEnable.integer )
		return;
	if ( !cg.derbyHitFxTime )
		return;

	elapsed  = cg.time - cg.derbyHitFxTime;
	duration = cg_derbyHitOverlayTime.integer;
	if ( duration < 120 ) duration = 120;
	if ( elapsed < 0 || elapsed >= duration ) return;

	frac         = 1.0f - (float)elapsed / (float)duration;
	overlayScale = cg_derbyHitOverlayScale.value;
	if ( overlayScale < 0.0f ) overlayScale = 0.0f;

	damageBoost = (float)cg.derbyHitFxDamage * (1.0f / 40.0f);
	if ( damageBoost > 0.35f ) damageBoost = 0.35f;

	switch ( cg.derbyHitFxLevel ) {
	default:
	case 0: color[0] = 1.0f; color[1] = 1.0f;  color[2] = 1.0f;  alpha = 0.14f; break;
	case 1: color[0] = 1.0f; color[1] = 0.72f; color[2] = 0.15f; alpha = 0.24f; break;
	case 2: color[0] = 1.0f; color[1] = 0.2f;  color[2] = 0.1f;  alpha = 0.34f; break;
	}

	alpha = ( alpha + damageBoost ) * overlayScale;
	if ( alpha > 0.80f ) alpha = 0.80f;
	color[3] = alpha * frac;

	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );
}
