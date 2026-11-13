#!/usr/bin/env python3
"""Build the Q3Rally weapon models ("Gebaut, nicht gekauft").

Every weapon is a workshop conversion of car parts and stands on the same
roof plate. All of them share one texture atlas (models/weapons2/q3r/parts.tga,
written by atlas.py) and the shader models/weapons2/q3r/parts.

  python tools/carbuild/weapons/build_weapons.py [spotlight ...]

Writes into baseq3r/models/weapons2/<weapon>/ the files cgame loads for it:
<weapon>.md3 (body, with tag_flash and, if it spins, tag_barrel),
<weapon>_barrel.md3 and <weapon>_flash.md3 (own muzzle flashes, tinted
per weapon; the Nitro Flamer has none, its fire is the stream).

Model space: x forward (muzzle), y left, z up; the origin is the car's
tag_weapon. Sizes are game units.
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
sys.path.insert(0, str(HERE))
import md3  # noqa: E402
from prim import Mesh  # noqa: E402
import atlas  # noqa: E402

ROOT = HERE.parents[2]
OUT = ROOT / "baseq3r" / "models" / "weapons2"
PARTS = "models/weapons2/q3r/parts"
CHROME = "models/weapons2/q3r/chrome"

PLATE_Z = -0.6                    # underside of the roof plate: tag_weapon sits on the roof skin


def plate(m):
    """The roof plate every weapon stands on: a gunmetal base, the orange
    quick-release plate, four bolt heads."""
    m.box((-8.5, -4.6, PLATE_Z), (8.5, 4.6, PLATE_Z + 0.8), "gunmetal")
    m.box((-7.5, -3.8, PLATE_Z + 0.8), (7.5, 3.8, PLATE_Z + 1.4), "orange")
    for x in (-6.3, 6.3):
        for y in (-3.9, 3.9):
            m.cyl((x, y, PLATE_Z + 0.8), (x, y, PLATE_Z + 1.3), 0.45, mat="alu", segs=6)
    return PLATE_Z + 1.4


# ---------------------------------------------------------------- Spotlight MG

LAMP_Z, LAMP_R = 4.4, 3.7
LAMP_X0, LAMP_X1 = -4.6, 4.2         # back and front of the lamp pot
ROTOR_X = LAMP_X1 + 0.9


def spotlight():
    """Machinegun: a round rally spotlight in its U-bracket; where the
    reflector would be, a three-barrel rotor (machinegun_barrel.md3 on
    tag_barrel); an ammo box under the lamp feeds it by a belt."""
    d = OUT / "machinegun"
    m = Mesh(atlas.CELLS)
    m.surf("body", PARTS)
    top = plate(m)
    # ammo box with a lid and a handle
    m.box((-6.0, -2.6, top), (1.6, 2.6, top + 3.0), "gunmetal")
    m.box((-6.2, -2.8, top + 3.0), (1.8, 2.8, top + 3.4), "black")
    m.tube([(-3.6, -1.2, top + 3.4), (-3.6, -1.2, top + 4.0), (-1.0, -1.2, top + 4.0), (-1.0, -1.2, top + 3.4)],
           0.22, "alu", segs=5)
    # belt from the box into the bottom of the lamp
    for k in range(6):
        t = k / 5
        x = 1.4 + 1.6 * math.sin(t * math.pi / 2)
        z = top + 2.2 + (LAMP_Z - LAMP_R - top - 1.6) * t
        m.box((x - 0.45, -1.1, z - 0.3), (x + 0.45, 1.1, z + 0.3), "copper")
    # U-bracket: two straps up to pivot bolts at the sides of the lamp
    for s in (-1, 1):
        y = s * (LAMP_R + 0.7)
        m.box((-1.0, y - 0.35, top), (1.0, y + 0.35, LAMP_Z), "alu")
        m.cyl((0, s * (LAMP_R + 1.3), LAMP_Z), (0, s * LAMP_R * 0.95, LAMP_Z), 1.1, mat="black", segs=8)
    m.box((-1.0, -LAMP_R - 1.05, top), (1.0, LAMP_R + 1.05, top + 0.5), "alu")
    # the lamp pot: closed rounded back, black
    m.cyl((LAMP_X0, 0, LAMP_Z), (LAMP_X1, 0, LAMP_Z), LAMP_R, mat="black", segs=20, cap0=False, cap1=False)
    m.cyl((LAMP_X0 - 1.2, 0, LAMP_Z), (LAMP_X0, 0, LAMP_Z), LAMP_R * 0.72, LAMP_R, mat="black", segs=20,
          cap1=False)
    # front: alu bezel, orange ring, chrome reflector cone around the rotor hub
    m.cyl((LAMP_X1, 0, LAMP_Z), (LAMP_X1 + 0.7, 0, LAMP_Z), LAMP_R + 0.35, mat="alu", segs=20, cap0=False,
          cap1=False)
    m.ring((LAMP_X1 + 0.7, 0, LAMP_Z), (1, 0, 0), LAMP_R - 0.6, LAMP_R + 0.35, "orange", segs=20)
    m.surf("chrome", CHROME)
    m.cyl((LAMP_X1 + 0.6, 0, LAMP_Z), (LAMP_X1 - 1.2, 0, LAMP_Z), LAMP_R - 0.6, 1.3, mat="chrome", segs=20,
          cap0=False, cap1=False)
    # stone guard: two thin bars across the front, like on a rally lamp
    for z in (LAMP_Z - 1.6, LAMP_Z + 1.6):
        w = math.sqrt(max(0.0, LAMP_R ** 2 - (z - LAMP_Z) ** 2))
        m.cyl((LAMP_X1 + 0.9, -w, z), (LAMP_X1 + 0.9, w, z), 0.18, mat="alu", segs=4)
    m.tag("tag_barrel", (ROTOR_X, 0, LAMP_Z))
    barrel_len = 6.4
    m.tag("tag_flash", (ROTOR_X + barrel_len + 0.3, 0, LAMP_Z))
    n_body = m.save(d / "machinegun.md3", "spotlight")

    # the rotor: a hub, three barrels, a clamp ring at the muzzle
    b = Mesh(atlas.CELLS)
    b.surf("rotor", PARTS)
    b.cyl((-1.6, 0, 0), (0.9, 0, 0), 1.25, mat="gunmetal", segs=12)
    for k in range(3):
        a = 2 * math.pi * k / 3
        y, z = 0.85 * math.cos(a), 0.85 * math.sin(a)
        b.cyl((0.9, y, z), (barrel_len, y, z), 0.42, mat="steel", segs=8, cap1=False)
        b.ring((barrel_len, y, z), (1, 0, 0), 0.22, 0.42, "steel", segs=8)
        b.cyl((barrel_len - 0.01, y, z), (barrel_len - 1.6, y, z), 0.22, mat="dark", segs=8, cap0=False)
    b.cyl((barrel_len - 1.6, 0, 0), (barrel_len - 0.7, 0, 0), 1.5, mat="gunmetal", segs=12)
    b.cyl((2.6, 0, 0), (3.2, 0, 0), 1.45, mat="gunmetal", segs=12)
    n_barrel = b.save(d / "machinegun_barrel.md3", "spotlight_rotor")

    # flash: a warm star and flame at the rotor, the lamp ring lights up
    def build(f):
        flash_star(f, (0.2, 0, 0), 4.2, "yellow")
        flash_cone(f, (0, 0, 0), 10.0, 4.6, "yellow")
        f.surf("lampring", "models/weapons2/q3r/lampflash")
        back = -(barrel_len + 0.3) + (LAMP_X1 + 0.75 - ROTOR_X)
        r = LAMP_R + 0.6
        f.quad([(back, r, r), (back, -r, r), (back, -r, -r), (back, r, -r)], (1, 0, 0), "orange")
        f.cur.st = [[0, 0], [1, 0], [1, 1], [0, 1]]
    n_flash = write_flash(d, "machinegun", build)
    return {"body": n_body, "barrel": n_barrel, "flash": n_flash}


# ---------------------------------------------------------------- muzzle flashes

FLASH_COLOURS = {          # tint of each flash shader; "entity" = the player's rail colour
    "yellow": (1.0, 0.82, 0.42), "orange": (1.0, 0.55, 0.16), "ice": (0.5, 0.85, 1.0),
    "blue": (0.3, 0.72, 1.0), "green": (0.55, 1.0, 0.38), "entity": None}


def fshader(colour, kind):
    return f"models/weapons2/q3r/flash_{colour}_{kind}"


def flash_star(m, c, r, colour, turn=0.0):
    """A flash seen from the front: a square facing +x round c."""
    use(m, colour + "_star", fshader(colour, "star"))
    cu, su = math.cos(turn), math.sin(turn)
    pts = [(c[0], c[1] + r * (cu * a - su * b), c[2] + r * (su * a + cu * b)) for a, b in ((-1, -1), (1, -1), (1, 1), (-1, 1))]
    i = [m.vert(p, (1, 0, 0), "orange", uv) for p, uv in zip(pts, ((0, 1), (1, 1), (1, 0), (0, 0)))]
    m.tri(i[0], i[1], i[2]); m.tri(i[0], i[2], i[3])


def flash_cone(m, c, length, width, colour):
    """A flash seen from the side: two crossed quads from c forward (+x)."""
    use(m, colour + "_cone", fshader(colour, "cone"))
    h = width / 2
    for n, off in (((0, 0, 1), (0, h, 0)), ((0, 1, 0), (0, 0, h))):
        pts = [(c[0], c[1] - off[1], c[2] - off[2]), (c[0] + length, c[1] - off[1], c[2] - off[2]),
               (c[0] + length, c[1] + off[1], c[2] + off[2]), (c[0], c[1] + off[1], c[2] + off[2])]
        i = [m.vert(p, n, "orange", uv) for p, uv in zip(pts, ((0, 1), (1, 1), (1, 0), (0, 0)))]
        m.tri(i[0], i[1], i[2]); m.tri(i[0], i[2], i[3])


def flash_ring(m, c, r, colour):
    use(m, colour + "_ring", fshader(colour, "ring"))
    pts = [(c[0], c[1] - r, c[2] - r), (c[0], c[1] + r, c[2] - r), (c[0], c[1] + r, c[2] + r), (c[0], c[1] - r, c[2] + r)]
    i = [m.vert(p, (1, 0, 0), "orange", uv) for p, uv in zip(pts, ((0, 1), (1, 1), (1, 0), (0, 0)))]
    m.tri(i[0], i[1], i[2]); m.tri(i[0], i[2], i[3])


def write_flash(d, name, build):
    """<name>_flash.md3 round tag_flash (x forward); the old LOD of the
    flash goes, or the engine would show it at a distance."""
    f = Mesh(atlas.CELLS)
    build(f)
    for lod in (d / f"{name}_flash_1.md3", d / f"{name}_flash_2.md3"):
        if lod.exists():
            lod.unlink()
    return f.save(d / f"{name}_flash.md3", name + "_flash")


# ---------------------------------------------------------------- helpers

GLOW = "models/weapons2/q3r/glow"
SPARK = "models/weapons2/q3r/spark"
PILOT = "models/weapons2/q3r/pilot"
CHAIN = "models/weapons2/q3r/chain"


def use(m, name, shader):
    """Switch to surface `name`, creating it on first use."""
    for s in m.surfs:
        if s.name == name:
            m.cur = s
            return s
    return m.surf(name, shader)


def ellipse_x(x, yc, zc, ry, rz, n=16, phase=0.0):
    """A ring of points round the x axis (counter-clockwise seen from +x)."""
    return [(x, yc + ry * math.cos(2 * math.pi * k / n + phase), zc + rz * math.sin(2 * math.pi * k / n + phase))
            for k in range(n)]


def rrect_x(x, yc, zc, hy, hz, r, nc=3):
    """A rounded rectangle round the x axis (counter-clockwise seen from +x)."""
    r = min(r, hy, hz)
    pts = []
    for q, (cy, cz) in enumerate(((hy - r, hz - r), (-hy + r, hz - r), (-hy + r, -hz + r), (hy - r, -hz + r))):
        for k in range(nc + 1):
            a = math.pi / 2 * (q + k / nc)
            pts.append((x, yc + cy + r * math.cos(a), zc + cz + r * math.sin(a)))
    return pts


def flat_ring(m, outer, inner, n, mat):
    """Flat band between two rings of the same point count, facing n."""
    for k in range(len(outer)):
        k1 = (k + 1) % len(outer)
        m.poly2([outer[k], outer[k1], inner[k1], inner[k]], n, mat)


def keep_flash(d, name):
    """The weapon keeps its old muzzle flash; nothing to write."""
    return "old" if (d / f"{name}_flash.md3").exists() else "none"


# ---------------------------------------------------------------- Harvester Saw

def harvester():
    """Gauntlet: the saw head of a forest harvester on a short hydraulic
    arm. Orange motor housing with a cooling fan at the back
    (gauntlet_barrel.md3, spins while sawing), the chain bar sticks out
    forward along the right side; the chain runs all the time."""
    d = OUT / "gauntlet"
    m = Mesh(atlas.CELLS)
    use(m, "body", PARTS)
    top = plate(m)
    # arm: pivot block on the plate, two beams forward to the head, a ram
    m.box((-6.8, -2.3, top), (-3.0, 2.3, top + 1.5), "gunmetal")
    m.cyl((-5.0, -2.7, top + 1.5), (-5.0, 2.7, top + 1.5), 0.9, mat="black", segs=10)
    HX, HZ = 2.6, 3.4                          # head pivot under the housing
    for y in (-1.7, 1.7):
        m.cyl((-5.0, y, top + 1.5), (HX, y, HZ), 0.5, mat="alu", segs=4, phase=math.pi / 4)
    m.cyl((HX, -2.4, HZ), (HX, 2.4, HZ), 0.8, mat="black", segs=10)
    m.cyl((-2.8, 0, top + 0.7), (0.6, 0, top + 1.7), 0.55, mat="steel", segs=8)
    use(m, "chrome", CHROME)
    m.cyl((0.6, 0, top + 1.7), (4.6, 0, 3.4), 0.28, mat="chrome", segs=6)
    use(m, "body", PARTS)
    m.box((4.2, -0.6, 3.0), (5.2, 0.6, 3.7), "gunmetal")
    # motor housing: orange, rounded
    hz0, hz1 = 3.6, 8.0
    zc, hy, hh = (hz0 + hz1) / 2, 2.6, (hz1 - hz0) / 2
    rings = [[(x, y, z) for (x, y, z) in rrect_x(x0, 0, zc, hy * s, hh * s, 1.0)]
             for x0, s in ((0.0, 0.86), (0.5, 1.0), (7.0, 1.0), (7.6, 0.84))]
    m.loft(rings, "orange", cap_mat="orange")
    m.box((1.5, -2.65, zc + 0.6), (6.0, 2.65, zc + 1.1), "black")          # stripe round the housing
    for x in (2.0, 5.5):                                                    # lifting eyes
        m.cyl((x, 0, hz1 - 0.1), (x, 0, hz1 + 0.5), 0.35, mat="alu", segs=6)
    # fan shroud at the back; the fan itself is the barrel
    m.cyl((-1.1, 0, zc), (0.05, 0, zc), 1.95, mat="alu", segs=16, cap0=False, cap1=False)
    m.ring((-1.1, 0, zc), (-1, 0, 0), 1.75, 1.95, "alu", segs=16)
    m.poly([(0.02, 1.8 * math.cos(-2 * math.pi * k / 12), zc + 1.8 * math.sin(-2 * math.pi * k / 12))
            for k in range(12)], (-1, 0, 0), "dark")
    m.tag("tag_barrel", (-0.6, 0, zc))
    # sprocket cover on the right side, the bar comes out of it
    YB, BZ = -3.35, zc - 0.4
    m.box((3.4, -3.95, BZ - 1.9), (7.6, -2.6, BZ + 1.9), "gunmetal")
    for x, z in ((4.4, BZ + 1.0), (6.6, BZ - 1.0)):
        m.cyl((x, -3.95, z), (x, -4.25, z), 0.4, mat="alu", segs=6)
    # chain bar
    X0, X1, R0, R1 = 6.0, 21.5, 1.5, 0.95
    outline = [(X0, BZ - R0), (X1, BZ - R1)]
    for k in range(1, 8):
        a = -math.pi / 2 + math.pi * k / 8
        outline.append((X1 + R1 * math.cos(a), BZ + R1 * math.sin(a)))
    outline += [(X1, BZ + R1), (X0, BZ + R0)]
    for y, n in ((YB + 0.2, (0, 1, 0)), (YB - 0.2, (0, -1, 0))):
        m.poly2([(x, y, z) for x, z in outline], n, "steel")
    # the chain: a band round the bar edge, scrolling texture
    use(m, "chain", CHAIN)
    path = outline[:]                           # bottom edge, nose, top edge
    off = []
    for k, (x, z) in enumerate(path):
        a = path[max(k - 1, 0)]
        b = path[min(k + 1, len(path) - 1)]
        tx, tz = b[0] - a[0], b[1] - a[1]
        ln = math.hypot(tx, tz) or 1
        nx, nz = tz / ln, -tx / ln              # outward (the outline runs counter-clockwise in x/z)
        off.append((x + 0.55 * nx, z + 0.55 * nz, nx, nz))
    s = 0.0
    prev = None
    for k, (x, z, nx, nz) in enumerate(off):
        if prev:
            s += math.hypot(x - prev[0], z - prev[1]) / 3.2
        prev = (x, z)
        off[k] = (x, z, nx, nz, s)
    cur = m.cur
    ia = [m.vert((x, YB - 0.45, z), (nx, 0, nz), "orange", (s, 0.3)) for x, z, nx, nz, s in off]
    ib = [m.vert((x, YB + 0.45, z), (nx, 0, nz), "orange", (s, 0.75)) for x, z, nx, nz, s in off]
    for k in range(len(off) - 1):
        m.tri(ia[k], ib[k + 1], ia[k + 1])
        m.tri(ia[k], ib[k], ib[k + 1])
    for y, sgn in ((YB + 0.45, 1), (YB - 0.45, -1)):      # the side plates of the chain
        for k in range(len(off) - 1):
            p0, p1 = path[k], path[k + 1]
            q0, q1 = off[k], off[k + 1]
            quad = [(p0[0], y, p0[1]), (p1[0], y, p1[1]), (q1[0], y, q1[1]), (q0[0], y, q0[1])]
            i = [m.vert(p, (0, sgn, 0), "orange", (q[4], t)) for p, q, t in
                 zip(quad, (q0, q1, q1, q0), (0.95, 0.95, 0.05, 0.05))]
            if sgn > 0:
                m.tri(i[0], i[2], i[1]); m.tri(i[0], i[3], i[2])
            else:
                m.tri(i[0], i[1], i[2]); m.tri(i[0], i[2], i[3])
    assert m.cur is cur
    use(m, "body", PARTS)
    m.tag("tag_flash", (X1 + R1 + 0.4, YB, BZ))
    n_body = m.save(d / "gauntlet.md3", "harvester")

    # the cooling fan: hub and six pitched blades
    b = Mesh(atlas.CELLS)
    b.surf("fan", PARTS)
    b.cyl((-0.5, 0, 0), (0.3, 0, 0), 0.55, mat="gunmetal", segs=10)
    b.cyl((-0.7, 0, 0), (-0.5, 0, 0), 0.3, 0.55, mat="alu", segs=10, cap1=False)
    for k in range(6):
        a = 2 * math.pi * k / 6
        pts = []
        for x, r, da in ((-0.1, 0.5, -0.28), (-0.1, 1.7, -0.34), (0.3, 1.7, 0.34), (0.3, 0.5, 0.28)):
            pts.append((x - 0.5 * (da > 0), r * math.cos(a + da), r * math.sin(a + da)))
        nrm = (0.6, -math.sin(a) * 0.8, math.cos(a) * 0.8)
        b.poly2(pts, nrm, "black")
        b.poly2(pts, tuple(-c for c in nrm), "black")
    n_barrel = b.save(d / "gauntlet_barrel.md3", "harvester_fan")
    return {"body": n_body, "barrel": n_barrel, "flash": keep_flash(d, "gauntlet")}


# ---------------------------------------------------------------- Twin Shotgun

def twin_shotgun():
    """Shotgun: a sport exhaust set on the roof back to front. Oval silencer
    at the back, two slash-cut stainless tips forward, a perforated heat
    shield over the pipes."""
    d = OUT / "shotgun"
    m = Mesh(atlas.CELLS)
    use(m, "body", PARTS)
    top = plate(m)
    MZ = top + 2.7
    ry, rz = 3.3, 2.4
    for x0, x1 in ((-6.6, -5.0), (-0.5, 1.1)):                 # saddles
        m.box((x0, -2.0, top), (x1, 2.0, MZ - 1.9), "gunmetal")
    m.loft([ellipse_x(x, 0, MZ, ry * s, rz * s, 18) for x, s in
            ((-8.3, 0.84), (-7.8, 1.0), (1.6, 1.0), (2.1, 0.84))], "steel", cap_mat="gunmetal")
    for x in (-7.2, 1.0):                                       # rolled seams
        m.loft([ellipse_x(x + dx, 0, MZ, ry + 0.12, rz + 0.12, 18) for dx in (-0.3, 0.3)], "alu", cap0=False,
               cap1=False)
    for x in (-5.8, 0.3):                                       # straps
        m.loft([ellipse_x(x + dx, 0, MZ, ry + 0.08, rz + 0.08, 18) for dx in (-0.35, 0.35)], "black", cap0=False,
               cap1=False)
    PZ, PR, K = MZ + 0.35, 1.05, 0.75
    TX = 17.6                                                   # tip end at the pipe axis
    for y in (-1.55, 1.55):
        m.cyl((1.4, y, PZ), (13.8, y, PZ), PR, mat="steel", segs=12, cap0=False, cap1=False)
        use(m, "chrome", CHROME)
        n = 14
        outer = []
        for x0 in (13.6, 14.3):
            outer.append(ellipse_x(x0, y, PZ, PR + 0.25, PR + 0.25, n))
        slant = [(TX + K * (p[2] - PZ), p[1], p[2]) for p in ellipse_x(0, y, PZ, PR + 0.25, PR + 0.25, n)]
        outer.append(slant)
        m.loft(outer, "chrome", cap0=True, cap1=False)
        inner = [(TX + K * (p[2] - PZ) - 0.02, p[1], p[2]) for p in ellipse_x(0, y, PZ, PR, PR, n)]
        deep = [(TX - K * PR - 0.3, p[1], p[2]) for p in ellipse_x(0, y, PZ, PR, PR, n)]
        flat_ring(m, slant, inner, (1, 0, -K), "chrome")
        use(m, "body", PARTS)
        m.loft([inner, deep], "dark", cap0=False, cap1=True, inside=True)
        for x in (4.4, 11.6):                                   # pipe clamps
            m.cyl((x - 0.35, y, PZ), (x + 0.35, y, PZ), PR + 0.18, mat="alu", segs=12)
    # strut under the pipes
    m.box((6.6, -0.5, top), (7.6, 0.5, PZ - PR + 0.1), "alu")
    m.box((6.6, -2.6, PZ - PR - 0.5), (7.6, 2.6, PZ - PR + 0.1), "alu")
    # heat shield: perforated, with side skirts
    hz = PZ + PR + 0.45
    m.box((3.4, -3.2, hz), (12.8, 3.2, hz + 0.18), "perf")
    for y in (-3.2, 3.02):
        m.box((3.4, y, PZ - 0.5), (12.8, y + 0.18, hz), "perf")
    for x in (4.4, 11.6):
        for y in (-1.55, 1.55):
            m.box((x - 0.2, y - 0.2, PZ + PR), (x + 0.2, y + 0.2, hz), "alu")
    m.tag("tag_flash", (TX + 0.5, 0, PZ))
    n_body = m.save(d / "shotgun.md3", "twin_shotgun")

    def build(f):                                   # one flame from each tip
        for y in (-1.55, 1.55):
            flash_star(f, (0, y, 0), 3.2, "orange", turn=0.4 * y)
            flash_cone(f, (0, y, 0), 9.0, 3.8, "orange")
    return {"body": n_body, "flash": write_flash(d, "shotgun", build)}


# ---------------------------------------------------------------- Drum Launcher

def drum_launcher():
    """Grenade launcher: a drilled brake disc standing across the roof is
    the drum magazine (grenadel_barrel.md3, turns while firing); the
    caliper on top is the breech, a short wide barrel with a funnel."""
    d = OUT / "grenadel"
    m = Mesh(atlas.CELLS)
    use(m, "body", PARTS)
    top = plate(m)
    DX, DR, DT = -3.0, 3.6, 0.5
    DZ = top + DR + 0.6
    # axle stand: base, two uprights, axle
    m.box((DX - 1.8, -2.2, top), (DX + 2.8, 2.2, top + 0.7), "gunmetal")
    for x0, x1 in ((DX - 1.4, DX - 0.7), (DX + 1.6, DX + 2.3)):
        m.box((x0, -0.9, top + 0.7), (x1, 0.9, DZ + 0.4), "gunmetal")
        m.cyl((x0, 0, DZ), (x1, 0, DZ), 0.9, mat="gunmetal", segs=10, cap0=x0 < DX, cap1=x0 > DX)
    m.cyl((DX - 1.6, 0, DZ), (DX + 2.5, 0, DZ), 0.42, mat="steel", segs=8)
    # caliper over the top of the disc
    CZ0, CZ1 = DZ + DR - 1.3, DZ + DR + 0.9
    m.box((DX - 1.4, -2.5, CZ0), (DX + 1.4, 2.5, CZ1), "orange")
    m.box((DX - 1.0, -2.1, CZ1), (DX + 1.0, 2.1, CZ1 + 0.4), "orange")
    for y in (-1.6, 1.6):
        m.cyl((DX + 1.4, y, CZ0 + 0.6), (DX + 1.8, y, CZ0 + 0.6), 0.38, mat="alu", segs=6)
    # barrel and funnel
    BZ, BR = (CZ0 + CZ1) / 2 + 0.1, 1.3
    m.cyl((DX + 1.4, 0, BZ), (9.2, 0, BZ), BR, mat="steel", segs=14, cap0=False, cap1=False)
    m.cyl((9.2, 0, BZ), (11.6, 0, BZ), BR, 2.1, mat="black", segs=14, cap0=False, cap1=False)
    m.ring((11.6, 0, BZ), (1, 0, 0), 1.85, 2.1, "alu", segs=14)
    m.loft([ellipse_x(11.6, 0, BZ, 1.85, 1.85, 14), ellipse_x(9.4, 0, BZ, 1.15, 1.15, 14)], "dark",
           cap0=False, cap1=True, inside=True)
    for x in (2.0, 7.0):                                        # cooling rings
        m.cyl((x - 0.3, 0, BZ), (x + 0.3, 0, BZ), BR + 0.2, mat="gunmetal", segs=14)
    # front support
    m.box((6.6, -0.6, top), (7.4, 0.6, BZ - BR), "alu")
    m.box((6.2, -1.6, top), (7.8, 1.6, top + 0.5), "alu")
    m.tag("tag_barrel", (DX, 0, DZ))
    m.tag("tag_flash", (12.0, 0, BZ))
    n_body = m.save(d / "grenadel.md3", "drum_launcher")

    # the disc: radial UVs into the drilled-disc cell
    b = Mesh(atlas.CELLS)
    b.surf("disc", PARTS)
    cu0, cv0, size = atlas.CELLS["disc"]
    half = size / 2
    n = 24
    for x, nx in ((DT, 1), (-DT, -1)):
        c = b.vert((x, 0, 0), (nx, 0, 0), "disc", (cu0 + half, cv0 + half))
        rim = []
        for k in range(n + 1):
            a = 2 * math.pi * k / n
            rim.append(b.vert((x, DR * math.cos(a), DR * math.sin(a)), (nx, 0, 0), "disc",
                              (cu0 + half + 0.98 * half * math.cos(a), cv0 + half - 0.98 * half * math.sin(a))))
        for k in range(n):
            if nx > 0:
                b.tri(c, rim[k], rim[k + 1])
            else:
                b.tri(c, rim[k + 1], rim[k])
    b.cyl((-DT, 0, 0), (DT, 0, 0), DR, mat="steel", segs=n, cap0=False, cap1=False)
    b.cyl((DT, 0, 0), (DT + 0.7, 0, 0), 1.7, mat="alu", segs=14, cap_mat="alu")
    for k in range(5):
        a = 2 * math.pi * k / 5
        b.cyl((DT + 0.7, 1.1 * math.cos(a), 1.1 * math.sin(a)), (DT + 1.0, 1.1 * math.cos(a), 1.1 * math.sin(a)),
              0.28, mat="steel", segs=6)
    n_barrel = b.save(d / "grenadel_barrel.md3", "drum")

    def build(f):                                   # a fat puff out of the funnel
        flash_star(f, (0, 0, 0), 4.6, "orange")
        flash_cone(f, (0, 0, 0), 7.5, 6.5, "orange")
    return {"body": n_body, "barrel": n_barrel, "flash": write_flash(d, "grenadel", build)}


# ---------------------------------------------------------------- Quad Launcher

def quad_launcher():
    """Rocket launcher: four exhaust tubes in a square, held by a carbon box
    and two clamps; heat-blued tips, a perforated heat shield behind."""
    d = OUT / "rocketl"
    m = Mesh(atlas.CELLS)
    use(m, "body", PARTS)
    top = plate(m)
    R = 1.4
    Z1 = top + 2.3
    Z2 = Z1 + 3.0
    ZC = (Z1 + Z2) / 2
    X0, XT, X1 = -9.5, 13.3, 15.0
    for y in (-1.55, 1.55):
        for z in (Z1, Z2):
            m.cyl((X0, y, z), (XT, y, z), R, mat="steel", segs=12, cap0=False, cap1=False)
            m.cyl((XT, y, z), (X1, y, z), R, R - 0.1, mat="heat", segs=12, cap0=False, cap1=False)
            m.ring((X1, y, z), (1, 0, 0), R - 0.4, R - 0.1, "heat", segs=12)
            m.bore((X1, y, z), (1, 0, 0), R - 0.4, 3.0)
            m.ring((X0, y, z), (-1, 0, 0), R - 0.35, R, "steel", segs=12)
            m.bore((X0, y, z), (-1, 0, 0), R - 0.35, 2.0)
    # carbon box with rounded edges
    hy, hz = 3.35, (Z2 + R + 0.35 - (top + 0.3)) / 2
    bz = top + 0.3 + hz
    m.loft([rrect_x(x, 0, bz, hy * s, hz * s, 0.9) for x, s in ((-3.2, 0.93), (-2.8, 1.0), (7.3, 1.0), (7.7, 0.93))],
           "carbon", cap_mat="carbon")
    m.box((-2.0, -2.4, top), (6.5, 2.4, top + 0.35), "gunmetal")
    for x in (-0.6, 5.1):                                       # stencil stripes
        m.box((x, -hy - 0.02, bz + hz - 1.4), (x + 0.6, hy + 0.02, bz + hz - 0.4), "orange")
    # clamps
    ch = (Z2 + R - (Z1 - R)) / 2 + 0.22
    for x in (-7.0, 11.0):
        m.loft([rrect_x(x + dx, 0, ZC, 3.0 + 0.22, ch, 1.3) for dx in (-0.4, 0.4)], "alu", cap0=False, cap1=False)
        m.cyl((x, 0, ZC + ch), (x, 0, ZC + ch + 0.45), 0.4, mat="steel", segs=6)
    m.box((-7.4, -1.0, top), (-6.6, 1.0, ZC - ch + 0.1), "alu")
    # heat shield behind, on four rods
    m.box((-11.6, -3.5, top + 0.6), (-11.35, 3.5, Z2 + R + 0.4), "perf")
    for y in (-2.6, 2.6):
        for z in (Z1 - 0.6, Z2 + 0.6):
            m.cyl((-11.35, y, z), (-7.4, y, z), 0.18, mat="alu", segs=5, cap0=False)
    m.tag("tag_flash", (X1 + 0.3, 0, ZC))
    n_body = m.save(d / "rocketl.md3", "quad_launcher")

    def build(f):                                   # all four tubes light up
        for y in (-1.55, 1.55):
            for z in (-1.5, 1.5):
                flash_star(f, (-0.3, y, z), 2.4, "orange", turn=y * z)
                flash_cone(f, (-0.3, y, z), 8.5, 2.9, "orange")
    return {"body": n_body, "flash": write_flash(d, "rocketl", build)}


# ---------------------------------------------------------------- Spark Gun

def spark_gun():
    """Lightning gun: an oversized ignition coil in a cage, copper winding
    showing; two spark plugs in front are the electrodes, orange HT leads
    run round the outside, an ice-blue spark between the tips."""
    d = OUT / "lightning"
    m = Mesh(atlas.CELLS)
    use(m, "body", PARTS)
    top = plate(m)
    CZ, CR = top + 3.5, 2.3
    for x0, x1 in ((-5.4, -4.0), (0.4, 1.8)):                  # saddles
        m.box((x0, -1.9, top), (x1, 1.9, CZ - CR + 0.5), "gunmetal")
    m.cyl((-5.0, 0, CZ), (2.0, 0, CZ), CR, mat="copper", segs=16, cap0=False, cap1=False)
    for k in range(6):                                          # winding ribs
        x = -4.5 + k * 1.15
        m.cyl((x, 0, CZ), (x + 0.25, 0, CZ), CR + 0.12, mat="copper", segs=16, cap0=False, cap1=False)
    m.cyl((-6.0, 0, CZ), (-5.0, 0, CZ), CR + 0.5, mat="black", segs=16)
    m.cyl((2.0, 0, CZ), (3.0, 0, CZ), CR + 0.5, mat="black", segs=16)
    TOWER = (1.0, 0, CZ + CR + 1.3)
    m.cyl((1.0, 0, CZ + CR - 0.2), TOWER, 0.95, 0.55, mat="black", segs=10)
    m.cyl(TOWER, (1.0, 0, TOWER[2] + 0.35), 0.35, mat="alu", segs=8)
    # cage: four rods and three hoops
    RC = CR + 1.0
    for k in range(4):
        a = math.pi / 4 + k * math.pi / 2
        m.cyl((-6.6, RC * math.cos(a), CZ + RC * math.sin(a)), (3.6, RC * math.cos(a), CZ + RC * math.sin(a)),
              0.2, mat="alu", segs=5)
    for x in (-6.3, -1.5, 3.3):
        hoop = ellipse_x(x, 0, CZ, RC, RC, 12)
        m.smooth_tube(hoop + hoop[:1], 0.17, "alu", segs=4)
    # insulator block and plugs
    m.box((3.0, -2.3, CZ - 1.5), (5.0, 2.3, CZ + 1.5), "black")
    m.box((5.0, -2.1, CZ - 1.3), (5.2, 2.1, CZ + 1.3), "alu")
    SX = 11.0
    for sy in (-1, 1):
        a = (5.2, sy * 1.3, CZ)
        tip = (9.6, sy * 0.55, CZ)
        dvec = [tip[i] - a[i] for i in range(3)]
        ln = math.sqrt(sum(c * c for c in dvec))
        u = [c / ln for c in dvec]

        def at(t):
            return tuple(a[i] + u[i] * t for i in range(3))
        m.cyl(at(0), at(1.9), 0.6, 0.48, mat="white", segs=10, cap0=False)
        for t in (0.4, 0.9, 1.4):
            m.cyl(at(t), at(t + 0.18), 0.66, mat="white", segs=10, cap0=False, cap1=False)
        m.cyl(at(1.9), at(2.7), 0.78, mat="steel", segs=6)
        m.cyl(at(2.7), at(ln), 0.46, mat="steel", segs=8, cap0=False)
        m.tube([tip, (SX - 0.9, sy * 0.4, CZ), (SX - 0.3, sy * 0.28, CZ)], 0.12, "chrome", segs=4)
        # HT lead from the coil tower to the back of the plug
        lead = [(TOWER[0], 0, TOWER[2] - 0.1), (1.8, sy * 0.9, TOWER[2] + 0.4), (3.6, sy * 2.9, CZ + 2.6),
                (5.0, sy * 2.7, CZ + 1.2), (5.7, sy * 1.6, CZ + 0.45)]
        m.smooth_tube(lead, 0.27, "orange", segs=6)
    # the spark
    use(m, "spark", SPARK)
    m.cross_quads((SX, 0, CZ), 0.9, 0.6, 0.75)
    use(m, "body", PARTS)
    m.tag("tag_flash", (SX + 0.3, 0, CZ))
    n_body = m.save(d / "lightning.md3", "spark_gun")

    def build(f):                                   # a crackling spark burst
        flash_star(f, (-0.3, 0, 0), 4.0, "ice")
        flash_star(f, (-0.2, 0, 0), 3.0, "ice", turn=0.7)
        flash_ring(f, (-0.4, 0, 0), 2.0, "ice")
    return {"body": n_body, "flash": write_flash(d, "lightning", build)}


# ---------------------------------------------------------------- Turbo Railgun

def turbo_railgun():
    """Railgun: a turbocharger at the back charges the gun (its compressor
    wheel is railgun_barrel.md3 and spools up while firing); two long
    carbon rails with copper coil rings, a mint glow between them."""
    d = OUT / "railgun"
    m = Mesh(atlas.CELLS)
    use(m, "body", PARTS)
    top = plate(m)
    TZ, TR = top + 2.9, 2.5
    RZ = TZ
    # compressor housing and inlet
    m.cyl((-7.0, 0, TZ), (-4.6, 0, TZ), TR, mat="alu", segs=18, cap0=True, cap1=False)
    m.cyl((-8.6, 0, TZ), (-7.0, 0, TZ), 1.85, mat="alu", segs=16, cap0=False, cap1=False)
    m.ring((-8.6, 0, TZ), (-1, 0, 0), 1.55, 1.85, "alu", segs=16)
    m.cyl((-8.9, 0, TZ), (-8.6, 0, TZ), 2.0, mat="black", segs=16, cap0=False, cap1=False)
    m.bore((-8.6, 0, TZ), (-1, 0, 0), 1.55, 2.2)
    # volute: a growing scroll round the housing, out into the plenum
    vol = []
    for k in range(14):
        a = math.radians(200 - k * 17)
        r = TR + 0.2 + 0.5 * k / 13
        vol.append((-5.8, r * math.cos(a), TZ + r * math.sin(a)))
    vol += [(-4.9, vol[-1][1] - 0.3, vol[-1][2] + 0.3), (-4.2, -2.4, TZ - 0.6)]
    m.smooth_tube(vol, 0.55, "alu", segs=8, cap0=True)
    # plenum block that holds the rails
    m.loft([rrect_x(x, 0, RZ, 2.6 * s, 2.3 * s, 0.8) for x, s in ((-4.6, 1.0), (-1.2, 1.0), (-0.8, 0.9))],
           "gunmetal", cap_mat="gunmetal")
    m.box((-4.3, -2.2, top), (-1.2, 2.2, RZ - 2.0), "gunmetal")
    # rails
    XR = 21.0
    for y0, y1 in ((0.55, 1.75), (-1.75, -0.55)):
        m.box((-1.0, y0, RZ - 1.0), (XR, y1, RZ + 1.0), "carbon")
        m.box((XR, y0 - 0.1, RZ - 1.1), (XR + 0.7, y1 + 0.1, RZ + 1.1), "alu")
    for x in (2.5, 7.0, 11.5, 16.0):
        m.loft([rrect_x(x + dx, 0, RZ, 2.2 * s, 1.45 * s, 0.6) for dx, s in
                ((-0.5, 0.93), (-0.4, 1.0), (0.4, 1.0), (0.5, 0.93))], "copper", cap0=False, cap1=False)
    m.box((5.6, -1.3, top), (7.0, 1.3, RZ - 1.0), "alu")
    m.box((5.2, -2.0, top), (7.4, 2.0, top + 0.4), "alu")
    use(m, "glow", GLOW)
    m.box((-0.8, -0.55, RZ - 0.35), (XR - 0.2, 0.55, RZ + 0.35), "mint")
    use(m, "body", PARTS)
    m.tag("tag_barrel", (-8.0, 0, TZ))
    m.tag("tag_flash", (XR + 1.1, 0, RZ))
    n_body = m.save(d / "railgun.md3", "turbo_railgun")

    # compressor wheel: hub cone and swept blades
    b = Mesh(atlas.CELLS)
    b.surf("wheel", CHROME)
    b.cyl((-0.6, 0, 0), (0.6, 0, 0), 0.25, 0.6, mat="chrome", segs=10)
    for k in range(7):
        a = 2 * math.pi * k / 7
        pts = [(-0.6, 0.3 * math.cos(a), 0.3 * math.sin(a)), (-0.5, 1.45 * math.cos(a + 0.25), 1.45 * math.sin(a + 0.25)),
               (0.6, 1.45 * math.cos(a - 0.15), 1.45 * math.sin(a - 0.15)), (0.6, 0.6 * math.cos(a), 0.6 * math.sin(a))]
        nrm = (0.3, -math.sin(a), math.cos(a))
        b.poly2(pts, nrm, "chrome")
        b.poly2(pts, tuple(-c for c in nrm), "chrome")
    n_barrel = b.save(d / "railgun_barrel.md3", "compressor")

    def build(f):                                   # in the player's rail colour
        flash_ring(f, (0, 0, 0), 2.6, "entity")
        flash_star(f, (0.1, 0, 0), 3.6, "entity")
        flash_cone(f, (0, 0, 0), 6.0, 2.4, "entity")
    return {"body": n_body, "barrel": n_barrel, "flash": write_flash(d, "railgun", build)}


# ---------------------------------------------------------------- Plasma Cooler

def plasma_cooler():
    """Plasma gun: an intercooler core between cast end tanks; the front tank
    runs into a nozzle with blue sight windows, an orange silicone hose
    loops back over the core."""
    d = OUT / "plasma"
    m = Mesh(atlas.CELLS)
    use(m, "body", PARTS)
    top = plate(m)
    CZ, HY, HZ = top + 2.9, 3.4, 1.9
    for x in (-4.2, 4.2):                                       # brackets
        m.box((x - 0.5, -2.8, top), (x + 0.5, 2.8, CZ - HZ - 0.3), "alu")
    m.box((-5.5, -HY, CZ - HZ), (7.5, HY, CZ + HZ), "fins")
    m.box((-5.5, -HY - 0.1, CZ + HZ), (7.5, HY + 0.1, CZ + HZ + 0.3), "alu")
    m.box((-5.5, -HY - 0.1, CZ - HZ - 0.3), (7.5, HY + 0.1, CZ - HZ), "alu")
    T = HZ + 0.3
    m.loft([rrect_x(x, 0, CZ, hy, hz, r) for x, hy, hz, r in
            ((-5.5, HY + 0.1, T, 0.4), (-6.9, 2.7, 1.8, 0.9), (-7.5, 2.2, 1.4, 0.9))], "alu", cap0=False)
    m.loft([rrect_x(x, 0, CZ, hy, hz, r) for x, hy, hz, r in
            ((7.5, HY + 0.1, T, 0.4), (9.2, 2.1, 1.75, 1.0), (10.3, 1.75, 1.75, 1.6))], "alu", cap0=False,
           cap1=False)
    # nozzle body with sight windows
    NR = 1.75
    m.cyl((10.3, 0, CZ), (14.6, 0, CZ), NR, mat="gunmetal", segs=16, cap0=False, cap1=False)
    m.cyl((14.6, 0, CZ), (17.0, 0, CZ), NR, 1.05, mat="alu", segs=16, cap0=False, cap1=False)
    m.ring((17.0, 0, CZ), (1, 0, 0), 0.7, 1.05, "alu", segs=16)
    wins = []
    for sy in (-1, 1):
        m.box((10.9, -0.05 + sy * 1.5 - 0.25 * (sy < 0), CZ - 0.85), (14.0, sy * 1.5 + 0.3 * (sy > 0), CZ + 0.85),
              "black")
        wins.append([(11.2, sy * 1.83, CZ - 0.6), (13.7, sy * 1.83, CZ - 0.6), (13.7, sy * 1.83, CZ + 0.6),
                     (11.2, sy * 1.83, CZ + 0.6)])
    m.box((10.9, -0.85, CZ + 1.4), (14.0, 0.85, CZ + 1.82), "black")
    top_win = [(11.2, -0.6, CZ + 1.85), (13.7, -0.6, CZ + 1.85), (13.7, 0.6, CZ + 1.85), (11.2, 0.6, CZ + 1.85)]
    # hose over the core, clamps at both ends
    hose = [(-6.7, 0, CZ + 1.3), (-6.6, 0, CZ + 2.7), (-5.0, 0, CZ + 3.6), (0.0, 0, CZ + 3.9), (5.0, 0, CZ + 3.7),
            (8.4, 0, CZ + 2.9), (9.3, 0, CZ + 1.5)]
    m.smooth_tube(hose, 0.62, "orange", segs=8)
    for p, q in ((hose[0], hose[1]), (hose[-1], hose[-2])):
        dv = [q[i] - p[i] for i in range(3)]
        ln = math.sqrt(sum(c * c for c in dv))
        e = tuple(p[i] + dv[i] / ln * 0.7 for i in range(3))
        b0 = tuple(p[i] + dv[i] / ln * 0.3 for i in range(3))
        m.cyl(b0, e, 0.74, mat="alu", segs=8, cap0=False, cap1=False)
    use(m, "glow", GLOW)
    for sy, w in zip((-1, 1), wins):
        m.poly2(w, (0, sy, 0), "blue")
    m.poly2(top_win, (0, 0, 1), "blue")
    m.bore((17.0, 0, CZ), (1, 0, 0), 0.7, 2.5, mat="blue")
    use(m, "body", PARTS)
    m.tag("tag_flash", (17.3, 0, CZ))
    n_body = m.save(d / "plasma.md3", "plasma_cooler")

    def build(f):
        flash_ring(f, (0, 0, 0), 2.0, "blue")
        flash_star(f, (0.1, 0, 0), 3.0, "blue")
        flash_cone(f, (0, 0, 0), 4.5, 2.8, "blue")
    return {"body": n_body, "flash": write_flash(d, "plasma", build)}


# ---------------------------------------------------------------- BFG Blower

def bfg_blower():
    """BFG: a Roots blower like on a dragster, the belt pulley at the back
    (bfg_barrel.md3, spins while firing); the orange scoop on top faces
    forward and is the muzzle, toxic green glow inside."""
    d = OUT / "bfg"
    m = Mesh(atlas.CELLS)
    use(m, "body", PARTS)
    top = plate(m)
    m.box((-6.0, -3.4, top), (7.0, 3.4, top + 1.0), "gunmetal")
    BZ, HY, HZ = top + 1.0 + 2.6, 3.5, 2.6
    m.loft([rrect_x(x, 0, BZ, HY * s, HZ * s, 1.4) for x, s in ((-5.8, 0.9), (-5.4, 1.0), (6.4, 1.0), (6.8, 0.9))],
           "alu", cap_mat="gunmetal")
    for k in range(7):                                          # ribs
        x = -4.0 + k * 1.5
        for sy in (-1, 1):
            m.box((x - 0.17, min(sy * (HY - 0.1), sy * (HY + 0.35)), BZ - 1.8),
                  (x + 0.17, max(sy * (HY - 0.1), sy * (HY + 0.35)), BZ + 1.8), "alu")
    for x in (-5.4, 6.4):                                       # end plate bolts
        for y in (-2.4, 2.4):
            for z in (BZ - 1.6, BZ + 1.6):
                m.cyl((x + (0.0 if x > 0 else 0.0), y, z), (x + (0.35 if x > 0 else -0.35), y, z), 0.28,
                      mat="steel", segs=6)
    # scoop: orange, open to the front
    m.box((-2.6, -2.3, BZ + HZ - 0.4), (6.2, 2.3, BZ + HZ + 0.4), "gunmetal")
    S = [(-2.2, 1.5, 0.5, 0.4), (0.0, 2.4, 1.1, 0.8), (8.6, 3.0, 1.7, 1.0), (9.2, 3.0, 1.7, 1.0)]
    zb = BZ + HZ + 0.3

    def sring(x, hy, hz, r, shrink=0.0):
        return rrect_x(x, 0, zb + hz, hy - shrink, hz - shrink, max(0.2, r - shrink))
    m.loft([sring(*s) for s in S], "orange", cap0=True, cap1=False)
    outer = sring(*S[-1])
    inner = sring(*S[-1], shrink=0.35)
    flat_ring(m, outer, inner, (1, 0, 0), "alu")
    deep = rrect_x(5.6, 0, zb + 1.6, 2.4, 1.2, 0.6)
    m.loft([inner, deep], "dark", cap0=False, cap1=False, inside=True)
    gz = zb + 1.7
    for z in (gz - 0.8, gz, gz + 0.8):                          # grille bars
        m.box((8.75, -2.6, z - 0.12), (8.95, 2.6, z + 0.12), "black")
    use(m, "glow", GLOW)
    m.poly2(deep, (1, 0, 0), "green")
    use(m, "body", PARTS)
    m.tag("tag_barrel", (-5.9, 0, BZ - 0.6))
    m.tag("tag_flash", (9.6, 0, gz))
    n_body = m.save(d / "bfg.md3", "bfg_blower")

    # the pulley: grooved, with lightening holes so the spin shows
    b = Mesh(atlas.CELLS)
    b.surf("pulley", PARTS)
    b.cyl((-1.0, 0, 0), (0.1, 0, 0), 2.3, mat="black", segs=20, cap_mat="black")
    for x in (-0.8, -0.45, -0.1):
        b.cyl((x - 0.1, 0, 0), (x + 0.1, 0, 0), 2.4, mat="steel", segs=20, cap0=False, cap1=False)
    b.cyl((-1.3, 0, 0), (-1.0, 0, 0), 0.8, mat="alu", segs=12)
    for k in range(4):
        a = 2 * math.pi * k / 4 + math.pi / 4
        b.poly([(-1.02, 1.45 * math.cos(a) + 0.5 * math.cos(-2 * math.pi * j / 8),
                 1.45 * math.sin(a) + 0.5 * math.sin(-2 * math.pi * j / 8)) for j in range(8)], (-1, 0, 0), "dark")
        b.cyl((-1.4, 0.45 * math.cos(a), 0.45 * math.sin(a)), (-1.3, 0.45 * math.cos(a), 0.45 * math.sin(a)),
              0.15, mat="steel", segs=6)
    n_barrel = b.save(d / "bfg_barrel.md3", "pulley")

    def build(f):                                   # the scoop belches green
        flash_star(f, (0, 0, 0), 7.0, "green")
        flash_cone(f, (0, 0, 0), 12.0, 7.5, "green")
    return {"body": n_body, "barrel": n_barrel, "flash": write_flash(d, "bfg", build)}


# ---------------------------------------------------------------- Nitro Flamer

def nitro_flamer():
    """Flame thrower: a lying orange pressure bottle with a Q3R band,
    regulator and gauge, a braided line up to an afterburner nozzle with
    petals; a small blue pilot flame burns under the nozzle."""
    d = OUT / "flamethrower"
    m = Mesh(atlas.CELLS)
    use(m, "body", PARTS)
    top = plate(m)
    FR = 2.3
    FZ = top + 0.4 + FR
    for x in (-4.9, 2.5):
        m.box((x - 0.5, -1.6, top), (x + 0.5, 1.6, FZ - FR + 0.5), "alu")
    rings = []
    n = 18
    for k in range(6):                                          # back dome
        ph = math.pi / 2 * (1 - k / 5)
        rings.append(ellipse_x(-6.0 - FR * 0.8 * math.sin(ph), 0, FZ, max(0.3, FR * math.cos(ph)),
                               max(0.3, FR * math.cos(ph)), n))
    rings.append(ellipse_x(3.6, 0, FZ, FR, FR, n))
    for k in range(1, 6):                                       # front dome down to the neck
        ph = math.pi / 2 * k / 5
        r = max(0.7, FR * math.cos(ph))
        rings.append(ellipse_x(3.6 + FR * 0.8 * math.sin(ph), 0, FZ, r, r, n))
    m.loft(rings, "orange", cap0=True, cap1=False)
    # the label band: u starts at the bottom, so the two copies of the text sit on the sides
    m.loft([ellipse_x(x, 0, FZ, FR + 0.04, FR + 0.04, n, -math.pi / 2) for x in (-4.0, -0.2)], "label", cap0=False,
           cap1=False)
    for x in (-4.9, 2.5):                                       # straps
        m.cyl((x - 0.3, 0, FZ), (x + 0.3, 0, FZ), FR + 0.12, mat="black", segs=n, cap0=False, cap1=False)
    # neck, regulator, gauge, knob
    m.cyl((5.4, 0, FZ), (6.6, 0, FZ), 0.65, mat="steel", segs=8, cap0=False)
    m.box((6.6, -1.0, FZ - 0.9), (8.0, 1.0, FZ + 0.9), "alu")
    m.cyl((7.3, 1.0, FZ + 0.15), (7.3, 1.45, FZ + 0.15), 0.85, mat="black", segs=12, cap_mat="white")
    m.cyl((7.3, 0, FZ + 0.9), (7.3, 0, FZ + 1.5), 0.45, mat="black", segs=8)
    # nozzle
    NZ, NR = FZ + 2.6, 1.35
    m.cyl((10.0, 0, NZ), (12.6, 0, NZ), NR, mat="gunmetal", segs=14, cap_mat="alu", cap1=False)
    m.cyl((12.6, 0, NZ), (15.2, 0, NZ), NR, 1.85, mat="alu", segs=14, cap0=False, cap1=False)
    m.ring((15.2, 0, NZ), (1, 0, 0), 1.25, 1.85, "alu", segs=14)
    m.bore((15.2, 0, NZ), (1, 0, 0), 1.25, 3.2)
    for k in range(10):                                         # petals
        a0 = 2 * math.pi * (k + 0.1) / 10
        a1 = 2 * math.pi * (k + 0.9) / 10
        pts = [(15.1, 1.8 * math.cos(a0), NZ + 1.8 * math.sin(a0)), (15.1, 1.8 * math.cos(a1), NZ + 1.8 * math.sin(a1)),
               (16.7, 1.5 * math.cos(a1), NZ + 1.5 * math.sin(a1)), (16.7, 1.5 * math.cos(a0), NZ + 1.5 * math.sin(a0))]
        am = (a0 + a1) / 2
        nrm = (0.2, math.cos(am), math.sin(am))
        m.poly2(pts, nrm, "heat")
        m.poly2(pts, tuple(-c for c in nrm), "heat")
    # braided line from the regulator up into the nozzle, struts, pilot tube
    m.smooth_tube([(8.0, 0, FZ + 0.4), (8.9, 0, FZ + 0.6), (9.6, 0, FZ + 1.4), (10.4, 0, NZ - 1.0),
                   (11.0, 0, NZ - NR + 0.3)], 0.32, "steel", segs=6)
    for y in (-0.9, 0.9):
        m.cyl((7.6, y, top), (11.6, y, NZ - NR + 0.2), 0.32, mat="alu", segs=4, phase=math.pi / 4)
    m.cyl((12.0, 0, NZ - NR + 0.1), (16.6, 0, NZ - NR - 0.35), 0.16, mat="steel", segs=5)
    use(m, "pilot", PILOT)
    m.cross_quads((17.15, 0, NZ - NR - 0.3), 0.55, 0.3, 0.42)
    use(m, "body", PARTS)
    m.tag("tag_flash", (17.0, 0, NZ))
    n_body = m.save(d / "flamethrower.md3", "nitro_flamer")
    return {"body": n_body, "flash": keep_flash(d, "flamethrower")}


WEAPONS = {"spotlight": spotlight, "harvester": harvester, "twin_shotgun": twin_shotgun,
           "drum_launcher": drum_launcher, "quad_launcher": quad_launcher, "spark_gun": spark_gun,
           "turbo_railgun": turbo_railgun, "plasma_cooler": plasma_cooler, "bfg_blower": bfg_blower,
           "nitro_flamer": nitro_flamer}


def write_shader():
    text = """// Q3Rally weapons (tools/carbuild/weapons/build_weapons.py): one shared atlas.

