"""Grates, fences, braces, cables and frames with alpha (cut-out or shine)."""
from common import *


def cut(d):
    """Alpha for alphaFunc GE128: hard edge on the shape."""
    return (d < 0).astype(F)


@recipe
def grates():
    out = {}
    w = h = 256
    x, y = grid(w, h)
    # two columns of slots with a bar in the middle
    _, ly = tiled(x, y, 1, 32)
    slot = np.minimum(sd_box(x, ly, 66, 0, 52, 8, 2), sd_box(x, ly, 190, 0, 52, 8, 2))
    bar = sd_box(x, y, 128, 128, 8, 200, 0)
    col = worn_plate(w, h, 600, '#5e4a36', rust=0.5, dirt=0.4)
    hm = bevel(-slot, 4) * 0.6 + bevel(bar, 3) * 0.4
    out['textures/base_floor/proto_grate4.tga'] = (finish(col, hm, 5, 0.6, 2, 600, gloss=0.2), 1 - cut(slot))

    # lattice of X braces with triangular openings
    lx, ly = tiled(x, y, 64, 64)
    a = np.abs(lx)
    b = np.abs(ly)
    frame = np.minimum(28 - np.maximum(a, b), 0) * -1
    xbr = np.abs(a - b) / 1.414 - 4.5
    solid = np.minimum(xbr, np.maximum(a, b) * -1 + 28)      # braces + cell border
    solid = np.minimum(xbr, 28 - np.maximum(a, b))
    hole = -solid
    col = worn_plate(w, h, 601, '#6a4a30', rust=0.65, dirt=0.4)
    hm = bevel(solid, 3) * 0.8
    out['textures/base_floor/proto_rustygrate.tga'] = (finish(col, hm, 5, 0.6, 2, 601, gloss=0.15), cut(solid))

    # wheel rim in the lattice: ring, five spokes, hub
    r = np.sqrt((x - 128) ** 2 + (y - 128) ** 2)
    ang = np.arctan2(y - 128, x - 128)
    rim = np.abs(r - 100) - 9
    spokes = np.full((h, w), 1e9, F)
    for k in range(5):
        aa = -np.pi / 2 + k * 2 * np.pi / 5
        for off in (-0.16, 0.16):
            spokes = np.minimum(spokes, sd_segment(x, y, 128 + np.cos(aa) * 24, 128 + np.sin(aa) * 24,
                                                   128 + np.cos(aa + off) * 96, 128 + np.sin(aa + off) * 96, 5))
    hub = r - 26
    hubhole = 9 - r
    inner = np.minimum(np.minimum(rim, spokes), np.maximum(hub, hubhole))
    lattice = np.where(r > 109, solid, 1e9)
    s2 = np.minimum(inner, lattice)
    s2 = np.minimum(s2, np.maximum(sd_box(x, y, 128, 128, 126, 126, 0) * -1, -1e9) * 0 + np.where(np.maximum(np.abs(x - 128), np.abs(y - 128)) > 122, -1, 1e9))
    col = worn_plate(w, h, 602, '#6a4a30', rust=0.6, dirt=0.4)
    hm = bevel(s2, 3) * 0.8 + bevel(rim + 5, 2) * 0.1
    out['textures/base_floor/rusty_pentagrate.tga'] = (finish(col, hm, 5, 0.6, 2, 602, gloss=0.2), cut(s2))

    # round drain grate with bars in a stone square
    stonec = concrete(w, h, 603, '#77736b')
    r = np.sqrt((x - 128) ** 2 + (y - 128) ** 2)
    ring = np.abs(r - 106) - 8
    bars = np.full((h, w), 1e9, F)
    for k in range(7):
        bx = 128 + (k - 3) * 28
        bars = np.minimum(bars, sd_box(x, y, bx, 128, 7, 120, 7))
    opening = r - 100
    solid2 = np.where(opening < 0, bars, -1)
    solid2 = np.minimum(solid2, ring)
    col = lerp(stonec, worn_plate(w, h, 604, '#55524c', dirt=0.4), mask(np.minimum(ring, np.where(opening < 0, bars, 1e9))))
    prof = np.where(opening < 0, np.sqrt(clamp(1 - ((((x - 128 + 14) % 28) - 14) / 7) ** 2)), 0)
    hm = np.where(opening < 0, prof * 0.6, 0.3) + bevel(ring, 4) * 0.5 - bevel(-(r - 114), 4) * 0.2
    out['textures/base_trim/metal2_2.tga'] = (finish(col, hm, 6, 0.6, 2, 603, gloss=0.25), cut(solid2))
    return out


