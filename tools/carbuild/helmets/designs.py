"""Paint designs of the driver helmets (build_helmets.py).

Every design is drawn on the helmet itself: the paint function gets, for
every texel of the 512 x 256 paint, the point on the shell (p, in game
units), its normal and these coordinates:
    x, y, z   position scaled to about -1..1 (x forward, y left, z up)
    phi       angle round the head, 0 ahead, -pi..pi
    theta     elevation from the shell centre
so stripes, checks and decals sit where they would on a real helmet.

kind: 'full' (full face with visor) or 'open' (open face, goggles).
visor: tint of the visor or goggle lenses (VISORS).
"""
import numpy as np

import tex as T

VISORS = {
    'smoke': ('#0c0e12', '#2c323a'),
    'iridium': ('#0a0a2a', '#2a5ab0'),
    'gold': ('#3a2406', '#d09a30'),
    'fire': ('#2a0606', '#c0401a'),
    'silver': ('#202428', '#9aa4b0'),
}

AA = 0.06          # anti-aliasing width in game units


def col(pts, c):
    return np.broadcast_to(np.array(T.rgb(c), np.float32), pts['p'].shape).copy()


def put(base, pts, mask, c):
    """Paint colour c where mask (0..1) is set."""
    return base * (1 - mask[..., None]) + col(pts, c) * mask[..., None]


def band(d):
    """Signed distance (units) -> 0..1 coverage, smooth edge."""
    return np.clip(0.5 - d / AA, 0, 1)


def gloss_shade(c, pts, amount=0.10):
    """A little top light baked in: lighter crown, darker lower edge."""
    k = 1 + amount * pts['z'] - amount * 0.5 * np.clip(-pts['z'] - 0.6, 0, 1)
    return np.clip(c * k[..., None], 0, 1)


def front_face(pts):
    """Region round the visor opening that stays the base colour (not stripes)."""
    return (pts['x'] > 0.35) & (pts['z'] < 0.42)


def stripe(pts, centre, half):
    """Front-to-back stripe over the crown at lateral offset `centre` (units)."""
    d = np.abs(pts['p'][..., 1] - centre) - half
    return band(d) * (~front_face(pts))


def side(pts, text=False):
    """2D coordinates on each side: a runs backwards from the front (the
    same on both sides, for flames and arrows), or with text=True left to
    right as a reader of that side sees it; b up. w: 0..1 weight that fades
    out towards the crown and the face."""
    s = np.sign(pts['p'][..., 1])
    a = -pts['p'][..., 0] * (s if text else 1)
    b = pts['p'][..., 2]
    w = np.clip((np.abs(pts['y']) - 0.45) / 0.15, 0, 1)
    return a, b, w


def lower(pts, z):
    """Everything below height z (units, absolute)."""
    return band(pts['p'][..., 2] - z)


def strap(pts):
    """Goggle strap of the open helmets, round the back at eye height."""
    th = np.degrees(pts['theta'])
    return band(np.abs(th - 0.0) * 0.08 - 0.42) * (np.abs(pts['phi']) > np.radians(55))


def star_sd(a, b, cx, cy, r):
    """Signed distance to a five-pointed star."""
    pts = []
    for k in range(10):
        ang = np.pi / 2 + k * np.pi / 5
        rr = r if k % 2 == 0 else r * 0.42
        pts.append((cx + np.cos(ang) * rr, cy + np.sin(ang) * rr))
    return T.sd_poly(a, b, pts)


def digit_sd(a, b, d, cx, cy, h, w=0.13):
    """Signed distance to a digit drawn as rounded strokes (font free)."""
    strokes = {
        '1': [[(0.35, 0.8), (0.55, 1.0), (0.55, 0.0)]],
        '3': [[(0.1, 1.0), (0.85, 1.0), (0.45, 0.55), (0.85, 0.3), (0.65, 0.0), (0.1, 0.05)]],
        '4': [[(0.7, 0.0), (0.7, 1.0), (0.05, 0.3), (0.95, 0.3)]],
        '7': [[(0.1, 1.0), (0.9, 1.0), (0.35, 0.0)]],
        '8': [[(0.5, 0.55), (0.15, 0.78), (0.5, 1.0), (0.85, 0.78), (0.5, 0.55), (0.1, 0.28), (0.5, 0.0), (0.9, 0.28), (0.5, 0.55)]],
    }[d]
    out = np.full(a.shape, 1e9, np.float32)
    sc = h
    for line in strokes:
        for (x0, y0), (x1, y1) in zip(line, line[1:]):
            out = np.minimum(out, T.sd_segment(a, b, cx + (x0 - 0.5) * sc * 0.7, cy + (y0 - 0.5) * sc,
                                               cx + (x1 - 0.5) * sc * 0.7, cy + (y1 - 0.5) * sc, w * sc))
    return out


