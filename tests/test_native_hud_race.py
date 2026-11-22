"""The in-game HUD of a race has to look like its design.

tests/fixtures/hud_race_layout.json holds what Chrome reports for the
design's race board (tools/ui_webstyle/hud_design/measure_hud_design.py
Main.dc.html), tests/fixtures/hud_notify_layout.json the board with the
notifications and the opened jukebox, tests/fixtures/hud_racingdm_layout.json
the race with weapons (its car with armor, weapons and an item). The native layout runs here with the
design's content (tools/ui_webstyle/hud_design/gen.py) against them.
"""
import json
import pathlib
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close

RACE = json.loads((ROOT / 'tests/fixtures/hud_race_layout.json').read_text())['els']
SPRINT = json.loads((ROOT / 'tests/fixtures/hud_sprint_layout.json').read_text())['els']
GHOST = json.loads((ROOT / 'tests/fixtures/hud_ghost_layout.json').read_text())['els']
TEAM_RACE = json.loads((ROOT / 'tests/fixtures/hud_teamrace_layout.json').read_text())['els']
CTF = json.loads((ROOT / 'tests/fixtures/hud_ctf_layout.json').read_text())['els']
END = json.loads((ROOT / 'tests/fixtures/hud_end_racing_layout.json').read_text())['els']
END_TEAM = json.loads((ROOT / 'tests/fixtures/hud_end_teamracing_layout.json').read_text())['els']
DRAG = json.loads((ROOT / 'tests/fixtures/hud_drag_layout.json').read_text())['els']
AUTOBALL = json.loads((ROOT / 'tests/fixtures/hud_autoball_layout.json').read_text())['els']
KOTH = json.loads((ROOT / 'tests/fixtures/hud_koth_layout.json').read_text())['els']
DOMINATION = json.loads((ROOT / 'tests/fixtures/hud_domination_layout.json').read_text())['els']
TEAMDM = json.loads((ROOT / 'tests/fixtures/hud_teamdm_layout.json').read_text())['els']
DEATHMATCH = json.loads((ROOT / 'tests/fixtures/hud_deathmatch_layout.json').read_text())['els']
DERBY = json.loads((ROOT / 'tests/fixtures/hud_derby_layout.json').read_text())['els']
LCS = json.loads((ROOT / 'tests/fixtures/hud_lcs_layout.json').read_text())['els']
ELIMINATION = json.loads((ROOT / 'tests/fixtures/hud_elimination_layout.json').read_text())['els']
MESSAGES = json.loads((ROOT / 'tests/fixtures/hud_messages_layout.json').read_text())['els']
RACING_DM = json.loads((ROOT / 'tests/fixtures/hud_racingdm_layout.json').read_text())['els']
NOTIFY = json.loads((ROOT / 'tests/fixtures/hud_notify_layout.json').read_text())['els']

# the race board of gen.py
RACE_MODEL = {
    'title': 'Race · Downtown', 'pos': 'P3', 'of': '/ 6', 'lapLabel': 'LAP', 'lapValue': '2/5',
    'kv': [('LAP', '0:42.18'), ('BEST', '0:40.95'), ('TO P2', '+1.07')],
    'orderTitle': 'Race order', 'orderRight': 'Gap',
    'rows': [('01', 'Sam', 'LEADER'), ('02', 'Bobby', '+0.85'), ('03', 'Player', '+1.92'),
             ('04', 'Janine', '+2.40'), ('05', 'Vlad', '+4.07'), ('06', 'Emma', '+6.31')],
    'mirror': 1, 'toast': 1, 'toastIcon': 1, 'tag': 'Personal best', 'toastTitle': 'Lap 1 · 0:40.95',
    'sub': 'BEST LAP ON Q3R_DOWNTOWN',
    'jukebox': 1, 'open': 0, 'line': 'Artist — Track title', 'time': '1:42',
    'jtitle': 'Track title', 'jartist': 'Artist', 'jstate': 'NOW PLAYING',
    'integrity': '118', 'armor': '', 'minimap': 1, 'slip': 1, 'slipValue': '+8%',
    'speed': '142', 'unit': 'MPH', 'gear': '4', 'nos': '60%', 'fuel': '82%',
}


def latin(text):
    # the game's strings are Latin-1; the body faces carry the em dash at 0x9F
    return text.replace('—', '\x9f')


def command(model, width=1600):
    m = model
    fields = [m['title'], m['pos'], m['of'], m['lapLabel'], m['lapValue'],
              ';'.join(f'{k}:{v}' for k, v in m['kv']), m['orderTitle'], m['orderRight'],
              ';'.join(f'{p}:{n}:{v}' for p, n, v in m['rows']),
              m['mirror'], m['toast'], m['toastIcon'], m['tag'], m['toastTitle'], m['sub'],
              m['jukebox'], m['open'], m['line'], m['time'], m['jtitle'], m['jartist'], m['jstate'],
              m['integrity'], m['armor'], m['minimap'], m['slip'], m['slipValue'],
              m['speed'], m['unit'], m['gear'], m['nos'], m['fuel'], m.get('vehicleWidth', 0),
              ';'.join(f'{a}:{b}:{c}' for a, b, c in m.get('slots', [])), m.get('progress', ''),
              m.get('posLabel', ''), ','.join(str(v) for v in m.get('kvSizes', [])),
              ';'.join(f'{n}:{v}' for n, v in m.get('teams', [])), m.get('swatch', 0),
              ';'.join(f'{a}:{w}:{b}' for a, w, b in m.get('kills', [])), m.get('elim', ''),
              m.get('posSize', 0), m.get('rowBars', 0), m.get('derby', ''), m.get('headerRow', 0),
              ';'.join(f'{n}~{st}~{no}' for n, st, no in m.get('flags', [])), m.get('zones', ''), m.get('hill', ''), m.get('scorebug', ''),
              m.get('labelRow', ''), m.get('tree', ''), m.get('tach', '')]
    return [f'hudrace {width}', latin('|'.join(str(f) for f in fields))]


def layouts(*commands):
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        return [json.loads(line) for line in run(exe, [line for c in commands for line in c])]


def check_all(mine, els, pairs):
    problems = []
    for name, path in pairs:
        if not close(mine[name], els[path]):
            problems.append((name, [round(v, 3) for v in mine[name]], [round(v, 3) for v in els[path]]))
    return problems


