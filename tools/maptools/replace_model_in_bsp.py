#!/usr/bin/env python3
"""Swap a misc_model that q3map2 has baked into a compiled BSP for another md3.

q3map2 turns every misc_model into map surfaces (triangle soups) and drops
the entity. When a map's source can no longer be compiled as it was (models
it used are gone, the lighting was tuned on the old compile), this tool
replaces only those surfaces and leaves the rest of the BSP as it is:

  - the surfaces whose shader starts with --old are removed,
  - every surface of the new md3 becomes one vertex-lit triangle soup at
    --origin (optionally turned by --angle round z),
  - its vertex colours come from the map's light grid, the same light the
    game gives a car standing there (scaled like the old model's colours),
  - each new surface is listed in every leaf its bounds touch, so the
    renderer finds it from every cluster that sees that spot,
  - the AAS next to the map gets the new BSP checksum (the bot navigation
    does not depend on non-solid models).

usage: replace_model_in_bsp.py maps/q3r_ctf_04.bsp
           --old models/mapobjects/Xaero_statue/
           --model models/mapobjects/q3r_trophy/trophy.md3
           --origin -8 0 -8 [--angle 0] [--base baseq3r] [--aas X.aas]
"""
import argparse
import math
import os
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'carbuild'))
import md3  # noqa: E402
import q3bsp  # noqa: E402

(ENTITIES, SHADERS, PLANES, NODES, LEAFS, LEAFSURFACES, LEAFBRUSHES, MODELS, BRUSHES, BRUSHSIDES,
 DRAWVERTS, DRAWINDEXES, FOGS, SURFACES, LIGHTMAPS, LIGHTGRID, VISIBILITY) = range(17)
VERT = np.dtype([('xyz', '<f4', 3), ('st', '<f4', 2), ('lm', '<f4', 2), ('n', '<f4', 3), ('rgba', 'u1', 4)])
SURF = struct.Struct('<12i12f2i')
LEAF = struct.Struct('<2i3i3i4i')
MODEL = struct.Struct('<6f4i')
MST_TRIANGLE_SOUP = 3
LIGHTMAP_BY_VERTEX = -3


