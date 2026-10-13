"""Convert Barbo's "Oldsmobile Cutlass Supreme Sedan '71" (Sketchfab, CC BY 4.0)
into Q3Rally body/wheel MD3s.
usage: build_olds.py <cutlass-sketchfab.fbx> <wheel.md3 to reuse> <outdir>
needs: pip install fast-simplification

The source is a ~65k triangle PBR model. Kept: paint shell, black trim, chrome,
glass and the lights (with their own UVs and texture). Dropped: badges and
script lettering (trademarks), the licence plate (the game adds its own), the
tyres (brand name on the sidewall; the shared wheel goes on their centres),
interior parts and the ground plane. Paint, black and chrome are decimated with
quadric edge collapse; the paint gets atlas UVs from layout.py.
The original texel UVs of the paint are kept in a side file (paint_src.npz) so
gen_olds_textures.py can bake the ambient occlusion onto the new atlas."""
import os, sys, math, copy
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import md3, fbxmesh, layout
import numpy as np
import fast_simplification
layout.configure(x0=-62.5, x1=62.5, y0=-24.5, y1=24.5, z0=-12.0, z1=18.5, sc=3.48, lx=34)
from layout import atlas
SRC, WHEEL, OUT = sys.argv[1:4]
P = 'models/players/cutlass/'
S = 0.3                                    # source -> Q3 units (length ~122)
GROUND_SRC = -59.6                         # ground plane of the source scene
WR = 7.95                                  # wheel radius after scaling (source tyres ~26.5)
MAXV = 990

parts = list(fbxmesh.meshes(SRC))
CZ = None
for p in parts:
    if p['model'] == 'body':
        Pz = p['pos'][:, 2]; CZ = (Pz.min() + Pz.max())/2
def q3(v):
    v = np.asarray(v, float)
    return np.stack([-S*(v[..., 2] - CZ), -S*v[..., 0], S*(v[..., 1] - GROUND_SRC) - 16.8], -1)   # nose (-z) to +x

# (model, material) -> (surface, decimate to this fraction of triangles or None)
KEEP = {
    ('body', 'M_Paint'): ('paint', 0.16), ('body', 'M_Black'): ('black', 0.35),
    ('black', 'M_Black'): ('black', 0.35), ('chrome', 'M_Black'): ('black', None),
    ('chrome', 'M_Chrome'): ('chrome', 0.11), ('trim', 'M_Badge'): ('chrome', 0.14),
    ('door_handle', 'M_Chrome'): ('chrome', 0.12),
    ('glass', 'M_Glass'): ('glass', None), ('glass2', 'M_Glass'): ('glass', None),
    ('lights', 'M_Lights'): ('lights', None),     # the clear lenses (lights-glass) would hide the lamps
    ('trim2', 'M_Lights'): ('lights', None),
}
groups = {}          # surface -> list of (positions(n,3), tris(m,3), uv(m,3,2) or None)
paint_src = []
for p in parts:
    for mat in np.unique(p['mat']):
        key = (p['model'], str(mat))
        if key not in KEEP: continue
        sname, frac = KEEP[key]
        sel = p['mat'] == mat
        T = p['tris'][sel][:, [0, 2, 1]]                       # FBX counter-clockwise -> md3 clockwise
        UVc = p['uv'][sel][:, [0, 2, 1]]
        Vw = q3(p['pos'])
        if sname == 'paint':
            paint_src.append((Vw[T], UVc))
        if frac:
            used = np.unique(T); remap = -np.ones(len(Vw), int); remap[used] = np.arange(len(used))
            pts, fcs = Vw[used].astype(np.float32), remap[T].astype(np.int32)
            # weld duplicated positions first so the simplifier sees one surface
            key_ = np.round(pts, 3); uniq, inv = np.unique(key_, axis=0, return_inverse=True)
            pts = uniq.astype(np.float32); fcs = inv.reshape(-1)[fcs]
            fcs = fcs[(fcs[:, 0] != fcs[:, 1]) & (fcs[:, 1] != fcs[:, 2]) & (fcs[:, 0] != fcs[:, 2])]
            pts2, fcs2 = fast_simplification.simplify(pts, fcs, target_reduction=1 - frac)
            groups.setdefault(sname, []).append((np.asarray(pts2, float), np.asarray(fcs2), None))
        else:
            groups.setdefault(sname, []).append((Vw, T, UVc))

