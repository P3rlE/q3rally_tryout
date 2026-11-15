#!/usr/bin/env python3
"""Drag Race maps: one strip, three venues.

    python3 tools/maptools/make_dragstrip.py            # all maps, textures, shader, arenas
    python3 tools/maptools/make_dragstrip.py desert     # only one map (desert, night, airfield)

Every map keeps the Drag contract of docs/drag-race-map.md: driving along +X
on Z=0, the cars start at (0, -160) and (0, 160), start gate at X=100,
finish gate 640 units wide (it defines the two lanes), the overhead start
lights of the prototype, concrete walls at Y=+-384 and the same asphalt
surface flag on both lanes. What differs is the distance and the venue:

    q3r_dragstrip       Desert Dragway   1/4 mile  dragway in the desert, day
    q3r_drag_night      Night Drag       1/8 mile  closed-off city avenue at night
    q3r_drag_airfield   Airfield Drag    1/4 mile  runway of an airfield at sunset

Writes baseq3r/maps/<name>.map, scripts/<name>.arena, scripts/q3r_drag.shader
and the generated textures in textures/q3r_drag/. Compile: docs/maps/drag/README.md.
"""
from __future__ import annotations

import math
import random
import sys
from itertools import combinations
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[2]
BASE = ROOT / "baseq3r"
TEX = "q3r_drag/"
TEXDIR = BASE / "textures" / "q3r_drag"
DETAIL = 134217728

QUARTER, EIGHTH = 14348, 7174          # finish plane X for 1/4 and 1/8 mile (docs/drag-race-map.md)
UNITS_PER_FT = QUARTER / 1320.0
WALL_Y, WALL_T = 384, 24               # inner face of the walls, their thickness
BACK_X = -784                          # wall behind the grid
ROAD = "q3r_road/asphalt"              # SURF_ASPHALT: the same grip on every map
PAINT = "q3r_redrock/paint_white"      # nonsolid, SURF_ASPHALT
METAL = "q3r_redrock/metal2"
CAULK = "common/caulk"

FONT_BOLD = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
FONT_COND = "/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf"
FONT_MONO = str(ROOT / "launcher/assets/fonts/ShareTechMono-Regular.ttf")


# ================================================================= geometry

def num(v):
    s = f"{v:.4f}".rstrip("0").rstrip(".")
    return s if s not in ("", "-0") else "0"


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def plane(n, point):
    up = (1, 0, 0) if abs(n[0]) < 1e-9 and abs(n[1]) < 1e-9 else (0, 0, 1)
    t1 = cross(up, n)
    t2 = cross(n, t1)
    size = max(abs(c) for c in t1 + t2)
    return [point, tuple(p + c * 128 / size for p, c in zip(point, t2)),
            tuple(p + c * 128 / size for p, c in zip(point, t1))]


BASEAXIS = (((0, 0, 1), (1, 0, 0), (0, -1, 0)), ((0, 0, -1), (1, 0, 0), (0, -1, 0)),
            ((1, 0, 0), (0, 1, 0), (0, 0, -1)), ((-1, 0, 0), (0, 1, 0), (0, 0, -1)),
            ((0, 1, 0), (1, 0, 0), (0, 0, -1)), ((0, -1, 0), (1, 0, 0), (0, 0, -1)))


def texvecs(n, rot, ss, ts):
    """Texture vectors as q3map2 builds them for the old brush format."""
    best, idx = 0.0, 0
    for i, (bn, _, _) in enumerate(BASEAXIS):
        if dot(n, bn) > best + 0.0001:
            best, idx = dot(n, bn), i
    _, u, v = BASEAXIS[idx]
    u, v = list(u), list(v)
    sv = 0 if u[0] else (1 if u[1] else 2)
    tv = 0 if v[0] else (1 if v[1] else 2)
    sn, cs = {0: (0.0, 1.0), 90: (1.0, 0.0), 180: (0.0, -1.0), 270: (-1.0, 0.0)}.get(
        rot % 360, (math.sin(math.radians(rot)), math.cos(math.radians(rot))))
    for vec in (u, v):
        a, b = vec[sv], vec[tv]
        vec[sv], vec[tv] = cs * a - sn * b, sn * a + cs * b
    return [c / ss for c in u], [c / ts for c in v]


_size = {}


def tex_size(name):
    if name not in _size:
        _size[name] = (256, 256)
        for ext in (".jpg", ".tga", ".png"):
            p = BASE / "textures" / (name + ext)
            if p.exists():
                _size[name] = Image.open(p).size
                break
    return _size[name]


def tface(n, point, spec, detail):
    """One brush face. spec: material (scale 0.5) or dict(m=, s=(ss, ts), rot=, at=(point, (s, t)))."""
    pts = plane(n, point)
    if isinstance(spec, str):
        spec = {"m": spec}
    ss, ts = spec.get("s", (0.5, 0.5))
    rot = spec.get("rot", 0)
    xo = yo = 0.0
    if "at" in spec:
        a, (sa, ta) = spec["at"]
        u, v = texvecs(n, rot, ss, ts)
        w, h = tex_size(spec["m"])
        xo, yo = (sa - dot(a, u)) % w, (ta - dot(a, v)) % h
    return (pts, spec["m"], [num(xo), num(yo), num(rot), num(ss), num(ts), str(DETAIL if detail else 0), "0", "0"])


SIDES = {"+x": (1, 0, 0), "-x": (-1, 0, 0), "+y": (0, 1, 0), "-y": (0, -1, 0), "top": (0, 0, 1), "bot": (0, 0, -1)}


def fit(img, side, lo, hi):
    """Spec that shows the whole image once on a vertical box face, upright and readable."""
    n = SIDES[side]
    r = cross(tuple(-c for c in n), (0, 0, 1))           # right, as the viewer in front sees it
    u = (0, 1, 0) if abs(n[0]) > 0.5 else (1, 0, 0)       # q3map2 s axis of that face
    width = hi[1] - lo[1] if abs(n[0]) > 0.5 else hi[0] - lo[0]
    height = hi[2] - lo[2]
    c = tuple((a + b) / 2 for a, b in zip(lo, hi))
    face = tuple(hi[i] if n[i] > 0 else (lo[i] if n[i] < 0 else c[i]) for i in range(3))
    tl = (face[0] - r[0] * width / 2, face[1] - r[1] * width / 2, hi[2])
    w, h = tex_size(img)
    return {"m": img, "s": (math.copysign(width / w, dot(u, r)), height / h), "at": (tl, (0, 0))}


