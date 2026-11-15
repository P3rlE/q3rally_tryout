#!/usr/bin/env python3
"""Rework q3r_ctf_04 "Trophy Square": two-team CTF on a city square at night.

The source is the hand-made map tools/maptools/src/q3r_ctf_04_orig.map (red
base west, blue base east, the trophy fountain in the middle, a car workshop
in the north-east block and an apartment house in the north-west block, the
south half empty). This script keeps all of it and adds:

- Fairness: the map becomes point symmetric about the trophy (x, y -> -x, -y).
  The workshop (BFG, tail weapons, health) is copied into the south-west
  block, the apartment house into the south-east block. Textures are carried
  over exactly (scale and rotation adjusted), so signs still read correctly.
- A city around the square: the world is 1152 units larger on every side.
  Behind a concrete barrier at the old edge (invisible, solid wall up to the
  sky) runs a ring road with sidewalks and street lamps, then a ring of
  buildings with facade textures.
- Street lamps along the inner roads, lantern posts on the square, parked
  cars, trees and low walls as cover, a parking lot and a small park in the
  outer corners, a few extra pickups; all of it point symmetric.
- Own team colours on the sides of the flag pads (were missing textures),
  solid collision for model props (common/full_clip is not solid for the
  physics; q3r_ctf_04/propclip is).

Writes baseq3r/maps/q3r_ctf_04.map and scripts/q3r_ctf_04.arena.
Run with Python 3 + Pillow. Compile: see docs/maps/ctf_04/README.md.
"""
from __future__ import annotations

import math
import random
import re
from itertools import combinations
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "tools/maptools/src/q3r_ctf_04_orig.map"
OUT = ROOT / "baseq3r/maps/q3r_ctf_04.map"
ARENA = ROOT / "baseq3r/scripts/q3r_ctf_04.arena"
TEX = "q3r_ctf_04/"
DETAIL = 134217728

EX, EY = 4224, 2176          # play area (the old world box)
BAND = 1152                  # city ring around it
BX, BY = EX + BAND, EY + BAND
ZG = -56                     # ground
SKY = 1536                   # sky ceiling
STORY = 144                  # storey height of the backdrop buildings

# ring around the play area: (offset from the edge, ground material)
RING = ((128, "q3r_road/cobbles01"), (448, "road"), (544, "q3r_road/cobbles01"), (BAND, "q3r_road/cobbles01"))
ROAD_IN, ROAD_OUT = 128, 448
ROADS_X = (-1792, 0, 1790)   # centres of the north-south roads (they end at the north and south edge)
ROAD_HALF = 128

# facade textures: (name, storeys in one tile)
FACADES = (("q3r_building/externalwall50a", 4), ("q3r_building/externalwall51a", 5),
           ("q3r_building/externalwall52a", 3), ("q3r_building/externalwall56a", 2),
           ("q3r_building/externalwall68a_", 5), ("q3r_building/externalwall72_", 3),
           ("q3r_building/externalwall75_256x256", 5), ("q3r_building/externalwall33-1a", 3),
           ("q3r_building/externalwall33-2a", 3), ("q3r_building/externalwall46b_a", 3),
           ("q3r_building/externalwall91_512x512", 2))

WORLD: list = []             # new world brushes (face lists)
ENTS: list = []              # new entities (key/value lists)
OBST: list = []              # (xmin, ymin, xmax, ymax) taken ground in the play area


# ---------------------------------------------------------------- .map reading and writing

FACE = re.compile(r"^\(\s*(\S+) (\S+) (\S+) \) \(\s*(\S+) (\S+) (\S+) \) \(\s*(\S+) (\S+) (\S+) \) (\S+) (.*)$")


def parse(path):
    text = path.read_text(errors="ignore").replace("\r\n", "\n")
    ents = []
    for part in re.split(r"(?m)^// entity \d+\n", text):
        if not part.strip():
            continue
        body = part.strip()
        assert body.startswith("{") and body.endswith("}"), body[:80]
        inner = body[1:-1]
        kv = re.findall(r'(?m)^"([^"]+)" "([^"]*)"$', inner)
        brushes = []
        for b in re.split(r"(?m)^// brush \d+\n", inner)[1:]:
            faces = []
            for line in b.strip().splitlines():
                m = FACE.match(line.strip())
                if m:
                    g = m.groups()
                    pts = [tuple(map(float, g[i:i + 3])) for i in (0, 3, 6)]
                    faces.append((pts, g[9], g[10].split()))
            brushes.append(faces)
        ents.append({"kv": [list(p) for p in kv], "brushes": brushes})
    return ents


def num(v):
    s = f"{v:.4f}".rstrip("0").rstrip(".")
    return s if s not in ("", "-0") else "0"


