"""The native driver page has to look like the WebUI's.

tests/fixtures/webui_driver_layout.json holds what Chrome reports for the
WebUI's driver page and its dialog (tools/ui_webstyle/measure_webui_driver.cjs:
ten viewports, seven states) with the game's own data,
tests/fixtures/driver_model.json (ui_driverDump 1).
"""
import json
import pathlib
import re
import tempfile
from decimal import Decimal, ROUND_HALF_UP

from test_native_gfxloading import ROOT, compile_harness, run, close

FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_driver_layout.json').read_text())
MODELS = json.loads((ROOT / 'tests/fixtures/driver_model.json').read_text())
KEYS = sorted(FIXTURE)
# model, tab, filter, dialog
STATES = {'overview': ('driver', 0, None, 0), 'stats': ('driver', 1, None, 0), 'achievements': ('driver', 2, None, 0),
          'filtered': ('driver', 2, 2, 0), 'empty': ('empty', 0, None, 0), 'edit': ('driver', 0, None, 1),
          'switch': ('driver', 0, None, 2)}
RANKS = re.findall(r'ENTRY\(\s*"([^"]+)"', (ROOT / 'engine/code/game/profile_shared.h').read_text())
COUNTRIES = {'DE': 'Germany'}
# the statistics in the order of the WebUI's driver.js (its groups), frozen
# with the fixture when the WebUI was removed
STAT_KEYS = [
    'gamesPlayed', 'wins', 'losses', 'playerScore', 'distanceKm', 'fuelUsed', 'topSpeedKph', 'bestLapMs',
    'kills', 'deaths', 'damageDealt', 'damageTaken', 'flagCaptures', 'flagAssists', 'accuracyAwards',
    'excellentAwards', 'impressiveAwards', 'perfectAwards', 'mostUsedVehicle', 'mostUsedVehicleTimeMs',
    'racingWins', 'racingPodiums', 'racingCompleted', 'racingTotalMs', 'racingDmWins', 'racingDmPodiums',
    'racingDmCompleted', 'racingDmTotalMs', 'sprintWins', 'sprintCompleted', 'sprintBestMs',
    'eliminationWins', 'eliminationCompleted', 'eliminationTotalRoundsLasted', 'lcsWins', 'lcsCompleted',
    'lcsTotalSurvivalMs', 'derbyWins', 'derbyCompleted', 'derbyKills', 'dmWins', 'dmCompleted', 'dmKills',
    'ctfWins', 'ctfCompleted', 'ctfCaptures', 'ctf4Wins', 'ctf4Completed', 'ctf4Captures', 'teamWins',
    'teamCompleted', 'teamKills', 'teamRacingWins', 'teamRacingCompleted', 'teamRacingPodiums',
    'teamRacingDmWins', 'teamRacingDmCompleted', 'teamRacingDmPodiums', 'dominationWins',
    'dominationCompleted', 'dominationZoneHoldMs', 'kothWins', 'kothCompleted', 'kothZoneHoldMs',
    'autoballWins', 'autoballCompleted', 'autoballGoals']


def fmt(value):
    """toLocaleString('en-US', {maximumFractionDigits: 1})"""
    value = Decimal(str(float(value or 0))).quantize(Decimal('0.1'), rounding=ROUND_HALF_UP)
    text = f'{value:,.1f}'
    return text[:-2] if text.endswith('.0') else text


def stat(stats, key):
    value = stats.get(key, 0)
    if key == 'mostUsedVehicle':
        return value or 'None yet'
    if key.endswith('Ms'):
        ms = max(0, int(value or 0))
        return f'{ms // 60000}:{ms // 1000 % 60:02d}.{ms % 1000:03d}' if ms else '\x97'
    return fmt(value) + {'distanceKm': ' km', 'fuelUsed': ' l', 'topSpeedKph': ' km/h'}.get(key, '')