class LightGrid:
    """The BSP light grid, sampled like the renderer does for entities."""

    def __init__(self, data, lumps, ents):
        o, l = lumps[MODELS]
        mins, maxs = np.array(MODEL.unpack_from(data, o)[0:3]), np.array(MODEL.unpack_from(data, o)[3:6])
        size = np.array([64.0, 64.0, 128.0])
        for e in ents:
            d = dict(e)
            if d.get('classname') == 'worldspawn' and 'gridsize' in d:
                size = np.array([float(v) for v in d['gridsize'].split()])
        self.size = size
        self.origin = size * np.ceil(mins / size)
        top = size * np.floor(maxs / size)
        self.bounds = ((top - self.origin) / size + 1).astype(int)
        o, l = lumps[LIGHTGRID]
        n = int(np.prod(self.bounds))
        if l != n * 8:
            raise ValueError(f'light grid has {l // 8} points, expected {n}')
        self.cells = np.frombuffer(data[o:o + l], np.uint8).reshape(n, 8)

    def sample(self, p):
        """Ambient colour, directed colour and light direction at p (0..255 scale)."""
        lp = (np.asarray(p, float) - self.origin) / self.size
        pos = np.floor(lp).astype(int)
        frac = lp - pos
        pos = np.clip(pos, 0, self.bounds - 1)
        amb = np.zeros(3)
        dirc = np.zeros(3)
        vec = np.zeros(3)
        total = 0.0
        bx, by, bz = self.bounds
        for i in range(8):
            f = 1.0
            q = pos.copy()
            for j in range(3):
                if i & (1 << j):
                    if q[j] + 1 > self.bounds[j] - 1:
                        break
                    q[j] += 1
                    f *= frac[j]
                else:
                    f *= 1 - frac[j]
            else:
                c = self.cells[q[0] + q[1] * bx + q[2] * bx * by]
                if not c[:6].any():                 # in solid: no light stored
                    continue
                total += f
                amb += f * c[0:3].astype(float)
                dirc += f * c[3:6].astype(float)
                lat, lng = int(c[6]) * 2 * math.pi / 255, int(c[7]) * 2 * math.pi / 255
                vec += f * np.array([math.cos(lat) * math.sin(lng), math.sin(lat) * math.sin(lng), math.cos(lng)])
        if total > 0:
            amb, dirc = amb / total, dirc / total
        ln = np.linalg.norm(vec)
        return amb, dirc, (vec / ln if ln else np.array([0, 0, 1.0]))

    def vertex_colour(self, p, n, k=1.0):
        amb, dirc, d = self.sample(p)
        return np.clip((amb + dirc * max(0.0, float(np.dot(n, d)))) * k, 0, 255)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    ap.add_argument('bsp')
    ap.add_argument('--old', required=True, help='shader prefix of the baked model to remove')
    ap.add_argument('--model', required=True, help='md3 path relative to --base')
    ap.add_argument('--origin', nargs=3, type=float, required=True)
    ap.add_argument('--angle', type=float, default=0.0)
    ap.add_argument('--base', default=None, help='game folder (default: the folder above maps/)')
    ap.add_argument('--aas', default=None)
    ap.add_argument('--out', default=None, help='write here instead of overwriting the BSP')
    a = ap.parse_args()
    base = a.base or os.path.dirname(os.path.dirname(os.path.abspath(a.bsp)))

    data = open(a.bsp, 'rb').read()
    lumps = q3bsp.read_lumps(data)
    blob = [data[o:o + l] for o, l in lumps]
    ents = q3bsp.parse_entities(q3bsp.get_entities(data))
    grid = LightGrid(data, lumps, ents)

    shaders = [list(struct.unpack_from('<64sii', blob[SHADERS], i * 72)) for i in range(len(blob[SHADERS]) // 72)]
    names = [s[0].split(b'\0')[0].decode('latin-1') for s in shaders]
    surfs = [list(SURF.unpack_from(blob[SURFACES], i * SURF.size)) for i in range(len(blob[SURFACES]) // SURF.size)]
    verts = np.frombuffer(blob[DRAWVERTS], VERT)
    idx = np.frombuffer(blob[DRAWINDEXES], '<i4')
    old = [i for i, s in enumerate(surfs) if names[s[0]].lower().startswith(a.old.lower())]
    if not old:
        sys.exit(f'no surface uses a shader starting with {a.old}')
    if any(surfs[i][2] != MST_TRIANGLE_SOUP for i in old):
        sys.exit('the old model is not made of triangle soups')
    models = [list(MODEL.unpack_from(blob[MODELS], i * MODEL.size)) for i in range(len(blob[MODELS]) // MODEL.size)]
    w0, wn = models[0][6], models[0][7]
    if not all(w0 <= i < w0 + wn for i in old):
        sys.exit('the old model is not part of the world')

    # scale of the stored vertex colours against the light grid, from the old model
    # (q3map2 also traces shadows, so only the overall level is taken over)
    pred, have = [], []
    for i in old:
        s = surfs[i]
        for v in verts[s[3]:s[3] + s[4]]:
            pred.append(grid.vertex_colour(v['xyz'], v['n']))
            have.append(v['rgba'][:3].astype(float))
    pred, have = np.array(pred), np.array(have)
    k = float(have.mean() / max(pred.mean(), 1e-6))     # same average brightness as the old model
    print(f'old model: {len(old)} surfaces, {len(pred)} vertices; light grid scale {k:.3f}')

    # the new model, placed
    m = md3.load(os.path.join(base, a.model))
    t = math.radians(a.angle)
    R = np.array([[math.cos(t), math.sin(t), 0], [-math.sin(t), math.cos(t), 0], [0, 0, 1]])
    O = np.array(a.origin)
    new = []
    for s in m.surfs:
        P = np.array([[c / 64 for c in v[:3]] for v in s.frames[0]]) @ R + O
        N = np.array([md3.dec_normal(v[3]) for v in s.frames[0]]) @ R
        vv = np.zeros(len(P), VERT)
        vv['xyz'], vv['n'], vv['st'] = P, N, np.array(s.st)
        vv['rgba'][:, 3] = 255
        for j in range(len(P)):
            vv['rgba'][j, :3] = np.round(grid.vertex_colour(P[j], N[j], k))
        tris = np.array(s.tris, '<i4').reshape(-1)          # same winding as in the md3
        new.append((s.shaders[0][0], vv, tris, P.min(0), P.max(0)))

    # shaders: reuse the old model's entries, then append
    free = sorted({surfs[i][0] for i in old} - {s[0] for j, s in enumerate(surfs) if j not in old})
    flags = shaders[free[0]][1:] if free else [0, 1]
    shader_of = {}
    for name, *_ in new:
        if name in shader_of:
            continue
        if name in names:
            shader_of[name] = names.index(name)
            continue
        slot = free.pop(0) if free else len(shaders)
        if slot == len(shaders):
            shaders.append(None)
            names.append(None)
        shaders[slot] = [name.encode('latin-1'), *flags]
        names[slot] = name
        shader_of[name] = slot

    # surfaces: the new ones take the place of the first old one
    template = surfs[old[0]]
    first = old[0]
    keep = [i for i in range(len(surfs)) if i not in set(old)]
    order = [('old', i) for i in keep if i < first] + [('new', j) for j in range(len(new))] + \
            [('old', i) for i in keep if i > first]
    remap = {}
    out_s, out_v, out_i = [], [], []
    nv = ni = 0
    for pos, (kind, i) in enumerate(order):
        if kind == 'old':
            s = list(surfs[i])
            remap[i] = pos
            vv = verts[s[3]:s[3] + s[4]]
            ii = idx[s[5]:s[5] + s[6]]
        else:
            name, vv, ii, lo, hi = new[i]
            s = list(template)
            s[0], s[2], s[7] = shader_of[name], MST_TRIANGLE_SOUP, LIGHTMAP_BY_VERTEX
            s[4], s[6] = len(vv), len(ii)
        s[3], s[5] = nv, ni
        nv += len(vv)
        ni += len(ii)
        out_s.append(s)
        out_v.append(vv.tobytes())
        out_i.append(np.asarray(ii, '<i4').tobytes())
    new_pos = [p for p, (kind, _) in enumerate(order) if kind == 'new']

    # models: world gets the new count, brush models shift
    shift = len(new) - len(old)
    for j, md in enumerate(models):
        if j == 0:
            md[7] += shift
        elif md[6] > first:
            md[6] += shift

    # leaf surface lists: old ids renumbered, new ones added where their bounds reach
    ls = np.frombuffer(blob[LEAFSURFACES], '<i4')
    leafs = [list(LEAF.unpack_from(blob[LEAFS], i * LEAF.size)) for i in range(len(blob[LEAFS]) // LEAF.size)]
    out_ls = []
    added = 0
    for lf in leafs:
        lst = [remap[int(x)] for x in ls[lf[8]:lf[8] + lf[9]] if int(x) in remap]
        if lf[0] >= 0:
            lmin, lmax = np.array(lf[2:5]), np.array(lf[5:8])
            for p, (_, _, _, lo, hi) in zip(new_pos, new):
                if np.all(hi >= lmin) and np.all(lo <= lmax):
                    lst.append(p)
                    added += 1
        lf[8], lf[9] = len(out_ls), len(lst)
        out_ls += lst

    blob[SHADERS] = b''.join(struct.pack('<64sii', *s) for s in shaders)
    blob[SURFACES] = b''.join(SURF.pack(*s) for s in out_s)
    blob[DRAWVERTS] = b''.join(out_v)
    blob[DRAWINDEXES] = b''.join(out_i)
    blob[MODELS] = b''.join(MODEL.pack(*md) for md in models)
    blob[LEAFS] = b''.join(LEAF.pack(*lf) for lf in leafs)
    blob[LEAFSURFACES] = np.array(out_ls, '<i4').tobytes()

    order_l = sorted(range(17), key=lambda i: lumps[i][0])
    res = bytearray(data[:8 + 8 * 17])
    for i in order_l:
        while len(res) % 4:
            res.append(0)
        struct.pack_into('<ii', res, 8 + 8 * i, len(res), len(blob[i]))
        res += blob[i]
    out = a.out or a.bsp
    open(out, 'wb').write(res)
    cs = q3bsp.bsp_checksum(bytes(res))
    print(f'new model: {len(new)} surfaces, {sum(len(n[1]) for n in new)} vertices, '
          f'{sum(len(n[2]) for n in new) // 3} triangles, in {added} leaf lists')
    aas = a.aas or os.path.splitext(a.bsp)[0] + '.aas'
    if os.path.exists(aas) and not a.out:
        q3bsp.aas_set_checksum(aas, cs)
        print('AAS checksum updated:', aas)
    print('written:', out, 'checksum', cs)


if __name__ == '__main__':
    main()
