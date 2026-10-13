"""Convert the rusty sheriff muscle car (PoliceCar.3DS) into Q3Rally body/wheel MD3s.
usage: build_police.py <PoliceCar.3DS> <wheel.md3 to reuse> <outdir>

The 3DS has the body (two materials: SFERIFF.JPG for the shell, SFERIFFI.JPG
for engine, interior and underbody) and four own wheels whose tyre texture
carries a brand name. The wheels are dropped; their centres become the wheel
tags for the shared Viper wheel. Lights and plate are found by mapping texture
features back onto the mesh."""
import os, sys, math, copy
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import md3, tds
import numpy as np
SRC, WHEEL, OUT = sys.argv[1:4]
P = 'models/players/sheriff/'
S = 0.02                                 # 3DS units -> Q3 units (length ~112)
WR = 8.4                                 # radius of the shared wheel
MAXV = 990

src = tds.load(SRC)
body = [x for x in src.meshes if x['name'] == 'body'][0]
wheels = [x for x in src.meshes if x['name'].startswith('wheel')]
BV = np.array(body['verts'])
CY = (BV[:, 1].min() + BV[:, 1].max())/2
low = min(np.array(w['verts'])[:, 2].min() for w in wheels)          # ground under the original tyres
DZ = -16.8 - S*low                                                   # same ground as the other cars
def q3(p):
    p = np.asarray(p, float)
    return np.stack([-S*(p[..., 1] - CY), S*p[..., 0], S*p[..., 2] + DZ], -1)   # nose (-y) to +x

V = q3(BV); UV = np.array(body['uv']); F = np.array(body['faces'])
fmat = {}
for k, fl in body['facemat'].items():
    for f in fl: fmat[f] = k
# 3DS winding is counter-clockwise, md3 clockwise
tris = F[:, [0, 2, 1]]
fn = np.cross(V[tris[:, 2]] - V[tris[:, 0]], V[tris[:, 1]] - V[tris[:, 0]])
fa = np.linalg.norm(fn, axis=1); fn = fn/np.maximum(fa, 1e-9)[:, None]

# smooth normals across faces meeting at a position within 50 degrees
corner_n = np.zeros((len(tris), 3, 3))
by_pos = {}
for fi, t in enumerate(tris):
    for k, vi in enumerate(t): by_pos.setdefault(tuple(V[vi].round(3)), []).append(fi)
COS = math.cos(math.radians(50))
for fi, t in enumerate(tris):
    for k, vi in enumerate(t):
        acc = np.zeros(3)
        for fj in by_pos[tuple(V[vi].round(3))]:
            if fn[fi] @ fn[fj] > COS: acc += fn[fj]*fa[fj]
        corner_n[fi, k] = acc/(np.linalg.norm(acc) or 1)

m = md3.Model(); m.name = P + 'body.md3'; m.flags = 0; m.surfs = []
for mat, sname, tex in (('1 - Default', 'body', 'sheriff'), ('2 - Default', 'parts', 'parts')):
    faces = [fi for fi in range(len(tris)) if fmat.get(fi) == mat]
    chunk = None; chunks = []
    for fi in faces:
        if chunk is None or len(chunk['v']) > MAXV - 3:
            chunk = {'v': [], 'idx': {}, 't': []}; chunks.append(chunk)
        ids = []
        for k, vi in enumerate(tris[fi]):
            uv = (float(UV[vi][0]), float(1 - UV[vi][1]))
            key = (tuple(V[vi].round(3)), tuple(corner_n[fi, k].round(2)), tuple(round(c, 4) for c in uv))
            if key not in chunk['idx']:
                chunk['idx'][key] = len(chunk['v']); chunk['v'].append((V[vi], corner_n[fi, k], uv))
            ids.append(chunk['idx'][key])
        chunk['t'].append(ids)
    for ci, c in enumerate(chunks):
        s = md3.Surf(); s.name = sname if ci == 0 else f'{sname}{ci+1}'; s.flags = 0
        s.shaders = [(P + tex + '.tga', 0)]
        s.tris = c['t']; s.st = [list(uv) for _, _, uv in c['v']]
        s.frames = [[[int(round(q*64)) for q in p] + [md3.enc_normal(*n)] for p, n, _ in c['v']]]
        m.surfs.append(s)

