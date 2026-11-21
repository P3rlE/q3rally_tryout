"""The Q3Rally HUD design (v0.9): writes the artboards of the design canvas.

python3 gen.py canvas <dir>   artboards for the Design canvas (uploaded fonts and scenes)
python3 gen.py local <dir>    the same pages for Chrome on this machine, without the
                              canvas runtime (measure_hud_design.py measures them)

The native HUD (engine/code/cgame/cg_hud_modern*.c) follows these pages; the
measurements are in tests/fixtures/hud_*_layout.json.
"""
import json, os, sys, datetime, pathlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from icons import ICON_PATHS  # noqa: E402

MODE = sys.argv[1] if len(sys.argv) > 1 else 'canvas'
OUT = os.path.join(sys.argv[2] if len(sys.argv) > 2 else 'out', 'project')
os.makedirs(OUT, exist_ok=True)
ROOT = pathlib.Path(__file__).resolve().parents[3]

FONT = {
    'barlow': '/_blob/93ccf45061215c5e8036dd1e87942434',
    'cond': '/_blob/9570036df42d1728696ae1ab426adafd',
    'mono': '/_blob/d169f0f60fa58b382367710e3e5c4fd8',
}
if MODE == 'local':
    FONTS = ROOT / 'launcher/assets/fonts'
    FONT = {'barlow': (FONTS / 'Barlow-Regular.ttf').as_uri(), 'cond': (FONTS / 'BarlowCondensed-Bold.ttf').as_uri(),
            'mono': (FONTS / 'ShareTechMono-Regular.ttf').as_uri()}
BG = {
    'racing_dm': '/_blob/d7d5f911462e5fcc92a7da143cdfc2dd',
    'sprint': '/_blob/56e90a94d9e696194ef52eacd082077f',
    'racing': '/_blob/21e7c1fb0ea58dbcbce6ca7b5a8fc5fd',
    'dm': '/_blob/751c72547bae8969962d7983aca3bf0c',
    'team_dm': '/_blob/191968115b156acf1f13afc9119ac285',
    'koth': '/_blob/48094ac0ae908988b5e9ebe14b4674d7',
    'autoball': '/_blob/7a7a65e9bbf0dadb6daa935d79509eb8',
    'drag': '/_blob/239e6c46001c7cc303ce14c7345c601c',
    'lcs': '/_blob/d4f0e53d72925faefab6377c7128167a',
}
if MODE == 'local':
    # HUD_SCENES: a folder with bg_<name>.jpg to show the scenes (comparison pictures)
    SCENES = os.environ.get('HUD_SCENES')
    BG = {k: (pathlib.Path(SCENES, f'bg_{k}.jpg').as_uri() if SCENES else None) for k in BG}

BODY = "'Barlow', sans-serif"
COND = "'Barlow Condensed', sans-serif"
MONO = "'Share Tech Mono', monospace"
ACC = '{{accent}}'
PANEL = '{{panel}}'
LINE = '#373737'
TEAM = {'red': '#f43d38', 'blue': '#478fff', 'green': '#2ee052', 'yellow': '#ffd129'}
FAST = '#5ab0ff'          # ahead / faster: blue (tells apart from the orange accent)


# ---------------------------------------------------------------- atoms
def lab(t, color='#bbbbbb', size=12, extra=''):
    return (f'<div style="font-family: {MONO}; font-size: {size}px; letter-spacing: 0.16em; '
            f'text-transform: uppercase; color: {color}; white-space: nowrap;{extra}">{t}</div>')


def big(t, size, color='#ffffff', extra=''):
    return (f'<div style="font-family: {COND}; font-weight: 700; font-style: italic; font-size: {size}px; '
            f'line-height: 1; color: {color}; white-space: nowrap;{extra}">{t}</div>')


def mono(t, size=16, color='#ffffff', extra=''):
    return f'<div style="font-family: {MONO}; font-size: {size}px; color: {color}; white-space: nowrap;{extra}">{t}</div>'


def text(t, size=15, color='#ffffff', weight=400, extra=''):
    return (f'<div style="font-family: {BODY}; font-size: {size}px; font-weight: {weight}; color: {color}; '
            f'white-space: nowrap; overflow: hidden; text-overflow: ellipsis;{extra}">{t}</div>')


def bar(frac, color='#e6e6e6', h=4, track='#2c2c2c'):
    return (f'<div style="height: {h}px; background: {track}; position: relative">'
            f'<div style="position: absolute; left: 0; top: 0; bottom: 0; width: {frac * 100:.0f}%; background: {color}"></div></div>')


def row(left, right, extra=''):
    return (f'<div style="display: flex; justify-content: space-between; align-items: baseline; gap: 12px;{extra}">'
            f'{left}{right}</div>')


def sep():
    return f'<div style="height: 1px; background: {LINE}"></div>'


def panel(inner, pad='14px 16px', accent=True, gap=10, extra=''):
    top = f'border-top: 2px solid {ACC};' if accent else ''
    return (f'<div style="background: {PANEL}; border: 1px solid {LINE}; {top} padding: {pad}; box-sizing: border-box; '
            f'display: flex; flex-direction: column; gap: {gap}px;{extra}">{inner}</div>')


def col(pos, inner, width, gap=10, align='stretch'):
    return (f'<div style="position: absolute; {pos}; width: {width}px; display: flex; flex-direction: column; '
            f'gap: {gap}px; align-items: {align}">{inner}</div>')


def swatch(color, size=8):
    return f'<div style="width: {size}px; height: {size}px; background: {color}; flex: none"></div>'


# ---------------------------------------------------------------- icons (stroke SVG)
def icon(kind, size=20, color='#ffffff'):
    p = ICON_PATHS[kind]
    return (f'<svg width="{size}" height="{size}" viewBox="0 0 24 24" fill="none" stroke="{color}" stroke-width="1.8" '
            f'stroke-linecap="round" stroke-linejoin="round" aria-hidden="true" style="flex: none">{p}</svg>')


# ---------------------------------------------------------------- modules
def cockpit(speed, gear, rpm, nos, fuel, unit='MPH'):
    segs = []
    lit = round(rpm * 20)
    for i in range(20):
        if i < lit:
            c = ACC if i >= 16 else '#f2f2f2'
        else:
            c = '#2c2c2c'
        segs.append(f'<div style="flex: 1; height: 10px; background: {c}"></div>')
    top = (f'<div style="display: flex; justify-content: space-between; align-items: flex-end">'
           f'<div style="display: flex; align-items: baseline; gap: 8px">{big(speed, 74)}{lab(unit, "#bbbbbb", 13)}</div>'
           f'<div style="display: flex; flex-direction: column; align-items: center; gap: 4px">{lab("GEAR", "#999999", 11)}'
           f'<div style="width: 58px; height: 52px; border: 1px solid {LINE}; display: flex; align-items: center; '
           f'justify-content: center">{big(gear, 40)}</div></div></div>')
    rpmrow = f'<div style="display: flex; gap: 3px">{"".join(segs)}</div>'
    gauges = (f'<div style="display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 16px">'
              f'<div style="display: flex; flex-direction: column; gap: 6px">{row(lab("NOS"), mono(f"{nos * 100:.0f}%", 13, "#ffffff"))}{bar(nos, ACC)}</div>'
              f'<div style="display: flex; flex-direction: column; gap: 6px">{row(lab("FUEL"), mono(f"{fuel * 100:.0f}%", 13, "#ffffff"))}{bar(fuel)}</div>'
              f'</div>')
    return panel(top + rpmrow + gauges, gap=12)


