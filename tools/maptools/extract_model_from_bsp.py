#!/usr/bin/env python3
"""Get a misc_model back out of a compiled BSP as an md3.

q3map2 bakes every misc_model into the map's surfaces. When the md3 itself
is lost, one placed copy in a BSP still holds its geometry: this tool takes
the triangle soups with the given shader prefix near --origin, undoes the
placement (origin, angle round z, modelscale) and writes them as an md3,
one surface per shader, with the texture coordinates and normals of the BSP.

usage: extract_model_from_bsp.py maps/x.bsp --shader models/mapobjects/spotlamp1/
           --origin 750 1371 448 [--angle 90] [--scale 0.25] [--radius 128] --out model.md3
"""
import argparse
import math
import os
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'carbuild'))
import md3  # noqa: E402
import q3bsp  # noqa: E402

VERT = np.dtype([('xyz', '<f4', 3), ('st', '<f4', 2), ('lm', '<f4', 2), ('n', '<f4', 3), ('rgba', 'u1', 4)])


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('bsp')
    ap.add_argument('--shader', required=True)
    ap.add_argument('--origin', nargs=3, type=float, required=True)
    ap.add_argument('--angle', type=float, default=0.0)
    ap.add_argument('--scale', type=float, default=1.0)
    ap.add_argument('--radius', type=float, default=256.0, help='only surfaces whose centre is this close')
    ap.add_argument('--out', required=True)
    a = ap.parse_args()

    d = open(a.bsp, 'rb').read()
    lumps = q3bsp.read_lumps(d)
    names = [d[lumps[1][0] + i * 72:lumps[1][0] + i * 72 + 64].split(b'\0')[0].decode('latin-1')
             for i in range(lumps[1][1] // 72)]
    verts = np.frombuffer(d[lumps[10][0]:lumps[10][0] + lumps[10][1]], VERT)
    idx = np.frombuffer(d[lumps[11][0]:lumps[11][0] + lumps[11][1]], '<i4')
    o = np.array(a.origin)
    t = math.radians(-a.angle)
    R = np.array([[math.cos(t), -math.sin(t), 0], [math.sin(t), math.cos(t), 0], [0, 0, 1]])

    surfs = {}
    for i in range(lumps[13][1] // 104):
        f = struct.unpack_from('<12i', d, lumps[13][0] + i * 104)
        name = names[f[0]]
        if f[2] != 3 or not name.lower().startswith(a.shader.lower()):
            continue
        vv = verts[f[3]:f[3] + f[4]]
        if np.linalg.norm(vv['xyz'].mean(0) - o) > a.radius:
            continue
        surfs.setdefault(name, []).append((vv, idx[f[5]:f[5] + f[6]]))
    if not surfs:
        sys.exit('nothing found')

    m = md3.Model()
    m.name, m.flags = os.path.splitext(os.path.basename(a.out))[0], 0
    m.tags = [[]]
    m.surfs = []
    allp = []
    for name, parts in sorted(surfs.items()):
        s = md3.Surf()
        s.name, s.flags, s.shaders = os.path.basename(name), 0, [(name, 0)]
        s.st, s.tris, frame = [], [], []
        for vv, ii in parts:
            base = len(s.st)
            P = (vv['xyz'] - o) @ R.T / a.scale
            N = vv['n'] @ R.T
            for p, n, st in zip(P, N, vv['st']):
                frame.append([round(p[0] * 64), round(p[1] * 64), round(p[2] * 64), md3.enc_normal(*n)])
                s.st.append([float(st[0]), float(st[1])])
                allp.append(p)
            s.tris += [[base + int(ii[k]), base + int(ii[k + 1]), base + int(ii[k + 2])] for k in range(0, len(ii), 3)]
        s.frames = [frame]
        m.surfs.append(s)
        print(f'{name}: {len(s.st)} verts, {len(s.tris)} tris')
    P = np.array(allp)
    lo, hi = P.min(0), P.max(0)
    m.frames = [[*lo, *hi, 0, 0, 0, float(np.linalg.norm(P, axis=1).max()), m.name.encode()[:16]]]
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    md3.save(m, a.out)
    print('written', a.out, 'bounds', lo.round(1), hi.round(1))


if __name__ == '__main__':
    main()
