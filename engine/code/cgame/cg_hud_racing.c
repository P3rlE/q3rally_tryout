/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2026 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.
===========================================================================
*/

/*
===========================================================================
  cg_hud_racing.c

  All racing-specific HUD elements:
    - Checkpoint arrow / wrong-way warning
    - LCS elimination timeline
===========================================================================
*/

#include "cg_local.h"
#include "cg_hud_elements.h"

/* LCS timeline layout */
#define HUD_RIGHT_EDGE          636.0f
#define HUD_TEXT_INSET            6.0f
#define HUD_ROW_HEIGHT          ((float)TINYCHAR_HEIGHT + 4.0f)

/* Drag Race telemetry. The launch zone is a broad recommendation measured on
 * the default dry strip; it never changes torque, shifting or race rules. */
#define DRAG_HUD_LAUNCH_RPM_MIN 4000
#define DRAG_HUD_LAUNCH_RPM_MAX 5000
#define DRAG_HUD_SHIFT_RPM      5900	/* shift light: best upshift is around 6000 */
#define DRAG_HUD_TOP           386
#define DRAG_HUD_TACH_X        180.0f
#define DRAG_HUD_TACH_WIDTH    270.0f

typedef struct {
	int rpm;
	int reactionMs;
	int elapsedMs;
	int totalMs;
	int launchBand;
	int engineStress;	/* ms in the red, 0..DRAG_ENGINE_STRESS_LIMIT_MS */
	qboolean engineBlown;
	qboolean shiftLight;
	qboolean launchGuide;
	const char *statusText;
	const char *hintText;
} cgDragHudState_t;

static float CG_DragHudRpmFraction( int rpm ) {
	if ( rpm <= 0 ) return 0.0f;
	if ( rpm >= CP_RPM_MAX ) return 1.0f;
	return rpm / (float)CP_RPM_MAX;
}

static int CG_DragHudLaunchBand( int rpm ) {
	if ( rpm < DRAG_HUD_LAUNCH_RPM_MIN ) return -1;
	if ( rpm > DRAG_HUD_LAUNCH_RPM_MAX ) return 1;
	return 0;
}

static void CG_BuildDragHudState( const playerState_t *ps, const centity_t *car,
	int now, cgDragHudState_t *state ) {
	int endTime;
	qboolean validRun;

	memset( state, 0, sizeof( *state ) );
	state->rpm = ps->stats[STAT_RPM];
	if ( state->rpm < 0 ) state->rpm = 0;
	if ( state->rpm > CP_RPM_MAX ) state->rpm = CP_RPM_MAX;
	state->reactionMs = state->elapsedMs = state->totalMs = -1;
	state->launchGuide = car->dragPhase < DRAG_RUNNING && ps->stats[STAT_GEAR] == 0;
	state->launchBand = CG_DragHudLaunchBand( state->rpm );
	state->engineStress = ps->stats[STAT_DRAG_ENGINE] & DRAG_ENGINE_STRESS_MASK;
	state->engineBlown = ( ps->stats[STAT_DRAG_ENGINE] & DRAG_ENGINE_BLOWN ) ? qtrue : qfalse;
	state->statusText = "GAS IN N TO STAGE";
	state->hintText = "MANUAL SHIFT";
	if ( car->dragPhase == DRAG_FALSE_START ) state->statusText = "FALSE START";
	else if ( car->dragPhase == DRAG_DNF ) state->statusText = "DID NOT FINISH";
	else if ( car->dragPhase == DRAG_FINISHED ) state->statusText = "FINISHED";
	else if ( car->dragPhase == DRAG_RUNNING ) state->statusText = "RUNNING";
	else if ( car->dragPhase == DRAG_COUNTDOWN ) {
		if ( car->dragGreenTime > 0 && now >= car->dragGreenTime )
			state->statusText = ps->stats[STAT_GEAR] == 0 ? "SELECT FIRST" : "LAUNCHING";
		else
			state->statusText = "WAIT FOR GREEN";
	} else if ( car->dragPhase == DRAG_STAGED ) state->statusText = "WAIT FOR LIGHTS";
	if ( state->engineBlown ) state->statusText = "ENGINE BLOWN";
	if ( state->engineBlown || car->dragPhase >= DRAG_FINISHED ) {
		state->hintText = "";
	} else if ( state->engineStress > 0 && state->rpm >= DRAG_ENGINE_REDLINE_RPM ) {
		/* The red zone is heating the engine: it blows at the limit. */
		state->hintText = "OVER-REV! ENGINE HEATING";
	} else if ( state->launchGuide ) {
		state->hintText = state->launchBand < 0 ? "BUILD REVS" :
			( state->launchBand > 0 ? "EASE OFF" : "HOLD REVS" );
	} else if ( ps->stats[STAT_GEAR] > 0 && ps->stats[STAT_GEAR] < 6 && state->rpm >= DRAG_HUD_SHIFT_RPM ) {
		state->shiftLight = qtrue;
		state->hintText = "SHIFT UP";
	} else if ( state->rpm >= CP_RPM_MAX - 320 ) {
		state->hintText = "REV LIMITER";
	}
	validRun = car->dragPhase == DRAG_RUNNING || car->dragPhase == DRAG_FINISHED;
	if ( !validRun || car->dragGreenTime <= 0 || car->dragLaunchTime < car->dragGreenTime ) return;
	state->reactionMs = car->dragLaunchTime - car->dragGreenTime;
	endTime = car->dragPhase == DRAG_FINISHED ? car->dragFinishTime : now;
	if ( endTime < car->dragLaunchTime ) endTime = car->dragLaunchTime;
	state->elapsedMs = endTime - car->dragLaunchTime;
	state->totalMs = endTime - car->dragGreenTime;
}