def slipstream(frac):
    return panel(row(lab('SLIPSTREAM', FAST), mono(f'+{frac * 12:.0f}%', 13, FAST)) + bar(frac, FAST),
                 pad='10px 16px', accent=False, gap=6)


def minimap(dots=((150, 118, '#ffffff'), (60, 175, '#ffffff'), (35, 110, '#ffffff')), me=(108, 46), size=176):
    d = ''.join(f'<circle cx="{x}" cy="{y}" r="5" fill="{c}" stroke="#111111" stroke-width="2"/>' for x, y, c in dots)
    svg = (f'<svg width="{size}" height="{size}" viewBox="0 0 200 200" aria-hidden="true">'
           f'<path d="M34 150 C18 92 58 38 110 44 C162 50 180 92 152 120 C130 142 172 162 150 182 C120 204 52 198 34 150 Z" '
           f'fill="none" stroke="#5a5a5a" stroke-width="12" stroke-linejoin="round"/>'
           f'<path d="M34 150 C18 92 58 38 110 44 C162 50 180 92 152 120 C130 142 172 162 150 182 C120 204 52 198 34 150 Z" '
           f'fill="none" stroke="#d9d9d9" stroke-width="3" stroke-linejoin="round"/>'
           f'<path d="M26 128 L44 124" stroke="#ffffff" stroke-width="4"/>'
           f'{d}<circle cx="{me[0]}" cy="{me[1]}" r="8" fill="{ACC}" stroke="#111111" stroke-width="2"/></svg>')
    return (f'<div style="background: {PANEL}; border: 1px solid {LINE}; padding: 6px; box-sizing: border-box; '
            f'align-self: flex-end; display: flex">{svg}</div>')


def vehicle(integrity, armor=None, weapons=None, item=None):
    low = integrity < 35
    head = (f'<div style="display: flex; justify-content: space-between; align-items: flex-end">'
            f'<div style="display: flex; flex-direction: column; gap: 4px">{lab("INTEGRITY")}{big(integrity, 40, ACC if low else "#ffffff")}</div>')
    if armor is not None:
        head += f'<div style="display: flex; flex-direction: column; gap: 4px; align-items: flex-end">{lab("ARMOR")}{big(armor, 28, "#dddddd")}</div>'
    head += '</div>'
    inner = head + bar(min(integrity, 125) / 125, ACC if low else '#f2f2f2')
    slots = []
    for slot, kind, name, ammo in (weapons or []):
        slots.append(f'<div style="display: flex; flex-direction: column; gap: 6px; min-width: 0">{lab(slot, "#999999", 11)}'
                     f'<div style="display: flex; align-items: center; gap: 8px">{icon(kind, 20)}{big(ammo, 24)}</div>'
                     f'{text(name, 12, "#bbbbbb")}</div>')
    if item:
        slots.append(f'<div style="display: flex; flex-direction: column; gap: 6px; min-width: 0">{lab("ITEM", "#999999", 11)}'
                     f'<div style="display: flex; align-items: center; gap: 8px">{icon(item[0], 20, ACC)}{big(item[2], 24, ACC)}</div>'
                     f'{text(item[1], 12, "#bbbbbb")}</div>')
    if slots:
        inner += sep() + (f'<div style="display: grid; grid-template-columns: repeat({len(slots)}, minmax(0, 1fr)); gap: 12px">'
                          + ''.join(slots) + '</div>')
    return panel(inner, gap=10)


def jukebox(artist='Artist', title='Track title', t='1:42', total='3:58', frac=0.43, expanded=False):
    if not expanded:
        return (f'<div style="background: {PANEL}; border: 1px solid {LINE}; box-sizing: border-box; display: flex; '
                f'flex-direction: column; align-self: flex-start; max-width: 300px">'
                f'<div style="display: flex; align-items: center; gap: 10px; padding: 8px 12px">{icon("note", 16, ACC)}'
                f'{text(f"{artist} — {title}", 14, "#e6e6e6", extra=" max-width: 190px;")}{mono(t, 12, "#999999")}</div>'
                f'{bar(frac, ACC, 2, "#1e1e1e")}</div>')
    ctrl = ''.join(f'<div style="width: 32px; height: 28px; border: 1px solid {LINE}; display: flex; align-items: center; '
                   f'justify-content: center">{icon(k, 14, "#dddddd")}</div>' for k in ('prev', 'play', 'next'))
    inner = (f'{row(lab("NOW PLAYING", ACC), mono(f"{t} / {total}", 12, "#999999"))}'
             f'<div style="display: flex; flex-direction: column; gap: 2px">{text(title, 20, "#ffffff", 700)}{text(artist, 14, "#bbbbbb")}</div>'
             f'{bar(frac, ACC, 3)}'
             f'<div style="display: flex; justify-content: space-between; align-items: center">'
             f'<div style="display: flex; gap: 6px">{ctrl}</div>{lab("SHUFFLE · REPEAT ALL", "#999999", 11)}</div>')
    return panel(inner, gap=10, extra=' width: 300px;')


def modecard(title, inner):
    return panel(lab(title, '#bbbbbb') + inner, gap=12)


def posline(pos, of, right_label, right_value):
    return (f'<div style="display: flex; justify-content: space-between; align-items: flex-end">'
            f'<div style="display: flex; align-items: baseline; gap: 6px">{big(pos, 64)}{big("/ " + str(of), 26, "#999999")}</div>'
            f'<div style="display: flex; flex-direction: column; align-items: flex-end; gap: 4px">{lab(right_label)}{big(right_value, 34)}</div></div>')


def kv(k, v, vcolor='#ffffff', size=17):
    return row(lab(k, '#999999'), mono(v, size, vcolor))


def standings(title, right, rows, teams=None):
    head = row(lab(title), lab(right, '#999999'))
    t = ''
    if teams:
        cells = ''.join(
            f'<div style="background: {c}; padding: 5px 8px; display: flex; justify-content: space-between; align-items: center">'
            f'{lab(n, "#111111", 11)}{big(s, 20, "#111111")}</div>' for n, c, s in teams)
        t = f'<div style="display: grid; grid-template-columns: repeat({len(teams)}, minmax(0, 1fr)); gap: 4px">{cells}</div>'
    body = []
    for r in rows:
        pos, name, val = r[0], r[1], r[2]
        me = len(r) > 3 and r[3] == 'me'
        out = len(r) > 3 and r[3] == 'out'
        tcol = r[4] if len(r) > 4 else None
        vcol = r[5] if len(r) > 5 else ('#999999' if out else '#dddddd')
        bg = 'background: rgba(255, 85, 0, 0.18);' if me else ''
        ncol = '#777777' if out else '#ffffff'
        body.append(f'<div style="display: flex; align-items: center; gap: 10px; height: 26px; padding: 0 8px; {bg}">'
                    f'{mono(pos, 13, "#999999")}{swatch(tcol) if tcol else ""}'
                    f'{text(name, 15, ncol, 700 if me else 400, " flex: 1;")}{mono(val, 13, vcol)}</div>')
    return panel(head + t + f'<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">{"".join(body)}</div>',
                 gap=10)


