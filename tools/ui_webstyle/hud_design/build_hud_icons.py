"""Draws the HUD design's stroke icons for the game.

python3 build_hud_icons.py

Writes baseq3r/menu/art/hud_<name>.png: white strokes on transparent, 64x64,
drawn by Chrome from the same SVG the design shows (icons.py). The HUD tints
them (cgame/cg_hud_modern.c).
"""
import pathlib
import sys

from playwright.sync_api import sync_playwright

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from icons import ICON_PATHS  # noqa: E402

ROOT = HERE.parents[2]
OUT = ROOT / 'baseq3r/menu/art'
# icon name in the design -> file name in the game
WANTED = {'note': 'hud_note', 'prev': 'hud_prev', 'play': 'hud_play', 'next': 'hud_next',
          'chevron': 'hud_chevron', 'msg': 'hud_message', 'gun': 'hud_gun', 'mine': 'hud_mine',
          'smoke': 'hud_smoke', 'flag': 'hud_flag', 'hill': 'hud_hill', 'trophy': 'hud_trophy'}
# not design icons: shapes the game tints (the drag tree's lamps)
SHAPES = {'hud_dot': '<circle cx="12" cy="12" r="11.5" fill="#ffffff" stroke="none"/>'}
SIZE = 64


def main():
    with sync_playwright() as p:
        browser = p.chromium.launch(executable_path='/usr/bin/google-chrome')
        page = browser.new_page(viewport={'width': SIZE, 'height': SIZE}, device_scale_factor=1)
        for name, shape in list(SHAPES.items()) + [(name, ICON_PATHS[kind]) for kind, name in WANTED.items()]:
            page.set_content(
                '<html><body style="margin:0;background:transparent">'
                f'<svg width="{SIZE}" height="{SIZE}" viewBox="0 0 24 24" fill="none" stroke="#ffffff" '
                f'stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">{shape}</svg>'
                '</body></html>')
            page.screenshot(path=str(OUT / f'{name}.png'), omit_background=True)
            print(name)
        browser.close()


if __name__ == '__main__':
    main()