models/weapons2/q3r/parts
{
	qer_editorimage models/weapons2/q3r/parts.tga
	{
		map models/weapons2/q3r/chrome.tga
		tcGen environment
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/q3r/parts.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		depthWrite
		rgbGen lightingDiffuse
	}
}

// polished parts: the atlas plus a reflection
models/weapons2/q3r/chrome
{
	qer_editorimage models/weapons2/q3r/parts.tga
	{
		map models/weapons2/q3r/parts.tga
		rgbGen lightingDiffuse
	}
	{
		map models/weapons2/q3r/chrome.tga
		blendFunc GL_ONE GL_ONE
		tcGen environment
		rgbGen const ( 0.45 0.45 0.45 )
	}
}

// glowing parts (rail, sight windows, scoop core): fullbright from the atlas
models/weapons2/q3r/glow
{
	nopicmip
	{
		map models/weapons2/q3r/parts.tga
		rgbGen wave sin 0.9 0.1 0 1.5
	}
}

// Spark Gun: the spark between the plug tips
models/weapons2/q3r/spark
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/spark.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave noise 0.7 0.4 0 7
	}
}

// Nitro Flamer: the pilot flame under the nozzle
models/weapons2/q3r/pilot
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/pilot.tga
		blendFunc GL_ONE GL_ONE
		rgbGen wave noise 0.8 0.25 0 5
	}
}

