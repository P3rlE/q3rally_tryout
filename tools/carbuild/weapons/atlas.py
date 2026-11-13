"""The shared material atlas of the Q3Rally weapons: 512 x 512, 4 x 4 cells
of 128 px, one material per cell. prim.Mesh maps every face into the cell
of its material (4 texels per unit, so faces up to ~30 units fit).

  gunmetal  black     orange    alu
  copper    chrome    steel     dark
  carbon    heat      fins      perf
  glow*     disc      label     white

glow* is split into four 64 px cells of flat light colours (ice, mint,
blue, green) for the glowing parts; disc (a drilled brake disc) and label
(the "Q3R" band of the Nitro Flamer bottle) are mapped by hand, not by
projection.

Also writes lampflash.tga (the Spotlight MG's ring flash), chain.tga (the
scrolling saw chain of the Harvester Saw), chrome.tga (environment map
for the chrome shader) and the muzzle flash textures: flash_star (seen
from the front), flash_cone (seen from the side, base at the left) and
their *_core versions, the white-hot middle. They are grey; each weapon's
flash shader tints them with its colour.
"""
from __future__ import annotations

import math
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

SIZE, CELL = 512, 128
ORDER = ["gunmetal", "black", "orange", "alu", "copper", "chrome", "steel", "dark",
         "carbon", "heat", "fins", "perf", None, "disc", "label", "white"]
