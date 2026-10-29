#!/usr/bin/env python3
"""Generate the lantern models for the maps (misc_model) and their textures.

    python3 tools/mapobjects/make_lanterns.py [--out baseq3r]

  models/mapobjects/q3r_lanterns/
    lantern.md3        hanging lantern: ring, chimney, roof, tapered glass
                       with an iron frame, base and finial, a flame inside.
                       Origin at the top of the ring (it hangs below it).
    lantern_post.md3   cast-iron post with a scrolled arm and the lantern
                       hanging from it. Origin on the ground, arm along +x.
    lantern_wall.md3   wall bracket with the lantern. Origin on the wall,
                       arm along +x.
    streetlamp.md3     sodium street lamp: tapered pole, swan-neck arm and a
                       cobra head with a glowing lens. Origin on the ground,
                       arm along +x (lens 88 units out, at z 316).
  plus textures; shaders in scripts/q3r_lanterns.shader.

The models carry no light: put a light entity at the glass (see
docs/lanterns.md for the offsets) and, where cars can hit the post, an invisible
solid brush around it (misc_models do not collide). Needs numpy.
"""
import argparse
import math
import os
import struct

import numpy as np

MAX_VERTS, MAX_INDEXES = 900, 5400       # renderer limits are 1000 / 6000 per surface
D = "models/mapobjects/q3r_lanterns/"
IRON, GLASS, FLAME = D + "iron", D + "glass", D + "flame"
POLE, LENS = D + "pole", D + "lens"


# ---------------------------------------------------------------- mesh builder

class Mesh:
    """Triangles per shader; split into several md3 surfaces when needed."""

    def __init__(self):
        self.parts = {}

    def add(self, shader, verts, normals, sts, tris, outward=None):
        """verts/normals/sts: lists; tris index into them. Triangles are turned
        so that they face along the vertex normals (Q3: clockwise from outside)."""
        lst = self.parts.setdefault(shader, [])
        V = np.array(verts, float)
        N = np.array(normals, float)
        N /= np.maximum(np.linalg.norm(N, axis=1), 1e-9)[:, None]
        fixed = []
        for a, b, c in tris:
            n = np.cross(V[b] - V[a], V[c] - V[a])
            if np.dot(n, N[a] + N[b] + N[c]) > 0:
                b, c = c, b
            fixed.append((a, b, c))
        lst.append((V, N, np.array(sts, float), fixed))

    def surfaces(self):
        out = []
        for shader, pieces in self.parts.items():
            cur = None
            for V, N, S, T in pieces:
                if cur is None or len(cur[0]) + len(V) > MAX_VERTS or 3 * (len(cur[3]) + len(T)) > MAX_INDEXES:
                    cur = [[], [], [], []]
                    out.append((shader, cur))
                base = len(cur[0])
                cur[0] += list(V)
                cur[1] += list(N)
                cur[2] += list(S)
                cur[3] += [(a + base, b + base, c + base) for a, b, c in T]
        return out


def grid(mesh, shader, P, Nrm, U, Vv, closed_u=False):
    """Quad grid from arrays [rows][cols] of points, normals and uv."""
    rows, cols = len(P), len(P[0])
    verts, norms, sts, tris = [], [], [], []
    for i in range(rows):
        for j in range(cols):
            verts.append(P[i][j])
            norms.append(Nrm[i][j])
            sts.append((U[i][j], Vv[i][j]))
    for i in range(rows - 1):
        for j in range(cols - 1):
            a, b = i * cols + j, i * cols + j + 1
            c, d = a + cols, b + cols
            tris += [(a, b, d), (a, d, c)]
    mesh.add(shader, verts, norms, sts, tris)


def lathe(mesh, shader, profile, segs, u_rep=1.0, phase=0.0, radial=None):
    """profile: list of (r, z, nr, nz, v). Rotated around z."""
    P, N, U, V = [], [], [], []
    for r, z, nr, nz, v in profile:
        rowp, rown, rowu, rowv = [], [], [], []
        for s in range(segs + 1):
            a = phase + 2 * math.pi * s / segs
            k = radial(a) if radial else 1.0
            rowp.append((r * k * math.cos(a), r * k * math.sin(a), z))
            rown.append((nr * math.cos(a), nr * math.sin(a), nz))
            rowu.append(u_rep * s / segs)
            rowv.append(v)
        P.append(rowp), N.append(rown), U.append(rowu), V.append(rowv)
    grid(mesh, shader, P, N, U, V)


