#!/usr/bin/env python3
"""Generate q3r_dd_02 "Figure of 8": the stadium with a real figure of eight.

Rework of the original map. The stadium stays as it was (stands with the
crowd, advertising boards, press box, the truss rig with its lights, the
skyline): it is read from tools/maptools/src/q3r_dd_02_orig.map. Everything
on the field is new. The original field was flat: the eight only painted on
the ground, four ramps up to a small platform in the middle and every spawn
around it. Now:

- The eight is a real track: asphalt 330 wide with red and white kerbs, two
  loops (580 to the centre line) and two straights crossing in the middle.
- One straight goes over the crossing on a bridge (deck 112 high, 440 long,
  ramps of 340 at 17 degrees), the other runs underneath. The railgun lies
  on the deck, the mega health under it.
- The loops hold islands: the tyre store (a ring of tyre walls, a concrete
  garage with health inside, the mines) and the scrap yard (a ring of
  concrete barriers, wrecks and containers, the smoke).
- The two free corners: the paddock (trucks, containers, a fuel station with
  fuel cans, the rocket launcher between the containers) and the mud pit
  (two kickers over a mud pool, the flamethrower in it, the shotgun).
- 12 spawns around the outside of the loops, in the corners and on the
  islands, all facing along the track.
- Light: four flood light masts at the corners of the field, light under
  the bridge, the fuel station and the garage; less haze.
- Derby added to the game types.

Props are models (tools/maptools/make_figure8_models.py and stock
mapobjects) with clip brushes from here. Point symmetric about the middle
of the field (-32 -32), but for the corner zones. Writes the .map,
scripts/q3r_dd_02.arena and docs/maps/figure8/ (layout and validation).
Run with Python 3 + numpy + Pillow.
"""
from __future__ import annotations

import json
import math
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(Path(__file__).resolve().parent))
NAME = "q3r_dd_02"
SRC = ROOT / "tools" / "maptools" / "src" / "q3r_dd_02_orig.map"
DETAIL = 134217728

OX, OY = -32.0, -32.0     # middle of the field (inside the advertising boards: -1600..1536)
HALF = 1568.0             # half the field
C1 = (-660.0, 660.0)      # loop centres, relative to the middle
C2 = (660.0, -660.0)
R = 580.0                 # loop centre line
H = 165.0                 # half the track
K = 22.0                  # kerb width
DECK = 112.0              # bridge deck top
DECK_T = 16.0             # deck thickness
DECK_S = 220.0            # deck from -DECK_S to DECK_S along its straight
RAMP_S = 560.0            # ramp foot
TOP = 6.0                 # track and kerb top; 4 over the grass, so the two never fight for the depth buffer
GRASS, ZONE = 2.0, 3.0    # grass; the corner zones (on the base, the grass is cut out under them)
ISLE = 8.0                # the islands in the loops, a little over the track
PAD = (300.0, 1440.0)     # the paddock square (north-east); the mud pit is the same square south-west
MODELS = "models/mapobjects/q3r_figure8"

MAT = {
    "grass": "q3r_dirt_grass/grass07", "track": "q3r_road/asph03", "kerb": "q3r_road/cement_redwhite",
    "island": "q3r_dirt_grass/darkdirt01", "mud": "q3r_dirt_grass/dirt02", "pool": "q3r_dirt_grass/darkdirt02",
    "pad": "q3r_road/asph02parking", "concrete": "q3r_road/concrete", "concrete2": "q3r_road/concrete2",
    "hazard": "base_trim/yellow_rustbx128", "kicker": "q3r_metals/cormetal03", "steel": "q3r_metals/plate01",
    "canopy": "q3r_metals/metrust01", "lamp": "base_light/baslt4_1_4k", "caulk": "common/caulk",
    "clip": "common/full_clip",
}

BRUSHES, ENTITIES, SOLIDS, PROPS = [], [], [], []
SHAPES = []      # (relative polygon, colour) for the overview: garage, canopy, kickers, pool


# ---------------------------------------------------------------- brush basics (as make_david5.py)

def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


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


def face(points, material, detail=True, tex=None):
    p = " ".join("( %s )" % " ".join(num(v) for v in xyz) for xyz in points)
    xo, yo, rot, ss, ts = tex if tex else (0, 0, 0, 0.5, 0.5)
    return f"{p} {material} {num(xo)} {num(yo)} {num(rot)} {num(ss)} {num(ts)} {DETAIL if detail else 0} 0 0"


def polytope(planes):
    eq = [(n, dot(n, p)) for n, p in planes]
    out = []
    for i in range(len(eq)):
        for j in range(i + 1, len(eq)):
            for k in range(j + 1, len(eq)):
                (n1, d1), (n2, d2), (n3, d3) = eq[i], eq[j], eq[k]
                c23, c31, c12 = cross(n2, n3), cross(n3, n1), cross(n1, n2)
                det = dot(n1, c23)
                if abs(det) < 1e-9:
                    continue
                p = tuple((d1 * c23[m] + d2 * c31[m] + d3 * c12[m]) / det for m in range(3))
                if all(dot(n, p) <= d + 0.01 for n, d in eq):
                    out.append(p)
    return out


def convex(planes, detail=True, solid=True):
    corners = polytope([(n, p) for n, p, _ in planes])
    assert len(corners) >= 4, planes
    faces = []
    for n, p, m in planes:
        mat, tex = (m if isinstance(m, tuple) else (m, None))
        faces.append(face(plane(n, p), mat, detail=detail, tex=tex))
    BRUSHES.append("{\n" + "\n".join(faces) + "\n}")
    if solid:
        SOLIDS.append([(n, dot(n, p)) for n, p, _ in planes])
    return corners


