"""Sky box, reflection maps, liquids, teleporters, CTF flags, map models."""
import os
from common import *
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))


# ---------------------------------------------------------------- night sky box
@recipe
def xnight2():
    """Stars and faint haze. Haze fades to one level at the face borders, so
    the faces meet without seams whatever the face orientation is."""
    out = {}
    w = h = 256
    x, y = grid(w, h)
    edge = np.minimum(np.minimum(x, w - x), np.minimum(y, h - y)) / (w / 2)
    for i, face in enumerate(('bk', 'dn', 'ft', 'lf', 'rt', 'up')):
        rng = np.random.default_rng(900 + i)
        img = np.zeros((h, w, 3), F)
        haze = noise(w, h, 910 + i, 1, 10, 2.4)
        hz = smooth(0.0, 0.6, edge) * smooth(0.45, 0.9, haze) * (0.0 if face in ('dn',) else 1.0)
        img += light(fill(w, h, '#0a1224'), hz * 0.8)
        n = 520
        sx, sy = rng.random(n) * w, rng.random(n) * h
        mag = rng.random(n) ** 3
        for px, py, m in zip(sx, sy, mag):
            if face == 'dn' and m < 0.5:
                continue
            r = 0.45 + m * 0.6
            g = np.exp(-((x - px) ** 2 + (y - py) ** 2) / (2 * r * r)) * (0.12 + 0.6 * m)
            tint = rgb(rng.choice(['#ffffff', '#d8e4ff', '#fff2d8']))
            img += g[..., None] * tint
        if face == 'lf':                                   # the moon, lit from the right
            dx, dy = x - 82, y - 82
            r = np.sqrt(dx * dx + dy * dy)
            disc = mask(r - 17)
            nz = np.sqrt(clamp(1 - (r / 17) ** 2))
            lit = clamp((dx / 17) * 0.8 + nz * 0.6)
            mare = 1 - 0.25 * smooth(0.5, 0.8, noise(w, h, 920, 6, 40))
            moon = light(fill(w, h, '#dde4f0'), lit * mare)
            img = lerp(img, moon, disc)
            img += light(fill(w, h, '#304060'), glow_blob(x, y, 82, 82, 46, 2.0) * 0.5)
        out[f'env/xnight2_{face}.jpg'] = (clamp(img), None)
    return out


# ---------------------------------------------------------------- reflection maps
@recipe
def env_maps():
    out = {}
    w = h = 128
    x, y = grid(w, h)
    f1, f2, cid = worley(w, h, 40, 930)
    v = cell_values(cid, 40, 931)
    blot = blur(v, 2.5)
    out['textures/base_wall/chrome_env.jpg'] = (np.stack([norm01(blot + noise(w, h, 932, 4, 30) * 0.3)] * 3, -1), None)
    c = noise(w, h, 933, 1, 8, 2.6)
    cc = gradient_map(c, [(0, '#102060'), (0.4, '#2050a0'), (0.6, '#406a30'), (0.8, '#c08030'), (1, '#f0d0a0')])
    out['textures/base_wall/comp3env.jpg'] = (blur(cc, 2), None)

    def room(seed, lamps):
        # an abstract hall: dark floor and ceiling, light bands, a few lamps
        t = gradient_map(clamp(1 - np.abs(y - 64) / 64), [(0, '#202020'), (0.6, '#585858'), (1, '#9a9a9a')])
        bars = (np.cos((x - 64) / 64 * np.pi * 3) > 0.85) * smooth(30, 10, np.abs(y - 64))
        t = t + bars[..., None] * 0.35
        for lx, ly, r in lamps:
            t = t + glow_blob(x, y, lx, ly, r, 1.6)[..., None]
        t = t * (0.8 + 0.3 * noise(w, h, seed, 4, 40))[..., None]
        return blur(clamp(t), 1.0)
    out['textures/base_trim/tinfx.jpg'] = (room(934, [(64, 20, 18)]), None)
    out['textures/effects/tinfx2c.jpg'] = (room(935, [(40, 40, 22), (90, 92, 22)]), None)
    out['textures/effects/tinfx2d.jpg'] = (room(936, [(36, 36, 20), (94, 96, 24)]), None)
    return out


# ---------------------------------------------------------------- liquids
def caustics(w, h, seed, n, sharp):
    f1, f2, _ = worley(w, h, n, seed)
    c = clamp(1 - (f2 - f1) / sharp) ** 2
    dx = (noise(w, h, seed + 1, 2, 12) - 0.5) * w * 0.12
    dy = (noise(w, h, seed + 2, 2, 12) - 0.5) * h * 0.12
    return warp(c, dx, dy)