@recipe
def proto_fence():
    w = h = 256
    x, y = grid(w, h)
    lx, ly = tiled(x, y, 128, 128)
    wire = np.abs(np.abs(lx) + np.abs(ly) - 64) / 1.414 - 2.2            # diamond mesh
    wob = (noise(w, h, 611, 8, 30) - 0.5) * 1.4
    wire = wire + wob
    ax, ay = np.abs(lx), np.abs(ly)
    knot = np.minimum(np.sqrt((ax - 64) ** 2 + ay ** 2), np.sqrt(ax ** 2 + (ay - 64) ** 2)) - 5.5
    d = np.minimum(wire, knot)
    prof = np.sqrt(clamp(-d / 2.5))
    col = worn_plate(w, h, 612, '#6a5848', rust=0.5, dirt=0.2)
    col = finish(col, prof, 3, 0.2, 1, 612, gloss=0.4)
    return {'textures/base_trim/proto_fence.tga': (col, cut(d))}


def brace(seed, base, rust, kind):
    w = h = 256
    x, y = grid(w, h)
    border = 18
    frame = -(np.maximum(np.abs(x - 128), np.abs(y - 128)) - (128 - border))
    frame = np.maximum(frame, np.maximum(np.abs(x - 128), np.abs(y - 128)) - 128)
    if kind == 'x':
        diag = np.minimum(sd_segment(x, y, 20, 20, 236, 236, 11), sd_segment(x, y, 236, 20, 20, 236, 11))
    else:
        diag = sd_segment(x, y, 236, 20, 20, 236, 13)
    corners = np.full((h, w), 1e9, F)
    for cx in (0, 256):
        for cy in (0, 256):
            corners = np.minimum(corners, sd_box(x, y, cx, cy, 44, 44, 10))
    s = np.minimum(np.minimum(frame, diag), corners)
    s = np.maximum(s, -1e9)
    col = worn_plate(w, h, seed, base, rust=rust, dirt=0.4, scratch=0.35)
    hm = bevel(frame, 6) * 0.6 + bevel(diag, 6) * 0.5 * (frame > 0)
    hm = np.maximum(hm, bevel(corners, 4) * 0.8)
    pts = [(cx + sx * 24, cy + sy * 24) for cx, sx in ((0, 1), (256, -1)) for cy, sy in ((0, 1), (256, -1))]
    pts += [(cx + sx * 9, cy + sy * 30) for cx, sx in ((0, 1), (256, -1)) for cy, sy in ((0, 1), (256, -1))]
    pts += [(cx + sx * 30, cy + sy * 9) for cx, sx in ((0, 1), (256, -1)) for cy, sy in ((0, 1), (256, -1))]
    hm = np.maximum(hm, 0.8 + rivet_field(x, y, pts, 5) * 0.3)
    return finish(col, hm, 6, 0.6, 3, seed, gloss=0.3), cut(s)


@recipe
def supports():
    xs = brace(620, '#5a4535', 0.6, 'x')
    xs2 = brace(621, '#6a6a68', 0.15, 'x')
    zs = brace(622, '#6a6660', 0.3, 'z')
    return {'textures/base_support/x_support.tga': xs,
            'textures/base_support/x_support2.tga': xs2,
            'textures/base_support/x_support2.png': xs2,
            'textures/base_support/z_support.tga': zs}