def paint_region(n):
    nx, ny, nz = n
    ax = max(abs(nx), abs(ny), abs(nz))
    if nz < -0.55: return 'U'
    if ax == abs(nz): return 'T'
    if ax == abs(ny): return 'L' if ny > 0 else 'R'
    return 'F' if nx > 0 else 'B'

TEX = {'paint': 'red_main', 'black': 'black', 'chrome': 'chrome', 'glass': 'glass', 'lights': 'lights'}
m = md3.Model(); m.name = P + 'body.md3'; m.flags = 0; m.surfs = []
COS = math.cos(math.radians(45))
stats = {}
for sname in TEX:
    corners = []
    for V, T, UVc in groups.get(sname, []):
        fn = np.cross(V[T[:, 2]] - V[T[:, 0]], V[T[:, 1]] - V[T[:, 0]])
        fa = np.linalg.norm(fn, axis=1); ok = fa > 1e-9
        T, fn, fa = T[ok], fn[ok]/fa[ok][:, None], fa[ok]
        if UVc is not None: UVc = UVc[ok]
        by_pos = {}
        for fi, t in enumerate(T):
            for vi in t: by_pos.setdefault(tuple(np.round(V[vi], 3)), []).append(fi)
        for fi, t in enumerate(T):
            reg = paint_region(fn[fi])
            tri = []
            for k, vi in enumerate(t):
                adj = by_pos[tuple(np.round(V[vi], 3))]
                acc = sum(fn[fj]*fa[fj] for fj in adj if fn[fi] @ fn[fj] > COS)
                n = acc/(np.linalg.norm(acc) or 1)
                if sname == 'paint': uv = atlas(reg, V[vi])
                elif sname == 'lights': uv = [UVc[fi, k, 0], 1 - UVc[fi, k, 1]]
                elif sname in ('glass', 'chrome'): uv = [0.5 + V[vi][0]/250, 0.5 - V[vi][2]/60]
                else: uv = [0.5, 0.5]
                tri.append((V[vi], n, uv))
            corners.append(tri)
    stats[sname] = len(corners)
    cur = None; chunks = []
    for tri in corners:
        if cur is None or len(cur['v']) > MAXV - 3:
            cur = {'v': [], 'idx': {}, 't': []}; chunks.append(cur)
        ids = []
        for p_, n, uv in tri:
            key = (tuple(np.round(p_, 3)), tuple(np.round(n, 2)), tuple(np.round(uv, 4)))
            if key not in cur['idx']: cur['idx'][key] = len(cur['v']); cur['v'].append((p_, n, uv))
            ids.append(cur['idx'][key])
        cur['t'].append(ids)
    for ci, c in enumerate(chunks):
        s = md3.Surf(); s.name = sname if ci == 0 else f'{sname}{ci+1}'; s.flags = 0
        s.shaders = [(P + TEX[sname] + '.tga', 0)]
        s.tris = c['t']; s.st = [[float(a) for a in uv] for _, _, uv in c['v']]
        s.frames = [[[int(round(q*64)) for q in p_] + [md3.enc_normal(*n)] for p_, n, _ in c['v']]]
        m.surfs.append(s)
print('triangles per surface', stats, 'total', sum(stats.values()))

# ---------------- tags ---------------------------------------------------------
ID = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
BACKWARD = [[-1, 0, 0], [0, -1, 0], [0, 0, 1]]
WLA = [[0, 1, 0], [1, 0, 0], [0, 0, -1]]
WRA = [[0, -1, 0], [-1, 0, 0], [0, 0, -1]]
tags = []
wheels = [p for p in parts if p['model'] == 'wheels'][0]
WV = q3(wheels['pos'][np.unique(wheels['tris'])])
for fx in (1, -1):
    for sy in (1, -1):
        q = WV[(WV[:, 0]*fx > 0) & (WV[:, 1]*sy > 0)]
        c = (q.min(0) + q.max(0))/2; outer = np.abs(q[:, 1]).max()
        axle = WV[WV[:, 0]*fx > 0]; ax_x = (axle[:, 0].min() + axle[:, 0].max())/2      # symmetric per axle
        tags.append(('tag_wheel' + ('f' if fx > 0 else 'r') + ('l' if sy > 0 else 'r'),
                     [ax_x, sy*(outer - 2.95), axle[:, 2].min() + WR], WLA if sy > 0 else WRA))
