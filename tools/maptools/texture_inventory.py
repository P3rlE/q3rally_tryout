#!/usr/bin/env python3
"""List the textures the shipped Q3Rally maps show, largest area first,
for planning hand-made normal / parallax / specular maps.

  python texture_inventory.py ../../baseq3r [--json out.json]

Area is taken from the planar brush faces of every compiled map in
baseq3r/maps (patches and models are not counted), summed per diffuse
image, because the renderer finds <image>_nh / _n / _s next to the image.
Test maps are skipped. For each image the list shows the maps it is used
in, the share of it on the ground (faces up), its size, the material (surfaceparm), whether the shader blends or is
alpha tested or unlit (no lightmap stage: surface maps have no effect),
and which surface maps already exist.
"""
import glob
import json
import math
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import q3bsp  # noqa: E402
from rescale_textures import (LUMP_DRAWVERTS, LUMP_DRAWINDEXES, VERT_SIZE, MST_PLANAR,  # noqa: E402
                              SURF_SKY, SURF_NODRAW, shader_names, surfaces, short_name, image_size)

SKIP_MAPS = {'test', 'q3r_watertest', 'q3r_ctf_bot_test'}
MATERIALS = [(0x2000000, 'ice'), (0x800000, 'snow'), (0x8000000, 'metal'), (0x1000000, 'gravel'),
             (0x4000000, 'dirt'), (0x40000, 'dust'), (0x10000000, 'sand'), (0x100000, 'grass'),
             (0x200000, 'asphalt')]


def shader_bodies(scripts):
    """shader name (lower case) -> body text of its first definition."""
    bodies = {}
    for path in sorted(glob.glob(os.path.join(scripts, '*.shader'))):
        text = re.sub(r'//[^\n]*', '', open(path, encoding='latin-1').read())
        depth, last, start, name = 0, None, 0, None
        for m in re.finditer(r'\{|\}|[^\s{}]+', text):
            tok = m.group(0)
            if tok == '{':
                if depth == 0:
                    name, start = last, m.end()
                depth += 1
            elif tok == '}':
                depth -= 1
                if depth == 0 and name:
                    bodies.setdefault(name.lower().replace('\\', '/'), text[start:m.start()])
            else:
                last = tok
    return bodies


def first_image(body):
    for m in re.finditer(r'\b(?:map|clampmap)\s+([^\s{}]+)', body, re.I):
        if not m.group(1).startswith('$'):
            return m.group(1).replace('\\', '/')
    m = re.search(r'\banimmap\s+[\d.]+\s+([^\s{}]+)', body, re.I)
    return m.group(1).replace('\\', '/') if m else None


def kind_of(body):
    """how the renderer treats the first image: opaque, alphatest, blend."""
    if body is None:
        return 'opaque'
    stages = re.findall(r'\{([^{}]*)\}', body)
    # no lightmap stage and no vertex lighting: drawn fullbright, so the
    # renderer never uses normal or specular maps on it
    if not any(re.search(r'\$lightmap', st, re.I) for st in stages) and \
            not any(re.search(r'rgbgen\s+(vertex|exactvertex|lightingdiffuse)', st, re.I) for st in stages):
        return 'unlit'
    for st in stages:
        if re.search(r'\$lightmap', st, re.I):
            continue
        if re.search(r'alphafunc', st, re.I):
            return 'alphatest'
        bf = re.search(r'blendfunc\s+(\S+)(?:\s+(\S+))?', st, re.I)
        if bf:
            a = bf.group(1).lower()
            b = (bf.group(2) or '').lower()
            if a == 'filter' or (a == 'gl_dst_color' and b == 'gl_zero') or (a == 'gl_zero' and b == 'gl_src_color'):
                return 'opaque'     # lightmap filter on top of the diffuse
            return 'blend'
        return 'opaque'
    return 'opaque'