CARD = '/div:1/div:0'
ORDER = '/div:2/div:0'
TOAST = '/div:0/div:1'
BL = '/div:3'
BR = '/div:4'
RACE_PAIRS = [
    ('mirror', '/div:0/div:0'), ('toast', TOAST), ('toastIcon', TOAST + '/div:0'),
    ('toastTag', TOAST + '/div:1/div:0'), ('toastTitle', TOAST + '/div:1/div:1'), ('toastSub', TOAST + '/div:1/div:2'),
    ('card', CARD), ('title', CARD + '/div:0'), ('sep', CARD + '/div:2'),
    ('pos', CARD + '/div:1/div:0/div:0'), ('of', CARD + '/div:1/div:0/div:1'),
    ('lapLabel', CARD + '/div:1/div:1/div:0'), ('lapValue', CARD + '/div:1/div:1/div:1'),
    ('order', ORDER), ('orderTitle', ORDER + '/div:0/div:0'), ('orderRight', ORDER + '/div:0/div:1'),
    ('jukebox', BL + '/div:0'), ('jukeboxIcon', BL + '/div:0/div:0/svg:0'), ('jukeboxLine', BL + '/div:0/div:0/div:1'),
    ('jukeboxTime', BL + '/div:0/div:0/div:2'), ('jukeboxBar', BL + '/div:0/div:1'),
    ('vehicle', BL + '/div:1'), ('integrityLabel', BL + '/div:1/div:0/div:0/div:0'),
    ('integrity', BL + '/div:1/div:0/div:0/div:1'), ('integrityBar', BL + '/div:1/div:1'),
    ('minimap', BR + '/div:0'), ('minimapView', BR + '/div:0/svg:0'),
    ('slip', BR + '/div:1'), ('slipLabel', BR + '/div:1/div:0/div:0'), ('slipValue', BR + '/div:1/div:0/div:1'),
    ('slipBar', BR + '/div:1/div:1'),
    ('cockpit', BR + '/div:2'), ('speed', BR + '/div:2/div:0/div:0/div:0'), ('unit', BR + '/div:2/div:0/div:0/div:1'),
    ('gearLabel', BR + '/div:2/div:0/div:1/div:0'), ('gearBox', BR + '/div:2/div:0/div:1/div:1'),
    ('gear', BR + '/div:2/div:0/div:1/div:1/div:0'), ('rpm', BR + '/div:2/div:1'),
    ('nosLabel', BR + '/div:2/div:2/div:0/div:0/div:0'), ('nosValue', BR + '/div:2/div:2/div:0/div:0/div:1'),
    ('nosBar', BR + '/div:2/div:2/div:0/div:1'),
    ('fuelLabel', BR + '/div:2/div:2/div:1/div:0/div:0'), ('fuelValue', BR + '/div:2/div:2/div:1/div:0/div:1'),
    ('fuelBar', BR + '/div:2/div:2/div:1/div:1'),
] + [(f'key{i}', CARD + f'/div:{3 + i}/div:0') for i in range(3)] \
  + [(f'value{i}', CARD + f'/div:{3 + i}/div:1') for i in range(3)] \
  + [(f'row{i}', ORDER + f'/div:1/div:{i}') for i in range(6)] \
  + [(f'rowPos{i}', ORDER + f'/div:1/div:{i}/div:0') for i in range(6)] \
  + [(f'rowName{i}', ORDER + f'/div:1/div:{i}/div:1') for i in range(6)] \
  + [(f'rowValue{i}', ORDER + f'/div:1/div:{i}/div:2') for i in range(6)]


def test_native_race_hud_matches_the_design():
    mine, = layouts(command(RACE_MODEL))
    problems = check_all(mine, RACE, RACE_PAIRS)
    assert not problems, problems


def test_the_opened_jukebox_matches_the_design():
    # the notifications board shows it in a stack 1150px from the left, 253.19px down
    model = dict(RACE_MODEL, open=1, time='1:42 / 3:58', line='SHUFFLE · REPEAT ALL')
    mine, = layouts(command(model))
    box = NOTIFY['/div:3/div:5']
    dx, dy = box[0] - mine['jukebox'][0], box[1] - mine['jukebox'][1]

    def moved(name):
        x, y, w, h = mine[name]
        return [x + dx, y + dy, w, h]

    panel = '/div:3/div:5'
    pairs = [('jukebox', panel), ('jukeboxState', panel + '/div:0/div:0'), ('jukeboxTime', panel + '/div:0/div:1'),
             ('jukeboxTitle', panel + '/div:1/div:0'), ('jukeboxArtist', panel + '/div:1/div:1'),
             ('jukeboxBar', panel + '/div:2'), ('jukeboxModes', panel + '/div:3/div:1')] \
        + [(f'jukeboxButton{i}', panel + f'/div:3/div:0/div:{i}') for i in range(3)]
    problems = [(n, moved(n), NOTIFY[p]) for n, p in pairs if not close(moved(n), NOTIFY[p])]
    assert not problems, problems


def test_the_columns_keep_to_their_corners_on_any_screen():
    # 4:3 and 21:9: the corners follow the edges, the centre stays centred
    narrow, wide = layouts(command(RACE_MODEL, 1200), command(RACE_MODEL, 2100))
    for mine, width in ((narrow, 1200), (wide, 2100)):
        assert mine['card'][0] == 32
        assert abs(mine['order'][0] + mine['order'][2] - (width - 32)) < 0.01
        assert abs(mine['cockpit'][0] + mine['cockpit'][2] - (width - 32)) < 0.01
        assert abs(mine['mirror'][0] + mine['mirror'][2] / 2 - width / 2) < 0.01
        assert mine['card'][0] + mine['card'][2] < mine['mirror'][0]
        assert mine['mirror'][0] + mine['mirror'][2] < mine['order'][0]


def test_long_names_are_cut_with_an_ellipsis():
    model = dict(RACE_MODEL, rows=[('01', 'A driver with a very long name indeed', 'LEADER')])
    mine, = layouts(command(model))
    assert mine['name0'].endswith('\x9e')
    assert mine['rowName0'][2] < 200


def test_the_car_with_weapons_and_an_item_matches_the_design():
    # the race with weapons: the car's card is 330px wide, armor 25, three slots
    model = dict(RACE_MODEL, integrity='71', armor='25', vehicleWidth=330, jukebox=0,
                 slots=[('FRONT', '140', 'Machine gun'), ('REAR', '3', 'Mines'), ('ITEM', '1', 'Smoke')])
    mine, = layouts(command(model))
    car = '/div:3/div:1'
    pairs = [('vehicle', car), ('integrityLabel', car + '/div:0/div:0/div:0'), ('integrity', car + '/div:0/div:0/div:1'),
             ('armorLabel', car + '/div:0/div:1/div:0'), ('armor', car + '/div:0/div:1/div:1'),
             ('integrityBar', car + '/div:1'), ('vehicleSep', car + '/div:2')]
    for i in range(3):
        slot = car + f'/div:3/div:{i}'
        pairs += [(f'slotLabel{i}', slot + '/div:0'), (f'slotIcon{i}', slot + '/div:1/svg:0'),
                  (f'slotValue{i}', slot + '/div:1/div:1'), (f'slotName{i}', slot + '/div:2')]
    problems = check_all(mine, RACING_DM, pairs)
    assert not problems, problems