body = np.vstack([V[np.unique(T)] for V, T, _ in groups['paint']])
roof = body[(np.abs(body[:, 1]) < 3) & (np.abs(body[:, 0] + 2) < 8), 2].max()
hood = body[(np.abs(body[:, 1]) < 3) & (np.abs(body[:, 0] - 40) < 3), 2].max()
from PIL import Image
LT = np.asarray(Image.open(os.path.join(os.path.dirname(SRC), '..', 'textures', 'M_Lights_Diffuse.png')).convert('RGB').resize((512, 512))).astype(int)
lp = [p for p in parts if p['model'] == 'lights'][0]
LC = q3(lp['pos'][lp['tris']].mean(1)); luv = lp['uv'].mean(1)
col = LT[np.clip(((1 - luv[:, 1]) % 1*511).astype(int), 0, 511), np.clip((luv[:, 0] % 1*511).astype(int), 0, 511)]
red = (col[:, 0] > 150) & (col[:, 1] < 90)
front = LC[LC[:, 0] > 40]; rear_red = LC[red & (LC[:, 0] < -40)]
rear_white = LC[~red & (LC[:, 0] < -40) & (col.mean(1) > 170)]
def side_centre(pts, sy):
    q = pts[pts[:, 1]*sy > 0]; return [q[:, 0].max(), float(q[:, 1].mean()), float(q[:, 2].mean())]
def rear_centre(pts, sy):
    q = pts[pts[:, 1]*sy > 0]; return [q[:, 0].min() - 0.2, float(q[:, 1].mean()), float(q[:, 2].mean())]
hl_l, hl_r = side_centre(front, 1), side_centre(front, -1)
bl_l, bl_r = rear_centre(rear_red, 1), rear_centre(rear_red, -1)
rv = rear_white if len(rear_white) > 4 else rear_red
rl_l, rl_r = rear_centre(rv, 1), rear_centre(rv, -1)
plate = [p for p in parts if p['model'] == 'plate'][0]
PC = q3(plate['pos'][np.unique(plate['tris'])]); PC = PC[PC[:, 0] < 0]
rear_x = body[:, 0].min()
tags += [
    ('tag_weapon',  [-2.0, 0.0, float(roof) + 0.4], ID),
    ('tag_plate',   [float(PC[:, 0].min()) - 0.1, 0.0, float(PC[:, 2].mean())], ID),
    ('tag_turbo',   [float(rear_x) + 1.0, 0.0, -10.5], BACKWARD),
    ('tag_smoke',   [40.0, 0.0, float(hood) + 0.3], BACKWARD),
    ('tag_hlite1',  [hl_l[0] + 0.2, hl_l[1], hl_l[2]], ID), ('tag_hlite2', [hl_r[0] + 0.2, hl_r[1], hl_r[2]], ID),
    ('tag_blite1',  bl_r, ID), ('tag_blite2', bl_l, ID),
    ('tag_rlite1',  rl_r, ID), ('tag_rlite2', rl_l, ID),
]
m.tags = [[[n, [round(float(c), 3) for c in o], copy.deepcopy(a)] for n, o, a in tags]]
allp = [[c/64 for c in v[:3]] for s in m.surfs for v in s.frames[0]]
mn = [min(p[k] for p in allp) for k in range(3)]; mx = [max(p[k] for p in allp) for k in range(3)]
m.frames = [mn + mx + [0.0, 0.0, 0.0, max(math.sqrt(sum(c*c for c in p)) for p in allp), b'cutlass']]
md3.save(m, OUT + '/body.md3')
print('body bounds', [round(c, 2) for c in mn], [round(c, 2) for c in mx])
for s in m.surfs: print('  ', s.name, len(s.st), 'verts', len(s.tris), 'tris')
for n, o, _ in m.tags[0]: print('  ', n, o)
np.savez_compressed(OUT + '/paint_src.npz', tri=np.vstack([a for a, _ in paint_src]), uv=np.vstack([b for _, b in paint_src]))

w = md3.load(WHEEL); w.name = P + 'wheel.md3'
WS = WR/8.4
w.frames = w.frames[:1]; w.frames[0][10] = b'wheel'
for s in w.surfs:
    s.frames = [[[int(round(v[k]*WS)) for k in range(3)] + [v[3]] for v in s.frames[0]]]
wp = [[c/64 for c in v[:3]] for s in w.surfs for v in s.frames[0]]
w.frames[0][:6] = [min(p[k] for p in wp) for k in range(3)] + [max(p[k] for p in wp) for k in range(3)]
w.frames[0][9] = max(math.sqrt(sum(c*c for c in p)) for p in wp)
md3.save(w, OUT + '/wheel.md3')
