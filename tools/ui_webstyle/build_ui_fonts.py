"""Bake the WebUI fonts into atlases the native menus draw with.

The native screens must look like the WebUI, so text is laid out the way
Chrome does it: linear (unhinted) advances and GPOS pair kerning from the
font, at any pixel size. The glyph images come from FreeType at a few fixed
sizes; the UI picks the next larger one and scales down. Bold and italic the
WebUI does not ship as files are synthesised the way Skia does it
(outline embolden by em/24, skew 1/4).

Output, per variant, into baseq3r/menu/fonts/:
  <variant>.dat         metrics, kerning, glyph rectangles for every size
  <variant>_<px>.png    one white/alpha atlas per size

File format (little endian int32 unless noted), version 2:
  'Q3UF' version upm ascent descent glyphCount kernCount sizeCount
  glyphCount x { codepoint advance }                       advance in font units
  kernCount  x { (left << 16) | right  adjust }            sorted, font units
  ligatureCount
  ligatureCount x { codepoint  characters[4 bytes] }       the 'liga' feature, in font order
  sizeCount  x { size*100 width height image[64 bytes]
                 glyphCount x { x y w h left top } }        pixels, top above baseline
"""
from __future__ import annotations
import argparse
import ctypes
import struct
from pathlib import Path

import freetype
from fontTools.ttLib import TTFont
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
FONTS = ROOT / 'launcher/assets/fonts'
OUTPUT = ROOT / 'baseq3r/menu/fonts'

CHARS = [c for c in range(32, 127)] + [c for c in range(160, 256)]
CONDENSED = [20.8, 24, 28, 32, 38, 44, 48, 56, 66, 78, 92, 112]
VARIANTS = {
    # name: (file, sizes, embolden, oblique); the condensed face only ever
    # shows uppercased text (h1, h2, navigation), so it carries no lowercase
    'sharetechmono':          ('ShareTechMono-Regular.ttf', [12.8], False, False),
    'barlow':                 ('Barlow-Regular.ttf', [13.6, 16], False, False),
    'barlow_bold':            ('Barlow-Regular.ttf', [16], True, False),   # font-weight 750 on a regular face
    'barlowcondensed':        ('BarlowCondensed-Bold.ttf', CONDENSED, False, False),
    'barlowcondensed_italic': ('BarlowCondensed-Bold.ttf', CONDENSED[6:], False, True),
}
PADDING = 2
# Chrome draws list markers with tabular figures (::marker { font-variant-numeric:
# tabular-nums }); the body faces carry them at 0x80-0x89, unused in Latin-1.
TABULAR_BASE = 0x80
# Chrome applies the standard ligatures (fi, fl, ij, ...) to text without
# letter-spacing; their glyphs follow at 0x8A, also unused in Latin-1.
LIGATURE_BASE = 0x8A


def kerning(font: TTFont, glyph_for: dict[int, str]) -> dict[tuple[int, int], int]:
    """Pair adjustments of the 'kern' feature, first match wins like HarfBuzz."""
    wanted = {name: code for code, name in glyph_for.items()}
    table = font['GPOS'].table if 'GPOS' in font else None
    pairs: dict[tuple[int, int], int] = {}
    if not table:
        return pairs
    lookups = sorted({index for record in table.FeatureList.FeatureRecord if record.FeatureTag == 'kern'
                      for index in record.Feature.LookupListIndex})
    for index in lookups:
        lookup = table.LookupList.Lookup[index]
        for sub in lookup.SubTable:
            if lookup.LookupType == 9:
                sub = sub.ExtSubTable
            if getattr(sub, 'LookupType', 2) != 2:
                continue
            coverage = sub.Coverage.glyphs
            if sub.Format == 1:
                for i, first in enumerate(coverage):
                    if first not in wanted:
                        continue
                    for record in sub.PairSet[i].PairValueRecord:
                        value = getattr(record.Value1, 'XAdvance', 0) if record.Value1 else 0
                        if value and record.SecondGlyph in wanted:
                            pairs.setdefault((wanted[first], wanted[record.SecondGlyph]), value)
            elif sub.Format == 2:
                second_classes: dict[int, list[str]] = {}
                for name, cls in sub.ClassDef2.classDefs.items():
                    if name in wanted:
                        second_classes.setdefault(cls, []).append(name)
                for first in coverage:
                    if first not in wanted:
                        continue
                    row = sub.Class1Record[sub.ClassDef1.classDefs.get(first, 0)]
                    for cls, record in enumerate(row.Class2Record):
                        value = getattr(record.Value1, 'XAdvance', 0) if record.Value1 else 0
                        if not value or not cls:
                            continue
                        for second in second_classes.get(cls, []):
                            pairs.setdefault((wanted[first], wanted[second]), value)
    return pairs


def tabular_digits(font: TTFont) -> dict[int, str]:
    cmap = font.getBestCmap()
    table = font['GSUB'].table if 'GSUB' in font else None
    if not table:
        return {}
    for record in table.FeatureList.FeatureRecord:
        if record.FeatureTag != 'tnum':
            continue
        for index in record.Feature.LookupListIndex:
            for sub in table.LookupList.Lookup[index].SubTable:
                mapping = getattr(sub, 'mapping', None) or getattr(getattr(sub, 'ExtSubTable', None), 'mapping', {})
                digits = {TABULAR_BASE + d: mapping.get(cmap[ord('0') + d]) for d in range(10)}
                if all(digits.values()):
                    return digits
    return {}


