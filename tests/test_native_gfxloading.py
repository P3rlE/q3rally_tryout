"""The native graphics loading screen has to look like the WebUI's screen 1.

tests/fixtures/webui_loading_layout.json holds what Chrome reports for the
WebUI (tools/ui_webstyle/measure_webui_loading.cjs, ten viewports, four
states). The native layout and text measurement run here against it.
"""
import json
import pathlib
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
CODE = ROOT / 'engine/code'
FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_loading_layout.json').read_text())
TITLES = {'s2': 'Preparing the setup menu', 's0': 'Starting up', 's8': 'Ready to race', 'prompt': 'Update available'}

HARNESS = r'''
#include "q3_ui/ui_local.h"
#include "q3_ui/ui_rally_gfxlayout.h"
#include <stdio.h>
uiStatic_t uis;
static FILE *files[8];
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode) {
    char path[1024]; FILE *h; int i = 1, length;
    snprintf(path, sizeof(path), "%s/%s", BASE, qpath);
    if (!(h = fopen(path, "rb"))) { *f = 0; return -1; }
    while (files[i]) i++;
    files[i] = h; *f = i;
    fseek(h, 0, SEEK_END); length = ftell(h); fseek(h, 0, SEEK_SET);
    return length;
}
void trap_FS_Read(void *buffer, int length, fileHandle_t f) { fread(buffer, 1, length, files[f]); }
void trap_FS_FCloseFile(fileHandle_t f) { fclose(files[f]); files[f] = 0; }
qhandle_t trap_R_RegisterShaderNoMip(const char *name) { return 1; }
void trap_R_SetColor(const float *rgba) {}
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t shader) {}
void QDECL Com_Printf(const char *fmt, ...) {}
void QDECL Com_Error(int level, const char *fmt, ...) { abort(); }
#include "q3_ui/ui_rally_webstyle.c"
#include "q3_ui/ui_rally_gfxlayout.c"

static void rect(const char *name, wsRect_t r) { printf("\"%s\":[%g,%g,%g,%g],", name, r.x, r.y, r.w, r.h); }

int main(void) {
    char command[16], text[256];
    while (scanf("%15s", command) == 1) {
        if (!strcmp(command, "layout")) {
            gfxLayout_t l; float w, h; int prompt, i;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %d %255[^\n]", &w, &h, &prompt, text);
            Q_strncpyz(l.title, text, sizeof(l.title));
            strcpy(l.vehicle, "Alpine"); strcpy(l.status, "Ready for the next rally");
            l.prompt = prompt;
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            GFX_Layout(&l, w, h);
            printf("{");
            rect("left", l.left); rect("right", l.right); rect("car", l.car);
            rect("update", l.buttons[0]); rect("skip", l.buttons[1]);
            printf("\"h1\":%g,\"status\":[%g,%g],\"title\":%g,\"progress\":[%g,%g,%g],\"percent\":%g,",
                l.h1Y, l.statusX, l.statusY, l.titleY, l.progressX, l.progressY, l.progressW, l.percentY);
            printf("\"prompt\":%g,\"vehicle\":%g,\"statusText\":%g,\"stageX\":%g,\"page\":%g,",
                l.promptY, l.vehicleY, l.statusTextY, l.stageX, l.pageHeight);
            printf("\"lines\":[%d,%d,%d,%d],\"stages\":[", l.titleLines.count, l.promptLines.count,
                l.vehicleLines.count, l.loadingLabelLines.count);
            for (i = 0; i < GFX_LAYOUT_STAGES; i++) printf("%s%g", i ? "," : "", l.stageY[i]);
            printf("]}\n");
        } else if (!strcmp(command, "width")) {
            wsTextStyle_t style; int font, upper; float size, spacing; char shown[256];
            scanf("%d %f %f %d %255[^\n]", &font, &size, &spacing, &upper, text);
            style.font = font; style.size = size; style.spacing = spacing; style.lineHeight = size; style.uppercase = upper;
            WS_Transform(&style, text, shown, sizeof(shown));
            printf("%g\n", WS_TextWidth(&style, shown, strlen(shown)));
        }
        fflush(stdout);
    }
    return 0;
}
'''


def compile_harness(folder):
    source = folder / 'harness.c'
    source.write_text(HARNESS.replace('BASE', json.dumps(str(ROOT / 'baseq3r'))))
    exe = folder / 'harness'
    command = [shutil.which('gcc') or 'gcc', '-DARCH_STRING="test"', '-I' + str(CODE), '-w', str(source),
               str(CODE / 'qcommon/q_shared.c'), str(CODE / 'qcommon/q_math.c'), '-o', str(exe), '-lm']
    subprocess.run(command, check=True)
    return exe


def run(exe, commands):
    result = subprocess.run([str(exe)], input='\n'.join(commands) + '\n', capture_output=True, text=True,
                            check=True, timeout=60)
    return result.stdout.splitlines()


def close(mine, chrome, tolerance=0.5):
    return all(abs(a - b) <= tolerance for a, b in zip(mine, chrome))


