"""Light fixtures and their glow layers (the .blend / glow images are drawn
from the same shapes, so lights and glow always line up)."""
from common import *


def black(w, h):
    return np.zeros((h, w, 3), F)


def tube(x, y, cx, y0, y1, r):
    """Vertical fluorescent tube: profile and mask."""
    d = sd_segment(x, y, cx, y0, cx, y1, r)
    prof = np.sqrt(clamp(1 - ((x - cx) / r) ** 2)) * (d < 0)
    return prof.astype(F), d


@recipe
def baslt4_1():
    w, h = 64, 256
    x, y = grid(w, h)
    frame = sd_box(x, y, 32, 128, 30, 126, 3)
    inner = sd_box(x, y, 32, 128, 24, 120, 2)
    hm = bevel(frame, 4) * 0.7 - bevel(inner, 2) * 0.6
    col = worn_plate(w, h, 400, '#8a8a86', dirt=0.4)
    col = lerp(col, fill(w, h, '#141414'), mask(inner))
    glow = np.zeros((h, w), F)
    for cx in (22, 42):
        p, d = tube(x, y, cx, 18, 238, 7)
        cap = mask(np.maximum(d, -(np.abs(y - 128) - 104)))
        col = lerp(col, light(fill(w, h, '#e8e4c4'), 0.55 + 0.5 * p), p > 0)
        col = lerp(col, fill(w, h, '#9a968e'), cap)
        hm = np.where(p > 0, 0.2 + p * 0.5, hm)
        glow = np.maximum(glow, p * (1 - cap))
    col = finish(col, hm, 4, 0.4, 2, 400, gloss=0.3)
    g = blur(glow, 1.5)
    return {'textures/base_light/baslt4_1.jpg': (col, None),
            'textures/base_light/baslt4_1.blend.jpg': (light(fill(w, h, '#fff6b0'), clamp(g * 1.3)), None)}


@recipe
def border11light():
    w, h = 256, 64
    x, y = grid(w, h)
    col = worn_plate(w, h, 410, '#76736c', dirt=0.4)
    hm = np.where(y < 30, 0.6, 0.2)
    hm = hm + bevel(np.abs(y - 15) - 13, 3) * 0.2
    # cable bundle below the lamps
    rng = np.random.default_rng(411)
    for cy in (36, 42, 47, 53, 58):
        r = rng.uniform(2, 3)
        hm = np.where(np.abs(y - cy) < r, 0.25 + np.sqrt(clamp(1 - ((y - cy) / r) ** 2)) * 0.2, hm)
    lx, _ = tiled(x, y, 128, 1, 64, 0)
    lx = lx + 0
    d = sd_segment(lx, y, -32, 15, 32, 15, 7)
    lamp = mask(d)
    p = np.sqrt(clamp(1 - ((y - 15) / 7) ** 2)) * lamp
    col = lerp(col, light(fill(w, h, '#f2e8d8'), 0.6 + 0.5 * p), lamp)
    hm = np.where(lamp > 0.5, 0.6 + p * 0.3, hm)
    col = finish(col, hm, 4, 0.5, 2, 410, gloss=0.25)
    g = blur(p, 2)
    return {'textures/base_light/border11light.jpg': (col, None),
            'textures/base_light/border11light.blend.jpg': (light(fill(w, h, '#ffe0d0'), clamp(g * 1.5)), None)}


@recipe
def jaildr1_3():
    w, h = 32, 256
    x, y = grid(w, h)
    col = worn_plate(w, h, 420, '#2c2a28', dirt=0.4)
    hm = bevel(sd_box(x, y, 16, 128, 14, 200, 2), 2) * 0.5
    _, ly = tiled(x, y, 1, 32)
    d = sd_box(x, ly, 16, 0, 8, 12, 2)
    lamp = mask(d)
    shade_ = 0.6 + 0.4 * smooth(-12, 0, -np.abs(ly))
    col = lerp(col, light(fill(w, h, '#b83a24'), shade_), lamp)
    hm = np.where(lamp > 0.5, 0.5 + bevel(d, 3) * 0.2, hm)
    col = finish(col, hm, 4, 0.4, 2, 420, gloss=0.2)
    return {'textures/base_light/jaildr1_3.jpg': (col, None),
            'textures/base_light/jaildr1_3.blend.jpg': (light(fill(w, h, '#ff4a2a'), blur(lamp, 1.2) * shade_), None)}