def profile_from_points(pts, smooth=True):
    """(r, z) polyline -> (r, z, nr, nz, v) with outward (right-hand) normals.
    Corners are split when smooth is False or the turn is sharp."""
    out = []
    length = 0.0
    seg_n = []
    for (r0, z0), (r1, z1) in zip(pts, pts[1:]):
        dr, dz = r1 - r0, z1 - z0
        ln = math.hypot(dr, dz) or 1
        seg_n.append((dz / ln, -dr / ln))           # right normal of the direction of travel
    acc = [0.0]
    for (r0, z0), (r1, z1) in zip(pts, pts[1:]):
        length += math.hypot(r1 - r0, z1 - z0)
        acc.append(length)
    for i, (r, z) in enumerate(pts):
        ns = []
        if i > 0:
            ns.append(seg_n[i - 1])
        if i < len(seg_n):
            ns.append(seg_n[i])
        if len(ns) == 2 and (ns[0][0] * ns[1][0] + ns[0][1] * ns[1][1] < 0.7 or not smooth):
            out.append((r, z, ns[0][0], ns[0][1], acc[i] / 24))
            out.append((r, z, ns[1][0], ns[1][1], acc[i] / 24))
        else:
            nr = sum(n[0] for n in ns)
            nz = sum(n[1] for n in ns)
            ln = math.hypot(nr, nz) or 1
            out.append((r, z, nr / ln, nz / ln, acc[i] / 24))
    return out


def lathe_pts(mesh, shader, pts, segs, u_rep=1.0, radial=None):
    """Rotate a polyline; split at sharp corners so each run is smooth."""
    prof = profile_from_points(pts)
    # cut into runs at duplicated points (sharp corners)
    runs, cur = [], [prof[0]]
    for p in prof[1:]:
        if abs(p[0] - cur[-1][0]) < 1e-9 and abs(p[1] - cur[-1][1]) < 1e-9:
            runs.append(cur)
            cur = [p]
        else:
            cur.append(p)
    runs.append(cur)
    for run in runs:
        if len(run) >= 2:
            lathe(mesh, shader, run, segs, u_rep, radial=radial)


def sphere(mesh, shader, c, r, segs=10, rings=7, squash=(1, 1, 1), axis=None):
    """UV sphere; squash scales x y z; axis (ex, ey, ez) rotates it."""
    ex, ey, ez = axis if axis else ((1, 0, 0), (0, 1, 0), (0, 0, 1))
    P, N, U, V = [], [], [], []
    for i in range(rings + 1):
        th = math.pi * i / rings
        rp, rn, ru, rv = [], [], [], []
        for j in range(segs + 1):
            ph = 2 * math.pi * j / segs
            lx, ly, lz = math.sin(th) * math.cos(ph), math.sin(th) * math.sin(ph), math.cos(th)
            px, py, pz = lx * squash[0] * r, ly * squash[1] * r, lz * squash[2] * r
            nx, ny, nz = lx / squash[0], ly / squash[1], lz / squash[2]
            rp.append(tuple(c[k] + px * ex[k] + py * ey[k] + pz * ez[k] for k in range(3)))
            rn.append(tuple(nx * ex[k] + ny * ey[k] + nz * ez[k] for k in range(3)))
            ru.append(j / segs)
            rv.append(i / rings)
        P.append(rp), N.append(rn), U.append(ru), V.append(rv)
    grid(mesh, shader, P, N, U, V)


def octahedron(mesh, shader, c, r, axis, tall=1.3):
    """Cut gem: eight flat facets, pointing along axis[2]."""
    ex, ey, ez = axis
    pts = [tuple(c[k] + r * (math.cos(a) * ex[k] + math.sin(a) * ey[k]) for k in range(3))
           for a in (0, math.pi / 2, math.pi, 1.5 * math.pi)]
    top = tuple(c[k] + r * tall * ez[k] for k in range(3))
    bot = tuple(c[k] - r * 0.5 * ez[k] for k in range(3))
    for apex in (top, bot):
        for i in range(4):
            a, b = pts[i], pts[(i + 1) % 4]
            n = np.cross(np.subtract(b, a), np.subtract(apex, a))
            mid = np.mean([a, b, apex], axis=0)
            if np.dot(n, mid - np.array(c)) < 0:
                n = -n
            mesh.add(shader, [a, b, apex], [n, n, n], [(0, 1), (1, 1), (0.5, 0)], [(0, 1, 2)])


