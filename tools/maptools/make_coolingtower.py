#!/usr/bin/env python3
"""Generate q3r_coolingtower (Cooling Tower): Autoball inside a disused cooling tower.

A round 4 vs 4 car football pitch in the dry water basin of a cooling tower
(concept: "Cooling Tower - Autoball-Arena", Nov 2026). Mirror symmetric
(x: red west, blue east; y).

- Pitch: a circle of radius 2304. Flat up to r 1536, then a banked curve with
  a circular profile up to BANK_DEG at the wall (z 295 at 42 degrees). The
  bank has full height at the sides and runs out towards the goals: flat
  within +-20 degrees of the goal axis, full from +-50 degrees.
- Goals (1088 x 320, 320 deep) sit in flat portal walls cut into the tower
  wall (+-15 degrees around the goal axis), posts and nets as in Sundown.
- Wall: a ring of steel grating (solid, see-through) on a concrete plinth up
  to z 640, a ring beam to 704, behind it 48 concrete columns.
- Shell: a hyperboloid from r 2304 at z 704 to its waist r 1664 at z 2560,
  opening r 1721 at z 3072. It leans inwards and sends high balls back down.
- Lid: a sky brush closes the opening at z 3072. The Bullet ball collides
  with every solid world brush, so it bounces off; weather starts under sky
  surfaces and q3map2 takes the sunlight from there, so rain, snow and sun
  only reach the middle of the pitch. No autoball_reset volumes.

--test writes q3r_coolingtower_test instead: a box with straight bank
samples at 25, 35 and 42 degrees, as brushes on the north side and as
patches on the south side, two goals and a ball (stage 0 of the concept).

Writes the map, scripts/q3r_coolingtower.shader and .arena, the textures in
textures/q3r_coolingtower/ and docs/maps/coolingtower/ (layout, validation).
Run with Python 3 + numpy + Pillow.
"""
from __future__ import annotations

import json
import math
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from make_rooftop import blur, cross, noise, num, plane, polytope  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
NAME = "q3r_coolingtower"
TEX = "q3r_coolingtower/"
DETAIL = 134217728

R_FLAT = 1536           # flat basin floor
R_WALL = 2304           # inner face of the wall ring
BANK_DEG = 42.0         # bank angle at the wall (stage 0 decides; <= 45 for the bot AAS)
BANK_RINGS = 8
SEG = 96                # segments around for the bank and the wall
APRON_FLAT = 20.0       # degrees around the goal axis without bank
APRON_FULL = 50.0       # from here the bank has full height
PORTAL = 15.0           # half angle of the flat portal wall around each goal
GRATE_TOP = 640
RING_TOP = 704
WAIST_R, WAIST_Z, WAIST_C = 1664.0, 2560.0, 1938.0
TOP_Z = 3072
SHELL_T = 48
SHELL_SEG = 64
GW, GH, GD = 544, 320, 320
BALL_R = 75
SCOREBOARD_Z = 1200

BRUSHES, ENTITIES, SOLIDS = [], [], []
TEAM = {1: "blue", -1: "red"}      # east end defended by blue
TEAM_RGB = {"red": (1.0, 0.18, 0.12), "blue": (0.15, 0.45, 1.0)}

GX = R_WALL * math.cos(math.radians(PORTAL))     # goal line (flat portal face)
PY = R_WALL * math.sin(math.radians(PORTAL))     # half width of the portal


# ---------------------------------------------------------------- brush basics

def face(points, material, scale=0.5, detail=True, tex=None):
    p = " ".join("( %s )" % " ".join(num(v) for v in xyz) for xyz in points)
    xo, yo, ss, ts = tex if tex else (0, 0, scale, scale)
    return f"{p} {material} {num(xo)} {num(yo)} 0 {num(ss)} {num(ts)} {DETAIL if detail else 0} 0 0"


def brush(faces, target=None):
    (target if target is not None else BRUSHES).append("{\n" + "\n".join(faces) + "\n}")


def convex(planes, detail=True, target=None, solid=False):
    """(outward normal, point, material) planes; checked for a real volume."""
    corners = polytope([(n, p) for n, p, _ in planes])
    assert len(corners) >= 4, planes
    brush([face(plane(n, p), m, detail=detail) for n, p, m in planes], target)
    if solid:
        SOLIDS.append((tuple(min(c[i] for c in corners) for i in range(3)),
                       tuple(max(c[i] for c in corners) for i in range(3))))


NORMALS = {"bot": (0, 0, -1), "top": (0, 0, 1), "-x": (-1, 0, 0), "+x": (1, 0, 0), "-y": (0, -1, 0),
           "+y": (0, 1, 0)}


def box(lo, hi, mat, faces=None, solid=True, detail=True, target=None):
    x1, y1, z1 = (min(a, b) for a, b in zip(lo, hi))
    x2, y2, z2 = (max(a, b) for a, b in zip(lo, hi))
    assert x2 > x1 and y2 > y1 and z2 > z1, (lo, hi)
    out = []
    for key, n in NORMALS.items():
        p = (x2 if n[0] > 0 else x1, y2 if n[1] > 0 else y1, z2 if n[2] > 0 else z1)
        spec = (faces or {}).get(key, mat)
        if isinstance(spec, tuple):
            out.append(face(plane(n, p), spec[1], detail=detail, tex=spec[2]))
        else:
            out.append(face(plane(n, p), spec, detail=detail))
    brush(out, target)
    if solid:
        SOLIDS.append(((x1, y1, z1), (x2, y2, z2)))


def entity(classname, x, y, z, **keys):
    fields = {"classname": classname, "origin": f"{round(x)} {round(y)} {round(z)}", **keys}
    ENTITIES.append(fields)
    return fields


