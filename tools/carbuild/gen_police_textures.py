"""Sheriff car skins from the hand painted textures.
usage: gen_police_textures.py <sferiff.jpg> <sferiffi.jpg> <body.md3> <outdir>

- clears all texels the mesh does not use (spare tyre with a brand name etc.)
- 'sheriff' is the original livery; the colour skins tint the white door/roof
  panels and keep rust and dirt; the rally skins also paint the sheriff decals
  out and add stripes and a roof number (the flanks share one mirrored texture
  region, so numbers only go on the roof)"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import md3
from paintlib import PLAIN, RALLY, FONT_NUM
from PIL import Image, ImageDraw, ImageFont
import numpy as np
SHELL, PARTS, BODY, OUT = sys.argv[1:5]
N = 512

m = md3.load(BODY)
def texmap(prefix):
    pos = np.zeros((N, N, 3), np.float32); nrm = np.zeros((N, N, 3), np.float32); used = np.zeros((N, N), bool)
    for s in m.surfs:
        if not s.name.startswith(prefix): continue
        V = np.array([[c/64 for c in v[:3]] for v in s.frames[0]]); ST = np.array(s.st)
        # roof and headliner share texels: draw low to high so the outer skin wins
        for t in sorted(s.tris, key=lambda t: V[t][:, 2].mean()):
            uv = ST[t]*N; p = V[t]
            n = np.cross(p[2]-p[0], p[1]-p[0]); n /= (np.linalg.norm(n) or 1)
            x0, y0 = np.floor(uv.min(0)).astype(int); x1, y1 = np.ceil(uv.max(0)).astype(int)
            x0, y0 = max(x0, 0), max(y0, 0); x1, y1 = min(x1, N-1), min(y1, N-1)
            if x1 < x0 or y1 < y0: continue
            gx, gy = np.meshgrid(np.arange(x0, x1+1)+0.5, np.arange(y0, y1+1)+0.5)
            (ua, va), (ub, vb), (uc, vc) = uv
            den = (vb-vc)*(ua-uc) + (uc-ub)*(va-vc)
            if abs(den) < 1e-9: continue
            a = ((vb-vc)*(gx-uc) + (uc-ub)*(gy-vc))/den; b = ((vc-va)*(gx-uc) + (ua-uc)*(gy-vc))/den; c = 1-a-b
            ins = (a >= -0.03) & (b >= -0.03) & (c >= -0.03)
            w = np.stack([a, b, c], -1)
            pos[y0:y1+1, x0:x1+1][ins] = (w @ p)[ins]; nrm[y0:y1+1, x0:x1+1][ins] = n
            used[y0:y1+1, x0:x1+1] |= ins
    return pos, nrm, used
def dilate(msk, r):
    out = msk.copy()
    for dy in range(-r, r+1):
        for dx in range(-r, r+1):
            out |= np.roll(np.roll(msk, dy, 0), dx, 1)
    return out
def inpaint(img, msk, iters=300):
    img = img.copy(); known = ~msk
    for _ in range(iters):
        acc = np.zeros_like(img); cnt = np.zeros(img.shape[:2], np.float32)
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            k = np.roll(np.roll(known, dy, 0), dx, 1); v = np.roll(np.roll(img, dy, 0), dx, 1)
            acc += v*k[..., None]; cnt += k
        upd = msk & (cnt > 0) & ~known
        img[upd] = acc[upd]/cnt[upd][:, None]; known = known | upd
    return img
def save(img, name, alpha=None):
    a = np.full((N, N), 255, np.uint8) if alpha is None else alpha
    Image.fromarray(np.dstack([np.clip(img, 0, 255).astype(np.uint8), a]), 'RGBA').save(f'{OUT}/{name}.tga')

# ---------------- engine / interior / underbody ---------------------------------------
_, _, pused = texmap('parts')
parts = np.asarray(Image.open(PARTS).convert('RGB').resize((N, N))).astype(np.float32)
parts[~dilate(pused, 4)] = (6, 6, 6)
save(parts, 'parts')

# ---------------- shell ------------------------------------------------------------
pos, nrm, used = texmap('body')
tex = np.asarray(Image.open(SHELL).convert('RGB').resize((N, N))).astype(np.float32)
keep = dilate(used, 4)
tex[~keep] = (6, 6, 6)
mx, mn = tex.max(-1), tex.min(-1)
lum = tex.mean(-1); sat = (mx - mn)/np.maximum(mx, 1)
panel_w = np.clip((lum - 105)/60, 0, 1)*np.clip((0.30 - sat)/0.12, 0, 1)*keep      # white door/roof panels
panel_w[340:440, 130:380] = 0                                     # headlights and grille stay as painted
panel = panel_w > 0.5
# sheriff decals (star, lettering): coloured texels mostly surrounded by white panel
# sheriff decals: roof star, roof lettering, door star (texture pixel boxes)
decal = np.zeros((N, N), bool)
for x0, y0, x1, y1 in ((380, 62, 456, 146), (346, 30, 380, 176), (206, 246, 270, 316)):
    decal[y0:y1, x0:x1] = True
decal &= keep

def tint(img, rgb, msk_w, lum_src=None):
    l = lum if lum_src is None else lum_src
    col = np.array(rgb, np.float32)[None, None, :]
    shade = np.clip(l/215, 0, 1.15)[..., None]
    return img*(1 - msk_w[..., None]) + col*shade*msk_w[..., None]

alpha = np.full((N, N), 255, np.uint8); alpha[panel] = 220        # light gloss on the panels only
save(tex, 'sheriff', alpha)
for n, c in PLAIN.items(): save(tint(tex, c, panel_w), n, alpha)

x, y, z = pos[..., 0], pos[..., 1], pos[..., 2]
up = (nrm[..., 2] > 0.6) & used
def roundel_img(sc, size=256):
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0)); d = ImageDraw.Draw(img); c = size/2; r = size/2 - 2
    d.ellipse([c-r, c-r*0.86, c+r, c+r*0.86], fill=sc['ring'] + (255,))
    d.ellipse([c-r*0.82, c-r*0.70, c+r*0.82, c+r*0.70], fill=(245, 245, 245, 255))
    f = ImageFont.truetype(FONT_NUM, int(r*(1.05 if len(sc['num']) == 1 else 0.92)))
    d.text((c, c+r*0.02), sc['num'], font=f, fill=sc['ink'] + (255,), anchor='mm')
    return np.asarray(img).astype(np.float32)
clean = inpaint(tex, decal)                                        # sheriff decals painted out
cl_lum = clean.mean(-1)
cl_sat = (clean.max(-1) - clean.min(-1))/np.maximum(clean.max(-1), 1)
cl_panel_w = np.clip((cl_lum - 105)/60, 0, 1)*np.clip((0.30 - cl_sat)/0.12, 0, 1)*keep
cl_panel_w[340:440, 130:380] = 0
for n, sc in RALLY.items():
    out = tint(clean, sc['body'], cl_panel_w, cl_lum)
    for y0, y1, c in [(-1.4, 6.6, sc['stripe']), (-3.6, -2.4, sc['edge'])]:
        msk = up & (y > y0) & (y < y1)
        out[msk] = (np.array(c, np.float32)[None, :]*np.clip(cl_lum[msk]/200, 0.55, 1.1)[:, None])
    im = roundel_img(sc); k = im.shape[0]; rad = 6.5; cx, cy = -4.0, 0.0
    iu = ((cy - y)/(2*rad) + 0.5)*k; iv = ((cx - x)/(2*rad) + 0.5)*k              # readable from the chase cam
    ok = up & (z > 10) & (iu >= 0) & (iu < k) & (iv >= 0) & (iv < k)
    px = im[np.clip(iv.astype(int), 0, k-1), np.clip(iu.astype(int), 0, k-1)]
    a = (px[..., 3:4]/255)*ok[..., None]
    out = out*(1 - a) + px[..., :3]*np.clip(cl_lum/200, 0.75, 1.05)[..., None]*a
    save(out, n, alpha)
print('panel texels', int(panel.sum()), 'decal texels', int(decal.sum()))