class Map:
    def __init__(self):
        self.world, self.ents = [], []

    def box(self, lo, hi, mat, faces=None, detail=True):
        lo, hi = tuple(map(min, lo, hi)), tuple(map(max, lo, hi))       # corners in any order
        assert all(h > l for l, h in zip(lo, hi)), (lo, hi)
        out = []
        for key, n in SIDES.items():
            p = tuple(hi[i] if n[i] > 0 else lo[i] for i in range(3))
            spec = (faces or {}).get(key, mat)
            if callable(spec):
                spec = spec(key, lo, hi)
            out.append(tface(n, p, spec, detail))
        self.world.append(out)
        return out

    def prism(self, pts, z0, z1, side, top=None, bottom=CAULK, detail=True):
        """Vertical prism over a convex polygon given counter-clockwise."""
        faces = []
        for i in range(len(pts)):
            (ax, ay), (bx, by) = pts[i], pts[(i + 1) % len(pts)]
            ln = math.hypot(by - ay, ax - bx)
            faces.append(tface(((by - ay) / ln, (ax - bx) / ln, 0), (ax, ay, z0), side, detail))
        faces.append(tface((0, 0, 1), (pts[0][0], pts[0][1], z1), top or side, detail))
        faces.append(tface((0, 0, -1), (pts[0][0], pts[0][1], z0), bottom, detail))
        self.world.append(faces)

    def hull(self, planes, mat, detail=True):
        """Brush from (normal, point) pairs."""
        self.world.append([tface(n, p, mat, detail) for n, p in planes])

    def entity(self, classname, x, y, z, **keys):
        kv = [["classname", classname], ["origin", f"{num(round(x, 2))} {num(round(y, 2))} {num(round(z, 2))}"]]
        self.ents.append({"kv": kv + [[k, str(v)] for k, v in keys.items()], "brushes": []})

    def trigger(self, classname, lo, hi, mat, **keys):
        b = []
        for key, n in SIDES.items():
            p = tuple(hi[i] if n[i] > 0 else lo[i] for i in range(3))
            b.append(tface(n, p, mat, False))
        self.ents.append({"kv": [["classname", classname]] + [[k, str(v)] for k, v in keys.items()], "brushes": [b]})

    def write(self, path, header, worldkv):
        out = [f"// {line}\n" for line in header]
        allents = [{"kv": [["classname", "worldspawn"]] + worldkv, "brushes": self.world}] + self.ents
        for i, e in enumerate(allents):
            out.append(f"// entity {i}\n{{\n")
            out += [f'"{k}" "{v}"\n' for k, v in e["kv"]]
            for j, b in enumerate(e["brushes"]):
                out.append(f"// brush {j}\n{{\n")
                for pts, tex, rest in b:
                    out.append(" ".join("( " + " ".join(num(c) for c in p) + " )" for p in pts) + f" {tex} {' '.join(rest)}\n")
                out.append("}\n")
            out.append("}\n")
        path.write_text("".join(out), newline="\n")
        return len(self.world)


# ================================================================= textures

def font(path, size):
    return ImageFont.truetype(path, size)


def text_center(d, box, text, f, fill):
    x0, y0, x1, y1 = box
    b = d.textbbox((0, 0), text, font=f)
    d.text(((x0 + x1 - (b[2] - b[0])) / 2 - b[0], (y0 + y1 - (b[3] - b[1])) / 2 - b[1]), text, font=f, fill=fill)


def fit_font(d, text, path, w, h):
    size = h
    while size > 8:
        f = font(path, size)
        b = d.textbbox((0, 0), text, font=f)
        if b[2] - b[0] <= w and b[3] - b[1] <= h:
            return f
        size -= 2
    return font(path, 8)


def noise(size, seed, scales=((64, 0.5), (16, 0.3), (4, 0.2))):
    rng = np.random.default_rng(seed)
    out = np.zeros((size, size), np.float32)
    for cells, amp in scales:
        small = rng.random((cells, cells)).astype(np.float32)
        tiled = np.tile(small, (3, 3))
        im = Image.fromarray((tiled * 255).astype(np.uint8)).resize((size * 3, size * 3), Image.BICUBIC)
        out += amp * np.asarray(im, np.float32)[size:2 * size, size:2 * size] / 255
    return out


def sign(text, path, bg=(18, 30, 58), fg=(250, 250, 245), border=(245, 196, 0), size=(512, 256)):
    im = Image.new("RGB", size, border)
    d = ImageDraw.Draw(im)
    w, h = size
    d.rectangle((12, 12, w - 13, h - 13), fill=bg)
    text_center(d, (24, 20, w - 24, h - 20), text, fit_font(d, text, FONT_BOLD, w - 64, int(h * 0.55)), fg)
    im.save(path, quality=92)


