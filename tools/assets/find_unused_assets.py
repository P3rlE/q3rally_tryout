"""Which images, models and sounds in baseq3r does the game actually use?

  python3 tools/assets/find_unused_assets.py . unused.json


References come from: every BSP (shader and fog lumps, entity values), the
shader scripts, md3 surfaces and skins of used models, string literals in the
game code (printf-style patterns become wildcards), and the text configs
(arenas, cfg, menus, bot/arena lists). The closure over shaders, md3s and
skins gives the used set; the rest is unused. Textures only named in the
map sources (.map) or only as editor images are listed apart.
"""
import json, os, re, struct, sys, zipfile, collections
from pathlib import Path

ROOT = Path(sys.argv[1])            # repo checkout
B = ROOT / 'baseq3r'
IMG = ('.tga', '.jpg', '.jpeg', '.png', '.dds', '.pcx', '.bmp')
SND = ('.wav', '.ogg', '.mp3', '.opus', '.flac')


def norm(s):
    return s.strip().strip('"').replace('\\', '/').lstrip('/').lower()


def stem(p):
    b, e = os.path.splitext(p)
    return b if e in IMG + SND + ('.md3', '.mdr', '.iqm', '.skin', '.shader', '.roq', '.ogv') else p


# ---------------------------------------------------------------- file universe (incl. pk3 contents)
files = {}          # norm path -> bytes loader
for f in B.rglob('*'):
    if f.is_file():
        rel = f.relative_to(B).as_posix()
        if rel.lower().endswith('.pk3'):
            z = zipfile.ZipFile(f)
            for n in z.namelist():
                if not n.endswith('/'):
                    files[norm(n)] = (lambda z=z, n=n: z.read(n))
            continue
        files[norm(rel)] = (lambda f=f: f.read_bytes())
by_stem = collections.defaultdict(set)
for p in files:
    by_stem[stem(p)].add(p)

# ---------------------------------------------------------------- shader scripts
shaders = {}        # name -> (body, script file)
# of two shaders with the same name the renderer takes the one in the
# alphabetically first script (checked in the game: railCore comes from
# gfx.shader, not weapons.shader), and within a file the first one
for p in sorted(files):
    if p.startswith('scripts/') and p.endswith('.shader'):
        t = files[p]().decode('latin-1')
        t = re.sub(r'//[^\n]*', '', t)
        i, n = 0, len(t)
        while i < n:
            m = re.compile(r'\s*([^\s{}]+)\s*\{').match(t, i)
            if not m:
                j = t.find('{', i)
                if j < 0: break
                i = j + 1
                continue
            depth, j = 1, m.end()
            while j < n and depth:
                depth += {'{': 1, '}': -1}.get(t[j], 0)
                j += 1
            name = norm(m.group(1))
            shaders.setdefault(name, (t[m.end():j - 1], p))   # first definition wins, like the engine
            i = j

used, editor_only, via = set(), set(), {}
used_shaders = set()
todo = []


def ref(r, why):
    r = norm(r)
    if r and len(r) < 200:
        todo.append((r, why))


SKY = ('_bk', '_dn', '_ft', '_lf', '_rt', '_up')


def visit_shader(name, why):
    if name in used_shaders: return
    used_shaders.add(name)
    body, _ = shaders[name]
    for line in body.splitlines():
        w = line.split()
        if not w: continue
        k = w[0].lower()
        if k in ('map', 'clampmap') and len(w) > 1 and not w[1].startswith(('$', '*')):
            ref(w[1], why)
        elif k == 'animmap':
            for x in w[2:]: ref(x, why)
        elif k == 'videomap' and len(w) > 1:
            ref('video/' + w[1], why)
        elif k == 'skyparms':
            for box in w[1:4:2]:
                if box != '-':
                    for s in SKY: ref(box + s, why)
        elif k in ('q3map_baseshader', 'q3map_remapshader', 'q3map_cloneshader') and len(w) > 1:
            ref(w[1], why)
        elif k in ('qer_editorimage', 'q3map_lightimage'):
            if len(w) > 1:
                for c in by_stem.get(stem(norm(w[1])), ()):
                    editor_only.add(c)


