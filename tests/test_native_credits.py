"""The native end credits have to look like the WebUI's screen 3.

tests/fixtures/webui_credits_layout.json holds what Chrome reports for the
WebUI's credits (tools/ui_webstyle/measure_webui_credits.cjs: ten viewports,
the roll paused 0, 6, 20 and 45 seconds in). The native layout and text
measurement run here against it, through the loading screen test's harness.
"""
import json
import pathlib
import re
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close, font_of

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_credits_layout.json').read_text())
ITEMS = FIXTURE['items']
KEYS = sorted(key for key in FIXTURE if key != 'items')


def layouts():
    commands = []
    for key in KEYS:
        size, seconds = key.split('_')
        width, height = size.split('x')
        commands.append(f'credits {width} {height} {seconds} {len(ITEMS)} ' + ' '.join(str(k) for _, k in ITEMS))
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        return [json.loads(line) for line in run(exe, commands)], exe


def test_native_credits_match_the_webui_at_every_size_and_time():
    output, _ = layouts()
    assert len(KEYS) == 40
    for key, mine in zip(KEYS, output):
        chrome = FIXTURE[key]['els']
        problems = []

        def check(name, value, reference):
            if not close(value, reference):
                problems.append((name, value, reference))

        check('exit', mine['exit'], chrome['exit'])
        check('window', mine['window'], chrome['window'])
        check('h1', [mine['h1']], [chrome['h1'][1]])
        check('hint', [mine['hint']], [chrome['hint'][1]])
        check('page', [mine['page']], [FIXTURE[key]['height']])
        check('rollHeight', [mine['rollHeight']], [chrome['rollHeight']])
        for index, (line, box) in enumerate(zip(mine['lines'], chrome['lines'])):
            check(f'line {index}', line, [box[1], box[3]])
        assert len(mine['lines']) == len(chrome['lines']), key
        assert not problems, (key, problems)


def test_the_roll_runs_as_long_as_the_animation():
    output, _ = layouts()
    for key, mine in zip(KEYS, output):
        chrome = FIXTURE[key]['els']
        assert abs(mine['duration'] - (chrome['window'][3] + chrome['rollHeight']) / 38) < 0.02, key


def test_credit_lines_measure_and_center_like_chrome():
    runs = {}
    for key in KEYS:
        window = FIXTURE[key]['els']['window']
        for text in FIXTURE[key]['texts']:
            if len(text['lines']) == 1:
                line = text['lines'][0]
                runs[(font_of(text), text['size'], text['spacing'], text['uppercase'], text['text'])] = line[2]
    keys = sorted(runs)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        widths = run(exe, [f'width {f} {s} {sp} {int(u)} {t}' for f, s, sp, u, t in keys])
    for key, width in zip(keys, widths):
        assert abs(float(width) - runs[key]) < 0.1, (key, width, runs[key])
    # centered in the window as text-align: center does
    key = '1920x1080_20'
    window = FIXTURE[key]['els']['window']
    for text in FIXTURE[key]['texts']:
        line = text['lines'][0]
        if window[1] < line[1] < window[1] + window[3] and text['family'] != 'Barlow Condensed' or text['size'] == 48:
            if window[1] < line[1] < window[1] + window[3]:
                assert abs(window[0] + (window[2] - line[2]) / 2 - line[0]) < 0.5, text['text']


def test_the_fixture_lists_the_native_credits():
    source = (ROOT / 'engine/code/q3_ui/ui_credits.c').read_text()
    table = source[source.index('static cr_line_t credits[]'):source.index('{ NULL }')]
    native = re.findall(r'\{\s*(?:PRODUCT_VERSION\s*)?"([^"]*)"', table)
    assert [re.sub(r'^v[\d.]+', '', text) for text, _ in ITEMS] == native
