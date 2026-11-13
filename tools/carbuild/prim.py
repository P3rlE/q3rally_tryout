"""Primitive modelling for small hard-surface md3 models (weapons, props).

A Mesh collects surfaces; every vertex gets its texture coordinate from a
material: a cell of a texture atlas (see atlas.py) and a planar projection
along the face normal, so boxes, tubes and discs need no hand UVs.

Units are game units, x forward, y left, z up.
"""
from __future__ import annotations

import math

import md3


def _norm(v):
    ln = math.sqrt(sum(c * c for c in v)) or 1.0
    return [c / ln for c in v]


def _cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def frame(d):
    """Two unit vectors perpendicular to d and to each other."""
    d = _norm(d)
    ref = (0, 0, 1) if abs(d[2]) < 0.9 else (1, 0, 0)
    u = _norm(_cross(d, ref))
    v = _cross(d, u)
    return u, v


class Mesh:
    """Surfaces of one md3. `cells` maps a material name to its atlas cell
    (u0, v0, size) in 0..1 texture space; `px` is texels per unit."""

    def __init__(self, cells, atlas_px=512, px=4.0):
        self.cells = cells
        self.scale = px / atlas_px
        self.surfs = []
        self.tags = []
        self.cur = None

    def surf(self, name, shader):
        s = md3.Surf()
        s.name, s.flags, s.shaders = name, 0, [(shader, 0)]
        s.tris, s.st, s.frames = [], [], [[]]
        self.surfs.append(s)
        self.cur = s
        return s

    def uv(self, p, n, mat):
        u0, v0, size = self.cells[mat]
        a = [abs(c) for c in n]
        if a[0] >= a[1] and a[0] >= a[2]:
            s, t = p[1], p[2]
        elif a[1] >= a[2]:
            s, t = p[0], p[2]
        else:
            s, t = p[0], p[1]
        half = size / 2
        su = max(-half * 0.92, min(half * 0.92, s * self.scale))
        tv = max(-half * 0.92, min(half * 0.92, t * self.scale))
        return u0 + half + su, v0 + half - tv

    def vert(self, p, n, mat, uv=None):
        s = self.cur
        n = _norm(n)
        s.frames[0].append([round(p[0] * 64), round(p[1] * 64), round(p[2] * 64), md3.enc_normal(*n)])
        s.st.append(list(uv) if uv else list(self.uv(p, n, mat)))
        return len(s.st) - 1

    def tri(self, a, b, c):
        self.cur.tris.append([a, c, b])            # Q3 draws clockwise triangles as front faces

    def quad(self, pts, n, mat):
        i = [self.vert(p, n, mat) for p in pts]
        self.tri(i[0], i[1], i[2])
        self.tri(i[0], i[2], i[3])

    def poly(self, pts, n, mat):
        """Convex polygon, counter-clockwise seen from the side n points to."""
        i = [self.vert(p, n, mat) for p in pts]
        for k in range(1, len(i) - 1):
            self.tri(i[0], i[k], i[k + 1])

    def poly2(self, pts, n, mat):
        """Convex polygon facing n, whatever order the points come in."""
        a, b, c = pts[0], pts[1], pts[2]
        t = _cross([b[i] - a[i] for i in range(3)], [c[i] - a[i] for i in range(3)])
        self.poly(pts if sum(t[i] * n[i] for i in range(3)) > 0 else list(reversed(pts)), n, mat)

    def box(self, lo, hi, mat):
        (x0, y0, z0), (x1, y1, z1) = lo, hi
        c = [(x, y, z) for x in (x0, x1) for y in (y0, y1) for z in (z0, z1)]
        for q, n in (((4, 6, 7, 5), (1, 0, 0)), ((0, 1, 3, 2), (-1, 0, 0)), ((2, 3, 7, 6), (0, 1, 0)),
                     ((0, 4, 5, 1), (0, -1, 0)), ((1, 5, 7, 3), (0, 0, 1)), ((0, 2, 6, 4), (0, 0, -1))):
            self.quad([c[k] for k in q], n, mat)

    def cyl(self, a, b, r0, r1=None, mat="metal", segs=12, cap0=True, cap1=True, cap_mat=None, phase=0.0):
        """Tube (or cone, r0 at a, r1 at b) with optional flat caps."""
        r1 = r0 if r1 is None else r1
        d = [b[i] - a[i] for i in range(3)]
        u, v = frame(d)
        dn = _norm(d)
        slope = (r0 - r1) / (math.sqrt(sum(c * c for c in d)) or 1)
        ring0, ring1, rim = [], [], []
        for k in range(segs + 1):
            t = 2 * math.pi * k / segs + phase
            radial = [math.cos(t) * u[i] + math.sin(t) * v[i] for i in range(3)]
            n = [radial[i] + slope * dn[i] for i in range(3)]
            p0 = [a[i] + r0 * radial[i] for i in range(3)]
            p1 = [b[i] + r1 * radial[i] for i in range(3)]
            rim.append((p0, p1))
            uvx = (k / segs)
            cu0, cv0, size = self.cells[mat]
            ring0.append(self.vert(p0, n, mat, (cu0 + size * (0.04 + 0.92 * uvx), cv0 + size * 0.9)))
            ring1.append(self.vert(p1, n, mat, (cu0 + size * (0.04 + 0.92 * uvx), cv0 + size * 0.1)))
        for k in range(segs):
            self.tri(ring0[k], ring0[k + 1], ring1[k + 1])
            self.tri(ring0[k], ring1[k + 1], ring1[k])
        cm = cap_mat or mat
        if cap0 and r0 > 0:
            self.poly([rim[k][0] for k in range(segs, 0, -1)], [-c for c in dn], cm)
        if cap1 and r1 > 0:
            self.poly([rim[k][1] for k in range(segs)], dn, cm)

    def ring(self, c, d, r0, r1, mat, segs=16):
        """Flat annulus at c facing d, between radii r0 < r1."""
        u, v = frame(d)
        inner, outer = [], []
        for k in range(segs + 1):
            t = 2 * math.pi * k / segs
            radial = [math.cos(t) * u[i] + math.sin(t) * v[i] for i in range(3)]
            inner.append(self.vert([c[i] + r0 * radial[i] for i in range(3)], d, mat))
            outer.append(self.vert([c[i] + r1 * radial[i] for i in range(3)], d, mat))
        for k in range(segs):
            self.tri(inner[k], outer[k], outer[k + 1])
            self.tri(inner[k], outer[k + 1], inner[k + 1])

    def loft(self, rings, mat, cap0=True, cap1=True, cap_mat=None, inside=False):
        """Skin through a list of rings (same point count, each a closed
        loop round the loft direction); smooth normals, cylinder-like UVs
        (u round the rings, v along the loft). inside=True turns the skin
        inside out (the inner wall of a duct or scoop)."""
        n, nr = len(rings[0]), len(rings)
        cents = [[sum(p[i] for p in r) / n for i in range(3)] for r in rings]
        axis = _norm([cents[-1][i] - cents[0][i] for i in range(3)])
        c0 = cents[0]
        turn = _cross([rings[0][0][i] - c0[i] for i in range(3)], [rings[0][1][i] - c0[i] for i in range(3)])
        if sum(turn[i] * axis[i] for i in range(3)) < 0:
            rings = [list(reversed(r)) for r in rings]
        cu0, cv0, size = self.cells[mat]
        rows = []
        for j, r in enumerate(rings):
            jp, jn = max(j - 1, 0), min(j + 1, nr - 1)
            row = []
            for k in range(n + 1):
                kk = k % n
                p = r[kk]
                ta = [r[(kk + 1) % n][i] - r[(kk - 1) % n][i] for i in range(3)]
                tl = [rings[jn][kk][i] - rings[jp][kk][i] for i in range(3)]
                nrm = _norm(_cross(ta, tl))
                out = [p[i] - cents[j][i] for i in range(3)]
                if (sum(nrm[i] * out[i] for i in range(3)) < 0) != inside:
                    nrm = [-c for c in nrm]
                uv = (cu0 + size * (0.04 + 0.92 * k / n), cv0 + size * (0.9 - 0.8 * j / max(1, nr - 1)))
                row.append(self.vert(p, nrm, mat, uv))
            rows.append(row)
        for j in range(nr - 1):
            for k in range(n):
                if inside:
                    self.tri(rows[j][k], rows[j + 1][k + 1], rows[j][k + 1])
                    self.tri(rows[j][k], rows[j + 1][k], rows[j + 1][k + 1])
                else:
                    self.tri(rows[j][k], rows[j][k + 1], rows[j + 1][k + 1])
                    self.tri(rows[j][k], rows[j + 1][k + 1], rows[j + 1][k])
        cm = cap_mat or mat
        if cap0:
            d = _norm([cents[1][i] - cents[0][i] for i in range(3)])
            self.poly(list(reversed(rings[0])), [-c for c in d], cm)
        if cap1:
            d = _norm([cents[-1][i] - cents[-2][i] for i in range(3)])
            self.poly(list(rings[-1]), d, cm)

    def bore(self, c, d, r, depth, mat="dark", segs=10):
        """The inside of a pipe: a tube seen from inside, from the mouth c
        `depth` units back against d (d points out of the mouth), closed."""
        dn = _norm(d)
        u, v = frame(dn)
        e = [c[i] - dn[i] * depth for i in range(3)]
        cu0, cv0, size = self.cells[mat]
        r0, r1 = [], []
        for k in range(segs + 1):
            t = 2 * math.pi * k / segs
            radial = [math.cos(t) * u[i] + math.sin(t) * v[i] for i in range(3)]
            n = [-x for x in radial]
            uvx = cu0 + size * (0.04 + 0.92 * k / segs)
            r0.append(self.vert([c[i] + r * radial[i] for i in range(3)], n, mat, (uvx, cv0 + size * 0.1)))
            r1.append(self.vert([e[i] + r * radial[i] for i in range(3)], n, mat, (uvx, cv0 + size * 0.9)))
        for k in range(segs):
            self.tri(r0[k], r1[k + 1], r0[k + 1])
            self.tri(r0[k], r1[k], r1[k + 1])
        pts = []
        for k in range(segs):
            t = 2 * math.pi * k / segs
            pts.append([e[i] + r * (math.cos(t) * u[i] + math.sin(t) * v[i]) for i in range(3)])
        self.poly(pts, dn, mat)

    def tube(self, pts, r, mat, segs=6):
        """A bent rod through a list of points."""
        for a, b in zip(pts, pts[1:]):
            self.cyl(a, b, r, mat=mat, segs=segs, cap0=False, cap1=False)

    def smooth_tube(self, pts, r, mat, segs=8, cap0=False, cap1=False):
        """A hose: one continuous skin through the points (rings mitred at
        the joints), unlike tube(), which is a chain of straight pieces."""
        rings = []
        up = None
        for j, p in enumerate(pts):
            a = pts[max(j - 1, 0)]
            b = pts[min(j + 1, len(pts) - 1)]
            d = _norm([b[i] - a[i] for i in range(3)])
            if up is None:
                u, v = frame(d)
            else:                                   # parallel transport: no twisting
                u = _norm(_cross(_cross(d, up), d))
                u = [-c for c in u] if sum(u[i] * up[i] for i in range(3)) < 0 else u
                v = _cross(d, u)
            up = u
            rings.append([[p[i] + r * (math.cos(2 * math.pi * k / segs) * u[i] + math.sin(2 * math.pi * k / segs) * v[i])
                           for i in range(3)] for k in range(segs)])
        self.loft(rings, mat, cap0=cap0, cap1=cap1)

    def cross_quads(self, c, sx, sy, sz, mat=None):
        """Two crossed quads (in the xz and xy planes) round c, for glows
        and flames; the whole cell of mat (or, without mat, a whole texture
        of its own) is mapped on each."""
        cu0, cv0, size = self.cells[mat] if mat else (0.0, 0.0, 1.0)
        mat = mat or next(iter(self.cells))
        uvs = [(cu0, cv0), (cu0 + size, cv0), (cu0 + size, cv0 + size), (cu0, cv0 + size)]
        for pts, n in (([(c[0] - sx, c[1], c[2] + sz), (c[0] + sx, c[1], c[2] + sz), (c[0] + sx, c[1], c[2] - sz),
                         (c[0] - sx, c[1], c[2] - sz)], (0, 1, 0)),
                       ([(c[0] - sx, c[1] - sy, c[2]), (c[0] + sx, c[1] - sy, c[2]), (c[0] + sx, c[1] + sy, c[2]),
                         (c[0] - sx, c[1] + sy, c[2])], (0, 0, 1))):
            i = [self.vert(p, n, mat, uv) for p, uv in zip(pts, uvs)]
            self.tri(i[0], i[1], i[2])
            self.tri(i[0], i[2], i[3])

    def tag(self, name, origin, axis=((1, 0, 0), (0, 1, 0), (0, 0, 1))):
        self.tags.append([name, list(origin), [list(a) for a in axis]])

    def save(self, path, name):
        m = md3.Model()
        m.name, m.flags = name, 0
        pts = [[c / 64 for c in v[:3]] for s in self.surfs for v in s.frames[0]]
        lo = [min(p[i] for p in pts) for i in range(3)]
        hi = [max(p[i] for p in pts) for i in range(3)]
        rad = max(math.sqrt(sum(c * c for c in p)) for p in pts)
        m.frames = [[*lo, *hi, 0, 0, 0, rad, name.encode()[:16]]]
        m.tags = [self.tags]
        m.surfs = self.surfs
        md3.save(m, str(path))
        return sum(len(s.tris) for s in self.surfs)
