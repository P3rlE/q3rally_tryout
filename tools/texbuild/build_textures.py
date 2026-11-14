"""Build Q3Rally's own map textures.

    python3 tools/texbuild/build_textures.py                 # write all into baseq3r/
    python3 tools/texbuild/build_textures.py --only light    # recipes whose name matches
    python3 tools/texbuild/build_textures.py --out /tmp/x --sheet /tmp/x/sheet.jpg --orig /tmp/orig

The textures keep the file names and sizes the maps expect, so no map has
to be recompiled. Every pixel comes from the recipes in this folder.
"""
import argparse
import os
import re
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import common  # noqa: E402
import r_metal, r_light, r_cutout, r_stone, r_nature, r_special, r_effects  # noqa: E402,F401
from tex import save  # noqa: E402
import json  # noqa: E402
import numpy as np  # noqa: E402

# mean colour per texture (three numbers each): the new texture is scaled to
# it, so the lighting the maps were built with stays the same
TARGETS = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'targets.json')))
NO_TONE = {f'env/xnight2_{f}.jpg' for f in ('bk', 'dn', 'ft', 'lf', 'rt', 'up')}


def tone(rel, col, alpha):
    t = TARGETS.get(rel)
    if t is None or rel in NO_TONE or getattr(col, 'no_tone', False):
        return col
    wgt = alpha if alpha is not None and alpha.min() < 0.5 and alpha.sum() > 1 else np.ones(col.shape[:2], np.float32)
    tgt = np.maximum(np.array(t, np.float32) / 255, 0.004)
    for _ in range(4):
        m = (col * wgt[..., None]).sum((0, 1)) / wgt.sum()
        col = np.clip(col * (tgt / np.maximum(m, 0.004)), 0, 1)
    return col

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))


def contact_sheet(paths, out, new_dir, orig_dir):
    from PIL import Image, ImageDraw, ImageFont
    T, cols = 256, 4
    try:
        font = ImageFont.truetype('DejaVuSans.ttf', 12)
    except OSError:
        font = ImageFont.load_default()
    rows = (len(paths) + cols - 1) // cols
    S = Image.new('RGB', (cols * (2 * T + 12), rows * (T + 22)), (32, 32, 32))
    d = ImageDraw.Draw(S)

    def tile(p):
        im = Image.open(p).convert('RGBA')
        w, h = im.size
        sc = min(T / w, T / h)
        im = im.resize((max(1, int(w * sc)), max(1, int(h * sc))), Image.LANCZOS)
        bg = Image.new('RGBA', im.size, (255, 0, 255, 255))
        bg.alpha_composite(im)
        return bg.convert('RGB')

    for i, p in enumerate(paths):
        x, y = (i % cols) * (2 * T + 12), (i // cols) * (T + 22)
        o = os.path.join(orig_dir, p) if orig_dir else None
        if o and os.path.exists(o):
            S.paste(tile(o), (x, y))
        S.paste(tile(os.path.join(new_dir, p)), (x + T + 4, y))
        d.text((x, y + T + 3), p.split('/', 1)[1][-58:], fill='white', font=font)
    S.save(out, quality=88)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default=os.path.join(ROOT, 'baseq3r'))
    ap.add_argument('--only', default='')
    ap.add_argument('--sheet')
    ap.add_argument('--orig')
    a = ap.parse_args()
    rx = re.compile(a.only) if a.only else None
    written = []
    for fn in common.RECIPES:
        if rx and not rx.search(fn.__name__):
            continue
        t = time.time()
        res = fn()
        for rel, (col, alpha) in res.items():
            if rx and not (rx.search(fn.__name__) or rx.search(rel)):
                continue
            save(os.path.join(a.out, rel), tone(rel, col, alpha), alpha)
            written.append(rel)
        print(f'{fn.__name__:28s} {len(res)} file(s) {time.time() - t:5.1f}s')
    if a.sheet:
        contact_sheet(written, a.sheet, a.out, a.orig)
    print(len(written), 'textures written to', a.out)


if __name__ == '__main__':
    main()