def test_the_hud_icons_are_there():
    # tools/ui_webstyle/hud_design/build_hud_icons.py; cgame/cg_hud_modern.c registers them
    source = (ROOT / 'engine/code/cgame/cg_hud_modern.c').read_text()
    for name in ('hud_note', 'hud_prev', 'hud_play', 'hud_next', 'hud_chevron', 'hud_message',
                 'hud_gun', 'hud_mine', 'hud_smoke', 'hud_flag', 'hud_hill', 'hud_dot', 'hud_trophy'):
        png = (ROOT / f'baseq3r/menu/art/{name}.png').read_bytes()
        assert png[16:24] == (64).to_bytes(4, 'big') * 2, name
        assert f'"menu/art/{name}"' in source


# the sprint board of gen.py
SPRINT_MODEL = dict(
    RACE_MODEL, title='Sprint \u00b7 Timber Run', pos='P2', of='/ 4', lapLabel='TO GO', lapValue='1,252 m',
    progress='7:CP 4 / 7:58%', kv=[('TIME', '0:51.06'), ('BEST', '1:28.40'), ('TO P1', '+1.14')],
    rows=[('01', 'Dean', 'LEADER'), ('02', 'Player', '+1.14'), ('03', 'Chloe', '+3.02'), ('04', 'Tony', '+7.88')],
    mirror=0, tag='Pickup', toastTitle='NOS +25%', sub='BOTTLE 2 OF 3', jukebox=0, integrity='125', slip=0,
    speed='97', gear='3', nos='25%', fuel='94%')


def test_native_sprint_hud_matches_the_design():
    mine, = layouts(command(SPRINT_MODEL))
    card, order, toast, br = '/div:1/div:0', '/div:2/div:0', '/div:0/div:0', '/div:4'
    pairs = [('card', card), ('title', card + '/div:0'),
             ('pos', card + '/div:1/div:0/div:0'), ('of', card + '/div:1/div:0/div:1'),
             ('lapLabel', card + '/div:1/div:1/div:0'), ('lapValue', card + '/div:1/div:1/div:1'),
             ('progressBar', card + '/div:2'), ('progressLeft', card + '/div:3/div:0'),
             ('progressRight', card + '/div:3/div:1'), ('sep', card + '/div:4'),
             ('order', order), ('toast', toast), ('toastTag', toast + '/div:1/div:0'),
             ('toastTitle', toast + '/div:1/div:1'), ('toastSub', toast + '/div:1/div:2'),
             ('vehicle', '/div:3/div:0'), ('minimap', br + '/div:0'), ('cockpit', br + '/div:1'),
             ('speed', br + '/div:1/div:0/div:0/div:0'), ('gear', br + '/div:1/div:0/div:1/div:1/div:0')] \
        + [(f'key{i}', card + f'/div:{5 + i}/div:0') for i in range(3)] \
        + [(f'value{i}', card + f'/div:{5 + i}/div:1') for i in range(3)] \
        + [(f'rowName{i}', order + f'/div:1/div:{i}/div:1') for i in range(4)]
    problems = check_all(mine, SPRINT, pairs)
    assert not problems, problems
    # the checkpoints sit at i / (ticks + 1) of the bar (drawn by cg_hud_modern.c)
    bar = mine['progressBar']
    for i in range(1, 8):
        tick = SPRINT[card + f'/div:2/div:{i}']
        assert abs(bar[0] + bar[2] * i / 8 - tick[0]) < 0.5


# the ghost race board of gen.py: the gap to the ghost instead of the place, splits at the right
GHOST_MODEL = dict(
    RACE_MODEL, title='Ghost race \u00b7 Country', pos='-0.84', of='', posLabel='VS GHOST', lapValue='3/5',
    kv=[('LAP', '0:38.77'), ('BEST', '0:39.61'), ('GHOST', 'PERSONAL \u00b7 1:21.40')], kvSizes=[17, 17, 14],
    orderTitle='Splits', orderRight='vs ghost',
    rows=[('S1', 'Start straight', '-0.21'), ('S2', 'Forest hairpin', '-0.48'), ('S3', 'Jump', '+0.12'),
          ('S4', 'Finish', '\u2014')],
    mirror=1, toast=0, jukebox=0, integrity='125', minimap=0, slip=0, speed='121', gear='4')


def test_native_ghost_hud_matches_the_design():
    mine, = layouts(command(GHOST_MODEL))
    card, order = '/div:1/div:0', '/div:2/div:0'
    pairs = [('card', card), ('title', card + '/div:0'),
             ('posLabel', card + '/div:1/div:0/div:0'), ('pos', card + '/div:1/div:0/div:1'),
             ('lapLabel', card + '/div:1/div:1/div:0'), ('lapValue', card + '/div:1/div:1/div:1'),
             ('sep', card + '/div:2'), ('order', order), ('orderTitle', order + '/div:0/div:0'),
             ('orderRight', order + '/div:0/div:1'), ('mirror', '/div:0/div:0'), ('vehicle', '/div:3/div:0'),
             ('cockpit', '/div:4/div:0')] \
        + [(f'key{i}', card + f'/div:{3 + i}/div:0') for i in range(3)] \
        + [(f'value{i}', card + f'/div:{3 + i}/div:1') for i in range(3)] \
        + [(f'rowPos{i}', order + f'/div:1/div:{i}/div:0') for i in range(4)] \
        + [(f'rowName{i}', order + f'/div:1/div:{i}/div:1') for i in range(4)] \
        + [(f'rowValue{i}', order + f'/div:1/div:{i}/div:2') for i in range(4)]
    problems = check_all(mine, GHOST, pairs)
    assert not problems, problems


# the team race board of gen.py: the race with the team's colour and the teams over the order
TEAM_RACE_MODEL = dict(
    RACE_MODEL, title='Team race \u00b7 Downtown', toast=0,
    kv=[('LAP', '0:42.18'), ('BEST', '0:40.95'), ('TO P2', '+1.07'), ('YOUR TEAM', 'BLUE')],
    orderTitle='Team race order', teams=[('RED', '1:42.10'), ('BLUE', '--')], swatch=1)


