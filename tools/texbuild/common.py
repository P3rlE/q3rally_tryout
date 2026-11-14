"""Shared material builders and the recipe registry."""
from tex import *

RECIPES = []


def recipe(fn):
    """A recipe returns {relative path: (colour, alpha or None)}."""
    RECIPES.append(fn)
    return fn


def finish(col, hmap, strength=4.0, dirt=0.4, r=3.0, seed=0, amb=0.35, gloss=0.0, power=20):
    """Light a coloured surface with its height map, darken creases, add a highlight."""
    out = light(col, shade(hmap, strength, amb=amb))
    if dirt:
        out = light(out, 1 - dirt * cavity(hmap, r))
    if gloss:
        out = out + gloss * spec(hmap, strength, power)[..., None]
    return clamp(out)


def worn_plate(w, h, seed, base, rust=0.0, dirt=0.35, var=0.14, scratch=0.25, rust_scale=6):
    """Painted or bare metal: blotches, scratches, optional rust patches."""
    col = metal(w, h, seed, base, var)
    if scratch:
        s = scratches(w, h, seed + 11, int(40 * w * h / 65536) + 10, max(20, w // 6))
        col = lerp(col, clamp(col * 1.45 + 0.04), s * scratch)
    if rust:
        m = rust_layer(w, h, seed + 21, rust, rust_scale)
        col = lerp(col, rust_colour(w, h, seed + 22), m)
    if dirt:
        col = grime(col, dirt, seed + 31)
    return col


def edge_wear(hmap, amount=0.4):
    """Bright rim where the height changes (worn paint on edges)."""
    g = np.abs(np.roll(hmap, 1, 0) - hmap) + np.abs(np.roll(hmap, 1, 1) - hmap)
    return clamp(g * 6 * amount)


def diamond_plate(w, h, cell, seed, base, rust=0.0, length=0.36, rad=0.075):
    """Classic tread plate: elongated bumps, alternating diagonal direction."""
    x, y = grid(w, h)
    lx, ly = tiled(x, y, cell, cell)
    L = cell * length / 2
    d1 = sd_segment(lx, ly, -L * 0.7071, -L * 0.7071, L * 0.7071, L * 0.7071, cell * rad)
    lx2, ly2 = tiled(x, y, cell, cell, cell / 2, cell / 2)
    d2 = sd_segment(lx2, ly2, -L * 0.7071, L * 0.7071, L * 0.7071, -L * 0.7071, cell * rad)
    d = np.minimum(d1, d2)
    hm = bevel(d, cell * rad * 1.1)
    hm = np.sqrt(hm)
    col = worn_plate(w, h, seed, base, rust=rust, dirt=0.45, scratch=0.3)
    col = lerp(col, clamp(col * 1.35 + 0.03), hm * 0.5)      # worn bump tops
    return finish(col, hm + noise(w, h, seed + 5, 30) * 0.08, 3.5, 0.6, 2, seed, gloss=0.12)


def tile_grid(w, h, nx, ny, seam=1.5, bev=3.0):
    """Height map of square tiles separated by seams."""
    x, y = grid(w, h)
    lx, ly = tiled(x, y, w / nx, h / ny)
    d = sd_box(lx, ly, 0, 0, w / nx / 2 - seam, h / ny / 2 - seam, 1.0)
    return bevel(d, bev)


def stone_blocks(w, h, rows, seed, minw=0.18, maxw=0.45, mortar=2.5, bev=6.0, stagger=True):
    """Irregular stone courses. Returns height map and per-block id map."""
    rng = np.random.default_rng(seed)
    x, y = grid(w, h)
    hm = np.zeros((h, w), F)
    bid = np.zeros((h, w), np.int32)
    ys = np.cumsum([0] + list(rng.uniform(0.7, 1.3, rows)))
    ys = ys / ys[-1] * h
    k = 0
    for r in range(rows):
        y0, y1 = ys[r], ys[r + 1]
        xs = [0.0]
        while xs[-1] < w:
            xs.append(xs[-1] + rng.uniform(minw, maxw) * w)
        xs = np.array(xs) * (w / xs[-1])
        off = rng.random() * w
        for i in range(len(xs) - 1):
            cx = (xs[i] + xs[i + 1]) / 2 + off
            hw = (xs[i + 1] - xs[i]) / 2 - mortar
            cy, hh = (y0 + y1) / 2, (y1 - y0) / 2 - mortar
            lx, ly = tiled(x, y, w, h, cx - w / 2, 0)
            d = sd_box(lx, y - cy, 0, 0, hw, hh, 2.5)
            b = bevel(d + noise(w, h, seed + k, 6, 40) * 3 - 1.5, bev)
            sel = b > hm
            bid = np.where(sel, k, bid)
            hm = np.maximum(hm, b)
            k += 1
    return hm, bid, k


def wrapd(x, y, cx, cy, w, h):
    """Wrapped offsets from a centre (for shapes that cross the border)."""
    dx = (x - cx + w / 2) % w - w / 2
    dy = (y - cy + h / 2) % h - h / 2
    return dx.astype(F), dy.astype(F)


def tube_layer(x, y, pts, r, w=None, h=None):
    """Distance to a polyline (unsigned) -> round tube profile 0..1 and mask."""
    d = np.full(x.shape, 1e9, F)
    for (x0, y0), (x1, y1) in zip(pts[:-1], pts[1:]):
        d = np.minimum(d, sd_segment(x, y, x0, y0, x1, y1, 0))
    prof = np.sqrt(clamp(1 - (d / r) ** 2))
    return prof, d < r


def rounded_path(pts, rad, steps=8):
    """Replace the corners of a polyline with arcs (quadratic smoothing)."""
    pts = [np.array(p, float) for p in pts]
    out = [pts[0]]
    for a, b, c in zip(pts[:-2], pts[1:-1], pts[2:]):
        da = (a - b) / (np.linalg.norm(a - b) + 1e-9)
        dc = (c - b) / (np.linalg.norm(c - b) + 1e-9)
        p0 = b + da * min(rad, np.linalg.norm(a - b) / 2)
        p2 = b + dc * min(rad, np.linalg.norm(c - b) / 2)
        for t in np.linspace(0, 1, steps):
            out.append((1 - t) ** 2 * p0 + 2 * (1 - t) * t * b + t * t * p2)
    out.append(pts[-1])
    return [tuple(p) for p in out]


def gear_sd(dx, dy, R, teeth, depth, width):
    """Signed distance (approx.) of a gear ring: outer radius R with teeth."""
    r = np.sqrt(dx * dx + dy * dy)
    a = np.arctan2(dy, dx)
    tooth = (np.cos(a * teeth) > 0.2).astype(F)
    outer = R + depth * blur_free(tooth)
    return np.maximum(r - outer, (R - width) - r)


def blur_free(t):
    return t
