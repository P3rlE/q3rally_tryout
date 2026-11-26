#!/usr/bin/env python3
"""Generate q3r_david5 "Underground Battle Arena": a car arena under the city.

Rework of the original map by |CE. The concept stays: an octagonal arena deep
underground, the crowd on stands all round, the rocket launcher for everybody
on spawn and the plasma gun to fight over. The original was an empty flat
octagon (2048 across) with every pickup in the middle, fake gates, walls of
830 units and very little light. What changed:

- Hub: a low octagonal mound in the middle with ramps all round (14 degrees,
  56 high) and the plasma gun on top. Cars drive across it and jump off it.
- Four pillars on the diagonals: cover against rockets, sight breakers, a
  post to push a car against in Derby. Each carries a floodlight; the fuel
  cans lie behind them.
- The four gates are real now: they lead into a tunnel ring under the stands
  (352 wide, 256 high) all round the arena, with health (north, south),
  armour (east, west) and rocket ammo (diagonals) in it.
- A banked strip (26 degrees, 64 high) along the foot of the arena wall, so
  cars carve along the wall instead of sticking in the corners. It tapers to
  the floor at the gates.
- The wall is 384 high (was 830): the crowd is in the driver's view. Eight
  rows of stands rise from the wall top to 768.
- Spawns face along the wall (alternating left and right) instead of all at
  the middle; four more in the tunnel ring.
- Light: floodlights under the ceiling, lights on the pillars, a light strip
  along the foot of the wall, warm light at the gates, lit tunnel.

Mirror symmetric about both axes; the arena centre is the origin, the floor
z = 0. Only stock textures (the original's plus base_light panels).
Writes the .map, scripts/q3r_david5.arena and docs/maps/david5/ (layout and
validation). Run with Python 3 + Pillow.
"""
from __future__ import annotations

import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
NAME = "q3r_david5"
DETAIL = 134217728
T225 = math.tan(math.radians(22.5))

A = 1024            # apothem of the arena wall's inner face (2048 across, as the original)
WT = 32             # wall thickness
WALL_H = 384        # arena wall height
GATE = 224          # half width of a gate
GATE_H = 256        # gate and tunnel height
TUN_IN = A + WT     # tunnel ring: inner face
TUN_OUT = 1408      # tunnel ring: outer face (352 wide)
SLAB = 32           # floor, tunnel roof and ceiling thickness
ROWS = 8            # stand rows
ROW_D = 96          # row depth
ROW_H = 48          # row rise
STAND_OUT = TUN_IN + ROWS * ROW_D   # 1824: back of the stands
BACK = STAND_OUT + 32               # outside of the back wall
CEIL = 1152         # ceiling (underside)
BANK_W, BANK_H = 128, 64            # banked strip along the arena wall
HUB_R, HUB_TOP, HUB_H = 352, 128, 56  # mound: base and top apothem, height
PILLAR_D, PILLAR_R, PILLAR_H = 600, 110, 448  # pillars: distance, half diagonal, height
SPAWN_R = 800       # arena spawns
RING_R = (TUN_IN + TUN_OUT) / 2     # tunnel ring centre line (1232)

MAT = {
    "floor": "base_floor/diamond2",
    "wall": "base_wall/c_met7_2",
    "bank": "base_trim/yellow_rustbx128",
    "trim": "base_trim/yellow_rustb_v1",
    "hub": "q3r_building/concrete02",
    "ramp": "base_wall/c_met7_2",
    "pad": "base_trim/yellow_rustbx128",
    "tunnel": "q3r_building/concrete02",
    "brick": "gothic_wall/streetbricks11",
    "ceil": "base_support/cable",
    "tread": "base_wall/c_met7_2",
    "crowd": ("q3r_people/peeprow01a", "q3r_people/peeprow02a", "q3r_people/peeprow03a", "q3r_people/peeprow04a"),
    "strip": "base_light/border11light",
    "flood": "base_light/baslt4_1_2k",
    "lamp": "base_light/baslt4_1_2k",
    "tunlight": "base_light/baslt4_1_2k",
    "tunroof": "base_wall/c_met7_2",
    "caulk": "common/caulk",
}

