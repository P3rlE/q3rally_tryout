"""Step 2 of the native offline race setup has to look like the WebUI's.

tests/fixtures/webui_offline_rules_layout.json holds what Chrome reports for
the WebUI's rules and drivers step
(tools/ui_webstyle/measure_webui_offline_rules.cjs: ten viewports; Racing,
Racing with an avatar and the grid scrolled, Team Racing, Drag Race) with the
game's own data, tests/fixtures/offline_rules_model.json (ui_offlineDump 1).
"""
import json
import pathlib
import re
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close, font_of

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_offline_rules_layout.json').read_text())
MODELS = json.loads((ROOT / 'tests/fixtures/offline_rules_model.json').read_text())
KEYS = sorted(FIXTURE)
STATES = {'racing': ('racing', ''), 'avatar': ('racing', 'gfx/avatars/preset/helmet_01'), 'team': ('team', ''), 'drag': ('drag', '')}
DRAG_RULES = ('Equal vehicle performance. A match is the best of the chosen heats; the Ladder keeps your best heat. '
              'A tournament fills a knockout bracket with bots; their matches can be skipped. Selecting first before '
              'green, or less than 0.100 s after it, is a false start.')
DRAG_GRID = ('Race a human or a bot, or practise solo. Hold 4000-5000 RPM in N, select first on green, shift up '
             'before the limiter: too long in the red blows the engine.')


def clean(text):
    return re.sub(r'\^[0-9]', '', text or '')


def page(model, avatar):
    """What offline.js's options() builds: the fields and the rows"""
    drag = model['modes'][model['mode']] == 'Drag Race'
    controls = [dict(zip(('index', 'type', 'label', 'text', 'value', 'disabled', 'slot', 'team', 'maxchars', 'choices'), c))
                for c in model['controls']]
    fields = [('I' if c['type'] in (1, 4) else 'S') + ':' + re.sub(r':\s*$', '', c['label']).strip()
              for c in controls if c['slot'] < 0]
    rows = []
    for slot in range(12):
        mine = [c for c in controls if c['slot'] == slot]
        if (not mine and slot) or (not model['allowsBots'] and slot) or (drag and slot > 1):
            continue
        kind = next((c for c in mine if not c['team']), None)
        team = next((c for c in mine if c['team']), None)
        human = slot == 0 and not model['dedicated']
        text = ('Server slot' if model['dedicated'] else 'You / ' + clean(model['players'][0])) if slot == 0 else f'Slot {slot + 1}'
        kinds = 'P'
        if human:
            kinds += 'A' if avatar else ''
        elif kind:
            kinds += 'T'
        if slot > 0 and kind and kind['value'] == 1 and model['allowsBots']:
            kinds += 'CB'
        if team:
            kinds += 'M'
        rows.append(f'{int(human)}~{text}~{kinds}')
    rule = DRAG_RULES if drag else 'Zero means no limit.'
    grid = DRAG_GRID if drag else 'Choose your opponents and teams.' if model['allowsBots'] else 'This mode is driven solo. Bots are disabled.'
    title = model['maps'][model['selected']][1]
    return [f"{title}|{model['modes'][model['mode']]}", rule, grid, '|'.join(fields), ';'.join(rows)]


def test_native_offline_rules_step_matches_the_webui_at_every_size():
    commands = []
    for key in KEYS:
        width, height = key.split('_')[0].split('x')
        model, avatar = STATES[key.split('_')[1]]
        els = FIXTURE[key]['els']
        commands.append(f'rules {width} {height} {els["optionsScroll"][0]} {els["playersScroll"][0]} 0')
        commands += page(MODELS[model], avatar)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        output = [json.loads(line) for line in run(exe, commands)]
    assert len(KEYS) == 40
    for key, mine in zip(KEYS, output):
        chrome = FIXTURE[key]['els']
        problems = []

        def check(what, value, reference):
            if not close(value, reference):
                problems.append((what, value, reference))

        for what in ('back', 'summary', 'start', 'left', 'right', 'options', 'players'):
            check(what, mine[what], chrome[what])
        for what in ('label', 'h1', 'summaryLabel', 'title', 'raceMode', 'rulesH2', 'gridH2'):
            check(what, [mine[what]], [chrome[what][1]])
        check('steps', mine['step'], [chrome['stepMap'][1], chrome['stepRules'][0]])
        check('rule help', [mine['ruleHelp'][0], mine['ruleHelp'][1] * 24], chrome['ruleHelp'][1::2])
        check('grid help', [mine['gridHelp'][0], mine['gridHelp'][1] * 24], chrome['gridHelp'][1::2])
        check('page', [mine['page']], [FIXTURE[key]['height']])
        # scrollHeight: whole pixels, at least the scrollport
        for what, content, scroll in (('options', mine['heights'][0], chrome['optionsScroll']),
                                      ('players', mine['heights'][1], chrome['playersScroll'])):
            if abs(max(content, scroll[3]) - scroll[1]) > 1:
                problems.append((what + ' content', content, scroll[1]))
        for index, field in enumerate(chrome['fields']):
            check(f'field {index}', mine['fields'][index], field['label'] + field['control'])
        assert len(mine['rows']) == len(chrome['rows']), key
        for index, (row, reference) in enumerate(zip(mine['rows'], chrome['rows'])):
            check(f'row {index}', row['row'], reference['row'])
            kinds = {'p': [0], 'label': [2, 5], 'img': [1, 3], 'select': [4]}
            for kind, parts in kinds.items():
                theirs = sorted((part for part in reference['parts'] if part[0] == kind), key=lambda part: part[1][0])
                ours = sorted((part for part in row['parts'] if part[0] in parts), key=lambda part: part[1])
                assert len(theirs) == len(ours), (key, index, kind)
                for a, b in zip(ours, theirs):
                    check(f'row {index} {kind}', a[1:5], b[1])
                    if kind == 'label':
                        check(f'row {index} select', a[5:9], b[2])
        assert not problems, (key, problems)


def test_offline_rules_texts_measure_like_chrome():
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
        assert abs(float(width) - runs[key]) < 0.2, (key, width, runs[key])
