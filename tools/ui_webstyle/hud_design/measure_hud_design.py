"""Measures the HUD design pages in Chrome.

python3 measure_hud_design.py <board> <fixture> [screenshot.png]

Writes every element of the board's 1600x900 root as [x, y, width, height]
(getBoundingClientRect), keyed by its path of tag:index from the root, e.g.
"/div:2/div:0/div:1". The native layout is held against these boxes.
"""
import json
import pathlib
import subprocess
import sys
import tempfile

from playwright.sync_api import sync_playwright

HERE = pathlib.Path(__file__).resolve().parent

SCRIPT = '''() => {
  const root = document.body.querySelector('div');
  const out = {};
  const walk = (el, path) => {
    const r = el.getBoundingClientRect();
    out[path] = [r.x, r.y, r.width, r.height];
    [...el.children].forEach((c, i) => walk(c, path + '/' + c.tagName.toLowerCase() + ':' + i));
  };
  walk(root, '');
  return out;
}'''


def main():
    board, fixture = sys.argv[1], pathlib.Path(sys.argv[2])
    with tempfile.TemporaryDirectory() as out:
        subprocess.run([sys.executable, str(HERE / 'gen.py'), 'local', out], check=True, capture_output=True)
        page_file = pathlib.Path(out) / 'project' / board
        with sync_playwright() as p:
            browser = p.chromium.launch(executable_path='/usr/bin/google-chrome')
            page = browser.new_page(viewport={'width': 1600, 'height': 900}, device_scale_factor=1)
            page.goto(page_file.as_uri())
            page.evaluate('document.fonts.ready')
            page.wait_for_timeout(300)
            boxes = page.evaluate(SCRIPT)
            if len(sys.argv) > 3:
                page.screenshot(path=sys.argv[3])
            browser.close()
    fixture.write_text(json.dumps({'board': board, 'viewport': [1600, 900], 'els': boxes}, indent=0) + '\n')
    print(len(boxes), 'boxes')


if __name__ == '__main__':
    main()
