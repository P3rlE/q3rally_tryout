"""The HUD options (Shift+H) have to look like their design.

tests/fixtures/hud_options_*_layout.json hold what Chrome reports for the
boards PanelRace, PanelDeathmatch and PanelDerby of the HUD design
(tools/ui_webstyle/hud_design/gen.py hud_panel, measure_hud_design.py).
"""
import json
import pathlib
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close

CARD, ORDER, CAR, MAP, TRACK, FX = range(6)
ON_OFF = 'Off,On'
SLIDERS = ['4~Size~-~~0~1.00~0.375', '4~Field of view~-~~0~90~0.666667']

# the boards' content, in the harness's terms (gen.py writes the same)
MODELS = {
    'race': ('Live HUD \xb7 Race \xb7 Ridge Racer|race|Back to race|'
             f'Card~{CARD}~0;On track~{TRACK}~0;Order~{ORDER}~0;Car and cockpit~{CAR}~1;Mini-map~{MAP}~1|'
             + ';'.join([f'0~Times~Lap, best, gap~{ON_OFF}~0~~0', f'0~Lap counter~-~{ON_OFF}~0~~0', f'0~Race position~-~{ON_OFF}~0~~0',
                         '1~Checkpoint arrow~On HUD: at the top of the screen~Off,On HUD,Above car~1~~0', f'1~Racing line~-~{ON_OFF}~0~~0',
                         f'2~Race order~-~{ON_OFF}~0~~0', f'3~HUD~Off hides every panel~{ON_OFF}~0~~0', f'3~Rear view~-~{ON_OFF}~0~~0',
                         f'3~Slipstream gauge~-~{ON_OFF}~0~~0', f'4~Mini-map~-~{ON_OFF}~0~~0'] + SLIDERS)),
    'deathmatch': ('Live HUD \xb7 Deathmatch \xb7 Battle Canyon|match|Back to match|'
                   f'Order~{ORDER}~0;Car and cockpit~{CAR}~0;Mini-map~{MAP}~1|'
                   + ';'.join([f'0~Frag order~-~{ON_OFF}~0~~0', f'1~HUD~Off hides every panel~{ON_OFF}~0~~0',
                               f'1~Rear view~-~{ON_OFF}~0~~0', f'2~Mini-map~-~{ON_OFF}~0~~0']
                              + [s.replace('4~', '2~', 1) for s in SLIDERS])),
    'derby': ('Live HUD \xb7 Derby \xb7 Figure of 8|derby|Back to match|'
              f'Order~{ORDER}~0;Effects~{FX}~0;Car and cockpit~{CAR}~0;Mini-map~{MAP}~1|'
              + ';'.join([f'0~Derby order~-~{ON_OFF}~0~~0', f'1~Hit flash~Red flash when you are hit~{ON_OFF}~0~~0',
                          f'2~HUD~Off hides every panel~{ON_OFF}~0~~0', f'2~Rear view~-~{ON_OFF}~0~~0', f'3~Mini-map~-~{ON_OFF}~0~~0']
                         + [s.replace('4~', '3~', 1) for s in SLIDERS])),
}


def layouts(names):
    lines = []
    for name in names:
        lines += ['hudopt 1600', MODELS[name]]
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        return [json.loads(line) for line in run(exe, lines)]


def check_board(name, mine):
    els = json.loads((ROOT / f'tests/fixtures/hud_options_{name}_layout.json').read_text())['els']
    problems = []

    def check(what, value, path, fields=(0, 1, 2, 3)):
        reference = els[path]
        if not close([value[i] for i in fields], [reference[i] for i in fields]):
            problems.append((what, value, reference))

    P = '/div:1'
    check('box', mine['box'], P)
    head = P + '/div:0'
    check('crumb', mine['crumb'], head + '/div:0/div:0', (0, 1, 3))
    check('title', mine['title'], head + '/div:0/div:1', (0, 1, 3))
    check('em', mine['em'], head + '/div:0/div:1/span:0', (0, 2))
    check('text', mine['text'], head + '/div:0/div:2', (0, 1, 3))
    hint = mine['hint']   # the text; Chrome's box has its 2px padding on top
    check('hint', [hint[0], hint[1] - 2, hint[2], hint[3] + 2], head + '/div:1')
    check('back', mine['back'], P + '/div:2/button:0')
    check('console', mine['console'], P + '/div:2/button:1')

    # the groups by column, in order; the items by group
    model = MODELS[name].split('|')
    groups = [g.split('~') for g in model[3].split(';')]
    items = [i.split('~') for i in model[4].split(';')]
    slots = {}
    for column in (0, 1):
        for g in [g for g in range(len(groups)) if mine['columns'][g] == column]:
            slots[g] = f'{P}/div:1/div:{column}/div:{sum(1 for k in slots if mine["columns"][k] == column)}'
    for g, path in slots.items():
        icon, title, line = mine['groups'][g]
        check(f'group {g} icon', icon, path + '/div:0/svg:0')
        check(f'group {g} title', title, path + '/div:0/div:1')
        check(f'group {g} line', line, path + '/div:0/div:2')
        for n, i in enumerate([i for i in range(len(items)) if int(items[i][0]) == g]):
            item, row = mine['items'][i], f'{path}/div:{n + 1}'
            if not items[i][3]:
                check(f'item {i} label', item['label'], row + '/div:0/div:0', (0, 1, 2, 3))
                check(f'item {i} value', item['value'], row + '/div:0/div:1')
                check(f'item {i} track', item['track'], row + '/div:1/div:0')
                check(f'item {i} thumb', item['thumb'], row + '/div:1/div:2')
                check(f'item {i} rule', [item['rule'][0], item['rule'][1], item['rule'][3]],
                      row, ())
                rule = els[row]
                if not close([item['rule'][1], item['rule'][3]], [rule[1], rule[3]]):
                    problems.append((f'item {i} rule', item['rule'], rule))
                continue
            check(f'item {i} label', item['label'], row + '/div:0/div:0', (0, 1, 3))
            if items[i][2] != '-':
                check(f'item {i} hint', item['hint'], row + '/div:0/div:1', (0, 1, 3))
            check(f'item {i} buttons', item['buttons'], row + '/div:1')
            for c, button in enumerate(item['button']):
                check(f'item {i} button {c}', button[:4], row + f'/div:1/button:{c}')
    return problems


