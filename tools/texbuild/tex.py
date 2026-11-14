"""Small toolkit for procedural, tileable game textures.

Images are float32 numpy arrays in 0..1, shape (h, w) for masks and height
maps, (h, w, 3) for colour. Everything that filters works on the torus
(FFT, np.roll), so a texture made from these parts tiles without seams.

Typical flow: build a height map from shapes (signed distance fields with a
bevel), light it with `shade`, multiply a material colour, add grime with
`cavity`, save with `save`.
"""
import os
import numpy as np
from PIL import Image

F = np.float32


# ---------------------------------------------------------------- grids and helpers
def grid(w, h):
    """Pixel-centre coordinates x, y (each shape (h, w))."""
    y, x = np.mgrid[0:h, 0:w].astype(F)
    return x + 0.5, y + 0.5


def clamp(a, lo=0.0, hi=1.0):
    return np.clip(a, lo, hi).astype(F)


def smooth(e0, e1, x):
    t = clamp((x - e0) / (e1 - e0 + 1e-9))
    return t * t * (3 - 2 * t)


def lerp(a, b, t):
    if np.ndim(t) == 2 and np.ndim(a) == 3 or np.ndim(b) == 3 and np.ndim(t) == 2:
        t = t[..., None]
    return (a + (b - a) * t).astype(F)


def norm01(a, lo=1.0, hi=99.0):
    p0, p1 = np.percentile(a, [lo, hi])
    return clamp((a - p0) / (p1 - p0 + 1e-9))


def rgb(c):
    """'#rrggbb' or (r, g, b) 0..255 -> float array (3,)."""
    if isinstance(c, str):
        c = c.lstrip('#')
        c = tuple(int(c[i:i + 2], 16) for i in (0, 2, 4))
    return np.array(c, F) / 255.0


def fill(w, h, c):
    return np.broadcast_to(rgb(c), (h, w, 3)).astype(F).copy()


def gradient_map(t, stops):
    """Map 0..1 values to colours. stops: [(pos, colour), ...]."""
    pos = np.array([s[0] for s in stops], F)
    cols = np.array([rgb(s[1]) for s in stops], F)
    out = np.empty(t.shape + (3,), F)
    for k in range(3):
        out[..., k] = np.interp(t, pos, cols[:, k])
    return out


def gray(img):
    return (img[..., 0] * 0.3 + img[..., 1] * 0.59 + img[..., 2] * 0.11).astype(F)


# ---------------------------------------------------------------- noise (periodic)
def noise(w, h, seed, fmin=1.0, fmax=None, beta=2.0, aniso=(1.0, 1.0)):
    """Periodic fractal noise by spectral synthesis.

    fmin/fmax: band in cycles per image; beta: spectral slope (2 = soft,
    1 = rough); aniso stretches frequencies (aniso=(1, 8) gives streaks
    along x, i.e. a brushed look)."""
    rng = np.random.default_rng(seed)
    wn = rng.standard_normal((h, w)).astype(F)
    fy = np.fft.fftfreq(h)[:, None] * h * aniso[1]
    fx = np.fft.fftfreq(w)[None, :] * w * aniso[0]
    f = np.sqrt(fx * fx + fy * fy)
    f[0, 0] = 1
    amp = f ** (-beta / 2.0)
    amp[f < fmin] = 0
    if fmax:
        amp *= np.exp(-(f / fmax) ** 2)
    amp[0, 0] = 0
    n = np.real(np.fft.ifft2(np.fft.fft2(wn) * amp)).astype(F)
    return norm01(n, 0.5, 99.5)


def blur(a, r):
    """Gaussian blur on the torus, r = sigma in pixels."""
    if r <= 0:
        return a
    if a.ndim == 3:
        return np.stack([blur(a[..., k], r) for k in range(a.shape[2])], -1)
    h, w = a.shape
    fy = np.fft.fftfreq(h)[:, None]
    fx = np.fft.fftfreq(w)[None, :]
    g = np.exp(-2 * (np.pi * r) ** 2 * (fx * fx + fy * fy))
    return np.real(np.fft.ifft2(np.fft.fft2(a) * g)).astype(F)