def box_sweep(mesh, shader, path, side, w, t, v_scale=1 / 24):
    """Rectangular bar along path (list of points); side: unit vector that
    stays sideways (w across it, t along the bend normal)."""
    P = [np.array(p, float) for p in path]
    S = np.array(side, float)
    corners_rows = []
    acc = 0.0
    vs = []
    for i, p in enumerate(P):
        tan = P[min(i + 1, len(P) - 1)] - P[max(i - 1, 0)]
        tan /= np.linalg.norm(tan)
        n = np.cross(S, tan)
        n /= np.linalg.norm(n)
        s = np.cross(tan, n)
        if i:
            acc += np.linalg.norm(P[i] - P[i - 1])
        vs.append(acc * v_scale)
        corners_rows.append((p + s * w + n * t, p - s * w + n * t, p - s * w - n * t, p + s * w - n * t, s, n))
    faces = ((0, 1, lambda s, n: n), (1, 2, lambda s, n: -s), (2, 3, lambda s, n: -n), (3, 0, lambda s, n: s))
    for a, b, fn in faces:
        Pp, Nn, U, V = [], [], [], []
        for row, v in zip(corners_rows, vs):
            nrm = fn(row[4], row[5])
            Pp.append([row[a], row[b]])
            Nn.append([nrm, nrm])
            U.append([0, 0.25])
            V.append([v, v])
        grid(mesh, shader, Pp, Nn, U, V)


def box(mesh, shader, lo, hi):
    x0, y0, z0 = lo
    x1, y1, z1 = hi
    faces = [((0, 0, 1), [(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]),
             ((0, 0, -1), [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0)]),
             ((1, 0, 0), [(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)]),
             ((-1, 0, 0), [(x0, y0, z0), (x0, y1, z0), (x0, y1, z1), (x0, y0, z1)]),
             ((0, 1, 0), [(x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1)]),
             ((0, -1, 0), [(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)])]
    for n, q in faces:
        mesh.add(shader, q, [n] * 4, [(0, 0), (1, 0), (1, 1), (0, 1)], [(0, 1, 2), (0, 2, 3)])


# ---------------------------------------------------------------- md3 out

def encode_normal(n):
    x, y, z = n
    lat = int(round(math.atan2(y, x) * 256 / (2 * math.pi))) & 255
    lng = int(round(math.acos(max(-1.0, min(1.0, z))) * 256 / (2 * math.pi))) & 255
    return (lat << 8) | lng


def name(s, n):
    return s.encode("ascii")[:n - 1].ljust(n, b"\0")


def write_md3(path, model_name, mesh):
    surfs = mesh.surfaces()
    blobs = []
    allv = []
    for idx, (shader, (V, N, S, T)) in enumerate(surfs):
        nv, nt = len(V), len(T)
        assert nv < 1000 and nt * 3 < 6000, (shader, nv, nt)
        ofs_shaders = 108
        ofs_tris = ofs_shaders + 68
        ofs_st = ofs_tris + 12 * nt
        ofs_xyz = ofs_st + 8 * nv
        end = ofs_xyz + 8 * nv
        sname = shader.split("/")[-1] + (f"_{idx}")
        b = struct.pack("<4s64s10i", b"IDP3", name(sname, 64), 0, 1, 1, nv, nt, ofs_tris, ofs_shaders,
                        ofs_st, ofs_xyz, end)
        b += name(shader, 64) + struct.pack("<i", 0)
        b += b"".join(struct.pack("<3i", *t) for t in T)
        b += b"".join(struct.pack("<2f", float(s[0]), float(s[1])) for s in S)
        for v, n in zip(V, N):
            b += struct.pack("<3hH", *(int(round(c * 64)) for c in v), encode_normal(n))
        assert len(b) == end
        blobs.append(b)
        allv += [tuple(v) for v in V]
    A = np.array(allv)
    mins, maxs = A.min(axis=0), A.max(axis=0)
    radius = float(np.max(np.linalg.norm(A, axis=1)))
    frame = struct.pack("<3f3f3ff16s", *mins, *maxs, 0, 0, 0, radius, name("frame0", 16))
    ofs_frames = 108
    ofs_surfs = ofs_frames + len(frame)
    body = b"".join(blobs)
    header = struct.pack("<4si64s9i", b"IDP3", 15, name(model_name, 64), 0, 1, 0, len(blobs), 0,
                         ofs_frames, ofs_surfs, ofs_surfs, ofs_surfs + len(body))
    with open(path, "wb") as f:
        f.write(header + frame + body)
    tris = sum(len(s[1][3]) for s in surfs)
    return len(blobs), len(A), tris, mins, maxs


