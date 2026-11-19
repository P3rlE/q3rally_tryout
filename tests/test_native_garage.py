"""The native garage has to look like the WebUI's.

tests/fixtures/webui_garage_layout.json holds what Chrome reports for the
WebUI's #garage (tools/ui_webstyle/measure_webui_garage.cjs: ten viewports,
three setups) with the catalog engine/webui/build_showroom.py makes from the
game files.
"""
import json
import pathlib
import re
import tempfile

from test_native_gfxloading import ROOT, compile_harness, run, close

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_garage_layout.json').read_text())
KEYS = sorted(FIXTURE)
SECTION = '/div.layout:1/section:1'


def picker_paths(els, index):
    """The carousel of picker <index> (vehicle, paint, wheels, driver, plate)."""
    if index == 0:
        base = SECTION + '/div.setup-controls:2/div.garage-picker:0'
    else:
        base = SECTION + f'/div#setup-controls.setup-controls:3/div.garage-picker:{index - 1}'
    return base


def commands(key):
    width, height = key.split('_')[0].split('x')
    state = FIXTURE[key]
    els, texts = state['els'], state['texts']
    pickers = []
    for index in range(5):
        base = picker_paths(els, index) + '/div.garage-carousel:2'
        count = next(t for p, t in texts.items() if p.startswith(base + '/div.carousel-current') and 'carousel-count' in p)
        current, total = (int(v) for v in count.split(' / '))
        cards = [p for p in els if p.startswith(base + '/') and p.count('/') == base.count('/') + 1 and 'carousel-arrow' not in p]
        locked = ['is-locked' in p for p in cards]
        if len(locked) == 1:
            locked = [False, locked[0], False]
        pickers.append(f'{total} {current - 1} {" ".join(str(int(x)) for x in locked)}')
    slots = []
    for slot in range(4):
        base = f'/section.favorite-panel:2/div#favorite-slots:3/div:{slot}'
        image = any(p.startswith(base + '/img') for p in els)
        load = next(t for p, t in texts.items() if p.startswith(base + '/button.secondary') and ('/button.secondary:1' in p or '/button.secondary:2' in p))
        slots.append(f'{int(load != "Empty")} {int(image)}|{load}')
    return [f'garage {width} {height} 0', texts['/div.layout:1/section.showroom:0/h2#vehicle-name:1'],
            texts.get(SECTION + '/p#status:7', ''), '', ' '.join(pickers)] + slots