CELLS = {name: ((i % 4) * CELL / SIZE, (i // 4) * CELL / SIZE, CELL / SIZE)
         for i, name in enumerate(ORDER) if name}
GLOWS = {"ice": (127, 216, 255), "mint": (124, 242, 200), "blue": (79, 195, 255), "green": (140, 255, 106)}
for _i, _name in enumerate(GLOWS):
    CELLS[_name] = (((_i % 2) * 64) / SIZE, (3 * CELL + (_i // 2) * 64) / SIZE, 64 / SIZE)

BASE = {"gunmetal": (58, 62, 68), "black": (26, 27, 29), "orange": (238, 100, 26), "alu": (176, 182, 190),
        "copper": (176, 110, 58), "chrome": (200, 204, 210), "steel": (84, 88, 96), "dark": (12, 12, 13),
        "carbon": (34, 36, 40), "heat": (96, 92, 120), "fins": (44, 47, 52), "perf": (168, 172, 178),
        "white": (226, 228, 230)}
FONT = "/usr/share/fonts/truetype/google-fonts/Poppins-Bold.ttf"


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
    elif name == "carbon":
        # 2x2 twill: diagonal bands of tows, 4 px per tow
        tw = ((xx // 4 + yy // 4) % 4 < 2)
        f = np.where(tw, 1.25, 0.8) * (0.95 + 0.1 * noise)
        f = f * (0.9 + 0.2 * (((xx + (yy // 4) * 4) % 4) / 3) * tw + 0.2 * ((yy % 4) / 3) * ~tw)
    elif name == "fins":
        # intercooler core: bright fin edges every unit, dark gaps
        f = np.where(xx % 4 == 0, 2.6, np.where(xx % 4 == 2, 0.6, 1.0)) * (0.95 + 0.1 * noise)
        f = f * np.where(yy % 16 == 0, 1.6, 1.0)
    else:
        f = 0.9 + 0.1 * noise
    img = base * f[..., None]
    if name == "heat":
        # heat-tinted stainless: straw, purple and blue in soft bands
        bands = np.array([[176, 150, 96], [130, 92, 128], [72, 96, 156], [110, 120, 140]], np.float32)
        t = (small * 3.0 + yy / CELL * 2.0) % 4
        i0 = np.floor(t).astype(int) % 4
        w = (t - np.floor(t))[..., None]
        img = (bands[i0] * (1 - w) + bands[(i0 + 1) % 4] * w) * (0.9 + 0.12 * noise[..., None])
    if name == "perf":
        hole = ((xx % 6 - 2.5) ** 2 + (yy % 6 - 2.5) ** 2) < 3.2
        img = np.where(hole[..., None], np.array([18, 18, 20], np.float32), img)
    # worn edges: brighter metal shows near the cell border (= edges of large faces)
    edge = np.minimum.reduce([xx, yy, CELL - 1 - xx, CELL - 1 - yy]).astype(np.float32)
    wear = np.clip(1 - edge / 10, 0, 1) * (rng.random((CELL, CELL)) > 0.55)
    if name in ("orange", "black", "gunmetal"):
        img = img * (1 - 0.5 * wear[..., None]) + np.array([150, 150, 150]) * 0.5 * wear[..., None]
    return img


def disc_texture():
    """A drilled, slotted brake disc filling the cell; mapped radially."""
    n = CELL
    yy, xx = np.mgrid[0:n, 0:n] - (n - 1) / 2
    rr = np.hypot(xx, yy) / (n / 2)
    ang = np.arctan2(yy, xx)
    rng = np.random.default_rng(7)
    img = np.empty((n, n, 3), np.float32)
    img[:] = (150, 154, 160)
    img *= (0.9 + 0.12 * np.cos(rr * 90))[..., None] * (0.95 + 0.05 * rng.random((n, n)))[..., None]
    hat = rr < 0.5
    img[hat] = (200, 204, 210)                                   # alu hat
    img[(rr > 0.47) & (rr < 0.5)] = (60, 62, 66)
    for k in range(5):                                           # wheel bolt holes in the hat
        a = 2 * np.pi * k / 5
        img[np.hypot(xx - 0.3 * n / 2 * np.cos(a), yy - 0.3 * n / 2 * np.sin(a)) < 4.5] = (30, 30, 32)
    img[rr < 0.12] = (40, 42, 46)
    img[np.hypot(xx, yy) < 3] = (200, 204, 210)
    for row, r in ((0, 0.62), (1, 0.75), (2, 0.88)):             # three staggered rows of drilled holes
        for k in range(18):
            a = 2 * np.pi * (k + row / 3) / 18
            img[np.hypot(xx - r * n / 2 * np.cos(a), yy - r * n / 2 * np.sin(a)) < 2.6] = (16, 16, 18)
    for k in range(6):                                           # curved slots
        a0 = 2 * np.pi * k / 6 + 0.3
        sl = (rr > 0.55) & (rr < 0.95) & (np.abs(((ang - a0 - (rr - 0.55) * 1.2 + np.pi) % (2 * np.pi)) - np.pi) < 0.03)
        img[sl] = (40, 40, 44)
    img[rr > 0.98] = (90, 92, 98)
    return img


def label_texture():
    """White band with an orange stripe and "Q3R" twice round; on a cylinder
    u runs round the bottle, v along it (v falls towards the front), so the
    text is turned on its side."""
    w = h = CELL
    im = Image.new("RGB", (w, h), BASE["white"])
    d = ImageDraw.Draw(im)
    for x0 in (0, w // 2):
        d.rectangle((x0 + 2, 0, x0 + 9, h), fill=BASE["orange"])
        d.rectangle((x0 + w // 2 - 9, 0, x0 + w // 2 - 2, h), fill=BASE["orange"])
    try:
        from PIL import ImageFont
        font = ImageFont.truetype(FONT, 30)
    except OSError:
        font = None
    txt = Image.new("RGBA", (h, w // 2 - 22), (0, 0, 0, 0))
    td = ImageDraw.Draw(txt)
    td.text((h / 2, (w // 2 - 22) / 2), "Q3R", fill=(22, 24, 27, 255), font=font, anchor="mm")
    # first copy on the left of the bottle (u runs upwards there), second on
    # the right (u runs downwards): turned the opposite ways, both read upright
    for x0, rot in ((11, -90), (w // 2 + 11, 90)):
        t = txt.rotate(rot, expand=True)
        im.paste(t, (x0, 0), t)
    return np.asarray(im).astype(np.float32)


def flash_textures(rng):
    """Star (front view) and cone (side view) of a muzzle flash, grey, plus
    the white-hot cores."""
    n = 128
    yy, xx = (np.mgrid[0:n, 0:n] - (n - 1) / 2) / (n / 2)
    rr = np.hypot(xx, yy)
    ang = np.arctan2(yy, xx)
    rays = np.zeros_like(rr)
    for k in range(9):
        a = 2 * np.pi * k / 9 + rng.uniform(-0.25, 0.25)
        length = rng.uniform(0.55, 1.0)
        width = rng.uniform(0.05, 0.11)
        d = np.abs(((ang - a + np.pi) % (2 * np.pi)) - np.pi)
        rays = np.maximum(rays, np.clip(1 - d / width, 0, 1) * np.clip(1 - rr / length, 0, 1) ** 1.3)
    glow = np.clip(1 - rr, 0, 1) ** 2.2
    star = np.clip(0.85 * rays + 0.75 * glow, 0, 1)
    star_core = np.clip(np.exp(-(rr / 0.2) ** 2) * 1.1, 0, 1)
    w, h = 256, 64
    v, u = np.mgrid[0:h, 0:w]
    u = u / (w - 1)
    v = (v - (h - 1) / 2) / (h / 2)
    flick = np.asarray(Image.fromarray((rng.random((8, 32)) * 255).astype(np.uint8)).resize((w, h), Image.BICUBIC)) / 255
    # a short fat flame: opens quickly from the muzzle, tapers to a ragged tip
    half = (0.22 + 0.7 * np.clip(u / 0.3, 0, 1) ** 0.5) * (1 - u) ** 0.9 * (0.75 + 0.5 * flick)
    body = np.clip(1 - np.abs(v) / np.maximum(half, 1e-3), 0, 1) ** 1.4
    cone = np.clip(body * (1.0 - 0.8 * u) ** 1.2 * (0.7 + 0.45 * flick) * 1.3, 0, 1)
    cone_core = np.clip(np.exp(-(v / 0.25) ** 2) * np.exp(-(u / 0.3) ** 2) * 1.2, 0, 1)
    out = {}
    for name, a in (("flash_star", star), ("flash_star_core", star_core), ("flash_cone", cone), ("flash_cone_core", cone_core)):
        out[name] = Image.fromarray((np.repeat(a[..., None], 3, 2) * 255).clip(0, 255).astype(np.uint8))
    return out


def write(out: Path):
    out.mkdir(parents=True, exist_ok=True)
    rng = np.random.default_rng(42)
    atlas = np.zeros((SIZE, SIZE, 3), np.float32)
    atlas[:] = (128, 0, 128)
    for name, (u0, v0, size) in CELLS.items():
        x, y, c = int(u0 * SIZE), int(v0 * SIZE), int(size * SIZE)
        if name in GLOWS:
            yy, xx = np.mgrid[0:c, 0:c] - (c - 1) / 2
            core = 1 - 0.35 * (np.hypot(xx, yy) / (c / 2)).clip(0, 1) ** 2
            col = np.array(GLOWS[name], np.float32)
            atlas[y:y + c, x:x + c] = (col + (255 - col) * 0.45 * core[..., None] ** 4) * (0.75 + 0.25 * core[..., None])
        elif name == "disc":
            atlas[y:y + c, x:x + c] = disc_texture()
        elif name == "label":
            atlas[y:y + c, x:x + c] = label_texture()
        else:
            atlas[y:y + c, x:x + c] = cell_texture(name, rng)
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
    # saw chain, 4 links across 64 px, repeats along s (the chain runs along s)
    w, h = 64, 16
    ch = np.zeros((h, w, 3), np.float32)
    yy, xx = np.mgrid[0:h, 0:w]
    ch[:] = (40, 42, 46)
    lx = xx % 16
    ch[(lx >= 1) & (lx <= 14) & (yy >= 4) & (yy <= 11)] = (120, 124, 132)      # side plates
    ch[((lx - 4) ** 2 + (yy - 7.5) ** 2 < 4) | ((lx - 11) ** 2 + (yy - 7.5) ** 2 < 4)] = (200, 204, 210)  # rivets
    tooth = (lx >= 8) & (lx <= 14) & (yy < 4) & (yy >= (lx - 8) * 0.6)
    ch[tooth] = (190, 194, 200)                                                 # cutter teeth on the outside
    ch[(yy > 12)] = (30, 30, 32)
    Image.fromarray(ch.clip(0, 255).astype(np.uint8)).save(out / "chain.tga")
    for name, im in flash_textures(np.random.default_rng(11)).items():
        im.save(out / f"{name}.tga")
    # the Spark Gun spark: a hot white core, ice-blue crackle round it
    n = 64
    yy, xx = (np.mgrid[0:n, 0:n] - (n - 1) / 2) / (n / 2)
    rr = np.hypot(xx, yy)
    ang = np.arctan2(yy, xx)
    rays = np.clip(np.cos(ang * 5 + 0.7) * np.cos(ang * 3 - 0.4), 0, 1) ** 3 * np.clip(1 - rr, 0, 1)
    core = np.exp(-(rr / 0.18) ** 2)
    halo = np.clip(1 - rr, 0, 1) ** 2.5
    ice = np.array(GLOWS["ice"], np.float32)
    sp = core[..., None] * 255 + (0.65 * halo + 0.8 * rays)[..., None] * ice
    Image.fromarray(sp.clip(0, 255).astype(np.uint8)).save(out / "spark.tga")
    # the Nitro Flamer pilot flame: a small blue teardrop, pointing forward (+s)
    fx = (xx + 1) / 2                          # 0 at the back, 1 at the tip
    width = 0.55 * np.sin(np.clip(fx, 0, 1) * np.pi) ** 0.7 * (1 - 0.5 * fx)
    body = np.clip(1 - np.abs(yy) / np.maximum(width, 1e-3), 0, 1) * (fx < 0.98)
    blue = np.array(GLOWS["blue"], np.float32)
    pl = body[..., None] ** 1.5 * (blue * 0.9 + (255 - blue) * np.clip(1 - fx * 2.2, 0, 1)[..., None] * 0.7)
    Image.fromarray(pl.clip(0, 255).astype(np.uint8)).save(out / "pilot.tga")
