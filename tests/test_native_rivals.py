"""The native Rivals screen has to look like the WebUI's screen 5.

tests/fixtures/webui_rivals_layout.json holds what Chrome reports for the
WebUI's Rivals screen (tools/ui_webstyle/measure_webui_rivals.cjs: ten
viewports; the start, a search and a scrolled list). Chrome is measured with
hidden scrollbars like the other screens; the native list shows CEF's 15px
scrollbar, so its buttons are 15px narrower than in the fixture where the
list overflows.
"""
import json
import pathlib
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close, font_of

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_rivals_layout.json').read_text())
RIVALS = FIXTURE['rivals']
KEYS = sorted(key for key in FIXTURE if key != 'rivals')


def test_native_rivals_match_the_webui_at_every_size():
    commands = []
    for key in KEYS:
        width, height = key.split('_')[0].split('x')
        els = FIXTURE[key]['els']
        pressed = [b[0] for b in els['buttons'] if b[5]]
        name, model, description, personality, weapon = RIVALS[pressed[0]]
        commands.append(f'rivals {width} {height} {len(els["buttons"])} {els["scroll"][0]}')
        commands.append('|'.join([name, personality, 'Favorite weapon / ' + weapon, description,
                                  'Drag to rotate / scroll to zoom']))
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        output = [json.loads(line) for line in run(exe, commands)]
    assert len(KEYS) == 30
    for key, mine in zip(KEYS, output):
        chrome = FIXTURE[key]['els']
        problems = []

        def check(name, value, reference):
            if not close(value, reference):
                problems.append((name, value, reference))

        for name in ('back', 'left', 'right', 'search', 'list', 'car'):
            check(name, mine[name], chrome[name])
        check('h1', [mine['h1']], [chrome['h1'][1]])
        check('scroll', [mine['scroll'], mine['content']], chrome['scroll'][:2])
        for name, element in (('searchLabel', 'searchLabel'), ('count', 'count'), ('label', 'detailLabel'),
                              ('name', 'name'), ('personality', 'personality'), ('weapon', 'weapon'),
                              ('description', 'description'), ('status', 'status')):
            reference = chrome[element][1] - (2 if element in ('searchLabel',) else 0)
            check(name, [mine[name]], [reference])
        check('page', [mine['page']], [FIXTURE[key]['height']])
        scrollbar = 15 if chrome['scroll'][1] > chrome['scroll'][3] else 0
        for row, button in enumerate(chrome['buttons']):
            check(f'button {row}', mine['buttons'][row], [button[1], button[2], button[3] - scrollbar, button[4]])
        assert not problems, (key, problems)


def test_rival_texts_measure_like_chrome():
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
