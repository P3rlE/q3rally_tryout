"""The native main menu has to look like the WebUI's screen 2.

tests/fixtures/webui_main_layout.json holds what Chrome reports for the
WebUI's main menu (tools/ui_webstyle/measure_webui_main.cjs: viewports from
800x600 to 3840x2160, with a profile, without one and with the quit dialog
open). The native layout and text measurement run here against it, through
the harness of the loading screen test.
"""
import json
import pathlib
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close, font_of

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_main_layout.json').read_text())


def summary_of(data):
    return next(text for text in data['texts'] if text['family'] == 'Barlow' and text['size'] == 13.6)


def test_native_main_menu_matches_the_webui_at_every_size():
    keys = sorted(FIXTURE)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        commands = []
        for key in keys:
            width, height = key.split('_')[0].split('x')
            commands.append(f'main {width} {height} {summary_of(FIXTURE[key])["text"]}')
        output = [json.loads(line) for line in run(exe, commands)]
    assert len(keys) >= 9
    for key, mine in zip(keys, output):
        chrome = FIXTURE[key]['els']
        problems = []

        def check(name, value, reference):
            if not close(value, reference):
                problems.append((name, value, reference))

        for index, button in enumerate(chrome['buttons']):
            check(f'button {index}', mine['buttons'][index], button)
        for name in ('right', 'car', 'garage'):
            check(name, mine[name], chrome[name])
        check('h1', [mine['h1']], [chrome['h1'][1]])
        check('status', mine['status'], chrome['status'][:2])
        check('vehicle', [mine['vehicle']], [chrome['vehicle'][1]])
        check('statusText', [mine['statusText']], [chrome['statusText'][1]])
        check('summary', mine['summary'], chrome['summary'][:2])
        check('page', [mine['page']], [FIXTURE[key]['height']])
        if key.endswith('quit'):
            for name in ('dialog', 'accept', 'cancel'):
                check(name, mine[name], chrome[name])
            check('dialog text', mine['dialogText'],
                  [chrome['dialogLabel'][0], chrome['dialogLabel'][1], chrome['dialogTitle'][1], chrome['dialogText'][1]])
        lines = {(text['text'], text['family']): len(text['lines']) for text in FIXTURE[key]['texts']}
        assert mine['lines'][0] == len(summary_of(FIXTURE[key])['lines']), key
        assert mine['lines'][1] == lines[('Alpine', 'Barlow Condensed')], key
        if key.endswith('quit'):
            assert mine['lines'][2] == lines[('Exit game?', 'Barlow Condensed')], key
        assert not problems, (key, problems)


def test_main_menu_texts_measure_like_chrome():
    runs = {}
    for data in FIXTURE.values():
        for text in data['texts']:
            if len(text['lines']) == 1:
                runs[(font_of(text), text['size'], text['spacing'], text['uppercase'], text['text'])] = text['lines'][0][2]
    keys = sorted(runs)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        widths = run(exe, [f'width {f} {s} {sp} {int(u)} {t}' for f, s, sp, u, t in keys])
    for key, width in zip(keys, widths):
        assert abs(float(width) - runs[key]) < 0.1, (key, width, runs[key])


def test_main_menu_labels_and_ranks():
    layout = (ROOT / 'engine/code/q3_ui/ui_rally_mainlayout.c').read_text()
    for label in ('Offline', 'Online', 'Garage', 'Config', 'Rivals', 'Demos', 'Downloads', 'Driver profile', 'Quit',
                  'Q3RALLY / FINISH YOUR SESSION', 'Exit game?', 'Thanks for racing with us.',
                  'Show credits and exit', 'Keep racing'):
        assert f'"{label}"' in layout, label
    menu = (ROOT / 'engine/code/q3_ui/ui_menu.c').read_text()
    assert 'PROFILE_RANK_TABLE( MAIN_RANK_NAME )' in menu