static void CG_DrawDragHudTime( int y, const char *label, int milliseconds,
	const float *labelColor ) {
	char value[24];
	if ( milliseconds < 0 ) Q_strncpyz( value, "--", sizeof( value ) );
	else Com_sprintf( value, sizeof( value ), "%d.%03d", milliseconds / 1000, milliseconds % 1000 );
	CG_DrawIngameString( 18, y + 2, label, UI_SMALLFONT, 0.50f, labelColor );
	CG_DrawIngameString( 154, y, value, UI_RIGHT | UI_SMALLFONT, 0.65f, colorWhite );
}

void CG_DrawDragTelemetryHud( void ) {
	static const vec4_t panel = { 0.008f, 0.012f, 0.016f, 0.76f };
	static const vec4_t line = { 0.300f, 0.390f, 0.430f, 0.64f };
	static const vec4_t muted = { 0.560f, 0.680f, 0.710f, 1.0f };
	static const vec4_t accent = Q3RALLY_ACCENT_COLOR;
	static const vec4_t ready = { 0.250f, 0.930f, 0.410f, 1.0f };
	static const vec4_t warning = { 1.0f, 0.660f, 0.100f, 1.0f };
	static const vec4_t danger = { 1.0f, 0.180f, 0.100f, 1.0f };
	static const vec4_t tach = { 0.075f, 0.095f, 0.105f, 0.98f };
	static const vec4_t fill = { 0.630f, 0.730f, 0.770f, 0.80f };
	static const vec4_t zone = { 0.100f, 0.520f, 0.230f, 0.94f };
	static const vec4_t redline = { 0.420f, 0.090f, 0.060f, 0.96f };
	const float *rpmColor, *statusColor;
	const playerState_t *ps;
	const centity_t *car;
	cgDragHudState_t state;
	screenPlacement_e savedHorizontal, savedVertical;
	float fraction, markerX, zoneX, zoneWidth, speed;
	int clientNum, gear, tick;
	char text[40], gearText[8];

	if ( !cg.snap || !cg_drawStatus.integer || cgs.gametype != GT_DRAG ) return;
	clientNum = cg.snap->ps.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) return;
	ps = cg.validPPS && cg.predictedPlayerState.clientNum == clientNum
		? &cg.predictedPlayerState : &cg.snap->ps;
	car = &cg_entities[clientNum];
	CG_BuildDragHudState( ps, car, cg.time, &state );
	fraction = CG_DragHudRpmFraction( state.rpm );
	rpmColor = state.rpm >= CP_RPM_MAX - 320 ? danger : colorWhite;
	if ( state.launchGuide ) rpmColor = state.launchBand == 0 ? ready : warning;
	statusColor = car->dragPhase >= DRAG_FALSE_START || state.engineBlown ? danger : accent;
	if ( car->dragPhase == DRAG_COUNTDOWN && car->dragGreenTime > 0 && cg.time >= car->dragGreenTime ) statusColor = ready;

	savedHorizontal = CG_GetScreenHorizontalPlacement();
	savedVertical = CG_GetScreenVerticalPlacement();
	CG_SetScreenPlacement( PLACE_STRETCH, PLACE_BOTTOM );
	CG_FillRect( 0, DRAG_HUD_TOP, SCREEN_WIDTH, SCREEN_HEIGHT - DRAG_HUD_TOP, panel );
	CG_FillRect( 0, DRAG_HUD_TOP, SCREEN_WIDTH, 1, accent );
	CG_SetScreenPlacement( PLACE_CENTER, PLACE_BOTTOM );
	CG_FillRect( 164, 398, 1, 70, line );
	CG_FillRect( 476, 398, 1, 70, line );
	CG_DrawIngameString( 18, 395, "DRAG RACE", UI_SMALLFONT, 0.58f, accent );
	if ( cg_hudShowTimes.integer ) {
		CG_DrawDragHudTime( 414, "RT", state.reactionMs, muted );
		CG_DrawDragHudTime( 432, "ET", state.elapsedMs, muted );
		CG_DrawDragHudTime( 450, "TOTAL", state.totalMs, muted );
	}
	CG_DrawIngameString( 180, 395, state.statusText, UI_SMALLFONT, 0.53f, statusColor );
	CG_DrawIngameString( 180, 418, "RPM", UI_SMALLFONT, 0.56f, muted );
	Com_sprintf( text, sizeof( text ), "%d", state.rpm );
	CG_DrawIngameString( 450, 409, text, UI_RIGHT, 1.10f, rpmColor );
	CG_FillRect( DRAG_HUD_TACH_X, 442, DRAG_HUD_TACH_WIDTH, 12, tach );
	CG_FillRect( DRAG_HUD_TACH_X, 442, DRAG_HUD_TACH_WIDTH * fraction, 12, fill );
	zoneX = DRAG_HUD_TACH_X + DRAG_HUD_TACH_WIDTH * CG_DragHudRpmFraction( DRAG_ENGINE_REDLINE_RPM );
	CG_FillRect( zoneX, 442, DRAG_HUD_TACH_X + DRAG_HUD_TACH_WIDTH - zoneX, 12, redline );
	if ( state.launchGuide ) {
		zoneX = DRAG_HUD_TACH_X + DRAG_HUD_TACH_WIDTH * CG_DragHudRpmFraction( DRAG_HUD_LAUNCH_RPM_MIN );
		zoneWidth = DRAG_HUD_TACH_WIDTH * ( DRAG_HUD_LAUNCH_RPM_MAX - DRAG_HUD_LAUNCH_RPM_MIN ) / CP_RPM_MAX;
		CG_FillRect( zoneX, 442, zoneWidth, 12, zone );
		CG_FillRect( zoneX, 440, zoneWidth, 2, ready );
		Com_sprintf( text, sizeof( text ), "%d-%d RPM", DRAG_HUD_LAUNCH_RPM_MIN, DRAG_HUD_LAUNCH_RPM_MAX );
		CG_DrawIngameString( 450, 462, text, UI_RIGHT | UI_SMALLFONT, 0.43f, muted );
	}
	for ( tick = 1; tick <= 6; tick++ ) {
		markerX = DRAG_HUD_TACH_X + DRAG_HUD_TACH_WIDTH * CG_DragHudRpmFraction( tick * 1000 );
		CG_FillRect( markerX, 452, 1, 4, line );
	}
	markerX = DRAG_HUD_TACH_X + ( DRAG_HUD_TACH_WIDTH - 2 ) * fraction;
	CG_FillRect( markerX, 439, 2, 18, rpmColor );
	if ( state.engineStress > 0 && !state.engineBlown ) {
		/* Engine heat under the tachometer; flashes when close to blowing. */
		fraction = state.engineStress / (float)DRAG_ENGINE_STRESS_LIMIT_MS;
		if ( fraction < 0.66f || ( cg.time / 120 ) & 1 ) {
			CG_FillRect( DRAG_HUD_TACH_X, 456, DRAG_HUD_TACH_WIDTH * fraction, 3,
				fraction < 0.5f ? warning : danger );
		}
	}
	CG_DrawIngameString( 180, 462, state.hintText, UI_SMALLFONT, 0.47f,
		state.engineStress > 0 && state.rpm >= DRAG_ENGINE_REDLINE_RPM ? danger :
		( state.shiftLight ? ready : rpmColor ) );

	gear = ps->stats[STAT_GEAR];
	if ( gear < 0 ) Q_strncpyz( gearText, "R", sizeof( gearText ) );
	else if ( gear == 0 ) Q_strncpyz( gearText, "N", sizeof( gearText ) );
	else Com_sprintf( gearText, sizeof( gearText ), "%d", gear );
	CG_DrawIngameString( 508, 395, "GEAR", UI_CENTER | UI_SMALLFONT, 0.50f, muted );
	CG_DrawIngameString( 508, 413, gearText, UI_CENTER, 1.60f, colorWhite );
	speed = Q3VelocityToRL( sqrt( ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1] ) );
	Com_sprintf( text, sizeof( text ), "%d", (int)( speed + 0.5f ) );
	CG_DrawIngameString( 626, 413, text, UI_RIGHT, 1.10f, colorWhite );
	CG_DrawIngameString( 626, 444, cg_metricUnits.integer ? "KM/H" : "MPH", UI_RIGHT | UI_SMALLFONT, 0.55f, muted );
	CG_SetScreenPlacement( savedHorizontal, savedVertical );
}
/* End Drag Race telemetry. */

