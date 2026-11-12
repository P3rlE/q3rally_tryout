#!/usr/bin/env python3
"""Generate q3r_timberrun (Timber Run): an A to B gravel sprint through an autumn forest.

From a timber yard on the ridge down into a creek valley and up again to a
meadow, early in the morning, about 48,000 units (roughly 90 seconds):

  1. start in the timber yard,
  2. the forest road: fast, gentle bends, two small jump crests,
  3. three hairpins down the slope,
  4. the creek valley with a ford (shallow water, wet grip),
  5. the spruce thicket: narrow, bend after bend,
  6. the clear-cut: wide and open, long fast bends,
  7. the sunken lane: uphill between earth banks,
  8. the big jump crest and the meadow,
  9. the finish.

The ground is one height field (as in Riviera Run): rolling forest hills that
are cut and filled to the road. Ground layers are blended per vertex with a
q3map2 index map; collision is closed triangular detail brushes on a
64/128/256/512 grid, fine only near the road and the creek. Invisible walls
stand a verge away on both sides of the road.

Stage 1: the drivable shell (ground, road, crests, ford, walls, start, finish,
checkpoints, bot route). The forest follows in stage 2.

Writes the .map, index map, shader script, arena, bot route, intro camera
route, the generated textures, an overview image and
docs/maps/timberrun/validation.json. Run with Python 3 + Pillow + numpy.
"""
from __future__ import annotations

import json
import math
import random
from itertools import combinations
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
NAME = "q3r_timberrun"
TITLE = "Timber Run"
TEX = "q3r_timberrun/"
DETAIL = 134217728
STEP = 64
FLOOR = 384
SAMPLE = 32
SHOULDER = 30           # soft verge beside the gravel
CUT, FILL = 0.6, 0.6     # side slopes of the cuttings and embankments
START_AT, FINISH_AT = 900, 2300     # units from either end of the centre line (a long run-out after the finish)
SCALE = 0.74            # the sketch below was drawn larger

# centre line control points with the road height there: (x, y, z), start (A) to finish (B),
# x east, y north, in sketch units (times SCALE)
SKETCH = [
    (-15000, 5000, 1200), (-14200, 5050, 1200),                                              # timber yard
    (-13000, 5300, 1195), (-11500, 5700, 1185), (-9800, 5300, 1175), (-8000, 5800, 1165),    # forest road
    (-6200, 5400, 1150),
    (-4800, 4900, 1125), (-4100, 4000, 1085), (-4700, 3250, 1045), (-6000, 3000, 1005),      # hairpins
    (-7200, 2600, 960), (-7500, 1800, 915), (-6800, 1200, 875), (-5400, 1050, 835),
    (-4200, 700, 795), (-3700, -100, 760), (-4300, -750, 730),
    (-5700, -1000, 705), (-7300, -1300, 685), (-8900, -1900, 665), (-9900, -2900, 650),      # creek valley
    (-10100, -3900, 640), (-9600, -4900, 655),                                               # (the ford)
    (-8600, -5600, 675), (-7400, -5200, 690), (-6300, -5900, 680), (-5100, -5400, 705),      # spruce thicket
    (-3900, -6100, 695), (-2600, -5600, 725), (-1400, -6200, 740),
    (500, -6300, 770), (2800, -5900, 810), (4800, -4800, 850), (6000, -3000, 890),           # clear-cut
    (6300, -1000, 930),
    (6200, 600, 965), (6900, 2000, 1010), (8100, 3000, 1050), (9300, 4000, 1085),            # sunken lane
    (10800, 4700, 1110), (12400, 4500, 1120), (13600, 3600, 1085), (14300, 2300, 1050),      # crest, meadow
    (14600, 900, 1035), (14700, -300, 1030),                                                 # finish
    (14650, -1500, 1026), (14450, -2700, 1024)]                                              # run-out
CTRL = [(x * SCALE, y * SCALE, z) for x, y, z in SKETCH]
C_YARD, C_ROAD, C_HAIRPINS, C_CREEK, C_FORD, C_THICKET, C_CLEAR, C_HOLLOW, C_CREST, C_FINISH = \
    0, 2, 7, 18, 22, 24, 31, 36, 40, 44

# keyframes along the course by position (fraction of its length without the run-out)
HALF_WIDTH = [(0.00, 260), (0.03, 240), (0.06, 180), (0.16, 175), (0.20, 170), (0.36, 170), (0.41, 185),
              (0.47, 160), (0.50, 152), (0.62, 152), (0.66, 190), (0.79, 188), (0.82, 152), (0.90, 152),
              (0.93, 185), (0.98, 200), (1.00, 220), (1.02, 260), (1.05, 300)]

# jump crests: (section, position in it, grade up, grade down, height)
CRESTS = (("road", 0.34, 0.07, 0.11, 28), ("road", 0.78, 0.07, 0.11, 28), ("crest", 0.30, 0.10, 0.16, 60))
FOOT = 200              # the feet of a crest are rounded over this length, its top is not

LAYERS = ("forest", "gravel", "grass", "rock", "cliff", "mud")
L_FOREST, L_GRAVEL, L_GRASS, L_ROCK, L_CLIFF, L_MUD = range(6)
FRICTION = {L_FOREST: "dirt", L_GRAVEL: "gravel", L_GRASS: "grass", L_ROCK: "rock", L_CLIFF: "rock",
            L_MUD: "wet"}

PATH, S, TAN, NOR, KAPPA, ZTAB = [], [], [], [], [], []
CTRL_S = []
GRID, MAT, LEAVES, LEAFOF, TRIS = {}, {}, [], {}, []
BRUSHES, GROUPS, ENTITIES, SOLIDS, TRIGGERS = [], {}, [], [], []
BOUNDS = {}
SECTIONS = {}       # name -> (first sample, last sample)
HOLLOW, OPEN = {}, {}               # sample -> depth of the sunken lane; sample -> how open the land is
FORD = {}                           # sample -> 0..1 how much it belongs to the ford
CREEK = []                          # centre line of the creek
COVERED = {-1: set(), 1: set()}     # side -> samples that already have a wall on that side
PLACED = {}                         # side -> sample -> lateral distance of the wall
CREST_AT = []                       # samples of the crest tops


