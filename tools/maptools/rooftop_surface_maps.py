"""Q3Rally: hand-made normal / normal+height (parallax) and specular maps
for the lit textures of q3r_rooftop.

The rooftop textures are procedural (make_rooftop.py), so their relief is
built from the same layout: panel seams every 64 / 128 texels, louvres
every 8 rows, a 6-texel grid with a fan ring, a door gap and handle, paint
chevrons. The shared textures (roof01 tiles, concrete02, plate01 rivets)
are derived from the image.

The backdrop shaders (streets, win_a/b/c, signs, ledscreen) are unlit
(nolightmap, rgbGen identity), so normal and parallax maps have no effect
there and none are made.

Conventions match renderergl2: normal x = left minus right, y = top minus
bottom of the height; _nh alpha = height (1 = top); _s RGB = linear
reflectance, A = gloss. The parallax alpha range is squeezed so that the
parallax depth (r_baseParallax 0.05 of the texture width) equals DEPTH.

usage: python rooftop_surface_maps.py <baseq3r> <out baseq3r> [--preview <dir>]
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


# ---------------------------------------------------------------- rooftop

def pitch_base():
    # rubber sports surface: fine granulate, no larger shapes
    l = lum(load("q3r_rooftop/pitch_base"))
    g = blur(l, 0.8) - blur(l, 4.0)
    h = norm01(np.clip(g / (g.std() * 3), -1, 1))
    write("q3r_rooftop/pitch_base", h, 0.9, 0.03, 0.18 + 0.1 * h, False)


def fillet():
    # dark rubber with chevrons of reflective paint, paint slightly raised
    yy, xx = np.mgrid[0:128, 0:128]
    chev = (((xx + yy) // 16) % 2).astype(np.float32)
    paint = blur(chev, 0.6)
    grain = noise(128, 128, 32, 41) * 0.25 + noise(128, 128, 64, 42) * 0.15
    h = 0.35 * paint + grain * (1 - paint) + 0.05 * grain * paint
    write("q3r_rooftop/fillet", norm01(h), 0.8, 0.03 + 0.03 * paint, 0.18 + 0.4 * paint, False)


def facade(team):
    # painted metal panels: recessed seams (rows every 64, columns every 128)
    yy, xx = np.mgrid[0:256, 0:256].astype(np.float32)
    dy = np.minimum(yy % 64, 64 - yy % 64)
    dx = np.minimum(xx % 128, 128 - xx % 128)
    d = np.minimum(dx, dy)
    h = groove(d, 0.6, 2.5)
    # panels pillow a little towards their centre
    pil = np.sin(np.pi * (yy % 64) / 64) * np.sin(np.pi * (xx % 128) / 128)
    h = h * (0.85 + 0.15 * pil)
    write(f"q3r_rooftop/facade_{team}", h, 3.0, 0.05, 0.3 + 0.25 * h, True)


def acunit():
    # louvres: each 8-row band is a slat that slopes out, gap on the dark line
    yy = np.mgrid[0:128, 0:128][0].astype(np.float32)
    ph = yy % 8
    h = np.where(ph == 0, 0.0, 0.25 + 0.75 * (ph - 1) / 6)
    h = blur(h, 0.5)
    write("q3r_rooftop/acunit", h, 2.0, 0.08, 0.35 + 0.15 * h, True)


def grille():
    # grating: grooves on the 6-texel grid, a raised fan ring
    yy, xx = np.mgrid[0:128, 0:128].astype(np.float32)
    d = np.minimum(np.minimum(yy % 6, 6 - yy % 6), np.minimum(xx % 6, 6 - xx % 6))
    bars = groove(d, 0.4, 1.0)
    r = np.hypot(yy - 64, xx - 64)
    ring = groove(3.0 - np.abs(r - 43), 0.0, 1.5)
    h = np.maximum(0.6 * bars, ring)
    write("q3r_rooftop/grille", h, 1.5, 0.1, 0.35 + 0.2 * h, True)


def door():
    # 128 x 256: gap between the leaves at x 60..68, handle at y 120..136, x 96..110
    yy, xx = np.mgrid[0:256, 0:128].astype(np.float32)
    gap = groove(np.abs(xx - 63.5), 3.5, 1.5)
    hy = np.maximum(0, np.maximum(120 - yy, yy - 135))
    hx = np.maximum(0, np.maximum(96 - xx, xx - 109))
    handle = 1.0 - groove(np.hypot(hx, hy), 0.0, 2.0)
    h = 0.5 * gap + 0.5 * handle
    write("q3r_rooftop/door", h, 2.5, 0.05 + 0.15 * handle, 0.4 + 0.3 * handle, True)


# ---------------------------------------------------------------- shared

def roof01():
    # clay tiles: bright tile faces high, the shadow under each row low
    l = lum(load("q3r_building/roof01"))
    band = blur(l, 1.0) - blur(l, 7.0)
    h = norm01(np.clip(band / (band.std() * 2.5), -1, 1))
    grain = blur(l, 0.5) - blur(l, 1.5)
    h = np.clip(h + 0.15 * grain / (grain.std() * 3), 0, 1)
    write("q3r_building/roof01", h, 3.5, 0.035, 0.2 + 0.15 * h, True)


def concrete02():
    # smooth cast concrete: faint unevenness, small pits stay low
    l = lum(load("q3r_building/concrete02"))
    fine = blur(l, 0.7) - blur(l, 3.0)
    wide = blur(l, 3.0) - blur(l, 14.0)
    h = 0.5 + 0.18 * fine / (fine.std() + 1e-6) + 0.08 * wide / (wide.std() + 1e-6)
    h = np.clip(h, 0, 1)
    write("q3r_building/concrete02", h, 1.0, 0.035, 0.22 + 0.1 * (h - 0.5), False)


def plate01():
    # riveted plates: thin dark lines are seams (low), small dark dots are rivet heads (raised)
    l = lum(load("q3r_metals/plate01"))
    dark = np.clip(-(blur(l, 0.6) - blur(l, 3.0)), 0, None)
    dark /= dark.max() + 1e-6
    m = (dark > 0.22).astype(np.uint8)
    lines_h = cv2.morphologyEx(m, cv2.MORPH_OPEN, cv2.getStructuringElement(cv2.MORPH_RECT, (9, 1)))
    lines_v = cv2.morphologyEx(m, cv2.MORPH_OPEN, cv2.getStructuringElement(cv2.MORPH_RECT, (1, 9)))
    lines = np.maximum(lines_h, lines_v).astype(np.float32)
    dots = (m.astype(np.float32) - lines).clip(0, 1)
    n, lab, stats, _ = cv2.connectedComponentsWithStats(dots.astype(np.uint8), 8)
    keep = (stats[:, cv2.CC_STAT_AREA] <= 12)
    keep[0] = False
    dots = keep[lab].astype(np.float32)
    rivet = np.clip(blur(dots, 0.9) * 3.0, 0, 1)
    seam = blur(lines, 0.6)
    h = 0.55 - 0.4 * seam + 0.45 * rivet
    h = np.clip(h + 0.05 * (blur(l, 2) - blur(l, 10)) / (l.std() + 1e-6), 0, 1)
    write("q3r_metals/plate01", h, 1.5, 0.12, 0.35 + 0.25 * rivet - 0.1 * seam, True)


for f in (pitch_base, fillet, lambda: facade("red"), lambda: facade("blue"), acunit, grille, door,
          roof01, concrete02, plate01):
    f()
