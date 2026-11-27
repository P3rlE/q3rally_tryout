"""Exercise the production HUD hook and HUD option availability in Drag."""
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
static int arrowCalls, ghostUpdates, standingsCalls, hitCalls;
qboolean isRallyRace(void) {
    return cgs.gametype==GT_RACING || cgs.gametype==GT_SPRINT || cgs.gametype==GT_DRAG;
}
qboolean isRallyNonDMRace(void) { return isRallyRace(); }
qboolean CG_IntroCam_IsActive(void) { return qfalse; }
qboolean CG_IsSprintTrack(void) { return cgs.gametype==GT_SPRINT; }
void CG_ModernHUD_DrawStandings(void) { standingsCalls++; }
void CG_DrawArrowToCheckpoint(void) { arrowCalls++; }
void CG_UpdateGhostSplitDelta(void) { ghostUpdates++; }
void CG_DrawHUD_DerbyHitImpact(void) { hitCalls++; }
void trap_Cvar_Update(vmCvar_t *cvar) {}
void trap_SendClientCommand(const char *text) {}
'''
    harness += "vmCvar_t " + ",".join(sorted(cvars)) + ";\n"
    harness += "\n".join(re.findall(r"(?m)^#define HUDOPT_MODE_[^\n]+", core)) + "\n"
    harness += entry_type + "\n"
    harness += function(core, "HUDOpt_Unavail") + "\n"
    harness += function(core, "HUDEntry_IsUnavail") + "\n"
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
    cg_hudShowOpponentList.integer=1; cg_derbyHitFxEnable.integer=1;
    cg.wrongWayStartTime=cg.wrongWayTime=1234;

    /* Drag: no course guidance, the preferences stay */
    CG_DrawHUD();
    assert(cgs.numRacers==1 && standingsCalls==1);
    assert(!arrowCalls && !ghostUpdates && !cg.wrongWayStartTime && !cg.wrongWayTime);
    assert(!RL_ShouldDraw());
    assert(cg_checkpointArrowMode.integer==2 && cg_racingLine.integer==1);
    assert(unavailable(HUDOPT_MODE_COURSE) && unavailable(HUDOPT_MODE_POSITION));
    assert(unavailable(HUDOPT_MODE_LAPS));
    assert(unavailable(HUDOPT_MODE_ORDER)); /* solo: no useful opponent */
    assert(!unavailable(HUDOPT_MODE_TIMES)); /* Drag time cells */
    assert(unavailable(HUDOPT_MODE_VIEWS)); /* the tachometer in their place */
    assert(!unavailable(HUDOPT_MODE_ANY)); /* the HUD itself */
    assert(!hitCalls);

    /* a duel: the opponent list is an option */
    cgs.clientinfo[1].infoValid=qtrue; cgs.clientinfo[1].team=TEAM_FREE;
    CG_DrawHUD(); assert(cgs.numRacers==2);
    assert(!unavailable(HUDOPT_MODE_ORDER));

    /* a race: the arrow (and wrong way) and the ghost split */
    cgs.gametype=GT_RACING; cgs.clientinfo[1].infoValid=qfalse;
    CG_DrawHUD();
    assert(arrowCalls==1 && ghostUpdates==1 && RL_ShouldDraw());
    assert(!unavailable(HUDOPT_MODE_COURSE) && !unavailable(HUDOPT_MODE_POSITION));
    assert(!unavailable(HUDOPT_MODE_LAPS) && !unavailable(HUDOPT_MODE_VIEWS));
    cg_checkpointArrowMode.integer=0; cg.wrongWayStartTime=1234;
    CG_DrawHUD();
    assert(arrowCalls==1 && ghostUpdates==2 && !cg.wrongWayStartTime);
    cg_racingLine.integer=0; assert(!RL_ShouldDraw());
    cgs.gametype=GT_SPRINT; assert(unavailable(HUDOPT_MODE_LAPS)); /* the way to the finish instead */

    /* a spectator gets no guidance; the HUD hidden: nothing at all */
    cg_checkpointArrowMode.integer=2;
    cgs.clientinfo[0].team=TEAM_SPECTATOR;
    CG_DrawHUD(); assert(arrowCalls==1 && ghostUpdates==2);
    cgs.clientinfo[0].team=TEAM_FREE;
    cg.showHUD=qfalse; CG_DrawHUD(); assert(arrowCalls==1 && standingsCalls==5);

    /* Derby: the hit flash; its own order, no race things */
    cg.showHUD=qtrue; cgs.gametype=GT_DERBY; CG_DrawHUD(); assert(hitCalls==1);
    assert(unavailable(HUDOPT_MODE_ORDER) && unavailable(HUDOPT_MODE_TIMES) && !unavailable(GT_DERBY));
    /* combat: the order (frags), no times */
    cgs.gametype=GT_DEATHMATCH; assert(!unavailable(HUDOPT_MODE_ORDER) && unavailable(HUDOPT_MODE_TIMES));
    cgs.gametype=GT_LCS; assert(!unavailable(HUDOPT_MODE_TIMES) && unavailable(HUDOPT_MODE_LAPS));
    return 0;
}
'''
    compile_run(harness)
