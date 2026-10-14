"""Paint the stock car texture (STOCKCAR.BMP is lost) for the atlas UVs of build_stockcar.py.
usage: gen_stockcar_textures.py <body.md3> <outdir>

Every texel is mapped back to its position and face normal on the body, so
windows, sticker headlights, grille, tail lights, stripes and the big door and
roof numbers are all placed in model space."""
import os, sys, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import md3
from paintlib import PLAIN, RALLY, FONT_NUM, FONT_TXT
from PIL import Image, ImageDraw, ImageFont
import numpy as np, zlib
BODY, OUT = sys.argv[1:3]
R = 1024                                   # painted at 2x, saved at 512

m = md3.load(BODY)
s = [x for x in m.surfs if x.name == 'body'][0]
tags = {n: o for n, o, _ in m.tags[0]}
V = np.array([[c/64 for c in v[:3]] for v in s.frames[0]]); ST = np.array(s.st)
pos = np.zeros((R, R, 3), np.float32); nrm = np.zeros((R, R, 3), np.float32); used = np.zeros((R, R), bool)
tri_id = np.full((R, R), -1)
TRI = []                                   # (centroid, face normal) per triangle
for ti, t in enumerate(s.tris):
    uv = ST[t]*R; p = V[t]
    n = np.cross(p[2]-p[0], p[1]-p[0]); n /= (np.linalg.norm(n) or 1)
    TRI.append((p.mean(0), n))
    x0, y0 = np.floor(uv.min(0)).astype(int) - 1; x1, y1 = np.ceil(uv.max(0)).astype(int) + 1
    x0, y0 = max(x0, 0), max(y0, 0); x1, y1 = min(x1, R-1), min(y1, R-1)
    gx, gy = np.meshgrid(np.arange(x0, x1+1)+0.5, np.arange(y0, y1+1)+0.5)
    (ua, va), (ub, vb), (uc, vc) = uv
    den = (vb-vc)*(ua-uc) + (uc-ub)*(va-vc)
    if abs(den) < 1e-9: continue
    a = ((vb-vc)*(gx-uc) + (uc-ub)*(gy-vc))/den; b = ((vc-va)*(gx-uc) + (ua-uc)*(gy-vc))/den; c = 1-a-b
    inside = (a >= -0.04) & (b >= -0.04) & (c >= -0.04)          # small bleed against seams
    w = np.stack([a, b, c], -1)
    pos[y0:y1+1, x0:x1+1][inside] = (w @ p)[inside]
    nrm[y0:y1+1, x0:x1+1][inside] = n
    tri_id[y0:y1+1, x0:x1+1][inside] = ti
    used[y0:y1+1, x0:x1+1] |= inside
x, y, z = pos[..., 0], pos[..., 1], pos[..., 2]
nx, ny, nz = nrm[..., 0], nrm[..., 1], nrm[..., 2]
front, rear, top = nx > 0.35, nx < -0.35, nz > 0.6
side = np.abs(ny) > 0.5

# ---------------- masks ---------------------------------------------------------------
# windows follow the mesh: whole triangles of the greenhouse
def is_glass(c, n):
    cx, cy, cz = c; nx_, ny_, nz_ = n
    if cz < 2.6 or cz > 12.0: return False
    if abs(ny_) > 0.8 and -12 < cx < 18: return True                      # side windows
    if nx_ > 0.3 and nz_ > 0.3 and abs(cy) < 14 and cx > 0: return True    # windscreen
    if nx_ < -0.3 and nz_ > 0.3 and abs(cy) < 9 and cz > 4.4: return True  # rear window
    return False
glass_tris = np.array([is_glass(c, n) for c, n in TRI] + [False])
glass = used & glass_tris[tri_id]
hl = tags['tag_hlite1']
headl = used & front & ~glass & ((((np.abs(y) - hl[1])/3.4)**2 + ((z - hl[2])/1.8)**2) < 1)
grille = used & front & ~glass & (np.abs(y) < 8.5) & (z > -7.4) & (z < -3.6)
bl, rl = tags['tag_blite2'], tags['tag_rlite2']
# tail lights: band on the vertical upper tail panel
tail = used & (nx < -0.7) & (np.abs(z - bl[2]) < 1.5) & (np.abs(y) > rl[1] - 1.6) & (np.abs(y) < bl[1] + 2.0)
reverse = tail & (np.abs(np.abs(y) - rl[1]) < 1.1)
sill = used & side & (z < -13.2)