def test_native_team_race_hud_matches_the_design():
    mine, = layouts(command(TEAM_RACE_MODEL))
    card, order = '/div:1/div:0', '/div:2/div:0'
    pairs = [('card', card), ('order', order), ('orderTitle', order + '/div:0/div:0'),
             ('orderRight', order + '/div:0/div:1')] \
        + [(f'key{i}', card + f'/div:{3 + i}/div:0') for i in range(4)] \
        + [(f'value{i}', card + f'/div:{3 + i}/div:1') for i in range(4)] \
        + [(f'team{i}', order + f'/div:1/div:{i}') for i in range(2)] \
        + [(f'teamName{i}', order + f'/div:1/div:{i}/div:0') for i in range(2)] \
        + [(f'teamValue{i}', order + f'/div:1/div:{i}/div:1') for i in range(2)]
    for i in range(6):
        row = order + f'/div:2/div:{i}'
        pairs += [(f'row{i}', row), (f'rowPos{i}', row + '/div:0'), (f'rowSwatch{i}', row + '/div:1'),
                  (f'rowName{i}', row + '/div:2'), (f'rowValue{i}', row + '/div:3')]
    problems = check_all(mine, TEAM_RACE, pairs)
    assert not problems, problems


# the big messages and the spectator's panel (gen.py Meldungen board)
def test_the_big_messages_match_the_design():
    warning, recovered, wide = layouts(['hudmsg 1600', 'WRONG WAY|56|300|TURN AROUND'],
                                       ['hudmsg 1600', 'CAR RECOVERED|44|420|BACK ON TRACK AT CP 4'],
                                       ['hudmsg 2100', 'WRONG WAY|56|300|TURN AROUND'])
    problems = [(n, mine[k], MESSAGES[p]) for n, mine, k, p in (
        ('warning', warning, 'title', '/div:1/div:0'), ('warning sub', warning, 'sub0', '/div:1/div:1'),
        ('recovered', recovered, 'title', '/div:2/div:0'), ('recovered sub', recovered, 'sub0', '/div:2/div:1'))
        if not close(mine[k], MESSAGES[p])]
    assert not problems, problems
    # centred on any screen
    assert abs(wide['title'][0] + wide['title'][2] / 2 - 1050) < 0.01


def test_a_long_centerprint_gets_smaller_and_then_cut():
    long, longer = layouts(['hudmsg 1600', 'Press FIRE or USE when ready to race, then wait for the others to be ready too|44|300|'],
                           ['hudmsg 1024', 'x' * 90 + '|44|300|'])
    assert 28 <= long['size'] < 44 and long['title'][2] <= 960
    assert longer['size'] == 28 and longer['titleText'].endswith('\x9e') and longer['title'][2] <= 1024 - 192


def test_the_spectator_panel_matches_the_design():
    mine, = layouts(['hudspec 1600', 'FOLLOWING|Sam|FIRE NEXT|ESC MENU'])
    panel = '/div:5/div:0'
    pairs = [('box', panel), ('label', panel + '/div:0/div:0/div:0'), ('name', panel + '/div:0/div:0/div:1'),
             ('hint1', panel + '/div:0/div:1/div:0'), ('hint2', panel + '/div:0/div:1/div:1')]
    problems = check_all(mine, MESSAGES, pairs)
    assert not problems, problems


# the race with weapons (gen.py RacingDM board): score in the card, the kill feed under the order
def test_native_race_combat_hud_matches_the_design():
    model = dict(RACE_MODEL, title='Race combat \u00b7 Downtown', pos='P2', lapValue='3/5',
                 kv=[('LAP', '0:44.02'), ('BEST', '0:41.37'), ('SCORE', '4')],
                 kills=[('Player', 'ROCKET', 'Paul'), ('Sam', 'MINES', 'Tony')])
    mine, = layouts(command(model))
    pairs = [('card', '/div:1/div:0'), ('order', '/div:2/div:0')] \
        + [(f'value{i}', f'/div:1/div:0/div:{3 + i}/div:1') for i in range(3)]
    for i in range(2):
        line = f'/div:2/div:{1 + i}'
        pairs += [(f'kill{i}', line), (f'killA{i}', line + '/div:0'), (f'killWeapon{i}', line + '/div:1'),
                  (f'killB{i}', line + '/div:2')]
    problems = check_all(mine, RACING_DM, pairs)
    assert not problems, problems


def test_a_death_without_a_killer_has_one_name():
    model = dict(RACE_MODEL, kills=[('Paul', 'CRASH', '')])
    mine, = layouts(command(model))
    assert abs(mine['kill0'][2] - (10 + mine['killA0'][2] + 8 + mine['killWeapon0'][2] + 10)) < 0.01
    assert abs(mine['kill0'][0] + mine['kill0'][2] - 1568) < 0.01


# the elimination board of gen.py: places left, who goes out next, still in and out
def test_native_elimination_hud_matches_the_design():
    model = dict(RACE_MODEL, title='Elimination \u00b7 Country', pos='P3', of='/ 4', lapLabel='LEFT', lapValue='4/6',
                 elim='NEXT OUT;0:12;LAST PLACE IS OUT AT 0:00', kv=[('TIME', '1:47.30')],
                 orderTitle='Still in', orderRight='Status',
                 rows=[('01', 'Carla', 'SAFE'), ('02', 'Tony', 'SAFE'), ('03', 'Player', 'SAFE'), ('04', 'Janine', 'DANGER'),
                       ('--', 'Bobby', 'OUT'), ('--', 'Emma', 'OUT')], mirror=0, toast=0, jukebox=0)
    mine, = layouts(command(model))
    card, order = '/div:0/div:0', '/div:1/div:0'
    pairs = [('card', card), ('pos', card + '/div:1/div:0/div:0'), ('of', card + '/div:1/div:0/div:1'),
             ('lapLabel', card + '/div:1/div:1/div:0'), ('lapValue', card + '/div:1/div:1/div:1'),
             ('sep', card + '/div:2'), ('elimLabel', card + '/div:3/div:0/div:0'),
             ('elimValue', card + '/div:3/div:0/div:1'), ('elimBar', card + '/div:3/div:1'),
             ('elimCaption', card + '/div:3/div:2'), ('key0', card + '/div:4/div:0'), ('value0', card + '/div:4/div:1'),
             ('order', order), ('orderTitle', order + '/div:0/div:0'), ('orderRight', order + '/div:0/div:1')]
    for i in range(6):
        row = order + f'/div:1/div:{i}'
        pairs += [(f'rowPos{i}', row + '/div:0'), (f'rowName{i}', row + '/div:1'), (f'rowValue{i}', row + '/div:2')]
    problems = check_all(mine, ELIMINATION, pairs)
    assert not problems, problems