BRUSHES, ENTITIES, SOLIDS = [], [], []   # SOLIDS: (planes) of every solid brush, for the checks


# ---------------------------------------------------------------- brush basics

def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def plane(normal, point):
    """Three points of the plane through point, ordered for an outward normal."""
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
    xo, yo, ss, ts = tex if tex else (0, 0, 0.5, 0.5)
    return f"{p} {material} {num(xo)} {num(yo)} 0 {num(ss)} {num(ts)} {DETAIL if detail else 0} 0 0"


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
    """planes: (outward normal, point, material or (material, tex)). Returns the corners."""
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


def prism(xy, z0, z1, side, top=None, bot=None, detail=True, sides=None):
    """Upright convex prism over the polygon xy (any orientation). sides: a
    material per edge (edge i from xy[i] to xy[i+1])."""
    area = sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(xy, xy[1:] + xy[:1]))
    if area < 0:
        xy = xy[::-1]
        if sides:
            sides = sides[::-1][1:] + sides[::-1][:1]
    planes = [((0, 0, 1), (xy[0][0], xy[0][1], z1), top or side), ((0, 0, -1), (xy[0][0], xy[0][1], z0), bot or side)]
    for i, ((ax, ay), (bx, by)) in enumerate(zip(xy, xy[1:] + xy[:1])):
        planes.append(((by - ay, -(bx - ax), 0.0), (ax, ay, z0), sides[i] if sides else side))
    return convex(planes, detail=detail)


def entity(classname, x, y, z, **keys):
    fields = {"classname": classname, "origin": f"{round(x)} {round(y)} {round(z)}", **keys}
    ENTITIES.append(fields)
    return fields


# ---------------------------------------------------------------- the octagon

def side_dir(k):
    """Unit normal of side k (k * 45 degrees: 0 east, 2 north, 4 west, 6 south)."""
    a = math.radians(45 * k)
    return math.cos(a), math.sin(a)


def local(k, n, t):
    """Point at distance n along side k's normal and t along the side (counter-clockwise)."""
    cx, cy = side_dir(k)
    return cx * n - cy * t, cy * n + cx * t


def ring_piece(k, a0, a1, t0=None, t1=None):
    """Plan of side k of an octagonal ring between the apothems a0 and a1,
    optionally cut to t0..t1 along the side (a gap)."""
    def tl(a, t, sign):
        lim = a * T225
        return max(-lim, min(lim, t)) if t is not None else sign * lim
    pts = [local(k, a0, tl(a0, t0, -1)), local(k, a1, tl(a1, t0, -1)), local(k, a1, tl(a1, t1, 1)),
           local(k, a0, tl(a0, t1, 1))]
    return pts


def octagon(a):
    return [local(k, a, a * T225) for k in range(8)]


def crowd_tex(k, z_top, mat):
    """Peeprow (256 x 32) on a riser: one row of people per ROW_H, top edge aligned."""
    ts = ROW_H / 32
    ss = ROW_D * 3 / 256 * (1.0 if k % 2 == 0 else 1 / math.sqrt(2))
    return (mat, (0, (z_top / ts) % 32, ss, ts))


# ---------------------------------------------------------------- the build

def shell():
    """Floor, the space under the stands, back wall and ceiling: structural, they seal the map."""
    prism(octagon(TUN_OUT + WT), -SLAB, 0, MAT["caulk"], top=MAT["floor"], detail=False)
    for k in range(8):
        prism(ring_piece(k, TUN_OUT + WT, STAND_OUT), -SLAB, GATE_H + SLAB, MAT["caulk"], detail=False)
        prism(ring_piece(k, STAND_OUT, BACK), -SLAB, CEIL + SLAB, MAT["caulk"],
              sides=[MAT["caulk"], MAT["caulk"], MAT["caulk"], MAT["wall"]], detail=False)
    prism(octagon(BACK), CEIL, CEIL + SLAB, MAT["caulk"], bot=MAT["ceil"], detail=False)


