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
    'panel_race': '/_blob/2edecaa6b8463b29672acfd9a95b5062',
    'panel_dm': '/_blob/9895b6cc2b95a3e867b00db5cbb4f567',
    'panel_derby': '/_blob/eb0885475918bab512df2f55265e4c4e',
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
# ======================================================================================================
# End screens (next to the HUD): the race result
# ======================================================================================================
CARS = {'viper': '/_blob/cd1e673beb8d39b5864206bcd4132038', 'murcielago': '/_blob/6bdb76631d67a9ecc73e239ee748c9ad',
        'db12': '/_blob/8baa19f0d9bdc588aabe66c23243706d'}
if MODE == 'local':
    CARS = {k: (pathlib.Path(SCENES, f'car_{k}.jpg').as_uri() if SCENES else '') for k in CARS}
PURPLE = '#b46bff'        # the fastest lap of the race, as on a timing screen


def blurred(bg):
    """The game world behind the result: slow orbit around the winner, dark and soft."""
    return (f'<img src="{bg}" alt="" style="position: absolute; left: -40px; top: -40px; width: 1680px; height: 980px; '
            f'object-fit: cover; filter: blur(10px) brightness(0.32) saturate(0.8)">'
            f'<div style="position: absolute; inset: 0; background: radial-gradient(ellipse at 50% 40%, rgba(0,0,0,0) 0%, '
            f'rgba(0,0,0,0.55) 100%)"></div>')


def car_view(car, w, h):
    """A 3D window with the car turning on the spot (here a render from the main menu)."""
    return (f'<div style="width: {w}px; height: {h}px; position: relative; overflow: hidden; '
            f'background: radial-gradient(ellipse at 50% 70%, #262321 0%, #121212 75%)">'
            f'<img src="{CARS[car]}" alt="" style="position: absolute; left: 50%; top: 50%; width: {w + 30}px; '
            f'transform: translate(-50%, -50%); -webkit-mask-image: radial-gradient(ellipse 62% 60% at 50% 55%, #000 55%, transparent 100%); '
            f'mask-image: radial-gradient(ellipse 62% 60% at 50% 55%, #000 55%, transparent 100%)"></div>')


def podium_card(place, name, car, time, gap, w, h, me=False):
    tag = {1: 'WINNER', 2: 'SECOND', 3: 'THIRD'}[place]
    border = f'border-top: 2px solid {ACC if place == 1 else "#e6e6e6" if me else LINE};'
    you = lab('YOU', ACC, 11) if me else ''
    return (f'<div style="width: {w}px; background: {PANEL}; border: 1px solid {LINE}; {border} '
            f'box-sizing: border-box; display: flex; flex-direction: column">'
            f'{car_view(car, w - 2, h)}'
            f'<div style="padding: 12px 14px 14px; display: flex; flex-direction: column; gap: 6px">'
            f'<div style="display: flex; justify-content: space-between; align-items: flex-end">'
            f'{big(str(place), 64 if place == 1 else 52, ACC if place == 1 else "#ffffff")}'
            f'<div style="display: flex; flex-direction: column; align-items: flex-end; gap: 4px">{you}{lab(tag, "#999999", 11)}</div></div>'
            f'{text(name, 20, "#ffffff", 700)}'
            f'{row(mono(time, 15, "#dddddd"), mono(gap, 13, ACC) if gap else "")}'
            f'</div></div>')


def award(kind, label, name, value, color):
    return (f'<div style="flex: 1; background: {PANEL}; border: 1px solid {LINE}; border-left: 3px solid {color}; '
            f'padding: 12px 14px; box-sizing: border-box; display: flex; gap: 12px; align-items: center">'
            f'{icon(kind, 22, color)}'
            f'<div style="display: flex; flex-direction: column; gap: 4px; min-width: 0">{lab(label, "#999999", 11)}'
            f'{text(name, 16, "#ffffff")}</div>'
            f'<div style="margin-left: auto">{mono(value, 15, color)}</div></div>')


def classification(rows):
    head = (f'<div style="display: grid; grid-template-columns: 34px 1fr 82px 70px; gap: 10px; padding: 0 8px; margin: 0 -8px">'
            f'{lab("POS", "#999999", 11)}{lab("DRIVER", "#999999", 11)}'
            f'<div style="text-align: right">{lab("TIME", "#999999", 11)}</div>'
            f'<div style="text-align: right">{lab("BEST", "#999999", 11)}</div></div>')
    out = []
    for pos, name, gap, best, me, fastest in rows:
        bg = 'background: rgba(255,85,0,0.18);' if me else ''
        out.append(f'<div style="display: grid; grid-template-columns: 34px 1fr 82px 70px; gap: 10px; align-items: center; '
                   f'height: 34px; padding: 0 8px; {bg}">'
                   f'{mono(pos, 14, ACC if pos == "01" else "#999999")}'
                   f'{text(name, 16, "#ffffff", 700 if me else 400, "overflow: hidden; text-overflow: ellipsis")}'
                   f'<div style="text-align: right">{mono(gap, 14, "#dddddd")}</div>'
                   f'<div style="text-align: right">{mono(best, 14, PURPLE if fastest else "#999999")}</div></div>')
    return panel(row(lab('Final classification'), lab('5 laps', '#999999')) + head
                 + f'<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">{"".join(out)}</div>', gap=12)


def lap_chart(laps):
    """Each lap against your best one: the bar is the time lost, the best lap a thin white line."""
    best = min(laps)
    worst = max(laps) - best
    bars = ''.join(
        f'<div style="flex: 1; display: flex; flex-direction: column; align-items: center; gap: 6px">'
        f'<div style="height: 56px; width: 100%; display: flex; align-items: flex-end">'
        f'<div style="width: 100%; height: {max(3, int(56 * (v - best) / worst))}px; '
        f'background: {"#e6e6e6" if v == best else "#5a5a5a"}"></div></div>'
        f'{mono("best" if v == best else f"+{v - best:.2f}", 11, "#ffffff" if v == best else "#bbbbbb")}'
        f'{lab(f"L{i + 1}", "#777777", 10)}</div>'
        for i, v in enumerate(laps))
    return f'<div style="display: flex; gap: 8px">{bars}</div>'


def stat(k, v, color='#ffffff', unit=''):
    u = lab(unit, '#777777', 11) if unit else ''
    return (f'<div style="display: flex; flex-direction: column; gap: 4px">{lab(k, "#999999", 11)}'
            f'<div style="display: flex; align-items: baseline; gap: 6px">{big(v, 30, color)}{u}</div></div>')


def footer(waiting, ready, total, keys=(('FIRE', 'READY'), ('TAB', 'DETAILS'), ('ESC', 'MENU'))):
    """Q3 waits for the drivers between maps: FIRE says you are ready, the bar how many are."""
    keys = ''.join(f'<div style="display: flex; gap: 8px; align-items: baseline">{lab(k, "#ffffff", 12)}{lab(a, "#777777", 12)}</div>'
                   for k, a in keys)
    return (f'<div style="position: absolute; left: 56px; right: 56px; bottom: 36px; display: flex; align-items: center; gap: 40px">'
            f'<div style="display: flex; gap: 28px">{keys}</div>'
            f'<div style="margin-left: auto; width: 360px; display: flex; flex-direction: column; gap: 6px">'
            f'{row(lab(waiting, "#bbbbbb", 11), mono(f"{ready} / {total} READY", 13, "#ffffff"))}{bar(ready / total, ACC, 3)}</div></div>')


end_results = [('01', 'Sam', '4:20.238', '0:50.871', False, True), ('02', 'Player', '+1.142', '0:51.204', True, False),
               ('03', 'Carla', '+3.907', '0:51.660', False, False), ('04', 'Tony', '+9.415', '0:52.013', False, False),
               ('05', 'Janine', '+12.770', '0:52.390', False, False), ('06', 'Ivan', '+21.004', '0:52.881', False, False),
               ('07', 'Minori', '+1 LAP', '0:53.512', False, False), ('08', 'Bee', 'DNF', '0:54.120', False, False)]

board('EndRacing.dc.html', 'Race result', None, [
    blurred(BG['racing']),
    # the header and your result
    f'<div style="position: absolute; left: 56px; top: 48px; display: flex; gap: 16px; align-items: baseline">'
    f'{lab("Race result", ACC, 13)}{lab("Downtown · 5 laps · 8 drivers", "#999999", 12)}</div>',
    f'<div style="position: absolute; left: 56px; top: 92px; width: 380px; display: flex; flex-direction: column; gap: 22px">'
    f'<div style="display: flex; align-items: baseline; gap: 14px">{big("P2", 168)}{big("/ 8", 48, "#777777")}</div>'
    f'<div style="display: flex; flex-direction: column; gap: 6px; margin-top: -18px">{lab("Finished")}'
    f'{big("4:21.380", 44)}{lab("+1.142 to the winner", ACC, 12)}</div>'
    + sep() +
    f'<div style="display: grid; grid-template-columns: 1fr 1fr; gap: 18px 24px">'
    f'{stat("Best lap", "0:51.204")}{stat("Top speed", "214", "#ffffff", "km/h")}'
    f'{stat("Places gained", "+3", FAST)}{stat("Avg lap", "0:52.276")}</div>'
    + sep() +
    f'<div style="display: flex; flex-direction: column; gap: 12px">{row(lab("Your laps"), lab("lost to your best", "#777777", 11))}'
    + lap_chart([53.81, 52.42, 51.20, 51.66, 52.29]) + '</div></div>',
    # the podium
    f'<div style="position: absolute; left: 476px; top: 92px; width: 600px; height: 410px; display: flex; gap: 12px; '
    f'align-items: flex-end">'
    + podium_card(2, 'Player', 'viper', '4:21.380', '+1.142', 192, 176, me=True)
    + podium_card(1, 'Sam', 'murcielago', '4:20.238', '', 210, 236)
    + podium_card(3, 'Carla', 'db12', '4:24.145', '+3.907', 192, 140) + '</div>',
    # the awards under the podium
    f'<div style="position: absolute; left: 476px; top: 530px; width: 600px; display: flex; flex-direction: column; gap: 10px">'
    f'{lab("Highlights")}'
    + award('trophy', 'Fastest lap', 'Sam', '0:50.871', PURPLE)
    + award('flag', 'Best sector', 'Player · S3', '0:11.402', FAST)
    + award('chevron', 'Biggest gain', 'Player · P5 → P2', '+3', ACC) + '</div>',
    # the classification
    col('right: 56px; top: 92px', classification(end_results), 420),
    footer('WAITING FOR 5 DRIVERS', 3, 8),
])

# ---------------------------------------------------------------------------
# the end screen of a team race: the teams by their average time
# ---------------------------------------------------------------------------

def team_podium_card(place, team, car, avg, gap, members, w, h, me=False):
    """A team on the podium: its lead car, the place in the team's colour, the
    average time and who drove for it."""
    tc = TEAM[team]
    tag = {1: 'WINNER', 2: 'SECOND', 3: 'THIRD'}[place]
    you = lab('YOUR TEAM', ACC, 11) if me else ''
    chips = ''.join(f'<div style="display: flex; gap: 6px; align-items: baseline">{mono(p, 12, "#999999")}'
                    f'{text(n, 14, "#ffffff", 700 if m else 400)}</div>' for p, n, m in members)
    return (f'<div style="width: {w}px; background: {PANEL}; border: 1px solid {LINE}; border-top: 3px solid {tc}; '
            f'box-sizing: border-box; display: flex; flex-direction: column">'
            f'{car_view(car, w - 2, h)}'
            f'<div style="padding: 12px 14px 14px; display: flex; flex-direction: column; gap: 6px">'
            f'<div style="display: flex; justify-content: space-between; align-items: flex-end">'
            f'{big(str(place), 64 if place == 1 else 52, tc)}'
            f'<div style="display: flex; flex-direction: column; align-items: flex-end; gap: 4px">{you}{lab(tag, "#999999", 11)}</div></div>'
            f'{text(team.capitalize(), 20, "#ffffff", 700)}'
            f'{row(mono(avg, 15, "#dddddd"), mono(gap, 13, ACC) if gap else "")}'
            f'<div style="height: 1px; background: {LINE}; margin: 2px 0"></div>'
            f'<div style="display: flex; flex-direction: column; gap: 3px">{chips}</div>'
            f'</div></div>')


def team_spread(teams, lo, hi):
    """How each average came about: a row per team, a dot per driver on the
    time axis (best finish at the left), the average a tick with its gap."""
    span = hi - lo
    out = []
    for team, times, avg, gap in teams:
        tc = TEAM[team]
        dots = ''.join(f'<div style="position: absolute; left: {(t - lo) / span * 100:.1f}%; top: 5px; width: 10px; height: 10px; '
                       f'margin-left: -5px; border-radius: 5px; background: {tc}"></div>' for t in times)
        tick = (f'<div style="position: absolute; left: {(avg - lo) / span * 100:.1f}%; top: 0; width: 2px; height: 20px; '
                f'margin-left: -1px; background: #ffffff"></div>')
        out.append(f'<div style="display: grid; grid-template-columns: 22px 1fr 62px; gap: 10px; align-items: center">'
                   f'{swatch(tc, 10)}'
                   f'<div style="position: relative; height: 20px"><div style="position: absolute; left: 0; right: 0; top: 9px; '
                   f'height: 2px; background: #2c2c2c"></div>{dots}{tick}</div>'
                   f'<div style="text-align: right">{mono(gap, 12, "#ffffff" if gap == "best" else "#bbbbbb")}</div></div>')
    return f'<div style="display: flex; flex-direction: column; gap: 10px">{"".join(out)}</div>'