def mirror(bg):
    if not bg:
        return '<div style="width: 380px; height: 92px; border: 1px solid rgba(255,255,255,0.35); box-sizing: border-box"></div>'
    return (f'<div style="width: 380px; height: 92px; border: 1px solid rgba(255,255,255,0.35); overflow: hidden; '
            f'position: relative; box-sizing: border-box">'
            f'<img src="{bg}" alt="" style="position: absolute; left: -40px; top: -330px; width: 460px; height: 680px; '
            f'object-fit: cover; transform: scaleX(-1); filter: brightness(0.9)"></div>')


def toast(kind, tag, title, sub, accentcol=None):
    c = accentcol or ACC
    return (f'<div style="width: 420px; background: {PANEL}; border: 1px solid {LINE}; border-top: 2px solid {c}; '
            f'box-sizing: border-box; padding: 12px 16px; display: flex; gap: 14px; align-items: center">'
            f'<div style="width: 44px; height: 44px; border: 1px solid {LINE}; display: flex; align-items: center; '
            f'justify-content: center; flex: none">{icon(kind, 24, c)}</div>'
            f'<div style="display: flex; flex-direction: column; gap: 4px; min-width: 0">{lab(tag, c, 11)}'
            f'{text(title, 19, "#ffffff", 700)}{mono(sub, 12, "#bbbbbb")}</div></div>')


def center(main, sub=None, top=300, size=72, color='#ffffff'):
    s = mono(sub, 15, '#dddddd', ' letter-spacing: 0.2em; text-transform: uppercase; margin-top: 10px;') if sub else ''
    return (f'<div style="position: absolute; left: 0; right: 0; top: {top}px; display: flex; flex-direction: column; '
            f'align-items: center; text-shadow: 0 2px 12px rgba(0,0,0,0.6)">{big(main, size, color)}{s}</div>')


def killfeed(lines):
    out = []
    for a, b, w in lines:
        out.append(f'<div style="align-self: flex-end; background: rgba(13,13,13,0.62); padding: 4px 10px; display: flex; '
                   f'gap: 8px; align-items: baseline">{text(a, 14, "#ffffff", 700)}{mono(w, 11, ACC)}{text(b, 14, "#dddddd")}</div>')
    return ''.join(out)


def chat(lines):
    return ''.join(f'<div style="background: rgba(13,13,13,0.55); padding: 4px 10px; align-self: flex-start; display: flex; gap: 6px">'
                   f'{text(n + ":", 14, ACC, 700)}{text(m, 14, "#e6e6e6")}</div>' for n, m in lines)


# ---------------------------------------------------------------- board scaffold
def board(name, title, bg, parts, lang='en', dim=0.0):
    shade = (f'<div style="position: absolute; inset: 0; background: rgba(0,0,0,{dim})"></div>' if dim else '')
    bgimg = (f'<img src="{bg}" alt="" style="position: absolute; left: 0; top: 0; width: 1600px; height: 900px; object-fit: cover">'
             if bg else '')
    html = f'''<!doctype html>
<html lang="{lang}">
<head>
<meta charset="utf-8">
<title>{title}</title>
<script src="./support.js"></script>
</head>
<body>
<x-dc>
<helmet>
<style>
@font-face{{font-family:'Barlow';src:url({FONT['barlow']}) format('truetype');font-weight:400 700;font-style:normal}}
@font-face{{font-family:'Barlow Condensed';src:url({FONT['cond']}) format('truetype');font-weight:700;font-style:normal italic}}
@font-face{{font-family:'Share Tech Mono';src:url({FONT['mono']}) format('truetype');font-weight:400}}
body{{margin:0;background:#0b0b0b}}
</style>
</helmet>
<div style="width: 1600px; height: 900px; position: relative; overflow: hidden; background: #0b0b0b; font-family: {BODY}; color: #ffffff">
{bgimg}{shade}
{''.join(parts)}
</div>
</x-dc>
<script type="text/x-dc" data-dc-script data-props='{{"accent":{{"editor":"color","default":"#ff5500","options":["#ff5500","#ffb000","#5ab0ff","#e6e6e6"]}},"panelOpacity":{{"editor":"range","default":82,"min":40,"max":100,"step":2,"unit":"%"}},"$preview":{{"width":1600,"height":900}}}}'>
class Component extends DCLogic {{
renderVals() {{
const a = (this.props.panelOpacity ?? 82) / 100;
return {{ accent: this.props.accent ?? '#ff5500', panel: 'rgba(13, 13, 13, ' + a + ')' }};
}}
}}
</script>
</body>
</html>
'''
    if MODE == 'local':
        html = html.replace('<script src="./support.js"></script>', '')
        html = html.replace('<x-dc>', '').replace('</x-dc>', '').replace('<helmet>', '').replace('</helmet>', '')
        html = html[:html.index('<script type="text/x-dc"')] + '</body>\n</html>\n'
        html = html.replace('{{accent}}', '#ff5500').replace('{{panel}}', 'rgba(13, 13, 13, 0.82)')
    open(os.path.join(OUT, name), 'w').write(html)


TL = 'left: 32px; top: 28px'
TR = 'right: 32px; top: 28px'
BL = 'left: 32px; bottom: 28px'
BR = 'right: 32px; bottom: 28px'
TC = 'left: 590px; top: 22px'      # 420 wide, centred on 1600


def top_center(inner):
    return (f'<div style="position: absolute; left: 0; right: 0; top: 22px; display: flex; flex-direction: column; '
            f'align-items: center; gap: 12px">{inner}</div>')


RACE_ROWS = [('01', 'Sam', 'LEADER'), ('02', 'Bobby', '+0.85'), ('03', 'Player', '+1.92', 'me'),
             ('04', 'Janine', '+2.40'), ('05', 'Vlad', '+4.07'), ('06', 'Emma', '+6.31')]

# 1 Race -------------------------------------------------------------------------------------------
board('Main.dc.html', 'Race', BG['racing_dm'], [
    top_center(mirror(BG['racing_dm']) + toast('trophy', 'Personal best', 'Lap 1 · 0:40.95', 'BEST LAP ON Q3R_DOWNTOWN')),
    col(TL, modecard('Race · Downtown', posline('P3', 6, 'LAP', '2/5') + sep()
                     + kv('LAP', '0:42.18') + kv('BEST', '0:40.95') + kv('TO P2', '+1.07', ACC)), 300),
    col(TR, standings('Race order', 'Gap', RACE_ROWS), 272),
    col(BL, jukebox() + vehicle(118), 300),
    col(BR, minimap() + slipstream(0.7) + cockpit('142', '4', 0.72, 0.6, 0.82), 330),
])

# 1b Team race: the race board with the teams (not on the canvas, which shows it as the race) ------------
TEAM_RACE_ROWS = [('01', 'Sam', 'LEADER', '', TEAM['red']), ('02', 'Bobby', '+0.85', '', TEAM['blue']),
                  ('03', 'Player', '+1.92', 'me', TEAM['blue']), ('04', 'Janine', '+2.40', '', TEAM['red']),
                  ('05', 'Vlad', '+4.07', '', TEAM['red']), ('06', 'Emma', '+6.31', '', TEAM['blue'])]
