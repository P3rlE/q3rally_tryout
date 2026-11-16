"""Run the production neutral/rev, clutch matching, tyre force and engine damage code."""
import csv
import io
import pathlib
import re
import subprocess
import sys
import tempfile

from test_plate_and_menu_regressions import CODE, ROOT, compile_run, function

sys.path.insert(0, str(ROOT / "tools/drag"))
import launch_sweep  # noqa: E402


def _drag_defines(source):
    """The DRAG_* tuning block: #ifndef/#define/#endif triples."""
    return "\n".join(re.findall(r"(?m)^#ifndef DRAG_\w+\n#define DRAG_.*\n#endif", source)) + "\n"


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
    harness += _drag_defines(source)
    for name in ("CP_TORQUE_SLOPE", "CP_GEAR_RATIOS"):
        harness += re.search(r"(?m)^static float " + name + r"\b.*;", source).group() + "\n"
    for name in ("PM_WheelSpeedtoRPM", "PM_ClampRPM", "PM_ApplyRevLimiter", "PM_ClutchOpen",
                 "PM_UpdateFreeEngineRPM", "PM_DragCoupleLaunchRPM", "PM_UpdateRPM",
                 "PM_DragBogFactor", "PM_TireEngineForces", "PM_GroundFrictionForces",
                 "PM_DragEngineUpdate"):
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
    assert(impulse1>0 && impulse1<DRAG_FLYWHEEL_INERTIA*(4500-CP_RPM_MIN)*M_PI/30.0f);
    car.rpm=4000; assert(PM_DragCoupleLaunchRPM(&car,4000,.008f)==0);
    assert(PM_DragCoupleLaunchRPM(&car,4000,0)==0);
    assert(PM_DragCoupleLaunchRPM(&car,CP_RPM_MIN,0)==0 && car.rpm==4000);
    car.rpm=CP_RPM_MAX;
    assert(PM_DragCoupleLaunchRPM(&car,CP_RPM_MIN,1.0f)>=0);
    assert(car.rpm>=CP_RPM_MIN && car.rpm<=CP_RPM_MAX);

    /* a gentle launch (about 1800 RPM) grips; the heavier Drag flywheel dumped from
       the redline breaks the tyres loose */
    middle=rearTraction(24,&middleSlip);
    redline=rearTraction(127,&redlineSlip);
    assert(!middleSlip && redlineSlip);
    /* high torque saturates the real tyre curve: a dump from the redline is
       left with sliding friction only */
    assert(fabs(redline-CP_KCOF*CP_GRAVITY*CP_CAR_MASS*.25f)<1.0f);
    printf("middle rear force %.1f, redline %.1f; coupled RPM %.2f/%.2f\n",middle,redline,rpm1,rpm2);

    /* Bog: a loaded engine below DRAG_BOG_RPM loses torque towards idle. */
    reset(&car,points); car.gear=1; car.rpm=CP_RPM_MIN;
    assert(fabs(PM_DragBogFactor(&car)-DRAG_BOG_MIN_TORQUE)<1e-5f);
    car.rpm=DRAG_BOG_RPM; assert(PM_DragBogFactor(&car)==1.0f);
    car.rpm=2000; middle=PM_DragBogFactor(&car); car.rpm=3000;
    assert(middle>DRAG_BOG_MIN_TORQUE && middle<PM_DragBogFactor(&car) && PM_DragBogFactor(&car)<1.0f);
    car.gear=0; car.rpm=CP_RPM_MIN; assert(PM_DragBogFactor(&car)==1.0f); /* free revs in N */
    car.gear=1; pm->dragRace=qfalse; assert(PM_DragBogFactor(&car)==1.0f); /* Drag only */

    /* Engine stress: only an armed engine, red zone builds, below it cools at half rate. */
    reset(&car,points); car.gear=0; car.rpm=CP_RPM_MAX;
    PM_DragEngineUpdate(&car,500); assert(state.stats[STAT_DRAG_ENGINE]==0);
    state.stats[STAT_DRAG_ENGINE]=DRAG_ENGINE_ARMED;
    PM_DragEngineUpdate(&car,500);
    assert(state.stats[STAT_DRAG_ENGINE]==(DRAG_ENGINE_ARMED|500));
    car.rpm=DRAG_ENGINE_REDLINE_RPM-1; PM_DragEngineUpdate(&car,200);
    assert(state.stats[STAT_DRAG_ENGINE]==(DRAG_ENGINE_ARMED|400));
    PM_DragEngineUpdate(&car,5000); assert(state.stats[STAT_DRAG_ENGINE]==DRAG_ENGINE_ARMED);
    car.rpm=DRAG_ENGINE_REDLINE_RPM;
    for(i=0;i<DRAG_ENGINE_STRESS_LIMIT_MS/16-1;i++) PM_DragEngineUpdate(&car,16);
    assert(!(state.stats[STAT_DRAG_ENGINE]&DRAG_ENGINE_BLOWN));
    PM_DragEngineUpdate(&car,16); PM_DragEngineUpdate(&car,16);
    assert(state.stats[STAT_DRAG_ENGINE]&DRAG_ENGINE_BLOWN);
    assert(!(state.stats[STAT_DRAG_ENGINE]&DRAG_ENGINE_OVERREV));
    i=state.stats[STAT_DRAG_ENGINE]; car.rpm=CP_RPM_MIN; PM_DragEngineUpdate(&car,1000);
    assert(state.stats[STAT_DRAG_ENGINE]==i); /* a blown engine stays blown */
    pm->dragRace=qfalse; state.stats[STAT_DRAG_ENGINE]=DRAG_ENGINE_ARMED; car.rpm=CP_RPM_MAX;
    PM_DragEngineUpdate(&car,1000); assert(state.stats[STAT_DRAG_ENGINE]==DRAG_ENGINE_ARMED);

    /* Over-rev by road speed: a downshift at speed blows it at once, while
       spinning wheels at standstill on the limiter do not. */
    reset(&car,points); state.stats[STAT_DRAG_ENGINE]=DRAG_ENGINE_ARMED;
    car.sBody.forward[0]=1; car.rpm=CP_RPM_MAX; car.gear=1;
    for(i=0;i<NUM_CAR_POINTS;i++) car.sPoints[i].w=-200; /* wheelspin */
    PM_DragEngineUpdate(&car,8); assert(!(state.stats[STAT_DRAG_ENGINE]&DRAG_ENGINE_BLOWN));
    /* 100 km/h in qu/s: fine in 4th, far past the limit in 2nd */
    car.sBody.v[0]=100.0f/3.6f*CP_M_2_QU; car.gear=4; car.rpm=4000;
    PM_DragEngineUpdate(&car,8); assert(!(state.stats[STAT_DRAG_ENGINE]&DRAG_ENGINE_BLOWN));
    car.gear=2; PM_DragEngineUpdate(&car,8);
    assert(state.stats[STAT_DRAG_ENGINE]&DRAG_ENGINE_BLOWN);
    assert(state.stats[STAT_DRAG_ENGINE]&DRAG_ENGINE_OVERREV);
    return 0;
}
'''
    compile_run(harness)


def test_launch_rpm_decides_the_run_and_the_limiter_blows_the_engine():
    """Full production movement: bog at low revs, smoke at high revs, a blown engine
    after about 2.7 s on the limiter, and a sweet spot in the HUD's 4000-5000 band."""
    with tempfile.TemporaryDirectory() as directory:
        binary = launch_sweep.build(pathlib.Path(directory))

        def sweep(*args):
            return subprocess.run([str(binary), *args], check=True, capture_output=True,
                                  text=True, timeout=60).stdout

        for step in ("8", "16", "33"):
            times = {}
            for rpm in ("1000", "2500", "4000", "4500", "5000", "6250"):
                row = next(csv.DictReader(io.StringIO(sweep(rpm, step))))
                times[int(rpm)] = float(row["et_s"])  # a FAIL would not parse
                assert int(row["engine_state"]) & 0x2000 == 0  # normal shifts never blow it
            best = min(times[4000], times[4500], times[5000])
            assert best <= min(times.values())
            assert times[1000] - best > 0.5  # bogging out of the cellar
            assert times[2500] - best > 0.15
            assert times[6250] - best > 0.05  # tyres going up in smoke
            seconds = float(re.search(r"blown after ([0-9.]+) s", sweep("limiter", step)).group(1))
            assert 2.4 < seconds < 3.0