def noise3(pts, seed, scale):
    """Smooth noise on the helmet (sum of a few waves in 3D, no seam)."""
    rng = np.random.default_rng(seed)
    p = pts['p']
    v = np.zeros(p.shape[:2], np.float32)
    for k in range(10):
        d = rng.normal(size=3)
        d /= np.linalg.norm(d)
        f = scale * rng.uniform(0.6, 1.6)
        v += np.sin((p @ d) * f + rng.uniform(0, 6.3)) / (1 + k * 0.15)
    return (v - v.min()) / (v.max() - v.min())


# ---------------------------------------------------------------- the designs
def classic(pts):
    c = col(pts, '#eceae4')
    c = put(c, pts, stripe(pts, 0, 0.38), '#1c3a8a')
    for s in (-1, 1):
        c = put(c, pts, stripe(pts, s * 0.9, 0.16), '#c01820')
    c = put(c, pts, lower(pts, -6.5), '#202020')
    return gloss_shade(c, pts)


def racer(pts):
    c = col(pts, '#c4141a')
    c = put(c, pts, stripe(pts, 0, 1.0), '#141414')
    c = put(c, pts, stripe(pts, 0, 0.8), '#f2f2f0')
    c = put(c, pts, stripe(pts, 0, 0.12), '#141414')
    return gloss_shade(c, pts)


def checker(pts):
    c = col(pts, '#d01a1a')
    th, ph = pts['theta'], pts['phi']
    s_lat = th * 4.4 / 0.9
    s_lon = ph * np.cos(th) * 4.6 / 0.9
    chk = ((np.floor(s_lat) + np.floor(s_lon)) % 2 == 0)
    top = band(-(pts['p'][..., 2] + 2.3)) * (~front_face(pts))
    c = put(c, pts, top * chk, '#f4f4f2')
    c = put(c, pts, top * (~chk), '#121212')
    c = put(c, pts, band(np.abs(pts['p'][..., 2] + 2.3) - 0.12) * (~front_face(pts)), '#f4f4f2')
    return gloss_shade(c, pts)


def flames(pts):
    c = col(pts, '#121214')
    a, b, w = side(pts)
    n = noise3(pts, 31, 1.6)
    d = np.full(a.shape, 1e9, np.float32)
    tongues = [(-4.6, 2.6, 0.55), (-3.9, 4.6, 0.6), (-3.1, 6.2, 0.5), (-5.4, 3.6, 0.45), (-2.3, 4.2, 0.4)]
    for b0, length, width in tongues:
        s = np.clip((a + 4.8) / length, 0, 1.4)            # 0 at the front, 1 at the tip
        bc = b0 + 1.1 * s ** 1.4 + 0.25 * np.sin(s * 7 + b0)
        dd = np.abs(b - bc) - width * np.clip(1 - s, 0, 1) ** 0.8 + (s > 1) * 9
        d = np.minimum(d, dd)
    d = d + (n - 0.5) * 0.25
    m = np.clip((np.abs(pts['y']) - 0.15) / 0.2, 0, 1)
    c = put(c, pts, band(d) * m, '#a8120c')
    c = put(c, pts, band(d + 0.14) * m, '#ec5a10')
    c = put(c, pts, band(d + 0.3) * m, '#ffd23a')
    return gloss_shade(c, pts, 0.06)


