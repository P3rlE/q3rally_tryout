"""Run real PHP Drag ingestion/profile code and browser aggregation in isolation."""
from __future__ import annotations

import json
from pathlib import Path
import re
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[2]
PHP_SOURCE = ROOT / "ladder_service/php_webservice"
PLAYER_A = "0a1b2c3d-1111-4222-8333-0000000000a1"
PLAYER_B = "0a1b2c3d-1111-4222-8333-0000000000b2"


def player(slot=0, *, reaction=200, elapsed=13000, status="finished", position=1):
    return {
        "clientNum": slot, "playerId": PLAYER_A if slot == 0 else PLAYER_B,
        "name": "Alice" if slot == 0 else "Bob", "score": 0, "isBot": False,
        "position": position if status == "finished" else 0,
        "dragStatus": status, "reactionMs": reaction if status == "finished" else 0,
        "elapsedMs": elapsed if status == "finished" else 0,
        "totalRaceMs": reaction + elapsed if status == "finished" else 0,
        "trapSpeedKph": 151.2 if status == "finished" else 0,
    }


def match(match_id="drag-1", players=None, *, online=True):
    players = players if players is not None else [player(), player(1, reaction=100, elapsed=13200, position=2)]
    return {
        "matchId": match_id, "mode": "GT_DRAG", "gametype": 9, "map": "q3r_dragstrip",
        "server": {"name": "Drag Test", "dedicated": online},
        "source": "online" if online else "offline", "weather": {"condition": "dry"},
        "settings": {"g_gametype": 9, "dragDuel": len(players) == 2},
        "winnerClientNum": 0, "players": players,
    }


@pytest.fixture
def php_ladder(tmp_path):
    php = shutil.which("php")
    if not php:
        pytest.skip("PHP is not installed")
    root = tmp_path / "ladder"
    shutil.copytree(PHP_SOURCE, root, ignore=shutil.ignore_patterns("data"))
    probe = root / "probe.php"
    probe.write_text("""<?php
define('LADDER_LIBRARY_ONLY', true);
require __DIR__ . '/index.php';
$out=[];
foreach (json_decode(file_get_contents('php://stdin'), true) as $input) {
    try {
        $payload=normalize_ingest_payload($input);
        profile_upsert_from_payload($payload);
        $profiles=[];
        foreach ($payload['players'] as $p) $profiles[] = profile_load($p['playerId']);
        $out[]=['payload'=>$payload, 'index'=>index_extract_entry($payload), 'profiles'=>$profiles];
    } catch (LadderApiException $e) {
        $out[]=['error'=>$e->errorCode(), 'status'=>$e->statusCode()];
    }
}
echo json_encode($out, JSON_THROW_ON_ERROR);
""")

    def run(payloads):
        result = subprocess.run([php, str(probe)], input=json.dumps(payloads), text=True,
                                capture_output=True, check=True, timeout=15)
        return json.loads(result.stdout)

    return run, root


def test_php_drag_total_wins_and_minimum_reaction_survives_profile_updates(php_ladder):
    run, _ = php_ladder
    first = match(players=[player(reaction=100, elapsed=13000), player(1, reaction=400, elapsed=12800, position=2)])
    second = match("drag-2", [player(reaction=500, elapsed=12000), player(1, reaction=500, elapsed=12500, position=2)])
    rows = run([first, first, second])
    assert rows[0]["payload"]["winnerClientNum"] == 0  # Bob's quicker ET is not the winning TOTAL.
    index = rows[0]["index"]
    assert index["mode"] == "GT_DRAG" and index["winner"] == "Alice"
    assert index["weather"] == "dry"
    assert {k: index["players"][0][k] for k in ("dragStatus", "reactionMs", "elapsedMs", "totalRaceMs", "trapSpeedKph")} == {
        "dragStatus": "finished", "reactionMs": 100, "elapsedMs": 13000, "totalRaceMs": 13100, "trapSpeedKph": 151.2}
    for p, score in zip(rows[0]["profiles"], (13, 6)):
        assert p["playerScore"] == score
        assert p["dragCompleted"] == 1
        assert p["bestLapMs"] == 0
    for first_profile, replay_profile in zip(rows[0]["profiles"], rows[1]["profiles"]):
        for key in ("playerScore", "wins", "losses", "dragWins", "dragCompleted", "dragBestMs", "dragBestReactionMs", "dragBestElapsedMs"):
            assert replay_profile[key] == first_profile[key]  # Same match is idempotent.
    local = rows[2]["profiles"][0]
    assert local["dragWins"] == local["dragCompleted"] == 2
    assert local["dragBestMs"] == 12500 and local["dragBestElapsedMs"] == 12000
    assert local["dragBestReactionMs"] == 100  # The fastest legal reaction is kept as the best.


