"""Q3Rally: hand-made normal / normal+height (parallax) and specular maps
for the textures of q3r_citydom (those not done earlier: cobbles01,
asph02*, q3r_road/asphalt, concrete02 have their own scripts).

Presets, all derived from the image:
  facade  photographed building fronts whose windows are found cleanly
          (areas enclosed by dark frames, brick-sized cells and odd shapes
          rejected): wall high, frame middle, pane low and glossy (parallax)
  facade_flat  fronts where that test also catches pillars or brickwork:
          fine relief only, no window depth (normal)
  relief  bricks, stone tiles, gravel, metal floor plates: dark joints low,
          faces high (parallax)
  panel   the drawn citydom panels (alu, deck, soffit, louvre, parapet,
          concrete): thin dark lines are seams, dark dots are tie holes,
          both recessed (parallax)
  flat    road concrete, plaster, stucco, tar roof: calm unevenness (normal)
  grass   lawns: soft and matt (normal)

Conventions match renderergl2: normal x = left minus right, y = top minus
bottom of the height; _nh alpha = height (1 = top); _s RGB = linear
reflectance, A = gloss. The parallax alpha range is squeezed so that the
parallax depth (r_baseParallax 0.05 of the texture width) equals DEPTH.

usage: python citydom_surface_maps.py <baseq3r> <out baseq3r> [--preview <dir>]
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


def bp(l, a, b):
    g = blur(l, a) - blur(l, b)
    return g / (np.abs(g).max() + 1e-6)


def local_std(l, s):
    m = blur(l, s)
    return np.sqrt(np.clip(blur(l * l, s) - m * m, 0, None))


def window_panes(l):
    """panes: areas fully enclosed by clearly dark lines (frames, mullions),
    bigger than a brick, compact and roughly rectangular."""
    H, W = l.shape
    sc = max(W, H) / 256.0
    loc = l - blur(l, 6.0 * sc)
    dark = (loc < -0.08).astype(np.uint8)
    dark = cv2.morphologyEx(dark, cv2.MORPH_CLOSE, np.ones((3, 3), np.uint8))
    n, lab, st, _ = cv2.connectedComponentsWithStats(1 - dark, 4)
    keep = np.zeros(n, bool)
    for i in range(1, n):
        a, w, h = st[i, cv2.CC_STAT_AREA], st[i, cv2.CC_STAT_WIDTH], st[i, cv2.CC_STAT_HEIGHT]
        if a < (7 * sc) ** 2 or a > (45 * sc) ** 2 or a / (w * h) < 0.6:
            continue
        if w >= W - 2 or h >= H - 2 or max(w, h) / max(1, min(w, h)) > 6:
            continue
        keep[i] = True
    return keep[lab].astype(np.float32), dark.astype(np.float32)


def facade(rel, depth=5.0):
    """facade whose windows are found cleanly: recessed windows (parallax)"""
    rgb = load(rel)
    l = lum(rgb)
    H, W = l.shape
    sc = max(W, H) / 256.0
    pane, dark = window_panes(l)
    # the frame: dark lines next to a pane
    frame = dark * (cv2.dilate(pane, np.ones((5, 5), np.uint8)) > 0)
    fine = bp(l, 0.6, 2.0)
    wall = 0.92 + 0.06 * fine
    h = wall * (1 - frame) * (1 - pane) + 0.5 * frame + 0.2 * pane
    h = blur(h, 0.7)
    write(rel, np.clip(h, 0, 1), depth * sc, 0.04 + 0.04 * pane, 0.22 + 0.5 * pane, True)


def facade_flat(rel, depth=1.5):
    """facade where windows can't be told from the wall reliably: fine
    relief only (bricks, ledge and frame edges), no window depth"""
    l = lum(load(rel))
    H, W = l.shape
    sc = max(W, H) / 256.0
    h = 0.5 + 0.18 * bp(l, 0.6, 2.0) + 0.12 * bp(l, 1.5 * sc, 6.0 * sc)
    write(rel, np.clip(h, 0, 1), depth * sc, 0.04, 0.22, False)


def relief(rel, depth=2.5, refl=0.035, gloss=0.2):
    l = lum(load(rel))
    H, W = l.shape
    sc = max(W, H) / 256.0
    broad = bp(l, 0.9 * sc, 6.0 * sc)
    fine = bp(l, 0.5, 1.5)
    joint = np.clip((blur(l, 4.0 * sc) - blur(l, 0.8 * sc)) / 0.08, 0, 1)   # thin, darker than around
    h = 0.55 + 0.35 * broad + 0.08 * fine - 0.4 * joint
    h = norm01(blur(h, 0.6))
    write(rel, h, depth * sc, refl, gloss + 0.1 * (h - 0.5), True)


def panel(rel, depth=1.5, refl=0.05, gloss=0.4, grain=0.04, line_contrast=0.06):
    l = lum(load(rel))
    lines = np.clip((blur(l, 3.0) - blur(l, 0.6) - 0.5 * line_contrast) / line_contrast, 0, 1)   # seams, slots, tie holes
    lines = blur(lines, 0.5)
    h = 0.9 - 0.75 * lines + grain * bp(l, 0.6, 2.5)
    write(rel, np.clip(h, 0, 1), depth, refl, gloss - 0.15 * lines, True)


def flat(rel, depth=1.0, gloss=0.2, refl=0.035):
    l = lum(load(rel))
    h = 0.5 + 0.18 * bp(l, 0.7, 3.0) + 0.1 * bp(l, 3.0, 14.0)
    write(rel, np.clip(h, 0, 1), depth, refl, gloss + 0.1 * (h - 0.5), False)


def grass(rel):
    l = lum(load(rel))
    h = 0.5 + 0.2 * bp(l, 0.6, 2.0) + 0.1 * bp(l, 2.0, 8.0)
    write(rel, np.clip(h, 0, 1), 0.8, 0.03, 0.12, False)


def louvre(rel):
    # horizontal slats: the row profile of the drawn shading, integrated
    l = lum(load(rel))
    prof = blur(l, 1.0).mean(1)
    slope = prof - blur(prof[:, None].repeat(4, 1), 6.0)[:, 0]
    wave = -np.cumsum(slope - slope.mean())
    wave = norm01(wave - np.linspace(wave[0], wave[-1], len(wave)))
    h = np.repeat(wave[:, None], l.shape[1], 1)
    write(rel, h, 2.0, 0.08, 0.45, True)


B = "q3r_building/"
JOBS = [
    (flat, "q3r_road/concrete", dict(depth=1.2, gloss=0.22)),
    (flat, B + "concrete03", {}),
    (flat, B + "concrete03a", {}),
    (flat, B + "concrete01", dict(depth=1.4)),
    (flat, B + "stucco01", dict(depth=1.4, gloss=0.15)),
    (flat, B + "roof", dict(depth=0.8, gloss=0.25)),
    (grass, "q3r_dirt_grass/greengrass", {}),
    (grass, "q3r_dirt_grass/grass04", {}),
    (relief, B + "bricks02", dict(depth=2.0)),
    (relief, B + "bricks03", dict(depth=2.0)),
    (relief, B + "bricks06", dict(depth=2.0)),
    (relief, "q3r_ground/pebble01", dict(depth=3.0, gloss=0.18)),
    (relief, "q3r_floor/trim18", dict(depth=2.5)),
    (relief, "gothic_floor/q1metal7_98d_256x256", dict(depth=1.5, refl=0.1, gloss=0.35)),
    (relief, "gothic_floor/q1metal7_99", dict(depth=1.5, refl=0.1, gloss=0.35)),
    (relief, B + "plate01", dict(depth=1.2, refl=0.12, gloss=0.4)),
    (panel, "q3r_citydom/alu", dict(depth=1.2, refl=0.15, gloss=0.55, grain=0.02, line_contrast=0.12)),   # brushed streaks stay shallow
    (panel, "q3r_citydom/deck", dict(depth=1.5, gloss=0.3)),
    (panel, "q3r_citydom/soffit", dict(depth=1.5, gloss=0.3)),
    (panel, "q3r_citydom/parapet", dict(depth=1.5, gloss=0.25)),
    (panel, "q3r_citydom/concrete", dict(depth=1.5, gloss=0.25)),
    (louvre, "q3r_citydom/louvre", {}),
]
for name in ("externalwall52a", "externalwall51a", "externalwall91_512x512", "externalwall75_256x256",
             "externalwall68a_"):
    JOBS.append((facade, B + name, {}))
for name in ("externalwall56a", "externalwall50a", "externalwall33-2a", "externalwall33-1a", "externalwall72_",
             "externalwall46b_a", "externalwall87_128x256", "externalwall85_128x256", "skyr06m"):
    JOBS.append((facade_flat, B + name, {}))
JOBS.append((panel, B + "garage_np", dict(depth=2.0, refl=0.06, gloss=0.35)))
JOBS.append((panel, B + "windows8", dict(depth=2.0, refl=0.06, gloss=0.5)))

for fn, rel, kw in JOBS:
    fn(rel, **kw)
