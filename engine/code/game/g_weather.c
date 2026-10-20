/*
===========================================================================
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
//
// g_weather.c -- Q3Rally weather on the server
//
// The server runs the weather of the whole map: dry spells and showers of
// rain or snow that build up, last a while and pass, also during a race.
// Every change goes to the clients as an anchor in CS_WEATHER (see
// bg_weather.c); between changes server and clients work out the same
// intensity and the same wet or snowed in ground on their own.
//
// Map keys (worldspawn):
//   weather          none | rain | snow | "rain snow"   what the map can have
//   weatherChance    0..100, share of the time with rain or snow (40)
//   weatherCycle     "min max" seconds a shower or a dry spell lasts (90 300)
//   weatherStrength  "min max" intensity of a shower, 0..1 (0.4 1)
//
// g_weather: 0 = always dry, 1 = as the map says, 2 = rain, 3 = snow.
// Ghost Race is always dry.

#include "g_local.h"

#define WEATHER_RAMP_MIN	30		// seconds for a shower to build up or pass
#define WEATHER_RAMP_MAX	60
#define WEATHER_SWITCH_RAMP	10		// seconds when g_weather is changed

typedef enum {
	WPHASE_DRY,			// no rain or snow until nextChange
	WPHASE_SHOWER,		// building up and holding until nextChange
	WPHASE_PASSING		// fading out until nextChange
} weatherPhase_t;

typedef struct {
	// what the map allows
	qboolean	rain, snow;
	qboolean	mapKey;			// the map has a "weather" key
	int			chance;
	int			cycleMin, cycleMax;
	float		strengthMin, strengthMax;
	qboolean	legacyRain, legacySnow;

	int			mode;			// g_weather in use (0 for Ghost Race)
	weatherPhase_t phase;
	int			nextChange;
	int			type;			// precipitation of the current or last shower
} gWeather_t;

static gWeather_t		gw;
static weatherGrid_t	g_weatherGrid;
static weatherState_t	g_weatherState;

/*
=================
G_WeatherBuildGrid

Called before the entities are spawned: the traces only see the world.
=================
*/
void G_WeatherBuildGrid( void ) {
	gentity_t *tmp;
	vec3_t mins, maxs;
	char buf[MAX_INFO_STRING];
	int start;

	memset( &gw, 0, sizeof( gw ) );
	memset( &g_weatherState, 0, sizeof( g_weatherState ) );
	g_weatherGrid.valid = qfalse;

	// the bounds of the world model, through a spare entity
	tmp = &g_entities[ENTITYNUM_MAX_NORMAL - 1];
	memset( tmp, 0, sizeof( *tmp ) );
	trap_SetBrushModel( tmp, "*0" );
	VectorCopy( tmp->r.mins, mins );
	VectorCopy( tmp->r.maxs, maxs );
	trap_UnlinkEntity( tmp );
	memset( tmp, 0, sizeof( *tmp ) );

	if ( maxs[0] <= mins[0] || maxs[1] <= mins[1] ) {
		trap_SetConfigstring( CS_WEATHERGRID, "" );
		return;
	}

	start = trap_Milliseconds();
	BG_WeatherGridPlace( &g_weatherGrid, mins, maxs );
	BG_WeatherGridBuild( &g_weatherGrid, trap_Trace );

	BG_WeatherGridString( &g_weatherGrid, buf, sizeof( buf ) );
	trap_SetConfigstring( CS_WEATHERGRID, buf );

	G_Printf( "weather grid %i x %i, cell %i, %i ms\n", g_weatherGrid.width, g_weatherGrid.height,
		(int)g_weatherGrid.cell, trap_Milliseconds() - start );
}

static void G_WeatherParseRange( const char *s, float *lo, float *hi ) {
	char buf[64];
	char *p, *tok;

	Q_strncpyz( buf, s, sizeof( buf ) );
	p = buf;
	tok = COM_Parse( &p );
	if ( tok[0] ) {
		*lo = atof( tok );
		tok = COM_Parse( &p );
		*hi = tok[0] ? atof( tok ) : *lo;
	}
	if ( *hi < *lo ) {
		float t = *lo;
		*lo = *hi;
		*hi = t;
	}
}