def carbon(pts):
    th, ph = pts['theta'], pts['phi']
    s_lat = th * 4.4 / 0.22
    s_lon = ph * np.cos(th) * 4.6 / 0.22
    weave = ((np.floor(s_lat) + np.floor(s_lon)) % 2).astype(np.float32)
    fl = np.abs(np.sin(np.where(weave > 0, s_lat, s_lon) * np.pi))
    v = 0.09 + 0.06 * weave + 0.05 * fl
    c = np.stack([v, v, v * 1.08], -1).astype(np.float32)
    c = put(c, pts, stripe(pts, 0.55, 0.06), '#d01818')
    c = put(c, pts, stripe(pts, -0.55, 0.06), '#d01818')
    return gloss_shade(c, pts, 0.05)


def police(pts):
    c = col(pts, '#16224a')
    zb = pts['p'][..., 2]
    bandm = band(np.abs(zb + 5.0) - 0.75) * (~((pts['x'] > 0.3) & (zb > -6.2)))
    th, ph = pts['theta'], pts['phi']
    s_lon = ph * np.cos(th) * 4.6 / 0.5
    row = np.floor((zb + 5.75) / 0.5)
    chk = ((np.floor(s_lon) + row) % 2 == 0)
    c = put(c, pts, bandm * chk, '#f2f2f0')
    c = put(c, pts, bandm * (~chk), '#1a3a9a')
    a, b, w = side(pts, True)
    c = put(c, pts, band(star_sd(a, b, 0.8, -2.6, 0.8)) * w, '#e0b830')
    return gloss_shade(c, pts)


def camo(pts):
    c = col(pts, '#b8a07a')
    for seed, thr, cc in ((41, 0.55, '#8a744e'), (42, 0.6, '#5e5236'), (43, 0.66, '#d4c4a0')):
        n = noise3(pts, seed, 2.2)
        c = put(c, pts, np.clip((n - thr) / 0.02, 0, 1), cc)
    return gloss_shade(c, pts, 0.04)


def neon(pts):
    c = col(pts, '#0c0e10')
    th, ph = pts['theta'], pts['phi']
    s_lat = th * 4.4
    s_lon = ph * np.cos(th) * 4.6
    rng = np.random.default_rng(51)
    d = np.full(th.shape, 1e9, np.float32)
    for _ in range(26):                                # traces: along, then up, then along
        x0, y0 = rng.uniform(-14, 14), rng.uniform(-5, 6)
        l1, l2 = rng.uniform(1, 4), rng.uniform(0.5, 2) * rng.choice([-1, 1])
        x1, y1 = x0 + l1, y0
        x2, y2 = x1 + abs(l2) * 0.7, y1 + l2
        for (ax, ay), (bx, by) in (((x0, y0), (x1, y1)), ((x1, y1), (x2, y2))):
            d = np.minimum(d, T.sd_segment(s_lon, s_lat, ax, ay, bx, by, 0.06))
        d = np.minimum(d, np.hypot(s_lon - x2, s_lat - y2) - 0.2)
    d = d + front_face(pts) * 9
    glow = np.exp(-np.clip(d, 0, None) / 0.25)
    c = c + np.array(T.rgb('#2aff60'), np.float32) * (glow * 0.35)[..., None]
    c = put(c, pts, band(d), '#7cff9a')
    return np.clip(gloss_shade(c, pts, 0.03), 0, 1)


def gold(pts):
    n = noise3(pts, 61, 3.0)
    c = col(pts, '#c89a38') * (0.9 + 0.2 * n)[..., None]
    a, b, w = side(pts)
    for k in range(3):                                 # chevrons pointing forward
        d = np.abs(np.abs(b + 3.6) * 0.9 + (a - (-1.5 + k * 1.3))) - 0.22
        c = put(c, pts, band(d) * w * (a > -4) * (np.abs(b + 3.6) < 1.6), '#161412')
    c = put(c, pts, lower(pts, -6.4), '#161412')
    return np.clip(gloss_shade(c, pts, 0.12), 0, 1)