static qboolean CG_IsMovingAwayFromCheckpoint( const vec3_t velocity,
	const vec3_t toCheckpoint, float angleDiff ) {
	return velocity[0] * velocity[0] + velocity[1] * velocity[1] > 1600 &&
		( toCheckpoint[0] != 0 || toCheckpoint[1] != 0 ) && fabs( angleDiff ) > 100;
}

/* -----------------------------------------------------------------------
   CG_DrawArrowToCheckpoint
   Draws the 3D arrow model and "WRONG WAY!" flash.
   ----------------------------------------------------------------------- */
float CG_DrawArrowToCheckpoint( float y ) {
	centity_t	*cent;
	vec3_t		forward, origin, angles;
	int			i;
	float		angle1, angle2, angleDiff;
	float		fx, fy, fw, fh;
	float		*color;
	refdef_t	refdef;
	refEntity_t	ent;
	vec3_t		mins, maxs, v;

	/* Suppress checkpoint arrow and Wrong Way during intro camera */
	if ( CG_IntroCam_IsActive() ||
		cg.snap->ps.pm_type != PM_NORMAL ||
		!cg_entities[cg.snap->ps.clientNum].startRaceTime ||
		cg.time < cg_entities[cg.snap->ps.clientNum].startRaceTime ||
		cg_entities[cg.snap->ps.clientNum].finishRaceTime ) {
		cg.wrongWayStartTime = 0;
		return y;
	}

	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		cent = &cg_entities[i];
		if ( cent->currentState.eType != ET_CHECKPOINT ) continue;
		if ( cent->currentState.weapon != cg.snap->ps.stats[STAT_NEXT_CHECKPOINT] ) continue;
		break;
	}

	if ( i == MAX_GENTITIES ) {
		cg.wrongWayStartTime = 0;
		return y;
	}

	trap_R_ModelBounds( cgs.inlineDrawModel[cent->currentState.modelindex], mins, maxs );

	if ( cent->currentState.frame == 0 ) {
		VectorAdd( mins, cent->currentState.origin, mins );
		VectorAdd( maxs, cent->currentState.origin, maxs );
	}

	for ( i = 0; i < 3; i++ ) {
		if      ( cg.predictedPlayerState.origin[i] < mins[i] ) v[i] = mins[i] - cg.predictedPlayerState.origin[i];
		else if ( cg.predictedPlayerState.origin[i] > maxs[i] ) v[i] = maxs[i] - cg.predictedPlayerState.origin[i];
		else                                                      v[i] = 0;
	}

	angle2 = ( v[0] == 0 && v[1] == 0 && v[2] == 0 )
	         ? cg.predictedPlayerState.viewangles[YAW]
	         : vectoyaw( v );

	if ( cg_checkpointArrowMode.integer == 1 ) {
		AngleVectors( cg.refdefViewAngles, forward, NULL, NULL );
		angle1     = vectoyaw( forward );
		angleDiff  = AngleDifference( angle1, angle2 );

		VectorSet( origin, 80, 0, 20 );
		VectorClear( angles );
		angles[YAW] = -angleDiff;

		fx = 320 - 64;  fy = 16;  fw = 128;  fh = 96;
		CG_AdjustFrom640( &fx, &fy, &fw, &fh );

		memset( &refdef, 0, sizeof( refdef ) );
		memset( &ent,    0, sizeof( ent    ) );

		AnglesToAxis( angles, ent.axis );
		VectorCopy( origin, ent.origin );
		VectorCopy( origin, ent.lightingOrigin );
		ent.hModel   = cgs.media.checkpointArrow;
		ent.renderfx = RF_NOSHADOW;

		refdef.rdflags = RDF_NOWORLDMODEL;
		vectoangles( origin, angles );
		AnglesToAxis( angles, refdef.viewaxis );
		refdef.fov_x  = 40;
		refdef.fov_y  = 30;
		refdef.x      = fx;  refdef.y      = fy;
		refdef.width  = fw;  refdef.height = fh;
		refdef.time   = cg.time;

		trap_R_ClearScene();
		trap_R_AddRefEntityToScene( &ent );
		trap_R_RenderScene( &refdef );
	}

	/* Judge travel direction, not where a stationary/reversing car is facing. */
	angle1    = vectoyaw( cg.predictedPlayerState.velocity );
	angleDiff = AngleDifference( angle1, angle2 );

	if ( CG_IsMovingAwayFromCheckpoint( cg.predictedPlayerState.velocity, v, angleDiff ) ) {
		cg.wrongWayTime = cg.time;
		if ( !cg.wrongWayStartTime )
			cg.wrongWayStartTime = cg.time;
	} else {
		cg.wrongWayStartTime = 0;
	}

	if ( !cg.wrongWayStartTime || cg.wrongWayStartTime > cg.time - 2000 )
		return y;

	color = CG_FadeColor( cg.wrongWayTime, 300 );
	if ( !color )
		return y;

	CG_DrawIngameString( (int)( SCREEN_WIDTH * 0.5f ),
	                      (int)( SCREEN_HEIGHT * 0.30f ), "WRONG WAY!",
	                      UI_CENTER | UI_DROPSHADOW, 0.9f, color );

	return y;
}
/* -----------------------------------------------------------------------
   CG_DrawEliminationTimeline
   ----------------------------------------------------------------------- */