def prism(xy, z0, z1, side, top=None, bot=None, detail=True, sides=None, solid=True):
    """Upright convex prism over the polygon xy (absolute). sides: a material per edge."""
    area = sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(xy, xy[1:] + xy[:1]))
    if area < 0:
        xy = xy[::-1]
        if sides:
            sides = sides[::-1][1:] + sides[::-1][:1]
    planes = [((0, 0, 1), (xy[0][0], xy[0][1], z1), top or side), ((0, 0, -1), (xy[0][0], xy[0][1], z0), bot or side)]
    for i, ((ax, ay), (bx, by)) in enumerate(zip(xy, xy[1:] + xy[:1])):
        planes.append(((by - ay, -(bx - ax), 0.0), (ax, ay, z0), sides[i] if sides else side))
    return convex(planes, detail=detail, solid=solid)


def entity(classname, x, y, z, **keys):
    """x, y relative to the middle of the field."""
    fields = {"classname": classname, "origin": f"{num(x + OX)} {num(y + OY)} {num(z)}", **keys}
    ENTITIES.append(fields)
    return fields


def A(p):
    """Relative point to absolute."""
    return (p[0] + OX, p[1] + OY)


def rot2(v, a):
    c, s = math.cos(a), math.sin(a)
    return (v[0] * c - v[1] * s, v[0] * s + v[1] * c)


def obox(cx, cy, yaw, hx, hy):
    """Corners of a box (relative centre, yaw in degrees, half sizes) in absolute coordinates."""
    a = math.radians(yaw)
    return [A((cx + p[0], cy + p[1])) for p in (rot2((-hx, -hy), a), rot2((hx, -hy), a), rot2((hx, hy), a), rot2((-hx, hy), a))]


# ---------------------------------------------------------------- the eight

def tangent(c, sign):
    D = math.hypot(*c)
    th = math.atan2(c[1], c[0]) + sign * math.asin(R / D)
    L = math.sqrt(D * D - R * R)
    return (L * math.cos(th), L * math.sin(th))


T1, T2 = tangent(C1, -1), tangent(C1, 1)          # line A (over the bridge), line B (underneath)
LEN = math.hypot(*T1)
UA = (T1[0] / LEN, T1[1] / LEN)
UB = (T2[0] / LEN, T2[1] / LEN)
VA, VB = (-UA[1], UA[0]), (-UB[1], UB[0])
ANG_A, ANG_B = math.degrees(math.atan2(*UA[::-1])), math.degrees(math.atan2(*UB[::-1]))


def on_line(u, v, s, t):
    return (u[0] * s + v[0] * t, u[1] * s + v[1] * t)


def kerb_tex(direction_deg):
    """cement_redwhite has its stripes along the texture's s: turn them across the kerb."""
    return (MAT["kerb"], (0, 0, -direction_deg, 0.25, 0.25))


def track_quad(p, z_top=TOP, mat=None, tex_dir=None):
    m = mat or MAT["track"]
    top = kerb_tex(tex_dir) if tex_dir is not None and mat == MAT["kerb"] else m
    prism([A(q) for q in p], 0.0, z_top, MAT["hazard"] if m == MAT["track"] else m, top=top, bot=MAT["caulk"])


def straight(u, v, s0, s1):
    """Asphalt and both kerbs along a straight from s0 to s1."""
    track_quad([on_line(u, v, s0, -H), on_line(u, v, s1, -H), on_line(u, v, s1, H), on_line(u, v, s0, H)])
    d = math.degrees(math.atan2(u[1], u[0]))
    for t0, t1 in ((-H - K, -H), (H, H + K)):
        track_quad([on_line(u, v, s0, t0), on_line(u, v, s1, t0), on_line(u, v, s1, t1), on_line(u, v, s0, t1)],
                   mat=MAT["kerb"], tex_dir=d)


def loop(c, t_a, t_b, step=4.0):
    """The arc of a loop from the tangent point t_a round the far side to t_b."""
    a0 = math.atan2(t_a[1] - c[1], t_a[0] - c[0])
    a1 = math.atan2(t_b[1] - c[1], t_b[0] - c[0])
    far = math.atan2(c[1], c[0])

    def inside(s0, e0):
        lo, hi = min(s0, e0), max(s0, e0)
        return any(lo <= far + 2 * math.pi * k <= hi for k in (-2, -1, 0, 1, 2))
    for e in (a1, a1 + 2 * math.pi, a1 - 2 * math.pi):
        if abs(e - a0) < 2 * math.pi and inside(a0, e):
            a1 = e
            break
    n = max(1, int(abs(a1 - a0) / math.radians(step) + 0.5))
    rings = ((R - H, R + H, MAT["track"]), (R - H - K, R - H, MAT["kerb"]), (R + H, R + H + K, MAT["kerb"]))
    for i in range(n):
        b0 = a0 + (a1 - a0) * i / n
        b1 = a0 + (a1 - a0) * (i + 1) / n
        for r0, r1, mat in rings:
            p = [(c[0] + r0 * math.cos(b0), c[1] + r0 * math.sin(b0)), (c[0] + r1 * math.cos(b0), c[1] + r1 * math.sin(b0)),
                 (c[0] + r1 * math.cos(b1), c[1] + r1 * math.sin(b1)), (c[0] + r0 * math.cos(b1), c[1] + r0 * math.sin(b1))]
            track_quad(p, mat=mat, tex_dir=math.degrees((b0 + b1) / 2) + 90)
    return a0, a1


