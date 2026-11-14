"""Small textured software renderer for md3 models (icons and previews).

The same idea as render3d.py, but as a library: any number of md3 models,
each with its own textures and placement, one camera.

    from mdlrender import Scene
    sc = Scene(size=128)
    sc.add('baseq3r/models/players/heads/classic.md3', skin='.../classic.skin')
    sc.camera((30, 18, 4), target=(0, 0, -3), fov=30)
    sc.render().save('icon.png')

Textures are found from the skin file or the surface's own shader name
(.tga, .jpg or .png under the base folder). Surfaces whose texture path
contains one of `shiny` get a simple sky reflection, like chrome or a visor.
"""
import math
import os

import numpy as np
from PIL import Image

import md3

ROOT = os.environ.get('Q3R_BASE', os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'baseq3r'))


def load_skin(path):
    d = {}
    for line in open(path):
        if ',' in line:
            a, b = line.strip().split(',', 1)
            if b:
                d[a.lower()] = b
    return d


_tex = {}


def texture(name, root=ROOT):
    if name not in _tex:
        stem = os.path.splitext(name)[0]
        img = None
        for ext in ('', '.tga', '.jpg', '.png'):
            p = os.path.join(root, (stem + ext) if ext else name)
            if ext == '' and os.path.splitext(name)[1] == '':
                continue
            if os.path.exists(p):
                img = Image.open(p).convert('RGBA')
                break
        if img is None:
            img = Image.new('RGBA', (4, 4), (255, 0, 255, 255))
        _tex[name] = np.asarray(img).astype(np.float32) / 255
    return _tex[name]


def sky(r):
    z = r[..., 2:3]
    top = np.array([0.55, 0.66, 0.85])
    hor = np.array([0.95, 0.95, 0.97])
    gnd = np.array([0.25, 0.23, 0.22])
    return np.where(z > 0, hor + (top - hor) * np.clip(z * 1.6, 0, 1), hor * 0.6 + (gnd - hor * 0.6) * np.clip(-z * 4, 0, 1))


