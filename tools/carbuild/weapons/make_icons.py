"""The weapon icons (icons/iconw_*.tga): one pictogram per weapon, the
typical car part of its model, in the weapon's old colour so it is still
recognised at a glance (the HUD shows it at 12 x 12 px).

The pictograms are SVG on a 64 grid; Chromium (Playwright) renders them to
256 x 256 RGBA, the shape colour through a mask so cut-outs are real alpha.

The same pictograms go on the ammo boxes: models/powerups/ammo/*ammo2.tga
(128 x 128, black field in a frame of the weapon colour, as before).

  python3 tools/carbuild/weapons/make_icons.py
"""
from __future__ import annotations

import io
import math
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "baseq3r/icons"
SIZE = 256

# icon file -> colour (the old colour code of the Q3Rally icons)
COLOURS = {
    "gauntlet": "#22C8FF",      # Harvester Saw
    "machinegun": "#FFE21A",    # Spotlight MG
    "shotgun": "#FF6A00",       # Twin Shotgun
    "grenade": "#16B04A",       # Drum Launcher
    "rocket": "#FF2A2A",        # Quad Launcher
    "lightning": "#FFF27A",     # Spark Gun
    "railgun": "#2BEA4E",       # Turbo Railgun
    "plasma": "#FF2EF0",        # Plasma Cooler
    "bfg": "#2F7BFF",           # BFG Blower
    "flame": "#FF5A1F",         # Nitro Flamer
}

# Pictograms on a 64 grid. {S} = solid colour, {K} = cut-out colour.
P = {}
P["gauntlet"] = """
<g transform="rotate(-30 34 30)">
 <rect x="14" y="23" width="48" height="16" rx="8" fill="{S}"/>
 {teeth}
 <rect x="24" y="29.6" width="33" height="2.8" rx="1.4" fill="{K}"/>
</g>
<rect x="3" y="33" width="26" height="26" rx="6" fill="{S}"/>
<circle cx="16" cy="46" r="7.5" fill="{K}"/><circle cx="16" cy="46" r="3" fill="{S}"/>
""".replace("{teeth}", "".join(
    f'<polygon points="{x},23 {x+3},18.5 {x+6},23" fill="{{S}}"/><polygon points="{x},39 {x+3},43.5 {x+6},39" fill="{{S}}"/>'
    for x in range(22, 56, 7)))
P["machinegun"] = """
<rect x="25" y="52" width="14" height="10" rx="2" fill="{S}"/>
<circle cx="32" cy="30" r="27" fill="{S}"/><circle cx="32" cy="30" r="20.5" fill="{K}"/>
<circle cx="32" cy="30" r="16.5" fill="{S}"/>
""" + "".join(f'<circle cx="{32+8.5*math.cos(a):.2f}" cy="{30+8.5*math.sin(a):.2f}" r="4.3" fill="{{K}}"/>'
              for a in (math.radians(-90), math.radians(30), math.radians(150)))
def tip(y):
    return (f'<circle cx="10" cy="{y+8}" r="8" fill="{{S}}"/><polygon points="10,{y} 61,{y} 48,{y+16} 10,{y+16}" fill="{{S}}"/>'
            f'<ellipse cx="54.5" cy="{y+8}" rx="8" ry="3.4" transform="rotate(-53 54.5 {y+8})" fill="{{K}}"/>'
            f'<rect x="22" y="{y-1}" width="3.5" height="18" fill="{{K}}"/>')