def eight():
    arcs = [loop(C1, T1, T2), loop(C2, (-T1[0], -T1[1]), (-T2[0], -T2[1]))]
    # line A: on the ground only from the ramp feet to the loops; line B all through
    straight(UA, VA, RAMP_S, LEN)
    straight(UA, VA, -LEN, -RAMP_S)
    straight(UB, VB, -LEN, LEN)
    return arcs


def bridge():
    w = H + 12.0
    deck = [A(on_line(UA, VA, s, t)) for s, t in ((-DECK_S, -w), (DECK_S, -w), (DECK_S, w), (-DECK_S, w))]
    prism(deck, DECK - DECK_T, DECK, MAT["hazard"], top=MAT["track"], bot=MAT["concrete"])
    # ramps: solid wedges from the deck down to the track at the feet; the inner ends are the abutments
    for sign in (1, -1):
        u = (UA[0] * sign, UA[1] * sign)
        v = (VA[0] * sign, VA[1] * sign)
        slope = (TOP - DECK) / (RAMP_S - DECK_S)
        o = A(on_line(u, v, DECK_S, 0))
        n_top = (-slope * u[0], -slope * u[1], 1.0)
        planes = [
            (n_top, (o[0], o[1], DECK), MAT["track"]),
            ((0, 0, -1), (o[0], o[1], 0.0), MAT["caulk"]),
            ((v[0], v[1], 0), (*A(on_line(u, v, DECK_S, H)), 0.0), MAT["concrete"]),
            ((-v[0], -v[1], 0), (*A(on_line(u, v, DECK_S, -H)), 0.0), MAT["concrete"]),
            ((-u[0], -u[1], 0), (o[0], o[1], 0.0), MAT["concrete"]),
            ((u[0], u[1], 0), (*A(on_line(u, v, RAMP_S, 0)), 0.0), MAT["hazard"]),
        ]
        convex(planes)
    # railings along the deck, both sides
    for side in (-1, 1):
        for k in range(7):
            s = -192.0 + k * 64.0
            p = on_line(UA, VA, s, side * (w - 4))
            prop("railing", p[0], p[1], DECK, ANG_A, clip=(32, 3, 30))
    # light under the deck and on the abutments
    for s in (-120.0, 120.0):
        p = on_line(UA, VA, s, 0)
        entity("light", p[0], p[1], DECK - DECK_T - 24, light="260", _color="1 0.86 0.66")


# ---------------------------------------------------------------- props

def prop(name, x, y, z, yaw=0.0, scale=1.0, clip=None, angles=None, model=None, clip_z0=None, remap=None):
    """A model on the field (x, y relative) and its clip box (half x, half y, height) turned with it."""
    path = model or f"{MODELS}/{name}.md3"
    keys = {"model": path}
    if remap:
        keys["_remap"] = f"*;{remap}"     # q3map2: every shader of the model
    if angles:
        keys["angles"] = angles
    else:
        keys["angle"] = num(yaw)
    if scale != 1.0:
        keys["modelscale"] = num(scale)
    entity("misc_model", x, y, z, **keys)
    PROPS.append((name, x, y, yaw, clip))
    if clip:
        hx, hy, hz = clip
        prism(obox(x, y, yaw, hx, hy), z if clip_z0 is None else clip_z0, z + hz, MAT["clip"])


def container(x, y, z, yaw, length, colour):
    prop(f"container{length}_{colour}", x, y, z, yaw, clip=((72.6 if length == 20 else 144.6), 29.6, 62))


def ring(c, r, count, gaps, fn):
    """count pieces round c at radius r, leaving out the slots in gaps; fn(x, y, yaw)."""
    for k in range(count):
        if k in gaps:
            continue
        a = 2 * math.pi * (k + 0.5) / count
        fn(c[0] + r * math.cos(a), c[1] + r * math.sin(a), math.degrees(a) + 90)


def island(c):
    pts = [A((c[0] + 380 * math.cos(2 * math.pi * k / 32), c[1] + 380 * math.sin(2 * math.pi * k / 32))) for k in range(32)]
    prism(pts, 0.0, ISLE, MAT["island"], bot=MAT["caulk"])


def entrances(c, count):
    """Slots facing the bridge, the outside of the loop and across: the ways in."""
    toward = math.atan2(-c[1], -c[0])
    slots = set()
    for off in (0.0, 2 * math.pi / 3, -2 * math.pi / 3):
        a = (toward + off) % (2 * math.pi)
        k = int(a / (2 * math.pi) * count) % count
        slots |= {k, (k + 1) % count} if count > 16 else {k}
    return slots