def make_textures():
    TEXDIR.mkdir(parents=True, exist_ok=True)
    for name, text in (("dist_60ft", "60 FT"), ("dist_330ft", "330 FT"), ("dist_660ft", "660 FT"),
                       ("dist_eighth", "1/8 MILE"), ("dist_1000ft", "1000 FT"), ("dist_quarter", "1/4 MILE")):
        sign(text, TEXDIR / f"{name}.jpg")
    sign("FINISH", TEXDIR / "sign_finish.jpg", bg=(170, 20, 24))
    sign("BURNOUT", TEXDIR / "sign_burnout.jpg", bg=(30, 30, 30))
    sign("STAGING LANES", TEXDIR / "sign_staging.jpg", bg=(30, 30, 30), size=(512, 128))

    # banners on the gantries
    for name, text, bg in (("banner_start", "Q3RALLY  DRAG  RACING", (20, 20, 24)),
                           ("banner_finish", "FINISH", (20, 20, 24))):
        im = Image.new("RGB", (1024, 128), bg)
        d = ImageDraw.Draw(im)
        for k in range(0, 1024, 16):                       # chequered borders
            for row in (0, 1):
                if (k // 16 + row) % 2 == 0:
                    d.rectangle((k, row * 12, k + 15, row * 12 + 11), fill=(240, 240, 240))
                    d.rectangle((k, 104 + row * 12, k + 15, 104 + row * 12 + 11), fill=(240, 240, 240))
        col = (245, 196, 0) if name == "banner_start" else (235, 40, 40)
        text_center(d, (20, 26, 1004, 102), text, fit_font(d, text, FONT_BOLD, 960, 70), col)
        im.save(TEXDIR / f"{name}.jpg", quality=92)

    # timing board: lane, reaction, elapsed time, finish speed
    im = Image.new("RGB", (1024, 512), (8, 8, 10))
    d = ImageDraw.Draw(im)
    d.rectangle((0, 0, 1023, 511), outline=(70, 70, 76), width=10)
    fl, fd = font(FONT_BOLD, 40), font(FONT_MONO, 92)
    for col, (x0, lane, c) in enumerate(((20, "LANE 1", (60, 160, 255)), (522, "LANE 2", (255, 70, 60)))):
        d.rectangle((x0, 20, x0 + 482, 90), fill=c)
        text_center(d, (x0, 20, x0 + 482, 90), lane, fl, (255, 255, 255))
        for row, (lab, val) in enumerate((("RT", "0.---"), ("ET", "--.---"), ("KM/H", "---.-"))):
            y = 115 + row * 128
            d.text((x0 + 14, y + 30), lab, font=font(FONT_BOLD, 34), fill=(200, 200, 200))
            d.text((x0 + 140, y), val, font=fd, fill=(255, 176, 30))
    im.save(TEXDIR / "board.jpg", quality=92)

    # rubber laid down by the burnouts and launches: dark, streaky along the lane (texture s axis)
    n = noise(256, 1, ((32, 0.4), (8, 0.3), (2, 0.3)))
    streak = np.asarray(Image.fromarray((noise(256, 2, ((256, 0.7), (64, 0.3))) * 255).astype(np.uint8))
                        .resize((16, 256)).resize((256, 256), Image.BICUBIC), np.float32) / 255
    v = 0.10 + 0.10 * n + 0.10 * streak
    Image.fromarray((np.dstack([v, v, v * 1.02]) * 255).clip(0, 255).astype(np.uint8)).save(TEXDIR / "rubber.jpg", quality=90)

    # runway concrete: slabs of 256 units (texture 512 px at scale 0.5) with dark joints and tyre marks
    n = noise(512, 3)
    v = 0.52 + 0.16 * (n - 0.5)
    v[:, :3] -= 0.18
    v[:3, :] -= 0.18
    v[:, 255:258] -= 0.12
    v[255:258, :] -= 0.12
    rgb = np.dstack([v * 0.98, v * 0.98, v * 0.95])
    Image.fromarray((rgb * 255).clip(0, 255).astype(np.uint8)).save(TEXDIR / "runway.jpg", quality=90)

    # flat paints
    for name, c in (("paint_yellow", (236, 186, 20)), ("paint_red", (190, 24, 28))):
        Image.new("RGB", (64, 64), c).save(TEXDIR / f"{name}.jpg", quality=95)

    # windsock: orange and white bands
    im = Image.new("RGB", (256, 64), (250, 250, 250))
    for k in range(0, 256, 64):
        ImageDraw.Draw(im).rectangle((k, 0, k + 31, 63), fill=(240, 100, 20))
    im.save(TEXDIR / "windsock.jpg", quality=92)

    # lamps: floodlight lens and runway light
    for name, c in (("lamp", (255, 248, 225)), ("lamp_amber", (255, 190, 90)), ("lamp_blue", (90, 150, 255))):
        im = Image.new("RGB", (64, 64), tuple(int(x * 0.75) for x in c))
        ImageDraw.Draw(im).ellipse((6, 6, 57, 57), fill=c)
        im.filter(ImageFilter.GaussianBlur(3)).save(TEXDIR / f"{name}.jpg", quality=95)

    # tent canvas and hangar door
    n = noise(256, 4, ((16, 0.6), (4, 0.4)))
    for name, c in (("canvas_red", (176, 36, 34)), ("canvas_white", (225, 222, 214)), ("canvas_blue", (34, 70, 150))):
        rgb = np.dstack([np.full_like(n, ch / 255) for ch in c]) * (0.92 + 0.12 * n[..., None])
        Image.fromarray((rgb * 255).clip(0, 255).astype(np.uint8)).save(TEXDIR / f"{name}.jpg", quality=90)


SHADER = r"""// Drag Race maps (tools/maptools/make_dragstrip.py). Generated: edit the script.

// runway concrete of the airfield: the same asphalt surface (grip) as every drag lane
textures/q3r_drag/runway
{
	qer_editorimage textures/q3r_drag/runway.jpg
	surfaceparm SURF_ASPHALT
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/q3r_drag/runway.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

// rubber from burnouts and launches: painted on, no collision, same surface
textures/q3r_drag/rubber
{
	qer_editorimage textures/q3r_drag/rubber.jpg
	surfaceparm SURF_ASPHALT
	surfaceparm nonsolid
	surfaceparm nomarks
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/q3r_drag/rubber.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_drag/paint_yellow
{
	qer_editorimage textures/q3r_drag/paint_yellow.jpg
	surfaceparm SURF_ASPHALT
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/q3r_drag/paint_yellow.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

textures/q3r_drag/paint_red
{
	qer_editorimage textures/q3r_drag/paint_red.jpg
	surfaceparm SURF_ASPHALT
	surfaceparm nonsolid
	{
		map $lightmap
		rgbGen identity
	}
	{
		map textures/q3r_drag/paint_red.jpg
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identity
	}
}

// spectators standing at the barriers (cut-out rows of people)
textures/q3r_drag/crowdrow
{
	qer_editorimage textures/q3r_people/peeprow01.tga
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	{
		map textures/q3r_people/peeprow01.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

textures/q3r_drag/crowdrow2
{
	qer_editorimage textures/q3r_people/peeprow02.tga
	surfaceparm nonsolid
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm alphashadow
	cull none
	{
		map textures/q3r_people/peeprow02.tga
		alphaFunc GE128
		depthWrite
		rgbGen vertex
	}
}

// timing board: lit from inside
textures/q3r_drag/board
{
	qer_editorimage textures/q3r_drag/board.jpg
	surfaceparm nomarks
	surfaceparm nolightmap
	q3map_surfacelight 40
	{
		map textures/q3r_drag/board.jpg
		rgbGen identity
	}
}

// lamp lenses (the light itself comes from light entities)
textures/q3r_drag/lamp
{
	qer_editorimage textures/q3r_drag/lamp.jpg
	surfaceparm nomarks
	surfaceparm nolightmap
	{
		map textures/q3r_drag/lamp.jpg
		rgbGen identity
	}
}

textures/q3r_drag/lamp_amber
{
	qer_editorimage textures/q3r_drag/lamp_amber.jpg
	surfaceparm nomarks
	surfaceparm nolightmap
	{
		map textures/q3r_drag/lamp_amber.jpg
		rgbGen identity
	}
}

textures/q3r_drag/lamp_blue
{
	qer_editorimage textures/q3r_drag/lamp_blue.jpg
	surfaceparm nomarks
	surfaceparm nolightmap
	{
		map textures/q3r_drag/lamp_blue.jpg
		rgbGen identity
	}
}

// invisible solid hull (the vehicle physics only takes solid brushes)
textures/q3r_drag/clip
{
	qer_trans 0.15
	surfaceparm nodraw
	surfaceparm nolightmap
	surfaceparm trans
	surfaceparm nomarks
	surfaceparm detail
}
"""


# ================================================================= the strip (identical on every map)

def digit_boxes(ch, cx, cy, h, up=(1, 0), right=(0, -1)):
    """Rectangles (x0, x1, y0, y1) of a painted seven-segment character, read looking along `up`."""
    seg = {"a": (0, 1, 1.84, 2), "b": (0.84, 1, 1, 2), "c": (0.84, 1, 0, 1), "d": (0, 1, 0, 0.16),
           "e": (0, 0.16, 0, 1), "f": (0, 0.16, 1, 2), "g": (0, 1, 0.92, 1.08)}
    lit = {"0": "abcdef", "1": "bc", "2": "abged", "3": "abgcd", "4": "fgbc", "5": "afgcd", "6": "afgedc",
           "7": "abc", "8": "abcdefg", "9": "abcdfg"}[ch]
    s = h / 2
    out = []
    for k in lit:
        u0, u1, v0, v1 = seg[k]
        pts = []
        for uu, vv in ((u0 - 0.5, v0 - 1), (u1 - 0.5, v1 - 1)):
            pts.append((cx + (uu * right[0] + vv * up[0]) * s, cy + (uu * right[1] + vv * up[1]) * s))
        out.append((min(p[0] for p in pts), max(p[0] for p in pts), min(p[1] for p in pts), max(p[1] for p in pts)))
    return out


def paint(m, x0, x1, y0, y1, mat=PAINT, z=1):
    m.box((x0, y0, 0), (x1, y1, z), {"m": mat, "s": (0.25, 0.25)}, faces={"bot": CAULK})


def strip(m, finish, runoff, wall, wall_top=None, road=ROAD, gantry=METAL, post="q3r_road/cement_redwhite",
          markers=True):
    """The drag strip: road, walls, paint, start and finish gantries, gates, spawns.
    wall: face spec for the walls; finish: X of the finish plane; runoff: X of the end wall."""
    W = WALL_Y
    m.box((BACK_X, -W, -16), (runoff, W, 0), CAULK, faces={"top": {"m": road, "s": (0.5, 0.5)}}, detail=False)
    for s in (1, -1):
        lo, hi = (BACK_X - WALL_T, W, 0), (runoff + WALL_T, W + WALL_T, 56)
        if s < 0:
            lo, hi = (lo[0], -hi[1], 0), (hi[0], -lo[1], 56)
        m.box(lo, hi, wall, faces={"top": wall_top or wall, "bot": CAULK}, detail=False)
    m.box((BACK_X - WALL_T, -W, 0), (BACK_X, W, 56), wall, faces={"bot": CAULK}, detail=False)
    m.box((runoff, -W, 0), (runoff + WALL_T, W, 96), wall, faces={"bot": CAULK}, detail=False)
    # crash barrier in front of the end wall
    m.box((runoff - 96, -W, 0), (runoff, W, 64), {"m": "q3r_metals/cormetal02", "s": (0.5, 0.5)}, faces={"bot": CAULK})

    # paint: lane edges, centre line, start line, chequered finish, lane numbers, 60 ft ticks
    paint(m, BACK_X, runoff - 96, -324, -316)
    paint(m, BACK_X, runoff - 96, 316, 324)
    paint(m, 104, finish, -4, 4)
    paint(m, 92, 108, -320, 320)
    paint(m, -40, -32, -320, 320, mat=TEX + "paint_yellow")                 # pre-stage line
    for k in range(-320, 320, 32):
        for row in (0, 1):
            if (k // 32 + row) % 2 == 0:
                paint(m, finish - 32 + row * 32, finish + row * 32, k, k + 32)
    for lane, ch in ((-160, "1"), (160, "2")):
        for x0, x1, y0, y1 in digit_boxes(ch, -300, lane, 160):
            paint(m, x0, x1, y0, y1)
    tick = round(60 * UNITS_PER_FT)
    for s in (1, -1):
        paint(m, tick - 8, tick + 8, s * 300, s * 316)
    # rubber: burnout box and the launch marks of each lane
    for lane in (-160, 160):
        m.box((-700, lane - 70, 0), (-260, lane + 70, 0.5), {"m": TEX + "rubber", "s": (0.5, 0.5)}, faces={"bot": CAULK})
        for side in (-1, 1):
            y = lane + side * 26
            m.box((0, y - 9, 0), (900, y + 9, 0.5), {"m": TEX + "rubber", "s": (0.5, 0.5)}, faces={"bot": CAULK})
            m.box((900, y - 7, 0), (2400, y + 7, 0.5), {"m": TEX + "rubber", "s": (1, 0.5)}, faces={"bot": CAULK})

    # start gantry: posts, beam, banner and the start lights of the prototype (docs/drag-race-map.md)
    for s in (1, -1):
        m.box((88, min(s * 360, s * 376), 0), (112, max(s * 360, s * 376), 256), post)
        m.box((finish - 12, min(s * 360, s * 376), 0), (finish + 12, max(s * 360, s * 376), 256), post)
    m.box((88, -376, 256), (112, 376, 280), "base_floor/concrete")
    banner = (92, -300, 280), (108, 300, 344)
    m.box(*banner, CAULK, faces={"-x": fit(TEX + "banner_start", "-x", *banner), "+x": fit(TEX + "banner_start", "+x", *banner),
                                 "top": "base_floor/concrete", "+y": "base_floor/concrete", "-y": "base_floor/concrete"})
    for y in (-179, -141, 141, 179):
        m.box((89, y - 1, 192), (91, y + 1, 256), gantry)
    for y in (-160, 160):
        m.entity("misc_model", 84, y, 168, model="models/mapobjects/startlight/startlight.md3", angle=180, _cs=1)
        m.entity("rally_startlight", 84, y, 168, angle=180)
    m.box((finish - 12, -376, 256), (finish + 12, 376, 280), "base_floor/concrete")
    banner = (finish - 8, -300, 280), (finish + 8, 300, 344)
    m.box(*banner, CAULK, faces={"-x": fit(TEX + "banner_finish", "-x", *banner), "+x": fit(TEX + "banner_finish", "+x", *banner),
                                 "top": "base_floor/concrete", "+y": "base_floor/concrete", "-y": "base_floor/concrete"})

    # distance boards behind the walls, both sides
    if markers:
        marks = [(60, "dist_60ft"), (330, "dist_330ft")]
        marks += [(660, "dist_eighth"), (1000, "dist_1000ft")] if finish == QUARTER else []
        for ft, img in marks:
            x = round(ft * UNITS_PER_FT)
            board_post(m, x, img)
        board_post(m, finish, "sign_finish")
        board_post(m, -520, "sign_burnout", h=96, z0=72)

    # gates, spawns
    m.trigger("rally_start", (96, -320, -8), (104, 320, 192), "common/startfinish", angle=0)
    half = finish // 2
    m.trigger("rally_checkpoint", (half - 4, -320, -8), (half + 4, 320, 192), "common/checkpoint",
              number=1, angle=0, spawnflags=4)
    m.trigger("rally_finish", (finish - 4, -320, -8), (finish + 4, 320, 192), "common/startfinish", angle=0)
    for n, y in ((1, -160), (2, 160)):
        m.entity("info_player_start", 0, y, 28, number=n, angle=0, nobots=1)


def board_post(m, x, img, h=128, z0=88):
    """Sign on two posts just outside each wall, facing the approaching cars."""
    for s in (1, -1):
        y0, y1 = WALL_Y + WALL_T + 16, WALL_Y + WALL_T + 16 + 2 * h
        lo, hi = (x - 4, min(s * y0, s * y1), z0), (x + 4, max(s * y0, s * y1), z0 + h)
        m.box(lo, hi, METAL, faces={"-x": fit(TEX + img, "-x", lo, hi), "+x": fit(TEX + img, "+x", lo, hi)})
        for yy in (y0 + 16, y1 - 16):
            m.box((x + 4, s * yy - 4 if s > 0 else -yy - 4, 0), (x + 12, s * yy + 4 if s > 0 else -yy + 4, z0), METAL)


def skybox(m, x0, x1, y0, y1, top, sky):
    t = 16
    m.box((x0 - t, y0 - t, top), (x1 + t, y1 + t, top + t), sky, detail=False)
    m.box((x0 - t, y0 - t, -64), (x1 + t, y0, top), sky, detail=False)
    m.box((x0 - t, y1, -64), (x1 + t, y1 + t, top), sky, detail=False)
    m.box((x0 - t, y0, -64), (x0, y1, top), sky, detail=False)
    m.box((x1, y0, -64), (x1 + t, y1, top), sky, detail=False)


def ground(m, x0, x1, y0, y1, runoff, mat, scale=1.0):
    """Ground slab around the strip (the strip's own slab covers the road)."""
    g = {"m": mat, "s": (scale, scale)}
    W = WALL_Y
    for lo, hi in (((x0, W, -16), (x1, y1, 0)), ((x0, y0, -16), (x1, -W, 0)),
                   ((x0, -W, -16), (BACK_X, W, 0)), ((runoff, -W, -16), (x1, W, 0))):
        m.box(lo, hi, CAULK, faces={"top": g}, detail=False)


def grandstand(m, x0, x1, side, y_front, rows=9, rise=40, tread=56, crowd=True, roof=True):
    """Stepped grandstand along the strip; the people are the crowd texture on the risers."""
    s = side
    seat = {"m": "q3r_building/concrete02", "s": (0.25, 0.25)}
    people = {"m": "q3r_people/crowd02", "s": (0.3, rise / 256)}
    for r in range(rows):
        ya, yb = y_front + r * tread, y_front + rows * tread
        lo, hi = (x0, min(s * ya, s * yb), 0), (x1, max(s * ya, s * yb), (r + 1) * rise)
        front = "-y" if s > 0 else "+y"
        m.box(lo, hi, seat, faces={front: people if crowd and r > 0 else seat, "bot": CAULK})
    depth = rows * tread
    back = y_front + depth
    m.box((x0, min(s * back, s * (back + 16)), 0), (x1, max(s * back, s * (back + 16)), rows * rise + 160), seat,
          faces={"bot": CAULK})
    for x in (x0, x1 - 16):
        m.box((x, min(s * y_front, s * back), 0), (x + 16, max(s * y_front, s * back), rows * rise + 24), seat,
              faces={"bot": CAULK})
    if roof:
        top = rows * rise + 220
        for x in range(int(x0), int(x1), 512):
            m.box((x, min(s * (y_front + 8), s * (y_front + 20)), 0), (x + 12, max(s * (y_front + 8), s * (y_front + 20)), top),
                  METAL, faces={"bot": CAULK})
        m.box((x0 - 32, min(s * (y_front - 40), s * (back + 16)), top), (x1 + 32, max(s * (y_front - 40), s * (back + 16)), top + 16),
              {"m": "q3r_metals/cormetal02", "s": (0.5, 0.5)})


def timing_board(m, x, side, y, z0=96, w=640, h=320):
    """Lit timing board on legs, facing the track."""
    s = side
    lo, hi = (x - w / 2, min(s * y, s * (y + 24)), z0), (x + w / 2, max(s * y, s * (y + 24)), z0 + h)
    face = "-y" if s > 0 else "+y"
    m.box(lo, hi, METAL, faces={face: fit(TEX + "board", face, lo, hi)})
    for xx in (x - w / 2 + 40, x + w / 2 - 56):
        m.box((xx, min(s * (y + 4), s * (y + 20)), 0), (xx + 16, max(s * (y + 4), s * (y + 20)), z0), METAL)


def light_mast(m, x, y, aim_y, h=640, color="1 0.97 0.9", power=1600):
    """Floodlight mast with a lamp bank; light entities just in front of it."""
    m.box((x - 10, y - 10, 0), (x + 10, y + 10, h), METAL)
    d = 1 if aim_y > y else -1
    m.box((x - 96, min(y, y + d * 24), h), (x + 96, max(y, y + d * 24), h + 64), METAL,
          faces={("+y" if d > 0 else "-y"): {"m": TEX + "lamp", "s": (0.5, 0.5)}})
    for dx in (-60, 60):
        m.entity("light", x + dx, y + d * 64, h + 20, light=power, _color=color)


# ================================================================= scenery helpers

def mesa(m, cx, cy, r, h, rnd, mat):
    """Rock butte: two stacked irregular convex prisms."""
    k = rnd.randint(6, 9)
    pts = []
    for i in range(k):
        a = 2 * math.pi * i / k + rnd.uniform(-0.2, 0.2)
        rr = r * rnd.uniform(0.8, 1.15)
        pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    spec = {"m": mat, "s": (2.0, h / 512 * 1.2)}
    m.prism(pts, -16, h * 0.6, spec, top={"m": "q3r_dirt_grass/dirt02", "s": (2, 2)}, detail=False)
    pts2 = [(cx + (x - cx) * 0.72, cy + (y - cy) * 0.72) for x, y in pts]
    m.prism(pts2, h * 0.6, h, spec, top={"m": "q3r_dirt_grass/dirt02", "s": (2, 2)}, detail=False)


def building(m, x0, x1, y0, y1, h, facade, sc, top="q3r_building/roof", hidden=()):
    faces = {k: {"m": facade, "s": (-sc if k in ("+y", "-x") else sc, sc), "at": ((x0, y0, 0), (0, 0))}
             for k in ("+x", "-x", "+y", "-y")}
    faces.update({"top": {"m": top, "s": (1, 1)}, "bot": CAULK})
    for k in hidden:
        faces[k] = CAULK
    m.box((x0, y0, 0), (x1, y1, h), CAULK, faces=faces, detail=False)
    m.box((x0 - 6, y0 - 6, h), (x1 + 6, y1 + 6, h + 14), {"m": "q3r_building/concrete02", "s": (0.5, 0.5)},
          faces={"bot": CAULK})


FACADES = (("q3r_building/externalwall50a", 4), ("q3r_building/externalwall51a", 5),
           ("q3r_building/externalwall52a", 3), ("q3r_building/externalwall56a", 2),
           ("q3r_building/externalwall68a_", 5), ("q3r_building/externalwall72_", 3),
           ("q3r_building/externalwall75_256x256", 5), ("q3r_building/externalwall33-1a", 3),
           ("q3r_building/externalwall33-2a", 3), ("q3r_building/externalwall46b_a", 3))
STORY = 144


def facade_scale(name, storeys):
    return STORY * storeys / tex_size(name)[1]


def worldkv(message, sky_note, extra=()):
    return [["message", message], ["author", "Q3Rally Team"], ["_lightmapscale", "4"],
            ["gridsize", "256 256 256"], ["weather", "none"]] + list(extra)


# ================================================================= the three venues

def desert():
    """Desert Dragway: quarter mile in the desert, grandstands, paddock, buttes, a blimp."""
    m = Map()
    rnd = random.Random(1320)
    X0, X1, Y0, Y1 = -3584, 21504, -4608, 4608
    runoff = 19072
    wall = {"m": "q3r_road/cement_redwhite", "s": (0.5, 0.5)}
    strip(m, QUARTER, runoff, wall)
    skybox(m, X0, X1, Y0, Y1, 3584, "skies/ame_desert")
    ground(m, X0, X1, Y0, Y1, runoff, "q3r_dirt_grass/dirt02", 1.5)
    # service road along the south side and the paddock behind the start
    m.box((-3000, -1500, 0), (20200, -1260, 1), {"m": "q3r_road/asph02", "s": (0.5, 0.5)}, faces={"bot": CAULK}, detail=False)
    m.box((-3000, -1260, 0), (-1000, 1600, 1), {"m": "q3r_road/asph02", "s": (0.5, 0.5)}, faces={"bot": CAULK}, detail=False)
    # grandstands at the start, both sides
    for s in (1, -1):
        grandstand(m, -400, 2600, s, 600)
        grandstand(m, 3000, 4600, s, 600, rows=6, roof=False)
        timing_board(m, QUARTER + 900, s, 640)
    # spectators standing at the fence in front of the stands
    for s in (1, -1):
        for x in range(-400, 4600, 512):
            m.box((x, s * 470 - 1, 0), (x + 512, s * 470 + 1, 64), TEX + ("crowdrow" if x % 1024 else "crowdrow2"))
    # timing tower at the finish (south side)
    tw = (QUARTER - 300, -1180, 0), (QUARTER + 300, -760, 520)
    m.box(*tw, {"m": "q3r_building/concrete02", "s": (0.5, 0.5)}, faces={"bot": CAULK}, detail=False)
    m.box((QUARTER - 280, -800, 300), (QUARTER + 280, -752, 500), {"m": "q3r_glass/darkglass", "s": (0.5, 0.5)})
    m.box((QUARTER - 330, -1210, 520), (QUARTER + 330, -730, 552), {"m": "q3r_building/concrete02", "s": (0.5, 0.5)})
    lo, hi = (QUARTER - 260, -752, 560), (QUARTER + 260, -736, 700)
    m.box(lo, hi, METAL, faces={"+y": fit("q3r_sponsors/q3rlong01", "+y", lo, hi)})
    # paddock: transporters and pickups, tents, the water tower
    trucks = ("vehicles/truck_flatbed", "vehicles/flatbed", "vehicles/pickup")
    for k in range(9):
        x = -2850 + (k % 3) * 520
        y = -900 + (k // 3) * 760
        m.entity("misc_model", x, y, 1, model=f"models/mapobjects/{trucks[k % 3]}.md3", angle=rnd.choice((0, 90, 180)),
                 modelscale=2.2 if k % 3 == 0 else 1.6)
    for k, (x, y) in enumerate(((-1700, -700), (-1700, 300), (-1700, 1200))):
        tent(m, x, y, 320, 240, ("canvas_red", "canvas_white", "canvas_blue")[k])
    m.entity("misc_model", -2500, 2600, 192, model="models/mapobjects/watertower/tower.md3", modelscale=1.4)
    # floodlights on the paddock and at the start
    for x, y in ((-2300, -1200), (-2300, 1500), (600, 1500), (600, -1500)):
        light_mast(m, x, y, 0)
    # buttes on the horizon
    for k in range(26):
        side = 1 if k % 2 else -1
        x = rnd.uniform(X0 + 1500, X1 - 1500)
        y = side * rnd.uniform(3000, Y1 - 700)
        mesa(m, x, y, rnd.uniform(380, 820), rnd.uniform(500, 1500), rnd, "q3r_canyonrush/strata")
    for k in range(5):
        mesa(m, X1 - rnd.uniform(600, 1100), rnd.uniform(-3200, 3200), rnd.uniform(400, 700), rnd.uniform(700, 1700), rnd,
             "q3r_canyonrush/strata")
    # cactuses in the open desert (not on the stands, roads or paddock)
    n = 0
    while n < 70:
        x, y = rnd.uniform(-3200, 21000), rnd.choice((1, -1)) * rnd.uniform(700, 2900)
        if (-3000 < x < -1000 and -1500 < y < 1600) or (-500 < x < 4700 and abs(y) < 1400) or -1520 < y < -1240:
            continue
        if QUARTER - 400 < x < QUARTER + 1300 and abs(y) < 1300:
            continue
        m.entity("misc_model", x, y, 0, model="models/mapobjects/cactus/cactus.md3", angle=rnd.randint(0, 359),
                 modelscale=round(rnd.uniform(1.6, 3.2), 2))
        n += 1
    m.entity("misc_model", 7600, 2200, 1900, model="models/mapobjects/blimp/blimp.md3", angle=90, modelscale=2)
    m.entity("info_player_intermission", -480, -320, 220, angles="12 20 0")
    m.entity("rally_sun", 0, 0, 1024, angles="35 230 0")
    return m, worldkv("Desert Dragway", "ame_desert")


def tent(m, x, y, w, d, canvas):
    c = {"m": TEX + canvas, "s": (0.5, 0.5)}
    m.box((x - w / 2, y - d / 2, 0), (x + w / 2, y + d / 2, 120), c, faces={"bot": CAULK})
    m.hull([((0, 0, -1), (x, y, 120)), ((-1, 0, 0), (x - w / 2 - 12, y, 0)), ((1, 0, 0), (x + w / 2 + 12, y, 0)),
            ((0, -0.6, 0.8), (x, y - d / 2 - 12, 120)), ((0, 0.6, 0.8), (x, y + d / 2 + 12, 120))], c)


def night():
    """Night Drag: an eighth mile on a closed-off avenue in the city at night."""
    m = Map()
    rnd = random.Random(660)
    X0, X1, Y0, Y1 = -3200, 13200, -2304, 2304
    runoff = 10624
    finish = EIGHTH
    wall = {"m": "q3r_building/concrete02", "s": (0.25, 0.25)}
    stripe = {"m": "q3r_road/cement_redwhite", "s": (0.5, 0.5)}
    strip(m, finish, runoff, wall, wall_top=stripe)
    skybox(m, X0, X1, Y0, Y1, 2816, "skies/ely_darkcity")
    ground(m, X0, X1, Y0, Y1, runoff, "q3r_road/asph02", 0.5)
    # sidewalks with kerbs
    for s in (1, -1):
        y0, y1 = WALL_Y + WALL_T, 760
        m.box((X0, min(s * y0, s * y1), 0), (X1, max(s * y0, s * y1), 8), {"m": "q3r_road/cobbles01", "s": (0.5, 0.5)},
              faces={"bot": CAULK}, detail=False)
    # building rows on both sides, behind the start and across the end of the avenue
    for s in (1, -1):
        x = X0
        while x < X1 - 1:
            w = rnd.choice((512, 640, 768, 896))
            w = min(w, X1 - x)
            if X1 - x - w < 300:
                w = X1 - x
            fac, st = rnd.choice(FACADES)
            h = rnd.choice((3, 4, 5, 6, 7, 8)) * STORY
            setback = rnd.choice((0, 0, 32, 64))
            y0, y1 = 800 + setback, Y1
            building(m, x, x + w, min(s * y0, s * y1), max(s * y0, s * y1), h, fac, facade_scale(fac, st),
                     hidden=("+y" if s > 0 else "-y",))
            # neon sign or lit shop front at street level on some
            if rnd.random() < 0.55:
                sign_img = rnd.choice(("q3r_rooftop/sign_nitro", "q3r_rooftop/sign_q3rally", "q3r_rooftop/ledscreen"))
                ya = s * (y0 - 12)
                lo, hi = (x + w / 2 - 160, min(ya, ya + s * 12), 200), (x + w / 2 + 160, max(ya, ya + s * 12), 280)
                face = "-y" if s > 0 else "+y"
                m.box(lo, hi, METAL, faces={face: fit(sign_img, face, lo, hi)})
            x += w
    for x0, x1 in ((X0, -1800), (12000, X1)):                # the avenue ends in buildings
        fac, st = rnd.choice(FACADES)
        building(m, x0, x1, -760, 760, 6 * STORY, fac, facade_scale(fac, st))
    # street lamps on the sidewalks and floodlights at start and finish
    k = 0
    for x in range(-1600, 11800, 512):
        s = 1 if k % 2 else -1
        k += 1
        m.entity("misc_model", x, s * 600, 8, model="models/mapobjects/q3r_lanterns/streetlamp.md3", angle=270 if s > 0 else 90)
        m.entity("light", x, s * 512, 298, light=420, _color="1 0.72 0.38")
    for x in (-200, finish - 200):
        for s in (1, -1):
            light_mast(m, x, s * 700, 0, h=560, color="0.85 0.9 1", power=2200)
    # spectators along the barriers: two standing rows on each sidewalk up to the finish
    for s in (1, -1):
        for x in range(-640, finish + 640, 512):
            for row, (yy, mat) in enumerate(((440, "crowdrow"), (500, "crowdrow2"))):
                ya = s * yy
                m.box((x, ya - 1, 8), (x + 512, ya + 1, 72), TEX + mat, detail=True)
        timing_board(m, finish + 700, s, 640, z0=140)
    # overpass across the avenue in the runoff
    deck = {"m": "q3r_building/concrete02", "s": (0.5, 0.5)}
    m.box((9000, -2304, 520), (9400, 2304, 600), deck)
    for s in (1, -1):
        for x in (9040, 9300):
            m.box((x, min(s * 640, s * 720), 8), (x + 60, max(s * 640, s * 720), 520), deck, faces={"bot": CAULK})
    lo, hi = (8988, -900, 520), (9000, 900, 600)
    m.box(lo, hi, METAL, faces={"-x": fit("q3r_rooftop/ledscreen", "-x", lo, hi)})
    m.box((9000, -2304, 600), (9012, 2304, 640), METAL)
    m.box((9388, -2304, 600), (9400, 2304, 640), METAL)
    m.entity("info_player_intermission", -480, -320, 220, angles="10 20 0")
    m.entity("rally_sun", 0, 0, 1024, angles="45 230 0")
    return m, worldkv("Night Drag", "ely_darkcity", (["_minlight", "6"],))


def airfield():
    """Airfield Drag: quarter mile on the runway, hangars, tower, apron, at sunset."""
    m = Map()
    rnd = random.Random(90)
    X0, X1, Y0, Y1 = -3840, 21760, -4608, 3072
    runoff = 19072
    finish = QUARTER
    rw = {"m": TEX + "runway", "s": (0.5, 0.5)}
    blocks = {"m": "q3r_building/concrete02", "s": (0.25, 0.25)}
    strip(m, finish, runoff, blocks, wall_top={"m": TEX + "paint_red", "s": (0.25, 0.25)}, road=TEX + "runway")
    skybox(m, X0, X1, Y0, Y1, 3328, "skies/ely_sunset")
    ground(m, X0, X1, Y0, Y1, runoff, "q3r_dirt_grass/grass07", 1.5)
    # runway beyond the drag lanes, threshold bars and the runway number, edge lights
    for s in (1, -1):
        m.box((-3200, min(s * (WALL_Y + WALL_T), s * 900), 0), (20800, max(s * (WALL_Y + WALL_T), s * 900), 1), rw,
              faces={"bot": CAULK}, detail=False)
    m.box((-3200, -WALL_Y, 0), (BACK_X - WALL_T, WALL_Y, 1), rw, faces={"bot": CAULK}, detail=False)
    m.box((runoff + WALL_T, -WALL_Y, 0), (20800, WALL_Y, 1), rw, faces={"bot": CAULK}, detail=False)
    for k in range(-7, 8):
        if k == 0:
            continue
        y = k * 110
        paint(m, -3100, -2700, y - 36, y + 36, z=2)
    for ch, y in (("0", 180), ("9", -180)):
        for x0, x1, y0, y1 in digit_boxes(ch, -2200, y, 560):
            paint(m, x0, x1, y0, y1, z=2)
    for s in (1, -1):
        paint(m, -3200, 20800, s * 870 - 12 if s > 0 else -882, s * 870 + 12 if s > 0 else -858, z=2)
        for x in range(-3100, 20800, 600):
            m.box((x - 8, s * 930 - 8, 0), (x + 8, s * 930 + 8, 18), METAL,
                  faces={"top": {"m": TEX + ("lamp_amber" if x > 17000 else "lamp"), "s": (0.25, 0.25)}})
    for x in range(-3000, 20800, 1200):                          # runway centre line beyond the walls
        if BACK_X - 200 < x < runoff + 200:
            continue
        paint(m, x, x + 600, -8, 8, z=2)
    # taxiway and apron
    m.box((-2400, -1900, 0), (20000, -1500, 1), rw, faces={"bot": CAULK}, detail=False)
    paint(m, -2400, 20000, -1708, -1692, mat=TEX + "paint_yellow", z=2)
    for x in (1500, 9800, 17500):                                 # connectors to the runway
        m.box((x, -1500, 0), (x + 400, -900, 1), rw, faces={"bot": CAULK}, detail=False)
    m.box((-2400, -4400, 0), (12000, -1900, 1), rw, faces={"bot": CAULK}, detail=False)
    # hangars along the apron
    for k, x in enumerate((-1600, 1200, 4000)):
        hangar(m, x, x + 2200, -4300, -3000)
    # control tower
    tower(m, 8200, -3200)
    # fuel tanks
    for k, (x, y) in enumerate(((10400, -3900), (10900, -3900))):
        oct_ = [(x + 180 * math.cos(math.radians(22.5 + 45 * i)), y + 180 * math.sin(math.radians(22.5 + 45 * i))) for i in range(8)]
        m.prism(oct_, 0, 420, {"m": "q3r_metals/plate01", "s": (0.5, 0.5)}, top={"m": "q3r_metals/plate01", "s": (0.5, 0.5)})
    # vehicles on the apron
    for k in range(7):
        m.entity("misc_model", rnd.uniform(-1500, 6000), rnd.uniform(-2850, -2100), 1,
                 model=f"models/mapobjects/{('vehicles/truck_flatbed', 'vehicles/pickup', 'vehicles/flatbed')[k % 3]}.md3",
                 angle=rnd.choice((0, 90, 180, 270)), modelscale=2.2 if k % 3 == 0 else 1.6)
    # spectators on bleachers north of the strip and floodlights
    grandstand(m, -400, 3400, 1, 1000, rows=7, roof=False)
    for x in range(-400, 3400, 512):
        m.box((x, 958, 1), (x + 512, 960, 65), TEX + "crowdrow")
    for s in (1, -1):
        timing_board(m, finish + 900, s, 1000)
    for x in (-600, 3800):
        light_mast(m, x, 1500, 0)
    # windsock
    m.box((5000, 1300, 0), (5012, 1312, 420), METAL)
    m.hull([((0, 0, 1), (5000, 1306, 420)), ((0, 0, -1), (5000, 1306, 360)), ((0, 1, 0), (5000, 1340, 0)),
            ((0, -1, 0), (5000, 1272, 0)), ((-1, 0, 0), (5012, 1306, 0)),
            ((0.18, 0, 0.98), (5300, 1306, 405)), ((0.18, 0, -0.98), (5300, 1306, 375))],
           {"m": TEX + "windsock", "s": (1.2, 1)})
    m.entity("misc_model", 9000, 1800, 1500, model="models/mapobjects/blimp/blimp.md3", angle=0, modelscale=2)
    m.entity("info_player_intermission", -480, -320, 220, angles="12 20 0")
    m.entity("rally_sun", 0, 0, 1024, angles="10 350 0")
    return m, worldkv("Airfield Drag", "ely_sunset")


def hangar(m, x0, x1, y0, y1, h=560):
    """Hangar with an open door towards the runway (+y) and a sloped roof."""
    wallm = {"m": "q3r_metals/cormetal02", "s": (0.5, 0.5)}
    t = 24
    m.box((x0, y0, 0), (x1, y0 + t, h), wallm, faces={"bot": CAULK})                  # back
    m.box((x0, y0, 0), (x0 + t, y1, h), wallm, faces={"bot": CAULK})                  # sides
    m.box((x1 - t, y0, 0), (x1, y1, h), wallm, faces={"bot": CAULK})
    m.box((x0, y1 - t, h - 140), (x1, y1, h), wallm, faces={"bot": CAULK})            # door lintel
    m.box((x0, y1 - t, 0), (x0 + 360, y1, h - 140), wallm, faces={"bot": CAULK})       # door leaves pushed aside
    m.box((x1 - 360, y1 - t, 0), (x1, y1, h - 140), wallm, faces={"bot": CAULK})
    mid = (y0 + y1) / 2
    m.hull([((0, 0, -1), (x0, mid, h)), ((-1, 0, 0), (x0 - 16, mid, h)), ((1, 0, 0), (x1 + 16, mid, h)),
            ((0, -0.33, 0.94), (x0, y0 - 16, h)), ((0, 0.33, 0.94), (x0, y1 + 16, h))], wallm)
    for x in range(int(x0) + 300, int(x1) - 200, 520):
        m.entity("light", x, mid, h - 80, light=500, _color="1 0.95 0.85")
    m.entity("misc_model", (x0 + x1) / 2, mid, 1, model="models/mapobjects/vehicles/truck_flatbed.md3", angle=90, modelscale=2.4)


def tower(m, x, y):
    conc = {"m": "q3r_building/concrete02", "s": (0.5, 0.5)}
    m.box((x - 120, y - 120, 0), (x + 120, y + 120, 720), conc, faces={"bot": CAULK}, detail=False)
    m.box((x - 220, y - 220, 720), (x + 220, y + 220, 760), conc)
    glass = {"m": "q3r_glass/darkglass", "s": (0.5, 0.5)}
    m.box((x - 200, y - 200, 760), (x + 200, y + 200, 900), glass, faces={"top": conc})
    m.box((x - 240, y - 240, 900), (x + 240, y + 240, 940), conc)
    m.box((x - 6, y - 6, 940), (x + 6, y + 6, 1200), METAL)
    m.box((x - 14, y - 14, 1200), (x + 14, y + 14, 1224), {"m": TEX + "lamp_amber", "s": (0.25, 0.25)})
    m.entity("light", x, y, 1260, light=300, _color="1 0.5 0.2")


VENUES = {
    "desert": ("q3r_dragstrip", "Desert Dragway", desert, "1/4 mile",
               "Quarter-mile dragway in the desert: grandstands, paddock, buttes on the horizon."),
    "night": ("q3r_drag_night", "Night Drag", night, "1/8 mile",
              "Eighth-mile sprint on a closed-off city avenue at night."),
    "airfield": ("q3r_drag_airfield", "Airfield Drag", airfield, "1/4 mile",
                 "Quarter mile on the runway of an airfield at sunset."),
}


def arena(name, title):
    return f"""{{
map "{name}"
longname "{title}"
author "Q3Rally Team"
type "q3r_drag"
starts "2"
laps "1"
laptime ""
checkpoints "1"
observerspots "1"
weapons "0"
powerups "0"
reversable "0"
tracklengths "1"
botsupport "No"
teams "None"
}}
"""


def main(which):
    make_textures()
    (BASE / "scripts" / "q3r_drag.shader").write_text(SHADER, newline="\n")
    lst = BASE / "scripts" / "shaderlist.txt"
    names = lst.read_text().split()
    if "q3r_drag" not in names:
        lst.write_text(lst.read_text().rstrip("\n") + "\nq3r_drag\n", newline="\n")
    for key in which:
        name, title, fn, dist, _ = VENUES[key]
        m, kv = fn()
        n = m.write(BASE / "maps" / f"{name}.map",
                    [f"{title} ({dist}), generated by tools/maptools/make_dragstrip.py: edit the script, not this file.",
                     "Drag contract: docs/drag-race-map.md (+X, start X=100, lanes Y=+-160, finish gate 640 wide)."], kv)
        (BASE / "scripts" / f"{name}.arena").write_text(arena(name, title), newline="\n")
        print(f"{name}: {n} brushes, {len(m.ents)} entities")


if __name__ == "__main__":
    main(sys.argv[1:] or list(VENUES))