def decal(img, msk, u0, v0, width, rgba, U, V):
    """project an RGBA image (width in world units, centred on u0/v0) onto the
    texels in msk; U/V are the world quantities along the image's x/y axes"""
    h, w = rgba.shape[:2]
    iu = (U - u0)/width*w + w/2; iv = (V - v0)/width*w + h/2
    ok = msk & (iu >= 0) & (iu < w) & (iv >= 0) & (iv < h)
    px = rgba[np.clip(iv.astype(int), 0, h-1), np.clip(iu.astype(int), 0, w-1)].astype(np.float32)
    a = (px[..., 3:4]/255)*ok[..., None]
    img[..., :3] = img[..., :3]*(1-a) + px[..., :3]*a
    img[..., 3:4] = np.maximum(img[..., 3:4], 255*a)

def number_img(sc, w=384, h=300):
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    d.rounded_rectangle([4, 4, w-4, h-4], radius=40, fill=(245, 245, 245, 255), outline=sc['ring'] + (255,), width=16)
    f = ImageFont.truetype(FONT_NUM, int(h*0.72))
    d.text((w/2, h/2 + 6), sc['num'], font=f, fill=sc['ink'] + (255,), anchor='mm', stroke_width=4, stroke_fill=sc['ring'])
    return np.asarray(im)
def text_img(txt, col, h=120):
    f = ImageFont.truetype(FONT_TXT, int(h*0.8)); w = int(f.getlength(txt)) + 30
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0)); ImageDraw.Draw(im).text((w/2, h/2), txt, font=f, fill=col + (255,), anchor='mm')
    return np.asarray(im)

