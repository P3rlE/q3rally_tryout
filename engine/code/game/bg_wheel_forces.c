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

#include "../qcommon/q_shared.h"
#ifdef GAME
#include "g_local.h"
#else
//#include "../cgame/cg_local.h"
#endif
#include "bg_public.h"
#include "bg_local.h"


static float CP_GEAR_RATIOS[] = {CP_GEAR1, CP_GEAR2, CP_GEAR3, CP_GEAR4, CP_GEAR5};

/*
===================
PM_WheelSpeedtoRPM
===================
*/
static float PM_WheelSpeedtoRPM( car_t *car, carPoint_t *points ){
	float	ratio, w;
	int		i;

	if (car->gear < 0)
		ratio = CP_GEARR;
	else if (car->gear == 0)
		ratio = CP_GEARN;
	else
		ratio = CP_GEAR_RATIOS[car->gear-1];

	w = 0;
	if (car->gear >= 0){
		for (i = 0; i < FIRST_FRAME_POINT; i++){
			w = min(w, points[i].w);
		}
	}
	else {
		for (i = 0; i < FIRST_FRAME_POINT; i++){
			w = max(w, points[i].w);
		}
	}

	return (-w / M_PI * 30) * (ratio * CP_AXLEGEAR);
}


/*
================================================================================
PM_UpdateRPM
================================================================================
*/
static void PM_UpdateRPM(car_t *car, carPoint_t *points){
	float	rpmTemp;
	float	shiftDownRPM, shiftUpRPM;

	shiftDownRPM = CP_RPM_MIN + (CP_RPM_MAX - CP_RPM_MIN) * (0.4f + 0.2f * car->throttle);
	shiftUpRPM = CP_RPM_MIN + (CP_RPM_MAX - CP_RPM_MIN) * (0.8f + 0.2f * car->throttle);

	if ( shiftUpRPM > CP_RPM_MAX )
		shiftUpRPM = CP_RPM_MAX;

	if (car->gear > 0){
		rpmTemp = PM_WheelSpeedtoRPM(car, points);

		while ( rpmTemp < shiftDownRPM ){
			if (car->gear > 1)
				car->gear--;
			else if ( rpmTemp < CP_RPM_MIN ){
				rpmTemp = CP_RPM_MIN;
				break;
			}
			else
				break;

			rpmTemp = PM_WheelSpeedtoRPM(car, points);
			if (rpmTemp > CP_RPM_MAX)
				rpmTemp = CP_RPM_MAX;
		}

		while ( rpmTemp > shiftUpRPM ){
			if ( !points[2].onGround || !points[3].onGround || points[2].slipping || points[3].slipping ){
				if ( rpmTemp > CP_RPM_MAX ){
					rpmTemp = CP_RPM_MAX;
					break;
				}
				else if ( rpmTemp > shiftUpRPM )
					break;
			}

			if (car->gear < 5){
				if ( points[2].onGround && points[3].onGround && !points[2].slipping && !points[3].slipping )
					car->gear++;
			}
			else if ( rpmTemp > CP_RPM_MAX ){
				rpmTemp = CP_RPM_MAX;
				break;
			}

			rpmTemp = PM_WheelSpeedtoRPM(car, points);
			if (rpmTemp < CP_RPM_MIN)
				rpmTemp = CP_RPM_MIN;
		}

		car->rpm = rpmTemp;
	}
	else if (car->gear == 0){
		car->rpm = CP_RPM_MIN;
	}
	else {
		rpmTemp = PM_WheelSpeedtoRPM(car, points);
		if (rpmTemp < CP_RPM_MIN)
			rpmTemp = CP_RPM_MIN;
		if (rpmTemp > CP_RPM_MAX)
			rpmTemp = CP_RPM_MAX;

		car->rpm = rpmTemp;
	}
}

/*
================================================================================
PM_AirFrictionForces
================================================================================
*/
static void PM_AirFrictionForces( car_t *car, carBody_t *body, carPoint_t *points, float sec ){
	vec3_t		dir, force;
	float		v, friction, area;
	int			i;

	area = fabs((float)(CAR_HEIGHT * CAR_WIDTH) / (float)(CP_M_2_QU*CP_M_2_QU));

	// dont do air friction on tires
	for (i = FIRST_FRAME_POINT; i < NUM_CAR_POINTS; i++){
		v = VectorNormalize2(points[i].v, dir);

		if (fabs(v) < 0.01f) continue;

		v /= CP_M_2_QU; // m / s

		friction = -0.5 * pm->car_air_cof * area * points[i].fluidDensity * v * v / (float)(NUM_CAR_POINTS);

		friction *= CP_M_2_QU; // to qforce

		VectorScale(dir, friction, force);

		// add down force
		VectorMA(force, -fabs(pm->car_air_frac_to_df * DotProduct(force, body->forward)), body->up, force);

		VectorAdd(points[i].forces[AIR_FRICTION], force, points[i].forces[AIR_FRICTION]);
	}
}

