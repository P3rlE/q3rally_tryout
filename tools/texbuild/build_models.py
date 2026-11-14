"""Own geometry for the CTF flags and the lightning impact.

    python3 tools/texbuild/build_models.py

Surface and shader names stay the same as before, so the existing shaders
(waving cloth, chrome pole, flame emblem) keep working. The cloth keeps
its size and texture direction (u along the cloth, v down from the top).
"""
import math
import os
import sys

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


def main():
    for colour in ('b', 'r'):
        path = os.path.join(BASE, 'models', 'flags', f'{colour}_flag.md3')
        print(path, flag(colour).save(path, f'{colour}_flag'), 'triangles')
    path = os.path.join(BASE, 'models', 'weaphits', 'crackle.md3')
    print(path, crackle().save(path, 'crackle'), 'triangles')


if __name__ == '__main__':
    main()