def write(ents, path):
    out = []
    for i, e in enumerate(ents):
        out.append(f"// entity {i}\n{{\n")
        out += [f'"{k}" "{v}"\n' for k, v in e["kv"]]
        for j, b in enumerate(e["brushes"]):
            out.append(f"// brush {j}\n{{\n")
            for pts, tex, rest in b:
                out.append(" ".join("( " + " ".join(num(c) for c in p) + " )" for p in pts) + f" {tex} {' '.join(rest)}\n")
            out.append("}\n")
        out.append("}\n")
    path.write_text("".join(out), newline="\n")


def get(e, key, default=None):
    for k, v in e["kv"]:
        if k == key:
            return v
    return default


def put(e, key, value):
    for p in e["kv"]:
        if p[0] == key:
            p[1] = value
            return
    e["kv"].append([key, value])


# ---------------------------------------------------------------- geometry

def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def normal(pts):
    """Plane normal as q3map2 computes it from the three points."""
    a, b, c = pts
    n = cross(sub(c, a), sub(b, a))
    ln = math.sqrt(dot(n, n))
    return tuple(x / ln for x in n)


def plane(n, point):
    up = (1, 0, 0) if abs(n[0]) < 1e-9 and abs(n[1]) < 1e-9 else (0, 0, 1)
    t1 = cross(up, n)
    t2 = cross(n, t1)
    size = max(abs(c) for c in t1 + t2)
    return [point, tuple(p + c * 128 / size for p, c in zip(point, t2)),
            tuple(p + c * 128 / size for p, c in zip(point, t1))]


def corners(faces):
    eq = [(normal(p), dot(normal(p), p[0])) for p, _, _ in faces]
    out = []
    for (n1, d1), (n2, d2), (n3, d3) in combinations(eq, 3):
        c23, c31, c12 = cross(n2, n3), cross(n3, n1), cross(n1, n2)
        det = dot(n1, c23)
        if abs(det) < 1e-9:
            continue
        p = tuple((d1 * c23[m] + d2 * c31[m] + d3 * c12[m]) / det for m in range(3))
        if all(dot(n, p) <= d + 0.01 for n, d in eq):
            out.append(p)
    return out


def bbox(faces):
    c = corners(faces)
    return tuple(min(p[i] for p in c) for i in range(3)), tuple(max(p[i] for p in c) for i in range(3))


# ---------------------------------------------------------------- texture projection (q3map2, old brush format)

BASEAXIS = (((0, 0, 1), (1, 0, 0), (0, -1, 0)), ((0, 0, -1), (1, 0, 0), (0, -1, 0)),
            ((1, 0, 0), (0, 1, 0), (0, 0, -1)), ((-1, 0, 0), (0, 1, 0), (0, 0, -1)),
            ((0, 1, 0), (1, 0, 0), (0, 0, -1)), ((0, -1, 0), (1, 0, 0), (0, 0, -1)))


def axis_index(n):
    best, idx = 0.0, 0
    for i, (bn, _, _) in enumerate(BASEAXIS):
        d = dot(n, bn)
        if d > best + 0.0001:
            best, idx = d, i
    return idx


def texvecs(n, rot, ss, ts):
    _, u, v = BASEAXIS[axis_index(n)]
    u, v = list(u), list(v)
    sv = 0 if u[0] else (1 if u[1] else 2)
    tv = 0 if v[0] else (1 if v[1] else 2)
    r = rot % 360
    if r == 0:
        sn, cs = 0.0, 1.0
    elif r == 90:
        sn, cs = 1.0, 0.0
    elif r == 180:
        sn, cs = 0.0, -1.0
    elif r == 270:
        sn, cs = -1.0, 0.0
    else:
        sn, cs = math.sin(math.radians(rot)), math.cos(math.radians(rot))
    for vec in (u, v):
        a, b = vec[sv], vec[tv]
        vec[sv], vec[tv] = cs * a - sn * b, sn * a + cs * b
    return [c / ss for c in u], [c / ts for c in v]


def st(face, p):
    pts, _, rest = face
    xo, yo, rot, xs, ys = map(float, rest[:5])
    u, v = texvecs(normal(pts), rot, xs or 1, ys or 1)
    return dot(p, u) + xo, dot(p, v) + yo


_size = {}


def tex_size(name):
    if name not in _size:
        for ext in (".jpg", ".tga", ".png"):
            p = ROOT / "baseq3r/textures" / (name + ext)
            if p.exists():
                _size[name] = Image.open(p).size
                break
        else:
            _size[name] = (256, 256)
    return _size[name]


def tface(n, point, spec, detail):
    """One face. spec: material name (scale 0.5) or dict(m=, s=(ss, ts), rot=, at=(world point, (s, t)))."""
    pts = plane(n, point)
    if isinstance(spec, str):
        spec = {"m": spec}
    m = spec["m"]
    ss, ts = spec.get("s", (0.5, 0.5))
    rot = spec.get("rot", 0)
    xo = yo = 0.0
    if "at" in spec:
        a, (sa, ta) = spec["at"]
        u, v = texvecs(n, rot, ss, ts)
        w, h = tex_size(m)
        xo, yo = (sa - dot(a, u)) % w, (ta - dot(a, v)) % h
    return (pts, m, [num(xo), num(yo), num(rot), num(ss), num(ts), str(DETAIL if detail else 0), "0", "0"])