def paint(name, base, sc=None):
    img = np.zeros((R, R, 4), np.float32)
    k = (0.80 + 0.22*np.clip((z + 14)/22, 0, 1))[..., None]
    img[..., :3] = np.array(base, np.float32)*k; img[..., 3] = 215
    if sc:
        st, ed = np.array(sc['stripe'], np.float32), np.array(sc['edge'], np.float32)
        for y0, y1, c in [(-1.4, 6.6, st), (-3.6, -2.4, ed)]:
            msk = used & (top | front | rear) & (y > y0) & (y < y1)
            img[msk, :3] = c*k[msk]
        img[used & side & (z > -9.6) & (z < -7.2), :3] = st
        img[used & side & (z > -10.6) & (z < -10.0), :3] = ed
    img[sill, :3] = (24, 24, 26); img[sill, 3] = 240
    # windows: dark tint, strongly reflective
    img[glass, :3] = np.array((22, 26, 34), np.float32)*np.clip(1.0 + 0.05*(z[glass] - 6), 0.8, 1.3)[..., None]
    img[glass, 3] = 165
    # sticker headlights, grille, tail lights
    img[headl, :3] = (238, 238, 222); img[headl, 3] = 255
    ring = used & front & ~glass & ((((np.abs(y) - hl[1])/3.4)**2 + ((z - hl[2])/1.8)**2) > 0.72) & headl
    img[ring, :3] = (60, 60, 64)
    img[grille, :3] = (16, 16, 18); img[grille, 3] = 245
    mesh = grille & ((np.floor(y*1.4) + np.floor(z*1.8)) % 2 == 0)
    img[mesh, :3] = (46, 46, 52)
    # tail light cluster: black housing, red lamps, white reverse lamp on the inside
    img[tail, :3] = (18, 18, 20); img[tail, 3] = 255
    lamp = tail & (np.abs(z - bl[2]) < 1.05) & (np.abs(np.abs(y) - (bl[1] + rl[1])/2 - 0.6) < 3.1)
    img[lamp, :3] = (215, 20, 16)
    img[lamp & ((np.abs(y)*1.2) % 1.0 < 0.12), :3] = (150, 10, 8)          # lens ribs
    img[reverse & (np.abs(z - bl[2]) < 1.05), :3] = (238, 238, 232)
    if sc:
        num = number_img(sc)
        L, Rr = used & side & (y > 0) & ~glass, used & side & (y < 0) & ~glass
        decal(img, L, 0.5, 2.2, 15.0, num, -x, -z)                    # door number, upright from outside
        decal(img, Rr, -0.5, 2.2, 15.0, num, x, -z)
        roofm = used & top & ~glass & (np.abs(x + 4) < 9)
        decal(img, roofm, 0.0, 4.0, 16.0, num, -y, -x)               # roof: readable from the chase cam (x centre -4)
        tc = tuple(int(c) for c in (sc['stripe'] if sum(sc['body']) < 400 else sc['edge']))
        q3 = text_img('Q3RALLY', tc)
        decal(img, L, 38.0, -1.0, 12.0, q3, -x, -z)
        decal(img, Rr, -38.0, -1.0, 12.0, q3, x, -z)
    img[~used] = (img[used].reshape(-1, 4).mean(0) if used.any() else 0)
    out = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8), 'RGBA').resize((R//2, R//2), Image.LANCZOS)
    out.save(f'{OUT}/{name}.tga')


# ---------------- NASCAR style liveries for the rally skins ------------------------------------
# Design patterns of current Cup cars (two-tone splits, huge sponsor wordmarks on the rear
# quarters and the hood, outlined slab numbers, windscreen banner, contingency decal block,
# window net); all sponsors are made up.
FONT_SLAB = '/usr/share/fonts/opentype/inter/InterDisplay-BlackItalic.otf'
FONT_WORD = '/usr/share/fonts/opentype/inter/Inter-Black.otf'
FONT_WORDI = '/usr/share/fonts/opentype/inter/Inter-BlackItalic.otf'
FONT_SMALL = '/usr/share/fonts/opentype/inter/Inter-ExtraBoldItalic.otf'

def txt(t, col, h=160, font=FONT_WORD, stroke=0, scol=(0, 0, 0), sub=None, subcol=None, track=0):
    f = ImageFont.truetype(font, int(h*0.82))
    w = int(f.getlength(t)) + 2*stroke + 40 + track*len(t)
    hh = h + (int(h*0.42) if sub else 0) + 2*stroke + 10
    im = Image.new('RGBA', (w, hh), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    d.text((w/2, h/2 + stroke + 4), t, font=f, fill=col + (255,), anchor='mm', stroke_width=stroke, stroke_fill=scol + (255,))
    if sub:
        fs = ImageFont.truetype(FONT_SMALL, int(h*0.28))
        d.text((w/2, h + stroke + int(h*0.18)), sub, font=fs, fill=(subcol or col) + (255,), anchor='mm')
    return np.asarray(im)

def slab_number(numtxt, fill, outline, outline2, h=300):
    f = ImageFont.truetype(FONT_SLAB, int(h*0.9))
    w = int(f.getlength(numtxt)) + 120
    im = Image.new('RGBA', (w, h + 60), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    c = (w/2, (h + 60)/2)
    d.text((c[0] + 10, c[1] + 10), numtxt, font=f, fill=(0, 0, 0, 120), anchor='mm', stroke_width=26, stroke_fill=(0, 0, 0, 120))   # drop shadow
    d.text(c, numtxt, font=f, fill=outline2 + (255,), anchor='mm', stroke_width=26, stroke_fill=outline2 + (255,))
    d.text(c, numtxt, font=f, fill=outline + (255,), anchor='mm', stroke_width=14, stroke_fill=outline + (255,))
    d.text(c, numtxt, font=f, fill=fill + (255,), anchor='mm')
    return np.asarray(im)

def oval_logo(t, fill, ring, ink, w=560, h=300, sub=None):
    im = Image.new('RGBA', (w, h), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    d.ellipse([4, 4, w-4, h-4], fill=ring + (255,)); d.ellipse([22, 22, w-22, h-22], fill=fill + (255,))
    f = ImageFont.truetype(FONT_WORD, int(h*0.26)); d.text((w/2, h*0.46), t, font=f, fill=ink + (255,), anchor='mm')
    if sub:
        fs = ImageFont.truetype(FONT_SMALL, int(h*0.11)); d.text((w/2, h*0.72), sub, font=fs, fill=ink + (255,), anchor='mm')
    return np.asarray(im)

CONTINGENCY = [('Q3R', (200, 20, 20), (255, 255, 255)), ('HASTE', (250, 200, 0), (20, 20, 20)), ('BFG', (20, 20, 20), (120, 230, 40)),
               ('REGEN', (30, 90, 200), (255, 255, 255)), ('QUAD', (60, 40, 160), (255, 255, 255)), ('BATTLESUIT', (240, 240, 240), (200, 120, 0)),
               ('ARMOR', (230, 120, 20), (20, 20, 20)), ('FLIGHT', (240, 240, 240), (30, 60, 160))]
def contingency_img():
    cw, ch, gap = 150, 62, 10
    im = Image.new('RGBA', (2*cw + gap, 4*(ch + gap)), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    for i, (t, bg, fg) in enumerate(CONTINGENCY):
        x0 = (i % 2)*(cw + gap); y0 = (i//2)*(ch + gap)
        d.rounded_rectangle([x0, y0, x0 + cw, y0 + ch], radius=8, fill=bg + (255,), outline=(255, 255, 255, 255), width=3)
        f = ImageFont.truetype(FONT_SMALL, 30 if len(t) < 7 else 21)
        d.text((x0 + cw/2, y0 + ch/2), t, font=f, fill=fg + (255,), anchor='mm')
    return np.asarray(im)
def series_badge():
    im = Image.new('RGBA', (300, 170), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    d.rounded_rectangle([2, 2, 298, 168], radius=14, fill=(250, 250, 250, 255), outline=(20, 20, 20, 255), width=6)
    d.rectangle([14, 14, 286, 70], fill=(200, 20, 20, 255)); d.rectangle([14, 70, 286, 110], fill=(30, 60, 160, 255))
    d.text((150, 44), 'Q3RALLY', font=ImageFont.truetype(FONT_WORDI, 44), fill=(255, 255, 255, 255), anchor='mm')
    d.text((150, 92), 'CUP SERIES', font=ImageFont.truetype(FONT_SMALL, 30), fill=(255, 255, 255, 255), anchor='mm')
    d.text((150, 138), 'OFFICIAL ENTRY', font=ImageFont.truetype(FONT_SMALL, 24), fill=(20, 20, 20, 255), anchor='mm')
    return np.asarray(im)
CONT, BADGE = contingency_img(), series_badge()

def jagged(xv, base, amp, period):
    """saw-tooth mountain line"""
    t = (xv/period) % 1.0
    return base + amp*(1 - 2*np.abs(t - 0.5)*2).clip(-1, 1)

LIVERIES = {
    # rally01: black / white diagonal split, gold number
    'rally01': dict(base=(26, 26, 30), second=(238, 238, 240), split='diag', accent=(222, 176, 70),
                    num=((238, 238, 240), (26, 26, 30), (222, 176, 70)), word=('RAILGUN', 'Oil & Lubricants'),
                    wordcol=(222, 176, 70), hoodcol=(238, 238, 240), banner=((26, 26, 30), (222, 176, 70), 'SARGE')),
    # rally07: navy with light blue lower band, gold outlined number
    'rally07': dict(base=(26, 58, 140), second=(120, 190, 235), split='lower', accent=(255, 255, 255),
                    num=((150, 210, 245), (255, 255, 255), (200, 160, 70)), word=('QUAD DAMAGE', 'Energy Drink & Power Snacks'),
                    wordcol=(255, 255, 255), hoodcol=(255, 255, 255), banner=((255, 255, 255), (26, 58, 140), 'XAERO')),
    # rally13: white with an orange/blue mountain line along the lower body
    'rally13': dict(base=(236, 238, 242), second=(236, 112, 30), split='jag', accent=(30, 60, 150),
                    num=((30, 60, 150), (255, 255, 255), (30, 60, 150)), word=('MEGA HEALTH', None),
                    wordcol=(30, 60, 150), hoodcol=(30, 60, 150), banner=((30, 60, 150), (255, 255, 255), 'KLESK')),
    # rally27: red with a yellow nose and an oval logo on the quarters
    'rally27': dict(base=(204, 36, 30), second=(248, 200, 30), split='nose', accent=(20, 20, 24),
                    num=((248, 200, 30), (20, 20, 24), (248, 248, 248)), word=('ROCKET FUEL', None), oval=True,
                    wordcol=(248, 200, 30), hoodcol=(204, 36, 30), banner=((248, 200, 30), (204, 36, 30), 'HUNTER')),
    # rally44: green with yellow speed slashes
    'rally44': dict(base=(40, 150, 70), second=(240, 220, 40), split='slash', accent=(20, 40, 24),
                    num=((255, 255, 255), (20, 40, 24), (240, 220, 40)), word=('FRAG FORCE', 'Tools for the Arena'),
                    wordcol=(255, 255, 255), hoodcol=(240, 220, 40), banner=((20, 40, 24), (240, 220, 40), 'MAJOR')),
    # rally88: purple fading into pink towards the rear
    'rally88': dict(base=(96, 50, 176), second=(236, 90, 170), split='fade', accent=(255, 255, 255),
                    num=((255, 255, 255), (60, 20, 110), (236, 90, 170)), word=('PLASMA COLA', 'Charged with Energy'),
                    wordcol=(255, 255, 255), hoodcol=(255, 255, 255), banner=((236, 90, 170), (255, 255, 255), 'ORBB')),
}

def nascar(name, L):
    img = np.zeros((R, R, 4), np.float32)
    k = (0.82 + 0.20*np.clip((z + 14)/22, 0, 1))[..., None]
    base, sec = np.array(L['base'], np.float32), np.array(L['second'], np.float32)
    col = np.broadcast_to(base, (R, R, 3)).copy()
    split = L['split']
    if split == 'diag':          # front (nose to door) in the second colour, rear in the base
        m2 = x > 4.0 + 1.1*z
    elif split == 'lower':
        m2 = (side & (z < -6.5)) | ((front | rear) & ~top & (z < -6.5))
    elif split == 'jag':
        m2 = (~top) & (z < jagged(x, -8.0, 3.2, 11.0))
        acc = (~top) & (np.abs(z - jagged(x, -8.0, 3.2, 11.0)) < 0.55)
    elif split == 'nose':
        m2 = (x > 30 - 0.5*z) | ((~top) & (z < -10.5))
    elif split == 'slash':
        t = (x - 0.9*z)
        m2 = (~top) & (x < 10) & (x > -54) & (((t + 60) % 13) < 3.2)
    elif split == 'fade':
        w = np.clip((-x + 20)/70, 0, 1)[..., None]
        col = base*(1 - w) + sec*w; m2 = np.zeros((R, R), bool)
    col[m2] = sec
    if split == 'jag': col[acc] = np.array(L['accent'], np.float32)
    img[..., :3] = col*k; img[..., 3] = 212
    img[sill, :3] = (24, 24, 26); img[sill, 3] = 240
    img[glass, :3] = np.array((22, 26, 34), np.float32)*np.clip(1.0 + 0.05*(z[glass] - 6), 0.8, 1.3)[..., None]
    img[glass, 3] = 165
    # window net on the driver's side (left)
    net = glass & side & (y > 0) & (x > -2) & (x < 15) & ((np.abs(((x + z)*1.5) % 1.0 - 0.5) < 0.1) | (np.abs(((x - z)*1.5) % 1.0 - 0.5) < 0.1))
    img[net, :3] = (14, 14, 16); img[net, 3] = 235
    # windscreen banner with the driver's name
    bcol, btxt, bname = L['banner']
    ban = glass & front & (z > 9.6)
    img[ban, :3] = bcol; img[ban, 3] = 245
    decal(img, ban, 0.0, -10.3, 13.0, txt(bname, btxt, 120, FONT_WORDI), y, -z)
    # sticker headlights, grille, tail lights (as on the plain skins)
    img[headl, :3] = (238, 238, 222); img[headl, 3] = 255
    ring = used & front & ~glass & ((((np.abs(y) - hl[1])/3.4)**2 + ((z - hl[2])/1.8)**2) > 0.72) & headl
    img[ring, :3] = (60, 60, 64)
    img[grille, :3] = (16, 16, 18); img[grille, 3] = 245
    mesh = grille & ((np.floor(y*1.4) + np.floor(z*1.8)) % 2 == 0)
    img[mesh, :3] = (46, 46, 52)
    img[tail, :3] = (18, 18, 20); img[tail, 3] = 255
    lamp = tail & (np.abs(z - bl[2]) < 1.05) & (np.abs(np.abs(y) - (bl[1] + rl[1])/2 - 0.6) < 3.1)
    img[lamp, :3] = (215, 20, 16)
    img[reverse & (np.abs(z - bl[2]) < 1.05), :3] = (238, 238, 232)

    Lf, Rf = used & (ny > 0.3) & ~glass, used & (ny < -0.3) & ~glass      # a bit wider than 'side': creases
    topm = used & top & ~glass
    num = slab_number(sc_num[name], *L['num'])
    def by_height(msk, u0, zc, hgt, rgba, U, maxw=None):
        ys_, xs_ = np.nonzero(rgba[..., 3] > 8)                      # crop to the visible part
        rgba = rgba[ys_.min():ys_.max()+1, xs_.min():xs_.max()+1]
        h, w = rgba.shape[:2]; width = hgt*w/h
        if maxw and width > maxw: width = maxw
        decal(img, msk, u0, -zc, width, rgba, U, -z)
    # the flank above the rear arch is only a thin strip (z 0.6 .. ~2, then the tumblehome
    # crease) where any lettering gets cut: number on the door, sponsor along the rocker
    DX = 1.0
    by_height(Lf, -DX, -2.0, 10.0, num, -x, 18); by_height(Rf, DX, -2.0, 10.0, num, x, 18)
    if L.get('oval'):
        logo = oval_logo(L['word'][0], (248, 200, 30), (20, 20, 24), (204, 36, 30), sub='RACING FUEL')
        by_height(Lf, -DX, -10.1, 4.6, logo, -x); by_height(Rf, DX, -10.1, 4.6, logo, x)
    else:
        # wide band along the rocker between the wheel arches
        word = txt(L['word'][0], L['wordcol'], 170, FONT_WORDI, 6, tuple(int(c) for c in (base*0.5)))
        by_height(Lf, -DX, -10.4, 3.2, word, -x, 34); by_height(Rf, DX, -10.4, 3.2, word, x, 34)
    # contingency block behind the front wheels, series badge at the A pillar
    decal(img, Lf, -22.5, 3.5, 7.0, CONT, -x, -z); decal(img, Rf, 22.5, 3.5, 7.0, CONT, x, -z)
    decal(img, Lf, -15.0, -3.6, 4.6, BADGE, -x, -z); decal(img, Rf, 15.0, -3.6, 4.6, BADGE, x, -z)
    # hood sponsor, readable from the front; roof number readable from the chase cam
    hood = topm & (x > 18)
    hw = txt(L['word'][0], L['hoodcol'], 170, FONT_WORDI, 6, tuple(int(c) for c in (base*0.5)))
    decal(img, hood, 0.0, 34.0, 30.0, hw, y, x)
    decal(img, topm & (np.abs(x + 4) < 9), 0.0, 4.0, 15.0, num, -y, -x)
    # rear: sponsor on the tail panel and the deck lid, small number by the lights
    tailp = used & (nx < -0.5) & ~glass & ~tail
    # tail: between the lights, above the game's licence plate (z -6)
    tw = txt(L['word'][0], L['wordcol'] if not L.get('oval') else (248, 200, 30), 140, FONT_WORDI, 5, tuple(int(c) for c in (base*0.5)))
    th, twd = tw.shape[:2]
    decal(img, tailp, 0.0, 1.0, min(15.0, 2.4*twd/th), tw, -y, -z)
    decal(img, topm & (x < -40) & (np.abs(y) < 9), 0.0, 47.0, 16.0, txt('Q3RALLY CUP', tuple(int(c) for c in L['accent']), 120, FONT_WORDI), -y, -x)
    img[~used] = img[used].reshape(-1, 4).mean(0)
    Image.fromarray(np.clip(img, 0, 255).astype(np.uint8), 'RGBA').resize((R//2, R//2), Image.LANCZOS).save(f'{OUT}/{name}.tga')

sc_num = {n: sc['num'] for n, sc in RALLY.items()}

for n, c in PLAIN.items(): paint(n, c)
paint('white', (232, 232, 236))
for n in RALLY: nascar(n, LIVERIES[n])
u = (np.random.RandomState(zlib.crc32(b'under') % 1000).rand(32, 32)*10 + 20).astype(np.uint8)
Image.fromarray(np.dstack([u, u, u])).save(f'{OUT}/under.tga')
print('ok')