# the last car standing board: cars left and score, the integrity of everyone still in
def test_native_last_car_standing_hud_matches_the_design():
    model = dict(RACE_MODEL, title='Last car standing \u00b7 DD02', posLabel='LEFT', pos='3 / 6', posSize=56, of='',
                 lapLabel='SCORE', lapValue='2', kv=[('MATCH', '2:41'), ('LONGEST ALIVE', 'Carla')],
                 orderTitle='Still in', orderRight='Integrity', rowBars=1,
                 rows=[('', 'Player', '88'), ('', 'Carla', '112'), ('', 'Josh', '21'), ('', 'Minori', 'OUT'),
                       ('', 'Sam', 'OUT'), ('', 'Ivan', 'OUT')], toast=0, jukebox=0)
    mine, = layouts(command(model))
    card, order = '/div:1/div:0', '/div:2/div:0'
    pairs = [('card', card), ('posLabel', card + '/div:1/div:0/div:0'), ('pos', card + '/div:1/div:0/div:1'),
             ('lapLabel', card + '/div:1/div:1/div:0'), ('lapValue', card + '/div:1/div:1/div:1'),
             ('sep', card + '/div:2'), ('key0', card + '/div:3/div:0'), ('value0', card + '/div:3/div:1'),
             ('key1', card + '/div:4/div:0'), ('value1', card + '/div:4/div:1'),
             ('order', order), ('orderTitle', order + '/div:0/div:0'), ('orderRight', order + '/div:0/div:1')]
    for i in range(6):
        row = order + f'/div:1/div:{i}'
        pairs += [(f'row{i}', row), (f'rowName{i}', row + '/div:0'), (f'rowBar{i}', row + '/div:1'),
                  (f'rowValue{i}', row + '/div:2')]
    problems = check_all(mine, LCS, pairs)
    assert not problems, problems


# the derby board: score and the no-ram clock, the car from above with its worst zone
def test_native_derby_card_matches_the_design():
    model = dict(RACE_MODEL, title='Derby \u00b7 David5', posLabel='SCORE', pos='840', posSize=56, of='',
                 lapLabel='NO RAM', lapValue='0:39', kv=[], derby='FRONT CRITICAL', toast=0, jukebox=0)
    mine, = layouts(command(model))
    card, block = '/div:1/div:0', '/div:1/div:0/div:3'
    pairs = [('card', card), ('posLabel', card + '/div:1/div:0/div:0'), ('pos', card + '/div:1/div:0/div:1'),
             ('lapLabel', card + '/div:1/div:1/div:0'), ('lapValue', card + '/div:1/div:1/div:1'),
             ('sep', card + '/div:2'), ('derbyCar', block + '/svg:0'), ('derbyStatus', block + '/div:1/div:0')]
    for i in range(3):
        pairs += [(f'derbySwatch{i}', block + f'/div:1/div:{1 + i}/div:0'),
                  (f'derbyLegend{i}', block + f'/div:1/div:{1 + i}/div:1')]
    problems = check_all(mine, DERBY, pairs)
    assert not problems, problems


# the deathmatch board: frags against the limit and the rank, the frag order and the kill feed
def test_native_deathmatch_hud_matches_the_design():
    model = dict(RACE_MODEL, title='Deathmatch \u00b7 Downtown', pos='12', of='/ 20', lapLabel='RANK', lapValue='2nd',
                 kv=[('TIME LEFT', '6:12'), ('LEADER', 'Chloe \u00b7 14')], orderTitle='Frag order', orderRight='Frags',
                 rows=[('01', 'Chloe', '14'), ('02', 'Player', '12'), ('03', 'Paul', '9'), ('04', 'Sam', '7'), ('05', 'Vlad', '3')],
                 kills=[('Player', 'RAIL', 'Vlad'), ('Chloe', 'ROCKET', 'Sam'), ('Paul', 'MG', 'Player')],
                 mirror=0, toast=0, jukebox=0)
    mine, = layouts(command(model))
    card, order = '/div:0/div:0', '/div:1/div:0'
    pairs = [('card', card), ('pos', card + '/div:1/div:0/div:0'), ('of', card + '/div:1/div:0/div:1'),
             ('lapLabel', card + '/div:1/div:1/div:0'), ('lapValue', card + '/div:1/div:1/div:1'),
             ('order', order)] \
        + [(f'key{i}', card + f'/div:{3 + i}/div:0') for i in range(2)] \
        + [(f'value{i}', card + f'/div:{3 + i}/div:1') for i in range(2)] \
        + [(f'kill{i}', f'/div:1/div:{1 + i}') for i in range(3)]
    for i in range(5):
        row = order + f'/div:1/div:{i}'
        pairs += [(f'rowPos{i}', row + '/div:0'), (f'rowName{i}', row + '/div:1'), (f'rowValue{i}', row + '/div:2')]
    problems = check_all(mine, DEATHMATCH, pairs)
    assert not problems, problems


# the team deathmatch board: your team and frags, the team frag order with the teams over it
def test_native_team_deathmatch_hud_matches_the_design():
    model = dict(RACE_MODEL, title='Team combat \u00b7 Battle Canyon', posLabel='YOUR TEAM', pos='Blue', posSize=44, of='',
                 lapLabel='YOUR FRAGS', lapValue='6', kv=[('TIME LEFT', '8:20'), ('LIMIT', '50')],
                 orderTitle='Team frag order', orderRight='Frags', teams=[('RED', '29'), ('BLUE', '34')], swatch=1,
                 rows=[('01', 'Sam', '11'), ('02', 'Vlad', '9'), ('03', 'Player', '6'), ('04', 'Tobias', '6'),
                       ('05', 'Minori', '5'), ('06', 'Chloe', '3')],
                 kills=[('Sam', 'ROCKET', 'Tobias')], mirror=0, toast=0, jukebox=0)
    mine, = layouts(command(model))
    card, order = '/div:0/div:0', '/div:1/div:0'
    pairs = [('card', card), ('posLabel', card + '/div:1/div:0/div:0'), ('pos', card + '/div:1/div:0/div:1'),
             ('lapLabel', card + '/div:1/div:1/div:0'), ('lapValue', card + '/div:1/div:1/div:1'),
             ('order', order), ('kill0', '/div:1/div:1')] \
        + [(f'key{i}', card + f'/div:{3 + i}/div:0') for i in range(2)] \
        + [(f'value{i}', card + f'/div:{3 + i}/div:1') for i in range(2)] \
        + [(f'team{i}', order + f'/div:1/div:{i}') for i in range(2)]
    for i in range(6):
        row = order + f'/div:2/div:{i}'
        pairs += [(f'rowPos{i}', row + '/div:0'), (f'rowSwatch{i}', row + '/div:1'), (f'rowName{i}', row + '/div:2'),
                  (f'rowValue{i}', row + '/div:3')]
    problems = check_all(mine, TEAMDM, pairs)
    assert not problems, problems


