"""Build the Cobra (427 roadster with side pipes) body.md3 and wheel.md3.

usage: build_model.py <COBRA.obj> <outdir> [cachedir]

Source: the project's own model (KeyShot OBJ export, 83,951 triangles, no
textures, no .mtl; see CREDITS.txt). It is an open car, so the cockpit is
part of what can be seen: only triangles invisible from every outside view
are dropped (cull.py, cached in cachedir), then materials above KEEP_SMALL
triangles are decimated towards TARGET with their borders in place. The
paint gets the projection atlas of layout.py, chrome and glass the shared
textures, everything else a colour from a palette (parts.tga). The driver's
head sits on tag_head over the left-hand seat, as on the Roadster.

OBJ axes: x = length (front at -x), y = up, z = width (+z is the car's left).
Q3 axes:  X = forward, Y = left, Z = up.
"""
import hashlib
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, HERE)            # cobra/layout.py before ../layout.py
import numpy as np
import cull
import mesh
import obj
from carlib import (BACKWARD, ID, WL, WR, Builder, build_wheel, save, snap_lights, straighten_tire, tri_normal,
                    wheel_tag_z)
from layout import atlas, region, X0, X1, Y0, Y1, Z0, Z1

SRC, OUT = sys.argv[1], sys.argv[2]
CACHE = sys.argv[3] if len(sys.argv) > 3 else OUT
P = 'models/players/cobra/'

LENGTH = 106.0            # world units bumper to bumper (a 4 m roadster among 4.6-5 m cars at 112-116)
WHEEL_Z = -9.7            # wheel centre height the model is laid out at (paint atlas, decals)
TARGET = 60000            # body triangles after decimation
TIRE_TARGET = 3000
KEEP_SMALL = 400

PAINT = 'Paint Metallic Black #2'
TIRE = 'TOPO2 tire-rubber #1'
KIND = {
    'Steel Polished #5': 'chrome', 'Steel Rough Scratched #1': 'chrome',
    'Glass Basic White #1': 'glass',
    'Glass Ridges White #2': 'hlights',         # headlamp lenses
    'LUZES': 'hlights',                          # headlamp reflectors
    'Glass Ridges Orange #1': 'lamps',           # indicators front, tail lamps back
}
DROP = {TIRE, 'Magnesium Rough #1',                   # tyres and rims: wheel.md3
        'Hard Textured Plastic Black #2',             # brake discs and calipers (inside the wheels)
        'COBRA', 'EMBLEMA LATERAL'}                   # badges and lettering (no textures in the source)

# colours of the KeyShot materials (no .mtl); lamps brighter so they read as lamps in game
COLOUR = {
    'Paint Metallic Dark Jasmine #1': (0.42, 0.36, 0.16),   # chassis and suspension
    'Paint Metallic Orange peel Grey #1': (0.42, 0.42, 0.43),
    'Paint Metallic Cool Grey #1': (0.30, 0.31, 0.33),      # seat shells
    'Black Fabric Mesh #2': (0.07, 0.07, 0.08),             # seat upholstery
    'Real wood #1': (0.36, 0.20, 0.09),                     # steering wheel rim
    'Iron Rough #2': (0.30, 0.30, 0.31),
    '[Metal Rough]': (0.55, 0.55, 0.56),
    'Hard Rough Plastic White #1': (0.80, 0.80, 0.78),
    'Metal Rough Grey 10mm Circular Mesh #1': (0.10, 0.10, 0.11),   # grille mesh
    'Cloth Beige #1': (0.10, 0.10, 0.11),                   # carpet (dark)
    'BANCOS': (0.08, 0.08, 0.09), 'Costura': (0.45, 0.10, 0.08), 'Soporte': (0.25, 0.25, 0.26),
    'PLASTICO BRILHANTE': (0.05, 0.05, 0.05), 'FAICHAS': (0.70, 0.70, 0.70),
    'BOTAO': (0.05, 0.05, 0.05), 'BOTAO 1': (0.05, 0.05, 0.05), 'BOTAO 2': (0.05, 0.05, 0.05),
    'headlamp': (0.88, 0.90, 0.94), 'indicator': (0.95, 0.55, 0.10), 'taillamp': (0.80, 0.05, 0.04),
}
GAUGE = (0.85, 0.85, 0.80)    # PAINEL* dials

