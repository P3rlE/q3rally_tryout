#!/usr/bin/env python3
"""Props for q3r_dd_02 "Figure of 8": models and their textures.

    python3 tools/maptools/make_figure8_models.py [--out baseq3r]

Everything is generated here (own content, GPL like the game). The map bakes
the models in with misc_model; make_figure8.py adds the clip brushes.

  models/mapobjects/q3r_figure8/
    container20_<colour>.md3   20 ft box, 144 x 58 x 62 (x along the box)
    container40_<colour>.md3   40 ft box, 288 x 58 x 62 (red and blue only)
                               colours: red, blue, green, orange
    tyre_stack3.md3, tyre_stack5.md3   loose stacks, 3 and 5 tyres (26 wide)
    tyre_wall.md3              barrier piece: 6 stacks of 4, 156 long, 24 high
    floodmast.md3              lattice mast 460 high, lamp bank facing +x
    barrier.md3                concrete K-rail, 72 long, 26 high
    armco.md3                  guard rail, 96 long, posts at both ends
    fuelpump.md3               petrol pump, 24 x 16 x 46
    railing.md3                bridge railing, 64 long, 30 high (yellow)
Units: a car is about 107 long and 40 high. Every model stands on z = 0
with its middle at x = y = 0. Shader: scripts/q3r_figure8.shader (lamps).
Needs numpy and Pillow.
"""
from __future__ import annotations

import argparse
import math
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "koth"))
from make_koth_models import Mesh, lathe_pts, noise, write_md3  # noqa: E402

DIR = "models/mapobjects/q3r_figure8"
FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
COLOURS = {"red": (168, 42, 32), "blue": (34, 74, 138), "green": (42, 104, 62), "orange": (208, 112, 34)}
CONTAINER40 = ("red", "blue")      # the long ones the map uses


def sh(name):
    return f"{DIR}/{name}"


# ---------------------------------------------------------------- geometry helpers

def quad(mesh, shader, pts, n, uvs):
    mesh.add(shader, pts, [n] * 4, uvs, [(0, 1, 2), (0, 2, 3)])