SIDES = {"+x": (1, 0, 0), "-x": (-1, 0, 0), "+y": (0, 1, 0), "-y": (0, -1, 0), "top": (0, 0, 1), "bot": (0, 0, -1)}


def box(lo, hi, mat, faces=None, detail=True, obstacle=False):
    """Axial box. faces: {side: spec} overrides mat; a callable spec gets (side, lo, hi)."""
    assert all(h > l for l, h in zip(lo, hi)), (lo, hi)
    out = []
    for key, n in SIDES.items():
        p = tuple(hi[i] if n[i] > 0 else lo[i] for i in range(3))
        spec = (faces or {}).get(key, mat)
        if callable(spec):
            spec = spec(key, lo, hi)
        out.append(tface(n, p, spec, detail))
    WORLD.append(out)
    if obstacle:
        OBST.append((lo[0], lo[1], hi[0], hi[1]))
    return out


def entity(classname, x, y, z, **keys):
    kv = [["classname", classname], ["origin", f"{num(round(x, 1))} {num(round(y, 1))} {num(round(z, 1))}"]]
    kv += [[k, str(v)] for k, v in keys.items()]
    ENTS.append({"kv": kv, "brushes": []})


def both(fn, x, y, yaw, *args, **kw):
    """Place an object and its copy turned by 180 degrees about the trophy."""
    fn(x, y, yaw, *args, **kw)
    fn(-x, -y, (yaw + 180) % 360, *args, **kw)


# ---------------------------------------------------------------- point mirror of existing content

def neg(text):
    """Negate a number given as text, keeping all its digits."""
    if float(text) == 0:
        return text
    return text[1:] if text.startswith("-") else "-" + text


def mirror_face(face):
    pts, tex, rest = face
    n = normal(pts)
    xo, yo, rot, xs, ys = rest[:5]
    if axis_index(n) < 2:          # floor/ceiling projection: both axes horizontal
        xs, ys = neg(xs), neg(ys)
    else:                          # wall projection: horizontal s, vertical t
        xs, rot = neg(xs), neg(rot)
    new = ([(-x, -y, z) for x, y, z in pts], tex, [xo, yo, rot, xs, ys] + rest[5:])
    for p in pts[:2]:              # the texture must stay on the geometry
        a, b = st(face, p), st(new, (-p[0], -p[1], p[2]))
        assert abs(a[0] - b[0]) < 0.01 and abs(a[1] - b[1]) < 0.01, (face, a, b)
    return new


def mirror_brush(b):
    return [mirror_face(f) for f in b]


def mirror_entity(e, suffix):
    kv = []
    for k, v in e["kv"]:
        if k == "origin":
            x, y, z = map(float, v.split())
            v = f"{num(-x)} {num(-y)} {num(z)}"
        elif k == "angle":
            v = num((float(v) + 180) % 360)
        elif k in ("target", "targetname"):
            v = v + suffix
        kv.append([k, v])
    if get(e, "angle") is None and get(e, "classname") not in ("light", "target_position"):
        kv.append(["angle", "180"])
    return {"kv": kv, "brushes": [mirror_brush(b) for b in e["brushes"]]}


def inside(lo, hi, rect, margin=2):
    x0, y0, x1, y1 = rect
    return lo[0] >= x0 - margin and hi[0] <= x1 + margin and lo[1] >= y0 - margin and hi[1] <= y1 + margin


def origin_in(e, rect):
    o = get(e, "origin")
    if o is None:
        return False
    x, y, _ = map(float, o.split())
    return rect[0] <= x <= rect[2] and rect[1] <= y <= rect[3]


def rework_source(ents):
    world = ents[0]
    kept = []
    for b in world["brushes"]:
        texs = {t for _, t, _ in b}
        if texs == {"skies/oasisnight"}:
            continue                                       # the old sky box
        if texs == {"common/full_clip"} and bbox(b)[1][2] > 100:   # tree trunks: solid for the physics as well
            b = [(p, TEX + "propclip", r) for p, _, r in b]       # (the low car lift clips stay player clip)
        b = [(p, {"kilt_texpack02/kilt_tribeswirl_red": "evil5_ctf/ctf_crete_red",
                  "kilt_texpack02/kilt_tribeswirl_blue": "evil5_ctf/ctf_crete_blue"}.get(t, t), r) for p, t, r in b]
        kept.append(b)

    blocks = (((128, 896, 1664, EY), "_sw"),                # workshop -> south-west
              ((-1664, 896, -128, EY), "_se"))              # apartment house -> south-east
    for rect, suffix in blocks:
        target = (-rect[2], -rect[3], -rect[0], -rect[1])
        old = [b for b in kept if inside(*bbox(b), target)]
        assert len(old) <= 1, f"{suffix}: {len(old)} brushes in the target block"
        kept = [b for b in kept if b not in old]
        src = [b for b in kept if inside(*bbox(b), rect)]
        kept += [mirror_brush(b) for b in src]
        ents += [mirror_entity(e, suffix) for e in ents[1:] if origin_in(e, rect) and not e["brushes"]]
        print(f"block {suffix}: {len(src)} brushes copied, {len(old)} removed")
    world["brushes"] = kept
    put(world, "message", "Trophy Square")
    trophy_base(ents)