def blur_xy(a, rx, ry):
    h, w = a.shape
    fy = np.fft.fftfreq(h)[:, None]
    fx = np.fft.fftfreq(w)[None, :]
    g = np.exp(-2 * np.pi ** 2 * ((rx * fx) ** 2 + (ry * fy) ** 2))
    return np.real(np.fft.ifft2(np.fft.fft2(a) * g)).astype(F)


def worley(w, h, n, seed, jitter=1.0):
    """Periodic cell noise. Returns f1, f2 (distances in px) and cell id."""
    rng = np.random.default_rng(seed)
    pts = rng.random((n, 2)) * (w, h)
    x, y = grid(w, h)
    f1 = np.full((h, w), 1e9, F)
    f2 = np.full((h, w), 1e9, F)
    cid = np.zeros((h, w), np.int32)
    for i, (px, py) in enumerate(pts):
        dx = np.abs(x - px)
        dx = np.minimum(dx, w - dx)
        dy = np.abs(y - py)
        dy = np.minimum(dy, h - dy)
        d = np.sqrt(dx * dx + dy * dy)
        closer = d < f1
        f2 = np.where(closer, f1, np.minimum(f2, d))
        cid = np.where(closer, i, cid)
        f1 = np.where(closer, d, f1)
    return f1, f2, cid


def cell_values(cid, n, seed):
    return np.random.default_rng(seed).random(n).astype(F)[cid]


def warp(a, dx, dy):
    """Sample a (2D or 3D) at (x + dx, y + dy) with wrap (bilinear)."""
    h, w = a.shape[:2]
    x, y = grid(w, h)
    sx = (x - 0.5 + dx) % w
    sy = (y - 0.5 + dy) % h
    x0 = np.floor(sx).astype(int)
    y0 = np.floor(sy).astype(int)
    fx = (sx - x0).astype(F)
    fy = (sy - y0).astype(F)
    x1 = (x0 + 1) % w
    y1 = (y0 + 1) % h
    if a.ndim == 3:
        fx = fx[..., None]
        fy = fy[..., None]
    return ((a[y0, x0] * (1 - fx) + a[y0, x1] * fx) * (1 - fy) +
            (a[y1, x0] * (1 - fx) + a[y1, x1] * fx) * fy).astype(F)


# ---------------------------------------------------------------- shapes (signed distance, negative inside)
def sd_box(x, y, cx, cy, hw, hh, r=0.0):
    qx = np.abs(x - cx) - hw + r
    qy = np.abs(y - cy) - hh + r
    out = np.sqrt(np.maximum(qx, 0) ** 2 + np.maximum(qy, 0) ** 2)
    return (out + np.minimum(np.maximum(qx, qy), 0) - r).astype(F)


def sd_circle(x, y, cx, cy, r):
    return (np.sqrt((x - cx) ** 2 + (y - cy) ** 2) - r).astype(F)


def sd_segment(x, y, x0, y0, x1, y1, r):
    px, py = x - x0, y - y0
    bx, by = x1 - x0, y1 - y0
    t = clamp((px * bx + py * by) / (bx * bx + by * by + 1e-9))
    return (np.sqrt((px - bx * t) ** 2 + (py - by * t) ** 2) - r).astype(F)


def sd_poly(x, y, pts):
    """Signed distance to a closed polygon [(x, y), ...]."""
    pts = np.asarray(pts, F)
    d = np.full(x.shape, 1e9, F)
    s = np.ones(x.shape, F)
    n = len(pts)
    for i in range(n):
        ax, ay = pts[i]
        bx, by = pts[(i + 1) % n]
        ex, ey = bx - ax, by - ay
        wx, wy = x - ax, y - ay
        t = clamp((wx * ex + wy * ey) / (ex * ex + ey * ey + 1e-9))
        dd = (wx - ex * t) ** 2 + (wy - ey * t) ** 2
        d = np.minimum(d, dd)
        c1 = y >= ay
        c2 = y < by
        c3 = ex * wy > ey * wx
        flip = (c1 & c2 & c3) | (~c1 & ~c2 & ~c3)
        s = np.where(flip, -s, s)
    return (s * np.sqrt(d)).astype(F)