# ---------------- load and cull --------------------------------------------------------
V, VT, VN, G = obj.load(SRC, groups_as_objects=True, encoding='latin1')
V = np.asarray(V, float)
T = np.array([[c[0] for c in t] for _, tris in G for t in tris], int)
MAT = np.array([m for (o, m), tris in G for _ in tris])
OBJ = np.array([o for (o, m), tris in G for _ in tris])

key = hashlib.sha1(open(SRC, 'rb').read()).hexdigest()[:16]
cache = os.path.join(CACHE, 'cobra_visible_%s.npy' % key)
if os.path.exists(cache):
    vis = np.load(cache)
else:
    vis = cull.visible(V, T, progress=lambda n, k, samples, seen: print(
        'visibility view %d/%d: %d samples, %d triangles seen' % (n, k, samples, seen), flush=True))
    np.save(cache, vis)

# ---------------- placement ----------------------------------------------------------------
body = vis & ~np.isin(MAT, list(DROP))
# loose mounting brackets hanging below the bumpers at the four corners (no bar on them)
tcen = V[T].mean(1)
body &= ~((OBJ == 'PLASTICO BRILHANTE PRETO') & ((tcen[:, 0] < -1.98) | (tcen[:, 0] > 1.93)) & (tcen[:, 1] < 0.37))
bp = V[np.unique(T[body])]
mn, mx = bp.min(0), bp.max(0)
S = LENGTH / (mx[0] - mn[0])
CX, CZ = (mn[0] + mx[0]) / 2, (mn[2] + mx[2]) / 2

tire_t = T[MAT == TIRE]
tc = V[tire_t].mean(1)
left, front = tc[:, 2] > CZ, tc[:, 0] < CX
wheels = {}
for name, sel in (('fl', left & front), ('fr', ~left & front), ('rl', left & ~front), ('rr', ~left & ~front)):
    p = V[np.unique(tire_t[sel])]
    wheels[name] = ((p.min(0) + p.max(0)) / 2, (p.max(0)[1] - p.min(0)[1]) / 2, tire_t[sel],
                    p.max(0)[2] - p.min(0)[2])
CY = np.mean([w[0][1] for w in wheels.values()]) - WHEEL_Z / S


def to_q3(p):
    p = np.asarray(p)
    return np.stack([-(p[..., 0] - CX) * S, (p[..., 2] - CZ) * S, (p[..., 1] - CY) * S], -1)


# tyre for wheel.md3: the rear left one (the rear tyres are wider; the front tags move
# inwards so the outer faces stay where the source has them)
c, r, tt, rear_w = wheels['rl']


def to_wheel(p):
    # model x = out of the car (left wheel), y = forward, z = down (see WL)
    return np.stack([(p[..., 2] - c[2]) * S, -(p[..., 0] - c[0]) * S, -(p[..., 1] - c[1]) * S], -1)


wv, wf = mesh.weld(to_wheel(V), tt)
wv, wf = mesh.decimate(wv, wf, TIRE_TARGET)
wv, _, _ = straighten_tire(wv, wf)
LIFT = (0.0, 0.0, wheel_tag_z(np.hypot(wv[:, 1], wv[:, 2]).max()) - WHEEL_Z)

# ---------------- palette -------------------------------------------------------------------
PAL_N = 8
vis_mats = set(MAT[body])
palette = sorted({m for m in vis_mats if m != PAINT and m not in KIND}
                 | {'headlamp', 'indicator', 'taillamp'})
cell = {m: i for i, m in enumerate(palette)}
assert len(palette) <= PAL_N * PAL_N, len(palette)


