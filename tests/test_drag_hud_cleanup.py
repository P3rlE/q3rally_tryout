"""Exercise the production Drag overlay dispatcher and HUD option availability."""
import re

from test_plate_and_menu_regressions import CODE, compile_run, function


def test_drag_keeps_driver_preferences_but_omits_course_guidance_and_solo_order():
    core = (CODE / "cgame/cg_hud_core.c").read_text()
    racing_line = (CODE / "cgame/cg_rally_racingline.c").read_text()
    entry_type = re.search(
        r"typedef struct \{\s*const char\s+\*label;.*?\} hudToggleEntry_t;", core, re.S
    ).group()
    dispatcher = function(core, "CG_DrawHUD")
    cvars = set(re.findall(r"trap_Cvar_Update\(\s*&(\w+)\s*\)", dispatcher))
    cvars.update(("cg_paused", "cg_derbyHitFxEnable", "cg_racingLine"))
    harness = r'''
#include "cgame/cg_local.h"
#include <assert.h>
cgs_t cgs;
cg_t cg;
static snapshot_t snapshot;
static int arrowCalls, ghostUpdates, orderCalls;
qboolean isRallyRace(void) {
    return cgs.gametype==GT_RACING || cgs.gametype==GT_SPRINT || cgs.gametype==GT_DRAG;
}
qboolean isRallyNonDMRace(void) { return isRallyRace(); }
qboolean BG_GametypeIsTimedRace(int mode) { return mode==GT_RACING || mode==GT_SPRINT || mode==GT_DRAG; }
qboolean CG_IsSprintTrack(void) { return cgs.gametype==GT_SPRINT || cgs.gametype==GT_DRAG; }
qboolean CG_IntroCam_IsActive(void) { return qfalse; }
float CG_DrawArrowToCheckpoint(float y) { arrowCalls++; return y; }
void CG_UpdateGhostSplitDelta(void) { ghostUpdates++; }
float CG_DrawScores(float x,float y) { return y; }
void trap_Cvar_Update(vmCvar_t *cvar) {}
void trap_SendClientCommand(const char *text) {}
static float CG_DrawRacingOrderHUD(float y) { orderCalls++; return y; }
static void CG_DrawDeathmatchOrderHUD(float y) {}
static void CG_DrawTeamDeathmatchOrderHUD(float y) {}
static void CG_DrawCTFOrderHUD(float y) {}
void CG_DrawKOTH_HillStatus(void) {}
void CG_DrawHUD_DerbyVehicleState(void) {}
void CG_DrawHUD_DerbyList(float x,float y) {}
void CG_DrawHUD_DerbyHitImpact(void) {}
float CG_DrawHUD_LCSList(float x,float y) { return y; }
float CG_DrawEliminationTimeline(float y) { return y; }
'''
    harness += "vmCvar_t " + ",".join(sorted(cvars)) + ";\n"
    harness += "\n".join(re.findall(r"(?m)^#define HUDOPT_MODE_[^\n]+", core)) + "\n"
    harness += entry_type + "\n"
    harness += function(core, "HUDEntry_IsUnavail") + "\n"
    harness += function(core, "CG_DrawUpperRightHUD") + "\n"
    harness += function(racing_line, "RL_ShouldDraw") + "\n"
    harness += dispatcher + "\n"
    harness += r'''
static qboolean unavailable(int group) {
    hudToggleEntry_t entry={0}; entry.gameTypeOnly=group;
    return HUDEntry_IsUnavail(&entry);
}
int main(void) {
    cg.snap=&snapshot; cg.showHUD=qtrue; cg.time=10000;
    cg.predictedPlayerState.pm_type=PM_NORMAL;
    cgs.gametype=GT_DRAG; cgs.maxclients=2;
    cgs.clientinfo[0].infoValid=qtrue; cgs.clientinfo[0].team=TEAM_FREE;
    cg_checkpointArrowMode.integer=2; cg_racingLine.integer=1;
    cg_hudShowOpponentList.integer=1;
    cg.wrongWayStartTime=cg.wrongWayTime=1234;
    assert(CG_DrawUpperRightHUD(10)==10 && cgs.numRacers==1);
    assert(!arrowCalls && !ghostUpdates && !cg.wrongWayStartTime && !cg.wrongWayTime);
    assert(!RL_ShouldDraw());
    assert(cg_checkpointArrowMode.integer==2 && cg_racingLine.integer==1);
    assert(unavailable(HUDOPT_MODE_RALLY_RACE));
    assert(unavailable(HUDOPT_MODE_RACE_WITH_LAPS));
    assert(unavailable(HUDOPT_MODE_SPRINT_TRACK));
    assert(unavailable(HUDOPT_MODE_RACE_OR_LCS));
    assert(!unavailable(HUDOPT_MODE_RACE_LCS_DERBY)); /* Drag time cells */
    assert(!unavailable(HUDOPT_MODE_ANY)); /* status, minimap, rear-view */
    CG_DrawHUD(); assert(!orderCalls); /* solo 1/1 list has no useful opponent */

    cgs.clientinfo[1].infoValid=qtrue; cgs.clientinfo[1].team=TEAM_FREE;
    CG_DrawUpperRightHUD(10); assert(cgs.numRacers==2);
    assert(!unavailable(HUDOPT_MODE_RACE_OR_LCS));
    CG_DrawHUD(); assert(orderCalls==1); /* duel list remains optional */
    cg_hudShowOpponentList.integer=0;
    CG_DrawHUD(); assert(orderCalls==1);

    cgs.gametype=GT_RACING; cg_hudShowOpponentList.integer=1;
    cgs.clientinfo[1].infoValid=qfalse;
    CG_DrawUpperRightHUD(10);
    assert(arrowCalls==1 && ghostUpdates==1 && RL_ShouldDraw());
    assert(!unavailable(HUDOPT_MODE_RALLY_RACE));
    assert(!unavailable(HUDOPT_MODE_RACE_WITH_LAPS));
    CG_DrawHUD(); assert(orderCalls==2); /* original solo race behavior */
    assert(cg_checkpointArrowMode.integer==2 && cg_racingLine.integer==1);
    cgs.gametype=GT_SPRINT; assert(!unavailable(HUDOPT_MODE_SPRINT_TRACK));
    cg_racingLine.integer=0; assert(!RL_ShouldDraw());
    return 0;
}
'''
    compile_run(harness)
