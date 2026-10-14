"""Build the Skyline body.md3 and wheel.md3 from the Sketchfab OBJ.

usage: build_model.py <r34.obj> <outdir>

Source: "Nissan Skyline R34 GT-R" by Lexyc16 (https://skfb.ly/6TxFX),
CC BY 4.0. The model is kept at full detail; only the main paint material
is split into four surfaces (front/rear x left/right) so every surface stays
below the engine's per-surface limit.

OBJ axes: x = width (+x is the car's left), y = up, z = length (+z front).
Q3 axes:  X = forward, Y = left, Z = up.
"""
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.dirname(HERE))
sys.path.insert(0, HERE)            # r34/layout.py before ../layout.py
import md3
import obj
from layout import atlas, region, X0, X1, Y0, Y1, Z0, Z1

SRC, OUT = sys.argv[1], sys.argv[2]
P = 'models/players/skyline/'

LENGTH = 112.0            # world units, like the other cars
WHEEL_Z = -9.7            # wheel centre height of the other cars

# material -> surface name; None = dropped
SURF = {
    'Material.001': 'paint',
    'Material': 'hlights',       # headlights
    'Material.011': 'ilights',   # indicators and side markers
    'Material.009': 'rlights',   # round tail lights
    'Material.006': 'glass',
    'Material.002': 'trim',      # window seals, lip, plastic trim
    'Material.005': 'trim',      # wing blade, plate frame
    'Material.004': 'chrome',    # exhaust, plate frame details
    'Material.013': 'chrome',    # badges
    'Material.015': 'chrome',
    'Material.032': None,        # manufacturer emblem on the boot lid, left out
    'Material.008': 'grill',
    'Material.003': 'bottom',    # underbody, fuel lid
}
TEX = {'hlights': 'lights', 'ilights': 'lights', 'rlights': 'tlite', 'glass': 'glass', 'trim': 'trim',
       'chrome': 'chrome', 'grill': 'grill', 'bottom': 'under'}
OWN_UV = ('hlights', 'ilights', 'rlights', 'grill')   # keep the model's UVs for these
WHEEL_OBJECTS = ('Circle', 'Circle.001', 'Circle.002_Circle.004', 'Circle.003_Circle.006')
DROP_OBJECTS = ('Circle.004_Circle.008',)            # manufacturer emblem
WHEEL_SRC = 'Circle'                                  # front left
TIRE_MTL = 'Material.010'

V, VT, VN, G = obj.load(SRC)

# ---------------- placement -----------------------------------------------------
body_ids = {c[0] for (o, m), tris in G if o not in WHEEL_OBJECTS for t in tris for c in t}
mn = [min(V[i][k] for i in body_ids) for k in range(3)]
mx = [max(V[i][k] for i in body_ids) for k in range(3)]
S = LENGTH / (mx[2] - mn[2])
CX, CZ = (mn[0] + mx[0]) / 2, (mn[2] + mx[2]) / 2

tire = [V[c[0]] for (o, m), tris in G if o == WHEEL_SRC and m == TIRE_MTL for t in tris for c in t]
W_C = [(min(p[k] for p in tire) + max(p[k] for p in tire)) / 2 for k in range(3)]   # wheel centre (obj)
W_R = (max(p[1] for p in tire) - min(p[1] for p in tire)) / 2 * S                    # tyre radius (world)
CY = W_C[1] - WHEEL_Z / S


def to_q3(p):
    return ((p[2] - CZ) * S, (p[0] - CX) * S, (p[1] - CY) * S)


def nrm_q3(n):
    return (n[2], n[0], n[1])


def tri_normal(pts):
    a, b, c = pts
    u = [b[k] - a[k] for k in range(3)]
    v = [c[k] - a[k] for k in range(3)]
    n = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
    l = math.sqrt(sum(x * x for x in n)) or 1.0
    return tuple(x / l for x in n)


