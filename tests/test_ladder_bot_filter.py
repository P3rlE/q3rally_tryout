"""Exercise the rendered Ladder filters against mixed human and bot matches."""

from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_bot_filter_rebuilds_rankings_and_preserves_elimination_results(tmp_path):
    site = tmp_path / "site"
    shutil.copytree(ROOT / "ladder_service/php_webservice", site,
                    ignore=shutil.ignore_patterns("data"))
    html = subprocess.check_output(["php", str(site / "index.php")], text=True, encoding="utf-8")
    scripts = "\n".join(re.findall(r"<script[^>]*>(.*?)</script>", html, re.S))
    scripts = scripts[:scripts.rindex("createModeTabs();")]
    harness = r"""
const assert = require('node:assert/strict');
const nodes = new Map();
const buttons = [];
function element() {
  return {
    style: {}, dataset: {}, children: [], listeners: {}, classList: { toggle() {} },
    set id(value) { nodes.set(value, this); },
    appendChild(child) { this.children.push(child); if ('source' in child.dataset || 'weather' in child.dataset) buttons.push(child); },
    append(...children) { children.forEach(child => this.appendChild(child)); },
    insertBefore(child) { this.appendChild(child); },
    addEventListener(type, handler) { this.listeners[type] = handler; }
  };
}
for (const id of ['stat-total', 'stat-last', 'stat-modes', 'stat-players', 'modeBreakdown', 'modeStatus']) nodes.set(id, element());
const panel = element();
const document = {
  documentElement: { dataset: {} },
  getElementById(id) { return nodes.get(id) || null; },
  querySelector(selector) { return selector === '.tabbed-panel' ? panel : null; },
  querySelectorAll(selector) { return selector.startsWith('#source-toggle button') ? buttons : []; },
  createElement: element
};
const localStorage = { getItem() { return null; }, setItem() {} };
"""
    harness += scripts
    harness += r"""
const bot = {name:'Bot Racer',isBot:true,bestLapMs:10000,kills:50,deaths:1,position:1,survivalMs:60000,captures:5,zoneHoldMs:90000,autoballGoals:3};
const human = {...bot,name:'Human Racer',playerId:'human',isBot:false,bestLapMs:30000,kills:10,deaths:2,position:2};
const modes = ['GT_RACING','GT_RACING_DM','GT_SPRINT','GT_TEAM_RACING','GT_TEAM_RACING_DM',
               'GT_DEATHMATCH','GT_TEAM','GT_DERBY','GT_LCS','GT_CTF','GT_CTF4','GT_DOMINATION','GT_KOTH','GT_AUTOBALL','GT_ELIMINATION'];
const records = agg => [...agg.raceBest.values(), ...agg.deathmatchBest.values(),
    ...agg.derbyBest.values(), ...agg.survivalBest.values(),
    ...agg.objectiveBest.values(), ...agg.eliminationBest.values()];
for (const mode of modes) {
  const match = {matchId:mode,mode,map:'q3r_country01',players:[bot,human]};
  const all = createEmptyAggregation(), humans = createEmptyAggregation();
  ingestMatchInto(match,all);
  ingestMatchInto(match,humans,true);
  assert.equal(records(all).length,2,mode);
  assert.equal(records(humans).length,1,mode);
  assert.equal(records(humans)[0].player,'Human Racer',mode);
  assert.equal(records(humans)[0].isBot,false,mode);
  assert.equal(humans.playerNames.size,1,mode);
  if (mode === 'GT_ELIMINATION') {
    assert.equal(records(humans)[0].value,records(all).find(row=>!row.isBot).value);
  }
}
const botOnly = {matchId:'bots',mode:'GT_RACING',map:'q3r_country01',players:[bot]};
const excluded = createEmptyAggregation();
ingestMatchInto(botOnly,excluded,true);
assert.equal(excluded.totalMatches,0);
const renamedBot = {...bot,name:human.name};
const mixedName = createEmptyAggregation();
ingestMatchInto({mode:'GT_RACING',map:'same-name',players:[renamedBot,human]},mixedName,true);
assert.equal([...mixedName.raceBest.values()][0].time,30);

const online = {matchId:'online',mode:'GT_RACING',map:'online',source:'online',weather:{condition:'wet'},players:[bot,human]};
const offline = {...online,matchId:'offline',map:'offline',source:'offline',weather:{condition:'dry'}};
state.allMatches = [online,offline,botOnly];
for (const match of state.allMatches) {
  ingestMatchInto(match,match.source === 'offline' ? state.aggregationOffline : state.aggregationOnline);
}
renderSourceToggle();
state.language = 'de';
updateSourceToggleUI();
assert.equal(nodes.get('hide-bots-label').textContent,'Bots ausblenden');
const checkbox = nodes.get('hide-bots');
checkbox.checked = true;
checkbox.listeners.change();
assert.equal(state.aggregation.totalMatches,1);
assert.equal(records(state.aggregation).length,1);
setActiveWeather('wet');
setActiveSource('all');
assert.equal(state.aggregation.totalMatches,1);
setActiveSource('offline');
assert.equal(state.aggregation.totalMatches,0);
setActiveWeather('dry');
assert.equal(state.aggregation.totalMatches,1);
checkbox.checked = false;
checkbox.listeners.change();
assert.equal(records(state.aggregation).length,2);
setActiveWeather('all');
setActiveSource('online');
assert.equal(state.aggregation,state.aggregationOnline);
assert.equal(records(state.aggregation).length,3);
"""
    probe = tmp_path / "bot-filter.cjs"
    probe.write_text(harness, encoding="utf-8")
    subprocess.run(["node", "--check", str(probe)], check=True, capture_output=True, text=True)
    result = subprocess.run(["node", str(probe)], capture_output=True, text=True, timeout=10)
    assert result.returncode == 0, result.stdout + result.stderr
