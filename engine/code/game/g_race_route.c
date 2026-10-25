/* Shared racing route adapter and geometric guidance. GPL-2.0-or-later. */
#include "g_local.h"

static botRaceRoute_t s_mapRoute;
static qboolean s_mapRouteBuilt;
static qboolean s_bezierRouteBuilt;
static qboolean s_selectionLogged;

// Conservative longitudinal deceleration estimate for braking-distance planning.
#define RACE_ROUTE_BRAKE_ACCEL 500.0f

static float RaceClamp( float value, float low, float high ) {
	return value < low ? low : ( value > high ? high : value );
}

void G_RaceRoute_Reset( void ) {
	Com_Memset( &s_mapRoute, 0, sizeof( s_mapRoute ) );
	s_mapRouteBuilt = qfalse;
	s_bezierRouteBuilt = qfalse;
	s_selectionLogged = qfalse;
}

// Use bounds only to cull candidates; confirm the sweep against the actual trigger brush.
static qboolean RaceCrossesGate( const vec3_t a, const vec3_t b,
	const gentity_t *gate, float minimumFraction, float *fraction ) {
	vec3_t edge, localMins, localMaxs, testMins, testMaxs, point;
	float enter = minimumFraction, leave = 1.0f;
	float length;
	int steps, sample;
	int axis;
	if ( !gate || !gate->inuse ) return qfalse;
	VectorSubtract( b, a, edge );
	for ( axis = 0; axis < 3; axis++ ) {
		float extent = ( axis == 2 ) ? CAR_HEIGHT * 0.5f : CAR_LENGTH * 0.5f;
		float delta = edge[axis];
		float low = gate->r.absmin[axis] - extent;
		float high = gate->r.absmax[axis] + extent;
		float first, last;
		if ( fabs( delta ) < 0.001f ) {
			if ( a[axis] < low || a[axis] > high ) return qfalse;
			continue;
		}
		first = ( low - a[axis] ) / delta;
		last = ( high - a[axis] ) / delta;
		if ( first > last ) { float swap = first; first = last; last = swap; }
		if ( first > enter ) enter = first;
		if ( last < leave ) leave = last;
		if ( enter > leave ) return qfalse;
	}

	// A checkpoint's AABB can be very loose for angled brushes. Match the game
	// trigger check using the car bounds against the actual brush model.
	VectorSet( localMins, -CAR_LENGTH * 0.5f, -CAR_LENGTH * 0.5f, -CAR_HEIGHT * 0.5f );
	VectorSet( localMaxs, CAR_LENGTH * 0.5f, CAR_LENGTH * 0.5f, CAR_HEIGHT * 0.5f );
	length = Distance( a, b );
	steps = (int)( length / 24.0f ) + 1;
	if ( steps < 1 ) steps = 1;
	for ( sample = 0; sample <= steps; sample++ ) {
		float testFraction = enter + ( 1.0f - enter ) * ( (float)sample / steps );
		VectorMA( a, testFraction, edge, point );
		VectorAdd( point, localMins, testMins );
		VectorAdd( point, localMaxs, testMaxs );
		if ( trap_EntityContact( testMins, testMaxs, gate ) ) {
			*fraction = testFraction;
			return qtrue;
		}
	}
	return qfalse;
}

qboolean G_RaceRoute_ValidateGhost( ghostBotRoute_t *route ) {
    int gateIndex, segment = 0, labelStart = 0, i;
	float fraction = 0.0f;
	if ( !route || !route->valid || route->numWaypoints < 2 ) return qfalse;
	// Gate order comes from the active length/direction, not the file's label.
	for ( gateIndex = 0; gateIndex < level.numCheckpoints; gateIndex++ ) {
		qboolean found = qfalse;
		for ( ; segment < route->numWaypoints - 1; segment++, fraction = 0.0f ) {
			if ( RaceCrossesGate( route->waypoints[segment].origin,
				route->waypoints[segment + 1].origin, level.checkpoints[gateIndex], fraction, &fraction ) ) {
				found = qtrue;
				break;
			}
		}
		if ( !found ) return qfalse;
		for ( i = labelStart; i <= segment; i++ ) route->segments[i].nextCheckpoint = gateIndex + 1;
		labelStart = segment + 1;
	}
	for ( i = labelStart; i < route->numWaypoints - 1; i++ )
		route->segments[i].nextCheckpoint = level.numCheckpoints + ( level.sprintTrack ? 1 : 0 );
	// Sprint has a separate finish beyond the last ordinary checkpoint.
	if ( level.sprintTrack && level.finishEnt ) {
		for ( ; segment < route->numWaypoints - 1; segment++, fraction = 0.0f ) {
			if ( RaceCrossesGate( route->waypoints[segment].origin,
				route->waypoints[segment + 1].origin, level.finishEnt, fraction, &fraction ) ) return qtrue;
		}
		return qfalse;
	}
	return qtrue;
}