def arena_wall():
    for k in range(8):
        if k % 2 == 0:      # a gate in the flat sides
            for t0, t1, edge in ((None, -GATE, 2), (GATE, None, 0)):
                sides = [MAT["wall"]] * 4
                sides[edge] = MAT["trim"]
                prism(ring_piece(k, A, TUN_IN, t0, t1), 0, GATE_H, MAT["wall"], sides=sides)
            prism(ring_piece(k, A, TUN_IN), GATE_H, WALL_H, MAT["wall"], bot=MAT["trim"])
        else:
            prism(ring_piece(k, A, TUN_IN), 0, WALL_H, MAT["wall"])
        # the light strip along the foot of the wall, above the bank
        if k % 2 == 0:
            for t0, t1 in ((None, -GATE - 16), (GATE + 16, None)):
                prism(ring_piece(k, A - 4, A, t0, t1), BANK_H + 8, BANK_H + 20, MAT["strip"])
        else:
            prism(ring_piece(k, A - 4, A), BANK_H + 8, BANK_H + 20, MAT["strip"])


def bank():
    """The banked strip: rises from A - BANK_W at the floor to BANK_H at the wall,
    tapering to the floor at the gates over BANK_W."""
    rise = BANK_H / BANK_W
    for k in range(8):
        cx, cy = side_dir(k)
        tx, ty = -cy, cx
        pieces = [(None, -GATE), (GATE, None)] if k % 2 == 0 else [(None, None)]
        for t0, t1 in pieces:
            planes = [((0, 0, -1), (0, 0, 0), MAT["caulk"]),
                      ((cx, cy, 0), (cx * A, cy * A, 0), MAT["caulk"]),
                      ((-cx * rise, -cy * rise, 1), (cx * (A - BANK_W), cy * (A - BANK_W), 0), MAT["bank"])]
            # the radial lines at the octagon's corners
            for sgn in (-1, 1):
                ang = math.radians(45 * k + sgn * 22.5)
                nrm = (-math.sin(ang) * sgn, math.cos(ang) * sgn, 0)
                planes.append((nrm, (0, 0, 0), MAT["caulk"]))
            if t1 is not None:      # tapers down towards the gate at t = -GATE
                planes.append(((tx * rise, ty * rise, 1), (tx * t1, ty * t1, 0), MAT["bank"]))
            if t0 is not None:
                planes.append(((-tx * rise, -ty * rise, 1), (tx * t0, ty * t0, 0), MAT["bank"]))
            convex(planes)


def tunnel():
    """The ring under the stands: outer wall, roof (the stands stand on it), lamps."""
    for k in range(8):
        prism(ring_piece(k, TUN_OUT, TUN_OUT + WT), 0, GATE_H, MAT["brick"])
        prism(ring_piece(k, TUN_IN, TUN_OUT + WT), GATE_H, GATE_H + SLAB, MAT["caulk"], bot=MAT["tunroof"])
        # two lamps in the roof of every side
        for t in (-200, 200):
            c = local(k, RING_R, t * (RING_R / A))
            cx, cy = side_dir(k)
            tx, ty = -cy, cx
            pts = [(c[0] + cx * dn + tx * dt, c[1] + cy * dn + ty * dt) for dn, dt in ((-48, -16), (48, -16), (48, 16), (-48, 16))]
            prism(pts, GATE_H - 8, GATE_H, MAT["wall"], bot=MAT["tunlight"])


def stands():
    for k in range(8):
        for r in range(ROWS):
            a0, a1 = TUN_IN + r * ROW_D, TUN_IN + (r + 1) * ROW_D
            top = WALL_H + (r + 1) * ROW_H
            crowd = MAT["crowd"][(r + k) % 4]
            # ring_piece's first edge runs from (a0, t0) to (a1, t0): radial; the last edge (a0) faces the arena
            sides = [MAT["caulk"], MAT["caulk"], MAT["caulk"], crowd_tex(k, top, crowd)]
            prism(ring_piece(k, a0, a1), GATE_H + SLAB, top, MAT["caulk"], top=MAT["tread"], sides=sides)
        # a rail on the front edge of the wall
        prism(ring_piece(k, A, A + 8), WALL_H, WALL_H + 40, MAT["trim"])


def hub():
    """The mound: an octagonal frustum, ramps all round."""
    run = (HUB_R - HUB_TOP) / HUB_H
    planes = [((0, 0, -1), (0, 0, 0), MAT["caulk"]), ((0, 0, 1), (0, 0, HUB_H), MAT["pad"])]
    for k in range(8):
        cx, cy = side_dir(k)
        planes.append(((cx, cy, run), (cx * HUB_R, cy * HUB_R, 0), MAT["ramp"]))
    convex(planes)