@recipe
def light1():
    w = h = 128
    x, y = grid(w, h)
    col = worn_plate(w, h, 430, '#5a4432', rust=0.5, dirt=0.4)
    r = np.sqrt((x - 64) ** 2 + (y - 64) ** 2)
    ring = bevel(np.abs(r - 52) - 9, 4)
    dome = np.sqrt(clamp(1 - (r / 42) ** 2))
    hm = np.maximum(ring * 0.7, dome * 0.9)
    hm = np.maximum(hm, rivet_field(x, y, [(14, 14), (114, 14), (14, 114), (114, 114)], 7) * 0.6)
    col = lerp(col, worn_plate(w, h, 431, '#6a5240', rust=0.3), mask(np.abs(r - 52) - 9))
    glass = light(fill(w, h, '#e6d6d6'), 0.75 + 0.3 * dome)
    col = lerp(col, glass, mask(r - 42))
    col = finish(col, hm, 5, 0.5, 2, 430, gloss=0.5, power=30)
    g = mask(r - 42) * (0.55 + 0.45 * dome)
    return {'textures/base_light/light1.jpg': (col, None),
            'textures/base_light/light1.blend.jpg': (light(fill(w, h, '#ffe2e2'), g), None)}


def strip_light(seed, frame_col, tube_col, dark_inside):
    w, h = 32, 256
    x, y = grid(w, h)
    frame = sd_box(x, y, 16, 128, 15, 127, 2)
    inner = sd_box(x, y, 16, 128, 10, 120, 1)
    hm = bevel(frame, 3) * 0.7 - bevel(inner, 2) * 0.5
    col = worn_plate(w, h, seed, frame_col, dirt=0.3)
    col = lerp(col, fill(w, h, '#0a0a0a' if dark_inside else '#8a8780'), mask(inner))
    p, d = tube(x, y, 16, 12, 244, 6)
    col = lerp(col, light(fill(w, h, tube_col), 0.5 + 0.6 * p), p > 0)
    hm = np.where(p > 0, 0.2 + p * 0.4, hm)
    return finish(col, hm, 4, 0.4, 2, seed, gloss=0.3), p


@recipe
def light5():
    a, _ = strip_light(440, '#9a9890', '#e8e2c8', False)
    b, p = strip_light(441, '#6a6862', '#f0e6c0', True)
    return {'textures/base_light/light5.jpg': (a, None),
            'textures/base_light/xlight5.jpg': (b, None),
            'textures/base_light/xlight5.blend.jpg': (light(fill(32, 256, '#fff0c0'), clamp(blur(p, 1.5) * 1.3)), None)}


def reflector(w, h, cx, cy, R, seed):
    """Lamp glass with reflector facets."""
    x, y = grid(w, h)
    dx, dy = x - cx, y - cy
    r = np.sqrt(dx * dx + dy * dy)
    a = np.arctan2(dy, dx)
    rings = 0.5 + 0.5 * np.cos(r / R * 2 * np.pi * 5)
    facets = rings * 0.6 + 0.4 * (np.cos(a * 32) > 0.6) * (r > R * 0.35) + 0.2 * noise(w, h, seed, 20, None, 1.0)
    hot = glow_blob(x, y, cx - R * 0.15, cy - R * 0.2, R * 0.9, 1.5)
    t = clamp(0.25 + 0.35 * facets + 0.6 * hot)
    col = gradient_map(t, [(0, '#2a2420'), (0.5, '#8a7a70'), (0.8, '#e0d8d0'), (1, '#ffffff')])
    return col, mask(r - R), r


