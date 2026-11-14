"""Q3Rally's driver helmets (models/players/heads).

    python3 tools/carbuild/helmets/build_helmets.py            # everything
    python3 tools/carbuild/helmets/build_helmets.py --sheet x.png

For every design in designs.py this writes
    models/players/heads/<key>.md3            the helmet (full face or open face)
    models/players/heads/<key>.skin           which texture goes on which surface
    models/players/heads/icon_<key>.tga       128 x 128 picture for the menus
    models/players/heads/helmets/<key>.jpg    the paint
and the shared parts (visors, goggles, rubber, balaclava) plus
scripts/heads.shader. The head list in the menus and the garage is built
from the .skin files, so a new design only needs an entry in designs.py.

Units are game units like the old heads, about 10 units long. The cars'
tag_head is turned 180 degrees round y, so the files hold the helmet upside
down and facing back, neck at the origin (TagMesh does that); the helmet is
modelled upright, x forward, z up.

The shell is a ring mesh: ring t (0 at the bottom edge, 1 at the top) and
angle phi round the head (0 straight ahead). P(phi, t) gives the point;
the face opening is the part of each ring between -phi_e(t) and phi_e(t),
so its edge is smooth whatever the ring count. The paint texture uses
(phi, t) as its coordinates: every texel knows its point on the helmet,
and the designs are drawn on the 3D helmet, not on the flat texture.
"""
import argparse
import math
import os
import sys

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..'))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'texbuild'))
from prim import Mesh  # noqa: E402
import designs  # noqa: E402

BASE = os.path.abspath(os.path.join(HERE, '..', '..', '..', 'baseq3r'))
HEADS = 'models/players/heads'
SHARED = f'{HEADS}/helmets'
PAINT_W, PAINT_H = 512, 256

C = np.array([-0.6, 0.0, -3.3])         # centre of the shell
RX_BACK, RY, RZ_TOP, RZ_BOT = 5.2, 4.25, 4.5, 4.4


def x_front(theta):
    """How far the front reaches (from the centre) at elevation theta: round
    above the eyes, nearly upright over the face, the chin bar curving in
    only at the bottom."""
    deg = -np.degrees(theta)
    down = 4.9 - 0.25 * np.clip(deg / 20, 0, 1) - 0.75 * np.clip((deg - 45) / 25, 0, 1) ** 2
    return np.where(theta >= 0, 4.9 * np.cos(theta), down)


def rx_front(theta):
    return x_front(theta) / np.maximum(np.cos(theta), 0.05)


def theta_bottom(phi):
    """Elevation of the lower edge: lower at the chin than at the neck."""
    return np.radians(-56.0 - 13.0 * np.cos(phi))


def theta_of(phi, t):
    tb = theta_bottom(phi)
    return tb + (np.pi / 2 - tb) * t


def shell_point(phi, t, scale=1.0):
    """Point and outward normal of the shell (arrays broadcast)."""
    phi, t = np.asarray(phi, float), np.asarray(t, float)
    th = theta_of(phi, t)
    rx = np.where(np.cos(phi) > 0, rx_front(th), RX_BACK)
    rz = np.where(th > 0, RZ_TOP, RZ_BOT)
    d = np.stack([np.cos(th) * np.cos(phi), np.cos(th) * np.sin(phi), np.sin(th)], -1)
    radii = np.stack([rx, np.full_like(rx, RY), rz], -1)
    p = C + d * radii * scale
    n = d / radii                                  # ellipsoid normal (good enough with the chin bulge)
    n /= np.linalg.norm(n, axis=-1, keepdims=True)
    return p, n


class Kind:
    """Shape of the face opening."""

    def __init__(self, name, t1, t2, width, rings):
        self.name, self.t1, self.t2, self.width = name, t1, t2, np.radians(width)
        self.rings = rings

    def phi_e(self, t, extra=0.0, grow=0.0):
        """Half width of the opening at ring t (0 outside it)."""
        t1, t2 = self.t1 - grow, self.t2 + grow
        if self.name == 'open':                     # widest at the bottom edge, closes at t2
            u = np.clip(t / t2, 0, 1)
        else:
            u = np.clip(np.abs((t - (t1 + t2) / 2) / ((t2 - t1) / 2)), 0, 1)
        w = (self.width + extra) * (1 - u ** 4) ** 0.25
        inside = (t >= t1) & (t <= t2) if self.name != 'open' else (t <= t2)
        return np.where(inside, w, 0.0)