def pillars():
    for k in (1, 3, 5, 7):
        cx, cy = side_dir(k)
        px, py = cx * PILLAR_D, cy * PILLAR_D
        diamond = [(px + PILLAR_R * math.cos(math.radians(45 * k + 90 * i)),
                    py + PILLAR_R * math.sin(math.radians(45 * k + 90 * i))) for i in range(4)]
        prism(diamond, 0, PILLAR_H, MAT["hub"])
        cap = [(px + (PILLAR_R + 56) * math.cos(math.radians(45 * k + 90 * i)),
                py + (PILLAR_R + 56) * math.sin(math.radians(45 * k + 90 * i))) for i in range(4)]
        prism(cap, PILLAR_H, PILLAR_H + 32, MAT["wall"], bot=MAT["lamp"])
        # a band of hazard trim at bumper height
        band = [(px + (PILLAR_R + 4) * math.cos(math.radians(45 * k + 90 * i)),
                 py + (PILLAR_R + 4) * math.sin(math.radians(45 * k + 90 * i))) for i in range(4)]
        prism(band, 0, 48, MAT["bank"])


def floodlights():
    """Light panels hanging under the ceiling: one over the hub, four over the ring of the floor."""
    for x, y, h in ((0, 0, 160), (520, 0, 112), (-520, 0, 112), (0, 520, 112), (0, -520, 112)):
        prism([(x - h, y - 48), (x + h, y - 48), (x + h, y + 48), (x - h, y + 48)], CEIL - 160, CEIL - 144,
              MAT["wall"], bot=MAT["flood"])
        prism([(x - 4, y - 4), (x + 4, y - 4), (x + 4, y + 4), (x - 4, y + 4)], CEIL - 144, CEIL, MAT["wall"])


def players():
    """Spawns along the wall, alternating left and right; four in the tunnel ring."""
    for k in range(8):
        x, y = local(k, SPAWN_R, 0)
        yaw = 45 * k + (90 if k % 2 == 0 else -90)
        entity("info_player_deathmatch", x, y, 24, angle=str(yaw % 360), target="target_give1")
    for k in (1, 3, 5, 7):
        x, y = local(k, RING_R, -220)
        entity("info_player_deathmatch", x, y, 24, angle=str((45 * k + 90) % 360), target="target_give1")
    x, y = local(5, 1700, 0)
    entity("info_player_intermission", x, y, 1000, angles="30 45 0")


def items():
    # the rocket launcher for everybody on spawn (as the original), out of reach under the ceiling
    entity("weapon_rocketlauncher", 0, 0, CEIL - 260, wait="10", count="20", targetname="weapon_rocketlauncher1")
    entity("target_give", 0, 0, CEIL - 240, targetname="target_give1", target="weapon_rocketlauncher1")
    entity("weapon_plasmagun", 0, 0, HUB_H + 16, wait="10")
    for k in (1, 3, 5, 7):
        x, y = local(k, PILLAR_D + 190, 0)
        entity("item_fuelcan", x, y, 16)
        x, y = local(k, RING_R, 220)
        entity("ammo_rockets", x, y, 16)
    for k in (2, 6):
        x, y = local(k, RING_R, 280)
        entity("item_health", x, y, 16)
    for k in (0, 4):
        x, y = local(k, RING_R, 280)
        entity("item_armor_combat", x, y, 16)


