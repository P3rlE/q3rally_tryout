"""Convert "Lightbody Lifted '85 - Low poly model" by Daniel Zhabotinsky
(Sketchfab, https://skfb.ly/pAMwJ, CC BY 4.0) into Q3Rally body/wheel MD3s.
usage: build_lightbody.py <krmlgtbdy.fbx> <wheel.md3 to reuse> <outdir>
needs: pip install fast-simplification

Kept: paint (body, hood, tailgate, front bumper), the dark parts with their own
texture (frame, lift kit, fender flares, bull bar, roll bar with lamps, rear
bumper), window glass and the lamps. Dropped: interior, steering wheel, badges,
licence plate (the game adds its own), the clear lamp covers (they hide the
lamps) and the branded tyres (the shared wheel goes on their centres).
Decimated parts get their UVs back from the nearest original triangle."""
import os, sys, math, copy
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import md3, fbxmesh, layout
import numpy as np
import fast_simplification
layout.configure(x0=-58.5, x1=58.5, y0=-23.0, y1=23.0, z0=-7.5, z1=24.5, sc=3.42, lx=34)
from layout import atlas
SRC, WHEEL, OUT = sys.argv[1:4]
P = 'models/players/lightbody/'
S = 24.0                                   # metres -> Q3 units (length ~118)
WR = 8.4                                   # shared wheel radius (source tyres 0.35 m)
MAXV = 990
MINTRI = 40
MINTRI_DARK = 10                          # the lift kit is many small parts

def q3(v):
    v = np.asarray(v, float)
    return np.stack([S*v[..., 0], -S*v[..., 2], S*v[..., 1] - 16.8], -1)      # ground at -16.8

PRE = 'krmlgtbdy85_'
# part -> (surface, keep fraction or None)
KEEP = {
    'Body': ('paint', 0.45), 'Bumper_Front': ('paint', 0.45), 'Hood': ('paint', 0.6), 'Trunkdoor': ('paint', 0.6),
    'Bumper_Rear': ('dark', 0.5), 'Lifted_Suspension': ('dark', 0.08), 'Bottom': ('dark', 0.08),
    'Lifted_Trim': ('dark', 0.45), 'Bullbar': ('dark', 0.3), 'Extra_A': ('dark', 0.35),
    'Trunk_door_inner': ('dark', 0.6), 'Hood_inner': ('dark', 0.6), 'Bumper_front_inner': ('dark', 0.6),
    'Body_Glass': ('glass', None), 'Glass_Rear': ('glass', None), 'Windshield': ('glass', None),
    'Glass_Driver': ('glass', None), 'Glass_Passanger': ('glass', None),
    'Headlights': ('lights', None), 'Blinkers': ('lights', None), 'Brakelights': ('lights', None), 'Reverse': ('lights', None),
}
parts = {p['model'][len(PRE):]: p for p in fbxmesh.meshes(SRC)}

def components(pts, fcs):
    parent = np.arange(len(pts))
    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]; a = parent[a]
        return a
    for a, b, c in fcs:
        ra = find(a); parent[find(b)] = ra; parent[find(c)] = ra
    root = np.array([find(i) for i in range(len(pts))]); lab = root[fcs[:, 0]]
    for r in np.unique(lab):
        f = fcs[lab == r]; used = np.unique(f); remap = -np.ones(len(pts), int); remap[used] = np.arange(len(used))
        yield pts[used].astype(np.float32), remap[f].astype(np.int32)

