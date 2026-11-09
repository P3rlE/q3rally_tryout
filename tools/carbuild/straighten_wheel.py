"""Rebuild a car's wheel.md3 (made by carlib.build_wheel) with the tyre running true.

usage: straighten_wheel.py <wheel.md3 in> <wheel.md3 out> [--dry-run]

The tyre triangles are read back from w_tread and w_rim (the generated discs
that close the rim opening are left out and made again), turned onto the axle
with carlib.straighten_tire and passed through build_wheel, so UVs and discs
are generated exactly as in a fresh build. Prints toe/camber/offset of the
tyre before and after; a wheel that runs true shows about 0 deg and 0.
"""
import math
import sys

import numpy as np

import md3
import mesh
from carlib import build_wheel, straighten_tire

SRC, DST = sys.argv[1], sys.argv[2]
DRY = '--dry-run' in sys.argv
m = md3.load(SRC)
surf = {s.name: s for s in m.surfs}
NX_OUT, NX_IN = md3.enc_normal(1, 0, 0), md3.enc_normal(-1, 0, 0)


def pos(s):
    return np.array([f[:3] for f in s.frames[0]], float) / 64


# radius the rim UVs were made with: u = 0.5 - y / (2R)
rim = surf['w_rim']
rp, st = pos(rim), np.array(rim.st)
ok = np.abs(st[:, 0] - 0.5) > 0.05
R = float(np.median(-rp[ok, 1] / (2 * (st[ok, 0] - 0.5))))

# discs: build_wheel writes each as its own connected piece of w_rim (its vertices
# carry the normal +-x and are shared with no tyre triangle) of 6 rings x 96 segments
rn = np.array([f[3] for f in rim.frames[0]])
tris_rim = np.array(rim.tris)
parent = list(range(len(rp)))


def find(i):
    while parent[i] != i:
        parent[i] = parent[parent[i]]
        i = parent[i]
    return i


for t in tris_rim:
    a, b, c = find(t[0]), find(t[1]), find(t[2])
    parent[b] = a
    parent[find(c)] = a
comp = np.array([find(t[0]) for t in tris_rim])
flat = np.isin(rn, (NX_OUT, NX_IN))
disc_t = np.zeros(len(tris_rim), bool)
for c in np.unique(comp):
    sel = comp == c
    if sel.sum() == 6 * 96 * 2 and flat[tris_rim[sel]].all():
        disc_t |= sel
print('rim radius %.3f; %d disc triangles left out of %d' % (R, disc_t.sum(), len(tris_rim)))
if disc_t.sum() != 2 * 6 * 96 * 2:
    sys.exit('unexpected disc triangle count, not a build_wheel wheel?')

# tyre triangles in FBX winding (md3 stores them the other way round)
P = np.vstack([pos(surf['w_tread']), rp])
T = np.vstack([np.array(surf['w_tread'].tris), tris_rim[~disc_t] + len(surf['w_tread'].frames[0])])[:, [0, 2, 1]]
verts, faces = mesh.weld(P, T, eps=1 / 128)


def report(tag, v):
    r = np.hypot(v[:, 1], v[:, 2])
    _, rot, centre = straighten_tire(v, faces)
    n = rot.T @ np.array([1.0, 0, 0])
    print('%-6s toe %+6.2f deg  camber %+6.2f deg  centre off x %+.3f y %+.3f z %+.3f  radius %.2f'
          % (tag, math.degrees(math.atan2(n[1], n[0])), math.degrees(math.atan2(n[2], n[0])),
             centre[0], centre[1], centre[2], r.max()))


report('before', verts)
verts, _, _ = straighten_tire(verts, faces)
report('after', verts)
if DRY:
    sys.exit()
normals = mesh.corner_normals(verts, faces, 30.0)
tire = [([tuple(verts[i]) for i in face], [tuple(n) for n in normals[f]]) for f, face in enumerate(faces)]
name = m.name.decode() if isinstance(m.name, bytes) else m.name
oo, oi, mn, mx = build_wheel(tire, R, DST, name)
print('written %s: opening out %.2f in %.2f, bounds %s %s' % (DST, oo, oi, [round(v, 2) for v in mn], [round(v, 2) for v in mx]))