def lights():
    # the floor: above the hub and around it
    entity("light", 0, 0, 640, light="1000", _color="1 0.95 0.85")
    for k in range(8):
        x, y = local(k, 620, 0)
        entity("light", x, y, 700, light="520", _color="1 0.95 0.85")
    # the pillars' floodlights
    for k in (1, 3, 5, 7):
        x, y = local(k, PILLAR_D, 0)
        entity("light", x, y, PILLAR_H - 40, light="420", _color="1 0.9 0.7")
    # warm light at the gates, inside and out
    for k in (0, 2, 4, 6):
        for n in (A - 96, RING_R):
            x, y = local(k, n, 0)
            entity("light", x, y, 200, light="450", _color="1 0.55 0.2")
    # the tunnel between the gates and the crowd under the ceiling
    for k in range(8):
        for t in (-260, 260):
            x, y = local(k, RING_R, t)
            entity("light", x, y, 96, light="160", _color="0.9 0.95 1")
        x, y = local(k, TUN_IN + ROWS * ROW_D / 2, 0)
        entity("light", x, y, 900, light="600", _color="1 0.9 0.8")
    # the crowd (as the original)
    for k in range(8):
        x, y = local(k, A + 300, 0)
        entity("target_speaker", x, y, 900, noise="/sound/world/crowds.wav", spawnflags="5")


# ---------------------------------------------------------------- checks

def clear(x, y, z, r=56, h=24):
    """No solid brush within a car's box around the point."""
    for planes in SOLIDS:
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


def validate():
    starts = [e for e in ENTITIES if e["classname"] == "info_player_deathmatch"]
    pickups = [e for e in ENTITIES if e["classname"].startswith(("item_", "ammo_")) or e["classname"] == "weapon_plasmagun"]
    for e in starts + pickups:
        x, y, z = map(float, e["origin"].split())
        on = HUB_H if e["classname"] == "weapon_plasmagun" else 0
        assert clear(x, y, on + 30, r=40 if e in pickups else 56, h=20), e
    # the gates: a car (64 wide) passes straight through the middle of each gate into the ring
    gate_clear = all(clear(*local(k, n, t), 40, r=36, h=20) for k in (0, 2, 4, 6) for n in range(A - 40, int(TUN_OUT) - 40, 16)
                     for t in (-GATE + 48, 0, GATE - 48))
    # all round the ring
    ring_clear = all(clear(*local(k, RING_R, t), 40, r=36, h=20) for k in range(8)
                     for t in range(-int(RING_R * T225) + 40, int(RING_R * T225) - 39, 32))
    assert gate_clear and ring_clear
    return {
        "map": NAME, "arena_across": 2 * A, "wall_height": WALL_H, "gates": 4, "gate_width": 2 * GATE,
        "gate_height": GATE_H, "tunnel_width": TUN_OUT - TUN_IN, "tunnel_height": GATE_H,
        "bank": {"width": BANK_W, "height": BANK_H, "degrees": round(math.degrees(math.atan(BANK_H / BANK_W)), 1)},
        "hub": {"base_apothem": HUB_R, "top_apothem": HUB_TOP, "height": HUB_H,
                "degrees": round(math.degrees(math.atan(HUB_H / (HUB_R - HUB_TOP))), 1)},
        "pillars": 4, "stand_rows": ROWS, "stands_top": WALL_H + ROWS * ROW_H, "ceiling": CEIL,
        "spawns_arena": 8, "spawns_tunnel": 4, "gates_clear": gate_clear, "ring_clear": ring_clear,
        "brushes": len(BRUSHES), "entities": len(ENTITIES),
        "pickups": {c: sum(1 for e in ENTITIES if e["classname"] == c) for c in
                    sorted({e["classname"] for e in pickups})},
    }


# ---------------------------------------------------------------- output

def write_map():
    fields = {"classname": "worldspawn", "message": "Underground Battle Arena", "reflectionImage": "/textures/reflect/david5.jpg", "_ambient": "6", "_minlight": "4",
              "gridsize": "128 128 256", "_blocksize": "0 0 0"}
    text = ["// Generated by tools/maptools/make_david5.py. Q3 brush format; editable in NetRadiant Custom.", "{"]
    text += [f'"{k}" "{v}"' for k, v in fields.items()]
    text += BRUSHES
    text.append("}")
    for ent in ENTITIES:
        text.append("{\n" + "\n".join(f'"{k}" "{v}"' for k, v in ent.items()) + "\n}")
    target = ROOT / "baseq3r" / "maps" / f"{NAME}.map"
    target.write_text("\n".join(text) + "\n", encoding="ascii")
    arena = ROOT / "baseq3r" / "scripts" / f"{NAME}.arena"
    arena.write_text("\r\n".join([
        "{", "//MAP FILE NAME", f'map "{NAME}"', "", "//IN-GAME MAP NAME", 'longname "Underground Battle Arena"', "",
        "//AUTHOR OF THE MAP", 'author "|CE, rework Q3Rally Team"', "", "//GAME TYPE SUPPORTS",
        'type "q3r_dm q3r_team_dm q3r_derby q3r_lcs"', "", "//CUSTOM BOTS", 'bots "Jim Bobby Sam Tobias"', "",
        "//PLAYER STARTS", 'starts "12"', "", "//LAPTIME", 'laptime ""', "", "//CHECKPOINTS", 'checkpoints ""', "",
        "//OBSERVER SPOTS", 'observerspots ""', "", "//WEAPONS", 'weapons "RL,PG"', "", "//POWERUPS", 'powerups "0"', "",
        "//REVERSABLE", 'reversable ""', "", "//TRACK LENGTHS", 'tracklengths ""', "", "//BOT SUPPORTS",
        'botsupport "Good"', "", "//TEAMS", 'teams "All"', "}", ""]), encoding="ascii")
    return target


