#!/usr/bin/env python3
"""Rebuild the editable Crossroads CTF4 terrain map (V0.2).

V0.2 compared with the V0.1 prototype:
- 30 % larger: 5376 x 5376 units of playable terrain, bases at (+-1664, +-1664).
- 32-unit collision grid (was 128). Heights come from a 16-unit sample grid that
  is smoothed before triangulation, and every cell takes the diagonal that fits
  the surface best, so there are far fewer kinks between triangles. The dirt
  roads are additionally levelled across their width, and on the diagonal
  roads the triangles run along the road.
- Blended ground: one shader index map (q3map2 "_indexmap") drives vertex alpha
  between six ground layers (meadow, road dirt, gravel, creek pebbles, dry
  grass, concrete), so materials fade into each other instead of switching per
  triangle. A large-scale macro texture breaks up the tiling.
  Tyre friction stays exact: the brush sides of each triangle use one of four
  parent shaders (grass / dirt / gravel / plain) with the matching surface flag.
- The creek is a real channel with banks, a pebble bed and clear, reflective
  water (OpenGL2 q3r_water). It meanders gently, its width varies along the
  course, it has two gravel islands and three wide gravel fords (centre and
  both side roads, where it crosses the axis).
  Deep sections slow the cars down (water drag), the fords do not.
  The creek valley continues into the surrounding hills.
- Boulders along the creek and in the gravel pit, gravel heaps in the pit.

Terrain relief, roads, bases and scenery keep the two mirror symmetries of
V0.1. The meandering creek is point symmetric about the centre (red <-> yellow,
blue <-> green); it crosses every road at the same place for all teams, so all
road distances, fords and slopes stay equal. Collision uses closed, triangulated detail brushes,
not render-only patches. Run with Python 3 + Pillow.
"""
from __future__ import annotations

import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
NAME = "q3r_crossroads"
VERSION = "0.2"
STEP = 32                      # collision grid
FSTEP = STEP // 2              # height sample grid
EXTENT = 2688                  # playable half size
CELLS = 2 * EXTENT // STEP     # 168
BASE = 1664                    # base centres at (+-BASE, +-BASE)
PLAZA = 320                    # flat yard half size around each base
PLAZA_Z = 176
WL = 36                        # creek water level
HORIZON = 8192
FLOOR = -128
TEX = "q3r_crossroads/"
DETAIL = 134217728
BASES = (("red", -BASE, BASE), ("blue", BASE, BASE),
         ("green", -BASE, -BASE), ("yellow", BASE, -BASE))
# Ground layers of the index map, in shader index order.
LAYERS = ("grass", "dirt", "gravel", "pebbles", "drygrass", "concrete")
L_GRASS, L_DIRT, L_GRAVEL, L_PEBBLES, L_DRY, L_CONCRETE = range(6)
# Collision class (parent shader, tyre friction) of each layer.
FRICTION = {L_GRASS: "grass", L_DRY: "grass", L_DIRT: "dirt", L_GRAVEL: "gravel",
            L_PEBBLES: "gravel", L_CONCRETE: "plain"}
ISLAND_X = 1040
MEANDER = 140                  # amplitude of the creek bends
HEAPS = ((330, 760, 44, 140), (760, 400, 36, 120))   # gravel heaps (x, y off the creek, height, radius)

GRID = {}        # (x, y) -> height of the collision vertices
DIAG = {}        # (x0, y0) -> "ac" or "bd", diagonal of the cell
MAT = {}         # (x, y) -> ground layer of the vertex
BRUSHES = []     # worldspawn brushes
GROUPS = {}      # friction class -> terrain brushes (func_group with index map)
ENTITIES = []
SOLIDS = []      # axis aligned bounds of solid scenery (validation)
ROCKS = []       # (x, y, radius) of boulders (validation)