@recipe
def liquids():
    out = {}
    w = h = 512
    c = blur(caustics(w, h, 940, 140, 7) * 0.7 + caustics(w, h, 941, 300, 4) * 0.3, 0.8)
    out['textures/liquids/pool3d_3c2.jpg'] = (gradient_map(c, [(0, '#404048'), (0.4, '#7a78a0'), (0.8, '#d8d0ff'), (1, '#ffffff')]), None)
    c = blur(caustics(w, h, 942, 200, 6) * 0.7 + caustics(w, h, 943, 420, 3.5) * 0.3, 0.8)
    out['textures/liquids/pool3d_5c2.jpg'] = (gradient_map(c, [(0, '#585c62'), (0.5, '#8c9098'), (1, '#f0f4ff')]), None)
    hm = noise(w, h, 944, 3, 40, 2.2)
    met = gradient_map(hm, [(0, '#2a3638'), (1, '#7a8a8c')])
    out['textures/liquids/pool3d_6c2.jpg'] = (finish(met, hm, 30, 0.2, 2, 944, gloss=0.9, power=40), None)
    w = h = 256
    out['textures/liquids/pool3d_4b2.jpg'] = (gradient_map(noise(w, h, 945, 2, 30, 2.0), [(0, '#000404'), (1, '#0c1c18')]), None)
    return out


# ---------------------------------------------------------------- teleporters
def swirl(w, h, seed, inner, outer, arms, tightness, colour, broken=0.0):
    x, y = grid(w, h)
    dx, dy = x - w / 2, y - h / 2
    r = np.sqrt(dx * dx + dy * dy) / (w / 2)
    a = np.arctan2(dy, dx)
    s = np.sin(a * arms + np.log(r + 1e-3) * tightness)
    band = clamp((s + 0.2) * 1.4) * smooth(inner - 0.1, inner + 0.1, r) * smooth(outer + 0.1, outer - 0.15, r)
    if broken:
        band *= smooth(0.3, 0.6, noise(w, h, seed, 6, 60) * (1 - broken) + broken * 0.5 + (s * 0.2))
    band = blur(band, w / 256)
    return light(fill(w, h, colour), band)


@recipe
def teleporters():
    out = {}
    out['textures/ctf/blue_telep.jpg'] = (swirl(256, 256, 950, 0.25, 0.8, 3, 4, '#c8c8c8'), None)
    out['textures/ctf/red_telep.jpg'] = (swirl(256, 256, 951, 0.25, 0.8, 3, 4, '#c8c8c8'), None)
    out['textures/ctf/blue_telep2.jpg'] = (swirl(1024, 1024, 952, 0.45, 0.75, 5, 3, '#7080ff', 0.3), None)
    out['textures/ctf/red_telep2.jpg'] = (swirl(1024, 1024, 953, 0.45, 0.75, 5, 3, '#ff7060', 0.3), None)
    # two clamp arcs, rotated by the shader
    w = h = 256
    x, y = grid(w, h)
    r = np.sqrt((x - 128) ** 2 + (y - 128) ** 2)
    arc = np.abs(r - 100) - 7
    arc = np.maximum(arc, 40 - np.abs(x - 128))
    blocks = np.minimum(sd_box(x, y, 26, 128, 9, 24, 3), sd_box(x, y, 230, 128, 9, 24, 3))
    s = np.minimum(arc, blocks)
    hm = bevel(s, 4)
    col = finish(worn_plate(w, h, 954, '#3a3a3e', dirt=0.3), hm, 5, 0.4, 2, 954, gloss=0.4)
    out['textures/ctf/telep.tga'] = (col, (s < 0).astype(F))
    return out


# ---------------------------------------------------------------- flags and banners
def fabric(w, h, seed, base):
    x, y = grid(w, h)
    weave = 0.85 + 0.15 * (np.cos(x * np.pi / 2) * np.cos(y * np.pi / 2))
    col = light(fill(w, h, base), weave * (0.75 + 0.5 * noise(w, h, seed, 2, 30)))
    col = lerp(col, clamp(col * 1.6), smooth(0.7, 0.9, noise(w, h, seed + 1, 30, None, 1.0)) * 0.4)
    return col


