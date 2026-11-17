"""Bake the WebUI's CSS backgrounds and its progress bar into menu art.

The native screens draw what the WebUI gets from CSS (garage.css):
  web_stripes.png      body: repeating-linear-gradient(132deg, #0b0b0b 0 12px,
                       #171717 12px 13px), one 13px period over 64 texels;
                       scripts/q3r_webstyle.shader turns the pixel position into
                       the distance along the gradient line
  web_showroom.png     .showroom: radial-gradient(ellipse at center, #38302c,
                       #141414 75%) in box-relative coordinates
  web_progress_*.png   Chrome's dark <progress>, as it paints it on a #171717
                       section: columns 0-3 left cap, 4 middle, 12-15 right cap
"""
from __future__ import annotations
import argparse
import math
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / 'baseq3r/menu/ui'

DARK, LIGHT = 0x0b, 0x17
PERIOD, LIGHT_FROM = 13.0, 12.0

# Captured from Chrome (headless, color-scheme: dark) at 1920x1080, box left
# 222.3, right 554.9; rows top to bottom, columns left to right.
TRACK_LEFT = [  # empty bar, left cap
    [23, 35, 81, 116], [35, 115, 96, 71], [82, 96, 59, 59], [116, 71, 59, 59],
    [116, 71, 59, 59], [82, 96, 59, 59], [35, 115, 95, 71], [23, 35, 81, 116]]
TRACK_MIDDLE = [133, 59, 59, 59, 59, 59, 59, 133]
TRACK_RIGHT = [
    [116, 81, 35, 23], [71, 96, 115, 35], [59, 95, 83, 23], [71, 116, 23, 23],
    [71, 116, 23, 23], [59, 95, 83, 23], [71, 95, 115, 35], [116, 81, 35, 23]]
FILL_LEFT = [
    [(23, 23, 23), (99, 50, 26), (178, 97, 57), (156, 121, 104)],
    [(102, 50, 25), (160, 121, 102), (194, 109, 67), (235, 93, 22)],
    [(179, 98, 57), (194, 109, 67), (255, 85, 0), (255, 85, 0)],
    [(157, 122, 104), (235, 93, 22), (255, 85, 0), (255, 85, 0)],
    [(157, 122, 104), (235, 93, 22), (255, 85, 0), (255, 85, 0)],
    [(179, 98, 57), (194, 109, 67), (255, 85, 0), (255, 85, 0)],
    [(102, 50, 25), (160, 121, 102), (194, 108, 66), (235, 93, 22)],
    [(23, 23, 23), (99, 50, 26), (178, 97, 57), (156, 121, 104)]]
FILL_MIDDLE = [(133, 133, 133)] + [(255, 85, 0)] * 6 + [(133, 133, 133)]
FILL_RIGHT = [  # value at 100%
    [(156, 121, 104), (178, 97, 57), (99, 50, 26), (23, 23, 23)],
    [(235, 93, 22), (194, 109, 67), (160, 121, 102), (102, 50, 25)],
    [(255, 85, 0), (255, 85, 0), (194, 108, 66), (179, 98, 58)],
    [(255, 85, 0), (255, 85, 0), (235, 93, 22), (157, 122, 104)],
    [(255, 85, 0), (255, 85, 0), (235, 93, 22), (157, 122, 104)],
    [(255, 85, 0), (255, 85, 0), (194, 108, 66), (179, 98, 58)],
    [(235, 93, 22), (194, 108, 66), (160, 121, 102), (102, 50, 25)],
    [(156, 121, 104), (178, 97, 57), (99, 50, 26), (23, 23, 23)]]


def stripes() -> Image.Image:
    image = Image.new('RGB', (64, 1))
    for i in range(64):
        start, end = i * PERIOD / 64, (i + 1) * PERIOD / 64
        light = max(0.0, min(end, PERIOD) - max(start, LIGHT_FROM)) / (end - start)
        value = round(DARK + (LIGHT - DARK) * light)
        image.putpixel((i, 0), (value, value, value))
    return image


def showroom(size: int = 512) -> Image.Image:
    inner, outer = (0x38, 0x30, 0x2c), (0x14, 0x14, 0x14)
    image = Image.new('RGB', (size, size))
    pixels = image.load()
    for y in range(size):
        v = ((y + 0.5) / size - 0.5) / (0.5 * math.sqrt(2))
        for x in range(size):
            u = ((x + 0.5) / size - 0.5) / (0.5 * math.sqrt(2))
            t = min(1.0, math.hypot(u, v) / 0.75)
            pixels[x, y] = tuple(round(a + (b - a) * t) for a, b in zip(inner, outer))
    return image


def progress(left, middle, right) -> Image.Image:
    def rgb(value):
        return value if isinstance(value, tuple) else (value, value, value)
    image = Image.new('RGB', (16, 8), (23, 23, 23))
    for y in range(8):
        for x in range(4):
            image.putpixel((x, y), rgb(left[y][x]))
            image.putpixel((12 + x, y), rgb(right[y][x]))
        for x in range(4, 12):
            image.putpixel((x, y), rgb(middle[y]))
    return image


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--output', type=Path, default=OUTPUT)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    stripes().save(args.output / 'web_stripes.png', optimize=True)
    showroom().save(args.output / 'web_showroom.png', optimize=True)
    progress(TRACK_LEFT, TRACK_MIDDLE, TRACK_RIGHT).save(args.output / 'web_progress_track.png', optimize=True)
    progress(FILL_LEFT, FILL_MIDDLE, FILL_RIGHT).save(args.output / 'web_progress_fill.png', optimize=True)


if __name__ == '__main__':
    main()