def test_native_garage_matches_the_webui_at_every_size():
    lines = []
    for key in KEYS:
        lines += commands(key)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        output = [json.loads(line) for line in run(exe, lines)]
    assert len(KEYS) == 30
    failures = []
    for key, mine in zip(KEYS, output):
        els, problems = FIXTURE[key]['els'], []

        def check(what, value, reference):
            if not close(value, reference):
                problems.append((what, value, reference))

        check('page', [mine['page']], [FIXTURE[key]['height']])
        check('back', mine['back'], els['/header:0/button#back.secondary:1'])
        check('showroom', mine['showroom'], els['/div.layout:1/section.showroom:0'])
        check('config', mine['config'], els[SECTION])
        check('vehicle', [mine['y'][0], mine['y'][1]], [els['/div.layout:1/section.showroom:0/p.label:0'][1], els['/div.layout:1/section.showroom:0/h2#vehicle-name:1'][1]])
        check('image', mine['image'], els['/div.layout:1/section.showroom:0/img#vehicle-image:2'])
        check('toggle', mine['toggle'], els['/div.layout:1/section.showroom:0/div.showroom-actions:4/button#view-toggle.secondary:0'])
        check('config text', [mine['y'][2], mine['y'][3]], [els[SECTION + '/p.label:0'][1], els[SECTION + '/h2#configuration:1'][1]])
        check('apply', mine['apply'], els[SECTION + '/button#apply:4'])
        check('link', mine['link'], els[SECTION + '/div.showroom-actions:5/button#web-link.secondary:0'])
        check('pull', mine['pull'], els[SECTION + '/div.showroom-actions:5/button#web-pull.secondary:1'])
        status = els[SECTION + '/p#status:7']
        check('status', [mine['y'][8], mine['lines'][1] * 24], [status[1], status[3]])
        for index, picker in enumerate(mine['pickers']):
            base = picker_paths(els, index)
            label = next(b for p, b in els.items() if p.startswith(base + '/span'))
            check(f'picker {index} label', [picker['label']], [label[1]])
            carousel = base + '/div.garage-carousel:2'
            check(f'picker {index}', picker['box'], els[carousel])
            check(f'picker {index} previous', picker['prev'], els[carousel + '/button.carousel-arrow.secondary:0'])
            check(f'picker {index} next', picker['next'], els[carousel + '/button.carousel-arrow.secondary:4'])
            cards = [p for p in els if p.startswith(carousel + '/') and p.count('/') == carousel.count('/') + 1 and 'carousel-arrow' not in p]
            order = [0, 1, 2] if len(cards) == 3 else [1]
            for slot, path in zip(order, cards):
                card = picker['cards'][slot]
                check(f'picker {index} card {slot}', card[0], els[path])
                check(f'picker {index} card {slot} image', card[1], els[path + '/img:0'])
                name = next(b for p, b in els.items() if p.startswith(path + '/span.carousel-name'))
                check(f'picker {index} card {slot} name', [card[2], card[4], card[3]], [name[0], name[1], name[2]])
                for p, b in els.items():
                    if p.startswith(path + '/span.carousel-lock'):
                        check(f'picker {index} card {slot} lock', [card[5]], [b[1]])
                    if p.startswith(path + '/span.carousel-count'):
                        check(f'picker {index} card {slot} count', [card[6]], [b[1]])
        check('favorites', mine['favorites'], els['/section.favorite-panel:2'])
        check('favorites text', [mine['y'][4], mine['y'][5], mine['y'][6]],
              [els['/section.favorite-panel:2/p.label:0'][1], els['/section.favorite-panel:2/h2#favorites-heading:1'][1], els['/section.favorite-panel:2/p:2'][1]])
        for index, slot in enumerate(mine['slots']):
            base = f'/section.favorite-panel:2/div#favorite-slots:3/div:{index}'
            check(f'slot {index}', slot['box'], els[base])
            check(f'slot {index} label', [slot['label']], [els[base + '/p:0'][1]])
            buttons = [b for p, b in els.items() if p.startswith(base + '/button')]
            if base + '/img:1' in els:
                check(f'slot {index} picture', slot['picture'], els[base + '/img:1'])
            check(f'slot {index} buttons', slot['load'] + slot['save'] + slot['upload'], buttons[0] + buttons[1] + buttons[2])
        if problems:
            failures.append((key, problems[:6]))
    assert not failures, (len(failures), failures[:6])


def test_garage_labels_follow_the_webui_catalog():
    """ui_rally_garage_labels.h holds catalog_labels.json's names that differ
    from the file name in title case (build_showroom.py's label())."""
    labels = json.loads((ROOT / 'engine/webui/catalog_labels.json').read_text())
    header = (ROOT / 'engine/code/q3_ui/ui_rally_garage_labels.h').read_text()
    title = lambda key: key.replace('_', ' ').title()
    paints = {(car, key, value['name']) for car, entry in labels['vehicles'].items()
              for key, value in entry.get('paints', {}).items() if value['name'] != title(key)}
    wheels = {(key, value['name'] if isinstance(value, dict) else value) for key, value in labels['wheels'].items()
              if (value['name'] if isinstance(value, dict) else value) != title(key)}
    assert set(re.findall(r'ENTRY\( "([^"]+)", "([^"]+)", "([^"]+)" \)', header)) == paints
    assert set(re.findall(r'ENTRY\( "([^"]+)", "([^"]+)" \)', header)) == wheels
