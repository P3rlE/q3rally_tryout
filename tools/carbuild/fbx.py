"""Minimal binary FBX (7.x) reader: node tree, plus geometry/model/material helpers."""
import struct, zlib
import numpy as np

class Node:
    def __init__(self, name, props, children):
        self.name, self.props, self.children = name, props, children
    def find(self, name): return [c for c in self.children if c.name == name]
    def first(self, name):
        r = self.find(name); return r[0] if r else None
    def __repr__(self): return f'Node({self.name}, {len(self.props)} props, {len(self.children)} children)'

ARR = {'f': ('<f', 4), 'd': ('<d', 8), 'l': ('<q', 8), 'i': ('<i', 4), 'b': ('<b', 1)}

def _props(d, o, n):
    out = []
    for _ in range(n):
        t = chr(d[o]); o += 1
        if t == 'Y': out.append(struct.unpack_from('<h', d, o)[0]); o += 2
        elif t == 'C': out.append(bool(d[o])); o += 1
        elif t == 'I': out.append(struct.unpack_from('<i', d, o)[0]); o += 4
        elif t == 'F': out.append(struct.unpack_from('<f', d, o)[0]); o += 4
        elif t == 'D': out.append(struct.unpack_from('<d', d, o)[0]); o += 8
        elif t == 'L': out.append(struct.unpack_from('<q', d, o)[0]); o += 8
        elif t in ARR:
            n_, enc, clen = struct.unpack_from('<III', d, o); o += 12
            raw = d[o:o+clen]; o += clen
            if enc == 1: raw = zlib.decompress(raw)
            fmt, sz = ARR[t]
            out.append(np.frombuffer(raw, dtype=np.dtype(fmt), count=n_).copy())
        elif t in 'SR':
            ln, = struct.unpack_from('<I', d, o); o += 4
            v = d[o:o+ln]; o += ln
            out.append(v.decode('utf-8', 'replace') if t == 'S' else v)
        else:
            raise ValueError(f'unknown property type {t!r} at {o}')
    return out

def _node(d, o, wide):
    if wide:
        end, nprops, plen = struct.unpack_from('<QQQ', d, o); o += 24
    else:
        end, nprops, plen = struct.unpack_from('<III', d, o); o += 12
    nl = d[o]; o += 1
    if end == 0: return None, o
    name = d[o:o+nl].decode('latin-1'); o += nl
    props = _props(d, o, nprops); o += plen
    children = []
    sentinel = 25 if wide else 13
    while o < end - sentinel or (o < end and d[o:o+sentinel] != b'\0'*sentinel):
        c, o2 = _node(d, o, wide)
        if c is None: o = o2; break
        children.append(c); o = o2
    return Node(name, props, children), end

def load(path):
    d = open(path, 'rb').read()
    assert d[:20] == b'Kaydara FBX Binary  '
    ver, = struct.unpack_from('<I', d, 23)
    wide = ver >= 7500
    o = 27; top = []
    while o < len(d):
        n, o2 = _node(d, o, wide)
        if n is None: break
        top.append(n); o = o2
    return Node('root', [], top)

def props70(node):
    """Properties70 of an object as {name: values}"""
    out = {}
    p = node.first('Properties70')
    if p:
        for c in p.find('P'): out[c.props[0]] = c.props[4:]
    return out