def test_native_layout_matches_the_webui_at_every_size():
    keys = sorted(FIXTURE)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        commands = []
        for key in keys:
            size, state = key.split('_')
            width, height = size.split('x')
            commands.append(f'layout {width} {height} {int(state == "prompt")} {TITLES[state]}')
        output = [json.loads(line) for line in run(exe, commands)]
    for key, mine in zip(keys, output):
        chrome = FIXTURE[key]['els']
        problems = []

        def check(name, value, reference):
            if not close(value, reference):
                problems.append((name, value, reference))

        for name in ('left', 'right', 'car'):
            check(name, mine[name], chrome[name])
        check('h1', [mine['h1']], [chrome['h1'][1]])
        check('status', mine['status'], chrome['status'][:2])
        check('title', [mine['title']], [chrome['title'][1]])
        check('percent', [mine['percent']], [chrome['percent'][1]])
        check('progress', mine['progress'], chrome['progress'][:3])
        check('vehicle', [mine['vehicle']], [chrome['vehicle'][1]])
        check('statusText', [mine['statusText']], [chrome['statusText'][1]])
        check('stages', mine['stages'], [stage[1] for stage in chrome['stages']])
        check('stageX', [mine['stageX']], [chrome['stages'][0][0]])
        check('page', [mine['page']], [FIXTURE[key]['height']])
        if key.endswith('prompt'):
            check('prompt', [mine['prompt']], [chrome['prompt'][1]])
            check('update', mine['update'], chrome['update'])
            check('skip', mine['skip'], chrome['skip'])
        lines = {(text['text'], text['family']): len(text['lines']) for text in FIXTURE[key]['texts']}
        assert mine['lines'][0] == lines[(TITLES[key.split('_')[1]], 'Barlow Condensed')], key
        assert mine['lines'][2] == lines[('Alpine', 'Barlow Condensed')], key
        assert mine['lines'][3] == lines[('01 / PREPARING YOUR RALLY', 'Share Tech Mono')], key
        if key.endswith('prompt'):
            assert mine['lines'][1] == lines[('A newer version of Q3Rally is available.', 'Barlow')], key
        assert not problems, (key, problems)


def font_of(text):
    if text['family'] == 'Share Tech Mono':
        return 0
    if text['family'] == 'Barlow Condensed':
        return 4 if text['italic'] else 3
    return 2 if text['weight'] >= 600 else 1


def test_text_runs_measure_like_chrome():
    """Advances, pair kerning, letter-spacing and text-transform."""
    runs = {}
    for key, data in FIXTURE.items():
        for text in data['texts']:
            if len(text['lines']) == 1:
                runs[(font_of(text), text['size'], text['spacing'], text['uppercase'], text['text'])] = text['lines'][0][2]
    keys = sorted(runs)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        widths = run(exe, [f'width {f} {s} {sp} {int(u)} {t}' for f, s, sp, u, t in keys])
    assert len(keys) > 40
    for key, width in zip(keys, widths):
        assert abs(float(width) - runs[key]) < 0.1, (key, width, runs[key])


def test_vehicle_names_follow_the_webui_catalog():
    labels = json.loads((ROOT / 'engine/webui/catalog_labels.json').read_text())['vehicles']
    header = (CODE / 'q3_ui/ui_rally_vehicle_names.h').read_text()
    native = dict(re.findall(r'ENTRY\( "([^"]+)", "([^"]+)" \)', header))
    assert native == {key: value['name'] for key, value in labels.items()}


def test_font_atlases_and_art_are_complete():
    fonts = ROOT / 'baseq3r/menu/fonts'
    for name in ('sharetechmono', 'barlow', 'barlow_bold', 'barlowcondensed', 'barlowcondensed_italic'):
        data = (fonts / f'{name}.dat').read_bytes()
        magic, version, upm, ascent, descent, glyphs, kerns, sizes = struct.unpack_from('<4s7i', data)
        assert magic == b'Q3UF' and version == 1 and glyphs <= 224 and kerns <= 4096 and 1 <= sizes <= 12
        offset = 32 + glyphs * 8 + kerns * 8
        for _ in range(sizes):
            size100, width, height, image = struct.unpack_from('<3i64s', data, offset)
            offset += 76 + glyphs * 24
            path = ROOT / 'baseq3r' / (image.split(b'\0')[0].decode() + '.png')
            assert path.is_file(), path
            assert path.read_bytes()[16:24] == struct.pack('>2I', width, height), path
        assert offset == len(data)
    shaders = (ROOT / 'baseq3r/scripts/q3r_webstyle.shader').read_text()
    for art in ('web_stripes', 'web_showroom', 'web_progress_track', 'web_progress_fill'):
        assert f'menu/ui/{art}\n' in shaders
        assert (ROOT / f'baseq3r/menu/ui/{art}.png').is_file()
    for license in ('Barlow-OFL.txt', 'BarlowCondensed-OFL.txt', 'ShareTechMono-OFL.txt'):
        assert (fonts / license).is_file()
