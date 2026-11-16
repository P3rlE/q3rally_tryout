"""Q3Rally: shared helpers and presets for the hand-made surface maps
(normal / normal+height for parallax / specular) next to map textures.

    import surface_maps_common as sm
    sm.setup(src_baseq3r, out_baseq3r, preview_dir=None)
    sm.relief("q3r_building/bricks02", depth=2.0)

Conventions match renderergl2 (R_GenerateSurfaceMaps, the auto lookup of
<image>_nh / _n / _s): normal x = left minus right, y = top minus bottom
of the height; _nh alpha = height (1 = top); _s RGB = linear reflectance,
A = gloss. The parallax alpha range is squeezed so that the parallax depth
(r_baseParallax 0.05 of the texture width) equals the relief in texels.

Presets (all derived from the image):
  flat         calm unevenness: concrete, plaster, tar (normal)
  grass        soft and matt (normal)
  relief       dark joints low, faces high: stone, tiles, gravel (parallax)
  sat_relief   saturated parts high, grey parts low: red bricks in light
               mortar (parallax)
  panel        thin dark lines and dots recessed: drawn panels (parallax)
  facade       building fronts with cleanly found windows (parallax)
  facade_flat  building fronts, fine relief only (normal)
  louvre       horizontal slats from the row shading (parallax)
  rust_metal   painted / bare metal, rust a little raised and rough (normal)
  tread_plate  diamond plate: lattice found and averaged, ovals as domes (parallax)
  tread_soft   faint tread plate: normal from the averaged pattern only
  window_grid  light frames over dark glass (parallax)
  corrugated   corrugated sheet / barrel tiles: wave integrated from the painted
               shading along the dominant axis (parallax); cormetal = rows
"""
import os
import numpy as np
import cv2
from PIL import Image

SRC = OUT = None
prev_dir = None
BASE_PARALLAX = 0.05
PAD = 32


def setup(src, out, preview=None):
    global SRC, OUT, prev_dir
    SRC, OUT, prev_dir = src, out, preview


def path_of(rel):
    """texture names are given without "textures/"; model skins as models/..."""
    return rel if rel.startswith(("models/", "gfx/")) else "textures/" + rel


def load(rel):
    for ext in (".jpg", ".tga", ".png"):
        p = os.path.join(SRC, path_of(rel) + ext)
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
    out = os.path.join(OUT, path_of(rel))
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




def sat_relief(rel, depth=2.0, refl=0.035, gloss=0.2):
    """bricks whose mortar is lighter and greyer than the brick"""
    rgb = load(rel)
    l = lum(rgb)
    s = blur(sat(rgb), 0.8)
    s = (s - np.percentile(s, 5)) / (np.percentile(s, 95) - np.percentile(s, 5) + 1e-6)
    h = np.clip(s, 0, 1)
    h = 0.15 + 0.8 * blur(np.clip((h - 0.25) / 0.35, 0, 1), 0.7) + 0.05 * bp(l, 0.5, 1.5)
    write(rel, np.clip(h, 0, 1), depth * max(l.shape) / 256.0, refl, gloss, True)


def shading_profile(l, axis, harmonics=3, light=1.0):
    """height along one axis from painted shading. Light from the top (axis 0)
    or the left (axis 1): brightness ~ +dh/d(axis) with y running down, so
    the height is the integral of the brightness. Only the dominant
    frequency (whole cycles across the texture, so it tiles) and its first
    harmonics are kept."""
    prof = l.mean(1 - axis)
    n = len(prof)
    F = np.fft.rfft(prof - prof.mean())
    k = int(np.argmax(np.abs(F[1:])) + 1)
    keep = np.zeros_like(F)
    H = np.zeros_like(F)
    for m in range(1, harmonics + 1):
        j = k * m
        if j < len(F):
            H[j] = light * F[j] / (2j * np.pi * j / n)          # integrate: divide by i*omega
    h = np.fft.irfft(H, n)
    return norm01(h), n / k


def corrugated(rel, axis=None, depth=2.0, cross=0.0, refl=0.12, gloss=0.45):
    """corrugated sheet / barrel tiles: the wave follows the axis with the
    stronger periodic shading (0 = rows, 1 = columns) unless given; cross
    adds the weaker axis (tile rows overlapping) with that weight"""
    rgb = load(rel)
    l = lum(rgb)
    H, W = l.shape
    if axis is None:
        e = [np.abs(np.fft.rfft(l.mean(1 - a) - l.mean()))[1:].max() for a in (0, 1)]
        axis = int(np.argmax(e))
    p, period = shading_profile(l, axis)
    h = p[:, None].repeat(W, 1) if axis == 0 else p[None, :].repeat(H, 0)
    if cross:
        q, _ = shading_profile(l, 1 - axis, 2)
        h = (1 - cross) * h + cross * (q[:, None].repeat(W, 1) if axis == 1 else q[None, :].repeat(H, 0))
    dent = blur(l, 0.8) - blur(l, 3.0)
    dent = dent - blur(dent, 4.0)
    h = np.clip(0.1 + 0.85 * norm01(h) + 0.06 * dent / (np.abs(dent).max() + 1e-6), 0, 1)
    rust = blur(np.clip((sat(rgb) - 0.1) / 0.2, 0, 1), 1.0)
    write(rel, h, depth * max(H, W) / 128.0 if max(H, W) > 128 else depth,
          refl - 0.07 * rust * (refl > 0.06), gloss - 0.3 * rust * (gloss > 0.3), True)
    return axis, period


