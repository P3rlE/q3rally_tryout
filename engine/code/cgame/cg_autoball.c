/*
===========================================================================
Copyright (C) 2002-2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*
===========================================================================
Autoball client side: match state, score bug, goal banner, ball indicator
and ball camera.

The indicator and the ball camera also work outside GT_AUTOBALL whenever a
test ball (ball_spawn) is in the snapshot, so the physics can be tried on
any map.

Cvars:  cg_autoballCam        0/1, the "ballcam" command toggles it
        cg_autoballIndicator  0/1, arrow/bracket that shows where the ball is
        cg_autoballShake      camera shake strength on goal blasts (0 = off)
        cg_autoballTrail      0/1, glowing trail behind a fast ball, fire above 150 km/h
===========================================================================
*/

#include "cg_local.h"

#define AB_SET4(v,a,b,c,d)	((v)[0]=(a),(v)[1]=(b),(v)[2]=(c),(v)[3]=(d))
#define AUTOBALL_BALL_RADIUS		75.0f
#define AUTOBALL_CAM_BLEND_MSEC		200.0f
#define AUTOBALL_CAM_MIN_DIST		120.0f	/* closer than this the yaw would spin */

/*
================
CG_Autoball_FindBall

The match ball from CS_AUTOBALLSTATUS, otherwise the first test ball in the
snapshot. Balls are SVF_BROADCAST, so they are always in the snapshot.
================
*/
centity_t *CG_Autoball_FindBall( void ) {
	int i;

	if ( !cg.snap )
		return NULL;
	if ( cgs.gametype == GT_AUTOBALL && cgs.autoballBallNum >= MAX_CLIENTS &&
		cgs.autoballBallNum < MAX_GENTITIES ) {
		centity_t *cent = &cg_entities[cgs.autoballBallNum];
		if ( cent->currentValid && cent->currentState.eType == ET_SCRIPTED )
			return cent;
	}
	for ( i = 0; i < cg.snap->numEntities; i++ ) {
		entityState_t *es = &cg.snap->entities[i];
		if ( es->eType == ET_SCRIPTED && ( es->generic1 & SCRIPTED_GENERIC1_NO_PREDICT ) )
			return &cg_entities[es->number];
	}
	return NULL;
}

/*
================
CG_ParseAutoballStatus

"state kickoffEnd ballEntity goalTeam scorerClient goalSpeedKmh"
================
*/
void CG_ParseAutoballStatus( void ) {
	char buffer[MAX_STRING_CHARS];
	char *cursor;
	int values[6];
	int i, oldState;

	Q_strncpyz( buffer, CG_ConfigString( CS_AUTOBALLSTATUS ), sizeof( buffer ) );
	cursor = buffer;
	for ( i = 0; i < 6; i++ ) {
		char *token = COM_Parse( &cursor );
		if ( !token[0] )
			return;
		values[i] = atoi( token );
	}

	oldState = cgs.autoballState;
	cgs.autoballState = values[0];
	cgs.autoballKickoffEnd = values[1];
	cgs.autoballBallNum = values[2];
	cgs.autoballGoalTeam = values[3];
	cgs.autoballScorer = values[4];
	cgs.autoballGoalSpeed = values[5];
	if ( cgs.autoballState == AUTOBALL_STATE_GOAL && oldState != AUTOBALL_STATE_GOAL )
		cgs.autoballGoalTime = cg.time;
}

/* Kick-off: hold the predicted car exactly like the server does. */
void CG_Autoball_FreezeCommand( usercmd_t *cmd ) {
	if ( cgs.gametype != GT_AUTOBALL || cgs.autoballState != AUTOBALL_STATE_KICKOFF )
		return;
	if ( cmd->serverTime >= cgs.autoballKickoffEnd )
		return;
	cmd->buttons = BUTTON_HANDBRAKE;
	cmd->forwardmove = 0;
	cmd->upmove = 0;
}

void CG_Autoball_ToggleCam_f( void ) {
	trap_Cvar_Set( "cg_autoballCam", cg_autoballCam.integer ? "0" : "1" );
	CG_Printf( "Ball camera %s\n", cg_autoballCam.integer ? "off" : "on" );
}