def ring_ts(t1, t2, n_open):
    below = list(np.linspace(0, t1, 5))[:-1]
    mid = list(np.linspace(t1, t2, n_open))
    above = list(np.linspace(t2, 1, 9))[1:]
    return np.array(below + mid + above)


FULL = Kind('full', 0.27, 0.54, 52, ring_ts(0.27, 0.54, 10))
OPEN = Kind('open', 0.0, 0.48, 62, np.array(list(np.linspace(0, 0.48, 11)) + list(np.linspace(0.48, 1, 9))[1:]))
SEGS = 28


def add_shell(m, kind, shader):
    m.surf('h_shell', shader)
    rows = []
    for t in kind.rings:
        pe = float(kind.phi_e(np.array(t)))
        phis = pe + (2 * np.pi - 2 * pe) * np.arange(SEGS + 1) / SEGS
        P, N = shell_point(phis, np.full_like(phis, t))
        rows.append([m.vert(tuple(P[k]), tuple(N[k]), 'm', (phis[k] / (2 * np.pi), 1 - t)) for k in range(SEGS + 1)])
    for j in range(len(rows) - 1):
        for k in range(SEGS):
            a, b, c, d = rows[j][k], rows[j][k + 1], rows[j + 1][k + 1], rows[j + 1][k]
            m.tri(a, b, c)
            if j + 1 < len(rows) - 1:               # the top row is the pole: one triangle per segment
                m.tri(a, c, d)


def add_visor(m, kind, shader):
    """Visor over the opening, a little proud of the shell."""
    m.surf('h_visor', shader)
    grow = 0.022
    ts = np.linspace(kind.t1 - grow, kind.t2 + grow, 12)
    n = 16
    rows = []
    for t in ts:
        pe = float(kind.phi_e(np.array(t), np.radians(6), grow))
        pe = max(pe, 1e-3)
        phis = np.linspace(-pe, pe, n + 1)
        P, N = shell_point(phis, np.full_like(phis, t), 1.035)
        v = (t - ts[0]) / (ts[-1] - ts[0])
        rows.append([m.vert(tuple(P[k]), tuple(N[k]), 'm', (0.5 + phis[k] / (2 * np.radians(60)), 1 - v)) for k in range(n + 1)])
    for j in range(len(rows) - 1):
        for k in range(n):
            a, b, c, d = rows[j][k], rows[j][k + 1], rows[j + 1][k + 1], rows[j + 1][k]
            m.tri(a, b, c)
            m.tri(a, c, d)


def ellipsoid(m, centre, radii, uv_scale=(1, 1), segs=20, rings=12, zmin=None, inside=False):
    """Closed lat-long ellipsoid (balaclava head, neck caps)."""
    cx = np.array(centre)
    r = np.array(radii)
    rows = []
    for j in range(rings + 1):
        th = -np.pi / 2 + np.pi * j / rings
        row = []
        for k in range(segs + 1):
            ph = 2 * np.pi * k / segs
            d = np.array([np.cos(th) * np.cos(ph), np.cos(th) * np.sin(ph), np.sin(th)])
            p = cx + d * r
            if zmin is not None:
                p[2] = max(p[2], zmin)
            nn = d / r
            nn /= np.linalg.norm(nn)
            if inside:
                nn = -nn
            row.append(m.vert(tuple(p), tuple(nn), 'm', (uv_scale[0] * k / segs, uv_scale[1] * (1 - j / rings))))
        rows.append(row)
    for j in range(rings):
        for k in range(segs):
            a, b, c, d = rows[j][k], rows[j][k + 1], rows[j + 1][k + 1], rows[j + 1][k]
            if inside:
                m.tri(a, c, b)
                m.tri(a, d, c)
            else:
                m.tri(a, b, c)
                m.tri(a, c, d)


