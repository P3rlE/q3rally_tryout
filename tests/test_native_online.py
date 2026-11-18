"""The native server browser has to look like the WebUI's.

tests/fixtures/webui_online_layout.json holds what Chrome reports for the
WebUI's online screen (tools/ui_webstyle/measure_webui_online.cjs: ten
viewports, eight states) with the game's own data,
tests/fixtures/online_model.json (ui_onlineDump 1).
"""
import json
import pathlib
import re
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close, font_of

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_online_layout.json').read_text())
MODELS = json.loads((ROOT / 'tests/fixtures/online_model.json').read_text())
KEYS = sorted(FIXTURE)
# model, selected, query, list scroll, scanning, dialog
STATES = {'start': ('favorites', '', '', 0, 0, 0), 'picked': ('favorites', '127.0.0.1:27962', '', 0, 0, 0),
          'scrolled': ('internet', '[2001:db8::1]:27960', '', 300, 0, 0), 'search': ('favorites', '', 'ridge', 0, 0, 0),
          'nomatch': ('favorites', '', 'zzz', 0, 0, 0), 'scanning': ('favorites', '', '', 0, 1, 0),
          'direct': ('favorites', '', '', 0, 0, 1), 'remove': ('favorites', '127.0.0.1:27961', '', 0, 0, 2)}


def clean(text):
    return re.sub(r'\^[0-9]', '', str(text or '') if text != 0 else '')


def commands(key):
    width, height = key.split('_')[0].split('x')
    model, selected, query, scroll, scanning, dialog = STATES[key.split('_')[1]]
    model = MODELS[model]
    servers = [] if scanning else [dict(zip(('name', 'address', 'map', 'mode', 'clients', 'max', 'ping', 'humans', 'favorite'), s))
                                   for s in model['servers']]
    visible = [s for s in servers if any(query in clean(s[k]).lower() for k in ('name', 'map', 'address', 'mode'))]
    server = next((s for s in servers if s['address'] == selected), None)
    status = 'Searching for servers...' if scanning else model['status']
    empty = ('Looking for servers...' if scanning else 'No servers match your search.' if servers
             else 'No servers found. Try another source, refresh, or use Direct connect.')
    details = ([server['mode'], server['map'] or 'Unknown', f"{server['clients']} / {server['max']}", clean(server['humans']),
                f"{server['ping']} ms" if server['ping'] > 0 else 'No response'] if server else [''] * 5)
    image = any('img#online-image' in path for path in FIXTURE[key]['els'])
    text = {1: 'Enter a valid server address and port (1-65535).', 2: selected}.get(dialog, '')
    head = '|'.join([status, f'{len(visible)} / {len(servers)} servers', empty, '' if dialog == 1 else model['message'],
                     clean(server['name']) if server else 'Choose a server',
                     server['address'] if server else 'Select a server from the list or connect directly.',
                     'Saved favorite' if server and server['favorite'] else 'Save favorite', text])
    rows = '|'.join(clean(s['name']) + '~' + (s['map'] or 'Unknown map') + ' / ' + s['mode'] + (' / Favorite' if s['favorite'] else '')
                    for s in visible)
    return [f'online {width} {height} {scroll} {int(bool(server))} {int(image)} {int(model["filters"][0] == 7)} {dialog}',
            head, '|'.join(details), rows]


def find(els, token):
    return [box for path, box in els.items() if path.split('/')[-1].startswith(token) or ('/' + token) in path[-len(token) - 4:]]


def element(els, token):
    found = [box for path, box in els.items() if token in path.split('/')[-1]]
    return found[0] if found else None