def test_php_drag_solo_ties_and_invalid_attempts_do_not_create_wins(php_ladder):
    run, _ = php_ladder
    solo = match("solo", [player()], online=False)
    tie = match("tie", [player(), player(1, position=1)], online=False)
    false_start = match("false", [player(status="false_start"), player(1)], online=False)
    dnf = match("dnf", [player(status="dnf"), player(1, status="dnf")], online=False)
    solo_row, tie_row, fault_row, dnf_row = run([solo, tie, false_start, dnf])
    assert solo_row["payload"]["winnerClientNum"] == -1 and solo_row["index"]["winner"] == ""
    assert solo_row["profiles"][0]["wins"] == solo_row["profiles"][0]["losses"] == 0
    assert solo_row["profiles"][0]["playerScore"] == 0 and solo_row["profiles"][0]["dragCompleted"] == 1
    assert tie_row["payload"]["winnerClientNum"] == -1 and tie_row["index"]["winner"] == ""
    assert all(p["playerScore"] == 0 and p["wins"] == p["losses"] == 0 for p in tie_row["profiles"])
    assert fault_row["payload"]["winnerClientNum"] == 1 and fault_row["index"]["winner"] == "Bob"
    assert fault_row["profiles"][0]["dragCompleted"] == 2  # Previous solo+tie only.
    assert fault_row["profiles"][0]["losses"] == 1 and fault_row["profiles"][0]["playerScore"] == 0
    assert fault_row["profiles"][1]["dragWins"] == 1 and fault_row["profiles"][1]["playerScore"] == 12
    assert dnf_row["payload"]["winnerClientNum"] == -1
    assert dnf_row["profiles"][0]["dragCompleted"] == 2 and dnf_row["profiles"][1]["dragCompleted"] == 2
    assert all(p["totalRaceMs"] == p["position"] == 0 for p in dnf_row["index"]["players"])


def test_php_drag_bot_duel_awards_the_human_and_a_faster_bot_wins(php_ladder):
    run, root = php_ladder
    bot = player(1, reaction=150, elapsed=12900, position=1)
    bot.update(isBot=True, playerId="bot-carla", name="Carla")
    lost = match("bot-1", [player(reaction=200, elapsed=13000, position=2), bot])
    bot_slow = player(1, reaction=300, elapsed=13300, position=2)
    bot_slow.update(isBot=True, playerId="bot-carla", name="Carla")
    won = match("bot-2", [player(reaction=200, elapsed=13000), bot_slow])
    lost_row, won_row = run([lost, won])
    assert lost_row["payload"]["winnerClientNum"] == 1 and lost_row["index"]["winner"] == "Carla"
    human = lost_row["profiles"][0]
    assert human["losses"] == 1 and human["dragWins"] == 0 and human["dragCompleted"] == 1
    assert human["playerScore"] == 6  # second place points, as against a human
    assert lost_row["profiles"][1] is None  # bots never get a profile
    assert won_row["payload"]["winnerClientNum"] == 0
    assert won_row["profiles"][0]["dragWins"] == 1 and won_row["profiles"][0]["playerScore"] == 6 + 13
    assert len(list((root / "data/profiles").glob("*.json"))) == 1


def test_php_drag_native_snapshot_is_not_double_credited(php_ladder):
    run, _ = php_ladder
    payload = match()
    payload["players"][0]["profile"] = {
        "valid": True, "playerScore": 120, "wins": 1, "dragWins": 1,
        "dragCompleted": 1, "dragBestMs": 13200, "dragBestReactionMs": 200, "dragBestElapsedMs": 13000,
    }
    row = run([payload])[0]
    p = row["profiles"][0]
    assert p["playerScore"] == 120 and p["wins"] == p["dragWins"] == p["dragCompleted"] == 1