/*
================
CG_Autoball_ApplyBallCam

Called with cg.refdef.vieworg at the car and cg.refdefViewAngles holding the
normal chase angles, right before the third-person offset. Turns the chase
camera towards the ball, blending in and out over AUTOBALL_CAM_BLEND_MSEC.
================
*/
void CG_Autoball_ApplyBallCam( void ) {
	static float blend;
	static float ballYaw, ballPitch;
	centity_t *ball;
	float target, step;

	ball = cg_autoballCam.integer ? CG_Autoball_FindBall() : NULL;
	target = ball ? 1.0f : 0.0f;
	step = cg.frametime / AUTOBALL_CAM_BLEND_MSEC;
	if ( blend < target ) {
		blend += step;
		if ( blend > target )
			blend = target;
	} else if ( blend > target ) {
		blend -= step;
		if ( blend < target )
			blend = target;
	}
	if ( blend <= 0.0f )
		return;

	if ( ball ) {
		vec3_t delta, angles;
		float horizontal;

		VectorSubtract( ball->lerpOrigin, cg.refdef.vieworg, delta );
		horizontal = sqrt( delta[0] * delta[0] + delta[1] * delta[1] );
		if ( horizontal > AUTOBALL_CAM_MIN_DIST ) {
			vectoangles( delta, angles );
			ballYaw = angles[YAW];
			ballPitch = AngleNormalize180( angles[PITCH] );
			if ( ballPitch < -30.0f )
				ballPitch = -30.0f;
			if ( ballPitch > 20.0f )
				ballPitch = 20.0f;
		}
	}
	cg.refdefViewAngles[YAW] = LerpAngle( cg.refdefViewAngles[YAW], ballYaw, blend );
	cg.refdefViewAngles[PITCH] = LerpAngle( cg.refdefViewAngles[PITCH], ballPitch, blend );
}

/*
================
HUD helpers
================
*/
static void CG_Autoball_TeamColor( int team, float alpha, vec4_t out ) {
	if ( team == TEAM_BLUE ) {
		out[0] = 0.16f; out[1] = 0.38f; out[2] = 0.92f;
	} else {
		out[0] = 0.88f; out[1] = 0.18f; out[2] = 0.16f;
	}
	out[3] = alpha;
}

static const char *CG_Autoball_SpeedText( int kmh ) {
	if ( cg_metricUnits.integer )
		return va( "%i km/h", kmh );
	return va( "%i mph", (int)( kmh * 0.621371f + 0.5f ) );
}

static void CG_Autoball_DrawScoreBug( void ) {
	vec4_t red, blue, center, white, accent;
	int msec, seconds;
	qboolean overtime = qfalse;
	char clock[16];
	const float y = 6.0f, h = 30.0f, teamW = 56.0f, clockW = 84.0f;
	float x = 320.0f - ( teamW + clockW / 2.0f );

	CG_Autoball_TeamColor( TEAM_RED, 0.85f, red );
	CG_Autoball_TeamColor( TEAM_BLUE, 0.85f, blue );
	AB_SET4( center, 0.02f, 0.03f, 0.04f, 0.72f );
	AB_SET4( white, 1.0f, 1.0f, 1.0f, 1.0f );
	AB_SET4( accent, 1.0f, 0.78f, 0.2f, 1.0f );

	msec = cg.time - cgs.levelStartTime;
	if ( cgs.timelimit > 0 ) {
		msec = cgs.timelimit * 60000 - msec;
		if ( msec < 0 ) {
			overtime = qtrue;
			msec = -msec;
		}
	}
	if ( msec < 0 )
		msec = 0;
	seconds = msec / 1000;
	/* own buffer: va() only rotates two buffers, the score va() calls below
	   would overwrite the clock text */
	Com_sprintf( clock, sizeof( clock ), "%s%i:%02i", overtime ? "+" : "", seconds / 60, seconds % 60 );

	CG_FillRect( x, y, teamW, h, red );
	CG_FillRect( x + teamW, y, clockW, h, center );
	CG_FillRect( x + teamW + clockW, y, teamW, h, blue );
	CG_DrawIngameString( (int)( x + teamW / 2 ), (int)( y + 4 ), va( "%i", cgs.scores1 ),
		UI_CENTER | UI_DROPSHADOW, 1.0f, white );
	CG_DrawIngameString( (int)( x + teamW + clockW + teamW / 2 ), (int)( y + 4 ), va( "%i", cgs.scores2 ),
		UI_CENTER | UI_DROPSHADOW, 1.0f, white );
	CG_DrawIngameString( 320, (int)( y + 8 ), clock, UI_CENTER | UI_SMALLFONT, 1.0f,
		overtime ? accent : white );
	if ( overtime ) {
		CG_DrawIngameString( 320, (int)( y + h + 3 ), "OVERTIME", UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW,
			0.75f, accent );
	} else if ( cgs.autoballState == AUTOBALL_STATE_KICKOFF ) {
		CG_DrawIngameString( 320, (int)( y + h + 3 ), "KICK-OFF", UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW,
			0.75f, white );
	}
}

