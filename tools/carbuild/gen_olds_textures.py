"""Cutlass textures. usage: gen_olds_textures.py <textures dir of the source> <outdir with body.md3 + paint_src.npz>

The paint atlas carries the source's baked ambient occlusion (panel gaps,
creases), found for each atlas texel through the nearest triangle of the
original high-poly paint mesh. Liveries as on the other cars; the flanks are
unique in the atlas, so the door numbers read correctly on both sides."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import md3, layout
layout.configure(x0=-62.5, x1=62.5, y0=-24.5, y1=24.5, z0=-12.0, z1=18.5, sc=3.48, lx=34)   # same as build_olds.py
from paintlib import PLAIN, RALLY, FONT_NUM, FONT_TXT
from PIL import Image, ImageDraw, ImageFont
import numpy as np, zlib
TEXDIR, OUT = sys.argv[1:3]
R = 1024                                         # painted at 2x, saved at 512

# ---------------- texel -> position on the decimated paint ---------------------------
m = md3.load(OUT + '/body.md3')
pos = np.zeros((R, R, 3), np.float32); nrm = np.zeros((R, R, 3), np.float32); used = np.zeros((R, R), bool)
for s in m.surfs:
    if not s.name.startswith('paint'): continue
    V = np.array([[c/64 for c in v[:3]] for v in s.frames[0]]); ST = np.array(s.st)
    for t in s.tris:
        uv = ST[t]*R; p = V[t]
        n = np.cross(p[2]-p[0], p[1]-p[0]); n /= (np.linalg.norm(n) or 1)
        x0, y0 = np.floor(uv.min(0)).astype(int) - 1; x1, y1 = np.ceil(uv.max(0)).astype(int) + 1
        x0, y0 = max(x0, 0), max(y0, 0); x1, y1 = min(x1, R-1), min(y1, R-1)
        gx, gy = np.meshgrid(np.arange(x0, x1+1)+0.5, np.arange(y0, y1+1)+0.5)
        (ua, va), (ub, vb), (uc, vc) = uv
        den = (vb-vc)*(ua-uc) + (uc-ub)*(va-vc)
        if abs(den) < 1e-9: continue
        a = ((vb-vc)*(gx-uc) + (uc-ub)*(gy-vc))/den; b = ((vc-va)*(gx-uc) + (ua-uc)*(gy-vc))/den; c = 1-a-b
        ins = (a >= -0.04) & (b >= -0.04) & (c >= -0.04)
        w = np.stack([a, b, c], -1)
        pos[y0:y1+1, x0:x1+1][ins] = (w @ p)[ins]; nrm[y0:y1+1, x0:x1+1][ins] = n
        used[y0:y1+1, x0:x1+1] |= ins

# ---------------- ambient occlusion bake ------------------------------------------------
src = np.load(OUT + '/paint_src.npz'); ST3, SUV = src['tri'], src['uv']
AO = np.asarray(Image.open(os.path.join(TEXDIR, 'M_Body_Mixed_AO.jpeg')).convert('L').resize((2048, 2048), Image.LANCZOS)).astype(np.float32)/255
cent = ST3.mean(1); CELL = 2.0
cell = {}
for i, c in enumerate(np.floor(cent/CELL).astype(int)):
    cell.setdefault(tuple(c), []).append(i)
ys, xs = np.nonzero(used); P = pos[ys, xs]; Nn = nrm[ys, xs]
ao = np.ones(len(P), np.float32)
keys = np.floor(P/CELL).astype(int)
order = np.lexsort(keys.T[::-1]); keys_s = keys[order]
split = np.nonzero(np.any(np.diff(keys_s, axis=0) != 0, axis=1))[0] + 1
for grp in np.split(order, split):
    k = tuple(keys[grp[0]])
    cand = [i for dx in (-1, 0, 1) for dy in (-1, 0, 1) for dz in (-1, 0, 1) for i in cell.get((k[0]+dx, k[1]+dy, k[2]+dz), [])]
    if not cand: continue
    cand = np.array(cand); A, B, C = ST3[cand, 0], ST3[cand, 1], ST3[cand, 2]
    p = P[grp][:, None, :]
    # closest point on each candidate triangle (barycentric, clamped)
    e0, e1 = B - A, C - A; v2 = p - A
    d00 = (e0*e0).sum(-1); d01 = (e0*e1).sum(-1); d11 = (e1*e1).sum(-1)
    d20 = (v2*e0).sum(-1); d21 = (v2*e1).sum(-1)
    den = np.maximum(d00*d11 - d01*d01, 1e-12)
    bv = np.clip((d11*d20 - d01*d21)/den, 0, 1); bw = np.clip((d00*d21 - d01*d20)/den, 0, 1)
    sw = np.maximum(bv + bw, 1e-12); over = sw > 1; bv = np.where(over, bv/sw, bv); bw = np.where(over, bw/sw, bw)
    q = A + bv[..., None]*e0 + bw[..., None]*e1
    dist = ((q - p)**2).sum(-1)
    # prefer triangles facing the same way (front skin, not the inner side of a panel)
    fn = np.cross(C - A, B - A); fn /= np.maximum(np.linalg.norm(fn, axis=-1, keepdims=True), 1e-9)
    facing = (fn[None, :, :]*Nn[grp][:, None, :]).sum(-1)
    dist = dist + (facing < 0.3)*100
    j = dist.argmin(1); bu, bvv = bv[np.arange(len(grp)), j], bw[np.arange(len(grp)), j]
    uvt = SUV[cand[j]]
    uv = uvt[:, 0]*(1 - bu - bvv)[:, None] + uvt[:, 1]*bu[:, None] + uvt[:, 2]*bvv[:, None]
    tx = np.clip((uv[:, 0] % 1)*2047, 0, 2047).astype(int); ty = np.clip((1 - uv[:, 1] % 1)*2047, 0, 2047).astype(int)
    ao[grp] = AO[ty, tx]
AOmap = np.ones((R, R), np.float32); AOmap[ys, xs] = 0.35 + 0.65*np.clip(ao, 0, 1)
ux, uy = int(layout.R_UNDER[0]*R/layout.A), int(layout.R_UNDER[1]*R/layout.A)
AOmap[uy:, ux:] = 0.8                     # the flat underside patch is too small for a meaningful bake

x, y, z = pos[..., 0], pos[..., 1], pos[..., 2]
nx, ny, nz = nrm[..., 0], nrm[..., 1], nrm[..., 2]
top, side = nz > 0.6, np.abs(ny) > 0.55
endf = np.abs(nx) > 0.55

def decal(img, msk, u0, v0, width, rgba, U, V):
    h, w = rgba.shape[:2]
    iu = (U - u0)/width*w + w/2; iv = (V - v0)/width*w + h/2
    ok = msk & (iu >= 0) & (iu < w) & (iv >= 0) & (iv < h)
    px = rgba[np.clip(iv.astype(int), 0, h-1), np.clip(iu.astype(int), 0, w-1)].astype(np.float32)
    a = (px[..., 3:4]/255)*ok[..., None]
    img[..., :3] = img[..., :3]*(1-a) + px[..., :3]*AOmap[..., None]*a
def roundel_img(sc, size=256):
    im = Image.new('RGBA', (size, size), (0, 0, 0, 0)); d = ImageDraw.Draw(im); c = size/2; r = size/2 - 2
    d.ellipse([c-r, c-r*0.86, c+r, c+r*0.86], fill=sc['ring'] + (255,))
    d.ellipse([c-r*0.82, c-r*0.70, c+r*0.82, c+r*0.70], fill=(245, 245, 245, 255))
    f = ImageFont.truetype(FONT_NUM, int(r*(1.05 if len(sc['num']) == 1 else 0.92)))
    d.text((c, c+r*0.02), sc['num'], font=f, fill=sc['ink'] + (255,), anchor='mm')
    return np.asarray(im)
def text_img(txt, col, h=120):
    f = ImageFont.truetype(FONT_TXT, int(h*0.8)); w = int(f.getlength(txt)) + 30
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0)); ImageDraw.Draw(im).text((w/2, h/2), txt, font=f, fill=col + (255,), anchor='mm')
    return np.asarray(im)

def paint(name, base, sc=None):
    img = np.zeros((R, R, 4), np.float32)
    k = (0.86 + 0.16*np.clip((z + 10)/26, 0, 1))
    img[..., :3] = np.array(base, np.float32)[None, None, :]*k[..., None]; img[..., 3] = 212
    if sc:
        for y0, y1, c in [(-1.6, 7.6, sc['stripe']), (-4.0, -2.8, sc['edge'])]:
            msk = used & (top | endf) & (y > y0) & (y < y1)
            img[msk, :3] = np.array(c, np.float32)*k[msk][:, None]
        img[used & side & (z > -7.4) & (z < -5.0), :3] = sc['stripe']
        img[used & side & (z > -8.6) & (z < -8.0), :3] = sc['edge']
    img[..., :3] *= AOmap[..., None]
    if sc:
        num = roundel_img(sc)
        L, Rr = used & side & (y > 0), used & side & (y < 0)
        decal(img, L, -3.0, -1.0, 12.0, num, -x, -z)
        decal(img, Rr, 3.0, -1.0, 12.0, num, x, -z)
        decal(img, used & top & (z > 14), 0.0, 2.0, 14.0, num, -y, -x)          # roof, chase cam
        tc = tuple(int(c) for c in (sc['stripe'] if sum(sc['body']) < 400 else sc['edge']))
        q3 = text_img('Q3RALLY', tc)
        decal(img, L, -42.0, -2.0, 13.0, q3, -x, -z)
        decal(img, Rr, 42.0, -2.0, 13.0, q3, x, -z)
    img[~used] = img[used].reshape(-1, 4).mean(0)
    Image.fromarray(np.clip(img, 0, 255).astype(np.uint8), 'RGBA').resize((R//2, R//2), Image.LANCZOS).save(f'{OUT}/{name}_main.tga')

for n, c in PLAIN.items(): paint(n, c)
paint('turquoise', (40, 150, 150))                 # a period colour
for n, sc in RALLY.items(): paint(n, sc['body'], sc)

def flat(name, rgb, alpha=255, size=32):
    rnd = np.random.RandomState(zlib.crc32(name.encode()) % 1000).rand(size, size, 1)*8 - 4
    px = np.clip(np.array(rgb)[None, None, :] + rnd, 0, 255).astype(np.uint8)
    Image.fromarray(np.dstack([px, np.full((size, size), alpha, np.uint8)]), 'RGBA').save(f'{OUT}/{name}.tga')
flat('black', (20, 20, 22))
yy = np.linspace(0, 1, 128)[:, None]*np.ones((1, 128))
c = np.zeros((128, 128, 4), np.uint8)
prof = np.interp(yy, [0, 0.45, 0.5, 0.55, 1], [235, 175, 80, 150, 205])
c[..., 0] = prof; c[..., 1] = prof; c[..., 2] = np.minimum(255, prof*1.04); c[..., 3] = 110
Image.fromarray(c).save(f'{OUT}/chrome.tga')
g = np.zeros((128, 128, 4), np.uint8)
g[..., 0] = 16 + 26*(1-yy); g[..., 1] = 20 + 30*(1-yy); g[..., 2] = 28 + 36*(1-yy); g[..., 3] = 165
Image.fromarray(g).save(f'{OUT}/glass.tga')
Image.open(os.path.join(TEXDIR, 'M_Lights_Diffuse.png')).convert('RGB').resize((512, 512), Image.LANCZOS).save(f'{OUT}/lights.tga')
print('ok, ao range', float(ao.min()), float(ao.max()))
