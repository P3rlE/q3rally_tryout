/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

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

#include "g_local.h"

static botPathRoute_t g_botPathRoutes[MAX_BOT_PATH_ROUTES];
static int g_botPathRouteCount;

static int G_BotPath_ClampSearchStart( int hintIndex, int hintWindow, int nodeCount ) {
	int start = 0;

	if ( hintIndex >= 0 && hintIndex < nodeCount ) {
		start = hintIndex - hintWindow;
		if ( start < 0 ) {
			start = 0;
		}
	}

	return start;
}

static int G_BotPath_ClampSearchEnd( int hintIndex, int hintWindow, int nodeCount ) {
	int end = nodeCount - 1;

	if ( hintIndex >= 0 && hintIndex < nodeCount ) {
		end = hintIndex + hintWindow;
		if ( end >= nodeCount ) {
			end = nodeCount - 1;
		}
	}

	return end;
}

void G_BotPath_ClearAllRoutes( void ) {
	Com_Memset( g_botPathRoutes, 0, sizeof( g_botPathRoutes ) );
	g_botPathRouteCount = 0;
}

int G_BotPath_RegisterRoute( const char *name, const botPathNode_t *nodes, int numNodes ) {
	botPathRoute_t *route;

	if ( g_botPathRouteCount < 0 ) {
		g_botPathRouteCount = 0;
	}

	if ( !name || !name[0] || !nodes ) {
		return -1;
	}

	if ( numNodes <= 0 || numNodes > MAX_BOT_PATH_NODES ) {
		return -1;
	}

	if ( g_botPathRouteCount >= MAX_BOT_PATH_ROUTES ) {
		return -1;
	}

	route = &g_botPathRoutes[g_botPathRouteCount];
	Com_Memset( route, 0, sizeof( *route ) );
	Q_strncpyz( route->name, name, sizeof( route->name ) );
	route->numNodes = numNodes;
	route->valid = qtrue;
	Com_Memcpy( route->nodes, nodes, sizeof( route->nodes[0] ) * numNodes );

	g_botPathRouteCount++;
	return g_botPathRouteCount - 1;
}

int G_BotPath_GetRouteCount( void ) {
	if ( g_botPathRouteCount < 0 ) {
		return 0;
	}
	if ( g_botPathRouteCount > MAX_BOT_PATH_ROUTES ) {
		return MAX_BOT_PATH_ROUTES;
	}
	return g_botPathRouteCount;
}

const botPathRoute_t *G_BotPath_GetRouteByIndex( int routeIndex ) {
	int routeCount = G_BotPath_GetRouteCount();

	if ( routeIndex < 0 || routeIndex >= routeCount ) {
		return NULL;
	}

	if ( !g_botPathRoutes[routeIndex].valid ) {
		return NULL;
	}

	return &g_botPathRoutes[routeIndex];
}

const botPathRoute_t *G_BotPath_GetRouteByName( const char *name ) {
	int i;
	int routeCount = G_BotPath_GetRouteCount();

	if ( !name || !name[0] ) {
		return NULL;
	}

	for ( i = 0; i < routeCount; ++i ) {
		if ( g_botPathRoutes[i].valid && !Q_stricmp( g_botPathRoutes[i].name, name ) ) {
			return &g_botPathRoutes[i];
		}
	}

	return NULL;
}

int G_BotPath_SelectClosestNode( const botPathRoute_t *route, const vec3_t origin, int hintIndex, int hintWindow ) {
	int i;
	int searchStart = 0;
	int searchEnd;
	int bestIndex = -1;
	float bestDistSq = 0.0f;

	if ( !route || !route->valid || !origin ) {
		return -1;
	}

	if ( route->numNodes <= 0 || route->numNodes > MAX_BOT_PATH_NODES ) {
		return -1;
	}

	if ( hintWindow <= 0 ) {
		hintWindow = 24;
	}

	searchEnd = route->numNodes - 1;
	if ( hintIndex >= 0 && hintIndex < route->numNodes ) {
		searchStart = G_BotPath_ClampSearchStart( hintIndex, hintWindow, route->numNodes );
		searchEnd = G_BotPath_ClampSearchEnd( hintIndex, hintWindow, route->numNodes );
	}

	for ( i = searchStart; i <= searchEnd; ++i ) {
		vec3_t delta;
		float distSq;

		VectorSubtract( route->nodes[i].origin, origin, delta );
		distSq = VectorLengthSquared( delta );
		if ( bestIndex < 0 || distSq < bestDistSq ) {
			bestIndex = i;
			bestDistSq = distSq;
		}
	}

	return bestIndex;
}

int G_BotPath_SelectLookAheadNode( const botPathRoute_t *route, const vec3_t origin, int hintIndex, int hintWindow, int lookAheadNodes ) {
	int closestIndex;
	int lookAheadIndex;

	closestIndex = G_BotPath_SelectClosestNode( route, origin, hintIndex, hintWindow );
	if ( closestIndex < 0 ) {
		return -1;
	}

	if ( lookAheadNodes < 0 ) {
		lookAheadNodes = 0;
	}

	lookAheadIndex = closestIndex + lookAheadNodes;
	if ( lookAheadIndex >= route->numNodes ) {
		lookAheadIndex = route->numNodes - 1;
	}

	return lookAheadIndex;
}
