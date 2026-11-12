#!/usr/bin/env python3
"""Build the Q3Rally weapon models ("Gebaut, nicht gekauft").

Every weapon is a workshop conversion of car parts and stands on the same
roof plate. All of them share one texture atlas (models/weapons2/q3r/parts.tga,
written by atlas.py) and the shader models/weapons2/q3r/parts.

  python tools/carbuild/weapons/build_weapons.py [spotlight ...]

Writes into baseq3r/models/weapons2/<weapon>/ the files cgame loads for it:
<weapon>.md3 (body, with tag_flash and, if it spins, tag_barrel),
<weapon>_barrel.md3 and <weapon>_flash.md3.

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

    # flash: the old muzzle flash plus a glowing ring around the lamp
    old = md3.load(str(d / "machinegun_flash.md3")) if (d / "machinegun_flash.md3").exists() else None
    f = Mesh(atlas.CELLS)
    if old:
        f.surfs = [s for s in old.surfs if s.name != "lampring"]
    f.surf("lampring", "models/weapons2/q3r/lampflash")
    back = -(barrel_len + 0.3) + (LAMP_X1 + 0.75 - ROTOR_X)
    r = LAMP_R + 0.6
    f.quad([(back, r, r), (back, -r, r), (back, -r, -r), (back, r, -r)], (1, 0, 0), "orange")
    s = f.cur
    s.st = [[0, 0], [1, 0], [1, 1], [0, 1]]
    n_flash = f.save(d / "machinegun_flash.md3", "spotlight_flash")
    return {"body": n_body, "barrel": n_barrel, "flash": n_flash}


WEAPONS = {"spotlight": spotlight}


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
    (ROOT / "baseq3r" / "scripts" / "q3r_weapons.shader").write_text(text, encoding="utf-8")


def main(names):
    atlas.write(OUT / "q3r")
    write_shader()
    for name in names or WEAPONS:
        print(name, WEAPONS[name]())


if __name__ == "__main__":
    main(sys.argv[1:])
