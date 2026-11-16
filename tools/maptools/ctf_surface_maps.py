"""Q3Rally: hand-made surface maps for the textures of q3r_ctf_01 and
q3r_ctf_04. Presets: surface_maps_common.py. q3r_asphalt/cobbles01 is a
byte copy of q3r_road/cobbles01 and gets copies of its maps (done by the
caller). Signs, banners, flags, lights, teleporters, crowd, water, glass,
effects and alpha-tested grates get none.

usage: python ctf_surface_maps.py <baseq3r> <out baseq3r> [--preview <dir>]
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

# q3r_ctf_01
sm.flat("base_wall/concrete", depth=1.2)
sm.relief("q3r_ground/trim18", depth=2.5)
sm.relief("gothic_block/blocks17", depth=2.5)
sm.relief("q3r_building/brick-red-1", depth=2.0)            # dark mortar
sm.relief("q3r_building/bricks01", depth=2.0)
sm.rust_metal("gothic_wall/iron01_e")
sm.relief("gothic_wall/iron01_p", depth=3.0, refl=0.08, gloss=0.3)   # skulls and spikes
sm.flat("q3r_road/road", depth=1.0, gloss=0.3)
sm.flat("q3r_dirt_grass/darkdirt01", depth=1.2, gloss=0.14)
sm.rust_metal("base_trim/rusty_pewter_big")
sm.flat("base_floor/dirt", depth=1.2, gloss=0.14)
sm.sat_relief("q3r_building/brick01", depth=2.0)             # red bricks, light mortar
sm.corrugated("gothic_trim/column2c_test", axis=1, depth=3.0, refl=0.04, gloss=0.3)   # flutes
sm.relief("gothic_trim/border7", depth=2.0)
sm.rust_metal("q3r_metals/iron02")
sm.relief("q3r_tech/tex9_13", depth=1.5, refl=0.1, gloss=0.4)
for i in range(1, 6):
    for s in "LR":
        sm.relief("gothic_cath/graycathflesh%d%s" % (i, s), depth=3.0)
# q3r_ctf_04
sm.grass("q3r_dirt_grass/grass01")
sm.relief("evil4_walls/rbrick", depth=2.0)
sm.flat("evil5_ctf/ctf_crete_red", depth=1.2)
sm.flat("evil5_ctf/ctf_crete_blue", depth=1.2)
sm.sat_relief("evil2_basewall/dirtybrick", depth=2.0)
sm.sat_relief("evil2_basewall/dirtybrick_faded", depth=2.0)
sm.flat("evil5_wall/concrete", depth=1.2)
sm.panel("evil8_floor/e8cretefloor_tile", depth=1.5, refl=0.035, gloss=0.25)
sm.rust_metal("q3r_tech/tex9_15", gloss=0.45)                 # hazard stripes
sm.tread_plate("evil8_floor/e8clangfloor", n=20, depth=2.0, refl=0.1, gloss=0.35)   # clean alternating ovals
# too few repeats per tile for the lattice average: plain normal from the image
sm.flat("evil8_floor/e8clangfloor05", depth=1.2, gloss=0.35, refl=0.1)
sm.flat("base_floor/clangdark", depth=1.2, gloss=0.3, refl=0.1)
sm.rust_metal("models/mapobjects/carlift/carlift")
sm.relief("evil2_misc/mtl_meshplate", depth=1.5, refl=0.1, gloss=0.35)
sm.flat("models/mapobjects/dumpster/lightgrey", depth=0.5, gloss=0.35, refl=0.05)
sm.relief("evil4_techfloors/tfloorhex_big", depth=2.0, refl=0.1, gloss=0.35)
sm.rust_metal("base_support/flat1_1")
sm.flat("q3r_wood/Holz", depth=0.8, gloss=0.25)