def tyre_store(c):
    """North-west loop: tyre walls all round, a garage in the middle, tyre stacks."""
    island(c)
    ring(c, 330.0, 13, entrances(c, 13), lambda x, y, yaw: prop("tyre_wall", x, y, ISLE, yaw, clip=(78, 14, 24)))
    # garage: open toward the bridge
    face_a = math.degrees(math.atan2(-c[1], -c[0]))
    a = math.radians(face_a)
    u, v = (math.cos(a), math.sin(a)), (-math.sin(a), math.cos(a))
    depth, width, height, wall = 200.0, 180.0, 80.0, 16.0

    def P(s, t):
        return A((c[0] + u[0] * s + v[0] * t, c[1] + u[1] * s + v[1] * t))
    s0, s1 = -depth / 2, depth / 2
    SHAPES.append(([(c[0] + u[0] * s + v[0] * t, c[1] + u[1] * s + v[1] * t) for s, t in
                    ((s0, -width / 2), (s1, -width / 2), (s1, width / 2), (s0, width / 2))], (150, 150, 146)))
    prism([P(s0, -width / 2), P(s0 + wall, -width / 2), P(s0 + wall, width / 2), P(s0, width / 2)], ISLE, ISLE + height, MAT["concrete"])
    for t0 in (-width / 2, width / 2 - wall):
        prism([P(s0 + wall, t0), P(s1, t0), P(s1, t0 + wall), P(s0 + wall, t0 + wall)], ISLE, ISLE + height, MAT["concrete"])
    prism([P(s0, -width / 2 - 8), P(s1 + 8, -width / 2 - 8), P(s1 + 8, width / 2 + 8), P(s0, width / 2 + 8)],
          ISLE + height, ISLE + height + 16, MAT["concrete"], top=MAT["concrete2"], bot=MAT["concrete2"])
    # light strip under the roof at the mouth
    prism([P(s1 - 24, -width / 2 + wall), P(s1 - 16, -width / 2 + wall), P(s1 - 16, width / 2 - wall), P(s1 - 24, width / 2 - wall)],
          ISLE + height - 4, ISLE + height, MAT["concrete"], bot=MAT["lamp"])
    g = (c[0] - u[0] * 20, c[1] - u[1] * 20)
    entity("light", g[0], g[1], ISLE + height - 24, light="320", _color="1 0.9 0.75")
    entity("item_health_large", g[0], g[1], ISLE + 24)
    # tyre stacks between the walls and the garage
    for k, (r, a_off) in enumerate(((200, 1.2), (230, 2.0), (210, 3.3), (240, 4.3), (190, 5.3))):
        aa = a + a_off
        prop("tyre_stack5" if k % 2 else "tyre_stack3", c[0] + r * math.cos(aa), c[1] + r * math.sin(aa), ISLE, k * 37.0,
             clip=(14, 14, 30 if k % 2 else 18))
    m = (c[0] + u[0] * 170, c[1] + u[1] * 170)
    entity("rally_weapon_mine", m[0], m[1], ISLE + 24)
    for side in (-1, 1):
        q = (m[0] + v[0] * side * 60, m[1] + v[1] * side * 60)
        entity("ammo_mines", q[0], q[1], ISLE + 24)


def scrap_yard(c):
    """South-east loop: concrete barriers all round, wrecks and containers."""
    island(c)
    ring(c, 340.0, 28, entrances(c, 28), lambda x, y, yaw: prop("barrier", x, y, ISLE, yaw, clip=(36, 12, 26)))
    toward = math.atan2(-c[1], -c[0])
    u = (math.cos(toward), math.sin(toward))
    v = (-u[1], u[0])

    def L(s, t):
        return (c[0] + u[0] * s + v[0] * t, c[1] + u[1] * s + v[1] * t)
    base = math.degrees(toward)
    # containers: an L behind the wrecks, one on top
    p = L(-170, -40)
    container(p[0], p[1], ISLE, base + 90, 20, "green")
    container(p[0], p[1], ISLE + 62, base + 84, 20, "red")
    p = L(-90, 150)
    container(p[0], p[1], ISLE, base, 20, "orange")
    # wrecks
    veh = "models/mapobjects/vehicles"
    p = L(40, -120)
    rust = "textures/q3r_metals/rustmet01"     # the damaged cars' own paint reads as pink in this light: rusted through instead
    prop("wreck1", p[0], p[1], ISLE, base + 30, 1.25, clip=(100, 38, 60), model=f"{veh}/flatbed_damaged01.md3", remap=rust)
    p = L(-40, 60)
    prop("wreck2", p[0], p[1], ISLE, base + 160, 1.2, clip=(64, 32, 42), model=f"{veh}/pickup_damaged02.md3", remap=rust)
    prop("wreck3", p[0], p[1], ISLE + 42, 0, 1.2, angles=f"0 {num(base + 200)} 14", model=f"{veh}/pickup_damaged01.md3",
         clip=None, remap=rust)
    PROPS.append(("wreck3", p[0], p[1], base + 200, (64, 32, 84)))
    p = L(130, 120)
    prop("wreck4", p[0], p[1], ISLE, base + 250, 1.25, clip=(100, 38, 60), model=f"{veh}/flatbed_damaged02.md3", remap=rust)
    q = L(50, 0)
    entity("rally_weapon_smoke", q[0], q[1], ISLE + 24)
    q = L(-60, -230)
    entity("item_health_large", q[0], q[1], ISLE + 24)
    for q in (L(200, -150), L(-210, 110)):
        entity("item_health", q[0], q[1], ISLE + 24)


