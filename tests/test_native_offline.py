"""Step 1 of the native offline race setup has to look like the WebUI's.

tests/fixtures/webui_offline_layout.json holds what Chrome reports for the
WebUI's track and mode step (tools/ui_webstyle/measure_webui_offline.cjs:
ten viewports; the start, a search, a later track in a scrolled list) with
the game's own data, tests/fixtures/offline_model.json (ui_offlineDump 1).
"""
import json
import pathlib
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close, font_of

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_offline_layout.json').read_text())
MODEL = json.loads((ROOT / 'tests/fixtures/offline_model.json').read_text())
KEYS = sorted(FIXTURE)
STATES = {'start': ('', 0, 0), 'search': ('ri', 1, 0), 'picked': ('', 6, 200)}


def visible(query):
    return [title for ident, title in MODEL['maps'] if query in (title + ' ' + ident).lower()]


def test_native_offline_track_step_matches_the_webui_at_every_size():
    commands = []
    for key in KEYS:
        width, height = key.split('_')[0].split('x')
        query, pick, scroll = STATES[key.split('_')[1]]
        ident, title = MODEL['maps'][pick]
        commands.append(f'offline {width} {height} {FIXTURE[key]["els"]["scroll"][0]} 1')
        commands.append(f'{ident}|{title}')
        commands.append('|'.join(visible(query)))
        commands.append('|'.join(detail.strip() for detail in MODEL['details']))
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        output = [json.loads(line) for line in run(exe, commands)]
    assert len(KEYS) == 30
    for key, mine in zip(KEYS, output):
        chrome = FIXTURE[key]['els']
        problems = []

        def check(what, value, reference):
            if not close(value, reference):
                problems.append((what, value, reference))

        for what in ('back', 'left', 'right', 'mode', 'search', 'list', 'image', 'next'):
            check(what, mine[what], chrome[what])
        for what in ('label', 'h1', 'modeLabel', 'searchLabel', 'count', 'mapId', 'name'):
            check(what, [mine[what]], [chrome[what][1]])
        check('steps', mine['step'], [chrome['stepMap'][1], chrome['stepRules'][0]])
        check('details', mine['details'], [chrome['details'][1], chrome['details'][3]])
        check('page', [mine['page']], [FIXTURE[key]['height']])
        for row, button in enumerate(chrome['buttons']):
            check(f'button {row}', mine['buttons'][row], button[:4])
        for index, box in enumerate(chrome['detailLines']):
            # grid items stretch to their row: the text starts at the top all the same
            check(f'detail {index}', mine['detailBoxes'][index][:3], box[:3])
        assert not problems, (key, problems)


def test_offline_texts_measure_like_chrome():
    runs = {}
    for key in KEYS:
        for text in FIXTURE[key]['texts']:
            if len(text['lines']) == 1 and text['text'] not in ('Racing',):
                runs[(font_of(text), text['size'], text['spacing'], text['uppercase'], text['text'])] = text['lines'][0][2]
    keys = sorted(runs)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        widths = run(exe, [f'width {f} {s} {sp} {int(u)} {t}' for f, s, sp, u, t in keys])
    for key, width in zip(keys, widths):
        assert abs(float(width) - runs[key]) < (0.2 if key[1] == 14.4 else 0.1), (key, width, runs[key])