def overview(report):
    from PIL import Image, ImageDraw
    s = 0.2
    R = BACK + 60
    W = int(2 * R * s)
    im = Image.new("RGB", (W, W), (20, 20, 24))
    d = ImageDraw.Draw(im)

    def P(x, y):
        return ((x + R) * s, (R - y) * s)
    d.polygon([P(*p) for p in octagon(BACK)], fill=(60, 52, 44))           # stands
    d.polygon([P(*p) for p in octagon(TUN_OUT)], fill=(92, 84, 70))        # tunnel ring
    d.polygon([P(*p) for p in octagon(TUN_IN)], fill=(120, 120, 130))      # wall
    d.polygon([P(*p) for p in octagon(A)], fill=(150, 120, 40))            # bank
    d.polygon([P(*p) for p in octagon(A - BANK_W)], fill=(52, 54, 60))     # floor
    for k in (0, 2, 4, 6):
        d.polygon([P(*p) for p in ring_piece(k, A - 1, TUN_IN + 1, -GATE, GATE)], fill=(92, 84, 70))
        d.polygon([P(*p) for p in ring_piece(k, A - BANK_W, A, -GATE, GATE)], fill=(52, 54, 60))
    d.polygon([P(*p) for p in octagon(HUB_R)], fill=(110, 100, 80))
    d.polygon([P(*p) for p in octagon(HUB_TOP)], fill=(160, 140, 90))
    for k in (1, 3, 5, 7):
        cx, cy = side_dir(k)
        px, py = cx * PILLAR_D, cy * PILLAR_D
        d.polygon([P(px + PILLAR_R * math.cos(math.radians(45 * k + 90 * i)),
                     py + PILLAR_R * math.sin(math.radians(45 * k + 90 * i))) for i in range(4)], fill=(170, 170, 180))
    colors = {"info_player_deathmatch": (90, 220, 120), "item_health": (240, 80, 70), "item_armor_combat": (250, 200, 40),
              "ammo_rockets": (230, 120, 60), "item_fuelcan": (240, 220, 90), "weapon_plasmagun": (90, 160, 255)}
    for e in ENTITIES:
        c = colors.get(e["classname"])
        if not c:
            continue
        x, y, _ = map(float, e["origin"].split())
        u, v = P(x, y)
        if e["classname"] == "info_player_deathmatch":
            a = math.radians(float(e["angle"]))
            d.ellipse([u - 5, v - 5, u + 5, v + 5], fill=c)
            d.line([u, v, u + 18 * math.cos(a), v - 18 * math.sin(a)], fill=c, width=3)
        else:
            d.rectangle([u - 5, v - 5, u + 5, v + 5], fill=c)
    out = ROOT / "docs" / "maps" / "david5"
    out.mkdir(parents=True, exist_ok=True)
    im.save(out / "david5_layout.png")
    (out / "validation.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


def main():
    shell()
    arena_wall()
    bank()
    tunnel()
    stands()
    hub()
    pillars()
    floodlights()
    players()
    items()
    lights()
    report = validate()
    target = write_map()
    overview(report)
    print(json.dumps(report, indent=2))
    print("Wrote", target)


if __name__ == "__main__":
    main()
