"""Q3Rally: hand-made normal / normal+height (parallax) and specular maps
for the textures of q3r_canyonrush.

The ground is q3map2 terrain (ter_<layer>, ter_<a>to<b>); the renderer
collapses every texture stage that finds the lightmap into its lit path and
looks up <image>_n / _nh / _s per stage, so the maps work on the blended
terrain too.

  dip_valley/sand        sand with crusted pebble patches (normal)
  q3r_dirt_grass/rock01  broken rock ground, shadows are cavities (parallax)
  q3r_canyonrush/strata  sandstone layers rebuilt exactly as make_canyonrush.py
                         draws them (random.Random(11)); each layer gets its
                         own hardness: hard ones stand out, soft ones are
                         weathered back (parallax)
  q3r_dirt_grass/dirt01  fine dirt (normal)
  q3r_dirt_grass/stones02  gravel, stones raised (parallax)
  q3r_dirt_grass/checker01 start line paint on asphalt (normal)
  base_trim/tin          mottled sheet metal (normal)

Conventions match renderergl2: normal x = left minus right, y = top minus
bottom of the height; _nh alpha = height (1 = top); _s RGB = linear
reflectance, A = gloss. The parallax alpha range is squeezed so that the
parallax depth (r_baseParallax 0.05 of the texture width) equals DEPTH.

usage: python canyonrush_surface_maps.py <baseq3r> <out baseq3r> [--preview <dir>]
"""
import os
import sys
import numpy as np
import cv2
from PIL import Image

args = sys.argv[1:]
prev_dir = None
if "--preview" in args:
    i = args.index("--preview")
    prev_dir = args[i + 1]
    del args[i:i + 2]
SRC, OUT = args[0], args[1]
BASE_PARALLAX = 0.05
PAD = 32


def load(rel):
    for ext in (".jpg", ".tga", ".png"):
        p = os.path.join(SRC, "textures", rel + ext)
        if os.path.exists(p):
            return np.asarray(Image.open(p).convert("RGB")).astype(np.float32) / 255.0
    raise FileNotFoundError(rel)


def wpad(a):
    return np.pad(a, ((PAD, PAD), (PAD, PAD)), mode="wrap")


def blur(a, s):
    if s <= 0:
        return a
    return cv2.GaussianBlur(wpad(a), (0, 0), s)[PAD:-PAD, PAD:-PAD]


def lum(rgb):
    return rgb @ np.array([0.299, 0.587, 0.114], np.float32)


def norm01(a):
    lo, hi = a.min(), a.max()
    return (a - lo) / (hi - lo + 1e-9)


def noise(h, w, cells, seed):
    """tileable value noise, same shape as make_rooftop.noise"""
    rng = np.random.default_rng(seed)
    g = rng.random((cells + 1, cells + 1))
    g[-1, :] = g[0, :]
    g[:, -1] = g[:, 0]
    yy = np.linspace(0, cells, h, endpoint=False)
    xx = np.linspace(0, cells, w, endpoint=False)
    y0, x0 = yy.astype(int), xx.astype(int)
    fy, fx = (yy - y0)[:, None], (xx - x0)[None, :]
    fy, fx = fy * fy * (3 - 2 * fy), fx * fx * (3 - 2 * fx)
    a, b = g[y0][:, x0], g[y0][:, x0 + 1]
    c, d = g[y0 + 1][:, x0], g[y0 + 1][:, x0 + 1]
    return a * (1 - fx) * (1 - fy) + b * fx * (1 - fy) + c * (1 - fx) * fy + d * fx * fy


def tobyte(v):
    return np.clip(np.round(v * 255.0), 0, 255).astype(np.uint8)


def write(rel, height, depth, refl, gloss, parallax):
    """height 0..1 (1 = top), depth = relief in texels at full height."""
    H, W = height.shape
    hp = wpad(height * depth)
    dx = ((np.roll(hp, 1, 1) - np.roll(hp, -1, 1)) * 0.5)[PAD:-PAD, PAD:-PAD]
    dy = ((np.roll(hp, 1, 0) - np.roll(hp, -1, 0)) * 0.5)[PAD:-PAD, PAD:-PAD]
    nl = np.sqrt(dx * dx + dy * dy + 1.0)
    rgb = [tobyte(dx / nl * 0.5 + 0.5), tobyte(dy / nl * 0.5 + 0.5), tobyte(1.0 / nl * 0.5 + 0.5)]
    out = os.path.join(OUT, "textures", rel)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    if parallax:
        rng = min(1.0, depth / (BASE_PARALLAX * W))
        Image.fromarray(np.dstack(rgb + [tobyte(1.0 - rng * (1.0 - height))]), "RGBA").save(out + "_nh.png", optimize=True)
    else:
        Image.fromarray(np.dstack(rgb), "RGB").save(out + "_n.png", optimize=True)
    refl = np.broadcast_to(np.asarray(refl, np.float32), height.shape)
    gloss = np.broadcast_to(np.asarray(gloss, np.float32), height.shape)
    Image.fromarray(np.dstack([tobyte(refl)] * 3 + [tobyte(np.clip(gloss, 0, 1))]), "RGBA").save(out + "_s.png", optimize=True)
    print("%-28s %s  %dx%d  depth %.1f" % (rel, "_nh" if parallax else "_n ", W, H, depth))
    if prev_dir:
        diffuse = load(rel)
        nx, ny, nz = dx / nl, dy / nl, 1.0 / nl
        tiles = []
        for L in ((-1, -1, 0.8), (1, 0.4, 0.5)):
            L = np.array(L, np.float32) / np.linalg.norm(L)
            ndl = np.clip(nx * L[0] + ny * L[1] + nz * L[2], 0, 1)
            tiles.append(np.clip(diffuse * (0.3 + 0.9 * ndl[..., None]), 0, 1))
        im = Image.fromarray(tobyte(np.hstack([diffuse] + tiles)))
        s = 256 / max(W, H)
        im.resize((int(im.width * s), int(im.height * s)), Image.NEAREST).save(
            os.path.join(prev_dir, rel.replace("/", "_") + ".png"))


