"""Exercise the Drag Race tournament bracket (g_drag_tourney.c) without a renderer."""
import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]


def test_drag_tournament_bracket():
    source = ROOT / "engine/code/game/g_drag_tourney.c"
    harness = '#include "' + source.as_posix() + '"\n'
    harness += r'''
#include <assert.h>
#include <strings.h>

level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
vmCvar_t g_gametype, g_dragTournament, g_dragTournamentSize;
static gclient_t clients[MAX_CLIENTS];

/* a tiny cvar store */
static char cvarNames[8][32], cvarValues[8][MAX_CVAR_VALUE_STRING];
static char *cvar(const char *name) {
    int i;
    for (i=0;i<8 && cvarNames[i][0];i++) if (!strcmp(cvarNames[i],name)) return cvarValues[i];
    assert(i<8); snprintf(cvarNames[i],32,"%s",name); cvarValues[i][0]=0; return cvarValues[i];
}
void trap_Cvar_Set(const char *name,const char *value) { snprintf(cvar(name),MAX_CVAR_VALUE_STRING,"%s",value); }
void trap_Cvar_VariableStringBuffer(const char *name,char *out,int size) { snprintf(out,size,"%s",cvar(name)); }
float trap_Cvar_VariableValue(const char *name) { return atof(cvar(name)); }

void Q_strncpyz(char *out,const char *in,int size) { snprintf(out,size,"%s",in); }
char *Q_CleanStr(char *text) { return text; }
int Q_stricmp(const char *a,const char *b) { return strcasecmp(a,b); }
void Q_strcat(char *dest,int size,const char *src) { strncat(dest,src,size-strlen(dest)-1); }
int QDECL Com_sprintf(char *out,int size,const char *format,...) {
    int n; va_list ap; va_start(ap,format); n=vsnprintf(out,size,format,ap); va_end(ap); return n;
}
char *QDECL va(char *format, ...) {
    static char buffer[2048]; va_list ap;
    va_start(ap, format); vsnprintf(buffer, sizeof(buffer), format, ap); va_end(ap);
    return buffer;
}
void QDECL G_LogPrintf(const char *fmt,...) {}

/* a bot file's entry: the model is the bot's name under "car/" */
char *G_GetBotInfoByName(const char *name) { static char info[64]; snprintf(info,sizeof(info),"%s",name); return info; }
char *Info_ValueForKey(const char *s,const char *key) { static char v[64]; snprintf(v,sizeof(v),"car/%s",s); return v; }

static char lastBracket[MAX_STRING_CHARS], lastCars[MAX_STRING_CHARS], lastCp[256], console[1024];
void trap_SendServerCommand(int recipient,const char *text) {
    if (!strncmp(text,"dragTourney ",12)) snprintf(lastBracket,sizeof(lastBracket),"%s",text);
    if (!strncmp(text,"dragTourneyCars",15)) snprintf(lastCars,sizeof(lastCars),"%s",text);
    if (!strncmp(text,"cp ",3)) snprintf(lastCp,sizeof(lastCp),"%s",text);
}
void trap_SendConsoleCommand(int when,const char *text) { strncat(console,text,sizeof(console)-strlen(console)-1); }
static char argument[16];
void trap_Argv(int n,char *out,int size) { snprintf(out,size,"%s",n==1?argument:""); }

static int exits, bonus;
void LogExit(const char *text) { exits++; }
void G_Profile_AddScore(int delta) { bonus+=delta; }
void SetTeam(gentity_t *ent,const char *team) {
    clients[ent-g_entities].sess.sessionTeam = !strcmp(team,"free") ? TEAM_FREE : TEAM_SPECTATOR;
}
int TeamCount(int ignore,team_t team) {
    int i, n=0;
    for (i=0;i<level.maxclients;i++)
        if (i!=ignore && clients[i].pers.connected==CON_CONNECTED && clients[i].sess.sessionTeam==team) n++;
    return n;
}
int G_DragMatchBestOf(void) { return 3; }
static int simulated;
void G_DragSimulateMatch(int a,int b,int bestOf,int *wa,int *wb) {
    simulated++; assert(bestOf==3); *wa = a>=b ? 2 : 1; *wb = a>=b ? 1 : 2;
}
static const char *bots[] = { "Chloe","Janine","Samuel","Jim","Carla","Dean","Vlad","Minori",
    "Bobby","Sam","Alexandra","Jennifer","Josh","Mia","Rex","Ivy","Zed" };
const char *G_DragTourneyBotName(int index) { return index<17 ? bots[index] : NULL; }

static void connectHuman(int i,const char *name) {
    g_entities[i].client=&clients[i]; g_entities[i].inuse=qtrue; g_entities[i].r.svFlags=0;
    clients[i].pers.connected=CON_CONNECTED; clients[i].pers.localClient = i==0;
    clients[i].sess.sessionTeam=TEAM_FREE;
    snprintf(clients[i].pers.netname,sizeof(clients[i].pers.netname),"%s",name);
}
static void dropClient(int i) { clients[i].pers.connected=CON_DISCONNECTED; }

/* What the server console would do with addbot and clientkick. */
static void runConsole(void) {
    char *line=console, *end;
    char name[32]; int skill, i, slot;
    while (*line) {
        end=strchr(line,'\n'); if (end) *end=0;
        if (sscanf(line,"addbot %31s %d",name,&skill)==2) {
            for (slot=0;slot<level.maxclients && clients[slot].pers.connected!=CON_DISCONNECTED;slot++) {}
            assert(slot<level.maxclients);
            connectHuman(slot,name); g_entities[slot].r.svFlags=SVF_BOT; clients[slot].pers.localClient=qfalse;
        } else if (sscanf(line,"clientkick %d",&i)==1) {
            assert(g_entities[i].r.svFlags & SVF_BOT); dropClient(i);
        }
        if (!end) break;
        line=end+1;
    }
    console[0]=0;
}

/* Lineup until the pairing stands, as the starter's think does. */
static void lineup(void) {
    int tries;
    for (tries=0;tries<4;tries++) {
        level.time+=50;
        if (G_DragTourneyLineup()) return;
        runConsole();
    }
    assert(!"lineup never stood");
}

static int entrantOf(const char *name) {
    int i;
    for (i=0;i<dt.size;i++) if (!strcmp(dt.field[i].name,name)) return i;
    return -1;
}

int main(void) {
    int a, b, ca, cb, i, wins[MAX_CLIENTS], per, round0, humanMatch;
    char saved[3][MAX_CVAR_VALUE_STRING];

    srand(3);
    level.maxclients=8; level.clients=clients; g_gametype.integer=GT_DRAG;
    g_dragTournament.integer=1; g_dragTournamentSize.integer=8;
    trap_Cvar_Set("g_spSkill","3");
    connectHuman(0,"Per");

    /* A new field: the human plus seven distinct bots around g_spSkill. */
    G_DragTourneyReset(); G_DragTourneyInit();
    assert(!G_DragTourneyActive());
    lineup();
    assert(G_DragTourneyActive() && dt.size==8 && dt.bestOf==3 && !dt.played);
    per=entrantOf("Per");
    assert(per>=0 && dt.field[per].client==0);
    for (i=0;i<8;i++) {
        if (i==per) continue;
        assert(dt.field[i].client==-1 && dt.field[i].skill>=2 && dt.field[i].skill<=4);
        assert(entrantOf(dt.field[i].name)==i);
    }
    assert(!strncmp(lastBracket,"dragTourney 8 3 0 ",18));
    /* each bot's car from its bot file, none for the human */
    {
        char want[MAX_STRING_CHARS] = "dragTourneyCars";
        for (i=0;i<8;i++) {
            char one[64];
            if (i==per) snprintf(one,sizeof(one)," \"\"");
            else snprintf(one,sizeof(one)," \"car/%s\"",dt.field[i].name);
            strcat(want,one);
        }
        assert(!strcmp(lastCars,want));
    }

    /* Only the pairing drives: the human watches unless it is his match. */
    DT_MatchEntrants(0,&a,&b);
    ca=DT_ClientOf(a); cb=DT_ClientOf(b);
    assert(ca>=0 && cb>=0);
    assert(clients[ca].sess.sessionTeam==TEAM_FREE && clients[cb].sess.sessionTeam==TEAM_FREE);
    assert(G_DragTourneyDriver(ca) && G_DragTourneyDriver(cb));
    if (per!=a && per!=b) assert(clients[0].sess.sessionTeam==TEAM_SPECTATOR && !G_DragTourneyDriver(0));
    assert(strstr(lastCp,"QUARTER-FINAL"));

    /* The bracket survives the map reload between matches. */
    for (i=0;i<3;i++) trap_Cvar_VariableStringBuffer(i==0?"g_dragTourneyA":i==1?"g_dragTourneyB":"g_dragTourneyR",saved[i],sizeof(saved[i]));
    {
        dtEntrant_t before[DT_MAX];
        memcpy(before,dt.field,sizeof(before));
        memset(&dt,0,sizeof(dt)); G_DragTourneyInit();
        assert(dt.loaded && dt.size==8 && !memcmp(before,dt.field,sizeof(before)));
    }

    /* A finished match goes into the bracket with its heat score. */
    lineup();
    DT_MatchEntrants(0,&a,&b); ca=DT_ClientOf(a); cb=DT_ClientOf(b);
    memset(wins,0,sizeof(wins));
    if (a==per) { wins[ca]=2; wins[cb]=1; G_DragTourneyMatchOver(ca,wins); }
    else { wins[ca]=1; wins[cb]=2; G_DragTourneyMatchOver(cb,wins); }
    trap_Cvar_VariableStringBuffer("g_dragTourneyR",saved[2],sizeof(saved[2]));
    if (a==per) assert(dt.played==1 && !dt.results[0].winner && !strcmp(saved[2],"0 2 1"));
    else assert(dt.played==1 && dt.results[0].winner==1 && !strcmp(saved[2],"1 1 2"));
    /* a stale report from a match that is no longer the pairing is ignored */
    G_DragTourneyMatchOver(cb,wins);
    G_DragTourneyMatchOver(ca,wins);
    assert(dt.played==1);

    /* Skipping: one bot match, then all of them up to the human's. */
    humanMatch=-1;
    for (i=1;i<4;i++) { DT_MatchEntrants(i,&a,&b); if (a==per||b==per) humanMatch=i; }
    round0 = humanMatch>=0 ? humanMatch : 0;
    exits=0;
    if (round0>1) {
        argument[0]=0; Cmd_DragSkip_f(&g_entities[0]);
        assert(dt.played==2 && exits==1);
    }
    if (humanMatch>dt.played) {
        snprintf(argument,sizeof(argument),"all"); Cmd_DragSkip_f(&g_entities[0]);
        assert(dt.played==humanMatch);
    }
    /* the human's own match cannot be skipped */
    if (humanMatch>=0) {
        int played=dt.played;
        Cmd_DragSkip_f(&g_entities[0]); assert(dt.played==played);
        lineup();
        assert(clients[0].sess.sessionTeam==TEAM_FREE && G_DragTourneyDriver(0));
        DT_MatchEntrants(dt.played,&a,&b); ca=DT_ClientOf(a); cb=DT_ClientOf(b);
        memset(wins,0,sizeof(wins)); wins[0]=2;
        G_DragTourneyMatchOver(0,wins);
        assert(DT_Winner(dt.played-1)==per);
    }

    /* Play out: the human wins every match he is in, bots are skipped. */
    while (G_DragTourneyActive()) {
        DT_MatchEntrants(dt.played,&a,&b);
        assert(a>=0 && b>=0 && a!=b);
        if (DT_IsBot(a) && DT_IsBot(b)) {
            snprintf(argument,sizeof(argument),"all"); Cmd_DragSkip_f(&g_entities[0]);
            continue;
        }
        lineup();
        memset(wins,0,sizeof(wins)); wins[0]=2;
        G_DragTourneyMatchOver(0,wins);
    }
    /* eight matches: four quarter-finals, two semi-finals, third place, final */
    assert(dt.played==8 && DT_Winner(7)==per && bonus==DT_CHAMPION_BONUS);
    DT_MatchEntrants(7,&a,&b);
    assert(a==DT_Winner(4) && b==DT_Winner(5));
    DT_MatchEntrants(6,&a,&b);
    assert(a==DT_Loser(4) && b==DT_Loser(5) && a!=per && b!=per);
    assert(DT_Winner(6)==a || DT_Winner(6)==b);
    assert(!strcmp(DT_MatchName(5),"SEMI-FINAL") && !strcmp(DT_MatchName(6),"THIRD PLACE")
        && !strcmp(DT_MatchName(7),"FINAL") && !strcmp(DT_MatchName(0),"QUARTER-FINAL"));
    assert(!strncmp(lastBracket,"dragTourney 8 3 8 -1 -1 ",24));

    /* The next load after the final starts over. */
    G_DragTourneyInit();
    assert(!dt.loaded && !cvar("g_dragTourneyA")[0]);

    /* Four drivers: losing the semi-final means racing for third place. */
    g_dragTournamentSize.integer=4; bonus=0;
    for (i=1;i<level.maxclients;i++) dropClient(i);
    lineup();
    assert(dt.size==4 && !strcmp(DT_MatchName(0),"SEMI-FINAL") && strstr(lastCp,"NEW DRAG TOURNAMENT"));
    per=entrantOf("Per");
    while (G_DragTourneyActive()) {
        DT_MatchEntrants(dt.played,&a,&b);
        if (DT_IsBot(a) && DT_IsBot(b)) { argument[0]=0; Cmd_DragSkip_f(&g_entities[0]); continue; }
        lineup();
        ca=DT_ClientOf(a); cb=DT_ClientOf(b);
        memset(wins,0,sizeof(wins));
        if (dt.played<2) { /* the human loses his semi-final ... */
            int rival = ca==0 ? cb : ca;
            wins[rival]=2; G_DragTourneyMatchOver(rival,wins);
            assert(strstr(lastCp,"QUARTER")==NULL);
        } else {           /* ... and wins third place */
            assert(dt.played==2 && !strcmp(DT_MatchName(2),"THIRD PLACE") && strstr(lastCp,"THIRD PLACE"));
            wins[0]=2; G_DragTourneyMatchOver(0,wins);
        }
    }
    assert(dt.played==4 && DT_Winner(2)==per && DT_Winner(3)!=per && !bonus);
    G_DragTourneyInit();

    /* A human who leaves forfeits: the opponent goes through. */
    g_dragTournamentSize.integer=4; bonus=0;
    for (i=1;i<level.maxclients;i++) dropClient(i);
    connectHuman(1,"Anna"); clients[1].pers.localClient=qfalse;
    lineup();
    assert(dt.size==4 && entrantOf("Anna")>=0);
    while (G_DragTourneyActive()) {
        DT_MatchEntrants(dt.played,&a,&b);
        if (a==entrantOf("Anna") || b==entrantOf("Anna")) break;
        if (DT_IsBot(a) && DT_IsBot(b)) { argument[0]=0; Cmd_DragSkip_f(&g_entities[0]); continue; }
        lineup(); memset(wins,0,sizeof(wins)); wins[0]=2; G_DragTourneyMatchOver(0,wins);
    }
    if (G_DragTourneyActive()) {
        int played=dt.played, anna=entrantOf("Anna");
        dropClient(1);
        assert(!G_DragTourneyLineup());
        assert(dt.played==played+1 && DT_Winner(played)!=anna);
    }

    /* Size 16 fits both cvars and the client command. */
    G_DragTourneyReset(); g_dragTournamentSize.integer=16;
    for (i=1;i<level.maxclients;i++) dropClient(i);
    lineup();
    assert(dt.size==16 && strlen(cvar("g_dragTourneyA"))<MAX_CVAR_VALUE_STRING-1);
    memset(&dt,0,sizeof(dt)); G_DragTourneyInit();
    assert(dt.loaded && dt.size==16 && entrantOf("Per")>=0);
    G_DragTourneyPublish(-1);
    assert(!strncmp(lastBracket,"dragTourney 16 3 0 ",19));

    /* Off: no tournament, everyone may drive. */
    g_dragTournament.integer=0; G_DragTourneyInit();
    assert(!G_DragTourneyActive() && G_DragTourneyDriver(5) && G_DragTourneyLineup());
    return 0;
}
'''
    compiler = shutil.which("gcc") or "C:/msys64/mingw64/bin/gcc.exe"
    with tempfile.TemporaryDirectory(dir=ROOT / "tests") as directory:
        c = pathlib.Path(directory) / "tourney.c"
        binary = pathlib.Path(directory) / "tourney.exe"
        c.write_text(harness)
        subprocess.run([compiler, '-DARCH_STRING="test"', str(c), "-o", str(binary), "-lm"], check=True)
        subprocess.run([str(binary)], check=True, timeout=5)