# ---------------------------------------------------------------- textures

def write_tga(path, img):
    h, w = img.shape[:2]
    grey = img.ndim == 2
    alpha = (not grey) and img.shape[2] == 4
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 3 if grey else 2, 0, 0, 0, 0, 0, w, h,
                         8 if grey else (32 if alpha else 24), 8 if alpha else 0)
    if grey:
        data = img[::-1].tobytes()
    elif alpha:
        data = img[::-1][:, :, [2, 1, 0, 3]].tobytes()
    else:
        data = img[::-1, :, ::-1].tobytes()
    with open(path, "wb") as f:
        f.write(header + data)


def noise(size, cells, seed):
    rng = np.random.default_rng(seed)
    acc = np.zeros((size, size))
    for c, w in ((cells, 1.0), (cells * 2, 0.5), (cells * 4, 0.25), (cells * 8, 0.125)):
        g = rng.random((c, c))
        g = np.vstack([g, g[:1]])
        g = np.hstack([g, g[:, :1]])
        x = np.linspace(0, c, size, endpoint=False)
        i0 = x.astype(int)
        f = x - i0
        f = f * f * (3 - 2 * f)
        rows = g[i0][:, i0] * (1 - f)[None, :] + g[i0][:, i0 + 1] * f[None, :]
        rows2 = g[i0 + 1][:, i0] * (1 - f)[None, :] + g[i0 + 1][:, i0 + 1] * f[None, :]
        acc += w * (rows * (1 - f)[:, None] + rows2 * f[:, None])
    acc -= acc.min()
    return acc / acc.max()


def write_tga(path, img):
    h, w = img.shape[:2]
    grey = img.ndim == 2
    alpha = (not grey) and img.shape[2] == 4
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 3 if grey else 2, 0, 0, 0, 0, 0, w, h,
                         8 if grey else (32 if alpha else 24), 8 if alpha else 0)
    if grey:
        data = img[::-1].tobytes()
    elif alpha:
        data = img[::-1][:, :, [2, 1, 0, 3]].tobytes()
    else:
        data = img[::-1, :, ::-1].tobytes()
    with open(path, "wb") as f:
        f.write(header + data)


def noise(size, cells, seed):
    rng = np.random.default_rng(seed)
    acc = np.zeros((size, size))
    for c, w in ((cells, 1.0), (cells * 2, 0.5), (cells * 4, 0.25), (cells * 8, 0.125)):
        g = rng.random((c, c))
        g = np.vstack([g, g[:1]])
        g = np.hstack([g, g[:, :1]])
        x = np.linspace(0, c, size, endpoint=False)
        i0 = x.astype(int)
        f = x - i0
        f = f * f * (3 - 2 * f)
        rows = g[i0][:, i0] * (1 - f)[None, :] + g[i0][:, i0 + 1] * f[None, :]
        rows2 = g[i0 + 1][:, i0] * (1 - f)[None, :] + g[i0 + 1][:, i0 + 1] * f[None, :]
        acc += w * (rows * (1 - f)[:, None] + rows2 * f[:, None])
    acc -= acc.min()
    return acc / acc.max()



# ---------------------------------------------------------------- extra shapes

