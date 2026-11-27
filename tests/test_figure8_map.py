"""q3r_dd_02 "Figure of 8": the generator's layout and props (tools/maptools/make_figure8*.py)."""
import importlib
import math
import os
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "maptools"))


def build():
    m = importlib.import_module("make_figure8")
    importlib.reload(m)          # fresh module globals (brush and entity lists)
    kv, kept, others = m.shell()
    arcs = m.eight()
    m.bridge()
    m.tyre_store(m.C1)
    m.scrap_yard(m.C2)
    m.paddock()
    m.mud_pit()
    m.floodlights()
    m.players()
    m.track_items()
    return m, kv, kept, others, m.validate(arcs)


def test_layout_is_valid():
    m, kv, kept, others, report = build()
    assert report["path_clear"]
    assert report["spawns"] == 12
    # the stadium stays, the old field goes
    assert 900 < len(kept) < 1100
    assert all("q3r_dirt_grass/darkdirtgrass01" not in b or "( -1782" in b for b in kept)
    # the bridge: cars pass under it, the ramps are drivable
    assert report["bridge"]["underpass_clearance"] >= 80
    assert report["bridge"]["ramp_degrees"] < 20
    # the two straights cross clearly (not grazing)
    assert 60 < abs(report["straight_angles"][0] - report["straight_angles"][1]) % 180 < 120
    # the weapons of the arena file are there
    picks = report["pickups"]
    for weapon in ("weapon_shotgun", "weapon_rocketlauncher", "weapon_railgun", "weapon_flamethrower"):
        assert picks.get(weapon) == 1


def test_everything_inside_the_boards():
    m, kv, kept, others, report = build()
    for e in m.ENTITIES:
        if e["classname"] in ("light", "info_null"):
            continue
        x, y, _ = map(float, e["origin"].split())
        assert -1600 < x < 1536 and -1600 < y < 1536, e


def test_spawns_face_along_and_apart():
    m, kv, kept, others, report = build()
    starts = [tuple(map(float, e["origin"].split()[:2])) for e in m.ENTITIES if e["classname"] == "info_player_deathmatch"]
    for i, a in enumerate(starts):
        for b in starts[i + 1:]:
            assert math.dist(a, b) > 250, (a, b)
    # none of them right at the bridge
    for x, y in starts:
        assert math.dist((x, y), (m.OX, m.OY)) > 500


def test_models_fit_the_renderer():
    sys.path.insert(0, str(ROOT / "tools" / "koth"))
    mm = importlib.import_module("make_figure8_models")
    for mesh in (mm.container(144.0, "red"), mm.container(288.0, "blue"), mm.tyre_wall(), mm.floodmast(), mm.barrier(),
                 mm.armco(), mm.fuelpump(), mm.railing(), mm.tyre_stack(5, 1)):
        for shader, (V, N, S, T) in mesh.surfaces():
            assert len(V) < 1000 and 3 * len(T) < 6000, shader
            assert shader.startswith("models/mapobjects/q3r_figure8/")


def test_props_on_disk_match_the_map():
    text = (ROOT / "baseq3r" / "maps" / "q3r_dd_02.map").read_text(encoding="latin1")
    folder = ROOT / "baseq3r" / "models" / "mapobjects" / "q3r_figure8"
    used = {line.split("/")[-1].strip('"') for line in text.splitlines() if "q3r_figure8/" in line and ".md3" in line}
    on_disk = {f for f in os.listdir(folder) if f.endswith(".md3")}
    assert used == on_disk
    arena = (ROOT / "baseq3r" / "scripts" / "q3r_dd_02.arena").read_text()
    assert "q3r_derby" in arena
