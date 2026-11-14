"""Effect textures: powerup and item reflection maps, fire, lightning, rail
and sun glows, the level-shot detail layer."""
from common import *


def flare(x, y, cx, cy, r, rays=6, ray_len=2.5, phase=0.0):
    """Light flare: hot core, soft halo, thin rays."""
    dx, dy = x - cx, y - cy
    d = np.sqrt(dx * dx + dy * dy)
    a = np.arctan2(dy, dx)
    core = np.exp(-(d / (r * 0.35)) ** 2)
    halo = np.exp(-(d / r) ** 2) * 0.45
    ray = (np.abs(np.cos((a + phase) * rays / 2)) ** 40) * np.exp(-d / (r * ray_len)) * 0.6
    return clamp(core + halo + ray)


def web(w, h, seed, n, width, warp_amt=0.08):
    """Thin branching lines (cell edges, warped): lightning / energy web."""
    f1, f2, _ = worley(w, h, n, seed)
    e = clamp(1 - (f2 - f1) / width) ** 2
    dx = (noise(w, h, seed + 1, 2, 16) - 0.5) * w * warp_amt
    dy = (noise(w, h, seed + 2, 2, 16) - 0.5) * h * warp_amt
    return warp(e, dx, dy)


def swirl_metal(w, h, seed, stops):
    """Liquid-metal swirls (domain-warped noise)."""
    n = noise(w, h, seed, 1, 12, 2.4)
    dx = (noise(w, h, seed + 1, 1, 6, 2.6) - 0.5) * w * 0.6
    dy = (noise(w, h, seed + 2, 1, 6, 2.6) - 0.5) * h * 0.6
    n = warp(n, dx, dy)
    bands = 0.5 + 0.5 * np.sin(n * np.pi * 3)
    return gradient_map(clamp(n * 0.55 + bands * 0.45), stops)


def room(w, h, seed, tint, lamps, floor=0.0):
    """Abstract interior for reflections: dark ends, light bands, lamps."""
    x, y = grid(w, h)
    t = clamp(1 - np.abs(y - h / 2) / (h / 2))
    base = gradient_map(t, [(0, '#101010'), (0.6, '#404040'), (1, '#808080')])
    bars = (np.cos((x - w / 2) / (w / 2) * np.pi * 3) > 0.85) * smooth(h * 0.25, h * 0.05, np.abs(y - h / 2))
    out = base + bars[..., None] * 0.3
    if floor:
        out = out + (smooth(h * 0.6, h, y) * floor)[..., None]
    for lx, ly, r in lamps:
        out = out + flare(x, y, lx * w, ly * h, r * w)[..., None]
    out = out * (0.8 + 0.3 * noise(w, h, seed, 4, 40))[..., None]
    return blur(clamp(out * rgb(tint) / max(rgb(tint))), w / 128)


def flames(w, h, seed, stops, alpha=False):
    """Rising flame tongues, tileable (they scroll in the shaders)."""
    n = noise(w, h, seed, 1, 60, 2.2, aniso=(1, 4))
    n2 = noise(w, h, seed + 1, 3, None, 1.8, aniso=(1, 3))
    t = clamp(n * 0.65 + n2 * 0.35)
    t = clamp((t - 0.25) * 1.6)
    col = gradient_map(t, stops)
    return (col, clamp(t * 0.65)) if alpha else (col, None)


@recipe
def rail_sun():
    out = {}
    w = h = 64
    x, y = grid(w, h)
    line = np.exp(-((y - 32) / 5.5) ** 2)
    sparks = (noise(w, h, 1000, 8, None, 1.0) > 0.8) * np.exp(-((y - 32) / 9) ** 2) * 0.4
    out['gfx/misc/railcorethin_mono.jpg'] = (np.stack([clamp(line * 0.55 + sparks)] * 3, -1), None)
    r = np.sqrt((x - 32) ** 2 + (y - 32) ** 2)
    ring = np.exp(-((r - 27) / 2.6) ** 2)
    inner = smooth(25, 22, r) * (0.18 + 0.2 * noise(w, h, 1001, 3, 20))
    out['gfx/misc/raildisc_mono2.jpg'] = (np.stack([clamp(ring + inner)] * 3, -1), None)
    g = clamp(np.exp(-(r / 9) ** 2) + np.exp(-(r / 20) ** 2) * 0.35 + flare(x, y, 32, 32, 14, 8, 1.4, 0.2) * 0.4) * smooth(32, 26, r)
    out['gfx/misc/sun.tga'] = (np.stack([g] * 3, -1), None)
    return out