def test_native_online_screen_matches_the_webui_at_every_size():
    lines = []
    for key in KEYS:
        lines += commands(key)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        output = [json.loads(line) for line in run(exe, lines)]
    assert len(KEYS) == 80
    for key, mine in zip(KEYS, output):
        els = FIXTURE[key]['els']
        problems = []

        def check(what, value, reference):
            if reference is not None and not close(value, reference):
                problems.append((what, value, reference))

        check('back', mine['back'], element(els, 'button#online-back'))
        check('toolbar', mine['toolbar'], element(els, 'section.online-toolbar'))
        sections = [box for path, box in els.items() if re.search(r'/div\.online-layout:2/section[^/]*$', path)]
        check('list section', mine['listSection'], sections[0])
        check('detail section', mine['detailSection'], sections[1])
        check('list', mine['list'], element(els, 'div#online-list'))
        check('page', [mine['page']], [FIXTURE[key]['height']])
        for index, name in enumerate(('source', 'mode', 'sort', 'search')):
            control = element(els, f'#online-{name}:')
            label = [box for path, box in els.items() if path.endswith(f'#online-{name}:0')][0]
            labels = [box for path, box in els.items() if re.search(r'online-filters:0/label:\d+$', path)]
            check(f'filter {name}', mine['filters'][index], labels[index] + control)
        toggles = [box for path, box in els.items() if re.search(r'online-toggles:1/label:\d+$', path)]
        for index, box in enumerate(toggles):
            check(f'toggle {index}', mine['toggles'][index], box)
        for index, name in enumerate(('refresh', 'stop', 'direct', 'host')):
            check(name, mine['actions'][index], element(els, f'button#online-{name}'))
        status = element(els, 'p#online-status')
        check('status', [mine['status'][0], mine['status'][1] * 24], [status[1], status[3]])
        heading = [box for path, box in els.items() if re.search(r'online-list-heading:0/span:\d+$', path)]
        check('heading', mine['heading'], [heading[0][1], heading[1][0], heading[2][0]])
        check('count', [mine['countY']], [element(els, 'p#online-count')[1]])
        rows = [box for path, box in els.items() if re.search(r'div#online-list:2/button[^/]*$', path)]
        assert len(rows) == len(mine['rows']), key
        for index, box in enumerate(rows):
            check(f'row {index}', mine['rows'][index], box)
        empty = element(els, 'p.online-empty-state')
        if empty:
            check('empty', [mine['emptyY']], [empty[1]])
        scroll = FIXTURE[key]['listScroll']
        if abs(max(mine['listHeight'], scroll[3]) - scroll[1]) > 1:
            problems.append(('list content', mine['listHeight'], scroll[1]))
        detail = [path for path in els if re.search(r'online-detail:1$', path)][0]
        check('label', [mine['label']], [els[detail + '/p.label:0'][1]])
        name = element(els, 'h2#online-name')
        check('name', [mine['name'][0], mine['name'][1] * name[3] / max(mine['name'][1], 1)], [name[1], name[3]])
        check('address', [mine['address'][0]], [element(els, 'p#online-address')[1]])
        image = element(els, 'img#online-image')
        if image:
            check('image', mine['image'], image)
        details = element(els, 'div#online-details')
        check('details', mine['details'], [details[1], details[3]])
        cards = [box for path, box in els.items() if re.search(r'div#online-details:4/div:\d+$', path)]
        for index, box in enumerate(cards):
            check(f'card {index}', mine['cards'][index][:4], box)
        for index, name in enumerate(('connect', 'favorite', 'remove')):
            box = element(els, f'button#online-{name}')
            if box:
                check(name, mine['buttons'][index], box)
        check('message', [mine['message']], [element(els, 'p#online-message')[1]])
        dialog = element(els, 'dialog#online-direct-dialog') or element(els, 'dialog#online-remove-dialog')
        if dialog:
            check('dialog', mine['dialog'], dialog)
            if 'direct' in key:
                check('dialog input', mine['dialogInput'], element(els, 'input#online-direct-address'))
                for index, name in enumerate(('direct-connect', 'direct-close')):
                    check(name, mine['dialogButtons'][index], element(els, f'button#online-{name}'))
                check('dialog lines', [mine['dialogY'][1], mine['dialogY'][3], mine['dialogY'][4]],
                      [element(els, 'h2#online-direct-title')[1], [b for p, b in els.items() if p.endswith('form:2/p:1')][0][1],
                       element(els, 'p#online-direct-error')[1]])
            else:
                for index, name in enumerate(('remove-confirm', 'remove-cancel')):
                    check(name, mine['dialogButtons'][index], element(els, f'button#online-{name}'))
                check('dialog lines', [mine['dialogY'][1], mine['dialogY'][5]],
                      [element(els, 'h2#online-remove-title')[1], element(els, 'p#online-remove-address')[1]])
        assert not problems, (key, problems)


def test_online_texts_measure_like_chrome():
    runs = {}
    for key in KEYS:
        for text in FIXTURE[key]['texts']:
            if len(text['lines']) == 1 and text['size'] in (16, 12.8, 14.4):
                runs[(font_of(text), text['size'], text['spacing'], text['uppercase'], text['text'].strip())] = text['lines'][0][2]
    keys = sorted(runs)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        widths = run(exe, [f'width {f} {s} {sp} {int(u)} {t}' for f, s, sp, u, t in keys])
    for key, width in zip(keys, widths):
        assert abs(float(width) - runs[key]) < 0.2, (key, width, runs[key])