def smooth(a, b, x):
    t = min(1.0, max(0.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def gauss(x, w):
    return math.exp(-(x / w) ** 2)


# ---------------------------------------------------------------- height field

def yc(x):
    """Centre line of the creek: odd in x, on the axis at the three fords."""
    return MEANDER * math.sin(math.pi * x / BASE)


def creek(x):
    """Half width, depth, ford and island weight of the creek at x (even in x)."""
    ax = abs(x)
    ford = max(gauss(x, 420), gauss(ax - BASE, 240))
    island = gauss(ax - ISLAND_X, 230)
    width = 150 + 30 * math.cos(x / 290) * math.cos(x / 610) + 60 * ford + 90 * island
    depth = 22 - 15 * ford
    return width, depth, ford, island


def creek_profile(x, y):
    """Channel, banks and valley floor near the creek."""
    ax, ay = abs(x), abs(y - yc(x))
    width, depth, _, _ = creek(x)
    d = ay / width
    floor = 50 + 8 * smooth(600, 2400, ax)
    if d < 1:
        z = WL - depth * (1 - smooth(0.5, 1.0, d))
    else:
        z = WL + (floor - WL) * smooth(1.0, 2.0, d)
    # gravel islands split the creek into two arms
    e = ((ax - ISLAND_X) / 220) ** 2 + (ay / 100) ** 2
    if e < 1:
        z = max(z, WL - depth + (depth + 6) * (1 - smooth(0.15, 1.0, e)))
    return z


def relief(x, y):
    ax, ay = abs(x), abs(y)
    r = math.hypot(x, y)
    z = 40 + 140 * smooth(360, 1540, r)                       # central basin
    z += 130 * smooth(2050, EXTENT, max(ax, ay))              # rim of the valley
    z += 26 * math.cos(x / 350) * math.cos(y / 400) * smooth(900, 1560, r)
    z += 9 * math.cos(x / 157) * math.cos(y / 181) * smooth(700, 1300, r)
    # low spoil banks break the sightlines across the basin
    z += 46 * math.exp(-((ax - 845) / 300) ** 2 - ((ay - 730) / 340) ** 2)
    return z


def height(x, y):
    """Analytic height. Two mirror symmetries keep all four teams equal."""
    ax, ay = abs(x), abs(y)
    stream = 1 - smooth(140, 1080, abs(y - yc(x)))
    z = relief(x, y) * (1 - stream) + creek_profile(x, y) * stream
    for hx, hy in heaps():
        hh, hr = hx[1], hy[1]
        z += hh * math.exp(-((x - hx[0]) ** 2 + (y - hy[0]) ** 2) / hr ** 2)
    cheb = max(abs(ax - BASE), abs(ay - BASE))
    plateau = 1 - smooth(PLAZA, PLAZA + 540, cheb)
    z = z * (1 - plateau) + PLAZA_Z * plateau
    return min(400.0, max(0.0, z))


def heaps():
    """Gravel heaps in the pit, placed relative to the creek (point symmetric)."""
    out = []
    for hx, hy, hh, hr in HEAPS:
        for sx in (-1, 1):
            for sy in (-1, 1):
                x = sx * hx
                out.append(((x, hh), (yc(x) + sy * hy, hr)))
    return out


def road(x, y):
    """0..1 weight of the dirt roads (soft, slightly wobbly edges)."""
    ax, ay = abs(x), abs(y)
    best = 0.0
    t = (ax + ay) / math.sqrt(2)
    if t < BASE * math.sqrt(2):                               # centre to bases
        hw = 80 + 12 * math.cos(t / 140)
        best = max(best, 1 - smooth(hw - 28, hw + 28, abs(ax - ay) / math.sqrt(2)))
    if ay < BASE:                                             # side roads N-S
        hw = 80 + 12 * math.cos(ay / 150)
        best = max(best, 1 - smooth(hw - 28, hw + 28, abs(ax - BASE)))
    if ax < BASE:                                             # side roads E-W
        hw = 80 + 12 * math.cos(ax / 150)
        best = max(best, 1 - smooth(hw - 28, hw + 28, abs(ay - BASE)))
    return best


def blur(field, n, passes):
    """Separable binomial blur with clamped borders (symmetric)."""
    k = (1, 4, 6, 4, 1)
    for _ in range(passes):
        for axis in (0, 1):
            out = [[0.0] * n for _ in range(n)]
            for i in range(n):
                for j in range(n):
                    s = 0.0
                    for o, w in zip(range(-2, 3), k):
                        if axis == 0:
                            s += w * field[min(n - 1, max(0, i + o))][j]
                        else:
                            s += w * field[i][min(n - 1, max(0, j + o))]
                    out[i][j] = s / 16
            field = out
    return field


def in_plaza(x, y):
    return any(max(abs(x - bx), abs(y - by)) <= PLAZA for _, bx, by in BASES)


def build_heights():
    n = 2 * EXTENT // FSTEP + 1
    coord = [-EXTENT + i * FSTEP for i in range(n)]
    raw = [[height(x, y) for y in coord] for x in coord]
    roads = [[road(x, y) for y in coord] for x in coord]
    soft = blur(raw, n, 3 * (32 // FSTEP) ** 2)
    fine = [[0.0] * n for _ in range(n)]
    for i, x in enumerate(coord):
        for j, y in enumerate(coord):
            a = 0.35 + 0.65 * roads[i][j]
            z = raw[i][j] * (1 - a) + soft[i][j] * a
            if in_plaza(x, y):
                z = PLAZA_Z
            fine[i][j] = z
    for i in range(0, n, 2):
        for j in range(0, n, 2):
            GRID[coord[i], coord[j]] = round(fine[i][j] * 8) / 8
    for i in range(1, n, 2):
        for j in range(1, n, 2):
            x0, y0 = coord[i - 1], coord[j - 1]
            a, b = GRID[x0, y0], GRID[x0 + STEP, y0]
            c, d = GRID[x0 + STEP, y0 + STEP], GRID[x0, y0 + STEP]
            hc = fine[i][j]
            ac, bd = abs((a + c) / 2 - hc), abs((b + d) / 2 - hc)
            cx, cy = x0 + STEP / 2, y0 + STEP / 2
            on_diagonal_road = abs(abs(cx) - abs(cy)) < 200 and abs(cx) + abs(cy) < 2 * BASE
            if abs(ac - bd) < 0.05 or on_diagonal_road:
                # quadrant rule: symmetric, and parallel to the diagonal roads
                DIAG[x0, y0] = "ac" if (x0 + STEP / 2) * (y0 + STEP / 2) >= 0 else "bd"
            else:
                DIAG[x0, y0] = "ac" if ac < bd else "bd"


def ground(x, y):
    """Height of the actual piecewise planar collision, not the analytic field."""
    ix = min(CELLS - 1, max(0, math.floor((x + EXTENT) / STEP)))
    iy = min(CELLS - 1, max(0, math.floor((y + EXTENT) / STEP)))
    x0, y0 = -EXTENT + ix * STEP, -EXTENT + iy * STEP
    u, v = (x - x0) / STEP, (y - y0) / STEP
    a, b = GRID[x0, y0], GRID[x0 + STEP, y0]
    c, d = GRID[x0 + STEP, y0 + STEP], GRID[x0, y0 + STEP]
    if DIAG[x0, y0] == "ac":
        if v <= u:
            return a + (b - a) * u + (c - b) * v
        return a + (c - d) * u + (d - a) * v
    if u + v <= 1:
        return a + (b - a) * u + (d - a) * v
    return c + (d - c) * (1 - u) + (b - c) * (1 - v)


def material(x, y):
    """Ground layer of a terrain vertex."""
    ax, ay = abs(x), abs(y)
    z = GRID[x, y]
    if in_plaza(x, y):
        return L_CONCRETE
    width, _, ford, island = creek(x)
    d = abs(y - yc(x)) / width
    if (ford > 0.45 and d < 1.45) or (island > 0.5 and d < 1.2):
        return L_GRAVEL
    if z < WL + 1.5 or d < 1.2:
        return L_PEBBLES
    wobble = 50 * math.cos(x / 120) * math.cos(y / 150)
    if math.hypot(x, y) < 760 + wobble:
        return L_GRAVEL
    for hx, hy in heaps():
        if math.hypot(x - hx[0], y - hy[0]) < hy[1] * 0.95:
            return L_GRAVEL
    w = road(x, y)
    if w > 0.5:
        return L_DIRT
    patches = math.cos(x / 430) * math.cos(y / 380) + 0.5 * math.cos(x / 170) * math.cos(y / 215)
    rim = max(ax, ay) > 2420 + 70 * math.cos(x / 230) * math.cos(y / 260)
    if w < 0.05 and (patches > 0.95 or rim):
        return L_DRY
    return L_GRASS


# ---------------------------------------------------------------- brushes

def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def plane(normal, point):
    up = (1, 0, 0) if tuple(normal[:2]) == (0, 0) else (0, 0, 1)
    t1 = cross(up, normal)
    t2 = cross(normal, t1)
    size = max(abs(c) for c in t1 + t2)
    return (point, tuple(p + c * 128 / size for p, c in zip(point, t2)),
            tuple(p + c * 128 / size for p, c in zip(point, t1)))


def num(v):
    s = f"{v:.5f}".rstrip("0").rstrip(".")
    return s if s not in ("", "-0") else "0"


def face(points, material, detail=True, scale=0.5):
    p = " ".join("( %s )" % " ".join(num(v) for v in xyz) for xyz in points)
    return f"{p} {material} 0 0 0 {scale} {scale} {DETAIL if detail else 0} 0 0"


def brush(faces, group=None):
    text = "{\n" + "\n".join(faces) + "\n}"
    (BRUSHES if group is None else GROUPS.setdefault(group, [])).append(text)


def box(lo, hi, material, detail=True, collision=True):
    x1, y1, z1 = lo
    x2, y2, z2 = hi
    assert x2 > x1 and y2 > y1 and z2 > z1, (lo, hi)
    normals = ((0, 0, -1), (0, 0, 1), (-1, 0, 0), (1, 0, 0), (0, -1, 0), (0, 1, 0))
    points = ((x1, y1, z1), (x1, y1, z2), (x1, y1, z1),
              (x2, y1, z1), (x1, y1, z1), (x1, y2, z1))
    brush([face(plane(n, p), material, detail) for n, p in zip(normals, points)])
    if collision and detail:
        SOLIDS.append((lo, hi))


def prism(vertices, material, scale=0.75, group=None):
    """CCW top vertices. Shared vertices give exactly watertight seams."""
    a, b, c = vertices
    faces = [face((a, c, b), material, scale=scale),
             face(plane((0, 0, -1), (0, 0, FLOOR)), "common/caulk")]
    for p, q in ((a, b), (b, c), (c, a)):
        # Clockwise edge side as seen from outside; vertical sides are hidden.
        faces.append(face(((p[0], p[1], FLOOR), p, q), "common/caulk"))
    brush(faces, group)


def convex(planes, material):
    """Brush from (outward normal, point on plane) pairs."""
    brush([face(plane(n, p), material) for n, p in planes])


def boulder(x, y, radius, tall, seed, mat=TEX + "rock"):
    """Irregular convex rock: a cut polytope, sunk into the ground."""
    z0 = min(ground(x + dx, y + dy) for dx in (-radius, 0, radius) for dy in (-radius, 0, radius))
    centre = (x, y, z0)
    planes = [((0, 0, -1), (x, y, z0 - 24)), ((0, 0, 1), (x, y, z0 + tall))]
    rnd = abs(math.sin(seed * 12.9898) * 43758.5453) % 233280
    for k in range(16):
        rnd = (rnd * 9301 + 49297) % 233280
        jitter = rnd / 233280
        if k < 7:       # steep flanks keep the footprint close to the radius
            azimuth = (k + 0.4 * jitter) * 2 * math.pi / 7 + seed
            elevation = math.radians(4 + 10 * jitter)
            reach = radius * (0.85 + 0.25 * jitter)
        else:           # sloping facets round off the top
            azimuth = (k - 7 + 0.5 * jitter) * 2 * math.pi / 9 + seed + 0.3
            elevation = math.radians(28 + 34 * ((k * 7 + int(seed * 3)) % 5) / 4)
            reach = radius * (0.7 + 0.25 * jitter)
        n = (math.cos(azimuth) * math.cos(elevation), math.sin(azimuth) * math.cos(elevation),
             math.sin(elevation))
        reach = reach * (1 - n[2]) + tall * 0.92 * n[2]
        planes.append((n, tuple(c + nc * reach for c, nc in zip(centre, n))))
    corners = polytope(planes)
    convex(planes, mat)
    lo = tuple(min(p[i] for p in corners) for i in range(3))
    hi = tuple(max(p[i] for p in corners) for i in range(3))
    SOLIDS.append((lo, hi))
    ROCKS.append((x, y, max(hi[0] - x, x - lo[0], hi[1] - y, y - lo[1])))


def polytope(planes):
    """Corners of the convex brush given by (outward normal, point) planes."""
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


def entity(classname, x, y, z=None, **keys):
    if z is None:
        z = ground(x, y) + 48
    fields = {"classname": classname, "origin": f"{x:g} {y:g} {z:g}", **keys}
    ENTITIES.append(fields)
    return fields


# ---------------------------------------------------------------- terrain

RINGS = (EXTENT, 3200, 4000, 5100, 6500, HORIZON)


def background_height(x, y):
    """Decorative landscape outside the solid playable boundary."""
    radius = max(abs(x), abs(y))
    bx, by = x * EXTENT / radius, y * EXTENT / radius
    z = ground(min(EXTENT, max(-EXTENT, bx)), min(EXTENT, max(-EXTENT, by)))
    distance = radius - EXTENT
    hills = 220 * smooth(0, 1600, distance) + 1000 * smooth(800, 5500, distance)
    hills += 240 * math.sin(x / 790) ** 2 * math.cos(y / 670) ** 2 * smooth(0, 1400, distance)
    # The creek valley runs out to the east and west and closes far away.
    valley = 0.0
    if abs(bx) >= abs(by):
        valley = (1 - smooth(320, 1150, abs(by - yc(bx)))) * (1 - smooth(3600, 5000, distance))
    return z + hills * (1 - valley)


def background():
    # Rings keep the border vertices of the playable mesh and widen
    # outward with the same number of points per side (no T-junctions).
    for inner, outer in zip(RINGS[:-1], RINGS[1:]):
        for side in range(4):
            def point(extent, step):
                t = -extent + step * 2 * extent / CELLS
                x, y = ((t, -extent), (extent, t), (-t, extent), (-extent, -t))[side]
                z = GRID[x, y] if extent == EXTENT else background_height(x, y)
                return (x, y, round(z * 8) / 8)
            for i in range(CELLS):
                a, b, c, d = point(inner, i), point(outer, i), point(outer, i + 1), point(inner, i + 1)
                prism((a, b, c), TEX + "backdrop", scale=1.5)
                prism((a, c, d), TEX + "backdrop", scale=1.5)


def terrain():
    build_heights()
    for (x, y) in GRID:
        MAT[x, y] = material(x, y)
    order = ("plain", "grass", "gravel", "dirt")
    for y in range(-EXTENT, EXTENT, STEP):
        for x in range(-EXTENT, EXTENT, STEP):
            a, b = (x, y, GRID[x, y]), (x + STEP, y, GRID[x + STEP, y])
            c, d = (x + STEP, y + STEP, GRID[x + STEP, y + STEP]), (x, y + STEP, GRID[x, y + STEP])
            triangles = ((a, b, c), (a, c, d)) if DIAG[x, y] == "ac" else ((a, b, d), (b, c, d))
            for tri in triangles:
                classes = [FRICTION[MAT[p[0], p[1]]] for p in tri]
                major = max(set(classes), key=lambda k: (classes.count(k), order.index(k)))
                prism(tri, TEX + "t_" + major, scale=1, group=major)
    # A translucent, invisible SOLID boundary also works with Bullet's solid-only
    # static world import. A playerclip-only wall would not block Bullet cars.
    e = EXTENT
    box((-e - 16, -e - 16, FLOOR), (-e, e + 16, 2048), TEX + "boundary", collision=False)
    box((e, -e - 16, FLOOR), (e + 16, e + 16, 2048), TEX + "boundary", collision=False)
    box((-e, -e - 16, FLOOR), (e, -e, 2048), TEX + "boundary", collision=False)
    box((-e, e, FLOOR), (e, e + 16, 2048), TEX + "boundary", collision=False)
    background()
    # Structural shell encloses the decorative hills as well as the field.
    h, roof = HORIZON, 3072
    box((-h - 64, -h - 64, -192), (h + 64, h + 64, FLOOR), "common/caulk", False)
    box((-h - 64, -h - 64, FLOOR), (-h, h + 64, roof), TEX + "sky", False)
    box((h, -h - 64, FLOOR), (h + 64, h + 64, roof), TEX + "sky", False)
    box((-h, -h - 64, FLOOR), (h, -h, roof), TEX + "sky", False)
    box((-h, h, FLOOR), (h, h + 64, roof), TEX + "sky", False)
    box((-h - 64, -h - 64, roof), (h + 64, h + 64, roof + 64), TEX + "sky", False)
    # Water: one sheet over the playable creek, two wedges along the valleys.
    # Only the surface is drawn; hidden sides would show as sheets under water.
    half, hidden = 300, "common/nodrawnonsolid"
    far = e + 5000

    def side(m):
        n = math.hypot(m, 1)
        return m / n, 1 / n
    wedges = []
    for s in (-1, 1):
        c = yc(s * e)
        up, lo = side((c + half) / e), side((c - half) / e)
        wedges.append([((0, 0, 1), (s * e, 0, WL)), ((0, 0, -1), (s * e, 0, WL - 60)),
                       ((-s, 0, 0), (s * e, 0, WL)), ((s, 0, 0), (s * far, 0, WL)),
                       ((-s * up[0], up[1], 0), (s * e, c + half, WL)),
                       ((s * lo[0], -lo[1], 0), (s * e, c - half, WL))])
    inner = MEANDER + half
    for planes in [[((0, 0, 1), (0, 0, WL)), ((0, 0, -1), (0, 0, WL - 60)),
                    ((-1, 0, 0), (-e, 0, WL)), ((1, 0, 0), (e, 0, WL)),
                    ((0, 1, 0), (0, inner, WL)), ((0, -1, 0), (0, -inner, WL))]] + wedges:
        brush([face(plane(nrm, pt), TEX + "water" if nrm == (0, 0, 1) else hidden)
               for nrm, pt in planes])
    # Light grid only where cars drive; the hills use the coarse world grid.
    box((-e, -e, -64), (e, e, 1100), "common/lightgrid", collision=False)


def index_map():
    """8 bit PCX: one pixel per terrain vertex, value = ground layer."""
    from PIL import Image
    n = CELLS + 1
    im = Image.new("L", (n, n))
    im.putdata([MAT[-EXTENT + i * STEP, EXTENT - j * STEP] for j in range(n) for i in range(n)])
    target = ROOT / "baseq3r" / "maps" / f"{NAME}_index.pcx"
    im.save(target)
    return target


# ---------------------------------------------------------------- dressing

def dressing():
    for color, bx, by in BASES:
        sx, sy = (-1 if bx < 0 else 1), (-1 if by < 0 else 1)

        def block(u1, v1, z1, u2, v2, z2, mat):
            xs, ys = sorted((bx + sx * u1, bx + sx * u2)), sorted((by + sy * v1, by + sy * v2))
            box((xs[0], ys[0], z1), (xs[1], ys[1], z2), TEX + mat)

        # Same footprint and open approaches; dressing stays behind the flag court.
        # Roof is safely above vehicle height, with 320 units between pillars.
        gz = max(ground(bx + sx * u, by + sy * v) for u in (-240, 240) for v in (384, 576))
        for u in (-224, 224):
            for v in (400, 560):
                block(u - 12, v - 12, ground(bx + sx * u, by + sy * v) - 24,
                      u + 12, v + 12, gz + 248, "wood" if color in ("red", "green") else "metal")
        block(-264, 368, gz + 240, 264, 592, gz + 264, "roof")
        block(-264, 366, gz + 208, 264, 382, gz + 240, color)
        # Equal flag courts stay flush with the terrain; two colored bollards
        # frame the flag across the diagonal road, clear of all three approaches.
        for u, v in ((-260, 260), (260, -260)):
            block(u - 12, v - 12, PLAZA_Z, u + 12, v + 12, PLAZA_Z + 68, color)
        # Each outbuilding has the same solid footprint for fair cover.
        gz2 = max(ground(bx + sx * u, by + sy * v) for u in (384, 576) for v in (-192, 192))
        block(384, -192, gz2 - 100, 576, 192, gz2 + 184,
              "wood" if color in ("red", "green") else "concrete")
        block(368, -208, gz2 + 184, 592, 208, gz2 + 208, "roof")
        block(376, -160, gz2 + 96, 384, 160, gz2 + 136, color)
        if color == "red":  # stacked timber, outside approaches
            for u in (-160, -64, 32, 128):
                block(u, 608, gz, u + 48, 752, gz + 48, "wood")
        elif color == "blue":  # abandoned fuel pumps behind the canopy
            for u in (-120, 120):
                block(u - 24, 612, gz, u + 24, 656, gz + 100, "metal")
                block(u - 28, 608, gz + 68, u + 28, 660, gz + 112, "blue")
        elif color == "green":  # ranger station equipment lockers
            for u in (-120, 24):
                block(u, 616, gz, u + 96, 696, gz + 112, "green")
        else:  # quarry hopper and conveyor, visually legible silhouette
            block(-128, 608, gz + 24, 128, 688, gz + 156, "metal")
            block(-144, 592, gz + 156, 144, 704, gz + 180, "yellow")
            block(-24, 704, gz + 32, 24, 752, gz + 48, "metal")
        # Two safe initial starts and four separate respawns per team.
        angle = math.degrees(math.atan2(-by, -bx)) % 360
        for u in (-128, 128):
            entity(f"team_CTF_{color}player", bx + sx * u, by + sy * 160, angle=f"{angle:g}")
        for u, v in ((-192, -160), (192, -160), (-128, 256), (128, 256)):
            entity(f"team_CTF_{color}spawn", bx + sx * u, by + sy * v, angle=f"{angle:g}")
        entity(f"team_CTF_{color}flag", bx, by)
        for classname, u, v in (("weapon_rocketlauncher", -384, 230),
                                 ("ammo_rockets", -384, -230),
                                 ("item_health_large", 230, -384),
                                 ("item_fuelcan", -230, -384)):
            entity(classname, bx + sx * u, by + sy * v)
        entity("info_player_deathmatch", bx - sx * 384, by, angle=f"{angle:g}")

    for sx in (-1, 1):
        for sy in (-1, 1):
            # Sparse symmetric edge groves with wide gaps, both shortcut routes open.
            for ax, ay, scale in ((2310, 1010, 1.65), (2390, 1440, 1.8), (2310, 1940, 1.65),
                                  (990, 2310, 1.7), (1400, 2420, 1.9), (1870, 2330, 1.7),
                                  (830, 1270, 1.35), (1250, 830, 1.5), (2450, 560, 1.6),
                                  (560, 2450, 1.6), (1130, 1420, 1.4), (2080, 2080, 1.9)):
                x, y = sx * ax, sy * ay
                z = ground(x, y)
                entity("misc_model", x, y, z, model="models/mapobjects/GR_trees/nadelbaum.md3",
                       modelscale=str(scale), angle=str((ax + ay) % 360))
                box((x - 14, y - 14, z - 8), (x + 14, y + 14, z + 76), TEX + "wood")
            # Boulders: on the creek banks (never in a ford), in the pit and
            # as landmarks next to the roads.
            seed = 0
            for ax, ay, radius, tall, bank in ((560, 215, 44, 46, 1), (1330, 205, 52, 58, 1),
                                                (2180, 225, 40, 40, 1), (1460, 290, 34, 34, 1),
                                                (2330, 300, 46, 52, 1), (520, 1000, 60, 66, 0),
                                                (1000, 520, 54, 58, 0), (240, 520, 40, 40, 0),
                                                (2000, 1150, 50, 54, 0)):
                seed += 1
                x = sx * ax
                y = round(yc(x)) + sy * ay if bank else sy * ay
                # same seed in all four quadrants: mirrored rocks would need
                # mirrored planes, equal size and position is what matters
                boulder(x, y, radius, tall, seed * 1.7)
    # Central pickups: the island and ford items sit in or next to the creek.
    for x, y in ((0, 640), (0, -640), (300, yc(300)), (-300, yc(-300))):
        entity("rally_item_turbo", x, round(y), count="2000", wait="10")
    for x, y in ((0, BASE), (0, -BASE), (BASE, 0), (-BASE, 0)):
        entity("item_armor_yellow", x, y)
    for x, y in ((0, 1040), (0, -1040), (ISLAND_X, yc(ISLAND_X)), (-ISLAND_X, yc(-ISLAND_X))):
        entity("weapon_shotgun", x, round(y))
    # A visible timber boundary explains the solid sky hull at the map edges.
    # It leaves a gap where the creek leaves the field.
    edge = EXTENT - 24
    for fixed_axis in (0, 1):
        for sign in (-1, 1):
            for along in range(-EXTENT + 128, EXTENT - 127, 256):
                x, y = (sign * edge, along) if fixed_axis == 0 else (along, sign * edge)
                if fixed_axis == 0 and abs(y - yc(x)) < 330:
                    continue
                z = ground(x, y)
                box((x - 8, y - 8, z - 16), (x + 8, y + 8, z + 96), TEX + "wood")
            for along in range(-EXTENT, EXTENT, 128):
                x, y = (sign * edge, along + 64) if fixed_axis == 0 else (along + 64, sign * edge)
                if fixed_axis == 0 and abs(y - yc(x)) < 384:
                    continue
                z = ground(x, y) + 64
                dx, dy = (4, 64) if fixed_axis == 0 else (64, 4)
                box((x - dx, y - dy, z), (x + dx, y + dy, z + 12), TEX + "wood")
    # Woods on the surrounding hills.
    for sx in (-1, 1):
        for sy in (-1, 1):
            for ax, ay, scale in ((3000, 1500, 2.6), (3300, 2300, 2.9), (2950, 2950, 2.7),
                                  (1500, 3000, 2.6), (2300, 3300, 2.9), (600, 3100, 2.8),
                                  (3600, 3500, 3.2), (3900, 1900, 3.0), (1900, 3900, 3.0),
                                  (3400, 1150, 2.5), (1100, 3500, 3.1), (4300, 2700, 3.3),
                                  (2700, 4300, 3.3), (200, 4200, 3.4), (3150, 820, 2.4)):
                x, y = sx * ax, sy * ay
                entity("misc_model", x, y, background_height(x, y) - 8,
                       model="models/mapobjects/GR_trees/nadelbaum.md3", modelscale=str(scale),
                       angle=str((ax * 7 + ay) % 360))
    # A denser forest band on the near hills (backdrop only, not symmetric).
    for gy in range(-21, 22):
        for gx in range(-21, 22):
            h = math.sin(gx * 127.1 + gy * 311.7) * 43758.5453
            h1, h2 = h - math.floor(h), (h * 7.31) - math.floor(h * 7.31)
            x, y = gx * 330 + (h1 - 0.5) * 240, gy * 330 + (h2 - 0.5) * 240
            radius = max(abs(x), abs(y))
            if not EXTENT + 260 < radius < EXTENT + 3200:
                continue
            bx, by = x * EXTENT / radius, y * EXTENT / radius
            if abs(bx) >= abs(by) and abs(by - yc(bx)) < 950:
                continue                      # keep the creek valley open
            if math.cos(x / 610) * math.cos(y / 530) + h1 * 0.9 < 0.15:
                continue                      # clearings
            entity("misc_model", round(x), round(y), round(background_height(x, y) - 10),
                   model="models/mapobjects/GR_trees/nadelbaum.md3",
                   modelscale=f"{2.2 + 1.4 * h2:.2f}", angle=str(int(h1 * 360)))
    entity("info_player_intermission", 0, -2450, 1300, angles="24 90 0")


# ---------------------------------------------------------------- checks

def validate():
    gradients, kinks = [], []
    slopes = {}
    for y in range(-EXTENT, EXTENT, STEP):
        for x in range(-EXTENT, EXTENT, STEP):
            a, b, c, d = GRID[x, y], GRID[x + STEP, y], GRID[x + STEP, y + STEP], GRID[x, y + STEP]
            if DIAG[x, y] == "ac":
                g1, g2 = ((b - a) / STEP, (c - b) / STEP), ((c - d) / STEP, (d - a) / STEP)
            else:
                g1, g2 = ((b - a) / STEP, (d - a) / STEP), ((c - d) / STEP, (c - b) / STEP)
            gradients += [math.hypot(*g1), math.hypot(*g2)]
            slopes[x, y] = (g1, g2)
    # Change of slope between neighbouring triangles on the roads ("bumps").
    for (x, y), (g1, g2) in slopes.items():
        if road(x + STEP / 2, y + STEP / 2) > 0.5 and not in_plaza(x + STEP / 2, y + STEP / 2):
            kinks.append(math.hypot(g1[0] - g2[0], g1[1] - g2[1]))
            for nb in ((x + STEP, y), (x, y + STEP)):
                if nb in slopes:
                    for h in slopes[nb]:
                        kinks.append(math.hypot(g2[0] - h[0], g2[1] - h[1]))
    assert max(gradients) < 0.65, max(gradients)
    # point symmetry everywhere; mirror symmetry away from the creek valley
    assert all(GRID[x, y] == GRID[-x, -y] and DIAG.get((x, y), "ac") == DIAG.get((-x - STEP, -y - STEP), "ac")
               for x, y in GRID if (-x - STEP, -y - STEP) in DIAG or (x, y) not in DIAG)
    assert all(MAT[x, y] == MAT[-x, -y] for x, y in MAT)
    assert all(abs(GRID[x, y] - GRID[-x, y]) < 0.3 and abs(GRID[x, y] - GRID[x, -y]) < 0.3
               for x, y in GRID if abs(y) > 1500)
    assert all(lo[0] >= -EXTENT and lo[1] >= -EXTENT and hi[0] <= EXTENT and hi[1] <= EXTENT
               for lo, hi in SOLIDS)
    assert all(GRID[x + dx, y + dy] == PLAZA_Z for _, x, y in BASES
               for dx in range(-PLAZA, PLAZA + 1, STEP) for dy in range(-PLAZA, PLAZA + 1, STEP))
    # Water only shows inside the creek channel.
    for (x, y), z in GRID.items():
        if z < WL - 0.25:
            assert abs(y - yc(x)) < creek(x)[0] * 1.1, ("dry ground below the water line", x, y, z)
    fords = {f"water_depth_x{x}": WL - ground(x, yc(x)) for x in (0, BASE, ISLAND_X, 1400)}
    assert fords["water_depth_x0"] <= 8 and fords[f"water_depth_x{BASE}"] <= 8, fords
    # Check actual car-sized footprints against scenery and uneven collision.
    starts = [e for e in ENTITIES if e["classname"].endswith(("player", "spawn"))
              or e["classname"] == "info_player_deathmatch"]
    for e in starts:
        x, y, z = map(float, e["origin"].split())
        for lo, hi in SOLIDS:
            assert not (x + 55 > lo[0] and x - 55 < hi[0] and y + 30 > lo[1]
                        and y - 30 < hi[1] and z + 24 > lo[2] and z - 22 < hi[2]), e
        assert min(z - 22 - ground(x + dx, y + dy) for dx in (-50, 50) for dy in (-25, 25)) >= 0, e
    # Pickups and flags keep clear of all scenery.
    for e in ENTITIES:
        if e["classname"].startswith(("weapon_", "ammo_", "item_", "rally_item_", "team_CTF_")) \
                and not e["classname"].endswith(("player", "spawn")):
            x, y, z = map(float, e["origin"].split())
            for lo, hi in SOLIDS:
                assert not (x + 24 > lo[0] and x - 24 < hi[0] and y + 24 > lo[1] and y - 24 < hi[1]
                            and z + 24 > lo[2] and z - 64 < hi[2]), ("pickup inside scenery", e, lo, hi)
    # No scenery on the roads or in the creek.
    for lo, hi in SOLIDS:
        cx, cy = (lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2
        if not in_plaza(cx, cy) and max(abs(cx), abs(cy)) < EXTENT - 64 and \
                min(abs(cx - bx) + abs(cy - by) for _, bx, by in BASES) > 900:
            assert road(cx, cy) < 0.2, ("scenery on a road", lo, hi)
    for x, y, r in ROCKS:
        width, _, ford, island = creek(x)
        assert abs(y - yc(x)) > width * 1.3 or (ford < 0.3 and island < 0.3), ("boulder in a ford", x, y)
    for e in ENTITIES:
        if e["classname"] == "misc_model":
            x, y, _ = map(float, e["origin"].split())
            if max(abs(x), abs(y)) < EXTENT:
                assert road(x, y) < 0.2 and creek(x)[0] * 1.3 < abs(y - yc(x)), ("tree on a road", e)
    counts = {}
    for m in MAT.values():
        counts[LAYERS[m]] = counts.get(LAYERS[m], 0) + 1
    return {"map": NAME, "version": VERSION, "terrain_units": [2 * EXTENT, 2 * EXTENT],
            "collision_grid_units": STEP, "terrain_triangles": 2 * CELLS * CELLS,
            "background_triangles": 2 * 4 * CELLS * (len(RINGS) - 1),
            "height_min": min(GRID.values()), "height_max": max(GRID.values()),
            "max_slope_degrees": round(math.degrees(math.atan(max(gradients))), 2),
            "road_max_slope_change": round(max(kinks), 3),
            "road_mean_slope_change": round(sum(kinks) / len(kinks), 4),
            "water_level": WL, **{k: round(v, 2) for k, v in fords.items()},
            "ground_layer_vertices": counts,
            "team_initial_starts": 8, "team_respawns": 16, "flags": 4,
            "boulders": len(ROCKS),
            "start_car_clearance_checks": len(starts),
            "brushes": len(BRUSHES) + sum(len(v) for v in GROUPS.values()),
            "status": f"V{VERSION}; travel times and match balance require driving tests"}


def exports(report):
    from PIL import Image, ImageDraw, ImageFont
    out = ROOT / "docs" / "maps" / "crossroads"
    out.mkdir(parents=True, exist_ok=True)
    hm = Image.new("I", (513, 513))
    k = 2 * EXTENT / 512
    hm.putdata([round(ground(-EXTENT + x * k, EXTENT - y * k) / 512 * 65535)
                for y in range(513) for x in range(513)])
    hm.convert("I;16").save(out / "crossroads_heightmap_16bit.png")
    size, margin = 1024, 90
    im = Image.new("RGB", (size + 2 * margin, size + 2 * margin), "#16222a")
    pix = im.load()
    colors = {L_GRASS: (104, 129, 73), L_DRY: (150, 140, 88), L_GRAVEL: (160, 153, 127),
              L_PEBBLES: (128, 124, 112), L_DIRT: (140, 113, 77), L_CONCRETE: (163, 166, 158)}
    scale = 2 * EXTENT / size

    def screen(x, y):
        return margin + int((x + EXTENT) / scale), margin + int((EXTENT - y) / scale)

    def nearest(v):
        return min(EXTENT, max(-EXTENT, round(v / STEP) * STEP))
    for v in range(size):
        y = EXTENT - v * scale
        for u in range(size):
            x = -EXTENT + u * scale
            z = ground(x, y)
            slope = (ground(min(EXTENT, x + 8), y) - ground(max(-EXTENT, x - 8), y)) / 16
            shade = max(0.65, min(1.2, 0.84 - slope * 0.36 + z / 1100))
            rgb = colors[MAT[nearest(x), nearest(y)]]
            if z < WL:
                rgb = (70, 138, 150)
            if z % 40 < 2:
                shade *= 0.84
            pix[u + margin, v + margin] = tuple(int(c * shade) for c in rgb)
    draw = ImageDraw.Draw(im)
    font = small = None
    for name in ("C:/Windows/Fonts/arial.ttf", "DejaVuSans.ttf"):
        try:
            font, small = ImageFont.truetype(name, 24), ImageFont.truetype(name, 18)
            break
        except OSError:
            pass
    if font is None:
        font = small = ImageFont.load_default()
    draw.text((margin, 24), "CROSSROADS / Q3RALLY / CTF4", font=font, fill="#f2f0df")
    draw.text((margin, size + margin + 26),
              f"{2 * EXTENT} x {2 * EXTENT} units | V{VERSION} | aus der Geometrie erzeugt, keine Spielaufnahme",
              font=small, fill="#c6d4d7")
    labels = {"red": ("HOLZFAELLERCAMP", "#ed5b4f"), "blue": ("TANKSTELLE", "#589df0"),
              "green": ("FORSTSTATION", "#52cd89"), "yellow": ("KIESWERK", "#f2ca4f")}
    for color, x, y in BASES:
        px, py = screen(x, y)
        label, col = labels[color]
        draw.ellipse((px - 17, py - 17, px + 17, py + 17), fill=col, outline="white", width=3)
        w = draw.textbbox((0, 0), label, font=small)[2]
        draw.rectangle((px - w / 2 - 8, py - 48, px + w / 2 + 8, py - 22), fill="#16222a")
        draw.text((px - w / 2, py - 46), label, font=small, fill="white")
    for e in ENTITIES:
        if e["classname"] == "misc_model":
            x, y, _ = map(float, e["origin"].split())
            if max(abs(x), abs(y)) < EXTENT:
                px, py = screen(x, y)
                draw.ellipse((px - 10, py - 10, px + 10, py + 10), fill="#284c35", outline="#a1b775")
    for x, y, r in ROCKS:
        px, py = screen(x, y)
        q = r / scale
        draw.ellipse((px - q, py - q, px + q, py + q), fill="#8b8a86", outline="#3d3c3a")
    draw.text((margin + size / 2 - 50, margin + size / 2 + 70), "KIESGRUBE", font=small, fill="#27303a")
    draw.text((margin + 12, margin + size / 2 - 64), "BACH / FURTEN", font=small, fill="#d3eef4")
    draw.text((size + margin - 24, margin + 12), "N", font=font, fill="white")
    im.save(out / "crossroads_layout.png")
    (out / "validation.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


def write_map():
    fields = {"classname": "worldspawn", "message": f"Crossroads - CTF4 V{VERSION}",
              "gravity": "1400", "weather": "none", "_ambient": "18", "_minlight": "6",
              "gridsize": "64 64 128", "_blocksize": "512 512 4096"}
    text = ["// Generated by tools/maptools/make_crossroads.py. Q3 brush format; editable in NetRadiant Custom.",
            "// The terrain func_groups use maps/q3r_crossroads_index.pcx (ground layers) at compile time.",
            "{"]
    text.extend(f'"{k}" "{v}"' for k, v in fields.items())
    text.extend(BRUSHES)
    text.append("}")
    e = EXTENT
    for group in ("grass", "dirt", "gravel", "plain"):
        keys = {"classname": "func_group", "_terrain": f"crossroads ground, {group} friction",
                "_indexmap": f"maps/{NAME}_index.pcx", "_layers": str(len(LAYERS)),
                "_shader": TEX + "ter", "min": f"{-e} {-e} {FLOOR}", "max": f"{e} {e} 512"}
        text.append("{\n" + "\n".join(f'"{k}" "{v}"' for k, v in keys.items()))
        text.extend(GROUPS.get(group, []))
        text.append("}")
    for ent in ENTITIES:
        text.append("{\n" + "\n".join(f'"{k}" "{v}"' for k, v in ent.items()) + "\n}")
    target = ROOT / "baseq3r" / "maps" / f"{NAME}.map"
    target.write_text("\n".join(text) + "\n", encoding="ascii")
    return target


def main():
    terrain()
    dressing()
    report = validate()
    target = write_map()
    index_map()
    exports(report)
    print(json.dumps(report, indent=2))
    print(f"Wrote {target}")


if __name__ == "__main__":
    main()