def paddock():
    """North-east corner: a concrete pad with trucks, containers and a fuel station."""
    lo, hi = PAD
    prism([A((lo, lo)), A((hi, lo)), A((hi, hi)), A((lo, hi))], 0.0, ZONE, MAT["pad"], bot=MAT["caulk"])
    # guard rails along the two edges toward the track, open at the ends
    for k in range(6):
        prop("armco", 740 + k * 96, lo + 6, ZONE, 0, clip=(48, 4, 21))
        prop("armco", lo + 6, 740 + k * 96, ZONE, 270, clip=(48, 4, 21))
    # containers
    container(900, 1340, ZONE, 0, 40, "blue")
    container(1340, 900, ZONE, 90, 40, "red")
    container(1270, 1270, ZONE, 45, 20, "green")
    container(1270, 1270, ZONE + 62, 52, 20, "orange")
    container(720, 880, ZONE, 30, 20, "orange")
    container(980, 640, ZONE, 75, 20, "blue")
    # trucks (the stock flatbed truck, scaled to size)
    truck = "models/mapobjects/vehicles/truck_flatbed.md3"
    prop("truck1", 560, 1000, ZONE + 21, 0, 2.3, clip=(124, 38, 80), model=truck, clip_z0=ZONE)
    prop("truck2", 1080, 420, ZONE + 21, 90, 2.3, clip=(124, 38, 80), model=truck, clip_z0=ZONE)
    # fuel station: canopy on four posts, two pumps
    fx, fy = 560.0, 1250.0
    for sx in (-1, 1):
        for sy in (-1, 1):
            px, py = fx + sx * 120, fy + sy * 60
            prism([A((px - 5, py - 5)), A((px + 5, py - 5)), A((px + 5, py + 5)), A((px - 5, py + 5))], ZONE, 118, MAT["steel"])
    SHAPES.append(([(fx - 140, fy - 80), (fx + 140, fy - 80), (fx + 140, fy + 80), (fx - 140, fy + 80)], (120, 70, 50)))
    prism([A((fx - 140, fy - 80)), A((fx + 140, fy - 80)), A((fx + 140, fy + 80)), A((fx - 140, fy + 80))], 118, 130,
          MAT["hazard"], top=MAT["canopy"], bot=MAT["steel"])
    prism([A((fx - 100, fy - 10)), A((fx + 100, fy - 10)), A((fx + 100, fy + 10)), A((fx - 100, fy + 10))], 114, 118,
          MAT["steel"], bot=MAT["lamp"])
    entity("light", fx, fy, 96, light="420", _color="1 0.95 0.85")
    for px in (fx - 70, fx + 70):
        prop("fuelpump", px, fy, ZONE, 90, clip=(16, 11, 46))
        entity("item_fuelcan", px, fy - 44, ZONE + 24)
    # rocket launcher in the corner of the containers
    entity("weapon_rocketlauncher", 1080, 1080, ZONE + 24)
    entity("ammo_rockets", 1160, 1040, ZONE + 24)
    entity("ammo_rockets", 1040, 1160, ZONE + 24)
    entity("item_armor_green", 820, 1160, ZONE + 24)
    # barrels and cones
    for k, (bx, by) in enumerate(((820, 1300), (846, 1288), (832, 1262), (1300, 760), (1288, 732), (380, 1080))):
        prop("barrel", bx, by, ZONE + 21, k * 50, 1.6, clip=(14, 14, 42), model="models/mapobjects/barrels/b_default.md3",
             clip_z0=ZONE)
    for k in range(6):
        prop("cone", 380 + k * 40, 340, ZONE + 10, 0, 1.4, model="models/mapobjects/cones/cone01.md3")


def mud_pit():
    """South-west corner: mud, a pool, two kickers facing each other across it."""
    lo, hi = -PAD[1], -PAD[0]
    pc = (-870.0, -870.0)
    pr = 220.0
    # the pool is a diamond in the middle of the square; the mud round it in four pentagons
    SHAPES.append(([(pc[0] + pr, pc[1]), (pc[0], pc[1] + pr), (pc[0] - pr, pc[1]), (pc[0], pc[1] - pr)], (60, 44, 30)))
    prism([A((pc[0] + pr, pc[1])), A((pc[0], pc[1] + pr)), A((pc[0] - pr, pc[1])), A((pc[0], pc[1] - pr))], 0.0, ZONE,
          MAT["pool"], bot=MAT["caulk"])
    for sx, sy in ((1, 1), (-1, 1), (-1, -1), (1, -1)):
        ex, ey = (hi if sx > 0 else lo), (hi if sy > 0 else lo)
        prism([A((pc[0] + sx * pr, pc[1])), A((ex, pc[1])), A((ex, ey)), A((pc[0], ey)), A((pc[0], pc[1] + sy * pr))],
              0.0, ZONE, MAT["mud"], bot=MAT["caulk"])
    # kickers: 150 long, 120 wide, 44 high, rising toward the pool
    for start, aim in (((-1170.0, -570.0), (0.7071, -0.7071)), ((-570.0, -1170.0), (-0.7071, 0.7071))):
        u = aim
        v = (-u[1], u[0])
        foot = start
        L, Wd, Ht = 150.0, 60.0, 44.0
        slope = Ht / L
        o = A(foot)
        top_end = A((foot[0] + u[0] * L, foot[1] + u[1] * L))
        planes = [
            ((-slope * u[0], -slope * u[1], 1.0), (o[0], o[1], ZONE), MAT["kicker"]),
            ((0, 0, -1), (o[0], o[1], 0.0), MAT["caulk"]),
            ((v[0], v[1], 0), (o[0] + v[0] * Wd, o[1] + v[1] * Wd, 0), MAT["hazard"]),
            ((-v[0], -v[1], 0), (o[0] - v[0] * Wd, o[1] - v[1] * Wd, 0), MAT["hazard"]),
            ((u[0], u[1], 0), (top_end[0], top_end[1], 0), MAT["hazard"]),
            ((-u[0], -u[1], 0), (o[0], o[1], 0), MAT["hazard"]),
        ]
        convex(planes)
        SHAPES.append(([(foot[0] + v[0] * Wd, foot[1] + v[1] * Wd), (foot[0] - v[0] * Wd, foot[1] - v[1] * Wd),
                        (foot[0] + u[0] * L - v[0] * Wd, foot[1] + u[1] * L - v[1] * Wd),
                        (foot[0] + u[0] * L + v[0] * Wd, foot[1] + u[1] * L + v[1] * Wd)], (200, 140, 40)))
        p = (foot[0] + u[0] * (L - 20), foot[1] + u[1] * (L - 20))
        entity("ammo_flame", p[0], p[1], ZONE + Ht + 20)
    entity("weapon_flamethrower", pc[0], pc[1], ZONE + 24)
    # tyre stacks round the pool, out of the jump line
    for k in range(10):
        a = 2 * math.pi * (k + 0.5) / 10 + math.pi / 4
        if min(abs(math.remainder(a - ang, 2 * math.pi)) for ang in (math.radians(135), math.radians(-45))) < 0.45:
            continue
        prop("tyre_stack3" if k % 2 else "tyre_stack5", pc[0] + 250 * math.cos(a), pc[1] + 250 * math.sin(a), ZONE, k * 41,
             clip=(14, 14, 30 if not k % 2 else 18))
    entity("weapon_shotgun", -1250, -1080, ZONE + 24)
    entity("ammo_shells", -1200, -1150, ZONE + 24)
    entity("ammo_shells", -1150, -1200, ZONE + 24)
    entity("item_health_large", -520, -1290, ZONE + 24)
    # barriers along the outer edges of the pit
    for k in range(5):
        prop("barrier", -1380, -1000 + k * 100, ZONE, 90, clip=(36, 12, 26))
        prop("barrier", -1000 + k * 100, -1380, ZONE, 0, clip=(36, 12, 26))


