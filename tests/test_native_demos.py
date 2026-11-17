"""The native demo archive has to look like the WebUI's screen 4.

tests/fixtures/webui_demos_layout.json holds what Chrome reports for the
WebUI's demo archive (tools/ui_webstyle/measure_webui_demos.cjs: ten
viewports; nothing playing, playing, paused) with the recordings of
tests/fixtures/demo_names.json. Like the other lists, the layout runs with
hidden scrollbars as Chrome was measured; in the game the listbox shows
CEF's scrollbar.
"""
import json
import pathlib
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close, font_of

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_demos_layout.json').read_text())
DEMOS = FIXTURE['demos']
KEYS = sorted(key for key in FIXTURE if key != 'demos')
STATES = {'start': (0, '0:00', '', 'Pause', 'Select a recording to preview'), 'playing': (2, '1:15', 'Playing', 'Pause', ''),
          'paused': (11, '62:05', 'Paused', 'Resume', '')}


def test_native_demo_archive_matches_the_webui_at_every_size():
    commands = []
    for key in KEYS:
        width, height = key.split('_')[0].split('x')
        pick, clock, message, pause, placeholder = STATES[key.split('_')[1]]
        name, size = DEMOS[pick]
        commands.append(f'demos {width} {height} {len(DEMOS)} {FIXTURE[key]["els"]["scroll"][0]}')
        commands.append('|'.join([f'{name} / {size / 1048576:.1f} MB', placeholder, clock, message, pause]))
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

        for name in ('back', 'left', 'right', 'list', 'play', 'viewport'):
            check(name, mine[name], chrome[name])
        check('h1', [mine['h1']], [chrome['h1'][1]])
        check('recordings', [mine['recordings']], [chrome['recordings'][1]])
        check('choose', [mine['choose']], [chrome['choose'][1] - 2])
        check('details', mine['details'], chrome['details'][1::2])
        check('preview', [mine['preview']], [chrome['preview'][1]])
        check('clock', [mine['clock']], [chrome['clockLine'][1] - 2])
        if STATES[key.split('_')[1]][2]:
            check('message', [mine['message']], [chrome['message'][0]])
        check('page', [mine['page']], [FIXTURE[key]['height']])
        option = chrome['options'][0]
        check('option', mine['option'][:2], [option[3], option[2]])
        for index, button in enumerate(chrome['transportButtons']):
            check(f'transport {index}', mine['transport'][index], button[:4])
        if not chrome.get('placeholder') is None:
            text = [t for t in FIXTURE[key]['texts'] if t['text'] == 'Select a recording to preview'][0]['lines'][0]
            check('placeholder', mine['placeholder'], [text[0], text[1] - 2])
        assert not problems, (key, problems)


def test_demo_texts_measure_like_chrome():
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
        # the .9rem details sum their rounding over long names
        assert abs(float(width) - runs[key]) < (0.2 if key[1] == 14.4 else 0.1), (key, width, runs[key])