P["shotgun"] = tip(10) + tip(37)
P["grenade"] = """
<circle cx="28" cy="36" r="25" fill="{S}"/>
""" + "".join(f'<circle cx="{28+18*math.cos(2*math.pi*k/10):.2f}" cy="{36+18*math.sin(2*math.pi*k/10):.2f}" r="2.6" fill="{{K}}"/>'
              for k in range(10)) + """
<circle cx="28" cy="36" r="11" fill="{K}"/><circle cx="28" cy="36" r="8.5" fill="{S}"/><circle cx="28" cy="36" r="2.6" fill="{K}"/>
<rect x="38" y="3" width="23" height="24" rx="7" fill="{S}"/>
<rect x="36" y="1" width="27" height="28" rx="8" fill="none" stroke="{K}" stroke-width="2.5"/>
<rect x="38" y="3" width="23" height="24" rx="7" fill="{S}"/>
"""
P["rocket"] = '<rect x="3" y="7" width="58" height="50" rx="10" fill="{S}"/>' + "".join(
    f'<circle cx="{x}" cy="{y}" r="10" fill="{{K}}"/><circle cx="{x}" cy="{y}" r="7.5" fill="{{S}}"/><circle cx="{x}" cy="{y}" r="4" fill="{{K}}"/>'
    for x in (18, 46) for y in (21, 43))
P["lightning"] = """
<g transform="rotate(-40 26 26)">
 <rect x="22.5" y="-1" width="7" height="7" rx="1.5" fill="{S}"/>
 <rect x="19" y="5" width="14" height="18" rx="5" fill="{S}"/>
 <rect x="19" y="10" width="14" height="2.2" fill="{K}"/><rect x="19" y="15" width="14" height="2.2" fill="{K}"/>
 <rect x="15" y="23" width="22" height="9" rx="1.5" fill="{S}"/>
 <rect x="20" y="32" width="12" height="10" fill="{S}"/>
 <rect x="20" y="35" width="12" height="1.8" fill="{K}"/><rect x="20" y="38.5" width="12" height="1.8" fill="{K}"/>
 <rect x="24.5" y="42" width="3" height="6" fill="{S}"/>
</g>
<polygon points="44,40 57,44 51,48 63,63 45,51 51,47" fill="{S}"/>
"""
P["railgun"] = """
<circle cx="20" cy="40" r="18" fill="{S}"/>
<circle cx="20" cy="40" r="9.5" fill="{K}"/><circle cx="20" cy="40" r="3.5" fill="{S}"/>
<rect x="20" y="22" width="16" height="15" rx="3" fill="{S}"/>
<rect x="32" y="22" width="30" height="5.5" rx="2" fill="{S}"/>
<rect x="32" y="31.5" width="30" height="5.5" rx="2" fill="{S}"/>
<rect x="41" y="18" width="4.5" height="23" rx="1.5" fill="{S}"/>
<rect x="53" y="18" width="4.5" height="23" rx="1.5" fill="{S}"/>
"""
P["plasma"] = """
<path d="M9,16 C9,4 22,3 30,3 C40,3 45,8 45,18" fill="none" stroke="{S}" stroke-width="5" stroke-linecap="round"/>
<rect x="3" y="16" width="35" height="34" rx="3" fill="{S}"/>
""" + "".join(f'<rect x="{x}" y="21" width="2.8" height="24" rx="1" fill="{{K}}"/>' for x in (8, 14, 20, 26, 32)) + """
<polygon points="38,16 48,24 48,42 38,50" fill="{S}"/>
<rect x="47" y="26" width="9" height="14" rx="2" fill="{S}"/>
<rect x="49.5" y="30" width="4" height="6" rx="1" fill="{K}"/>
<circle cx="61" cy="33" r="3" fill="{S}"/>
"""
P["bfg"] = """
<rect x="5" y="32" width="54" height="28" rx="9" fill="{S}"/>
""" + "".join(f'<rect x="{x}" y="37" width="3" height="18" rx="1" fill="{{K}}"/>' for x in (14, 22, 30, 38, 46)) + """
<polygon points="7,4 57,4 50,32 14,32" fill="{S}"/>
<rect x="16" y="10" width="32" height="3.5" rx="1" fill="{K}"/>
<rect x="17.5" y="17" width="29" height="3.5" rx="1" fill="{K}"/>
<rect x="19" y="24" width="26" height="3.5" rx="1" fill="{K}"/>
"""
P["flame"] = """
<rect x="3" y="16" width="22" height="44" rx="11" fill="{S}"/>
<rect x="3" y="31" width="22" height="9" fill="{K}"/>
<rect x="3" y="33.5" width="22" height="4" fill="{S}"/>
<rect x="9" y="7" width="10" height="10" rx="2" fill="{S}"/>
<rect x="22" y="52" width="14" height="4.5" fill="{S}"/>
<rect x="33" y="50" width="25" height="10" rx="2.5" fill="{S}"/>
<path d="M46,50 C36,50 31,44 31,36 C31,27 37,22 38,14 C42,19 43,23 42,28 C46,23 49,15 47,3 C55,10 62,21 62,34 C62,44 56,50 46,50 Z" fill="{S}"/>
<path d="M46,47 C41,47 38,43 38,39 C38,34 41,32 42,28 C44,32 46,34 45,37 C48,35 50,31 50,27 C54,31 56,35 56,39 C56,44 51,47 46,47 Z" fill="{K}"/>
"""