def team_classification(teams, rows):
    """The teams' averages over the drivers, each driver with the team's square."""
    cells = ''.join(f'<div style="flex: 1; background: {TEAM[t]}; padding: 6px 8px; display: flex; flex-direction: column; gap: 2px">'
                    f'{lab(t.upper(), "#111111", 10)}{mono(avg, 13, "#111111")}</div>' for t, avg in teams)
    grid = 'display: grid; grid-template-columns: 34px 10px 1fr 82px 70px; gap: 10px'
    head = (f'<div style="{grid}; padding: 0 8px; margin: 0 -8px">{lab("POS", "#999999", 11)}<div></div>{lab("DRIVER", "#999999", 11)}'
            f'<div style="text-align: right">{lab("TIME", "#999999", 11)}</div>'
            f'<div style="text-align: right">{lab("BEST", "#999999", 11)}</div></div>')
    out = []
    for pos, team, name, gap, best, me, fastest in rows:
        bg = 'background: rgba(255,85,0,0.18);' if me else ''
        out.append(f'<div style="{grid}; align-items: center; height: 34px; padding: 0 8px; {bg}">'
                   f'{mono(pos, 14, ACC if pos == "01" else "#999999")}{swatch(TEAM[team], 10)}'
                   f'{text(name, 16, "#ffffff", 700 if me else 400, "overflow: hidden; text-overflow: ellipsis")}'
                   f'<div style="text-align: right">{mono(gap, 14, "#dddddd")}</div>'
                   f'<div style="text-align: right">{mono(best, 14, PURPLE if fastest else "#999999")}</div></div>')
    return panel(row(lab('Final classification'), lab('team average', '#999999'))
                 + f'<div style="display: flex; gap: 2px">{cells}</div>' + head
                 + f'<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">{"".join(out)}</div>', gap=12)


team_rows = [('01', 'blue', 'Sam', '4:20.238', '0:50.871', False, True), ('02', 'red', 'Carla', '+1.142', '0:51.204', False, False),
             ('03', 'blue', 'Player', '+3.907', '0:51.660', True, False), ('04', 'green', 'Tony', '+5.415', '0:52.013', False, False),
             ('05', 'red', 'Janine', '+8.770', '0:52.390', False, False), ('06', 'green', 'Ivan', '+11.004', '0:52.881', False, False),
             ('07', 'blue', 'Minori', '+14.512', '0:53.120', False, False), ('08', 'red', 'Bee', '+19.120', '0:53.512', False, False)]

board('EndTeamRacing.dc.html', 'Team race result', None, [
    blurred(BG['racing']),
    f'<div style="position: absolute; left: 56px; top: 48px; display: flex; gap: 16px; align-items: baseline">'
    f'{lab("Team race result", ACC, 13)}{lab("Downtown · 5 laps · 3 teams · 8 drivers", "#999999", 12)}</div>',
    # your team, then you
    f'<div style="position: absolute; left: 56px; top: 92px; width: 380px; display: flex; flex-direction: column; gap: 22px">'
    f'<div style="display: flex; align-items: baseline; gap: 14px">{big("P1", 168, TEAM["blue"])}{big("/ 3", 48, "#777777")}</div>'
    f'<div style="display: flex; flex-direction: column; gap: 6px; margin-top: -18px">{lab("Team blue · average", TEAM["blue"])}'
    f'{big("4:26.378", 44)}{lab("won by 2.070", ACC, 12)}</div>'
    + sep() +
    f'<div style="display: grid; grid-template-columns: 1fr 1fr; gap: 18px 24px">'
    f'{stat("Your place", "P3")}{stat("Your time", "4:24.145")}'
    f'{stat("Vs team average", "-2.233", FAST)}{stat("Best lap", "0:51.660")}</div>'
    + sep() +
    f'<div style="display: flex; flex-direction: column; gap: 12px">{row(lab("Team averages"), lab("a dot a driver · to the winner", "#777777", 11))}'
    + team_spread([('blue', [260.238, 264.145, 274.750], 266.378, 'best'),
                   ('red', [261.380, 269.008, 279.358], 269.915, '+3.537'),
                   ('green', [265.653, 271.242], 268.448, '+2.070')], 258.0, 281.0) + '</div></div>',
    # the teams on the podium
    f'<div style="position: absolute; left: 476px; top: 92px; width: 600px; height: 410px; display: flex; gap: 12px; '
    f'align-items: flex-end">'
    + team_podium_card(2, 'green', 'db12', '4:28.448', '+2.070', [('P4', 'Tony', False), ('P6', 'Ivan', False)], 192, 128)
    + team_podium_card(1, 'blue', 'murcielago', '4:26.378', '', [('P1', 'Sam', False), ('P3', 'Player', True), ('P7', 'Minori', False)], 210, 172, me=True)
    + team_podium_card(3, 'red', 'viper', '4:29.915', '+3.537', [('P2', 'Carla', False), ('P5', 'Janine', False), ('P8', 'Bee', False)], 192, 92)
    + '</div>',
    f'<div style="position: absolute; left: 476px; top: 530px; width: 600px; display: flex; flex-direction: column; gap: 10px">'
    f'{lab("Highlights")}'
    + award('trophy', 'Fastest lap', 'Sam', '0:50.871', PURPLE)
    + award('flag', 'Team player', 'Sam · 6.140 under blue', '-6.140', FAST)
    + award('chevron', 'Biggest gain', 'Tony · P7 → P4', '+3', ACC) + '</div>',
    col('right: 56px; top: 92px', team_classification([('blue', '4:26.378'), ('green', '4:28.448'), ('red', '4:29.915')], team_rows), 420),
    footer('WAITING FOR 5 DRIVERS', 3, 8),
])

# ---------------------------------------------------------------------------
# the end screen of a sprint: one run from A to B, no laps. In place of the
# laps your sectors (what each lost to the quickest anybody drove there) with
# your place at every checkpoint; the classification with the average speed.
# ---------------------------------------------------------------------------

def sector_chart(lost, places):
    """lost: per sector the time lost to the quickest there (0: you were it); places: yours at its end."""
    worst = max(lost)
    cols = []
    for i, (v, p) in enumerate(zip(lost, places)):
        quickest = v == 0
        bar_ = (f'<div style="width: 100%; height: {max(3, int(56 * v / worst))}px; '
                f'background: {FAST if quickest else "#5a5a5a"}"></div>')
        val = mono('best' if quickest else f'+{v:.2f}', 11, FAST if quickest else '#bbbbbb')
        name = 'FIN' if i == len(lost) - 1 else f'S{i + 1}'
        cols.append(f'<div style="flex: 1; display: flex; flex-direction: column; align-items: center; gap: 6px">'
                    f'<div style="height: 56px; width: 100%; display: flex; align-items: flex-end">{bar_}</div>'
                    f'{val}{lab(name, "#777777", 10)}'
                    f'<div style="height: 1px; width: 100%; background: {LINE}"></div>'
                    f'{mono(p, 12, "#ffffff" if p != places[i - 1] or i == 0 else "#777777")}</div>')
    return f'<div style="display: flex; gap: 8px">{"".join(cols)}</div>'


def sprint_classification(rows, right):
    grid = 'display: grid; grid-template-columns: 34px 1fr 82px 70px; gap: 10px'
    head = (f'<div style="{grid}; padding: 0 8px; margin: 0 -8px">{lab("POS", "#999999", 11)}{lab("DRIVER", "#999999", 11)}'
            f'<div style="text-align: right">{lab("TIME", "#999999", 11)}</div>'
            f'<div style="text-align: right">{lab("AVG KM/H", "#999999", 11)}</div></div>')
    out = []
    for pos, name, gap, avg, me in rows:
        bg = 'background: rgba(255,85,0,0.18);' if me else ''
        out.append(f'<div style="{grid}; align-items: center; height: 34px; padding: 0 8px; {bg}">'
                   f'{mono(pos, 14, ACC if pos == "01" else "#999999")}'
                   f'{text(name, 16, "#ffffff", 700 if me else 400, "overflow: hidden; text-overflow: ellipsis")}'
                   f'<div style="text-align: right">{mono(gap, 14, "#dddddd")}</div>'
                   f'<div style="text-align: right">{mono(avg, 14, "#999999")}</div></div>')
    return panel(row(lab('Final classification'), lab(right, '#999999')) + head
                 + f'<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">{"".join(out)}</div>', gap=12)


SPRINT_M = 2980
sprint_times = [('Sam', 87.260), ('Player', 88.402), ('Carla', 91.167), ('Tony', 91.381), ('Janine', 97.030),
                ('Ivan', 99.264), ('Minori', 105.772)]


def kmh(t):
    return f'{SPRINT_M / t * 3.6:.1f}'


sprint_rows = [(f'{i + 1:02d}', n, '1:27.260' if i == 0 else f'+{t - sprint_times[0][1]:.3f}', kmh(t), n == 'Player')
               for i, (n, t) in enumerate(sprint_times)] + [('08', 'Bee', 'DNF', 'CP 5', False)]

board('EndSprint.dc.html', 'Sprint result', None, [
    blurred(BG['sprint']),
    f'<div style="position: absolute; left: 56px; top: 48px; display: flex; gap: 16px; align-items: baseline">'
    f'{lab("Sprint result", ACC, 13)}{lab("Timber Run · 2.98 km · 7 checkpoints · 8 drivers", "#999999", 12)}</div>',
    # your result
    f'<div style="position: absolute; left: 56px; top: 92px; width: 380px; display: flex; flex-direction: column; gap: 22px">'
    f'<div style="display: flex; align-items: baseline; gap: 14px">{big("P2", 168)}{big("/ 8", 48, "#777777")}</div>'
    f'<div style="display: flex; flex-direction: column; gap: 6px; margin-top: -18px">{lab("Finished")}'
    f'{big("1:28.402", 44)}{lab("+1.142 to the winner", ACC, 12)}</div>'
    + sep() +
    f'<div style="display: grid; grid-template-columns: 1fr 1fr; gap: 18px 24px">'
    f'{stat("Avg speed", kmh(88.402), "#ffffff", "km/h")}{stat("Top speed", "172", "#ffffff", "km/h")}'
    f'{stat("Places gained", "+3", FAST)}{stat("Sectors quickest", "2 / 8", FAST)}</div>'
    + sep() +
    f'<div style="display: flex; flex-direction: column; gap: 12px">{row(lab("Your sectors"), lab("lost to the quickest · your place", "#777777", 11))}'
    + sector_chart([0.42, 0.18, 0.0, 0.31, 0.12, 0.0, 0.27, 0.09], ['P4', 'P4', 'P3', 'P3', 'P3', 'P2', 'P2', 'P2']) + '</div></div>',
    # the podium
    f'<div style="position: absolute; left: 476px; top: 92px; width: 600px; height: 410px; display: flex; gap: 12px; '
    f'align-items: flex-end">'
    + podium_card(2, 'Player', 'viper', '1:28.402', '+1.142', 192, 176, me=True)
    + podium_card(1, 'Sam', 'murcielago', '1:27.260', '', 210, 236)
    + podium_card(3, 'Carla', 'db12', '1:31.167', '+3.907', 192, 140) + '</div>',
    f'<div style="position: absolute; left: 476px; top: 530px; width: 600px; display: flex; flex-direction: column; gap: 10px">'
    f'{lab("Highlights")}'
    + award('flag', 'Best sector', 'Player · S3', '0:09.814', FAST)
    + award('trophy', 'Closest finish', 'Tony · behind Carla', '0.214', PURPLE)
    + award('chevron', 'Biggest gain', 'Player · P5 to P2', '+3', ACC) + '</div>',
    col('right: 56px; top: 92px', sprint_classification(sprint_rows, '2.98 km'), 420),
    footer('WAITING FOR 5 DRIVERS', 3, 8),
])

# ---------------------------------------------------------------------------
# the end screen of a ghost race: you against a recorded lap. The big figure
# is the margin; in place of the laps your best lap against the ghost, sector
# by sector (blue gained, orange lost) with the running gap under it; in the
# middle you and the ghost side by side; at the right everybody's result
# against their ghost and the ghost ladder with your time in it.
# ---------------------------------------------------------------------------

def ghost_sector_chart(deltas, names):
    """deltas: per sector your time minus the ghost's (negative: gained); under it the running gap."""
    worst = max(abs(v) for v in deltas)
    run, cols = 0.0, []
    for v, name in zip(deltas, names):
        run += v
        h = max(2, int(26 * abs(v) / worst))
        gained = v <= 0
        c = FAST if gained else ACC
        up = f'<div style="width: 100%; height: {h if gained else 0}px; background: {c}"></div>'
        down = f'<div style="width: 100%; height: {0 if gained else h}px; background: {c}"></div>'
        cols.append(f'<div style="flex: 1; display: flex; flex-direction: column; align-items: center; gap: 6px">'
                    f'<div style="height: 56px; width: 100%; display: flex; flex-direction: column">'
                    f'<div style="height: 27px; display: flex; align-items: flex-end">{up}</div>'
                    f'<div style="height: 2px; background: #777777"></div>'
                    f'<div style="height: 27px; display: flex; align-items: flex-start">{down}</div></div>'
                    f'{mono(f"{v:+.2f}", 11, c)}{lab(name, "#777777", 10)}'
                    f'<div style="height: 1px; width: 100%; background: {LINE}"></div>'
                    f'{mono(f"{run:+.2f}", 12, FAST if run <= 0 else ACC)}</div>')
    return f'<div style="display: flex; gap: 8px">{"".join(cols)}</div>'


def duel_card(tag, tag_color, name, car, lap, note, note_color, ghost=False):
    view = car_view(car, 266, 250)
    if ghost:
        view = (f'<div style="position: relative; opacity: 0.55; filter: saturate(0.2)">{view}'
                f'<div style="position: absolute; inset: 0; background: rgba(90,176,255,0.16)"></div></div>')
    return (f'<div style="width: 268px; background: {PANEL}; border: 1px solid {LINE}; border-top: 2px solid {tag_color}; '
            f'box-sizing: border-box; display: flex; flex-direction: column">{view}'
            f'<div style="padding: 12px 14px 14px; display: flex; flex-direction: column; gap: 6px">'
            f'{lab(tag, tag_color, 11)}{text(name, 20, "#ffffff", 700)}'
            f'{row(big(lap, 34), mono(note, 13, note_color))}</div></div>')


