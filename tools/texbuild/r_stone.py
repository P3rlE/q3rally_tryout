"""Stone, brick and plaster walls; the large wall picture of q3r_ctf_01."""
from common import *


def block_wall(w, h, rows, seed, palette, minw, maxw, mortar_col, rough=1.0, cracks=0.4):
    hm, bid, n = stone_blocks(w, h, rows, seed, minw, maxw, 2.0, 5.0)
    rng = np.random.default_rng(seed + 1)
    pick = rng.integers(0, len(palette), n)
    tval = rng.uniform(0.8, 1.15, n).astype(F)
    col = np.zeros((h, w, 3), F)
    for k, c in enumerate(palette):
        col = lerp(col, fill(w, h, c), (pick[bid] == k).astype(F))
    col = light(col, tval[bid])
    sc, t = stone(w, h, seed + 2, '#404040', '#c0c0c0', rough)
    col = light(col, 0.6 + 0.6 * t)
    f1, f2, _ = worley(w, h, max(20, w * h // 4000), seed + 3)
    crack = smooth(1.2, 0, f2 - f1) * cracks * (noise(w, h, seed + 4, 3, 20) > 0.45)
    col = light(col, 1 - crack * 0.6)
    mort = (hm < 0.05).astype(F)
    col = lerp(col, light(fill(w, h, mortar_col), 0.7 + 0.5 * t), mort)
    surf = hm * 0.8 + t * 0.25 * rough - crack * 0.15
    return finish(col, surf, 6, 0.6, 3, seed, gloss=0.05)


@recipe
def stone_walls():
    out = {}
    out['textures/gothic_block/block10d.jpg'] = (block_wall(512, 512, 7, 700, ['#6a5a48', '#5a4e44', '#7a6248', '#4e4a44', '#6e4a36'],
                                                            0.12, 0.3, '#2a241e', 1.2, 0.5), None)
    out['textures/gothic_block/blocks17.jpg'] = (block_wall(512, 512, 4, 701, ['#6a6064', '#5e565c', '#726a68', '#625a5e'],
                                                            0.2, 0.45, '#3a3436', 0.8, 0.25), None)
    # small regular bricks
    w = h = 256
    x, y = grid(w, h)
    rowi = (y // 16).astype(int)
    off = (rowi % 2) * 18
    lx = (x + off) % 36 - 18
    ly = y % 16 - 8
    d = sd_box(lx, ly, 0, 0, 16.5, 6.5, 1.5) + (noise(w, h, 702, 8, 60) - 0.5) * 2.5
    hm = bevel(d, 3)
    bidx = ((x + off) // 36).astype(int) * 31 + rowi * 7
    tv = np.random.default_rng(703).uniform(0.7, 1.2, 4096).astype(F)[bidx % 4096]
    sc, t = stone(w, h, 704, '#2a2826', '#6a645c')
    col = light(sc, tv)
    col = lerp(col, fill(w, h, '#151413'), (hm < 0.05).astype(F))
    out['textures/gothic_wall/streetbricks11.jpg'] = (finish(col, hm * 0.8 + t * 0.2, 5, 0.6, 2, 702), None)
    return out


@recipe
def support2b():
    w = h = 512
    x, y = grid(w, h)
    col = concrete(w, h, 710, '#b8a48a', 0.25)
    stain = noise(w, h, 711, 2, 20)
    col = lerp(col, fill(w, h, '#8a6a50'), smooth(0.55, 0.85, stain) * 0.5)
    f1, f2, _ = worley(w, h, 40, 712)
    crack = smooth(1.0, 0, f2 - f1) * (noise(w, h, 713, 3, 20) > 0.6) * (y < 440)
    col = light(col, 1 - crack * 0.5)
    hm = noise(w, h, 714, 20) * 0.1 - crack * 0.2
    for cy, wd, hg in ((8, 14, 0.6), (452, 30, 0.8), (480, 10, 0.5), (500, 14, 0.7)):
        b = bevel(np.abs(y - cy) - wd / 2, min(5, wd / 2), hg)
        hm = np.maximum(hm, b)
    deco = (y > 436) | (y < 16)
    col = lerp(col, worn_plate(w, h, 715, '#8a7a5e', dirt=0.4), deco.astype(F))
    return {'textures/gothic_wall/support2b.jpg': (finish(col, hm, 5, 0.6, 3, 710), None)}


@recipe
def wall_picture():
    """One 512 x 1280 relief cut into ten 256 px tiles (L/R, rows 5 at the top to 1 at the bottom)."""
    W, H = 512, 1280
    x, y = grid(W, H)
    bg = block_wall(W, H, 26, 720, ['#5a5654', '#4e4a4a', '#625c58'], 0.12, 0.3, '#262424', 0.9, 0.2)
    cx = 256
    # pointed arch at the top, straight body, flared foot
    arch = np.maximum(sd_circle(x, y, cx + 150, 330, 260), sd_circle(x, y, cx - 150, 330, 260))
    arch = np.maximum(arch, y - 330)
    arch = np.maximum(arch, 40 - y)
    body = sd_box(x, y, cx, 760, 110, 440, 0)
    foot = sd_poly(x, y, [(cx - 110, 1100), (cx + 110, 1100), (cx + 220, 1270), (cx - 220, 1270)])
    foot = np.maximum(foot, -np.minimum(sd_circle(x, y, cx - 250, 1150, 90), sd_circle(x, y, cx + 250, 1150, 90)))
    panel = np.minimum(np.minimum(arch, body), foot)
    hm = np.where(panel < 0, 0.45 + bevel(panel, 10) * 0.3, 0.0)
    col = lerp(bg, worn_plate(W, H, 721, '#4a2a34', dirt=0.4, scratch=0.5, var=0.25), mask(panel))
    # steering-wheel emblem
    r = np.sqrt((x - cx) ** 2 + (y - 430) ** 2)
    ring = np.abs(r - 118) - 16
    spokes = np.minimum(np.minimum(sd_segment(x, y, cx, 430, cx - 112, 450, 12), sd_segment(x, y, cx, 430, cx + 112, 450, 12)),
                        sd_segment(x, y, cx, 430, cx, 548, 14))
    hub = r - 34
    emb = np.minimum(np.minimum(ring, spokes), hub)
    hm = np.maximum(hm, np.where(emb < 0, 0.75 + bevel(emb, 8) * 0.3, 0))
    hm = np.maximum(hm, np.where(r < 34, 0.9 + rivet_field(x, y, [(cx, 430)], 16) * 0.2, 0))
    brass = worn_plate(W, H, 722, '#a08458', dirt=0.4, scratch=0.4)
    col = lerp(col, brass, mask(emb))
    # vertical rib with bolts, and side brackets
    rib = sd_box(x, y, cx, 840, 16, 250, 6)
    hm = np.maximum(hm, np.where(rib < 0, 0.65 + bevel(rib, 5) * 0.2, 0))
    bolts = [(cx, 610 + 40 * j) for j in range(12)] + [(cx + sx * 90, 300 + 60 * j) for sx in (-1, 1) for j in range(14)]
    bf = rivet_field(x, y, bolts, 6)
    hm = np.maximum(hm, np.where(bf > 0, 0.7 + bf * 0.2, 0))
    col = lerp(col, brass, (bf > 0) * 0.7)
    for sy in (720, 980):
        for sx in (-1, 1):
            arc = np.abs(np.sqrt((x - cx - sx * 110) ** 2 + (y - sy) ** 2) - 70) - 8
            arc = np.maximum(arc, sx * (cx + sx * 110 - x))
            hm = np.maximum(hm, np.where(arc < 0, 0.5 + bevel(arc, 4) * 0.3, 0))
            col = lerp(col, brass, mask(arc))
    out_img = finish(col, hm, 6, 0.6, 4, 720, gloss=0.25)
    out = {}
    for row in range(5):
        n = 5 - row
        for side, x0 in (('L', 0), ('R', 256)):
            tile = out_img[row * 256:(row + 1) * 256, x0:x0 + 256]
            out[f'textures/gothic_cath/graycathflesh{n}{side}.jpg'] = (np.ascontiguousarray(tile), None)
    return out