def floodlights():
    """A lattice mast in each corner of the field, the lamps toward the middle."""
    for sx, sy in ((1, 1), (-1, 1), (-1, -1), (1, -1)):
        x, y = sx * 1500.0, sy * 1500.0
        yaw = math.degrees(math.atan2(-y, -x))
        prop("floodmast", x, y, GRASS, yaw, clip=(22, 22, 470))
        a = math.radians(yaw)
        head = (x + 12 * math.cos(a), y + 12 * math.sin(a))
        for k, (tx, ty) in enumerate(((x * -0.2, y * -0.2), (x * -0.55, y * 0.25), (x * 0.25, y * -0.55))):
            name = f"flood_{'p' if sx > 0 else 'm'}{'p' if sy > 0 else 'm'}_{k}"
            entity("info_null", tx, ty, 0, targetname=name)
            entity("light", head[0], head[1], 470, light="3000", target=name, radius="1100", _color="1 0.95 0.86")
    # the even part of the light: a grid under the rig
    for gx in range(4):
        for gy in range(4):
            x, y = -1176.0 + gx * 784.0, -1176.0 + gy * 784.0
            entity("light", x, y, 360, light="480", _color="1 0.96 0.9")


def players():
    """12 spawns: outside the loops, two in each corner zone, one on each island; along the track."""
    out = []
    for c, angs in ((C1, (98, 140, 182)), (C2, (-82, -40, 2))):
        for a in angs:
            r = 800.0
            x, y = c[0] + r * math.cos(math.radians(a)), c[1] + r * math.sin(math.radians(a))
            out.append((x, y, GRASS, a + 90))
    out += [(500, 640, ZONE, 45), (1420, 560, ZONE, 135), (-500, -640, ZONE, 225), (-1300, -420, ZONE, 315)]
    for c in (C1, C2):
        toward = math.atan2(-c[1], -c[0])
        out.append((c[0] + 240 * math.cos(toward), c[1] + 240 * math.sin(toward), ISLE, math.degrees(toward) + 90))
    for x, y, z, yaw in out:
        entity("info_player_deathmatch", x, y, z + 32, angle=num(round(yaw % 360)))


def track_items():
    # the bridge: railgun in the middle, slugs on it
    entity("weapon_railgun", 0, 0, DECK + 24)
    for s in (-140.0, 140.0):
        p = on_line(UA, VA, s, 0)
        entity("ammo_slugs", p[0], p[1], DECK + 24)
    entity("item_health_mega", 0, 0, TOP + 24)
    # turbo before each ramp
    for s in (660.0, -660.0):
        p = on_line(UA, VA, s, 0)
        entity("rally_item_turbo", p[0], p[1], TOP + 24)
    # along the loops
    for c, a_rock, a_flame, a_health in ((C1, 135, 205, 70), (C2, -45, 25, -110)):
        for a, cls in ((a_rock, "ammo_rockets"), (a_flame, "rally_weapon_flame"), (a_health, "item_health")):
            x, y = c[0] + R * math.cos(math.radians(a)), c[1] + R * math.sin(math.radians(a))
            entity(cls, x, y, TOP + 24)


# ---------------------------------------------------------------- the stadium from the original

JUMBO_UP = 80.0
RIG_DIM = 0.5


def shift_z(text, dz, clamp=None):
    """Move a brush up by dz; with clamp, points above clamp - dz stay where they are (a hanger that
    keeps its top and gets shorter)."""
    import re

    def point(m):
        x, y, z = (float(v) for v in m.group(1).split())
        if clamp is None or z + dz <= clamp:
            z += dz
        return f"( {num(x)} {num(y)} {num(z)} )"
    return re.sub(r"\( ([^()]*) \)", point, text)