board('TeamRacing.dc.html', 'Team race', BG['racing_dm'], [
    top_center(mirror(BG['racing_dm'])),
    col(TL, modecard('Team race · Downtown', posline('P3', 6, 'LAP', '2/5') + sep()
                     + kv('LAP', '0:42.18') + kv('BEST', '0:40.95') + kv('TO P2', '+1.07', ACC)
                     + kv('YOUR TEAM', 'BLUE', TEAM['blue'])), 300),
    col(TR, standings('Team race order', 'Gap', TEAM_RACE_ROWS,
                      teams=[('RED', TEAM['red'], '1:42.10'), ('BLUE', TEAM['blue'], '--')]), 272),
    col(BL, jukebox() + vehicle(118), 300),
    col(BR, minimap() + slipstream(0.7) + cockpit('142', '4', 0.72, 0.6, 0.82), 330),
])

# 1c Big messages in the race: centred under the mirror, and the spectator's panel at the bottom (not on
# the canvas, which shows them on the notifications board) -------------------------------------------
def bigmsg(main, sub=None, size=44, color='#ffffff', top=300):
    s = lab(sub, '#dddddd', 14 if size >= 56 else 13) if sub else ''
    return (f'<div style="position: absolute; left: 0; right: 0; top: {top}px; display: flex; flex-direction: column; '
            f'align-items: center; gap: 6px; text-shadow: 0 2px 12px rgba(0,0,0,0.6)">{big(main, size, color)}{s}</div>')


def spectator(name):
    return panel(row(f'<div style="display: flex; flex-direction: column; gap: 4px">{lab("FOLLOWING")}{text(name, 22, "#ffffff", 700)}</div>',
                     f'<div style="display: flex; flex-direction: column; align-items: flex-end; gap: 4px">'
                     f'{lab("FIRE NEXT", "#999999", 11)}{lab("ESC MENU", "#999999", 11)}</div>'), gap=8)


board('Meldungen.dc.html', 'Race messages', BG['racing_dm'], [
    top_center(mirror(BG['racing_dm'])),
    bigmsg('WRONG WAY', 'TURN AROUND', 56, ACC),
    bigmsg('CAR RECOVERED', 'BACK ON TRACK AT CP 4', 44, top=420),
    col(TL, modecard('Race · Downtown', posline('P3', 6, 'LAP', '2/5') + sep()
                     + kv('LAP', '0:42.18') + kv('BEST', '0:40.95') + kv('TO P2', '+1.07', ACC)), 300),
    col(TR, standings('Race order', 'Gap', RACE_ROWS), 272),
    col('left: 650px; bottom: 28px', spectator('Sam'), 300),
    col(BL, vehicle(118), 300),
    col(BR, minimap() + cockpit('142', '4', 0.72, 0.6, 0.82), 330),
])

# 2 Zones (German explanation board) ---------------------------------------------------------------
def zone(x, y, w, h, num, name, desc):
    return (f'<div style="position: absolute; left: {x}px; top: {y}px; width: {w}px; height: {h}px; box-sizing: border-box; '
            f'border: 2px dashed {ACC}; background: rgba(255, 85, 0, 0.08); padding: 12px 14px; display: flex; '
            f'flex-direction: column; gap: 6px">'
            f'<div style="display: flex; align-items: baseline; gap: 10px">{big(num, 30, ACC)}{text(name, 18, "#ffffff", 700)}</div>'
            f'<div style="font-family: {BODY}; font-size: 14px; line-height: 1.4; color: #dddddd">{desc}</div></div>')


board('Zonen.dc.html', 'Zonen und Bausteine', BG['racing_dm'], [
    zone(32, 28, 300, 230, '1', 'Modus-Karte',
         'Das Wichtigste des Modus: Platz, Runde, Zeiten, Punkte, Ziele. Gleicher Ort in jedem Modus, der Inhalt wechselt.'),
    zone(1296, 28, 272, 230, '2', 'Wertung',
         'Rangliste bis 6 Zeilen um den eigenen Platz, bei Teams mit Teamständen darüber. Darunter der Kill-Feed.'),
    zone(610, 22, 380, 92, '3', 'Rückspiegel', 'Optional, oben in der Mitte wie im echten Auto.'),
    zone(590, 126, 420, 120, '4', 'Einblendungen',
         'Eine Spur für alles Kurze: Pickups, Belohnungen, Erfolge, Rang, Nachrichten. Immer nur eine, die nächste wartet.'),
    zone(400, 290, 800, 160, '5', 'Großmeldungen',
         'Countdown, GO, falsche Richtung, Auto geborgen, Flagge, Zuschauer-Hinweise. Mitte bleibt sonst frei.'),
    zone(32, 560, 300, 92, '6', 'Jukebox', 'Eine Zeile, beim Titelwechsel 15 s groß.'),
    zone(32, 664, 300, 208, '7', 'Fahrzeug &amp; Waffen',
         'Zustand und Panzerung. In Waffenmodi darunter Front, Heck, Pickup oder Item.'),
    zone(1384, 350, 184, 184, '8', 'Minimap', 'Optional.'),
    zone(1238, 546, 330, 56, '9', 'Windschatten', 'Nur wenn aktiv.'),
    zone(1238, 614, 330, 258, '10', 'Cockpit',
         'Tempo, Gang, Drehzahl, NOS, Tank. In jedem Modus gleich, auch an gleicher Stelle.'),
], lang='de', dim=0.55)

# 3 Sprint ---------------------------------------------------------------------------------------------
def sprint_progress(frac, cps, done):
    ticks = ''.join(
        f'<div style="position: absolute; left: {i / (cps + 1) * 100:.1f}%; top: -3px; width: 2px; height: 12px; '
        f'background: {"#ffffff" if i <= done else "#5a5a5a"}"></div>' for i in range(1, cps + 1))
    return (f'<div style="position: relative; height: 6px; background: #2c2c2c; margin-top: 4px">'
            f'<div style="position: absolute; left: 0; top: 0; bottom: 0; width: {frac * 100:.0f}%; background: {ACC}"></div>{ticks}</div>')


board('Sprint.dc.html', 'Sprint', BG['sprint'], [
    top_center(toast('nos', 'Pickup', 'NOS +25%', 'BOTTLE 2 OF 3')),
    col(TL, modecard('Sprint · Timber Run', posline('P2', 4, 'TO GO', '1,252 m')
                     + sprint_progress(0.58, 7, 4) + row(lab('CP 4 / 7', '#999999'), lab('58%', '#999999')) + sep()
                     + kv('TIME', '0:51.06') + kv('BEST', '1:28.40') + kv('TO P1', '+1.14', ACC)), 300),
    col(TR, standings('Race order', 'Gap', [('01', 'Dean', 'LEADER'), ('02', 'Player', '+1.14', 'me'),
                                            ('03', 'Chloe', '+3.02'), ('04', 'Tony', '+7.88')]), 272),
    col(BL, vehicle(125), 300),
    col(BR, minimap(me=(52, 160)) + cockpit('97', '3', 0.64, 0.25, 0.94), 330),
])

