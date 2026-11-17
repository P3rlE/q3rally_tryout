"""The native Downloads screen has to look like the WebUI's screen 6.

tests/fixtures/webui_downloads_layout.json holds what Chrome reports for the
WebUI's Downloads screen (tools/ui_webstyle/measure_webui_downloads.cjs: ten
viewports, five states) with the catalog tests/fixtures/downloads_index.json.
Like for Rivals, the layout runs with hidden scrollbars as Chrome was measured;
in the game the list shows CEF's scrollbar where it overflows.
"""
import json
import pathlib
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close, font_of

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_downloads_layout.json').read_text())
ITEMS = FIXTURE['items']
KEYS = sorted(key for key in FIXTURE if key != 'items')
STATES = {'start': (2, 0), 'filter': (2, 0), 'search': (2, 0), 'downloading': (3, 42), 'installed': (4, 100)}
MESSAGES = ['Choose content to install.', 'Fetching catalog...', 'Catalog ready.',
            'Downloading trk_night_city_x.pk3 / 42%', 'Installed. Restart Q3Rally to use the new content.']


def test_native_downloads_match_the_webui_at_every_size():
    commands = []
    for key in KEYS:
        width, height = key.split('_')[0].split('x')
        els = FIXTURE[key]['els']
        (name, author, kind, version, ident), size, installed = ITEMS[[b[0] for b in els['buttons'] if b[5]][0]]
        detail = f'{kind} / {version} / {size / 1024:.1f} MB / ' + ('Installed' if installed else 'Available')
        commands.append(f'rivals {width} {height} {len(els["buttons"])} {els["scroll"][0]} 1')
        commands.append('|'.join([name, '', '', '', detail, 'By ' + author, MESSAGES[STATES[key.split('_')[1]][0]]]))
        commands.append('|'.join(ITEMS[b[0]][0][0] + (' / Installed' if ITEMS[b[0]][2] else '') for b in els['buttons']))
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        output = [json.loads(line) for line in run(exe, commands)]
    assert len(KEYS) == 50
    for key, mine in zip(KEYS, output):
        chrome = FIXTURE[key]['els']
        problems = []

        def check(name, value, reference):
            if not close(value, reference):
                problems.append((name, value, reference))

        for name in ('back', 'left', 'right', 'search', 'list', 'type', 'install', 'refresh', 'cancel', 'progress'):
            check(name, mine[name], chrome[name])
        check('h1', [mine['h1']], [chrome['h1'][1]])
        check('typeLabel', [mine['typeLabel']], [chrome['typeLabel'][1] - 2])
        for name, element in (('count', 'count'), ('label', 'detailLabel'), ('name', 'name'), ('author', 'author'),
                              ('status', 'status'), ('downloadStatus', 'downloadStatus')):
            check(name, [mine[name]], [chrome[element][1]])
        check('page', [mine['page']], [FIXTURE[key]['height']])
        for row, button in enumerate(chrome['buttons']):
            check(f'button {row}', mine['buttons'][row], [button[1], button[2], button[3], button[4]])
        assert not problems, (key, problems)


def test_download_texts_measure_like_chrome():
    runs = {}
    for key in KEYS:
        for text in FIXTURE[key]['texts']:
            if len(text['lines']) == 1 and text['text'] != 'All content':
                runs[(font_of(text), text['size'], text['spacing'], text['uppercase'], text['text'])] = text['lines'][0][2]
    keys = sorted(runs)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        widths = run(exe, [f'width {f} {s} {sp} {int(u)} {t}' for f, s, sp, u, t in keys])
    for key, width in zip(keys, widths):
        assert abs(float(width) - runs[key]) < 0.1, (key, width, runs[key])
