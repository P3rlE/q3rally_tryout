"""Q3Rally: hand-tuned normal+height (_nh) and specular (_s) maps for
textures/q3r_road/cobbles01.jpg.

Conventions match the renderergl2 auto loader and R_GenerateSurfaceMaps:
  _nh: RGB = tangent-space normal (x = left minus right, y = top minus
       bottom of the height), A = height (1 = stone top, 0 = joint);
       the shader samples depth as 1 - A.
  _s : RGB = specular reflectance (linear), A = gloss.
Everything is computed with wrap-around so the maps tile like the diffuse.

usage: python cobble_surface_maps.py <cobbles01.jpg> <out dir> [preview dir]
"""
import sys
import numpy as np
import cv2
from PIL import Image

src, out_dir = sys.argv[1], sys.argv[2]
prev_dir = sys.argv[3] if len(sys.argv) > 3 else None
PAD = 48

rgb = np.asarray(Image.open(src).convert("RGB")).astype(np.float32) / 255.0
H, W, _ = rgb.shape

def wpad(a):
    return np.pad(a, ((PAD, PAD), (PAD, PAD)) + ((0, 0),) * (a.ndim - 2), mode="wrap")

def crop(a):
    return a[PAD:PAD + H, PAD:PAD + W]

def blur(a, s):
    return crop(cv2.GaussianBlur(wpad(a), (0, 0), s))

R, G, B = rgb[..., 0], rgb[..., 1], rgb[..., 2]
lum = 0.299 * R + 0.587 * G + 0.114 * B

# stone = grey/blue granite, joint = brown sand: blue minus red separates them
# plus local darkness: narrow joints in shadow are dark grey, not brown
def zlocal(a, s_small, s_big):
    a = blur(a, s_small)
    a = a - blur(a, s_big)
    return a / (a.std() + 1e-6)
feat = zlocal(B - R, 1.2, 12.0) + 0.9 * zlocal(lum, 1.0, 12.0)
t = np.percentile(feat, 24)                    # joints cover roughly a quarter of the area
m = wpad((feat > t).astype(np.uint8))
k3 = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (3, 3))
m = cv2.morphologyEx(m, cv2.MORPH_OPEN, k3, iterations=2)
m = cv2.medianBlur(m * 255, 5) // 255
# fill dark spots inside stones (holes not connected to the joint network)
n, lab, stats, _ = cv2.connectedComponentsWithStats(1 - m, 4)
holes = stats[:, cv2.CC_STAT_AREA] < 400
m[(m == 0) & holes[lab]] = 1

# split stones that touch: watershed on the distance to the joints
dist0 = cv2.distanceTransform(m, cv2.DIST_L2, 5)
peaks = (dist0 >= 5.5).astype(np.uint8)
peaks = cv2.erode(peaks, k3)
n, markers = cv2.connectedComponents(peaks, 8)
markers = markers.astype(np.int32) + 1
markers[(m == 0)] = 0                          # unknown inside stones is filled
markers[(m == 0) & (dist0 == 0)] = 1           # joints = background label 1
markers[(m == 1) & (peaks == 0)] = 0
relief = cv2.cvtColor(np.clip(255 - dist0 * 12, 0, 255).astype(np.uint8), cv2.COLOR_GRAY2BGR)
ws = cv2.watershed(relief, markers)
ridge = cv2.dilate((ws == -1).astype(np.uint8), k3)   # watershed cuts, ~3 texels wide
m = ((ws > 1) & (ridge == 0)).astype(np.uint8)
# drop tiny islands (sand grains that read as "stone")
n, lab, stats, _ = cv2.connectedComponentsWithStats(m, 8)
small = stats[:, cv2.CC_STAT_AREA] < 150
small[0] = False
m[small[lab]] = 0
mask_p = m