def frustum(mesh, shader, n, z0, a0, z1, a1, phase=None, cap0=False, cap1=False, uv=1 / 16):
    """Flat-sided n-gon frustum between z0 (apothem a0) and z1 (apothem a1).
    Each side is its own quad (sharp edges); caps optional."""
    phase = math.pi / n if phase is None else phase
    r0, r1 = a0 / math.cos(math.pi / n), a1 / math.cos(math.pi / n)
    up = 1 if z1 > z0 else -1
    for k in range(n):
        t0, t1 = phase + 2 * math.pi * k / n, phase + 2 * math.pi * (k + 1) / n
        tm = (t0 + t1) / 2
        p = [(r0 * math.cos(t0), r0 * math.sin(t0), z0), (r0 * math.cos(t1), r0 * math.sin(t1), z0),
             (r1 * math.cos(t1), r1 * math.sin(t1), z1), (r1 * math.cos(t0), r1 * math.sin(t0), z1)]
        slope = (a0 - a1) / (abs(z1 - z0) or 1) * up
        nrm = (math.cos(tm), math.sin(tm), slope)
        w0, w1 = 2 * a0 * math.tan(math.pi / n), 2 * a1 * math.tan(math.pi / n)
        h = math.hypot(z1 - z0, a1 - a0)
        st = [(0, 0), (w0 * uv, 0), (w1 * uv, h * uv), (0, h * uv)]
        mesh.add(shader, p, [nrm] * 4, st, [(0, 1, 2), (0, 2, 3)])
    for z, r, cap, sgn in ((z0, r0, cap0, -up), (z1, r1, cap1, up)):
        if not cap:
            continue
        pts = [(r * math.cos(phase + 2 * math.pi * k / n), r * math.sin(phase + 2 * math.pi * k / n), z)
               for k in range(n)]
        verts = [(0, 0, z)] + pts
        tris = [(0, 1 + k, 1 + (k + 1) % n) for k in range(n)]
        mesh.add(shader, verts, [(0, 0, sgn)] * (n + 1), [(v[0] * uv, v[1] * uv) for v in verts], tris)


def tube(mesh, shader, path, r, sides=8, closed=False, v_scale=1 / 16, caps=True):
    """Round tube along a polyline (parallel-transported frame)."""
    P = [np.array(p, float) for p in path]
    T = []
    for i in range(len(P)):
        if closed:
            d = P[(i + 1) % len(P)] - P[i - 1]
        else:
            d = P[min(i + 1, len(P) - 1)] - P[max(i - 1, 0)]
        T.append(d / np.linalg.norm(d))
    ref = np.array([0.0, 0.0, 1.0]) if abs(T[0][2]) < 0.9 else np.array([1.0, 0.0, 0.0])
    Nn = np.cross(T[0], ref)
    Nn /= np.linalg.norm(Nn)
    frames = []
    for i in range(len(P)):
        if i:
            Nn = Nn - T[i] * np.dot(Nn, T[i])
            Nn /= np.linalg.norm(Nn)
        B = np.cross(T[i], Nn)
        frames.append((Nn.copy(), B))
    pts = list(range(len(P))) + ([0] if closed else [])
    rows_p, rows_n, rows_u, rows_v = [], [], [], []
    acc = 0.0
    for idx, i in enumerate(pts):
        if idx:
            acc += np.linalg.norm(P[i] - P[pts[idx - 1]])
        Nn, B = frames[i]
        rp, rn, ru, rv = [], [], [], []
        for s in range(sides + 1):
            a = 2 * math.pi * s / sides
            d = Nn * math.cos(a) + B * math.sin(a)
            rp.append(tuple(P[i] + d * r))
            rn.append(tuple(d))
            ru.append(s / sides)
            rv.append(acc * v_scale)
        rows_p.append(rp), rows_n.append(rn), rows_u.append(ru), rows_v.append(rv)
    grid(mesh, shader, rows_p, rows_n, rows_u, rows_v)
    if caps and not closed:
        for i, sgn in ((0, -1), (len(P) - 1, 1)):
            Nn, B = frames[i]
            ring = [tuple(P[i] + (Nn * math.cos(2 * math.pi * s / sides) + B * math.sin(2 * math.pi * s / sides)) * r)
                    for s in range(sides)]
            verts = [tuple(P[i])] + ring
            tris = [(0, 1 + k, 1 + (k + 1) % sides) for k in range(sides)]
            mesh.add(shader, verts, [tuple(T[i] * sgn)] * (sides + 1), [(0, 0)] * (sides + 1), tris)


def arc(c, r, a0, a1, steps, plane="xz"):
    out = []
    for k in range(steps + 1):
        a = a0 + (a1 - a0) * k / steps
        if plane == "xz":
            out.append((c[0] + r * math.cos(a), c[1], c[2] + r * math.sin(a)))
        else:
            out.append((c[0] + r * math.cos(a), c[1] + r * math.sin(a), c[2]))
    return out


# ---------------------------------------------------------------- lantern

