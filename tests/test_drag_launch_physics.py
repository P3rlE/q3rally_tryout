"""Run the production neutral/rev, clutch matching and tyre force calculations."""
import re

from test_plate_and_menu_regressions import CODE, compile_run, function


def test_drag_staged_revs_transfer_bounded_energy_and_excess_revs_slip():
    source = (CODE / "game/bg_wheel_forces.c").read_text()
    harness = r'''
#include "qcommon/q_shared.h"
#include "game/bg_public.h"
#include <assert.h>
pmove_t movement;
pmove_t *pm = &movement;
playerState_t state;
float CP_M_2_QU = 35.6613f;
void QDECL Com_Printf(const char *fmt, ...) {}
vec_t VectorNormalize(vec3_t v) {
    float length=VectorLength(v);
    if(length>0) { v[0]/=length; v[1]/=length; v[2]/=length; }
    return length;
}
#define CP_RPM_LIMITER_CUT CP_RPM_MAX
#define CP_RPM_LIMITER_RESUME (CP_RPM_MAX-320.0f)
'''
    for name in ("CP_TORQUE_SLOPE", "CP_GEAR_RATIOS"):
        harness += re.search(r"(?m)^static float " + name + r"\b.*;", source).group() + "\n"
    for name in ("PM_WheelSpeedtoRPM", "PM_ClampRPM", "PM_ApplyRevLimiter", "PM_ClutchOpen",
                 "PM_UpdateFreeEngineRPM", "PM_DragCoupleLaunchRPM", "PM_UpdateRPM",
                 "PM_TireEngineForces", "PM_GroundFrictionForces"):
        harness += function(source, name) + "\n"
    harness += r'''
static void reset(car_t *car,carPoint_t *points) {
    memset(car,0,sizeof(*car)); memset(points,0,sizeof(carPoint_t)*NUM_CAR_POINTS);
    memset(&movement,0,sizeof(movement)); memset(&state,0,sizeof(state));
    pm->ps=&state; pm->transmissionMode=TR_MANUAL; pm->dragRace=qtrue;
    car->fuel=100; car->gear=0; car->rpm=CP_RPM_MIN;
}
static float integrateFlywheel(float sec,float *finalRpm) {
    car_t car; int i; float impulse=0;
    memset(&car,0,sizeof(car)); car.rpm=4500;
    for(i=0;i<(int)(.8f/sec+.5f);i++)
        impulse += PM_DragCoupleLaunchRPM(&car,CP_RPM_MIN,sec)*1.355818f*sec;
    *finalRpm=car.rpm;
    return impulse;
}
static float rearTraction(int throttle,qboolean *slipping) {
    car_t car; carPoint_t points[NUM_CAR_POINTS];
    float extra,demand; int i=2;
    vec3_t forward={1,0,0},right={0,1,0},up={0,0,1};
    reset(&car,points); car.gear=1;
    car.throttle=throttle/127.0f;
    car.rpm=CP_RPM_MIN+car.throttle*(CP_RPM_MAX-CP_RPM_MIN);
    points[i].mass=CP_WHEEL_MASS; points[i].scof=CP_SCOF; points[i].kcof=CP_KCOF;
    points[i].forces[NORMAL][2]=CP_GRAVITY*CP_CAR_MASS*.25f;
    extra=PM_UpdateRPM(&car,points,.008f);
    PM_TireEngineForces(&car,points,i,forward,extra);
    demand=-points[i].netMoment/WHEEL_RADIUS;
    /* Present the production tyre curve with the longitudinal slip demand
       required to balance this launch shaft torque on a loaded rear wheel. */
    points[i].w=-demand*.008f/(.1f*points[i].mass*WHEEL_RADIUS);
    PM_GroundFrictionForces(&car,points,i,forward,right,up,.008f);
    *slipping=points[i].slipping;
    return points[i].forces[ROAD][0];
}
int main(void) {
    car_t car; carPoint_t points[NUM_CAR_POINTS];
    float extra,rpm1,rpm2,impulse1,impulse2,middle,redline;
    int i; qboolean middleSlip,redlineSlip;
    vec3_t forward={1,0,0};
    reset(&car,points);
    for(i=0;i<250;i++) PM_UpdateRPM(&car,points,.008f);
    assert(car.rpm==CP_RPM_MIN);
    car.throttle=64.0f/127.0f;
    for(i=0;i<250;i++) PM_UpdateRPM(&car,points,.008f);
    assert(fabs(car.rpm-(CP_RPM_MIN+5500.0f*64.0f/127.0f))<.1f);
    PM_TireEngineForces(&car,points,2,forward,0);
    assert(points[2].netMoment==0); /* neutral never drives the wheels */
    car.gear=1; extra=PM_UpdateRPM(&car,points,.008f);
    assert(car.rpm>3000 && extra>0); /* selecting first retains staged energy */
    PM_TireEngineForces(&car,points,2,forward,extra);
    assert(points[2].netMoment<0);

    reset(&car,points); car.gear=1; car.throttle=1; car.rpm=CP_RPM_MIN;
    assert(PM_UpdateRPM(&car,points,.008f)==0 && car.rpm==CP_RPM_MIN);
    reset(&car,points); car.gear=1; car.rpm=4500; pm->dragRace=qfalse;
    assert(PM_UpdateRPM(&car,points,.008f)==0 && car.rpm==CP_RPM_MIN);
    reset(&car,points); car.gear=2; car.rpm=4500;
    assert(PM_UpdateRPM(&car,points,.008f)==0 && car.rpm==CP_RPM_MIN);

    impulse1=integrateFlywheel(.001f,&rpm1);
    impulse2=integrateFlywheel(.020f,&rpm2);
    assert(fabs(rpm1-rpm2)<.5f);
    assert(fabs(impulse1-impulse2)<.005f);
    assert(impulse1>0 && impulse1<.08f*(4500-CP_RPM_MIN)*M_PI/30.0f);
    car.rpm=4000; assert(PM_DragCoupleLaunchRPM(&car,4000,.008f)==0);
    assert(PM_DragCoupleLaunchRPM(&car,4000,0)==0);
    assert(PM_DragCoupleLaunchRPM(&car,CP_RPM_MIN,0)==0 && car.rpm==4000);
    car.rpm=CP_RPM_MAX;
    assert(PM_DragCoupleLaunchRPM(&car,CP_RPM_MIN,1.0f)>=0);
    assert(car.rpm>=CP_RPM_MIN && car.rpm<=CP_RPM_MAX);

    middle=rearTraction(64,&middleSlip);
    redline=rearTraction(127,&redlineSlip);
    assert(!middleSlip && redlineSlip);
    assert(middle>redline); /* high torque saturates the real tyre curve */
    printf("middle rear force %.1f, redline %.1f; coupled RPM %.2f/%.2f\n",middle,redline,rpm1,rpm2);
    return 0;
}
'''
    compile_run(harness)