static void CG_Autoball_DrawGoalBanner( void ) {
	vec4_t color, white;
	int elapsed;
	float alpha, scale;
	const char *who;

	if ( cgs.autoballState != AUTOBALL_STATE_GOAL )
		return;
	elapsed = cg.time - cgs.autoballGoalTime;
	if ( elapsed < 0 )
		elapsed = 0;
	alpha = elapsed < 3000 ? 1.0f : 1.0f - ( elapsed - 3000 ) / 1000.0f;
	if ( alpha <= 0.0f )
		return;
	/* a short punch-in, then steady */
	scale = elapsed < 250 ? 1.0f + ( 250 - elapsed ) / 250.0f * 0.6f : 1.0f;

	CG_Autoball_TeamColor( cgs.autoballGoalTeam, alpha, color );
	AB_SET4( white, 1.0f, 1.0f, 1.0f, alpha );
	CG_DrawIngameString( 320, 140, cgs.autoballGoalTeam == TEAM_BLUE ? "BLUE SCORES!" : "RED SCORES!",
		UI_CENTER | UI_DROPSHADOW, 1.6f * scale, color );

	if ( cgs.autoballScorer >= 0 && cgs.autoballScorer < MAX_CLIENTS &&
		cgs.clientinfo[cgs.autoballScorer].infoValid ) {
		who = cgs.clientinfo[cgs.autoballScorer].name;
	} else {
		who = "Own goal";
	}
	CG_DrawIngameString( 320, 184, va( "%s^7  -  %s", who, CG_Autoball_SpeedText( cgs.autoballGoalSpeed ) ),
		UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, 1.0f, white );
}

/*
================
CG_Autoball_DrawIndicator

On screen: four corner brackets around the ball. Off screen: a marker on the
screen edge in the ball's direction, with the distance.
================
*/
/* ball radius from the server (g_autoballBallScale), 75 for old servers */
static float CG_Autoball_Radius( const centity_t *cent ) {
	return cent->currentState.time2 > 0 ? (float)cent->currentState.time2 : AUTOBALL_BALL_RADIUS;
}

