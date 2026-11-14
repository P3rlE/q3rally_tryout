"""Metal walls, floors and trims."""
from common import *


# ---------------------------------------------------------------- floors
@recipe
def diamond_floors():
    return {
        'textures/base_floor/diamond2.jpg': (diamond_plate(256, 256, 32, 101, '#55575a'), None),
        'textures/base_floor/diamond2c.jpg': (diamond_plate(512, 512, 40, 102, '#4a4c4f'), None),
        'textures/base_floor/clang_floor.jpg': (diamond_plate(256, 256, 26, 103, '#6a5a48', rust=0.55), None),
    }


@recipe
def hfloor3():
    w = h = 256
    x, y = grid(w, h)
    cell = 256 / 14
    lx, ly = tiled(x, y, cell, cell)
    lx2, ly2 = tiled(x, y, cell, cell, cell / 2, cell / 2)
    d = np.minimum(sd_circle(lx, ly, 0, 0, cell * 0.24), sd_circle(lx2, ly2, 0, 0, cell * 0.24))
    hole = mask(d)
    hm = smooth(-1.5, 1.5, d)
    col = worn_plate(w, h, 110, '#8a6a42', rust=0.35, dirt=0.4)
    col = finish(col, blur(hm, 0.8), 6, 0.5, 2, 110, gloss=0.15)
    col = lerp(col, fill(w, h, '#0c0a08'), hole * 0.92)
    return {'textures/base_floor/hfloor3.jpg': (col, None)}


def tile_floor(w, h, n, seed):
    hm = tile_grid(w, h, n, n, 0.8, 1.2)
    col = worn_plate(w, h, seed, '#3e3529', dirt=0.5, var=0.25, scratch=0.15)
    blot = noise(w, h, seed + 9, 2, 12)
    col = light(col, 0.8 + 0.4 * blot)
    return finish(col, hm * 0.5 + noise(w, h, seed + 3, 20) * 0.05, 3, 0.7, 2, seed), hm


@recipe
def tilefloor7():
    a, _ = tile_floor(128, 128, 2, 120)
    w = h = 256
    b, hm = tile_floor(w, h, 4, 121)
    x, y = grid(w, h)
    # broken-off area: an irregular hole through the tiles shows a slatted grating below
    hole = sd_box(x, y, 138, 150, 72, 42, 6) + (noise(w, h, 122, 4, 30) - 0.5) * 34
    hole = np.minimum(hole, sd_box(x, y, 175, 92, 36, 30, 4) + (noise(w, h, 123, 4, 30) - 0.5) * 22)
    m = mask(hole, 1.0)
    slat = 0.5 + 0.5 * np.cos(y / 256 * 2 * np.pi * 22)
    under = light(fill(w, h, '#2b2724'), 0.25 + 0.75 * slat ** 3)
    under = light(under, 0.6 + 0.4 * noise(w, h, 124, 8))
    rim = smooth(0, 3, -hole) * (1 - smooth(3, 6, -hole))
    out = lerp(b, under, m)
    out = lerp(out, fill(w, h, '#0e0c0a'), rim * 0.8)
    # glow seen through the gaps (added on top in the shader)
    fx = light(fill(w, h, '#ff7a1a'), m * slat ** 6 * (0.3 + 0.7 * noise(w, h, 125, 3, 20)))
    return {'textures/base_floor/tilefloor7.jpg': (a, None),
            'textures/base_floor/tilefloor7_ow.jpg': (out, None),
            'textures/base_floor/tilefloor7_owfx.jpg': (clamp(fx * 0.7), None)}


