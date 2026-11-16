"""Q3Rally: hand-made normal / normal+height (parallax) and specular maps
for the textures of q3r_babel.

Road asphalt and the concrete family get calm relief from the image. The
sci-fi tech panels (q3r_tech/tex8_*, tex9_*) have their lighting painted
in, so brightness is only partly height: painted signs and rust are kept
flat (high saturation) and made rough in the specular map instead, and
the panels get normal maps only. The floor grate and the rail strip get
parallax, because their dark parts really are holes and slots.

Conventions match renderergl2: normal x = left minus right, y = top minus
bottom of the height; _nh alpha = height (1 = top); _s RGB = linear
reflectance, A = gloss. The parallax alpha range is squeezed so that the
parallax depth (r_baseParallax 0.05 of the texture width) equals DEPTH.

usage: python babel_surface_maps.py <baseq3r> <out baseq3r> [--preview <dir>]
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



def sat(rgb):
    return rgb.max(2) - rgb.min(2)


def asphalt():
    # fine dark asphalt (128 texels): grain clusters, no paint
    l = lum(load("q3r_road/asphalt")) ** 2.2
    g = blur(l, 0.7) - blur(l, 3.0)
    g /= g.std() + 1e-6
    w = blur(l, 3.0) - blur(l, 12.0)
    w /= w.std() + 1e-6
    h = np.clip(0.5 + 0.16 * g + 0.06 * w, 0, 1)
    write("q3r_road/asphalt", h, 0.7, 0.035, 0.3 + 0.1 * np.clip(g * 0.5, -1, 1), False)


def concrete(rel, depth=1.0, gloss=0.22):
    # smooth concrete: faint unevenness, small pits low
    l = lum(load(rel))
    fine = blur(l, 0.7) - blur(l, 3.0)
    wide = blur(l, 3.0) - blur(l, 14.0)
    h = 0.5 + 0.18 * fine / (fine.std() + 1e-6) + 0.08 * wide / (wide.std() + 1e-6)
    h = np.clip(h, 0, 1)
    write(rel, h, depth, 0.035, gloss + 0.1 * (h - 0.5), False)
    return h


def cretewall01():
    # wall over a set-back plinth: the step is at row 192 (highlight 191, shadow 193)
    rel = "dsi_textures/cretewall01"
    l = lum(load(rel))
    yy = np.mgrid[0:256, 0:256][0].astype(np.float32)
    step = np.clip((192.5 - yy) / 2.0, 0, 1)            # 1 above, 0 below
    step = step * step * (3 - 2 * step)
    fine = blur(l, 0.7) - blur(l, 3.0)
    fine /= fine.std() + 1e-6
    h = 0.35 + 0.55 * step + 0.04 * fine
    write(rel, np.clip(h, 0, 1), 3.0, 0.035, 0.22, True)


def tech(rel, depth=2.0, parallax=False, hole_bias=0.0):
    """painted sci-fi panel: brightness band-pass as height, saturated
    (paint, rust) areas held flat and made rough."""
    rgb = load(rel)
    l = lum(rgb)
    s = blur(np.clip((sat(rgb) - 0.12) / 0.2, 0, 1), 1.0)       # 1 = paint / rust
    mid = blur(l, 1.2) - blur(l, 10.0)
    fine = blur(l, 0.6) - blur(l, 2.0)
    h = 0.5 + 0.9 * mid / (np.abs(mid).max() + 1e-6) + 0.25 * fine / (np.abs(fine).max() + 1e-6)
    h = h * (1 - 0.8 * s) + 0.5 * 0.8 * s                         # paint stays at mid height
    if hole_bias:
        dark = np.clip((0.18 - blur(l, 0.8)) / 0.18, 0, 1)        # holes: really deep
        h = h * (1 - hole_bias * dark)
    h = np.clip(h, 0, 1)
    gloss = np.clip(0.5 - 0.35 * s + 0.1 * (h - 0.5), 0.1, 0.7)
    refl = 0.12 - 0.08 * s
    write(rel, h, depth, refl, gloss, parallax)


asphalt()
concrete("base_wall/concrete1")
for c in ("cretebase", "cretebase2", "cretebase4"):
    concrete("dsi_textures/" + c, 0.9)
cretewall01()
for t in ("tex8_1", "tex8_2", "tex8_3", "tex8_4", "tex8_7", "tex8_17"):
    tech("q3r_tech/" + t, 2.0)
tech("q3r_tech/tex9_9", 3.0, True, 0.8)        # floor grate over tread plate
tech("q3r_tech/tex9_11", 3.0, True, 0.8)       # rail strip with slots