# 4 Ghost ----------------------------------------------------------------------------------------------
board('Ghost.dc.html', 'Ghost race', BG['racing'], [
    top_center(mirror(BG['racing'])),
    col(TL, modecard('Ghost race · Country', f'<div style="display: flex; justify-content: space-between; align-items: flex-end">'
                     f'<div style="display: flex; flex-direction: column; gap: 4px">{lab("VS GHOST")}{big("-0.84", 64, FAST)}</div>'
                     f'<div style="display: flex; flex-direction: column; align-items: flex-end; gap: 4px">{lab("LAP")}{big("3/5", 34)}</div></div>'
                     + sep() + kv('LAP', '0:38.77') + kv('BEST', '0:39.61') + kv('GHOST', 'PERSONAL · 1:21.40', '#dddddd', 14)), 300),
    col(TR, standings('Splits', 'vs ghost', [('S1', 'Start straight', '-0.21', '', None, FAST),
                                              ('S2', 'Forest hairpin', '-0.48', '', None, FAST),
                                              ('S3', 'Jump', '+0.12', '', None, ACC),
                                              ('S4', 'Finish', '—', 'me')]), 272),
    col(BL, vehicle(125), 300),
    col(BR, cockpit('121', '4', 0.81, 0.9, 0.7), 330),
])

# 5 Drag -----------------------------------------------------------------------------------------------
def drag_tach():
    segs = []
    for i in range(40):
        lit = i < 26
        win = 22 <= i <= 29
        lim = i >= 36
        if lim:
            c = ACC if lit else '#4a2412'
        elif win:
            c = FAST if lit else '#2a3c4d'
        else:
            c = '#f2f2f2' if lit else '#2c2c2c'
        segs.append(f'<div style="flex: 1; height: 26px; background: {c}"></div>')
    return (f'<div style="position: absolute; left: 400px; right: 400px; bottom: 28px">'
            + panel(f'<div style="display: flex; justify-content: space-between; align-items: flex-end">'
                    f'<div style="display: flex; flex-direction: column; gap: 4px">{lab("RPM")}{big("5,900", 44)}</div>'
                    f'<div style="display: flex; flex-direction: column; align-items: center; gap: 4px">{lab("LAUNCH WINDOW", FAST)}{mono("5,500 – 7,000", 15, FAST)}</div>'
                    f'<div style="display: flex; align-items: flex-end; gap: 22px">'
                    f'<div style="display: flex; flex-direction: column; align-items: center; gap: 4px">{lab("GEAR")}{big("1", 44)}</div>'
                    f'<div style="display: flex; flex-direction: column; align-items: flex-end; gap: 4px">{lab("MPH")}{big("0", 44)}</div></div></div>'
                    f'<div style="display: flex; gap: 3px">{"".join(segs)}</div>'
                    + row(lab('IDLE', '#999999', 11), lab('REV LIMIT · ENGINE BLOWS', ACC, 11)), gap=12)
            + '</div>')


def tree():
    def light(c):
        return f'<div style="width: 20px; height: 20px; border-radius: 10px; background: {c}"></div>'
    rows = [('PRE', ['#ffd129', '#ffd129']), ('STAGE', ['#ffd129', '#ffd129']),
            ('', ['#3a3220', '#3a3220']), ('', ['#3a3220', '#3a3220']), ('', ['#3a3220', '#3a3220']),
            ('GO', ['#1c3a24', '#1c3a24'])]
    inner = ''.join(f'<div style="display: flex; align-items: center; gap: 10px">{light(a)}{light(b)}{lab(n, "#999999", 11)}</div>'
                    for n, (a, b) in rows)
    return panel(lab('Tree') + f'<div style="display: flex; flex-direction: column; gap: 8px">{inner}</div>', gap=12)


board('Drag.dc.html', 'Drag race', BG['drag'], [
    top_center(toast('msg', 'Heat', 'Heat 2 of 3', 'YOU LEAD 1 – 0', FAST)),
    col(TL, modecard('Drag · Quarter mile', f'{row(lab("YOU"), lab("LANE LEFT", "#999999"))}' + sep()
                     + kv('REACTION', '0.412') + kv('60 FT', '2.08') + kv('330 FT', '—', '#777777')
                     + kv('1/8 MILE', '—', '#777777') + kv('1/4 MILE', '—', '#777777') + kv('TRAP', '—', '#777777')), 300),
    col(TR, standings('Duel · Heat 2/3', 'ET', [('L', 'Player', '—', 'me'), ('R', 'Vlad', '—')])
        + tree(), 272),
    drag_tach(),
    col(BL, vehicle(125), 300),
])

# 6 Racing DM ------------------------------------------------------------------------------------------
WEAP = [('FRONT', 'gun', 'Machine gun', '140'), ('REAR', 'mine', 'Mines', '3')]
board('RacingDM.dc.html', 'Race combat', BG['dm'], [
    top_center(toast('medal', 'Reward', 'Excellent', 'TWO HITS IN 2 SECONDS')),
    col(TL, modecard('Race combat · Downtown', posline('P2', 6, 'LAP', '3/5') + sep()
                     + kv('LAP', '0:44.02') + kv('BEST', '0:41.37') + kv('SCORE', '4')), 300),
    col(TR, standings('Race order', 'Gap', [('01', 'Vlad', 'LEADER'), ('02', 'Player', '+0.62', 'me'),
                                            ('03', 'Paul', '+2.10'), ('04', 'Sam', '+3.85'), ('05', 'Chloe', '+5.02'),
                                            ('06', 'Tony', '+9.44')])
        + killfeed([('Player', 'Paul', 'ROCKET'), ('Sam', 'Tony', 'MINES')]), 272, 8),
    col(BL, jukebox() + vehicle(71, 25, WEAP, ('smoke', 'Smoke', '1')), 330),
    col(BR, minimap() + cockpit('118', '3', 0.66, 0.35, 0.61), 330),
])

# 7 Elimination ----------------------------------------------------------------------------------------
board('Elimination.dc.html', 'Elimination', BG['racing'], [
    col(TL, modecard('Elimination · Country', posline('P3', 4, 'LEFT', '4/6') + sep()
                     + f'<div style="display: flex; flex-direction: column; gap: 6px">{row(lab("NEXT OUT", ACC), mono("0:12", 17, ACC))}{bar(0.4, ACC, 6)}'
                     f'{lab("LAST PLACE IS OUT AT 0:00", "#999999", 11)}</div>' + kv('TIME', '1:47.30')), 300),
    col(TR, standings('Still in', 'Status', [('01', 'Carla', 'SAFE'), ('02', 'Tony', 'SAFE'), ('03', 'Player', 'SAFE', 'me'),
                                             ('04', 'Janine', 'DANGER', '', None, ACC), ('--', 'Bobby', 'OUT', 'out'),
                                             ('--', 'Emma', 'OUT', 'out')]), 272),
    col(BL, vehicle(103), 300),
    col(BR, cockpit('131', '4', 0.77, 0.15, 0.73), 330),
    center('3RD · SAFE', 'Bobby is out', 210, 46),
])