# the four team CTF board: your team in a row, a line per flag, the teams' captures over the order
def test_native_ctf_hud_matches_the_design():
    model = dict(RACE_MODEL, title='Four team CTF \u00b7 CTF4_01', posLabel='YOUR TEAM', pos='Green', headerRow=1, of='',
                 lapValue='', flags=[('Red', 'CARRIED', 'Player'), ('Blue', 'AT BASE', ''), ('Green', 'DROPPED', '0:12'),
                                     ('Yellow', 'AT BASE', '')],
                 kv=[('TIME LEFT', '11:03')], orderTitle='CTF driver order', orderRight='Caps',
                 teams=[('R', '1'), ('B', '2'), ('G', '1'), ('Y', '0')], swatch=1,
                 rows=[('01', 'Ivan', '2'), ('02', 'Player', '1'), ('03', 'Kuro', '1'), ('04', 'Minori', '0')],
                 toast=0, jukebox=0)
    mine, = layouts(command(model))
    card, order = '/div:1/div:0', '/div:2/div:0'
    pairs = [('card', card), ('posLabel', card + '/div:1/div:0'), ('pos', card + '/div:1/div:1'), ('sep', card + '/div:2'),
             ('sep2', card + '/div:4'), ('key0', card + '/div:5/div:0'), ('value0', card + '/div:5/div:1'), ('order', order)] \
        + [(f'team{i}', order + f'/div:1/div:{i}') for i in range(4)]
    for i in range(4):
        line = card + f'/div:3/div:{i}'
        pairs += [(f'flagIcon{i}', line + '/svg:0'), (f'flagName{i}', line + '/div:1'), (f'flagState{i}', line + '/div:2')]
        if i in (0, 2):
            pairs.append((f'flagNote{i}', line + '/div:3'))
    problems = check_all(mine, CTF, pairs)
    assert not problems, problems


def test_native_domination_hud_matches_the_design():
    model = dict(RACE_MODEL, title='Domination \u00b7 Babel', pos='', of='', lapValue='', zones='ABCDE~CAPTURING C~65%',
                 kv=[('YOUR TEAM', 'BLUE'), ('TIME LEFT', '9:41')], orderTitle='Domination order', orderRight='Score',
                 teams=[('R', '112'), ('B', '131'), ('G', '64'), ('Y', '40')], swatch=1,
                 rows=[('01', 'Dean', '41'), ('02', 'Player', '38'), ('03', 'Janine', '22'), ('04', 'Carla', '17')],
                 toast=0, jukebox=0)
    mine, = layouts(command(model))
    card, order = '/div:2/div:0', '/div:3/div:0'
    pairs = [('card', card), ('title', card + '/div:0'), ('zoneLabel', card + '/div:2/div:0/div:0'),
             ('zoneValue', card + '/div:2/div:0/div:1'), ('zoneBar', card + '/div:2/div:1'), ('sep', card + '/div:3'),
             ('key0', card + '/div:4/div:0'), ('value0', card + '/div:4/div:1'),
             ('key1', card + '/div:5/div:0'), ('value1', card + '/div:5/div:1'), ('order', order)] \
        + [(f'zoneTile{i}', card + f'/div:1/div:{i}') for i in range(5)] \
        + [(f'zoneName{i}', card + f'/div:1/div:{i}/div:0') for i in range(5)] \
        + [(f'team{i}', order + f'/div:1/div:{i}') for i in range(4)]
    problems = check_all(mine, DOMINATION, pairs)
    assert not problems, problems


def test_native_koth_hud_matches_the_design():
    model = dict(RACE_MODEL, title='King of the hill \u00b7 Mesa King', pos='', of='', lapValue='',
                 hill='HILL HELD BY~Red', zones='~CONTESTED~2 v 1',
                 kv=[('RESPAWN WAVE', '0:07'), ('TIME LEFT', '4:55')], orderTitle='KOTH driver order', orderRight='Pts',
                 teams=[('RED', '52'), ('BLUE', '30')], swatch=1,
                 rows=[('01', 'Ivan', '28'), ('02', 'Tanya', '24'), ('03', 'Player', '19'), ('04', 'Paul', '11')],
                 toast=0, jukebox=0)
    mine, = layouts(command(model))
    card, order = '/div:1/div:0', '/div:2/div:0'
    pairs = [('card', card), ('title', card + '/div:0'), ('hillIcon', card + '/div:1/svg:0'),
             ('hillLabel', card + '/div:1/div:1/div:0'), ('hillOwner', card + '/div:1/div:1/div:1'),
             ('zoneLabel', card + '/div:2/div:0/div:0'), ('zoneValue', card + '/div:2/div:0/div:1'),
             ('zoneBar', card + '/div:2/div:1'), ('sep', card + '/div:3'),
             ('key0', card + '/div:4/div:0'), ('value0', card + '/div:4/div:1'),
             ('key1', card + '/div:5/div:0'), ('value1', card + '/div:5/div:1'), ('order', order)] \
        + [(f'team{i}', order + f'/div:1/div:{i}') for i in range(2)]
    problems = check_all(mine, KOTH, pairs)
    assert not problems, problems


def test_native_autoball_hud_matches_the_design():
    model = dict(RACE_MODEL, title='Autoball \u00b7 Rooftop', posLabel='YOUR TEAM', pos='Blue', headerRow=1, of='',
                 lapValue='', mirror=0, scorebug='RED~2~3:41~2ND HALF~1~BLUE',
                 kv=[('BALL', 'RED HALF \u00b7 42 M'), ('YOUR GOALS', '1'), ('POSSESSION', '46%')],
                 orderTitle='Autoball order', orderRight='Goals',
                 rows=[('01', 'Vlad', '2'), ('02', 'Player', '1'), ('03', 'Steve', '0'), ('04', 'Jennifer', '0')],
                 toast=0)
    mine, = layouts(command(model))
    bug, card, order = '/div:1/div:0', '/div:2/div:0', '/div:3/div:0'
    pairs = [('bugBox', bug), ('bugCell0', bug + '/div:0'), ('bugCell1', bug + '/div:1'), ('bugCell2', bug + '/div:2'),
             ('bugLeftLabel', bug + '/div:0/div:0'), ('bugLeft', bug + '/div:0/div:1'),
             ('bugClock', bug + '/div:1/div:0'), ('bugSub', bug + '/div:1/div:1'),
             ('bugRight', bug + '/div:2/div:0'), ('bugRightLabel', bug + '/div:2/div:1'),
             ('card', card), ('posLabel', card + '/div:1/div:0'), ('pos', card + '/div:1/div:1'), ('sep', card + '/div:2'),
             ('order', order)] \
        + [(f'key{i}', card + f'/div:{3 + i}/div:0') for i in range(3)] \
        + [(f'value{i}', card + f'/div:{3 + i}/div:1') for i in range(3)] \
        + [(f'row{i}', order + f'/div:1/div:{i}') for i in range(4)]
    problems = check_all(mine, AUTOBALL, pairs)
    assert not problems, problems