def tiled(x, y, px, py, ox=0.0, oy=0.0):
    """Local coordinates in a repeating cell of size px x py (cell centred)."""
    return ((x - ox) % px - px / 2).astype(F), ((y - oy) % py - py / 2).astype(F)


def u(*ds):
    return np.minimum.reduce(ds)


def bevel(d, width, top=1.0):
    """Height from a distance field: 0 outside, ramps to `top` over `width` px inside."""
    return (smooth(0, width, -d) * top).astype(F)


def mask(d, aa=0.75):
    return smooth(aa, -aa, d)


# ---------------------------------------------------------------- lighting
LIGHT = np.array([-0.55, -0.65, 0.52], F)
LIGHT /= np.linalg.norm(LIGHT)


def normals(hmap, strength):
    dx = (np.roll(hmap, -1, 1) - np.roll(hmap, 1, 1)) * 0.5 * strength
    dy = (np.roll(hmap, -1, 0) - np.roll(hmap, 1, 0)) * 0.5 * strength
    n = np.stack([-dx, -dy, np.ones_like(hmap)], -1)
    return n / np.linalg.norm(n, axis=-1, keepdims=True)


def shade(hmap, strength=4.0, light=None, amb=0.35):
    """Lambert factor (flat = 1.0) from a height map; light from top-left."""
    L = LIGHT if light is None else np.asarray(light, F) / np.linalg.norm(light)
    n = normals(hmap, strength)
    lam = np.clip((n * L).sum(-1), 0, 1)
    flat = L[2]
    return ((amb + (1 - amb) * lam / flat)).astype(F)


def spec(hmap, strength=4.0, power=24, light=None):
    L = LIGHT if light is None else np.asarray(light, F) / np.linalg.norm(light)
    n = normals(hmap, strength)
    hv = L + np.array([0, 0, 1], F)
    hv /= np.linalg.norm(hv)
    return (np.clip((n * hv).sum(-1), 0, 1) ** power).astype(F)


def cavity(hmap, r=3.0):
    """>0 in creases (lower than surroundings): use for dirt."""
    return clamp((blur(hmap, r) - hmap) * 4)


def light(col, f):
    return (col * (f[..., None] if np.ndim(f) == 2 else f)).astype(F)


def screen(a, b):
    return (1 - (1 - a) * (1 - b)).astype(F)


# ---------------------------------------------------------------- materials
def metal(w, h, seed, base, var=0.12, brushed=0.0, rough=0.5):
    """Painted / bare metal with soft blotches and fine grain."""
    blot = noise(w, h, seed, 2, 24, 2.2)
    grain = noise(w, h, seed + 1, 40, None, 1.0)
    t = 1 + (blot - 0.5) * var * 2 + (grain - 0.5) * 0.08 * rough
    if brushed:
        br = noise(w, h, seed + 2, 4, None, 1.2, aniso=(1, 12))
        t = t + (br - 0.5) * brushed
    return light(fill(w, h, base), t)


def rust_layer(w, h, seed, amount=0.5, scale=6):
    """Rust coverage mask 0..1 (blotchy, with pitting at the edges)."""
    n = noise(w, h, seed, scale * 0.5, scale * 6, 1.6)
    d = noise(w, h, seed + 7, 30, None, 1.0)
    m = smooth(1 - amount - 0.16, 1 - amount + 0.12, n * 0.8 + d * 0.2)
    return m * (0.55 + 0.45 * noise(w, h, seed + 9, 4, 40))


RUST = [(0.0, '#2e1a10'), (0.35, '#4f2c18'), (0.6, '#6e3c1e'), (0.85, '#87512a'), (1.0, '#9a6a40')]


def rust_colour(w, h, seed):
    t = noise(w, h, seed, 3, None, 1.4)
    return gradient_map(t, RUST)


