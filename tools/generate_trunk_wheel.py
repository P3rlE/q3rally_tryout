#!/usr/bin/env python3
"""Generate the anti-aliased masks for the trunk (holdable item) wheel.

The 2D HUD can only draw rectangles and pictures, so the round wheel is drawn
from white alpha masks that cgame tints with trap_R_SetColor.  Every mask
covers the whole wheel box, so all of them are drawn at the same rectangle.

Keep the geometry in sync with TRUNK_WHEEL_* in code/cgame/cg_draw.c:
  box    72 units (wheel drawn at centre +/- 36)
  outer  32 units, inner 13 units, gap 1.5 units between sectors

Writes baseq3r/gfx/hud/trunk_sector{1,2,3}.png, trunk_edge{1,2,3}.png and
trunk_outline.png.  Run from anywhere:  python tools/generate_trunk_wheel.py
"""

import math
import os

import numpy as np
from PIL import Image

SIZE = 256          # texture size in pixels
SAMPLES = 8         # supersamples per axis
BOX = 72.0          # wheel box in 640x480 HUD units
OUTER = 32.0
INNER = 13.0
GAP = 0.75          # half the gap between neighbouring sectors
STROKE = 0.8        # outline width
EDGE = 2.0          # width of the highlight band on the outer rim
# Sector centre angles, screen space (y down): top, lower right, lower left.
CENTRES = (-90.0, 30.0, 150.0)
HALF = 60.0

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "baseq3r", "gfx", "hud")


def grid():
    n = SIZE * SAMPLES
    coords = (np.arange(n) + 0.5) / n * BOX - BOX * 0.5
    return np.meshgrid(coords, coords)


def sector(x, y, centre, inner, outer, gap):
    radius = np.hypot(x, y)
    delta = np.degrees(np.arctan2(y, x)) - centre
    delta = (delta + 180.0) % 360.0 - 180.0
    inside = np.abs(delta) <= HALF
    # perpendicular distance to the nearer of the two dividing lines
    side = radius * np.sin(np.radians(np.clip(HALF - np.abs(delta), 0.0, 90.0)))
    return inside & (radius >= inner) & (radius <= outer) & (side >= gap)


def save(mask, name):
    coverage = mask.reshape(SIZE, SAMPLES, SIZE, SAMPLES).mean(axis=(1, 3))
    alpha = np.round(coverage * 255.0).astype(np.uint8)
    rgba = np.zeros((SIZE, SIZE, 4), dtype=np.uint8)
    rgba[..., :3] = 255
    rgba[..., 3] = alpha
    Image.fromarray(rgba, "RGBA").save(os.path.join(OUT, name), optimize=True)
    print("wrote", os.path.relpath(os.path.join(OUT, name), ROOT))


def main():
    os.makedirs(OUT, exist_ok=True)
    x, y = grid()
    outline = np.zeros_like(x, dtype=bool)
    for slot, centre in enumerate(CENTRES, start=1):
        fill = sector(x, y, centre, INNER, OUTER, GAP)
        core = sector(x, y, centre, INNER + STROKE, OUTER - STROKE, GAP + STROKE)
        rim = fill & (np.hypot(x, y) >= OUTER - EDGE)
        outline |= fill & ~core
        save(fill, "trunk_sector%d.png" % slot)
        save(rim, "trunk_edge%d.png" % slot)
    save(outline, "trunk_outline.png")


if __name__ == "__main__":
    main()