def trophy_base(ents):
    """The old statue column in the fountain (32 units wide) is narrower than the plinth of the
    trophy (53): an octagonal granite base with a cap instead, the trophy centred on it."""
    world = ents[0]
    col = [b for b in world["brushes"] if [tuple(map(round, v)) for v in bbox(b)] == [(-16, -16, -56), (16, 16, -8)]]
    assert len(col) == 1, len(col)
    world["brushes"].remove(col[0])
    stone = {"m": TEX + "trophy_stone", "s": (0.125, 0.125)}

    def octagon(r):                 # same facets as the plinth (faces on the axes)
        return [(r * math.cos(math.radians(22.5 + 45 * k)), r * math.sin(math.radians(22.5 + 45 * k))) for k in range(8)]
    prism(octagon(34), -56, -13, stone)
    prism(octagon(37), -13, -8, stone)
    model = [e for e in ents if get(e, "model") == "models/mapobjects/q3r_trophy/trophy.md3"]
    assert len(model) == 1
    put(model[0], "origin", "0 0 -8")


# ---------------------------------------------------------------- city ring

def ring(ix, iy, ox, oy, z0, z1, spec, detail=False):
    """Four boxes filling the ring between the half extents (ix, iy) and (ox, oy)."""
    box((-ox, iy, z0), (ox, oy, z1), spec, detail=detail)
    box((-ox, -oy, z0), (ox, -iy, z1), spec, detail=detail)
    box((ix, -iy, z0), (ox, iy, z1), spec, detail=detail)
    box((-ox, -iy, z0), (-ix, iy, z1), spec, detail=detail)


def ground():
    """Sidewalk, ring road (centre line on the straight parts) and pavement up to the sky walls."""
    inner = 0
    for off, mat in RING:
        if mat == "road":
            lines, sc = "q3r_road/asph02linien", (ROAD_OUT - ROAD_IN) / 512
            a, b, c, d = EX + ROAD_IN, EX + ROAD_OUT, EY + ROAD_IN, EY + ROAD_OUT
            mid = (ROAD_IN + ROAD_OUT) / 2
            for sg in (1, -1):
                box((-a, min(sg * c, sg * d), ZG - 8), (a, max(sg * c, sg * d), ZG), "common/caulk", detail=False,
                    faces={"top": {"m": lines, "s": (sc, sc), "at": ((0, sg * (EY + mid), ZG), (0, 256))}})
                box((min(sg * a, sg * b), -c, ZG - 8), (max(sg * a, sg * b), c, ZG), "common/caulk", detail=False,
                    faces={"top": {"m": lines, "s": (sc, sc), "rot": 90, "at": ((sg * (EX + mid), 0, ZG), (0, 256))}})
                for sx in (1, -1):
                    box((min(sx * a, sx * b), min(sg * c, sg * d), ZG - 8), (max(sx * a, sx * b), max(sg * c, sg * d), ZG),
                        "common/caulk", detail=False, faces={"top": "q3r_road/asph02"})
        else:
            for r in _ring_boxes(EX + inner, EY + inner, EX + off, EY + off):
                cuts = [(r[0], mat)]
                if inner == 0 and r[2] - r[0] > 2 * EX:          # road mouths through the inner sidewalk
                    for c in ROADS_X:
                        cuts += [(c - ROAD_HALF, "q3r_road/asph02"), (c + ROAD_HALF, mat)]
                cuts.append((r[2], None))
                for (xa, m), (xb, _) in zip(cuts, cuts[1:]):
                    box((xa, r[1], ZG - 8), (xb, r[3], ZG), "common/caulk", detail=False,
                        faces={"top": {"m": m, "s": (0.5, 0.5)}})
        inner = off


def _ring_boxes(ix, iy, ox, oy):
    return ((-ox, iy, ox, oy), (-ox, -oy, ox, -iy), (ix, -iy, ox, iy), (-ox, -iy, -ix, iy))