def smooth(a, b, x):
    t = min(1.0, max(0.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def keyed(keys, f):
    for (f0, v0), (f1, v1) in zip(keys, keys[1:]):
        if f <= f1:
            return v0 + (v1 - v0) * smooth(f0, f1, f)
    return keys[-1][1]


# ---------------------------------------------------------------- course

def build_path():
    pts2 = [(p[0], p[1]) for p in CTRL]
    pts = [pts2[0]] + pts2 + [pts2[-1]]
    dense, marks = [], []
    for i in range(1, len(pts) - 2):
        p0, p1, p2, p3 = pts[i - 1], pts[i], pts[i + 1], pts[i + 2]
        marks.append(len(dense))
        n = max(4, int(math.dist(p1, p2) / 8))
        for k in range(n):
            t = k / n
            dense.append(tuple(0.5 * ((2 * p1[j]) + (-p0[j] + p2[j]) * t
                                      + (2 * p0[j] - 5 * p1[j] + 4 * p2[j] - p3[j]) * t * t
                                      + (-p0[j] + 3 * p1[j] - 3 * p2[j] + p3[j]) * t ** 3)
                               for j in range(2)))
    marks.append(len(dense))
    dense.append(pts2[-1])
    # arc length of the dense polyline, the control points' positions on it
    acc_d = [0.0]
    for a, b in zip(dense, dense[1:]):
        acc_d.append(acc_d[-1] + math.dist(a, b))
    CTRL_S.extend(acc_d[m] for m in marks)
    PATH.append(dense[0])
    acc = 0.0
    for a, b in zip(dense, dense[1:]):
        seg = math.dist(a, b)
        while acc + seg >= SAMPLE:
            t = (SAMPLE - acc) / seg
            a = (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t)
            seg = math.dist(a, b)
            PATH.append(a)
            acc = 0.0
        acc += seg
    n = len(PATH)
    for i, p in enumerate(PATH):
        S.append(i * SAMPLE)
        q0, q1 = PATH[max(0, i - 1)], PATH[min(n - 1, i + 1)]
        d = math.dist(q0, q1)
        t = ((q1[0] - q0[0]) / d, (q1[1] - q0[1]) / d)
        TAN.append(t)
        NOR.append((-t[1], t[0]))                     # left of the driving direction
    heading = [math.atan2(t[1], t[0]) for t in TAN]
    raw = []
    for i in range(n):
        a, b = heading[max(0, i - 1)], heading[min(n - 1, i + 1)]
        db = (b - a + math.pi) % (2 * math.pi) - math.pi
        raw.append(db / (2 * SAMPLE))
    w = 8
    for i in range(n):
        part = raw[max(0, i - w): i + w + 1]
        KAPPA.append(sum(part) / len(part))


def length():
    return S[-1]


def frac(s):
    """Position as a fraction of the course without the run-out (1 = the old
    end of the centre line), so the run-out leaves all widths as they were."""
    return min(1.05, max(0.0, s / CTRL_S[C_FINISH + 1]))


def half_width(s):
    return keyed(HALF_WIDTH, frac(s))


def foot(t):
    """0 below 0, straight above FOOT, a parabola between."""
    if t <= 0:
        return 0.0
    return t * t / (2 * FOOT) if t < FOOT else t - FOOT / 2


def build_profile():
    """Road height per sample: the control point heights joined linearly over
    the arc length and smoothed over +-640 units; then the ford dip and the
    jump crests on top (a crest keeps its sharp top, so cars take off)."""
    n = len(PATH)
    zs = [p[2] for p in CTRL]
    lin = []
    for i in range(n):
        s = S[i]
        k = 0
        while k < len(CTRL_S) - 2 and CTRL_S[k + 1] < s:
            k += 1
        s0, s1 = CTRL_S[k], CTRL_S[k + 1]
        t = min(1.0, max(0.0, (s - s0) / max(1.0, s1 - s0)))
        lin.append(zs[k] + (zs[k + 1] - zs[k]) * t)
    w = 20
    for i in range(n):
        part = lin[max(0, i - w): i + w + 1]
        ZTAB.append(sum(part) / len(part))
    # the ford: the lowest point of the course, a shallow dip under the water
    f = min(range(*SECTIONS["creek"]), key=lambda i: ZTAB[i])
    for i in range(n):
        d = math.exp(-((S[i] - S[f]) / 170) ** 2)
        ZTAB[i] -= 16 * d
        if d > 0.02:
            FORD[i] = d
    BOUNDS.update(i_ford=f, water_z=round(ZTAB[f] + 12))
    # crests: where the road is straightest near the wanted place
    for sec, pos, up, down, h in CRESTS:
        a, b = SECTIONS[sec]
        want = int(a + (b - a) * pos)
        top = min(range(want - 12, want + 13), key=lambda i: max(abs(k) for k in KAPPA[i - 8: i + 9]))
        l_up, l_down = h / up + FOOT / 2, h / down + FOOT / 2
        for i in range(n):
            ds = S[i] - S[top]
            if -l_up < ds < l_down:
                ZTAB[i] += min(up * foot(ds + l_up), down * foot(l_down - ds))
        CREST_AT.append(top)


def road_z(s):
    n = len(PATH)
    u = min(n - 1.0, max(0.0, s / SAMPLE))
    i = min(n - 2, int(u))
    t = u - i
    return ZTAB[i] * (1 - t) + ZTAB[i + 1] * t




def bank(i):
    """Cross fall in the bends; none in the ford (the water is level)."""
    return max(-0.08, min(0.08, -120 * KAPPA[i])) * (1 - FORD.get(i, 0.0))


def idx(i):
    return max(0, min(len(PATH) - 1, i))


def sample_at(x, y):
    """Nearest sample to a point (used to find the sections from the control points)."""
    return min(range(len(PATH)), key=lambda i: (PATH[i][0] - x) ** 2 + (PATH[i][1] - y) ** 2)


def sections():
    c = [sample_at(*p[:2]) for p in CTRL]
    last = len(PATH) - 1
    SECTIONS.update(yard=(0, c[C_ROAD]), road=(c[C_ROAD], c[C_HAIRPINS]), hairpins=(c[C_HAIRPINS], c[C_CREEK]),
                    creek=(c[C_CREEK], c[C_THICKET]), thicket=(c[C_THICKET], c[C_CLEAR]),
                    clearcut=(c[C_CLEAR], c[C_HOLLOW]), hollow=(c[C_HOLLOW], c[C_CREST]),
                    crest=(c[C_CREST], c[C_FINISH]), finish=(c[C_FINISH], last))
    h0, h1 = SECTIONS["hollow"]
    for i in range(h0, h1 + 1):
        HOLLOW[i] = smooth(h0, h0 + 24, i) * (1 - smooth(h1 - 24, h1, i))
    for a, b in ((-40, SECTIONS["yard"][1]), (SECTIONS["clearcut"][0] + 10, SECTIONS["clearcut"][1] - 10),
                 (SECTIONS["crest"][0] + 10, last + 40)):
        for i in range(max(0, a), min(last, b) + 1):
            OPEN[i] = smooth(a, a + 30, i) * (1 - smooth(b - 30, b, i))


# ---------------------------------------------------------------- spatial lookups

BUCKET = 512
HASH = {}


def hash_path():
    for i, (x, y) in enumerate(PATH):
        HASH.setdefault((int(x // BUCKET), int(y // BUCKET)), []).append(i)


def nearby(x, y, radius):
    r = int(radius // BUCKET) + 1
    bx, by = int(x // BUCKET), int(y // BUCKET)
    for gx in range(bx - r, bx + r + 1):
        for gy in range(by - r, by + r + 1):
            yield from HASH.get((gx, gy), ())


def leg_nearest(x, y, radius):
    """The closest centre line sample of every part of the course in reach."""
    cand = sorted(i for i in nearby(x, y, radius)
                  if abs(x - PATH[i][0]) <= radius and abs(y - PATH[i][1]) <= radius)
    groups, prev = [], -10
    for i in cand:
        if i - prev > 2:
            groups.append([])
        groups[-1].append(i)
        prev = i
    return [min(g, key=lambda i: (x - PATH[i][0]) ** 2 + (y - PATH[i][1]) ** 2) for g in groups]


def nearest(x, y, radius=2000):
    best, bi = 1e18, 0
    for i in nearby(x, y, radius):
        d = (x - PATH[i][0]) ** 2 + (y - PATH[i][1]) ** 2
        if d < best:
            best, bi = d, i
    return math.sqrt(best), bi


def local(i, x, y):
    """Continuous position along the course, signed lateral offset (left +).
    Beyond the ends the road ends in a round head."""
    px, py = PATH[i]
    dx, dy = x - px, y - py
    along = dx * TAN[i][0] + dy * TAN[i][1]
    lat = dx * NOR[i][0] + dy * NOR[i][1]
    if (i == 0 and along < 0) or (i == len(PATH) - 1 and along > 0):
        return S[i], math.copysign(math.hypot(lat, along), lat or 1)
    s = S[i] + max(-SAMPLE / 2, min(SAMPLE / 2, along))
    return s, lat


# ---------------------------------------------------------------- landscape

CREEK_W = 220           # half width of the creek channel
CREEK_BOX = []          # x0, y0, x1, y1 of the water


def build_creek():
    """The creek crosses the road at the ford, at a right angle, and winds
    away to both sides until another part of the course comes near."""
    f = BOUNDS["i_ford"]
    (fx, fy), t, n = PATH[f], TAN[f], NOR[f]

    def point(u):
        w = 150 * math.sin(u / 260) * smooth(0, 500, abs(u))
        return fx + n[0] * u + t[0] * w, fy + n[1] * u + t[1] * w
    ends = []
    for sign in (-1, 1):
        u = 0
        while abs(u) < 5200:
            x, y = point(u + sign * 100)
            if any(abs(j - f) > 60 and math.dist((x, y), PATH[j]) < 1100 for j in nearby(x, y, 1100)):
                break
            u += sign * 100
        ends.append(u)
    CREEK.extend(point(u) for u in range(ends[0], ends[1] + 1, 100))
    xs, ys = [p[0] for p in CREEK], [p[1] for p in CREEK]
    m = CREEK_W + 40
    CREEK_BOX.extend((math.floor(min(xs) - m), math.floor(min(ys) - m), math.ceil(max(xs) + m), math.ceil(max(ys) + m)))


def creek_distance(x, y):
    x0, y0, x1, y1 = CREEK_BOX
    if not (x0 - 700 < x < x1 + 700 and y0 - 700 < y < y1 + 700):
        return 1e9
    best = 1e18
    for (ax, ay), (bx, by) in zip(CREEK, CREEK[1:]):
        dx, dy = bx - ax, by - ay
        t = max(0.0, min(1.0, ((x - ax) * dx + (y - ay) * dy) / (dx * dx + dy * dy)))
        best = min(best, (x - ax - dx * t) ** 2 + (y - ay - dy * t) ** 2)
    return math.sqrt(best)


def creek_shape(x, y, z):
    """Ground with the creek cut into it: a channel along the creek, and no
    ground below the water anywhere else under the water sheet."""
    x0, y0, x1, y1 = CREEK_BOX
    out = max(x0 - x, x - x1, y0 - y, y - y1, 0.0)
    if out > 400:
        return z
    bank_z = BOUNDS["water_z"] + 25
    z = max(z, bank_z - 0.5 * out)
    dc = creek_distance(x, y)
    if dc < CREEK_W:
        bed = BOUNDS["water_z"] - 26
        z = min(z, bed + (bank_z - bed) * (dc / CREEK_W) ** 2)
    return z


def macro(x, y):
    """Height of the land without the road: the heights of the course spread
    over the land, with hills on top."""
    acc = wsum = 0.0
    for cx, cy, cz in CTRL:
        w = ((x - cx) ** 2 + (y - cy) ** 2 + 1.44e6) ** -1.5
        acc += w * cz
        wsum += w
    h = acc / wsum
    h += 70 * math.sin(x / 900 + 0.3) * math.cos(y / 700) + 32 * math.cos(x / 310) * math.sin(y / 270)
    return h


def road_bounds(i, x, y):
    """(lower bound, upper bound, distance beyond the road edge) for one leg."""
    s, dsig = local(i, x, y)
    d = abs(dsig)
    hw = half_width(s)
    h = road_z(s)
    ze = h + bank(i) * max(-hw - SHOULDER, min(hw + SHOULDER, dsig))
    if d <= hw + SHOULDER:
        return ze, ze, d - hw
    e = d - hw - SHOULDER
    # the sunken lane: earth banks on both sides, whatever the land does
    low = ze + min(e * 0.85, 150) * HOLLOW[i] if i in HOLLOW else ze - e * FILL
    return low, ze + e * max(CUT, 1.0 * HOLLOW.get(i, 0)), d - hw


def base(x, y, legs):
    m = macro(x, y)
    if not legs:
        return m + 112
    acc = wsum = 0.0
    pull = 0.0
    near = 1e9
    opn = 0.0
    for i in legs:
        s, dsig = local(i, x, y)
        e = max(0.0, abs(dsig) - half_width(s))
        w = math.exp(-(e / 380) ** 2) + 1e-9
        acc += w * road_z(s)
        wsum += w
        pull = max(pull, math.exp(-(e / 520) ** 2))
        opn += w * OPEN.get(i, 0.0)
        near = min(near, e)
    # the land rises a little away from the road and closes the view; less so
    # in the open (yard, clear-cut, meadow)
    m += 0.09 * min(1250.0, max(0.0, near - 250)) * (1 - 0.7 * opn / wsum)
    return m + (acc / wsum - m) * pull


def height(x, y):
    legs = leg_nearest(x, y, 1500)
    z = creek_shape(x, y, base(x, y, legs))
    lo, hi = -1e9, 1e9
    for lo_i, hi_i, _ in sorted((road_bounds(i, x, y) for i in legs), key=lambda c: c[2]):
        nl, nh = max(lo, lo_i), min(hi, hi_i)
        if nl <= nh:
            lo, hi = nl, nh
    return min(max(z, lo), hi)


# ---------------------------------------------------------------- terrain mesh

def build_terrain():
    xs = [p[0] for p in PATH]
    ys = [p[1] for p in PATH]
    x0 = math.floor((min(xs) - 2400) / 512) * 512
    x1 = math.ceil((max(xs) + 2400) / 512) * 512
    y0 = math.floor((min(ys) - 2400) / 512) * 512
    y1 = math.ceil((max(ys) + 2400) / 512) * 512
    BOUNDS.update(x0=x0, x1=x1, y0=y0, y1=y1)
    edge, fine, owner = {}, {}, {}
    for y in range(y0, y1 + 1, STEP):
        for x in range(x0, x1 + 1, STEP):
            GRID[x, y] = height(x, y)
            d, i = nearest(x, y, 2400)
            edge[x, y] = d - half_width(S[i]) if d < 1e8 else 1e9
            owner[x, y] = i
            fine[x, y] = min(edge[x, y], creek_distance(x, y) + 170)
    raw = dict(GRID)

    def split(cx, cy, size):
        pts = [(cx + a * STEP, cy + b * STEP) for a in range(size // STEP + 1) for b in range(size // STEP + 1)]
        near = min(fine[p] for p in pts)
        limit = {512: 2000, 256: 650, 128: 160}.get(size)
        if size == STEP or near > limit:
            LEAVES.append((cx, cy, size))
            return
        h = size // 2
        for qy in (cy, cy + h):
            for qx in (cx, cx + h):
                split(qx, qy, h)
    for cy in range(y0, y1, 512):
        for cx in range(x0, x1, 512):
            split(cx, cy, 512)
    for (lx, ly, size) in sorted(LEAVES, key=lambda c: -c[2]):
        for (ax, ay), (bx, by) in (((lx, ly), (lx + size, ly)), ((lx + size, ly), (lx + size, ly + size)),
                                   ((lx, ly + size), (lx + size, ly + size)), ((lx, ly), (lx, ly + size))):
            n = size // STEP
            for k in range(1, n):
                GRID[ax + (bx - ax) * k // n, ay + (by - ay) * k // n] = \
                    GRID[ax, ay] + (GRID[bx, by] - GRID[ax, ay]) * k / n
    for k in GRID:
        GRID[k] = round(GRID[k] * 8) / 8
    for (x, y), z in raw.items():
        gx = (raw.get((x + STEP, y), z) - raw.get((x - STEP, y), z)) / (2 * STEP)
        gy = (raw.get((x, y + STEP), z) - raw.get((x, y - STEP), z)) / (2 * STEP)
        slope = math.hypot(gx, gy)
        e = edge[x, y]
        if e < SHOULDER + 8:
            m = L_MUD if z < BOUNDS["water_z"] + 3 else L_GRAVEL
        elif z < BOUNDS["water_z"] + 8 and creek_distance(x, y) < CREEK_W + 80:
            m = L_MUD
        elif slope > 1.4:
            m = L_CLIFF
        elif slope > 1.05:
            m = L_ROCK
        elif OPEN.get(owner[x, y], 0) > 0.5 and e < 1700:
            patch = math.cos(x / 410) * math.cos(y / 370) + 0.5 * math.cos(x / 157) * math.cos(y / 181)
            m = L_FOREST if patch > 0.9 else L_GRASS
        else:
            m = L_FOREST
        MAT[x, y] = m
    order = ("rock", "dirt", "grass", "gravel", "wet")
    for (lx, ly, size) in LEAVES:
        a, b = (lx, ly, GRID[lx, ly]), (lx + size, ly, GRID[lx + size, ly])
        c, d = (lx + size, ly + size, GRID[lx + size, ly + size]), (lx, ly + size, GRID[lx, ly + size])
        hc = height(lx + size / 2, ly + size / 2) if size == STEP else raw[lx + size // 2, ly + size // 2]
        ac, bd = abs((a[2] + c[2]) / 2 - hc), abs((b[2] + d[2]) / 2 - hc)
        diag = "ac" if ac <= bd else "bd"
        for fx in range(lx, lx + size, STEP):
            for fy in range(ly, ly + size, STEP):
                LEAFOF[fx, fy] = (lx, ly, size, diag)
        tris = ((a, b, c), (a, c, d)) if diag == "ac" else ((a, b, d), (b, c, d))
        for tri in tris:
            TRIS.append(tri)
            classes = [FRICTION[MAT[p[0], p[1]]] for p in tri]
            major = max(set(classes), key=lambda k: (classes.count(k), -order.index(k)))
            for road in ("wet", "gravel"):
                if classes.count(road) >= 2:
                    major = road
                    break
            prism(tri, TEX + "t_" + major, group=major)


def cell_height(x0, y0, size, diag, x, y):
    u, v = (x - x0) / size, (y - y0) / size
    a, b = GRID[x0, y0], GRID[x0 + size, y0]
    c, d = GRID[x0 + size, y0 + size], GRID[x0, y0 + size]
    if diag == "ac":
        if v <= u:
            return a + (b - a) * u + (c - b) * v
        return a + (c - d) * u + (d - a) * v
    if u + v <= 1:
        return a + (b - a) * u + (d - a) * v
    return c + (d - c) * (1 - u) + (b - c) * (1 - v)


def ground(x, y):
    fx = BOUNDS["x0"] + math.floor((x - BOUNDS["x0"]) / STEP) * STEP
    fy = BOUNDS["y0"] + math.floor((y - BOUNDS["y0"]) / STEP) * STEP
    lx, ly, size, diag = LEAFOF[fx, fy]
    return cell_height(lx, ly, size, diag, x, y)


# ---------------------------------------------------------------- brushes

def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def plane(normal, point):
    up = (1, 0, 0) if abs(normal[0]) < 1e-9 and abs(normal[1]) < 1e-9 else (0, 0, 1)
    t1 = cross(up, normal)
    t2 = cross(normal, t1)
    size = max(abs(c) for c in t1 + t2)
    return (point, tuple(p + c * 128 / size for p, c in zip(point, t2)),
            tuple(p + c * 128 / size for p, c in zip(point, t1)))


def num(v):
    s = f"{v:.4f}".rstrip("0").rstrip(".")
    return s if s not in ("", "-0") else "0"


def face(points, material, scale=0.5, detail=True, tex=None):
    """tex: (x offset, y offset, x scale, y scale) instead of the plain scale."""
    p = " ".join("( %s )" % " ".join(num(v) for v in xyz) for xyz in points)
    xo, yo, ss, ts = tex or (0, 0, scale, scale)
    return f"{p} {material} {num(xo)} {num(yo)} 0 {num(ss)} {num(ts)} {DETAIL if detail else 0} 0 0"


def brush(faces, group=None):
    text = "{\n" + "\n".join(faces) + "\n}"
    (BRUSHES if group is None else GROUPS.setdefault(group, [])).append(text)


def prism(vertices, material, group=None):
    a, b, c = vertices
    faces = [face((a, c, b), material, scale=1),
             face(plane((0, 0, -1), (0, 0, FLOOR)), "common/caulk")]
    for p, q in ((a, b), (b, c), (c, a)):
        faces.append(face(((p[0], p[1], FLOOR), p, q), "common/caulk"))
    brush(faces, group)


def polytope(planes):
    eq = [(n, sum(a * b for a, b in zip(n, p))) for n, p in planes]
    out = []
    for i in range(len(eq)):
        for j in range(i + 1, len(eq)):
            for k in range(j + 1, len(eq)):
                (n1, d1), (n2, d2), (n3, d3) = eq[i], eq[j], eq[k]
                c23, c31, c12 = cross(n2, n3), cross(n3, n1), cross(n1, n2)
                det = sum(a * b for a, b in zip(n1, c23))
                if abs(det) < 1e-9:
                    continue
                p = tuple((d1 * c23[m] + d2 * c31[m] + d3 * c12[m]) / det for m in range(3))
                if all(sum(a * b for a, b in zip(n, p)) <= d + 0.01 for n, d in eq):
                    out.append(p)
    return out


def convex(planes, solid=True, scale=0.5, detail=True):
    """(outward normal, point, material[, texture fit]) planes."""
    brush([face(plane(q[0], q[1]), q[2], scale=scale, detail=detail, tex=q[3] if len(q) > 3 else None)
           for q in planes])
    if solid:
        corners = polytope([(q[0], q[1]) for q in planes])
        if corners:
            SOLIDS.append((tuple(min(c[i] for c in corners) for i in range(3)),
                           tuple(max(c[i] for c in corners) for i in range(3))))


def obox(cx, cy, t, along, across, z0, z1, mat, side=None, solid=True, top=None, bottom=None, scale=0.5):
    """Box turned to the direction t (unit), centred on cx, cy."""
    n = (-t[1], t[0])
    side = side or mat
    convex([((t[0], t[1], 0), (cx + t[0] * along, cy + t[1] * along, z0), side),
            ((-t[0], -t[1], 0), (cx - t[0] * along, cy - t[1] * along, z0), side),
            ((n[0], n[1], 0), (cx + n[0] * across, cy + n[1] * across, z0), mat),
            ((-n[0], -n[1], 0), (cx - n[0] * across, cy - n[1] * across, z0), mat),
            ((0, 0, 1), (cx, cy, z1), top or side), ((0, 0, -1), (cx, cy, z0), bottom or side)],
           solid=solid, scale=scale)


def hexa(corners, mats, solid=True, scale=0.5, texfit=None):
    """Convex 8-corner block: corners 0-3 one end (ring order), 4-7 the other.
    mats: one material or a dict per face index (0 end a, 1 end b, 2-5 sides).
    texfit: face index -> (x offset, y offset, x scale, y scale)."""
    c = [sum(p[i] for p in corners) / 8 for i in range(3)]
    quads = [(0, 1, 2, 3), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    planes = []
    for k, q in enumerate(quads):
        p0, p1, p2 = corners[q[0]], corners[q[1]], corners[q[2]]
        n = cross(tuple(b - a for a, b in zip(p0, p1)), tuple(b - a for a, b in zip(p0, p2)))
        if sum(nc * (pc - cc) for nc, pc, cc in zip(n, p0, c)) < 0:
            n = tuple(-v for v in n)
        m = mats.get(k, mats.get("default")) if isinstance(mats, dict) else mats
        planes.append((n, p0, m, (texfit or {}).get(k)))
    convex(planes, solid=solid, scale=scale)


def strip(i0, i1, lat0, lat1, z0, z1, mats, step=2, follow=True, solid=True, scale=0.5):
    """Blocks along the centre line from sample i0 to i1, lateral lat0..lat1
    (left +; either may be a function of the sample, so the block follows a
    changing road width). follow: heights relative to the road surface."""
    for k in range(i0, i1, step):
        a, b = idx(k), idx(min(k + step, i1))
        if a == b:
            continue
        corners = []
        for j in (a, b):
            (px, py), n = PATH[j], NOR[j]
            l0 = lat0(j) if callable(lat0) else lat0
            l1 = lat1(j) if callable(lat1) else lat1
            for lat, z in ((l0, z0), (l1, z0), (l1, z1), (l0, z1)):
                zz = z + (road_z(S[j]) + bank(j) * lat if follow else 0)
                corners.append((px + n[0] * lat, py + n[1] * lat, zz))
        hexa(corners, mats, solid=solid, scale=scale)


def boulder(x, y, radius, tall, seed, mat):
    z0 = min(ground(x + dx, y + dy) for dx in (-radius, 0, radius) for dy in (-radius, 0, radius))
    centre = (x, y, z0)
    planes = [((0, 0, -1), (x, y, z0 - 24), mat), ((0, 0, 1), (x, y, z0 + tall), mat)]
    rnd = abs(math.sin(seed * 12.9898) * 43758.5453) % 233280
    for k in range(14):
        rnd = (rnd * 9301 + 49297) % 233280
        jitter = rnd / 233280
        if k < 7:
            azimuth = (k + 0.4 * jitter) * 2 * math.pi / 7 + seed
            elevation = math.radians(4 + 10 * jitter)
            reach = radius * (0.85 + 0.25 * jitter)
        else:
            azimuth = (k - 7 + 0.5 * jitter) * 2 * math.pi / 7 + seed + 0.3
            elevation = math.radians(30 + 30 * ((k * 7 + int(seed * 3)) % 5) / 4)
            reach = radius * (0.7 + 0.25 * jitter)
        n = (math.cos(azimuth) * math.cos(elevation), math.sin(azimuth) * math.cos(elevation), math.sin(elevation))
        reach = reach * (1 - n[2]) + tall * 0.92 * n[2]
        planes.append((n, tuple(c + nc * reach for c, nc in zip(centre, n)), mat))
    convex(planes)


def box(lo, hi, material, collision=True, faces=None, scale=0.5):
    x1, y1, z1 = lo
    x2, y2, z2 = hi
    assert x2 > x1 and y2 > y1 and z2 > z1, (lo, hi)
    keys = ("bot", "top", "-x", "+x", "-y", "+y")
    normals = ((0, 0, -1), (0, 0, 1), (-1, 0, 0), (1, 0, 0), (0, -1, 0), (0, 1, 0))
    points = ((x1, y1, z1), (x2, y2, z2), (x1, y1, z1), (x2, y2, z2), (x1, y1, z1), (x2, y2, z2))
    brush([face(plane(n, p), (faces or {}).get(k, material), scale=scale) for k, n, p in zip(keys, normals, points)])
    if collision:
        SOLIDS.append((lo, hi))


def entity(classname, x, y, z, **keys):
    fields = {"classname": classname, "origin": f"{round(x)} {round(y)} {round(z)}", **keys}
    ENTITIES.append(fields)
    return fields


def heading_deg(i):
    return math.degrees(math.atan2(TAN[i][1], TAN[i][0])) % 360


def road_point(i, lat=0.0):
    (px, py), n = PATH[i], NOR[i]
    x, y = px + n[0] * lat, py + n[1] * lat
    return x, y, road_z(S[i]) + bank(i) * lat


def at(s):
    return idx(int(round(s / SAMPLE)))


def runs(indices):
    """Consecutive runs of sample numbers."""
    out = []
    for i in sorted(indices):
        if out and i == out[-1][1] + 1:
            out[-1][1] = i
        else:
            out.append([i, i])
    return out


LAND_GAP = 130      # verge between the road shoulder and the invisible wall


def end_walls():
    """Invisible walls across both ends of the course."""
    for i, back in ((0, -1), (len(PATH) - 1, 1)):
        (px, py), t = PATH[i], TAN[i]
        hw = half_width(S[i])
        x, y = px + t[0] * back * 140, py + t[1] * back * 140
        obox(x, y, t, 16, hw + 400, road_z(S[i]) - 200, road_z(S[i]) + 500, TEX + "boundary", solid=False)


def corridor():
    """Invisible walls on both sides of the road, a verge away, so nobody cuts
    across the slope between the hairpin legs. Where another leg is close the
    wall moves in to sit between the two; on the inside of a bend it stays
    short of the centre of it."""
    done = 0
    for side in (-1, 1):
        placed = {}
        for i in range(len(PATH)):
            if i in COVERED[side]:
                continue
            hw = half_width(S[i])
            gap = LAND_GAP
            while gap >= 10:
                lat = side * (hw + SHOULDER + gap)
                x, y, _ = road_point(i, lat)
                d, j = nearest(x, y, abs(lat) + 600)
                ok = abs(j - i) <= 10 and d >= abs(lat) - 30
                if ok:
                    for k in leg_nearest(x, y, 900):
                        if abs(k - i) > 10:
                            s, dsig = local(k, x, y)
                            if abs(dsig) - half_width(s) - SHOULDER < 30:
                                ok = False
                                break
                if ok:
                    placed[i] = hw + SHOULDER + gap
                    break
                gap -= 10
        PLACED[side] = placed
        for a, b in runs(placed):
            a0, b0 = max(0, a - 1), min(len(PATH) - 1, b + 1)       # overlap the neighbouring walls
            for k in range(a0, b0, 2):
                e = min(k + 2, b0)
                lat = side * min(placed.get(k, placed.get(k + 1, 0)), placed.get(e, placed.get(e - 1, 1e9)))
                strip(k, e, *sorted((lat, lat + side * 24)), -420, 420, TEX + "boundary", step=2, solid=False)
                done += 1
            for k, e in ((a0, a0 + 1), (b0 - 1, b0)):
                j = a if k == a0 else b
                hw = half_width(S[j])
                strip(k, e, *sorted((side * (hw + SHOULDER - 4), side * (placed[j] + 24))), -420, 420,
                      TEX + "boundary", step=1, solid=False)
        for i, back in ((0, -1), (len(PATH) - 1, 1)):
            lat = placed.get(i) or (half_width(S[i]) + SHOULDER)
            x, y, z = road_point(i, side * (lat + 12))
            t = TAN[i]
            obox(x + t[0] * back * 80, y + t[1] * back * 80, t, 88, 12, z - 420, z + 420, TEX + "boundary",
                 solid=False)
    BOUNDS["corridor_blocks"] = done
    BOUNDS["wall_gaps"] = [len(PATH) - len(PLACED[s]) - len(COVERED[s]) for s in (-1, 1)]


# ---------------------------------------------------------------- course objects

def trigger(classname, i, number=None, target=None):
    """Thin trigger across the road; it must not reach a neighbouring leg."""
    (px, py), t, n = PATH[i], TAN[i], NOR[i]
    hw = half_width(S[i])
    z = road_z(S[i])
    reach = hw + 200
    for j in nearby(px, py, 1400):
        if abs(j - i) * SAMPLE > 1500:
            dlat = (PATH[j][0] - px) * n[0] + (PATH[j][1] - py) * n[1]
            dalong = (PATH[j][0] - px) * t[0] + (PATH[j][1] - py) * t[1]
            if abs(dalong) < half_width(S[j]) + 80:
                reach = min(reach, abs(dlat) - half_width(S[j]) - 40)
    reach = max(reach, hw + 40)
    keys = {"classname": classname}
    if number is not None:
        keys["number"] = str(number)
    if target:
        keys["target"] = target
    tex = "common/checkpoint" if classname == "rally_checkpoint" else "common/startfinish"
    planes = [((t[0], t[1], 0), (px + t[0] * 24, py + t[1] * 24, z), tex),
              ((-t[0], -t[1], 0), (px - t[0] * 24, py - t[1] * 24, z), tex),
              ((n[0], n[1], 0), (px + n[0] * reach, py + n[1] * reach, z), tex),
              ((-n[0], -n[1], 0), (px - n[0] * reach, py - n[1] * reach, z), tex),
              ((0, 0, 1), (px, py, z + 400), tex),
              ((0, 0, -1), (px, py, z - 100), tex)]
    body = "\n".join(face(plane(nn, p), m, detail=False) for nn, p, m in planes)
    TRIGGERS.append("{\n" + "\n".join(f'"{k}" "{v}"' for k, v in keys.items()) + "\n{\n" + body + "\n}\n}")
    if target:
        x, y, zz = road_point(i)
        entity("info_notnull", x, y, zz + 40, targetname=target, angle=f"{heading_deg(i):.0f}")
    return reach


CHECKPOINTS = []
ROUTE_TOL = 64          # how far the bot route (Bezier between checkpoints) may stray from the centre line
ROUTE_SPAN = 64         # at most this many samples between two checkpoints
HANDLE_MAX = 900        # longest Bezier handle (it is shared by the segments either side)


def bez_point(i):
    x, y, z = road_point(i)
    return (x, y, z + 40)            # same as the info_notnull target the trigger moves to


def bez_handle(i, length):
    """Handle along the road at sample i (3D: it follows the gradient)."""
    j0, j1 = idx(i - 1), idx(i + 1)
    dz = (road_z(S[j1]) - road_z(S[j0])) / max(1.0, S[j1] - S[j0])
    return (TAN[i][0] * length, TAN[i][1] * length, dz * length)


def bez_error(a, b, ha, hb):
    """Largest gap between the cubic a..b (handles ha out of a, hb into b, as
    g_race_route.c builds it) and the centre line between the two samples."""
    import numpy as np
    p0, p3 = np.array(bez_point(a)[:2]), np.array(bez_point(b)[:2])
    c1, c2 = p0 + np.array(ha[:2]), p3 - np.array(hb[:2])
    n = max(24, (b - a) * 2)
    t = np.linspace(0, 1, n)[:, None]
    curve = ((1 - t) ** 3) * p0 + 3 * t * (1 - t) ** 2 * c1 + 3 * t * t * (1 - t) * c2 + t ** 3 * p3
    line = np.array(PATH[a:b + 1])
    d = np.sqrt(((curve[:, None, :] - line[None, :, :]) ** 2).sum(-1))
    return max(d.min(1).max(), d.min(0).max())


def plan_checkpoints(i0, i1):
    """Checkpoints where the bot route needs them: from the start line on, each
    one as far along as a single cubic (the BPD segment the bots drive) still
    follows the road within ROUTE_TOL. Returns [(sample, handle length)] and
    keeps the handles for write_bpd()."""
    def straight(a, b):
        pa, pb = bez_point(a), bez_point(b)
        return tuple((q - p) / 3 for p, q in zip(pa, pb))
    def candidates(prev, hprev):
        cands = []
        for j in range(prev + 6, min(i1 - 6, prev + ROUTE_SPAN) + 1, 2):
            chord = math.dist(bez_point(prev)[:2], bez_point(j)[:2])
            ho = straight(prev, j) if hprev is None else hprev
            for f in (0.15, 0.22, 0.3, 0.36, 0.42, 0.5, 0.6):
                length_j = min(chord * f, HANDLE_MAX)
                hj = bez_handle(j, length_j)
                if bez_error(prev, j, ho, hj) <= ROUTE_TOL:
                    cands.append((j, length_j, hj))
                    break
        return sorted(cands, key=lambda c: -c[0])

    def finishes(prev, hprev):
        h_out = straight(prev, i1) if hprev is None else hprev
        return i1 - prev <= ROUTE_SPAN and bez_error(prev, i1, h_out, straight(prev, i1)) <= ROUTE_TOL

    # depth first, farthest checkpoint first; back up when a handle leaves no way on
    stack = [(i0, None, candidates(i0, None))]
    chosen = []
    while True:
        prev, hprev, cands = stack[-1]
        if finishes(prev, hprev):
            break
        if not cands:
            stack.pop()
            if not stack:
                raise SystemExit("no checkpoint plan fits the road")
            chosen.pop()
            continue
        j, length_j, hj = cands.pop(0)
        chosen.append((j, length_j, hj))
        stack.append((j, hj, candidates(j, hj)))
    ROUTE.extend((j, hj) for j, _, hj in chosen)
    return [(j, length_j) for j, length_j, _ in chosen]


ROUTE = []      # (sample, handle) per checkpoint, for the .bpd


def write_bpd():
    """maps/<name>.bpd: checkpoint number : point : handle (see g_rally_tools.c)."""
    lines = []
    for k, (i, h) in enumerate(ROUTE, 1):
        p = bez_point(i)
        lines.append(f"{k} : {p[0]:.3f} {p[1]:.3f} {p[2]:.3f} : {h[0]:.3f} {h[1]:.3f} {h[2]:.3f}")
    target = ROOT / "baseq3r" / "maps" / f"{NAME}.bpd"
    target.write_text("\n".join(lines) + "\n", encoding="ascii")


def course():
    """Start, checkpoints (one per section, more on long ones), finish, grid."""
    L = length()
    i0, i1 = at(START_AT), at(L - FINISH_AT)
    BOUNDS.update(i_start=i0, i_finish=i1)
    trigger("rally_start", i0, target="start")
    for k, (i, h) in enumerate(plan_checkpoints(i0, i1), 1):
        reach = trigger("rally_checkpoint", i, number=k, target=f"cp{k}")
        CHECKPOINTS.append({"number": k, "sample": i, "fraction": round(i / len(PATH), 3),
                            "half_reach": round(reach), "handle": round(h)})
    trigger("rally_finish", i1, target="finish")
    # start grid: 4 rows of 2, staggered, behind the line
    for row in range(4):
        for col, lat in enumerate((-110, 110)):
            j = idx(i0 - int((170 + row * 180 + col * 90) / SAMPLE))
            x, y, z = road_point(j, lat)
            entity("info_player_start", x, y, z + 48, angle=f"{heading_deg(j):.0f}")
    j = idx(i0 - int(880 / SAMPLE))
    x, y, z = road_point(j)
    entity("info_player_deathmatch", x, y, z + 48, angle=f"{heading_deg(j):.0f}")


BASE_AXES = (((0, 0, 1), (1, 0, 0), (0, -1, 0)), ((0, 0, -1), (1, 0, 0), (0, -1, 0)),
             ((1, 0, 0), (0, 1, 0), (0, 0, -1)), ((-1, 0, 0), (0, 1, 0), (0, 0, -1)),
             ((0, 1, 0), (1, 0, 0), (0, 0, -1)), ((0, -1, 0), (1, 0, 0), (0, 0, -1)))


def fitted(normal, left, top_z, width, height, img_w, img_h):
    """Shift/scale of the Q3 axial texture projection that fits an image
    unmirrored onto an upright face of any yaw."""
    best = max(BASE_AXES, key=lambda b: sum(x * y for x, y in zip(normal, b[0])))
    sv = best[1]
    right = (-normal[1], normal[0], 0.0)
    ss = width * sum(x * y for x, y in zip(right, sv)) / img_w
    dl = sum(x * y for x, y in zip(left, sv))
    ts = height / img_h
    return (-dl / ss) % img_w, (top_z / ts) % img_h, ss, ts


def gantry(i, banner):
    """Portal across the road: posts and a beam with the banner fitted on both
    faces (one copy across the road)."""
    (px, py), t, n = PATH[i], TAN[i], NOR[i]
    hw = half_width(S[i])
    z = road_z(S[i])
    for side in (-1, 1):
        cx, cy = px + n[0] * side * (hw + 60), py + n[1] * side * (hw + 60)
        obox(cx, cy, t, 18, 18, ground(cx, cy) - 40, z + 420, TEX + "metal")
    half, z0, z1 = hw + 78, z + 330, z + 420
    faces = []
    for f in (t, (-t[0], -t[1])):
        right = (-f[1], f[0])
        c = (px + f[0] * 12, py + f[1] * 12)
        left = (c[0] - right[0] * half, c[1] - right[1] * half, 0.0)
        xo, yo, ss, ts = fitted((f[0], f[1], 0.0), left, z1, 2 * half, z1 - z0, 512, 64)
        pts = " ".join("( %s )" % " ".join(num(v) for v in q) for q in plane((f[0], f[1], 0), (c[0], c[1], z0)))
        faces.append(f"{pts} {TEX + banner} {num(xo)} {num(yo)} 0 {num(ss)} {num(ts)} {DETAIL} 0 0")
    for nn, p in (((n[0], n[1], 0), (px + n[0] * half, py + n[1] * half, z0)),
                  ((-n[0], -n[1], 0), (px - n[0] * half, py - n[1] * half, z0)),
                  ((0, 0, 1), (px, py, z1)), ((0, 0, -1), (px, py, z0))):
        faces.append(face(plane(nn, p), TEX + "metal"))
    brush(faces)


SIGN_W, SIGN_H, SIGN_Z0 = 120, 60, 52     # board size and height of its lower edge above the road
SIGN_POST_Y, SIGN_POST_X = 38, -3.9        # posts behind the board


def sign_model(arrow):
    """Curve sign as a model: an aluminium board with rounded corners, a
    folded edge, two round posts with clamps behind it. Faces +x (the board
    front), y is across, z up; the origin is on the road surface."""
    md3 = load_md3()
    half, top, thick, rad = SIGN_W / 2, SIGN_Z0 + SIGN_H, 1.2, 6.0
    zc = SIGN_Z0 + SIGN_H / 2
    outline = []                                     # rounded rectangle, ccw seen from the front
    for cy, cz, a0 in ((half - rad, SIGN_Z0 + rad, -90), (half - rad, top - rad, 0),
                       (-half + rad, top - rad, 90), (-half + rad, SIGN_Z0 + rad, 180)):
        for k in range(6):
            a = math.radians(a0 + 90 * k / 5)
            outline.append((cy + rad * math.cos(a), cz + rad * math.sin(a)))

    surfs = []

    def surf(name, shader):
        s = md3.Surf()
        s.name, s.flags, s.shaders = name, 0, [(shader, 0)]
        s.tris, s.st, s.frames = [], [], [[]]
        surfs.append(s)
        return s

    def vert(s, p, nrm, uv):
        s.frames[0].append([round(p[0] * 64), round(p[1] * 64), round(p[2] * 64), md3.enc_normal(*nrm)])
        s.st.append([uv[0], uv[1]])
        return len(s.st) - 1

    def tri(s, a, b, c):
        s.tris.append([a, c, b])                  # Q3 draws clockwise triangles as front faces

    # the front and the back: a fan over two rings (enough vertices for the light)
    for s, x, nx in ((surf("front", OWN_MODELS + f"sign_{arrow}"), thick, 1), (surf("back", OWN_MODELS + "sign_metal"), -thick, -1)):
        front = s.name == "front"
        ctr = vert(s, (x, 0, zc), (nx, 0, 0), (0.5, (top - zc) / SIGN_H) if front else (0, zc / 64))
        rings = []
        for f in (0.5, 1.0):
            ring = []
            for y, z in outline:
                py, pz = y * f, zc + (z - zc) * f
                uvp = ((py + half) / SIGN_W, (top - pz) / SIGN_H) if front else (py / 64, pz / 64)
                ring.append(vert(s, (x, py, pz), (nx, 0, 0), uvp))
            rings.append(ring)
        m = len(outline)
        for k in range(m):
            a, b = k, (k + 1) % m
            if nx > 0:
                tri(s, ctr, rings[0][a], rings[0][b])
                tri(s, rings[0][a], rings[1][a], rings[1][b])
                tri(s, rings[0][a], rings[1][b], rings[0][b])
            else:
                tri(s, ctr, rings[0][b], rings[0][a])
                tri(s, rings[0][a], rings[1][b], rings[1][a])
                tri(s, rings[0][a], rings[0][b], rings[1][b])
    # the folded edge
    s = surf("edge", OWN_MODELS + "sign_metal")
    m = len(outline)
    for k in range(m):
        (y0, z0), (y1, z1) = outline[k], outline[(k + 1) % m]
        ny, nz = z1 - z0, -(y1 - y0)
        ln = math.hypot(ny, nz) or 1
        nrm = (0, ny / ln, nz / ln)
        q = [vert(s, (xx, yy, zz), nrm, (uu, vv)) for xx, yy, zz, uu, vv in
             ((thick, y0, z0, 0, 0), (thick, y1, z1, 0.25, 0), (-thick, y1, z1, 0.25, 0.04), (-thick, y0, z0, 0, 0.04))]
        tri(s, q[0], q[1], q[2])
        tri(s, q[0], q[2], q[3])
    # posts (round tubes into the ground) with caps, and clamps to the board
    s = surf("posts", OWN_MODELS + "sign_metal")
    pr, segs = 2.3, 10
    for py in (-SIGN_POST_Y, SIGN_POST_Y):
        ring0, ring1 = [], []
        for k in range(segs + 1):
            a = 2 * math.pi * k / segs
            c, sn = math.cos(a), math.sin(a)
            ring0.append(vert(s, (SIGN_POST_X + pr * c, py + pr * sn, -110), (c, sn, 0), (k / segs, 3.5)))
            ring1.append(vert(s, (SIGN_POST_X + pr * c, py + pr * sn, top - 3), (c, sn, 0), (k / segs, 0)))
        for k in range(segs):
            tri(s, ring0[k], ring0[k + 1], ring1[k + 1])
            tri(s, ring0[k], ring1[k + 1], ring1[k])
        cap = vert(s, (SIGN_POST_X, py, top - 1.5), (0, 0, 1), (0.5, 0.5))
        for k in range(segs):
            tri(s, ring1[k], ring1[k + 1], cap)
        for zz in (SIGN_Z0 + 9, top - 9):        # clamp: a small box around the post
            box = [(x, y, z) for x in (-thick - 0.2, SIGN_POST_X - pr - 0.8) for y in (py - 4, py + 4)
                   for z in (zz - 2.5, zz + 2.5)]
            for quad, nrm in (((0, 1, 3, 2), (1, 0, 0)), ((4, 6, 7, 5), (-1, 0, 0)), ((0, 4, 5, 1), (0, -1, 0)),
                              ((2, 3, 7, 6), (0, 1, 0)), ((0, 2, 6, 4), (0, 0, -1)), ((1, 5, 7, 3), (0, 0, 1))):
                idx4 = [vert(s, box[q], nrm, (box[q][1] / 16, box[q][2] / 16)) for q in quad]
                tri(s, idx4[0], idx4[2], idx4[1])
                tri(s, idx4[0], idx4[3], idx4[2])
    m_ = md3.Model()
    m_.name, m_.flags = f"sign_{arrow}", 0
    r = math.hypot(half, top)
    m_.frames = [[SIGN_POST_X - pr, -half, -110, thick, half, top, 0, 0, 0, r, b"sign"]]
    m_.tags = []
    m_.surfs = surfs
    out = ROOT / "baseq3r" / OWN_MODELS
    out.mkdir(parents=True, exist_ok=True)
    md3.save(m_, str(out / f"sign_{arrow}.md3"))


def write_sign_textures():
    """Chevron board (red on white reflective sheeting, a little road dirt at
    the bottom) and the folded aluminium."""
    from PIL import Image, ImageDraw, ImageFilter
    import numpy as np
    out = ROOT / "baseq3r" / OWN_MODELS
    out.mkdir(parents=True, exist_ok=True)
    w, h = 512, 256
    rng = np.random.default_rng(9)
    im = Image.new("RGB", (w, h), (236, 236, 230))
    d = ImageDraw.Draw(im)
    for k in range(3):
        x = 58 + k * 150
        d.polygon([(x, 20), (x + 82, h / 2), (x, h - 20), (x + 60, h - 20), (x + 142, h / 2), (x + 60, 20)],
                  fill=(196, 24, 22))
    a = np.asarray(im).astype(np.float32)
    # the micro-prism pattern of reflective sheeting, faint
    yy, xx = np.mgrid[0:h, 0:w]
    hexes = 0.5 + 0.5 * np.cos(xx * 0.9) * np.cos((yy + 0.5 * xx) * 0.9)
    a *= (0.97 + 0.03 * hexes)[..., None]
    # dirt sprayed up from the road: darker and browner towards the bottom
    dirt = np.clip((yy / h - 0.55) / 0.45, 0, 1) ** 1.6 * (0.6 + 0.4 * rng.random((h, w)))
    a = a * (1 - 0.35 * dirt[..., None]) + np.array([96, 80, 60]) * 0.35 * dirt[..., None]
    a *= (0.96 + 0.04 * rng.random((h, w)))[..., None]
    im = Image.fromarray(a.clip(0, 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.5))
    im.save(out / "sign_r.jpg", quality=93)                        # arrows to the right
    im.transpose(Image.FLIP_LEFT_RIGHT).save(out / "sign_l.jpg", quality=93)


def sign_shaders():
    M = OWN_MODELS
    out = ["// curve signs (models, vertex lit by q3map2)"]
    for arrow in ("l", "r"):
        out += [f"{M}sign_{arrow}", "{", f"\tqer_editorimage {M}sign_{arrow}.jpg", "\tsurfaceparm nolightmap",
                "\tsurfaceparm SURF_METAL", "\tcull disable", "\t{", f"\t\tmap {M}sign_{arrow}.jpg", "\t\trgbGen vertex", "\t}", "}", ""]
    out += [f"{M}sign_metal", "{", "\tqer_editorimage textures/base_trim/tin.jpg", "\tsurfaceparm nolightmap",
            "\tsurfaceparm SURF_METAL", "\tcull disable", "\t{", "\t\tmap textures/base_trim/tin.jpg", "\t\trgbGen vertex", "\t}", "}", ""]
    return out


def log(c, axis, r, length, ends=TEX + "logend", bark=TEX + "logbark", solid=True, sides=8):
    """A lying log (or round bale): a prism with `sides` sides along axis (horizontal unit vector)."""
    ax, ay = axis
    t = (ay, -ax)                                    # across the log, horizontal
    planes = []
    for k in range(sides):
        a = 2 * math.pi * (k + 0.5) / sides
        d = (t[0] * math.cos(a), t[1] * math.cos(a), math.sin(a))
        planes.append((d, (c[0] + d[0] * r, c[1] + d[1] * r, c[2] + d[2] * r), bark))
    for sgn in (1, -1):
        nrm = (ax * sgn, ay * sgn, 0.0)
        p = (c[0] + ax * sgn * length / 2, c[1] + ay * sgn * length / 2, c[2])
        right = (-nrm[1], nrm[0])
        left = (p[0] - right[0] * r, p[1] - right[1] * r, 0.0)
        fit = fitted(nrm, left, c[2] + r, 2 * r, 2 * r, 128, 128)
        planes.append((nrm, p, ends, fit))
    convex(planes, solid=solid)


def log_pile():
    """A pile of logs across the end of the run-out: what you hit when the
    brakes are late (the invisible wall stays behind it)."""
    i = len(PATH) - 1
    rng = random.Random(4)
    (px, py), t, n = PATH[i], TAN[i], NOR[i]
    hw = half_width(S[i])
    z = road_z(S[i])
    r = 24
    for row, count in enumerate((4, 3, 2)):
        for k in range(count):
            back = -60 - (k - (count - 1) / 2) * 2 * r * 1.02
            cx, cy = px + t[0] * back, py + t[1] * back
            cz = z + r + row * r * 1.72 - 4
            length = 2 * hw + 260 + rng.uniform(-40, 40)
            off = rng.uniform(-20, 20)
            log((cx + n[0] * off, cy + n[1] * off, cz), n, r + rng.uniform(-2, 2), length)
    BOUNDS["log_pile"] = round(S[i] - 60)


def write_wood_textures():
    """End grain of the logs (rings, bark rim, cracks)."""
    from PIL import Image, ImageDraw, ImageFilter
    import numpy as np
    out = ROOT / "baseq3r" / "textures" / "q3r_timberrun"
    n = 128
    yy, xx = np.mgrid[0:n, 0:n] - (n - 1) / 2
    rr = np.hypot(xx, yy) / (n / 2)
    rng = np.random.default_rng(12)
    wob = 1 + 0.03 * np.sin(np.arctan2(yy, xx) * 5)
    rings = 0.5 + 0.5 * np.sin(rr * wob * 46)
    col = np.array([196, 160, 112]) * (0.86 + 0.14 * rings[..., None]) * (0.95 + 0.05 * rng.random((n, n)))[..., None]
    col = np.where((rr > 0.88)[..., None], np.array([70, 52, 38]), col)
    im = Image.fromarray(col.clip(0, 255).astype(np.uint8))
    d = ImageDraw.Draw(im)
    for _ in range(3):
        a = rng.uniform(0, 2 * math.pi)
        d.line((n / 2, n / 2, n / 2 + math.cos(a) * n * 0.42, n / 2 + math.sin(a) * n * 0.42), fill=(110, 84, 58), width=2)
    im.filter(ImageFilter.GaussianBlur(0.6)).save(out / "logend.jpg", quality=92)
    # spruce bark, the furrows along the log (along s)
    w, h = 256, 128
    small = rng.random((h // 2, 8))
    fur = np.asarray(Image.fromarray((small * 255).astype(np.uint8)).resize((w, h), Image.BICUBIC)).astype(np.float32) / 255
    fine = np.asarray(Image.fromarray((rng.random((h, w // 4)) * 255).astype(np.uint8)).resize((w, h), Image.BILINEAR)).astype(np.float32) / 255
    v = 0.55 * fur + 0.45 * fine
    col = np.array([92, 74, 60]) * (0.62 + 0.55 * v[..., None])
    col = np.where((v < 0.33)[..., None], col * 0.6, col)
    Image.fromarray(col.clip(0, 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.5)).save(
        out / "logbark.jpg", quality=92)


def signs():
    """Curve signs before the tight bends (red/white chevrons): three models
    on the outside of the bend, each with two thin solid posts."""
    n = len(PATH)
    count = 0
    for i in range(20, n - 20):
        k = abs(KAPPA[i])
        if k < 1 / 520 or not (k >= abs(KAPPA[i - 1]) and k > abs(KAPPA[i + 1])):
            continue
        side = 1 if KAPPA[i] > 0 else -1           # inside of the bend
        for d in (-6, 0, 6):
            j = idx(i + d)
            hw = half_width(S[j])
            x, y, z = road_point(j, -side * (hw + SHOULDER + 30))
            f = (-TAN[j][0], -TAN[j][1])           # the board looks back at the oncoming cars
            yaw = math.degrees(math.atan2(f[1], f[0]))
            entity("misc_model", x, y, z, model=OWN_MODELS + ("sign_l.md3" if KAPPA[i] > 0 else "sign_r.md3"),
                   angle=f"{yaw:.1f}", _cs="1")
            for py in (-SIGN_POST_Y, SIGN_POST_Y):
                cx = x + f[0] * SIGN_POST_X - f[1] * py
                cy = y + f[1] * SIGN_POST_X + f[0] * py
                obox(cx, cy, f, 3, 3, ground(cx, cy) - 20, z + SIGN_Z0 + SIGN_H, TEX + "trunk", solid=False)
            count += 1
    BOUNDS["signs"] = count


def water():
    """The creek: one level sheet of water (not solid, no liquid physics).
    The ground is above it everywhere but in the channel and on the ford."""
    x0, y0, x1, y1 = CREEK_BOX
    wz = BOUNDS["water_z"]
    box((x0, y0, wz - 40), (x1, y1, wz), "common/nodraw", collision=False, faces={"top": TEX + "creek"})


def shell():
    x0, x1, y0, y1 = BOUNDS["x0"], BOUNDS["x1"], BOUNDS["y0"], BOUNDS["y1"]
    top = max(GRID.values()) + 2400
    m = 64
    sky = TEX + "sky"
    for lo, hi, mat in (((x0 - m, y0 - m, FLOOR - 64), (x1 + m, y1 + m, FLOOR), "common/caulk"),
                        ((x0 - m, y0 - m, FLOOR), (x0, y1 + m, top), sky),
                        ((x1, y0 - m, FLOOR), (x1 + m, y1 + m, top), sky),
                        ((x0, y0 - m, FLOOR), (x1, y0, top), sky),
                        ((x0, y1, FLOOR), (x1, y1 + m, top), sky),
                        ((x0 - m, y0 - m, top), (x1 + m, y1 + m, top + m), sky)):
        (ax, ay, az), (bx, by, bz) = lo, hi
        brush([face(plane(nrm, pt), mat, detail=False) for nrm, pt in
               (((0, 0, -1), (ax, ay, az)), ((0, 0, 1), (bx, by, bz)), ((-1, 0, 0), (ax, ay, az)),
                ((1, 0, 0), (bx, by, bz)), ((0, -1, 0), (ax, ay, az)), ((0, 1, 0), (bx, by, bz)))])
    xs = [p[0] for p in PATH]
    ys = [p[1] for p in PATH]
    brush([face(plane(nrm, pt), "common/lightgrid") for nrm, pt in
           (((0, 0, -1), (min(xs) - 800, min(ys) - 800, 500)), ((0, 0, 1), (max(xs) + 800, max(ys) + 800, 1900)),
            ((-1, 0, 0), (min(xs) - 800, 0, 0)), ((1, 0, 0), (max(xs) + 800, 0, 0)),
            ((0, -1, 0), (0, min(ys) - 800, 0)), ((0, 1, 0), (0, max(ys) + 800, 0)))])


def misc_entities():
    n = len(PATH)
    for f, lat, up in ((0.04, 600, 300), (0.14, -700, 320), (0.28, 700, 380), (0.44, -700, 300),
                       (0.70, 800, 340), (0.93, -700, 300)):
        i = idx(int(f * n))
        x, y, z = road_point(i, lat)
        ang = math.degrees(math.atan2(PATH[i][1] - y, PATH[i][0] - x))
        entity("info_observer_spot", x, y, max(z, ground(x, y)) + up, angles=f"15 {ang:.0f} 0", spawnflags="1")
    a, b = SECTIONS["hairpins"]
    i = (a + b) // 2
    x, y, z = road_point(i, 900)
    ang = math.degrees(math.atan2(PATH[i][1] - y, PATH[i][0] - x))
    entity("info_player_intermission", x, y, max(z, ground(x, y)) + 650, angles=f"22 {ang:.0f} 0")


# ---------------------------------------------------------------- details (stage 3)

FOOTPRINTS = []         # (x, y, radius): no trees here


def ground_spread(pts):
    zs = [ground(x, y) for x, y in pts]
    return min(zs), max(zs)


def stack(c, axis, length, rows, r, marked, rng, solid=False):
    """Logs lying parallel along axis, in rows (bottom first), on the ground
    at c (x, y, z). Decoration beyond the walls is not solid."""
    ax, ay = axis
    n = (-ay, ax)
    sfx = "" if solid else "_ns"
    for row, count in enumerate(rows):
        for k in range(count):
            off = (k - (count - 1) / 2) * 2 * r * 1.03
            rr = r + rng.uniform(-2.5, 2.5)
            cz = c[2] + rr + row * r * 1.72 - 3
            shift = rng.uniform(-14, 14)
            ends = TEX + ("logend_mark" if marked and rng.random() < 0.7 else "logend") + sfx
            log((c[0] + n[0] * off + ax * shift, c[1] + n[1] * off + ay * shift, cz), axis, rr,
                length + rng.uniform(-24, 24), ends=ends, bark=TEX + "logbark" + sfx, solid=solid)


def roadside(i, side, extra, half_len):
    """A flat spot beside the road behind the wall: (x, y, z, axis) or None."""
    hw = half_width(S[i])
    lat = PLACED[side].get(i, hw + SHOULDER + LAND_GAP) + extra
    x, y, _ = road_point(i, side * lat)
    t = TAN[i]
    pts = [(x + t[0] * d, y + t[1] * d) for d in (-half_len, 0, half_len)]
    lo, hi = ground_spread(pts)
    if hi - lo > 22:
        return None
    for px, py in pts:                      # keep clear of other parts of the course
        d, j = nearest(px, py, 900)
        if abs(j - i) > 20 and d - half_width(S[j]) < 300:
            return None
    return x, y, (lo + hi) / 2, t


def details():
    """Log piles at the timber yard and along the forest road, stumps in the
    clear-cut, round bales on the meadow."""
    rng = random.Random(33)
    piles = 0
    # timber yard: four long piles behind the walls, marked for the sawmill
    a, b = SECTIONS["yard"]
    for s_at, side in ((280, 1), (700, 1), (420, -1), (1150, -1)):
        i = at(s_at)
        spot = roadside(i, side, 90, 220)
        if spot:
            x, y, z, t = spot
            stack((x, y, z), t, 440, (5, 4, 3), 22, True, rng)
            FOOTPRINTS.append((x, y, 300))
            piles += 1
    # along the course: smaller piles on straight bits
    for sec, pos in (("road", 0.18), ("road", 0.6), ("hairpins", 0.12), ("creek", 0.15), ("thicket", 0.35),
                     ("thicket", 0.8), ("clearcut", 0.12), ("clearcut", 0.45), ("clearcut", 0.8)):
        a, b = SECTIONS[sec]
        want = int(a + (b - a) * pos)
        for d in sorted(range(-25, 26), key=abs):
            i = idx(want + d)
            if abs(KAPPA[i]) > 1 / 1400:
                continue
            side = 1 if (want // 7) % 2 else -1
            spot = roadside(i, side, 80, 170) or roadside(i, -side, 80, 170)
            if spot:
                x, y, z, t = spot
                stack((x, y, z), t, 340, rng.choice(((4, 3, 2), (3, 2, 1), (4, 3))), 20, rng.random() < 0.5, rng)
                FOOTPRINTS.append((x, y, 240))
                piles += 1
                break
    # stumps in the clear-cut
    a, b = SECTIONS["clearcut"]
    stumps = 0
    tries = 0
    while stumps < 90 and tries < 3000:
        tries += 1
        i = rng.randrange(a + 10, b - 10)
        if OPEN.get(i, 0) < 0.5:
            continue
        side = rng.choice((-1, 1))
        hw = half_width(S[i])
        lat = PLACED[side].get(i, hw + SHOULDER + LAND_GAP) + rng.uniform(30, 1100)
        x, y, _ = road_point(i, side * lat)
        d, j = nearest(x, y, 900)
        if d - half_width(S[j]) < 200 or creek_distance(x, y) < CREEK_W + 80:
            continue
        z = ground(x, y)
        r = rng.uniform(13, 22)
        h = rng.uniform(10, 30)
        stump((x, y, z), r, h, rng)
        FOOTPRINTS.append((x, y, 60))
        stumps += 1
    # round bales on the meadow before the finish
    a, b = SECTIONS["crest"]
    bales = 0
    for k in range(40):
        if bales >= 9:
            break
        i = rng.randrange(a + (b - a) // 2, b)
        side = rng.choice((-1, 1))
        hw = half_width(S[i])
        lat = PLACED[side].get(i, hw + SHOULDER + LAND_GAP) + rng.uniform(60, 500)
        x, y, _ = road_point(i, side * lat)
        d, j = nearest(x, y, 900)
        if d - half_width(S[j]) < 220 or any(math.dist((x, y), f[:2]) < f[2] for f in FOOTPRINTS):
            continue
        yaw = rng.uniform(0, math.pi)
        lo, hi = ground_spread([(x + math.cos(yaw) * 45 * e, y + math.sin(yaw) * 45 * e) for e in (-1, 1)])
        if hi - lo > 14:
            continue
        log((x, y, lo + 38), (math.cos(yaw), math.sin(yaw)), 40, 92, ends=TEX + "bale_end",
            bark=TEX + "bale", solid=False, sides=14)
        FOOTPRINTS.append((x, y, 120))
        bales += 1
    BOUNDS["details"] = {"log_piles": piles, "stumps": stumps, "bales": bales}


def stump(c, r, h, rng):
    """A short upright octagonal stump, nonsolid (only beyond the walls)."""
    planes = []
    rot = rng.uniform(0, math.pi / 4)
    for k in range(8):
        a = rot + 2 * math.pi * k / 8
        d = (math.cos(a), math.sin(a), 0.0)
        planes.append((d, (c[0] + d[0] * r, c[1] + d[1] * r, c[2]), TEX + "logbark_ns"))
    top = (0.0, 0.0, 1.0)
    planes.append((top, (c[0], c[1], c[2] + h), TEX + "logend_ns"))
    planes.append(((0.0, 0.0, -1.0), (c[0], c[1], c[2] - 30), TEX + "logbark_ns"))
    convex(planes, solid=False)


def sounds():
    """Ambient loops: the forest along the course, the creek at the ford."""
    n = len(PATH)
    count = 0
    for k, i in enumerate(range(20, n - 20, int(1500 / SAMPLE))):
        side = 1 if k % 2 else -1
        x, y, z = road_point(i, side * 520)
        if creek_distance(x, y) < 900:
            continue
        entity("target_speaker", x, y, ground(x, y) + 160, noise="sound/world/q3r_timberrun/forest.ogg",
               spawnflags="1")
        count += 1
    wz = BOUNDS["water_z"]
    creek = 0
    last = None
    for x, y in CREEK:
        if last and math.dist((x, y), last) < 650:
            continue
        entity("target_speaker", x, y, wz + 40, noise="sound/world/q3r_timberrun/creek.ogg", spawnflags="1")
        last = (x, y)
        creek += 1
    BOUNDS["speakers"] = {"forest": count, "creek": creek}


def write_detail_textures():
    """End grain with a forester's spray mark, straw for the bales."""
    from PIL import Image, ImageDraw, ImageFilter
    import numpy as np
    out = ROOT / "baseq3r" / "textures" / "q3r_timberrun"
    im = Image.open(out / "logend.jpg").convert("RGB")
    d = ImageDraw.Draw(im)
    d.line((40, 52, 88, 76), fill=(206, 40, 32), width=7)
    d.line((44, 80, 84, 48), fill=(206, 40, 32), width=6)
    im.filter(ImageFilter.GaussianBlur(0.7)).save(out / "logend_mark.jpg", quality=92)
    rng = np.random.default_rng(21)
    w, h = 256, 128
    a = np.zeros((h, w, 3))
    base = np.array([164, 140, 92])
    a[:] = base * 0.8
    img = Image.fromarray(a.astype(np.uint8))
    d = ImageDraw.Draw(img)
    for _ in range(1800):
        x, y = rng.uniform(0, w), rng.uniform(0, h)
        L = rng.uniform(10, 40)
        ang = rng.normal(0, 0.15)
        c = tuple(int(v * rng.uniform(0.6, 1.15)) for v in base)
        for ox in (-w, 0, w):
            d.line((x + ox, y, x + ox + L * math.cos(ang), y + L * math.sin(ang)), fill=c, width=1)
    img.filter(ImageFilter.GaussianBlur(0.4)).save(out / "bale.jpg", quality=92)
    n = 128
    yy, xx = np.mgrid[0:n, 0:n] - (n - 1) / 2
    rr = np.hypot(xx, yy) / (n / 2)
    th = np.arctan2(yy, xx)
    spiral = 0.5 + 0.5 * np.sin(rr * 40 + th)
    col = base * (0.72 + 0.3 * spiral[..., None]) * (0.9 + 0.1 * rng.random((n, n)))[..., None]
    Image.fromarray(col.clip(0, 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.6)).save(
        out / "bale_end.jpg", quality=92)


# ---------------------------------------------------------------- forest (stage 2)

MAPOBJ = "models/mapobjects/"
OWN_MODELS = f"models/mapobjects/{NAME}/"
# kind: model, scale range, leaf shader of the model, own leaf texture
KINDS = {
    "spruce": ("GR_trees/nadelbaum.md3", (3.0, 4.4), "GR_trees/nadel", "spruce"),
    "young": ("GR_trees/nadelbaum2.md3", (2.0, 3.2), "GR_trees/nadel", "spruce"),
    "beech_y": ("GR_trees/tree1.md3", (2.1, 2.8), "GR_trees/tree", "beech_y"),
    "beech_r": ("GR_trees/tree3.md3", (2.1, 2.8), "GR_trees/tree3", "beech_r"),
    "beech_g": ("GR_trees/tree4.md3", (2.0, 2.6), "GR_trees/tree4", "beech_g"),
    "oak": ("oak/oak2.md3", (2.0, 2.6), "oak/oakblaetter2", "oak"),
    "snag": ("oak/oak4.md3", (1.3, 1.8), None, None),
}
BARK = (("GR_trees/stamm", "bark"), ("oak/oakstamm", "oakbark"))
SPRITES = ("spruce", "beech_y", "beech_r", "beech_g", "oak")
SPRITE_OF = {"young": "spruce", "snag": "spruce"}

# tree mix per section: (kind, weight) and the density near the road
MIX = {
    "yard": ((("spruce", 6), ("beech_y", 1)), 0.55),
    "road": ((("spruce", 5), ("beech_y", 2), ("beech_r", 1.5), ("beech_g", 1.5)), 0.62),
    "hairpins": ((("beech_y", 3.5), ("beech_r", 2.5), ("beech_g", 1.5), ("spruce", 2.5)), 0.58),
    "creek": ((("beech_g", 4), ("beech_y", 3), ("beech_r", 1), ("spruce", 2)), 0.5),
    "thicket": ((("spruce", 11), ("young", 1)), 0.9),
    "clearcut": ((("young", 6), ("snag", 2), ("beech_y", 1)), 0.5),
    "hollow": ((("beech_y", 4), ("beech_r", 3), ("oak", 2), ("beech_g", 1)), 0.62),
    "crest": ((("oak", 5), ("beech_y", 3), ("spruce", 2)), 0.5),
    "finish": ((("oak", 5), ("beech_y", 3), ("spruce", 2)), 0.5),
}
NEAR = 480          # real tree models up to this far beyond the road edge, flat crosses beyond
FAR = 2700          # nothing beyond this
SEC_OF = []


def section_of(i):
    if not SEC_OF:
        SEC_OF.extend([""] * len(PATH))
        for name, (a, b) in SECTIONS.items():
            for k in range(a, b + 1):
                SEC_OF[k] = SEC_OF[k] or name
    return SEC_OF[i]


def pick(rng, mix):
    total = sum(w for _, w in mix)
    r = rng.random() * total
    for kind, w in mix:
        r -= w
        if r <= 0:
            return kind
    return mix[-1][0]


def forest():
    """Trees: real models near the road (solid trunks inside the walls),
    crossed flat sprites further out, nothing in the creek or on the road.
    Open land (yard, clear-cut, meadow) keeps few trees near the road."""
    rng = random.Random(77)
    x0, x1, y0, y1 = BOUNDS["x0"], BOUNDS["x1"], BOUNDS["y0"], BOUNDS["y1"]
    wz = BOUNDS["water_z"]
    counts = {"models": 0, "sprites": 0, "trunks": 0, "inside": 0}
    kinds = {}
    for cell, zone in ((150, "near"), (210, "far")):
        for gy in range(y0 + 150, y1 - 150, cell):
            for gx in range(x0 + 150, x1 - 150, cell):
                x, y = gx + rng.uniform(0, cell), gy + rng.uniform(0, cell)
                roll, kroll, srand = rng.random(), rng.random(), rng.random()
                d, i = nearest(x, y, FAR + 400)
                if d > 1e8:
                    continue
                e = d - half_width(S[i]) - SHOULDER
                if (zone == "near") != (e < NEAR) or e < 80 or e > FAR:
                    continue
                z = ground(x, y)
                if z < wz + 12 or creek_distance(x, y) < CREEK_W + 60:
                    continue
                if any((x - fx) ** 2 + (y - fy) ** 2 < fr * fr for fx, fy, fr in FOOTPRINTS):
                    continue
                sl = max(abs(ground(x + 40, y) - ground(x - 40, y)), abs(ground(x, y + 40) - ground(x, y - 40))) / 80
                if sl > 0.95:
                    continue
                sec = section_of(i)
                mix, dens = MIX[sec]
                opn = OPEN.get(i, 0.0) * (1 - smooth(1100, 1700, e))
                s_, dsig = local(i, x, y)
                side = 1 if dsig > 0 else -1
                wall = PLACED[side].get(i)
                inside = wall is not None and abs(dsig) < wall + 40
                if inside:
                    # inside the walls only on straight road, never at the outside of a bend
                    if abs(KAPPA[i]) > 1 / 900 or sec in ("yard", "finish") or i in FORD:
                        continue
                    dens *= 0.45
                if zone == "far":
                    dens = 0.78
                dens *= 1 - 0.88 * opn
                if roll > dens:
                    continue
                kind = pick(random.Random(kroll), mix)
                if opn > 0.5 and sec == "clearcut" and e > 1300:
                    kind = "spruce"                         # the old forest around the clear-cut
                if kind in ("oak", "snag") and (inside or e < 300):
                    kind = "beech_y"                        # the oak trunks are wide at the foot
                if zone == "near":
                    model, (a, b), _, _ = KINDS[kind]
                    scale = a + (b - a) * srand
                    sink = 6 + sl * 30
                    keys = {"model": MAPOBJ + model, "modelscale": f"{scale:.2f}",
                            "angle": str(rng.randrange(360)), "_cs": "1", "_rs": "1"}
                    for k, (src, dst) in enumerate(remaps(kind)):
                        keys[f"_remap{k or ''}"] = f"{src};{dst}"
                    entity("misc_model", x, y, z - sink, **keys)
                    counts["models"] += 1
                    if inside:                      # outside the walls no car gets near
                        r = 16 if kind in ("young", "snag") else 18
                        # not "boundary": the AAS proxy keeps those everywhere, these only where cars get
                        box((x - r, y - r, z - 30), (x + r, y + r, z + 150), TEX + "trunk")
                        counts["trunks"] += 1
                        counts["inside"] += inside
                else:
                    kind = SPRITE_OF.get(kind, kind)
                    _, (a, b), _, _ = KINDS[kind]
                    scale = a + (b - a) * srand
                    entity("misc_model", x, y, z - 8 - sl * 30, model=OWN_MODELS + f"cross_{kind}.md3",
                           modelscale=f"{scale:.2f}", angle=str(rng.randrange(90)))
                    counts["sprites"] += 1
                kinds[kind] = kinds.get(kind, 0) + 1
    BOUNDS["forest"] = {**counts, "kinds": kinds}


def remaps(kind):
    _, _, leaf, own = KINDS[kind]
    out = []
    for src, dst in BARK:
        for ext in ("", ".tga", ".jpg"):
            out.append((MAPOBJ + src + ext, OWN_MODELS + dst))
    if leaf:
        for ext in ("", ".tga"):
            out.append((MAPOBJ + leaf + ext, OWN_MODELS + own))
    return out


def load_md3():
    import sys
    sys.path.insert(0, str(ROOT / "tools" / "carbuild"))
    import md3
    return md3


def leaf_texture(own):
    """The leaf textures of the models, recoloured for an autumn morning."""
    from PIL import Image
    import numpy as np
    src = {"spruce": "GR_trees/nadel.tga", "beech_y": "GR_trees/tree.tga", "beech_r": "GR_trees/tree3.tga",
           "beech_g": "GR_trees/tree4.tga", "oak": "oak/oakblaetter2.tga"}[own]
    a = np.asarray(Image.open(ROOT / "baseq3r" / MAPOBJ / src).convert("RGBA")).astype(np.float32)
    mul = {"spruce": (0.50, 0.66, 0.50), "beech_y": (1.0, 0.92, 0.80), "beech_r": (0.95, 0.85, 0.80),
           "beech_g": (0.92, 0.92, 0.80), "oak": (0.95, 0.88, 0.75)}[own]
    a[..., :3] *= mul
    return a


def sprite(kind, w=256, h=512):
    """Side view of a tree model (orthographic, z-buffered, alpha tested): the
    texture of the crossed planes far from the road."""
    import numpy as np
    md3 = load_md3()
    model, _, leaf, own = KINDS[kind]
    m = md3.load(str(ROOT / "baseq3r" / MAPOBJ / model))
    from PIL import Image
    texs = {}
    tris = []
    for s in m.surfs:
        name = s.shaders[0][0]
        if leaf and name.startswith(MAPOBJ + leaf):
            t = leaf_texture(own)
        else:
            p = ROOT / "baseq3r" / (name.rsplit(".", 1)[0] + ".jpg")
            if not p.exists():
                p = p.with_suffix(".tga")
            t = np.asarray(Image.open(p).convert("RGBA")).astype(np.float32)
            t[..., :3] *= 0.8
        texs[name] = t
        v = np.array([f[:3] for f in s.frames[0]], dtype=np.float32) / 64
        st = np.array(s.st, dtype=np.float32)
        for a, b, c in s.tris:
            tris.append((v[[a, b, c]], st[[a, b, c]], name))
    allv = np.concatenate([t[0] for t in tris])
    rad = float(max(np.abs(allv[:, 0]).max(), np.abs(allv[:, 1]).max()))
    top = float(allv[:, 2].max())
    sx, sz = w / (2 * rad), h / top
    img = np.zeros((h, w, 4), np.float32)
    depth = np.full((h, w), 1e9, np.float32)
    for v, st, name in tris:
        tex = texs[name]
        th, tw = tex.shape[:2]
        px = (v[:, 0] + rad) * sx
        py = (top - v[:, 2]) * sz
        u0, u1 = int(max(0, np.floor(px.min()))), int(min(w - 1, np.ceil(px.max())))
        v0, v1 = int(max(0, np.floor(py.min()))), int(min(h - 1, np.ceil(py.max())))
        if u1 < u0 or v1 < v0:
            continue
        gu, gv = np.meshgrid(np.arange(u0, u1 + 1) + 0.5, np.arange(v0, v1 + 1) + 0.5)
        (ax, bx, cx), (ay, by, cy) = px, py
        den = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy)
        if abs(den) < 1e-6:
            continue
        l1 = ((by - cy) * (gu - cx) + (cx - bx) * (gv - cy)) / den
        l2 = ((cy - ay) * (gu - cx) + (ax - cx) * (gv - cy)) / den
        l3 = 1 - l1 - l2
        ins = (l1 >= 0) & (l2 >= 0) & (l3 >= 0)
        if not ins.any():
            continue
        dep = l1 * v[0, 1] + l2 * v[1, 1] + l3 * v[2, 1]
        s_ = (l1 * st[0, 0] + l2 * st[1, 0] + l3 * st[2, 0]) % 1.0
        t_ = (l1 * st[0, 1] + l2 * st[1, 1] + l3 * st[2, 1]) % 1.0
        col = tex[np.clip((t_ * th).astype(int), 0, th - 1), np.clip((s_ * tw).astype(int), 0, tw - 1)]
        ok = ins & (col[..., 3] > 127) & (dep < depth[v0:v1 + 1, u0:u1 + 1])
        # darker towards the back and the bottom of the crown
        shade = (0.72 + 0.28 * np.clip((rad - dep) / (2 * rad), 0, 1)) * (0.75 + 0.25 * (1 - gv / h))
        region = img[v0:v1 + 1, u0:u1 + 1]
        region[ok, :3] = col[ok, :3] * shade[ok, None]
        region[ok, 3] = 255
        depth[v0:v1 + 1, u0:u1 + 1][ok] = dep[ok]
    # bleed the colour into the transparent texels (no dark fringe in the mip maps)
    from PIL import ImageFilter
    rgb = Image.fromarray(img[..., :3].clip(0, 255).astype(np.uint8))
    alpha = img[..., 3] > 0
    filled = np.asarray(rgb).astype(np.float32)
    for r in (2, 4, 8, 16):
        blur = np.asarray(rgb.filter(ImageFilter.BoxBlur(r))).astype(np.float32)
        wgt = np.asarray(Image.fromarray((alpha * 255).astype(np.uint8)).filter(ImageFilter.BoxBlur(r))).astype(np.float32)
        est = blur / np.maximum(wgt[..., None] / 255, 1e-3)
        hole = ~alpha & (wgt > 0) & (filled.sum(-1) == 0)
        filled[hole] = est[hole]
    out = np.dstack([filled.clip(0, 255), alpha * 255]).astype(np.uint8)
    return Image.fromarray(out, "RGBA"), rad, top


def cross_model(kind, rad, top):
    """Two crossed quads, each with both windings, the size of the tree model."""
    md3 = load_md3()
    m = md3.Model()
    m.name, m.flags = f"cross_{kind}", 0
    m.frames = [[-rad, -rad, 0, rad, rad, top, 0, 0, 0, math.hypot(rad, top), b"cross"]]
    m.tags = []
    s = md3.Surf()
    s.name, s.flags = "cross", 0
    s.shaders = [(OWN_MODELS + f"sprite_{kind}", 0)]
    verts, st, tris = [], [], []
    up = md3.enc_normal(0, 0, 1)            # lit like the ground under it (the shader draws both sides)
    for ax in ((1, 0), (0, 1)):
        base = len(verts)
        for u, zz in ((0, 0), (1, 0), (1, top), (0, top)):
            p = -rad + 2 * rad * u
            verts.append([round(ax[0] * p * 64), round(ax[1] * p * 64), round(zz * 64), up])
            st.append([u, 1 - zz / top])
        tris += [[base, base + 2, base + 1], [base, base + 3, base + 2]]
    s.tris, s.st, s.frames = tris, st, [verts]
    m.surfs = [s]
    out = ROOT / "baseq3r" / OWN_MODELS
    out.mkdir(parents=True, exist_ok=True)
    md3.save(m, str(out / f"cross_{kind}.md3"))


def write_trees():
    """Leaf textures, sprites and the crossed-plane models."""
    from PIL import Image
    import numpy as np
    out = ROOT / "baseq3r" / OWN_MODELS
    out.mkdir(parents=True, exist_ok=True)
    for own in ("spruce", "beech_y", "beech_r", "beech_g", "oak"):
        Image.fromarray(leaf_texture(own).clip(0, 255).astype(np.uint8), "RGBA").save(out / f"{own}.tga", compression="tga_rle")
    for kind in SPRITES:
        im, rad, top = sprite(kind)
        im.save(out / f"sprite_{kind}.tga", compression="tga_rle")
        cross_model(kind, rad, top)


def tree_shaders():
    M = OWN_MODELS
    out = ["// trees: leaves alpha tested, cast alpha shadows; vertex lit by q3map2"]
    for own in ("spruce", "beech_y", "beech_r", "beech_g", "oak"):
        out += [f"{M}{own}", "{", f"\tqer_editorimage {M}{own}.tga", "\tsurfaceparm nonsolid",
                "\tsurfaceparm alphashadow", "\tsurfaceparm nomarks", "\tsurfaceparm nolightmap",
                "\tsurfaceparm trans", "\tcull disable",
                "\t{", f"\t\tmap {M}{own}.tga", "\t\talphaFunc GE128", "\t\tdepthWrite", "\t\trgbGen vertex",
                "\t}", "}", ""]
    for kind in SPRITES:
        out += [f"{M}sprite_{kind}", "{", f"\tqer_editorimage {M}sprite_{kind}.tga", "\tsurfaceparm nonsolid",
                "\tsurfaceparm nomarks", "\tsurfaceparm nolightmap", "\tsurfaceparm trans",
                "\tsurfaceparm noimpact", "\tcull disable", "\t{", f"\t\tmap {M}sprite_{kind}.tga", "\t\talphaFunc GE128",
                "\t\tdepthWrite", "\t\trgbGen vertex", "\t}", "}", ""]
    for src, dst in BARK:
        out += [f"{M}{dst}", "{", f"\tqer_editorimage {MAPOBJ}{src}.jpg", "\tsurfaceparm nonsolid",
                "\tsurfaceparm nolightmap",
                "\t{", f"\t\tmap {MAPOBJ}{src}.jpg", "\t\trgbGen vertex", "\t}", "}", ""]
    return out


# ---------------------------------------------------------------- sky, forest floor

SUN_YAW, SUN_PITCH = 300, 20        # south-east-ish, low: early morning in autumn


def write_sky():
    """A misty morning sky as a skybox, the sun where q3map_sun puts it."""
    import numpy as np
    from PIL import Image
    out = ROOT / "baseq3r" / "env" / NAME
    out.mkdir(parents=True, exist_ok=True)
    n = 512
    yaw, pitch = math.radians(SUN_YAW), math.radians(SUN_PITCH)
    sun = np.array([math.cos(yaw) * math.cos(pitch), math.sin(yaw) * math.cos(pitch), math.sin(pitch)])
    rng = np.random.default_rng(5)
    grids = [rng.random((g + 1, g + 1)) for g in (4, 8, 16, 32, 64)]

    def noise(u, v):
        acc = np.zeros_like(u)
        amp, tot = 1.0, 0.0
        for g in grids:
            k = g.shape[0] - 1
            fu, fv = (u % 1) * k, (v % 1) * k
            iu, iv = np.minimum(np.floor(fu).astype(int), k - 1), np.minimum(np.floor(fv).astype(int), k - 1)
            tu, tv = fu - iu, fv - iv
            tu, tv = tu * tu * (3 - 2 * tu), tv * tv * (3 - 2 * tv)
            g2 = g.copy()
            g2[-1, :], g2[:, -1] = g2[0, :], g2[:, 0]
            a = g2[iv, iu] * (1 - tu) + g2[iv, iu + 1] * tu
            b = g2[iv + 1, iu] * (1 - tu) + g2[iv + 1, iu + 1] * tu
            acc += amp * (a * (1 - tv) + b * tv)
            tot += amp
            amp *= 0.55
        return acc / tot

    faces = {"rt": lambda s, t: (np.ones_like(s), -s, t), "lf": lambda s, t: (-np.ones_like(s), s, t),
             "bk": lambda s, t: (s, np.ones_like(s), t), "ft": lambda s, t: (-s, -np.ones_like(s), t),
             "up": lambda s, t: (-t, -s, np.ones_like(s)), "dn": lambda s, t: (t, -s, -np.ones_like(s))}
    c = (np.arange(n) + 0.5) / n
    uu, vv = np.meshgrid(c, c)
    s, t = 2 * uu - 1, 1 - 2 * vv
    horizon = np.array([0.84, 0.82, 0.78])
    zenith = np.array([0.50, 0.60, 0.74])
    warm = np.array([1.0, 0.84, 0.60])
    for name, f in faces.items():
        x, y, z = f(s, t)
        ln = np.sqrt(x * x + y * y + z * z)
        x, y, z = x / ln, y / ln, z / ln
        el = np.arcsin(np.clip(z, -1, 1))
        k = np.clip(el / 0.9, 0, 1) ** 0.6
        col = horizon * (1 - k[..., None]) + zenith * k[..., None]
        cosang = np.clip(x * sun[0] + y * sun[1] + z * sun[2], -1, 1)
        ang = np.arccos(cosang)
        glow = 0.55 * np.exp(-ang / 0.10) + 0.22 * np.exp(-ang / 0.6)
        col = col * (1 - 0.3 * glow[..., None]) + warm * glow[..., None]
        # thin cloud streaks on a plane over the land
        h = np.maximum(z, 0.02)
        pu, pv = x / h * 0.06 + 0.3, y / h * 0.18 + 0.7
        cl = noise(pu, pv)
        cover = np.clip((cl - 0.52) * 3.2, 0, 1) * np.clip((el - 0.02) / 0.15, 0, 1)
        lit = 0.85 + 0.5 * np.exp(-ang / 0.5)
        cloud = np.array([0.88, 0.86, 0.84]) * lit[..., None] * np.array([1.0, 0.96, 0.9])
        col = col * (1 - 0.55 * cover[..., None]) + cloud * 0.55 * cover[..., None]
        disc = np.clip((0.012 - ang) / 0.004, 0, 1)
        col = col * (1 - disc[..., None]) + np.array([1.0, 0.97, 0.88]) * 1.15 * disc[..., None]
        # the haze band near the horizon and below it
        band = np.exp(-np.maximum(el, 0) / 0.07)[..., None]
        col = col * (1 - 0.6 * band) + horizon * 0.6 * band
        col[el < 0] = horizon * 0.95
        Image.fromarray((col.clip(0, 1) * 255).astype(np.uint8)).save(out / f"{NAME}_{name}.jpg", quality=94)


def write_floor():
    """Forest floor: brown soil with fallen leaves, a few needles and moss;
    seamless, 512 x 512."""
    import numpy as np
    from PIL import Image, ImageDraw, ImageFilter
    out = ROOT / "baseq3r" / "textures" / "q3r_timberrun"
    n = 512
    rng = random.Random(31)
    nrng = np.random.default_rng(31)
    small = nrng.random((16, 16))
    base = np.asarray(Image.fromarray((np.tile(small, (3, 3)) * 255).astype(np.uint8)).resize(
        (n * 3, n * 3), Image.BICUBIC).crop((n, n, 2 * n, 2 * n))).astype(np.float32) / 255
    grain = nrng.random((n, n)) * 0.25
    soil = np.array([74, 54, 38], np.float32)
    img = soil * (0.75 + 0.35 * base[..., None] + grain[..., None])
    im = Image.fromarray(img.clip(0, 255).astype(np.uint8))
    d = ImageDraw.Draw(im)
    colours = [(176, 96, 34), (196, 132, 40), (150, 70, 30), (120, 78, 40), (206, 160, 60), (98, 64, 38),
               (160, 110, 50), (130, 50, 28)]
    # moss patches
    for _ in range(40):
        x, y, r = rng.uniform(0, n), rng.uniform(0, n), rng.uniform(6, 22)
        for ox in (-n, 0, n):
            for oy in (-n, 0, n):
                d.ellipse((x + ox - r, y + oy - r, x + ox + r, y + oy + r), fill=(64, 72, 40))
    for k in range(2000):
        x, y = rng.uniform(0, n), rng.uniform(0, n)
        L, W = rng.uniform(5, 11), rng.uniform(2.5, 5)
        a = rng.uniform(0, math.pi)
        col = rng.choice(colours)
        shade = rng.uniform(0.5, 0.82)
        col = tuple(int(c * shade) for c in col)
        ca, sa = math.cos(a), math.sin(a)
        pts = []
        for j in range(10):
            th = 2 * math.pi * j / 10
            px, py = L * math.cos(th), W * math.sin(th) * (1 - 0.3 * math.cos(th))
            pts.append((px * ca - py * sa, px * sa + py * ca))
        for ox in (-n, 0, n):
            for oy in (-n, 0, n):
                if -15 < x + ox < n + 15 and -15 < y + oy < n + 15:
                    d.polygon([(x + ox + px, y + oy + py) for px, py in pts], fill=col,
                              outline=tuple(int(c * 0.85) for c in col))
    # needles and twigs
    for _ in range(500):
        x, y = rng.uniform(0, n), rng.uniform(0, n)
        a, L = rng.uniform(0, math.pi), rng.uniform(4, 14)
        col = (110, 80, 50) if rng.random() < 0.7 else (60, 44, 30)
        for ox in (-n, 0, n):
            for oy in (-n, 0, n):
                d.line((x + ox, y + oy, x + ox + L * math.cos(a), y + oy + L * math.sin(a)), fill=col, width=1)
    im = im.filter(ImageFilter.GaussianBlur(0.8))
    im.save(out / "forestfloor.jpg", quality=92)


# ---------------------------------------------------------------- shaders, textures

LAYER_TEX = (("forest", "textures/q3r_timberrun/forestfloor.jpg", 256),
             ("gravel", "textures/q3r_dirt_grass/stones01.tga", 160),
             ("grass", "textures/q3r_dirt_grass/grass07.tga", 320),
             ("rock", "textures/q3r_dirt_grass/rock03.tga", 288),
             ("cliff", "textures/q3r_dirt_grass/rock03.tga", 320),
             ("mud", "textures/q3r_dirt_grass/darkdirt02.tga", 224))
PARENTS = (("dirt", ("surfaceparm SURF_DIRT",), 0), ("gravel", ("surfaceparm SURF_GRAVEL",), 1),
           ("grass", ("surfaceparm SURF_GRASS",), 2), ("rock", (), 3),
           ("wet", ("surfaceparm SURF_GRAVEL", "surfaceparm SURF_WET"), 5))
TINT = {"forest": "\t\trgbGen const ( 0.95 0.90 0.84 )", "gravel": "\t\trgbGen const ( 0.92 0.88 0.80 )",
        "grass": "\t\trgbGen const ( 0.86 0.82 0.62 )", "rock": "\t\trgbGen const ( 0.80 0.78 0.74 )",
        "cliff": "\t\trgbGen const ( 0.76 0.74 0.70 )", "mud": "\t\trgbGen const ( 0.62 0.56 0.48 )"}


def tcgen(layer):
    name, _, tile = LAYER_TEX[layer]
    k = 1.0 / tile
    if name == "cliff":
        return f"\t\ttcGen vector ( {0.71 * k:.7f} {0.71 * k:.7f} 0 ) ( 0 0 {k:.7f} )"
    return f"\t\ttcGen vector ( {k:.7f} 0 0 ) ( 0 {k:.7f} 0 )"


def stage(layer, blend):
    lines = ["\t{", f"\t\tmap {LAYER_TEX[layer][1]}"]
    if blend:
        lines += ["\t\tblendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA", "\t\talphaGen vertex"]
    return lines + [tcgen(layer), TINT.get(LAYER_TEX[layer][0], "\t\trgbGen identity"), "\t}"]


def tail():
    return ["\t{", f"\t\tmap textures/{TEX}macro.jpg", "\t\tblendFunc GL_DST_COLOR GL_SRC_COLOR",
            "\t\ttcGen vector ( 0.00023 0 0 ) ( 0 0.00023 0 )", "\t\trgbGen identity", "\t}",
            "\t{", "\t\tmap $lightmap", "\t\tblendFunc GL_DST_COLOR GL_ZERO", "\t\trgbGen identity", "\t}"]


def lit(name, image, extra=(), tex_extra=()):
    rg = [] if any("rgbGen" in s for s in tex_extra) else ["\t\trgbGen identity"]
    return [f"textures/{TEX}{name}", "{", f"\tqer_editorimage {image}", *extra, "\t{", f"\t\tmap {image}",
            *tex_extra, *rg, "\t}", "\t{", "\t\tmap $lightmap", "\t\tblendFunc GL_DST_COLOR GL_ZERO",
            "\t\trgbGen identity", "\t}", "}", ""]


def tinted(name, image, rgb, extra=()):
    return lit(name, image, extra, (f"\t\trgbGen const ( {rgb[0]:.2f} {rgb[1]:.2f} {rgb[2]:.2f} )",))


def glow(name, rgb, light):
    return [f"textures/{TEX}{name}", "{", "\tqer_editorimage textures/q3r_signage/white.jpg", "\tsurfaceparm nomarks",
            "\tsurfaceparm nolightmap", "\tsurfaceparm nonsolid", f"\tq3map_surfacelight {light}",
            f"\tq3map_lightRGB {rgb[0]} {rgb[1]} {rgb[2]}", "\t{", "\t\tmap textures/q3r_signage/white.jpg",
            f"\t\trgbGen const ( {rgb[0]} {rgb[1]} {rgb[2]} )", "\t}", "}", ""]


def write_shader():
    T = f"textures/{TEX}"
    out = [f"// {TITLE} - A to B gravel sprint through an autumn forest, early in the morning.",
           "// Written by tools/maptools/make_timberrun.py. Compile with -custinfoparms.", ""]
    out += [f"{T}sky", "{", f"\tqer_editorimage env/{NAME}/{NAME}_ft.jpg", "\tsurfaceparm noimpact",
            "\tsurfaceparm nolightmap", "\tsurfaceparm sky", "\tq3map_globaltexture",
            f"\tq3map_sun 1 0.84 0.64 230 {SUN_YAW} {SUN_PITCH}", "\tq3map_skylight 240 4",
            f"\tq3gl2_sun 1 0.84 0.64 150 {SUN_YAW} {SUN_PITCH} 0.6", f"\tskyparms env/{NAME}/{NAME} - -",
            f"\tq3r_haze 0.80 0.80 0.78 3000 1300 {BOUNDS['water_z'] + 60} 320 0.8"
            "\t// Q3Rally: morning mist, thick in the creek valley", "}", ""]
    out += [f"{T}creek", "{", "\tqer_editorimage textures/q3r_liquids/water.tga", "\tqer_trans 0.35",
            "\tsurfaceparm nonsolid", "\tsurfaceparm trans", "\tsurfaceparm nomarks", "\tsurfaceparm nolightmap",
            "\tq3map_globaltexture", "\tcull disable",
            "\tq3r_water textures/q3r_liquids/water_normal.tga 160 0.03 0.014", "\tq3r_waterColor 0.10 0.13 0.10 0.55",
            "\tq3r_waterFresnel 0.10 4", "\tq3r_waterSpecular 0.8",
            "\t{", "\t\tmap textures/q3r_liquids/water.tga", "\t\tblendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA",
            "\t\trgbGen const ( 0.22 0.27 0.22 )", "\t\talphaGen const 0.62", "\t\ttcMod scale 0.5 0.5",
            "\t\ttcMod scroll 0.02 0.004", "\t}", "}", ""]
    out += tree_shaders()
    out += sign_shaders()
    out += lit("logbark", f"{T}logbark.jpg", ("\tsurfaceparm SURF_DIRT",))
    out += lit("logend", f"{T}logend.jpg", ("\tsurfaceparm SURF_DIRT",))
    ns = ("\tsurfaceparm nonsolid", "\tsurfaceparm nomarks")
    for name, image in (("logbark", "logbark.jpg"), ("logend", "logend.jpg"), ("logend_mark", "logend_mark.jpg"),
                        ("bale", "bale.jpg"), ("bale_end", "bale_end.jpg")):
        out += lit(name + "_ns" if name.startswith("log") else name, f"{T}{image}", ns)
    out += lit("metal", "textures/base_trim/tin.jpg", ("\tsurfaceparm SURF_METAL",))
    out += lit("banner_start", f"{T}banner_start.jpg")
    out += lit("banner_finish", f"{T}banner_finish.jpg")
    out += [f"{T}trunk", "{", "\tqer_editorimage textures/base_floor/concrete.jpg", "\tqer_trans 0.15",
            "\t// solid core of a tree trunk, invisible", "\tsurfaceparm nodraw", "\tsurfaceparm nolightmap",
            "\tsurfaceparm trans", "\tsurfaceparm nomarks", "\tsurfaceparm detail", "}", ""]
    out += [f"{T}boundary", "{", "\tqer_editorimage textures/base_floor/concrete.jpg", "\tqer_trans 0.15",
            "\t// invisible but SOLID; trans lets light through",
            "\tsurfaceparm nodraw", "\tsurfaceparm nolightmap", "\tsurfaceparm trans", "\tsurfaceparm nomarks",
            "\tsurfaceparm noimpact", "\tsurfaceparm detail", "}", ""]
    out += ["// ground: t_* parents on the brush sides (collision, friction); q3map2 draws",
            f"// ter_<layer> / ter_<a>to<b> from maps/{NAME}_index.pcx. Layers: " +
            ", ".join(f"{i} {n}" for i, (n, _, _) in enumerate(LAYER_TEX)), ""]
    for name, parms, layer in PARENTS:
        out += [f"{T}t_{name}", "{", f"\tqer_editorimage {LAYER_TEX[layer][1]}", "\tq3map_indexed",
                "\tq3map_shadeAngle 120", "\tq3map_nonplanar"] + [f"\t{p}" for p in parms] + ["}", ""]
    for i in range(len(LAYER_TEX)):
        out += [f"{T}ter_{i}", "{", f"\tqer_editorimage {LAYER_TEX[i][1]}", "\tq3map_shadeAngle 120",
                "\tq3map_lightmapSampleSize 24",
                "\tq3map_nonplanar"] + stage(i, False) + tail() + ["}", ""]
    for i, j in combinations(range(len(LAYER_TEX)), 2):
        out += [f"{T}ter_{i}to{j}", "{", f"\tqer_editorimage {LAYER_TEX[i][1]}", "\tq3map_shadeAngle 120",
                "\tq3map_lightmapSampleSize 24",
                "\tq3map_nonplanar"] + stage(i, False) + stage(j, True) + tail() + ["}", ""]
    (ROOT / "baseq3r" / "scripts" / f"{NAME}.shader").write_text("\n".join(out), encoding="utf-8")


def write_textures():
    from PIL import Image, ImageDraw, ImageFilter
    out = ROOT / "baseq3r" / "textures" / "q3r_timberrun"
    out.mkdir(parents=True, exist_ok=True)
    rnd = random.Random(21)
    size = 512
    acc = [0.0] * (size * size)
    for cells, weight in ((4, 1.0), (8, 0.6), (16, 0.35), (32, 0.18)):
        small = Image.new("F", (cells, cells))
        small.putdata([rnd.random() for _ in range(cells * cells)])
        tiled = Image.new("F", (cells * 3, cells * 3))
        for i in range(3):
            for j in range(3):
                tiled.paste(small, (i * cells, j * cells))
        big = tiled.resize((size * 3, size * 3), Image.BICUBIC).crop((size, size, 2 * size, 2 * size))
        acc = [a + weight * v for a, v in zip(acc, list(big.get_flattened_data()))]
    lo, hi = min(acc), max(acc)
    m = Image.new("L", (size, size))
    m.putdata([int(128 + ((v - lo) / (hi - lo) - 0.5) * 84) for v in acc])
    m.filter(ImageFilter.GaussianBlur(1.2)).convert("RGB").save(out / "macro.jpg", quality=92)
    # start / finish banners: chequer band with a word (5x7 dot font)
    for name, word in (("banner_start", "START"), ("banner_finish", "FINISH")):
        im = Image.new("RGB", (512, 64), (250, 250, 245))
        d = ImageDraw.Draw(im)
        for k in range(0, 512, 16):
            for r in (0, 1):
                if (k // 16 + r) % 2 == 0:
                    d.rectangle([k, r * 8, k + 15, r * 8 + 7], fill=(20, 20, 20))
                    d.rectangle([k, 48 + r * 8, k + 15, 48 + r * 8 + 7], fill=(20, 20, 20))
        cell = 4
        wx = len(word) * 6 * cell - cell
        x0 = (512 - wx) // 2
        for i, ch in enumerate(word):
            for r, row in enumerate(FONT[ch]):
                for c, bit in enumerate(row):
                    if bit == "1":
                        x = x0 + (i * 6 + c) * cell
                        y = 18 + r * cell
                        d.rectangle([x, y, x + cell - 1, y + cell - 1], fill=(200, 30, 25))
        im.save(out / f"{name}.jpg", quality=92)


FONT = {
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "F": ["11111", "10000", "10000", "11110", "10000", "10000", "10000"],
    "I": ["11111", "00100", "00100", "00100", "00100", "00100", "11111"],
    "N": ["10001", "11001", "10101", "10011", "10001", "10001", "10001"],
    "H": ["10001", "10001", "10001", "11111", "10001", "10001", "10001"],
}


def index_map():
    from PIL import Image
    x0, x1, y0, y1 = BOUNDS["x0"], BOUNDS["x1"], BOUNDS["y0"], BOUNDS["y1"]
    w, h = (x1 - x0) // STEP + 1, (y1 - y0) // STEP + 1
    im = Image.new("L", (w, h))
    im.putdata([MAT[x0 + i * STEP, y1 - j * STEP] for j in range(h) for i in range(w)])
    im.save(ROOT / "baseq3r" / "maps" / f"{NAME}_index.pcx")


# ---------------------------------------------------------------- checks and output

def speed_profile():
    vmax, alat, acc, brake = 560.0, 900.0, 160.0, 450.0
    n = len(PATH)
    lim = [min(vmax, math.sqrt(alat / max(1e-6, abs(KAPPA[i])))) for i in range(n)]
    v = lim[:]
    v[0] = 0.0
    for i in range(1, n):
        g = (ZTAB[i] - ZTAB[i - 1]) / SAMPLE
        v[i] = min(lim[i], math.sqrt(max(0.0, v[i - 1] ** 2 + 2 * (acc - 800 * g) * SAMPLE)))
    for i in range(n - 2, -1, -1):
        v[i] = min(v[i], math.sqrt(v[i + 1] ** 2 + 2 * brake * SAMPLE))
    return v


def validate():
    n = len(PATH)
    grades = [abs(ZTAB[i + 1] - ZTAB[i]) / SAMPLE for i in range(n - 1)]
    assert max(grades) < 0.25, max(grades)
    worst = max(abs(ground(*PATH[i]) - road_z(S[i])) for i in range(0, n, 3))
    # legs must not merge: gap between the road edges of far-apart parts
    close = 1e9
    for i in range(0, n, 3):
        for j in nearby(*PATH[i], 1600):
            if abs(j - i) * SAMPLE > 3000:
                close = min(close, math.dist(PATH[i], PATH[j]) - half_width(S[i]) - half_width(S[j]))
    assert close > 450, ("legs too close", close)
    starts = [e for e in ENTITIES if e["classname"] in ("info_player_start", "info_player_deathmatch")]
    for e in starts:
        x, y, z = map(float, e["origin"].split())
        for slo, shi in SOLIDS:
            assert not (x + 60 > slo[0] and x - 60 < shi[0] and y + 60 > slo[1] and y - 60 < shi[1]
                        and z + 30 > slo[2] and z - 30 < shi[2]), (e, slo, shi)
        assert z - 26 > max(ground(x + dx, y + dy) for dx in (-50, 50) for dy in (-50, 50)), e
    wz = BOUNDS["water_z"]
    wet = [i for i in range(n) if ZTAB[i] < wz]
    assert wet and all(i in range(*SECTIONS["creek"]) for i in wet), "water on the road outside the ford"
    # water must only show in the channel and on the ford
    x0, y0, x1, y1 = CREEK_BOX
    flooded = sum(1 for (x, y), z in GRID.items() if x0 <= x <= x1 and y0 <= y <= y1 and z < wz
                  and creek_distance(x, y) > CREEK_W and nearest(x, y, 600)[1] not in wet)
    v = speed_profile()
    t = sum(SAMPLE / max(30.0, (v[i] + v[i + 1]) / 2) for i in range(BOUNDS["i_start"], BOUNDS["i_finish"]))
    return {"map": NAME, "course_length": round(S[BOUNDS["i_finish"]] - S[BOUNDS["i_start"]]), "path_length": round(length()),
            "run_out": round(length() - S[BOUNDS["i_finish"]]), "estimated_time_s": round(t, 1),
            "start_z": round(ZTAB[0]), "finish_z": round(ZTAB[-1]), "lowest_z": round(min(ZTAB)),
            "max_grade_pct": round(100 * max(grades), 1),
            "min_turn_radius": round(1 / max(abs(k) for k in KAPPA)),
            "road_mesh_error": round(worst, 2), "closest_legs_gap": round(close),
            "sections": {k: [round(S[a]), round(S[b])] for k, (a, b) in SECTIONS.items()},
            "crests_at": [round(S[i]) for i in CREST_AT],
            "ford": {"at": round(S[BOUNDS["i_ford"]]), "water_z": wz, "length": len(wet) * SAMPLE,
                     "depth": round(wz - min(ZTAB), 1), "flooded_grid_points_elsewhere": flooded},
            "creek_length": round(sum(math.dist(a, b) for a, b in zip(CREEK, CREEK[1:]))),
            "terrain_triangles": len(TRIS),
            "leaves": {s: sum(1 for leaf in LEAVES if leaf[2] == s) for s in (64, 128, 256, 512)},
            "brushes": len(BRUSHES) + sum(len(g) for g in GROUPS.values()), "checkpoints": CHECKPOINTS,
            "wall_gaps": BOUNDS.get("wall_gaps"), "signs": BOUNDS.get("signs"), "forest": BOUNDS.get("forest"),
            "details": BOUNDS.get("details"), "speakers": BOUNDS.get("speakers")}


def overview(report):
    from PIL import Image, ImageDraw
    x0, x1, y0, y1 = BOUNDS["x0"], BOUNDS["x1"], BOUNDS["y0"], BOUNDS["y1"]
    scale = max(x1 - x0, y1 - y0) / 1800
    w, h = int((x1 - x0) / scale), int((y1 - y0) / scale)
    im = Image.new("RGB", (w, h))
    pix = im.load()
    colors = {L_FOREST: (104, 96, 62), L_GRAVEL: (206, 196, 172), L_GRASS: (150, 160, 92),
              L_ROCK: (150, 146, 138), L_CLIFF: (130, 126, 120), L_MUD: (120, 104, 84)}
    zmin, zmax = min(GRID.values()), max(GRID.values())
    wz = BOUNDS["water_z"]
    bx0, by0, bx1, by1 = CREEK_BOX
    for v in range(h):
        y = y1 - v * scale
        gy = y0 + round((y - y0) / STEP) * STEP
        for u in range(w):
            x = x0 + u * scale
            gx = x0 + round((x - x0) / STEP) * STEP
            z = GRID.get((gx, gy), zmin)
            if z < wz and bx0 <= x <= bx1 and by0 <= y <= by1:
                pix[u, v] = (70, 120, 150)
                continue
            zx = GRID.get((gx + STEP, gy), z)
            shade = 0.62 + 0.38 * (z - zmin) / (zmax - zmin) - (zx - z) / STEP * 0.25
            c = colors[MAT.get((gx, gy), L_FOREST)]
            pix[u, v] = tuple(max(0, min(255, int(k * shade))) for k in c)
    d = ImageDraw.Draw(im)
    for i in CREST_AT:
        u, vv = (PATH[i][0] - x0) / scale, (y1 - PATH[i][1]) / scale
        d.polygon([(u, vv - 9), (u + 8, vv + 6), (u - 8, vv + 6)], outline=(255, 220, 60), width=2)
    for e in ENTITIES:
        x, y, _ = map(float, e["origin"].split())
        u, vv = (x - x0) / scale, (y1 - y) / scale
        if e["classname"] == "info_notnull":
            d.ellipse((u - 6, vv - 6, u + 6, vv + 6), outline=(255, 255, 255), width=2)
            d.text((u + 8, vv - 9), e["targetname"], fill=(255, 255, 255))
    out = ROOT / "docs" / "maps" / "timberrun"
    out.mkdir(parents=True, exist_ok=True)
    im.save(out / "timberrun_layout.png")
    (out / "validation.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


def write_route():
    """Intro camera flight along the course (same format as the recorded routes)."""
    n = len(PATH)
    v = speed_profile()
    lines = ["Q3RALLY_INTRO_ROUTE 1", f"map {NAME}", "track_length 1", "track_reversed 0"]
    frames, t = [], 0.0
    target = 20.0
    total = sum(SAMPLE / max(120.0, v[k]) for k in range(n))
    for k in range(0, n, 2):
        x, y, z = road_point(k)
        j = idx(k + 1)
        pitch = -math.degrees(math.atan2(ZTAB[j] - ZTAB[k], SAMPLE))
        frames.append(f"{round(t * 1000)} {x:.3f} {y:.3f} {z + 60:.3f} {pitch:.3f} {heading_deg(k):.3f} 0.000")
        t += sum(SAMPLE / max(120.0, v[idx(k + m)]) for m in range(2)) * target / total
    lines.append(f"frames {len(frames)}")
    out = ROOT / "baseq3r" / "intro_routes"
    out.mkdir(parents=True, exist_ok=True)
    (out / f"{NAME}_tl1_rev0.route").write_text("\n".join(lines + frames) + "\n", encoding="ascii")


def write_map():
    fields = {"classname": "worldspawn", "message": TITLE, "_ambient": "26", "_minlight": "18",
              "gridsize": "128 128 256", "_blocksize": "1024 1024 4096",
              # vis drops what lies more than about 9,400 units away; the haze hides it long before
              "_farplanedist": "7000",
              "weather": "rain", "weatherChance": "40", "weatherCycle": "120 240", "weatherStrength": "0.3 0.8",
              "weatherWind": "20 120"}
    text = ["// Generated by tools/maptools/make_timberrun.py. Q3 brush format.",
            f"// The terrain func_groups use maps/{NAME}_index.pcx at compile time.", "{"]
    text += [f'"{k}" "{v}"' for k, v in fields.items()]
    text += BRUSHES
    text.append("}")
    x0, x1, y0, y1 = BOUNDS["x0"], BOUNDS["x1"], BOUNDS["y0"], BOUNDS["y1"]
    for group in ("dirt", "gravel", "grass", "rock", "wet"):
        if group not in GROUPS:
            continue
        keys = {"classname": "func_group", "_terrain": f"timberrun ground, {group} friction",
                "_indexmap": f"maps/{NAME}_index.pcx", "_layers": str(len(LAYER_TEX)),
                "_shader": TEX + "ter", "min": f"{x0} {y0} {FLOOR}", "max": f"{x1} {y1} 4096"}
        text.append("{\n" + "\n".join(f'"{k}" "{v}"' for k, v in keys.items()))
        text += GROUPS[group]
        text.append("}")
    text += TRIGGERS
    for ent in ENTITIES:
        text.append("{\n" + "\n".join(f'"{k}" "{v}"' for k, v in ent.items()) + "\n}")
    target = ROOT / "baseq3r" / "maps" / f"{NAME}.map"
    target.write_text("\n".join(text) + "\n", encoding="ascii")
    arena = ROOT / "baseq3r" / "scripts" / f"{NAME}.arena"
    arena.write_text(
        '{\n'
        f'\tmap "{NAME}"\n'
        f'\tlongname "{TITLE}"\n'
        '\tauthor "Q3Rally Team"\n'
        '\ttype "q3r_sprint"\n'
        '\tstarts "8"\n'
        '\tlaps "1"\n'
        f'\tcheckpoints "{len(CHECKPOINTS)}"\n'
        '\tobserverspots "6"\n'
        '\tweapons "0"\n'
        '\tpowerups "0"\n'
        '\tbotsupport "Ok"\n'
        '}\n'
        , encoding="ascii")
    return target


def main():
    build_path()
    hash_path()
    sections()
    build_profile()
    build_creek()
    build_terrain()
    course()
    gantry(BOUNDS["i_start"], "banner_start")
    gantry(BOUNDS["i_finish"], "banner_finish")
    end_walls()
    log_pile()
    corridor()
    signs()
    details()
    sounds()
    forest()
    water()
    misc_entities()
    shell()
    report = validate()
    write_textures()
    write_floor()
    write_wood_textures()
    write_detail_textures()
    write_sign_textures()
    sign_model("l")
    sign_model("r")
    write_sky()
    write_trees()
    write_shader()
    index_map()
    target = write_map()
    write_bpd()
    write_route()
    overview(report)
    print(json.dumps({k: v for k, v in report.items() if k != "checkpoints"}, indent=2))
    for c in report["checkpoints"]:
        print(c)
    print("Wrote", target)


if __name__ == "__main__":
    main()
