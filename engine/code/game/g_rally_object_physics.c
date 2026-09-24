/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
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

#include "g_local.h"

#define RALLY_OBJECT_CONTACT_EPSILON 0.125f

static qboolean G_RallyObject_InverseWorldInertia( gentity_t *self,
		vec3_t axis[3], float inverseWorld[3][3] ) {
	float size[3];
	float inverseBody[3];
	float rotationScaled[3][3];
	float rotationTranspose[3][3];
	int i;

	if ( !self || self->mass <= 0 )
		return qfalse;

	for ( i = 0; i < 3; i++ ) {
		size[i] = self->r.maxs[i] - self->r.mins[i];
		if ( size[i] <= 0.01f )
			return qfalse;
	}

	/* Solid cuboid inertia about its centre: Ixx = m/12 * (y^2 + z^2). */
	inverseBody[0] = 12.0f / ( self->mass * ( size[1] * size[1] + size[2] * size[2] ) );
	inverseBody[1] = 12.0f / ( self->mass * ( size[0] * size[0] + size[2] * size[2] ) );
	inverseBody[2] = 12.0f / ( self->mass * ( size[0] * size[0] + size[1] * size[1] ) );

	AnglesToOrientation( self->s.apos.trBase, axis );
	for ( i = 0; i < 3; i++ ) {
		rotationScaled[0][i] = inverseBody[i] * axis[0][i];
		rotationScaled[1][i] = inverseBody[i] * axis[1][i];
		rotationScaled[2][i] = inverseBody[i] * axis[2][i];
	}
	MatrixTranspose( axis, rotationTranspose );
	MatrixMultiply( rotationScaled, rotationTranspose, inverseWorld );
	return qtrue;
}

void G_RallyObject_ApplyForce( gentity_t *self, vec3_t force, vec3_t at ) {
	vec3_t arm;
	vec3_t moment;

	if ( !self )
		return;

	VectorSubtract( at, self->s.pos.trBase, arm );
	VectorAdd( self->netForce, force, self->netForce );
	CrossProduct( arm, force, moment );
	VectorAdd( self->netMoment, moment, self->netMoment );
}

qboolean G_RallyObject_ApplyCollision( gentity_t *self, vec3_t at, vec3_t normal, float elasticity ) {
	vec3_t arm;
	vec3_t contactVelocity;
	vec3_t impulse;
	vec3_t impulseMoment;
	vec3_t cross;
	vec3_t inverseAxis[3];
	float inverseWorld[3][3];
	float denominator;
	float normalSpeed;
	float impulseMagnitude;

	if ( !self || self->mass <= 0 )
		return qfalse;

	VectorSubtract( at, self->s.pos.trBase, arm );
	CrossProduct( self->s.apos.trDelta, arm, cross );
	VectorAdd( self->s.pos.trDelta, cross, contactVelocity );
	normalSpeed = DotProduct( normal, contactVelocity );
	if ( normalSpeed >= -0.01f )
		return qfalse;

	/* Degenerate bounds still get a safe translational response. */
	denominator = 1.0f / self->mass;
	if ( G_RallyObject_InverseWorldInertia( self, inverseAxis, inverseWorld ) ) {
		CrossProduct( arm, normal, cross );
		VectorRotate( cross, inverseWorld, impulseMoment );
		CrossProduct( impulseMoment, arm, cross );
		denominator += DotProduct( cross, normal );
	}

	if ( denominator <= 0.000001f )
		return qfalse;

	elasticity = Com_Clamp( 0.0f, 1.0f, elasticity );
	if ( normalSpeed > -8.0f )
		elasticity = 0.0f; /* suppress low-speed resting-contact jitter */
	impulseMagnitude = -( 1.0f + elasticity ) * normalSpeed / denominator;
	VectorScale( normal, impulseMagnitude, impulse );
	VectorMA( self->s.pos.trDelta, 1.0f / self->mass, impulse, self->s.pos.trDelta );
	CrossProduct( arm, impulse, impulseMoment );
	VectorAdd( self->angularMomentum, impulseMoment, self->angularMomentum );

	return qtrue;
}