def box(mesh, shader, lo, hi, uv=64.0, faces="tbxXyY", uvfn=None):
    """Axis box with world scaled uvs (uv units per texture); faces: t top, b bottom,
    x/X -x/+x, y/Y -y/+y. uvfn(face, (u, v)) can map them differently."""
    x0, y0, z0 = lo
    x1, y1, z1 = hi
    F = {
        "t": ((0, 0, 1), [(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], lambda p: (p[0], p[1])),
        "b": ((0, 0, -1), [(x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (x1, y0, z0)], lambda p: (p[0], p[1])),
        "X": ((1, 0, 0), [(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], lambda p: (p[1], -p[2])),
        "x": ((-1, 0, 0), [(x0, y1, z0), (x0, y0, z0), (x0, y0, z1), (x0, y1, z1)], lambda p: (-p[1], -p[2])),
        "Y": ((0, 1, 0), [(x1, y1, z0), (x0, y1, z0), (x0, y1, z1), (x1, y1, z1)], lambda p: (-p[0], -p[2])),
        "y": ((0, -1, 0), [(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)], lambda p: (p[0], -p[2])),
    }
    for f in faces:
        n, pts, proj = F[f]
        if uvfn:
            uvs = [uvfn(f, p) for p in pts]
        else:
            uvs = [tuple(c / uv for c in proj(p)) for p in pts]
        quad(mesh, shader, pts, n, uvs)


def beam(mesh, shader, a, b, t, uv=64.0):
    """Square beam of side t from point a to point b."""
    a, b = np.array(a, float), np.array(b, float)
    d = b - a
    L = np.linalg.norm(d)
    w = d / L
    u = np.cross(w, (0, 0, 1)) if abs(w[2]) < 0.9 else np.cross(w, (1, 0, 0))
    u /= np.linalg.norm(u)
    v = np.cross(w, u)
    h = t / 2
    corners = [u * h + v * h, -u * h + v * h, -u * h - v * h, u * h - v * h]
    for k in range(4):
        c0, c1 = corners[k], corners[(k + 1) % 4]
        n = (c0 + c1) / np.linalg.norm(c0 + c1)
        pts = [a + c0, a + c1, b + c1, b + c0]
        quad(mesh, shader, [tuple(p) for p in pts], tuple(n), [(0, 0), (t / uv, 0), (t / uv, L / uv), (0, L / uv)])
    for p, s in ((a, -1), (b, 1)):
        pts = [p + c for c in corners] if s > 0 else [p + c for c in corners[::-1]]
        quad(mesh, shader, [tuple(q) for q in pts], tuple(w * s), [(0, 0), (1, 0), (1, 1), (0, 1)])


def extrude(mesh, shader, prof, x0, x1, uv=64.0, cap=True):
    """Convex (y, z) profile (counter clockwise seen from +x) extruded along x."""
    n = len(prof)
    v_acc = [0.0]
    for i in range(n):
        y0_, z0_ = prof[i]
        y1_, z1_ = prof[(i + 1) % n]
        v_acc.append(v_acc[-1] + math.hypot(y1_ - y0_, z1_ - z0_))
    for i in range(n):
        y0_, z0_ = prof[i]
        y1_, z1_ = prof[(i + 1) % n]
        dy, dz = y1_ - y0_, z1_ - z0_
        L = math.hypot(dy, dz)
        if L < 1e-6:
            continue
        nrm = (0.0, dz / L, -dy / L)
        pts = [(x0, y0_, z0_), (x1, y0_, z0_), (x1, y1_, z1_), (x0, y1_, z1_)]
        uvs = [(x0 / uv, v_acc[i] / uv), (x1 / uv, v_acc[i] / uv), (x1 / uv, v_acc[i + 1] / uv), (x0 / uv, v_acc[i + 1] / uv)]
        quad(mesh, shader, pts, nrm, uvs)
    if cap:
        for x, s in ((x0, -1), (x1, 1)):
            pts = [(x, y, z) for y, z in prof]
            uvs = [(y / uv, z / uv) for y, z in prof]
            tris = [(0, i, i + 1) for i in range(1, n - 1)]
            mesh.add(shader, pts, [(s, 0, 0)] * n, uvs, tris)


def transformed(mesh_src, mesh_dst, R, t):
    """Copy all pieces of mesh_src into mesh_dst, rotated by R and moved by t."""
    for shader, pieces in mesh_src.parts.items():
        for V, N, S, T in pieces:
            V2 = V @ R.T + t
            N2 = N @ R.T
            mesh_dst.parts.setdefault(shader, []).append((V2, N2, S, T))


def rot_z(a):
    c, s = math.cos(a), math.sin(a)
    return np.array([[c, -s, 0], [s, c, 0], [0, 0, 1]])


def rot_y(a):
    c, s = math.cos(a), math.sin(a)
    return np.array([[c, 0, s], [0, 1, 0], [-s, 0, c]])


# ---------------------------------------------------------------- textures

def save(out, name, img, jpg=True):
    path = os.path.join(out, DIR, name + (".jpg" if jpg else ".tga"))
    im = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
    if jpg:
        im.save(path, quality=92)
    else:
        im.save(path)
    return path


def grime(size_y, size_x, seed, cells=6):
    n = noise(max(size_x, size_y), cells, seed)
    return n[:size_y, :size_x]


def streaks(h, w, seed, count, length=(0.3, 0.9), width=(2, 6)):
    """Rust/dirt streaks running down from the top: 0..1 strength."""
    rng = np.random.default_rng(seed)
    out = np.zeros((h, w))
    yy = np.arange(h)[:, None]
    for _ in range(count):
        x = rng.uniform(0, w)
        top = rng.uniform(0, 0.25) * h
        L = rng.uniform(*length) * h
        wd = rng.uniform(*width)
        xs = np.arange(w)[None, :]
        prof = np.exp(-((xs - x) / wd) ** 2)
        fall = np.clip(1 - (yy - top) / L, 0, 1) * (yy >= top)
        out = np.maximum(out, prof * fall * rng.uniform(0.4, 1.0))
    return out


def corrugated_side(colour, seed, w=512, h=256, text=True):
    """Container side: vertical trapezoid ribs, top/bottom rails, grime, stencil."""
    base = np.array(colour, float)
    x = np.arange(w)
    period = 24
    p = x % period
    prof = np.where(p < 8, 0.10, np.where(p < 12, 0.10 - 0.30 * (p - 8) / 4,
                    np.where(p < 20, -0.20, -0.20 + 0.30 * (p - 20) / 4)))
    shade = 1 + prof[None, :] * np.ones((h, 1))
    img = base[None, None, :] * shade[:, :, None]
    g = grime(h, w, seed)
    img *= (0.82 + 0.25 * g)[:, :, None]
    s = streaks(h, w, seed + 1, 22)
    rust = np.array([96, 52, 30], float)
    img = img * (1 - 0.55 * s[:, :, None]) + rust * 0.55 * s[:, :, None]
    # dirt at the bottom
    fade = np.clip((np.arange(h) - h * 0.78) / (h * 0.22), 0, 1)[:, None]
    img *= (1 - 0.35 * fade * (0.6 + 0.4 * g))[:, :, None]
    # rails
    img[:10] *= 0.62
    img[-12:] *= 0.55
    im = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
    if text:
        d = ImageDraw.Draw(im)
        f1 = ImageFont.truetype(FONT, 46)
        f2 = ImageFont.truetype(FONT, 15)
        d.text((40, 52), "Q3R LOGISTICS", font=f1, fill=(236, 232, 222))
        rng = np.random.default_rng(seed)
        code = f"Q3RU {rng.integers(100000, 999999)} {rng.integers(0, 9)}"
        d.text((w - 170, 22), code, font=f2, fill=(236, 232, 222))
        d.text((w - 170, 40), "22G1  MAX GROSS 30480 KG", font=ImageFont.truetype(FONT, 9), fill=(220, 216, 206))
        arr = np.asarray(im, float)
        # paint wears off with the grime
        wear = (grime(h, w, seed + 7, 10) > 0.72)[:, :, None]
        arr = np.where(wear & (arr.mean(2, keepdims=True) > 200), arr * 0.75 + img * 0.25, arr)
        img = arr * (1 + prof[None, :, None] * 0.6)
    return np.asarray(img, float)


def container_door(colour, seed, w=256, h=256):
    base = np.array(colour, float)
    img = np.ones((h, w, 3)) * base
    x = np.arange(w)
    # vertical corrugation on each leaf
    p = x % 20
    prof = np.where(p < 7, 0.08, np.where(p < 10, -0.15, np.where(p < 17, -0.18, 0.05)))
    img *= (1 + prof)[None, :, None]
    g = grime(h, w, seed)
    img *= (0.84 + 0.22 * g)[:, :, None]
    s = streaks(h, w, seed + 3, 10)
    img = img * (1 - 0.5 * s[:, :, None]) + np.array([96, 52, 30]) * 0.5 * s[:, :, None]
    im = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
    d = ImageDraw.Draw(im)
    dark = tuple(int(c * 0.45) for c in colour)
    d.rectangle([0, 0, w - 1, 9], fill=dark)
    d.rectangle([0, h - 12, w - 1, h - 1], fill=dark)
    d.rectangle([0, 0, 7, h - 1], fill=dark)
    d.rectangle([w - 8, 0, w - 1, h - 1], fill=dark)
    d.line([(w // 2, 10), (w // 2, h - 12)], fill=(30, 30, 30), width=3)
    rod = (176, 176, 168)
    for rx in (40, 92, 164, 216):
        d.rectangle([rx - 3, 12, rx + 3, h - 14], fill=rod)
        d.rectangle([rx - 6, 24, rx + 6, 34], fill=(120, 120, 114))
        d.rectangle([rx - 6, h - 40, rx + 6, h - 30], fill=(120, 120, 114))
        d.rectangle([rx - 2, 120, rx + 16, 128], fill=(150, 150, 142))
    for hy in (30, 90, 160, 220):
        d.rectangle([2, hy, 12, hy + 10], fill=(90, 90, 86))
        d.rectangle([w - 13, hy, w - 3, hy + 10], fill=(90, 90, 86))
    d.rectangle([110, 70, 146, 96], fill=(200, 196, 186))
    d.text((114, 76), "CSC", font=ImageFont.truetype(FONT, 11), fill=(40, 40, 40))
    d.text((40, 44), "Q3R", font=ImageFont.truetype(FONT, 22), fill=(236, 232, 222))
    return np.asarray(im, float)


def container_end(colour, seed, w=256, h=256):
    side = corrugated_side(colour, seed + 11, w=w, h=h, text=False)
    im = Image.fromarray(np.clip(side, 0, 255).astype(np.uint8))
    d = ImageDraw.Draw(im)
    dark = tuple(int(c * 0.45) for c in colour)
    d.rectangle([0, 0, 7, h - 1], fill=dark)
    d.rectangle([w - 8, 0, w - 1, h - 1], fill=dark)
    return np.asarray(im, float)


def frame_tex(seed, w=128, h=128):
    g = grime(h, w, seed, 4)
    img = np.ones((h, w, 3)) * np.array([58, 58, 56], float)
    img *= (0.8 + 0.35 * g)[:, :, None]
    s = streaks(h, w, seed, 8, width=(1, 3))
    return img * (1 - 0.4 * s[:, :, None]) + np.array([90, 50, 28]) * 0.4 * s[:, :, None]


def tyre_tex(painted, seed, w=256, h=128):
    """u around, v along the profile (lathe_pts: 24 units per texture): 0..0.24 lower
    wall, ..0.44 tread, ..0.68 upper wall; below that the inside of the hole."""
    rng = np.random.default_rng(seed)
    img = np.ones((h, w, 3)) * 30.0
    g = grime(h, w, seed, 8)
    img *= (0.8 + 0.4 * g)[:, :, None]
    v = np.arange(h)
    tread = (v > h * 0.24) & (v < h * 0.44)
    x = np.arange(w)
    blocks = ((x // 8) % 2 == 0)[None, :] ^ (((v // 6) % 2) == 0)[:, None]
    grooves = (x % 8 < 2)[None, :] | ((v % 12) < 2)[:, None]
    img[tread] *= 1.25
    tmask = tread[:, None] & grooves
    img[tmask] *= 0.35
    img[tread[:, None] & blocks & ~grooves] *= 1.1
    if painted:
        wall = ~tread
        paint = np.array([226, 224, 216], float)
        chips = grime(h, w, seed + 5, 12) > 0.66
        m = wall[:, None] & ~chips
        img[m] = paint * (0.75 + 0.25 * g[m])[:, None]
        # paint smeared onto the tread edges
        edge = tread[:, None] & (grime(h, w, seed + 9, 16) > 0.8)
        img[edge] = img[edge] * 0.3 + paint * 0.5
    else:
        # raised letters on the sidewall
        im = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
        d = ImageDraw.Draw(im)
        f = ImageFont.truetype(FONT, 11)
        for k in range(3):
            d.text((10 + k * 86, 8), "Q3R RACE", font=f, fill=(62, 62, 62))
            d.text((10 + k * 86, int(h * 0.50)), "Q3R RACE", font=f, fill=(62, 62, 62))
        img = np.asarray(im, float)
    return img


def steel_tex(seed, tint=(150, 152, 150), w=128, h=128):
    g = grime(h, w, seed, 6)
    img = np.ones((h, w, 3)) * np.array(tint, float)
    img *= (0.78 + 0.32 * g)[:, :, None]
    sp = (grime(h, w, seed + 2, 24) > 0.78)[:, :, None]
    img = np.where(sp, img * 0.8, img)
    return img


def lamp_tex(w=64, h=64):
    yy, xx = np.mgrid[0:h, 0:w]
    r = np.hypot(xx - w / 2 + 0.5, yy - h / 2 + 0.5) / (w / 2)
    core = np.clip(1.15 - r, 0, 1)
    rings = 0.85 + 0.15 * np.cos(r * 28)
    c = np.array([255, 246, 220], float)
    img = c * (0.55 + 0.45 * core)[:, :, None] * rings[:, :, None]
    img[r > 0.95] = (60, 60, 60)
    return img


def concrete_tex(seed, w=256, h=128):
    g = grime(h, w, seed, 5)
    fine = grime(h, w, seed + 1, 40)
    img = np.ones((h, w, 3)) * np.array([164, 162, 154], float)
    img *= (0.78 + 0.18 * g + 0.12 * fine)[:, :, None]
    im = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
    d = ImageDraw.Draw(im)
    # reflector band and the stencil
    d.rectangle([0, 40, w - 1, 50], fill=(214, 172, 40))
    d.text((92, 72), "Q3R", font=ImageFont.truetype(FONT, 26), fill=(70, 70, 66))
    arr = np.asarray(im, float)
    chips = (grime(h, w, seed + 3, 18) > 0.74)[:, :, None]
    return np.where(chips, arr * 0.82, arr)


def armco_tex(seed, w=128, h=64):
    img = steel_tex(seed, (176, 180, 182), w, h)
    v = np.arange(h)
    wave = 1 + 0.22 * np.cos(v / h * 4 * math.pi)
    img *= wave[:, None, None]
    return img


def pump_front(w=256, h=512):
    img = np.ones((h, w, 3)) * np.array([228, 226, 220], float)
    img *= (0.9 + 0.1 * grime(h, w, 4, 6))[:, :, None]
    im = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
    d = ImageDraw.Draw(im)
    d.rectangle([0, 0, w - 1, 70], fill=(176, 36, 28))
    d.text((40, 14), "Q3R FUEL", font=ImageFont.truetype(FONT, 32), fill=(255, 255, 255))
    d.rectangle([30, 100, w - 30, 190], fill=(24, 30, 26))
    d.text((44, 112), "88.88", font=ImageFont.truetype(FONT, 34), fill=(120, 230, 120))
    d.text((44, 152), "LITRES", font=ImageFont.truetype(FONT, 14), fill=(120, 230, 120))
    for k, (lbl, col) in enumerate((("RACE 102", (40, 120, 200)), ("SUPER 98", (210, 160, 30)), ("DIESEL", (40, 40, 40)))):
        y = 220 + k * 56
        d.rectangle([30, y, w - 30, y + 44], fill=col)
        d.text((44, y + 10), lbl, font=ImageFont.truetype(FONT, 20), fill=(255, 255, 255))
    d.rectangle([60, 400, w - 60, 470], fill=(70, 70, 70))
    d.rectangle([0, h - 30, w - 1, h - 1], fill=(60, 60, 58))
    return np.asarray(im, float)


def pump_side(w=128, h=512):
    img = np.ones((h, w, 3)) * np.array([226, 224, 218], float)
    img[:70] = (176, 36, 28)
    img[h - 30:] = (60, 60, 58)
    img *= (0.9 + 0.1 * grime(h, w, 8, 6))[:, :, None]
    return img


def yellow_paint(seed, w=64, h=64):
    g = grime(h, w, seed, 4)
    img = np.ones((h, w, 3)) * np.array([222, 176, 32], float)
    img *= (0.82 + 0.22 * g)[:, :, None]
    chips = (grime(h, w, seed + 3, 12) > 0.75)[:, :, None]
    return np.where(chips, np.array([80, 76, 70], float), img)


def textures(out):
    os.makedirs(os.path.join(out, DIR), exist_ok=True)
    written = []
    for k, (cname, col) in enumerate(COLOURS.items()):
        written.append(save(out, f"cont_side_{cname}", corrugated_side(col, 10 + k)))
        written.append(save(out, f"cont_door_{cname}", container_door(col, 20 + k)))
        written.append(save(out, f"cont_end_{cname}", container_end(col, 30 + k)))
    written.append(save(out, "cont_frame", frame_tex(40)))
    written.append(save(out, "tyre_black", tyre_tex(False, 50)))
    written.append(save(out, "tyre_white", tyre_tex(True, 51)))
    written.append(save(out, "mast_steel", steel_tex(60)))
    written.append(save(out, "mast_housing", steel_tex(61, (64, 66, 68))))
    written.append(save(out, "mast_lamp", lamp_tex()))
    written.append(save(out, "barrier_concrete", concrete_tex(70)))
    written.append(save(out, "armco", armco_tex(80)))
    written.append(save(out, "armco_post", steel_tex(81, (120, 122, 124))))
    written.append(save(out, "pump_front", pump_front()))
    written.append(save(out, "pump_side", pump_side()))
    written.append(save(out, "rail_yellow", yellow_paint(90)))
    return written


# ---------------------------------------------------------------- models

def container(length, cname):
    m = Mesh()
    L, W, H = length, 58.0, 62.0
    side, door, end = sh(f"cont_side_{cname}"), sh(f"cont_door_{cname}"), sh(f"cont_end_{cname}")
    frame = sh("cont_frame")
    x0, x1, y0, y1, z0, z1 = -L / 2 + 3, L / 2 - 3, -W / 2 + 1, W / 2 - 1, 3.0, H - 3

    def uvfn(face, p):
        if face in "yY":     # the long sides: 144 units per texture width
            u = (p[0] - x0) / 144.0 if face == "y" else (x1 - p[0]) / 144.0
            return (u, 1 - (p[2] - z0) / (z1 - z0))
        if face in "xX":
            u = (p[1] - y0) / (y1 - y0) if face == "X" else (y1 - p[1]) / (y1 - y0)
            return (u, 1 - (p[2] - z0) / (z1 - z0))
        return (p[0] / 144.0, p[1] / 72.0)
    box(m, side, (x0, y0, z0), (x1, y1, z1), faces="yY", uvfn=uvfn)
    box(m, end, (x0, y0, z0), (x1, y1, z1), faces="t", uvfn=uvfn)     # the roof: ribs, no lettering
    box(m, door, (x0, y0, z0), (x1, y1, z1), faces="X", uvfn=uvfn)
    box(m, end, (x0, y0, z0), (x1, y1, z1), faces="x", uvfn=uvfn)
    # frame: corner posts, top and bottom rails, cross members at the ends, corner castings
    for sx in (-1, 1):
        for sy in (-1, 1):
            cx, cy = sx * (L / 2 - 2), sy * (W / 2 - 2)
            box(m, frame, (cx - 2, cy - 2, 2), (cx + 2, cy + 2, H - 2), uv=32)
            for cz in (0, H - 3):
                box(m, frame, (cx - 2.6, cy - 2.6, cz), (cx + 2.6, cy + 2.6, cz + 3), uv=16)
    for sy in (-1, 1):
        for cz in (0, H - 3):
            box(m, frame, (-L / 2 + 2.6, sy * (W / 2 - 2) - 2, cz), (L / 2 - 2.6, sy * (W / 2 - 2) + 2, cz + 3), uv=32)
    for sx in (-1, 1):
        for cz in (0, H - 3):
            box(m, frame, (sx * (L / 2 - 2) - 2, -W / 2 + 2.6, cz), (sx * (L / 2 - 2) + 2, W / 2 - 2.6, cz + 3), uv=32)
    return m


TYRE_PROFILE = [(7.0, 0.0), (12.0, 0.0), (13.2, 1.6), (13.2, 4.4), (12.0, 6.0), (7.0, 6.0)]


def tyre(mesh, shader, pos, phase, bottom=True):
    """One tyre lying flat, 26 across, 6 high; the hole open (inner wall)."""
    t = Mesh()
    pts = TYRE_PROFILE
    # lathe_pts wants (r, z) from the inside out; the v runs along the profile
    lathe_pts(t, shader, pts if bottom else pts[1:], 10, u_rep=1.0)
    # inner wall of the hole
    lathe_pts(t, sh("tyre_black"), [(7.0, 6.0), (7.0, 0.0)], 10, u_rep=1.0)
    transformed(t, mesh, rot_z(phase), np.array(pos))


def tyre_stack(n, seed, painted_every=0):
    rng = np.random.default_rng(seed)
    m = Mesh()
    for k in range(n):
        shader = sh("tyre_white") if painted_every and k % painted_every == 0 else sh("tyre_black")
        tyre(m, shader, (rng.uniform(-0.8, 0.8), rng.uniform(-0.8, 0.8), k * 6.0), rng.uniform(0, 6.3), bottom=k == 0)
    return m


def tyre_wall():
    """6 stacks of 4 tyres in a row along x (156 long), white and black stacks taking turns."""
    m = Mesh()
    for s in range(6):
        st = tyre_stack(4, 100 + s, painted_every=1 if s % 2 == 0 else 0)
        transformed(st, m, np.eye(3), np.array([-65.0 + s * 26.0, 0.0, 0.0]))
    return m


def floodmast():
    """Lattice mast, 460 to the lamp head; the lamps look along +x and tilt down."""
    m = Mesh()
    steel = sh("mast_steel")
    top = 440.0

    def half(z):
        return 16.0 - 9.0 * z / top
    legs = [(sx, sy) for sx in (-1, 1) for sy in (-1, 1)]
    for sx, sy in legs:
        beam(m, steel, (sx * half(0), sy * half(0), 0), (sx * half(top), sy * half(top), top), 3.0, uv=32)
    levels = np.linspace(0, top, 12)
    for i in range(len(levels) - 1):
        za, zb = levels[i], levels[i + 1]
        ha, hb = half(za), half(zb)
        for (ax, ay), (bx, by) in (((-1, -1), (1, -1)), ((1, -1), (1, 1)), ((1, 1), (-1, 1)), ((-1, 1), (-1, -1))):
            if i % 2 == 0:
                beam(m, steel, (ax * ha, ay * ha, za), (bx * hb, by * hb, zb), 1.6, uv=32)
            else:
                beam(m, steel, (bx * ha, by * ha, za), (ax * hb, ay * hb, zb), 1.6, uv=32)
            beam(m, steel, (ax * hb, ay * hb, zb), (bx * hb, by * hb, zb), 1.6, uv=32)
    # base plate
    box(m, sh("mast_housing"), (-22, -22, 0), (22, 22, 4), uv=32)
    # platform and the lamp frame
    box(m, sh("mast_housing"), (-12, -34, top), (14, 34, top + 3), uv=32)
    for sy in (-1, 1):
        beam(m, steel, (-10, sy * 33, top + 3), (-10, sy * 33, top + 18), 2.0, uv=32)
    beam(m, steel, (-10, -33, top + 18), (-10, 33, top + 18), 2.0, uv=32)
    # 3 rows of 4 lamps, tilted 25 degrees down, looking along +x
    tilt = rot_y(math.radians(25))
    for row in range(3):
        for col in range(4):
            lamp = Mesh()
            housing = [(0.01, -4.0), (6.4, -4.0), (7.0, -2.0), (7.0, 3.0)]
            lathe_pts(lamp, sh("mast_housing"), housing, 12, u_rep=1.0)
            face = [(7.0, 3.0), (3.0, 3.4), (0.01, 3.5)]
            lathe_pts(lamp, sh("mast_lamp"), face, 12, u_rep=1.0)
            # the lathe runs along z: turn it to look along +x, then tilt down
            R = tilt @ np.array([[0, 0, 1], [0, 1, 0], [-1, 0, 0]])
            pos = np.array([2.0, -24.0 + col * 16.0, top + 34.0 - row * 15.0])
            transformed(lamp, m, R, pos)
    box(m, sh("mast_housing"), (-8, -34, top + 3), (-4, 34, top + 44), uv=32)
    return m


def barrier():
    m = Mesh()
    prof = [(-12.0, 0.0), (12.0, 0.0), (11.0, 3.0), (6.0, 9.0), (3.0, 26.0), (-3.0, 26.0), (-6.0, 9.0), (-11.0, 3.0)]
    extrude(m, sh("barrier_concrete"), prof, -36.0, 36.0, uv=72.0)     # counter clockwise seen from +x
    return m


def armco():
    m = Mesh()
    rail = sh("armco")
    # W-beam: a flat box with a slight bulge, 8 high, from z 12 to 20, on the -y side of the posts
    prof = [(-4.0, 12.0), (-2.4, 12.0), (-1.6, 14.0), (-1.6, 18.0), (-2.4, 20.0), (-4.0, 20.0), (-3.0, 16.0)]
    extrude(m, rail, prof, -48.0, 48.0, uv=96.0)
    for x in (-46.0, 0.0, 46.0):
        box(m, sh("armco_post"), (x - 1.5, -1.5, 0), (x + 1.5, 1.5, 21), uv=32)
        box(m, sh("armco_post"), (x - 1.0, -1.6, 13), (x + 1.0, 0.0, 19), uv=16)
    return m


def fuelpump():
    m = Mesh()
    front, side = sh("pump_front"), sh("pump_side")

    def uvfn(face, p):
        if face in "yY":
            u = (p[0] + 12) / 24 if face == "y" else (12 - p[0]) / 24
            return (u, 1 - p[2] / 46)
        if face in "xX":
            u = (p[1] + 8) / 16 if face == "X" else (8 - p[1]) / 16
            return (u, 1 - p[2] / 46)
        return (0.5, 0.95)
    box(m, front, (-12, -8, 0), (12, 8, 46), faces="yY", uvfn=uvfn)
    box(m, side, (-12, -8, 0), (12, 8, 46), faces="txX", uvfn=uvfn)
    box(m, sh("barrier_concrete"), (-16, -11, -1), (16, 11, 2), uv=64)
    # nozzle holsters and hoses
    for sx in (-1, 1):
        x0, x1 = sorted((sx * 12.0, sx * 15.0))
        box(m, sh("mast_housing"), (x0, -3, 20), (x1, 3, 26), uv=16)
        beam(m, sh("mast_housing"), (sx * 14.0, 0, 21), (sx * 17.0, 0, 6), 1.2, uv=16)
    return m


def railing():
    m = Mesh()
    paint = sh("rail_yellow")
    for x in (-30.0, 0.0, 30.0):
        box(m, paint, (x - 1.5, -1.5, 0), (x + 1.5, 1.5, 30), uv=32)
    box(m, paint, (-32, -2, 28), (32, 2, 31), uv=32)
    box(m, paint, (-32, -1.2, 15), (32, 1.2, 17.4), uv=32)
    box(m, paint, (-32, -0.8, 0), (32, 0.8, 6), uv=32)
    return m


def models(out):
    os.makedirs(os.path.join(out, DIR), exist_ok=True)
    report = []

    def w(name, mesh):
        r = write_md3(os.path.join(out, DIR, name + ".md3"), f"{DIR}/{name}.md3", mesh)
        report.append((name, r[0], r[1], r[2], tuple(np.round(r[3], 1)), tuple(np.round(r[4], 1))))
    for cname in COLOURS:
        w(f"container20_{cname}", container(144.0, cname))
    for cname in CONTAINER40:
        w(f"container40_{cname}", container(288.0, cname))
    w("tyre_stack3", tyre_stack(3, 7))
    w("tyre_stack5", tyre_stack(5, 8, painted_every=2))
    w("tyre_wall", tyre_wall())
    w("floodmast", floodmast())
    w("barrier", barrier())
    w("armco", armco())
    w("fuelpump", fuelpump())
    w("railing", railing())
    return report


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "baseq3r"))
    a = ap.parse_args()
    for p in textures(a.out):
        print("texture", os.path.relpath(p, a.out))
    for name, surfs, verts, tris, mins, maxs in models(a.out):
        print(f"model {name}: {surfs} surfaces, {verts} verts, {tris} tris, {mins} .. {maxs}")


if __name__ == "__main__":
    main()