# ---------------------------------------------------------------- simple plates
@recipe
def plates():
    out = {}
    # rust-streaked sheet metal with two horizontal seams
    w = h = 512
    x, y = grid(w, h)
    seams = np.minimum(np.abs(y - 170), np.abs(y - 342))
    vlines = np.abs(((x + 40) % 128) - 64)
    hm = 1 - smooth(2.5, 0, seams) * 0.7 - smooth(1.2, 0, 64 - vlines) * 0.25
    col = worn_plate(w, h, 130, '#7d8486', rust=0.12, dirt=0.25, rust_scale=4)
    st = streaks(w, h, 131, 160, (0.15, 0.6), (1.5, 7))
    col = lerp(col, rust_colour(w, h, 132), st * 0.85)
    out['textures/base_support/surface2.jpg'] = (finish(col, hm, 4, 0.5, 2, 130), None)

    # brown pewter, rust along the top edge
    w = h = 256
    x, y = grid(w, h)
    col = worn_plate(w, h, 133, '#4b4337', dirt=0.4, scratch=0.35)
    top = smooth(26, 4, y) * (0.6 + 0.4 * noise(w, h, 134, 10))
    col = lerp(col, rust_colour(w, h, 135), top)
    out['textures/base_trim/rusty_pewter_big.jpg'] = (finish(col, noise(w, h, 136, 20) * 0.1, 3, 0.3), None)

    # dark red paint, vertical drips
    w = h = 128
    col = worn_plate(w, h, 137, '#5e1612', dirt=0.25, scratch=0.15, var=0.2)
    st = streaks(w, h, 138, 30, (0.2, 0.9), (0.6, 2))
    col = lerp(col, fill(w, h, '#2a0a08'), st * 0.6)
    out['textures/base_wall/redmet4small.jpg'] = (col, None)

    # pitted rust
    w = h = 512
    col = rust_colour(w, h, 139)
    pits = worley(w, h, 900, 140)[0]
    hm = clamp(pits / 5) + noise(w, h, 141, 30) * 0.3
    col = light(col, 0.75 + 0.35 * noise(w, h, 142, 2, 16))
    col = lerp(col, fill(w, h, '#3d3a36'), smooth(0.55, 0.8, noise(w, h, 143, 3, 30)) * 0.5)
    out['textures/gothic_trim/pitted_rust.jpg'] = (finish(col, hm, 3, 0.6, 1.5, 139), None)

    # rough rusty plate (used as floor)
    col = rust_colour(w, h, 144)
    col = light(col, 0.7 + 0.4 * noise(w, h, 145, 2, 10))
    col = lerp(col, fill(w, h, '#2a1810'), smooth(0.6, 0.9, noise(w, h, 146, 4, 40)) * 0.6)
    hm = noise(w, h, 147, 6, None, 1.3)
    out['textures/gothic_floor/blocks17floor3.jpg'] = (finish(col, hm, 6, 0.4, 2, 144), None)
    return out


def purple_plate(w, h, seed, rivets_x=None, nrivets=8, inset=10):
    col = worn_plate(w, h, seed, '#7a6a72', rust=0.25, dirt=0.25, var=0.2, rust_scale=5)
    col = lerp(col, fill(w, h, '#5a5a86'), smooth(0.55, 0.85, noise(w, h, seed + 40, 2, 12)) * 0.45)
    x, y = grid(w, h)
    hm = noise(w, h, seed + 41, 20) * 0.06
    if rivets_x:
        pts = [(rx, (j + 0.5) * h / nrivets) for rx in rivets_x for j in range(nrivets)]
        hm = hm + rivet_field(x, y, pts, 4.0) * 0.8
    return finish(col, hm, 5, 0.3, 2, seed, gloss=0.25)


@recipe
def bluemetal():
    return {
        'textures/base_wall/bluemetal3.jpg': (purple_plate(256, 512, 150, (12, 244), 10), None),
        'textures/base_wall/bluemetal3b.jpg': (purple_plate(512, 512, 151), None),
        'textures/base_wall/bluemetalsupport.jpg': (purple_plate(192, 204, 152, (10, 182), 6), None),
    }


# ---------------------------------------------------------------- slotted, banded
@recipe
def grate1_4():
    w = h = 256
    x, y = grid(w, h)
    ly = (y % 32) - 16
    d = sd_box(x, ly, 128, 0, 100, 4.5, 4.5)
    slot = mask(d)
    hm = 1 - bevel(-d + 0, 3)
    col = worn_plate(w, h, 160, '#6a5a46', rust=0.55, dirt=0.4)
    col = finish(col, hm, 5, 0.6, 3, 160)
    col = lerp(col, fill(w, h, '#0b0907'), slot * 0.95)
    return {'textures/base_support/grate1_4.jpg': (col, None)}


def bands(w, h, spec_list, seed, axis=0):
    """Horizontal (axis=0) relief bands: [(pos, width, height, colour), ...] in px."""
    x, y = grid(w, h)
    t = y if axis == 0 else x
    hm = np.zeros((h, w), F)
    col = worn_plate(w, h, seed, '#55524d', dirt=0.3)
    for pos, wd, hgt, c, *rest in spec_list:
        d = np.abs(t - pos) - wd / 2
        prof = bevel(d, min(wd / 2, rest[0] if rest else 3), hgt)
        sel = prof > 0.02
        if c:
            col = lerp(col, worn_plate(w, h, seed + int(pos), c, dirt=0.3), mask(d))
        hm = np.where(sel, np.maximum(hm, prof), hm)
    return col, hm


