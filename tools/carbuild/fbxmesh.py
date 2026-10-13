"""Triangulated, transformed meshes out of a binary FBX (via fbx.py)."""
import math
import numpy as np
import fbx

def euler_xyz(rx, ry, rz):
    rx, ry, rz = (math.radians(a) for a in (rx, ry, rz))
    X = np.array([[1, 0, 0], [0, math.cos(rx), -math.sin(rx)], [0, math.sin(rx), math.cos(rx)]])
    Y = np.array([[math.cos(ry), 0, math.sin(ry)], [0, 1, 0], [-math.sin(ry), 0, math.cos(ry)]])
    Z = np.array([[math.cos(rz), -math.sin(rz), 0], [math.sin(rz), math.cos(rz), 0], [0, 0, 1]])
    return Z @ Y @ X

def meshes(path):
    """yield dicts: model name, material name per triangle, world positions (n,3),
    per-corner uv (m,3,2), triangles (m,3) indexing positions"""
    r = fbx.load(path)
    obj = r.first('Objects'); byid = {c.props[0]: c for c in obj.children}
    links = [(c.props[1], c.props[2]) for c in r.first('Connections').find('C')]
    name = lambda n: n.props[1].split('\x00')[0]
    for gsrc, mdst in links:
        g, mdl = byid.get(gsrc), byid.get(mdst)
        if g is None or g.name != 'Geometry' or mdl is None or mdl.name != 'Model': continue
        mats = [name(byid[s]) for s, d in links if d == mdst and s in byid and byid[s].name == 'Material']
        p = fbx.props70(mdl)
        T = np.array(p.get('Lcl Translation', [0, 0, 0]), float)
        R = euler_xyz(*p.get('Lcl Rotation', [0, 0, 0]))
        S = np.array(p.get('Lcl Scaling', [1, 1, 1]), float)
        V = g.first('Vertices').props[0].reshape(-1, 3)
        W = (R @ (V*S).T).T + T
        pvi = g.first('PolygonVertexIndex').props[0]
        uvl = g.first('LayerElementUV')
        uv = uvi = None
        if uvl:
            uv = uvl.first('UV').props[0].reshape(-1, 2)
            ui = uvl.first('UVIndex'); uvi = ui.props[0] if ui else np.arange(len(pvi))
        ml = g.first('LayerElementMaterial')
        pm = ml.first('Materials').props[0] if ml else None
        tris, tuv, tmat = [], [], []
        poly, start, pidx = [], 0, 0
        for k, v in enumerate(pvi):
            poly.append(k)
            if v < 0:
                idx = [pvi[c] if pvi[c] >= 0 else ~pvi[c] for c in poly]
                mi = (pm[pidx] if pm is not None and len(pm) > 1 else (pm[0] if pm is not None else 0))
                for j in range(1, len(poly) - 1):
                    cs = (poly[0], poly[j], poly[j+1])
                    tris.append([idx[0], idx[j], idx[j+1]])
                    tuv.append([uv[uvi[c]] if uv is not None else (0, 0) for c in cs])
                    tmat.append(mats[mi] if mi < len(mats) else '?')
                poly = []; pidx += 1
        yield {'model': name(mdl), 'pos': W, 'tris': np.array(tris), 'uv': np.array(tuv, float), 'mat': np.array(tmat)}