def groove(dist, half_width, bevel):
    """0 in a groove of the given half width, easing up to 1 over the bevel."""
    t = np.clip((dist - half_width) / bevel, 0, 1)
    return t * t * (3 - 2 * t)



import math
import random


def sat(rgb):
    return rgb.max(2) - rgb.min(2)


def bandpass(l, a, b):
    g = blur(l, a) - blur(l, b)
    return g / (np.abs(g).max() + 1e-6)


def sand():
    rel = "dip_valley/sand"
    rgb = load(rel)
    l = lum(rgb)
    grain = bandpass(l, 0.6, 2.0)
    crust = np.clip((blur(l, 1.5) - blur(l, 10.0)) / 0.08, 0, 1)     # light pebble patches
    h = 0.4 + 0.12 * grain + 0.35 * blur(crust, 0.8)
    write(rel, np.clip(h, 0, 1), 1.2, 0.03, 0.1 + 0.05 * crust, False)


def rock01():
    rel = "q3r_dirt_grass/rock01"
    l = lum(load(rel))
    broad = bandpass(l, 1.5, 14.0)
    cav = np.clip((0.12 - blur(l, 1.2)) / 0.12, 0, 1)               # deep shadows are holes
    fine = bandpass(l, 0.6, 2.0)
    h = 0.55 + 0.4 * broad - 0.35 * cav + 0.08 * fine
    write(rel, norm01(blur(h, 0.6)), 4.0, 0.03, 0.12 + 0.08 * (h - 0.5), True)


def strata():
    # the band layout of make_canyonrush.write_textures, same random sequence
    rel = "q3r_canyonrush/strata"
    rgb = load(rel)
    size = rgb.shape[0]
    rnd = random.Random(11)
    bands, pos = [], 0
    while pos < size:
        w = rnd.choice((6, 10, 14, 22, 30, 44))
        bands.append((pos, min(size, pos + w), rnd.randrange(6)))
        pos += w
    band_of = np.zeros(size, int)
    edge_of = np.zeros(size, np.float32)          # distance to the band's nearer edge
    for a, b, c in bands:
        band_of[a:b] = c
        for y in range(a, b):
            edge_of[y] = min(y - a + 0.5, b - y - 0.5)
    # hardness per palette colour: the light, sandy layers are the hard ones
    hard = np.array([0.45, 0.7, 0.3, 1.0, 0.2, 0.8], np.float32)
    xs = np.arange(size)
    wave = 6 * np.sin(2 * np.pi * xs / size * 2) + 3 * np.sin(2 * np.pi * xs / size * 7 + 1.3)
    yy = (np.arange(size)[:, None] + wave[None, :]).astype(int) % size
    level = hard[band_of[yy]]
    # rounded edges: a layer stands out fully a few texels in from its edges
    e = np.clip(edge_of[yy] / 3.0, 0, 1)
    level = level * (0.55 + 0.45 * (1 - (1 - e) ** 2))
    level = blur(level, 0.8)
    grain = bandpass(lum(rgb), 0.6, 2.0)
    h = 0.15 + 0.75 * level + 0.06 * grain
    write(rel, np.clip(h, 0, 1), 6.0, 0.03, 0.12, True)


def dirt01():
    rel = "q3r_dirt_grass/dirt01"
    l = lum(load(rel))
    h = 0.5 + 0.2 * bandpass(l, 0.6, 2.5) + 0.12 * bandpass(l, 2.5, 12.0)
    write(rel, np.clip(h, 0, 1), 1.2, 0.03, 0.14, False)


def stones02():
    rel = "q3r_dirt_grass/stones02"
    l = lum(load(rel))
    stones = np.clip((blur(l, 0.8) - blur(l, 6.0)) / 0.1, -1, 1)
    h = 0.45 + 0.45 * stones + 0.08 * bandpass(l, 0.5, 1.5)
    h = blur(h, 0.6)
    write(rel, np.clip(h, 0, 1), 3.0, 0.035, 0.15 + 0.15 * np.clip(stones, 0, 1), True)


def checker01():
    rel = "q3r_dirt_grass/checker01"
    rgb = load(rel)
    l = lum(rgb)
    white = blur((blur(l, 2.0) > 0.4).astype(np.float32), 0.8)
    grain = bandpass(l - blur(l, 3.0), 0.6, 2.0)
    h = 0.5 + 0.18 * grain * (1 - 0.6 * white) + 0.06 * white
    write(rel, np.clip(h, 0, 1), 1.0, 0.035 + 0.02 * white, 0.3 + 0.2 * white, False)


def tin():
    rel = "base_trim/tin"
    l = lum(load(rel))
    h = 0.5 + 0.12 * bandpass(l, 0.8, 3.0) + 0.1 * bandpass(l, 3.0, 16.0)
    mott = bandpass(l, 2.0, 12.0)
    write(rel, np.clip(h, 0, 1), 1.0, 0.1, 0.4 + 0.15 * mott, False)


sand()
rock01()
strata()
dirt01()
stones02()
checker01()
tin()