@recipe
def banded_trims():
    out = {}
    # rusty upper plate, pipe bands, rusty lower plate
    w, h = 128, 256
    col, hm = bands(w, h, [(150, 10, 0.8, '#4d4a45', 5), (168, 6, 0.5, None), (196, 14, 0.9, '#5a5248', 7), (226, 6, 0.5, None)], 170)
    col = lerp(col, rust_colour(w, h, 171), rust_layer(w, h, 172, 0.55, 4) * 0.9)
    out['textures/base_support/pip03_3.jpg'] = (finish(col, hm, 5, 0.6, 2, 170, gloss=0.2), None)

    # wide support beam: stacked grey bands
    w, h = 512, 256
    col, hm = bands(w, h, [(28, 26, 0.7, '#6e6c66', 4), (70, 18, 0.4, '#3a3833', 3), (112, 36, 1.0, '#8a8780', 6),
                           (158, 14, 0.5, '#4a4740', 3), (192, 30, 0.8, '#76736b', 5), (234, 16, 0.4, '#2e2c28', 3)], 173)
    out['textures/base_trim/basemetalsupport.jpg'] = (finish(col, hm, 4, 0.6, 2, 173, gloss=0.25), None)

    # dark grimy strip
    w, h = 512, 64
    col, hm = bands(w, h, [(8, 10, 0.5, '#2e2c29', 3), (34, 34, 0.9, '#3d3a35', 8), (58, 8, 0.4, '#24221f', 3)], 174)
    col = grime(col, 0.6, 175)
    out['textures/gothic_trim/border7.jpg'] = (finish(col, hm, 4, 0.7, 2, 174), None)

    # banded trim with one bright band
    w = h = 128
    col, hm = bands(w, h, [(10, 14, 0.6, '#3a2e2a', 4), (34, 18, 0.9, '#4a3a33', 6), (58, 12, 0.5, '#2a221f', 3),
                           (88, 34, 1.0, '#7a6f72', 6), (116, 12, 0.5, '#4f4545', 3)], 176)
    out['textures/gothic_wall/crack5c.jpg'] = (finish(col, hm, 4, 0.6, 2, 176, gloss=0.2), None)
    return out


@recipe
def cable():
    w = h = 256
    x, y = grid(w, h)
    rng = np.random.default_rng(180)
    hm = np.zeros((h, w), F)
    tone = np.zeros((h, w), F)
    pos = 0.0
    while pos < h:
        r = rng.uniform(4, 9)
        c = pos + r
        prof = np.sqrt(clamp(1 - ((y - c) / r) ** 2))
        sel = np.abs(y - c) < r
        hm = np.where(sel, prof * r / 9, hm)
        tone = np.where(sel, rng.uniform(0.2, 0.6), tone)
        pos += 2 * r + rng.uniform(0, 2)
    base = light(fill(w, h, '#7a7466'), tone)
    col = finish(base, hm * 3, 3, 0.2, 1, 180, gloss=0.5, power=30)
    col = light(col, 0.85 + 0.3 * noise(w, h, 181, 4, None, 1.2, aniso=(1, 10)))
    return {'textures/base_support/cable.jpg': (col, None)}


# ---------------------------------------------------------------- concrete with pipes
@recipe
def concretepipe01():
    w = h = 256
    x, y = grid(w, h)
    col = concrete(w, h, 190, '#86827a')
    hm = noise(w, h, 191, 30) * 0.15
    pipes = np.zeros((h, w), F)
    for cy in (100, 150):
        r = 17
        prof = np.sqrt(clamp(1 - ((y - cy) / r) ** 2))
        pipes = np.maximum(pipes, prof)
    bracket = mask(sd_box(x, y, 128, 125, 6, 52, 2))
    hp = np.maximum(pipes * 14, bracket * 18) / 18
    pcol = worn_plate(w, h, 192, '#4c463e', rust=0.25, dirt=0.4)
    shadow = 1 - 0.55 * smooth(0, 1, blur((hp > 0.01).astype(F), 6)) * (y > 110)
    col = light(col, shadow)
    st = streaks(w, h, 193, 60, (0.1, 0.35), (1, 4)) * (y > 160) * (y < 250)
    col = lerp(col, fill(w, h, '#3a2618'), st * 0.8)
    col = lerp(col, pcol, (hp > 0.01).astype(F))
    col = finish(col, hp * 1.0 + hm, 6, 0.4, 3, 190, gloss=0.25)
    return {'textures/base_wall/concretepipe01.jpg': (col, None)}