static void CG_Autoball_DrawIndicator( void ) {
	centity_t *ball;
	vec3_t delta;
	vec4_t color, shadow;
	float forward, left, up, focalX, focalY, sx, sy, dist;

	ball = CG_Autoball_FindBall();
	if ( !ball )
		return;

	VectorSubtract( ball->lerpOrigin, cg.refdef.vieworg, delta );
	forward = DotProduct( delta, cg.refdef.viewaxis[0] );
	left = DotProduct( delta, cg.refdef.viewaxis[1] );
	up = DotProduct( delta, cg.refdef.viewaxis[2] );
	dist = VectorLength( delta ) / CP_M_2_QU;
	focalX = 320.0f / tan( cg.refdef.fov_x * M_PI / 360.0f );
	focalY = 240.0f / tan( cg.refdef.fov_y * M_PI / 360.0f );

	AB_SET4( color, 1.0f, 0.62f, 0.12f, 0.9f );
	AB_SET4( shadow, 0.0f, 0.0f, 0.0f, 0.6f );
	CG_SetScreenPlacement( PLACE_STRETCH, PLACE_STRETCH );

	if ( forward > 1.0f ) {
		sx = 320.0f - left / forward * focalX;
		sy = 240.0f - up / forward * focalY;
		if ( sx > 24.0f && sx < 616.0f && sy > 24.0f && sy < 456.0f ) {
			float r = CG_Autoball_Radius( ball ) / forward * focalX * 1.15f;
			float len;
			if ( r < 8.0f )
				r = 8.0f;
			if ( r > 120.0f )
				r = 120.0f;
			len = r * 0.4f;
			/* four corner brackets */
			CG_FillRect( sx - r, sy - r, len, 2, color );
			CG_FillRect( sx - r, sy - r, 2, len, color );
			CG_FillRect( sx + r - len, sy - r, len, 2, color );
			CG_FillRect( sx + r - 2, sy - r, 2, len, color );
			CG_FillRect( sx - r, sy + r - 2, len, 2, color );
			CG_FillRect( sx - r, sy + r - len, 2, len, color );
			CG_FillRect( sx + r - len, sy + r - 2, len, 2, color );
			CG_FillRect( sx + r - 2, sy + r - len, 2, len, color );
			if ( dist > 25.0f ) {
				CG_DrawIngameString( (int)sx, (int)( sy + r + 4 ), va( "%im", (int)dist ),
					UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, 0.7f, color );
			}
			CG_PopScreenPlacement();
			return;
		}
	}

	{
		/* screen-space direction: right = -left, down = -up */
		float dx = -left, dy = -up, len, t;
		const char *arrow;

		if ( forward <= 0.0f && fabs( dx ) < 1.0f && fabs( dy ) < 1.0f )
			dy = 1.0f;
		len = sqrt( dx * dx + dy * dy );
		if ( len < 0.001f ) {
			dx = 0.0f;
			dy = 1.0f;
			len = 1.0f;
		}
		dx /= len;
		dy /= len;
		t = 1e9f;
		if ( fabs( dx ) > 0.001f )
			t = 280.0f / fabs( dx );
		if ( fabs( dy ) > 0.001f && 196.0f / fabs( dy ) < t )
			t = 196.0f / fabs( dy );
		sx = 320.0f + dx * t;
		sy = 240.0f + dy * t;

		if ( fabs( dx ) > fabs( dy ) )
			arrow = dx > 0.0f ? ">" : "<";
		else
			arrow = dy > 0.0f ? "v" : "^";

		CG_FillRect( sx - 15, sy - 15, 30, 30, shadow );
		CG_DrawRect( sx - 15, sy - 15, 30, 30, 2, color );
		CG_DrawIngameString( (int)sx, (int)( sy - 9 ), arrow, UI_CENTER, 0.75f, color );
		CG_DrawIngameString( (int)sx, (int)( sy + ( dy > 0.5f ? -32 : 18 ) ), va( "%im", (int)dist ),
			UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, 0.7f, color );
	}
	CG_PopScreenPlacement();
}

/*
================
CG_Autoball_BallShadow

Dark disc straight below the ball, smaller and fainter the higher it flies,
so players can judge where an airborne ball will land.
================
*/
#define AUTOBALL_SHADOW_RANGE	1600.0f

void CG_Autoball_BallShadow( centity_t *cent ) {
	trace_t trace;
	vec3_t end;
	float height, frac;

	if ( !cgs.media.shadowMarkShader )
		return;
	VectorCopy( cent->lerpOrigin, end );
	end[2] -= AUTOBALL_SHADOW_RANGE;
	CG_Trace( &trace, cent->lerpOrigin, NULL, NULL, end, cent->currentState.number, MASK_SOLID );
	if ( trace.fraction >= 1.0f || trace.startsolid )
		return;
	height = trace.fraction * AUTOBALL_SHADOW_RANGE;
	frac = 1.0f - height / AUTOBALL_SHADOW_RANGE;
	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, 0,
		1.0f, 1.0f, 1.0f, 0.35f + 0.5f * frac, qfalse,
		CG_Autoball_Radius( cent ) * ( 0.6f + 0.5f * frac ), qtrue );
}

/* seam glow colour of the ball: last touching team, dim white for none */
void CG_Autoball_BallColor( const entityState_t *s, byte *rgba ) {
	int team = ( s->generic1 & SCRIPTED_GENERIC1_TEAM_MASK ) >> SCRIPTED_GENERIC1_TEAM_SHIFT;

	if ( team == TEAM_RED ) {
		rgba[0] = 255; rgba[1] = 60; rgba[2] = 40;
	} else if ( team == TEAM_BLUE ) {
		rgba[0] = 50; rgba[1] = 110; rgba[2] = 255;
	} else {
		rgba[0] = rgba[1] = rgba[2] = 80;
	}
	rgba[3] = 255;
}