class Builder:
    """collects one md3 surface; vertices are unique per (pos, normal, uv)"""

    def __init__(self, name, shader):
        self.s = md3.Surf()
        self.s.name, self.s.flags, self.s.shaders = name, 0, [(shader, 0)]
        self.s.tris, self.s.st, self.verts, self.idx = [], [], [], {}

    def vert(self, pos, n, uv):
        q = [int(round(c * 64)) for c in pos]
        key = (tuple(q), md3.enc_normal(*n), round(uv[0], 5), round(uv[1], 5))
        if key not in self.idx:
            self.idx[key] = len(self.verts)
            self.verts.append(q + [key[1]])
            self.s.st.append([uv[0], uv[1]])
        return self.idx[key]

    def tri(self, corners):
        # md3 triangles run the other way round than OBJ faces
        self.s.tris.append([corners[0], corners[2], corners[1]])

    def done(self):
        self.s.frames = [self.verts]
        return self.s


def corner(c):
    return to_q3(V[c[0]]), nrm_q3(VN[c[2]]) if c[2] is not None else None, VT[c[1]] if c[1] is not None else (0, 0)


# ---------------- body ------------------------------------------------------------
surfs = {}


def surf(name, tex):
    if name not in surfs:
        surfs[name] = Builder(name, P + tex + '.tga')
    return surfs[name]


box = {'X': (X0, X1), 'Y': (Y0, Y1), 'Z': (Z0, Z1)}


def box_uv(reg, p):
    """planar projection over the whole car for the shared trim textures"""
    (a0, a1), (b0, b1) = {'L': (box['X'], box['Z']), 'R': (box['X'], box['Z']), 'F': (box['Y'], box['Z']),
                          'B': (box['Y'], box['Z'])}.get(reg, (box['X'], box['Y']))
    i, j = {'L': (0, 2), 'R': (0, 2), 'F': (1, 2), 'B': (1, 2)}.get(reg, (0, 1))
    return [(p[i] - a0) / (a1 - a0), (b1 - p[j]) / (b1 - b0)]


for (o, m), tris in G:
    if o in WHEEL_OBJECTS or o in DROP_OBJECTS:
        continue
    kind = SURF.get(m)
    if kind is None:
        continue
    for t in tris:
        cs = [corner(c) for c in t]
        pts = [c[0] for c in cs]
        fn = tri_normal(pts)
        ns = [c[1] or fn for c in cs]
        if kind == 'paint':
            cx = sum(p[0] for p in pts) / 3
            cy = sum(p[1] for p in pts) / 3
            b = surf('body_%s%s' % ('f' if cx >= 0 else 'r', 'l' if cy >= 0 else 'r'), 'red_main')
            reg = region(fn)
            uvs = [atlas(reg, p) for p in pts]
        elif kind in OWN_UV:
            b = surf(kind, TEX[kind])
            uvs = [[c[2][0], 1 - c[2][1]] for c in cs]
        else:
            b = surf(kind, TEX[kind])
            reg = region(fn)
            uvs = [box_uv(reg, p) for p in pts]
        b.tri([b.vert(pts[k], ns[k], uvs[k]) for k in range(3)])

ORDER = ['body_fl', 'body_fr', 'body_rl', 'body_rr', 'glass', 'trim', 'chrome', 'grill',
         'hlights', 'ilights', 'rlights', 'bottom']

# ---------------- tags --------------------------------------------------------------
def centre(mtl, sel=lambda p: True):
    pts = [to_q3(V[c[0]]) for (o, m), tris in G if m == mtl and o not in WHEEL_OBJECTS
           for t in tris for c in t]
    pts = [p for p in pts if sel(p)]
    return [(min(p[k] for p in pts) + max(p[k] for p in pts)) / 2 for k in range(3)], pts


def wheel_pos(o):
    pts = [to_q3(V[c[0]]) for (oo, m), tris in G if oo == o and m == TIRE_MTL for t in tris for c in t]
    return [(min(p[k] for p in pts) + max(p[k] for p in pts)) / 2 for k in range(3)]


ID = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
BACKWARD = [[-1, 0, 0], [0, -1, 0], [0, 0, 1]]
WL = [[0, 1, 0], [1, 0, 0], [0, 0, -1]]          # left wheels: model +x points out of the car
WR = [[0, -1, 0], [-1, 0, 0], [0, 0, -1]]        # right wheels
wheels = {o: wheel_pos(o) for o in WHEEL_OBJECTS}
front = [w for w in wheels.values() if w[0] > 0]
rear = [w for w in wheels.values() if w[0] < 0]
FX = sum(w[0] for w in front) / len(front)
RX = sum(w[0] for w in rear) / len(rear)
WY = sum(abs(w[1]) for w in wheels.values()) / 4
WZ = sum(w[2] for w in wheels.values()) / 4