def add_lantern(mesh, ox=0.0, oy=0.0, oz=0.0):
    """Hanging lantern; (ox, oy, oz) is the top of the ring."""
    def at(p):
        return (p[0] + ox, p[1] + oy, p[2] + oz)
    # ring (in the xz plane) and knob
    tube(mesh, IRON, [at(p) for p in arc((0, 0, -3.6), 3.0, 0, 2 * math.pi, 14)[:-1]], 0.8, 6, closed=True)
    sphere(mesh, IRON, at((0, 0, -7.6)), 1.6, 8, 5)
    # chimney with a little cap
    for args in ((oz - 8.6, 2.2, oz - 9.6, 3.8, True, False), (oz - 9.6, 2.6, oz - 13.4, 3.0, False, False)):
        _sq(mesh, IRON, ox, oy, *args)
    # roof: overhanging pyramid, rim band, flat underside
    _sq(mesh, IRON, ox, oy, oz - 13.4, 3.0, oz - 19.0, 11.0, False, False)
    _sq(mesh, IRON, ox, oy, oz - 19.0, 11.0, oz - 20.6, 11.0, False, True)
    # glass: tapered to the bottom, slightly inside the frame
    _sq(mesh, GLASS, ox, oy, oz - 20.6, 8.6, oz - 38.0, 6.4, False, False, uv=1 / 17.4)
    # frame: corner bars, a top and a bottom collar
    for k in range(4):
        a = math.pi / 4 + k * math.pi / 2
        ca, sa = math.cos(a), math.sin(a)
        top = (9.0 * math.sqrt(2) * ca, 9.0 * math.sqrt(2) * sa, -20.6)
        bot = (6.9 * math.sqrt(2) * ca, 6.9 * math.sqrt(2) * sa, -38.0)
        tube(mesh, IRON, [at(top), at(bot)], 0.75, 5)
    for k in range(4):                        # mid bars across each pane
        a0, a1 = math.pi / 4 + k * math.pi / 2, math.pi / 4 + (k + 1) * math.pi / 2
        rr = 7.9 * math.sqrt(2)
        tube(mesh, IRON, [at((rr * math.cos(a0), rr * math.sin(a0), -28.0)),
                          at((rr * math.cos(a1), rr * math.sin(a1), -28.0))], 0.45, 4)
    _sq(mesh, IRON, ox, oy, oz - 20.6, 9.4, oz - 21.8, 9.2, False, False)
    _sq(mesh, IRON, ox, oy, oz - 37.4, 7.2, oz - 39.2, 7.6, False, False)
    # base and finial
    _sq(mesh, IRON, ox, oy, oz - 39.2, 7.6, oz - 42.0, 5.0, True, True)
    tube(mesh, IRON, [at((0, 0, -42.0)), at((0, 0, -44.5))], 1.6, 8)
    sphere(mesh, IRON, at((0, 0, -46.0)), 1.8, 8, 5)
    # flame / bulb inside
    sphere(mesh, FLAME, at((0, 0, -30.5)), 2.2, 8, 6, squash=(1, 1, 1.7))
    tube(mesh, IRON, [at((0, 0, -38.0)), at((0, 0, -34.0))], 1.2, 6)


def _sq(mesh, shader, ox, oy, z0, a0, z1, a1, cap0, cap1, uv=1 / 16):
    m = Mesh()
    frustum(m, shader, 4, z0, a0, z1, a1, phase=math.pi / 4, cap0=cap0, cap1=cap1, uv=uv)
    for V, N, S, T in m.parts[shader]:
        mesh.add(shader, [tuple(v + np.array([ox, oy, 0])) for v in V], list(N), list(S), T)


def lantern():
    mesh = Mesh()
    add_lantern(mesh)
    return mesh