# ---------------------------------------------------------------- pipes on a tech wall
def tech_wall(seed, rust_bottom=0.0):
    w = h = 256
    x, y = grid(w, h)
    rng = np.random.default_rng(seed)
    # background: small recessed panels
    lx, ly = tiled(x, y, 16, 10, 0, (x // 16 % 2) * 5)
    bg = bevel(sd_box(lx, ly, 0, 0, 6.5, 3.5, 1), 1.5) * 0.25
    col = worn_plate(w, h, seed, '#4e4239', dirt=0.5, scratch=0.1)
    hm = bg.copy()
    pcol = worn_plate(w, h, seed + 1, '#6a5e52', rust=0.25, dirt=0.3)
    tint = np.zeros((h, w), F)
    layers = []
    for cx in (14, 52, 60, 118, 170, 178, 226):            # vertical pipes run through
        r = rng.uniform(3, 6)
        layers.append(([(cx, -10), (cx, h + 10)], r))
    for cy in (36, 132, 206):
        layers.append(([(-10, cy), (w + 10, cy)], rng.uniform(4, 6)))
    for k in range(5):                                       # bent pipes between the straight ones
        x0 = rng.uniform(20, 230)
        y0 = rng.uniform(10, 120)
        pts = [(x0, y0), (x0, y0 + rng.uniform(30, 60)), (x0 + rng.uniform(-40, 40), y0 + rng.uniform(70, 100)),
               (x0 + rng.uniform(-40, 40), y0 + rng.uniform(110, 130))]
        pts = [(px, py) for px, py in pts]
        layers.append((rounded_path([pts[0], (pts[0][0], pts[1][1]), (pts[2][0], pts[1][1]), (pts[2][0], pts[3][1])], 12), rng.uniform(2.5, 4)))
    for i, (pts, r) in enumerate(layers):
        prof, m = tube_layer(x, y, pts, r)
        lift = 0.3 + i * 0.03
        hm = np.where(m, lift + prof * r / 6, hm)
        tint = np.where(m, rng.uniform(0.7, 1.15), tint)
    pipes = tint > 0
    col = lerp(col, light(pcol, np.where(pipes, tint, 1)), pipes.astype(F))
    out = finish(col, hm, 7, 0.6, 2, seed, gloss=0.25)
    if rust_bottom:
        m = smooth(150, 250, y) * rust_layer(w, h, seed + 5, 0.6, 5)
        out = lerp(out, light(rust_colour(w, h, seed + 6), shade(hm, 7)), m * rust_bottom)
    return out


@recipe
def atech():
    return {'textures/base_wall/atech1_a.jpg': (tech_wall(200), None),
            'textures/base_wall/atech1_b.jpg': (tech_wall(201), None),
            'textures/base_wall/atech1_e.jpg': (tech_wall(202, 0.9), None)}


# ---------------------------------------------------------------- circuit traces
@recipe
def metalfloor_wall_10():
    w = h = 512
    x, y = grid(w, h)
    rng = np.random.default_rng(210)
    g = 32
    tr = np.full((h, w), 1e9, F)
    pads = np.full((h, w), 1e9, F)
    for _ in range(30):
        px, py = rng.integers(0, w // g) * g, rng.integers(0, h // g) * g
        pts = [(px, py)]
        for _ in range(rng.integers(2, 6)):
            dirn = rng.integers(0, 6)
            L = rng.integers(1, 4) * g
            dx, dy = [(1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1)][dirn if rng.random() < 0.8 else rng.integers(0, 4)]
            px, py = px + dx * L, py + dy * L
            pts.append((px, py))
        for (x0, y0), (x1, y1) in zip(pts[:-1], pts[1:]):
            for ox in (-w, 0, w):
                for oy in (-h, 0, h):
                    tr = np.minimum(tr, sd_segment(x, y, x0 + ox, y0 + oy, x1 + ox, y1 + oy, 3.4))
        ex, ey = pts[-1]
        dx_, dy_ = wrapd(x, y, ex, ey, w, h)
        pads = np.minimum(pads, np.sqrt(dx_ ** 2 + dy_ ** 2) - 6)
    pads = np.maximum(pads, 3 - np.sqrt(np.minimum(pads + 6, 99) ** 2))   # ring pads
    d = np.minimum(tr, pads)
    hm = bevel(d, 1.5) * 0.6 + noise(w, h, 211, 30) * 0.1
    col = worn_plate(w, h, 212, '#3a1d18', dirt=0.4, var=0.25)
    col = lerp(col, worn_plate(w, h, 213, '#8a3a2a', dirt=0.2), mask(d))
    return {'textures/base_wall/metalfloor_wall_10.jpg': (finish(col, hm, 5, 0.6, 2, 210, gloss=0.2), None)}


# ---------------------------------------------------------------- gear relief
def gear_relief(seed, base):
    w = h = 512
    x, y = grid(w, h)
    rng = np.random.default_rng(seed)
    hm = noise(w, h, seed, 20, None, 1.2) * 0.12
    for k in range(9):
        cx, cy = rng.random() * w, rng.random() * h
        dx, dy = wrapd(x, y, cx, cy, w, h)
        R = rng.uniform(50, 150)
        lvl = 0.25 + 0.08 * k
        if k % 3 == 2:
            d = np.abs(np.sqrt(dx * dx + dy * dy) - R) - rng.uniform(5, 10)    # plain ring
        else:
            d = gear_sd(dx, dy, R, int(R / 9), 9, rng.uniform(16, 26))
        hub = sd_circle(dx, dy, 0, 0, rng.uniform(10, 22))
        d = np.minimum(d, np.maximum(hub, -(hub + 6)))
        b = bevel(d, 3) * lvl
        hm = np.where(b > 0.01, np.maximum(hm, 0.1 + b), hm)
    col = worn_plate(w, h, seed + 1, base, dirt=0.5, var=0.25, scratch=0.2)
    grit = worley(w, h, 1600, seed + 2)[0]
    col = light(col, 0.85 + 0.25 * clamp(grit / 4))
    return finish(col, hm + clamp(grit / 6) * 0.03, 14, 0.9, 4, seed, gloss=0.15)


@recipe
def metaltech():
    return {'textures/base_wall/metaltech04final.jpg': (gear_relief(220, '#6b6052'), None),
            'textures/base_wall/metaltech06final.jpg': (gear_relief(221, '#4e3b2c'), None)}


# ---------------------------------------------------------------- angular floor blocks
@recipe
def metfloor_block_3():
    """Raised blocks made of 32 px grid cells (region growing), 45 degree corners."""
    from scipy import ndimage
    w = h = 512
    n, cs = 16, 32
    rng = np.random.default_rng(230)
    reg = -np.ones((n, n), int)
    k = 0
    for i in rng.permutation(n * n):
        cy, cx = divmod(i, n)
        if reg[cy, cx] >= 0:
            continue
        size = rng.integers(5, 14)
        front = [(cy, cx)]
        reg[cy, cx] = k
        got = 1
        tries = 0
        while got < size and tries < 60:
            tries += 1
            fy, fx = front[rng.integers(len(front))]
            dy, dx = [(0, 1), (1, 0), (0, -1), (-1, 0)][rng.integers(4)]
            ny, nx = (fy + dy) % n, (fx + dx) % n
            if reg[ny, nx] < 0:
                reg[ny, nx] = k
                front.append((ny, nx))
                got += 1
        k += 1
    ids = np.kron(reg, np.ones((cs, cs), int))
    edge = (ids != np.roll(ids, 1, 0)) | (ids != np.roll(ids, 1, 1))
    big = np.tile(~edge, (3, 3))
    dist = ndimage.distance_transform_edt(big)[h:2 * h, w:2 * w].astype(F)
    dist = blur(dist, 2.0)                          # rounds the inner corners off
    hm = smooth(2, 7, dist) * 0.45 + smooth(13, 15, dist) * 0.06
    tone_ = cell_values(ids, k, 231)
    col = worn_plate(w, h, 232, '#6b5a50', dirt=0.4, var=0.2, scratch=0.3)
    col = light(col, 0.85 + 0.3 * tone_)
    col = lerp(col, rust_colour(w, h, 233), rust_layer(w, h, 234, 0.3, 5) * 0.6)
    return {'textures/base_wall/metfloor_block_3.jpg': (finish(col, hm, 4, 0.5, 3, 230, gloss=0.05), None)}


# ---------------------------------------------------------------- patched sheets
def patched(seed, base, rust=0.2, rust_scale=4):
    w = h = 512
    x, y = grid(w, h)
    rng = np.random.default_rng(seed)
    col = worn_plate(w, h, seed, base, rust=rust, dirt=0.5, var=0.25, rust_scale=rust_scale)
    hm = noise(w, h, seed + 1, 20) * 0.08
    for k in range(16):
        cx, cy = rng.random() * w, rng.random() * h
        hw, hh = rng.uniform(30, 120), rng.uniform(30, 120)
        dx, dy = wrapd(x, y, cx, cy, w, h)
        d = sd_box(dx, dy, 0, 0, hw, hh, 2)
        m = mask(d)
        pc = worn_plate(w, h, seed + 10 + k, base, rust=rust * 1.2, dirt=0.4, var=0.3, rust_scale=rust_scale)
        pc = light(pc, rng.uniform(0.75, 1.2))
        col = lerp(col, pc, m)
        hm = np.where(m > 0.5, 0.2 + 0.02 * k + bevel(d, 1.5) * 0.1, hm)
        seam = smooth(1.5, 0, np.abs(d))
        col = lerp(col, fill(w, h, '#14110c'), seam * 0.6)
    col = grime(col, 0.4, seed + 3)
    return finish(col, hm, 4, 0.6, 2, seed)


@recipe
def patch10():
    return {'textures/base_wall/patch10_beatup2.jpg': (patched(240, '#4f4a36', 0.15), None),
            'textures/base_wall/patch10_beatup3.jpg': (patched(241, '#5a4632', 0.2), None),
            'textures/base_wall/patch10rusty5.jpg': (patched(242, '#7a5a3e', 0.5, 6), None)}


# ---------------------------------------------------------------- vents and inset panels
def frame_panel(w, h, seed, base, inset, r=10):
    x, y = grid(w, h)
    d = sd_box(x, y, w / 2, h / 2, w / 2 - inset, h / 2 - inset, r)
    outer = bevel(sd_box(x, y, w / 2, h / 2, w / 2 - 1, h / 2 - 1, r + 4), 5)
    hm = outer * 0.7 - bevel(d, 5) * 0.4
    col = worn_plate(w, h, seed, base, dirt=0.4, scratch=0.3)
    return x, y, d, hm, col


@recipe
def bluemetalsupport2():
    out = {}
    w = h = 128
    x, y, d, hm, col = frame_panel(w, h, 250, '#6d6c68', 14)
    inside = d < 0
    ly = (y - 14) % 7
    slat = np.where(inside & (np.abs(x - 64) > 3), 0.15 + 0.3 * smooth(0, 5, ly) - 0.4 * smooth(5, 7, ly), 0)
    hm = np.where(inside, -0.4 + slat, hm)
    col = lerp(col, rust_colour(w, h, 251), smooth(90, 125, y) * rust_layer(w, h, 252, 0.7, 4))
    out['textures/base_wall/bluemetalsupport2c_1vent.tga'] = (finish(col, hm, 6, 0.7, 2, 250, gloss=0.2), None)

    # plank / inset square / plank; the tv version has a screen in the square
    w, h = 128, 256
    x, y = grid(w, h)
    band = (np.abs(y - 128) > 66)
    col = metal(w, h, 253, '#8a8780', 0.1, brushed=0.25)
    col = grime(col, 0.3, 254)
    hm = np.where(band, 0.5, 0.0) + noise(w, h, 255, 3, None, 1.2, aniso=(1, 10)) * 0.05
    d = sd_box(x, y, 64, 128, 52, 52, 8)
    hm = hm + bevel(d, 6) * 0.6 - bevel(d + 12, 4) * 0.3
    hm = hm - (smooth(1.5, 0, np.abs(np.abs(y - 128) - 66))) * 0.4
    col = lerp(col, rust_colour(w, h, 256), smooth(160, 185, y) * (y < 194) * rust_layer(w, h, 257, 0.6, 4))
    base = finish(col, hm, 5, 0.6, 2, 253, gloss=0.2)
    out['textures/base_wall/bluemetalsupport2f.tga'] = (base, None)
    scr = sd_box(x, y, 64, 126, 30, 28, 7)
    a = 1 - mask(scr)
    tv = lerp(base, fill(w, h, '#050505'), 1 - a)
    out['textures/base_wall/bluemetalsupport2ftv.tga'] = (tv, a)
    # glow for the screen: drawn at twice the size of the texture
    gx, gy = grid(256, 512)
    g = glow_blob(gx, gy, 128, 252, 90, 2.2) * 0.8 + glow_blob(gx, gy, 128, 252, 14, 1.0)
    out['textures/base_wall/bluemetalsupport2ftv_glow.jpg'] = (light(fill(256, 512, '#3cff40'), g) + glow_blob(gx, gy, 128, 252, 6)[..., None], None)
    return out


# ---------------------------------------------------------------- columns and strips
@recipe
def columns():
    out = {}
    w = h = 512
    x, y = grid(w, h)
    ph = (x % 64) / 64
    hm = np.sin(ph * np.pi) ** 0.5 - smooth(0.03, 0, np.minimum(ph, 1 - ph)) * 0.3
    col = metal(w, h, 260, '#5d6270', 0.15, brushed=0.15)
    col = grime(col, 0.35, 261)
    col = lerp(col, fill(w, h, '#1d1e22'), streaks(w, h, 262, 40, (0.2, 0.6), (3, 12)) * 0.4)
    out['textures/gothic_trim/column2c_test.jpg'] = (finish(col, hm * 0.8, 6, 0.5, 4, 260, gloss=0.35), None)

    # copper bars with rings
    ph = (x % 32) / 32
    hm = np.sin(ph * np.pi) * 0.6
    rings = np.zeros((h, w), F)
    for cy, wd in ((96, 14), (180, 6), (196, 6), (212, 6), (228, 6), (400, 16)):
        rings = np.maximum(rings, bevel(np.abs(y - cy) - wd / 2, min(3, wd / 2)))
    hm = np.maximum(hm, rings * 0.9)
    col = metal(w, h, 263, '#6a4430', 0.2, brushed=0.2)
    col = lerp(col, fill(w, h, '#a06a48'), rings * 0.4)
    col = grime(col, 0.5, 264)
    out['textures/gothic_wall/xiantourneycolumn_b6.jpg'] = (finish(col, hm, 5, 0.6, 3, 263, gloss=0.4, power=16), None)

    # rusty strip with two rows of rivets
    w, h = 128, 512
    x, y = grid(w, h)
    pts = [(cx, (j + 0.5) * 32) for cx in (18, 110) for j in range(16)]
    hm = rivet_field(x, y, pts, 5) * 0.9 + noise(w, h, 265, 20) * 0.08
    hm = hm - smooth(2, 0, np.minimum(x, w - x)) * 0.5
    col = worn_plate(w, h, 266, '#5a4a36', rust=0.45, dirt=0.4)
    out['textures/gothic_wall/supportborder_blue_b.jpg'] = (finish(col, hm, 6, 0.5, 2, 266, gloss=0.3), None)
    return out


@recipe
def panel_edged():
    w, h = 256, 512
    x, y = grid(w, h)
    col = worn_plate(w, h, 270, '#6a5a48', rust=0.2, dirt=0.5)
    hm = np.zeros((h, w), F)
    for k in range(4):
        cy = 64 + k * 128
        d = sd_box(x, y, 128, cy, 120, 58, 10)
        hm = np.maximum(hm, bevel(d, 6) * 0.6)
        # raised bracket in the middle: wide top, narrow neck
        br = np.minimum(sd_box(x, y, 128, cy - 26, 44, 14, 6), sd_box(x, y, 128, cy + 6, 16, 30, 4))
        hm = np.maximum(hm, 0.6 + bevel(br, 4) * 0.4 * (br < 0))
        bolt = [(128 - 90, cy - 40), (128 + 90, cy - 40), (128 - 90, cy + 40), (128 + 90, cy + 40)]
        hm = np.maximum(hm, 0.6 + rivet_field(x, y, bolt, 5) * 0.3)
    col = finish(col, hm, 6, 0.7, 3, 270, gloss=0.25)
    return {'textures/base_trim/panel_edged.jpg': (col, None)}


@recipe
def padwall():
    w = h = 512
    x, y = grid(w, h)
    side = (x < 64) | (x >= 448)
    col = concrete(w, h, 280, '#a59e92')
    dark = worn_plate(w, h, 281, '#3d3f3d', dirt=0.5)
    col = lerp(col, dark, side.astype(F))
    bars = (np.abs(x - 70) < 7) | (np.abs(x - 442) < 7)
    col = lerp(col, worn_plate(w, h, 282, '#4a2620', rust=0.4), bars.astype(F))
    hm = np.where(side, 0.6, 0.2) + np.where(bars, 0.4 * np.sin(np.clip((7 - np.minimum(np.abs(x - 70), np.abs(x - 442))) / 14 * np.pi, 0, np.pi)), 0)
    return {'textures/base_trim/xian_dm3padwall_x.jpg': (finish(col, hm + noise(w, h, 283, 30) * 0.08, 4, 0.5, 3, 280), None)}


# ---------------------------------------------------------------- iron walls (gothic)
def iron_wall(seed, lower, deco):
    w = h = 512
    x, y = grid(w, h)
    upper = worn_plate(w, h, seed, '#5e4232', rust=0.5, dirt=0.4, rust_scale=8)
    low = worn_plate(w, h, seed + 1, lower, dirt=0.5, rust=0.15)
    col = lerp(upper, low, (y > 330).astype(F))
    hm = noise(w, h, seed + 2, 30) * 0.08 + np.where(y > 330, 0.35, 0.2)
    rail = bevel(np.abs(y - 318) - 12, 4)
    hm = np.maximum(hm, rail * 0.8)
    col = lerp(col, worn_plate(w, h, seed + 3, '#4a4440', dirt=0.4), mask(np.abs(y - 318) - 12))
    pts = [((i + 0.5) * 32, 345) for i in range(16)] + [(14, 380 + 40 * j) for j in range(3)]
    hm = np.maximum(hm, 0.35 + rivet_field(x, y, pts, 4) * 0.4)
    if deco == 'notch':
        lx, _ = tiled(x, y, 128, 1, 64, 0)
        d = sd_poly(lx, y, [(-34, 306), (-22, 282), (22, 282), (34, 306)])
        d = np.maximum(d, -sd_poly(lx, y, [(-22, 306), (-14, 292), (14, 292), (22, 306)]))
        hm = np.maximum(hm, bevel(d, 3) * 0.9)
        col = lerp(col, fill(w, h, '#2a2624'), mask(d) * 0.6)
    else:
        lx, _ = tiled(x, y, 128, 1, 64, 0)
        d = sd_poly(lx, y, [(-30, 306), (0, 170), (30, 306)])            # spikes
        hm = np.maximum(hm, bevel(d, 14) * 1.0)
        col = lerp(col, worn_plate(w, h, seed + 4, '#5b5450', dirt=0.4), mask(d))
        lx, _ = tiled(x, y, 102.4, 1, 0, 0)                               # wheel-hub medallions
        dx, dy = lx, y - 430
        r = np.sqrt(dx * dx + dy * dy)
        d = r - 40
        hub = bevel(d, 8) * 0.6
        lug = np.zeros_like(r)
        for k in range(5):
            a = k * 2 * np.pi / 5 - np.pi / 2
            lug = np.maximum(lug, rivet_field(dx, dy, [(np.cos(a) * 22, np.sin(a) * 22)], 6))
        hub = np.maximum(hub, 0.6 - bevel(r - 11, 3) * 0.3 + lug * 0.3 * (r < 40))
        hm = np.where(r < 41, 0.35 + hub, hm)
        col = lerp(col, worn_plate(w, h, seed + 5, '#77706a', dirt=0.4), mask(d))
    return finish(col, hm, 6, 0.6, 3, seed, gloss=0.25)


@recipe
def iron01():
    return {'textures/gothic_wall/iron01_c.jpg': (iron_wall(290, '#3c2a22', 'notch'), None),
            'textures/gothic_wall/iron01_c3.jpg': (iron_wall(291, '#3a2232', 'notch'), None),
            'textures/gothic_wall/iron01_p.jpg': (iron_wall(292, '#3c2a22', 'hub'), None)}


@recipe
def baseboard01():
    w, h = 256, 64
    x, y = grid(w, h)
    col = worn_plate(w, h, 300, '#4e4c48', dirt=0.4)
    ribs = (y < 9) * (0.5 + 0.5 * np.cos((x % 6) / 6 * 2 * np.pi))
    hm = np.where(y < 9, 0.4 + ribs * 0.4, 0.3)
    hm = np.where((y > 46), 0.15, hm)
    col = lerp(col, rust_colour(w, h, 301), (y > 46).astype(F) * 0.8)
    lx, _ = tiled(x, y, 51.2, 1, 25.6, 0)
    r = np.sqrt(lx ** 2 + (y - 28) ** 2)
    ring = bevel(np.abs(r - 11) - 4, 3) * 0.9
    plate = bevel(sd_box(lx, y - 28, 0, 0, 20, 12, 6), 3) * 0.5
    hm = np.maximum(hm, np.maximum(plate + 0.3 * (plate > 0), 0.4 + ring))
    hm = np.maximum(hm, 0.3 + bevel(np.abs(y - 28) - 3, 2) * 0.3)
    col = lerp(col, worn_plate(w, h, 302, '#8a8680', dirt=0.3), mask(np.abs(r - 11) - 4))
    return {'textures/gothic_trim/baseboard01.jpg': (finish(col, hm, 5, 0.6, 2, 300, gloss=0.3), None)}


def small_tiles(seed, colours):
    w = h = 512
    x, y = grid(w, h)
    n = 14
    cs = w / n
    hm = tile_grid(w, h, n, n, 0.6, 1.0)
    ix = (x // cs).astype(int)
    iy = (y // cs).astype(int)
    rng = np.random.default_rng(seed)
    pick = rng.choice(len(colours), (n, n), p=[0.7] + [0.3 / (len(colours) - 1)] * (len(colours) - 1))
    tval = rng.uniform(0.9, 1.1, (n, n)).astype(F)
    col = np.zeros((h, w, 3), F)
    for k, c in enumerate(colours):
        col = lerp(col, fill(w, h, c), (pick[iy % n, ix % n] == k).astype(F))
    col = light(col, tval[iy % n, ix % n] * (0.9 + 0.2 * noise(w, h, seed + 7, 2, 20)) * (0.94 + 0.12 * noise(w, h, seed + 8, 40, None, 1.0)))
    col = lerp(col, clamp(col * 1.4), scratches(w, h, seed + 6, 120, 50) * 0.3)
    col = grime(col, 0.2, seed + 9)
    return finish(col, hm * 0.25, 4, 0.6, 2, seed)


@recipe
def q1metal7():
    return {'textures/gothic_floor/q1metal7_98d_256x256.jpg': (small_tiles(310, ['#5f625a', '#6a6a60', '#55584f', '#6e5e50']), None),
            'textures/gothic_floor/q1metal7_99.jpg': (small_tiles(311, ['#5f7040', '#6e7a48', '#7a6a48', '#5a6a40']), None)}