def brush_entity(classname, brushes, **keys):
    ENTITIES.append({"classname": classname, **keys, "_brushes": brushes})


def xs(sx, a, b):
    return min(sx * a, sx * b), max(sx * a, sx * b)


def unit(v):
    n = math.sqrt(sum(c * c for c in v))
    return tuple(c / n for c in v)


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def tri_normal(a, b, c):
    return cross(sub(b, a), sub(c, a))


def polar(r, a_deg, z=0.0):
    a = math.radians(a_deg)
    return (r * math.cos(a), r * math.sin(a), z)


def side_planes(a0, a1, mat):
    """The two vertical radial planes of the sector a0..a1 (degrees, a0 < a1)."""
    r0, r1 = math.radians(a0), math.radians(a1)
    return [((math.sin(r0), -math.cos(r0), 0.0), (0.0, 0.0, 0.0), mat),
            ((-math.sin(r1), math.cos(r1), 0.0), (0.0, 0.0, 0.0), mat)]


# ---------------------------------------------------------------- shape of the pitch

def goal_angle(a_deg):
    """Angle from the nearest goal axis, 0..90 degrees."""
    a = abs((a_deg + 180.0) % 360.0 - 180.0)
    return min(a, 180.0 - a)


def bank_factor(a_deg):
    t = min(1.0, max(0.0, (goal_angle(a_deg) - APRON_FLAT) / (APRON_FULL - APRON_FLAT)))
    return t * t * (3 - 2 * t)


def bank_radius(deg=BANK_DEG, run=R_WALL - R_FLAT):
    return run / math.sin(math.radians(deg))


def bank_profile(d, deg=BANK_DEG, run=R_WALL - R_FLAT):
    """Height of the circular bank profile d units out from its foot."""
    rb = bank_radius(deg, run)
    d = min(max(d, 0.0), run)
    return rb - math.sqrt(rb * rb - d * d)


BANK_H = bank_profile(R_WALL - R_FLAT)


def height(x, y):
    """Pitch surface height at (x, y) (0 on the flat floor)."""
    r = math.hypot(x, y)
    if r <= R_FLAT:
        return 0.0
    return bank_factor(math.degrees(math.atan2(y, x))) * bank_profile(r - R_FLAT)


# ---------------------------------------------------------------- the basin

def floor():
    tex = (1024.0, 1024.0, 2 * R_WALL / 2048.0, 2 * R_WALL / 2048.0)
    box((-2800, -2800, -32), (2800, 2800, 0), "common/caulk", faces={"top": ("tex", TEX + "pitch", tex)},
        solid=False)
    # ground around the tower
    g = 14000
    box((-g, -g, -96), (g, g, -32), "common/caulk", faces={"top": TEX + "ground"}, solid=False, detail=False)


def bank():
    """Triangle prisms under the bank surface, SEG x BANK_RINGS quads, two each."""
    radii = [R_FLAT + (R_WALL - R_FLAT) * i / BANK_RINGS for i in range(BANK_RINGS + 1)]
    mat = TEX + "bank"
    count = 0
    for k in range(SEG):
        a0, a1 = 360.0 * k / SEG, 360.0 * (k + 1) / SEG
        f0, f1 = bank_factor(a0), bank_factor(a1)
        if max(f0, f1) < 1e-4:
            continue
        for i in range(BANK_RINGS):
            ra, rb = radii[i], radii[i + 1]
            pts = {}
            for key, r, a, f in (("a0", ra, a0, f0), ("a1", ra, a1, f1), ("b0", rb, a0, f0), ("b1", rb, a1, f1)):
                x, y, _ = polar(r, a)
                pts[key] = (x, y, f * bank_profile(r - R_FLAT))
            for tri in ((pts["a0"], pts["b0"], pts["b1"]), (pts["a0"], pts["b1"], pts["a1"])):
                if max(p[2] for p in tri) < 1.0:
                    continue
                n = tri_normal(*tri)
                if n[2] < 0:
                    n = tuple(-c for c in n)
                planes = [(n, tri[0], mat), ((0, 0, -1), (0, 0, -16), "common/caulk")]
                for p, q in zip(tri, tri[1:] + tri[:1]):
                    e = sub(q, p)
                    side = (e[1], -e[0], 0.0)
                    c = tuple(sum(t[m] for t in tri) / 3 for m in range(2))
                    if side[0] * (c[0] - p[0]) + side[1] * (c[1] - p[1]) > 0:
                        side = tuple(-s for s in side)
                    planes.append((side, (p[0], p[1], 0.0), "common/caulk"))
                convex(planes)
                count += 1
    return count