def ghost_results(rows):
    grid = 'display: grid; grid-template-columns: 34px 1fr 82px 70px; gap: 10px'
    head = (f'<div style="{grid}; padding: 0 8px; margin: 0 -8px">{lab("POS", "#999999", 11)}{lab("DRIVER", "#999999", 11)}'
            f'<div style="text-align: right">{lab("BEST", "#999999", 11)}</div>'
            f'<div style="text-align: right">{lab("VS GHOST", "#999999", 11)}</div></div>')
    out = []
    for pos, name, best, vs, me in rows:
        bg = 'background: rgba(255,85,0,0.18);' if me else ''
        out.append(f'<div style="{grid}; align-items: center; height: 34px; padding: 0 8px; {bg}">'
                   f'{mono(pos, 14, ACC if pos == "01" else "#999999")}'
                   f'{text(name, 16, "#ffffff", 700 if me else 400, "overflow: hidden; text-overflow: ellipsis")}'
                   f'<div style="text-align: right">{mono(best, 14, "#dddddd")}</div>'
                   f'<div style="text-align: right">{mono(vs, 14, FAST if vs.startswith("-") else ACC)}</div></div>')
    return panel(row(lab('Results against the ghost'), lab('5 laps', '#999999')) + head
                 + f'<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">{"".join(out)}</div>', gap=12)


def ghost_ladder(rows, right):
    """The ladder's ghosts for this track, your best lap put in where it ranks."""
    grid = 'display: grid; grid-template-columns: 34px 1fr 82px 40px; gap: 10px'
    out = []
    for pos, name, lap, tag, me in rows:
        bg = 'background: rgba(255,85,0,0.18);' if me else ''
        out.append(f'<div style="{grid}; align-items: center; height: 30px; padding: 0 8px; {bg}">'
                   f'{mono(pos, 13, "#ffffff" if me else "#999999")}'
                   f'{text(name, 15, "#ffffff", 700 if me else 400, "overflow: hidden; text-overflow: ellipsis")}'
                   f'<div style="text-align: right">{mono(lap, 13, "#dddddd" if me else "#999999")}</div>'
                   f'<div style="text-align: right">{lab(tag, FAST, 10) if tag else ""}</div></div>')
    return panel(row(lab('Ghost ladder'), lab(right, '#999999')) +
                 f'<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">{"".join(out)}</div>', gap=12)


board('EndGhost.dc.html', 'Ghost race result', None, [
    blurred(BG['racing']),
    f'<div style="position: absolute; left: 56px; top: 48px; display: flex; gap: 16px; align-items: baseline">'
    f'{lab("Ghost race result", ACC, 13)}{lab("Country · 5 laps · 3 drivers", "#999999", 12)}</div>',
    # your result: the margin to the ghost
    f'<div style="position: absolute; left: 56px; top: 92px; width: 380px; display: flex; flex-direction: column; gap: 22px">'
    f'<div style="display: flex; align-items: baseline; gap: 14px">{big("-0.842", 120, FAST)}</div>'
    f'<div style="display: flex; flex-direction: column; gap: 6px; margin-top: -10px">{lab("Best lap")}'
    f'{big("1:20.558", 44)}{lab("Ghost beaten · Rocket 1:21.400", FAST, 12)}</div>'
    + sep() +
    f'<div style="display: grid; grid-template-columns: 1fr 1fr; gap: 18px 24px">'
    f'{stat("Ladder place", "#3", "#ffffff", "of 42")}{stat("Personal best", "-0.31", FAST, "new")}'
    f'{stat("Top speed", "172", "#ffffff", "km/h")}{stat("Laps faster", "3 / 5", FAST)}</div>'
    + sep() +
    f'<div style="display: flex; flex-direction: column; gap: 12px">{row(lab("Your best lap"), lab("against the ghost · running gap", "#777777", 11))}'
    + ghost_sector_chart([-0.21, -0.48, 0.12, -0.05, -0.31, 0.088], ['S1', 'S2', 'S3', 'S4', 'S5', 'FIN']) + '</div></div>',
    # you and the ghost
    f'<div style="position: absolute; left: 476px; top: 92px; width: 600px; display: flex; gap: 64px; align-items: flex-start">'
    + duel_card('YOU', ACC, 'Player', 'viper', '1:20.558', '-0.842', FAST)
    + duel_card('GHOST · LADDER #3', FAST, 'Rocket', 'murcielago', '1:21.400', 'Murcielago', '#999999', ghost=True) + '</div>',
    f'<div style="position: absolute; left: 744px; top: 196px; width: 64px; text-align: center">{big("VS", 34, "#777777")}</div>',
    f'<div style="position: absolute; left: 476px; top: 530px; width: 600px; display: flex; flex-direction: column; gap: 10px">'
    f'{lab("Highlights")}'
    + award('flag', 'Biggest gain', 'S2 · against the ghost', '-0.48', FAST)
    + award('trophy', 'Fastest lap', 'Player · lap 4', '1:20.558', PURPLE)
    + award('chevron', 'Personal best', 'Player · new ghost saved', '-0.31', '#e6e6e6') + '</div>',
    col('right: 56px; top: 92px', ghost_results([('01', 'Player', '1:20.558', '-0.842', True),
                                                  ('02', 'Carla', '1:21.902', '+0.214', False),
                                                  ('03', 'Tony', '1:23.115', '-0.310', False)])
        + ghost_ladder([('#1', 'Vlad', '1:19.904', '', False), ('#2', 'Sam', '1:20.311', '', False),
                        ('#3', 'Player', '1:20.558', '', True), ('#4', 'Rocket', '1:21.400', 'VS', False),
                        ('#5', 'Minori', '1:21.977', '', False), ('#6', 'Bee', '1:22.630', '', False)], '42 ghosts'), 420, gap=16),
    footer('WAITING FOR 2 DRIVERS', 1, 3),
])

# ---------------------------------------------------------------------------
# the end screens of the combat modes: Deathmatch, Derby (also Last Car
# Standing) and Team Deathmatch. The race's page: your place and score, in
# place of the laps the match minute by minute (your frags or damage, under
# it your place, the drivers left or your team's lead); the podium with the
# cars; the highlights from the kills the client writes down; the
# classification with the mode's columns.
# ---------------------------------------------------------------------------

def minute_chart(values, labels, unders, under_colors=None, bar_color='#5a5a5a', best_color='#e6e6e6'):
    """A bar a minute (the best one lit), its value, the minute; under a rule what it ended with."""
    top = max(values)
    cols = []
    for i, (v, name, u) in enumerate(zip(values, labels, unders)):
        lit = v == top
        uc = under_colors[i] if under_colors else ('#ffffff' if i == 0 or u != unders[i - 1] else '#777777')
        cols.append(f'<div style="flex: 1; display: flex; flex-direction: column; align-items: center; gap: 6px">'
                    f'<div style="height: 56px; width: 100%; display: flex; align-items: flex-end">'
                    f'<div style="width: 100%; height: {max(3, int(56 * v / top))}px; background: {best_color if lit else bar_color}"></div></div>'
                    f'{mono(f"{v:,}", 11, "#ffffff" if lit else "#bbbbbb")}{lab(name, "#777777", 10)}'
                    f'<div style="height: 1px; width: 100%; background: {LINE}"></div>'
                    f'{mono(u, 12, uc)}</div>')
    return f'<div style="display: flex; gap: 8px">{"".join(cols)}</div>'


def combat_list(title, right, heads, rows, teams=None):
    """rows: (pos, team or None, name, score, last, last colour, me); teams: (team, score) cells over it."""
    sw = teams is not None
    grid = ('display: grid; grid-template-columns: 34px 10px 1fr 82px 70px; gap: 10px' if sw
            else 'display: grid; grid-template-columns: 34px 1fr 82px 70px; gap: 10px')
    head = (f'<div style="{grid}; padding: 0 8px; margin: 0 -8px">{lab(heads[0], "#999999", 11)}{"<div></div>" if sw else ""}'
            f'{lab(heads[1], "#999999", 11)}'
            f'<div style="text-align: right">{lab(heads[2], "#999999", 11)}</div>'
            f'<div style="text-align: right">{lab(heads[3], "#999999", 11)}</div></div>')
    out = []
    for pos, team, name, score, last, lc, me in rows:
        bg = 'background: rgba(255,85,0,0.18);' if me else ''
        out.append(f'<div style="{grid}; align-items: center; height: 34px; padding: 0 8px; {bg}">'
                   f'{mono(pos, 14, ACC if pos == "01" else "#999999")}{swatch(TEAM[team], 10) if sw else ""}'
                   f'{text(name, 16, "#ffffff", 700 if me else 400, "overflow: hidden; text-overflow: ellipsis")}'
                   f'<div style="text-align: right">{mono(score, 14, "#dddddd")}</div>'
                   f'<div style="text-align: right">{mono(last, 14, lc)}</div></div>')
    cells = ''
    if sw:
        cells = ('<div style="display: flex; gap: 2px">' + ''.join(
            f'<div style="flex: 1; background: {TEAM[t]}; padding: 6px 8px; display: flex; flex-direction: column; gap: 2px">'
            f'{lab(t.upper(), "#111111", 10)}{mono(v, 13, "#111111")}</div>' for t, v in teams) + '</div>')
    return panel(row(lab(title), lab(right, '#999999')) + cells + head
                 + f'<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">{"".join(out)}</div>', gap=12)


def end_left(big_text, of, big_color, done_label, done_value, note, note_color, stats, chart_title, chart_note, chart,
             label_color='#999999'):
    return (f'<div style="position: absolute; left: 56px; top: 92px; width: 380px; display: flex; flex-direction: column; gap: 22px">'
            f'<div style="display: flex; align-items: baseline; gap: 14px">{big(big_text, 168, big_color)}{big(of, 48, "#777777")}</div>'
            f'<div style="display: flex; flex-direction: column; gap: 6px; margin-top: -18px">{lab(done_label, label_color)}'
            f'{big(done_value, 44)}{lab(note, note_color, 12)}</div>'
            + sep() +
            f'<div style="display: grid; grid-template-columns: 1fr 1fr; gap: 18px 24px">{"".join(stat(*s) for s in stats)}</div>'
            + sep() +
            f'<div style="display: flex; flex-direction: column; gap: 12px">{row(lab(chart_title), lab(chart_note, "#777777", 11))}'
            + chart + '</div></div>')


def end_header(title, sub):
    return (f'<div style="position: absolute; left: 56px; top: 48px; display: flex; gap: 16px; align-items: baseline">'
            f'{lab(title, ACC, 13)}{lab(sub, "#999999", 12)}</div>')


def end_awards(awards):
    return (f'<div style="position: absolute; left: 476px; top: 530px; width: 600px; display: flex; flex-direction: column; gap: 10px">'
            f'{lab("Highlights")}' + ''.join(award(*a) for a in awards) + '</div>')


def podium3(cards):
    return (f'<div style="position: absolute; left: 476px; top: 92px; width: 600px; height: 410px; display: flex; gap: 12px; '
            f'align-items: flex-end">' + ''.join(cards) + '</div>')


board('EndDeathmatch.dc.html', 'Deathmatch result', None, [
    blurred(BG['dm']),
    end_header('Deathmatch result', 'Battle Canyon · fraglimit 15 · 6 drivers'),
    end_left('P2', '/ 6', '#ffffff', 'Score', '13', '2 behind the winner', ACC,
             [('Kills', '13'), ('Deaths', '9'), ('Best streak', '4', FAST), ('Damage dealt', '2,140')],
             'Your frags', 'a bar a minute · your place',
             minute_chart([2, 1, 3, 0, 2, 4, 1], [f'M{i}' for i in range(1, 8)], ['P3', 'P4', 'P2', 'P3', 'P3', 'P2', 'P2'])),
    podium3([podium_card(2, 'Player', 'viper', '13 frags', '-2', 192, 176, me=True),
             podium_card(1, 'Sam', 'murcielago', '15 frags', '', 210, 236),
             podium_card(3, 'Carla', 'db12', '11 frags', '-4', 192, 140)]),
    end_awards([('trophy', 'Best streak', 'Sam · 6 in a row', '6', PURPLE),
                ('gun', 'Most damage', 'Carla · dealt', '2,880', FAST),
                ('chevron', 'Your nemesis', 'Sam · got you 4 times', '4', ACC)]),
    col('right: 56px; top: 92px', combat_list('Final classification', 'fraglimit 15', ['POS', 'DRIVER', 'SCORE', 'K / D'], [
        ('01', None, 'Sam', '15', '15 / 7', '#999999', False), ('02', None, 'Player', '13', '13 / 9', '#999999', True),
        ('03', None, 'Carla', '11', '11 / 10', '#999999', False), ('04', None, 'Tony', '8', '8 / 12', '#999999', False),
        ('05', None, 'Janine', '6', '6 / 11', '#999999', False), ('06', None, 'Ivan', '3', '3 / 14', '#999999', False)]), 420),
    footer('WAITING FOR 3 DRIVERS', 1, 4),
])