static qboolean RaceImportMap( const botPathRoute_t *path ) {
	int i, edgeCount, time = 0, startNode = 0, nodeCount;
	float width = 0.0f;
	if ( !path || !path->valid || path->numNodes < 2 ) return qfalse;
	Com_Memset( &s_mapRoute, 0, sizeof( s_mapRoute ) );
	s_mapRoute.closed = !level.sprintTrack;
	s_mapRoute.fromMapPath = qtrue;
	Q_strncpyz( s_mapRoute.path, path->name, sizeof( s_mapRoute.path ) );
	nodeCount = path->numNodes;
	if ( s_mapRoute.closed && Distance( path->nodes[0].origin, path->nodes[nodeCount - 1].origin ) <= 0.5f ) nodeCount--;
	if ( nodeCount < 2 ) return qfalse;
	// Authored loops can start anywhere. Normalize their seam to just after finish.
	if ( s_mapRoute.closed && level.finishEnt ) {
		for ( i = 0; i < nodeCount; i++ ) {
			float fraction;
			if ( RaceCrossesGate( path->nodes[i].origin, path->nodes[( i + 1 ) % nodeCount].origin,
				level.finishEnt, 0.0f, &fraction ) ) { startNode = ( i + 1 ) % nodeCount; break; }
		}
	}
	edgeCount = s_mapRoute.closed ? nodeCount : nodeCount - 1;
	for ( i = 0; i < edgeCount; i++ ) {
		int j, from = ( startNode + i ) % nodeCount, end = ( from + 1 ) % nodeCount;
		float length = Distance( path->nodes[from].origin, path->nodes[end].origin );
		int steps = (int)( length / 96.0f ) + 1;
		float speed = from < path->numSegments ? path->segments[from].recommendedSpeed : path->segments[0].recommendedSpeed;
		speed = RaceClamp( speed, 150.0f, 2600.0f );
		if ( s_mapRoute.numWaypoints + steps + 1 > MAX_GHOST_BOT_WAYPOINTS ) return qfalse;
		if ( width <= 0.0f || path->nodes[from].effectiveWidth < width ) width = path->nodes[from].effectiveWidth;
		for ( j = 0; j < steps; j++ ) {
			ghostWaypoint_t *wp = &s_mapRoute.waypoints[s_mapRoute.numWaypoints];
			ghostRouteSegment_t *seg = &s_mapRoute.segments[s_mapRoute.numWaypoints++];
			int family, axis;
			for ( axis = 0; axis < 3; axis++ ) wp->origin[axis] = path->nodes[from].origin[axis] +
				( path->nodes[end].origin[axis] - path->nodes[from].origin[axis] ) * j / steps;
			wp->timeOffset = time;
			time += (int)( 1000.0f * length / ( speed * steps ) ) + 1;
			seg->recommendedSpeed = speed;
			for ( family = 0; family < GHOST_LINE_FAMILY_COUNT; family++ ) seg->lines[family].speedScale = 1.0f;
		}
	}
	VectorCopy( path->nodes[s_mapRoute.closed ? startNode : path->numNodes - 1].origin,
		s_mapRoute.waypoints[s_mapRoute.numWaypoints].origin );
	s_mapRoute.waypoints[s_mapRoute.numWaypoints++].timeOffset = time;
	s_mapRoute.numSegments = s_mapRoute.numWaypoints - 1;
	s_mapRoute.corridorWidth = width;
	s_mapRoute.valid = qtrue;
	if ( !G_RaceRoute_ValidateGhost( &s_mapRoute ) ) {
		s_mapRoute.valid = qfalse;
		return qfalse;
	}
	return qtrue;
}

