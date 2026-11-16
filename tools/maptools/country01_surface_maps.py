"""Q3Rally: hand-made surface maps for the textures of q3r_country01
("Roll On Down The Line"). Presets: surface_maps_common.py.

usage: python country01_surface_maps.py <baseq3r> <out baseq3r> [--preview <dir>]
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

B = "q3r_building/"
sm.relief("gothic_block/block10d", depth=2.5)                      # the road: rough stone blocks
sm.facade_flat("q3r_trees/pinewall02a", depth=1.5)                 # painted forest wall: trunks, foliage
sm.cormetal("q3r_metals/cormetal01")
sm.flat(B + "stucco02", depth=1.4, gloss=0.15)
sm.relief(B + "wood03", depth=2.0, gloss=0.18)                     # planks, dark gaps between them
sm.flat("q3r_metals/rustmet01", depth=1.0, gloss=0.3, refl=0.08)
sm.sat_relief(B + "bricks11", depth=2.0)                           # red bricks in light mortar
sm.facade_flat(B + "bricks05", depth=1.2)                          # painted bricks with poster and lettering
sm.facade_flat(B + "bricks04", depth=1.2)
