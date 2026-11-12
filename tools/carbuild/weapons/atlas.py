"""The shared material atlas of the Q3Rally weapons: 512 x 512, 4 x 4 cells
of 128 px, one material per cell. prim.Mesh maps every face into the cell
of its material (4 texels per unit, so faces up to ~30 units fit).

  gunmetal  black     orange    alu
  copper    chrome    steel     dark
  (free)    ...

Also writes lampflash.tga (the Spotlight MG's ring flash) and chrome.tga
(environment map for the chrome shader).
"""
from __future__ import annotations

import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

SIZE, CELL = 512, 128
ORDER = ["gunmetal", "black", "orange", "alu", "copper", "chrome", "steel", "dark"]
CELLS = {name: ((i % 4) * CELL / SIZE, (i // 4) * CELL / SIZE, CELL / SIZE) for i, name in enumerate(ORDER)}

BASE = {"gunmetal": (58, 62, 68), "black": (26, 27, 29), "orange": (238, 100, 26), "alu": (176, 182, 190),
        "copper": (176, 110, 58), "chrome": (200, 204, 210), "steel": (84, 88, 96), "dark": (12, 12, 13)}


def cell_texture(name, rng):
    base = np.array(BASE[name], np.float32)
    yy, xx = np.mgrid[0:CELL, 0:CELL]
    noise = rng.random((CELL, CELL)).astype(np.float32)
    small = np.asarray(Image.fromarray((rng.random((16, 16)) * 255).astype(np.uint8)).resize(
        (CELL, CELL), Image.BICUBIC)).astype(np.float32) / 255
    if name in ("alu", "chrome", "steel"):
        # brushed: streaks along one direction
        streak = np.asarray(Image.fromarray((rng.random((CELL, 8)) * 255).astype(np.uint8)).resize(
            (CELL, CELL), Image.BILINEAR)).astype(np.float32) / 255
        f = 0.88 + 0.16 * streak + 0.04 * noise
    elif name in ("black", "orange", "gunmetal"):
        # powder coat / paint: fine grain, slight cloudiness
        f = 0.93 + 0.08 * small + 0.04 * noise
    elif name == "copper":
        f = 0.8 + 0.25 * small + 0.08 * noise
    else:
        f = 0.9 + 0.1 * noise
    img = base * f[..., None]
    # worn edges: brighter metal shows near the cell border (= edges of large faces)
    edge = np.minimum.reduce([xx, yy, CELL - 1 - xx, CELL - 1 - yy]).astype(np.float32)
    wear = np.clip(1 - edge / 10, 0, 1) * (rng.random((CELL, CELL)) > 0.55)
    if name in ("orange", "black", "gunmetal"):
        img = img * (1 - 0.5 * wear[..., None]) + np.array([150, 150, 150]) * 0.5 * wear[..., None]
    return img


def write(out: Path):
    out.mkdir(parents=True, exist_ok=True)
    rng = np.random.default_rng(42)
    atlas = np.zeros((SIZE, SIZE, 3), np.float32)
    atlas[:] = (128, 0, 128)
    for name, (u0, v0, _) in CELLS.items():
        x, y = int(u0 * SIZE), int(v0 * SIZE)
        atlas[y:y + CELL, x:x + CELL] = cell_texture(name, rng)
    Image.fromarray(atlas.clip(0, 255).astype(np.uint8)).save(out / "parts.tga")
    # environment map for chrome: sky above, dark ground below, a bright horizon band
    w, h = 128, 128
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    t = yy / h
    env = np.where(t[..., None] < 0.5, np.array([150, 165, 185]) * (0.7 + 0.6 * t[..., None]),
                   np.array([40, 38, 36]) * (1.5 - t[..., None]))
    env += np.exp(-((t - 0.5) / 0.04) ** 2)[..., None] * 140
    Image.fromarray(env.clip(0, 255).astype(np.uint8)).save(out / "chrome.tga")
    # the ring flash of the Spotlight MG lamp
    n = 128
    yy, xx = np.mgrid[0:n, 0:n] - (n - 1) / 2
    rr = np.hypot(xx, yy) / (n / 2)
    ring = np.exp(-((rr - 0.78) / 0.12) ** 2) + 0.35 * np.clip(1 - rr, 0, 1) ** 2
    col = np.clip(ring, 0, 1)[..., None] * np.array([255, 236, 190])
    Image.fromarray(col.clip(0, 255).astype(np.uint8)).save(out / "lampflash.tga")