static qboolean RaceImportBezier( void ) {
	int checkpointCount = level.numCheckpoints;
	int segmentIndex, sampleCount = 0, time = 0;
	int handleCount = 0;
	qboolean hasHandles = qfalse;
	if ( level.sprintTrack || checkpointCount < 3 || checkpointCount > MAX_GHOST_BOT_WAYPOINTS / 8 ) {
		G_Printf( "G_RaceRoute: Bezier unavailable (sprint=%d checkpoints=%d)\n", level.sprintTrack, checkpointCount );
		return qfalse;
	}
	for ( segmentIndex = 0; segmentIndex < checkpointCount; segmentIndex++ ) {
		if ( !level.checkpoints[segmentIndex] || !level.checkpoints[segmentIndex]->inuse ) {
			G_Printf( "G_RaceRoute: Bezier unavailable (checkpoint slot %d missing)\n", segmentIndex + 1 );
			return qfalse;
		}
		if ( VectorLengthSquared( level.checkpoints[segmentIndex]->s.angles2 ) > 0.01f ) {
			hasHandles = qtrue;
			handleCount++;
		}
	}
	if ( !hasHandles ) {
		G_Printf( "G_RaceRoute: Bezier unavailable (no loaded checkpoint handles)\n" );
		return qfalse;
	}
	G_Printf( "G_RaceRoute: importing Bezier handles=%d/%d\n", handleCount, checkpointCount );

	Com_Memset( &s_mapRoute, 0, sizeof( s_mapRoute ) );
	s_mapRoute.closed = qtrue;
	s_mapRoute.fromBezierPath = qtrue;
	Q_strncpyz( s_mapRoute.path, "checkpoint_bezier", sizeof( s_mapRoute.path ) );
	for ( segmentIndex = 0; segmentIndex < checkpointCount; segmentIndex++ ) {
		int startIndex = ( checkpointCount - 1 + segmentIndex ) % checkpointCount;
		int endIndex = ( startIndex + 1 ) % checkpointCount;
		gentity_t *start = level.checkpoints[startIndex];
		gentity_t *end = level.checkpoints[endIndex];
		vec3_t previous = { 0.0f, 0.0f, 0.0f }, point, nextPoint;
		float approximateLength = 0.0f;
		int step, steps;
		for ( step = 0; step <= 12; step++ ) {
			G_GetPointOnCurveBetweenCheckpoints( start, end, step / 12.0f, point );
			if ( step > 0 ) {
				VectorSubtract( point, previous, nextPoint );
				approximateLength += VectorLength( nextPoint );
			}
			VectorCopy( point, previous );
		}
		steps = (int)( approximateLength / 72.0f ) + 1;
		if ( steps < 8 ) steps = 8;
		if ( steps > 128 ) steps = 128;
		if ( sampleCount + steps + 1 > MAX_GHOST_BOT_WAYPOINTS ) return qfalse;
		for ( step = 0; step < steps; step++ ) {
			float fraction = (float)step / steps;
			float midpoint = ( (float)step + 0.5f ) / steps;
			vec3_t alpha, delta, cross, chord;
			float chordLength, derivativeLength, curvature, recommendedSpeed;
			float gravity = CP_CURRENT_GRAVITY > 1.0f ? CP_CURRENT_GRAVITY : 800.0f;
			int family;
			ghostWaypoint_t *waypoint = &s_mapRoute.waypoints[sampleCount];
			ghostRouteSegment_t *segment = &s_mapRoute.segments[sampleCount];

			G_GetPointOnCurveBetweenCheckpoints( start, end, fraction, waypoint->origin );
			waypoint->timeOffset = time;
			waypoint->required = ( sampleCount == 0 ) ? qtrue : qfalse;
			G_GetPointOnCurveBetweenCheckpoints( start, end, (float)( step + 1 ) / steps, nextPoint );
			VectorSubtract( nextPoint, waypoint->origin, chord );
			chordLength = VectorLength( chord );
			G_GetDervOnCurveBetweenCheckpoints( start, end, midpoint, delta );
			G_Get2ndDervOnCurveBetweenCheckpoints( start, end, midpoint, alpha );
			CrossProduct( delta, alpha, cross );
			derivativeLength = VectorLength( delta );
			curvature = derivativeLength > 0.001f ? VectorLength( cross ) /
				( derivativeLength * derivativeLength * derivativeLength ) : 0.0f;
			recommendedSpeed = curvature > 0.000001f ? sqrt( gravity / curvature ) : 2600.0f;
			segment->recommendedSpeed = RaceClamp( recommendedSpeed, 300.0f, 2600.0f );
			segment->curvature = curvature;
			for ( family = 0; family < GHOST_LINE_FAMILY_COUNT; family++ ) segment->lines[family].speedScale = 1.0f;
			time += (int)( 1000.0f * chordLength / segment->recommendedSpeed ) + 1;
			sampleCount++;
		}
	}
	VectorCopy( s_mapRoute.waypoints[0].origin, s_mapRoute.waypoints[sampleCount].origin );
	s_mapRoute.waypoints[sampleCount].timeOffset = time;
	s_mapRoute.waypoints[sampleCount].required = qtrue;
	s_mapRoute.numWaypoints = sampleCount + 1;
	s_mapRoute.numSegments = sampleCount;
	s_mapRoute.valid = qtrue;
	if ( !G_RaceRoute_ValidateGhost( &s_mapRoute ) ) {
		s_mapRoute.valid = qfalse;
		G_Printf( "G_RaceRoute: Bezier rejected (route misses checkpoint order)\n" );
		return qfalse;
	}
	G_Printf( "G_RaceRoute: Bezier ready waypoints=%d\n", s_mapRoute.numWaypoints );
	return qtrue;
}