def commands(key):
    width, height = key.split('_')[0].split('x')
    model, tab, chosen, dialog = STATES[key.split('_')[1]]
    model = MODELS[model]
    profile = model['profile']
    stats = profile['progress']['stats'] if profile else {}
    info = profile['identity']['info'] if profile else {}
    rank = model['rank']
    name = profile['identity']['name'] if profile else 'Choose your driver'
    ranked = (RANKS[rank[0]] if rank[0] < len(RANKS) else 'Driver') if profile else 'Create or select a profile to begin.'
    country = 'Country: ' + COUNTRIES.get(info['country'], info['country']) if info.get('country') else ''
    score = stats.get('playerScore', 0)
    progress = ('No active profile' if not profile else fmt(max(0, rank[2] - score)) + ' points to the next rank'
                if rank[2] > rank[1] else 'Highest rank reached')
    medals, earned, total = [], 0, 0
    for index, (title, value, tiers) in enumerate(model['categories']):
        for tier_name, text, icon, threshold, won in tiers:
            total += 1
            earned += won == '1'
            if chosen is not None and chosen != index:
                continue
            status = 'Earned' if won == '1' else f'Locked \xb7 {fmt(value)} / {fmt(threshold)}'
            medals.append(f'{won}~{title}~{tier_name}~{text}~{status}')
    profiles = '|'.join(n + (' \xb7 Active' if profile and n == profile['identity']['name'] else '') for n in sorted(model['available']))
    return [f'driver {width} {height} {tab} {int(bool(profile))} {int(bool(info.get("avatar")))} {dialog} 12',
            '|'.join([name, ranked, country, progress, 'All achievements', f'{earned} / {total} achievements earned',
                      'Profile: ' + (profile['identity']['name'] if profile else ''), '']),
            '|'.join([fmt(stats.get('playerScore')), fmt(stats.get('gamesPlayed')), fmt(stats.get('wins')), fmt(stats.get('distanceKm')) + ' km']),
            '|'.join(stat(stats, k) for k in STAT_KEYS), '|'.join(medals), profiles]


def get(els, pattern):
    found = [box for path, box in els.items() if re.search(pattern, path)]
    return found