def lantern_post():
    mesh = Mesh()
    # plinth, shaft with collars, top finial (round, cast iron look)
    base = [(0.01, 0.0), (9.5, 0.0), (9.5, 3.0), (8.0, 4.5), (7.5, 9.0), (5.5, 12.0), (4.4, 15.0),
            (3.4, 22.0), (3.2, 120.0), (4.6, 121.0), (4.6, 124.0), (3.2, 125.0), (3.0, 147.0),
            (4.4, 148.0), (4.4, 151.0), (2.6, 152.0), (2.0, 155.0)]
    lathe_pts(mesh, IRON, base, 12, u_rep=2)
    sphere(mesh, IRON, (0, 0, 157.5), 2.8, 10, 6)
    # arm: slightly arched bar out to the hook, a scroll brace below it
    arm = [(1.0 + 34.0 * t, 0, 146.0 + 5.0 * math.sin(math.pi * t)) for t in np.linspace(0, 1, 9)]
    tube(mesh, IRON, arm, 1.1, 6)
    brace = arc((14.0, 0, 128.0), 13.0, math.pi, math.pi / 2, 8)
    tube(mesh, IRON, [(p[0] - 0.0, p[1], p[2]) for p in brace], 0.8, 5)
    scroll = arc((22.0, 0, 139.0), 4.5, -math.pi / 2, math.pi * 1.2, 10)
    tube(mesh, IRON, scroll, 0.7, 5)
    sphere(mesh, IRON, (35.6, 0, 146.6), 1.6, 8, 5)
    # hook and two chain links
    tube(mesh, IRON, [(34.0, 0, 145.0), (34.0, 0, 141.5)], 0.6, 5)
    tube(mesh, IRON, arc((34.0, 0, 139.5), 1.6, 0, 2 * math.pi, 10)[:-1], 0.45, 4, closed=True)
    tube(mesh, IRON, [(34.0 + 1.6 * math.cos(a), 1.6 * math.sin(a), 136.6) for a in np.linspace(0, 2 * math.pi, 11)][:-1],
         0.45, 4, closed=True)
    add_lantern(mesh, 34.0, 0.0, 135.0)
    return mesh


def lantern_wall():
    mesh = Mesh()
    box(mesh, IRON, (0.0, -4.0, -12.0), (1.6, 4.0, 6.0))                # wall plate
    for z in (-8.0, 2.0):
        sphere(mesh, IRON, (1.8, 0, z), 1.0, 6, 4)                     # rivets
    arm = [(1.0 + 23.0 * t, 0, 0.0 + 2.5 * math.sin(math.pi * t)) for t in np.linspace(0, 1, 7)]
    tube(mesh, IRON, arm, 1.0, 6)
    tube(mesh, IRON, arc((15.0, 0, -11.0), 13.5, math.pi, math.pi * 0.55, 8), 0.7, 5)
    sphere(mesh, IRON, (24.6, 0, 0.4), 1.4, 8, 5)
    tube(mesh, IRON, [(22.0, 0, -0.8), (22.0, 0, -3.5)], 0.55, 5)
    add_lantern(mesh, 22.0, 0.0, -3.5)
    return mesh


# ---------------------------------------------------------------- street lamp

def streetlamp():
    mesh = Mesh()
    base = [(0.01, 0.0), (11.5, 0.0), (11.5, 20.0), (10.0, 22.0), (7.2, 24.0), (6.6, 30.0),
            (4.4, 296.0), (4.0, 300.0)]
    lathe_pts(mesh, POLE, base, 10, u_rep=2)
    # swan neck: up from the pole top, over and down to the head
    neck = [(0.0, 0, 296.0), (0.0, 0, 306.0), (3.0, 0, 316.0), (12.0, 0, 323.0), (26.0, 0, 326.0),
            (45.0, 0, 325.0), (62.0, 0, 322.0), (74.0, 0, 320.0)]
    tube(mesh, POLE, neck, 2.4, 8)
    # cobra head: flattened lathe along x, glowing lens underneath
    head = []
    L = 36.0
    for i in range(13):
        t = i / 12
        x = 70.0 + L * t
        w = 9.5 * math.sin(math.pi * (0.12 + 0.88 * t)) ** 0.6 + 2.0
        head.append((x, w))
    P, N, U, V = [], [], [], []
    for x, w in head:
        rp, rn, ru, rv = [], [], [], []
        for s in range(13):
            a = math.pi * s / 12                  # upper half shell
            y, z = w * math.cos(a), 0.55 * w * math.sin(a)
            rp.append((x, y, 316.0 + z))
            rn.append((0.0, math.cos(a), math.sin(a) / 0.55))
            ru.append(s / 12)
            rv.append((x - 70) / 36)
        P.append(rp), N.append(rn), U.append(ru), V.append(rv)
    grid(mesh, POLE, P, N, U, V)
    # lens: flat ellipse on the underside
    verts, sts = [(88.0, 0, 315.6)], [(0.5, 0.5)]
    ring = []
    for s in range(20):
        a = 2 * math.pi * s / 20
        verts.append((88.0 + 15.5 * math.cos(a), 7.4 * math.sin(a), 315.6))
        sts.append((0.5 + 0.5 * math.cos(a), 0.5 + 0.5 * math.sin(a)))
    tris = [(0, 1 + k, 1 + (k + 1) % 20) for k in range(20)]
    mesh.add(LENS, verts, [(0, 0, -1)] * 21, sts, tris)
    # rim around the lens (lower edge of the shell)
    rim = [(88.0 + 16.5 * math.cos(a), 8.2 * math.sin(a), 315.8) for a in np.linspace(0, 2 * math.pi, 21)][:-1]
    tube(mesh, POLE, rim, 0.9, 5, closed=True)
    return mesh


