"""Q3Rally: hand-made normal / normal+height (parallax) and specular maps
for the textures of q3r_battlecanyon.

  Stein3          canyon rock: broad shapes from the image, cracks deep (parallax)
  clang_floor,    tread plate: the drawn ovals (light side and shadow rim
  achtung_clang   together) found by local contrast and raised (parallax);
                  painted stripes stay flat
  tin_floor04ach  grey tread ovals on orange / black stripes, found by
                  saturation (parallax)
  tilefloor7      four floor plates, seams at 0 and 64 (parallax)
  panel02         riveted panel, relief from the image (normal)
  cormetal03/04   corrugated sheet: the painted shading of one 8-row wave is
                  averaged and integrated into its profile (parallax)

Conventions match renderergl2: normal x = left minus right, y = top minus
bottom of the height; _nh alpha = height (1 = top); _s RGB = linear
reflectance, A = gloss. The parallax alpha range is squeezed so that the
parallax depth (r_baseParallax 0.05 of the texture width) equals DEPTH.

usage: python battlecanyon_surface_maps.py <baseq3r> <out baseq3r> [--preview <dir>]
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


def stein3():
    rel = "q3r_stone/Stein3"
    l = lum(load(rel))
    broad = blur(l, 2.0) - blur(l, 28.0)
    broad /= np.abs(broad).max() + 1e-6
    crack = np.clip((blur(l, 5.0) - blur(l, 0.9)) / 0.12, 0, 1)      # thin and darker than around
    fine = blur(l, 0.6) - blur(l, 2.0)
    fine /= np.abs(fine).max() + 1e-6
    h = 0.55 + 0.45 * broad - 0.45 * crack + 0.08 * fine
    h = np.clip(blur(h, 0.7), 0, 1)
    write(rel, norm01(h), 8.0, 0.03, 0.12 + 0.08 * h, True)


def lattice_avg(a, n):
    """average a periodic pattern over its n x n lattice (period W/n, may be
    fractional): noise and paint fall out, every repeat gets the same shape"""
    H, W = a.shape
    P = W / n
    yy, xx = np.mgrid[0:H, 0:W].astype(np.float32)
    acc = np.zeros_like(a)
    for i in range(n):
        for j in range(n):
            mx = ((xx + i * P) % W).astype(np.float32)
            my = ((yy + j * P) % H).astype(np.float32)
            acc += cv2.remap(a, mx, my, cv2.INTER_LINEAR, borderMode=cv2.BORDER_WRAP)
    return acc / (n * n)


def dome_from(mask, radius):
    m = (mask > 0.5).astype(np.uint8)
    dist = cv2.distanceTransform(wpad(m), cv2.DIST_L2, 5)[PAD:-PAD, PAD:-PAD]
    d = np.clip(dist / radius, 0, 1)
    return blur(1 - (1 - d) ** 2, 0.7)


def tread_template():
    # clang_floor averaged over its 12 x 12 lattice (period 256/12) is a clean
    # diamond plate: alternating ovals, each drawn with a highlight and a
    # shadow. Every texel that differs clearly from the flat plate grey is
    # part of an oval; holes are filled and the oval becomes a dome.
    # achtung_clang is the same plate (correlation 0.98 at zero shift).
    l = lum(load("base_floor/clang_floor"))
    t = lattice_avg(l, 12)
    dev = blur(np.abs(t - np.median(t)), 0.7)
    m = (dev > np.percentile(dev, 70)).astype(np.uint8)
    m = cv2.morphologyEx(wpad(m), cv2.MORPH_CLOSE, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3)))
    inv = (1 - m).astype(np.uint8)
    n, lab, stats, _ = cv2.connectedComponentsWithStats(inv, 4)
    small = stats[:, cv2.CC_STAT_AREA] < 40                        # holes inside an oval
    m[(inv == 1) & small[lab]] = 1
    n, lab, stats, _ = cv2.connectedComponentsWithStats(m, 8)
    tiny = stats[:, cv2.CC_STAT_AREA] < 8
    tiny[0] = False
    m[tiny[lab]] = 0
    m = m[PAD:-PAD, PAD:-PAD].astype(np.float32)
    return dome_from(m, 2.0)


TREAD = None


def tread(rel, depth=2.0):
    global TREAD
    if TREAD is None:
        TREAD = tread_template()
    l = lum(load(rel))
    grain = blur(l, 0.6) - blur(l, 2.0)
    h = 0.12 + 0.83 * TREAD + 0.05 * grain / (np.abs(grain).max() + 1e-6)
    write(rel, np.clip(h, 0, 1), depth, 0.08, 0.25 + 0.25 * TREAD, True)


def tin_floor():
    # grey ovals on orange / black stripes, 20 per 512 texels; the local
    # contrast averaged over the lattice gives the whole oval (lit and shaded half)
    rel = "proto2/tin_floor04ach"
    l = lum(load(rel))
    c = blur(np.abs(l - blur(l, 2.5)), 1.0)
    t = lattice_avg(c, 20)
    t = blur(t, 0.6)
    dome = dome_from((t > np.percentile(t, 80)).astype(np.float32), 2.5)
    h = 0.12 + 0.88 * dome
    write(rel, np.clip(h, 0, 1), 3.0, 0.06 + 0.04 * dome, 0.25 + 0.3 * dome, True)


def tilefloor7():
    rel = "base_floor/tilefloor7"
    l = lum(load(rel))
    yy, xx = np.mgrid[0:128, 0:128].astype(np.float32)
    dx = np.abs(((xx + 0.5) % 64) - 0.0)
    dx = np.minimum(dx, 64 - dx)
    dy = np.abs(((yy + 0.5) % 64) - 0.0)
    dy = np.minimum(dy, 64 - dy)
    seam = groove(np.minimum(dx, dy), 0.5, 1.5)
    fine = blur(l, 0.6) - blur(l, 3.0)
    h = 0.2 + 0.75 * seam + 0.05 * fine / (np.abs(fine).max() + 1e-6)
    write(rel, np.clip(h, 0, 1), 2.0, 0.04, 0.3 + 0.1 * seam, True)


def panel02():
    rel = "proto2/panel02"
    l = lum(load(rel))
    mid = blur(l, 1.0) - blur(l, 8.0)
    fine = blur(l, 0.6) - blur(l, 2.0)
    h = 0.5 + 0.8 * mid / (np.abs(mid).max() + 1e-6) + 0.2 * fine / (np.abs(fine).max() + 1e-6)
    write(rel, np.clip(h, 0, 1), 1.5, 0.1, 0.35, False)


def cormetal(rel):
    rgb = load(rel)
    l = lum(rgb)
    H, W = l.shape
    period = 8
    prof = l.mean(1).reshape(-1, period).mean(0)          # one wave, averaged
    slope = prof - prof.mean()                            # lit from above: brightness ~ -dh/dy
    wave = -np.cumsum(slope)
    wave = np.roll(wave, 1)
    wave -= wave.mean()
    # smooth periodic fit: first two harmonics
    k = np.arange(period)
    F = np.fft.rfft(wave)
    F[3:] = 0
    wave = np.fft.irfft(F, period)
    wave = (wave - wave.min()) / (wave.max() - wave.min() + 1e-9)
    yy = np.arange(H) % period
    xs = np.linspace(0, period, 1)                        # unused, keeps shape clear
    # sample the wave at sub-texel positions for a smooth profile
    fine_y = np.arange(H, dtype=np.float32)
    h_rows = np.interp(fine_y % period, np.arange(period + 1), np.append(wave, wave[0]))
    h = np.repeat(h_rows[:, None], W, 1)
    dent = blur(l, 0.8) - blur(l, 3.0)
    dent = dent - blur(dent, 4.0)
    h = np.clip(0.1 + 0.85 * h + 0.06 * dent / (np.abs(dent).max() + 1e-6), 0, 1)
    rust = blur(np.clip((sat(rgb) - 0.1) / 0.2, 0, 1), 1.0)
    write(rel, h, 2.0, 0.12 - 0.07 * rust, 0.45 - 0.3 * rust, True)


stein3()
tread("base_floor/clang_floor")
tread("base_floor/achtung_clang")
tin_floor()
tilefloor7()
panel02()
cormetal("q3r_metals/cormetal03")
cormetal("q3r_metals/cormetal04")