def md3_shaders(data):
    out = []
    if data[:4] != b'IDP3': return out
    nsurf = struct.unpack_from('<i', data, 84)[0]
    ofs = struct.unpack_from('<i', data, 100)[0]
    for _ in range(nsurf):
        nsh = struct.unpack_from('<i', data, ofs + 76)[0]
        osh = struct.unpack_from('<i', data, ofs + 92)[0]
        for k in range(nsh):
            out.append(data[ofs + osh + 68 * k: ofs + osh + 68 * k + 64].split(b'\0')[0].decode('latin-1'))
        ofs += struct.unpack_from('<i', data, ofs + 104)[0]
    return out


def use_file(p, why):
    if p in used: return
    used.add(p); via[p] = why
    if p.endswith('.md3'):
        for s in md3_shaders(files[p]()): ref(s, why)
        b = p[:-4]
        for lod in (b + '_1.md3', b + '_2.md3'):
            if lod in files: use_file(lod, why)
    elif p.endswith('.skin'):
        for line in files[p]().decode('latin-1').splitlines():
            if ',' in line: ref(line.split(',', 1)[1], why)


def resolve(r, why):
    s = stem(r)
    hit = False
    if s in shaders:
        visit_shader(s, why); hit = True
    for p in by_stem.get(s, ()):
        use_file(p, why); hit = True
    if r in files:
        use_file(r, why); hit = True
    return hit


patterns = []        # (regex, why)


def add_pattern(lit, why):
    lit = norm(lit)
    if '%' not in lit:
        ref(lit, why); return
    b, e = os.path.splitext(lit)
    if e in IMG + SND + ('.md3', '.skin'):
        lit = b                       # names are matched without extension
    root = lit.split('%')[0]
    if root.count('/') < 1 and len(root) < 6: return       # too generic ("%s/%s")
    rx = re.escape(lit)
    rx = re.sub(r'%(?:0?\d+)?[sdicuxl]+', '[^/]*', rx.replace('\\%', '%'))
    patterns.append((re.compile('^' + rx + '$'), why))