/*
=================
G_WeatherReadMap

From SP_worldspawn.
=================
*/
void G_WeatherReadMap( void ) {
	char *s;
	float lo, hi;

	gw.mapKey = G_SpawnString( "weather", "", &s );
	if ( gw.mapKey ) {
		gw.rain = Q_stristr( s, "rain" ) != NULL;
		gw.snow = Q_stristr( s, "snow" ) != NULL;
	}

	G_SpawnString( "weatherChance", "40", &s );
	gw.chance = atoi( s );
	if ( gw.chance < 0 ) {
		gw.chance = 0;
	} else if ( gw.chance > 100 ) {
		gw.chance = 100;
	}

	lo = 90.0f;
	hi = 300.0f;
	G_SpawnString( "weatherCycle", "90 300", &s );
	G_WeatherParseRange( s, &lo, &hi );
	gw.cycleMin = MAX( 10, (int)lo );
	gw.cycleMax = MAX( gw.cycleMin, (int)hi );

	lo = 0.4f;
	hi = 1.0f;
	G_SpawnString( "weatherStrength", "0.4 1", &s );
	G_WeatherParseRange( s, &lo, &hi );
	gw.strengthMin = Com_Clamp( 0.05f, 1.0f, lo );
	gw.strengthMax = Com_Clamp( gw.strengthMin, 1.0f, hi );
}

void G_WeatherLegacyZone( int type ) {
	if ( type == WEATHER_SNOW ) {
		gw.legacySnow = qtrue;
	} else {
		gw.legacyRain = qtrue;
	}
}

/*
=================
G_WeatherPublish

Starts a new anchor from where the ground is now. The server works from
the anchor as the clients read it from the string, so both round the same.
=================
*/
static void G_WeatherPublish( int type, int rampSeconds, float target, qboolean instant ) {
	weatherAnchor_t a;
	char buf[MAX_INFO_STRING];

	BG_WeatherAdvance( &g_weatherState, level.time );

	a.type = type;
	a.startTime = level.time;
	a.endTime = level.time + rampSeconds * 1000;
	a.intensity0 = instant ? target : g_weatherState.intensity;
	a.intensity1 = target;
	a.wetness0 = g_weatherState.wetness;
	a.snow0 = g_weatherState.snow;

	if ( g_weatherState.anchor.type != type && !instant ) {
		a.intensity0 = 0.0f;	// a different kind of weather starts from nothing
	}
	if ( instant ) {
		// a race that starts in the rain starts on a wet track
		if ( type == WEATHER_RAIN ) {
			a.wetness0 = 0.35f + 0.65f * target;
			a.snow0 = 0.0f;
		} else if ( type == WEATHER_SNOW ) {
			a.snow0 = target > 0.0f ? 1.0f : 0.0f;
			a.wetness0 = 0.0f;
		} else {
			a.wetness0 = a.snow0 = 0.0f;
		}
	}

	BG_WeatherAnchorString( &a, buf, sizeof( buf ) );
	BG_WeatherParseAnchor( buf, &a );
	BG_WeatherSetAnchor( &g_weatherState, &a );
	BG_WeatherAdvance( &g_weatherState, level.time );
	trap_SetConfigstring( CS_WEATHER, buf );
}

static int G_WeatherRandomTime( int lo, int hi ) {
	return lo + (int)( random() * ( hi - lo ) );
}

// when the next shower comes after a dry spell of up to fraction of the usual length
static int G_WeatherDrySpellEnd( float fraction ) {
	int t;

	if ( gw.chance <= 0 ) {
		return 0x7fffffff;		// never
	}
	t = G_WeatherRandomTime( gw.cycleMin, gw.cycleMax ) * ( 100 - gw.chance ) / gw.chance;
	return level.time + (int)( t * fraction ) * 1000;
}

static int G_WeatherPickType( void ) {
	if ( gw.rain && gw.snow ) {
		return random() < 0.5f ? WEATHER_RAIN : WEATHER_SNOW;
	}
	return gw.snow ? WEATHER_SNOW : WEATHER_RAIN;
}

