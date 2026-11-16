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
static int lightReplays, statusMismatches;
static char lastCp[2][160];
static char lastLight[64];
static char userIds[MAX_CLIENTS][64];

void trap_GetUserinfo(int clientNum,char *out,int size) { snprintf(out,size,"%d",clientNum); }
static char botSkill[8]="5";
char *Info_ValueForKey(const char *info,const char *key) {
    int n=atoi(info);
    if(!strcmp(key,"cl_uuid")) return userIds[n];
    if(!strcmp(key,"name")) return clients[n].pers.netname;
    if(!strcmp(key,"skill")) return botSkill;
    return "";
}
float vectoyaw(const vec3_t v) { return atan2(v[1],v[0])*180.0f/M_PI; }
float WheelAngle(float carYaw,float viewYaw) { return viewYaw-carYaw; }
void Q_strncpyz(char *out,const char *in,int size) { snprintf(out,size,"%s",in); }
char *Q_CleanStr(char *text) { return text; }
int QDECL Com_sprintf(char *out,int size,const char *format,...) {
    int n; va_list ap; va_start(ap,format); n=vsnprintf(out,size,format,ap); va_end(ap); return n;
}
qboolean G_Profile_GetUUID(char *out,int size) { out[0]=0; return qfalse; }

char *QDECL va(char *format, ...) {
    static char buffer[2048]; va_list ap;
    va_start(ap, format); vsnprintf(buffer, sizeof(buffer), format, ap); va_end(ap);
    return buffer;
}
static char lastMatch[256], lastCpAll[256];
void trap_SendServerCommand(int recipient, const char *text) {
    if (strncmp(text,"dragMatch ",10)==0 && recipient==-1) snprintf(lastMatch,sizeof(lastMatch),"%s",text);
    if (strncmp(text,"cp ",3)==0 && recipient==-1) snprintf(lastCpAll,sizeof(lastCpAll),"%s",text);
    if (strncmp(text, "dragStatus ", 11) == 0) {
        int client, phase, countdown, green;
        statuses++;
        assert(sscanf(text+11,"%d %d %d %d",&client,&phase,&countdown,&green)==4);
        /* Status always carries the current heat and the green that has
           actually happened: never the scheduled, still secret, green. */
        if (countdown!=dragCountdownTime || green!=dragGreenTime) statusMismatches++;
        if (green && green==dragScheduledGreenTime && !level.startRaceTime) statusMismatches++;
    }
    if (recipient>=0 && recipient<2 && strncmp(text,"cp ",3)==0)
        snprintf(lastCp[recipient],sizeof(lastCp[recipient]),"%s",text);
    if (recipient==2 && strncmp(text,"rc ",3)==0) {
        lightReplays++;
        snprintf(lastLight,sizeof(lastLight),"%s",text);
    }
}
static int engineBangs;
int G_SoundIndex(char *name) { assert(strstr(name,"drag_engine_blow")); return 7; }
void G_Sound(gentity_t *ent,int channel,int sound) { assert(sound==7); engineBangs++; }
void QDECL G_LogPrintf(const char *fmt,...) {}
vmCvar_t g_dragBestOf, g_dragTournament;
static int respawns, consoleCommands; static char lastConsole[256];
void Q_strcat(char *dest,int size,const char *src) { strncat(dest,src,size-strlen(dest)-1); }
void ClientSpawn(gentity_t *ent) {
    /* back to the own grid slot, as the spawn point selection does */
    respawns++;
    VectorSet(ent->client->ps.origin, 0, ent->s.number ? 160 : -160, 28);
    VectorClear(ent->client->ps.velocity);
    ent->client->ps.eFlags=0;
    G_DragClientSpawn(ent);
}
const char *G_DragNextRivalName(const char *previous) { return "Josh"; }
static int tourneyLineups, tourneyMatchOvers, tourneyWinner=-2, tourneyLineupReady=1;
void G_DragTourneyInit(void) {}
void G_DragTourneyPublish(int recipient) {}
qboolean G_DragTourneyLineup(void) { tourneyLineups++; return tourneyLineupReady; }
void G_DragTourneyMatchOver(int winnerClient,const int *wins) { tourneyMatchOvers++; tourneyWinner=winnerClient; }
void trap_SendConsoleCommand(int when,const char *text) { consoleCommands++; snprintf(lastConsole,sizeof(lastConsole),"%s",text); }
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
    consoleCommands = 0; lastConsole[0]=lastMatch[0]=lastCpAll[0]='\0';
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
    for(i=0;i<MAX_CLIENTS;i++) snprintf(userIds[i],sizeof(userIds[i]),"driver-%d",i);
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
    assert(dragScheduledGreenTime >= 4000 && dragScheduledGreenTime <= 5000);
    assert(!dragGreenTime); /* the coming green is not published */
    assert(!level.startRaceTime && !G_DragCanJoin());
}

