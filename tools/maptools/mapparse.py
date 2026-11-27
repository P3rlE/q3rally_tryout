"""Parse a Q3 .map (brush format 1: three points + texture): entities, brushes, brush corners and bounds.
Used by make_figure8.py to take the stadium over from the original map."""
import itertools, re
import numpy as np

NUM = r'(-?[\d.e+-]+)'
PLANE = re.compile(r'^\s*\( %s %s %s \) \( %s %s %s \) \( %s %s %s \) (\S+)(.*)$' % ((NUM,) * 9))

def split_entities(text):
    ents, depth, start = [], 0, None
    for m in re.finditer(r'^[{}]\s*$', text, re.M):
        if m.group().strip() == '{':
            if depth == 0: start = m.start()
            depth += 1
        else:
            depth -= 1
            if depth == 0: ents.append(text[start:m.end()])
    return ents

def entity_parts(ent):
    """(keyvalue lines, [brush texts])"""
    body = ent.strip()[1:-1]
    brushes = [m.group(0) for m in re.finditer(r'^\{\n.*?^\}', body, re.M | re.S)]
    rest = re.sub(r'^\{\n.*?^\}\n?', '', body, flags=re.M | re.S)
    kv = dict(re.findall(r'"([^"]*)" "([^"]*)"', rest))
    return kv, brushes

class Brush:
    def __init__(self, text):
        self.text = text
        self.planes = []
        for line in text.splitlines():
            m = PLANE.match(line)
            if m:
                v = list(map(float, m.groups()[:9]))
                self.planes.append((np.array(v[0:3]), np.array(v[3:6]), np.array(v[6:9]), m.group(10)))
        self.verts = self._verts()
        if len(self.verts):
            self.mins, self.maxs = self.verts.min(0), self.verts.max(0)
        else:
            self.mins = self.maxs = None
        self.textures = [p[3] for p in self.planes]

    def _verts(self):
        for sign in (1, -1):
            P = []
            for p1, p2, p3, tex in self.planes:
                n = np.cross(p3 - p1, p2 - p1) * sign
                L = np.linalg.norm(n)
                if L: n = n / L; P.append((n, n @ p1))
            V = []
            for a, b, c in itertools.combinations(range(len(P)), 3):
                A = np.array([P[a][0], P[b][0], P[c][0]])
                if abs(np.linalg.det(A)) < 1e-9: continue
                x = np.linalg.solve(A, [P[a][1], P[b][1], P[c][1]])
                if all(n @ x <= d + 0.05 for n, d in P): V.append(x)
            if len(V) >= 4: return np.array(V)
        return np.zeros((0, 3))

def load(path):
    text = open(path, encoding='latin1').read()
    return [entity_parts(e) for e in split_entities(text)]