/*
================================================================================
PM_AddRoadForces

  Calculates longitudinal (traction/braking) and lateral (cornering) tire forces
  using a simplified slip ratio + slip angle model.

  - Uses point->scof and point->kcof from PM_CheckSurfaceFlags()
    to determine static vs kinetic grip.
  - Tire stiffness values are controlled by Cvars:
      pm_tireStiffLong (default 3000)
      pm_tireStiffLat  (default 5000)
================================================================================
*/
void PM_AddRoadForces( car_t *car, carBody_t *body, carPoint_t *points, float sec )
{
    int i;
	float v, targetAngle;
	vec3_t		temp;
	vec3_t		forward, right, up;

	// --- Update car state from player input ---
	v = DotProduct(body->v, body->forward);

	if (pm->ps->stats[STAT_HEALTH] > 0){
		car->throttle = pm->cmd.forwardmove / 127.0F;

		if (!pm->manualShift){
			if (car->gear < 0)
				car->throttle *= -1.0f;

			if (car->throttle < 0){
				if (car->gear > 0 && v < 40.0f){
					car->gear = -1;
					car->throttle *= -1.0f;
				}
				else if (car->gear < 0 && v > -40.0f){
					car->gear = 1;
					car->throttle *= -1.0f;
				}
			}
		}

		if (pm->controlMode == CT_MOUSE){
			car->wheelAngle = WheelAngle(pm->ps->viewangles[YAW], pm->ps->damageAngles[YAW]);

			if (v < 0.5f && v > -0.5f)
				car->wheelAngle = 0.0;
			else if ( car->gear < 0 )
				car->wheelAngle *= -1.0;
		}
		else {
			targetAngle = pm->cmd.rightmove / 127.0F * 30.0f;

			if( car->wheelAngle > 0 && targetAngle < car->wheelAngle )
			{
				if ( fabs(car->wheelAngle - targetAngle) < fabs(90.0f * sec) )
					car->wheelAngle = targetAngle;
				else
					car->wheelAngle -= 90.0f * sec;
			}
			else if ( car->wheelAngle < 0 && targetAngle > car->wheelAngle )
			{
				if ( fabs(car->wheelAngle - targetAngle) < fabs(90.0f * sec) )
					car->wheelAngle = targetAngle;
				else
					car->wheelAngle += 90.0f * sec;
			}
			else if (car->wheelAngle != targetAngle){
				if (fabs(car->wheelAngle - targetAngle) < fabs(75.0f * sec / (1 + fabs(v) / 800.0f)))
					car->wheelAngle = targetAngle;
				else if (car->wheelAngle > targetAngle)
					car->wheelAngle -= 75.0f * sec / (1 + fabs(v) / 800.0f);
				else if (car->wheelAngle < targetAngle)
					car->wheelAngle += 75.0f * sec / (1 + fabs(v) / 800.0f);
			}

			if (car->wheelAngle > 20.0f)
				car->wheelAngle = 20.0f;
			if (car->wheelAngle < -20.0f)
				car->wheelAngle = -20.0f;

			pm->ps->damageAngles[PITCH] = 0.0f;
			pm->ps->damageAngles[YAW] = car->wheelAngle;

			pm->ps->damagePitch = ANGLE2BYTE(pm->ps->damageAngles[PITCH]);
			pm->ps->damageYaw = ANGLE2BYTE(pm->ps->damageAngles[YAW]);
		}
	}
	else {
		car->throttle = 0.0f;
	}

	// used for drawing car clientside
	if (car->gear < 0)
		pm->ps->extra_eFlags |= CF_REVERSE;
	else
		pm->ps->extra_eFlags &= ~CF_REVERSE;

	if (car->throttle < 0)
		pm->ps->extra_eFlags |= CF_BRAKE;
	else
		pm->ps->extra_eFlags &= ~CF_BRAKE;


	PM_UpdateRPM(car, points);

	PM_AirFrictionForces(car, body, points, sec);


	// --- Calculate tire forces ---
    for (i = 0; i < FIRST_FRAME_POINT; i++) {
        carPoint_t *wheel;
        float v_forward, v_wheel, v_side;
        float slip, slipAngle;
        float gripForceLong, gripForceLat;
        float tireStiffnessLong, tireStiffnessLat;
        float maxTireForce, totalForceMag, scale;
        vec3_t longForce, latForce;

        wheel = &points[i];

        /* only calculate if wheel is on the ground */
        if (!wheel->onGround) {
            VectorClear(wheel->forces[ROAD]);
            continue;
        }

		// determine the wheel's local coordinate system
		if ( i < 2 ) { // front wheels
			VectorCopy(wheel->normals[0], up);
			if( up[0] == 0.0f && up[1] == 0.0f && up[2] == 0.0f )
				up[2] = 1.0f;
			CrossProduct(body->forward, up, temp);
			RotatePointAroundVector(right, up, temp, -car->wheelAngle);
			VectorNormalize(right);
			CrossProduct(up, right, forward);
		} else { // rear wheels
			VectorCopy(wheel->normals[0], up);
			CrossProduct(body->forward, up, right);
			CrossProduct(up, right, forward);
		}

        /* --- Longitudinal force (slip ratio) --- */
        v_forward = DotProduct(wheel->v, forward);
        v_wheel   = wheel->w * WHEEL_RADIUS;

        slip = 0.0f;
        if (fabs(v_forward) > 1.0f) {
            slip = (v_wheel - v_forward) / fabs(v_forward);
        }
        slip = Com_Clamp(-1.0f, 1.0f, slip);

        /* stiffness from cvars */
        tireStiffnessLong = pm->pm_tireStiffLong;
        tireStiffnessLat  = pm->pm_tireStiffLat;

        /* base max grip from static coefficient */
        maxTireForce = wheel->mass * CP_CURRENT_GRAVITY * wheel->scof;
		if (pm->ps->powerups[PW_TURBO] > 0)
			maxTireForce *= 2.0f;

        gripForceLong = slip * tireStiffnessLong;

        /* --- Lateral force (slip angle) --- */
        v_side = DotProduct(wheel->v, right);

        slipAngle = 0.0f;
        if (fabs(v_forward) > 5.0f) { /* only valid at higher speeds */
            slipAngle = atan2(v_side, fabs(v_forward));
        }
        gripForceLat = -tireStiffnessLat * slipAngle;

        /* --- Friction Circle Limitation --- */
        totalForceMag = sqrt(gripForceLong * gripForceLong + gripForceLat * gripForceLat);

		/* if total force exceeds max grip, check for slipping */
        if (totalForceMag > maxTireForce) {
			/* if slip is large, switch to kinetic friction */
			if (fabs(slip) > 0.2f || fabs(slipAngle) > 0.2f) { // 0.2 rad ~= 11.5 degrees
				maxTireForce = wheel->mass * CP_CURRENT_GRAVITY * wheel->kcof;
				if (pm->ps->powerups[PW_TURBO] > 0)
					maxTireForce *= 2.0f;
				wheel->slipping = qtrue;
			} else {
				wheel->slipping = qfalse;
			}

			// Scale the forces down to fit within the friction circle
            scale = maxTireForce / totalForceMag;
            gripForceLong *= scale;
            gripForceLat *= scale;
        } else {
			wheel->slipping = qfalse;
		}

        /* --- Total force applied on wheel --- */
        VectorScale(forward, gripForceLong, longForce);
        VectorScale(right, gripForceLat, latForce);

        VectorAdd(longForce, latForce, wheel->forces[ROAD]);

		/* --- Apply engine/braking torque --- */
		// This is a simplified model. A more complex one would be tied to the RPM and torque curve.
		if (car->throttle > 0.01f) { // Accelerating
			float engine_torque = car->throttle * 20000.0f; // Simplified engine torque
			if (pm->ps->powerups[PW_TURBO] > 0)
				engine_torque *= 2.5f;
			wheel->netMoment += engine_torque;
		} else if (car->throttle < -0.01f) { // Braking
			float brake_torque = car->throttle * 30000.0f; // Simplified brake torque
			if (fabs(brake_torque) > fabs(wheel->w * 1000.0f)) { // Prevent wheel lockup from reversing direction instantly
				wheel->w = 0;
			} else {
				wheel->netMoment += brake_torque;
			}
		}

		if (pm->cmd.buttons & BUTTON_HANDBRAKE && i >= 2){ // handbrake on rear wheels
			wheel->w = 0;
			wheel->netMoment = 0;
			// Also drastically reduce lateral grip for handbrake turns
			VectorScale(wheel->forces[ROAD], 0.1f, wheel->forces[ROAD]);
		}
    }

	// calculate net forces
	for (i = 0; i < FIRST_FRAME_POINT; i++){
		PM_CalculateNetForce(&points[i], i);
	}
}