# ---------------------------------------------------------------- sources
# 1. maps
for p in sorted(files):
    if p.startswith('maps/') and p.endswith('.bsp'):
        d = files[p]()
        mapname = p[5:-4]
        why = 'Karte ' + mapname
        so, sl = struct.unpack_from('<ii', d, 8 + 8 * 1)
        for i in range(sl // 72):
            ref(d[so + 72 * i: so + 72 * i + 64].split(b'\0')[0].decode('latin-1'), why)
        fo, fl = struct.unpack_from('<ii', d, 8 + 8 * 12)
        for i in range(fl // 72):
            ref(d[fo + 72 * i: fo + 72 * i + 64].split(b'\0')[0].decode('latin-1'), why)
        eo, el = struct.unpack_from('<ii', d, 8)
        for k, v in re.findall(r'"([^"]+)"\s+"([^"]*)"', d[eo:eo + el].decode('latin-1')):
            for part in re.split(r'[;,]', v):
                part = part.strip()
                if '/' in part or part.lower().endswith(IMG + SND + ('.md3',)):
                    ref(part, why)
        ref('levelshots/' + mapname, 'Levelshot ' + mapname)
# 2. code
suffixes = set()
CODE = ROOT / 'engine/code'
for f in CODE.rglob('*.[ch]'):
    if any(x in f.parts for x in ('botlib', 'libs', 'zlib', 'jpeg-8c', 'libogg', 'libvorbis', 'opus', 'curl', 'SDL2', 'AL', 'libtheora')):
        continue
    t = f.read_text(errors='ignore')
    for lit in re.findall(r'"((?:[^"\\\n]|\\.)*)"', t):
        l = lit.replace('\\\\', '/')
        if l.startswith('_') and l.lower().endswith(IMG + SND + ('.md3', '.skin')) and '/' not in l:
            suffixes.add(norm(l))          # "_flash.md3": appended to a model's name in code
            continue
        if ('/' in l or l.lower().endswith(IMG + SND + ('.md3', '.skin'))) and ' ' not in l.strip():
            add_pattern(l, 'Code ' + f.name)
        if '/' in l:
            # paths inside longer strings ("r_colorLut=gfx/luts/warm")
            for sub in re.findall(r'(?:models|gfx|menu|textures|sprites|icons|levelshots|env|sound|music|fonts|ui|video)/[A-Za-z0-9_./%\-]+', l):
                add_pattern(sub, 'Code ' + f.name)
        elif l and ' ' not in l and norm(l) in shaders:
            ref(l, 'Code ' + f.name)        # shader registered by its bare name ("rocketExplosion")
# 3. text configs
for p in sorted(files):
    if p.endswith(('.arena', '.txt', '.cfg', '.menu', '.game', '.skin')) and not p.startswith('botfiles/'):
        t = files[p]().decode('latin-1', 'ignore')
        for tok in re.findall(r'[A-Za-z0-9_\-./\\]+', t):
            if '/' in tok and re.match(r'^(models|gfx|menu|textures|sprites|icons|levelshots|env|sound|music|fonts|ui|video|maps)/', tok, re.I):
                ref(tok, 'Datei ' + p)
        if p.endswith('.arena') or p.endswith('arenas.txt'):
            for m in re.findall(r'map\s+"?([A-Za-z0-9_\-]+)', t):
                ref('levelshots/' + m, 'Levelshot ' + m)
# 4. whole folders the code lists at run time (cars, characters, music)
for p in files:
    if p.startswith(('models/players/', 'music/', 'fonts/')):
        ref(p, 'Ordner wird zur Laufzeit gelistet')

# ---------------------------------------------------------------- closure
names = sorted(set(shaders) | set(by_stem))
pat_done = False
while True:
    while todo:
        r, why = todo.pop()
        resolve(r, why)
    if not pat_done:
        for rx, why in patterns:
            for n in names:
                if rx.match(n): ref(n, why)
        pat_done = True
    for m in list(used):
        if m.endswith('.md3'):
            for sfx in suffixes:
                cand = m[:-4] + sfx
                if cand in files and cand not in used:
                    ref(cand, via.get(m, '') + ' (Code hängt ' + sfx + ' an)')
    if not todo:
        break

# ---------------------------------------------------------------- map sources (.map) and result
src_refs = collections.defaultdict(set)
for p in files:
    if p.startswith('maps/') and p.endswith(('.map', '.bpd')):
        t = files[p]().decode('latin-1', 'ignore')
        for tok in set(re.findall(r'(?:^|[\s"])\(?\s*([A-Za-z0-9_\-]+/[A-Za-z0-9_\-/.]+)', t)):
            s = stem('textures/' + norm(tok)) if not tok.lower().startswith(('textures/', 'models/')) else stem(norm(tok))
            src_refs[s].add(p[5:])

MODEL = ('.md3', '.mdr', '.iqm')
rows = []
for p in sorted(files):
    if not p.endswith(IMG + MODEL + SND) or p.startswith('maps/'): continue
    if p.endswith(MODEL + SND) and '!' not in p and p.startswith(('models/players/', 'music/')): continue
    if p in used: status = 'benutzt'
    elif p in editor_only: status = 'nur Editorbild'
    elif stem(p) in src_refs: status = 'nur in Map-Quelle'
    else: status = 'ungenutzt'
    typ = 'Bild' if p.endswith(IMG) else 'Modell' if p.endswith(MODEL) else 'Sound'
    rows.append({'pfad': p, 'typ': typ, 'status': status, 'grund': via.get(p, ''),
                 'quelle': ', '.join(sorted(src_refs.get(stem(p), ()))[:4])})
json.dump({'rows': rows, 'shaders_used': len(used_shaders), 'shaders': len(shaders)}, open(sys.argv[2], 'w'))
c = collections.Counter((r['typ'], r['status']) for r in rows)
print(c, len(used_shaders), '/', len(shaders), 'shaders used')
