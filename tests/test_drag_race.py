"""Exercise Drag Race's production start, finish and heat rules without a renderer."""
import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]


def test_drag_race_rules():
    source = ROOT / "engine/code/game/g_drag.c"
    harness = '#include "' + source.as_posix() + '"\n'
    harness += r'''
#include <assert.h>

level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
vmCvar_t g_gametype;
float CP_M_2_QU = 35.66f;
static gclient_t clients[MAX_CLIENTS];
static gentity_t startGate, finishGate, starter;
static int exits, rankCalls, statuses, positionPublications, publishedPositions[2];
static char lastExit[128];
static int lightReplays;
static char lastLight[64];

char *QDECL va(char *format, ...) {
    static char buffer[2048]; va_list ap;
    va_start(ap, format); vsnprintf(buffer, sizeof(buffer), format, ap); va_end(ap);
    return buffer;
}
void trap_SendServerCommand(int recipient, const char *text) {
    if (strncmp(text, "dragStatus ", 11) == 0) statuses++;
    if (recipient==2 && strncmp(text,"rc ",3)==0) {
        lightReplays++;
        snprintf(lastLight,sizeof(lastLight),"%s",text);
    }
}
void CalculateRanks(void) { rankCalls++; }
void Cmd_RacePositions_f(void) {
    positionPublications++;
    publishedPositions[0]=clients[0].ps.stats[STAT_POSITION];
    publishedPositions[1]=clients[1].ps.stats[STAT_POSITION];
}
void LogExit(const char *text) { exits++; snprintf(lastExit, sizeof(lastExit), "%s", text); }
vec_t VectorNormalize(vec3_t v) {
    float length = VectorLength(v);
    if (length > 0.0f) { v[0]/=length; v[1]/=length; v[2]/=length; }
    return length;
}

static void resetHeat(int count) {
    int i;
    memset(&level, 0, sizeof(level));
    memset(g_entities, 0, sizeof(g_entities));
    memset(clients, 0, sizeof(clients));
    memset(&startGate, 0, sizeof(startGate));
    memset(&finishGate, 0, sizeof(finishGate));
    memset(&starter, 0, sizeof(starter));
    exits = rankCalls = statuses = positionPublications = 0;
    lightReplays=0; lastLight[0]='\0';
    publishedPositions[0] = publishedPositions[1] = 0;
    g_gametype.integer = GT_DRAG;
    level.time = 1000; level.maxclients = 2; level.clients = clients;
    VectorSet(startGate.r.absmin, 92,-320,-8);
    VectorSet(startGate.r.absmax, 108,320,192);
    VectorSet(finishGate.r.absmin, 992,-320,-8);
    VectorSet(finishGate.r.absmax, 1008,320,192);
    finishGate.number = 2;
    level.startEnt = &startGate; level.finishEnt = &finishGate;
    G_DragInit();
    for (i=0;i<count;i++) {
        gentity_t *car = &g_entities[i];
        car->inuse = qtrue; car->client = &clients[i];
        car->s.number = car->s.clientNum = i;
        clients[i].pers.connected = CON_CONNECTED;
        clients[i].sess.sessionTeam = TEAM_FREE;
        clients[i].ps.commandTime = 900;
        clients[i].ps.stats[STAT_HEALTH] = 100;
        VectorSet(clients[i].ps.origin, 0, i ? 160 : -160, 28);
        G_DragClientSpawn(car);
    }
}

static usercmd_t input(int clientNum, int time, int throttle) {
    usercmd_t cmd;
    memset(&cmd,0,sizeof(cmd));
    cmd.serverTime = time; cmd.forwardmove = throttle;
    cmd.buttons = BUTTON_ATTACK | BUTTON_USE_HOLDABLE;
    G_DragFilterCommand(&g_entities[clientNum], &cmd);
    return cmd;
}

static void stage(int count) {
    int i;
    for (i=0;i<count;i++) {
        usercmd_t cmd = input(i, level.time, 64);
        assert(cmd.forwardmove == 64 && cmd.buttons == BUTTON_HANDBRAKE);
        assert(clients[i].car.gear==0 && clients[i].ps.stats[STAT_GEAR]==0);
    }
    G_DragStarterThink(&starter);
    assert(level.raceState == RACE_STATE_COUNTDOWN);
    assert(dragGreenTime >= 4000 && dragGreenTime <= 5000);
    assert(!level.startRaceTime && !G_DragCanJoin());
}

static usercmd_t launch(int clientNum, int time) {
    input(clientNum,time,127);
    assert(G_DragTryGearChange(&g_entities[clientNum],1,time));
    return input(clientNum,time,127);
}

static void spawnViewer(void) {
    gentity_t *viewer=&g_entities[2];
    viewer->inuse=qtrue; viewer->client=&clients[2];
    viewer->s.number=viewer->s.clientNum=2;
    clients[2].pers.connected=CON_CONNECTED;
    clients[2].sess.sessionTeam=TEAM_SPECTATOR;
    G_DragClientSpawn(viewer);
}

static void finishAt(int clientNum, int crossingTime, float speed) {
    gentity_t *car = &g_entities[clientNum];
    vec3_t before;
    car->number = finishGate.number;
    VectorSet(clients[clientNum].ps.velocity, speed,0,0);
    VectorSet(before,800,clients[clientNum].ps.origin[1],28);
    clients[clientNum].ps.origin[0] = 900;
    clients[clientNum].ps.commandTime = crossingTime - 100;
    G_DragPostMove(car,before,crossingTime-200);
    VectorSet(before,900,clients[clientNum].ps.origin[1],28);
    clients[clientNum].ps.origin[0] = 1100;
    clients[clientNum].ps.commandTime = crossingTime + 50;
    G_DragPostMove(car,before,crossingTime-50);
}

int main(void) {
    int green;
    usercmd_t cmd;

    /* A spectator arriving after a countdown message receives the current
       red stage, including the variable final hold, without a global replay. */
    resetHeat(2); stage(2); green=dragGreenTime;
    spawnViewer();
    assert(lightReplays==1 && !strcmp(lastLight,"rc \"3 - HOLD RPM IN N\" 3"));
    level.time=dragCountdownTime+1000; G_DragStarterThink(&starter);
    spawnViewer();
    assert(lightReplays==2 && !strcmp(lastLight,"rc \"2 - HOLD RPM IN N\" 2"));
    level.time=dragCountdownTime+2000; G_DragStarterThink(&starter);
    spawnViewer();
    assert(lightReplays==3 && !strcmp(lastLight,"rc \"1 - HOLD RPM IN N\" 1"));
    level.time=green-1; spawnViewer();
    assert(lightReplays==4 && !strcmp(lastLight,"rc \"1 - HOLD RPM IN N\" 1"));
    level.time=green; G_DragStarterThink(&starter); spawnViewer();
    assert(lightReplays==5 && !strcmp(lastLight,"rc \"GO!\" 0"));
    level.time=green+4000; spawnViewer();
    assert(lightReplays==5); /* No stale green replay after the lamps expire. */

    resetHeat(2); stage(2);
    G_DragClientDisconnect(1); spawnViewer();
    assert(!lightReplays); /* A cancelled countdown has no stage to replay. */

    resetHeat(2);
    assert(G_DragCanJoin());
    assert(clients[0].car.gear==0 && clients[1].car.gear==0);
    assert(G_DragTryGearChange(&g_entities[0],1,1000));
    assert(G_DragTryGearChange(&g_entities[0],-1,1000));
    assert(clients[0].car.gear==0); /* neither first nor reverse bypasses staging */
    cmd=input(0,1000,0);
    assert(cmd.forwardmove==0 && cmd.buttons==BUTTON_HANDBRAKE);
    input(1,1000,64);
    G_DragStarterThink(&starter);
    assert(!dragCountdownTime); /* every connected driver must stage */
    stage(2); green=dragGreenTime;
    assert(clients[0].pers.transmissionMode==TR_MANUAL);
    assert(clients[0].ps.stats[STAT_HOLDABLE_ITEM]==0);
    level.time=green-1;
    cmd=input(0,green-1,0);
    assert(dragClients[0].phase==DRAG_COUNTDOWN); /* lifting gas is not a false start */
    assert(G_DragTryGearChange(&g_entities[0],1,green-1));
    cmd=input(0,green-1,127);
    assert(dragClients[0].phase==DRAG_FALSE_START && cmd.forwardmove==0);
    input(0,green+20,127);
    G_DragTryGearChange(&g_entities[0],1,green+20);
    assert(dragClients[0].phase==DRAG_FALSE_START); /* cannot undo by re-staging */
    level.time=green+150;
    clients[1].car.rpm=3600;
    cmd=launch(1,green+150);
    assert(level.startRaceTime==green && dragClients[1].phase==DRAG_RUNNING);
    assert(dragClients[1].launchTime-green==150);
    assert(clients[1].car.gear==1 && clients[1].ps.stats[STAT_GEAR]==1);
    assert(clients[1].car.rpm==3600); /* staged RPM survives the shift */
    assert(clients[1].ps.commandTime==green+150); /* no pre-shift acceleration */
    assert(cmd.forwardmove==127 && !(cmd.buttons&(BUTTON_ATTACK|BUTTON_USE_HOLDABLE)));
    finishAt(1,green+1100,1000);
    assert(dragClients[1].phase==DRAG_FINISHED);
    assert(dragClients[1].finishTime==green+1100);
    assert(dragClients[1].trapSpeed==(int)(1000*36.0f/CP_M_2_QU+0.5f));
    assert(G_DragCompare(1,0)<0); /* DSQ cannot win with an earlier finish marker */
    G_DragRunFrame();
    assert(level.winnerNumber==1 && clients[1].ps.stats[STAT_POSITION]==1);
    assert(positionPublications==1 && publishedPositions[1]==1 && publishedPositions[0]==2);
    assert(rankCalls==1); /* final result and changed positions share one rank update */
    level.time+=3000;
    assert(G_DragCheckExit() && exits==1);

    resetHeat(2); stage(2); green=dragGreenTime;
    level.time=green-10;
    cmd=input(0,green+5,127); /* legal future timestamp, not early movement */
    assert(G_DragTryGearChange(&g_entities[0],1,green+5));
    assert(!level.startRaceTime && clients[0].car.gear==0 && cmd.buttons==BUTTON_HANDBRAKE);
    assert(G_DragTryGearChange(&g_entities[0],-1,green+6));
    assert(G_DragTryGearChange(&g_entities[0],1,green+8));
    assert(dragClients[0].launchTime==green+5); /* first request cannot be overwritten */
    level.time=green+100;
    input(0,green+100,127); launch(1,green+100);
    assert(dragClients[0].launchTime==green+5 && clients[0].car.gear==1);
    /* Process the slower car first in the same frame. Correct timing wins. */
    finishAt(0,green+2200,1000); finishAt(1,green+2150,1000);
    G_DragRunFrame();
    assert(level.winnerNumber==1 && G_DragCompare(1,0)<0);
    assert(clients[0].ps.stats[STAT_POSITION]==2 && clients[1].ps.stats[STAT_POSITION]==1);

    resetHeat(2); stage(2); green=dragGreenTime; level.time=green+100;
    launch(0,green+100); launch(1,green+100);
    finishAt(0,green+2000,1000); finishAt(1,green+2000,1000);
    G_DragRunFrame();
    assert(level.winnerNumber==-1 && G_DragCompare(0,1)==0);
    assert(clients[0].ps.stats[STAT_POSITION]==1 && clients[1].ps.stats[STAT_POSITION]==1);

    resetHeat(1); stage(1); green=dragGreenTime; level.time=green+100;
    G_DragRunFrame();
    cmd=input(0,green+100,127);
    assert(cmd.forwardmove==127 && cmd.buttons==BUTTON_HANDBRAKE && dragClients[0].phase==DRAG_COUNTDOWN);
    assert(clients[0].car.gear==0); /* pre-held gas cannot launch automatically */
    /* Late delivery does not turn an input timestamp before green legal. */
    assert(G_DragTryGearChange(&g_entities[0],1,green-1));
    cmd=input(0,green+100,127);
    assert(cmd.forwardmove==0 && dragClients[0].phase==DRAG_FALSE_START);

    resetHeat(1); stage(1); green=dragGreenTime; level.time=green+20;
    launch(0,green+20);
    clients[0].ps.origin[1]=0;
    clients[0].ps.commandTime=green+120;
    { vec3_t before={0,-160,28}; G_DragPostMove(&g_entities[0],before,green+20); }
    assert(dragClients[0].phase==DRAG_DNF); /* crossing the center line forfeits */
    G_DragClientSpawn(&g_entities[0]);
    assert(dragClients[0].phase==DRAG_DNF); /* no reset/retry exploit */
    G_DragRunFrame(); level.time+=3000; G_DragCheckExit();
    assert(exits==1 && level.winnerNumber==-1);

    resetHeat(2); stage(2);
    G_DragClientDisconnect(1);
    g_entities[1].inuse=qfalse;
    assert(!dragCountdownTime && !dragGreenTime && G_DragCanJoin());
    G_DragStarterThink(&starter); /* remaining player can explicitly solo-test */
    assert(dragCountdownTime && dragClients[0].participant);

    resetHeat(1); stage(1);
    clients[0].sess.sessionTeam=TEAM_SPECTATOR;
    G_DragClientSpawn(&g_entities[0]);
    assert(!dragCountdownTime && G_DragCanJoin());

    resetHeat(1); stage(1); green=dragGreenTime; level.time=green+100;
    launch(0,green+100);
    clients[0].sess.sessionTeam=TEAM_SPECTATOR;
    G_DragClientSpawn(&g_entities[0]);
    assert(dragClients[0].participant && dragClients[0].phase==DRAG_DNF);
    G_DragRunFrame(); level.time+=3000; G_DragCheckExit();
    assert(exits==1); /* leaving a solo heat cannot hang it forever */

    resetHeat(2); stage(2); green=dragGreenTime; level.time=green+100;
    launch(0,green+100); launch(1,green+100);
    G_DragRunFrame();
    assert(positionPublications==1 && rankCalls==1);
    {
        vec3_t before0={0,-160,28}, before1={0,160,28};
        clients[0].ps.origin[0]=100; clients[1].ps.origin[0]=50;
        clients[0].ps.commandTime=clients[1].ps.commandTime=green+200;
        G_DragPostMove(&g_entities[0],before0,green+100);
        G_DragPostMove(&g_entities[1],before1,green+100);
    }
    G_DragRunFrame();
    assert(positionPublications==2 && publishedPositions[0]==1 && publishedPositions[1]==2);
    G_DragRunFrame();
    assert(positionPublications==2 && rankCalls==2); /* unchanged frames stay quiet */
    {
        vec3_t before={50,160,28};
        clients[1].ps.origin[0]=150; clients[1].ps.commandTime=green+300;
        G_DragPostMove(&g_entities[1],before,green+200);
    }
    G_DragRunFrame();
    assert(positionPublications==3 && rankCalls==3);
    assert(publishedPositions[0]==2 && publishedPositions[1]==1); /* live overtake published */

    resetHeat(1); stage(1); green=dragGreenTime;
    level.time=green+60000;
    G_DragRunFrame();
    assert(dragClients[0].phase==DRAG_DNF); /* stalled heats cannot hang forever */

    resetHeat(1); level.finishEnt=NULL;
    G_DragStarterThink(&starter); G_DragCheckExit();
    assert(exits==1 && strstr(lastExit,"Invalid"));

    resetHeat(1); g_gametype.integer=GT_RACING;
    memset(&cmd,0,sizeof(cmd)); cmd.forwardmove=127;
    G_DragFilterCommand(&g_entities[0],&cmd);
    assert(cmd.forwardmove==127 && !G_DragCheckExit());
    assert(statuses>0);
    return 0;
}
'''
    compiler = shutil.which("gcc") or "C:/msys64/mingw64/bin/gcc.exe"
    with tempfile.TemporaryDirectory(dir=ROOT / "tests") as directory:
        c = pathlib.Path(directory) / "drag.c"
        binary = pathlib.Path(directory) / "drag.exe"
        c.write_text(harness)
        subprocess.run([compiler, '-DARCH_STRING="test"', str(c), "-o", str(binary), "-lm"], check=True)
        subprocess.run([str(binary)], check=True, timeout=5)