# 8 LCS ------------------------------------------------------------------------------------------------
def int_rows(rows):
    out = []
    for n, v, me, tag in rows:
        bg = 'background: rgba(255, 85, 0, 0.18);' if me else ''
        out.append(f'<div style="display: flex; align-items: center; gap: 10px; height: 26px; padding: 0 8px; {bg}">'
                   f'{text(n, 15, "#777777" if tag == "OUT" else "#ffffff", 700 if me else 400, " width: 92px;")}'
                   f'<div style="flex: 1">{bar(v / 125, ACC if v < 35 else "#e6e6e6", 4)}</div>{mono(tag, 12, "#999999" if tag == "OUT" else "#dddddd", " width: 40px; text-align: right;")}</div>')
    return ''.join(out)


board('LCS.dc.html', 'Last car standing', BG['lcs'], [
    top_center(mirror(BG['lcs'])),
    col(TL, modecard('Last car standing · DD02', f'<div style="display: flex; justify-content: space-between; align-items: flex-end">'
                     f'<div style="display: flex; flex-direction: column; gap: 4px">{lab("LEFT")}{big("3 / 6", 56)}</div>'
                     f'<div style="display: flex; flex-direction: column; align-items: flex-end; gap: 4px">{lab("SCORE")}{big("2", 34)}</div></div>'
                     + sep() + kv('MATCH', '2:41') + kv('LONGEST ALIVE', 'Carla')), 300),
    col(TR, panel(row(lab('Still in'), lab('Integrity', '#999999'))
                  + f'<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">'
                  + int_rows([('Player', 88, True, '88'), ('Carla', 112, False, '112'), ('Josh', 21, False, '21'),
                              ('Minori', 0, False, 'OUT'), ('Sam', 0, False, 'OUT'), ('Ivan', 0, False, 'OUT')]) + '</div>'), 272),
    col(BL, vehicle(88, 50, [('FRONT', 'gun', 'Plasma', '62'), ('REAR', 'smoke', 'Smoke', '2')]), 300),
    col(BR, cockpit('64', '2', 0.58, 0.8, 0.55), 330),
])

# 9 Derby ----------------------------------------------------------------------------------------------
def derby_car():
    z = {'front': ACC, 'left': '#e6e6e6', 'right': '#7a7a7a', 'rear': '#e6e6e6'}
    svg = (f'<svg width="110" height="170" viewBox="0 0 110 170" aria-hidden="true">'
           f'<rect x="25" y="10" width="60" height="150" rx="14" fill="none" stroke="#5a5a5a" stroke-width="2"/>'
           f'<rect x="31" y="14" width="48" height="16" rx="4" fill="{z["front"]}"/>'
           f'<rect x="31" y="140" width="48" height="16" rx="4" fill="{z["rear"]}"/>'
           f'<rect x="13" y="40" width="10" height="90" rx="3" fill="{z["left"]}"/>'
           f'<rect x="87" y="40" width="10" height="90" rx="3" fill="{z["right"]}"/>'
           f'<rect x="36" y="52" width="38" height="56" rx="4" fill="none" stroke="#5a5a5a" stroke-width="2"/></svg>')
    legend = ''.join(f'<div style="display: flex; align-items: center; gap: 8px">{swatch(c, 10)}{lab(n, "#bbbbbb", 11)}</div>'
                     for n, c in (('OK', '#e6e6e6'), ('WORN', '#7a7a7a'), ('CRITICAL', ACC)))
    return (f'<div style="display: flex; gap: 18px; align-items: center">{svg}'
            f'<div style="display: flex; flex-direction: column; gap: 10px">{lab("FRONT CRITICAL", ACC)}{legend}</div></div>')


board('Derby.dc.html', 'Derby', BG['lcs'], [
    top_center(toast('medal', 'Hit', 'Rear hit on Josh', '+120 SCORE')),
    col(TL, modecard('Derby · David5', f'<div style="display: flex; justify-content: space-between; align-items: flex-end">'
                     f'<div style="display: flex; flex-direction: column; gap: 4px">{lab("SCORE")}{big("840", 56)}</div>'
                     f'<div style="display: flex; flex-direction: column; align-items: flex-end; gap: 4px">{lab("NO RAM", ACC)}{big("0:39", 34, ACC)}</div></div>'
                     + sep() + derby_car()), 300),
    col(TR, panel(row(lab('Derby order'), lab('Integrity', '#999999'))
                  + f'<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">'
                  + int_rows([('Player', 66, True, '66'), ('Chloe', 104, False, '104'), ('Josh', 30, False, '30'),
                              ('Sam', 0, False, 'OUT')]) + '</div>'), 272),
    col(BL, vehicle(66, 0), 300),
    col(BR, cockpit('38', '2', 0.5, 0.4, 0.88), 330),
])

# 10 Deathmatch ----------------------------------------------------------------------------------------
board('Deathmatch.dc.html', 'Deathmatch', BG['dm'], [
    col(TL, modecard('Deathmatch · Downtown', f'<div style="display: flex; justify-content: space-between; align-items: flex-end">'
                     f'<div style="display: flex; align-items: baseline; gap: 6px">{big("12", 64)}{big("/ 20", 26, "#999999")}</div>'
                     f'<div style="display: flex; flex-direction: column; align-items: flex-end; gap: 4px">{lab("RANK")}{big("2nd", 34)}</div></div>'
                     + sep() + kv('TIME LEFT', '6:12') + kv('LEADER', 'Chloe · 14'))
        + chat([('Sam', 'nice shot'), ('Vlad', 'gg')]), 300),
    col(TR, standings('Frag order', 'Frags', [('01', 'Chloe', '14'), ('02', 'Player', '12', 'me'), ('03', 'Paul', '9'),
                                              ('04', 'Sam', '7'), ('05', 'Vlad', '3')])
        + killfeed([('Player', 'Vlad', 'RAIL'), ('Chloe', 'Sam', 'ROCKET'), ('Paul', 'Player', 'MG')]), 272, 8),
    col(BL, vehicle(45, 100, [('FRONT', 'gun', 'Railgun', '7'), ('REAR', 'mine', 'Mines', '5')], ('box', 'Med kit', '1')), 330),
    col(BR, minimap() + cockpit('56', '2', 0.48, 0.0, 0.66), 330),
])

# 11 Team DM -------------------------------------------------------------------------------------------
board('TeamDM.dc.html', 'Team deathmatch', BG['team_dm'], [
    col(TL, modecard('Team combat · Battle Canyon', f'<div style="display: flex; justify-content: space-between; align-items: flex-end">'
                     f'<div style="display: flex; flex-direction: column; gap: 4px">{lab("YOUR TEAM", TEAM["blue"])}{big("Blue", 44, TEAM["blue"])}</div>'
                     f'<div style="display: flex; flex-direction: column; align-items: flex-end; gap: 4px">{lab("YOUR FRAGS")}{big("6", 34)}</div></div>'
                     + sep() + kv('TIME LEFT', '8:20') + kv('LIMIT', '50')), 300),
    col(TR, standings('Team frag order', 'Frags', [('01', 'Sam', '11', '', TEAM['blue']), ('02', 'Vlad', '9', '', TEAM['red']),
                                                   ('03', 'Player', '6', 'me', TEAM['blue']), ('04', 'Tobias', '6', '', TEAM['red']),
                                                   ('05', 'Minori', '5', '', TEAM['red']), ('06', 'Chloe', '3', '', TEAM['blue'])],
                      teams=[('RED', TEAM['red'], '29'), ('BLUE', TEAM['blue'], '34')])
        + killfeed([('Sam', 'Tobias', 'ROCKET')]), 272, 8),
    col(BL, vehicle(75, 0, [('FRONT', 'gun', 'Rockets', '18'), ('REAR', 'smoke', 'Smoke', '2')]), 300),
    col(BR, cockpit('44', '2', 0.4, 0.1, 0.99), 330),
])