def cormetal(rel):
    """the small corrugated sheets (128 x 64): waves across the rows"""
    return corrugated(rel, axis=0, depth=2.0)


def rust_metal(rel, depth=1.2, refl=0.1, gloss=0.4):
    """painted or bare metal with rust: rust patches a little raised and
    rough, paint / bare metal smooth (normal)"""
    rgb = load(rel)
    l = lum(rgb)
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    rust = blur(np.clip(((r - b) - 0.12) / 0.2, 0, 1) * np.clip((0.55 - l) / 0.3, 0, 1), 1.0)
    h = 0.45 + 0.2 * rust + 0.15 * bp(l, 0.6, 2.0) + 0.06 * bp(l, 2.0, 10.0)
    write(rel, np.clip(h, 0, 1), depth * max(l.shape) / 256.0, refl - 0.05 * rust, gloss - 0.28 * rust, False)


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
            acc += cv2.remap(a.astype(np.float32), mx, my, cv2.INTER_LINEAR, borderMode=cv2.BORDER_WRAP)
    return acc / (n * n)


def lattice_count(l, lo=4, hi=40):
    """repeats across the width of a periodic pattern: the n whose period
    W/n gives the strongest autocorrelation of the local contrast"""
    H, W = l.shape
    c = blur(np.abs(l - blur(l, 2.5)), 1.0)
    c = c - c.mean()
    F = np.fft.fft2(c)
    ac = np.real(np.fft.ifft2(F * np.conj(F)))
    ac /= ac[0, 0]
    best, bn = -1, None
    for n in range(lo, hi + 1):
        P = W / n
        if P < 5:
            break
        # both axes, interpolated at the fractional period
        x0 = int(P); f = P - x0
        v = (1 - f) * ac[0, x0 % W] + f * ac[0, (x0 + 1) % W] + (1 - f) * ac[x0 % H, 0] + f * ac[(x0 + 1) % H, 0]
        if v > best:
            best, bn = v, n
    return bn


def lattice_best(l, lo=4, hi=32):
    """repeats across the width: the pattern only adds up at its true period
    and at its divisors, so take the largest n whose lattice average keeps
    nearly all of the best contrast"""
    hp = l - blur(l, 6.0)                      # large brightness changes would win otherwise
    W = l.shape[1]
    sd = {k: float(lattice_avg(hp, k).std()) for k in range(lo, hi + 1) if W / k >= 5}
    best = max(sd.values())
    return max(k for k, v in sd.items() if v >= 0.85 * best)


def tread_plate(rel, n=None, depth=2.0, refl=0.08, gloss=0.3, template_from=None):
    """diamond / tread plate: the pattern averaged over its lattice; every
    texel clearly off the flat plate grey is part of an oval, holes filled,
    each oval a dome (parallax). template_from: take the ovals from another
    texture of the same plate (paint over it hides them)"""
    l = lum(load(template_from or rel))
    if n is None:
        n = lattice_best(l)
    t = lattice_avg(l, n)
    dev = blur(np.abs(t - np.median(t)), 0.7)
    m = (dev > np.percentile(dev, 70)).astype(np.uint8)
    m = cv2.morphologyEx(wpad(m), cv2.MORPH_CLOSE, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3)))
    inv = (1 - m).astype(np.uint8)
    k, lab, st, _ = cv2.connectedComponentsWithStats(inv, 4)
    small = st[:, cv2.CC_STAT_AREA] < 40
    m[(inv == 1) & small[lab]] = 1
    k, lab, st, _ = cv2.connectedComponentsWithStats(m, 8)
    tiny = st[:, cv2.CC_STAT_AREA] < 8
    tiny[0] = False
    m[tiny[lab]] = 0
    m = m[PAD:-PAD, PAD:-PAD].astype(np.float32)
    d = cv2.distanceTransform(wpad((m > 0.5).astype(np.uint8)), cv2.DIST_L2, 5)[PAD:-PAD, PAD:-PAD]
    dome = blur(1 - (1 - np.clip(d / 2.0, 0, 1)) ** 2, 0.7)
    l2 = lum(load(rel))
    h = 0.12 + 0.83 * dome + 0.05 * bp(l2, 0.6, 2.0)
    write(rel, np.clip(h, 0, 1), depth, refl, gloss + 0.2 * dome, True)
    return n


def window_grid(rel, depth=3.0):
    """window with light frames over dark glass: frames high, panes low and glossy"""
    l = lum(load(rel))
    t = blur(l, 0.8)
    thr = np.percentile(t, 55)
    frame = blur(np.clip((t - thr) / 0.08, 0, 1), 0.6)
    h = 0.2 + 0.75 * frame + 0.05 * bp(l, 0.6, 2.0)
    write(rel, np.clip(h, 0, 1), depth * max(l.shape) / 256.0, 0.04 + 0.04 * (1 - frame), 0.3 + 0.4 * (1 - frame), True)


def tread_soft(rel, n=None, depth=1.2, refl=0.1, gloss=0.35):
    """faint, noisy tread plate where the ovals can't be cut out cleanly:
    normal map from the lattice-averaged (noise free) pattern, no parallax"""
    l = lum(load(rel))
    if n is None:
        n = lattice_best(l)
    t = lattice_avg(l, n)
    t = np.minimum(t, np.percentile(t, 99))
    h = 0.5 + 0.3 * bp(t, 0.8, 4.0) + 0.08 * bp(l, 0.6, 2.0)
    write(rel, np.clip(h, 0, 1), depth, refl, gloss, False)
    return n
