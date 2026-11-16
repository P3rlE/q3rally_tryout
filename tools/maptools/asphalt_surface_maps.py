"""Q3Rally: hand-tuned normal (_n) and specular (_s) maps for asphalt
textures (textures/q3r_road/asph02, asph02linien, asph02parking).

The diffuse has small baked highlights on every grain; turned into relief
pixel by pixel (as R_GenerateSurfaceMaps does) that reads as noise in the
sun. Here the relief comes from grain clusters a few texels across, plus a
faint wear undulation that catches grazing light. White road paint is
found by colour and becomes a smooth, very slightly raised, glossier layer.
No parallax: asphalt relief is far below what parallax can show.

Conventions match renderergl2: normal x = left minus right, y = top minus
bottom of the height; _s RGB = linear reflectance, A = gloss. Wrap-around
everywhere so the maps tile like the diffuse.

usage: python asphalt_surface_maps.py <out dir> <texture.jpg> [...] [--preview <dir>]
"""
import os
import sys
import numpy as np
import cv2
from PIL import Image

args = sys.argv[1:]
prev_dir = None
if "--preview" in args:
    i = args.index("--preview")
    prev_dir = args[i + 1]
    del args[i:i + 2]
out_dir, sources = args[0], args[1:]

PAD = 48
DEPTH = 1.2          # relief of the grain clusters in texels
WEAR = 0.35          # share of the slow wear undulation in the height
PAINT = 0.10         # paint layer above the asphalt top, in height units


def tobyte(v):
    return np.clip(np.round(v * 255.0), 0, 255).astype(np.uint8)


def process(src):
    rgb = np.asarray(Image.open(src).convert("RGB")).astype(np.float32) / 255.0
    H, W, _ = rgb.shape

    def wpad(a):
        return np.pad(a, ((PAD, PAD), (PAD, PAD)), mode="wrap")

    def crop(a):
        return a[PAD:PAD + H, PAD:PAD + W]

    def blur(a, s):
        return crop(cv2.GaussianBlur(wpad(a), (0, 0), s))

    lum_s = rgb @ np.array([0.299, 0.587, 0.114], np.float32)
    sat = rgb.max(2) - rgb.min(2)

    # road paint: bright and colourless, in solid areas (not single grains)
    paint = ((lum_s > 0.80) & (sat < 0.12)).astype(np.uint8)
    k3 = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
    p = wpad(paint)
    p = cv2.morphologyEx(p, cv2.MORPH_OPEN, k3, iterations=1)
    n, lab, stats, _ = cv2.connectedComponentsWithStats(p, 8)
    keep = stats[:, cv2.CC_STAT_AREA] >= 60
    keep[0] = False
    p = keep[lab].astype(np.uint8)
    p = cv2.morphologyEx(p, cv2.MORPH_CLOSE, k3, iterations=2)
    paint = crop(p).astype(np.float32)
    paint_soft = blur(paint, 1.0)

    # asphalt grain: fill the paint with the surrounding asphalt level so
    # the paint edge doesn't become a cliff in the band-pass
    lum = lum_s ** 2.2
    fill = blur(lum * (1 - paint), 6.0) / np.maximum(blur(1 - paint, 6.0), 1e-3)
    lum_a = lum * (1 - paint_soft) + fill * paint_soft

    clusters = blur(lum_a, 1.1) - blur(lum_a, 4.0)
    clusters /= clusters.std() + 1e-6
    wear = blur(lum_a, 10.0) - blur(lum_a, 40.0)
    wear /= wear.std() + 1e-6

    height = 0.5 + 0.16 * clusters + 0.16 * WEAR * wear
    top = np.percentile(height, 97)
    height = height * (1 - paint_soft) + (top + PAINT + 0.02 * wear) * paint_soft
    height = blur(height, 0.6)

    # normal (wrap), same signs as R_GenerateSurfaceMaps
    hp = wpad(height) * (DEPTH / 0.32)          # 0.32 ~ the +-1 sigma height span
    dx = crop((np.roll(hp, 1, 1) - np.roll(hp, -1, 1)) * 0.5)
    dy = crop((np.roll(hp, 1, 0) - np.roll(hp, -1, 0)) * 0.5)
    nl = np.sqrt(dx * dx + dy * dy + 1.0)
    nx, ny, nz = dx / nl, dy / nl, 1.0 / nl

    # specular: polished grain tops a bit glossier, voids dull, paint smooth
    tops = np.clip(clusters * 0.5, -1, 1)
    gloss = 0.30 + 0.12 * tops + 0.04 * wear
    gloss = gloss * (1 - paint_soft) + 0.50 * paint_soft
    gloss = np.clip(gloss, 0.12, 0.6)
    refl = 0.035 * (1 - paint_soft) + 0.05 * paint_soft

    base = os.path.splitext(os.path.basename(src))[0]
    normal = np.dstack([tobyte(nx * 0.5 + 0.5), tobyte(ny * 0.5 + 0.5), tobyte(nz * 0.5 + 0.5)])
    Image.fromarray(normal, "RGB").save(f"{out_dir}/{base}_n.png", optimize=True)
    spec = np.dstack([tobyte(refl)] * 3 + [tobyte(gloss)])
    Image.fromarray(spec, "RGBA").save(f"{out_dir}/{base}_s.png", optimize=True)
    print(f"{base}: paint {paint.mean() * 100:.1f} %")

    if prev_dir:
        # lit comparison with what R_GenerateSurfaceMaps makes (SURF_ASPHALT bump 0.6)
        def box(a, r):
            k = 2 * r + 1
            return crop(cv2.blur(wpad(a), (k, k)))
        l2 = ((rgb[..., 0] * 0.299 + rgb[..., 1] * 0.587 + rgb[..., 2] * 0.114)) ** 2
        l2 = l2 - box(l2, max(2, W // 24))
        fine, coarse = box(l2, 1), box(l2, max(2, W // 128))
        sc = 6.0 * W / 256.0
        gdx = (np.roll(fine, 1, 1) - np.roll(fine, -1, 1)) * 0.5 + (np.roll(coarse, 3, 1) - np.roll(coarse, -3, 1)) * 0.25
        gdy = (np.roll(fine, 1, 0) - np.roll(fine, -1, 0)) * 0.5 + (np.roll(coarse, 3, 0) - np.roll(coarse, -3, 0)) * 0.25
        gx, gy = gdx * sc * 0.6, gdy * sc * 0.6
        gl = np.sqrt(gx * gx + gy * gy + 1)
        gen = (gx / gl, gy / gl, 1 / gl)

        def lit(nn, L):
            L = np.array(L, np.float32); L /= np.linalg.norm(L)
            ndl = np.clip(nn[0] * L[0] + nn[1] * L[1] + nn[2] * L[2], 0, 1)
            return np.clip(rgb * (0.25 + 0.95 * ndl[..., None]), 0, 1)
        L = (-1, -0.6, 0.45)       # low sun
        row = np.hstack([lit(gen, L)[:256, :256], lit((nx, ny, nz), L)[:256, :256]])
        Image.fromarray(tobyte(row)).resize((1024, 512), Image.NEAREST).save(f"{prev_dir}/prev_{base}.png")
        Image.fromarray(tobyte(paint)).save(f"{prev_dir}/prev_{base}_paint.png")


for s in sources:
    process(s)
