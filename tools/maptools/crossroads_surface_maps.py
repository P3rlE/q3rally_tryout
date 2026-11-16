"""Q3Rally: hand-made surface maps for the textures of q3r_crossroads.
Presets: surface_maps_common.py.

The ground is terrain (q3r_crossroads.shader from crossroads_textures.py);
every layer stage that finds the lightmap gets its own maps. pebble01 has
its maps from the citydom set.

Also rebuilds cormetal01/03/04: the corrugation used to be integrated with
the wrong sign (lit crests came out as troughs).

usage: python crossroads_surface_maps.py <baseq3r> <out baseq3r> [--preview <dir>]
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

G = "q3r_dirt_grass/"
B = "q3r_building/"
sm.grass(G + "grass07")                                   # the big meadows
sm.grass(G + "grass09")                                   # dry grass
sm.flat(G + "dirt02", depth=1.2, gloss=0.14)              # dirt tracks, fine pebbles
sm.relief(G + "stones01", depth=3.0, gloss=0.18)          # gravel, stones raised
sm.relief(G + "rock03", depth=4.0, gloss=0.15)            # weathered rock
sm.flat("base_floor/concrete", depth=1.2, gloss=0.22)     # concrete pads
sm.corrugated(B + "wellblech", depth=2.0)                 # vertical waves, 10 across
sm.corrugated(B + "roof3", axis=1, depth=4.0, cross=0.3, refl=0.04, gloss=0.35)   # barrel tiles
sm.relief(B + "wood02", depth=2.0, gloss=0.18)            # plank wall
sm.flat(B + "doors_wood01", depth=0.8, gloss=0.3)
for c in ("cormetal01", "cormetal03", "cormetal04"):
    sm.cormetal("q3r_metals/" + c)
