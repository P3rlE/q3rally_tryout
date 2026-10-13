"""Minimal Autodesk 3DS reader: meshes (vertices, faces, UVs, face materials) and materials."""
import struct

def _cstr(d, o):
    e = d.index(b'\0', o)
    return d[o:e].decode('latin-1'), e + 1

def load(path):
    d = open(path, 'rb').read()
    out = type('TDS', (), {})(); out.meshes = []; out.materials = []

    def color(o, end):
        while o < end:
            cid, ln = struct.unpack_from('<HI', d, o)
            if cid == 0x0011: return tuple(d[o+6:o+9])
            if cid == 0x0010: return tuple(int(c*255) for c in struct.unpack_from('<3f', d, o+6))
            o += ln
        return None

    def walk(o, end, ctx):
        while o + 6 <= end:
            cid, ln = struct.unpack_from('<HI', d, o)
            if ln < 6: break
            body, nxt = o + 6, o + ln
            if cid in (0x4D4D, 0x3D3D, 0x4100):
                walk(body, nxt, ctx)
            elif cid == 0x4000:
                name, p = _cstr(d, body)
                mesh = {'name': name, 'verts': [], 'faces': [], 'uv': [], 'facemat': {}}
                walk(p, nxt, mesh)
                if mesh['verts']: out.meshes.append(mesh)
            elif cid == 0x4110:
                n, = struct.unpack_from('<H', d, body)
                ctx['verts'] = [struct.unpack_from('<3f', d, body + 2 + 12*i) for i in range(n)]
            elif cid == 0x4120:
                n, = struct.unpack_from('<H', d, body)
                ctx['faces'] = [struct.unpack_from('<3H', d, body + 2 + 8*i) for i in range(n)]
                walk(body + 2 + 8*n, nxt, ctx)
            elif cid == 0x4130:
                name, p = _cstr(d, body)
                n, = struct.unpack_from('<H', d, p)
                ctx['facemat'][name] = list(struct.unpack_from(f'<{n}H', d, p + 2))
            elif cid == 0x4140:
                n, = struct.unpack_from('<H', d, body)
                ctx['uv'] = [struct.unpack_from('<2f', d, body + 2 + 8*i) for i in range(n)]
            elif cid == 0x4160:
                ctx['matrix'] = struct.unpack_from('<12f', d, body)
            elif cid == 0xAFFF:
                mat = {'name': '', 'diffuse': None, 'texture': ''}
                walk(body, nxt, mat); out.materials.append(mat)
            elif cid == 0xA000:
                ctx['name'], _ = _cstr(d, body)
            elif cid == 0xA020:
                ctx['diffuse'] = color(body, nxt)
            elif cid == 0xA200:
                walk(body, nxt, ctx)
            elif cid == 0xA300:
                ctx['texture'], _ = _cstr(d, body)
            o = nxt
    walk(0, len(d), {})
    return out
