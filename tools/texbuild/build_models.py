"""Own geometry for the CTF flags, the lightning impact and the debris.

    python3 tools/texbuild/build_models.py

The debris pieces keep the size, position and shader of the pieces they
replace (debris_sizes.json holds only those numbers); their shapes are new.
Surface and shader names stay the same as before, so the existing shaders
(waving cloth, chrome pole, flame emblem) keep working. The cloth keeps
its size and texture direction (u along the cloth, v down from the top).
"""
import math
import os
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'carbuild'))
from prim import Mesh  # noqa: E402

BASE = os.path.join(HERE, '..', '..', 'baseq3r')


def flag(colour):
    m = Mesh({'m': (0.0, 0.0, 1.0)})
    # pole: a slim tube with a base collar and a top collar, u around, v along
    m.surf('pole', 'models/flags/pole')
    rings = [(-12.5, 1.6), (-10.5, 1.6), (-10.0, 1.15), (58.0, 1.15), (58.5, 1.5), (60.6, 1.5)]
    segs = 10
    idx = []
    for z, r in rings:
        row = []
        for k in range(segs + 1):
            a = 2 * math.pi * k / segs
            row.append(m.vert((math.cos(a) * r, math.sin(a) * r, z), (math.cos(a), math.sin(a), 0), 'm',
                              (k / segs, (z + 12.5) / 73.1)))
        idx.append(row)
    for r0, r1 in zip(idx[:-1], idx[1:]):
        for k in range(segs):
            m.tri(r0[k], r0[k + 1], r1[k + 1])
            m.tri(r0[k], r1[k + 1], r1[k])
    # top: a faceted ball (lat-long), the emblem sits inside it
    m.surf('sphere', 'models/flags/flagsphere')
    c, R, nu, nv = (0.0, 0.0, 64.1), 3.9, 12, 7
    grid = []
    for j in range(nv + 1):
        th = math.pi * j / nv
        row = []
        for k in range(nu + 1):
            ph = 2 * math.pi * k / nu
            n = (math.sin(th) * math.cos(ph), math.sin(th) * math.sin(ph), math.cos(th))
            row.append(m.vert(tuple(c[i] + n[i] * R for i in range(3)), n, 'm', (k / nu, j / nv)))
        grid.append(row)
    for j in range(nv):
        for k in range(nu):
            a, b, cc, d = grid[j][k], grid[j][k + 1], grid[j + 1][k + 1], grid[j + 1][k]
            m.tri(a, cc, b)
            m.tri(a, d, cc)
    # cloth: 16 x 16 cells so the wave deform has vertices to move
    m.surf('cloth', f'models/flags/{colour}_flag')
    x0, x1, z0, z1, n = -33.2, -1.3, 25.7, 58.4, 16
    g = [[m.vert((x0 + (x1 - x0) * i / n, 0.0, z1 - (z1 - z0) * j / n), (0, -1, 0), 'm', (i / n, 1 + j / n))
          for i in range(n + 1)] for j in range(n + 1)]
    for j in range(n):
        for i in range(n):
            m.tri(g[j][i], g[j][i + 1], g[j + 1][i + 1])
            m.tri(g[j][i], g[j + 1][i + 1], g[j + 1][i])
    # emblem inside the ball: a wheel (rim, five spokes, hub), both sides
    m.surf('logo', f'textures/sfx/fire_ctf{"blue" if colour == "b" else "red"}')
    cy, cz = 0.0, 64.1

    def p(y, z):
        return (0.0, cy + y, cz + z)

    def uv(y, z):
        return (0.5 + y / 7.0, 0.5 - z / 7.0)

    quads = []
    seg = 20
    for k in range(seg):                                  # rim
        a0, a1 = 2 * math.pi * k / seg, 2 * math.pi * (k + 1) / seg
        quads.append([(math.cos(a0) * 3.4, math.sin(a0) * 3.4), (math.cos(a1) * 3.4, math.sin(a1) * 3.4),
                      (math.cos(a1) * 2.7, math.sin(a1) * 2.7), (math.cos(a0) * 2.7, math.sin(a0) * 2.7)])
    for k in range(5):                                    # spokes
        a = math.pi / 2 + 2 * math.pi * k / 5
        ux, uy = math.cos(a), math.sin(a)
        px, py = -uy * 0.35, ux * 0.35
        quads.append([(ux * 0.9 + px, uy * 0.9 + py), (ux * 2.8 + px, uy * 2.8 + py),
                      (ux * 2.8 - px, uy * 2.8 - py), (ux * 0.9 - px, uy * 0.9 - py)])
    for k in range(8):                                    # hub
        a0, a1 = 2 * math.pi * k / 8, 2 * math.pi * (k + 1) / 8
        quads.append([(0, 0), (math.cos(a0) * 1.0, math.sin(a0) * 1.0), (math.cos(a1) * 1.0, math.sin(a1) * 1.0), (0, 0)])
    for q in quads:
        for nx in (1, -1):
            ids = [m.vert(p(y, z), (nx, 0, 0), 'm', uv(y, z)) for y, z in q]
            if nx > 0:
                m.tri(ids[0], ids[1], ids[2]); m.tri(ids[0], ids[2], ids[3])
            else:
                m.tri(ids[0], ids[2], ids[1]); m.tri(ids[0], ids[3], ids[2])
    return m