def picto(key, s, k):
    return P[key].replace("{S}", s).replace("{K}", k)


def icon_svg(key, px=SIZE):
    """The icon as a standalone SVG: colour through a luminance mask (white
    = shape, black = cut-out), transparent elsewhere."""
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="{px}" height="{px}">'
            f'<defs><mask id="m" maskUnits="userSpaceOnUse" x="0" y="0" width="64" height="64">'
            f'<rect width="64" height="64" fill="#000"/>{picto(key, "#FFF", "#000")}</mask></defs>'
            f'<rect width="64" height="64" fill="{COLOURS[key]}" mask="url(#m)"/></svg>')


# ammo box label -> icon
AMMO = {"mach": "machinegun", "shot": "shotgun", "gren": "grenade", "rock": "rocket", "ligh": "lightning",
        "rail": "railgun", "plas": "plasma", "bfg": "bfg", "flame": "flame"}
AMMO_DIR = ROOT / "baseq3r/models/powerups/ammo"
AMMO_SIZE = 128


def ammo_svg(key):
    """The label on the ammo box: black field, 5 px frame in the weapon
    colour, the pictogram inside."""
    col = COLOURS[key]
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128" width="{AMMO_SIZE}" height="{AMMO_SIZE}">'
            f'<defs><mask id="m" maskUnits="userSpaceOnUse" x="0" y="0" width="64" height="64">'
            f'<rect width="64" height="64" fill="#000"/>{picto(key, "#FFF", "#000")}</mask></defs>'
            f'<rect width="128" height="128" fill="{col}"/><rect x="5" y="5" width="118" height="118" fill="#000"/>'
            f'<g transform="translate(20 20) scale(1.375)"><rect width="64" height="64" fill="{col}" mask="url(#m)"/></g>'
            f'</svg>')


def render_all():
    from playwright.sync_api import sync_playwright
    with sync_playwright() as pw:
        browser = pw.chromium.launch()
        page = browser.new_page(viewport={"width": SIZE, "height": SIZE})
        for key in COLOURS:
            page.set_content(f'<html><body style="margin:0;background:transparent">{icon_svg(key)}</body></html>')
            png = page.screenshot(omit_background=True, clip={"x": 0, "y": 0, "width": SIZE, "height": SIZE})
            im = Image.open(io.BytesIO(png)).convert("RGBA")
            im.save(OUT / f"iconw_{key}.tga")
            print(f"iconw_{key}.tga")
        page.set_viewport_size({"width": AMMO_SIZE, "height": AMMO_SIZE})
        for box, key in AMMO.items():
            page.set_content(f'<html><body style="margin:0;background:#000">{ammo_svg(key)}</body></html>')
            png = page.screenshot(clip={"x": 0, "y": 0, "width": AMMO_SIZE, "height": AMMO_SIZE})
            im = Image.open(io.BytesIO(png)).convert("RGBA")
            im.save(AMMO_DIR / f"{box}ammo2.tga")
            print(f"{box}ammo2.tga")
        browser.close()


if __name__ == "__main__":
    render_all()