def wall():
    """Plinth, grating and ring beam on the wall ring, columns behind it."""
    t_out = R_WALL + 48
    for k in range(SEG):
        a0, a1 = 360.0 * k / SEG, 360.0 * (k + 1) / SEG
        if goal_angle(0.5 * (a0 + a1)) < PORTAL:
            continue
        h0 = bank_factor(a0) * BANK_H
        h1 = bank_factor(a1) * BANK_H
        p0, p1 = polar(R_WALL, a0, h0), polar(R_WALL, a1, h1)
        q0 = polar(R_WALL + 96, a0, h0)
        top = tri_normal(p0, q0, p1)
        if top[2] < 0:
            top = tuple(-c for c in top)
        mid = 0.5 * (a0 + a1)
        inner_n = tuple(-c for c in polar(1.0, mid))
        chord = math.cos(math.radians(0.5 * (a1 - a0)))
        # plinth: concrete up to the bank edge (its top follows the edge)
        planes = [(top, p0, TEX + "concrete"), ((0, 0, -1), (0, 0, -32), "common/caulk"),
                  (inner_n, polar(R_WALL * chord, mid), TEX + "concrete"),
                  (polar(1.0, mid), polar(R_WALL + 96, mid), "common/caulk")]
        convex(planes + side_planes(a0, a1, "common/caulk"))
        # grating from the plinth to the ring beam
        lo_n = tuple(-c for c in top)
        # caulk, not common/nodraw: that one is nonsolid in Q3Rally and makes the whole brush passable
        planes = [((0, 0, 1), (0, 0, GRATE_TOP), "common/caulk"), (lo_n, p0, "common/caulk"),
                  (inner_n, polar(R_WALL * chord, mid), TEX + "grate"),
                  (polar(1.0, mid), polar(t_out, mid), TEX + "grate")]
        convex(planes + side_planes(a0, a1, "common/caulk"))
        # ring beam
        planes = [((0, 0, 1), (0, 0, RING_TOP), "common/caulk"), ((0, 0, -1), (0, 0, GRATE_TOP), TEX + "concrete"),
                  (inner_n, polar(R_WALL * chord, mid), TEX + "concrete"),
                  (polar(1.0, mid), polar(R_WALL + 128, mid), TEX + "concrete")]
        convex(planes + side_planes(a0, a1, "common/caulk"))
    # columns behind the grating, every 7.5 degrees outside the portals
    for k in range(48):
        a = 7.5 * k + 3.75
        if goal_angle(a) < PORTAL + 4:
            continue
        c = polar(R_WALL + 96, a)
        u, v = polar(1.0, a), polar(1.0, a + 90)
        planes = [((0, 0, 1), (0, 0, GRATE_TOP), "common/caulk"), ((0, 0, -1), (0, 0, -32), "common/caulk")]
        for n, d in ((u, 40), (tuple(-x for x in u), 40), (v, 28), (tuple(-x for x in v), 28)):
            planes.append((n, (c[0] + n[0] * d, c[1] + n[1] * d, 0.0), TEX + "column"))
        convex(planes)


def shell_radius(z):
    return WAIST_R * math.sqrt(1 + ((z - WAIST_Z) / WAIST_C) ** 2)


def shell():
    """Hyperboloid shell from the ring beam to the opening, SHELL_SEG x rings."""
    zs = [RING_TOP + (TOP_Z - RING_TOP) * i / 10 for i in range(11)]
    for k in range(SHELL_SEG):
        a0, a1 = 360.0 * k / SHELL_SEG, 360.0 * (k + 1) / SHELL_SEG
        for z0, z1 in zip(zs, zs[1:]):
            r0, r1 = shell_radius(z0), shell_radius(z1)
            A, B, C = polar(r0, a0, z0), polar(r0, a1, z0), polar(r1, a0, z1)
            n = tri_normal(A, B, C)
            mid = polar(1.0, 0.5 * (a0 + a1))
            if n[0] * mid[0] + n[1] * mid[1] > 0:       # inner face points to the axis
                n = tuple(-c for c in n)
            nu = unit(n)
            back = tuple(p - c * SHELL_T for p, c in zip(A, nu))
            mat_in = TEX + ("shell_top" if z1 >= TOP_Z else "shell")
            planes = [(n, A, mat_in), (tuple(-c for c in n), back, TEX + "shell_out"),
                      ((0, 0, 1), (0, 0, z1), TEX + "concrete" if z1 >= TOP_Z else "common/caulk"),
                      ((0, 0, -1), (0, 0, z0), "common/caulk")]
            convex(planes + side_planes(a0, a1, "common/caulk"))
    # the lid: sky, solid for the ball, sunlight and weather come from it
    box((-1900, -1900, TOP_Z), (1900, 1900, TOP_Z + 64), TEX + "sky", solid=False, detail=False)


def rod(a, b, w, mat):
    """Straight square bar from a to b, w units thick (not solid: see the shader)."""
    d = unit(sub(b, a))
    u = unit(cross(d, (0.0, 0.0, 1.0)))
    v = cross(u, d)
    planes = [(tuple(-c for c in d), a, mat), (d, b, mat)]
    for n in (u, v, tuple(-c for c in u), tuple(-c for c in v)):
        planes.append((n, tuple(p + c * w / 2 for p, c in zip(a, n)), mat))
    convex(planes)


def scoreboard_rig():
    """The scoreboard hangs on four steel ropes from brackets on the shell.
    Model bounds: +-169 across, z -99..290 around its origin."""
    steel = TEX + "steel"
    head = SCOREBOARD_Z + 290
    box((-128, -128, head), (128, 128, head + 32), steel, solid=False)
    box((-24, -24, head + 32), (24, 24, head + 72), steel, solid=False)
    anchor_z = 2400
    r_anchor = shell_radius(anchor_z) - 24
    for a in (45, 135, 225, 315):
        c = polar(112 * math.sqrt(2), a, head + 32)
        b = polar(r_anchor, a, anchor_z)
        rod(c, b, 6, steel)
        e = polar(r_anchor + 8, a, anchor_z)
        box((e[0] - 32, e[1] - 32, anchor_z - 40), (e[0] + 32, e[1] + 32, anchor_z + 24), steel, solid=False)