board('EndDerby.dc.html', 'Derby result', None, [
    blurred(BG['lcs']),
    end_header('Derby result', 'David5 · 6 drivers'),
    end_left('P2', '/ 6', '#ffffff', 'Survived', '3:42', 'out 12 s before Chloe', ACC,
             [('Score', '840'), ('Wrecked', '2', FAST), ('Damage dealt', '1,260'), ('Damage taken', '980')],
             'Your damage', 'a bar a minute · drivers left',
             minute_chart([180, 320, 240, 410], ['M1', 'M2', 'M3', 'M4'], ['6', '5', '3', '2'])),
    podium3([podium_card(2, 'Player', 'viper', '840 pts', 'out 3:42', 192, 176, me=True),
             podium_card(1, 'Chloe', 'murcielago', '1,120 pts', '', 210, 236),
             podium_card(3, 'Josh', 'db12', '610 pts', 'out 2:58', 192, 140)]),
    end_awards([('trophy', 'Last car standing', 'Chloe · 3:54', '1,120', ACC),
                ('gun', 'Most damage', 'Player · dealt', '1,260', FAST),
                ('chevron', 'Took you out', 'Chloe · 3:42', '', ACC)]),
    col('right: 56px; top: 92px', combat_list('Final classification', 'match 3:54', ['POS', 'DRIVER', 'SCORE', 'OUT'], [
        ('01', None, 'Chloe', '1,120', 'LAST', ACC, False), ('02', None, 'Player', '840', '3:42', '#999999', True),
        ('03', None, 'Josh', '610', '2:58', '#999999', False), ('04', None, 'Sam', '400', '2:11', '#999999', False),
        ('05', None, 'Minori', '280', '1:20', '#999999', False), ('06', None, 'Ivan', '90', '0:47', '#999999', False)]), 420),
    footer('WAITING FOR 3 DRIVERS', 1, 4),
])

board('EndTeamDM.dc.html', 'Team deathmatch result', None, [
    blurred(BG['team_dm']),
    end_header('Team deathmatch result', 'Arena · fraglimit 42 · 2 teams · 8 drivers'),
    end_left('P1', '/ 2', TEAM['blue'], 'Team blue · score', '42', 'won by 5', ACC,
             [('Your place', 'P2'), ('Your frags', '12'), ('Deaths', '7'), ('Best streak', '3', FAST)],
             'Your frags', 'a bar a minute · your team\'s lead',
             minute_chart([1, 2, 3, 1, 2, 3], [f'M{i}' for i in range(1, 7)], ['+1', '+3', '+2', '-1', '+3', '+5'],
                          [FAST, FAST, FAST, ACC, FAST, FAST]), label_color=TEAM['blue']),
    podium3([team_podium_card(2, 'red', 'viper', '37 frags', '-5', [('P1', 'Carla', False), ('P4', 'Tony', False),
                                                                      ('P7', 'Janine', False), ('P8', 'Bee', False)], 192, 128),
             team_podium_card(1, 'blue', 'murcielago', '42 frags', '', [('P2', 'Player', True), ('P3', 'Sam', False),
                                                                          ('P5', 'Minori', False), ('P6', 'Ivan', False)], 210, 172, me=True)]),
    end_awards([('trophy', 'Best streak', 'Carla · 5 in a row', '5', PURPLE),
                ('gun', 'Most damage', 'Sam · dealt', '2,410', FAST),
                ('chevron', 'Your nemesis', 'Carla · got you 3 times', '3', ACC)]),
    col('right: 56px; top: 92px', combat_list('Final classification', 'team score', ['POS', 'DRIVER', 'SCORE', 'K / D'], [
        ('01', 'red', 'Carla', '14', '14 / 6', '#999999', False), ('02', 'blue', 'Player', '12', '12 / 7', '#999999', True),
        ('03', 'blue', 'Sam', '11', '11 / 8', '#999999', False), ('04', 'red', 'Tony', '10', '10 / 9', '#999999', False),
        ('05', 'blue', 'Minori', '10', '10 / 10', '#999999', False), ('06', 'blue', 'Ivan', '9', '9 / 12', '#999999', False),
        ('07', 'red', 'Janine', '8', '8 / 11', '#999999', False), ('08', 'red', 'Bee', '5', '5 / 15', '#999999', False)],
        teams=[('blue', '42'), ('red', '37')]), 420),
    footer('WAITING FOR 5 DRIVERS', 3, 8),
])

# ---------------------------------------------------------------------------
# the race variants: Racing DM (a race with weapons: the race's page with
# frags and deaths; Team Racing DM the team race's page the same way) and
# Elimination (the last one out at the end of every lap: the order is who
# lasted longest, under the laps how many were left).
# ---------------------------------------------------------------------------

def lap_chart_under(laps, unders):
    """The race's lap chart with a line under it: what each lap ended with."""
    best = min(laps)
    worst = max(laps) - best
    cols = []
    for i, (v, u) in enumerate(zip(laps, unders)):
        cols.append(f'<div style="flex: 1; display: flex; flex-direction: column; align-items: center; gap: 6px">'
                    f'<div style="height: 56px; width: 100%; display: flex; align-items: flex-end">'
                    f'<div style="width: 100%; height: {max(3, int(56 * (v - best) / worst))}px; '
                    f'background: {"#e6e6e6" if v == best else "#5a5a5a"}"></div></div>'
                    f'{mono("best" if v == best else f"+{v - best:.2f}", 11, "#ffffff" if v == best else "#bbbbbb")}'
                    f'{lab(f"L{i + 1}", "#777777", 10)}'
                    f'<div style="height: 1px; width: 100%; background: {LINE}"></div>'
                    f'{mono(u, 12, "#ffffff" if i == 0 or u != unders[i - 1] else "#777777")}</div>')
    return f'<div style="display: flex; gap: 8px">{"".join(cols)}</div>'


board('EndRacingDM.dc.html', 'Racing DM result', None, [
    blurred(BG['racing_dm']),
    end_header('Racing DM result', 'Downtown · 5 laps · 8 drivers'),
    end_left('P2', '/ 8', '#ffffff', 'Finished', '4:21.380', '+1.142 to the winner', ACC,
             [('Best lap', '0:51.204'), ('Top speed', '214', '#ffffff', 'km/h'), ('Frags', '4', FAST), ('Deaths', '2')],
             'Your laps', 'lost to your best', lap_chart([53.81, 52.42, 51.20, 51.66, 52.29])),
    podium3([podium_card(2, 'Player', 'viper', '4:21.380', '+1.142', 192, 176, me=True),
             podium_card(1, 'Sam', 'murcielago', '4:20.238', '', 210, 236),
             podium_card(3, 'Carla', 'db12', '4:24.145', '+3.907', 192, 140)]),
    end_awards([('trophy', 'Fastest lap', 'Sam', '0:50.871', PURPLE),
                ('gun', 'Most frags', 'Carla · 6 frags', '6', FAST),
                ('chevron', 'Your nemesis', 'Tony · got you 2 times', '2', ACC)]),
    col('right: 56px; top: 92px', combat_list('Final classification', '5 laps', ['POS', 'DRIVER', 'TIME', 'K / D'], [
        ('01', None, 'Sam', '4:20.238', '3 / 1', '#999999', False), ('02', None, 'Player', '+1.142', '4 / 2', '#999999', True),
        ('03', None, 'Carla', '+3.907', '6 / 1', '#999999', False), ('04', None, 'Tony', '+9.415', '2 / 3', '#999999', False),
        ('05', None, 'Janine', '+12.770', '1 / 2', '#999999', False), ('06', None, 'Ivan', '+21.004', '0 / 4', '#999999', False),
        ('07', None, 'Minori', '+1 LAP', '1 / 3', '#999999', False), ('08', None, 'Bee', 'DNF', '0 / 1', '#999999', False)]), 420),
    footer('WAITING FOR 5 DRIVERS', 3, 8),
])

board('EndElimination.dc.html', 'Elimination result', None, [
    blurred(BG['racing']),
    end_header('Elimination result', 'Country · the last one out every lap · 8 drivers'),
    end_left('P2', '/ 8', '#ffffff', 'Survived', '6:12', 'out in the last lap · Sam won', ACC,
             [('Best lap', '0:51.204'), ('Top speed', '198', '#ffffff', 'km/h'), ('Laps survived', '6 / 7'), ('Places gained', '+3', FAST)],
             'Your laps', 'lost to your best · drivers left',
             lap_chart_under([52.93, 51.88, 51.20, 51.62, 51.47, 51.81], ['7', '6', '5', '4', '3', '2'])),
    podium3([podium_card(2, 'Player', 'viper', '6 laps', 'out lap 7', 192, 176, me=True),
             podium_card(1, 'Sam', 'murcielago', '7 laps', '', 210, 236),
             podium_card(3, 'Carla', 'db12', '5 laps', 'out lap 6', 192, 140)]),
    end_awards([('trophy', 'Fastest lap', 'Sam', '0:50.871', PURPLE),
                ('flag', 'First out', 'Bee · lap 1', '', ACC),
                ('chevron', 'Biggest gain', 'Player · P5 to P2', '+3', FAST)]),
    col('right: 56px; top: 92px', combat_list('Final classification', '7 laps', ['POS', 'DRIVER', 'OUT', 'BEST'], [
        ('01', None, 'Sam', 'WINNER', '0:50.871', PURPLE, False), ('02', None, 'Player', 'LAP 7', '0:51.204', '#999999', True),
        ('03', None, 'Carla', 'LAP 6', '0:51.660', '#999999', False), ('04', None, 'Tony', 'LAP 5', '0:52.013', '#999999', False),
        ('05', None, 'Janine', 'LAP 4', '0:52.390', '#999999', False), ('06', None, 'Ivan', 'LAP 3', '0:52.881', '#999999', False),
        ('07', None, 'Minori', 'LAP 2', '0:53.512', '#999999', False), ('08', None, 'Bee', 'LAP 1', '0:54.120', '#999999', False)]), 420),
    footer('WAITING FOR 5 DRIVERS', 3, 8),
])

# ---------------------------------------------------------------------------
# the objective modes: the team deathmatch's page with what each team played
# for. CTF the captures a minute, Domination the zones held a minute (four
# teams: the podium the first three, the list all four), King of the Hill the
# seconds on the hill a minute, Autoball the goals one by one.
# ---------------------------------------------------------------------------

def lead_colors(leads):
    return [FAST if v.startswith('+') else ACC if v.startswith('-') else '#999999' for v in leads]


def goal_list(goals):
    """The goals one by one: the minute, who scored (his team's swatch), the shot, the score after it."""
    out = []
    for minute, team, name, kmh_, score in goals:
        out.append(f'<div style="display: grid; grid-template-columns: 30px 10px 1fr 70px 44px; gap: 10px; align-items: center; height: 22px">'
                   f'{mono(minute, 12, "#999999")}{swatch(TEAM[team], 10)}'
                   f'{text(name, 14, "#ffffff", 700 if name == "Player" else 400, "overflow: hidden; text-overflow: ellipsis")}'
                   f'<div style="text-align: right">{mono(kmh_ + " km/h", 12, "#999999")}</div>'
                   f'<div style="text-align: right">{mono(score, 13, "#ffffff")}</div></div>')
    return f'<div style="display: flex; flex-direction: column; gap: 2px">{"".join(out)}</div>'


CTF_LEADS = ['-1', '0', '-1', '+1', '+2', '+1', '+2']
board('EndCTF.dc.html', 'Capture the flag result', None, [
    blurred(BG['team_dm']),
    end_header('Capture the flag result', 'Capture in the Jungle · capturelimit 5 · 2 teams · 8 drivers'),
    end_left('P1', '/ 2', TEAM['blue'], 'Team blue · captures', '5', 'won by 2', ACC,
             [('Your captures', '2', FAST), ('Assists', '1'), ('Defends', '3'), ('Your frags', '9')],
             'Team captures', 'a bar a minute · your team\'s lead',
             minute_chart([0, 1, 0, 2, 1, 0, 1], [f'M{i}' for i in range(1, 8)], CTF_LEADS, lead_colors(CTF_LEADS)),
             label_color=TEAM['blue']),
    podium3([team_podium_card(2, 'red', 'viper', '3 caps', '-2', [('P1', 'Carla', False), ('P5', 'Tony', False),
                                                                   ('P6', 'Janine', False), ('P8', 'Bee', False)], 192, 128),
             team_podium_card(1, 'blue', 'murcielago', '5 caps', '', [('P2', 'Ivan', False), ('P3', 'Player', True),
                                                                       ('P4', 'Sam', False), ('P7', 'Minori', False)], 210, 172, me=True)]),
    end_awards([('flag', 'Most captures', 'Ivan · 3 captures', '3', FAST),
                ('trophy', 'Best defender', 'Carla · 6 defends', '6', PURPLE),
                ('chevron', 'Your nemesis', 'Carla · got you 3 times', '3', ACC)]),
    col('right: 56px; top: 92px', combat_list('Final classification', 'capturelimit 5', ['POS', 'DRIVER', 'SCORE', 'CAPS'], [
        ('01', 'red', 'Carla', '41', '1', '#999999', False), ('02', 'blue', 'Ivan', '38', '3', '#999999', False),
        ('03', 'blue', 'Player', '33', '2', '#999999', True), ('04', 'blue', 'Sam', '24', '0', '#999999', False),
        ('05', 'red', 'Tony', '21', '2', '#999999', False), ('06', 'red', 'Janine', '15', '0', '#999999', False),
        ('07', 'blue', 'Minori', '12', '0', '#999999', False), ('08', 'red', 'Bee', '6', '0', '#999999', False)],
        teams=[('blue', '5'), ('red', '3')]), 420),
    footer('WAITING FOR 5 DRIVERS', 3, 8),
])

