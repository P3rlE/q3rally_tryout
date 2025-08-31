/*
=============================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2025 Q3Rally Team (Per Thormann - q3rally@gmail.com)

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
=============================================================================
*/

#include "g_local.h"

/*
 * Basic rally ball entity implementation. The ball uses the
 * generic item physics to move and bounce through the world.
 */
static void G_RallyBall_Think( gentity_t *ent ) {
	ent->nextthink = level.time + FRAMETIME;
}

gentity_t *G_SpawnRallyBall( vec3_t origin ) {
	gentity_t *ball;

	    ball = G_Spawn();
        ball->classname = "rallyball";
        ball->s.eType = ET_RALLYBALL;

        ball->physicsObject = qtrue;
        ball->physicsBounce = 0.6f;
        ball->clipmask = MASK_SOLID;
        ball->r.contents = CONTENTS_BODY;
        ball->mass = ball_mass.integer;
        ball->s.modelindex = G_ModelIndex("models/rallyball/rallyball.md3");

        ball->s.pos.trType = TR_GRAVITY;
        ball->s.pos.trTime = level.time;
        VectorCopy( origin, ball->s.pos.trBase );
        VectorClear( ball->s.pos.trDelta );

	ball->think = G_RallyBall_Think;
	ball->nextthink = level.time + FRAMETIME;

	trap_LinkEntity( ball );
	return ball;
}