def portal(sx):
    """Flat portal wall with the goal at x = sx * GX."""
    team = TEAM[sx]
    con = TEX + "portal"
    net, post = f"q3r_sundown/net_{team}", f"q3r_sundown/goalpost_{team}"
    back = 2440
    for sy in (-1, 1):
        p0, p1 = sorted((sy * GW, sy * (GW + 32)))
        x0, x1 = xs(sx, GX, GX + 32)
        box((x0, p0, 0), (x1, p1, GH + 32), post)
        n0, n1 = xs(sx, GX + 32, GX + GD + 32)
        box((n0, p0, 0), (n1, p1, GH), net)
        # frame beside the goal up to the ring beam
        f0, f1 = sorted((sy * (GW + 32), sy * PY))
        b0, b1 = xs(sx, GX, back)
        box((b0, f0, -32), (b1, f1, RING_TOP + 160), con)
        # goal housing beside the net
        h0, h1 = sorted((sy * (GW + 32), sy * (GW + 96)))
        c0, c1 = xs(sx, GX + 32, GX + GD + 96)
        box((c0, h0, 0), (c1, h1, GH + 64), TEX + "concrete")
    x0, x1 = xs(sx, GX, GX + 32)
    box((x0, -GW - 32, GH), (x1, GW + 32, GH + 32), post)                    # crossbar
    r0, r1 = xs(sx, GX + 32, GX + GD + 32)
    box((r0, -GW - 32, GH), (r1, GW + 32, GH + 32), net)                      # net roof
    b0, b1 = xs(sx, GX + GD, GX + GD + 32)
    box((b0, -GW, 0), (b1, GW, GH), net)                                      # back net
    c0, c1 = xs(sx, GX + GD + 32, GX + GD + 96)
    box((c0, -GW - 96, 0), (c1, GW + 96, GH + 64), TEX + "concrete")          # housing back
    c0, c1 = xs(sx, GX + 32, GX + GD + 96)
    box((c0, -GW - 96, GH + 32), (c1, GW + 96, GH + 64), TEX + "concrete")    # housing roof
    # wall above the goal, up into the shell (no ledge under the shell)
    a0, a1 = xs(sx, GX, back)
    box((a0, -PY, GH + 32), (a1, PY, RING_TOP + 160), con,
        faces={("-x" if sx > 0 else "+x"): TEX + "portal_" + team})
    # goal volume: one ball radius behind the line, to the back net
    v0, v1 = xs(sx, GX + BALL_R, GX + GD)
    goal = []
    box((v0, -GW, 0), (v1, GW, GH), "common/trigger", solid=False, target=goal)
    brush_entity("autoball_goal", goal, team=team)


def sky_box():
    s, z0, z1 = 15000, -400, 9000
    sky = TEX + "sky"
    for lo, hi in (((-s - 64, -s, z0), (-s, s, z1)), ((s, -s, z0), (s + 64, s, z1)),
                   ((-s, -s - 64, z0), (s, -s, z1)), ((-s, s, z0), (s, s + 64, z1)),
                   ((-s, -s, z0 - 64), (s, s, z0)), ((-s, -s, z1), (s, s, z1 + 64))):
        box(lo, hi, sky, solid=False, detail=False)


# ---------------------------------------------------------------- entities

def players():
    entity("autoball_ball", 0, 0, 100)
    kick = [(-730, 640), (-730, -640), (-1550, 0), (-1950, 300)]
    spawn = [(-1950, 420), (-1950, -420), (-1650, 900), (-1650, -900)]
    for team, s in (("red", 1), ("blue", -1)):
        ang = "0" if team == "red" else "180"
        for x, y in kick:
            entity(f"team_CTF_{team}player", s * x, s * y, height(s * x, s * y) + 48, angle=ang)
        for x, y in spawn:
            entity(f"team_CTF_{team}spawn", s * x, s * y, height(s * x, s * y) + 48, angle=ang)
    for x in (-1500, 1500):
        for y in (-900, -300, 300, 900):
            entity("info_player_deathmatch", x, y, height(x, y) + 48, angle="0" if x < 0 else "180")
    # NOS: the big ones on the bank, the small ones on the centre line and the apron corners
    for a in (45, 135, 225, 315):
        x, y, _ = polar(1900, a)
        entity("rally_item_turbo", x, y, height(x, y) + 48, count="5000", wait="10")
    for x, y in ((0, 1024), (0, -1024), (-1700, 700), (1700, 700), (-1700, -700), (1700, -700)):
        entity("rally_item_turbo", x, y, height(x, y) + 48, count="1500", wait="4")
    entity("misc_model", 0, 0, SCOREBOARD_Z, model="models/mapobjects/q3r_scoreboard/scoreboard.md3", angle="270")
    entity("rally_scoreboard", 0, 0, SCOREBOARD_Z)
    entity("info_player_intermission", -1500, -1500, 1300, angles="18 45 0")


def lights():
    """Daylight comes from the sky (lid, grating); a little warm fill inside and
    the team colours in the portals."""
    for k in range(8):
        x, y, _ = polar(1750, 22.5 + 45 * k)
        entity("light", x, y, 560, light="1400", _color="1 0.92 0.8")
    for sx in (-1, 1):
        entity("light", sx * (GX + GD / 2), 0, GH - 40, light="300",
               _color=" ".join(f"{c:.2f}" for c in TEAM_RGB[TEAM[sx]]))
        entity("light", sx * (GX - 40), 0, GH + 160, light="500",
               _color=" ".join(f"{c:.2f}" for c in TEAM_RGB[TEAM[sx]]))


# ---------------------------------------------------------------- textures

def save(img, name, out, alpha=False):
    from PIL import Image
    a = (np.clip(img, 0, 1) * 255).astype(np.uint8)
    im = Image.fromarray(a, "RGBA" if alpha else "RGB")
    if name.endswith(".jpg"):
        im.save(out / name, quality=92)
    else:
        im.save(out / name)