void CG_Autoball_Draw2D( void ) {
	if ( !cg.snap || cg.showScores )
		return;
	if ( cg.snap->ps.pm_type == PM_INTERMISSION )
		return;
	if ( cg_autoballIndicator.integer )
		CG_Autoball_DrawIndicator();
	if ( cgs.gametype != GT_AUTOBALL )
		return;
	CG_SetScreenPlacement( PLACE_CENTER, PLACE_TOP );
	CG_Autoball_DrawScoreBug();
	CG_PopScreenPlacement();
	CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );
	CG_Autoball_DrawGoalBanner();
	CG_PopScreenPlacement();
}


/*
===========================================================================
Goal blast: a large explosion in the scoring team's colour plus a camera
shake that fades with distance. Triggered by EV_EXPLOSION carrying
EXPLOSION_PARM_AUTOBALL_GOAL.
===========================================================================
*/

#define AUTOBALL_SHAKE_DURATION   1400
#define AUTOBALL_SHAKE_NEAR       1200.0f
#define AUTOBALL_SHAKE_FAR        6000.0f
#define AUTOBALL_SHAKE_MIN        0.30f
#define AUTOBALL_SHAKE_ANGLE      4.0f    /* degrees at full strength */
#define AUTOBALL_SHAKE_OFFSET     8.0f    /* units at full strength */

static int   ab_shakeStart;
static float ab_shakeAmp;
static float ab_shakePhase[4];

static void CG_Autoball_StartShake( const vec3_t origin ) {
	vec3_t delta;
	float dist, amp;
	int i;

	if ( cg_autoballShake.value <= 0.0f )
		return;

	VectorSubtract( origin, cg.refdef.vieworg, delta );
	dist = VectorLength( delta );
	if ( dist <= AUTOBALL_SHAKE_NEAR ) {
		amp = 1.0f;
	} else if ( dist >= AUTOBALL_SHAKE_FAR ) {
		amp = AUTOBALL_SHAKE_MIN;
	} else {
		amp = 1.0f - ( 1.0f - AUTOBALL_SHAKE_MIN ) *
			( dist - AUTOBALL_SHAKE_NEAR ) / ( AUTOBALL_SHAKE_FAR - AUTOBALL_SHAKE_NEAR );
	}
	amp *= cg_autoballShake.value;

	/* a running shake is only replaced by a stronger one */
	if ( ab_shakeStart && cg.time - ab_shakeStart < AUTOBALL_SHAKE_DURATION ) {
		float t = (float)( cg.time - ab_shakeStart ) / AUTOBALL_SHAKE_DURATION;
		float current = ab_shakeAmp * ( 1.0f - t ) * ( 1.0f - t );
		if ( current > amp )
			return;
	}
	ab_shakeStart = cg.time;
	ab_shakeAmp = amp;
	for ( i = 0; i < 4; i++ )
		ab_shakePhase[i] = random() * 2.0f * M_PI;
}

void CG_Autoball_ApplyShake( void ) {
	float t, env, sec;
	vec3_t axis[3];

	if ( !ab_shakeStart )
		return;
	if ( cg_autoballShake.value <= 0.0f || cg.time < ab_shakeStart
		|| cg.time - ab_shakeStart >= AUTOBALL_SHAKE_DURATION ) {
		ab_shakeStart = 0;
		return;
	}

	t = (float)( cg.time - ab_shakeStart ) / AUTOBALL_SHAKE_DURATION;
	env = ab_shakeAmp * ( 1.0f - t ) * ( 1.0f - t );
	sec = cg.time * 0.001f;

	/* layered sines instead of per-frame noise: rough but not flickering */
	cg.refdefViewAngles[PITCH] += env * AUTOBALL_SHAKE_ANGLE *
		( 0.7f * sin( sec * 2.0f * M_PI * 11.0f + ab_shakePhase[0] ) + 0.3f * sin( sec * 2.0f * M_PI * 23.0f ) );
	cg.refdefViewAngles[YAW] += env * AUTOBALL_SHAKE_ANGLE * 0.6f *
		sin( sec * 2.0f * M_PI * 8.0f + ab_shakePhase[1] );
	cg.refdefViewAngles[ROLL] += env * AUTOBALL_SHAKE_ANGLE * 0.8f *
		sin( sec * 2.0f * M_PI * 6.0f + ab_shakePhase[2] );

	AnglesToAxis( cg.refdefViewAngles, axis );
	VectorMA( cg.refdef.vieworg, env * AUTOBALL_SHAKE_OFFSET *
		sin( sec * 2.0f * M_PI * 14.0f + ab_shakePhase[3] ), axis[2], cg.refdef.vieworg );
	VectorMA( cg.refdef.vieworg, env * AUTOBALL_SHAKE_OFFSET * 0.5f *
		sin( sec * 2.0f * M_PI * 9.0f + ab_shakePhase[0] ), axis[1], cg.refdef.vieworg );
}