def test_native_hud_options_match_their_design():
    names = sorted(MODELS)
    failures = []
    for name, mine in zip(names, layouts(names)):
        problems = check_board(name, mine)
        if problems:
            failures.append((name, problems[:8]))
    assert not failures, failures


def test_a_long_map_name_stops_short_of_the_hint():
    model = MODELS['race'].replace('Ridge Racer', 'Riiiiiidge Raaaaaacerrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr')
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        mine, = [json.loads(line) for line in run(exe, ['hudopt 1600', model])]
    crumb, hint = mine['crumb'], mine['hint']
    assert crumb[0] + crumb[2] <= hint[0] - 24 + 0.01


def test_hud_option_changes_validate_values_and_open_state():
    from test_web_config import extract, run_c
    source = (ROOT / 'engine/code/cgame/cg_hud_core.c').read_text()
    harness = r"""
#include <assert.h>
#define HUDOPT_NUM_ENTRIES 2
#define HUDOPT_ITEM_SIZE -2
#define HUDOPT_ITEM_FOV -3
#define HUDOPT_SIZE_MIN 40
#define HUDOPT_SIZE_MAX 200
#define HUDOPT_FOV_MIN 30
#define HUDOPT_FOV_MAX 120
#define qfalse 0
#define qtrue 1
typedef struct {int integer;} vmCvar_t;
typedef struct {vmCvar_t *cvar;const char *cvarName;int onValue;} hudToggleEntry_t;
static vmCvar_t cg_hudOptionsOpen,cg_mmap_size,cg_mmap_fov,a,b;
static hudToggleEntry_t hudToggleTable[]={{&a,"a",1},{&b,"b",3}};
static int calls,unavailable;static float last;
static void trap_Cvar_Update(vmCvar_t *v){}
static int CG_HUDOptionsIsOpen(void){return cg_hudOptionsOpen.integer;}
static int HUDEntry_IsUnavail(const hudToggleEntry_t *entry){return unavailable;}
static const char *va(const char *format,int value){return "test";}
static void trap_Cvar_Set(const char *name,const char *value){calls++;}
static void HUDOpt_SetFloatCvar(const char *name,vmCvar_t *v,float value,int integral){calls++;last=value;}
""" + extract(source, 'void CG_HUDOptions_Set(') + r"""
int main(void){CG_HUDOptions_Set(0,1);assert(calls==0);cg_hudOptionsOpen.integer=1;
CG_HUDOptions_Set(0,1);assert(calls==1);
CG_HUDOptions_Set(0,2);CG_HUDOptions_Set(-1,0);CG_HUDOptions_Set(2,0);CG_HUDOptions_Set(-4,0);assert(calls==1);
CG_HUDOptions_Set(1,3);assert(calls==2);
unavailable=1;CG_HUDOptions_Set(0,0);assert(calls==2);unavailable=0;
CG_HUDOptions_Set(HUDOPT_ITEM_SIZE,40);assert(calls==3&&last>.39f&&last<.41f);
CG_HUDOptions_Set(HUDOPT_ITEM_SIZE,201);CG_HUDOptions_Set(HUDOPT_ITEM_SIZE,41);assert(calls==3);
CG_HUDOptions_Set(HUDOPT_ITEM_FOV,120);assert(calls==4&&last==120);
CG_HUDOptions_Set(HUDOPT_ITEM_FOV,125);CG_HUDOptions_Set(HUDOPT_ITEM_FOV,29);assert(calls==4);}
"""
    run_c(harness)
