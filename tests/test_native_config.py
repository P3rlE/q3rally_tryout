"""The native settings have to look like the WebUI's.

tests/fixtures/webui_config_layout.json holds what Chrome reports for the
WebUI's settings (tools/ui_webstyle/measure_webui_config.cjs: ten viewports,
every page of tests/fixtures/config_model.json and the key capture dialog)
with the game's own pages (ui_configDump 1).
"""
import json
import pathlib
import re
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_config_layout.json').read_text())
MODELS = json.loads((ROOT / 'tests/fixtures/config_model.json').read_text())
KEYS = sorted(FIXTURE)


def clean(text):
    return re.sub(r'\^[0-9]', '', str(text or ''))


def commands(key):
    width, height = key.split('_')[0].split('x')
    state = key.split('_', 1)[1]
    capture = state == 'capture'
    model = MODELS['controls' if capture else state]
    page, confirm = model['page'], model['confirmation']
    buttons = [clean(i[2]) for i in model['items'] if i[1] == 6 and (confirm or i[2].strip().lower() != 'back' and
                                                                     not (page == 10 and re.match('resume', i[2], re.I)))]
    fields = []
    for index, kind, label, text, value, low, high, disabled, choices in model['items']:
        if kind == 6:
            continue
        shown = clean(text) if kind == 5 else '\x1e'.join(clean(c[0]) for c in choices) if kind == 7 else ''
        fields.append(f'{kind}~{re.sub(r":\s*$", "", clean(label))}~{shown}')
    action = next((clean(i[2]) for i in model['items'] if i[1] == 5), '') if capture else ''
    dialog = 1 if confirm else 2 if capture else 0
    return [f'config {width} {height} {page} {dialog} 0', clean(model['title']), clean(model['description']).replace('\n', '\x1e'),
            action, '|'.join(buttons), '|'.join(fields)]


def test_native_settings_match_the_webui_at_every_size():
    lines = []
    for key in KEYS:
        lines += commands(key)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        output = [json.loads(line) for line in run(exe, lines)]
    assert len(KEYS) == 150
    failures = []
    for key, mine in zip(KEYS, output):
        els = FIXTURE[key]['els']
        problems = []

        def check(what, value, reference):
            if reference is not None and not close(value, reference):
                problems.append((what, value, reference))

        check('back', mine['back'], els['/header:0/button#config-back.secondary:1'])
        check('page', [mine['page']], [FIXTURE[key]['height']])
        description = els['/p#config-description:1']
        check('description', [mine['description'][0], mine['description'][1] * 24], description[1::2])
        nav = els.get('/nav#config-actions.config-actions:2')
        dialog = els.get('dialog')
        if not dialog or 'capture' in key:
            check('nav', mine['nav'], nav)
            for index, box in enumerate(b for p, b in els.items() if re.match(r'/nav[^/]*/button', p)):
                check(f'button {index}', mine['buttons'][index], box)
        section = els.get('/section#config-settings:3')
        assert bool(section) == bool(mine['settings']), key
        if section:
            check('section', mine['section'], section)
            fields = [p for p in els if re.search(r'config-fields:0/div\.config-field:\d+$', p)]
            assert len(fields) == len(mine['fields']), key
            for index, path in enumerate(fields):
                box, control, output, rebind, clear, labels = mine['fields'][index]
                check(f'field {index}', box, els[path])
                check(f'label {index}', [labels * 24], [els[path + '/label:0'][3]])
                inner = [(p, b) for p, b in els.items() if p.startswith(path + '/') and p.count('/') == path.count('/') + 1 and '/label:0' not in p]
                for p, b in inner:
                    if '/output:' in p:
                        check(f'output {index}', [output], [b[1]])
                    elif 'div.config-actions' in p:
                        check(f'actions {index}', [control[1], control[3]], [b[1], b[3]])
                        check(f'rebind {index}', rebind, els[p + '/button.secondary:0'])
                        check(f'clear {index}', clear, els[p + '/button.secondary:1'])
                    else:
                        check(f'control {index}', control, b)
        if dialog:
            check('dialog', mine['dialog'], dialog)
            check('dialog label', [mine['dialogY'][0]], [els['dialog/p.label:0'][1]])
            title = [b for p, b in els.items() if p.startswith('dialog/h2')][0]
            check('dialog title', [mine['dialogY'][1], mine['h2'][0] * mine['h2'][1]], [title[1], title[3]])
            if 'capture' in key:
                check('capture action', [mine['dialogY'][2]], [els['dialog/p#config-capture-action:2'][1]])
                check('capture help', [mine['dialogY'][3]], [els['dialog/p:3'][1]])
                check('cancel', mine['cancel'], els['dialog/div.config-actions:4/button#config-capture-cancel.secondary:0'])
            else:
                for index, box in enumerate(b for p, b in els.items() if re.match(r'dialog/div#config-confirm-actions[^/]*/button', p)):
                    check(f'confirm {index}', mine['buttons'][index], box)
        if problems:
            failures.append((key, problems[:6]))
    assert not failures, (len(failures), failures[:8])
