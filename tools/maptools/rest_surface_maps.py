"""Q3Rally: hand-made surface maps for all remaining lit textures of the
shipped maps (Ridge Pass, Ridge Racer, Downtown, mk, Lavafalls, Valley,
Riviera, Quarry, Canyon Cruise, Sundown, David4/5, dd_02, dm02, Demobowl,
Dirty Fun, Night City, LCS). Presets: surface_maps_common.py.

Left out: signs and arrows (turns, chevrons, scoreboard, gasoline), cloth
banners (ctf/test2*), screen and environment images (comp3env, comp3text).

usage: python rest_surface_maps.py <baseq3r> <out baseq3r> [--preview <dir>]
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import surface_maps_common as sm  # noqa: E402

args = sys.argv[1:]
prev = None
if "--preview" in args:
    i = args.index("--preview")
    prev = args[i + 1]
    del args[i:i + 2]
sm.setup(args[0], args[1], prev)

MET = dict(refl=0.1, gloss=0.35)
FLAT = lambda d=1.2, g=0.2: (sm.flat, dict(depth=d, gloss=g))
ROAD = (sm.flat, dict(depth=1.0, gloss=0.3))
DIRT = (sm.flat, dict(depth=1.2, gloss=0.14))
SNOW = (sm.flat, dict(depth=0.8, gloss=0.4, refl=0.03))
GRASS = (sm.grass, {})
RUST = (sm.rust_metal, {})
REL = lambda d=2.5, **k: (sm.relief, dict(depth=d, **k))
RMET = lambda d=2.0: (sm.relief, dict(depth=d, **MET))

JOBS = {
    # Ridge Pass / Ridge Racer
    "arctic/rock1_1": REL(3.0, gloss=0.15),
    "arctic/concrete1_1": FLAT(),
    "q3r_ground/snowy04": SNOW,
    "q3r_ground/snowy02_slick": (sm.flat, dict(depth=0.8, gloss=0.6, refl=0.035)),   # icy
    "q3r_road/cement_redwhite": FLAT(1.0, 0.3),
    "q3r_building/windows21a": REL(2.0),
    "arctic/brick5_1": REL(2.0), "arctic/brick5_4": REL(2.0), "arctic/brick5_6": REL(2.0),
    "arctic/snow1_1": SNOW, "arctic/snow1_3_asph": SNOW,
    "arctic/concrete2_1": FLAT(), "arctic/concrete1_10": FLAT(), "arctic/concrete5_1": FLAT(),
    "arctic/concrete4_1": FLAT(), "arctic/floor1_1": FLAT(),
    "arctic/pipe2_5": (sm.corrugated, dict(axis=1, depth=2.0)),
    "arctic/trim1_2": RUST, "arctic/support4_1": RMET(), "arctic/support6_3": RMET(),
    "arctic/trim5_5": RMET(),
    "q3r_building/redbrick": (sm.sat_relief, dict(depth=2.0)),
    # Downtown
    "base_wall/concrete_dark": FLAT(), "q3r_road/road-1": ROAD, "q3r_road/road_02": ROAD,
    "q3r_road/road_grid_s": ROAD, "q3r_road/road_pitent": ROAD, "olirally/road-1": ROAD,
    "q3r_metals/terra_tilesml": FLAT(0.6, 0.3), "base_trim/pewter": RUST,
    "base_trim/xian_dm3padwall_x": REL(2.5), "gothic_wall/support2b": FLAT(1.0, 0.3),
    "q3r_metals/barrel": RUST,
    # mk
    "evil2_basefloor/flr_cemtilesmll_drk": (sm.panel, dict(depth=1.5, refl=0.035, gloss=0.25)),
    "q3r_building/brick02": REL(2.0), "evil5_floor/confllr": FLAT(), "evil5_metal/metal": RUST,
    "evil8_base/e8base_blue": FLAT(1.0, 0.3), "evil8_base/e8base_red": FLAT(1.0, 0.3),
    "evil8_base/e8base_green": FLAT(1.0, 0.3), "evil8_base/e8base_yellow": FLAT(1.0, 0.3),
    "evil5_ctf/ctf_crete_green": FLAT(), "evil5_ctf/ctf_crete_yellow": FLAT(),
    "evil4_metals/defaultmetal": RUST, "evil5_metal/panel128b": RMET(1.5),
    # Lavafalls
    "stone/pjrock1": REL(3.0, gloss=0.15), "jim_misc/trim18": REL(2.5), "stone/rockwall": REL(4.0, gloss=0.15),
    "organics/dirt2": DIRT, "gothic_floor/largerblock3b3dim": REL(3.0), "organics/pjrock7d": REL(4.0, gloss=0.15),
    "base_wall/metfloor_block_3": REL(2.5), "gothic_trim/pitted_rust": RUST, "base_trim/panel_edged": RMET(),
    "base_floor/diamond2c": (sm.flat, dict(depth=1.2, **MET)), "gothic_floor/largerblock3b3": REL(3.0),
    "gothic_wall/streetbricks14": REL(2.5), "base_wall/patch10_beatup2": RUST,
    "gothic_trim/km_arena1tower1": REL(2.0), "gothic_floor/q1metal7_97": RMET(1.5),
    "base_wall/bluemetalsupport": RUST, "gothic_wall/xiantourneycolumn_b6": REL(3.0),
    "base_wall/patch10_beatup4": RUST, "gothic_wall/iron01_c3": RMET(), "base_wall/atech1_a": RMET(),
    "jim_castle/drock019": REL(3.0, gloss=0.15), "gothic_wall/crack5c": REL(2.0), "base_wall/atech1_e": RMET(),
    "base_wall/concretepipe01": REL(2.0), "base_support/pip03_2": RMET(), "base_wall/patch10_beatup3": RUST,
    "base_support/surface2": RUST, "gothic_floor/blocks17floor3": REL(2.5), "gothic_wall/iron01_c": RMET(),
    "organics/wire02a_f": RMET(2.5), "base_wall/atech1_b": RMET(), "gothic_wall/supportborder_blue_b": RMET(),
    "base_wall/patch10rusty5": RUST, "base_wall/metalfloor_wall_10": RMET(1.5),
    "base_floor/tilefloor7_ow": REL(2.0), "base_wall/bluemetal3": RUST, "base_support/grate1_4": RMET(),
    "base_trim/metal2_2": RMET(2.5), "base_support/pip03_3": RMET(),
    # LCS
    "q3r_road/asph03": ROAD, "q3r_redrock/paint_white": FLAT(1.0, 0.35), "q3r_redrock/asph_patch": ROAD,
    # Night City
    "base_wall/bluemetal3b": RUST,
    # Quarry
    "q3r_quarry/dust": DIRT, "q3r_quarry/limestone": REL(3.0, gloss=0.15), "q3r_quarry/gravel": REL(3.0, gloss=0.15),
    "q3r_quarry/block": REL(3.0, gloss=0.15),
    # Riviera
    "q3r_dirt_grass/redrock03": REL(4.0, gloss=0.15), "q3r_stone/stein2": REL(3.0, gloss=0.15),
    "q3r_building/stuccowin01": REL(2.0), "q3r_building/stuccowin02": REL(2.0),
    "q3r_building/stuccowin03": REL(2.0), "q3r_building/stuccowin04": REL(2.0),
    # Canyon Cruise
    "q3r_dirt_grass/dirt01road03": DIRT, "q3r_dirt_grass/grass08": GRASS, "q3r_dirt_grass/dirt02road04f": DIRT,
    "q3r_dirt_grass/dirt02road03b": DIRT, "q3r_metals/cormetal02": (sm.cormetal, {}), "gothic_trim/wood2": REL(2.0, gloss=0.18),
    # Sundown
    "q3r_sundown/turf": GRASS, "q3r_tech/tex9_15_blu": (sm.rust_metal, dict(gloss=0.45)),
    "q3r_tech/tex9_5": (sm.flat, dict(depth=1.2, **MET)),
    # Valley
    "dip_valley/stone": REL(4.0, gloss=0.15), "evil1_walls/cementwall": FLAT(1.4), "evil1_woods/wood_pnl": REL(2.0, gloss=0.18),
    "evil1_woods/wood": REL(2.0, gloss=0.18), "evil1_walls/cementwall_btm": FLAT(1.4), "evil1_walls/drtwll3": FLAT(1.4),
    "evil1_metals/de-r_mtl2b": RUST, "evil1_trims/crude_warn": RUST, "evil1_walls/cemold": FLAT(1.4),
    "evil1_walls/smt_cemwll": FLAT(1.2), "evil1_metals/de-cleanmtl": (sm.flat, dict(depth=1.0, **MET)),
    "base_wall/redmet4small": RUST,
    # David4 / David5 / dd_02 / dm02 / Demobowl / Dirty Fun
    "q3r_dirt_grass/darkdirtgrass01": GRASS, "base_trim/yellow_rustb": RUST,
    "models/mapobjects/oak/oakstamm": REL(2.0, gloss=0.15),
    "gothic_wall/streetbricks11": REL(2.5), "base_wall/c_met7_2": RMET(1.5), "e7/e7sbrickfloor": REL(2.5),
    "base_support/cable": (sm.corrugated, dict(axis=0, depth=2.0)), "gothic_trim/baseboard01": RMET(),
    "base_trim/yellow_rustb_v1": RUST, "base_floor/clang_floor2": (sm.flat, dict(depth=1.2, **MET)),
    "base_trim/basemetalsupport": RMET(), "q3r_dirt_grass/gravel01": REL(3.0, gloss=0.15),
    "q3r_dirt_grass/dirt01road01a": DIRT, "q3r_dirt_grass/dirt02road01e": DIRT, "jim/road": ROAD,
    "q3r_dirt_grass/lawn01": GRASS, "q3r_dirt_grass/dirt01road01c": DIRT, "base_floor/concfloor_rain": FLAT(),
}
for n in ("wideroad_line", "wideroad_dots", "sand_trk", "road", "road_line", "sand_trk_ed", "wideroad_sand2",
          "wideroad_sand", "sand_trk_eu", "wideroad_end", "road_sand_u"):
    JOBS["dip_valley/" + n] = ROAD

# the file names on disk can differ in case from the shader names
import glob  # noqa: E402


def disk_name(rel):
    base = os.path.join(sm.SRC, sm.path_of(rel))
    d, stem = os.path.dirname(base), os.path.basename(base).lower()
    for c in glob.glob(os.path.join(d, "*")):
        s, e = os.path.splitext(os.path.basename(c))
        if s.lower() == stem and e.lower() in (".jpg", ".tga", ".png"):
            return rel[: len(rel) - len(stem)] + s
    return rel


fails = []
for rel, (fn, kw) in JOBS.items():
    try:
        fn(disk_name(rel), **kw)
    except Exception as e:  # keep going, report at the end
        fails.append((rel, repr(e)))
print("done", len(JOBS) - len(fails), "of", len(JOBS))
for f in fails:
    print("FAIL", *f)