/*
===========================================================================
Ball trail and goal lights
===========================================================================
*/

#define AB_TRAIL_MIN_KMH        100.0f   /* trail starts here */
#define AB_TRAIL_FIRE_KMH       150.0f   /* fire on top of the trail */
#define AB_TRAIL_SPACING        22.0f    /* units between two trail puffs */
#define AB_TRAIL_MAX_STEPS      10

#define AB_GOAL_LIGHT_TIME      3000
#define AB_GOAL_LIGHT_SIDE      380.0f   /* side lights, from the goal centre */

static vec3_t ab_trailLast[MAX_GENTITIES];
static int    ab_trailTime[MAX_GENTITIES];
static qhandle_t ab_trailShader;

static int   ab_goalLightStart;
static vec3_t ab_goalLightOrigin;
static vec3_t ab_goalLightSide;
static vec3_t ab_goalLightColor;

/* full-strength colour of the last touching team, white for nobody */
static void CG_Autoball_TrailColor( const entityState_t *s, vec3_t out ) {
	int team = ( s->generic1 & SCRIPTED_GENERIC1_TEAM_MASK ) >> SCRIPTED_GENERIC1_TEAM_SHIFT;

	if ( team == TEAM_RED ) {
		VectorSet( out, 1.0f, 0.25f, 0.15f );
	} else if ( team == TEAM_BLUE ) {
		VectorSet( out, 0.2f, 0.45f, 1.0f );
	} else {
		VectorSet( out, 0.85f, 0.85f, 0.85f );
	}
}

void CG_Autoball_BallTrail( centity_t *cent ) {
	int num = cent->currentState.number;
	float kmh, dist, f, radius, frac;
	vec3_t color, pos, delta, vel;
	int i, steps;

	kmh = VectorLength( cent->currentState.pos.trDelta ) / CP_M_2_QU * 3.6f;
	VectorSubtract( cent->lerpOrigin, ab_trailLast[num], delta );
	dist = VectorLength( delta );

	/* off, too slow, or the ball jumped (kick-off reset, first frame) */
	if ( !cg_autoballTrail.integer || kmh < AB_TRAIL_MIN_KMH || cg.time - ab_trailTime[num] > 250 ||
		dist > 600.0f ) {
		VectorCopy( cent->lerpOrigin, ab_trailLast[num] );
		ab_trailTime[num] = cg.time;
		return;
	}
	if ( dist < AB_TRAIL_SPACING )
		return;
	if ( !ab_trailShader )
		ab_trailShader = trap_R_RegisterShader( "autoballTrail" );

	CG_Autoball_TrailColor( &cent->currentState, color );
	f = ( kmh - AB_TRAIL_MIN_KMH ) / ( AB_TRAIL_FIRE_KMH - AB_TRAIL_MIN_KMH );
	if ( f > 1.0f )
		f = 1.0f;
	radius = cent->currentState.time2 > 0 ? (float)cent->currentState.time2 : AUTOBALL_BALL_RADIUS;

	steps = (int)( dist / AB_TRAIL_SPACING );
	if ( steps > AB_TRAIL_MAX_STEPS )
		steps = AB_TRAIL_MAX_STEPS;
	VectorClear( vel );
	for ( i = 1; i <= steps; i++ ) {
		frac = (float)i / steps;
		VectorMA( ab_trailLast[num], frac, delta, pos );

		/* glow in team colour, longer and brighter the faster the ball */
		CG_SmokePuff( pos, vel, radius * ( 0.55f + 0.25f * f ),
			color[0], color[1], color[2], 0.3f + 0.35f * f,
			220.0f + 260.0f * f, cg.time, 0, LEF_PUFF_DONT_SCALE, ab_trailShader );

		if ( kmh >= AB_TRAIL_FIRE_KMH ) {
			/* fire core and a little smoke */
			CG_SmokePuff( pos, vel, radius * 0.75f, 1.0f, 0.55f, 0.12f, 0.75f,
				320.0f, cg.time, 0, LEF_PUFF_DONT_SCALE, ab_trailShader );
			if ( ( i & 3 ) == 0 ) {
				vec3_t up;
				VectorSet( up, crandom() * 20.0f, crandom() * 20.0f, 40.0f );
				CG_SmokePuff( pos, up, radius * 0.9f, 0.25f, 0.25f, 0.25f, 0.4f,
					750.0f, cg.time, 0, 0, cgs.media.smokePuffShader );
			}
		}
	}
	if ( kmh >= AB_TRAIL_FIRE_KMH )
		trap_R_AddLightToScene( cent->lerpOrigin, 180.0f + 60.0f * random(), 1.0f, 0.6f, 0.2f );

	VectorCopy( cent->lerpOrigin, ab_trailLast[num] );
	ab_trailTime[num] = cg.time;
}