def shell():
    from mapparse import Brush, load
    ents = load(str(SRC))
    kv, brushes = ents[0]
    kept = []
    lo, hi = OX - HALF, OY + HALF   # -1600 .. 1536
    for text in brushes:
        b = Brush(text)
        if b.mins is None:
            continue
        inside = (b.mins[0] > lo + 0.5 and b.maxs[0] < hi - 0.5 and b.mins[1] > lo + 0.5 and b.maxs[1] < hi - 0.5)
        if inside and b.maxs[2] <= 170:
            continue            # the old field: painted eight, ramps, the platform
        if b.maxs[2] <= 12 and any("dirt" in t for t in b.textures) and b.mins[0] > lo - 40 and b.maxs[0] < hi + 40 \
                and b.mins[1] > lo - 40 and b.maxs[1] < hi + 40:
            continue            # the painted eight where it touches the boards
        cx, cy = (b.mins[0] + b.maxs[0]) / 2, (b.mins[1] + b.maxs[1]) / 2
        if abs(cx) < 90 and abs(cy + 32) < 90 and b.mins[2] >= 190:
            if b.maxs[2] <= 272.5:
                # the screen box over the middle: up by JUMBO_UP, clear of cars on the bridge
                kept.append(shift_z(text.strip(), JUMBO_UP))
            elif b.mins[2] >= 271.5 and b.maxs[2] <= 384.5 and b.maxs[0] - b.mins[0] < 20:
                kept.append(shift_z(text.strip(), JUMBO_UP, clamp=384.0))   # its hangers, shorter
            else:
                kept.append(text.strip())
            continue
        kept.append(text.strip())
    others = []
    for kv2, brs in ents[1:]:
        cls = kv2.get("classname", "")
        z = float(kv2.get("origin", "0 0 0").split()[2]) if "origin" in kv2 else 0
        if cls == "light" and z >= 180:
            x, y = map(float, kv2["origin"].split()[:2])
            if abs(x) < 90 and abs(y + 32) < 90 and z < 200:
                kv2 = dict(kv2, origin=f"{num(x)} {num(y)} {num(z + JUMBO_UP)}")
            elif z > 300:
                # the rig lights round the edge: half as bright, the masts and the grid light the field now
                kv2 = dict(kv2, light=num(float(kv2.get("light", "300")) * RIG_DIM))
            others.append(kv2)
        elif cls == "misc_model" and z >= 300:
            others.append(kv2)
    # the grass over the field but for the two corner zones (convex pieces round them)
    a, b = PAD
    for x0, y0, x1, y1 in ((-HALF, -HALF, HALF, -b), (-HALF, b, HALF, HALF), (-HALF, -b, -b, b), (b, -b, HALF, b),
                           (-a, -b, b, -a), (-b, -a, b, a), (-b, a, a, b)):
        prism([A((x0, y0)), A((x1, y0)), A((x1, y1)), A((x0, y1))], 0.0, GRASS, MAT["grass"], bot=MAT["caulk"], detail=False)
    return kv, kept, others


# ---------------------------------------------------------------- checks and output

def clear(x, y, z, r=40, h=20, solids=None):
    for planes in (SOLIDS if solids is None else solids):
        hit = True
        for n, d in planes:
            ln = math.sqrt(dot(n, n))
            reach = r * (abs(n[0]) + abs(n[1])) / ln + h * abs(n[2]) / ln
            if dot(n, (x, y, z)) / ln - reach > d / ln:
                hit = False
                break
        if hit:
            return False
    return True


def validate(arcs):
    floor = SOLIDS[:]
    starts = [e for e in ENTITIES if e["classname"] == "info_player_deathmatch"]
    pickups = [e for e in ENTITIES if e["classname"].startswith(("item_", "ammo_", "weapon_", "rally_"))]
    bad = []
    for e in starts:
        x, y, z = map(float, e["origin"].split())
        if not clear(x, y, z + 8, r=56, h=20):
            bad.append(e)
    for e in pickups:
        x, y, z = map(float, e["origin"].split())
        if not clear(x, y, z, r=20, h=14):
            bad.append(e)
    assert not bad, bad
    # drive the centre line of the whole eight at car height: nothing in the way (the bridge deck is above
    # the underpass: checked at the deck top on line A)
    def free(p, z):
        return clear(p[0] + OX, p[1] + OY, z, r=34, h=16)
    path_ok = True
    for c, (a0, a1) in zip((C1, C2), arcs):
        for i in range(0, 61):
            a = a0 + (a1 - a0) * i / 60
            path_ok &= free((c[0] + R * math.cos(a), c[1] + R * math.sin(a)), TOP + 20)
    for s in np.linspace(-LEN, LEN, 60):
        path_ok &= free(on_line(UB, VB, s, 0), TOP + 20)
        # (the box test is conservative at the slope: keep 40 off the ramp ends)
        if abs(s) >= RAMP_S + 40:
            path_ok &= free(on_line(UA, VA, s, 0), TOP + 20)
        elif abs(s) <= DECK_S - 40:
            path_ok &= free(on_line(UA, VA, s, 0), DECK + 20)
    clearance = DECK - DECK_T - TOP
    assert path_ok
    return {
        "map": NAME, "field": [2 * HALF, 2 * HALF], "track_width": 2 * H, "kerb": K, "loop_radius": R,
        "loop_centres": [C1, C2], "straight_angles": [round(ANG_A, 1), round(ANG_B, 1)],
        "bridge": {"deck_top": DECK, "deck_length": 2 * DECK_S, "underpass_clearance": clearance,
                   "ramp_length": RAMP_S - DECK_S,
                   "ramp_degrees": round(math.degrees(math.atan((DECK - TOP) / (RAMP_S - DECK_S))), 1)},
        "spawns": len(starts), "path_clear": path_ok, "brushes_new": len(BRUSHES), "props": len(PROPS),
        "pickups": {c: sum(1 for e in ENTITIES if e["classname"] == c) for c in sorted({e["classname"] for e in pickups})},
    }