def crackle():
    """Three crossed quads for the lightning impact sprite."""
    m = Mesh({'m': (0.0, 0.0, 1.0)})
    m.surf('crackle01', 'models/weaphits/electric.tga')
    s = 32.0
    uvs = [(0, 0), (1, 0), (1, 1), (0, 1)]
    planes = [([(-s, 0, s), (s, 0, s), (s, 0, -s), (-s, 0, -s)], (0, 1, 0)),
              ([(0, -s, s), (0, s, s), (0, s, -s), (0, -s, -s)], (1, 0, 0)),
              ([(-s, -s, 0), (s, -s, 0), (s, s, 0), (-s, s, 0)], (0, 0, 1))]
    for pts, n in planes:
        i = [m.vert(pt, n, 'm', uv) for pt, uv in zip(pts, uvs)]
        m.tri(i[0], i[1], i[2])
        m.tri(i[0], i[2], i[3])
    return m


def chunk(rng, size, kind):
    """Points of a debris piece inside its box `size` (x, y, z)."""
    sx, sy, sz = (s / 2 for s in size)
    if kind == 'wood':                          # a splintered plank along its longest axis
        ax = int(np.argmax(size))
        half = [sx, sy, sz]
        o = [i for i in range(3) if i != ax]
        pts = []
        for end in (-1, 1):
            for k in range(rng.integers(3, 6)):    # ragged end: points pulled back by different amounts
                p = [rng.uniform(-1, 1) * h for h in half]
                p[ax] = end * half[ax] * rng.uniform(0.55, 1.0)
                pts.append(p)
            for a in (-1, 1):
                for b in (-1, 1):
                    p = [0.0, 0.0, 0.0]
                    p[o[0]], p[o[1]] = a * half[o[0]], b * half[o[1]]
                    p[ax] = end * half[ax] * rng.uniform(0.45, 0.8)
                    pts.append(p)
        return np.array(pts)
    if kind == 'glass':                         # a flat shard with a few sharp corners
        thin = int(np.argmin(size))
        o = [i for i in range(3) if i != thin]
        k = rng.integers(4, 7)
        ang = np.sort(rng.uniform(0, 2 * np.pi, k))
        rad = rng.uniform(0.45, 1.0, k)
        pts = []
        for a, r in zip(ang, rad):
            for side in (-1, 1):
                p = [0.0, 0.0, 0.0]
                p[o[0]] = np.cos(a) * r * size[o[0]] / 2
                p[o[1]] = np.sin(a) * r * size[o[1]] / 2
                p[thin] = side * size[thin] / 2
                pts.append(p)
        return np.array(pts)
    # concrete and stone: a lumpy block, flatter faces where it broke
    n = 16 if max(size) < 30 else 22
    d = rng.normal(size=(n, 3))
    d /= np.linalg.norm(d, axis=1)[:, None]
    d = np.sign(d) * np.abs(d) ** 0.6           # pushed towards a box: broken blocks have flat sides
    return d * [sx, sy, sz] * rng.uniform(0.8, 1.0, (n, 1))


def debris(name, info, seed):
    """One debris model: the convex hull of chunk(), flat-shaded."""
    from scipy.spatial import ConvexHull
    rng = np.random.default_rng(seed)
    kind = 'wood' if 'wood' in name else 'glass' if 'glass' in name else 'stone'
    pts = chunk(rng, info['size'], kind)
    lo, hi = pts.min(0), pts.max(0)             # stretch to exactly the old size
    pts = (pts - (lo + hi) / 2) * (np.array(info['size']) / np.maximum(hi - lo, 1e-3)) + np.array(info['centre'])
    hull = ConvexHull(pts)
    m = Mesh({'m': (0.0, 0.0, 1.0)})
    m.surf('debris', info['shader'])
    ax = int(np.argmax(info['size']))
    for simplex, eq in zip(hull.simplices, hull.equations):
        n = eq[:3]
        a, b, c = (pts[i] for i in simplex)
        if np.dot(np.cross(b - a, c - a), n) < 0:
            b, c = c, b
        ids = []
        for p in (a, b, c):
            if kind == 'wood':                  # grain runs along the plank
                o = [i for i in range(3) if i != ax]
                uv = ((p[ax] - pts[:, ax].min()) / max(info['size'][ax], 1), 0.5 + (p[o[0]] + p[o[1]]) / 16.0)
            else:                               # box projection, one texture repeat per 64 units
                k = int(np.argmax(np.abs(n)))
                o = [i for i in range(3) if i != k]
                uv = (p[o[0]] / 64.0, p[o[1]] / 64.0)
            ids.append(m.vert(tuple(float(v) for v in p), tuple(float(v) for v in n), 'm', uv))
        m.tri(*ids)
    return m