def test_native_driver_page_matches_the_webui_at_every_size():
    lines = []
    for key in KEYS:
        lines += commands(key)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        output = [json.loads(line) for line in run(exe, lines)]
    assert len(KEYS) == 70
    for key, mine in zip(KEYS, output):
        els = FIXTURE[key]['els']
        state = key.split('_')[1]
        problems = []

        def check(what, value, reference):
            if not close(value, reference):
                problems.append((what, value, reference))

        check('back', mine['back'], get(els, r'^/header:0/button\.secondary:1$')[0])
        for index in range(3):
            check(f'tab {index}', mine['tabs'][index], get(els, rf'button#driver-tab-{index}:{index}$')[0])
        if state not in ('edit', 'switch'):
            check('page', [mine['page']], [FIXTURE[key]['height']])
        if STATES[state][1] == 0:
            check('identity', mine['identity'], get(els, r'section\.driver-identity:0$')[0])
            check('career', mine['career'], get(els, r'driver-overview:2/section:1$')[0])
            image = get(els, r'driver-identity:0/img:0$')
            if image:
                check('avatar', mine['avatar'], image[0])
            div = get(els, r'driver-identity:0/div:\d$')[0]
            check('identity text', [mine['identityX'], mine['y']['identity']], div[:2])
            for name, pattern in (('label', r'p\.label:0'), ('name', r'h2:1'), ('rank', r'p:2'), ('country', r'p:3')):
                found = get(els, r'driver-identity:0/div:\d/' + pattern + '$')
                if found:
                    check(name, [mine['y'][name]], [found[0][1]])
            check('career h2', [mine['y']['careerH2']], [get(els, r'driver-overview:2/section:1/h2:0$')[0][1]])
            for index, box in enumerate(get(els, r'driver-overview:2/section:1/div\.driver-metrics:1/div:\d$')):
                check(f'card {index}', mine['overview'][index], box)
            check('progress', [mine['y']['progress']], [get(els, r'driver-overview:2/section:1/progress:2$')[0][1]])
            check('progress text', [mine['y']['progressText']], [get(els, r'driver-overview:2/section:1/p:3$')[0][1]])
            for index, box in enumerate(get(els, r'driver-actions:4/button\.secondary:\d$')):
                check(f'action {index}', mine['actions'][index], box)
        elif STATES[state][1] == 1:
            sections = get(els, r'driver-stats:3/section:\d+$')
            cards = get(els, r'driver-stats:3/section:\d+/div\.driver-metrics:1/div:\d+$')
            assert len(sections) == 16 and len(cards) == 67
            for index, box in enumerate(sections):
                check(f'group {index}', mine['groups'][index][:4], box)
            for index, box in enumerate(cards):
                check(f'stat {index}', mine['stats'][index], box)
        else:
            check('category', [mine['y']['category']], [get(els, r'driver-achievements:4/label:0$')[0][1] - 2])
            check('filter', mine['filter'], get(els, r'select#driver-achievement-filter:1$')[0])
            check('count', [mine['y']['count']], [get(els, r'p#driver-achievement-count:2$')[0][1]])
            cards = get(els, r'driver-medals:3/article[^/]*$')
            assert len(cards) == len(mine['medals']), key
            for index, path in enumerate(p for p in els if re.search(r'driver-medals:3/article[^/]*$', p)):
                medal = mine['medals'][index]
                check(f'medal {index}', medal[:4], els[path])
                check(f'medal {index} image', [medal[4]], [els[path + '/img:0'][0]])
                check(f'medal {index} lines', medal[5:10], [els[path + '/p.label:1'][1], els[path + '/h3:2'][1], els[path + '/p:3'][1],
                                                             els[path + '/progress:4'][1], els[path + '/p:5'][1]])
        if state in ('edit', 'switch'):
            check('dialog', mine['dialog'], els['dialog'])
            check('dialog lines', [mine['y']['dialogLabel'], mine['y']['dialogH2']], [els['dialog/p.label:0'][1], els['dialog/h2:1'][1]])
            check('close', mine['close'], get(els, r'^dialog/button\.secondary:\d$')[0])
            check('status', [mine['y']['status']], [get(els, r'^dialog/p\.driver-dialog-status:\d$')[0][1]])
            if state == 'edit':
                check('profile line', [mine['y']['profileLine']], [els['dialog/form.driver-form:2/p:0'][1]])
                for index in range(3):
                    label = els[f'dialog/form.driver-form:2/label:{index + 1}']
                    control = get(els, rf'^dialog/form\.driver-form:2/label:{index + 1}/(select|input):0$')[0]
                    check(f'field {index}', mine['fields'][index], control + [label[1]])
                check('avatar label', [mine['y']['avatarLabel']], [els['dialog/form.driver-form:2/p.label:4'][1]])
                for index, box in enumerate(get(els, r'driver-avatar-choices:5/button\.secondary:\d+$')):
                    check(f'avatar {index}', mine['avatars'][index], box)
                check('submit', mine['submit'], els['dialog/form.driver-form:2/button:6'])
            else:
                check('list', mine['list'], els['dialog/div.driver-profile-list:2'])
                for index, box in enumerate(get(els, r'driver-profile-list:2/button\.secondary:\d+$')):
                    check(f'profile {index}', mine['profiles'][index], box)
                check('create', [mine['y']['create'], mine['y']['nameLabel'], mine['y']['help']],
                      [els['dialog/form.driver-form:3/h3:0'][1], els['dialog/form.driver-form:3/label:1'][1], els['dialog/form.driver-form:3/p:2'][1]])
                check('name input', mine['nameInput'], els['dialog/form.driver-form:3/label:1/input:0'])
                check('submit', mine['submit'], els['dialog/form.driver-form:3/button:3'])
        assert not problems, (key, problems)