float CG_DrawEliminationTimeline( float y ) {
	int			i;
	float		x;
	char		line[64];
	const float	columnWidth = CG_GetEliminationColumnWidth();
	const float	rowHeight   = HUD_ROW_HEIGHT;

	if ( !cg_elimTimeline.integer )                                       return y;
	if ( cgs.gametype != GT_LCS )                                        return y;
	if ( cg.elimTimelineCount <= 0 )                                      return y;

	x = HUD_RIGHT_EDGE - columnWidth;

	for ( i = cg.elimTimelineCount - 1; i >= 0; --i ) {
		const cgElimTimelineEvent_t *event = &cg.elimTimelineEvents[i];
		const char *name;
		int elapsedSeconds = 0;

		if ( event->clientNum < 0 || event->clientNum >= MAX_CLIENTS ) continue;

		name = cgs.clientinfo[event->clientNum].name;
		if ( !name || !name[0] ) name = va("#%d", event->clientNum);

		if ( event->timestamp > 0 && cg.time > event->timestamp )
			elapsedSeconds = ( cg.time - event->timestamp ) / 1000;

		Com_sprintf( line, sizeof(line), "R%02d LEFT%02d %s (%is)",
		             event->round, event->remaining, name, elapsedSeconds );

		CG_FillRect( x, y, columnWidth, rowHeight, bgColor );
		CG_DrawIngameSmallString( (int)( x + HUD_TEXT_INSET ), (int)y + 1, line, colorWhite );
		y += rowHeight;
	}

	return y;
}
