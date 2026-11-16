"""Exercise shared production Drag lights and the existing Timber Run lights."""
from test_plate_and_menu_regressions import CODE, compile_run, function


def test_drag_world_and_hud_follow_the_countdown_and_announced_green():
    racing = (CODE / "cgame/cg_rally_racetools.c").read_text()
    effects = (CODE / "cgame/cg_rally_fx.c").read_text()
    harness = r'''
#include "cgame/cg_local.h"
#include <assert.h>
cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
vmCvar_t cg_startLightPreview;
void QDECL Com_Printf(const char *format, ...) {}
void QDECL Com_Error(int code, const char *format, ...) { abort(); }
float AngleNormalize180(float angle) { return angle; }
'''
    harness += function(racing, "CG_DragStartLightState") + "\n"
    harness += function(effects, "CG_FX_StartLightState") + "\n"
    harness += r'''
static void check(int expectedReds, qboolean expectedGreen, int expectedHeat, int expectedGreenTime) {
    int reds, heat, greenTime, worldReds;
    qboolean green, worldGreen;
    heat=CG_DragStartLightState(&reds,&green,&greenTime);
    CG_FX_StartLightState(4,&worldReds,&worldGreen);
    assert(heat==expectedHeat && greenTime==expectedGreenTime);
    assert(reds==expectedReds && green==expectedGreen);
    assert(worldReds==reds && worldGreen==green);
}
static void arm(int countdown, int greenTime) {
    cg_entities[0].dragCountdownTime=cg_entities[1].dragCountdownTime=countdown;
    cg_entities[0].dragGreenTime=cg_entities[1].dragGreenTime=greenTime;
}
int main(void) {
    snapshot_t snap={0};
    int count, reds;
    qboolean green;
    cg.snap=&snap; cgs.gametype=GT_DRAG;
    check(0,qfalse,0,0);
    cgs.clientinfo[0].infoValid=cgs.clientinfo[1].infoValid=qtrue;
    cg_entities[0].dragPhase=cg_entities[1].dragPhase=DRAG_COUNTDOWN;
    /* The server publishes the countdown start, never the coming green. */
    arm(6000,0);
    cg.time=6000; check(1,qfalse,6000,0);
    cg.time=6999; check(1,qfalse,6000,0);
    cg.time=7000; check(2,qfalse,6000,0);
    cg.time=8000; check(3,qfalse,6000,0);
    /* The lamps do not depend on countdown text arriving. */
    strcpy(cg.countDownPrint,"GO!"); cg.time=8500; check(3,qfalse,6000,0);
    cg.countDownPrint[0]='\0';
    /* The third red holds for the whole variable final wait. */
    cg.time=9999; check(3,qfalse,6000,0);
    cg.time=12000; check(3,qfalse,6000,0);
    /* A personal false start never changes the shared lamps for the opponent. */
    cg_entities[0].dragPhase=DRAG_FALSE_START; check(3,qfalse,6000,0);
    /* Green is shown from the frame that announced it. */
    arm(6000,9550); cg.time=9550; check(0,qtrue,6000,9550);
    cg.time=13549; check(0,qtrue,6000,9550);
    cg.time=13550; check(0,qfalse,6000,9550);
    /* Only one driver's status may carry green yet; it still counts. */
    cg_entities[0].dragGreenTime=0; cg.time=9600; check(0,qtrue,6000,9550);
    /* A clock briefly behind the published green keeps the third red. */
    cg.time=9549; check(3,qfalse,6000,9550);
    /* Spectators and drivers see the same common timestamps. */
    snap.ps.clientNum=5; snap.ps.persistant[PERS_TEAM]=TEAM_SPECTATOR;
    cg.time=9600; check(0,qtrue,6000,9550);
    cgs.clientinfo[0].infoValid=qfalse; check(0,qtrue,6000,9550);
    cgs.clientinfo[0].infoValid=qtrue;
    /* Cancellation/restaging clears the countdown. */
    arm(0,0); cg_entities[0].dragPhase=cg_entities[1].dragPhase=DRAG_STAGED;
    check(0,qfalse,0,0);
    /* A newer heat replaces stale state from an earlier one. */
    cg_entities[0].dragPhase=DRAG_FINISHED; cg_entities[0].dragCountdownTime=6000;
    cg_entities[0].dragGreenTime=9550;
    cg_entities[1].dragPhase=DRAG_COUNTDOWN; cg_entities[1].dragCountdownTime=18000;
    cg_entities[1].dragGreenTime=0;
    cg.time=20500; check(3,qfalse,18000,0);
    cg_entities[1].dragGreenTime=21200; cg.time=21200; check(0,qtrue,18000,21200);
    snap.ps.pm_type=PM_INTERMISSION; check(0,qfalse,0,0);
    snap.ps.pm_type=PM_NORMAL;
    /* Map restart clears every participant, including an obsolete opponent. */
    memset(cg_entities,0,sizeof(cg_entities)); check(0,qfalse,0,0);
    cg.snap=NULL; check(0,qfalse,0,0); cg.snap=&snap;
    /* Mapper preview remains independent of Drag state and of sounds. */
    cg_startLightPreview.integer=2;
    CG_FX_StartLightState(4,&reds,&green); assert(reds==2 && !green);
    cg_startLightPreview.integer=4;
    CG_FX_StartLightState(4,&reds,&green); assert(reds==0 && green);
    cg_startLightPreview.integer=0;
    /* The original Sprint / Timber Run message-driven behaviour is retained. */
    cgs.gametype=GT_SPRINT; cg.time=3000; cg.countDownEnd=6000;
    strcpy(cg.countDownPrint,"3");
    CG_FX_StartLightState(4,&reds,&green); assert(reds==1 && !green);
    strcpy(cg.countDownPrint,"2");
    CG_FX_StartLightState(4,&reds,&green); assert(reds==2 && !green);
    strcpy(cg.countDownPrint,"1");
    CG_FX_StartLightState(4,&reds,&green); assert(reds==3 && !green);
    strcpy(cg.countDownPrint,"GO!"); cg.countDownEnd=3000;
    CG_FX_StartLightState(4,&reds,&green); assert(reds==0 && green);
    cg.time=7000;
    CG_FX_StartLightState(4,&reds,&green); assert(reds==0 && !green);
    /* Configurable lamp counts stay within their housing's lens count. */
    cgs.gametype=GT_DRAG; cgs.clientinfo[1].infoValid=qtrue;
    cg_entities[1].dragPhase=DRAG_COUNTDOWN; cg_entities[1].dragCountdownTime=6000;
    for(count=2;count<=8;count++) {
        cg.time=6000; CG_FX_StartLightState(count,&reds,&green);
        assert(reds>=1 && reds<count && !green);
        cg.time=8000; CG_FX_StartLightState(count,&reds,&green);
        assert(reds==count-1 && !green);
    }
    return 0;
}
'''
    compile_run(harness, shared=True)