def lightning(pts):
    c = col(pts, '#1a4ec0')
    a, b, w = side(pts)
    pts2 = [(-4.6, -1.2), (-2.4, -2.4), (-2.6, -3.0), (0.4, -3.9), (0.1, -4.5), (3.6, -5.4)]
    d = np.full(a.shape, 1e9, np.float32)
    for (x0, y0), (x1, y1) in zip(pts2, pts2[1:]):
        d = np.minimum(d, T.sd_segment(a, b, x0, y0, x1, y1, 0.0))
    t = np.clip((a + 4.6) / 8.2, 0, 1)
    d = d - (0.42 - 0.3 * t)
    c = put(c, pts, band(d - 0.12) * w, '#0a1840')
    c = put(c, pts, band(d) * w, '#ffd820')
    c = put(c, pts, stripe(pts, 0, 0.14), '#ffd820')
    return gloss_shade(c, pts)


def tricolore(pts):
    x = pts['p'][..., 0]
    c = col(pts, '#f0eee8')
    c = put(c, pts, band(-(x - 1.0)) * (~front_face(pts)), '#119048')
    c = put(c, pts, band(x + 2.4), '#c8202a')
    c = put(c, pts, band(-(x - 1.0)) * front_face(pts), '#119048')
    return gloss_shade(c, pts)


def stars(pts):
    c = col(pts, '#14204a')
    c = put(c, pts, stripe(pts, 0, 0.9), '#f2f2f0')
    for s in (-0.5, 0, 0.5):
        c = put(c, pts, stripe(pts, s, 0.14), '#c41a24')
    a, b, w = side(pts)
    for cx, cy, r in ((-1.8, -2.2, 0.7), (0.4, -1.6, 0.55), (2.2, -2.6, 0.6), (-0.6, -4.2, 0.5), (1.6, -4.6, 0.45)):
        c = put(c, pts, band(star_sd(a, b, cx, cy, r)) * w, '#f2f2f0')
    return gloss_shade(c, pts)


def sunset(pts):
    z = pts['p'][..., 2]
    t = np.clip((z + 7.2) / 8.0, 0, 1)
    c = T.gradient_map(t, [(0, '#3a0a5a'), (0.4, '#c0207a'), (0.7, '#f06a2a'), (1, '#ffc040')]).astype(np.float32)
    for k in range(5):
        zz = -4.0 - k * 0.55
        c = put(c, pts, band(np.abs(z - zz) - (0.05 + 0.04 * k)) * (z < -3.6), '#1a0a20')
    return gloss_shade(c, pts, 0.05)


def arrow(pts):
    c = col(pts, '#f0f0ee')
    a, b, w = side(pts)
    for k in range(4):
        d = np.abs(np.abs(b + 3.4) * 0.8 + (a - (-2.2 + k * 1.15))) - 0.28
        c = put(c, pts, band(d) * w * (np.abs(b + 3.4) < 1.5), ['#ff6a10', '#ff8a20', '#ffaa30', '#ffca50'][k])
    c = put(c, pts, stripe(pts, 0, 0.2), '#ff6a10')
    c = put(c, pts, lower(pts, -6.5), '#2a2a2a')
    return gloss_shade(c, pts)


# open-face helmets: the strap of the goggles runs round the back
def retro(pts):
    c = col(pts, '#ece2c8')
    for s in (-0.42, 0.42):
        c = put(c, pts, stripe(pts, s, 0.1), '#7a1a1a')
    c = put(c, pts, strap(pts), '#2a1a12')
    return gloss_shade(c, pts)


def vintage(pts):
    n = noise3(pts, 71, 2.5)
    c = col(pts, '#b4bac2') * (0.92 + 0.12 * n)[..., None]
    a, b, w = side(pts, True)
    ring = np.hypot(a - 1.2, b + 4.0) - 1.25
    c = put(c, pts, band(ring) * w, '#f4f4f0')
    c = put(c, pts, band(digit_sd(a, b, '7', 1.2, -4.0, 1.5)) * w, '#1a1a1a')
    c = put(c, pts, strap(pts), '#1a1a1a')
    return np.clip(gloss_shade(c, pts), 0, 1)


def moto(pts):
    c = col(pts, '#f07818')
    c = put(c, pts, stripe(pts, 0, 0.75), '#141414')
    th, ph = pts['theta'], pts['phi']
    chk = ((np.floor(th * 4.4 / 0.3) + np.floor(pts['p'][..., 1] / 0.3)) % 2 == 0)
    c = put(c, pts, stripe(pts, 0, 0.6) * chk, '#f4f4f2')
    c = put(c, pts, strap(pts), '#141414')
    return gloss_shade(c, pts)