@recipe
def grill():
    """Shader: alpha 1 shows chrome, alpha 0 shows the texture colour."""
    w = h = 512
    x, y = grid(w, h)
    lx, _ = tiled(x, y, 64, 1)
    bar = np.abs(lx) - 5
    fine = 0.5 + 0.5 * np.cos(y / 512 * 2 * np.pi * 96)
    col = light(fill(w, h, '#3a3a3c'), 0.4 + 0.6 * fine)
    col = lerp(col, fill(w, h, '#18181a'), (y < 80).astype(F))
    hm = bevel(bar, 4) * 0.8 + fine * 0.1
    col = finish(col, hm, 5, 0.4, 2, 630, gloss=0.3)
    a = clamp(mask(bar) * (0.6 + 0.4 * noise(w, h, 631, 4, 60)) + 0.15 * fine * (y > 80))
    return {'textures/base_wall/grill.tga': (col, a)}


@recipe
def pipecolumn4():
    w = h = 256
    x, y = grid(w, h)
    lx, _ = tiled(x, y, 85.33, 1)
    prof = np.cos(lx / 42.66 * np.pi / 2) ** 0.6
    collar = (np.abs(y - 52) < 22).astype(F)
    col = metal(w, h, 640, '#8a2a10', 0.15, brushed=0.4)
    col = lerp(col, worn_plate(w, h, 641, '#7a7672', dirt=0.4), collar)
    rings = bevel(np.abs(y - 52) - 22, 3) * 0.3 + bevel(np.abs(y - 82) - 3, 2) * 0.3
    hm = prof * 0.8 + rings
    col = finish(col, hm, 6, 0.5, 2, 640, gloss=0.6, power=12)
    a = clamp(prof * 0.9 * (0.7 + 0.3 * noise(w, h, 642, 2, None, 1.2, aniso=(1, 12))))
    return {'textures/base_support/pipecolumn4.tga': (col, a)}


@recipe
def shiny_panels():
    """Alpha 1 = painted texture, lower = environment shine shows through."""
    out = {}
    w = h = 256
    x, y = grid(w, h)
    hm = np.zeros((h, w), F)
    shapes = [sd_box(x, y, 128, 30, 124, 26, 14), sd_box(x, y, 128, 128, 124, 64, 14), sd_box(x, y, 128, 226, 124, 26, 14)]
    for s in shapes:
        hm = np.maximum(hm, bevel(s, 6) * 0.5)
    plateau = np.minimum(sd_box(x, y, 128, 98, 46, 26, 12), sd_box(x, y, 128, 146, 20, 30, 4))
    hm = np.maximum(hm, 0.5 + bevel(plateau, 5) * 0.4 * (plateau < 0))
    col = worn_plate(w, h, 650, '#76685a', rust=0.3, dirt=0.4)
    col = finish(col, hm, 6, 0.6, 3, 650, gloss=0.3)
    a = np.where(plateau < 0, 0.35, 1.0) * (1 - 0.3 * scratches(w, h, 651, 40, 40))
    for cx in (22, 234):
        for cy in (30, 226):
            a = np.where(sd_box(x, y, cx, cy, 14, 16, 4) < 0, 0.4, a)
    out['textures/base_trim/panel_shiny.tga'] = (col, clamp(a))

    w, h = 128, 256
    x, y = grid(w, h)
    side = np.abs(x - 64) > 34
    col = worn_plate(w, h, 652, '#5c5650', dirt=0.4)
    hm = np.where(side, 0.6, 0.1)
    _, ly = tiled(x, y, 1, 32)
    rung = sd_box(x, ly, 64, 0, 30, 9, 2)
    hm = np.where(~side & (rung < 0), 0.35, hm)
    pts = [(cx, (j + 0.5) * 32) for cx in (14, 114) for j in range(8)]
    hm = np.maximum(hm, 0.6 + rivet_field(x, y, pts, 5) * 0.3)
    col = lerp(col, fill(w, h, '#3a2a40'), (~side & (rung >= 0)).astype(F) * 0.7)
    col = finish(col, hm, 5, 0.6, 2, 652, gloss=0.3)
    a = np.where(side, 0.95, np.where(rung < 0, 0.85, 0.45)) - 0.2 * scratches(w, h, 653, 30, 30)
    out['textures/base_support/support1shiny.tga'] = (col, clamp(a))

    w = h = 256
    col = metal(w, h, 654, '#8a847c', 0.2)
    col = lerp(col, clamp(col * 1.3), scratches(w, h, 655, 150, 40) * 0.4)
    a = 0.1 + 0.75 * noise(w, h, 656, 3, None, 1.3)
    out['textures/gothic_trim/zinc_shiny.tga'] = (finish(col, noise(w, h, 657, 10) * 0.2, 3, 0.3), clamp(a))
    return out


