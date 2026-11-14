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


if __name__ == '__main__':
    main()
