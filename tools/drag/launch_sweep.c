/*
 * Drag launch sweep: runs the production Pmove, compiled from the shared bg_*
 * sources, on a flat, dry asphalt plane and measures a quarter-mile run per
 * launch RPM. Build and run with tools/drag/launch_sweep.py.
 *
 * The game runs pmove_fixed 1 with pmove_msec 18; this harness does the same.
 * The upshift policy belongs to the harness; the game still requires manual
 * shifts. Output is CSV, one line per launch RPM.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "qcommon/q_shared.h"
#include "game/bg_public.h"

void Pmove( pmove_t *pmove );
#define moveCar Pmove

/* Engine/cgame services the shared movement code links against. */
void QDECL Com_Printf( const char *fmt, ... ) {}
void QDECL Com_Error( int level, const char *fmt, ... ) { fprintf( stderr, "Com_Error\n" ); exit( 3 ); }
void CG_Sparks( const vec3_t origin, const vec3_t normal, const vec3_t direction, const float speed ) {}
qboolean isRaceObserver( int clientNum ) { return qfalse; }

#define QUARTER_MILE 14348.0f
#define QU_PER_M 35.6613f

static void floorTrace( trace_t *tr, const vec3_t start, const vec3_t mins,
	const vec3_t maxs, const vec3_t end, int pass, int mask ) {
	float a = start[2] + mins[2], b = end[2] + mins[2], fraction;
	int i;
	memset( tr, 0, sizeof( *tr ) ); tr->fraction = 1; tr->entityNum = ENTITYNUM_NONE;
	for ( i = 0; i < 3; i++ ) tr->endpos[i] = end[i];
	if ( a < -0.001f ) {
		tr->startsolid = qtrue; tr->allsolid = b < 0; tr->contents = CONTENTS_SOLID;
		tr->entityNum = ENTITYNUM_WORLD; return;
	}
	if ( b < 0 && a >= 0 ) {
		fraction = ( a - 0.125f ) / ( a - b );
		if ( fraction < 0 ) fraction = 0;
		tr->fraction = fraction;
		for ( i = 0; i < 3; i++ ) tr->endpos[i] = start[i] + fraction * ( end[i] - start[i] );
		tr->plane.normal[2] = 1; tr->plane.type = 2;
		tr->surfaceFlags = SURF_ASPHALT; tr->contents = CONTENTS_SOLID;
		tr->entityNum = ENTITYNUM_WORLD;
	}
}
static int contents( const vec3_t point, int pass ) { return point[2] < 0 ? CONTENTS_SOLID : 0; }
static qboolean friction( const carPoint_t *point, float *s, float *k ) {
	*s = CP_SCOF; *k = CP_KCOF; return qtrue;
}
static void setup( pmove_t *m, playerState_t *p, car_t *car ) {
	memset( m, 0, sizeof( *m ) ); memset( p, 0, sizeof( *p ) ); memset( car, 0, sizeof( *car ) );
	m->ps = p; m->car = car; m->client = qtrue; m->trace = floorTrace; m->pointcontents = contents;
	m->frictionFunc = friction; m->tracemask = MASK_PLAYERSOLID; m->controlMode = 1;
	m->manualShift = qtrue; m->transmissionMode = TR_MANUAL; m->dragRace = qtrue;
	m->pmove_fixed = 1; m->pmove_msec = 18;
	m->car_spring = 120; m->car_shock_up = 12; m->car_shock_down = 11;
	m->car_swaybar = 20; m->car_wheel = 2400; m->car_wheel_damp = 140;
	m->car_frontweight_dist = .5f; m->car_IT_xScale = m->car_IT_yScale = m->car_IT_zScale = 1;
	m->car_body_elasticity = .05f; m->car_air_cof = .31f; m->car_air_frac_to_df = .5f;
	m->car_friction_scale = 1.1f;
	p->pm_type = PM_NORMAL; p->gravity = 800; p->stats[STAT_HEALTH] = 125;
	p->stats[STAT_MAX_HEALTH] = 125; p->origin[2] = 28; car->initializeOnNextMove = qtrue;
	car->fuel = car->maxFuel = 100;
}