def ligatures(font: TTFont, glyph_for: dict[int, str]) -> list[tuple[str, str]]:
    """The 'liga' ligatures of the default Latin language whose characters the
    atlas carries, in the font's order: HarfBuzz takes the first that matches."""
    table = font['GSUB'].table if 'GSUB' in font else None
    if not table:
        return []
    names = {name: chr(code) for code, name in glyph_for.items() if code < TABULAR_BASE or code >= 0xa0}
    script = next((record.Script for record in table.ScriptList.ScriptRecord if record.ScriptTag == 'latn'), None)
    if script is None or script.DefaultLangSys is None:
        return []
    found = []
    for index in script.DefaultLangSys.FeatureIndex:
        record = table.FeatureList.FeatureRecord[index]
        if record.FeatureTag != 'liga':
            continue
        for lookup_index in record.Feature.LookupListIndex:
            lookup = table.LookupList.Lookup[lookup_index]
            for sub in lookup.SubTable:
                if lookup.LookupType == 7:
                    sub = sub.ExtSubTable
                for first, entries in getattr(sub, 'ligatures', {}).items():
                    for entry in entries:
                        parts = [first] + list(entry.Component)
                        if all(part in names for part in parts) and len(parts) <= 4:
                            found.append((''.join(names[part] for part in parts), entry.LigGlyph))
    return found


def raster(face: freetype.Face, glyph: int, size: float, embolden: bool, oblique: bool):
    face.set_char_size(int(round(size * 64)))
    if oblique:
        # Skia's fake italic: x' = x + y/4 (y up)
        face.set_transform(freetype.Matrix(0x10000, 0x4000, 0, 0x10000), freetype.Vector(0, 0))
    else:
        face.set_transform(freetype.Matrix(0x10000, 0, 0, 0x10000), freetype.Vector(0, 0))
    face.load_glyph(glyph, freetype.FT_LOAD_NO_BITMAP | freetype.FT_LOAD_TARGET_LIGHT)
    slot = face.glyph
    if embolden:
        strength = int(round(size * 64 / 24))
        freetype.FT_Outline_Embolden(ctypes.byref(slot.outline._FT_Outline), strength)
    slot.render(freetype.FT_RENDER_MODE_NORMAL)
    bitmap = slot.bitmap
    pixels = bytes(bitmap.buffer)
    image = None
    if bitmap.width and bitmap.rows:
        image = Image.new('L', (bitmap.width, bitmap.rows))
        image.putdata([pixels[y * bitmap.pitch + x] for y in range(bitmap.rows) for x in range(bitmap.width)])
    return image, slot.bitmap_left, slot.bitmap_top


def pack(images: list) -> tuple[int, int, list[tuple[int, int]]]:
    """Shelf packing into the smallest square-ish power-of-two texture."""
    width = 64
    while True:
        x = y = shelf = 0
        places = []
        fits = True
        for image in images:
            w, h = (image.size if image else (0, 0))
            if not w:
                places.append((0, 0))
                continue
            if x + w + PADDING > width:
                x, y, shelf = 0, y + shelf + PADDING, 0
            places.append((x + PADDING, y + PADDING))
            x += w + PADDING
            shelf = max(shelf, h)
            if y + shelf + 2 * PADDING > width:
                fits = False
                break
        if fits:
            height = 64
            while height < y + shelf + 2 * PADDING:
                height *= 2
            return width, height, places
        width *= 2


def build(name: str, output: Path) -> None:
    filename, sizes, embolden, oblique = VARIANTS[name]
    path = FONTS / filename
    font = TTFont(path)
    cmap = font.getBestCmap()
    codes = [code for code in CHARS if code in cmap
             and (not name.startswith('barlowcondensed') or chr(code).upper() == chr(code))]
    glyph_for = {code: cmap[code] for code in codes}
    if not name.startswith('barlowcondensed'):
        glyph_for.update(tabular_digits(font))
    ligas = ligatures(font, glyph_for)
    for index, (_, glyph) in enumerate(ligas):
        glyph_for[LIGATURE_BASE + index] = glyph
    codes = sorted(glyph_for)
    upm = font['head'].unitsPerEm
    hhea = font['hhea']
    kerns = sorted(kerning(font, glyph_for).items(), key=lambda item: (item[0][0] << 16) | item[0][1])
    face = freetype.Face(str(path))

    data = bytearray(b'Q3UF')
    data += struct.pack('<6i', 2, upm, hhea.ascent, -hhea.descent, len(codes), len(kerns))
    data += struct.pack('<i', len(sizes))
    for code in codes:
        data += struct.pack('<2i', code, font['hmtx'][glyph_for[code]][0])
    for (left, right), value in kerns:
        data += struct.pack('<2i', (left << 16) | right, value)
    data += struct.pack('<i', len(ligas))
    for index, (characters, _) in enumerate(ligas):
        data += struct.pack('<i4s', LIGATURE_BASE + index, characters.encode('latin-1'))
    for size in sizes:
        rendered = [raster(face, font.getGlyphID(glyph_for[code]), size, embolden, oblique) for code in codes]
        width, height, places = pack([image for image, _, _ in rendered])
        atlas = Image.new('L', (width, height), 0)
        for (image, _, _), place in zip(rendered, places):
            if image:
                atlas.paste(image, place)
        label = f'{name}_{int(round(size * 10))}'
        rgba = Image.merge('RGBA', (Image.new('L', atlas.size, 255),) * 3 + (atlas,))
        rgba.save(output / f'{label}.png', optimize=True)
        shader = f'menu/fonts/{label}'.encode()
        data += struct.pack('<3i64s', int(round(size * 100)), width, height, shader)
        for (image, left, top), (x, y) in zip(rendered, places):
            w, h = image.size if image else (0, 0)
            data += struct.pack('<6i', x, y, w, h, left, top)
    (output / f'{name}.dat').write_bytes(data)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--output', type=Path, default=OUTPUT)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    for name in VARIANTS:
        build(name, args.output)


if __name__ == '__main__':
    main()