def test_native_drag_hud_matches_the_design():
    model = dict(RACE_MODEL, title='Drag \u00b7 Quarter mile', pos='', of='', lapValue='', labelRow='YOU~LANE LEFT',
                 kv=[('REACTION', '0.412'), ('60 FT', '2.08'), ('330 FT', '--'), ('1/8 MILE', '--'), ('1/4 MILE', '--'),
                     ('TRAP', '--')], kvSizes=[],
                 orderTitle='Duel \u00b7 Heat 2/3', orderRight='ET', rows=[('L', 'Player', '--'), ('R', 'Vlad', '--')],
                 tree='PRE~STAGE~~~~GO', tach='5,900~LAUNCH WINDOW~5,500 - 7,000~1~MPH~0~IDLE~REV LIMIT \u00b7 ENGINE BLOWS',
                 mirror=0, toast=0, jukebox=0, armor='')
    mine, = layouts(command(model))
    card, order, tree, tach = '/div:2/div:0', '/div:3/div:0', '/div:3/div:1', '/div:4/div:0'
    pairs = [('card', card), ('title', card + '/div:0'), ('labelRowLeft', card + '/div:1/div:0'),
             ('labelRowRight', card + '/div:1/div:1'), ('sep', card + '/div:2'), ('order', order),
             ('rowPos0', order + '/div:1/div:0/div:0'),
             ('treeBox', tree), ('treeTitle', tree + '/div:0'),
             ('tachBox', tach), ('tachRpmLabel', tach + '/div:0/div:0/div:0'), ('tachRpm', tach + '/div:0/div:0/div:1'),
             ('tachMidLabel', tach + '/div:0/div:1/div:0'), ('tachMidValue', tach + '/div:0/div:1/div:1'),
             ('tachGearLabel', tach + '/div:0/div:2/div:0/div:0'), ('tachGear', tach + '/div:0/div:2/div:0/div:1'),
             ('tachUnit', tach + '/div:0/div:2/div:1/div:0'), ('tachSpeed', tach + '/div:0/div:2/div:1/div:1'),
             ('tachSeg0', tach + '/div:1/div:0'), ('tachSeg39', tach + '/div:1/div:39'),
             ('tachLeft', tach + '/div:2/div:0'), ('tachRight', tach + '/div:2/div:1'),
             ('vehicle', '/div:5/div:0')] \
        + [(f'key{i}', card + f'/div:{3 + i}/div:0') for i in range(6)] \
        + [(f'value{i}', card + f'/div:{3 + i}/div:1') for i in range(2)] \
        + [(f'treeLamp{i}{j}', tree + f'/div:1/div:{i}/div:{j}') for i in range(6) for j in range(2)] \
        + [(f'treeLabel{i}', tree + f'/div:1/div:{i}/div:2') for i in (0, 1, 5)]
    problems = check_all(mine, DRAG, pairs)
    assert not problems, problems


def end_command(fields, width=1600):
    return [f'hudend {width}', latin('|'.join(str(f) for f in fields))]


END_MODEL = ['RACE RESULT', 'DOWNTOWN \u00b7 5 LAPS \u00b7 8 DRIVERS', 'P2', '/ 8', 'FINISHED', '4:21.380',
             '+1.142 TO THE WINNER',
             'BEST LAP~0:51.204~;TOP SPEED~214~KM/H;PLACES GAINED~+3~;AVG LAP~0:52.276~',
             '1~+2.61~L1;0.47~+1.22~L2;0~best~L3;0.18~+0.46~L4;0.42~+1.09~L5',
             'Sam~4:20.238~~WINNER~0;Player~4:21.380~+1.142~SECOND~1;Carla~4:24.145~+3.907~THIRD~0',
             'FASTEST LAP~Sam~0:50.871;BEST SECTOR~Player \u00b7 S3~0:11.402;BIGGEST GAIN~Player \u00b7 P5 > P2~+3',
             'FINAL CLASSIFICATION', '5 LAPS', 0,
             ';'.join(f'{p}~{n}~{t}~{b}' for p, n, t, b in [
                 ('01', 'Sam', '4:20.238', '0:50.871'), ('02', 'Player', '+1.142', '0:51.204'),
                 ('03', 'Carla', '+3.907', '0:51.660'), ('04', 'Tony', '+9.415', '0:52.013'),
                 ('05', 'Janine', '+12.770', '0:52.390'), ('06', 'Ivan', '+21.004', '0:52.881'),
                 ('07', 'Minori', '+1 LAP', '0:53.512'), ('08', 'Bee', 'DNF', '0:54.120')]),
             'FIRE~READY;TAB~DETAILS;ESC~MENU', 'WAITING FOR 5 DRIVERS', '3 / 8 READY']


def test_native_end_screen_matches_the_design():
    mine, = layouts(end_command(END_MODEL))
    left, pod, aw, lst, foot = '/div:3', '/div:4', '/div:5', '/div:6/div:0', '/div:7'
    pairs = [('title', '/div:2/div:0'), ('subtitle', '/div:2/div:1'),
             ('pos', left + '/div:0/div:0'), ('of', left + '/div:0/div:1'),
             ('doneLabel', left + '/div:1/div:0'), ('doneTime', left + '/div:1/div:1'), ('doneNote', left + '/div:1/div:2'),
             ('sep0', left + '/div:2'), ('sep1', left + '/div:4'),
             ('lapsTitle', left + '/div:5/div:0/div:0'), ('lapsNote', left + '/div:5/div:0/div:1'),
             ('awardsTitle', aw + '/div:0'), ('list', lst), ('listTitle', lst + '/div:0/div:0'),
             ('listRight', lst + '/div:0/div:1'),
             ('readyLabel', foot + '/div:1/div:0/div:0'), ('readyValue', foot + '/div:1/div:0/div:1'),
             ('readyBar', foot + '/div:1/div:1')]
    pairs += [(f'statLabel{i}', left + f'/div:3/div:{i}/div:0') for i in range(4)]
    pairs += [(f'statValue{i}', left + f'/div:3/div:{i}/div:1') for i in range(4)]
    pairs += [(f'lapSlot{i}', left + f'/div:5/div:1/div:{i}') for i in range(5)]
    # the podium, by place: the winner stands in the middle
    for place, div in ((0, 1), (1, 0), (2, 2)):
        card = pod + f'/div:{div}'
        pairs += [(f'podium{place}', card), (f'podiumCar{place}', card + '/div:0'),
                  (f'podiumName{place}', card + '/div:1/div:1')]
    # the third award's "P5 \u2192 P2" has an arrow the game's font does not have
    for i in range(2):
        pairs += [(f'award{i}', aw + f'/div:{i + 1}'), (f'awardLabel{i}', aw + f'/div:{i + 1}/div:1/div:0'),
                  (f'awardValue{i}', aw + f'/div:{i + 1}/div:2/div:0')]
    pairs += [(f'head{i}', lst + f'/div:1/div:{i}') for i in range(4)]
    pairs += [(f'row{i}', lst + f'/div:2/div:{i}') for i in range(8)]
    pairs += [(f'keyName{i}', foot + f'/div:0/div:{i}/div:0') for i in range(3)]
    pairs += [(f'keyAction{i}', foot + f'/div:0/div:{i}/div:1') for i in range(3)]
    problems = check_all(mine, END, pairs)
    assert not problems, problems