@recipe
def proto_light():
    w = h = 128
    x, y = grid(w, h)
    col = worn_plate(w, h, 450, '#5c5e60', dirt=0.4)
    r = np.sqrt((x - 64) ** 2 + (y - 64) ** 2)
    hm = bevel(sd_box(x, y, 64, 64, 62, 62, 14), 5) * 0.4
    hm = np.maximum(hm, bevel(np.abs(r - 52) - 8, 4) * 0.8)
    for sx, sy in ((1, 1), (-1, 1), (1, -1), (-1, -1)):       # triangular corner studs
        cx, cy = 64 + sx * 44, 64 + sy * 44
        d = sd_poly(x, y, [(cx, cy - sy * 14), (cx - sx * 14, cy), (cx + sx * 6, cy + sy * 6)])
        hm = np.maximum(hm, bevel(d, 4) * 0.9)
    glass, m, _ = reflector(w, h, 64, 64, 44, 451)
    col = finish(col, hm, 5, 0.5, 2, 450, gloss=0.3)
    col = lerp(col, glass, m)
    lm = light(glass, m)
    return {'textures/base_light/proto_light.jpg': (col, None),
            'textures/base_light/proto_lightmap.jpg': (lm, None)}


@recipe
def proto_lightred():
    w = h = 128
    x, y = grid(w, h)
    frame = sd_box(x, y, 64, 64, 62, 62, 3)
    inner = sd_box(x, y, 64, 64, 54, 54, 2)
    rib = 0.5 + 0.5 * np.cos(y / 128 * 2 * np.pi * 9)
    hm = bevel(frame, 4) * 0.6 - mask(inner) * 0.3 + mask(inner) * rib * 0.3
    hot = glow_blob(x, y, 64, 72, 70, 1.2)
    base = gradient_map(clamp(0.3 + 0.5 * rib * (0.5 + hot)), [(0, '#3a0602'), (0.5, '#b01a08'), (0.8, '#ff4a10'), (1, '#ffb040')])
    col = lerp(worn_plate(w, h, 460, '#7a3a2a', dirt=0.3), base, mask(inner))
    return {'textures/base_light/proto_lightred.jpg': (finish(col, hm, 4, 0.4, 2, 460, gloss=0.3), None)}


@recipe
def runway():
    w, h = 128, 512
    x, y = grid(w, h)
    col = worn_plate(w, h, 470, '#6c6a64', dirt=0.4, rust=0.15)
    hm = bevel(np.abs(x - 64) - 60, 4) * 0.4 + noise(w, h, 471, 20) * 0.06
    lamps = np.zeros((h, w), F)
    for cy in (100, 380):
        box = sd_box(x, y, 64, cy, 30, 26, 4)
        lens = sd_box(x, y, 64, cy, 22, 18, 2)
        hm = np.maximum(hm, bevel(box, 4) * 0.7)
        rib = 0.6 + 0.4 * np.cos((y - cy) / 4 * np.pi)
        lamps = np.maximum(lamps, mask(lens) * rib)
        hm = np.where(lens < 0, 0.5 + rib * 0.05, hm)
    amber = gradient_map(lamps, [(0, '#3a2008'), (1, '#ffa020')])
    col = finish(col, hm, 5, 0.6, 2, 470, gloss=0.2)
    col = lerp(col, amber, lamps > 0)
    lampg = light(fill(w, h, '#ff9a20'), lamps)
    # running light: one soft bar, the shader scrolls it along the strip
    gl = light(fill(w, h, '#ffd020'), clamp(1 - np.abs(y - 60) / 9) * smooth(30, 20, np.abs(x - 64)))
    return {'textures/base_light/runway.jpg': (col, None),
            'textures/base_light/runwayb.jpg': (lampg, None),
            'textures/base_light/runway_glow.jpg': (gl, None)}


@recipe
def wsupprt1_12():
    w, h = 64, 128
    x, y = grid(w, h)
    wav = 3 * np.sin(y / 16 * np.pi)
    side = sd_box(x, y, 32, 64, 26 + wav, 70, 0)
    col = worn_plate(w, h, 480, '#4a3f30', dirt=0.4, rust=0.2)
    hm = bevel(side, 3) * 0.5
    pts = [(cx, (j + 0.5) * 16) for cx in (6, 58) for j in range(8)]
    hm = np.maximum(hm, rivet_field(x, y, pts, 3) * 0.5)
    _, ly = tiled(x, y, 1, 16)
    slot = sd_box(x, ly, 32, 0, 17, 3.5, 3.5)
    p = mask(slot) * (0.7 + 0.3 * np.sqrt(clamp(1 - (ly / 3.5) ** 2)))
    col = finish(col, hm, 4, 0.5, 2, 480)
    col = lerp(col, light(fill(w, h, '#fff2d8'), p), mask(slot))
    return {'textures/base_light/wsupprt1_12.jpg': (col, None)}