def pilot(pts):
    c = col(pts, '#5a6236')
    n = noise3(pts, 81, 2.0)
    c = c * (0.94 + 0.1 * n)[..., None]
    a, b, w = side(pts)
    c = put(c, pts, band(star_sd(a, b, 1.0, -4.0, 1.25)) * w, '#f0eee4')
    c = put(c, pts, strap(pts), '#2a2418')
    return np.clip(gloss_shade(c, pts, 0.04), 0, 1)


DESIGNS = {
    'classic': {'kind': 'full', 'paint': classic, 'visor': 'smoke'},
    'racer': {'kind': 'full', 'paint': racer, 'visor': 'smoke'},
    'checker': {'kind': 'full', 'paint': checker, 'visor': 'iridium'},
    'flames': {'kind': 'full', 'paint': flames, 'visor': 'fire'},
    'carbon': {'kind': 'full', 'paint': carbon, 'visor': 'fire'},
    'police': {'kind': 'full', 'paint': police, 'visor': 'smoke'},
    'camo': {'kind': 'full', 'paint': camo, 'visor': 'gold'},
    'neon': {'kind': 'full', 'paint': neon, 'visor': 'iridium'},
    'gold': {'kind': 'full', 'paint': gold, 'visor': 'gold'},
    'lightning': {'kind': 'full', 'paint': lightning, 'visor': 'iridium'},
    'tricolore': {'kind': 'full', 'paint': tricolore, 'visor': 'silver'},
    'stars': {'kind': 'full', 'paint': stars, 'visor': 'silver'},
    'sunset': {'kind': 'full', 'paint': sunset, 'visor': 'fire'},
    'arrow': {'kind': 'full', 'paint': arrow, 'visor': 'smoke'},
    'retro': {'kind': 'open', 'paint': retro, 'visor': 'smoke'},
    'vintage': {'kind': 'open', 'paint': vintage, 'visor': 'gold'},
    'moto': {'kind': 'open', 'paint': moto, 'visor': 'smoke'},
    'pilot': {'kind': 'open', 'paint': pilot, 'visor': 'silver'},
}


def shared_textures():
    """Visors, gloss reflection, rubber trim, balaclava."""
    out = {}
    w, h = 64, 64
    x, y = T.grid(w, h)
    for name, (c0, c1) in VISORS.items():
        t = np.clip(y / h, 0, 1)
        out[f'visor_{name}.jpg'] = (T.gradient_map(1 - t, [(0, c0), (1, c1)]), None)
    w = h = 128
    x, y = T.grid(w, h)
    t = np.clip(1 - y / h, 0, 1)
    env = T.gradient_map(t, [(0, '#000000'), (0.5, '#141414'), (0.62, '#505860'), (1, '#c8d4e0')])
    env = T.screen(env, np.ones((h, w, 3), np.float32) * (T.glow_blob(x, y, 44, 28, 16, 2.0) * 0.9)[..., None])
    out['visor_env.jpg'] = (env, None)
    gl = T.gradient_map(t, [(0, '#000000'), (0.55, '#000000'), (0.7, '#1a1c20'), (1, '#4a5058')])
    gl = T.screen(gl, np.ones((h, w, 3), np.float32) * (T.glow_blob(x, y, 44, 28, 12, 2.4) * 0.55)[..., None])
    out['gloss.jpg'] = (gl, None)
    w = h = 64
    n = T.noise(w, h, 91, 8, None, 1.2)
    out['trim.jpg'] = (T.gradient_map(n, [(0, '#0e0e10'), (1, '#2a2a2e')]), None)
    w = h = 128
    x, y = T.grid(w, h)
    knit = 0.5 + 0.5 * np.sin(x * np.pi / 2) * np.sin(y * np.pi / 3)
    n = T.noise(w, h, 92, 6, None, 1.4)
    out['balaclava.jpg'] = (T.gradient_map(np.clip(knit * 0.12 + n * 0.5 + 0.2, 0, 1), [(0, '#141416'), (1, '#3a3a40')]), None)
    return out