board('EndDomination.dc.html', 'Domination result', None, [
    blurred(BG['koth']),
    end_header('Domination result', 'Dominate the City · timelimit 8 · 4 teams · 8 drivers'),
    end_left('P2', '/ 4', TEAM['blue'], 'Team blue · score', '131', '12 behind red', ACC,
             [('Your captures', '4', FAST), ('Your frags', '8'), ('Deaths', '6'), ('Zones held', '2.1', '#ffffff', 'avg')],
             'Zones held', 'a bar a minute · your team\'s place',
             minute_chart([1, 2, 3, 2, 1, 2, 3, 3], [f'M{i}' for i in range(1, 9)], ['P3', 'P2', 'P1', 'P1', 'P2', 'P2', 'P2', 'P2']),
             label_color=TEAM['blue']),
    podium3([team_podium_card(2, 'blue', 'viper', '131 pts', '-12', [('P2', 'Player', True), ('P4', 'Kuro', False)], 192, 128, me=True),
             team_podium_card(1, 'red', 'murcielago', '143 pts', '', [('P1', 'Dean', False), ('P3', 'Tanya', False)], 210, 172),
             team_podium_card(3, 'green', 'db12', '64 pts', '-79', [('P5', 'Janine', False), ('P7', 'Ivan', False)], 192, 92)]),
    end_awards([('hill', 'Longest hold', 'Red · zone C', '4:12', PURPLE),
                ('gun', 'Most frags', 'Dean · 14 frags', '14', FAST),
                ('chevron', 'Your nemesis', 'Dean · got you 3 times', '3', ACC)]),
    col('right: 56px; top: 92px', combat_list('Final classification', 'team score', ['POS', 'DRIVER', 'SCORE', 'K / D'], [
        ('01', 'red', 'Dean', '41', '14 / 5', '#999999', False), ('02', 'blue', 'Player', '38', '8 / 6', '#999999', True),
        ('03', 'red', 'Tanya', '30', '9 / 7', '#999999', False), ('04', 'blue', 'Kuro', '27', '7 / 9', '#999999', False),
        ('05', 'green', 'Janine', '22', '6 / 8', '#999999', False), ('06', 'yellow', 'Carla', '17', '5 / 9', '#999999', False),
        ('07', 'green', 'Ivan', '12', '3 / 10', '#999999', False), ('08', 'yellow', 'Bee', '9', '2 / 10', '#999999', False)],
        teams=[('red', '143'), ('blue', '131'), ('green', '64'), ('yellow', '40')]), 420),
    footer('WAITING FOR 5 DRIVERS', 3, 8),
])

KOTH_LEADS = ['-3', '+2', '+9', '+13', '+18', '+16', '+22']
board('EndKOTH.dc.html', 'King of the hill result', None, [
    blurred(BG['koth']),
    end_header('King of the hill result', 'Mesa King · timelimit 7 · 2 teams · 6 drivers'),
    end_left('P1', '/ 2', TEAM['red'], 'Team red · score', '52', 'won by 22', ACC,
             [('Hill time', '1:48'), ('Contested', '0:42'), ('Hill kills', '5', FAST), ('Your frags', '9')],
             'Hill held', 'seconds a minute · your team\'s lead',
             minute_chart([12, 38, 60, 44, 51, 20, 47], [f'M{i}' for i in range(1, 8)], KOTH_LEADS, lead_colors(KOTH_LEADS)),
             label_color=TEAM['red']),
    podium3([team_podium_card(2, 'blue', 'viper', '30 pts', '-22', [('P4', 'Paul', False), ('P5', 'Minori', False),
                                                                     ('P6', 'Bee', False)], 192, 128),
             team_podium_card(1, 'red', 'murcielago', '52 pts', '', [('P1', 'Ivan', False), ('P2', 'Tanya', False),
                                                                      ('P3', 'Player', True)], 210, 172, me=True)]),
    end_awards([('hill', 'King of the hill', 'Ivan · 2:41 on the hill', '2:41', PURPLE),
                ('gun', 'Most hill kills', 'Tanya · 7 on the hill', '7', FAST),
                ('chevron', 'Longest reign', 'Red · held it 1:52', '1:52', ACC)]),
    col('right: 56px; top: 92px', combat_list('Final classification', 'team score', ['POS', 'DRIVER', 'SCORE', 'HILL'], [
        ('01', 'red', 'Ivan', '28', '2:41', '#999999', False), ('02', 'red', 'Tanya', '24', '2:05', '#999999', False),
        ('03', 'red', 'Player', '19', '1:48', '#999999', True), ('04', 'blue', 'Paul', '11', '1:12', '#999999', False),
        ('05', 'blue', 'Minori', '10', '0:58', '#999999', False), ('06', 'blue', 'Bee', '6', '0:31', '#999999', False)],
        teams=[('red', '52'), ('blue', '30')]), 420),
    footer('WAITING FOR 3 DRIVERS', 3, 6),
])

board('EndAutoball.dc.html', 'Autoball result', None, [
    blurred(BG['autoball']),
    end_header('Autoball result', 'Rooftop Arena · timelimit 8 · 2 teams · 6 drivers'),
    end_left('P1', '/ 2', TEAM['blue'], 'Team blue · goals', '3', 'won 3:2', ACC,
             [('Your goals', '1', FAST), ('Assists', '1'), ('Saves', '2'), ('Best shot', '112', '#ffffff', 'km/h')],
             'Goals', 'minute · scorer · shot · red : blue',
             goal_list([("2'", 'red', 'Vlad', '96', '1:0'), ("3'", 'blue', 'Player', '112', '1:1'),
                        ("5'", 'blue', 'Steve', '88', '1:2'), ("6'", 'red', 'Vlad', '104', '2:2'),
                        ("8'", 'blue', 'Steve', '91', '2:3')]),
             label_color=TEAM['blue']),
    podium3([team_podium_card(2, 'red', 'viper', '2 goals', '-1', [('P1', 'Vlad', False), ('P5', 'Jennifer', False),
                                                                    ('P6', 'Kuro', False)], 192, 128),
             team_podium_card(1, 'blue', 'murcielago', '3 goals', '', [('P2', 'Steve', False), ('P3', 'Player', True),
                                                                        ('P4', 'Tanya', False)], 210, 172, me=True)]),
    end_awards([('trophy', 'Man of the match', 'Steve · 2 goals, 1 save', '2', PURPLE),
                ('chevron', 'Hardest shot', 'Player · 112 km/h', '112', FAST),
                ('flag', 'Most saves', 'Jennifer · 4 saves', '4', ACC)]),
    col('right: 56px; top: 92px', combat_list('Final classification', 'timelimit 8', ['POS', 'DRIVER', 'SCORE', 'G / A'], [
        ('01', 'red', 'Vlad', '640', '2 / 0', '#999999', False), ('02', 'blue', 'Steve', '610', '2 / 0', '#999999', False),
        ('03', 'blue', 'Player', '480', '1 / 1', '#999999', True), ('04', 'blue', 'Tanya', '260', '0 / 1', '#999999', False),
        ('05', 'red', 'Jennifer', '240', '0 / 0', '#999999', False), ('06', 'red', 'Kuro', '120', '0 / 1', '#999999', False)],
        teams=[('blue', '3'), ('red', '2')]), 420),
    footer('WAITING FOR 3 DRIVERS', 3, 6),
])

# ---------------------------------------------------------------------------
# the end screen of a drag race: the duel. A time slip a lane, the deciding
# heat line by line, the better figure of each line in blue; the heats on
# the right.
# ---------------------------------------------------------------------------

def time_slip(lane, name, car, rows, win, me, w=294, h=176):
    """rows: (label, value, better). The winner's slip has the accent on top."""
    border = f'border-top: 2px solid {ACC if win else "#e6e6e6" if me else LINE};'
    tag = lab('WIN', ACC, 11) if win else lab('', '#999999', 11)
    you = lab('YOU', ACC, 11) if me else ''
    lines = ''.join(
        f'<div style="display: flex; justify-content: space-between; align-items: baseline; height: 22px; '
        f'border-bottom: 1px dashed #2c2c2c">{lab(k, "#999999", 11)}{mono(v, 14, FAST if better else "#dddddd")}</div>'
        for k, v, better in rows)
    return (f'<div style="width: {w}px; background: {PANEL}; border: 1px solid {LINE}; {border} '
            f'box-sizing: border-box; display: flex; flex-direction: column">'
            f'{car_view(car, w - 2, h)}'
            f'<div style="padding: 12px 14px 14px; display: flex; flex-direction: column; gap: 8px">'
            f'<div style="display: flex; justify-content: space-between; align-items: baseline">{lab(lane, "#999999", 11)}'
            f'<div style="display: flex; gap: 10px">{you}{tag}</div></div>'
            f'{text(name, 20, "#ffffff", 700)}'
            f'<div style="display: flex; flex-direction: column">{lines}</div>'
            f'</div></div>')


def heats_list(title, right, rows):
    grid = 'display: grid; grid-template-columns: 34px 1fr 82px 70px; gap: 10px'
    head = (f'<div style="{grid}; padding: 0 8px; margin: 0 -8px">{lab("HEAT", "#999999", 11)}{lab("WINNER", "#999999", 11)}'
            f'<div style="text-align: right">{lab("ET", "#999999", 11)}</div>'
            f'<div style="text-align: right">{lab("MARGIN", "#999999", 11)}</div></div>')
    out = []
    for heat, name, et, margin, me, foul in rows:
        bg = 'background: rgba(255,85,0,0.18);' if me else ''
        out.append(f'<div style="{grid}; align-items: center; height: 34px; padding: 0 8px; {bg}">'
                   f'{mono(heat, 14, "#999999")}'
                   f'{text(name, 16, "#ffffff", 700 if me else 400, "overflow: hidden; text-overflow: ellipsis")}'
                   f'<div style="text-align: right">{mono(et, 14, "#dddddd")}</div>'
                   f'<div style="text-align: right">{mono(margin, 14, ACC if foul else "#999999")}</div></div>')
    return panel(row(lab(title), lab(right, '#999999')) + head
                 + f'<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">{"".join(out)}</div>', gap=12)


def heat_chart(heats):
    """Each heat's elapsed time against your quickest: the bar is what it lost; a foul in the accent."""
    times = [t for t in heats if t]
    best, worst = min(times), max(times) - min(times)
    bars = []
    for i, v in enumerate(heats):
        if v is None:
            bar_ = f'<div style="width: 100%; height: 3px; background: {ACC}"></div>'
            val = mono('foul', 11, ACC)
        else:
            bar_ = (f'<div style="width: 100%; height: {max(3, int(56 * (v - best) / worst))}px; '
                    f'background: {"#e6e6e6" if v == best else "#5a5a5a"}"></div>')
            val = mono('best' if v == best else f'+{v - best:.3f}', 11, '#ffffff' if v == best else '#bbbbbb')
        bars.append(f'<div style="flex: 1; display: flex; flex-direction: column; align-items: center; gap: 6px">'
                    f'<div style="height: 56px; width: 100%; display: flex; align-items: flex-end">{bar_}</div>'
                    f'{val}{lab(f"H{i + 1}", "#777777", 10)}</div>')
    return f'<div style="display: flex; gap: 8px">{"".join(bars)}</div>'


slip_you = [('Reaction', '0.412', False), ('60 ft', '1.982', True), ('330 ft', '5.104', True),
            ('1/8 mile', '7.631', True), ('1/4 mile', '11.482', True), ('Trap', '187.4 km/h', True)]
slip_rival = [('Reaction', '0.388', True), ('60 ft', '2.034', False), ('330 ft', '5.187', False),
              ('1/8 mile', '7.702', False), ('1/4 mile', '11.589', False), ('Trap', '184.9 km/h', False)]

board('EndDrag.dc.html', 'Drag result', None, [
    blurred(BG['drag']),
    f'<div style="position: absolute; left: 56px; top: 48px; display: flex; gap: 16px; align-items: baseline">'
    f'{lab("Drag result", ACC, 13)}{lab("Airfield · quarter mile · best of 3", "#999999", 12)}</div>',
    # the match, then your runs
    f'<div style="position: absolute; left: 56px; top: 92px; width: 380px; display: flex; flex-direction: column; gap: 22px">'
    f'<div style="display: flex; align-items: baseline; gap: 14px">{big("2-1", 168)}{big("/ 3", 48, "#777777")}</div>'
    f'<div style="display: flex; flex-direction: column; gap: 6px; margin-top: -18px">{lab("Match won · your quickest run")}'
    f'{big("11.482", 44)}{lab("0.107 quicker than Vlad", FAST, 12)}</div>'
    + sep() +
    f'<div style="display: grid; grid-template-columns: 1fr 1fr; gap: 18px 24px">'
    f'{stat("Best reaction", "0.396")}{stat("Top trap", "187.4", "#ffffff", "km/h")}'
    f'{stat("Best total", "11.878")}{stat("Fouls", "0")}</div>'
    + sep() +
    f'<div style="display: flex; flex-direction: column; gap: 12px">{row(lab("Your heats"), lab("lost to your quickest", "#777777", 11))}'
    + heat_chart([11.613, None, 11.482]) + '</div></div>',
    # the deciding heat: a time slip a lane
    f'<div style="position: absolute; left: 476px; top: 92px; width: 600px; display: flex; flex-direction: column; gap: 10px">'
    f'{row(lab("Heat 3 · deciding"), lab("better figure in blue", "#777777", 11))}'
    f'<div style="display: flex; gap: 12px">'
    + time_slip('Left lane', 'Player', 'viper', slip_you, True, True)
    + time_slip('Right lane', 'Vlad', 'murcielago', slip_rival, False, False) + '</div></div>',
    f'<div style="position: absolute; left: 476px; top: 540px; width: 600px; display: flex; flex-direction: column; gap: 10px">'
    f'{lab("Highlights")}'
    + award('flag', 'Best reaction', 'Vlad · heat 1', '0.371', FAST)
    + award('trophy', 'Quickest run', 'Player · heat 3', '11.482', PURPLE) + '</div>',
    # the heats
    col('right: 56px; top: 92px', heats_list('Heats', 'best of 3', [
        ('H1', 'Player', '11.613', '+0.054', True, False),
        ('H2', 'Vlad', '11.667', 'FOUL', False, True),
        ('H3', 'Player', '11.482', '+0.107', True, False)]), 420),
    footer('WAITING FOR 1 DRIVER', 1, 2),
])