def write_map(kv, kept, others):
    fields = dict(kv)
    fields.update({"message": "Figure of 8", "_ambient": "6", "_minlight": "5", "haze": "0.20 0.10 0.07 9000",
                   "autoExposureMinMax": "-2 1"})
    text = ["// Generated by tools/maptools/make_figure8.py (the stadium from tools/maptools/src/q3r_dd_02_orig.map).", "{"]
    text += [f'"{k}" "{v}"' for k, v in fields.items()]
    text += kept
    text += BRUSHES
    text.append("}")
    for ent in others + ENTITIES:
        text.append("{\n" + "\n".join(f'"{k}" "{v}"' for k, v in ent.items()) + "\n}")
    target = ROOT / "baseq3r" / "maps" / f"{NAME}.map"
    target.write_text("\n".join(text) + "\n", encoding="latin1")
    arena = ROOT / "baseq3r" / "scripts" / f"{NAME}.arena"
    arena.write_text("\n".join([
        "{", f'map "{NAME}"', 'longname "Figure of 8"', "fraglimit 20", 'type "q3r_dm q3r_team_dm q3r_derby q3r_lcs"',
        'bots "Jim Bobby Sam Bee"', 'starts "12"', 'observerspots "0"', 'weapons "SG,RL,RG,FT"', 'powerups "0"', 'author "P3rlE, rework Q3Rally Team"',
        "}", ""]), encoding="ascii")
    return target


def overview(report, arcs):
    from PIL import Image, ImageDraw
    s = 0.36
    E = 1800.0
    W = int(2 * E * s)
    im = Image.new("RGB", (W, W), (20, 20, 24))
    d = ImageDraw.Draw(im, "RGBA")

    def P(x, y):
        return ((x + E) * s, (E - y) * s)
    d.rectangle([P(-E + 40, E - 40), P(E - 40, -E + 40)], fill=(70, 70, 76))
    d.rectangle([P(-HALF, HALF), P(HALF, -HALF)], fill=(56, 70, 44))
    d.rectangle([P(300, 1440), P(1440, 300)], fill=(84, 84, 92))
    d.rectangle([P(-PAD[1], -PAD[0]), P(-PAD[0], -PAD[1])], fill=(96, 76, 50))

    def band(points, w, col):
        r = w * s / 2
        for x, y in points:
            u, v = P(x, y)
            d.ellipse([u - r, v - r, u + r, v + r], fill=col)
    path = []
    for c, (a0, a1) in zip((C1, C2), arcs):
        path += [(c[0] + R * math.cos(a0 + (a1 - a0) * i / 200), c[1] + R * math.sin(a0 + (a1 - a0) * i / 200)) for i in range(201)]
    for u in (UA, UB):
        path += [on_line(u, (0, 0), t, 0) for t in np.linspace(-LEN, LEN, 120)]
    band(path, 2 * (H + K), (230, 60, 50))
    band(path, 2 * H, (86, 86, 92))
    for c in (C1, C2):
        d.ellipse([P(c[0] - 380, c[1] + 380), P(c[0] + 380, c[1] - 380)], fill=(80, 64, 44))
    deck = [P(*on_line(UA, VA, a, b)) for a, b in ((-DECK_S, -H - 12), (DECK_S, -H - 12), (DECK_S, H + 12), (-DECK_S, H + 12))]
    d.polygon(deck, fill=(180, 160, 120))
    for sign in (1, -1):
        rp = [P(*on_line(UA, VA, sign * a, b)) for a, b in ((DECK_S, -H), (RAMP_S, -H), (RAMP_S, H), (DECK_S, H))]
        d.polygon(rp, fill=(130, 120, 106))
    for poly, col in SHAPES:
        d.polygon([P(*q) for q in poly], fill=col)
    for name, x, y, yaw, clip in PROPS:
        if not clip:
            continue
        hx, hy, _ = clip
        a = math.radians(yaw)
        pts = [P(x + p[0], y + p[1]) for p in (rot2((-hx, -hy), a), rot2((hx, -hy), a), rot2((hx, hy), a), rot2((-hx, hy), a))]
        col = (200, 90, 50) if name.startswith("container") else (40, 40, 40) if name.startswith("tyre") else (170, 170, 170)
        d.polygon(pts, fill=col)
    colors = {"info_player_deathmatch": (90, 220, 120)}
    for e in ENTITIES:
        cls = e["classname"]
        x, y, _ = map(float, e["origin"].split())
        u, v = P(x - OX, y - OY)
        if cls == "info_player_deathmatch":
            a = math.radians(float(e["angle"]))
            d.ellipse([u - 6, v - 6, u + 6, v + 6], fill=colors[cls])
            d.line([u, v, u + 20 * math.cos(a), v - 20 * math.sin(a)], fill=colors[cls], width=3)
        elif cls.startswith(("weapon_", "rally_weapon")):
            d.rectangle([u - 6, v - 6, u + 6, v + 6], fill=(255, 70, 70))
        elif cls.startswith("item_"):
            d.ellipse([u - 5, v - 5, u + 5, v + 5], fill=(90, 170, 255))
        elif cls.startswith(("ammo_", "rally_item")):
            d.rectangle([u - 3, v - 3, u + 3, v + 3], fill=(255, 210, 60))
    out = ROOT / "docs" / "maps" / "figure8"
    out.mkdir(parents=True, exist_ok=True)
    im.save(out / "figure8_layout.png")
    (out / "validation.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


def main():
    kv, kept, others = shell()
    arcs = eight()
    bridge()
    tyre_store(C1)
    scrap_yard(C2)
    paddock()
    mud_pit()
    floodlights()
    players()
    track_items()
    report = validate(arcs)
    report["brushes_kept"] = len(kept)
    target = write_map(kv, kept, others)
    overview(report, arcs)
    print(json.dumps(report, indent=2))
    print("wrote", target)


if __name__ == "__main__":
    main()