/* the ball nearest to point (the one that just scored) */
static centity_t *CG_Autoball_BallNear( const vec3_t point ) {
	centity_t *best = NULL;
	float d, bestDist = 1.0e18f;
	int i;

	if ( !cg.snap )
		return NULL;
	for ( i = 0; i < cg.snap->numEntities; i++ ) {
		entityState_t *es = &cg.snap->entities[i];
		if ( es->eType != ET_SCRIPTED || !( es->generic1 & SCRIPTED_GENERIC1_NO_PREDICT ) )
			continue;
		d = DistanceSquared( cg_entities[es->number].lerpOrigin, point );
		if ( d < bestDist ) {
			bestDist = d;
			best = &cg_entities[es->number];
		}
	}
	return best;
}

static void CG_Autoball_StartGoalLights( const vec3_t origin, const vec4_t color ) {
	centity_t *ball = CG_Autoball_BallNear( origin );
	vec3_t into;

	ab_goalLightStart = cg.time;
	VectorCopy( origin, ab_goalLightOrigin );
	ab_goalLightOrigin[2] += 120.0f;
	VectorCopy( color, ab_goalLightColor );

	/* the ball flies into the goal: the goal line is across its path */
	VectorClear( ab_goalLightSide );
	if ( ball ) {
		VectorCopy( ball->currentState.pos.trDelta, into );
		into[2] = 0.0f;
		if ( VectorNormalize( into ) > 0.1f )
			VectorSet( ab_goalLightSide, -into[1], into[0], 0.0f );
	}
}

/* strobing team-coloured lights at the goal, called every frame */
void CG_Autoball_AddSceneEffects( void ) {
	int elapsed;
	float env, strobe, intensity;
	vec3_t pos;
	qboolean flip;

	if ( !ab_goalLightStart )
		return;
	elapsed = cg.time - ab_goalLightStart;
	if ( elapsed < 0 || elapsed > AB_GOAL_LIGHT_TIME ) {
		ab_goalLightStart = 0;
		return;
	}
	env = 1.0f - (float)elapsed / AB_GOAL_LIGHT_TIME;
	env = sqrt( env );
	flip = ( ( elapsed / 140 ) & 1 ) ? qtrue : qfalse;
	strobe = flip ? 1.0f : 0.35f;

	/* centre: team colour, pulsing */
	intensity = ( 380.0f + 220.0f * strobe ) * env;
	trap_R_AddLightToScene( ab_goalLightOrigin, intensity,
		ab_goalLightColor[0], ab_goalLightColor[1], ab_goalLightColor[2] );

	/* posts: alternate left/right, white flashes between the colour */
	if ( VectorLengthSquared( ab_goalLightSide ) > 0.0f ) {
		VectorMA( ab_goalLightOrigin, AB_GOAL_LIGHT_SIDE, ab_goalLightSide, pos );
		if ( flip )
			trap_R_AddLightToScene( pos, 320.0f * env, ab_goalLightColor[0], ab_goalLightColor[1], ab_goalLightColor[2] );
		else
			trap_R_AddLightToScene( pos, 260.0f * env, 1.0f, 1.0f, 1.0f );
		VectorMA( ab_goalLightOrigin, -AB_GOAL_LIGHT_SIDE, ab_goalLightSide, pos );
		if ( !flip )
			trap_R_AddLightToScene( pos, 320.0f * env, ab_goalLightColor[0], ab_goalLightColor[1], ab_goalLightColor[2] );
		else
			trap_R_AddLightToScene( pos, 260.0f * env, 1.0f, 1.0f, 1.0f );
	}
}