static void G_RallyObject_ApplyTangentialFriction( gentity_t *self, vec3_t normal,
		float incomingNormalSpeed, float friction ) {
	vec3_t tangentVelocity;
	float normalSpeed;
	float tangentSpeed;
	float frictionDrop;
	float remainingTangentSpeed;

	normalSpeed = DotProduct( self->s.pos.trDelta, normal );
	VectorMA( self->s.pos.trDelta, -normalSpeed, normal, tangentVelocity );
	tangentSpeed = VectorNormalize( tangentVelocity );
	if ( tangentSpeed <= 0.001f )
		return;

	frictionDrop = Com_Clamp( 0.0f, tangentSpeed, friction * Q_fabs( incomingNormalSpeed ) );
	remainingTangentSpeed = tangentSpeed - frictionDrop;
	VectorScale( tangentVelocity, remainingTangentSpeed, tangentVelocity );
	VectorMA( tangentVelocity, normalSpeed, normal, self->s.pos.trDelta );
}

static void G_RallyObject_ResolvePairCollision( gentity_t *self, gentity_t *other,
		vec3_t normal, vec3_t contact ) {
	vec3_t relativeVelocity;
	vec3_t tangentVelocity;
	vec3_t normalImpulse;
	vec3_t frictionImpulseVector;
	vec3_t totalImpulse;
	vec3_t arm;
	float normalSpeed;
	float denominator;
	float impulseMagnitude;
	float tangentSpeed;
	float frictionImpulse;
	float elasticity;
	float friction;

	if ( !other || !other->moveable || other->s.eType != ET_SCRIPTED || other->mass <= 0 )
		return;

	VectorSubtract( self->s.pos.trDelta, other->s.pos.trDelta, relativeVelocity );
	normalSpeed = DotProduct( relativeVelocity, normal );
	if ( normalSpeed >= -0.01f )
		return;

	denominator = ( 1.0f / self->mass ) + ( 1.0f / other->mass );
	if ( denominator <= 0.000001f )
		return;

	elasticity = Com_Clamp( 0.0f, 1.0f, ( self->elasticity + other->elasticity ) * 0.5f );
	if ( normalSpeed > -8.0f )
		elasticity = 0.0f;
	impulseMagnitude = -( 1.0f + elasticity ) * normalSpeed / denominator;
	VectorScale( normal, impulseMagnitude, normalImpulse );
	VectorCopy( normalImpulse, totalImpulse );
	VectorMA( self->s.pos.trDelta, impulseMagnitude / self->mass, normal, self->s.pos.trDelta );
	VectorMA( other->s.pos.trDelta, -impulseMagnitude / other->mass, normal, other->s.pos.trDelta );

	/* A bounded Coulomb-style tangential impulse prevents friction from
	 * reversing the relative slide velocity. */
	VectorSubtract( self->s.pos.trDelta, other->s.pos.trDelta, relativeVelocity );
	VectorMA( relativeVelocity, -DotProduct( relativeVelocity, normal ), normal, tangentVelocity );
	tangentSpeed = VectorNormalize( tangentVelocity );
	friction = Com_Clamp( 0.0f, 4.0f, ( self->friction + other->friction ) * 0.5f );
	frictionImpulse = Com_Clamp( 0.0f, tangentSpeed / denominator, friction * impulseMagnitude );
	if ( tangentSpeed > 0.001f && frictionImpulse > 0.0f ) {
		VectorScale( tangentVelocity, frictionImpulse, frictionImpulseVector );
		VectorMA( self->s.pos.trDelta, -1.0f / self->mass, frictionImpulseVector, self->s.pos.trDelta );
		VectorMA( other->s.pos.trDelta, 1.0f / other->mass, frictionImpulseVector, other->s.pos.trDelta );
		VectorSubtract( totalImpulse, frictionImpulseVector, totalImpulse );
	}

	/* Transfer contact torque as well as linear momentum. */
	VectorSubtract( contact, self->s.pos.trBase, arm );
	CrossProduct( arm, totalImpulse, relativeVelocity );
	VectorAdd( self->angularMomentum, relativeVelocity, self->angularMomentum );
	VectorSubtract( contact, other->s.pos.trBase, arm );
	CrossProduct( arm, totalImpulse, relativeVelocity );
	VectorMA( other->angularMomentum, -1.0f, relativeVelocity, other->angularMomentum );
}