@recipe
def small_fx():
    out = {}
    w = h = 64
    x, y = grid(w, h)
    r = np.sqrt((x - 32) ** 2 + (y - 32) ** 2)
    fb = noise(w, h, 1010, 2, 20, 1.6) * 0.7 + noise(w, h, 1011, 6, None, 1.4) * 0.3
    fb = clamp(fb * smooth(28, 6, r) * 1.6)
    out['models/mapobjects/barrel/barrel2fx.jpg'] = (gradient_map(fb, [(0, '#000000'), (0.35, '#601800'), (0.7, '#e08020'), (1, '#fff0b0')]), None)
    # haste: scrolling glyph strokes
    rng = np.random.default_rng(1012)
    d = np.full((h, w), 1e9, F)
    for _ in range(9):
        x0, y0 = rng.random(2) * 64
        pts = [(x0, y0)]
        for _ in range(rng.integers(1, 4)):
            a = rng.integers(0, 8) * np.pi / 4
            L = rng.uniform(6, 16)
            pts.append((pts[-1][0] + np.cos(a) * L, pts[-1][1] + np.sin(a) * L))
        for (ax, ay), (bx, by) in zip(pts[:-1], pts[1:]):
            for ox in (-64, 0, 64):
                for oy in (-64, 0, 64):
                    d = np.minimum(d, sd_segment(x, y, ax + ox, ay + oy, bx + ox, by + oy, 1.8))
    s = clamp(np.exp(-np.maximum(d, 0) / 2.2))
    out['models/powerups/armor/energy_yel3.jpg'] = (light(fill(w, h, '#ffd030'), blur(s, 0.8)), None)
    # dark reflection for the blue flag shell (alpha unused)
    env = gradient_map(noise(w, h, 1013, 1, 10, 2.4), [(0, '#100000'), (1, '#a00808')])
    out['textures/effects/blueflagmap.tga'] = (env, np.zeros((h, w), F))
    return out


@recipe
def electric():
    out = {}
    w = h = 256
    x, y = grid(w, h)
    rng = np.random.default_rng(1020)
    bolt = np.zeros((h, w), F)
    for _ in range(9):
        a = rng.random() * 2 * np.pi
        pts = [(128.0, 128.0)]
        L = rng.uniform(45, 75)
        for k in range(6):
            a += rng.uniform(-0.8, 0.8)
            pts.append((pts[-1][0] + np.cos(a) * L / 6, pts[-1][1] + np.sin(a) * L / 6))
        dd = np.full((h, w), 1e9, F)
        for (ax, ay), (bx, by) in zip(pts[:-1], pts[1:]):
            dd = np.minimum(dd, sd_segment(x, y, ax, ay, bx, by, 0))
        bolt = np.maximum(bolt, np.exp(-dd / 1.4))
    glow = blur(bolt, 5) * 2.5 + np.exp(-(((x - 128) ** 2 + (y - 128) ** 2) / 300.0)) * 0.6
    t = clamp(bolt * 0.9 + glow * 0.5)
    out['models/weaphits/electric.jpg'] = (gradient_map(t, [(0, '#000000'), (0.4, '#0a3080'), (0.75, '#40a0ff'), (1, '#e8f6ff')]), None)
    # flag / megahealth shell: blue electric web with a hot spot
    w = h = 128
    x, y = grid(w, h)
    e = web(w, h, 1021, 14, 3.0)
    t = clamp(e * 0.8 + flare(x, y, 42, 40, 18) * 0.8 + noise(w, h, 1022, 2, 10) * 0.25)
    out['textures/sfx/proto_zzztblu3.jpg'] = (gradient_map(t, [(0, '#000010'), (0.35, '#0010a0'), (0.7, '#2070ff'), (1, '#e0f0ff')]), None)
    # bolts: bright cracks (scrolled over a head skin)
    w = h = 256
    e = web(w, h, 1023, 10, 3.5, 0.05) * smooth(0.35, 0.6, noise(w, h, 1024, 2, 10))
    t = clamp(e + blur(e, 4) * 0.8)
    out['textures/sfx/bolts.jpg'] = (gradient_map(t, [(0, '#000000'), (0.4, '#4a2a10'), (0.75, '#e0b080'), (1, '#ffffff')]), None)
    return out


