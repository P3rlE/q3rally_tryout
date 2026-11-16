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

/* "HEAT 2 OF 3   YOU 1 : 0 CARLA" above the Drag panel, for best-of matches. */
static void CG_DrawDragMatchLine( const float *panel, const float *accent, const float *muted ) {
	char text[96], left[32], right[32];
	int self, i;
	if ( cgs.dragMatch.bestOf <= 1 || !cgs.dragMatch.heat ) return;
	self = cg.snap ? cg.snap->ps.clientNum : -1;
	if ( cgs.dragMatch.over ) {
		Com_sprintf( text, sizeof( text ), "MATCH OVER" );
	} else {
		Com_sprintf( text, sizeof( text ), "HEAT %d OF %d", cgs.dragMatch.heat, cgs.dragMatch.bestOf );
	}
	CG_FillRect( 196, DRAG_HUD_TOP - 18, 248, 18, panel );
	CG_DrawIngameString( 204, DRAG_HUD_TOP - 15, text, UI_SMALLFONT, 0.46f, muted );
	if ( cgs.dragMatch.count != 2 ) return;
	for ( i = 0; i < 2; i++ ) {
		const char *name = cgs.dragMatch.client[i] == self ? "YOU"
			: cgs.clientinfo[cgs.dragMatch.client[i]].name;
		Q_strncpyz( i ? right : left, name, sizeof( left ) );
		Q_CleanStr( i ? right : left );
	}
	Com_sprintf( text, sizeof( text ), "%s %d : %d %s", left, cgs.dragMatch.wins[0], cgs.dragMatch.wins[1], right );
	CG_DrawIngameString( 436, DRAG_HUD_TOP - 15, text, UI_RIGHT | UI_SMALLFONT, 0.46f, accent );
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
	CG_DrawDragMatchLine( panel, accent, muted );
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

/*
=================
CG_DrawDragTourney

The Drag tournament bracket between matches. The heat results come first;
holding the scoreboard key brings them back. Returns qfalse when the
results screen should be drawn instead.
=================
*/
#define DT_TOP		104
#define DT_BOTTOM	430
#define DT_LEFT		24
#define DT_RIGHT	616

static void CG_DragTourneyFit( const char *in, char *out, int size, int width, float scale ) {
	int length;
	Q_strncpyz( out, in, size );
	for ( length = strlen( out ); length > 1 && CG_IngameStringWidth( out, UI_SMALLFONT, scale ) > width; ) {
		out[--length] = '\0';
	}
}

static void CG_DragTourneyBox( int x, int y, int w, int h, int entrant, int wins,
	qboolean lost, qboolean next, const char *self ) {
	static const vec4_t box = { 0.040f, 0.052f, 0.060f, 0.94f };
	static const vec4_t edge = { 0.300f, 0.390f, 0.430f, 0.64f };
	static const vec4_t muted = { 0.560f, 0.680f, 0.710f, 1.0f };
	static const vec4_t dim = { 0.380f, 0.430f, 0.450f, 1.0f };
	static const vec4_t accent = Q3RALLY_ACCENT_COLOR;
	const float *color;
	char name[20], text[24];
	float scale = h >= 18 ? 0.55f : 0.46f;
	int textY = y + ( h - (int)( 14 * scale ) ) / 2;

	CG_FillRect( x, y, w, h, box );
	if ( next ) {
		CG_FillRect( x, y, 2, h, accent );
		CG_FillRect( x, y, w, 1, accent );
		CG_FillRect( x, y + h - 1, w, 1, accent );
		CG_FillRect( x + w - 1, y, 1, h, accent );
	} else {
		CG_FillRect( x, y + h - 1, w, 1, edge );
	}
	if ( entrant < 0 ) {
		CG_DrawIngameString( x + 6, textY, "-", UI_SMALLFONT, scale, dim );
		return;
	}
	if ( lost ) color = dim;
	else if ( !Q_stricmp( cgs.dragTourney.name[entrant], self ) && cgs.dragTourney.human[entrant] ) color = accent;
	else color = cgs.dragTourney.human[entrant] ? colorWhite : muted;
	CG_DragTourneyFit( cgs.dragTourney.name[entrant], name, sizeof( name ), w - ( wins >= 0 ? 24 : 10 ), scale );
	CG_DrawIngameString( x + 6, textY, name, UI_SMALLFONT, scale, color );
	if ( wins >= 0 ) {
		Com_sprintf( text, sizeof( text ), "%d", wins );
		CG_DrawIngameString( x + w - 6, textY, text, UI_SMALLFONT | UI_RIGHT, scale, lost ? dim : colorWhite );
	}
}

qboolean CG_DrawDragTourney( void ) {
	static const vec4_t background = { 0.045f, 0.045f, 0.045f, 1.0f };
	static const vec4_t line = { 0.300f, 0.390f, 0.430f, 0.64f };
	static const vec4_t muted = { 0.560f, 0.680f, 0.710f, 1.0f };
	static const vec4_t accent = Q3RALLY_ACCENT_COLOR;
	static int since, lastSeen;
	int slot[5][16], start[5], rounds, size, played, r, j, m, column, boxW, boxH, x, y, pairY;
	float area;
	char self[MAX_QPATH], text[96];
	qboolean ready;
	int clientNum;

	size = cgs.dragTourney.size;
	if ( cgs.gametype != GT_DRAG || !size || !cg.snap ) return qfalse;
	if ( cg.time - lastSeen > 1000 || cg.time < lastSeen ) since = cg.time;
	lastSeen = cg.time;
	if ( cg.time - since < 5000 || cg.showScores ) return qfalse;

	played = cgs.dragTourney.played;
	for ( rounds = 0; ( 1 << rounds ) < size; rounds++ ) {
	}
	/* who sits in which slot: round 0 is the draw, then the winners */
	start[0] = 0;
	for ( j = 0; j < size; j++ ) slot[0][j] = j;
	for ( r = 1; r <= rounds; r++ ) {
		start[r] = start[r - 1] + ( size >> r );
		for ( j = 0; j < size >> r; j++ ) {
			m = start[r - 1] + j;
			if ( m < played && slot[r - 1][2 * j] >= 0 && slot[r - 1][2 * j + 1] >= 0 ) {
				slot[r][j] = slot[r - 1][2 * j + cgs.dragTourney.winner[m]];
			} else {
				slot[r][j] = -1;
			}
		}
	}

	clientNum = cg.snap->ps.clientNum;
	self[0] = '\0';
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		Q_strncpyz( self, cgs.clientinfo[cg.clientNum].name, sizeof( self ) );
		Q_CleanStr( self );
	}

	CG_SetScreenPlacement( PLACE_STRETCH, PLACE_STRETCH );
	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, background );
	CG_SetScreenPlacement( PLACE_CENTER, PLACE_CENTER );
	CG_FillRect( 24, 24, SCREEN_WIDTH - 48, 2, accent );
	CG_DrawIngameString( 24, 35, "DRAG TOURNAMENT", UI_SMALLFONT, 0.8f, accent );
	if ( played >= size - 1 ) {
		Com_sprintf( text, sizeof( text ), "%s WINS", cgs.dragTourney.name[slot[rounds][0]] );
	} else {
		static const char *roundNames[] = { "FINAL", "SEMI-FINAL", "QUARTER-FINAL", "ROUND OF 16" };
		int index, left;
		for ( r = 0; r < rounds && played >= start[r] + ( size >> ( r + 1 ) ); r++ ) {
		}
		left = rounds - r - 1;
		index = left < 0 ? 0 : ( left > 3 ? 3 : left );
		Com_sprintf( text, sizeof( text ), "%s", roundNames[index] );
	}
	CG_DrawIngameString( 24, 53, text, UI_BIGFONT, 1.1f, colorWhite );
	Com_sprintf( text, sizeof( text ), "%d DRIVERS  -  BEST OF %d", size, cgs.dragTourney.bestOf );
	CG_DrawIngameString( SCREEN_WIDTH - 24, 38, text, UI_SMALLFONT | UI_RIGHT, 0.6f, muted );
	ready = clientNum >= 0 && clientNum < 32 && ( cg.snap->ps.stats[STAT_CLIENTS_READY] & ( 1u << clientNum ) );
	if ( !cg.demoPlayback ) {
		CG_DrawIngameString( SCREEN_WIDTH - 24, 58, ready ? "READY" : "FIRE / USE TO CONTINUE",
			UI_SMALLFONT | UI_RIGHT, 0.7f, colorWhite );
	}

	column = ( DT_RIGHT - DT_LEFT ) / ( rounds + 1 );
	boxW = column - 16;
	area = (float)( DT_BOTTOM - DT_TOP ) / size;
	boxH = (int)area - 4;
	if ( boxH > 22 ) boxH = 22;
	for ( r = 0; r <= rounds; r++ ) {
		int count = size >> r;
		float pitch = ( DT_BOTTOM - DT_TOP ) / (float)count;
		x = DT_LEFT + r * column;
		for ( j = 0; j < count; j++ ) {
			int wins = -1;
			qboolean lost = qfalse, next = qfalse;
			y = DT_TOP + (int)( ( j + 0.5f ) * pitch ) - boxH / 2;
			if ( r < rounds ) {
				m = start[r] + j / 2;
				if ( m < played ) {
					wins = cgs.dragTourney.wins[m][j & 1];
					lost = cgs.dragTourney.winner[m] != ( j & 1 );
				} else if ( m == played ) {
					next = qtrue;
				}
				/* connector to the next round */
				if ( j & 1 ) {
					pairY = DT_TOP + (int)( ( j - 0.5f ) * pitch );
					CG_FillRect( x + boxW, pairY, 8, 1, line );
					CG_FillRect( x + boxW, y + boxH / 2, 8, 1, line );
					CG_FillRect( x + boxW + 8, pairY, 1, y + boxH / 2 - pairY + 1, line );
					CG_FillRect( x + boxW + 8, DT_TOP + (int)( ( j / 2 + 0.5f ) * 2 * pitch ), 8, 1, line );
				}
			}
			CG_DragTourneyBox( x, y, boxW, boxH, slot[r][j], wins, lost, next, self );
		}
	}

	if ( played < size - 1 && cgs.dragTourney.current[0] >= 0 && cgs.dragTourney.current[1] >= 0
		&& !cgs.dragTourney.human[cgs.dragTourney.current[0]] && !cgs.dragTourney.human[cgs.dragTourney.current[1]] ) {
		CG_DrawIngameString( 24, 448, "NEXT IS A BOT MATCH:  /dragskip  SKIPS IT,  /dragskip all  SKIPS TO YOUR NEXT MATCH",
			UI_SMALLFONT, 0.5f, muted );
	} else {
		CG_DrawIngameString( 24, 448, "HOLD SCOREBOARD FOR THE HEAT RESULTS", UI_SMALLFONT, 0.5f, muted );
	}
	return qtrue;
}