qboolean G_RaceRoute_Get( const botRaceRoute_t **outRoute ) {
	const ghostBotRoute_t *ghost = NULL;
	int i;
	if ( !outRoute ) return qfalse;
	*outRoute = NULL;
	if ( G_Ghost_GetBotRoute( &ghost ) && ghost ) {
		*outRoute = ghost;
	} else {
		if ( !s_mapRouteBuilt ) {
			s_mapRouteBuilt = qtrue;
			// Registration is dense. A missing path_0 does not turn path_2 into an aggressive line.
			for ( i = 0; i < G_BotPath_GetRouteCount(); i++ ) {
				if ( RaceImportMap( G_BotPath_GetRouteByIndex( i ) ) ) break;
			}
		}
		if ( !s_mapRoute.valid && !s_bezierRouteBuilt ) {
			s_bezierRouteBuilt = qtrue;
			RaceImportBezier();
		}
		if ( s_mapRoute.valid ) *outRoute = &s_mapRoute;
	}
	if ( !s_selectionLogged ) {
		G_Printf( "G_RaceRoute: source=%s route=%s\n", *outRoute ?
			( ( *outRoute )->fromBezierPath ? "bezier" : ( ( *outRoute )->fromMapPath ? "map" : "ghost" ) ) : "checkpoint",
			*outRoute ? ( *outRoute )->path : "fallback" );
		s_selectionLogged = qtrue;
	}
	return *outRoute != NULL;
}