# 12 CTF (four teams) ----------------------------------------------------------------------------------
def flags(rows):
    out = []
    for team, state, note in rows:
        c = TEAM[team]
        out.append(f'<div style="display: flex; align-items: center; gap: 10px">{icon("flag", 18, c)}'
                   f'{text(team.capitalize(), 15, "#ffffff", 700, " width: 60px;")}'
                   f'{mono(state, 13, ACC if state != "AT BASE" else "#dddddd", " flex: 1;")}{mono(note, 12, "#999999")}</div>')
    return f'<div style="display: flex; flex-direction: column; gap: 8px">{"".join(out)}</div>'


board('CTF.dc.html', 'Capture the flag (four teams)', BG['team_dm'], [
    top_center(toast('flag', 'Flag', 'You took the red flag', 'BRING IT HOME', TEAM['red'])),
    col(TL, modecard('Four team CTF · CTF4_01', f'{row(lab("YOUR TEAM", TEAM["green"]), big("Green", 30, TEAM["green"]))}' + sep()
                     + flags([('red', 'CARRIED', 'Player'), ('blue', 'AT BASE', ''), ('green', 'DROPPED', '0:12'),
                              ('yellow', 'AT BASE', '')]) + sep() + kv('TIME LEFT', '11:03')), 300),
    col(TR, standings('CTF driver order', 'Caps', [('01', 'Ivan', '2', '', TEAM['blue']), ('02', 'Player', '1', 'me', TEAM['green']),
                                                   ('03', 'Kuro', '1', '', TEAM['red']), ('04', 'Minori', '0', '', TEAM['yellow'])],
                      teams=[('R', TEAM['red'], '1'), ('B', TEAM['blue'], '2'), ('G', TEAM['green'], '1'), ('Y', TEAM['yellow'], '0')]), 272),
    col(BL, vehicle(97, 50, [('FRONT', 'gun', 'Machine gun', '88'), ('REAR', 'mine', 'Mines', '2')]), 300),
    col(BR, minimap(dots=((150, 118, TEAM['blue']), (60, 175, TEAM['red']), (35, 110, TEAM['yellow']))) + cockpit('83', '3', 0.6, 0.45, 0.5), 330),
])

# 13 Domination ----------------------------------------------------------------------------------------
def zones(states, capturing):
    tiles = []
    for n, owner in states:
        c = TEAM.get(owner, '#3a3a3a')
        fg = '#111111' if owner in TEAM else '#dddddd'
        tiles.append(f'<div style="height: 40px; background: {c}; display: flex; align-items: center; justify-content: center">{big(n, 22, fg)}</div>')
    n, team, frac = capturing
    return (f'<div style="display: grid; grid-template-columns: repeat({len(states)}, minmax(0, 1fr)); gap: 4px">{"".join(tiles)}</div>'
            f'<div style="display: flex; flex-direction: column; gap: 6px">{row(lab("CAPTURING " + n, TEAM[team]), mono(f"{frac * 100:.0f}%", 13, TEAM[team]))}'
            f'{bar(frac, TEAM[team], 6)}</div>')


board('Domination.dc.html', 'Domination', BG['koth'], [
    top_center(mirror(BG['koth'])),
    col(TL, modecard('Domination · Babel', zones([('A', 'red'), ('B', 'blue'), ('C', 'none'), ('D', 'blue'), ('E', 'green')],
                                                 ('C', 'blue', 0.65)) + sep()
                     + kv('YOUR TEAM', 'BLUE', TEAM['blue']) + kv('TIME LEFT', '9:41')), 300),
    col(TR, standings('Domination order', 'Score', [('01', 'Dean', '41', '', TEAM['red']), ('02', 'Player', '38', 'me', TEAM['blue']),
                                                    ('03', 'Janine', '22', '', TEAM['green']), ('04', 'Carla', '17', '', TEAM['yellow'])],
                      teams=[('R', TEAM['red'], '112'), ('B', TEAM['blue'], '131'), ('G', TEAM['green'], '64'), ('Y', TEAM['yellow'], '40')]), 272),
    col(BL, vehicle(110, 25, [('FRONT', 'gun', 'Plasma', '40'), ('REAR', 'smoke', 'Smoke', '1')]), 300),
    col(BR, cockpit('35', '2', 0.36, 0.2, 0.77), 330),
])

# 14 KOTH ----------------------------------------------------------------------------------------------
board('KOTH.dc.html', 'King of the hill', BG['koth'], [
    col(TL, modecard('King of the hill · Mesa King', f'<div style="display: flex; align-items: center; gap: 12px">{icon("hill", 34, TEAM["red"])}'
                     f'<div style="display: flex; flex-direction: column; gap: 4px">{lab("HILL HELD BY")}{big("Red", 40, TEAM["red"])}</div></div>'
                     + f'<div style="display: flex; flex-direction: column; gap: 6px">{row(lab("CONTESTED", ACC), mono("2 v 1", 13, ACC))}{bar(0.3, TEAM["blue"], 6)}</div>'
                     + sep() + kv('RESPAWN WAVE', '0:07') + kv('TIME LEFT', '4:55')), 300),
    col(TR, standings('KOTH driver order', 'Pts', [('01', 'Ivan', '28', '', TEAM['red']), ('02', 'Tanya', '24', '', TEAM['red']),
                                                   ('03', 'Player', '19', 'me', TEAM['blue']), ('04', 'Paul', '11', '', TEAM['blue'])],
                      teams=[('RED', TEAM['red'], '52'), ('BLUE', TEAM['blue'], '30')]), 272),
    col(BL, vehicle(84, 0, [('FRONT', 'gun', 'Machine gun', '120')]), 300),
    col(BR, minimap(dots=((150, 118, TEAM['red']), (60, 175, TEAM['red']), (35, 110, TEAM['blue']))) + cockpit('61', '3', 0.5, 0.6, 0.62), 330),
])

# 15 Autoball ------------------------------------------------------------------------------------------
def scorebug():
    return (f'<div style="display: flex; align-items: stretch; height: 52px; border: 1px solid {LINE}">'
            f'<div style="background: {TEAM["red"]}; width: 92px; display: flex; align-items: center; justify-content: center; gap: 10px">'
            f'{lab("RED", "#111111", 11)}{big("2", 34, "#111111")}</div>'
            f'<div style="background: {PANEL}; width: 120px; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 2px">'
            f'{mono("3:41", 22)}{lab("2ND HALF", "#999999", 10)}</div>'
            f'<div style="background: {TEAM["blue"]}; width: 92px; display: flex; align-items: center; justify-content: center; gap: 10px">'
            f'{big("1", 34, "#111111")}{lab("BLUE", "#111111", 11)}</div></div>')