# lights: per side centres of the lamp materials
hl, _ = centre('Material', lambda p: p[1] > 0)
_, tl = centre('Material.009')
tl_left = sorted({(round(p[1], 1)) for p in tl if p[1] > 0})
# the four round tail lamps: split the left ones at the gap between inner and outer lamp
lamps_l = [p for p in tl if p[1] > 0]
gap = max((b - a, (a + b) / 2) for a, b in zip(tl_left, tl_left[1:]))[1]
inner = [p for p in lamps_l if p[1] < gap]
outer = [p for p in lamps_l if p[1] >= gap]


def mid(pts):
    return [(min(p[k] for p in pts) + max(p[k] for p in pts)) / 2 for k in range(3)]


ti, to = mid(inner), mid(outer)
ex, _ = centre('Material.004', lambda p: p[0] < -40 and p[2] < -5)   # exhaust tip
roof = max(to_q3(V[i])[2] for i in body_ids)
front_x = max(to_q3(V[i])[0] for i in body_ids)
# rearmost point of the bumper face behind the plate (plate_usa is 9.7 x 4.7 units)
plate_x = min(p[0] for p in (to_q3(V[i]) for i in body_ids) if abs(p[1]) < 4.9 and -6.9 < p[2] < -2.1)

tags = [
    ('tag_wheelfl', [FX, WY, WZ], WL), ('tag_wheelfr', [FX, -WY, WZ], WR),
    ('tag_wheelrl', [RX, WY, WZ], WL), ('tag_wheelrr', [RX, -WY, WZ], WR),
    ('tag_weapon', [-8.0, 0.0, roof], ID),                        # roof
    ('tag_plate', [plate_x - 0.2, 0.0, -4.5], ID),                # just off the flat bumper face
    ('tag_turbo', [ex[0] - 1.0, ex[1], ex[2]], BACKWARD),         # exhaust
    ('tag_smoke', [front_x - 18.0, 0.0, 4.5], BACKWARD),          # hood
    ('tag_hlite1', [hl[0] + 0.5, hl[1], hl[2]], ID), ('tag_hlite2', [hl[0] + 0.5, -hl[1], hl[2]], ID),
    ('tag_blite1', [to[0] - 0.5, -to[1], to[2]], ID), ('tag_blite2', [to[0] - 0.5, to[1], to[2]], ID),
    ('tag_rlite1', [ti[0] - 0.5, -ti[1], ti[2]], ID), ('tag_rlite2', [ti[0] - 0.5, ti[1], ti[2]], ID),
]

m = md3.Model()
m.name, m.flags = P + 'body.md3', 0
m.surfs = [surfs[n].done() for n in ORDER if n in surfs]
m.tags = [[[n, list(o), [list(r) for r in a]] for n, o, a in tags]]
allp = [[c / 64 for c in v[:3]] for s in m.surfs for v in s.frames[0]]
bmn = [min(p[k] for p in allp) for k in range(3)]
bmx = [max(p[k] for p in allp) for k in range(3)]
rad = max(math.sqrt(sum(c * c for c in p)) for p in allp)
m.frames = [bmn + bmx + [0.0, 0.0, 0.0, rad, b'skyline']]
md3.save(m, os.path.join(OUT, 'body.md3'))
print('scale %.2f  body bounds %s %s radius %.1f' % (S, [round(c, 1) for c in bmn], [round(c, 1) for c in bmx], rad))
for n, o, a in tags:
    print('  %-12s %s' % (n, [round(c, 2) for c in o]))

# ---------------- wheel -------------------------------------------------------------
# The R34 tyre at full detail. Its sidewalls and a dished disc in the rim opening
# get the planar mapping of the shared rim textures (w_rim), the tread the
# cylindrical mapping of tread.tga (w_tread), like the other cars' wheels, so the
# rim selection keeps working. The modelled 6-spoke rim is not used: the rim
# textures paint their own spokes.
def to_wheel(p):
    # model x = out of the car (left wheel), y = forward, z = down (see WL)
    return ((p[0] - W_C[0]) * S, (p[2] - W_C[2]) * S, -(p[1] - W_C[1]) * S)