# ---------------------------------------------------------------------------
# the drag tournament: the bracket between the matches, the podium after the
# final. A match a card, two lines: who, the heats won; the winner white, the
# one out grey, you lit, bots in grey type; the match up next outlined.
# ---------------------------------------------------------------------------
TOURNEY_KEYS = (('FIRE', 'CONTINUE'), ('TAB', 'HEAT RESULTS'), ('ESC', 'MENU'))


def t_entrant(name, wins, state, row_h, bot=False, me=False):
    """state: 'won', 'out' or '' (not played yet)."""
    color = '#777777' if state == 'out' else '#bbbbbb' if bot else '#ffffff'
    bg = 'background: rgba(255,85,0,0.18);' if me else ''
    w = mono(str(wins), 14, '#777777' if state == 'out' else '#ffffff') if wins != '' else ''
    return (f'<div style="height: {row_h}px; display: flex; align-items: center; gap: 8px; padding: 0 10px; {bg}">'
            f'{text(name or "–", 15 if row_h >= 26 else 13, color if name else "#555555", 700 if me else 400, "flex: 1; overflow: hidden; white-space: nowrap; text-overflow: ellipsis")}'
            f'{lab("BOT", "#666666", 9) if bot and name else ""}{w}</div>')


def t_match(a, b, w, row_h, nxt=False):
    border = f'1px solid {ACC}' if nxt else f'1px solid {LINE}'
    return (f'<div style="width: {w}px; background: {PANEL}; border: {border}; box-sizing: border-box">'
            f'{t_entrant(*a)}<div style="height: 1px; background: {LINE}"></div>{t_entrant(*b)}</div>')


def bracket(rounds, heads, x0, w_total, top, bottom, card_w, row_h, third=None, champion=None):
    """rounds: a list of rounds, each a list of matches (a, b, next); laid out from the left, every
    match centred on the two it comes from. Connectors in the line colour."""
    n_cols = len(rounds) + 1
    gap = (w_total - n_cols * card_w) / (n_cols - 1)
    card_h = 2 * row_h + 1
    out, centres = [], []
    first = len(rounds[0])
    pitch = (bottom - top) / first
    for r, matches in enumerate(rounds):
        x = x0 + r * (card_w + gap)
        cs = []
        for j, (a, b, nxt) in enumerate(matches):
            cy = top + (j + 0.5) * pitch if r == 0 else (centres[r - 1][2 * j] + centres[r - 1][2 * j + 1]) / 2
            cs.append(cy)
            if nxt:
                out.append(f'<div style="position: absolute; left: {x}px; top: {cy - card_h / 2 - 18}px">{lab("Up next", ACC, 10)}</div>')
            out.append(f'<div style="position: absolute; left: {x}px; top: {cy - card_h / 2}px">{t_match(a, b, card_w, row_h, nxt)}</div>')
            if r > 0:  # the lines in from the two before
                px = x - gap
                for k in (2 * j, 2 * j + 1):
                    py = centres[r - 1][k]
                    out.append(f'<div style="position: absolute; left: {px}px; top: {py}px; width: {gap / 2}px; height: 1px; background: #3a3a3a"></div>')
                lo, hi = sorted((centres[r - 1][2 * j], centres[r - 1][2 * j + 1]))
                out.append(f'<div style="position: absolute; left: {px + gap / 2}px; top: {lo}px; width: 1px; height: {hi - lo}px; background: #3a3a3a"></div>'
                           f'<div style="position: absolute; left: {px + gap / 2}px; top: {cy}px; width: {gap / 2}px; height: 1px; background: #3a3a3a"></div>')
        centres.append(cs)
    # the heads over the columns
    for r, h in enumerate(heads):
        out.append(f'<div style="position: absolute; left: {x0 + r * (card_w + gap)}px; top: {top - 40}px">{lab(h, "#999999", 11)}</div>')
    # the winner: the final's line out to a card of its own
    xw = x0 + len(rounds) * (card_w + gap)
    cy = centres[-1][0]
    out.append(f'<div style="position: absolute; left: {xw - gap}px; top: {cy}px; width: {gap}px; height: 1px; background: #3a3a3a"></div>')
    name = champion or ''
    out.append(f'<div style="position: absolute; left: {xw}px; top: {cy - 28}px; width: {card_w}px; height: 56px; background: {PANEL}; '
               f'border: 1px solid {LINE}; border-top: 2px solid {ACC if name else LINE}; box-sizing: border-box; display: flex; align-items: center; gap: 10px; padding: 0 12px">'
               f'{icon("trophy", 20, ACC if name else "#555555")}{text(name or "?", 16, "#ffffff" if name else "#555555")}</div>')
    # third place under it
    if third:
        a, b, nxt = third
        ty = cy + 90
        out.append(f'<div style="position: absolute; left: {xw}px; top: {ty - 18}px">{lab("Third place", ACC if nxt else "#999999", 10)}</div>')
        out.append(f'<div style="position: absolute; left: {xw}px; top: {ty}px">{t_match(a, b, card_w, row_h, nxt)}</div>')
    return ''.join(out)


def path_line(round_, what, state):
    color = {'won': '#ffffff', 'next': ACC, 'out': '#777777', '': '#555555'}[state]
    tag = {'won': lab('Won', FAST, 11), 'next': lab('Up next', ACC, 11), 'out': lab('Out', ACC, 11), '': lab('', '#555555', 11)}[state]
    return (f'<div style="display: flex; align-items: baseline; gap: 12px; height: 26px">'
            f'<div style="width: 112px">{lab(round_, "#999999", 11)}</div>{text(what, 15, color)}'
            f'<div style="margin-left: auto">{tag}</div></div>')


def tourney_left(up_label, big_text, a, b, path, note=None):
    vs = (f'<div style="display: flex; flex-direction: column; gap: 6px">'
          f'<div style="display: flex; align-items: baseline; gap: 10px">{text(a[0], 22, ACC if a[1] else "#ffffff", 700)}'
          f'{lab("YOU", ACC, 11) if a[1] else lab("BOT", "#777777", 11) if a[2] else ""}</div>{lab("against", "#777777", 11)}'
          f'<div style="display: flex; align-items: baseline; gap: 10px">{text(b[0], 22, ACC if b[1] else "#ffffff", 700)}'
          f'{lab("BOT", "#777777", 11) if b[2] else ""}</div></div>')
    hint = (f'<div style="display: flex; flex-direction: column; gap: 6px">{lab("A bot match", "#bbbbbb", 11)}'
            f'{text(note, 14, "#999999")}</div>') if note else ''
    return (f'<div style="position: absolute; left: 56px; top: 92px; width: 380px; display: flex; flex-direction: column; gap: 22px">'
            f'<div style="display: flex; flex-direction: column; gap: 8px">{lab(up_label, ACC, 12)}{big(big_text, 64)}</div>'
            + vs + sep() +
            f'<div style="display: flex; flex-direction: column; gap: 4px">{lab("Your way")}'
            + ''.join(path_line(*x) for x in path) + '</div>' + (sep() + hint if hint else '') + '</div>')


# eight drivers, the quarter-finals done, your semi-final up next
qf = [(('Player', 2, 'won', 18, False, True), ('Tony', 1, 'out', 18, True), False),
      (('Vlad', 2, 'won', 18, True), ('Ivan', 0, 'out', 18, False), False),
      (('Sam', 2, 'won', 18, False), ('Bee', 0, 'out', 18, True), False),
      (('Carla', 1, 'out', 18, True), ('Janine', 2, 'won', 18, False), False)]
qf = [(e1[:3] + e1[4:], e2[:3] + e2[4:], n) for e1, e2, n in qf]
sf = [(('Player', '', '', False, True), ('Vlad', '', '', True), True), (('Sam', '', ''), ('Janine', '', ''), False)]
fin = [(('', '', ''), ('', '', ''), False)]


def rows_h(matches, h):
    return [((a[0], a[1], a[2], h) + tuple(a[3:]), (b[0], b[1], b[2], h) + tuple(b[3:]), n) for a, b, n in matches]


board('Tourney.dc.html', 'Drag tournament', None, [
    blurred(BG['drag']),
    f'<div style="position: absolute; left: 56px; top: 48px; display: flex; gap: 16px; align-items: baseline">'
    f'{lab("Drag tournament", ACC, 13)}{lab("8 drivers · best of 3 · quarter-finals done", "#999999", 12)}</div>',
    tourney_left('Up next', 'Semi-final', ('Player', True, False), ('Vlad', False, True),
                 [('Quarter-final', 'beat Tony 2-1', 'won'), ('Semi-final', 'against Vlad', 'next'), ('Final', '', '')]),
    bracket([rows_h(qf, 30), rows_h(sf, 30), rows_h(fin, 30)], ['Quarter-finals', 'Semi-finals', 'Final', 'Winner'],
            476, 1068, 172, 760, 210, 30, third=(('', '', '', 30), ('', '', '', 30), False)),
    footer('WAITING FOR 1 DRIVER', 0, 1, TOURNEY_KEYS),
])

# sixteen drivers: a round more, lower lines; the next match is between bots
names16 = ['Player', 'Tony', 'Vlad', 'Ivan', 'Sam', 'Bee', 'Carla', 'Janine', 'Minori', 'Josh', 'Tanya', 'Emma',
           'Bobby', 'Samuel', 'Paul', 'Kai']
bots16 = {'Tony', 'Vlad', 'Bee', 'Carla', 'Minori', 'Josh', 'Tanya', 'Emma', 'Bobby', 'Samuel', 'Paul', 'Kai'}
r16 = []
for j in range(8):
    a, b = names16[2 * j], names16[2 * j + 1]
    played = j < 5
    wa = (j % 3 != 1)
    r16.append(((a, (2 if wa else 1) if played else '', ('won' if wa else 'out') if played else '', 22, a in bots16, a == 'Player'),
                (b, (1 if wa else 2) if played else '', ('out' if wa else 'won') if played else '', 22, b in bots16, b == 'Player'),
                j == 5))
q16 = [(('Player', '', '', 22, False, True), ('Ivan', '', '', 22, False), False),
       (('Sam', '', '', 22, False), ('Janine', '', '', 22, False), False),
       (('Josh', '', '', 22, True), ('', '', '', 22), False), (('', '', '', 22), ('', '', '', 22), False)]
s16 = [(('', '', '', 22), ('', '', '', 22), False)] * 2
board('Tourney16.dc.html', 'Drag tournament, 16 drivers', None, [
    blurred(BG['drag']),
    f'<div style="position: absolute; left: 56px; top: 48px; display: flex; gap: 16px; align-items: baseline">'
    f'{lab("Drag tournament", ACC, 13)}{lab("16 drivers · best of 3 · round of 16", "#999999", 12)}</div>',
    tourney_left('Up next', 'Round of 16', ('Tanya', False, True), ('Emma', False, True),
                 [('Round of 16', 'beat Tony 2-1', 'won'), ('Quarter-final', 'against Ivan', ''), ('Semi-final', '', ''), ('Final', '', '')],
                 note='/dragskip skips it, /dragskip all skips to your next match.'),
    bracket([r16, q16, s16, [(('', '', '', 22), ('', '', '', 22), False)]],
            ['Round of 16', 'Quarter-finals', 'Semi-finals', 'Final', 'Winner'],
            476, 1068, 172, 760, 176, 22, third=(('', '', '', 22), ('', '', '', 22), False)),
    footer('WAITING FOR 1 DRIVER', 0, 1, TOURNEY_KEYS),
])


# after the final: the podium, where you ended, every match
def t_results(rows):
    grid = 'display: grid; grid-template-columns: 34px 1fr 44px 1fr; gap: 10px'
    head = (f'<div style="{grid}; padding: 0 8px; margin: 0 -8px">{lab("RND", "#999999", 11)}{lab("WINNER", "#999999", 11)}'
            f'<div style="text-align: center">{lab("HEATS", "#999999", 11)}</div>{lab("BEATEN", "#999999", 11)}</div>')
    out = []
    for rnd, w, score, l, me in rows:
        bg = 'background: rgba(255,85,0,0.18);' if me else ''
        out.append(f'<div style="{grid}; align-items: center; height: 34px; padding: 0 8px; {bg}">'
                   f'{mono(rnd, 13, "#999999")}{text(w, 15, "#ffffff", 400, "overflow: hidden; white-space: nowrap; text-overflow: ellipsis")}'
                   f'<div style="text-align: center">{mono(score, 14, "#dddddd")}</div>'
                   f'{text(l, 15, "#777777", 400, "overflow: hidden; white-space: nowrap; text-overflow: ellipsis")}</div>')
    return panel(row(lab('Every match'), lab('8 drivers', '#999999')) + head
                 + f'<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">{"".join(out)}</div>', gap=12)