# ---------------------------------------------------------------- trophy (q3r_ctf_04)
TROPHY = 'models/mapobjects/q3r_trophy'


def lathe(m, profile, segs, uscale=1.0, vlen=None, phase=0.0):
    """Smooth surface of revolution round z through profile [(r, z), ...].
    A point given twice is a crease (the runs on both sides get their own
    normals). u runs round (uscale repeats), v along the profile (one repeat
    per vlen units of profile length, or once over the whole profile)."""
    runs, cur = [], [profile[0]]
    for a in profile[1:]:
        if a == cur[-1]:
            runs.append(cur)
            cur = [a]
        else:
            cur.append(a)
    runs.append(cur)
    total = sum(math.dist(a, b) for a, b in zip(profile, profile[1:]))
    vlen = vlen or total
    s0 = 0.0
    for run in runs:
        if len(run) < 2:
            continue
        acc = [s0]
        for a, b in zip(run, run[1:]):
            acc.append(acc[-1] + math.dist(a, b))
        s0 = acc[-1]
        rows = []
        for j, (r, z) in enumerate(run):
            a, b = run[max(j - 1, 0)], run[min(j + 1, len(run) - 1)]
            dr, dz = b[0] - a[0], b[1] - a[1]
            ln = math.hypot(dr, dz) or 1.0
            nr, nz = dz / ln, -dr / ln
            row = []
            for k in range(segs + 1):
                t = 2 * math.pi * k / segs + phase
                c, sn = math.cos(t), math.sin(t)
                row.append(m.vert((r * c, r * sn, z), (nr * c, nr * sn, nz), 'm', (uscale * k / segs, acc[j] / vlen)))
            rows.append(row)
        for j in range(len(rows) - 1):
            for k in range(segs):
                a, b, c, d = rows[j][k], rows[j][k + 1], rows[j + 1][k + 1], rows[j + 1][k]
                m.tri(a, b, c)
                m.tri(a, c, d)


def prism(m, profile, sides, phase):
    """Faceted stone: like lathe() (same profile direction: the outside is
    on the right of the walk), but every side is a flat face; the texture
    repeats every 64 units, so joints and grain match from face to face."""
    ang = [2 * math.pi * k / sides + phase for k in range(sides + 1)]
    for (r0, z0), (r1, z1) in zip(profile, profile[1:]):
        dr, dz = r1 - r0, z1 - z0
        ln = math.hypot(dr, dz)
        if ln < 1e-9:
            continue
        nr, nz = dz / ln, -dr / ln
        for k in range(sides):
            a0, a1 = ang[k], ang[k + 1]
            am = (a0 + a1) / 2
            p = [(r0 * math.cos(a0), r0 * math.sin(a0), z0), (r0 * math.cos(a1), r0 * math.sin(a1), z0),
                 (r1 * math.cos(a1), r1 * math.sin(a1), z1), (r1 * math.cos(a0), r1 * math.sin(a0), z1)]
            n = (nr * math.cos(am), nr * math.sin(am), nz)
            if abs(dz) < 1e-9:                  # flat ring: planar uv from above
                uvs = [(q[0] / 64, q[1] / 64) for q in p]
            else:                               # side: across the face, height
                uvs = [((-q[0] * math.sin(am) + q[1] * math.cos(am)) / 64, -q[2] / 64) for q in p]
            ids = [m.vert(q, n, 'm', uv) for q, uv in zip(p, uvs)]
            m.tri(ids[0], ids[1], ids[2])
            m.tri(ids[0], ids[2], ids[3])


def bent_tube(m, pts, r, segs):
    """A round tube through pts with smooth normals; rings are carried along
    the curve without twisting, so it can bend round tight loops."""
    pts = np.array(pts, float)
    rings, prev = [], None
    for j in range(len(pts)):
        d = pts[min(j + 1, len(pts) - 1)] - pts[max(j - 1, 0)]
        d /= np.linalg.norm(d)
        if prev is None:
            ref = np.array([1.0, 0, 0]) if abs(d[0]) < 0.9 else np.array([0, 0, 1.0])
            u = np.cross(d, ref)
        else:
            u = prev - d * np.dot(prev, d)
        u /= np.linalg.norm(u)
        v = np.cross(d, u)
        prev = u
        rings.append([(np.cos(2 * np.pi * k / segs) * u + np.sin(2 * np.pi * k / segs) * v) for k in range(segs + 1)])
    rows = [[m.vert(tuple(pts[j] + r * n), tuple(n), 'm', (k / segs, j / (len(pts) - 1))) for k, n in enumerate(ring)]
            for j, ring in enumerate(rings)]
    for j in range(len(rows) - 1):
        for k in range(segs):
            a, b, c, d = rows[j][k], rows[j][k + 1], rows[j + 1][k + 1], rows[j + 1][k]
            m.tri(a, b, c)
            m.tri(a, c, d)


