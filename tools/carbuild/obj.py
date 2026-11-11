"""Minimal Wavefront OBJ reader (positions, uvs, normals, objects, materials).

load(path) -> (V, VT, VN, groups)
groups is a list of ((object, material), tris) in file order; every tri is
three (v, vt, vn) index triples (0-based, vt/vn may be None). Polygons are
fan-triangulated in the file's counter-clockwise order.
"""


def load(path, groups_as_objects=False, encoding=None):
    """groups_as_objects: 'g' lines name the object (KeyShot exports have no 'o').
    Negative (relative) indices are resolved; material names keep their spaces."""
    V, VT, VN = [], [], []
    groups, order = {}, []
    obj = mtl = None
    for line in open(path, encoding=encoding):
        p = line.split()
        if not p:
            continue
        t = p[0]
        if t == 'v':
            V.append(tuple(map(float, p[1:4])))
        elif t == 'vt':
            VT.append(tuple(map(float, p[1:3])))
        elif t == 'vn':
            VN.append(tuple(map(float, p[1:4])))
        elif t == 'o' or (t == 'g' and groups_as_objects):
            obj = line.split(None, 1)[1].strip() if len(p) > 1 else ''
        elif t == 'usemtl':
            mtl = line.split(None, 1)[1].strip() if groups_as_objects else p[1]
        elif t == 'f':
            key = (obj, mtl)
            if key not in groups:
                groups[key] = []
                order.append(key)
            corners = []
            for c in p[1:]:
                a = (c.split('/') + ['', ''])[:3]
                counts = (len(V), len(VT), len(VN))
                corners.append(tuple((int(x) - 1 if int(x) > 0 else n + int(x)) if x else None
                                     for x, n in zip(a, counts)))
            for i in range(1, len(corners) - 1):
                groups[key].append((corners[0], corners[i], corners[i + 1]))
    return V, VT, VN, [(k, groups[k]) for k in order]