// Harvester Saw: the chain runs round the bar
models/weapons2/q3r/chain
{
	{
		map models/weapons2/q3r/chain.tga
		tcMod scroll -1.6 0
		rgbGen lightingDiffuse
	}
}

// muzzle flashes: grey star / cone / ring textures, tinted per weapon,
// a white-hot core on top
@@FLASHES@@
// Spotlight MG: the lamp ring flashes with every shot (in machinegun_flash.md3)
models/weapons2/q3r/lampflash
{
	nopicmip
	cull none
	{
		clampmap models/weapons2/q3r/lampflash.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}
}
"""
    parts = []
    for colour, rgb in FLASH_COLOURS.items():
        gen = "rgbGen entity" if rgb is None else "rgbGen const ( %.2f %.2f %.2f )" % rgb
        for kind, tex, core in (("star", "flash_star", "flash_star_core"), ("cone", "flash_cone", "flash_cone_core"),
                                ("ring", "lampflash", None)):
            stages = f"""	{{
		clampmap models/weapons2/q3r/{tex}.tga
		blendFunc GL_ONE GL_ONE
		{gen}
	}}
"""
            if core:
                stages += f"""	{{
		clampmap models/weapons2/q3r/{core}.tga
		blendFunc GL_ONE GL_ONE
		rgbGen identity
	}}
"""
            parts.append(f"{fshader(colour, kind)}\n{{\n	nopicmip\n	cull none\n{stages}}}\n")
    text = text.replace("@@FLASHES@@\n", "\n".join(parts) + "\n")
    (ROOT / "baseq3r" / "scripts" / "q3r_weapons.shader").write_text(text, encoding="utf-8")


def main(names):
    atlas.write(OUT / "q3r")
    write_shader()
    for name in names or WEAPONS:
        print(name, WEAPONS[name]())


if __name__ == "__main__":
    main(sys.argv[1:])