def flag(seed, base, emblem):
    w, h = 1024, 2048
    x, y = grid(w, h)
    col = fabric(w, h, seed, base)
    frame = np.maximum(np.abs(x - 512) - 496, np.abs(y - 1024) - 1008)
    col = lerp(col, worn_plate(w, h, seed + 2, '#8a8a8a', dirt=0.2), (frame > 0).astype(F))
    cx, cy = 512, 1024
    if emblem == 'wheel':
        r = np.sqrt((x - cx) ** 2 + (y - cy) ** 2)
        d = np.abs(r - 300) - 50
        for k in range(3):
            a = np.pi / 2 + k * 2 * np.pi / 3
            d = np.minimum(d, sd_segment(x, y, cx, cy, cx + np.cos(a) * 290, cy + np.sin(a) * 290, 40))
        d = np.minimum(d, r - 90)
    else:
        lx, ly = (x - cx), (y - cy)
        u_, v_ = (lx + ly) / 1.414, (lx - ly) / 1.414
        diamond = np.maximum(np.abs(u_), np.abs(v_)) - 300
        checker = ((np.floor(u_ / 100) + np.floor(v_ / 100)) % 2 == 0)
        d = np.where(checker, diamond, np.maximum(diamond, 1.0))
        d = np.minimum(d, np.abs(diamond) - 18)
    m = mask(d, 1.5)
    col = lerp(col, fill(w, h, '#f4f4f4'), m)
    return col, 1 - m


@recipe
def ctf_flags():
    return {'textures/ctf/ctf_blueflag.tga': flag(960, '#1a2a9a', 'wheel'),
            'textures/ctf/ctf_redflag.tga': flag(961, '#a01a14', 'checker')}


@recipe
def ctf_misc():
    out = {}
    w, h = 256, 512
    x, y = grid(w, h)
    col = fabric(w, h, 970, '#1a2a6a')
    pts = [(cx, (j + 0.5) * 32) for cx in (8, 248) for j in range(16)]
    col = lerp(col, fill(w, h, '#9a9aa0'), rivet_field(x, y, pts, 3) > 0)
    out['textures/ctf/test2.tga'] = (col, np.zeros((h, w), F))     # used as a colour filter only
    w = h = 256
    x, y = grid(w, h)
    s = np.ones((h, w), F)
    for cx, cy, r in ((64, 64, 40), (192, 64, 40), (128, 128, 34), (64, 192, 40), (192, 192, 40)):
        s -= 0.55 * glow_blob(x, y, cx, cy, r * 1.6, 1.4)
    out['textures/sfx/shadow.jpg'] = (np.stack([clamp(blur(s, 6))] * 3, -1) * 0.85, None)
    return out


# ---------------------------------------------------------------- map model skins
def island_mask(md3, shader_part, size, grow=2):
    """Alpha of a model skin: rasterise the UV triangles of the model's
    surfaces whose shader name contains `shader_part`."""
    import struct
    from PIL import ImageDraw, ImageFilter
    d = open(os.path.join(HERE, '..', '..', 'baseq3r', md3), 'rb').read()
    w, h = size
    im = Image.new('L', (w * 4, h * 4), 0)
    dr = ImageDraw.Draw(im)
    nsurf, ofs = struct.unpack_from('<i', d, 84)[0], struct.unpack_from('<i', d, 100)[0]
    for _ in range(nsurf):
        nsh, ntri, otri, osh, ost, oend = (struct.unpack_from('<i', d, ofs + k)[0] for k in (76, 84, 88, 92, 96, 104))
        nvert = struct.unpack_from('<i', d, ofs + 80)[0]
        name = d[ofs + osh: ofs + osh + 64].split(b'\0')[0].decode('latin-1').lower()
        if shader_part in name:
            st = np.frombuffer(d, '<f4', nvert * 2, ofs + ost).reshape(-1, 2)
            tri = np.frombuffer(d, '<i4', ntri * 3, ofs + otri).reshape(-1, 3)
            for t3 in tri:
                pts = [((st[i, 0] % 1.0001) * w * 4, (st[i, 1] % 1.0001) * h * 4) for i in t3]
                dr.polygon(pts, fill=255)
        ofs += oend
    im = im.resize((w, h), Image.LANCZOS).filter(ImageFilter.MaxFilter(2 * grow + 1))
    return (np.asarray(im, F) / 255 > 0.5).astype(F)


