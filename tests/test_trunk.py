"""Exercise the shared inventory and authoritative item use with game types."""

from test_plate_and_menu_regressions import CODE, compile_run, function


def test_trunk_holds_one_of_each_and_consumes_only_the_selected_item():
    shared = (CODE / 'game/bg_misc.c').read_text()
    items = (CODE / 'game/g_items.c').read_text()
    active = (CODE / 'game/g_active.c').read_text()
    source = r'''
#include "game/g_local.h"
#include <assert.h>

level_locals_t level;
vmCvar_t g_gametype;
gitem_t bg_itemlist[] = {
    {0},
    {.giType = IT_HOLDABLE, .giTag = HI_TELEPORTER},
    {.giType = IT_HOLDABLE, .giTag = HI_MEDKIT},
    {.giType = IT_HOLDABLE, .giTag = HI_FUELCAN}
};
int bg_numItems = sizeof(bg_itemlist) / sizeof(bg_itemlist[0]);
const holdable_t bg_trunkItems[TRUNK_ITEM_COUNT] = {HI_FUELCAN, HI_TELEPORTER, HI_MEDKIT};
static qboolean racing;
static int used, applied, dropped;

void QDECL Com_Error(int code, const char *format, ...) { abort(); }
qboolean isRallyRace(void) { return racing; }
qboolean G_Autoball_CarsFrozen(int time) { return qfalse; }
void BG_AddPredictableEventToPlayerstate(int event, int parm, playerState_t *ps) {
    used = event;
    ps->eventSequence++;
}
void ClientEvents(gentity_t *ent, int previous) {
    assert(ent->client->ps.eventSequence == previous + 1);
    applied++;
}
gentity_t *Drop_Item(gentity_t *ent, gitem_t *item, float angle) {
    dropped = item->giTag;
    return NULL;
}
'''
    for name in ('BG_FindItemForHoldable', 'BG_TrunkBit', 'BG_TrunkMask',
                 'BG_CanAddHoldable', 'BG_HoldableItemIndex'):
        source += '\n' + function(shared, name)
    source += '\n' + next(line for line in items.splitlines()
                           if line.startswith('#define') and 'RESPAWN_HOLDABLE' in line)
    for name in ('Pickup_Holdable', 'Pickup_FuelCan', 'G_DropHoldable'):
        source += '\n' + function(items, name)
    source += '\n' + function(active, 'G_UseTrunkItem')
    source += r'''
int main(void) {
    gclient_t client = {0};
    gentity_t player = {0}, pickup = {0};
    int *value = &client.ps.stats[STAT_HOLDABLE_ITEM];
    player.client = &client;
    player.health = 50;
    client.ps.stats[STAT_MAX_HEALTH] = 100;
    client.ps.pm_type = PM_NORMAL;

    pickup.item = &bg_itemlist[3];
    assert(Pickup_FuelCan(&pickup, &player));
    assert(!Pickup_FuelCan(&pickup, &player));
    pickup.item = &bg_itemlist[1];
    assert(Pickup_Holdable(&pickup, &player));
    pickup.item = &bg_itemlist[2];
    assert(Pickup_Holdable(&pickup, &player));
    assert(BG_TrunkMask(*value) == TRUNK_MASK);
    assert(BG_HoldableItemIndex(2) == 2);  /* Legacy demo stat. */

    racing = qtrue;
    G_UseTrunkItem(&player, HI_TELEPORTER);
    assert(!applied && BG_TrunkMask(*value) == TRUNK_MASK);
    level.time = level.startRaceTime = 1000;
    G_UseTrunkItem(&player, HI_MEDKIT);
    assert(applied == 1 && used == EV_USE_ITEM0 + HI_MEDKIT);
    assert(BG_TrunkMask(*value) == (BG_TrunkBit(HI_TELEPORTER) | BG_TrunkBit(HI_FUELCAN)));
    G_UseTrunkItem(&player, HI_MEDKIT);
    assert(applied == 1);
    G_DropHoldable(&player, &bg_itemlist[3]);
    assert(dropped == HI_FUELCAN && BG_TrunkMask(*value) == BG_TrunkBit(HI_TELEPORTER));
    G_UseTrunkItem(&player, HI_TELEPORTER);
    assert(applied == 2 && used == EV_USE_ITEM0 + HI_TELEPORTER && *value == 0);
    return 0;
}
'''
    compile_run(source)
