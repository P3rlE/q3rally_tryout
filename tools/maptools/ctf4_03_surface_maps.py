"""Q3Rally: hand-made surface maps for the textures of q3r_ctf4_03
(Industrial Crossfire). Presets: surface_maps_common.py.

The ballast bed (make_industrialcrossfire.write_textures) and the wagon
skins (tools/mapobjects/make_rail.py) are procedural: their relief is
rebuilt from the same recipe. q3r_ctf4_01 only has q3r_layout placeholder
textures and gets no maps.

usage: python ctf4_03_surface_maps.py <baseq3r> <out baseq3r> [--preview <dir>]
"""
import math
import os
import random
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import surface_maps_common as sm  # noqa: E402

args = sys.argv[1:]
prev = None
if "--preview" in args:
    i = args.index("--preview")
    prev = args[i + 1]
    del args[i:i + 2]
sm.setup(args[0], args[1], prev)


def ballast():
    """the 900 stones of write_textures, same random sequence: domes"""
    rel = "q3r_ctf4_03/ballast"
    size = 256
    rnd = random.Random(5)
    for _ in range(size * size):
        rnd.randint(62, 122)
    h = np.zeros((size, size), np.float32)
    for _ in range(900):
        cx, cy, r = rnd.randrange(size), rnd.randrange(size), rnd.uniform(2.5, 5.5)
        rnd.uniform(0.85, 1.2)
        for dy in range(-6, 7):
            for dx in range(-6, 7):
                d = math.hypot(dx, dy * 1.2) / r
                if d < 1:
                    y, x = (cy + dy) % size, (cx + dx) % size
                    h[y, x] = max(h[y, x], math.sqrt(1 - d * d) * min(1.0, r / 4.0))
    l = sm.lum(sm.load(rel))
    h = 0.1 + 0.85 * sm.blur(h, 0.6) + 0.05 * sm.bp(l, 0.5, 1.5)
    sm.write(rel, np.clip(h, 0, 1), 3.0, 0.03, 0.15 + 0.1 * h, True)


R = "models/mapobjects/q3r_rail/"


def wagon_body(color):
    """make_rail: boards = 0.04 * cos(2 pi 6 x)^8 darkens 6 board joints;
    paint flakes (bright patches) sit a little lower"""
    rel = R + "body_" + color
    rgb = sm.load(rel)
    S = rgb.shape[0]
    yy, xx = np.mgrid[0:S, 0:S] / S
    joint = np.cos(xx * 2 * math.pi * 6) ** 8
    l = sm.lum(rgb)
    flake = sm.blur(np.clip((l - sm.blur(l, 6)) / 0.05, 0, 1), 0.8)
    h = 0.95 - 0.8 * joint - 0.12 * flake + 0.03 * sm.bp(l, 0.5, 1.5)
    sm.write(rel, np.clip(h, 0, 1), 2.5, 0.04, 0.35 - 0.15 * flake, True)


def wagon_deck():
    """make_rail: 8 planks across, gap where |frac(8x) - 0.5| > 0.47"""
    rel = R + "deck"
    S = 128
    yy, xx = np.mgrid[0:S, 0:S] / S
    f = np.abs((xx * 8) % 1 - 0.5)
    gap = sm.blur((f > 0.47).astype(np.float32), 0.5)
    alt = (np.floor(xx * 8) % 2) * 0.08
    l = sm.lum(sm.load(rel))
    h = 0.85 + alt - 0.8 * gap + 0.04 * sm.bp(l, 0.5, 2.0)
    sm.write(rel, np.clip(h, 0, 1), 1.5, 0.03, 0.18, True)


B = "q3r_building/"
M = "q3r_metals/"
sm.window_grid(B + "ind-windows-1", depth=3.0)
sm.rust_metal(M + "metrust01")
sm.rust_metal(M + "rustblumet01")
sm.rust_metal(M + "Rost", gloss=0.35)
sm.rust_metal("base_trim/yellow_rustbx128", gloss=0.45)           # hazard stripes on rusty plate
print("diamond2 lattice", sm.tread_soft("base_floor/diamond2"))   # pattern too faint to cut out the bosses
sm.sat_relief(B + "brick4", depth=2.0)                             # bricks in light mortar
sm.flat("q3r_road/concrete2", depth=1.2, gloss=0.2)                # kerb / coping
ballast()
for c in ("brown", "grey", "red"):
    wagon_body(c)
wagon_deck()
sm.corrugated(R + "roof", axis=1, depth=2.0)                        # 4 waves across
sm.flat(R + "tank", depth=0.5, gloss=0.5, refl=0.045)               # smooth paint