def add_trim(m, kind):
    """Rubber edge at the bottom, the dark inside of the shell, the neck,
    visor pivots (full face) or the peak (open face)."""
    m.surf('h_trim', f'{SHARED}/trim')
    # rubber lip: the bottom ring turned inwards
    pe0 = float(kind.phi_e(np.array(0.0)))
    phis = pe0 + (2 * np.pi - 2 * pe0) * np.arange(SEGS + 1) / SEGS
    outer, no = shell_point(phis, np.zeros_like(phis))
    inner, _ = shell_point(phis, np.full_like(phis, 0.12), 0.9)
    lo = outer + (C - outer) * 0.06 + np.array([0, 0, -0.12])
    r0 = [m.vert(tuple(outer[k]), tuple(no[k]), 'm', (k / SEGS, 0.0)) for k in range(SEGS + 1)]
    r1 = [m.vert(tuple(lo[k]), (0, 0, -1), 'm', (k / SEGS, 0.5)) for k in range(SEGS + 1)]
    r2 = [m.vert(tuple(inner[k]), tuple(-(no[k])), 'm', (k / SEGS, 1.0)) for k in range(SEGS + 1)]
    for a_, b_ in ((r0, r1), (r1, r2)):
        for k in range(SEGS):
            m.tri(a_[k], b_[k], b_[k + 1])
            m.tri(a_[k], b_[k + 1], a_[k + 1])
    if kind.name == 'full':
        # chin and cheek padding seen through the bottom: a dark closed cap inside
        ellipsoid(m, C + np.array([0.0, 0, -0.2]), (4.5, 3.9, 4.1), segs=12, rings=8, inside=True)
        for side in (-1, 1):                         # visor pivots
            p, n = shell_point(np.radians(side * 64), 0.5, 1.0)
            m.cyl(tuple(p), tuple(p + n * 0.2), 0.32, mat='m', segs=8, cap0=False)
        neck_top = -6.4
    else:
        neck_top = -6.0
    # neck: fireproof suit collar under the helmet
    m.cyl((C[0] - 0.4, 0, neck_top), (C[0] - 0.4, 0, -7.9), 1.6, 1.7, mat='m', segs=12, cap0=False, cap1=True)


def add_face(m, shader_lens):
    """Open face: a balaclava head with goggles."""
    m.surf('h_face', f'{SHARED}/balaclava')
    ellipsoid(m, C + np.array([0.35, 0, -0.7]), (4.05, 3.55, 4.15), uv_scale=(1, 1), segs=20, rings=14)
    m.surf('h_goggles', f'{SHARED}/trim')
    for side in (-1, 1):
        c0 = np.array([C[0] + 4.25, side * 1.25, C[2]])
        m.cyl(tuple(c0 - [0.55, 0, 0]), tuple(c0), 0.92, 0.88, mat='m', segs=12, cap0=False, cap1=False)
    m.cyl((C[0] + 4.1, -0.5, C[2]), (C[0] + 4.1, 0.5, C[2]), 0.22, mat='m', segs=6)
    m.surf('h_lens', shader_lens)
    for side in (-1, 1):
        c0 = np.array([C[0] + 4.25, side * 1.25, C[2]])
        ids = []
        for k in range(12):
            a = 2 * np.pi * k / 12
            p = c0 + np.array([0.02, np.cos(a) * 0.82, np.sin(a) * 0.82])
            ids.append(m.vert(tuple(p), (1, 0, 0), 'm', (0.5 + np.cos(a) * 0.45, 0.5 - np.sin(a) * 0.45)))
        cen = m.vert(tuple(c0 + [0.08, 0, 0]), (1, 0, 0), 'm', (0.5, 0.5))
        for k in range(12):
            m.tri(cen, ids[k], ids[(k + 1) % 12])


# The cars' tag_head is turned 180 degrees round y (axis -x, y, -z): the head
# models are stored upside down and facing back, with the neck at the origin
# (like the old heads). The helmet is modelled upright and turned here.
TAG_SHIFT = np.array([1.0, 0.0, 7.0])          # upright model -> place above the tag
TAG_SCALE = 0.92                                # a helmet is bigger than a head: fit the cockpits
TAG_AXIS = np.diag([-1.0, 1.0, -1.0])


class TagMesh(Mesh):
    def vert(self, p, n, mat, uv=None):
        p = TAG_AXIS @ ((np.asarray(p, float) + TAG_SHIFT) * TAG_SCALE)
        n = TAG_AXIS @ np.asarray(n, float)
        return super().vert(tuple(float(c) for c in p), tuple(float(c) for c in n), mat, uv)


def helmet(kind, paint_shader, visor_shader):
    m = TagMesh({'m': (0.0, 0.0, 1.0)})
    add_shell(m, kind, paint_shader)
    if kind.name == 'full':
        add_visor(m, kind, visor_shader)
    else:
        add_face(m, visor_shader)
    add_trim(m, kind)
    m.tag('tag_head', (0, 0, 0))
    return m