void CG_Autoball_GoalExplosion( vec3_t origin, int team ) {
	localEntity_t *ex;
	vec3_t up, pos, vel;
	vec4_t col;
	float ang;
	int i;

	VectorSet( up, 0, 0, 1 );
	CG_Autoball_TeamColor( team, 1.0f, col );
	CG_Autoball_StartGoalLights( origin, col );

	/* sound: the crowd comes from the server, add the bang here */
	trap_S_StartLocalSound( cgs.media.sfx_rockexp, CHAN_LOCAL_SOUND );

	/* fireball: a cluster of large sprite explosions */
	for ( i = 0; i < 5; i++ ) {
		VectorCopy( origin, pos );
		if ( i > 0 ) {
			pos[0] += crandom() * 90.0f;
			pos[1] += crandom() * 90.0f;
			pos[2] += random() * 110.0f;
		}
		ex = CG_MakeExplosion( pos, up, cgs.media.dishFlashModel,
			cgs.media.rocketExplosionShader, 900 + i * 150, qtrue );
		ex->radius = ( i == 0 ) ? 340.0f : 170.0f + random() * 90.0f;
		if ( i == 0 ) {
			ex->light = 900;
			ex->lightColor[0] = 0.6f + 0.4f * col[0];
			ex->lightColor[1] = 0.45f + 0.35f * col[1];
			ex->lightColor[2] = 0.2f + 0.6f * col[2];
		}
	}

	/* rising plume */
	if ( cg_oldRocket.integer == 0 ) {
		VectorSet( vel, 0, 0, 60 );
		CG_ParticleExplosion( "explode1", origin, vel, 1800, 90, 240 );
		for ( i = 0; i < 3; i++ ) {
			VectorCopy( origin, pos );
			pos[0] += crandom() * 70.0f;
			pos[1] += crandom() * 70.0f;
			VectorSet( vel, crandom() * 40.0f, crandom() * 40.0f, 90.0f );
			CG_ParticleExplosion( "explode1", pos, vel, 1400, 60, 170 );
		}
	}

	/* ground shock ring */
	for ( i = 0; i < 28; i++ ) {
		ang = i * ( 2.0f * M_PI / 28.0f );
		VectorSet( vel, cos( ang ) * 950.0f, sin( ang ) * 950.0f, 40.0f );
		VectorCopy( origin, pos );
		CG_SmokePuff( pos, vel, 70.0f, 0.85f, 0.85f, 0.85f, 0.6f,
			850.0f, cg.time, 0, 0, cgs.media.smokePuffShader );
	}

	/* lingering smoke column */
	for ( i = 0; i < 10; i++ ) {
		VectorCopy( origin, pos );
		pos[0] += crandom() * 60.0f;
		pos[1] += crandom() * 60.0f;
		VectorSet( vel, crandom() * 25.0f, crandom() * 25.0f, 60.0f + random() * 60.0f );
		CG_SmokePuff( pos, vel, 90.0f + random() * 60.0f, 0.35f, 0.35f, 0.35f, 0.55f,
			2600.0f + random() * 800.0f, cg.time, 0, 0, cgs.media.smokePuffShader );
	}

	/* confetti sparks in the scoring team's colour, plus white ones */
	CG_Particles( origin, 90, 700, 1600, 7, PT_GRAVITY,
		(byte)( col[0] * 255 ), (byte)( col[1] * 255 ), (byte)( col[2] * 255 ) );
	CG_Particles( origin, 40, 550, 1200, 5, PT_GRAVITY, 255, 230, 160 );

	CG_Autoball_StartShake( origin );
}