def sky():
    s = "skies/oasisnight"
    t = 16
    box((-BX - t, -BY - t, SKY), (BX + t, BY + t, SKY + t), s, detail=False)
    box((-BX - t, BY, ZG - 72), (BX + t, BY + t, SKY), s, detail=False)
    box((-BX - t, -BY - t, ZG - 72), (BX + t, -BY, SKY), s, detail=False)
    box((BX, -BY, ZG - 72), (BX + t, BY, SKY), s, detail=False)
    box((-BX - t, -BY, ZG - 72), (-BX, BY, SKY), s, detail=False)


def prism(pts, z0, z1, side, top=None, obstacle=False):
    """Vertical prism over a convex polygon (counter-clockwise points)."""
    faces = []
    m = len(pts)
    for i in range(m):
        (ax, ay), (bx, by) = pts[i], pts[(i + 1) % m]
        n = (by - ay, ax - bx, 0)
        ln = math.hypot(n[0], n[1])
        faces.append(tface((n[0] / ln, n[1] / ln, 0), (ax, ay, z0), side, True))
    faces.append(tface((0, 0, 1), (pts[0][0], pts[0][1], z1), top or side, True))
    faces.append(tface((0, 0, -1), (pts[0][0], pts[0][1], z0), side, True))
    WORLD.append(faces)
    if obstacle:
        xs, ys = [p[0] for p in pts], [p[1] for p in pts]
        OBST.append((min(xs), min(ys), max(xs), max(ys)))


def corner_planters():
    """Triangular planters in the corners of the play area and of the ring road: no corner
    for a car (or a bot) to get wedged in."""
    conc = {"m": "q3r_building/concrete02", "s": (0.25, 0.25)}
    grass = {"m": "q3r_dirt_grass/grass01", "s": (0.5, 0.5)}
    for sx in (1, -1):
        for sy in (1, -1):
            for cx, cy, leg, tree in ((EX, EY, 256, "tree6"), (EX + RING[2][0], EY + RING[2][0], 320, "tree4")):
                pts = [(sx * cx, sy * cy), (sx * (cx - leg), sy * cy), (sx * cx, sy * (cy - leg))]
                if sx * sy < 0:
                    pts.reverse()
                prism(pts, ZG, ZG + 48, conc, grass, obstacle=cx == EX)
                entity("misc_model", sx * (cx - leg / 3), sy * (cy - leg / 3), ZG + 46,
                       model=f"models/mapobjects/GR_trees/{tree}.md3")


def boundary():
    """Concrete barrier at the old edge, open where the roads meet the ring road (red and white
    behind the base lanes); the invisible solid wall up to the sky stands in front of the
    buildings, so the ring road belongs to the play area."""
    W, H = 32, 48
    conc = {"m": "q3r_building/concrete02", "s": (0.25, 0.25)}
    stripe = {k: {"m": "q3r_road/cement_redwhite", "s": (0.25, 48 / 256), "at": ((0, 0, ZG), (0, 0))}
              for k in ("+x", "-x", "+y", "-y")}
    xs = [-EX - W] + [v for c in ROADS_X for v in (c - ROAD_HALF, c + ROAD_HALF)] + [EX + W]
    for sgn in (1, -1):
        y0, y1 = (EY, EY + W) if sgn > 0 else (-EY - W, -EY)
        for k, (xa, xb) in enumerate(zip(xs, xs[1:])):
            if k % 2 == 0:                                   # odd pieces are the road mouths
                box((xa, y0, ZG), (xb, y1, ZG + H), conc, detail=False)
        x0, x1 = (EX, EX + W) if sgn > 0 else (-EX - W, -EX)
        for ya, yb, red in ((-EY, -688, False), (-688, 688, True), (688, EY, False)):
            box((x0, ya, ZG), (x1, yb, ZG + H), conc, faces=stripe if red else None, detail=False)
    o, t = RING[2][0], 32                                     # in front of the buildings
    box((-EX - o - t, EY + o, ZG), (EX + o + t, EY + o + t, SKY), TEX + "boundary", detail=False)
    box((-EX - o - t, -EY - o - t, ZG), (EX + o + t, -EY - o, SKY), TEX + "boundary", detail=False)
    box((EX + o, -EY - o, ZG), (EX + o + t, EY + o, SKY), TEX + "boundary", detail=False)
    box((-EX - o - t, -EY - o, ZG), (-EX - o, EY + o, SKY), TEX + "boundary", detail=False)
    corner_planters()


def facade(name, storeys):
    w, h = tex_size(name)
    sc = STORY * storeys / h
    return name, sc, w * sc