def paint_points(w=PAINT_W, h=PAINT_H):
    """The point on the helmet under every texel of the paint."""
    u = (np.arange(w) + 0.5) / w
    v = (np.arange(h) + 0.5) / h
    U, V = np.meshgrid(u, v)
    phi, t = U * 2 * np.pi, 1 - V
    P, N = shell_point(phi, t)
    th = theta_of(phi, t)
    q = (P - C) / np.array([5.0, RY, 4.45])
    return {'p': P, 'n': N, 'x': q[..., 0], 'y': q[..., 1], 'z': q[..., 2], 'phi': np.where(phi > np.pi, phi - 2 * np.pi, phi),
            'theta': th, 't': t, 'w': w, 'h': h}


def save_img(path, col, alpha=None, quality=92):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    a = np.clip(col * 255 + 0.5, 0, 255).astype(np.uint8)
    if alpha is not None:
        a = np.dstack([a, np.clip(alpha * 255 + 0.5, 0, 255).astype(np.uint8)])
        Image.fromarray(a, 'RGBA').save(path)
    elif path.endswith('.jpg'):
        Image.fromarray(a, 'RGB').save(path, quality=quality)
    else:
        Image.fromarray(a, 'RGB').save(path)


SHADER_HEAD = """// Driver helmets: written by tools/carbuild/helmets/build_helmets.py, do not edit.
"""


def shader_text(keys):
    out = [SHADER_HEAD]
    for key in keys:
        out.append(f"""
{SHARED}/{key}
{{
	{{
		map {SHARED}/{key}.jpg
		rgbGen lightingDiffuse
	}}
	{{
		map {SHARED}/gloss.jpg
		tcGen environment
		blendFunc GL_ONE GL_ONE
		rgbGen lightingDiffuse
	}}
}}
""")
    for tint in designs.VISORS:
        out.append(f"""
{SHARED}/visor_{tint}
{{
	{{
		map {SHARED}/visor_{tint}.jpg
		rgbGen lightingDiffuse
	}}
	{{
		map {SHARED}/visor_env.jpg
		tcGen environment
		blendFunc GL_ONE GL_ONE
		rgbGen lightingDiffuse
	}}
}}
""")
    return ''.join(out)


def render_icon(md3_path, skin):
    from mdlrender import Scene
    sc = Scene(size=128, ss=4)
    sc.add(md3_path, skin, axis=TAG_AXIS / TAG_SCALE, origin=-TAG_SHIFT)   # back to upright for the picture
    sc.camera((24, 15, 3.5), target=(-0.4, 0.2, -3.6), fov=34)
    sc.light = np.array([0.6, 0.45, 0.7])
    return sc.render(fit=0.94)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--sheet', help='contact sheet of all icons (png)')
    ap.add_argument('--only', default='')
    a = ap.parse_args()
    pts = paint_points()
    shared = designs.shared_textures()
    for name, (col, alpha) in shared.items():
        save_img(os.path.join(BASE, SHARED, name), col, alpha)
    keys = []
    icons = []
    for key, d in designs.DESIGNS.items():
        if a.only and a.only not in key:
            continue
        keys.append(key)
        kind = OPEN if d['kind'] == 'open' else FULL
        col = d['paint'](pts)
        save_img(os.path.join(BASE, SHARED, f'{key}.jpg'), col)
        visor = f"{SHARED}/visor_{d.get('visor', 'smoke')}"
        m = helmet(kind, f'{SHARED}/{key}', visor)
        path = os.path.join(BASE, HEADS, f'{key}.md3')
        n = m.save(path, key)
        skin = {s.name: s.shaders[0][0] for s in m.surfs}
        with open(os.path.join(BASE, HEADS, f'{key}.skin'), 'w', newline='\n') as f:
            for s in m.surfs:
                f.write(f'{s.name},{s.shaders[0][0]}\n')
            f.write('tag_head,\n')
        icon = render_icon(path, skin)
        icon.save(os.path.join(BASE, HEADS, f'icon_{key}.tga'))
        icons.append(icon)
        print(f'{key:12s} {kind.name:5s} {n:5d} triangles')
    if not a.only:
        with open(os.path.join(BASE, 'scripts', 'heads.shader'), 'w', newline='\n') as f:
            f.write(shader_text(keys))
    if a.sheet:
        cols = 6
        rows = (len(icons) + cols - 1) // cols
        S = Image.new('RGBA', (cols * 136, rows * 136), (48, 52, 58, 255))
        for i, im in enumerate(icons):
            S.alpha_composite(im, ((i % cols) * 136 + 4, (i // cols) * 136 + 4))
        S.save(a.sheet)


if __name__ == '__main__':
    main()
