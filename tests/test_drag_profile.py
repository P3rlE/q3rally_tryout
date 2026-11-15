"""Exercise production Drag career awards, bests and idempotent outcomes."""
import re

from test_plate_and_menu_regressions import CODE, compile_run, function


def test_drag_career_uses_existing_race_awards_only_in_duels():
    source = (CODE / "game/g_profile.c").read_text()
    constants = "\n".join(re.findall(r"^#define PROFILE_SCORE_.*$", source, re.M))
    harness = r'''
#include "game/g_local.h"
#include "game/bg_achievements.h"
#include <assert.h>
vmCvar_t g_gametype;
level_locals_t level;
static qboolean duelHeat;
qboolean G_DragLadderDuel(void) { return duelHeat; }
static struct {
    qboolean loaded,dirty,isOnlineSession;
    profile_stats_t stats;
    profile_info_t info;
} s_profileState;
static int achievementChecks;
static void G_Profile_CheckAchievementProgress(gclient_t *client,bgAchievementCategory_t category) {
    assert(category==BG_ACHIEVEMENT_WINS || category==BG_ACHIEVEMENT_SPRINT_WINS);
    achievementChecks++;
}
static void G_Profile_UpdateRankState(void) {}
static void G_Profile_MaybeAutosave(void) {}
qboolean G_Profile_GetRank(const profile_stats_t *stats,profile_rank_t *rank) { return qfalse; }
void trap_SendServerCommand(int client,const char *command) {}
void QDECL Com_Printf(const char *format,...) {}
void QDECL Com_Error(int code,const char *format,...) { abort(); }
float AngleNormalize180(float angle) { return angle; }
'''
    harness += constants + "\n"
    for name in ("G_Profile_ShouldTrackClient", "G_Profile_IsRacingGametype",
                 "G_Profile_AddScore", "G_Profile_RecordRacePlacement",
                 "G_Profile_RecordWin", "G_Profile_RecordLoss", "G_Profile_RecordDragResult"):
        harness += function(source, name) + "\n"
    harness += r'''
static gclient_t driver;
static void reset(qboolean duel,qboolean online) {
    memset(&driver,0,sizeof(driver)); memset(&s_profileState,0,sizeof(s_profileState));
    s_profileState.loaded=qtrue; s_profileState.isOnlineSession=online;
    driver.pers.localClient=qtrue; duelHeat=duel; g_gametype.integer=GT_DRAG;
    achievementChecks=0;
}
int main(void) {
    reset(qfalse,qfalse);
    G_Profile_RecordDragResult(&driver,0,13000,1,qfalse,qfalse,qfalse);
    assert(s_profileState.stats.dragCompleted==1 && s_profileState.stats.gamesPlayed==1);
    assert(!s_profileState.stats.dragWins && !s_profileState.stats.wins && !s_profileState.stats.losses);
    assert(!s_profileState.stats.playerScore && s_profileState.stats.dragBestMs==13000);
    assert(s_profileState.stats.dragBestReactionMs==0 && s_profileState.stats.dragBestElapsedMs==13000);
    G_Profile_RecordDragResult(&driver,0,13000,1,qfalse,qfalse,qfalse);
    assert(s_profileState.stats.dragCompleted==1); /* LogExit + intermission */
    G_Profile_AddScore(12); assert(!s_profileState.stats.playerScore); /* no accidental solo points */
    driver.pers.profileMatchOutcomeRecorded=qfalse;
    G_Profile_RecordDragResult(&driver,200,12500,1,qfalse,qfalse,qfalse);
    assert(s_profileState.stats.dragCompleted==2 && s_profileState.stats.dragBestMs==12700);
    assert(s_profileState.stats.dragBestReactionMs==0 && s_profileState.stats.dragBestElapsedMs==12500);

    reset(qtrue,qfalse);
    G_Profile_RecordDragResult(&driver,220,13000,1,qtrue,qtrue,qtrue);
    assert(s_profileState.stats.dragWins==1 && s_profileState.stats.wins==1 && achievementChecks==1);
    assert(s_profileState.stats.dragCompleted==1 && s_profileState.stats.playerScore==12);
    assert(!s_profileState.stats.sprintWins && !s_profileState.stats.sprintCompleted);
    assert(!s_profileState.stats.bestLapMs); /* Drag times are not lap records. */
    G_Profile_RecordDragResult(&driver,220,13000,1,qtrue,qtrue,qtrue);
    assert(s_profileState.stats.playerScore==12 && s_profileState.stats.gamesPlayed==1);
    reset(qtrue,qtrue);
    G_Profile_RecordDragResult(&driver,220,13000,1,qtrue,qtrue,qtrue);
    assert(s_profileState.stats.playerScore==13); /* native per-award float32 bonus */

    reset(qtrue,qtrue);
    G_Profile_RecordDragResult(&driver,500,13000,2,qtrue,qfalse,qtrue);
    assert(s_profileState.stats.playerScore==6 && s_profileState.stats.losses==1);
    assert(s_profileState.stats.dragCompleted==1 && !s_profileState.stats.dragWins);
    reset(qtrue,qfalse);
    G_Profile_RecordDragResult(&driver,0,0,0,qtrue,qfalse,qtrue); /* false start / DNF */
    assert(s_profileState.stats.losses==1 && s_profileState.stats.gamesPlayed==1);
    assert(!s_profileState.stats.playerScore && !s_profileState.stats.dragCompleted && !s_profileState.stats.dragBestMs);
    reset(qtrue,qfalse);
    G_Profile_RecordDragResult(&driver,220,13000,1,qtrue,qfalse,qfalse); /* exact tie */
    assert(s_profileState.stats.gamesPlayed==1 && s_profileState.stats.dragCompleted==1);
    assert(!s_profileState.stats.wins && !s_profileState.stats.losses && !s_profileState.stats.playerScore);
    reset(qtrue,qfalse);
    driver.pers.localClient=qfalse;
    strcpy(driver.pers.uuid,"other-profile"); strcpy(s_profileState.info.uuid,"active-profile");
    G_Profile_RecordDragResult(&driver,220,13000,1,qtrue,qtrue,qtrue);
    assert(!s_profileState.stats.dragCompleted && !s_profileState.stats.playerScore);

    reset(qfalse,qfalse); g_gametype.integer=GT_SPRINT;
    G_Profile_RecordWin(&driver); assert(s_profileState.stats.playerScore==12);
    assert(s_profileState.stats.sprintWins==1 && s_profileState.stats.sprintCompleted==1);
    return 0;
}
'''
    compile_run(harness, shared=True)