def streaks(w, h, seed, n=40, length=(0.2, 0.7), width=(1, 4), down=True):
    """Vertical run-off streaks (rust / water), 0..1."""
    rng = np.random.default_rng(seed)
    x, y = grid(w, h)
    out = np.zeros((h, w), F)
    for _ in range(n):
        cx = rng.random() * w
        y0 = rng.random() * h
        L = rng.uniform(*length) * h
        wd = rng.uniform(*width)
        dx = np.abs(x - cx)
        dx = np.minimum(dx, w - dx)
        dy = (y - y0) % h
        a = np.exp(-(dx / wd) ** 2) * np.clip(1 - dy / L, 0, 1) * rng.uniform(0.3, 1)
        out = np.maximum(out, a.astype(F))
    wob = noise(w, h, seed + 3, 8, None, 1.2)
    return clamp(out * (0.6 + 0.8 * wob))


def scratches(w, h, seed, n=60, maxlen=40, strength=1.0):
    rng = np.random.default_rng(seed)
    x, y = grid(w, h)
    out = np.zeros((h, w), F)
    for _ in range(n):
        x0, y0 = rng.random() * w, rng.random() * h
        a = rng.random() * np.pi
        L = rng.uniform(5, maxlen)
        x1, y1 = x0 + np.cos(a) * L, y0 + np.sin(a) * L
        lx, ly = tiled(x, y, w, h, x0 - w / 2, y0 - h / 2)
        d = sd_segment(lx, ly, 0, 0, x1 - x0, y1 - y0, 0.35)
        out = np.maximum(out, mask(d, 0.6) * rng.uniform(0.3, 1))
    return out * strength


def concrete(w, h, seed, base='#8a857c', var=0.18):
    a = noise(w, h, seed, 2, 30, 2.0)
    b = noise(w, h, seed + 1, 30, None, 1.0)
    p = worley(w, h, max(8, w * h // 2500), seed + 2)[0]
    pits = clamp(1 - p / 1.4) * (noise(w, h, seed + 3, 20) > 0.6)
    t = 1 + (a - 0.5) * var * 2 + (b - 0.5) * 0.14 - pits * 0.25
    return light(fill(w, h, base), t)


def stone(w, h, seed, c0='#5a554e', c1='#8b8379', rough=1.0):
    a = noise(w, h, seed, 2, 40, 1.8)
    b = noise(w, h, seed + 1, 20, None, 1.1)
    t = clamp(a * 0.7 + b * 0.3 * rough)
    return gradient_map(t, [(0, c0), (1, c1)]), t


def grime(col, amount, seed, c='#1a1610'):
    w, h = col.shape[1], col.shape[0]
    n = noise(w, h, seed, 2, 30, 1.6)
    return lerp(col, fill(w, h, c), clamp(amount * smooth(0.3, 1, n)))


def rivet_field(x, y, centres, r):
    """Height of round rivet heads (domes) at the given centres."""
    hmap = np.zeros(x.shape, F)
    w, h = x.shape[1], x.shape[0]
    for cx, cy in centres:
        dx = np.abs(x - cx)
        dx = np.minimum(dx, w - dx)
        dy = np.abs(y - cy)
        dy = np.minimum(dy, h - dy)
        d2 = (dx * dx + dy * dy) / (r * r)
        hmap = np.maximum(hmap, np.sqrt(np.clip(1 - d2, 0, 1)))
    return hmap


def glow_blob(x, y, cx, cy, r, power=2.0):
    d = np.sqrt((x - cx) ** 2 + (y - cy) ** 2) / r
    return (np.clip(1 - d, 0, 1) ** power).astype(F)


# ---------------------------------------------------------------- output
def to8(a):
    return (np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8)


def save(path, col, alpha=None, quality=90):
    """Write jpg / tga / png. TGA is written uncompressed like the originals,
    unless it is larger than 1 MB (then RLE, which the engine reads too)."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if col.ndim == 2:
        col = np.stack([col] * 3, -1)
    arr = to8(col)
    ext = os.path.splitext(path)[1].lower()
    if alpha is not None and ext != '.jpg':
        im = Image.fromarray(np.dstack([arr, to8(alpha)]), 'RGBA')
    else:
        im = Image.fromarray(arr, 'RGB')
    if ext == '.jpg':
        im.save(path, quality=quality, subsampling=0)
    elif ext == '.tga':
        big = im.size[0] * im.size[1] * len(im.getbands()) > 1 << 20
        im.save(path, compression='tga_rle' if big else None)
    else:
        im.save(path, optimize=True)
    return im