static void run( float startRpm, int step, float shiftRpm ) {
	pmove_t m; playerState_t p; car_t car;
	int t, launch, minRpm = 99999, engine;
	float startX, lastX = 0, x, t60ft = 0, speed;
	setup( &m, &p, &car );
	/* Settle on the grid in N, then build the launch RPM with the throttle. */
	m.cmd.buttons = BUTTON_HANDBRAKE;
	for ( t = step; t <= 2000; t += step ) { m.cmd.serverTime = t; moveCar( &m ); }
	/* The countdown arms the engine: two seconds of holding these revs. */
	p.stats[STAT_DRAG_ENGINE] = DRAG_ENGINE_ARMED;
	m.cmd.forwardmove = (signed char)( 127.0f * ( startRpm - CP_RPM_MIN ) / ( CP_RPM_MAX - CP_RPM_MIN ) + 0.5f );
	if ( startRpm >= CP_RPM_MAX ) m.cmd.forwardmove = 127;
	for ( ; t <= 4000; t += step ) { m.cmd.serverTime = t; moveCar( &m ); }
	startX = p.origin[0];
	car.gear = 1; p.stats[STAT_GEAR] = 1;
	m.cmd.buttons = 0; m.cmd.forwardmove = 127; launch = p.commandTime;
	for ( t = step; t <= 25000; t += step ) {
		lastX = p.origin[0] - startX;
		if ( t > 300 && car.gear < 6 && car.rpm >= shiftRpm ) car.gear++;
		m.cmd.serverTime = launch + t; moveCar( &m ); x = p.origin[0] - startX;
		if ( t < 1500 && p.stats[STAT_RPM] < minRpm ) minRpm = p.stats[STAT_RPM];
		if ( getenv( "LAUNCH_SWEEP_TRACE" ) && ( t % 96 == 0 || t <= 4 * step ) ) {
			float vx = p.velocity[0];
			fprintf( stderr, "t=%5d x=%7.1f kph=%5.1f gear=%d rpm=%4d slip=%d%d%d%d rearslip=%.0f\n", t, x / QU_PER_M,
				vx * 3.6f / QU_PER_M, car.gear, p.stats[STAT_RPM],
				car.sPoints[0].slipping, car.sPoints[1].slipping, car.sPoints[2].slipping, car.sPoints[3].slipping,
				car.sPoints[2].w * WHEEL_RADIUS + vx );
		}
		speed = sqrtf( p.velocity[0] * p.velocity[0] + p.velocity[1] * p.velocity[1] ) * 3.6f / QU_PER_M;
		if ( !t60ft && x >= 18.288f * QU_PER_M ) t60ft = t - step + step * ( 18.288f * QU_PER_M - lastX ) / ( x - lastX );
		if ( x >= QUARTER_MILE ) {
			float et = t - step + step * ( QUARTER_MILE - lastX ) / ( x - lastX );
			engine = p.stats[STAT_DRAG_ENGINE];
			printf( "%.0f,%d,%.0f,%.3f,%.3f,%.1f,%d,%d\n", startRpm, step, shiftRpm,
				et / 1000.0f, t60ft / 1000.0f, speed, minRpm, engine );
			return;
		}
	}
	printf( "%.0f,%d,%.0f,FAIL x=%.1f gear=%d rpm=%.0f engine=%d\n", startRpm, step, shiftRpm,
		p.origin[0] - startX, car.gear, car.rpm, p.stats[STAT_DRAG_ENGINE] );
}

/* Armed and flat out in N: how long until the engine blows on the limiter? */
static void limiter( int step ) {
	pmove_t m; playerState_t p; car_t car;
	int t;
	setup( &m, &p, &car );
	m.cmd.buttons = BUTTON_HANDBRAKE;
	for ( t = step; t <= 1000; t += step ) { m.cmd.serverTime = t; moveCar( &m ); }
	p.stats[STAT_DRAG_ENGINE] = DRAG_ENGINE_ARMED;
	m.cmd.forwardmove = 127;
	for ( t = step; t <= 20000; t += step ) {
		m.cmd.serverTime = 1000 + t; moveCar( &m );
		if ( p.stats[STAT_DRAG_ENGINE] & DRAG_ENGINE_BLOWN ) {
			printf( "limiter,%d,blown after %.2f s\n", step, t / 1000.0f );
			return;
		}
	}
	printf( "limiter,%d,survived 20 s\n", step );
}

int main( int argc, char **argv ) {
	int rpm, step;
	float shiftRpm;
	step = argc > 2 ? atoi( argv[2] ) : 16;
	if ( step <= 0 ) step = 16;
	shiftRpm = argc > 3 ? (float)atof( argv[3] ) : 6000.0f;
	puts( "launch_rpm,cmd_ms,shift_rpm,et_s,60ft_s,trap_kph,min_rpm_1.5s,engine_state" );
	if ( argc > 1 && !strcmp( argv[1], "limiter" ) ) {
		limiter( step );
	} else if ( argc > 1 && strcmp( argv[1], "all" ) ) {
		run( (float)atof( argv[1] ), step, shiftRpm );
	} else {
		for ( rpm = 1000; rpm <= 6250; rpm += 250 ) run( (float)rpm, step, shiftRpm );
	}
	return 0;
}
