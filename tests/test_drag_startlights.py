"""Exercise shared production Drag lights and the existing Timber Run lights."""
from test_plate_and_menu_regressions import CODE, compile_run, function


def test_drag_world_and_hud_use_the_same_authoritative_green():
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
static void check(int expectedReds, qboolean expectedGreen, int expectedTime) {
    int reds, time, worldReds;
    qboolean green, worldGreen;
    time=CG_DragStartLightState(&reds,&green);
    CG_FX_StartLightState(4,&worldReds,&worldGreen);
    assert(time==expectedTime && reds==expectedReds && green==expectedGreen);
    assert(worldReds==reds && worldGreen==green);
}
int main(void) {
    snapshot_t snap={0};
    int count, reds;
    qboolean green;
    cg.snap=&snap; cgs.gametype=GT_DRAG;
    check(0,qfalse,0);
    cgs.clientinfo[0].infoValid=cgs.clientinfo[1].infoValid=qtrue;
    cg_entities[0].dragPhase=cg_entities[1].dragPhase=DRAG_COUNTDOWN;
    cg_entities[0].dragGreenTime=cg_entities[1].dragGreenTime=10000;
    cg.time=6000; strcpy(cg.countDownPrint,"3 - HOLD RPM"); check(1,qfalse,10000);
    cg.time=7000; strcpy(cg.countDownPrint,"2 - HOLD RPM"); check(2,qfalse,10000);
    cg.time=8000; strcpy(cg.countDownPrint,"1 - HOLD RPM"); check(3,qfalse,10000);
    /* A randomized final red hold is not timed from the message expiry. */
    cg.countDownEnd=0; cg.time=9999; check(3,qfalse,10000);
    /* A personal false start never changes the shared lamps for the opponent. */
    cg_entities[0].dragPhase=DRAG_FALSE_START; check(3,qfalse,10000);
    cg.time=10000; check(0,qtrue,10000); /* GO message deliberately still missing. */
    cg.time=13999; check(0,qtrue,10000);
    cg.time=14000; check(0,qfalse,10000);
    /* Spectators and drivers see the same common timestamp. */
    snap.ps.clientNum=5; snap.ps.persistant[PERS_TEAM]=TEAM_SPECTATOR;
    cg.time=10020; check(0,qtrue,10000);
    cgs.clientinfo[0].infoValid=qfalse; check(0,qtrue,10000);
    /* Cancellation/restaging clears the participants' timestamps. */
    cg_entities[1].dragGreenTime=0; strcpy(cg.countDownPrint,"START CANCELLED");
    check(0,qfalse,0);
    cg_entities[1].dragGreenTime=20000; cg.time=18000;
    strcpy(cg.countDownPrint,"1"); check(3,qfalse,20000);
    /* A stale text GO cannot release the next heat early. */
    strcpy(cg.countDownPrint,"GO!"); check(0,qfalse,20000);
    cg.time=20000; check(0,qtrue,20000);
    snap.ps.pm_type=PM_INTERMISSION; check(0,qfalse,0);
    snap.ps.pm_type=PM_NORMAL;
    /* Map restart clears every participant, including an obsolete opponent. */
    memset(cg_entities,0,sizeof(cg_entities)); check(0,qfalse,0);
    cg.snap=NULL; check(0,qfalse,0); cg.snap=&snap;
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
    cg_entities[1].dragPhase=DRAG_COUNTDOWN; cg_entities[1].dragGreenTime=10000;
    cg.time=6000;
    for(count=2;count<=8;count++) {
        strcpy(cg.countDownPrint,"3"); CG_FX_StartLightState(count,&reds,&green);
        assert(reds>=1 && reds<count && !green);
        strcpy(cg.countDownPrint,"1"); CG_FX_StartLightState(count,&reds,&green);
        assert(reds==count-1 && !green);
    }
    return 0;
}
'''
    compile_run(harness, shared=True)