def concrete(size, seed, base, joints=0):
    rng = np.random.default_rng(seed)
    n = base + 0.08 * noise(size, size, 4, seed) + 0.05 * noise(size, size, 32, seed + 1) \
        + rng.normal(0, 0.018, (size, size))
    # lime streaks running down
    streak = blur(np.clip(noise(size, size, 16, seed + 2) - 0.62, 0, 1)[:1].repeat(size, 0), 2) * 0.35
    n = n + streak * np.linspace(1, 0.2, size)[:, None]
    if joints:
        for r in range(0, size, size // joints):
            n[r:r + 2, :] *= 0.72
    return np.clip(n, 0, 1)


def write_textures():
    out = ROOT / "baseq3r" / "textures" / "q3r_coolingtower"
    out.mkdir(parents=True, exist_ok=True)
    c = concrete(512, 3, 0.50, joints=4)
    save(np.stack([c * 0.98, c * 0.97, c * 0.94], -1), "concrete.jpg", out)
    b = concrete(512, 11, 0.42)
    save(np.stack([b * 0.95, b * 0.95, b * 0.93], -1), "basin.jpg", out)
    g = 0.30 + 0.10 * noise(256, 256, 8, 21) + 0.05 * noise(256, 256, 64, 22)
    save(np.stack([g * 0.70, g * 0.82, g * 0.45], -1), "ground.jpg", out)

    # steel grating with alpha: 256 px = 64 units, bars every 16 px
    a = np.zeros((256, 256), np.float32)
    a[:, (np.arange(256) % 16) < 3] = 1
    a[(np.arange(256) % 32) < 4, :] = 1
    rust = 0.35 + 0.15 * noise(256, 256, 16, 31)
    rgba = np.stack([rust * 1.0, rust * 0.72, rust * 0.55, a], -1)
    save(rgba, "grate.tga", out, alpha=True)

    # pitch markings over the whole basin (2048 px = 4608 units, 2.25 units per pixel)
    W = 2048
    s = W / (2 * R_WALL)
    yy, xx = np.mgrid[0:W, 0:W].astype(np.float32)
    X = xx / s - R_WALL
    Y = R_WALL - yy / s
    R = np.hypot(X, Y)
    line = np.zeros((W, W), np.float32)
    lw = 14.0
    line[np.abs(X) < lw / 2 * (R < R_FLAT)] = 1                               # centre line
    line[np.abs(R - 640) < lw / 2] = 1                                        # centre circle
    line[R < 24] = 1                                                          # spot
    dash = (np.floor(np.degrees(np.arctan2(Y, X)) / 4) % 2) == 0
    line[(np.abs(R - R_FLAT) < lw / 2) & dash] = 0.7                          # foot of the bank
    for sx in (-1, 1):
        box_ = (np.abs(Y) < 900) & (sx * X > GX - 560) & (sx * X < GX)
        edge = box_ & ((np.abs(np.abs(Y) - 900) < lw / 2) | (np.abs(sx * X - (GX - 560)) < lw / 2))
        line[edge] = 1
        line[(np.abs(sx * X - GX + 4) < lw / 2) & (np.abs(Y) < GW + 32)] = 1  # goal line
    line = blur(line, 1)
    tint = np.zeros((W, W, 3), np.float32)
    inside = R < R_WALL - 4
    tint[(X < 0) & inside] = (0.08, 0.015, 0.01)
    tint[(X > 0) & inside] = (0.01, 0.025, 0.08)
    paint = np.array((0.95, 0.92, 0.80), np.float32)
    img = tint + line[..., None] * paint * 0.85
    img[[0, -1], :] = 0
    img[:, [0, -1]] = 0
    save(img, "pitch_lines.jpg", out)


# ---------------------------------------------------------------- shader

def lit(name, image, extra=(), stage_extra=()):
    rgb = [s for s in stage_extra if "rgbGen" in s] or ["\t\trgbGen identity"]
    other = [s for s in stage_extra if "rgbGen" not in s]
    return [f"textures/{TEX}{name}", "{", f"\tqer_editorimage {image}", *extra, "\t{", f"\t\tmap {image}",
            *other, *rgb, "\t}", "\t{", "\t\tmap $lightmap", "\t\tblendFunc GL_DST_COLOR GL_ZERO",
            "\t\trgbGen identity", "\t}", "}", ""]


def write_shader():
    T = f"textures/{TEX}"
    out = ["// Cooling Tower - Autoball in a disused cooling tower.",
           "// Written by tools/maptools/make_coolingtower.py. Compile with -custinfoparms.", ""]
    # sun high (65 degrees) from +y, across the pitch: both halves get the same light
    out += [f"{T}sky", "{", "\tqer_editorimage env/arcsky/arcsky2_ft.jpg", "\tsurfaceparm noimpact",
            "\tsurfaceparm nolightmap", "\tsurfaceparm sky", "\tq3map_globaltexture",
            "\tq3map_sun 1 0.95 0.86 220 90 65", "\tq3map_skylight 140 4",
            "\tq3gl2_sun 1 0.95 0.86 150 90 65 0.5",
            "\tskyparms env/arcsky/arcsky2 1024 -",
            "\tq3r_haze 0.78 0.76 0.72 9000\t// Q3Rally: dusty late summer haze", "}", ""]
    out += [f"{T}pitch", "{", f"\tqer_editorimage {T}pitch_lines.jpg", "\t{",
            f"\t\tmap {T}basin.jpg", "\t\ttcMod scale 9 9", "\t\trgbGen identity", "\t}", "\t{",
            "\t\tmap $lightmap", "\t\tblendFunc GL_DST_COLOR GL_ZERO", "\t\trgbGen identity", "\t}", "\t{",
            f"\t\tclampmap {T}pitch_lines.jpg", "\t\tblendFunc GL_ONE GL_ONE", "\t\trgbGen identity", "\t}", "}", ""]
    out += lit("bank", f"{T}basin.jpg", ("\tq3map_nonplanar", "\tq3map_shadeAngle 60"))
    out += lit("concrete", f"{T}concrete.jpg")
    out += lit("portal", f"{T}concrete.jpg", (), ("\t\trgbGen const ( 0.8 0.8 0.82 )",))
    out += lit("column", f"{T}concrete.jpg", ("\tq3map_lightmapSampleSize 32",), ("\t\trgbGen const ( 0.7 0.7 0.7 )",))
    out += lit("shell", f"{T}concrete.jpg", ("\tq3map_nonplanar", "\tq3map_shadeAngle 60",
                                             "\tq3map_lightmapSampleSize 32"))
    out += lit("shell_top", f"{T}concrete.jpg", ("\tq3map_nonplanar", "\tq3map_shadeAngle 60"),
               ("\t\trgbGen const ( 0.85 0.85 0.85 )",))
    out += lit("shell_out", f"{T}concrete.jpg", ("\tq3map_lightmapSampleSize 64",))
    out += lit("steel", "textures/q3r_metals/plate01.tga", ("\tsurfaceparm nonsolid", "\tsurfaceparm nomarks",
                                                          "\tsurfaceparm SURF_METAL", "\tq3map_lightmapSampleSize 8"),
               ("\t\trgbGen const ( 0.45 0.45 0.48 )",))
    out += lit("ground", f"{T}ground.jpg", ("\tq3map_lightmapSampleSize 64",), ("\t\ttcMod scale 0.25 0.25",))
    for team in ("red", "blue"):
        r, g, b = TEAM_RGB[team]
        out += [f"{T}portal_{team}", "{", f"\tqer_editorimage {T}concrete.jpg", "\t{", f"\t\tmap {T}concrete.jpg",
                "\t\trgbGen identity", "\t}", "\t{", "\t\tmap $lightmap", "\t\tblendFunc GL_DST_COLOR GL_ZERO",
                "\t\trgbGen identity", "\t}", "\t{", f"\t\tmap {T}concrete.jpg", "\t\tblendFunc GL_ONE GL_ONE",
                f"\t\trgbGen const ( {r * 0.25:.2f} {g * 0.25:.2f} {b * 0.25:.2f} )", "\t}", "}", ""]
    # grating: solid for ball and cars, see-through
    out += [f"{T}grate", "{", f"\tqer_editorimage {T}grate.tga", "\tqer_trans 0.5", "\tsurfaceparm trans",
            "\tsurfaceparm alphashadow", "\tsurfaceparm nomarks", "\tsurfaceparm SURF_METAL", "\tcull none",
            "\tnopicmip", "\t{", f"\t\tmap {T}grate.tga", "\t\talphaFunc GE128", "\t\tdepthWrite",
            "\t\trgbGen identity", "\t}", "\t{", "\t\tmap $lightmap", "\t\tblendFunc GL_DST_COLOR GL_ZERO",
            "\t\tdepthFunc equal", "\t\trgbGen identity", "\t}", "}", ""]
    # bank samples of the stage 0 test box (q3r_coolingtower_test)
    for name, rgb in (("test25", (0.55, 1.0, 0.55)), ("test35", (1.0, 0.95, 0.45)), ("test42", (1.0, 0.5, 0.45))):
        out += lit(name, f"{T}basin.jpg", ("\tcull none", "\tq3map_nonplanar", "\tq3map_shadeAngle 60"),
                   (f"\t\trgbGen const ( {rgb[0]} {rgb[1]} {rgb[2]} )",))
    path = ROOT / "baseq3r" / "scripts" / f"{NAME}.shader"
    path.write_text("\n".join(out), encoding="utf-8")


# ---------------------------------------------------------------- checks and output

def validate():
    starts = [e for e in ENTITIES if e["classname"].endswith(("player", "spawn")) or
              e["classname"] == "info_player_deathmatch"]
    for e in starts + [e for e in ENTITIES if e["classname"] == "rally_item_turbo"]:
        x, y, z = map(float, e["origin"].split())
        assert math.hypot(x, y) < R_WALL - 300 and abs(x) < GX - 150, e
        assert z >= height(x, y) + 40, e
        for lo, hi in SOLIDS:
            assert not (x + 60 > lo[0] and x - 60 < hi[0] and y + 60 > lo[1] and y - 60 < hi[1]
                        and z + 30 > lo[2] and z - 30 < hi[2]), (e, lo, hi)
    goals = [e for e in ENTITIES if e["classname"] == "autoball_goal"]
    return {"map": NAME, "pitch_diameter": 2 * R_WALL, "flat_diameter": 2 * R_FLAT,
            "bank_deg": BANK_DEG, "bank_height": round(BANK_H, 1),
            "goal_mouth": [2 * GW, GH], "goal_to_goal": round(2 * GX, 1),
            "shell": {"ring_top": RING_TOP, "waist": [WAIST_R, WAIST_Z], "opening": [round(shell_radius(TOP_Z)), TOP_Z]},
            "goals": len(goals), "brushes": len(BRUSHES), "entities": len(ENTITIES),
            "kickoff_per_team": sum(1 for e in starts if e["classname"] == "team_CTF_redplayer"),
            "spawns_per_team": sum(1 for e in starts if e["classname"] == "team_CTF_redspawn"),
            "turbo": sum(1 for e in ENTITIES if e["classname"] == "rally_item_turbo")}


def write_map(name, message, fields_extra=None):
    fields = {"classname": "worldspawn", "message": message, "_ambient": "8", "_minlight": "8",
              "gridsize": "256 256 512", "_blocksize": "0 0 0", **(fields_extra or {})}
    text = [f"// Generated by tools/maptools/make_coolingtower.py. Q3 brush format.", "{"]
    text += [f'"{k}" "{v}"' for k, v in fields.items()]
    text += BRUSHES
    text.append("}")
    for ent in ENTITIES:
        body = [f'"{k}" "{v}"' for k, v in ent.items() if k != "_brushes"]
        text.append("{\n" + "\n".join(body + ent.get("_brushes", [])) + "\n}")
    target = ROOT / "baseq3r" / "maps" / f"{name}.map"
    target.write_text("\n".join(text) + "\n", encoding="ascii")
    return target


def write_arena():
    arena = ROOT / "baseq3r" / "scripts" / f"{NAME}.arena"
    arena.write_text("\r\n".join([
        "{", "//MAP FILE NAME", f'map "{NAME}"', "", "//IN-GAME MAP NAME", 'longname "Cooling Tower"', "",
        "//AUTHOR OF THE MAP", 'author "Q3Rally Team"', "", "//GAME TYPE SUPPORTS", 'type "q3r_autoball"', "",
        "//CUSTOM BOTS", 'bots "Chloe Jim Zack Paul Tobias Josh"', "", "//PLAYER STARTS", 'starts "8"', "",
        "//LAPTIME", 'laptime ""', "", "//CHECKPOINTS", 'checkpoints ""', "", "//OBSERVER SPOTS",
        'observerspots ""', "", "//WEAPONS", 'weapons ""', "", "//POWERUPS", 'powerups "10"', "",
        "//REVERSABLE", 'reversable ""', "", "//TRACK LENGTHS", 'tracklengths ""', "", "//BOT SUPPORTS",
        'botsupport "None"', "", "//TEAMS", 'teams "2"', "}", ""]), encoding="ascii")


def overview(report):
    from PIL import Image, ImageDraw
    s = 0.16
    E = R_WALL + 700
    W = int(2 * E * s)
    im = Image.new("RGB", (W, W), (70, 78, 60))
    d = ImageDraw.Draw(im)

    def P(x, y):
        return ((x + E) * s, (E - y) * s)
    # bank shading by height
    for k in range(SEG):
        a0, a1 = 360.0 * k / SEG, 360.0 * (k + 1) / SEG
        for i in range(BANK_RINGS, -1, -1):
            r = R_FLAT + (R_WALL - R_FLAT) * i / BANK_RINGS
            h = bank_factor(0.5 * (a0 + a1)) * bank_profile(r - R_FLAT) / BANK_H
            g = int(120 - 70 * h)
            pts = [P(*polar(r, a)[:2]) for a in np.linspace(a0, a1, 4)] + [P(0, 0)]
            d.polygon(pts, fill=(g, g, g + 6))
    d.ellipse([P(-R_FLAT, R_FLAT), P(R_FLAT, -R_FLAT)], fill=(126, 124, 118))
    d.ellipse([P(-R_WALL, R_WALL), P(R_WALL, -R_WALL)], outline=(200, 190, 170), width=3)
    opening = shell_radius(TOP_Z)
    d.ellipse([P(-opening, opening), P(opening, -opening)], outline=(110, 200, 255), width=2)
    for sx in (-1, 1):
        col = (230, 50, 40) if TEAM[sx] == "red" else (40, 110, 255)
        x0, x1 = xs(sx, GX, GX + GD)
        d.rectangle([P(x0, GW), P(x1, -GW)], fill=col)
    for e in ENTITIES:
        if "origin" not in e:
            continue
        x, y, _ = map(float, e["origin"].split())
        u, v = P(x, y)
        c = e["classname"]
        if c == "rally_item_turbo":
            r = 6 if e.get("count") == "5000" else 4
            d.ellipse([u - r, v - r, u + r, v + r], fill=(250, 190, 30))
        elif c.startswith("team_CTF_") and c.endswith("player"):
            col = (255, 90, 80) if "red" in c else (90, 150, 255)
            d.polygon([(u - 6, v + 5), (u + 6, v + 5), (u, v - 7)], fill=col)
        elif c.startswith("team_CTF_") and c.endswith("spawn"):
            col = (255, 90, 80) if "red" in c else (90, 150, 255)
            d.rectangle([u - 4, v - 4, u + 4, v + 4], outline=col)
        elif c == "autoball_ball":
            d.ellipse([u - 9, v - 9, u + 9, v + 9], fill=(240, 240, 240))
    out = ROOT / "docs" / "maps" / "coolingtower"
    out.mkdir(parents=True, exist_ok=True)
    im.save(out / "coolingtower_layout.png")
    (out / "validation.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


# ---------------------------------------------------------------- stage 0: bank test box

def patch(rows, mat):
    """patchDef2 from rows of (x, y, z, s, t) control points."""
    lines = ["{", "patchDef2", "{", mat, f"( {len(rows)} {len(rows[0])} 0 0 0 )", "("]
    for row in rows:
        lines.append("( " + " ".join("( %s %s %s %s %s )" % tuple(num(v) for v in p) for p in row) + " )")
    lines += [")", "}", "}"]
    BRUSHES.append("\n".join(lines))


def test_ramp(x0, x1, sy, deg, mode, mat):
    """Straight bank from y = sy * (Y - run) (flat) to the wall at y = sy * Y."""
    Y, run = 1536, R_WALL - R_FLAT
    y_foot = sy * (Y - run)

    def yy(d):
        return y_foot + sy * d
    if mode == "brush":
        ds = [run * i / BANK_RINGS for i in range(BANK_RINGS + 1)]
        for da, db in zip(ds, ds[1:]):
            pa = (0.0, yy(da), bank_profile(da, deg))
            pb = (0.0, yy(db), bank_profile(db, deg))
            dy, dz = pb[1] - pa[1], pb[2] - pa[2]
            top = (0.0, -dz, dy) if dy > 0 else (0.0, dz, -dy)
            lo, hi = sorted((yy(da), yy(db)))
            planes = [(top, pa, mat), ((0, 0, -1), (0, 0, -16), "common/caulk"),
                      ((-1, 0, 0), (x0, 0, 0), "common/caulk"), ((1, 0, 0), (x1, 0, 0), "common/caulk"),
                      ((0, -1, 0), (0, lo, 0), "common/caulk"), ((0, 1, 0), (0, hi, 0), "common/caulk")]
            convex(planes)
    else:
        # two quadratic segments of the arc; control points where the tangents meet
        rb = bank_radius(deg)

        def arc(t):
            ang = math.radians(deg) * t
            return rb * math.sin(ang), rb * (1 - math.cos(ang))

        def ctrl(t0, t1):
            (u0, v0), (u1, v1) = arc(t0), arc(t1)
            a0, a1 = math.radians(deg) * t0, math.radians(deg) * t1
            k0, k1 = math.tan(a0), math.tan(a1)
            u = (v1 - v0 + k0 * u0 - k1 * u1) / (k0 - k1) if abs(k0 - k1) > 1e-9 else 0.5 * (u0 + u1)
            return u, v0 + k0 * (u - u0)
        prof = [arc(0), ctrl(0, 0.5), arc(0.5), ctrl(0.5, 1), arc(1)]
        rows = []
        for x in (x0, 0.5 * (x0 + x1), x1):
            rows.append([(x, yy(u), v, x / 256.0, u / 256.0) for u, v in prof])
        if sy < 0:
            rows = rows[::-1]
        patch(rows, mat)
        # fill under the patch: tangent planes stay below the (convex) arc
        for t0, t1 in ((0, 0.25), (0.25, 0.5), (0.5, 0.75), (0.75, 1.0)):
            tm = 0.5 * (t0 + t1)
            um, vm = arc(tm)
            k = math.tan(math.radians(deg) * tm)
            ua, ub = arc(t0)[0], arc(t1)[0]
            top_pt = (0.0, yy(um), vm - 2)
            top = (0.0, -sy * k, 1.0)
            lo, hi = sorted((yy(ua), yy(ub)))
            planes = [(top, top_pt, "common/caulk"), ((0, 0, -1), (0, 0, -16), "common/caulk"),
                      ((-1, 0, 0), (x0, 0, 0), "common/caulk"), ((1, 0, 0), (x1, 0, 0), "common/caulk"),
                      ((0, -1, 0), (0, lo, 0), "common/caulk"), ((0, 1, 0), (0, hi, 0), "common/caulk")]
            if vm - 2 - k * (um - ua) > -14:
                convex(planes)


def build_test():
    X, Y, H = 2048, 1536, 768
    con = TEX + "concrete"
    box((-X, -Y, -32), (X, Y, 0), "common/caulk", faces={"top": TEX + "basin"}, solid=False, detail=False)
    for lo, hi in (((-X - 64, -Y - 64, -32), (-X, Y + 64, H)), ((X, -Y - 64, -32), (X + 64, Y + 64, H)),
                   ((-X, -Y - 64, -32), (X, -Y, H)), ((-X, Y, -32), (X, Y + 64, H))):
        box(lo, hi, con, solid=False, detail=False)
    box((-X - 64, -Y - 64, H), (X + 64, Y + 64, H + 64), TEX + "sky", solid=False, detail=False)
    spans = [(-1856, -704), (-576, 576), (704, 1856)]
    for (x0, x1), deg in zip(spans, (25, 35, 42)):
        for sy, mode in ((1, "brush"), (-1, "patch")):
            test_ramp(x0, x1, sy, deg, mode, TEX + f"test{deg}")
    # pillars between and beside the ramps cap their open ends
    for xa, xb in ((-X, -1856), (-704, -576), (576, 704), (1856, X)):
        for sy in (-1, 1):
            y0, y1 = sorted((sy * (Y - 900), sy * Y))
            box((xa, y0, -16), (xb, y1, 400), con, solid=False)
    # small goals in the end walls
    for sx in (-1, 1):
        team = TEAM[sx]
        net = f"q3r_sundown/net_{team}"
        goal = []
        v0, v1 = xs(sx, X - 200, X - 20)
        box((v0, -300, 0), (v1, 300, 200), "common/trigger", solid=False, target=goal)
        brush_entity("autoball_goal", goal, team=team)
        n0, n1 = xs(sx, X - 230, X - 200)
        box((n0, -330, 0), (n1, 330, 230), net, solid=False)
    entity("autoball_ball", 0, 0, 100)
    for team, s in (("red", 1), ("blue", -1)):
        ang = "0" if team == "red" else "180"
        for y in (-300, 300):
            entity(f"team_CTF_{team}player", s * -900, y, 48, angle=ang)
            entity(f"team_CTF_{team}spawn", s * -1400, y, 48, angle=ang)
    for x in (-600, 600):
        entity("info_player_deathmatch", x, 0, 48, angle="90")
    entity("light", 0, 0, 600, light="4000")
    write_textures()
    write_shader()
    return write_map("q3r_coolingtower_test", "Cooling Tower bank test")


def main():
    if "--test" in sys.argv:
        print("Wrote", build_test())
        return
    floor()
    bank_brushes = bank()
    wall()
    shell()
    for sx in (-1, 1):
        portal(sx)
    scoreboard_rig()
    sky_box()
    players()
    lights()
    report = validate()
    report["bank_brushes"] = bank_brushes
    write_textures()
    write_shader()
    target = write_map(NAME, "Cooling Tower", {
        "weather": "rain snow", "weatherChance": "35", "weatherCycle": "120 240", "weatherStrength": "0.3 0.8",
        "weatherWind": "40 220"})
    write_arena()
    overview(report)
    print(json.dumps(report, indent=2))
    print("Wrote", target)


if __name__ == "__main__":
    main()