# ---------------------------------------------------------------- shiny panels with glow (alpha = how much chrome shows)
@recipe
def bluemetalsupport2clight():
    w = h = 256
    x, y = grid(w, h)
    outer = sd_box(x, y, 128, 128, 126, 126, 20)
    inner = sd_box(x, y, 128, 128, 104, 112, 14)
    hm = bevel(outer, 8) * 0.6 - bevel(inner, 6) * 0.25
    win1 = sd_poly(x, y, [(64, 52), (192, 52), (174, 112), (82, 112)])
    win2 = sd_poly(x, y, [(82, 144), (174, 144), (192, 204), (64, 204)])
    win = np.minimum(win1, win2)
    hm = hm - bevel(win + 4, 3) * 0.2
    col = worn_plate(w, h, 490, '#8a8a8e', dirt=0.3, scratch=0.4)
    col = lerp(col, rust_colour(w, h, 491), smooth(200, 250, y) * rust_layer(w, h, 492, 0.6, 5))
    col = finish(col, hm, 5, 0.5, 2, 490, gloss=0.3)
    glass = light(fill(w, h, '#3c8890'), 0.7 + 0.5 * noise(w, h, 493, 6, 60))
    glass = lerp(glass, fill(w, h, '#a0e0e0'), scratches(w, h, 494, 40, 30) * 0.5)
    col = lerp(col, glass, mask(win))
    # alpha: opaque glass, the metal lets some chrome through, worn spots more
    a = 0.8 - 0.15 * smooth(0.6, 0.9, noise(w, h, 495, 4, 60)) - 0.25 * scratches(w, h, 496, 50, 40)
    a = np.where(win < 0, 1.0, a)
    a = np.where(outer > 0, 0.0, a)
    glowc = gradient_map(clamp(0.5 + 0.5 * glow_blob(x, y, 128, 128, 120, 1)), [(0, '#606a70'), (1, '#f0ffff')])
    return {'textures/base_wall/bluemetalsupport2clight.tga': (col, clamp(a)),
            'textures/base_wall/bluemetalsupport2clight.glow.tga': (light(glowc, mask(win)), clamp(a))}


@recipe
def shinymetaldoor():
    w = h = 512
    x, y = grid(w, h)
    col = worn_plate(w, h, 500, '#2a2b2d', dirt=0.4, scratch=0.4)
    seam = smooth(2, 0, np.abs(x - 256))
    hm = noise(w, h, 501, 20) * 0.05 - seam * 0.5
    hm = hm + bevel(sd_box(x, y, 256, 256, 250, 250, 6), 6) * 0.3
    r = np.sqrt((x - 256) ** 2 + (y - 262) ** 2)
    ring = np.abs(r - 186) - 9
    hm = np.maximum(hm, bevel(ring, 5) * 0.6)
    col = lerp(col, fill(w, h, '#6a0c08'), mask(ring))
    col = lerp(col, light(fill(w, h, '#1c1c1e'), 0.8 + 0.3 * noise(w, h, 502, 40)), mask(r - 177))
    col = finish(col, hm, 5, 0.5, 2, 500, gloss=0.3)
    a = 0.55 + 0.2 * noise(w, h, 503, 4, 60) - 0.25 * scratches(w, h, 504, 80, 60)
    a = np.where(r < 195, 1.0, a)
    glow = light(fill(w, h, '#c01808'), clamp(1 - np.abs(r - 186) / 9) ** 1.5)
    return {'textures/base_door/shinymetaldoor_outside3a2.tga': (col, clamp(a)),
            'textures/base_door/shinymetaldoor_outside3glow.tga': (glow, clamp(a))}
