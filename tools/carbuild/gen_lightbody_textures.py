"""Lightbody textures. usage: gen_lightbody_textures.py <textures dir of the source> <outdir with body.md3>
The paint atlas carries the liveries; frame, lift kit, bull bar and lamps keep the source's textures."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import md3, layout
layout.configure(x0=-58.5, x1=58.5, y0=-23.0, y1=23.0, z0=-7.5, z1=24.5, sc=3.42, lx=34)   # same as build_lightbody.py
from paintlib import PLAIN, RALLY, FONT_NUM, FONT_TXT
from PIL import Image, ImageDraw, ImageFont
import numpy as np
TEXDIR, OUT = sys.argv[1:3]
R = 1024

m = md3.load(OUT + '/body.md3')
pos = np.zeros((R, R, 3), np.float32); nrm = np.zeros((R, R, 3), np.float32); used = np.zeros((R, R), bool)
depth = np.full((R, R), -1e9, np.float32)
OUTWARD = {'F': (1, 0, 0), 'B': (-1, 0, 0), 'L': (0, 1, 0), 'R': (0, -1, 0), 'T': (0, 0, 1), 'U': (0, 0, -1)}
def paint_region(n):                       # same rule as build_lightbody.py
    nx, ny, nz = n
    ax = max(abs(nx), abs(ny), abs(nz))
    if nz < -0.55:
        if abs(ny) > 0.25: return 'L' if ny > 0 else 'R'
        if abs(nx) > 0.25: return 'F' if nx > 0 else 'B'
        return 'U'
    if ax == abs(nz): return 'T'
    if ax == abs(ny): return 'L' if ny > 0 else 'R'
    return 'F' if nx > 0 else 'B'
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
        wp = np.stack([a, b, c], -1) @ p
        d = wp @ np.array(OUTWARD[paint_region(n)], np.float32)
        ins &= d > depth[y0:y1+1, x0:x1+1]
        depth[y0:y1+1, x0:x1+1][ins] = d[ins]
        pos[y0:y1+1, x0:x1+1][ins] = wp[ins]; nrm[y0:y1+1, x0:x1+1][ins] = n
        used[y0:y1+1, x0:x1+1] |= ins
x, y, z = pos[..., 0], pos[..., 1], pos[..., 2]
nx, ny, nz = nrm[..., 0], nrm[..., 1], nrm[..., 2]
top, side, endf = nz > 0.6, np.abs(ny) > 0.55, np.abs(nx) > 0.55

def decal(img, msk, u0, v0, width, rgba, U, V):
    h, w = rgba.shape[:2]
    iu = (U - u0)/width*w + w/2; iv = (V - v0)/width*w + h/2
    ok = msk & (iu >= 0) & (iu < w) & (iv >= 0) & (iv < h)
    px = rgba[np.clip(iv.astype(int), 0, h-1), np.clip(iu.astype(int), 0, w-1)].astype(np.float32)
    a = (px[..., 3:4]/255)*ok[..., None]
    img[..., :3] = img[..., :3]*(1-a) + px[..., :3]*a
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

def paint(name, base, sc=None, two_tone=None):
    img = np.zeros((R, R, 4), np.float32)
    k = 0.84 + 0.18*np.clip((z + 6)/30, 0, 1)
    img[..., :3] = np.array(base, np.float32)[None, None, :]*k[..., None]; img[..., 3] = 212
    if two_tone:                                            # 80s two-tone: lower flanks in a second colour
        msk = used & side & (z < 4.5)
        img[msk, :3] = np.array(two_tone, np.float32)*k[msk][:, None]
        img[used & side & (np.abs(z - 4.8) < 0.3), :3] = (230, 230, 230)
    if sc:
        for y0, y1, c in [(-1.6, 7.6, sc['stripe']), (-4.0, -2.8, sc['edge'])]:
            msk = used & (top | endf) & (y > y0) & (y < y1)
            img[msk, :3] = np.array(c, np.float32)*k[msk][:, None]
        img[used & side & (z > -1.6) & (z < 0.8), :3] = sc['stripe']
        img[used & side & (z > -2.8) & (z < -2.2), :3] = sc['edge']
    if sc:
        num = roundel_img(sc)
        L, Rr = used & side & (y > 0), used & side & (y < 0)
        decal(img, L, -9.0, -8.0, 12.0, num, -x, -z)          # doors
        decal(img, Rr, 9.0, -8.0, 12.0, num, x, -z)
        decal(img, used & top & (z > 22), 6.0, 0.0, 15.0, num, -y, -x)        # cab roof, chase cam
        tc = tuple(int(c) for c in (sc['stripe'] if sum(sc['body']) < 400 else sc['edge']))
        q3 = text_img('Q3RALLY', tc)
        decal(img, L, 36.0, -6.0, 15.0, q3, -x, -z)           # bed sides
        decal(img, Rr, -36.0, -6.0, 15.0, q3, x, -z)
    img[~used] = img[used].reshape(-1, 4).mean(0)
    Image.fromarray(np.clip(img, 0, 255).astype(np.uint8), 'RGBA').resize((R//2, R//2), Image.LANCZOS).save(f'{OUT}/{name}_main.tga')

for n, c in PLAIN.items(): paint(n, c)
paint('sunset', (226, 120, 30), two_tone=(70, 40, 24))        # brown/orange 80s two-tone
for n, sc in RALLY.items(): paint(n, sc['body'], sc)

Image.open(os.path.join(TEXDIR, 'UCB_BOTTOM_DIF.jpg')).convert('RGB').resize((512, 512), Image.LANCZOS).save(f'{OUT}/dark.tga')
Image.open(os.path.join(TEXDIR, 'UCB_Lights_and_Glass_Dif.jpg')).convert('RGB').resize((512, 512), Image.LANCZOS).save(f'{OUT}/lights.tga')
yy = np.linspace(0, 1, 128)[:, None]*np.ones((1, 128))
g = np.zeros((128, 128, 4), np.uint8)
g[..., 0] = 16 + 26*(1-yy); g[..., 1] = 20 + 30*(1-yy); g[..., 2] = 28 + 36*(1-yy); g[..., 3] = 165
Image.fromarray(g).save(f'{OUT}/glass.tga')
print('ok')
