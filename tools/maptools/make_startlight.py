#!/usr/bin/env python3
"""Build the start light model for the rally_startlight entity.

  models/mapobjects/startlight/startlight.md3   housing with four lamps
  models/mapobjects/startlight/*.tga|jpg         housing, lenses, glow
  scripts/startlight.shader

The model faces +x. Its origin is the centre of the lamp row on the lens
plane: three red lamps and then a green one, 16 apart, lens radius 5.5, from
left to right as the drivers see them (along +y). These are the
defaults of rally_startlight (cg_rally_fx.c), which draws the light of the
lamps; put the misc_model and the entity at the same origin and angle.
Two hanger rods reach up to z 24, so the light can hang under a beam.

Run with Python 3 + numpy + Pillow.
"""
from __future__ import annotations

import math
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "carbuild"))
import md3  # noqa: E402

DIR = "models/mapobjects/startlight/"
COUNT, SPACING, RADIUS = 4, 16.0, 5.5
HANG = 24.0                       # top of the hanger rods above the origin


class Mesh:
    """Surfaces of one md3, built triangle by triangle."""

    def __init__(self):
        self.surfs = []

    def surf(self, name, shader):
        s = md3.Surf()
        s.name, s.flags, s.shaders = name, 0, [(shader, 0)]
        s.tris, s.st, s.frames = [], [], [[]]
        self.surfs.append(s)
        return s

    @staticmethod
    def vert(s, p, n, uv):
        ln = math.sqrt(sum(c * c for c in n)) or 1.0
        s.frames[0].append([round(p[0] * 64), round(p[1] * 64), round(p[2] * 64),
                            md3.enc_normal(n[0] / ln, n[1] / ln, n[2] / ln)])
        s.st.append([uv[0], uv[1]])
        return len(s.st) - 1

    @staticmethod
    def tri(s, a, b, c):
        s.tris.append([a, c, b])

    def quad(self, s, pts, n, uvs=((0, 0), (1, 0), (1, 1), (0, 1))):
        idx = [self.vert(s, p, n, uv) for p, uv in zip(pts, uvs)]
        self.tri(s, idx[0], idx[1], idx[2])
        self.tri(s, idx[0], idx[2], idx[3])

    def box(self, s, lo, hi, tex=16.0):
        (x0, y0, z0), (x1, y1, z1) = lo, hi
        c = [(x, y, z) for x in (x0, x1) for y in (y0, y1) for z in (z0, z1)]
        for q, n in (((4, 6, 7, 5), (1, 0, 0)), ((0, 1, 3, 2), (-1, 0, 0)), ((2, 3, 7, 6), (0, 1, 0)),
                     ((0, 4, 5, 1), (0, -1, 0)), ((1, 5, 7, 3), (0, 0, 1)), ((0, 2, 6, 4), (0, 0, -1))):
            pts = [c[k] for k in q]
            ax = [i for i in range(3) if n[i] == 0]
            uvs = [(p[ax[0]] / tex, p[ax[1]] / tex) for p in pts]
            self.quad(s, pts, n, uvs)

    def tube(self, s, a, b, r, segs=8):
        """A round rod from a to b (any direction)."""
        d = [b[i] - a[i] for i in range(3)]
        ln = math.sqrt(sum(c * c for c in d))
        d = [c / ln for c in d]
        ref = (0, 0, 1) if abs(d[2]) < 0.9 else (1, 0, 0)
        u = [d[1] * ref[2] - d[2] * ref[1], d[2] * ref[0] - d[0] * ref[2], d[0] * ref[1] - d[1] * ref[0]]
        ul = math.sqrt(sum(c * c for c in u))
        u = [c / ul for c in u]
        v = [d[1] * u[2] - d[2] * u[1], d[2] * u[0] - d[0] * u[2], d[0] * u[1] - d[1] * u[0]]
        r0, r1 = [], []
        for k in range(segs + 1):
            t = 2 * math.pi * k / segs
            n = [math.cos(t) * u[i] + math.sin(t) * v[i] for i in range(3)]
            r0.append(self.vert(s, [a[i] + r * n[i] for i in range(3)], n, (k / segs, 0)))
            r1.append(self.vert(s, [b[i] + r * n[i] for i in range(3)], n, (k / segs, ln / 32)))
        for k in range(segs):
            self.tri(s, r0[k], r0[k + 1], r1[k + 1])
            self.tri(s, r0[k], r1[k + 1], r1[k])

    def save(self, path, name):
        m = md3.Model()
        m.name, m.flags = name, 0
        pts = [[c / 64 for c in v[:3]] for s in self.surfs for v in s.frames[0]]
        lo = [min(p[i] for p in pts) for i in range(3)]
        hi = [max(p[i] for p in pts) for i in range(3)]
        rad = max(math.sqrt(sum(c * c for c in p)) for p in pts)
        m.frames = [[*lo, *hi, 0, 0, 0, rad, name.encode()[:16]]]
        m.tags = []
        m.surfs = self.surfs
        path.parent.mkdir(parents=True, exist_ok=True)
        md3.save(m, str(path))