static void G_WeatherStartShower( qboolean instant ) {
	int ramp = instant ? 0 : G_WeatherRandomTime( WEATHER_RAMP_MIN, WEATHER_RAMP_MAX );
	float strength = gw.strengthMin + random() * ( gw.strengthMax - gw.strengthMin );

	gw.type = G_WeatherPickType();
	G_WeatherPublish( gw.type, ramp, strength, instant );
	gw.phase = WPHASE_SHOWER;
	gw.nextChange = level.time + ( ramp + G_WeatherRandomTime( gw.cycleMin, gw.cycleMax ) ) * 1000;
}

/*
=================
G_WeatherStart

Sets the weather for the current mode, from scratch (instant) at the start
of a level or with a short change over when g_weather is changed.
=================
*/
static void G_WeatherStart( qboolean instant ) {
	int ramp = instant ? 0 : WEATHER_SWITCH_RAMP;

	gw.mode = g_weather.integer;
	if ( g_gametype.integer == GT_GHOST ) {
		gw.mode = 0;			// Ghost Race laps must be comparable
	}

	switch ( gw.mode ) {
	case 2:
		G_WeatherPublish( WEATHER_RAIN, ramp, 1.0f, instant );
		gw.phase = WPHASE_SHOWER;
		gw.nextChange = 0x7fffffff;
		return;
	case 3:
		G_WeatherPublish( WEATHER_SNOW, ramp, 1.0f, instant );
		gw.phase = WPHASE_SHOWER;
		gw.nextChange = 0x7fffffff;
		return;
	case 1:
		if ( gw.rain || gw.snow ) {
			break;
		}
		// the map has no weather
		// fall through
	default:
		G_WeatherPublish( WEATHER_NONE, ramp, 0.0f, instant );
		gw.phase = WPHASE_DRY;
		gw.nextChange = 0x7fffffff;
		return;
	}

	if ( instant && random() * 100.0f < gw.chance ) {
		G_WeatherStartShower( qtrue );
	} else {
		G_WeatherPublish( gw.type, ramp, 0.0f, instant );
		gw.phase = WPHASE_DRY;
		// the first dry spell is cut short at random, so maps do not all
		// stay dry for the same time after loading
		gw.nextChange = G_WeatherDrySpellEnd( random() );
	}
}

void G_InitWeather( void ) {
	if ( !gw.mapKey ) {
		gw.rain = gw.legacyRain;
		gw.snow = gw.legacySnow;
	}
	gw.type = gw.snow && !gw.rain ? WEATHER_SNOW : WEATHER_RAIN;

	G_WeatherStart( qtrue );

	G_Printf( "weather: map allows%s%s%s, chance %i%%, g_weather %i\n",
		gw.rain ? " rain" : "", gw.snow ? " snow" : "", ( gw.rain || gw.snow ) ? "" : " none",
		gw.chance, gw.mode );
}

/*
=================
G_RunWeather

Every server frame: the showers come and go.
=================
*/
void G_RunWeather( void ) {
	int ramp;

	if ( g_weather.integer != gw.mode && g_gametype.integer != GT_GHOST ) {
		G_WeatherStart( qfalse );
	}

	if ( level.time >= gw.nextChange ) {
		switch ( gw.phase ) {
		case WPHASE_DRY:
			G_WeatherStartShower( qfalse );
			break;
		case WPHASE_SHOWER:
			ramp = G_WeatherRandomTime( WEATHER_RAMP_MIN, WEATHER_RAMP_MAX );
			G_WeatherPublish( gw.type, ramp, 0.0f, qfalse );
			gw.phase = WPHASE_PASSING;
			gw.nextChange = level.time + ramp * 1000;
			break;
		case WPHASE_PASSING:
			gw.phase = WPHASE_DRY;
			gw.nextChange = G_WeatherDrySpellEnd( 1.0f );
			break;
		}
	}

	BG_WeatherAdvance( &g_weatherState, level.time );
}

float G_WeatherPointWet( const vec3_t point ) {
	return BG_WeatherWetness( &g_weatherState, &g_weatherGrid, point );
}

float G_WeatherPointSnow( const vec3_t point ) {
	return BG_WeatherSnow( &g_weatherState, &g_weatherGrid, point );
}