@pytest.mark.parametrize("mutation", [
    lambda m: m["settings"].update(dragDuel=False),
    lambda m: m["settings"].update(g_gametype=1),
    lambda m: m.update(teams=[{"team": "red"}]),
    lambda m: m["players"][0].update(isBot="yes"),
    lambda m: m["players"][0].update(dragStatus="countdown"),
    lambda m: m["players"][0].update(reactionMs=True),
    lambda m: m["players"][0].update(reactionMs=99, totalRaceMs=99 + 13000),  # a false start
    lambda m: m["players"][0].update(reactionMs=0, totalRaceMs=13000),
    lambda m: m["players"][0].update(elapsedMs=0),
    lambda m: m["players"][0].update(totalRaceMs=1200),
    lambda m: m["players"][0].update(trapSpeedKph=1001),
    lambda m: m["players"][0].update(dragStatus="false_start"),
    lambda m: m["players"][1].update(playerId=PLAYER_A),
    lambda m: m["players"][1].update(clientNum=0),
    lambda m: m["players"][1].update(position=1),
])
def test_php_drag_rejects_inconsistent_results_without_creating_profiles(php_ladder, mutation):
    run, root = php_ladder
    payload = match()
    mutation(payload)
    result = run([payload])[0]
    assert result["status"] == 422
    assert not list((root / "data/profiles").glob("*.json"))


def _js_function(source, name):
    start = source.index("function " + name + "(")
    opening = source.index("{", start)
    # These production helpers contain no braces in strings except balanced templates.
    level, end = 1, opening + 1
    while level:
        level += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def test_browser_drag_leaderboard_uses_total_and_preserves_rt_et_and_trap(tmp_path, php_ladder):
    node = shutil.which("node")
    if not node:
        pytest.skip("Node is not installed")
    source = (PHP_SOURCE / "index.php").read_text(encoding="utf-8")
    _, php_root = php_ladder
    html = subprocess.run([shutil.which("php"), str(php_root / "index.php")], check=True,
                          capture_output=True, text=True, encoding="utf-8", timeout=10).stdout
    rendered_js = "\n".join(re.findall(r"<script[^>]*>(.*?)</script>", html, re.S))
    whole_page = tmp_path / "ladder-frontend.cjs"
    whole_page.write_text(rendered_js, encoding="utf-8")
    subprocess.run([node, "--check", str(whole_page)], check=True, capture_output=True, text=True, timeout=10)
    script = """const assert = require('node:assert/strict');
const MODE_CONFIG_MAP = new Map([['gt_drag',{}],['gt_racing',{}]]);
function extractIsBot(e) { return !!e.isBot; }
function extractLeaderboardPlayer(e) { return e.name; }
function extractVehicle(e) { return e.vehicle || ''; }
function isReasonableRaceTime(t) { return t > 0 && t < 100000; }
function findBestTime() { throw new Error('Drag must never infer a lap time'); }
"""
    for name in ("extractDragResult", "ingestRaceEntries", "canonicalMode", "getColumnsForMode"):
        script += _js_function(source, name) + "\n"
    script += "const input = " + json.dumps(match()["players"]) + ";\n"
    script += """
assert.equal(canonicalMode('Drag Race'), 'gt_drag');
assert.equal(canonicalMode('GT_DRAG'), 'gt_drag');
assert.equal(canonicalMode('nonsense'), null);
const agg = {raceBest:new Map()};
const ctx = {modeKey:'gt_drag',map:'q3r_dragstrip',mapKey:'q3r_dragstrip',matchId:'1'};
input[0].bestLapMs=1; input[0].elapsedMs=13000; input[0].reactionMs=100; input[0].totalRaceMs=13100;
input.push({...input[0],name:'False Start',dragStatus:'false_start'});
input.push({...input[0],name:'DNF',dragStatus:'dnf'});
input.push({...input[0],name:'Bot',isBot:true});
input.push({...input[0],name:'Contradiction',totalRaceMs:1});
input.push({...input[0],name:'Too Fast',reactionMs:99,totalRaceMs:13099}); /* older record, now a false start */
ingestRaceEntries(agg,input,ctx);
assert.equal(agg.raceBest.size,2);
const rows = [...agg.raceBest.values()].sort((a,b)=>a.time-b.time);
assert.equal(rows[0].player,'Alice'); assert.equal(rows[0].time,13.1);
assert.equal(rows[0].reactionMs,100); assert.equal(rows[0].elapsedMs,13000);
assert.equal(rows[0].trapSpeedKph,151.2);
assert.deepEqual(getColumnsForMode({key:'gt_drag',type:'race'}).map(x=>x.key),
 ['rank','player','time','reaction','elapsed','trap','vehicle','recorded']);
assert.equal(getColumnsForMode({key:'gt_racing',type:'race'}).length,5);
"""
    probe = tmp_path / "drag-frontend.cjs"
    probe.write_text(script)
    subprocess.run([node, str(probe)], check=True, timeout=10, capture_output=True, text=True)