@recipe
def map_models():
    out = {}
    w = h = 128
    x, y = grid(w, h)
    bone = gradient_map(noise(w, h, 980, 2, 30, 1.8), [(0, '#5a4430'), (0.5, '#a08860'), (1, '#d8c8a0')])
    f1, f2, _ = worley(w, h, 30, 981)
    crack = smooth(1.0, 0, f2 - f1) * (noise(w, h, 982, 3, 20) > 0.55)
    hm = noise(w, h, 983, 4, 40) * 0.4 - crack * 0.3
    bone = finish(light(bone, 1 - crack * 0.5), hm, 6, 0.6, 3, 980)
    # face features where the model's UV layout puts them
    sock = np.minimum(sd_circle((x - 42) / 1.4, y - 72, 0, 0, 10), sd_circle((x - 88) / 1.4, y - 72, 0, 0, 10))
    nose = sd_poly(x, y, [(64, 80), (56, 96), (72, 96)])
    teeth = sd_box(x, y, 66, 108, 36, 8, 3)
    tx = ((x - 30) % 8) - 4
    each = np.maximum(sd_box(tx, y - 108, 0, 0, 3.2, 7, 2.5), teeth)
    gaps = (teeth < 0) & (each > 0)
    hm2 = -bevel(sock, 6) * 0.6 - bevel(nose, 3) * 0.4 + bevel(each, 2) * 0.3
    bone = finish(bone, hm2, 6, 0.6, 3, 984)
    bone = lerp(bone, fill(w, h, '#120c08'), mask(np.minimum(sock + 4, nose + 1)) * 0.9)
    bone = lerp(bone, fill(w, h, '#e0d4b0'), mask(teeth) * 0.35)
    bone = lerp(bone, fill(w, h, '#2a1c10'), gaps.astype(F) * 0.8)
    bone = lerp(bone, fill(w, h, '#1a120a'), mask(np.abs(y - 108) - 1.2) * (teeth < 0) * 0.8)
    bone = light(bone, 0.6 + 0.4 * glow_blob(x, y, 64, 54, 90, 0.8))
    out['models/mapobjects/Skull/skull.jpg'] = (bone, None)

    # grate torch: dark top with smoke, wooden slats below
    w, h = 32, 64
    x, y = grid(w, h)
    wood = gradient_map(noise(w, h, 984, 1, None, 1.6, aniso=(8, 1)), [(0, '#3a1a08'), (0.5, '#8a4418'), (1, '#c06a30')])
    smoke = light(fill(w, h, '#c0c0c0'), glow_blob(x, y, 16, 18, 12, 1.5) * 0.7)
    out['models/mapobjects/gratelamp/gratetorch2.jpg'] = (np.where((y < 28)[..., None], smoke, wood), None)
    dome = np.minimum(sd_circle(x, y, 16, 26, 14), sd_box(x, y, 16, 46, 15, 20, 0))
    finial = sd_box(x, y, 16, 9, 2.5, 4, 1)
    m = mask(np.minimum(dome, finial))
    bands = bevel(np.abs(y - 30) - 2, 1.5) + bevel(np.abs(y - 48) - 2, 1.5)
    slats = finish(wood, np.abs(np.sin(x / 32 * np.pi * 4)) * 0.5 + bands * 0.5, 4, 0.3)
    out['models/mapobjects/gratelamp/gratetorch2b.tga'] = (slats, m)

    # spot lamp: metal everywhere the islands are, bevelled at their edges
    w, h = 128, 256
    m = island_mask('models/mapobjects/spotlamp/spotlamp.md3', 'spotlamp/spotlamp', (w, h))
    x, y = grid(w, h)
    m = np.where((x > 58) & (y < 104), 0, m)
    m = np.where((x > 60) & (y > 104) & (y < 164), 0, m)
    fork = np.minimum(sd_box(x, y, 92, 62, 26, 18, 6), np.minimum(sd_box(x, y, 72, 30, 5, 28, 2), sd_box(x, y, 112, 30, 5, 28, 2)))
    fork = np.minimum(fork, sd_box(x, y, 92, 88, 10, 10, 3))
    fork = np.maximum(fork, -sd_circle(x, y, 92, 62, 6))
    r = np.sqrt((x - 92) ** 2 + (y - 134) ** 2)
    gear = gear_sd(x - 92, y - 134, 22, 14, 3, 6)
    for k in range(4):
        a = k * np.pi / 2 + 0.4
        gear = np.minimum(gear, sd_segment(x, y, 92, 134, 92 + np.cos(a) * 18, 134 + np.sin(a) * 18, 2.5))
    gear = np.minimum(gear, r - 5)
    m = np.maximum(m, mask(np.minimum(fork, gear)))
    edge = blur(m, 2.0)
    col = worn_plate(w, h, 985, '#5a5856', dirt=0.4, scratch=0.3)
    col = finish(col, edge, 6, 0.6, 2, 985, gloss=0.3)
    out['models/mapobjects/spotlamp/spotlamp.tga'] = (col, m)
    w, h = 32, 128
    x, y = grid(w, h)
    beam = light(fill(w, h, '#a89078'), (1 - y / h) ** 1.6)
    out['models/mapobjects/spotlamp/beam.jpg'] = (beam, None)
    w = h = 32
    x, y = grid(w, h)
    r = np.sqrt((x - 16) ** 2 + (y - 16) ** 2)
    lens = gradient_map(clamp(1 - r / 15), [(0, '#201818'), (0.3, '#806868'), (1, '#fff4f0')])
    out['models/mapobjects/spotlamp/spotlamp_l.jpg'] = (lens, None)
    return out