@recipe
def powerup_envs():
    out = {}
    for name, size, tint, seed in (('envmapblue', 64, '#3060ff', 1030), ('envmapred', 64, '#ff2020', 1031)):
        w = h = size
        x, y = grid(w, h)
        base = gradient_map(noise(w, h, seed, 1, 6, 2.6), [(0, '#000000'), (1, '#808080')])
        base = light(base, rgb(tint) * 1.2)
        f = flare(x, y, w * 0.78, h * 0.3, w * 0.16, 6) + flare(x, y, w * 0.3, h * 0.66, w * 0.12, 4, 2, 0.4) * 0.8
        out[f'textures/effects/{name}.jpg'] = (clamp(base + f[..., None]), None)
    out['textures/effects/envmapgold.jpg'] = (swirl_metal(128, 128, 1032, [(0, '#3a2000'), (0.4, '#8a6020'), (0.7, '#d0a050'), (1, '#fff0c0')]), None)
    out['textures/effects/envmapyel.jpg'] = (swirl_metal(128, 128, 1033, [(0, '#504000'), (0.4, '#c0a000'), (0.7, '#ffe020'), (1, '#ffffc0')]), None)
    for name, stops, seed in (('envmapligh', [(0, '#000000'), (0.4, '#3a1800'), (0.75, '#d08020'), (1, '#fff0c0')], 1034),
                              ('envmaprail', [(0, '#000000'), (0.4, '#083008'), (0.75, '#30d030'), (1, '#e0ffe0')], 1035)):
        w = h = 128
        x, y = grid(w, h)
        e = web(w, h, seed, 12, 2.5, 0.12)
        t = clamp(e + flare(x, y, 80, 52, 12, 6) + flare(x, y, 40, 90, 8, 4) * 0.7 + noise(w, h, seed + 5, 2, 12) * 0.2)
        out[f'textures/effects/{name}.jpg'] = (gradient_map(t, stops), None)
    out['textures/effects/envmapmach.jpg'] = (room(128, 128, 1036, '#ffe060', [(0.5, 0.75, 0.1), (0.3, 0.15, 0.05), (0.7, 0.15, 0.05)], 0.4), None)
    out['textures/effects/tinfx2b.jpg'] = (clamp(room(128, 128, 1037, '#d0d0d0', [(0.5, 0.2, 0.08)]) +
                                                 light(fill(128, 128, '#ff4030'), flare(*grid(128, 128), 76, 92, 14)) * 1.0), None)
    # quad: magenta energy web
    w = h = 128
    x, y = grid(w, h)
    e = web(w, h, 1038, 22, 4.0, 0.1)
    t = clamp(e * 1.1 + flare(x, y, 50, 36, 14) * 0.7)
    out['textures/effects/quadmap2.jpg'] = (gradient_map(t, [(0, '#000000'), (0.4, '#500030'), (0.75, '#ff20c0'), (1, '#fff0ff')]), None)
    # regen: red veins from a hot centre
    w = h = 64
    x, y = grid(w, h)
    r = np.sqrt((x - 32) ** 2 + (y - 32) ** 2)
    a = np.arctan2(y - 32, x - 32)
    veins = (np.abs(np.sin(a * 5 + noise(w, h, 1039, 2, 8) * 4)) < 0.12) * smooth(30, 4, r)
    t = clamp(blur(veins.astype(F), 0.8) * 0.8 + np.exp(-(r / 7) ** 2) + noise(w, h, 1040, 2, 8) * 0.2)
    out['textures/effects/regenmap2.jpg'] = (gradient_map(t, [(0, '#100000'), (0.5, '#a01000'), (0.85, '#ff5020'), (1, '#ffd0a0')]), None)
    # chrome gib reflections: soft light grid
    w = h = 128
    x, y = grid(w, h)
    g = np.zeros((h, w), F)
    for cx in (0, 64, 128):
        for cy in (0, 64, 128):
            g = np.maximum(g, flare(x, y, cx, cy, 20 if (cx, cy) != (64, 64) else 26, 8, 1.2))
    out['textures/sfx/specular.jpg'] = (light(fill(w, h, '#fff0e8'), clamp(g * 0.95)), None)
    return out


@recipe
def fire():
    out = {}
    out['textures/sfx/fire_ctfblue.jpg'] = flames(256, 256, 1050, [(0, '#000020'), (0.4, '#0820a0'), (0.75, '#3060ff'), (1, '#c0b0ff')])
    out['textures/sfx/fire_ctfred.jpg'] = flames(256, 256, 1051, [(0, '#200000'), (0.4, '#a01000'), (0.75, '#ff4010'), (1, '#ffd0a0')])
    out['textures/sfx/firegorre.tga'] = flames(256, 256, 1052, [(0, '#401000'), (0.5, '#b04000'), (1, '#ffb030')], alpha=True)
    out['textures/sfx/firewalla.tga'] = flames(256, 256, 1053, [(0, '#301000'), (0.5, '#a03800'), (1, '#ffa020')], alpha=True)
    n = noise(256, 256, 1054, 1, 30, 2.2)
    out['textures/sfx/portalfog.jpg'] = (gradient_map(n, [(0, '#707070'), (1, '#e8e8e8')]), None)
    return out


@recipe
def levelshot_detail():
    """Laid over level shots with a 2x modulate: 0.5 grey leaves them unchanged."""
    out = {}
    w = h = 512
    g = 0.5 + (noise(w, h, 1060, 40, None, 1.0) - 0.5) * 0.12 + (noise(w, h, 1061, 3, 30) - 0.5) * 0.06
    out['textures/sfx/detail.jpg'] = (np.stack([clamp(g)] * 3, -1), None)
    w, h = 1920, 1080
    x, y = grid(w, h)
    d = np.sqrt(((x - w / 2) / (w * 0.62)) ** 2 + ((y - h / 2) / (h * 0.62)) ** 2)
    v = 0.36 + 0.16 * smooth(1.0, 0.0, d)
    rng = np.random.default_rng(1062)
    v = v + (rng.random((h, w)).astype(F) - 0.5) * 0.02
    out['textures/sfx/detail.tga'] = (np.stack([clamp(v)] * 3, -1), None)
    return out