def transfer_uv(Vn, Tn, Vo, To, UVo):
    """UVs for a decimated mesh: each new triangle takes the plane mapping of the
    original triangle nearest to its centroid (no smearing across UV seams)"""
    A, B, C = Vo[To[:, 0]], Vo[To[:, 1]], Vo[To[:, 2]]
    cen = (A + B + C)/3
    out = np.zeros((len(Tn), 3, 2))
    for i, t in enumerate(Tn):
        P = Vn[t]; c = P.mean(0)
        j = np.argmin(((cen - c)**2).sum(1))
        e0, e1 = B[j] - A[j], C[j] - A[j]
        d00, d01, d11 = e0 @ e0, e0 @ e1, e1 @ e1
        den = max(d00*d11 - d01*d01, 1e-12)
        for k in range(3):
            v2 = P[k] - A[j]; d20, d21 = v2 @ e0, v2 @ e1
            bv = (d11*d20 - d01*d21)/den; bw = (d00*d21 - d01*d20)/den
            out[i, k] = UVo[j, 0]*(1 - bv - bw) + UVo[j, 1]*bv + UVo[j, 2]*bw
    return out

groups = {}
for name, (sname, frac) in KEEP.items():
    p = parts[name]
    T = p['tris'][:, [0, 2, 1]]; UVc = p['uv'][:, [0, 2, 1]]          # FBX CCW -> md3 CW
    Vw = q3(p['pos'])
    if not frac:
        groups.setdefault(sname, []).append((Vw, T, UVc)); continue
    used = np.unique(T); remap = -np.ones(len(Vw), int); remap[used] = np.arange(len(used))
    pts, fcs = Vw[used].astype(np.float32), remap[T].astype(np.int32)
    uniq, inv = np.unique(np.round(pts, 3), axis=0, return_inverse=True)
    pts = uniq.astype(np.float32); fcs = inv.reshape(-1)[fcs]
    fcs = fcs[(fcs[:, 0] != fcs[:, 1]) & (fcs[:, 1] != fcs[:, 2]) & (fcs[:, 0] != fcs[:, 2])]
    if sname == 'dark':
        # the load bed is in plain view: its big flat panels are kept as they are
        cen = pts[fcs].mean(1)
        bed = (cen[:, 0] > -56) & (cen[:, 0] < -12) & (np.abs(cen[:, 1]) < 21) & (cen[:, 2] > -3)
        if bed.any():
            groups.setdefault(sname, []).append((pts.astype(float), fcs[bed], transfer_uv(pts.astype(float), fcs[bed], Vw, T, UVc)))
            fcs = fcs[~bed]
    for cpts, cfcs in components(pts, fcs):
        n = len(cfcs); keep = min(n, max(int(n*frac), MINTRI_DARK if sname == 'dark' else MINTRI))
        if keep < n:
            cpts, cfcs = fast_simplification.simplify(cpts, cfcs, target_reduction=1 - keep/n)
        cpts, cfcs = np.asarray(cpts, float), np.asarray(cfcs)
        uvn = transfer_uv(cpts, cfcs, Vw, T, UVc) if sname == 'dark' else None
        groups.setdefault(sname, []).append((cpts, cfcs, uvn))

def paint_region(n):
    nx, ny, nz = n
    ax = max(abs(nx), abs(ny), abs(nz))
    if nz < -0.55:
        if abs(ny) > 0.25: return 'L' if ny > 0 else 'R'
        if abs(nx) > 0.25: return 'F' if nx > 0 else 'B'
        return 'U'
    if ax == abs(nz): return 'T'
    if ax == abs(ny): return 'L' if ny > 0 else 'R'
    return 'F' if nx > 0 else 'B'

TEX = {'paint': 'red_main', 'dark': 'dark', 'glass': 'glass', 'lights': 'lights'}
m = md3.Model(); m.name = P + 'body.md3'; m.flags = 0; m.surfs = []
COS = math.cos(math.radians(45)); stats = {}
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
            reg = paint_region(fn[fi]); tri = []
            for k, vi in enumerate(t):
                acc = sum(fn[fj]*fa[fj] for fj in by_pos[tuple(np.round(V[vi], 3))] if fn[fi] @ fn[fj] > COS)
                n = acc/(np.linalg.norm(acc) or 1)
                if sname == 'paint': uv = atlas(reg, V[vi])
                elif UVc is not None: uv = [UVc[fi, k, 0], 1 - UVc[fi, k, 1]]
                else: uv = [0.5 + V[vi][0]/250, 0.5 - V[vi][2]/60]
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
def bbox(name):
    p = parts[name]; v = q3(p['pos'][np.unique(p['tris'])]); return v.min(0), v.max(0), v