def building(a, b, front, back, side, h, fac, rnd):
    """Backdrop building on one side of the ring. a..b along the side, front..back across it
    (distance from the centre line), h storeys."""
    name, sc, tile = facade(*fac)
    top = ZG + h * STORY

    def wall(key, lo, hi):
        # anchor the texture at the lower left corner of each face (seen from outside)
        # +y and -x faces would show the texture mirrored: flip s there
        return {"m": name, "s": (-sc if key in ("+y", "-x") else sc, sc), "at": ((lo[0], lo[1], ZG), (0, 0))}

    roof = {"m": "q3r_building/roof", "s": (1, 1)}
    if side in ("n", "s"):
        sg = 1 if side == "n" else -1
        lo, hi = (a, min(sg * front, sg * back), ZG), (b, max(sg * front, sg * back), top)
        backkey = "+y" if sg > 0 else "-y"
    else:
        sg = 1 if side == "e" else -1
        lo, hi = (min(sg * front, sg * back), a, ZG), (max(sg * front, sg * back), b, top)
        backkey = "+x" if sg > 0 else "-x"
    faces = {k: wall for k in ("+x", "-x", "+y", "-y")}
    faces.update({backkey: "common/caulk", "bot": "common/caulk", "top": roof})
    box(lo, hi, "common/caulk", faces=faces, detail=False)
    # cornice
    cap = {"m": "q3r_building/concrete02", "s": (0.5, 0.5)}
    box((lo[0] - 6, lo[1] - 6, top), (hi[0] + 6, hi[1] + 6, top + 14), cap, faces={"bot": "common/caulk"}, detail=True)
    # something on the roof
    if h >= 3 and rnd.random() < 0.7:
        cx, cy = rnd.uniform(lo[0] + 80, hi[0] - 80), rnd.uniform(lo[1] + 80, hi[1] - 80)
        s = rnd.choice((40, 56, 72))
        box((cx - s, cy - s, top + 14), (cx + s, cy + s, top + 14 + rnd.choice((48, 64, 96))),
            {"m": "q3r_metals/rustmet01", "s": (0.5, 0.5)}, faces={"bot": "common/caulk"}, detail=True)


def city():
    rnd = random.Random(404)
    front0 = 544
    widths = (384, 448, 512, 576, 640, 768)
    for side in ("n", "s", "e", "w"):
        if side in ("n", "s"):
            a, b, base = -BX, BX, EY
        else:
            a, b, base = -(EY + front0), EY + front0, EX
        x = a
        last = None
        while x < b - 1:
            w = rnd.choice(widths)
            if b - x - w < 300:
                w = b - x
            fac = rnd.choice([f for f in FACADES if f != last])
            last = fac
            h = rnd.choice((2, 3, 3, 4, 4, 5, 6))
            setback = rnd.choice((0, 0, 0, 32, 64))
            building(x, x + w, base + front0 + setback, base + BAND, side, h, fac, rnd)
            x += w


# ---------------------------------------------------------------- props

def streetlamp(x, y, yaw, solid=True):
    """Sodium street lamp (q3r_lanterns, arm towards yaw), light under the head,
    solid hull around the pole in the play area."""
    entity("misc_model", x, y, ZG, model="models/mapobjects/q3r_lanterns/streetlamp.md3", angle=num(yaw))
    a = math.radians(yaw)
    entity("light", x + 88 * math.cos(a), y + 88 * math.sin(a), ZG + 290, light=340, _color="1 0.72 0.38")
    if solid:
        box((x - 6, y - 6, ZG), (x + 6, y + 6, ZG + 330), TEX + "propclip", obstacle=True)


def lantern(x, y, yaw):
    entity("misc_model", x, y, ZG, model="models/mapobjects/q3r_lanterns/lantern_post.md3", angle=num(yaw))
    a = math.radians(yaw)
    entity("light", x + 34 * math.cos(a), y + 34 * math.sin(a), ZG + 106, light=220, _color="1 0.72 0.38")
    box((x - 5, y - 5, ZG), (x + 5, y + 5, ZG + 150), TEX + "propclip", obstacle=True)


def oak(x, y, yaw, kind=1):
    entity("misc_model", x, y, ZG - 2, model=f"models/mapobjects/oak/oak{kind}.md3", angle=num(yaw))
    box((x - 12, y - 12, ZG), (x + 12, y + 12, ZG + 140), TEX + "propclip", obstacle=True)


def parked_car(x, y, yaw, model="raptor/raptor_mapmodel"):
    entity("misc_model", x, y, ZG + 16, model=f"models/mapobjects/carmodels/{model}.md3", angle=num(yaw))
    a = math.radians(yaw)
    c, s = abs(math.cos(a)), abs(math.sin(a))
    hx, hy = 54 * c + 23 * s, 54 * s + 23 * c
    box((x - hx, y - hy, ZG), (x + hx, y + hy, ZG + 40), TEX + "propclip", obstacle=True)


def dumpster(x, y, yaw):
    entity("misc_model", x, y, ZG, model="models/mapobjects/dumpster/dumpster.md3", angle=num(yaw), modelscale=1.5)
    a = math.radians(yaw)
    c, s = abs(math.cos(a)), abs(math.sin(a))
    hx, hy = 40 * c + 20 * s, 40 * s + 20 * c
    box((x - hx, y - hy, ZG), (x + hx, y + hy, ZG + 56), TEX + "propclip", obstacle=True)