# ---------------------------------------------------------------- textures

def textures(mdir):
    rng = np.random.default_rng(5)
    s = 128
    n = noise(s, 4, 11)
    fine = rng.normal(0, 1, (s, s)) * 0.03
    iron = 0.13 + 0.07 * n + fine
    rust = np.clip(noise(s, 8, 12) - 0.72, 0, 1) * 1.6
    img = np.stack([iron + 0.10 * rust, iron + 0.04 * rust, iron + 0.01 * rust], -1)
    write_tga(os.path.join(mdir, "iron.tga"), (np.clip(img, 0, 1) * 255).astype(np.uint8))

    c = (np.arange(s) + 0.5) / s * 2 - 1
    X, Y = np.meshgrid(c, c)
    glow = np.exp(-(X ** 2 * 1.4 + (Y - 0.1) ** 2) / 0.45)
    frost = 0.85 + 0.15 * noise(s, 8, 13)
    edge = np.clip(1 - np.maximum(abs(X), abs(Y)) ** 8, 0, 1)
    v = glow * frost * edge
    glass = np.stack([v * 1.0, v * 0.72, v * 0.36], -1) * 0.95
    write_tga(os.path.join(mdir, "glass.tga"), (np.clip(glass, 0, 1) * 255).astype(np.uint8))

    f = 64
    c = (np.arange(f) + 0.5) / f * 2 - 1
    X, Y = np.meshgrid(c, c)
    core = np.exp(-(X ** 2 + Y ** 2) / 0.5)
    flame = np.stack([0.9 + 0.1 * core, 0.62 + 0.35 * core, 0.25 + 0.6 * core], -1)
    write_tga(os.path.join(mdir, "flame.tga"), (np.clip(flame, 0, 1) * 255).astype(np.uint8))

    p = noise(s, 4, 14)
    streak = np.repeat(rng.normal(0, 1, (1, s)), s, axis=0) * 0.03
    pole = 0.46 + 0.10 * p + streak + fine
    write_tga(os.path.join(mdir, "pole.tga"), (np.clip(np.stack([pole, pole * 1.01, pole * 1.03], -1), 0, 1) * 255).astype(np.uint8))

    c = (np.arange(s) + 0.5) / s
    X, Y = np.meshgrid(c, c)
    prism = 0.78 + 0.22 * np.abs(np.sin(X * math.pi * 10)) * np.abs(np.sin(Y * math.pi * 6))
    rr = np.sqrt((X - 0.5) ** 2 + (Y - 0.5) ** 2)
    hot = np.clip(1.2 - rr * 1.6, 0.35, 1)
    lens = np.stack([prism * hot, prism * hot * 0.66, prism * hot * 0.30], -1)
    write_tga(os.path.join(mdir, "lens.tga"), (np.clip(lens, 0, 1) * 255).astype(np.uint8))


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", default=os.path.normpath(os.path.join(here, "..", "..", "baseq3r")))
    args = ap.parse_args()
    mdir = os.path.join(args.out, "models", "mapobjects", "q3r_lanterns")
    os.makedirs(mdir, exist_ok=True)
    for fname, mesh in (("lantern.md3", lantern()), ("lantern_post.md3", lantern_post()),
                        ("lantern_wall.md3", lantern_wall()), ("streetlamp.md3", streetlamp())):
        ns, nv, nt, mins, maxs = write_md3(os.path.join(mdir, fname), D + fname, mesh)
        print(f"{fname}: {ns} surfaces, {nv} verts, {nt} tris, bounds {np.round(mins, 1)} .. {np.round(maxs, 1)}")
    textures(mdir)
    print("textures written")


if __name__ == "__main__":
    main()