void G_RallyObject_TracePhysics( gentity_t *self, float time ) {
	trace_t trace;
	vec3_t start;
	vec3_t end;
	vec3_t normal;
	vec3_t obstacleVelocity;
	float incomingNormalSpeed;
	gentity_t *hit;

	if ( !self || !self->moveable || time <= 0.0f )
		return;

	VectorCopy( self->s.pos.trBase, start );
	VectorMA( start, time, self->s.pos.trDelta, end );
	trap_Trace( &trace, start, self->r.mins, self->r.maxs, end,
		self->s.number, MASK_PLAYERSOLID );

	if ( trace.fraction >= 1.0f && !trace.startsolid && !trace.allsolid ) {
		VectorCopy( end, self->s.pos.trBase );
		self->s.pos.trTime = level.time;
		return;
	}

	VectorCopy( trace.plane.normal, normal );
	if ( VectorNormalize( normal ) == 0.0f ) {
		/* A start-solid recovery with no usable plane should not teleport the
		 * prop. Its velocity is stopped until a valid contact can be found. */
		VectorClear( self->s.pos.trDelta );
		return;
	}

	VectorMA( trace.endpos, RALLY_OBJECT_CONTACT_EPSILON, normal, self->s.pos.trBase );
	self->s.pos.trTime = level.time;
	hit = ( trace.entityNum >= 0 && trace.entityNum < ENTITYNUM_MAX_NORMAL )
		? &g_entities[trace.entityNum] : NULL;
	if ( hit && ( hit->flags & FL_EXTRA_BBOX ) &&
		hit->r.ownerNum >= 0 && hit->r.ownerNum < level.num_entities ) {
		hit = &g_entities[hit->r.ownerNum];
	}

	if ( hit && hit->inuse && hit != self && hit->s.eType == ET_SCRIPTED &&
		hit->moveable && !( hit->s.eFlags & EF_DEAD ) ) {
		G_RallyObject_ResolvePairCollision( self, hit, normal, trace.endpos );
		return;
	}

	VectorClear( obstacleVelocity );
	if ( hit && hit->client )
		VectorCopy( hit->s.pos.trDelta, obstacleVelocity );
	VectorSubtract( self->s.pos.trDelta, obstacleVelocity, self->s.pos.trDelta );
	incomingNormalSpeed = DotProduct( self->s.pos.trDelta, normal );
	G_RallyObject_ApplyCollision( self, trace.endpos, normal, self->elasticity );
	if ( DotProduct( self->s.pos.trDelta, normal ) < 0.0f ) {
		/* Always remove inward normal motion, even below the impulse threshold. */
		VectorMA( self->s.pos.trDelta, -DotProduct( self->s.pos.trDelta, normal ), normal, self->s.pos.trDelta );
	}
	G_RallyObject_ApplyTangentialFriction( self, normal, incomingNormalSpeed, self->friction );
	VectorAdd( self->s.pos.trDelta, obstacleVelocity, self->s.pos.trDelta );
}

void G_RallyObject_IntegratePhysics( gentity_t *self, float time ) {
	vec3_t axis[3];
	float inverseWorld[3][3];
	float rotationStep[3][3];
	float rotationResult[3][3];
	int i;

	if ( !self || !self->moveable || self->mass <= 0 || time <= 0.0f )
		return;

	VectorMA( self->s.pos.trDelta, time / self->mass, self->netForce, self->s.pos.trDelta );

	if ( VectorLengthSquared( self->netMoment ) > 0.000001f ||
		VectorLengthSquared( self->angularMomentum ) > 0.000001f ) {
		if ( G_RallyObject_InverseWorldInertia( self, axis, inverseWorld ) ) {
			VectorMA( self->angularMomentum, time, self->netMoment, self->angularMomentum );
			VectorRotate( self->angularMomentum, inverseWorld, self->s.apos.trDelta );

			rotationStep[0][0] = 0.0f;
			rotationStep[0][1] = -time * self->s.apos.trDelta[2];
			rotationStep[0][2] =  time * self->s.apos.trDelta[1];
			rotationStep[1][0] =  time * self->s.apos.trDelta[2];
			rotationStep[1][1] = 0.0f;
			rotationStep[1][2] = -time * self->s.apos.trDelta[0];
			rotationStep[2][0] = -time * self->s.apos.trDelta[1];
			rotationStep[2][1] =  time * self->s.apos.trDelta[0];
			rotationStep[2][2] = 0.0f;
			MatrixMultiply( rotationStep, axis, rotationResult );
			MatrixAdd( axis, rotationResult, axis );
			OrthonormalizeOrientation( axis );
			OrientationToAngles( axis, self->s.apos.trBase );
		}
	}

	for ( i = 0; i < 3; i++ ) {
		if ( self->s.pos.trDelta[i] != 0.0f ) {
			VectorCopy( self->s.pos.trDelta, self->lastNonZeroVelocity );
			break;
		}
	}
}
