"""Verify match exits and the score state captured for the Ladder."""
from test_plate_and_menu_regressions import CODE, compile_run, function


def test_match_exit_rules():
    def game_source(name):
        return (CODE / 'game' / (name + '.c')).read_text(encoding='utf-8')

    main, team, ball, combat, rally, modes, gates = map(game_source, (
        'g_main', 'g_team', 'g_autoball', 'g_combat', 'g_rally_racetools',
        'bg_misc', 'g_rally_mapents',
    ))
    harness = r'''
#include "game/g_local.h"
#include "game/g_drag.h"
#include "game/g_profile.h"
#include "game/bg_koth.h"
#include "qcommon/rally_physics.h"
#include <assert.h>
#include <limits.h>

level_locals_t level;
float CP_M_2_QU = 35.66f;
gentity_t g_entities[MAX_GENTITIES];
static gclient_t clients[3];
vmCvar_t g_gametype, g_timelimit, g_maxclients, g_fraglimit, g_capturelimit, g_humanplayers;
vmCvar_t g_kothOvertime, g_kothOvertimeHoldTime, g_kothScoreWin, g_autoballGoalDelay;
vmCvar_t g_finishRaceDelay, g_timeTrialFinishDelay, g_derbyNoRamTime, g_derbyMinPlayers;
vmCvar_t g_dominationScoreInterval;
static int reports, reportedAssists, reportedFlagAssists, podiums;
static const char *reasons[4];

void CheckExitRules(void);
void LogExit(const char *reason);
void trap_SetConfigstring(int idx, const char *value) {}
void trap_Cvar_Set(const char *name, const char *value) {}
void trap_Cvar_Update(vmCvar_t *cvar) {}
void trap_SendServerCommand(int client, const char *text) {}
void QDECL G_LogPrintf(const char *fmt, ...) {}
void QDECL G_Printf(const char *fmt, ...) {}
char *QDECL va(char *format, ...) {
    static char buffer[1024]; va_list ap;
    va_start(ap, format); vsnprintf(buffer, sizeof(buffer), format, ap); va_end(ap);
    return buffer;
}
float Com_Clamp(float lo, float hi, float x) { return x < lo ? lo : x > hi ? hi : x; }
void BeginIntermission(void) { level.intermissiontime=level.time; }
void CheckIntermissionExit(void) {}
void SendScoreboardMessageToAllClients(void) {}
qboolean G_DragCheckExit(void) { return qfalse; }
qboolean G_Autoball_HoldMatchEnd(void) { return qfalse; }
qboolean isRaceObserver(int cn) { return g_entities[cn].raceObserver; }
gentity_t *G_TempEntity(vec3_t origin, int event) { static gentity_t eventEntity; return &eventEntity; }
void G_Damage(gentity_t *target, gentity_t *inflictor, gentity_t *attacker,
              vec3_t dir, vec3_t point, int damage, int flags, int mod) {}
int TeamCount(int ignore, team_t team) {
    int i, n = 0;
    for (i=0; i<level.maxclients; ++i)
        n += i != ignore && clients[i].pers.connected != CON_DISCONNECTED && clients[i].sess.sessionTeam == team;
    return n;
}
static gentity_t *KOTH_GetHillEntity(void) { static gentity_t hill; return &hill; }
static void KOTH_CountPlayersInHill(const gentity_t *hill, int *red, int *blue) { *red=1; *blue=0; }
static void G_LadderSubmitMatchReport(const char *reason) {
    assert(reports < ARRAY_LEN(reasons));
    reasons[reports++] = reason;
    reportedAssists = clients[1].pers.autoballAssists;
    reportedFlagAssists = clients[1].ps.persistant[PERS_ASSIST_COUNT];
}
void G_Profile_FlushIfDirty(void) {}
qboolean G_Profile_IsDirty(void) { return qfalse; }
qboolean G_DragLadderDuel(void) { return qfalse; }
qboolean G_DragLadderIdentityMatches(int cn) { return qfalse; }
qboolean G_DragPopulateLadderPlayer(int cn, ladderPlayerPayload_t *p) { return qfalse; }
void G_Profile_RecordDragResult(gclient_t *c,int rt,int et,int place,qboolean duel,qboolean won,qboolean hasWinner) {}
void ClientUserinfoChanged(int cn) {}
void G_Profile_RecordWin(gclient_t *c) {}
void G_Profile_RecordLoss(gclient_t *c) {}
void G_Profile_RecordZoneHold(gclient_t *c, int ms) {}
void G_Profile_RecordAutoballGoals(gclient_t *c, int goals) {}
static void G_Profile_RecordMatchAchievements(gclient_t *c) {}
void G_Profile_RecordRacePlacement(gclient_t *c, int position) { podiums++; }
void G_Profile_RecordRaceTime(gclient_t *c, int ms) {}
void G_Profile_RecordSprintTime(gclient_t *c, int ms) {}
void G_Profile_RecordSurvivalTime(gclient_t *c, int ms) {}
void G_Profile_RecordEliminationRound(gclient_t *c, int rounds) {}
void ScorePlum(gentity_t *e, vec3_t origin, int score) {}
void AddTeamScore(vec3_t origin, int team, int score) { level.teamScores[team] += score; }
void Rally_Sound(gentity_t *e,int event,int channel,int sound) {}
int G_SoundIndex(char *name) { return 0; }
const char *TeamName(int team) { return ""; }
const char *TeamColorString(int team) { return ""; }
void QDECL PrintMsg(gentity_t *e, const char *fmt, ...) {}
void Team_ForceGesture(int team) {}
void Team_CaptureFlagSound(gentity_t *e, int team) {}
void Team_ReturnFlagSound(gentity_t *e, int team) {}
void Team_ResetFlags(void) {}
gentity_t *Team_ResetFlag(int team) { return NULL; }
void G_Profile_RecordFlagCapture(gclient_t *c) {}
void G_Profile_RecordFlagAssist(gclient_t *c) {}
static void G_Autoball_GoalPush(const vec3_t origin) {}
static void G_Autoball_SetState(int state) { level.autoballState = state; }
'''
    start = team.index('typedef struct teamgame_s {')
    end = team.index('gentity_t\t*neutralObelisk;', start)
    # The real teamgame type uses the internal domination_sigil_t declaration.
    start = team.rindex('typedef struct', 0, start)
    harness += team[start:end]
    harness += '\n'.join(line for line in ball.splitlines() if line.startswith('#define AUTOBALL_')) + '\n'
    for source, names in (
        (modes, ('BG_GametypeIsRace', 'BG_GametypeHasRaceFinish')),
        (rally, ('GetTeamAtRank',)),
        (gates, ('G_RallyUpdateTeamTime', 'G_RallyUpdateAllTeamTimes')),
        (main, ('ScoreIsTied', 'G_RaceFinishDelayElapsed', 'G_IsMatchParticipant',
                'G_IsRaceFinisher', 'G_TeamRaceWinner', 'G_LadderWinnerForGametype',
                'G_RecordMatchOutcome', 'LogExit', 'CheckExitRules', 'SortRanks', 'CalculateRanks')),
        (team, ('KOTH_HandleOvertime', 'Sigil_Think', 'OtherTeam', 'Team_TouchOurFlag')),
        (combat, ('AddScoreWithoutRankUpdate', 'AddScore')),
        (ball, ('G_Autoball_OpposingTeam', 'G_Autoball_ScoreGoal')),
    ):
        for name in names:
            harness += function(source, name) + '\n'
    harness += r'''
qboolean isRallyRace(void) { return BG_GametypeIsRace(g_gametype.integer); }

static void reset(int mode, int count) {
    int i;
    memset(&level,0,sizeof(level)); memset(clients,0,sizeof(clients)); memset(g_entities,0,sizeof(g_entities));
    memset(&teamgame,0,sizeof(teamgame));
    g_gametype.integer=mode;
    g_timelimit.integer=g_fraglimit.integer=g_capturelimit.integer=0;
    g_kothOvertime.integer=g_kothScoreWin.integer=0;
    g_maxclients.integer=level.maxclients=count;
    level.clients=clients; level.startTime=1000; level.time=50000;
    reports = podiums = 0;
    level.winnerNumber = -1;
    reportedAssists = reportedFlagAssists = -1;
    for (i=0; i<count; ++i) {
        g_entities[i].client=&clients[i]; g_entities[i].inuse=qtrue; g_entities[i].s.number=i;
        clients[i].pers.connected=CON_CONNECTED; clients[i].ps.clientNum=i; clients[i].ps.stats[STAT_HEALTH]=100;
        clients[i].sess.sessionTeam=TEAM_FREE;
    }
}

static void testDomination(void) {
    gentity_t sigil = {0};
    const team_t fourTeamWinners[] = {TEAM_RED, TEAM_GREEN, TEAM_YELLOW};
    int i;

    for (i = 0; i < ARRAY_LEN(fourTeamWinners); ++i) {
        team_t winner = fourTeamWinners[i];
        reset(GT_DOMINATION, 2);
        clients[0].sess.sessionTeam = winner;
        clients[1].sess.sessionTeam = TEAM_BLUE;
        g_capturelimit.integer = 100;
        level.teamScores[winner] = 100;
        CalculateRanks();
        assert(reports == 1 && !strcmp(reasons[0], "Capturelimit hit."));
    }
    sigil.s.powerups = PW_SIGILYELLOW;
    Sigil_Think(&sigil);
    assert(level.teamScores[TEAM_YELLOW] == 100);
}

static void testAutoball(void) {
    gentity_t ball = {0}, goal = {0};

    reset(GT_AUTOBALL, 3);
    clients[0].sess.sessionTeam = clients[1].sess.sessionTeam = TEAM_RED;
    clients[2].sess.sessionTeam = TEAM_BLUE;
    g_capturelimit.integer = 5;
    level.teamScores[TEAM_RED] = 4;
    level.teamScores[TEAM_BLUE] = 2;
    ball.ballLastToucher = 0;
    ball.ballLastTouchTime = level.time - 100;
    ball.ballTouchClient[0] = 0;
    ball.ballTouchClient[1] = 1;
    ball.ballTouchTime[1] = level.time - 200;
    goal.count = TEAM_BLUE;

    CalculateRanks();
    G_Autoball_ScoreGoal(&ball, &goal);
    assert(reports == 1 && reportedAssists == 1);
}

static void testFlagCapture(int mode, team_t homeTeam, powerup_t enemyFlag) {
    gentity_t flag = {0};

    reset(mode, 3);
    clients[0].sess.sessionTeam = clients[1].sess.sessionTeam = homeTeam;
    clients[2].sess.sessionTeam = TEAM_BLUE;
    g_capturelimit.integer = 5;
    level.teamScores[homeTeam] = 4;
    level.teamScores[TEAM_BLUE] = 2;
    clients[0].ps.powerups[enemyFlag] = INT_MAX;
    clients[1].pers.teamState.lastreturnedflag = level.time - 100;

    CalculateRanks();
    Team_TouchOurFlag(&flag, &g_entities[0], homeTeam);
    assert(reports == 1 && reportedFlagAssists == 1);
    clients[0].ps.powerups[enemyFlag] = INT_MAX;
    Team_TouchOurFlag(&flag, &g_entities[0], homeTeam);
    assert(level.teamScores[homeTeam] == 5 && reports == 1);
}

static void testKoth(void) {
    reset(GT_KOTH, 2);
    clients[0].sess.sessionTeam = TEAM_RED;
    clients[1].sess.sessionTeam = TEAM_BLUE;
    g_timelimit.integer = g_kothOvertime.integer = 1;
    g_kothOvertimeHoldTime.integer = 10000;
    level.teamScores[TEAM_RED] = level.teamScores[TEAM_BLUE] = 20;
    teamgame.kothOwner = TEAM_RED;
    level.time = 61000;
    CalculateRanks();
    assert(reports == 0 && teamgame.kothOvertimeActive);

    level.time = 71000;
    CheckExitRules();
    LogExit("Another exit request.");
    assert(reports == 1 && !strcmp(reasons[0], "KOTH overtime finished."));
    assert(level.teamScores[TEAM_RED] == 21);
}

static void testLastCarStanding(void) {
    reset(GT_LCS, 1);
    g_derbyMinPlayers.integer = 2;
    level.derbyStartPlayerCount = 1;
    level.startRaceTime = 20000;
    clients[0].ps.stats[STAT_HEALTH] = 0;
    CalculateRanks();
    assert(reports == 1 && level.intermissionQueued);
    assert(!clients[0].sess.wins && !clients[0].sess.losses);
    assert(G_LadderWinnerForGametype(GT_LCS) == -1);
}

static void testRaceOutcomes(void) {
    const int soloModes[] = {GT_RACING, GT_RACING_DM, GT_SPRINT, GT_SINGLE_PLAYER};
    const int teamModes[] = {GT_TEAM_RACING, GT_TEAM_RACING_DM};
    int i;

    for (i = 0; i < ARRAY_LEN(soloModes); ++i) {
        reset(soloModes[i], 2);
        g_timelimit.integer = 1;
        level.time = 61000;
        level.startRaceTime = 20000;
        clients[0].ps.stats[STAT_POSITION] = 2;
        clients[1].ps.stats[STAT_POSITION] = 1;
        CalculateRanks();
        assert(reports == 1 && !clients[1].sess.wins && !clients[0].sess.losses && !podiums);
        assert(G_LadderWinnerForGametype(soloModes[i]) == -1);

        reset(soloModes[i], 2);
        level.startRaceTime = 20000;
        clients[0].ps.stats[STAT_POSITION] = 2;
        clients[1].ps.stats[STAT_POSITION] = 1;
        clients[1].finishRaceTime = 40000;
        CalculateRanks();
        LogExit("Race finished.");
        assert(clients[1].sess.wins == 1 && clients[0].sess.losses == 1 && podiums == 1);
        assert(G_LadderWinnerForGametype(soloModes[i]) == 1);
    }

    for (i = 0; i < ARRAY_LEN(teamModes); ++i) {
        reset(teamModes[i], 2);
        clients[0].sess.sessionTeam = TEAM_RED;
        clients[1].sess.sessionTeam = TEAM_BLUE;
        level.startRaceTime = 1000;
        clients[0].finishRaceTime = clients[1].finishRaceTime = 49000;
        CalculateRanks();
        LogExit("Race finished.");
        assert(reports == 1 && !clients[0].sess.wins && !clients[1].sess.losses);
    }
}

int main(void) {
    testDomination();
    testAutoball();
    testFlagCapture(GT_CTF, TEAM_RED, PW_BLUEFLAG);
    testFlagCapture(GT_CTF4, TEAM_GREEN, PW_BLUEFLAG);
    testKoth();
    testLastCarStanding();
    testRaceOutcomes();
    return 0;
}
'''
    compile_run(harness)