def _spread(teams, lo=258.0, hi=281.0):
    frac = lambda t: f'{(t - lo) / (hi - lo):.5f}'
    return ';'.join(f"{','.join(frac(t) for t in times)}~{frac(avg)}~{gap}" for times, avg, gap in teams)


END_TEAM_MODEL = ['TEAM RACE RESULT', 'DOWNTOWN \u00b7 5 LAPS \u00b7 3 TEAMS \u00b7 8 DRIVERS', 'P1', '/ 3',
                  'TEAM BLUE \u00b7 AVERAGE', '4:26.378', 'WON BY 2.070',
                  'YOUR PLACE~P3~;YOUR TIME~4:24.145~;VS TEAM AVERAGE~-2.233~;BEST LAP~0:51.660~',
                  '',
                  'Blue~4:26.378~~WINNER~1;Green~4:28.448~+2.070~SECOND~0;Red~4:29.915~+3.537~THIRD~0',
                  'FASTEST LAP~Sam~0:50.871;TEAM PLAYER~Sam \u00b7 6.140 under blue~-6.140;BIGGEST GAIN~Tony \u00b7 P7 > P4~+3',
                  'FINAL CLASSIFICATION', 'TEAM AVERAGE', 0,
                  ';'.join(f'{p}~{n}~{t}~{b}' for p, n, t, b in [
                      ('01', 'Sam', '4:20.238', '0:50.871'), ('02', 'Carla', '+1.142', '0:51.204'),
                      ('03', 'Player', '+3.907', '0:51.660'), ('04', 'Tony', '+5.415', '0:52.013'),
                      ('05', 'Janine', '+8.770', '0:52.390'), ('06', 'Ivan', '+11.004', '0:52.881'),
                      ('07', 'Minori', '+14.512', '0:53.120'), ('08', 'Bee', '+19.120', '0:53.512')]),
                  'FIRE~READY;TAB~DETAILS;ESC~MENU', 'WAITING FOR 5 DRIVERS', '3 / 8 READY',
                  'TEAM AVERAGES~A DOT A DRIVER \u00b7 TO THE WINNER',
                  _spread([([260.238, 264.145, 274.750], 266.378, 'best'),
                           ([261.380, 269.008, 279.358], 269.915, '+3.537'),
                           ([265.653, 271.242], 268.448, '+2.070')]),
                  'YOUR TEAM',
                  'P1:Sam,P3:Player,P7:Minori;P4:Tony,P6:Ivan;P2:Carla,P5:Janine,P8:Bee',
                  'BLUE~4:26.378;GREEN~4:28.448;RED~4:29.915', 1]


def test_native_team_end_screen_matches_the_design():
    mine, = layouts(end_command(END_TEAM_MODEL))
    left, pod, aw, lst = '/div:3', '/div:4', '/div:5', '/div:6/div:0'
    spread = left + '/div:5/div:1'
    pairs = [('pos', left + '/div:0/div:0'), ('of', left + '/div:0/div:1'),
             ('doneLabel', left + '/div:1/div:0'), ('doneTime', left + '/div:1/div:1'), ('doneNote', left + '/div:1/div:2'),
             ('lapsTitle', left + '/div:5/div:0/div:0'), ('lapsNote', left + '/div:5/div:0/div:1'),
             ('awardsTitle', aw + '/div:0'), ('list', lst), ('listTitle', lst + '/div:0/div:0'),
             ('listRight', lst + '/div:0/div:1')]
    pairs += [(f'statValue{i}', left + f'/div:3/div:{i}/div:1') for i in range(4)]
    dots = (3, 3, 2)
    for t in range(3):
        r = spread + f'/div:{t}'
        pairs += [(f'spreadSwatch{t}', r + '/div:0'), (f'spreadTrack{t}', r + '/div:1/div:0'),
                  (f'spreadTick{t}', r + f'/div:1/div:{dots[t] + 1}'), (f'spreadValue{t}', r + '/div:2/div:0')]
        pairs += [(f'spreadDot{t}_{k}', r + f'/div:1/div:{k + 1}') for k in range(dots[t])]
    members = (3, 2, 3)
    for place, div in ((0, 1), (1, 0), (2, 2)):
        card = pod + f'/div:{div}'
        pairs += [(f'podium{place}', card), (f'podiumCar{place}', card + '/div:0'),
                  (f'podiumPlace{place}', card + '/div:1/div:0/div:0'), (f'podiumTag{place}', card + '/div:1/div:0/div:1/div:' + ('1' if place == 0 else '0')),
                  (f'podiumName{place}', card + '/div:1/div:1'), (f'podiumTime{place}', card + '/div:1/div:2/div:0'),
                  (f'podiumRule{place}', card + '/div:1/div:3')]
        pairs += [(f'member{place}_{k}', card + f'/div:1/div:4/div:{k}/div:1') for k in range(members[place])]
        pairs += [(f'memberPos{place}_{k}', card + f'/div:1/div:4/div:{k}/div:0') for k in range(members[place])]
    pairs += [('podiumYou0', pod + '/div:1/div:1/div:0/div:1/div:0')]
    for i in range(2):
        pairs += [(f'award{i}', aw + f'/div:{i + 1}'), (f'awardLabel{i}', aw + f'/div:{i + 1}/div:1/div:0'),
                  (f'awardValue{i}', aw + f'/div:{i + 1}/div:2/div:0')]
    for c in range(3):
        pairs += [(f'teamCell{c}', lst + f'/div:1/div:{c}'), (f'teamCellName{c}', lst + f'/div:1/div:{c}/div:0'),
                  (f'teamCellValue{c}', lst + f'/div:1/div:{c}/div:1')]
    pairs += [('head0', lst + '/div:2/div:0'), ('head1', lst + '/div:2/div:2'),
              ('head2', lst + '/div:2/div:3'), ('head3', lst + '/div:2/div:4')]
    pairs += [(f'row{i}', lst + f'/div:3/div:{i}') for i in range(8)]
    pairs += [(f'rowSwatch{i}', lst + f'/div:3/div:{i}/div:1') for i in range(8)]
    problems = check_all(mine, END_TEAM, pairs)
    assert not problems, problems