tags = []
for wn, (fx, sy) in (('FL', (1, 1)), ('FR', (1, -1)), ('RL', (-1, 1)), ('RR', (-1, -1))):
    mn, mx, _ = bbox('WheelStock_' + wn)
    c = (mn + mx)/2
    outer = np.mean([max(abs(a), abs(b)) for a, b in ((bbox('WheelStock_' + w2)[0][1], bbox('WheelStock_' + w2)[1][1]) for w2 in ('FL', 'FR', 'RL', 'RR'))])
    tags.append(('tag_wheel' + wn.lower(), [c[0], sy*(outer - 3.1), mn[2] + WR], WLA if sy > 0 else WRA))
def side(name, sy, front):
    _, _, v = bbox(name); q = v[v[:, 1]*sy > 0]
    return [(q[:, 0].max() + 0.2) if front else (q[:, 0].min() - 0.2), float(q[:, 1].mean()), float(q[:, 2].mean())]
paint = np.vstack([V[np.unique(T)] for V, T, _ in groups['paint']])
cab = paint[(np.abs(paint[:, 1]) < 8) & (paint[:, 0] > -6) & (paint[:, 0] < 20)]
roof = cab[:, 2].max(); roof_x = float(cab[cab[:, 2].argmax(), 0])
hood = paint[(np.abs(paint[:, 1]) < 8) & (np.abs(paint[:, 0] - 42) < 5), 2].max()
pmn, pmx, _ = bbox('Numberplates_Rear')
_, _, rb = bbox('Bumper_Rear')
tags += [
    ('tag_weapon',  [roof_x, 0.0, float(roof) + 0.4], ID),
    ('tag_plate',   [float(pmn[0]) - 0.1, 0.0, float((pmn[2] + pmx[2])/2)], ID),
    ('tag_turbo',   [float(rb[:, 0].min()) - 0.3, 0.0, float(rb[:, 2].min()) - 1.0], BACKWARD),
    ('tag_smoke',   [42.0, 0.0, float(hood) + 0.3], BACKWARD),
    ('tag_hlite1',  side('Headlights', 1, True), ID), ('tag_hlite2', side('Headlights', -1, True), ID),
    ('tag_blite1',  side('Brakelights', -1, False), ID), ('tag_blite2', side('Brakelights', 1, False), ID),
    ('tag_rlite1',  side('Reverse', -1, False), ID), ('tag_rlite2', side('Reverse', 1, False), ID),
]
# the five roof lamps on the roll bar light up with the headlights
_, _, ex = bbox('Extra_A')
lamps = ex[ex[:, 2] > ex[:, 2].max() - 2.5]
if len(lamps):
    tags.append(('tag_hlite3', [float(lamps[:, 0].max()) + 0.2, 0.0, float(lamps[:, 2].mean())], ID))
m.tags = [[[n, [round(float(c), 3) for c in o], copy.deepcopy(a)] for n, o, a in tags]]
allp = [[c/64 for c in v[:3]] for s in m.surfs for v in s.frames[0]]
mn, mx = [min(p[k] for p in allp) for k in range(3)], [max(p[k] for p in allp) for k in range(3)]
m.frames = [mn + mx + [0.0, 0.0, 0.0, max(math.sqrt(sum(c*c for c in p)) for p in allp), b'lightbody']]
md3.save(m, OUT + '/body.md3')
print('body bounds', [round(c, 2) for c in mn], [round(c, 2) for c in mx])
for s in m.surfs: print('  ', s.name, len(s.st), 'verts', len(s.tris), 'tris')
for n, o, _ in m.tags[0]: print('  ', n, o)

w = md3.load(WHEEL); w.name = P + 'wheel.md3'
w.frames = w.frames[:1]; w.frames[0][10] = b'wheel'
for s in w.surfs: s.frames = s.frames[:1]
md3.save(w, OUT + '/wheel.md3')