def sag_cable(x, y, x0, x1, ytop, sag, r, phase=0.0):
    t = (x - x0) / (x1 - x0)
    yc = ytop + sag * 4 * t * (1 - t) + np.sin(x / 9 + phase) * 0.4
    d = np.abs(y - yc) - r
    d = np.where((t < 0) | (t > 1), 1e9, d)
    return d.astype(F), yc


def cable_tex(w, h, specs, seed):
    x, y = grid(w, h)
    col = np.zeros((h, w, 3), F)
    a = np.zeros((h, w), F)
    rng = np.random.default_rng(seed)
    for (ytop, sag, r) in specs:
        d, yc = sag_cable(x, y, -2, w + 2, ytop, sag, r, rng.random() * 6)
        prof = np.sqrt(clamp(1 - ((y - yc) / r) ** 2))
        band = (np.cos(x / 3.0) > 0.85) * 0.25                       # wrapping tape
        c = light(fill(w, h, rng.choice(['#8a8070', '#6a5040', '#9a8a7a'])), 0.35 + 0.75 * prof - band)
        sel = d < 0
        col = np.where(sel[..., None], c, col)
        a = np.maximum(a, sel.astype(F))
    # clamps on the cables
    for cx in rng.uniform(20, w - 20, 4):
        d = sd_box(x, y, cx, specs[0][0] + specs[0][1] * 4 * (cx / w) * (1 - cx / w), 4, specs[0][2] + 2, 1)
        col = lerp(col, fill(w, h, '#b0aaa0'), mask(d))
        a = np.maximum(a, cut(d))
    return col, a


@recipe
def wires():
    return {'textures/base_trim/wires01.tga': cable_tex(256, 32, [(10, 6, 3), (14, 8, 2.5)], 660),
            'textures/base_trim/wires02.tga': cable_tex(256, 128, [(14, 70, 5), (8, 86, 4), (20, 60, 3)], 661)}


@recipe
def comp3():
    """Monitor frame; the transparent middle shows the scrolling screen."""
    out = {}
    w = h = 256
    x, y = grid(w, h)
    screen = sd_poly(x, y, [(20, 18), (236, 18), (236, 120), (248, 136), (248, 236), (8, 236), (8, 136), (20, 120)])
    screen = np.maximum(screen, sd_box(x, y, 128, 128, 112, 110, 4))
    frame_h = bevel(-screen, 3) * 0.6 + bevel(sd_box(x, y, 128, 128, 126, 126, 2), 3) * 0.2
    col = worn_plate(w, h, 670, '#4a3a34', dirt=0.4)
    lx, ly = tiled(x, y, 8, 6)
    detail = bevel(sd_box(lx, ly, 0, 0, 2.5, 1.5, 0.5), 1) * 0.2 * (screen > 3)
    col = finish(col, frame_h + detail, 5, 0.6, 2, 670, gloss=0.2)
    inner = light(fill(w, h, '#0c0c10'), 0.6 + 0.8 * glow_blob(x, y, 128, 60, 120, 2))
    col = lerp(col, inner, mask(screen))
    a = 1 - mask(screen) * (1 - 0.1 * glow_blob(x, y, 128, 60, 120, 2))
    out['textures/base_wall/comp3.tga'] = (col, clamp(a))
    out['textures/base_wall/comp3c.tga'] = (col, clamp(a))
    # scrolling content: a scan line, and coloured static
    w2 = 128
    x2, y2 = grid(w2, w2)
    out['textures/base_wall/comp3text.jpg'] = (light(fill(w2, w2, '#c01010'), clamp(1 - np.abs(y2 - 32) / 2.5)), None)
    rng = np.random.default_rng(671)
    st = rng.random((w2, w2, 3)).astype(F) ** 3
    out['textures/base_wall/comp3textb.jpg'] = (st * 0.6, None)
    return out
