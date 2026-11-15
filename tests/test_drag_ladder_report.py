"""Exercise the production Drag branches in match outcome and report population."""
from test_plate_and_menu_regressions import CODE, compile_run, function


def test_drag_report_keeps_armed_identity_and_current_local_profile():
    source = (CODE / "game/g_main.c").read_text()
    populate = source[source.index("static qboolean G_LadderPopulatePlayer( ladderMatchPayload_t *payload, int clientNum ) {"):]
    populate = populate[:populate.index("\n        if ( client->pers.connected != CON_CONNECTED )")] + "\nreturn qfalse;\n}\n"
    outcome = function(source, "G_RecordMatchOutcome")
    outcome = outcome[:outcome.index("\n        if ( level.numPlayingClients <= 0 )")] + "\n}\n"
    harness = r'''
#include "game/g_local.h"
#include "game/g_profile.h"
#include <assert.h>
level_locals_t level;
gentity_t g_entities[MAX_GENTITIES];
vmCvar_t g_gametype;
static gclient_t clients[MAX_CLIENTS];
static ladderPlayerPayload_t armed[2];
static qboolean included[2], matching[2], duel;
static int records[2], userinfoUpdates, completed;
qboolean G_DragPopulateLadderPlayer(int n,ladderPlayerPayload_t *out) {
    if(n>1 || !included[n]) return qfalse;
    *out=armed[n]; return qtrue;
}
qboolean G_DragLadderIdentityMatches(int n) { return n<2 && matching[n] && clients[n].pers.connected==CON_CONNECTED; }
qboolean G_DragLadderDuel(void) { return duel; }
qboolean G_Profile_GetUUID(char *out,int size) { strcpy(out,"local-owner"); return qtrue; }
qboolean G_Profile_GetLadderSnapshot(ladderProfileSnapshot_t *snap,int *revision,int *epoch) {
    snap->dragCompleted=completed; *revision=3; *epoch=10; return qtrue;
}
void G_Profile_RecordDragResult(gclient_t *client,int reaction,int elapsed,int position,
    qboolean isDuel,qboolean won,qboolean hasWinner) {
    int n=(int)(client-clients);
    assert(isDuel==duel);
    if(n==0) { assert(reaction==200 && elapsed==12500 && position==1); completed++; }
    else { assert(!reaction && !elapsed && !position); }
    assert(won==(duel && n==level.winnerNumber));
    assert(hasWinner==duel);
    records[n]++;
}
void ClientUserinfoChanged(int n) { userinfoUpdates++; }
'''
    harness += function(source, "G_LadderModeForGametype") + "\n" + populate + outcome
    harness += r'''
int main(void) {
    static ladderMatchPayload_t payload;
    g_gametype.integer=GT_DRAG; level.clients=clients; level.maxclients=3;
    included[0]=included[1]=matching[0]=matching[1]=qtrue; duel=qtrue;
    strcpy(armed[0].playerId,"local-owner"); strcpy(armed[0].dragStatus,"finished");
    armed[0].reactionMs=200; armed[0].elapsedMs=12500; armed[0].totalRaceMs=12700;
    armed[0].position=1;
    strcpy(armed[1].playerId,"remote-owner"); strcpy(armed[1].dragStatus,"dnf");
    clients[0].pers.localClient=qtrue; clients[0].pers.connected=CON_CONNECTED;
    clients[1].pers.connected=CON_DISCONNECTED;
    level.winnerNumber=0;
    assert(!strcmp(G_LadderModeForGametype(GT_DRAG),"GT_DRAG"));
    G_RecordMatchOutcome(); G_RecordMatchOutcome();
    assert(records[0]==1 && records[1]==0 && userinfoUpdates==1);
    assert(G_LadderPopulatePlayer(&payload,0));
    assert(payload.playerCount==1 && payload.players[0].profileAttached);
    assert(payload.players[0].profile.valid && payload.players[0].profile.dragCompleted==1);
    assert(payload.players[0].profile.snapshotRevision==3);
    assert(G_LadderPopulatePlayer(&payload,1));
    assert(payload.playerCount==2 && !payload.players[1].profileAttached);
    assert(!strcmp(payload.players[1].playerId,"remote-owner"));
    assert(!G_LadderPopulatePlayer(&payload,2)); /* unarmed viewer excluded */

    /* A new occupant does not receive the old participant's result/profile. */
    payload.playerCount=0;
    clients[1].pers.localClient=qtrue; clients[1].pers.connected=CON_CONNECTED;
    assert(G_LadderPopulatePlayer(&payload,1));
    assert(!payload.players[0].profileAttached && !strcmp(payload.players[0].playerId,"remote-owner"));
    matching[1]=qfalse; level.matchOutcomeRecorded=qfalse;
    G_RecordMatchOutcome(); assert(records[1]==0);

    /* Solo practice still records a valid finish, with no competitive win. */
    included[1]=matching[1]=qfalse; duel=qfalse;
    level.matchOutcomeRecorded=qfalse;
    G_RecordMatchOutcome(); assert(records[0]==3);
    return 0;
}
'''
    compile_run(harness)