class Scene:
    def __init__(self, size=256, ss=3, shiny=('visor', 'chrome', 'bronze', 'glass')):
        self.size, self.ss, self.shiny = size, ss, shiny
        self.items = []
        self.cam = np.array([100.0, 0, 0])
        self.target = np.zeros(3)
        self.fov = 30.0
        self.light = np.array([0.45, 0.35, 0.82])

    def add(self, model, skin=None, origin=(0, 0, 0), axis=None, textures=None):
        """model: md3 path or md3.Model; skin: .skin path or {surface: texture};
        textures: {texture name: replacement image path} (optional)."""
        m = md3.load(model) if isinstance(model, str) else model
        sk = load_skin(skin) if isinstance(skin, str) else (skin or {})
        R = np.array(axis, float) if axis is not None else np.eye(3)
        O = np.array(origin, float)
        for s in m.surfs:
            tp = sk.get(s.name.lower(), s.shaders[0][0] if s.shaders else '')
            if not tp:
                continue
            V = np.array([[c / 64 for c in v[:3]] for v in s.frames[0]]) @ R + O
            N = np.array([md3.dec_normal(v[3]) for v in s.frames[0]]) @ R
            tx = texture((textures or {}).get(tp, tp))
            self.items.append((V, N, np.array(s.st), np.array(s.tris), tx, any(k in tp for k in self.shiny)))

    def camera(self, pos, target=(0, 0, 0), fov=30.0):
        self.cam, self.target, self.fov = np.array(pos, float), np.array(target, float), fov

    def render(self, fit=None, bg=None):
        W = self.size * self.ss
        fwd = self.target - self.cam
        fwd /= np.linalg.norm(fwd)
        right = np.cross(fwd, [0, 0, 1])
        right /= np.linalg.norm(right)
        up = np.cross(right, fwd)
        f = W / 2 / math.tan(math.radians(self.fov / 2))
        L = self.light / np.linalg.norm(self.light)
        color = np.zeros((W, W, 3), np.float32)
        alpha = np.zeros((W, W), np.float32)
        zbuf = np.full((W, W), 1e9, np.float32)
        for V, Nn, ST, T, TX, shiny in self.items:
            rel = V - self.cam
            cz, cx, cy = rel @ fwd, rel @ right, rel @ up
            sx, sy = W / 2 + f * cx / cz, W / 2 - f * cy / cz
            th, tw = TX.shape[:2]
            for a, c, b in T:                      # md3 stores clockwise: swap back
                xs = np.array([sx[a], sx[b], sx[c]])
                ys = np.array([sy[a], sy[b], sy[c]])
                den = (ys[1] - ys[2]) * (xs[0] - xs[2]) + (xs[2] - xs[1]) * (ys[0] - ys[2])
                if den >= -1e-9:                    # back face (or degenerate): not drawn, like the game
                    continue
                x0, x1 = max(int(xs.min()), 0), min(int(xs.max()) + 1, W - 1)
                y0, y1 = max(int(ys.min()), 0), min(int(ys.max()) + 1, W - 1)
                if x1 < x0 or y1 < y0:
                    continue
                gx, gy = np.meshgrid(np.arange(x0, x1 + 1) + 0.5, np.arange(y0, y1 + 1) + 0.5)
                w0 = ((ys[1] - ys[2]) * (gx - xs[2]) + (xs[2] - xs[1]) * (gy - ys[2])) / den
                w1 = ((ys[2] - ys[0]) * (gx - xs[2]) + (xs[0] - xs[2]) * (gy - ys[2])) / den
                w2 = 1 - w0 - w1
                inside = (w0 >= -1e-4) & (w1 >= -1e-4) & (w2 >= -1e-4)
                if not inside.any():
                    continue
                iz = np.array([1 / cz[a], 1 / cz[b], 1 / cz[c]])
                pw = np.stack([w0 * iz[0], w1 * iz[1], w2 * iz[2]], -1)
                zi = pw.sum(-1)
                pw = pw / zi[..., None]
                depth = 1 / zi
                sub = zbuf[y0:y1 + 1, x0:x1 + 1]
                msk = inside & (depth < sub)
                if not msk.any():
                    continue
                uv = pw @ ST[[a, b, c]]
                tu = (uv[..., 0] % 1.0) * tw
                tv = (uv[..., 1] % 1.0) * th
                texel = TX[np.clip(tv.astype(int), 0, th - 1), np.clip(tu.astype(int), 0, tw - 1)]
                n = pw @ Nn[[a, b, c]]
                n /= np.linalg.norm(n, axis=-1, keepdims=True) + 1e-9
                p = pw @ V[[a, b, c]]
                vd = p - self.cam
                vd /= np.linalg.norm(vd, axis=-1, keepdims=True)
                diff = np.clip(n @ L, 0, 1)[..., None]
                rgb = texel[..., :3] * (0.42 + 0.7 * diff)
                r = vd - 2 * (vd * n).sum(-1, keepdims=True) * n
                if shiny:
                    fres = 0.25 + 0.5 * (1 - np.abs((vd * n).sum(-1, keepdims=True))) ** 2
                    rgb = rgb * (1 - fres) + sky(r) * texel[..., :3] * fres * 1.4
                spec = np.clip(r @ L, 0, 1)[..., None] ** 30
                rgb = rgb + spec * (0.6 if shiny else 0.12)
                color[y0:y1 + 1, x0:x1 + 1][msk] = np.clip(rgb, 0, 1)[msk]
                alpha[y0:y1 + 1, x0:x1 + 1][msk] = 1
                sub[msk] = depth[msk]
        im = Image.fromarray((np.dstack([color, alpha]) * 255).astype(np.uint8), 'RGBA')
        if fit:                                     # scale the model to fill `fit` of the frame, centred
            bx = im.getbbox()
            if bx:
                obj = im.crop(bx)
                sc = fit * W / max(obj.width, obj.height)
                obj = obj.resize((max(1, int(obj.width * sc)), max(1, int(obj.height * sc))), Image.LANCZOS)
                im = Image.new('RGBA', (W, W), (0, 0, 0, 0))
                im.alpha_composite(obj, ((W - obj.width) // 2, (W - obj.height) // 2))
        im = im.resize((self.size, self.size), Image.LANCZOS)
        if bg:
            out = Image.new('RGBA', im.size, bg)
            out.alpha_composite(im)
            im = out
        return im