def low_wall(xa, xb, ya, yb, h=56):
    """Low concrete wall with a cap: cover against shots, cars stop at it."""
    (x0, x1), (y0, y1) = sorted((xa, xb)), sorted((ya, yb))
    box((x0, y0, ZG), (x1, y1, ZG + h), {"m": "q3r_building/concrete02", "s": (0.25, 0.25)}, obstacle=True)
    box((x0 - 4, y0 - 4, ZG + h), (x1 + 4, y1 + 4, ZG + h + 8), {"m": "q3r_building/concrete01", "s": (0.25, 0.25)})


def planter(xa, xb, ya, yb, tree=None):
    (x0, x1), (y0, y1) = sorted((xa, xb)), sorted((ya, yb))
    box((x0, y0, ZG), (x1, y1, ZG + 40), {"m": "q3r_building/concrete02", "s": (0.25, 0.25)},
        faces={"top": {"m": "q3r_dirt_grass/grass01", "s": (0.5, 0.5)}}, obstacle=True)
    if tree:
        entity("misc_model", (x0 + x1) / 2, (y0 + y1) / 2, ZG + 38, model=f"models/mapobjects/GR_trees/{tree}.md3")


def kiosk(xa, xb, ya, yb, h=176):
    """Small stucco building with a flat roof (cover in the open areas)."""
    (x0, x1), (y0, y1) = sorted((xa, xb)), sorted((ya, yb))
    wall = {"m": "q3r_building/stuccowin03", "s": (1.25, 1.25), "at": ((x0, y0, ZG), (0, 0))}
    box((x0, y0, ZG), (x1, y1, ZG + h), wall, faces={"top": {"m": "q3r_building/roof", "s": (1, 1)}, "bot": "common/caulk"},
        obstacle=True)
    box((x0 - 12, y0 - 12, ZG + h), (x1 + 12, y1 + 12, ZG + h + 16), {"m": "q3r_building/concrete01", "s": (0.25, 0.25)})


def parking(xa, xb, ya, yb):
    """Parking lot surface (2 units above the grass): a row of bays (across y) at each long side,
    the aisle between them plain."""
    (x0, x1), (y0, y1) = sorted((xa, xb)), sorted((ya, yb))
    plain = {"m": "q3r_road/asph02", "s": (0.5, 0.5)}
    bay = 160
    for ya, yb, kind in ((y0, y0 + bay, "bay"), (y0 + bay, y1 - bay, "aisle"), (y1 - bay, y1, "bay")):
        top = plain if kind == "aisle" else {"m": "q3r_road/asph02parking", "s": (80 / 512, bay / 512),
                                             "at": ((x0, yb, ZG + 2), (0, 0))}
        box((x0, ya, ZG), (x1, yb, ZG + 2), plain, faces={"top": top, "bot": "common/caulk"})


def item(x, y, yaw, classname, z=ZG + 24):
    entity(classname, x, y, z, angle=num(yaw))


