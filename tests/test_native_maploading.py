"""The native map loading screen has to look like the WebUI's screen 14.

tests/fixtures/webui_maploading_layout.json holds what Chrome reports for the
WebUI's map loading screen (tools/ui_webstyle/measure_webui_maploading.cjs:
ten viewports; connecting, loading with a levelshot, downloading).
"""
import json
import pathlib
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close, font_of

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_maploading_layout.json').read_text())
KEYS = sorted(FIXTURE)
TEXTS = {'connecting': (0, 'Preparing your race', 'race.example.net:27960', 'Awaiting challenge', 'Connection attempt 3'),
         'loading': (1, 'Canyon Rush', 'Local race', 'Loading vehicles', ''),
         'download': (1, 'q3r_battlecanyon', 'race.example.net:27960', 'Downloading maps/q3r_newtrack.pk3',
                      '36% · 3.3 / 9.0 MB')}


def lines_of(key, text):
    return len(next(t for t in FIXTURE[key]['texts'] if t['text'] == text)['lines'])


def test_native_map_loading_matches_the_webui_at_every_size():
    commands = []
    for key in KEYS:
        width, height = key.split('_')[0].split('x')
        image, name, server, detail, transfer = TEXTS[key.split('_')[1]]
        commands.append(f'mapload {width} {height} {image}')
        commands.append('|'.join([name, server, detail, transfer]))
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        output = [json.loads(line) for line in run(exe, commands)]
    assert len(KEYS) == 30
    for key, mine in zip(KEYS, output):
        chrome = FIXTURE[key]['els']
        image, name, server, detail, transfer = TEXTS[key.split('_')[1]]
        problems = []

        def check(what, value, reference):
            if not close(value, reference):
                problems.append((what, value, reference))

        for what in ('preview', 'status', 'progress', 'cancel'):
            check(what, mine[what], chrome[what])
        if image:
            check('image', mine['image'], chrome['image'])
        label = next(t for t in FIXTURE[key]['texts'] if t['text'] == 'Q3RALLY / NEXT STAGE')['lines'][0]
        check('label', [mine['label']], [label[1] - 2])
        check('h1', mine['h1'], chrome['h1'][:2])
        check('name', [mine['name'][0]], [chrome['name'][1]])
        check('server', [mine['server']], [chrome['server'][1]])
        check('statusLabel', [mine['statusLabel']], [chrome['statusLabel'][1]])
        check('detail', [mine['detail'][0]], [chrome['detail'][1]])
        check('transfer', [mine['transfer']], [chrome['transfer'][1]])
        check('note', [mine['note'][0]], [chrome['note'][1]])
        assert mine['detail'][1] == lines_of(key, detail), key
        assert mine['note'][1] == lines_of(key, 'Your race starts when the track and vehicles are ready.'), key
        assert not problems, (key, problems)


def test_map_loading_texts_measure_like_chrome():
    runs = {}
    for key in KEYS:
        for text in FIXTURE[key]['texts']:
            if len(text['lines']) == 1:
                runs[(font_of(text), text['size'], text['spacing'], text['uppercase'], text['text'])] = text['lines'][0][2]
    keys = sorted(runs)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        widths = run(exe, [f'width {f} {s} {sp} {int(u)} {t}' for f, s, sp, u, t in keys])
    for key, width in zip(keys, widths):
        assert abs(float(width) - runs[key]) < 0.1, (key, width, runs[key])