qboolean G_RaceRoute_Guidance( const botRaceRoute_t *route, const vec3_t origin,
	int hint, qboolean allowWrap, int nextCheckpoint, float actualSpeed, botRaceGuidance_t *out ) {
	int i, best = -1, count;
	float bestDistance = 1e30f, bestFraction = 0.0f;
	float lookAhead, travelled, preview, limit;
	qboolean targetSet = qfalse;
	if ( !route || !route->valid || !out || route->numSegments < 1 ) return qfalse;
	count = route->numSegments;
	if ( hint >= 0 && hint < count && nextCheckpoint > 0 &&
		route->segments[hint].nextCheckpoint > 0 && route->segments[hint].nextCheckpoint != nextCheckpoint ) hint = -1;
	Com_Memset( out, 0, sizeof( *out ) );
	for ( i = 0; i < count; i++ ) {
		vec3_t edge, delta, closest;
		float lengthSq, fraction, distance;
		if ( nextCheckpoint > 0 && route->segments[i].nextCheckpoint > 0 &&
			route->segments[i].nextCheckpoint != nextCheckpoint ) continue;
		if ( hint >= 0 && hint < count && ( i < hint - 3 || i > hint + 48 ) &&
			!( route->closed && allowWrap && hint >= count - 48 && i < 48 ) ) continue;
		VectorSubtract( route->waypoints[i + 1].origin, route->waypoints[i].origin, edge );
		lengthSq = VectorLengthSquared( edge );
		if ( lengthSq < 0.01f ) continue;
		VectorSubtract( origin, route->waypoints[i].origin, delta );
		fraction = RaceClamp( DotProduct( delta, edge ) / lengthSq, 0.0f, 1.0f );
		VectorMA( route->waypoints[i].origin, fraction, edge, closest );
		VectorSubtract( origin, closest, delta );
		distance = VectorLengthSquared( delta );
		if ( distance < bestDistance ) { best = i; bestDistance = distance; bestFraction = fraction; }
	}
	if ( best < 0 ) return qfalse;
	out->segment = best;
	out->distance = sqrt( bestDistance );
	VectorSubtract( route->waypoints[best + 1].origin, route->waypoints[best].origin, out->tangent );
	VectorNormalize( out->tangent );
	lookAhead = RaceClamp( 180.0f + actualSpeed * 0.25f, 180.0f, 600.0f );
	preview = RaceClamp( actualSpeed * actualSpeed / ( 2.0f * RACE_ROUTE_BRAKE_ACCEL ) + lookAhead,
		lookAhead, 4096.0f );
	travelled = 0.0f;
	limit = 2600.0f;
	for ( i = 0; i < count && travelled <= preview; i++ ) {
		int segment = ( best + i ) % count;
		int next = ( segment + 1 ) % count;
		vec3_t edge, nextEdge;
		float length, remaining, speed, dot = 1.0f;
		if ( best + i >= count && !route->closed ) break;
		VectorSubtract( route->waypoints[segment + 1].origin, route->waypoints[segment].origin, edge );
		length = VectorNormalize( edge );
		remaining = length * ( i == 0 ? 1.0f - bestFraction : 1.0f );
		speed = RaceClamp( route->segments[segment].recommendedSpeed, 150.0f, 2600.0f );
		// Incoming corner limit, applied with braking distance rather than an average.
		if ( segment + 1 < count || route->closed ) {
			float nextLength, cornerSpeed;
			VectorSubtract( route->waypoints[next + 1].origin, route->waypoints[next].origin, nextEdge );
			nextLength = VectorNormalize( nextEdge );
			dot = RaceClamp( DotProduct( edge, nextEdge ), -1.0f, 1.0f );
			cornerSpeed = sqrt( 600.0f * ( length + nextLength ) * 0.5f / ( 0.01f + sqrt( 2.0f * ( 1.0f - dot ) ) ) );
			if ( cornerSpeed < speed ) speed = cornerSpeed;
		}
		if ( sqrt( speed * speed + 2.0f * RACE_ROUTE_BRAKE_ACCEL * travelled ) < limit )
			limit = sqrt( speed * speed + 2.0f * RACE_ROUTE_BRAKE_ACCEL * travelled );
		if ( i == 0 ) out->curvature = 1.0f - dot;
		if ( !targetSet ) {
			float offset = RaceClamp( lookAhead - travelled, 0.0f, remaining );
			VectorMA( route->waypoints[segment].origin, ( i == 0 ? bestFraction * length : 0.0f ) + offset, edge, out->target );
			out->targetIndex = segment + 1;
			if ( travelled + remaining >= lookAhead ) targetSet = qtrue;
			// Hold the next gate as the target until the game confirms its crossing.
			if ( nextCheckpoint > 0 && route->segments[next].nextCheckpoint > 0 &&
				route->segments[next].nextCheckpoint != nextCheckpoint ) targetSet = qtrue;
		}
		travelled += remaining;
	}
	out->targetSpeed = limit;
	return qtrue;
}