def props():
    # lamps along the inner roads (each with its copy turned by 180 degrees)
    # (56 units from the kerb: bots cutting a corner must not catch the pole)
    for x, y, yaw in ((1608, 2000, 0), (1972, 1500, 180), (1608, 1050, 0),
                      (1608, -1100, 0), (1972, -1550, 180), (1608, -2000, 0),
                      (184, 1700, 180), (184, 2060, 180),
                      (-1300, 588, 90), (-500, 588, 90), (500, 588, 90), (1300, 588, 90),
                      (900, 184, 270), (1400, -184, 90)):
        both(lambda a, b, c: streetlamp(a, b, c), x, y, yaw)
    # lantern posts at the corners of the square
    for x, y in ((360, 360), (-360, 360)):
        both(lambda a, b, c: lantern(a, b, c), x, y, math.degrees(math.atan2(-y, -x)))

    # inner blocks between the roads and the square
    def inner(x, y, yaw):
        s = 1 if yaw == 0 else -1
        oak(s * 1050, s * 430, yaw, 1)
        planter(*sorted((s * 640, s * 900)), *sorted((s * 330, s * 410)), tree="tree6")
        low_wall(*sorted((s * 1250, s * 1520)), *sorted((s * 300, s * 332)))
        item(s * 1380, s * 500, yaw, "item_health")
    inner(0, 0, 0)
    inner(0, 0, 180)

    def inner_west(x, y, yaw):
        s = 1 if yaw == 0 else -1
        kiosk(*sorted((s * -1010, s * -850)), *sorted((s * 300, s * 420)))
        oak(s * -1420, s * 380, yaw, 3)
        parked_car(s * -560, s * 470, 90 if s > 0 else 270, "sidepipe/sidepipe_mapmodel")
        item(s * -700, s * 250, yaw, "ammo_grenades")
    inner_west(0, 0, 0)
    inner_west(0, 0, 180)

    # north-west corner (red side): parking lot; copy in the south-east (blue side)
    def lot(x, y, yaw):
        s = 1 if yaw == 0 else -1
        X = lambda v: s * v
        x0, x1, y0, y1 = sorted((X(-3800), X(-2300))) + sorted((X(1180), X(1860)))
        parking(x0, x1, y0, y1)
        models = ("raptor/raptor_mapmodel", "raptor2/raptor2_mapmodel", "sidepipe/sidepipe_mapmodel")
        rnd = random.Random(11)
        for row_y in (1260, 1780):
            for k, bx in enumerate(range(-3760, -2340, 80)):
                if rnd.random() < 0.45:
                    parked_car(X(bx), X(row_y), 270 if (row_y > 1500) == (s > 0) else 90, rnd.choice(models))
        streetlamp(X(-3050), X(1520), (90 if s > 0 else 270) + 90, solid=True)
        streetlamp(X(-2550), X(1520), (90 if s > 0 else 270) - 90, solid=True)
        dumpster(X(-3950), X(2000), 0)
        dumpster(X(-3850), X(2010), 0)
        item(X(-3050), X(1650), yaw, "item_health_large")
        item(X(-2550), X(1400), yaw, "ammo_slugs")
    lot(0, 0, 0)
    lot(0, 0, 180)

    # north-east corner (blue side): small park with a kiosk; copy in the south-west (red side)
    def park(x, y, yaw):
        s = 1 if yaw == 0 else -1
        X = lambda v: s * v
        kiosk(*sorted((X(2900), X(3140))), *sorted((X(1300), X(1440))))
        for k, (px, py) in enumerate(((2350, 1150), (2600, 1900), (3600, 1250), (3900, 1750), (3300, 2000), (2250, 1700))):
            oak(X(px), X(py), yaw, (1, 3, 5, 2, 4, 1)[k])
        low_wall(*sorted((X(3500), X(3800))), *sorted((X(1500), X(1532))))
        streetlamp(X(3000), X(1700), 0 if s > 0 else 180)
        item(X(3020), X(1220), yaw, "item_health_large")
        item(X(3600), X(1600), yaw, "ammo_shells")
    park(0, 0, 0)
    park(0, 0, 180)

    # lamps on the outer sidewalk of the ring road
    for k in range(-6, 7):
        x = k * 640
        for sgn in (1, -1):
            streetlamp(sgn * x, sgn * (EY + 496), 270 if sgn > 0 else 90)
    for k in range(-2, 3):
        for sgn in (1, -1):
            streetlamp(sgn * (EX + 496), sgn * k * 700, 180 if sgn > 0 else 0)


# ---------------------------------------------------------------- checks and output

def check_free(world_old):
    """Props must not stand in the original geometry (except the ground)."""
    taken = []
    for b in world_old:
        lo, hi = bbox(b)
        if hi[2] > ZG + 6 and lo[2] < ZG + 200 and max(abs(lo[0]), abs(hi[0])) <= EX and max(abs(lo[1]), abs(hi[1])) <= EY:
            taken.append((lo[0], lo[1], hi[0], hi[1]))
    bad = 0
    for o in OBST:
        for t in taken:
            if o[0] < t[2] and o[2] > t[0] and o[1] < t[3] and o[3] > t[1]:
                print("overlap:", [round(v) for v in o], "with", [round(v) for v in t])
                bad += 1
                break
    return bad


ARENA_TEXT = """{
//MAP FILE NAME
map "q3r_ctf_04"

//IN-GAME MAP NAME
longname "Trophy Square"

//AUTHOR OF THE MAP
author "P3rlE"

//GAME TYPE SUPPORTS
type "q3r_ctf"

//CUSTOM BOTS
//bots ""

//PLAYER STARTS
starts "10"

//LAPTIME
laptime ""

//CHECKPOINTS
checkpoints ""

//OBSERVER SPOTS
observerspots ""

//WEAPONS
weapons "BFG,SG,GL,FT,RG"

//POWERUPS
powerups ""

//REVERSABLE
reversable ""

//TRACK LENGTHS
tracklengths ""

//BOT SUPPORTS
botsupport "Good"

//TEAMS
teams "2"
}
"""


def main():
    ents = parse(SRC)
    rework_source(ents)
    old_world = list(ents[0]["brushes"])
    sky()
    ground()
    boundary()
    city()
    props()
    bad = check_free(old_world)
    ents[0]["brushes"] += WORLD
    ents += ENTS
    write(ents, OUT)
    ARENA.write_text(ARENA_TEXT, newline="\n")
    nb = sum(len(e["brushes"]) for e in ents)
    print(f"written {OUT.relative_to(ROOT)}: {len(ents)} entities, {nb} brushes, {len(WORLD)} new; {bad} overlaps")


if __name__ == "__main__":
    main()