# ---------------- tags ---------------------------------------------------------
def texel(px, py, mat='1 - Default', N=512):
    """model position(s) of a texel of the shell texture"""
    u, v = px/N, 1 - py/N; out = []
    for fi, f in enumerate(F):
        if fmat.get(fi) != mat: continue
        (u0, v0), (u1, v1), (u2, v2) = UV[list(f)]
        den = (v1-v2)*(u0-u2) + (u2-u1)*(v0-v2)
        if abs(den) < 1e-12: continue
        a = ((v1-v2)*(u-u2) + (u2-u1)*(v-v2))/den; b = ((v2-v0)*(u-u2) + (u0-u2)*(v-v2))/den; c = 1-a-b
        if min(a, b, c) >= -1e-6: out.append(q3(a*BV[f[0]] + b*BV[f[1]] + c*BV[f[2]]))
    return out
ID = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
BACKWARD = [[-1, 0, 0], [0, -1, 0], [0, 0, 1]]
WLA = [[0, 1, 0], [1, 0, 0], [0, 0, -1]]
WRA = [[0, -1, 0], [-1, 0, 0], [0, 0, -1]]
tags = []
for w in wheels:
    W = q3(np.array(w['verts']))
    c = (W.min(0) + W.max(0))/2
    ground = W[:, 2].min()
    left = c[1] > 0
    name = 'tag_wheel' + ('f' if c[0] > 0 else 'r') + ('l' if left else 'r')
    outer = np.abs(W[:, 1]).max()                      # keep the original tyre's outer face
    tags.append((name, [c[0], (outer - 3.1)*(1 if left else -1), ground + WR], WLA if left else WRA))
hl = sorted(texel(175, 395) + texel(335, 395), key=lambda p: p[1])           # painted headlights
tl = sorted(texel(240, 40) + texel(240, 70) + texel(240, 170) + texel(240, 200), key=lambda p: p[1])
BY = (abs(tl[0][1]) + abs(tl[3][1]))/2; RY = (abs(tl[1][1]) + abs(tl[2][1]))/2   # the painted bars are slightly uneven
plate = texel(220, 100)[0]                                                  # 'DANGER' sign on the tail
roof = max(p[2] for p in V if abs(p[1]) < 3 and abs(p[0] + 4) < 6)
blower = V[(np.abs(V[:, 1]) < 4) & (V[:, 0] > 20)]
blower_top = blower[blower[:, 2].argmax()]
exh = V[(V[:, 0] < V[:, 0].min() + 3) & (V[:, 2] < V[:, 2].min() + 6)]
tags += [
    ('tag_weapon',  [-4.0, 0.0, roof + 0.4], ID),
    ('tag_plate',   [plate[0] - 0.2, 0.0, plate[2]], ID),
    ('tag_turbo',   [exh[:, 0].min() - 0.3, 0.0, float(exh[:, 2].mean())], BACKWARD),
    ('tag_smoke',   [float(blower_top[0]), 0.0, float(blower_top[2]) + 0.3], BACKWARD),   # supercharger
    ('tag_hlite1',  [hl[1][0] + 0.2, hl[1][1], hl[1][2]], ID), ('tag_hlite2', [hl[0][0] + 0.2, hl[0][1], hl[0][2]], ID),
    ('tag_blite1',  [tl[0][0] - 0.2, -BY, tl[0][2]], ID), ('tag_blite2', [tl[3][0] - 0.2, BY, tl[3][2]], ID),
    ('tag_rlite1',  [tl[1][0] - 0.2, -RY, tl[1][2]], ID), ('tag_rlite2', [tl[2][0] - 0.2, RY, tl[2][2]], ID),
]
order = ['tag_wheelfl', 'tag_wheelfr', 'tag_wheelrl', 'tag_wheelrr']
tags.sort(key=lambda t: order.index(t[0]) if t[0] in order else 9)
m.tags = [[[n, [round(float(c), 3) for c in o], copy.deepcopy(a)] for n, o, a in tags]]
allp = [[c/64 for c in v[:3]] for s in m.surfs for v in s.frames[0]]
mn = [min(p[k] for p in allp) for k in range(3)]; mx = [max(p[k] for p in allp) for k in range(3)]
m.frames = [mn + mx + [0.0, 0.0, 0.0, max(math.sqrt(sum(c*c for c in p)) for p in allp), b'sheriff']]
md3.save(m, OUT + '/body.md3')
print('body bounds', [round(c, 2) for c in mn], [round(c, 2) for c in mx])
for s in m.surfs: print('  ', s.name, len(s.st), 'verts', len(s.tris), 'tris')
for n, o, _ in m.tags[0]: print('  ', n, o)

w = md3.load(WHEEL); w.name = P + 'wheel.md3'
w.frames = w.frames[:1]; w.frames[0][10] = b'wheel'
for s in w.surfs: s.frames = s.frames[:1]
md3.save(w, OUT + '/wheel.md3')