# rounded stone tops: distance to the joint, eased
dist = cv2.distanceTransform(mask_p, cv2.DIST_L2, 5)
EDGE = 7.0                                     # texels from joint to flat top
d = np.clip(dist / EDGE, 0.0, 1.0)
dome = 1.0 - (1.0 - d) ** 2
dome = crop(dome)

# surface detail: granite grain on stones, sand grain in joints
hp = lum - blur(lum, 3.0)
fine = blur(hp, 0.7)
stone = crop(mask_p).astype(np.float32)
stone_soft = blur(stone, 1.2)

height = 0.10 + 0.82 * dome + fine * (1.2 * stone_soft + 0.6 * (1 - stone_soft))
height = blur(height, 0.8)
height = (height - height.min()) / (height.max() - height.min())

# normal from the height (wrap), same signs as R_GenerateSurfaceMaps
DEPTH = 5.0                                    # relief in texels at full height
# parallax depth in the shader is r_baseParallax (0.05) of the texture width
# for alpha 0..1; squeeze the alpha range so the parallax relief matches the
# normal map relief (DEPTH texels) instead of a twentieth of the texture
BASE_PARALLAX = 0.05
alpha_range = min(1.0, DEPTH / (BASE_PARALLAX * max(W, H)))
hp_ = wpad(height) * DEPTH
dx = (np.roll(hp_, 1, 1) - np.roll(hp_, -1, 1)) * 0.5
dy = (np.roll(hp_, 1, 0) - np.roll(hp_, -1, 0)) * 0.5
dx, dy = crop(dx), crop(dy)
nz = np.ones_like(dx)
nl = np.sqrt(dx * dx + dy * dy + nz * nz)
nx, ny, nz = dx / nl, dy / nl, nz / nl

def tobyte(v):
    return np.clip(np.round(v * 255.0), 0, 255).astype(np.uint8)

nh = np.dstack([tobyte(nx * 0.5 + 0.5), tobyte(ny * 0.5 + 0.5), tobyte(nz * 0.5 + 0.5), tobyte(1.0 - alpha_range * (1.0 - height))])
Image.fromarray(nh, "RGBA").save(f"{out_dir}/cobbles01_nh.png", optimize=True)

# specular: worn granite tops a bit glossier, sand joints dull
gloss = 0.16 + 0.22 * stone_soft * dome + np.clip(fine, 0, None) * 0.6 * stone_soft
gloss = np.clip(gloss, 0.12, 0.5)
refl = 0.02 + 0.012 * stone_soft
spec = np.dstack([tobyte(refl)] * 3 + [tobyte(gloss)])
Image.fromarray(spec, "RGBA").save(f"{out_dir}/cobbles01_s.png", optimize=True)

# previews for checking
if prev_dir:
    ov = rgb.copy(); ov[crop(mask_p) == 0] = ov[crop(mask_p) == 0] * 0.3 + np.array([0.7, 0.1, 0.1]) * 0.7
    Image.fromarray(tobyte(ov)).save(f"{prev_dir}/prev_mask.png")
    Image.fromarray(tobyte(height)).save(f"{prev_dir}/prev_height.png")
    Image.fromarray(nh[..., :3]).save(f"{prev_dir}/prev_normal.png")

    def lit(lx, ly, lz):
        L = np.array([lx, ly, lz], np.float32); L /= np.linalg.norm(L)
        # image y runs down; tangent y matches the generator (top minus bottom)
        ndl = np.clip(nx * L[0] + ny * L[1] + nz * L[2], 0, 1)
        return np.clip(rgb * (0.25 + 0.95 * ndl[..., None]), 0, 1)

    row = np.hstack([rgb, lit(-1, -1, 0.7), lit(1, 1, 0.7)])
    Image.fromarray(tobyte(row)).save(f"{prev_dir}/prev_lit.png")
    tile = np.tile(lit(-1, -1, 0.9), (2, 2, 1))
    Image.fromarray(tobyte(tile)).save(f"{prev_dir}/prev_tile.png")

print("joint share %.2f, height mean %.2f" % (1 - stone.mean(), height.mean()))
