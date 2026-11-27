"""Exercise race feedback and finish timing without a running renderer/server."""
import pathlib

from test_plate_and_menu_regressions import CODE, compile_run

ROOT = pathlib.Path(__file__).resolve().parents[1]


def function(source, marker):
    start = source.index(marker)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def test_race_feedback():
    server = (ROOT / 'engine/code/game/g_rally_mapents.c').read_text()
    rules = (ROOT / 'engine/code/game/g_main.c').read_text()
    rally = (ROOT / 'engine/code/game/g_rally_tools.c').read_text()
    shared = (ROOT / 'engine/code/qcommon/q_shared.c').read_text()
    hud = (ROOT / 'engine/code/cgame/cg_hud_racing.c').read_text()
    harness = '#include "' + (ROOT / 'engine/code/game/g_local.h').as_posix() + '"\n'
    harness += r'''
#include <assert.h>
#include <limits.h>
level_locals_t level;
vmCvar_t g_gametype, g_finishRaceDelay, g_timeTrialFinishDelay;
static int notices, recipient;
static char message[256], broadcast[256];
static int recordedTeamTime, submittedTeamTime;
void QDECL G_LogPrintf(const char *format, ...) {}
void trap_SetConfigstring(int index, const char *value) {}
void G_Profile_FlushIfDirty(void) {}
qboolean G_Profile_IsDirty(void) { return qfalse; }
static void G_RecordMatchOutcome(void) { recordedTeamTime = level.teamTimes[TEAM_RED]; }
static void G_LadderSubmitMatchReport(const char *reason) { submittedTeamTime = level.teamTimes[TEAM_RED]; }
char *QDECL va(char *format, ...) {
    static char result[1024]; va_list ap;
    va_start(ap,format); vsnprintf(result,sizeof(result),format,ap); va_end(ap);
    return result;
}
void trap_SendServerCommand(int client, const char *text) {
    notices++; recipient=client; snprintf(message,sizeof(message),"%s",text);
    if (client == -1) snprintf(broadcast, sizeof(broadcast), "%s", text);
}
'''
    harness += function(server, 'static void G_RallyWarnMissedCheckpoint(') + '\n'
    harness += function(server, 'static const char *G_RallyPlaceString(') + '\n'
    harness += function(server, 'static void G_RallyReportFinish(') + '\n'
    harness += function(server, 'static void G_RallyUpdateTeamTime(') + '\n'
    harness += function(server, 'void G_RallyUpdateAllTeamTimes(') + '\n'
    harness += function(rules, 'void LogExit( const char *string ) {') + '\n'
    harness += function(rules, 'static qboolean G_RaceFinishDelayElapsed(') + '\n'
    harness += function(rally, 'qboolean isRallyDMRace(') + '\n'
    harness += function(shared, 'float Com_Clamp(') + '\n'
    harness += function(hud, 'static qboolean CG_IsMovingAwayFromCheckpoint(') + '\n'
    harness += r'''
static void TestTeamTimesAtRaceEnd(void) {
    static gclient_t drivers[3];

    level.clients = drivers;
    level.maxclients = ARRAY_LEN(drivers);
    level.startRaceTime = 10000;
    level.time = 60000;
    drivers[0].pers.connected = drivers[1].pers.connected = CON_CONNECTED;
    drivers[0].sess.sessionTeam = drivers[1].sess.sessionTeam = TEAM_RED;
    drivers[0].finishRaceTime = 40000;
    drivers[2].sess.sessionTeam = TEAM_RED; /* Disconnected drivers are excluded. */

    g_gametype.integer = GT_TEAM_RACING;
    G_RallyUpdateAllTeamTimes();
    assert(level.teamTimes[TEAM_RED] == 40000);

    g_gametype.integer = GT_TEAM_RACING_DM;
    level.teamScores[TEAM_RED] = 4;
    level.time = 70000;
    LogExit("Race finished.");
    assert(level.intermissionQueued == 70000);
    assert(recordedTeamTime == 43000 && submittedTeamTime == 43000);
    assert(!strcmp(broadcast, "times 43000 0 0 0"));
    assert(!drivers[1].finishRaceTime);

    notices = 0;
    level.time = 70500;
    G_RallyUpdateAllTeamTimes();
    level.intermissionQueued = 0;
    level.intermissiontime = 71000;
    level.time = 80000;
    G_RallyUpdateAllTeamTimes();
    assert(level.teamTimes[TEAM_RED] == 43000 && !notices);
}

static void TestFinishDelay(void) {
    level.finishRaceTime = 20000;
    g_finishRaceDelay.integer = 30;
    g_timeTrialFinishDelay.integer = 10;
    g_gametype.integer = GT_SINGLE_PLAYER;
    level.time = 30000;
    assert(!G_RaceFinishDelayElapsed());
    level.time++;
    assert(G_RaceFinishDelayElapsed());

    g_gametype.integer = GT_TEAM_RACING;
    assert(!G_RaceFinishDelayElapsed());
    assert(g_finishRaceDelay.integer == 30);
    level.time = 50001;
    assert(G_RaceFinishDelayElapsed());

    g_gametype.integer = GT_SINGLE_PLAYER;
    g_timeTrialFinishDelay.integer = 0;
    level.time = 30001;
    assert(!G_RaceFinishDelayElapsed());
    level.time = 50001;
    assert(G_RaceFinishDelayElapsed());
    g_finishRaceDelay.integer = INT_MAX;
    assert(!G_RaceFinishDelayElapsed());
}

static void TestFinishMessages(gentity_t *car) {
    const int pendingModes[] = {GT_RACING_DM, GT_TEAM_RACING, GT_TEAM_RACING_DM};
    int i;

    car->s.clientNum = 7;
    car->client->finishRaceTime = 25000;
    for (i = 0; i < ARRAY_LEN(pendingModes); i++) {
        g_gametype.integer = pendingModes[i];
        level.finishRaceTime = 0;
        notices = 0;
        G_RallyReportFinish(car);
        assert(level.finishRaceTime == 25000 && level.winnerNumber == 7);
        assert(notices == 2 && recipient == 7);
        assert(strstr(broadcast, "finished the race") && strstr(message, "Race finished"));
        car->client->ps.stats[STAT_POSITION] = 2;
        G_RallyReportFinish(car);
        assert(!strstr(broadcast, "place"));
    }

    level.finishRaceTime = 0;
    g_gametype.integer = GT_GHOST;
    notices = 0;
    G_RallyReportFinish(car);
    assert(notices == 1 && recipient == -1 && strstr(broadcast, "finished the race"));

    level.finishRaceTime = 0;
    g_gametype.integer = GT_SPRINT;
    notices = 0;
    G_RallyReportFinish(car);
    assert(notices == 2 && recipient == 7 && strstr(message, "won the race"));
    car->client->ps.stats[STAT_POSITION] = 2;
    G_RallyReportFinish(car);
    assert(strstr(broadcast, "second place"));
}

int main(void) {
    static gentity_t car, gate;
    static gclient_t client;
    vec3_t velocity={0,0,0}, target={100,0,0};
    car.client=&client; car.s.number=7; car.number=2;
    client.ps.pm_type=PM_NORMAL; gate.number=4;
    level.time=10000;
    G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==0);
    level.startRaceTime=11000;
    G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==0);
    level.startRaceTime=1000;
    // The shared start/finish carries the last gate number, even at lap start.
    level.finishEnt=&gate; car.number=1; gate.number=20;
    client.lapStartTime=1000; client.lastCheckpointTime=1000;
    G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==0);
    assert(client.checkpointWarningTime==0 && car.number==1);
    level.time+=5000; // repeated/slow start-line contact must remain quiet
    G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==0);
    // The same protection also covers the start of subsequent laps.
    car.currentLap=2; client.lapStartTime=client.lastCheckpointTime=level.time-500;
    G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==0);
    // A separate sprint finish must still warn about missing checkpoint 1.
    level.sprintTrack=qtrue;
    G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==1);
    assert(strstr(message,"checkpoint 1"));
    level.sprintTrack=qfalse; level.finishEnt=NULL;
    notices=0; client.checkpointWarningTime=0;
    client.lastCheckpointTime=0; car.number=2;
    gate.number=1; G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==0);
    gate.number=2; G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==0);
    gate.number=4; G_RallyWarnMissedCheckpoint(&gate,&car);
    assert(notices==1 && recipient==7 && strstr(message,"checkpoint 2"));
    assert(strchr(message,'\n') && !strstr(message,"\\n"));
    assert(car.number==2 && client.lastCheckpointTime==0);
    G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==1);
    level.time+=4000;
    G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==2);
    level.time+=4000; client.finishRaceTime=level.time;
    G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==2);
    client.finishRaceTime=0; car.r.svFlags=SVF_BOT;
    G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==2);
    car.r.svFlags=0; client.sess.sessionTeam=TEAM_SPECTATOR;
    G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==2);
    client.sess.sessionTeam=TEAM_FREE; client.ps.pm_type=PM_DEAD;
    G_RallyWarnMissedCheckpoint(&gate,&car); assert(notices==2);
    assert(!CG_IsMovingAwayFromCheckpoint(velocity,target,180));
    velocity[2]=200; assert(!CG_IsMovingAwayFromCheckpoint(velocity,target,180));
    velocity[0]=40; assert(!CG_IsMovingAwayFromCheckpoint(velocity,target,180));
    velocity[0]=-100;
    assert(CG_IsMovingAwayFromCheckpoint(velocity,target,180));
    assert(CG_IsMovingAwayFromCheckpoint(velocity,target,-180));
    assert(!CG_IsMovingAwayFromCheckpoint(velocity,target,100));
    assert(!CG_IsMovingAwayFromCheckpoint(velocity,target,0));
    target[0]=0; assert(!CG_IsMovingAwayFromCheckpoint(velocity,target,180));
    TestFinishDelay();
    TestFinishMessages(&car);
    TestTeamTimesAtRaceEnd();
    return 0;
}
'''
    compile_run(harness)