def wn(n):
    return (n[0], n[2], -n[1])


tread = Builder('w_tread', 'models/players/wheels/tread.tga')
rim = Builder('w_rim', 'models/players/wheels/rim_mm_m11_2.tga')
R = W_R


def rim_uv(p):
    return [0.5 - p[1] / (2 * R), 0.5 + p[2] / (2 * R)]


tire_tris = [t for (o, m), tris in G if o == WHEEL_SRC and m == TIRE_MTL for t in tris]
wx = [to_wheel(V[c[0]])[0] for t in tire_tris for c in t]
XMIN, XMAX = min(wx), max(wx)
open_out = open_in = R
for t in tire_tris:
    pts = [to_wheel(V[c[0]]) for c in t]
    ns = [wn(VN[c[2]]) for c in t]
    fn = tri_normal(pts)
    if abs(fn[0]) < 0.6:                          # tread and shoulders
        ang = [math.atan2(p[2], p[1]) for p in pts]
        for k in (1, 2):                          # keep the triangle on one branch
            while ang[k] - ang[0] > math.pi:
                ang[k] -= 2 * math.pi
            while ang[k] - ang[0] < -math.pi:
                ang[k] += 2 * math.pi
        # tread.tga repeats twice around the wheel; u runs with the angle
        uvs = [[a / math.pi + 1.0, (p[0] - XMIN) / (XMAX - XMIN)] for a, p in zip(ang, pts)]
        tread.tri([tread.vert(p, n, uv) for p, n, uv in zip(pts, ns, uvs)])
    else:                                         # sidewalls
        rim.tri([rim.vert(p, n, rim_uv(p)) for p, n in zip(pts, ns)])
        for p in pts:
            r = math.hypot(p[1], p[2])
            if p[0] > 0:
                open_out = min(open_out, r)
            else:
                open_in = min(open_in, r)


def disc(x_edge, r_edge, depth, outward):
    """fine dished disc closing the rim opening; outward = +1 (outer face) or -1"""
    SEG, RINGS = 96, 6
    n_face = (outward, 0.0, 0.0)
    rings = []
    for i in range(RINGS + 1):
        f = 1 - i / RINGS                          # 1 at the tyre, 0 in the centre
        r = (r_edge + 0.15) * f
        x = x_edge - outward * depth * (1 - f * f)
        rings.append([(x, r * math.cos(2 * math.pi * j / SEG), r * math.sin(2 * math.pi * j / SEG))
                      for j in range(SEG)])
    for i in range(RINGS):
        for j in range(SEG):
            a, b = rings[i][j], rings[i][(j + 1) % SEG]
            c, d = rings[i + 1][j], rings[i + 1][(j + 1) % SEG]
            quad = [(a, b, d), (a, d, c)] if outward > 0 else [(a, d, b), (a, c, d)]
            for tri in quad:
                if tri_normal(tri)[0] * outward < 0:
                    tri = (tri[0], tri[2], tri[1])
                rim.tri([rim.vert(p, n_face, rim_uv(p)) for p in tri])


x_out = max(to_wheel(V[c[0]])[0] for t in tire_tris for c in t
            if math.hypot(*to_wheel(V[c[0]])[1:]) < open_out + 0.3)
x_in = min(to_wheel(V[c[0]])[0] for t in tire_tris for c in t
           if math.hypot(*to_wheel(V[c[0]])[1:]) < open_in + 0.3)
disc(x_out, open_out, 1.2, +1)
disc(x_in, open_in, 0.0, -1)

w = md3.Model()
w.name, w.flags = P + 'wheel.md3', 0
w.surfs = [tread.done(), rim.done()]
w.tags = []
wp = [[c / 64 for c in v[:3]] for s in w.surfs for v in s.frames[0]]
wmn = [min(p[k] for p in wp) for k in range(3)]
wmx = [max(p[k] for p in wp) for k in range(3)]
w.frames = [wmn + wmx + [0.0, 0.0, 0.0, max(math.sqrt(sum(c * c for c in p)) for p in wp), b'wheel']]
md3.save(w, os.path.join(OUT, 'wheel.md3'))
print('wheel radius %.2f, opening out %.2f in %.2f, bounds %s %s'
      % (R, open_out, open_in, [round(c, 2) for c in wmn], [round(c, 2) for c in wmx]))