board('TourneyEnd.dc.html', 'Drag tournament over', None, [
    blurred(BG['drag']),
    f'<div style="position: absolute; left: 56px; top: 48px; display: flex; gap: 16px; align-items: baseline">'
    f'{lab("Drag tournament", ACC, 13)}{lab("8 drivers · best of 3 · Sam wins", "#999999", 12)}</div>',
    f'<div style="position: absolute; left: 56px; top: 92px; width: 380px; display: flex; flex-direction: column; gap: 22px">'
    f'<div style="display: flex; align-items: baseline; gap: 14px">{big("P2", 168)}{big("/ 8", 48, "#777777")}</div>'
    f'<div style="display: flex; flex-direction: column; gap: 6px; margin-top: -18px">{lab("You finish second")}'
    f'{big("1-2", 44)}{lab("lost the final to Sam", ACC, 12)}</div>'
    + sep() +
    f'<div style="display: grid; grid-template-columns: 1fr 1fr; gap: 18px 24px">'
    f'{stat("Matches won", "2")}{stat("Matches lost", "1")}{stat("Heats won", "5")}{stat("Heats lost", "3")}</div>'
    + sep() +
    f'<div style="display: flex; flex-direction: column; gap: 4px">{lab("Your way")}'
    + path_line('Quarter-final', 'beat Tony 2-1', 'won') + path_line('Semi-final', 'beat Vlad 2-0', 'won')
    + path_line('Final', 'lost to Sam 1-2', 'out') + '</div></div>',
    f'<div style="position: absolute; left: 476px; top: 92px; width: 600px; height: 410px; display: flex; gap: 12px; align-items: flex-end">'
    + podium_card(2, 'Player', 'viper', '2-1 · 2-0', '', 192, 176, me=True).replace('SECOND', 'FINALIST')
    + podium_card(1, 'Sam', 'murcielago', '2-0 · 2-1 · 2-1', '', 210, 236).replace('WINNER', 'CHAMPION')
    + podium_card(3, 'Vlad', 'db12', 'third place 2-1', '', 192, 140) + '</div>',
    f'<div style="position: absolute; left: 476px; top: 530px; width: 600px; display: flex; flex-direction: column; gap: 10px">'
    f'{lab("Highlights")}'
    + award('flag', 'Clean sweep', 'Sam · no heat lost', '6-0', FAST)
    + award('trophy', 'Most heats won', 'Sam · three matches', '6', PURPLE) + '</div>',
    col('right: 56px; top: 92px', t_results([
        ('QF', 'Player', '2-1', 'Tony', True), ('QF', 'Vlad', '2-0', 'Ivan', False), ('QF', 'Sam', '2-0', 'Bee', False),
        ('QF', 'Janine', '2-1', 'Carla', False), ('SF', 'Player', '2-0', 'Vlad', True), ('SF', 'Sam', '2-1', 'Janine', False),
        ('3RD', 'Vlad', '2-1', 'Janine', False), ('F', 'Sam', '2-1', 'Player', False)]), 420),
    footer('WAITING FOR 1 DRIVER', 0, 1, (('FIRE', 'NEXT TOURNAMENT'), ('TAB', 'HEAT RESULTS'), ('ESC', 'MENU'))),
])

# ---------------------------------------------------------------------------
# the TAB table: held during the game (and when you are wrecked), the game
# dimmed behind it, the HUD out of the way. One table for every mode: the
# columns are the mode's, your row lit, bots without a ping.
# ---------------------------------------------------------------------------
TAB_X, TAB_W = 360, 880


def tab_header(mode, sub, right_label, right_value):
    return (f'<div style="position: absolute; left: {TAB_X}px; top: 64px; width: {TAB_W}px; display: flex; '
            f'align-items: flex-end; gap: 16px">'
            f'<div style="display: flex; flex-direction: column; gap: 8px">{lab(mode, ACC, 13)}{lab(sub, "#999999", 12)}</div>'
            f'<div style="margin-left: auto; display: flex; flex-direction: column; align-items: flex-end; gap: 6px">'
            f'{lab(right_label, "#999999", 11)}{big(right_value, 34)}</div></div>')


def tab_footer(spectators, server):
    return (f'<div style="display: flex; align-items: baseline; gap: 12px; padding: 0 2px">{lab("Spectators", "#777777", 11)}{text(spectators, 14, "#bbbbbb")}'
            f'<div style="margin-left: auto">{lab(server, "#777777", 11)}</div></div>')


def tab_name(name, car, me, out=False):
    return (f'<div style="display: flex; align-items: baseline; gap: 10px; min-width: 0">'
            f'{text(name, 16, "#777777" if out else "#ffffff", 700 if me else 400)}{lab(car, "#666666", 10)}</div>')


def tab_table(cols, rows, top=None):
    """cols: (header, width or '1fr', align); rows: (cells, me, out). A cell is html."""
    grid = 'display: grid; grid-template-columns: ' + ' '.join(str(w) + ('px' if isinstance(w, int) else '') for _, w, _ in cols) + '; gap: 10px'
    head = (f'<div style="{grid}; padding: 0 8px; margin: 0 -8px">'
            + ''.join(f'<div style="text-align: {a}">{lab(h, "#999999", 11)}</div>' for h, _, a in cols) + '</div>')
    out = []
    for cells, me, gone in rows:
        bg = 'background: rgba(255,85,0,0.18);' if me else ''
        out.append(f'<div style="{grid}; align-items: center; height: 34px; padding: 0 8px; {bg}">'
                   + ''.join(f'<div style="text-align: {a}; min-width: 0">{c}</div>' for c, (_, _, a) in zip(cells, cols)) + '</div>')
    return panel((top or '') + head + f'<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">{"".join(out)}</div>',
                 pad='16px 18px 14px', gap=12)


def ping(v):
    if v == 'BOT':
        return lab('BOT', '#777777', 11)
    return mono(str(v), 13, '#999999' if v < 100 else ACC)


def race_rows(data, teams=False):
    rows = []
    for d in data:
        pos, team, name, car, lap, gap, last, best, pg, flag = d
        me, out, fastest, done = flag == 'me', flag == 'out', flag == 'fast', flag == 'done'
        gapc = ACC if gap == 'LEADER' else '#777777' if out else '#dddddd'
        gapcell = (f'<div style="display: flex; gap: 6px; justify-content: flex-end; align-items: center">'
                   f'{icon("flag", 14, "#ffffff") if done else ""}{mono(gap, 14, gapc)}</div>')
        cells = [mono(pos, 14, ACC if pos == '01' else '#999999')]
        if teams:
            cells.append(swatch(TEAM[team], 10))
        cells += [tab_name(name, car, me, out), mono(lap, 14, '#777777' if out else '#dddddd'), gapcell,
                  mono(last, 14, '#999999'), mono(best, 14, PURPLE if fastest else '#999999'), ping(pg)]
        rows.append((cells, me, out))
    return rows


RACE_COLS = [('Pos', 34, 'left'), ('Driver', '1fr', 'left'), ('Lap', 44, 'right'), ('Gap', 96, 'right'),
             ('Last', 84, 'right'), ('Best', 84, 'right'), ('Ping', 44, 'right')]
tab_race = [('01', 'red', 'Sam', 'Murcielago', '3', 'LEADER', '0:51.204', '0:50.871', 38, 'fast'),
            ('02', 'blue', 'Bobby', 'Viper', '3', '+0.85', '0:51.880', '0:51.312', 'BOT', ''),
            ('03', 'blue', 'Player', 'Murcielago', '3', '+1.92', '0:52.013', '0:51.660', 24, 'me'),
            ('04', 'red', 'Janine', 'DB12', '3', '+2.40', '0:52.390', '0:52.120', 61, ''),
            ('05', 'red', 'Vlad', 'Beetle', '3', '+4.07', '0:53.004', '0:52.731', 'BOT', ''),
            ('06', 'blue', 'Emma', 'Viper', '3', '+6.31', '0:52.881', '0:52.881', 88, ''),
            ('07', 'blue', 'Tony', 'DB12', '2', '+1 LAP', '0:58.120', '0:53.512', 132, ''),
            ('08', 'red', 'Bee', 'Beetle', '2', 'OUT', '--', '0:54.120', 45, 'out')]

board('TabRacing.dc.html', 'TAB: race', BG['racing'], [
    tab_header('Race', 'Downtown · lap 3 / 5 · 8 drivers', 'Race time', '2:41.38'),
    col(f'left: {TAB_X}px; top: 150px', tab_table(RACE_COLS, race_rows(tab_race)) + tab_footer('Ivan · Minori', 'Q3Rally Münster · 10 / 16'), TAB_W, gap=14),
], dim=0.62)

# a team race: the teams over the drivers, each with the team's square
team_cells = ''.join(
    f'<div style="flex: 1; background: {TEAM[t]}; padding: 6px 10px; display: flex; justify-content: space-between; align-items: center">'
    f'<div style="display: flex; flex-direction: column; gap: 2px">{lab(t.upper(), "#111111", 10)}{mono(v, 13, "#111111")}</div>'
    f'{big(p, 26, "#111111")}</div>' for t, v, p in (('red', 'best P1 · 4 drivers', 'P1'), ('blue', 'best P2 · 4 drivers', 'P2')))
board('TabTeamRacing.dc.html', 'TAB: team race', BG['racing'], [
    tab_header('Team race', 'Downtown · lap 3 / 5 · 2 teams · 8 drivers', 'Race time', '2:41.38'),
    col(f'left: {TAB_X}px; top: 150px', tab_table(RACE_COLS[:1] + [('', 10, 'left')] + RACE_COLS[1:], race_rows(tab_race, teams=True),
                                                  top=f'<div style="display: flex; gap: 2px">{team_cells}</div>') + tab_footer('Ivan · Minori', 'Q3Rally Münster · 10 / 16'), TAB_W, gap=14),
], dim=0.62)

# combat: the score big, the damage dealt and taken
DM_COLS = [('Pos', 34, 'left'), ('Driver', '1fr', 'left'), ('Score', 64, 'right'), ('Dealt', 84, 'right'),
           ('Taken', 84, 'right'), ('Ping', 44, 'right')]
tab_dm = [('01', 'Sam', 'Murcielago', '17', '4 210', '2 380', 38, ''), ('02', 'Player', 'Viper', '14', '3 905', '2 610', 24, 'me'),
          ('03', 'Bobby', 'Viper', '11', '3 120', '3 004', 'BOT', ''), ('04', 'Janine', 'DB12', '9', '2 870', '3 330', 61, ''),
          ('05', 'Vlad', 'Beetle', '6', '1 940', '3 760', 'BOT', ''), ('06', 'Emma', 'Viper', '4', '1 505', '4 120', 88, ''),
          ('07', 'Tony', 'DB12', '-1', '620', '2 950', 132, 'out')]
dm_rows = [([mono(p, 14, ACC if p == '01' else '#999999'), tab_name(n, c, f == 'me', f == 'out'),
             big(s, 24, ACC if p == '01' else '#777777' if f == 'out' else '#ffffff'),
             mono(d, 14, '#dddddd'), mono(t, 14, '#999999'), ping(pg)], f == 'me', f == 'out')
           for p, n, c, s, d, t, pg, f in tab_dm]
board('TabDeathmatch.dc.html', 'TAB: deathmatch', BG['dm'], [
    tab_header('Deathmatch', 'Arena · fraglimit 20 · 7 drivers', 'Time left', '6:12'),
    col(f'left: {TAB_X}px; top: 150px', tab_table(DM_COLS, dm_rows) + tab_footer('Ivan', 'Q3Rally Münster · 8 / 16'), TAB_W, gap=14),
], dim=0.62)


# teams: a panel a team side by side (four teams: two by two), the score in its head
def team_side(team, score, note, rows, w):
    cols = [('Driver', '1fr', 'left'), ('Score', 44, 'right'), ('Caps', 36, 'right'), ('Ast', 36, 'right'),
            ('Def', 36, 'right'), ('Ping', 40, 'right')]
    grid = 'display: grid; grid-template-columns: 1fr 44px 36px 36px 36px 40px; gap: 8px'
    head = (f'<div style="{grid}; padding: 0 8px; margin: 0 -8px">'
            + ''.join(f'<div style="text-align: {a}">{lab(h, "#999999", 11)}</div>' for h, _, a in cols) + '</div>')
    out = []
    for n, c, s, cp, a, d, pg, f in rows:
        bg = 'background: rgba(255,85,0,0.18);' if f == 'me' else ''
        out.append(f'<div style="{grid}; align-items: center; height: 34px; padding: 0 8px; {bg}">'
                   f'{tab_name(n, c, f == "me")}<div style="text-align: right">{big(s, 22)}</div>'
                   + ''.join(f'<div style="text-align: right">{mono(v, 14, "#dddddd" if v != "0" else "#666666")}</div>' for v in (cp, a, d))
                   + f'<div style="text-align: right">{ping(pg)}</div></div>')
    top = (f'<div style="display: flex; justify-content: space-between; align-items: flex-end">'
           f'<div style="display: flex; flex-direction: column; gap: 6px">{lab(team + " team", TEAM[team], 13)}{lab(note, "#999999", 11)}</div>'
           f'{big(score, 56)}</div>')
    return (f'<div style="width: {w}px; background: {PANEL}; border: 1px solid {LINE}; border-top: 3px solid {TEAM[team]}; '
            f'box-sizing: border-box; padding: 16px 18px 14px; display: flex; flex-direction: column; gap: 12px">'
            f'{top}{head}<div style="display: flex; flex-direction: column; gap: 2px; margin: 0 -8px">{"".join(out)}</div></div>')


red_side = [('Sam', 'Murcielago', '14', '3', '1', '4', 38, ''), ('Janine', 'DB12', '9', '1', '2', '6', 61, ''),
            ('Vlad', 'Beetle', '6', '1', '0', '2', 'BOT', ''), ('Tony', 'DB12', '3', '0', '1', '1', 132, '')]
blue_side = [('Player', 'Viper', '12', '2', '1', '3', 24, 'me'), ('Bobby', 'Viper', '8', '1', '0', '5', 'BOT', ''),
             ('Emma', 'Viper', '5', '0', '2', '1', 88, ''), ('Carla', 'Beetle', '2', '0', '0', '0', 70, '')]