def build_model():
    me = Mesh()
    half = (COUNT - 1) * SPACING / 2 + SPACING / 2 + 2
    body = me.surf("housing", DIR + "housing")
    me.box(body, (-11, -half, -11), (-0.6, half, 11))
    # visors: open half tubes over every lamp, against the morning sun
    visor = me.surf("visors", DIR + "housing")
    vr, segs = RADIUS + 1.8, 9
    for k in range(COUNT):
        y = (k - (COUNT - 1) / 2) * SPACING
        ring0, ring1 = [], []
        for j in range(segs + 1):
            a = math.radians(-20 + 220 * j / segs)          # over the top, a bit down the sides
            n = (0, math.cos(a), math.sin(a))
            ring0.append(me.vert(visor, (-0.6, y + vr * n[1], vr * n[2]), n, (j / segs, 0)))
            ring1.append(me.vert(visor, (7.5, y + vr * n[1], vr * n[2]), n, (j / segs, 0.5)))
        for j in range(segs):
            me.tri(visor, ring0[j], ring0[j + 1], ring1[j + 1])
            me.tri(visor, ring0[j], ring1[j + 1], ring1[j])
        # the lens
        lens = me.surf(f"lens{k}", DIR + ("lens_green" if k == COUNT - 1 else "lens_red"))
        c = me.vert(lens, (0, y, 0), (1, 0, 0), (0.5, 0.5))
        rim = []
        for j in range(17):
            a = 2 * math.pi * j / 16
            rim.append(me.vert(lens, (0, y + RADIUS * math.cos(a), RADIUS * math.sin(a)), (1, 0, 0),
                               (0.5 + 0.5 * math.cos(a), 0.5 - 0.5 * math.sin(a))))
        for j in range(16):
            me.tri(lens, c, rim[j + 1], rim[j])
    # bracket and hanger rods
    metal = me.surf("bracket", DIR + "housing")
    me.box(metal, (-9, -half * 0.7, 11), (-2.5, half * 0.7, 12.5))
    for y in (-half * 0.55, half * 0.55):
        me.tube(metal, (-5.8, y, 12.5), (-5.8, y, HANG), 0.9, 6)
    me.save(ROOT / "baseq3r" / DIR / "startlight.md3", "startlight")


def write_textures():
    import numpy as np
    from PIL import Image, ImageFilter
    out = ROOT / "baseq3r" / DIR
    out.mkdir(parents=True, exist_ok=True)
    rng = np.random.default_rng(3)
    a = 34 + 10 * rng.random((128, 128))
    Image.fromarray(np.dstack([a, a, a * 1.05]).clip(0, 255).astype(np.uint8)).filter(
        ImageFilter.GaussianBlur(0.8)).save(out / "housing.jpg", quality=92)
    n = 64
    yy, xx = np.mgrid[0:n, 0:n] - (n - 1) / 2
    rr = np.hypot(xx, yy) / (n / 2)
    rings = 0.5 + 0.5 * np.cos(rr * 30)
    for name, col in (("lens_red", (90, 12, 10)), ("lens_green", (12, 70, 26))):
        c = np.array(col) * (0.75 + 0.25 * rings[..., None]) * (1 + 0.6 * np.exp(-((xx + 12) ** 2 + (yy + 12) ** 2) / 40))[..., None]
        Image.fromarray(c.clip(0, 255).astype(np.uint8)).save(out / f"{name}.jpg", quality=94)
    glow = np.clip(1 - rr, 0, 1) ** 2.2 + 0.9 * np.clip(1 - rr / 0.35, 0, 1)
    g = (np.clip(glow, 0, 1) * 255).astype(np.uint8)
    Image.fromarray(np.dstack([g, g, g])).save(out / "glow.tga")


def write_shader():
    D = DIR
    text = f"""// Start light (rally_startlight). Written by tools/maptools/make_startlight.py.

{D}housing
{{
	qer_editorimage {D}housing.jpg
	surfaceparm nolightmap
	surfaceparm SURF_METAL
	cull disable
	{{
		map {D}housing.jpg
		rgbGen vertex
	}}
}}

{D}lens_red
{{
	qer_editorimage {D}lens_red.jpg
	surfaceparm nolightmap
	{{
		map {D}lens_red.jpg
		rgbGen vertex
	}}
	{{
		map {D}lens_red.jpg
		blendFunc GL_ONE GL_ONE
		tcGen environment
		rgbGen const ( 0.25 0.25 0.25 )
	}}
}}

{D}lens_green
{{
	qer_editorimage {D}lens_green.jpg
	surfaceparm nolightmap
	{{
		map {D}lens_green.jpg
		rgbGen vertex
	}}
	{{
		map {D}lens_green.jpg
		blendFunc GL_ONE GL_ONE
		tcGen environment
		rgbGen const ( 0.25 0.25 0.25 )
	}}
}}

// the light of a lit lamp, drawn by cgame (additive, coloured per vertex)
{D}glow
{{
	nopicmip
	nomipmaps
	cull disable
	{{
		clampmap {D}glow.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}}
}}
"""
    (ROOT / "baseq3r" / "scripts" / "startlight.shader").write_text(text, encoding="utf-8")


if __name__ == "__main__":
    build_model()
    write_textures()
    write_shader()
    print("wrote", ROOT / "baseq3r" / DIR)