def pal_uv(m):
    i = cell[m]
    return [((i % PAL_N) + 0.5) / PAL_N, ((i // PAL_N) + 0.5) / PAL_N]


# ---------------- decimate and build the body surfaces -------------------------------------
total = int(body.sum())
ratio = min(1.0, TARGET / total)
surfs, current = {}, {}
MAX_VERTS = 9000


def surf(group, tex):
    name = current.get(group, group)
    b = surfs.get(name)
    if b is not None and (len(b.verts) > MAX_VERTS or len(b.s.tris) > 19000):
        n = 2
        while '%s%d' % (group, n) in surfs:
            n += 1
        name, b = '%s%d' % (group, n), None
    if b is None:
        b = surfs[name] = Builder(name, P + tex + '.tga', LIFT)
        current[group] = name
    return b


box = {'X': (X0, X1), 'Y': (Y0, Y1), 'Z': (Z0, Z1 + 12)}


def box_uv(reg, p):
    (a0, a1), (b0, b1) = {'L': (box['X'], box['Z']), 'R': (box['X'], box['Z']), 'F': (box['Y'], box['Z']),
                          'B': (box['Y'], box['Z'])}.get(reg, (box['X'], box['Y']))
    i, j = {'L': (0, 2), 'R': (0, 2), 'F': (1, 2), 'B': (1, 2)}.get(reg, (0, 1))
    return [(p[i] - a0) / (a1 - a0), (b1 - p[j]) / (b1 - b0)]


counts = {}
Q = to_q3(V)
for m in sorted(vis_mats):
    sel = body & (MAT == m)
    verts, faces = mesh.weld(Q, T[sel])
    n0 = len(faces)
    if n0 > KEEP_SMALL and ratio < 1.0:
        verts, faces = mesh.decimate(verts, faces, max(KEEP_SMALL, round(n0 * ratio)))
    counts[m] = (n0, len(faces))
    cn = mesh.corner_normals(verts, faces)
    kind = 'paint' if m == PAINT else KIND.get(m, 'parts')
    for f, face in enumerate(faces):
        pts = [tuple(verts[i]) for i in face]
        fn = tri_normal(pts)
        if fn == (0.0, 0.0, 0.0) or not all(map(math.isfinite, fn)):
            continue
        ns = [tuple(n) for n in cn[f]]
        cx = sum(p[0] for p in pts) / 3
        if kind == 'paint':
            cy = sum(p[1] for p in pts) / 3
            b = surf('body_%s%s' % ('f' if cx >= 0 else 'r', 'l' if cy >= 0 else 'r'), 'red_main')
            uvs = [atlas(region(fn), p) for p in pts]
        elif kind in ('glass', 'chrome'):
            b = surf(kind, kind)
            uvs = [box_uv(region(fn), p) for p in pts]
        elif kind == 'hlights':
            b = surf('hlights', 'parts')
            uvs = [pal_uv('headlamp')] * 3
        elif kind == 'lamps':
            b = surf('hlights' if cx > 0 else 'rlights', 'parts')
            uvs = [pal_uv('indicator' if cx > 0 else 'taillamp')] * 3
        else:
            b = surf('parts', 'parts')
            uvs = [pal_uv(m)] * 3
        b.tri([b.vert(pts[k], ns[k], uvs[k]) for k in range(3)])

GROUPS = ['body_fl', 'body_fr', 'body_rl', 'body_rr', 'glass', 'chrome', 'parts', 'hlights', 'rlights']
ORDER = sorted(surfs, key=lambda n: (GROUPS.index(n.rstrip('0123456789')), n))


# ---------------- tags --------------------------------------------------------------------------
def pts_of(mats, sel=lambda p: np.ones(len(p), bool)):
    p = Q[np.unique(T[body & np.isin(MAT, mats)])]
    return p[sel(p)]


def mid(p):
    return (p.min(0) + p.max(0)) / 2


wq = {k: to_q3(w[0]) for k, w in wheels.items()}
front_w = wheels['fl'][3]
inset = (rear_w - front_w) / 2 * S                      # front tags move in by this
FX = (wq['fl'][0] + wq['fr'][0]) / 2
RX = (wq['rl'][0] + wq['rr'][0]) / 2
FY = np.mean([abs(wq['fl'][1]), abs(wq['fr'][1])]) - inset
RY = np.mean([abs(wq['rl'][1]), abs(wq['rr'][1])])
WZ = np.mean([w[2] for w in wq.values()])
allp = Q[np.unique(T[body])]
front_x, rear_x = allp[:, 0].max(), allp[:, 0].min()

hl = mid(pts_of(['Glass Ridges White #2'], lambda p: p[:, 1] > 0))
tl = mid(pts_of(['Glass Ridges Orange #1'], lambda p: (p[:, 0] < 0) & (p[:, 1] > 0)))
# side pipes: the rearmost chrome low down on the left
ex_p = pts_of(['Steel Polished #5'], lambda p: (p[:, 2] < -9) & (p[:, 1] > 15))
ex = np.array([ex_p[:, 0].min(), mid(ex_p)[1], mid(ex_p)[2]]) if len(ex_p) else np.array([-20.0, 20.0, -12.0])
# plate: low on the tail between the lamps
tail = allp[(np.abs(allp[:, 1]) < 6) & (allp[:, 2] < tl[2] - 1) & (allp[:, 2] > tl[2] - 6)]
plate_x = tail[:, 0].min() if len(tail) else rear_x
plate_z = tl[2] - 3.5
# driver's head over the left-hand seat (steering wheel side)
seat = pts_of(['Black Fabric Mesh #2'], lambda p: p[:, 1] > 0)
wheel_rim = mid(pts_of(['Real wood #1']))
head = [float(seat[:, 0].min() + 0.35 * (seat[:, 0].max() - seat[:, 0].min())), float(wheel_rim[1]),
        float(seat[:, 2].max() - 2.0)]
cockpit_rim = float(pts_of([PAINT], lambda p: np.abs(p[:, 0] - head[0]) < 8)[:, 2].max())

tags = [
    ('tag_wheelfl', [FX, FY, WZ], WL), ('tag_wheelfr', [FX, -FY, WZ], WR),
    ('tag_wheelrl', [RX, RY, WZ], WL), ('tag_wheelrr', [RX, -RY, WZ], WR),
    ('tag_weapon', [head[0] - 10.0, 0.0, cockpit_rim + 2.0], ID),
    ('tag_plate', [plate_x - 0.2, 0.0, plate_z], ID),
    ('tag_turbo', [ex[0] - 1.0, ex[1], ex[2]], BACKWARD),
    ('tag_smoke', [front_x - 16.0, 0.0, 2.0], BACKWARD),
    ('tag_head', head, ID),
    ('tag_hlite1', [hl[0] + 0.5, hl[1], hl[2]], ID), ('tag_hlite2', [hl[0] + 0.5, -hl[1], hl[2]], ID),
    ('tag_blite1', [tl[0] - 0.5, -tl[1], tl[2]], ID), ('tag_blite2', [tl[0] - 0.5, tl[1], tl[2]], ID),
    ('tag_rlite1', [tl[0] - 0.5, -tl[1], tl[2] - 1.5], ID), ('tag_rlite2', [tl[0] - 0.5, tl[1], tl[2] - 1.5], ID),
]
tags = [(n, [float(o[0]), float(o[1]), float(o[2]) + LIFT[2]], a) for n, o, a in tags]
tags = snap_lights(tags, [surfs[n] for n in ORDER])
bmn, bmx, rad = save(os.path.join(OUT, 'body.md3'), P + 'body.md3',
                     [surfs[n].done() for n in ORDER], tags, b'cobra')
print('scale %.2f  body bounds %s %s radius %.1f' % (S, [round(c, 1) for c in bmn], [round(c, 1) for c in bmx], rad))
print('visible %d of %d triangles, written %d' % (total, len(T), sum(c[1] for c in counts.values())))
for m, (a, b) in sorted(counts.items(), key=lambda x: -x[1][0]):
    print('  %-40s %6d -> %6d' % (m, a, b))
for n, o, a in tags:
    print('  %-12s %s' % (n, [round(c, 2) for c in o]))

# ---------------- palette texture ----------------------------------------------------------------
from PIL import Image
pal = np.zeros((PAL_N * 8, PAL_N * 8, 3), np.uint8)
for m, i in cell.items():
    rgb = COLOUR.get(m, GAUGE if m.startswith('PAINEL') else (0.5, 0.5, 0.5))
    if m not in COLOUR and not m.startswith('PAINEL'):
        print('  no colour for', m)
    pal[(i // PAL_N) * 8:(i // PAL_N) * 8 + 8, (i % PAL_N) * 8:(i % PAL_N) * 8 + 8] = \
        [min(255, int(round(255 * (max(0.0, v) ** (1 / 2.2))))) for v in rgb]
Image.fromarray(pal).save(os.path.join(OUT, 'parts.tga'), compression='tga_rle')

# ---------------- wheel ---------------------------------------------------------------------------
wn = mesh.corner_normals(wv, wf, 30.0)
tire = [([tuple(wv[i]) for i in face], [tuple(n) for n in wn[f]]) for f, face in enumerate(wf)]
open_out, open_in, wmn, wmx = build_wheel(tire, r * S, os.path.join(OUT, 'wheel.md3'), P + 'wheel.md3')
print('wheel radius %.2f, opening out %.2f in %.2f, bounds %s %s'
      % (r * S, open_out, open_in, [round(v, 2) for v in wmn], [round(v, 2) for v in wmx]))