board('TabCTF.dc.html', 'TAB: capture the flag', BG['team_dm'], [
    tab_header('Capture the flag', 'Canyon · capturelimit 8 · 8 drivers', 'Time left', '11:20'),
    f'<div style="position: absolute; left: {TAB_X}px; top: 150px; width: {TAB_W}px; display: flex; flex-direction: column; gap: 14px">'
    f'<div style="display: flex; gap: 16px; align-items: flex-start">'
    + team_side('red', '5', 'leads by 2', red_side, (TAB_W - 16) // 2)
    + team_side('blue', '3', '4 drivers', blue_side, (TAB_W - 16) // 2) + '</div>'
    + tab_footer('Ivan · Minori', 'Q3Rally Münster · 10 / 16') + '</div>',
], dim=0.62)

# HUD options (Shift+H) ------------------------------------------------------------------------------
# A panel in the middle, where the HUD keeps the screen free: every change shows in the corners at
# once. Only the toggles that do something in the mode, in groups; a small screen beside a group's
# name shows where its part of the HUD is. The native panel: cg_hud_panel.c, MH_PanelLayout.
PANEL_W = 840
PANEL_REGION = {'card': (1, 1, 9, 6), 'order': (17, 1, 9, 5), 'car': (1, 10, 26, 6), 'map': (19, 9, 8, 7),
                'track': (8, 4, 12, 9), 'fx': (1, 1, 26, 15)}
PANEL_MONO = f"font-family: {MONO}; letter-spacing: 0.16em; text-transform: uppercase; white-space: nowrap;"


def p_icon(region):
    x, y, w, h = PANEL_REGION[region]
    return (f'<svg width="28" height="17" viewBox="0 0 28 17" aria-hidden="true" style="flex: none">'
            f'<rect x="0.5" y="0.5" width="27" height="16" fill="none" stroke="#555555"></rect>'
            f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="{ACC}"></rect></svg>')


def p_head(title, region):
    return (f'<div style="display: flex; align-items: center; gap: 10px; height: 24px">{p_icon(region)}'
            f'<div style="{PANEL_MONO} font-size: 12px; color: #bbbbbb">{title}</div>'
            f'<div style="flex: 1; height: 1px; background: {LINE}"></div></div>')


def p_seg(opts, sel, full=False):
    out = [f'<div style="display: {"flex" if full else "inline-flex"}; border: 1px solid #3b3b3b; flex: none">']
    for i, o in enumerate(opts):
        on = i == sel
        st = (f'{PANEL_MONO} font-size: 12px; letter-spacing: 0.12em; height: 30px; padding: 0 14px; display: flex; '
              f'align-items: center; justify-content: center; box-sizing: border-box; ' + ('flex: 1; ' if full else '')
              + (f'background: {ACC}; color: #0d0d0d;' if on else 'background: #1a1a1a; color: #999999;')
              + ('' if i == 0 else ' border-left: 1px solid #3b3b3b;'))
        out.append(f'<button type="button" aria-pressed="{"true" if on else "false"}" '
                   f'style="border: 0; margin: 0; cursor: pointer; {st}">{o}</button>')
    return ''.join(out) + '</div>'


def p_label(t, hint=None):
    s = (f'<div style="display: flex; flex-direction: column; gap: 2px; min-width: 0">'
         f'<div style="font-size: 16px; color: #ffffff; white-space: nowrap">{t}</div>')
    if hint:
        s += f'<div style="font-size: 13px; color: #999999; white-space: nowrap">{hint}</div>'
    return s + '</div>'


def p_row(t, on, hint=None):
    return (f'<div style="display: flex; align-items: center; justify-content: space-between; gap: 16px; min-height: 44px">'
            f'{p_label(t, hint)}{p_seg(["Off", "On"], 1 if on else 0)}</div>')


def p_multi(t, opts, sel, hint=None):
    return (f'<div style="display: flex; flex-direction: column; gap: 8px; padding: 6px 0 4px 0">'
            f'{p_label(t, hint)}{p_seg(opts, sel, True)}</div>')


def p_slider(t, val, frac):
    return (f'<div style="display: flex; flex-direction: column; gap: 8px; padding: 4px 0 6px 16px; border-left: 1px solid {LINE}; margin-left: 2px">'
            f'<div style="display: flex; justify-content: space-between; align-items: baseline">'
            f'<div style="font-size: 14px; color: #dddddd">{t}</div>'
            f'<div style="font-family: {MONO}; font-size: 14px; color: {ACC}">{val}</div></div>'
            f'<div style="position: relative; height: 14px">'
            f'<div style="position: absolute; left: 0; right: 0; top: 5px; height: 4px; background: #2c2c2c"></div>'
            f'<div style="position: absolute; left: 0; top: 5px; height: 4px; width: {frac}%; background: {ACC}"></div>'
            f'<div style="position: absolute; top: 0; left: calc({frac}% - 7px); width: 14px; height: 14px; background: #ffffff"></div>'
            f'</div></div>')


def p_group(title, region, rows):
    return f'<div style="display: flex; flex-direction: column; gap: 6px">{p_head(title, region)}{"".join(rows)}</div>'


def p_minimap(on=True):
    return [p_row('Mini-map', on)] + ([p_slider('Size', '1.00', 37.5), p_slider('Field of view', '90', 66.6667)] if on else [])


def hud_panel(name, title, bg, crumb, noun, back, left, right, top):
    cols = ''.join(f'<div style="display: flex; flex-direction: column; gap: 22px">{"".join(c)}</div>' for c in (left, right))
    panel = (f'<div role="dialog" aria-label="HUD options" style="position: absolute; left: {(1600 - PANEL_W) // 2}px; top: {top}px; '
             f'width: {PANEL_W}px; box-sizing: border-box; background: rgba(13,13,13,0.95); border: 1px solid {LINE}; '
             f'border-top: 2px solid {ACC}; padding: 26px 32px 28px 32px; display: flex; flex-direction: column; gap: 22px">'
             f'<div style="display: flex; justify-content: space-between; align-items: flex-start; gap: 24px">'
             f'<div style="display: flex; flex-direction: column; gap: 8px">'
             f'<div style="{PANEL_MONO} font-size: 12px; color: {ACC}">Live HUD · {crumb}</div>'
             f'<div style="font-family: {COND}; font-weight: 700; font-style: italic; font-size: 44px; line-height: 1; '
             f'text-transform: uppercase; white-space: nowrap">Your {noun}. Your <span style="color: {ACC}">display.</span></div>'
             f'<div style="font-size: 15px; color: #dddddd">Changes show right away. Only what this mode\'s HUD has is listed.</div>'
             f'</div><div style="{PANEL_MONO} font-size: 12px; color: #999999; padding-top: 2px">Shift + H · Esc</div></div>'
             f'<div style="display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); column-gap: 44px">{cols}</div>'
             f'<div style="display: flex; gap: 10px; align-items: center">'
             f'<button type="button" style="border: 0; margin: 0; cursor: pointer; background: {ACC}; color: #ffffff; '
             f'font-family: {BODY}; font-weight: 700; font-size: 16px; height: 44px; padding: 0 20px">Back to {back}</button>'
             f'<button type="button" style="margin: 0; cursor: pointer; background: transparent; color: #ffffff; border: 1px solid #555555; '
             f'font-family: {BODY}; font-weight: 700; font-size: 16px; height: 44px; padding: 0 20px">Console</button>'
             f'</div></div>')
    board(name, title, bg, [panel], dim=0.25)


hud_panel('PanelRace.dc.html', 'HUD options: race', BG.get('panel_race'), 'Race · Ridge Racer', 'race', 'race',
          [p_group('Card', 'card', [p_row('Times', True, 'Lap, best, gap'), p_row('Lap counter', True), p_row('Race position', True)]),
           p_group('On track', 'track', [p_multi('Checkpoint arrow', ['Off', 'On HUD', 'Above car'], 2, 'On HUD: at the top of the screen'),
                                         p_row('Racing line', False)]),
           p_group('Order', 'order', [p_row('Race order', True)])],
          [p_group('Car and cockpit', 'car', [p_row('HUD', True, 'Off hides every panel'), p_row('Rear view', False),
                                              p_row('Slipstream gauge', True)]),
           p_group('Mini-map', 'map', p_minimap())], 102)
hud_panel('PanelDeathmatch.dc.html', 'HUD options: deathmatch', BG.get('panel_dm'), 'Deathmatch · Battle Canyon', 'match', 'match',
          [p_group('Order', 'order', [p_row('Frag order', True)]),
           p_group('Car and cockpit', 'car', [p_row('HUD', True, 'Off hides every panel'), p_row('Rear view', False)])],
          [p_group('Mini-map', 'map', p_minimap())], 221.5)
hud_panel('PanelDerby.dc.html', 'HUD options: derby', BG.get('panel_derby'), 'Derby · Figure of 8', 'derby', 'match',
          [p_group('Order', 'order', [p_row('Derby order', True)]),
           p_group('Effects', 'fx', [p_row('Hit flash', True, 'Red flash when you are hit')]),
           p_group('Car and cockpit', 'car', [p_row('HUD', True, 'Off hides every panel'), p_row('Rear view', False)])],
          [p_group('Mini-map', 'map', p_minimap())], 173.5)


ROWS = [['Main.dc.html', 'Zonen.dc.html', 'Sprint.dc.html', 'Ghost.dc.html'],
        ['Drag.dc.html', 'RacingDM.dc.html', 'Elimination.dc.html', 'LCS.dc.html'],
        ['Derby.dc.html', 'Deathmatch.dc.html', 'TeamDM.dc.html', 'CTF.dc.html'],
        ['Domination.dc.html', 'KOTH.dc.html', 'Autoball.dc.html', 'Einblendungen.dc.html'],
        ['EndRacing.dc.html', 'EndTeamRacing.dc.html', 'EndSprint.dc.html', 'EndGhost.dc.html', 'EndDrag.dc.html'],
        ['EndDeathmatch.dc.html', 'EndDerby.dc.html', 'EndTeamDM.dc.html', 'EndRacingDM.dc.html', 'EndElimination.dc.html'],
        ['EndCTF.dc.html', 'EndDomination.dc.html', 'EndKOTH.dc.html', 'EndAutoball.dc.html'],
        ['Tourney.dc.html', 'Tourney16.dc.html', 'TourneyEnd.dc.html'],
        ['TabRacing.dc.html', 'TabTeamRacing.dc.html', 'TabDeathmatch.dc.html', 'TabCTF.dc.html']]
order = [n for r in ROWS for n in r]
titles = {'Main.dc.html': 'Rennen (Racing, Team Racing)', 'Zonen.dc.html': 'Zonen und Bausteine', 'Sprint.dc.html': 'Sprint',
          'Ghost.dc.html': 'Ghost Race', 'Drag.dc.html': 'Drag Race', 'RacingDM.dc.html': 'Racing DM (auch Team Racing DM)',
          'Elimination.dc.html': 'Elimination', 'LCS.dc.html': 'Last Car Standing', 'Derby.dc.html': 'Derby',
          'Deathmatch.dc.html': 'Deathmatch', 'TeamDM.dc.html': 'Team Deathmatch', 'CTF.dc.html': 'CTF und CTF4',
          'Domination.dc.html': 'Domination', 'KOTH.dc.html': 'King of the Hill', 'Autoball.dc.html': 'Autoball',
          'Einblendungen.dc.html': 'Einblendungen, Großmeldungen, Jukebox', 'EndRacing.dc.html': 'Endbildschirm Racing', 'EndTeamRacing.dc.html': 'Endbildschirm Team Racing', 'EndSprint.dc.html': 'Endbildschirm Sprint', 'EndGhost.dc.html': 'Endbildschirm Ghost Race', 'EndDrag.dc.html': 'Endbildschirm Drag Race', 'EndDeathmatch.dc.html': 'Endbildschirm Deathmatch', 'EndDerby.dc.html': 'Endbildschirm Derby (auch Last Car Standing)', 'EndTeamDM.dc.html': 'Endbildschirm Team Deathmatch', 'EndRacingDM.dc.html': 'Endbildschirm Racing DM (Team Racing DM: Team-Racing-Seite mit Frags)', 'EndElimination.dc.html': 'Endbildschirm Elimination', 'EndCTF.dc.html': 'Endbildschirm CTF (CTF4: vier Teams wie Domination)', 'EndDomination.dc.html': 'Endbildschirm Domination (vier Teams)', 'EndKOTH.dc.html': 'Endbildschirm King of the Hill', 'EndAutoball.dc.html': 'Endbildschirm Autoball', 'Tourney.dc.html': 'Drag-Turnier (8 Fahrer)', 'Tourney16.dc.html': 'Drag-Turnier (16 Fahrer)', 'TourneyEnd.dc.html': 'Drag-Turnier vorbei',
          'TabRacing.dc.html': 'TAB: Rennen (auch Sprint, Ghost, Drag)', 'TabTeamRacing.dc.html': 'TAB: Team Racing',
          'TabDeathmatch.dc.html': 'TAB: Kampf (DM, Racing DM, Derby, Elimination, LCS)',
          'TabCTF.dc.html': 'TAB: Teams (TDM, CTF, Domination, KOTH, Autoball)'}
rows_titles = ['Aufbau und Rennen', 'Drag, Waffen und Überleben', 'Derby und Arena', 'Ziele, Autoball und Einblendungen',
               'Endbildschirme', 'Endbildschirme Kampf', 'Endbildschirme Ziele', 'Drag-Turnier', 'TAB-Tabelle']
boards, notes = {}, {}
for r, names in enumerate(ROWS):
    for c, n in enumerate(names):
        boards[n] = {'x': c * 1680, 'y': r * 1300, 'w': 1600, 'h': 900, 'title': titles[n]}
for r, t in enumerate(rows_titles):
    notes[f'row{r + 1}'] = {'x': 0, 'y': r * 1300 - 260, 'text': t, 'kind': 'title1', 'maxW': 6640}
canvas = {'v': 3, 'createdOnFiles': {'v': 1, 'at': datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')},
          'title': 'Q3Rally HUD – Entwurf v0.9', 'launch': {'view': 'canvas'}, 'pages': [],
          'boards': boards, 'order': order, 'notes': notes, 'designSystems': []}
json.dump(canvas, open(os.path.join(OUT, 'canvas.json'), 'w'), ensure_ascii=False, indent=1)
print('ok', len(order))