def trophy():
    """The monument in the fountain of q3r_ctf_04: a rally trophy on a stone
    plinth. Origin on the fountain floor, about 135 units high like the old
    statue."""
    m = Mesh({'m': (0.0, 0.0, 1.0)})
    oct_ = math.pi / 8
    m.surf('plinth', f'{TROPHY}/stone')
    prism(m, [(26.5, -8), (26.5, 14), (21.5, 14), (21.5, 46), (24.5, 46), (24.5, 49), (21.5, 52),
              (0, 52)], 8, oct_)
    m.surf('cup', f'{TROPHY}/bronze')
    lathe(m, [(0, 52), (15, 52), (15, 52), (15, 55), (15, 55), (12.5, 57), (6, 63), (4.2, 67), (4.2, 67),
              (7.5, 69), (8, 71), (7.5, 73), (7.5, 73), (4.4, 75), (4.6, 78), (8.5, 81), (16, 88), (22, 98),
              (25.5, 109), (26.8, 120), (26.8, 120), (28.6, 121.5), (28.6, 124.5), (28.6, 124.5),
              (26.6, 124.5), (26.6, 124.5), (25.2, 118), (22, 104), (15, 92), (6, 87.5), (0, 87)], 32, 4, 24)
    for side in (-1, 1):                       # two handles, one continuous bent tube each
        ctrl = [(22.5, 113), (33, 116), (40.5, 110), (41.5, 99), (36, 89.5), (26, 86), (14.5, 89.5)]
        pts = []
        for i in range(len(ctrl) - 1):          # Catmull-Rom through the control points
            p0, p1, p2, p3 = (ctrl[max(i - 1, 0)], ctrl[i], ctrl[i + 1], ctrl[min(i + 2, len(ctrl) - 1)])
            for t in [k / 4 for k in range(4)]:
                yz = [0.5 * (2 * p1[c] + (-p0[c] + p2[c]) * t + (2 * p0[c] - 5 * p1[c] + 4 * p2[c] - p3[c]) * t * t
                             + (-p0[c] + 3 * p1[c] - 3 * p2[c] + p3[c]) * t ** 3) for c in (0, 1)]
                pts.append((0, side * yz[0], yz[1]))
        pts.append((0, side * ctrl[-1][0], ctrl[-1][1]))
        bent_tube(m, pts, 2.3, 10)
    m.surf('band', f'{TROPHY}/band')          # chequered ring round the bowl
    lathe(m, [(24.25, 103.5), (25.35, 108.2), (25.95, 112.5)], 48, 12)
    m.surf('plaques', f'{TROPHY}/plaque')     # a bronze plaque on four sides of the plinth
    a = 21.5 * math.cos(oct_) + 0.35
    for k in range(4):
        t = k * math.pi / 2
        c, sn = math.cos(t), math.sin(t)
        q = [(-6.5, 24), (6.5, 24), (6.5, 38), (-6.5, 38)]
        pts = [(a * c - u * sn, a * sn + u * c, z) for u, z in q]
        uvs = [(0, 1), (1, 1), (1, 0), (0, 0)]
        ids = [m.vert(p, (c, sn, 0), 'm', uv) for p, uv in zip(pts, uvs)]
        m.tri(ids[0], ids[1], ids[2])
        m.tri(ids[0], ids[2], ids[3])
    return m


def main():
    import json
    sizes = json.load(open(os.path.join(HERE, 'debris_sizes.json')))
    for i, (name, info) in enumerate(sorted(sizes.items())):
        path = os.path.join(BASE, 'models', 'debris', name)
        debris(name, info, 2000 + i).save(path, name[:-4])
    print(len(sizes), 'debris models')
    for colour in ('b', 'r'):
        path = os.path.join(BASE, 'models', 'flags', f'{colour}_flag.md3')
        print(path, flag(colour).save(path, f'{colour}_flag'), 'triangles')
    path = os.path.join(BASE, 'models', 'weaphits', 'crackle.md3')
    print(path, crackle().save(path, 'crackle'), 'triangles')
    path = os.path.join(BASE, *TROPHY.split('/'), 'trophy.md3')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    print(path, trophy().save(path, 'trophy'), 'triangles')


if __name__ == '__main__':
    main()