/* Run the server frame of the scheduled green. */
static int goGreen(void) {
    level.time = dragScheduledGreenTime;
    G_DragStarterThink(&starter);
    assert(dragGreenTime == level.time && level.startRaceTime == level.time);
    return dragGreenTime;
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

/* One heat of a match: both staged, green, launches and finishes relative
   to green (finish<0: false start). Returns green. */
static int playHeat(int count, int finish0, int finish1) {
    int i, green, finish[2];
    finish[0]=finish0; finish[1]=finish1;
    for (i=0;i<count;i++) input(i,level.time,64);
    G_DragStarterThink(&starter);
    assert(dragCountdownTime);
    green=goGreen(); level.time=green+150;
    for (i=0;i<count;i++) {
        if (finish[i]<0) { assert(G_DragTryGearChange(&g_entities[i],1,green-5)); continue; }
        launch(i,green+150);
    }
    for (i=0;i<count;i++) if (finish[i]>=0) finishAt(i,green+finish[i],1000);
    G_DragRunFrame();
    return green;
}

static void nextHeat(void) {
    level.time=dragAllDoneTime+DRAG_NEXT_HEAT_MS;
    G_DragCheckExit();
}

int main(void) {
    int green;
    usercmd_t cmd;
    ladderPlayerPayload_t result;

    /* Ladder facts come from actual Drag state, never the finish marker
       that also makes a disqualified driver enter the observer delay. */
    resetHeat(2); stage(2); green=goGreen();
    assert(G_DragLadderDuel() && G_DragLadderIdentityMatches(0));
    level.time=green+100;
    launch(0,green+100);
    assert(G_DragTryGearChange(&g_entities[1],1,green-1));
    finishAt(0,green+2000,1000); G_DragRunFrame();
    assert(G_DragPopulateLadderPlayer(0,&result));
    assert(!strcmp(result.playerId,"driver-0") && !strcmp(result.dragStatus,"finished"));
    assert(result.reactionMs==100 && result.elapsedMs==1900 && result.totalRaceMs==2000);
    assert(result.position==1 && result.trapSpeedKph>100 && !result.bestLapMs && !result.lapCount);
    assert(G_DragPopulateLadderPlayer(1,&result));
    assert(!strcmp(result.dragStatus,"false_start") && clients[1].finishRaceTime>0);
    assert(!result.reactionMs && !result.elapsedMs && !result.totalRaceMs && !result.position && !result.finishRaceTime);
    strcpy(userIds[1],"new-spectator"); clients[1].sess.sessionTeam=TEAM_SPECTATOR;
    assert(!G_DragLadderIdentityMatches(1));
    assert(G_DragPopulateLadderPlayer(1,&result) && !strcmp(result.playerId,"driver-1"));
    assert(!G_DragPopulateLadderPlayer(2,&result)); /* spectators were not armed */

    resetHeat(2); stage(2); green=goGreen(); level.time=green+100;
    launch(0,green+100); launch(1,green+100);
    G_DragClientDisconnect(1); clients[1].pers.connected=CON_DISCONNECTED;
    assert(G_DragPopulateLadderPlayer(1,&result) && !strcmp(result.dragStatus,"dnf"));
    assert(!result.totalRaceMs && G_DragLadderDuel()); /* forfeit still belongs to this duel */
    resetHeat(1); stage(1); green=goGreen(); level.time=green+100;
    launch(0,green+100); finishAt(0,green+2000,1000); G_DragRunFrame();
    assert(!G_DragLadderDuel() && G_DragPopulateLadderPlayer(0,&result));
    assert(result.position==1 && !strcmp(result.dragStatus,"finished"));
    resetHeat(2); stage(2); G_DragClientDisconnect(1);
    assert(!G_DragPopulateLadderPlayer(0,&result) && !G_DragLadderDuel());
    resetHeat(2); g_entities[1].r.svFlags=SVF_BOT; stage(2);
    assert(G_DragLadderDuel() && G_DragPopulateLadderPlayer(1,&result) && result.isBot); /* bots race too */

    /* A spectator arriving after a countdown message receives the current
       red stage, including the variable final hold, without a global replay. */
    resetHeat(2); stage(2); green=dragScheduledGreenTime;
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
    stage(2); green=dragScheduledGreenTime;
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
    assert(strstr(lastCp[0],"before GREEN"));
    goGreen(); level.time=green+150;
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

    /* Before the green frame no request is legal, even one whose timestamp
       lies after the scheduled green: that green is not public knowledge. */
    resetHeat(2); stage(2); green=dragScheduledGreenTime;
    level.time=green-10;
    cmd=input(0,green+DRAG_MIN_REACTION_MS+5,127);
    assert(G_DragTryGearChange(&g_entities[0],1,green+DRAG_MIN_REACTION_MS+5));
    assert(!level.startRaceTime && dragClients[0].phase==DRAG_FALSE_START);
    assert(clients[0].car.gear==0);
    assert(G_DragTryGearChange(&g_entities[0],1,green+DRAG_MIN_REACTION_MS+8));
    assert(dragClients[0].launchTime==green+DRAG_MIN_REACTION_MS+5); /* first request is final */
    goGreen(); level.time=green+130; launch(1,green+130);
    assert(dragClients[1].phase==DRAG_RUNNING && dragClients[0].phase==DRAG_FALSE_START);

    /* Green is the server frame that announces it, not the scheduled time. */
    resetHeat(1); stage(1); green=dragScheduledGreenTime;
    level.time=green+37; G_DragRunFrame();
    assert(dragGreenTime==green+37 && level.startRaceTime==green+37);
    assert(G_DragTryGearChange(&g_entities[0],1,green+37+DRAG_MIN_REACTION_MS));
    assert(dragClients[0].phase==DRAG_RUNNING);

    /* Reacting faster than the minimum is a false start; exactly the minimum
       is legal (checked in the first heat above, 0.100 s). */
    resetHeat(2); stage(2); green=goGreen(); level.time=green+120;
    assert(G_DragTryGearChange(&g_entities[0],1,green+DRAG_MIN_REACTION_MS-1));
    assert(dragClients[0].phase==DRAG_FALSE_START);
    assert(strstr(lastCp[0],"reaction 0.099 s, minimum 0.100 s"));
    assert(G_DragPopulateLadderPlayer(0,&result) && !strcmp(result.dragStatus,"false_start"));
    assert(!result.reactionMs && !result.totalRaceMs && !result.position);
    assert(G_DragTryGearChange(&g_entities[1],1,green+DRAG_MIN_REACTION_MS));
    assert(dragClients[1].phase==DRAG_RUNNING);

    /* A gearUp timestamp cannot predate the movement already received:
       a backdated stamp is moved up to the last accepted usercmd. */
    resetHeat(1); stage(1); green=goGreen(); level.time=green+300;
    clients[0].pers.cmd.serverTime=green+260;
    assert(G_DragTryGearChange(&g_entities[0],1,green+DRAG_MIN_REACTION_MS));
    assert(dragClients[0].phase==DRAG_RUNNING && dragClients[0].launchTime==green+260);
    /* ...and a stamp before green stays a false start after being moved up
       only when the received movement was itself before green. */
    resetHeat(1); stage(1); green=dragScheduledGreenTime; level.time=green-20;
    clients[0].pers.cmd.serverTime=green-30;
    assert(G_DragTryGearChange(&g_entities[0],1,green-40));
    assert(dragClients[0].phase==DRAG_FALSE_START && dragClients[0].launchTime==green-30);

    resetHeat(2); stage(2); green=goGreen(); level.time=green+100;
    input(0,green+100,127); launch(0,green+100); launch(1,green+100);
    /* Process the slower car first in the same frame. Correct timing wins. */
    finishAt(0,green+2200,1000); finishAt(1,green+2150,1000);
    G_DragRunFrame();
    assert(level.winnerNumber==1 && G_DragCompare(1,0)<0);
    assert(clients[0].ps.stats[STAT_POSITION]==2 && clients[1].ps.stats[STAT_POSITION]==1);

    resetHeat(2); stage(2); green=goGreen(); level.time=green+100;
    launch(0,green+100); launch(1,green+100);
    finishAt(0,green+2000,1000); finishAt(1,green+2000,1000);
    G_DragRunFrame();
    assert(level.winnerNumber==-1 && G_DragCompare(0,1)==0);
    assert(clients[0].ps.stats[STAT_POSITION]==1 && clients[1].ps.stats[STAT_POSITION]==1);

    resetHeat(1); stage(1); green=goGreen(); level.time=green+100;
    G_DragRunFrame();
    cmd=input(0,green+100,127);
    assert(cmd.forwardmove==127 && cmd.buttons==BUTTON_HANDBRAKE && dragClients[0].phase==DRAG_COUNTDOWN);
    assert(clients[0].car.gear==0); /* pre-held gas cannot launch automatically */
    /* Late delivery does not turn an input timestamp before green legal. */
    assert(G_DragTryGearChange(&g_entities[0],1,green-1));
    cmd=input(0,green+100,127);
    assert(cmd.forwardmove==0 && dragClients[0].phase==DRAG_FALSE_START);

    resetHeat(1); stage(1); green=goGreen(); level.time=green+120;
    launch(0,green+120);
    clients[0].ps.origin[1]=0;
    clients[0].ps.commandTime=green+220;
    { vec3_t before={0,-160,28}; G_DragPostMove(&g_entities[0],before,green+120); }
    assert(dragClients[0].phase==DRAG_DNF); /* crossing the center line forfeits */
    G_DragClientSpawn(&g_entities[0]);
    assert(dragClients[0].phase==DRAG_DNF); /* no reset/retry exploit */
    G_DragRunFrame(); level.time+=3000; G_DragCheckExit();
    assert(exits==1 && level.winnerNumber==-1);

    resetHeat(2); stage(2);
    G_DragClientDisconnect(1);
    g_entities[1].inuse=qfalse;
    assert(!dragCountdownTime && !dragScheduledGreenTime && !dragGreenTime && G_DragCanJoin());
    G_DragStarterThink(&starter); /* remaining player can explicitly solo-test */
    assert(dragCountdownTime && dragClients[0].participant);

    resetHeat(1); stage(1);
    clients[0].sess.sessionTeam=TEAM_SPECTATOR;
    G_DragClientSpawn(&g_entities[0]);
    assert(!dragCountdownTime && G_DragCanJoin());

    resetHeat(1); stage(1); green=goGreen(); level.time=green+100;
    launch(0,green+100);
    clients[0].sess.sessionTeam=TEAM_SPECTATOR;
    G_DragClientSpawn(&g_entities[0]);
    assert(dragClients[0].participant && dragClients[0].phase==DRAG_DNF);
    G_DragRunFrame(); level.time+=3000; G_DragCheckExit();
    assert(exits==1); /* leaving a solo heat cannot hang it forever */

    resetHeat(2); stage(2); green=goGreen(); level.time=green+100;
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

    resetHeat(1); stage(1); green=goGreen();
    level.time=green+60000;
    G_DragRunFrame();
    assert(dragClients[0].phase==DRAG_DNF); /* stalled heats cannot hang forever */

    /* The countdown arms the engine; staging alone cannot blow it. */
    resetHeat(2);
    assert(!clients[0].ps.stats[STAT_DRAG_ENGINE]);
    stage(2);
    assert(clients[0].ps.stats[STAT_DRAG_ENGINE]==DRAG_ENGINE_ARMED);
    assert(clients[1].ps.stats[STAT_DRAG_ENGINE]==DRAG_ENGINE_ARMED);
    /* Blown on the limiter during the lights: DNF, smoke, one bang. */
    engineBangs=0;
    clients[0].ps.stats[STAT_DRAG_ENGINE]|=DRAG_ENGINE_BLOWN;
    G_DragRunFrame(); G_DragRunFrame();
    assert(dragClients[0].phase==DRAG_DNF && engineBangs==1);
    assert(clients[0].ps.eFlags & EF_SMOKE_DARK);
    assert(strstr(lastCp[0],"too long on the limiter"));
    assert(dragClients[1].phase==DRAG_COUNTDOWN); /* the opponent races on */
    green=goGreen(); level.time=green+150; launch(1,green+150);
    /* Over-revved by road speed while running. */
    clients[1].ps.stats[STAT_DRAG_ENGINE]|=DRAG_ENGINE_BLOWN|DRAG_ENGINE_OVERREV;
    G_DragRunFrame();
    assert(dragClients[1].phase==DRAG_DNF && engineBangs==2);
    assert(strstr(lastCp[1],"over-revved"));
    assert(G_DragPopulateLadderPlayer(1,&result) && !strcmp(result.dragStatus,"dnf"));
    /* A cancelled countdown disarms the engines of the drivers who stay. */
    resetHeat(2); stage(2);
    G_DragClientDisconnect(1);
    assert(!clients[0].ps.stats[STAT_DRAG_ENGINE]);

    /* A bot races the second lane: it stages with the revs of its plan,
       launches exactly at green + its reaction and shifts on its plan. */
    resetHeat(2); g_entities[1].r.svFlags=SVF_BOT; strcpy(botSkill,"5");
    {
        usercmd_t botCmd; int reaction, launchAt;
        memset(&botCmd,0,sizeof(botCmd));
        assert(G_DragBotInput(1,&botCmd) && dragClients[1].botPlanned);
        assert(!G_DragBotInput(0,&botCmd)); /* humans drive themselves */
        G_DragBotInput(1,&botCmd);
        assert(botCmd.forwardmove>0 && botCmd.buttons==0);
        assert(dragClients[1].botLaunchRpm>=1500 && dragClients[1].botLaunchRpm<DRAG_ENGINE_REDLINE_RPM);
        reaction=dragClients[1].botReactionMs;
        assert(reaction>=120 && reaction<300); /* skill 5 never anticipates */
        input(0,level.time,64); input(1,level.time,botCmd.forwardmove);
        G_DragStarterThink(&starter);
        assert(dragCountdownTime && G_DragLadderDuel()); /* a bot duel is a duel */
        G_DragBotInput(1,&botCmd); /* now on the lane: steer along it */
        assert(botCmd.angles[YAW]==ANGLE2SHORT(0));
        green=goGreen(); launchAt=green+reaction;
        level.time=launchAt-1; clients[1].ps.commandTime=level.time; G_DragRunFrame();
        assert(dragClients[1].phase==DRAG_COUNTDOWN);
        level.time=launchAt+40; clients[1].ps.commandTime=level.time; G_DragRunFrame();
        assert(dragClients[1].phase==DRAG_RUNNING && dragClients[1].launchTime==launchAt);
        assert(clients[1].ps.commandTime==launchAt); /* integrates from its real launch */
        G_DragBotInput(1,&botCmd); assert(botCmd.forwardmove==127);
        clients[1].car.rpm=dragClients[1].botShiftRpm+1;
        G_DragRunFrame(); assert(clients[1].car.gear==1); /* settle after the launch */
        level.time+=300; G_DragRunFrame(); assert(clients[1].car.gear==2);
        G_DragRunFrame(); assert(clients[1].car.gear==2); /* one gear per settle time */
        assert(G_DragPopulateLadderPlayer(1,&result) && result.isBot);
        assert(G_DragPopulateLadderPlayer(0,&result) && !result.isBot);
    }
    /* A rookie that jumps the light gets the minimum reaction false start. */
    resetHeat(1); g_entities[0].r.svFlags=SVF_BOT; strcpy(botSkill,"1");
    {
        usercmd_t botCmd; memset(&botCmd,0,sizeof(botCmd));
        G_DragBotInput(0,&botCmd); input(0,level.time,botCmd.forwardmove);
        G_DragStarterThink(&starter); green=goGreen();
        dragClients[0].botReactionMs=60;
        level.time=green+80; G_DragRunFrame();
        assert(dragClients[0].phase==DRAG_FALSE_START);
    }
    g_entities[0].r.svFlags=0;

    /* Best of 3: the same drivers until one has won two heats, without a
       map restart. The Ladder gets each driver's best heat; the match
       standing (heat wins, then best heat) gives the positions. */
    g_dragBestOf.integer=3;
    resetHeat(2); assert(dragMatchBestOf==3 && G_DragCanJoin());
    playHeat(2,2100,2200); /* heat 1: driver 0 */
    assert(dragMatch[0].wins==1 && !dragMatchOver && dragMatchHeats==1 && exits==0);
    assert(strstr(lastMatch,"dragMatch 3 2 0 -1 2 0 1 1 0"));
    assert(strstr(lastCpAll,"HEAT TO") && strstr(lastCpAll,"1 - 0"));
    respawns=0; nextHeat();
    assert(respawns==2 && !dragCountdownTime && !level.startRaceTime && exits==0);
    assert(dragClients[0].phase==DRAG_WAITING && dragMatch[0].entered && dragMatch[1].entered);
    assert(!G_DragCanJoin()); /* the match has begun */
    playHeat(2,2300,2050); /* heat 2: driver 1, the fastest heat of the match */
    assert(dragMatch[0].wins==1 && dragMatch[1].wins==1 && !dragMatchOver);
    nextHeat();
    playHeat(2,2000,-1); /* heat 3: driver 1 jumps the light */
    assert(dragMatchOver && dragMatchWinner==0 && G_DragCanJoin());
    level.time=dragAllDoneTime+DRAG_RESULT_HOLD_MS; G_DragCheckExit();
    assert(exits==1 && strstr(lastExit,"match") && level.winnerNumber==0);
    assert(strstr(lastCpAll,"WINS THE MATCH") && strstr(lastCpAll,"2 - 1"));
    assert(G_DragLadderDuel());
    assert(G_DragPopulateLadderPlayer(0,&result) && result.dragHeatWins==2 && result.position==1);
    assert(!strcmp(result.dragStatus,"finished") && result.totalRaceMs==2000); /* best heat */
    assert(G_DragPopulateLadderPlayer(1,&result) && result.dragHeatWins==1 && result.position==2);
    assert(!strcmp(result.dragStatus,"finished") && result.totalRaceMs==2050); /* faster, still second */
    assert(consoleCommands==0); /* no bots, no rival change */

    /* A heat without a winner is run again; after twice the heats the match ends. */
    resetHeat(2);
    playHeat(2,2000,2000); /* tie */
    assert(dragMatchHeats==0 && dragMatchAttempts==1 && !dragMatchOver);
    assert(strstr(lastCpAll,"HEAT AGAIN"));
    for (green=0;green<5;green++) { nextHeat(); playHeat(2,-1,-1); }
    assert(dragMatchAttempts==6 && dragMatchOver && dragMatchWinner==-1);

    /* Leaving between heats forfeits the match; the driver ahead wins it. */
    resetHeat(2); playHeat(2,2100,2200); nextHeat();
    g_entities[1].inuse=qfalse; clients[1].pers.connected=CON_DISCONNECTED;
    G_DragCheckExit();
    assert(exits==1 && strstr(lastExit,"forfeit") && level.winnerNumber==0);

    /* Solo practice: three heats, the best one is reported. */
    resetHeat(1);
    playHeat(1,2400,0); nextHeat(); playHeat(1,2200,0); nextHeat();
    assert(!dragMatchOver); playHeat(1,2300,0);
    assert(dragMatchOver && dragMatchHeats==3 && !G_DragLadderDuel());
    assert(G_DragPopulateLadderPlayer(0,&result) && result.totalRaceMs==2200 && result.position==1);

    /* After a match against a bot, the next match is against another bot. */
    resetHeat(2); g_entities[1].r.svFlags=SVF_BOT; strcpy(clients[1].pers.netname,"Carla"); strcpy(botSkill,"5");
    playHeat(2,2100,2200); nextHeat(); playHeat(2,2100,2200);
    level.time=dragAllDoneTime+DRAG_RESULT_HOLD_MS; G_DragCheckExit();
    assert(exits==1 && consoleCommands==1);
    assert(strstr(lastConsole,"clientkick 1") && strstr(lastConsole,"addbot Josh 5 free"));
    /* a bot joining during the intermission is not a disqualified driver */
    G_DragClientSpawn(&g_entities[1]);
    assert(dragClients[1].phase!=DRAG_DNF);
    g_dragTournament.integer=1; resetHeat(2); g_entities[1].r.svFlags=SVF_BOT;
    playHeat(2,2100,2200); nextHeat(); playHeat(2,2100,2200);
    level.time=dragAllDoneTime+DRAG_RESULT_HOLD_MS; G_DragCheckExit();
    assert(consoleCommands==0); /* the tournament picks its own drivers */
    g_dragTournament.integer=0;
    g_dragBestOf.integer=0;

    resetHeat(1); level.finishEnt=NULL;
    G_DragStarterThink(&starter); G_DragCheckExit();
    assert(exits==1 && strstr(lastExit,"Invalid"));

    resetHeat(1); g_gametype.integer=GT_RACING;
    memset(&cmd,0,sizeof(cmd)); cmd.forwardmove=127;
    G_DragFilterCommand(&g_entities[0],&cmd);
    assert(cmd.forwardmove==127 && !G_DragCheckExit());
    assert(statuses>0 && !statusMismatches);

    /* Tournament hooks: no lane is armed before the lineup stands, and a
     * finished match is handed to the bracket with its winner. */
    {
        int i, before, exitsBefore;
        g_gametype.integer=GT_DRAG; g_dragBestOf.integer=1; G_DragInit(); resetHeat(2);
        tourneyLineupReady=0; tourneyLineups=0;
        for(i=0;i<10;i++) { level.time+=50; input(0,level.time,64); input(1,level.time,64); G_DragStarterThink(&starter); }
        assert(tourneyLineups>0 && !dragCountdownTime);
        tourneyLineupReady=1;
        before=tourneyMatchOvers; exitsBefore=exits;
        playHeat(2,2100,2000); nextHeat();
        assert(exits==exitsBefore+1 && tourneyMatchOvers==before+1 && tourneyWinner==1);
    }

    /* Skipped bot matches: a knockout always has a winner with a majority,
     * and the stronger bot wins most of them. */
    {
        int bestOf, wa, wb, strong=0, n;
        srand(7);
        for(bestOf=1;bestOf<=7;bestOf+=2) {
            for(n=0;n<200;n++) {
                G_DragSimulateMatch(3,3,bestOf,&wa,&wb);
                assert(wa!=wb && wa>=0 && wb>=0 && wa+wb<=2*bestOf+1);
                assert(wa==bestOf/2+1 || wb==bestOf/2+1 || wa+wb>=2*bestOf);
            }
        }
        for(n=0;n<400;n++) { G_DragSimulateMatch(5,1,3,&wa,&wb); if(wa>wb) strong++; }
        assert(strong>300);
    }
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


def test_minimum_reaction_is_the_same_in_game_server_and_ladder_services():
    """The rule exists in C, the C test stub, Python and PHP; keep them equal."""
    import re
    expected = re.search(r"#define DRAG_MIN_REACTION_MS (\d+)",
                         (ROOT / "engine/code/game/bg_public.h").read_text()).group(1)
    assert re.search(r"#define DRAG_MIN_REACTION_MS (\d+)",
                     (ROOT / "tests/server.h").read_text()).group(1) == expected
    assert re.search(r"^DRAG_MIN_REACTION_MS = (\d+)$",
                     (ROOT / "ladder_service/ladder_service/schemas.py").read_text(), re.M).group(1) == expected
    php = (ROOT / "ladder_service/php_webservice/index.php").read_text(encoding="utf-8")
    assert "$player['reactionMs'] < " + expected + ")" in php
    assert "reactionMs < " + expected + " /*" in php