board('Autoball.dc.html', 'Autoball', BG['autoball'], [
    top_center(scorebug()),
    col(TL, modecard('Autoball · Rooftop', f'{row(lab("YOUR TEAM", TEAM["blue"]), big("Blue", 30, TEAM["blue"]))}' + sep()
                     + kv('BALL', 'RED HALF · 42 M') + kv('YOUR GOALS', '1') + kv('POSSESSION', '46%')), 300),
    col(TR, standings('Autoball order', 'Goals', [('01', 'Vlad', '2', '', TEAM['red']), ('02', 'Player', '1', 'me', TEAM['blue']),
                                                  ('03', 'Steve', '0', '', TEAM['blue']), ('04', 'Jennifer', '0', '', TEAM['red'])]), 272),
    col(BL, jukebox(t='0:21', total='3:12', frac=0.11) + vehicle(125), 300),
    col(BR, cockpit('88', '3', 0.7, 0.85, 0.9), 330),
])


# 16 Notifications & jukebox (German captions) ---------------------------------------------------------
def cap(t):
    return f'<div style="font-family: {BODY}; font-size: 14px; line-height: 1.4; color: #bbbbbb; max-width: 420px">{t}</div>'


def stack(x, y, w, inner):
    return f'<div style="position: absolute; left: {x}px; top: {y}px; width: {w}px; display: flex; flex-direction: column; gap: 14px">{inner}</div>'


def heading(t):
    return f'<div style="font-family: {COND}; font-weight: 700; font-style: italic; font-size: 34px; color: #ffffff">{t}</div>'


board('Einblendungen.dc.html', 'Einblendungen und Jukebox', None, [
    f'<div style="position: absolute; inset: 0; background: repeating-linear-gradient(132deg, #0b0b0b 0 12px, #171717 12px 13px)"></div>',
    stack(48, 40, 440, heading('Einblendungen') + cap('Eine Spur oben in der Mitte, unter dem Spiegel. Jede Karte 3 s, die nächste wartet. Farbe der Kante nach Art.')
          + toast('nos', 'Pickup', 'NOS +25%', 'BOTTLE 2 OF 3')
          + toast('medal', 'Reward', 'Excellent', 'TWO HITS IN 2 SECONDS')
          + toast('trophy', 'Achievement', 'Road Warrior · Silver', '50 RACES FINISHED')
          + toast('chevron', 'Rank up', 'Master of Speed', '12,840 POINTS', FAST)
          + toast('msg', 'Message', 'Vote passed: next map', 'Q3R_TIMBERRUN', '#999999')),
    stack(560, 40, 520, heading('Großmeldungen') + cap('Mitte, nur kurz. Ersetzen die Centerprints, auch die von target_print, in der Menüschrift.')
          + f'<div style="display: flex; gap: 40px; align-items: baseline; padding: 10px 0">{big("3", 96)}{big("GO!", 96, ACC)}</div>'
          + f'<div style="display: flex; flex-direction: column; gap: 6px">{big("WRONG WAY", 56, ACC)}{lab("TURN AROUND", "#dddddd", 14)}</div>'
          + f'<div style="display: flex; flex-direction: column; gap: 6px">{big("CAR RECOVERED", 44)}{lab("BACK ON TRACK AT CP 4", "#dddddd", 13)}</div>'
          + f'<div style="display: flex; flex-direction: column; gap: 6px">{big("FINISHED · P2", 44)}{lab("1:24.08 · +1.14 TO WINNER", "#dddddd", 13)}</div>'),
    stack(1150, 40, 400, heading('Jukebox') + cap('Im Rennen eine Zeile über der Fahrzeugkarte. Beim Titelwechsel 15 s groß, dann wieder klein. Aus, wenn nichts läuft.')
          + lab('Klein', '#999999', 11) + jukebox()
          + lab('Groß (Titelwechsel)', '#999999', 11) + jukebox(expanded=True)
          + heading('Zuschauer') + cap('Unten in der Mitte, statt Cockpit.')
          + panel(row(f'<div style="display: flex; flex-direction: column; gap: 4px">{lab("FOLLOWING")}{text("Sam", 22, "#ffffff", 700)}</div>',
                      f'<div style="display: flex; flex-direction: column; align-items: flex-end; gap: 4px">{lab("FIRE NEXT", "#999999", 11)}{lab("ESC MENU", "#999999", 11)}</div>'), gap=8)
          + heading('Abstimmung')
          + panel(row(text('Change map to Timber Run?', 16, '#ffffff', 700), mono('0:24', 14, '#bbbbbb'))
                  + row(lab('F1 YES · 3', FAST), lab('F2 NO · 1', ACC)), gap=8)),
], lang='de')


# ---------------------------------------------------------------- canvas index
order = ['Main.dc.html', 'Zonen.dc.html', 'Sprint.dc.html', 'Ghost.dc.html',
         'Drag.dc.html', 'RacingDM.dc.html', 'Elimination.dc.html', 'LCS.dc.html',
         'Derby.dc.html', 'Deathmatch.dc.html', 'TeamDM.dc.html', 'CTF.dc.html',
         'Domination.dc.html', 'KOTH.dc.html', 'Autoball.dc.html', 'Einblendungen.dc.html']
titles = {'Main.dc.html': 'Rennen (Racing, Team Racing)', 'Zonen.dc.html': 'Zonen und Bausteine', 'Sprint.dc.html': 'Sprint',
          'Ghost.dc.html': 'Ghost Race', 'Drag.dc.html': 'Drag Race', 'RacingDM.dc.html': 'Racing DM (auch Team Racing DM)',
          'Elimination.dc.html': 'Elimination', 'LCS.dc.html': 'Last Car Standing', 'Derby.dc.html': 'Derby',
          'Deathmatch.dc.html': 'Deathmatch', 'TeamDM.dc.html': 'Team Deathmatch', 'CTF.dc.html': 'CTF und CTF4',
          'Domination.dc.html': 'Domination', 'KOTH.dc.html': 'King of the Hill', 'Autoball.dc.html': 'Autoball',
          'Einblendungen.dc.html': 'Einblendungen, Großmeldungen, Jukebox'}
rows_titles = ['Aufbau und Rennen', 'Drag, Waffen und Überleben', 'Derby und Arena', 'Ziele, Autoball und Einblendungen']
boards, notes = {}, {}
for i, n in enumerate(order):
    r, c = divmod(i, 4)
    boards[n] = {'x': c * 1680, 'y': r * 1300, 'w': 1600, 'h': 900, 'title': titles[n]}
for r, t in enumerate(rows_titles):
    notes[f'row{r + 1}'] = {'x': 0, 'y': r * 1300 - 260, 'text': t, 'kind': 'title1', 'maxW': 6640}
canvas = {'v': 3, 'createdOnFiles': {'v': 1, 'at': datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')},
          'title': 'Q3Rally HUD – Entwurf v0.9', 'launch': {'view': 'canvas'}, 'pages': [],
          'boards': boards, 'order': order, 'notes': notes, 'designSystems': []}
json.dump(canvas, open(os.path.join(OUT, 'canvas.json'), 'w'), ensure_ascii=False, indent=1)
print('ok', len(order))
