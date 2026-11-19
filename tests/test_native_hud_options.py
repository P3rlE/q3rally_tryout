"""The native HUD options panel has to look like the WebUI's.

tests/fixtures/webui_hud_layout.json holds what Chrome reports for the
WebUI's #hud-screen (tools/ui_webstyle/measure_webui_hud.cjs: ten viewports)
with the toggles of tests/fixtures/hud_model.json.
"""
import json
import pathlib
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_hud_layout.json').read_text())
MODEL = json.loads((ROOT / 'tests/fixtures/hud_model.json').read_text())


def test_native_hud_options_match_the_webui_at_every_size():
    keys = sorted(FIXTURE)
    lines = []
    for key in keys:
        width, height = key.split('x')
        lines += [f'hud {width} {height} 0', '|'.join(item[1] for item in MODEL['items'])]
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        output = [json.loads(line) for line in run(exe, lines)]
    assert len(keys) == 10
    failures = []
    for key, mine in zip(keys, output):
        els, problems = FIXTURE[key]['els'], []

        def check(what, value, reference):
            if not close(value, reference):
                problems.append((what, value, reference))

        check('panel', mine['panel'], els['panel'])
        check('content', [mine['content']], [FIXTURE[key]['scrollHeight']])
        check('label', [mine['y'][0]], [els['/header:0/div:0/p.label:0'][1]])
        h2 = els['/header:0/div:0/h2:1']
        check('h2', [mine['y'][1], 2 * mine['h2']], [h2[1], h2[3]])
        check('em', [mine['x'][2]], [els['/header:0/div:0/h2:1/em:1'][0]])
        p = els['/p:1']
        check('text', [p[0], mine['y'][2], mine['x'][1], mine['text'] * 24], p)
        check('close', mine['close'], els['/div.session-actions:2/button#hud-close:0'])
        check('console', mine['console'], els['/div.session-actions:2/button#hud-console.secondary:1'])
        for index, item in enumerate(mine['items']):
            label = els[f'/div#hud-fields:3/label:{index}']
            check(f'item {index}', [item[0], item[1] * 24 + 6.390625 + item[5]], [label[1], label[3]])
            check(f'select {index}', item[2:], els[f'/div#hud-fields:3/label:{index}/select#hud-item-{index}:0'])
        check('h3', [mine['y'][3]], [els['/h3:4'][1]])
        check('size', [mine['y'][4], mine['y'][5]], [els['/label:5'][1], els['/label:5/output#hud-size-value:0'][1]])
        check('size range', mine['size'], els['/label:5/input#hud-size:1'])
        check('zoom', [mine['y'][6], mine['y'][7]], [els['/label:6'][1], els['/label:6/output#hud-zoom-value:0'][1]])
        check('zoom range', mine['zoom'], els['/label:6/input#hud-zoom:1'])
        check('hint', [mine['y'][8]], [els['/p.label:7'][1]])
        if problems:
            failures.append((key, problems[:6]))
    assert not failures, (len(failures), failures[:8])
