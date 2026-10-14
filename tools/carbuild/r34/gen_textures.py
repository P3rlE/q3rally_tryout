"""Paint the Skyline textures and convert the lamp/grill textures of the source.

usage: gen_textures.py <source textures dir> <outdir>

The paint atlases follow layout.py. Panel gaps, vents and lamps are modelled,
so the paint only carries colour, shading and the rally liveries. TGAs are
written run-length encoded.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from layout import *

SRC, OUT = sys.argv[1], sys.argv[2]
K = 2                                   # supersampling
N = int(A) * K
FONT_NUM = '/usr/share/fonts/opentype/inter/Inter-BlackItalic.otf'
FONT_TXT = '/usr/share/fonts/opentype/inter/Inter-ExtraBoldItalic.otf'

PLAIN = {'red': (196, 8, 8), 'blue': (10, 36, 196), 'green': (24, 176, 16), 'yellow': (250, 186, 4)}
# same house rally colours as the Mustang
RALLY = {
    'rally01': dict(num='1',  body=(34, 35, 42),    stripe=(228, 178, 88),  edge=(150, 152, 160), ring=(228, 178, 88),  ink=(25, 25, 30)),
    'rally07': dict(num='7',  body=(28, 50, 84),    stripe=(238, 240, 242), edge=(105, 160, 225), ring=(238, 240, 242), ink=(28, 50, 84)),
    'rally13': dict(num='13', body=(212, 214, 222), stripe=(236, 112, 30),  edge=(48, 104, 196),  ring=(226, 80, 20),   ink=(25, 25, 30)),
    'rally27': dict(num='27', body=(204, 48, 36),   stripe=(240, 240, 240), edge=(20, 20, 24),    ring=(20, 20, 24),    ink=(20, 20, 24)),
    'rally44': dict(num='44', body=(56, 178, 80),   stripe=(244, 226, 40),  edge=(18, 40, 24),    ring=(230, 220, 30),  ink=(20, 30, 22)),
    'rally88': dict(num='88', body=(100, 56, 180),  stripe=(240, 118, 196), edge=(245, 245, 250), ring=(232, 80, 170),  ink=(36, 16, 70)),
}
ALPHA = 238                             # paint over the reflection; satin, less glossy than the other cars


def P(region, x=0, y=0, z=0):
    """world point -> supersampled atlas pixel"""
    if region in ('L', 'R'):
        uv = atlas(region, (x, 0, z))
    elif region == 'T':
        uv = atlas('T', (x, y, 0))
    else:
        uv = atlas(region, (0, y, z))
    return (uv[0] * N, uv[1] * N)


def rect(dr, region, a, b, fill):
    if region in ('L', 'R'):
        pts = [P(region, a[0], z=a[1]), P(region, b[0], z=b[1])]
    elif region == 'T':
        pts = [P('T', a[0], a[1]), P('T', b[0], b[1])]
    else:
        pts = [P(region, y=a[0], z=a[1]), P(region, y=b[0], z=b[1])]
    x0, x1 = sorted([pts[0][0], pts[1][0]])
    y0, y1 = sorted([pts[0][1], pts[1][1]])
    dr.rectangle([x0, y0, x1, y1], fill=fill)


def poly(dr, region, pts, fill):
    if region in ('L', 'R'):
        q = [P(region, x, z=z) for x, z in pts]
    elif region == 'T':
        q = [P('T', x, y) for x, y in pts]
    else:
        q = [P(region, y=y, z=z) for y, z in pts]
    dr.polygon(q, fill=fill)


def roundel(r, sc, rot=0):
    img = Image.new('RGBA', (int(r * 2 + 8), int(r * 2 + 8)), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    c = r + 4
    d.ellipse([c - r, c - r * 0.86, c + r, c + r * 0.86], fill=sc['ring'] + (255,))
    d.ellipse([c - r * 0.82, c - r * 0.70, c + r * 0.82, c + r * 0.70], fill=(245, 245, 245, 255))
    f = ImageFont.truetype(FONT_NUM, int(r * (1.05 if len(sc['num']) == 1 else 0.92)))
    d.text((c, c + r * 0.02), sc['num'], font=f, fill=sc['ink'] + (255,), anchor='mm')
    if rot:
        img = img.rotate(rot, expand=True, resample=Image.BICUBIC)
    return img


def paste(canvas, img, cx, cy):
    canvas.alpha_composite(img, (int(cx - img.width / 2), int(cy - img.height / 2)))


def text(s, size, col, rot=0):
    f = ImageFont.truetype(FONT_TXT, int(size))
    w, h = int(f.getlength(s)) + 20, int(size * 1.6)
    img = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    ImageDraw.Draw(img).text((w / 2, h / 2), s, font=f, fill=col + (255,), anchor='mm')
    return img.rotate(rot, expand=True, resample=Image.BICUBIC) if rot else img


def shade(rgb, k):
    return tuple(max(0, min(255, int(c * k))) for c in rgb)


def save(img, name):
    img.save(os.path.join(OUT, name + '.tga'), compression='tga_rle')


def paint(name, base, sc=None):
    img = Image.new('RGBA', (N, N), base + (ALPHA,))
    dr = ImageDraw.Draw(img)
    # flanks a little darker towards the sills, a lighter band on the shoulder line
    for side in 'LR':
        steps = 48
        for i in range(steps):
            z0 = Z0 + (Z1 - Z0) * i / steps
            z1 = Z0 + (Z1 - Z0) * (i + 1) / steps
            k = 0.78 + 0.24 * min(1, max(0, (z0 + 13) / 16)) + (0.05 if 2.5 < z0 < 6 else 0)
            rect(dr, side, (X0, z0), (X1, z1), shade(base, k) + (ALPHA,))
    for reg in 'FB':
        for i in range(48):
            z0 = Z0 + (Z1 - Z0) * i / 48
            z1 = Z0 + (Z1 - Z0) * (i + 1) / 48
            rect(dr, reg, (Y0, z0), (Y1, z1), shade(base, 0.84 + 0.16 * min(1, max(0, (z0 + 10) / 14))) + (ALPHA,))

    if sc:
        st, ed = sc['stripe'], sc['edge']
        # twin stripes over nose, bonnet, roof, boot and tail, offset like the Mustang's
        for y0, y1, c in ((-1.4, 7.2, st), (-3.8, -2.4, ed)):
            rect(dr, 'T', (X0, y0), (X1, y1), c + (ALPHA,))
            rect(dr, 'F', (y0, -8.0), (y1, Z1), c + (ALPHA,))
            rect(dr, 'B', (y0, -6.0), (y1, Z1), c + (ALPHA,))    # same world y as on the boot lid
        # side stripe from the front wheel arch to the rear wheel arch, rising to the back
        for side in 'LR':
            poly(dr, side, [(25.0, -6.2), (-24.0, -5.2), (-24.0, -7.6), (25.0, -8.6)], st + (ALPHA,))
            poly(dr, side, [(25.0, -9.2), (-24.0, -8.2), (-24.0, -8.9), (25.0, -9.9)], ed + (ALPHA,))
    # underside patch
    dr.rectangle([R_UNDER[0] * K, R_UNDER[1] * K, N, N], fill=(20, 20, 22, 250))

    if sc:
        for side in 'LR':
            x, y = P(side, -31.0, z=1.2)         # rear wing, above the wheel arch
            paste(img, text('Q3RALLY', 2.0 * SC * K, sc['stripe'] if sum(base) < 400 else sc['edge']), x, y)
            x, y = P(side, 2.0, z=-0.5)
            paste(img, roundel(5.6 * SC * K, sc), x, y)
        x, y = P('T', -7.0, 0.0)
        paste(img, roundel(7.0 * SC * K, sc, rot=-90), x, y)   # roof, readable from the chase cam
        x, y = P('T', 36.0, 0.0)
        paste(img, text('Q3RALLY', 2.6 * SC * K, sc['ink'] if sum(st) > 400 else (240, 240, 240), rot=-90), x, y)
    img = img.resize((int(A), int(A)), Image.LANCZOS)
    save(img, name)
    return img


os.makedirs(OUT, exist_ok=True)
for n, c in PLAIN.items():
    paint(n + '_main', c)
for n, sc in RALLY.items():
    paint(n + '_main', sc['body'], sc)

# ---------------- shared parts -----------------------------------------------------
yy = np.linspace(0, 1, 128)[:, None] * np.ones((1, 128))
# glass: dark tint, nearly opaque (there is no interior behind it)
g = np.zeros((128, 128, 4), np.uint8)
g[..., 0] = 14 + 26 * (1 - yy)
g[..., 1] = 18 + 30 * (1 - yy)
g[..., 2] = 26 + 36 * (1 - yy)
g[..., 3] = 215
save(Image.fromarray(g), 'glass')
# chrome: sky / horizon / ground gradient
c = np.zeros((128, 128, 4), np.uint8)
prof = np.interp(yy, [0, 0.45, 0.5, 0.55, 1], [235, 170, 70, 140, 200])
c[..., 0] = prof
c[..., 1] = prof
c[..., 2] = np.minimum(255, prof * 1.05)
c[..., 3] = 120
save(Image.fromarray(c), 'chrome')
# trim: satin black plastic
t = np.zeros((64, 64, 3), np.uint8)
t[..., :] = (np.random.RandomState(3).rand(64, 64, 1) * 6 + 20).astype(np.uint8)
save(Image.fromarray(t), 'trim')
# underside
u = (np.random.RandomState(7).rand(64, 64) * 10 + 22).astype(np.uint8)
save(Image.fromarray(np.dstack([u, u, u + 2])), 'under')
# lamps and grille from the source model
for src, name, size in (('lights_texture.png', 'lights', 512), ('backlights.png', 'tlite', 512),
                        ('grill-texture-png-11.png', 'grill', 256)):
    im = Image.open(os.path.join(SRC, src)).convert('RGBA').resize((size, size), Image.LANCZOS)
    save(im, name)
# ---------------- skins ------------------------------------------------------------------
P_DIR = 'models/players/skyline/'
FIXED = [('bottom', 'under'), ('glass', 'glass'), ('trim', 'trim'), ('chrome', 'chrome'), ('grill', 'grill'),
         ('hlights', 'lights'), ('ilights', 'lights'), ('rlights', 'tlite')]
TAGS = ['tag_weapon', 'tag_wheelfr', 'tag_wheelfl', 'tag_wheelrr', 'tag_wheelrl', 'tag_turbo', 'tag_smoke',
        'tag_plate', 'tag_hlite1', 'tag_hlite2', 'tag_blite1', 'tag_blite2', 'tag_rlite1', 'tag_rlite2']
for skin in list(PLAIN) + list(RALLY):
    lines = ['%s,%s%s.tga' % (s, P_DIR, t) for s, t in FIXED[:5]]
    lines += ['%s,%s%s_main.tga' % (s, P_DIR, skin) for s in ('body_fl', 'body_fr', 'body_rl', 'body_rr')]
    lines += ['%s,%s%s.tga' % (s, P_DIR, t) for s, t in FIXED[5:]]
    lines += [t + ',' for t in TAGS]
    open(os.path.join(OUT, skin + '.skin'), 'w').write('\n'.join(lines) + '\n')
print('ok')