def material_of(body, flags):
    """surfaceparm SURF_xxx of the shader script; the BSP flags only as a
    fallback (maps compiled without the Q3Rally parms don't carry them)."""
    if body:
        for m in re.finditer(r'surfaceparm\s+(?:SURF_)?(\w+)', body, re.I):
            name = m.group(1).lower()
            if name in [n for _, n in MATERIALS]:
                return name
    return next((n for bit, n in MATERIALS if flags & bit), '')


def existing(base, image):
    stem = os.path.splitext(image)[0]
    found = []
    for suf in ('_nh', '_n', '_s'):
        if any(os.path.exists(os.path.join(base, stem + suf + e)) for e in ('.png', '.tga', '.jpg')):
            found.append(suf)
    return found


def main():
    base = os.path.abspath(sys.argv[1])
    out_json = sys.argv[sys.argv.index('--json') + 1] if '--json' in sys.argv else None
    bodies = shader_bodies(os.path.join(base, 'scripts'))
    acc = {}
    for bsp in sorted(glob.glob(os.path.join(base, 'maps', '*.bsp'))):
        mapname = os.path.splitext(os.path.basename(bsp))[0]
        if mapname in SKIP_MAPS:
            continue
        data = open(bsp, 'rb').read()
        names = shader_names(data)
        lumps = q3bsp.read_lumps(data)
        vo, io = lumps[LUMP_DRAWVERTS][0], lumps[LUMP_DRAWINDEXES][0]
        for sh, _, typ, fv, nv, fi, ni in surfaces(data):
            name, flags = names[sh]
            if typ != MST_PLANAR or flags & (SURF_SKY | SURF_NODRAW):
                continue
            world = floor = 0.0
            for t in range(0, ni, 3):
                a, b, c = (struct.unpack_from('<3f', data, vo + (fv + struct.unpack_from('<i', data, io + 4 * (fi + t + j))[0]) * VERT_SIZE)
                           for j in range(3))
                u = [b[j] - a[j] for j in range(3)]
                v = [c[j] - a[j] for j in range(3)]
                cx, cy, cz = u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]
                ar = 0.5 * math.sqrt(cx * cx + cy * cy + cz * cz)
                world += ar
                if ar > 0 and abs(cz) / (2 * ar) > 0.7:     # faces up (or down): ground the cars drive on
                    floor += ar
            key = name.lower().replace('\\', '/')
            body = bodies.get(key)
            image = (first_image(body) if body else None) or key
            image = os.path.splitext(image)[0].lower()
            e = acc.setdefault(image, {'area': 0.0, 'floor': 0.0, 'maps': {}, 'shaders': set(), 'mats': {}, 'kinds': set()})
            e['area'] += world
            e['floor'] += floor
            mat = material_of(body, flags)
            e['mats'][mat] = e['mats'].get(mat, 0.0) + world
            e['maps'][mapname] = e['maps'].get(mapname, 0.0) + world
            e['shaders'].add(short_name(key))
            e['kinds'].add(kind_of(body))
    total = sum(e['area'] for e in acc.values()) or 1.0
    rows = []
    for image, e in sorted(acc.items(), key=lambda kv: -kv[1]['area']):
        size = image_size(base, image)
        mat = max(e['mats'], key=e['mats'].get)
        rows.append({
            'image': short_name(image),
            'share': round(100 * e['area'] / total, 3),
            'floor': round(e['floor'] / e['area'], 2) if e['area'] else 0,
            'maps': sorted(e['maps'], key=lambda m: -e['maps'][m]),
            'size': '%dx%d' % size if size else '',
            'material': mat,
            'kind': '/'.join(sorted(e['kinds'])),
            'shaders': sorted(e['shaders']),
            'existing': existing(base, image),
        })
    if out_json:
        json.dump(rows, open(out_json, 'w'), indent=1)
    for r in rows[:60]:
        print('%6.2f%% %3d%%  %-42s %-9s %-8s %-10s %-6s %s' % (r['share'], 100 * r['floor'], r['image'], r['size'], r['material'], r['kind'],
                                                       ''.join(r['existing']) or '-', ', '.join(r['maps'][:4])))
    print('%d images, %d with own maps' % (len(rows), sum(1 for r in rows if r['existing'])))


if __name__ == '__main__':
    main()
