"""Bounded native desktop-client checks using an isolated home and staged data.

Requires a real display/OpenGL context and audio device. This verifies device
initialization, not perceived sound quality or interactive input usability.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import time

UUID = '12345678-1234-4234-8234-123456789abc'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--basepath', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--renderer', action='append', choices=('opengl1', 'opengl2'))
    parser.add_argument('--timeout', type=int, default=180)
    args = parser.parse_args()
    executable = args.executable.resolve(strict=True)
    basepath = args.basepath.resolve(strict=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    home = output / 'home'
    game = home / 'baseq3r'
    profiles = game / 'profiles'
    profiles.mkdir(parents=True)
    profile = profiles / 'InstallProbe.json'
    profile.write_text(json.dumps({'uuid': UUID, 'name': 'InstallProbe',
                                  'info': {'country': 'DE'},
                                  'stats': {'kills': 3, 'deaths': 1, 'wins': 2,
                                            'playerScore': 321, 'distanceKm': 2.5}}))
    results = []
    for index, renderer in enumerate(args.renderer or ('opengl1', 'opengl2')):
        script = ['echo CLIENT_PROBE_BEGIN', 'wait 360', 'echo PROBE_SETTINGS_BEGIN',
                  'sensitivity', 'cg_drawFPS', 'bind F8', 'profile_active', 'cl_uuid',
                  'echo PROBE_SETTINGS_END', f'screenshotJPEG menu-{renderer}', 'wait 2']
        if index == 0:
            script += ['seta sensitivity 7.25', 'seta cg_drawFPS 1',
                       'bind F8 screenshotJPEG', 'writeconfig persisted.cfg']
        # The world is loaded synchronously before these frames. A short warmup
        # covers first snapshots without turning software rendering into a benchmark.
        script += ['map q3r_country01', 'wait 30', f'screenshotJPEG track-{renderer}',
                   'play sound/misc/menu1.ogg', 'wait 5', 'vid_restart', 'wait 30',
                   f'screenshotJPEG restarted-{renderer}', 'wait 2', 'sensitivity',
                   'cg_drawFPS', 'bind F8', 'profile_active', 'cl_uuid',
                   'echo CLIENT_PROBE_END', 'quit']
        (game / 'validation.cfg').write_text('\n'.join(script) + '\n')
        command = [str(executable), '+set', 'fs_basepath', str(basepath),
                   '+set', 'fs_homepath', str(home), '+set', 'r_fullscreen', '0',
                   '+set', 'r_mode', '-1', '+set', 'r_customwidth', '800',
                   '+set', 'r_customheight', '600', '+set', 'cl_renderer', renderer,
                   '+set', 'in_mouse', '0', '+set', 'in_nograb', '1',
                   '+set', 's_initsound', '1', '+set', 's_useOpenAL', '0',
                   '+set', 's_sdlCapture', '0', '+set', 'com_maxfps', '30',
                   '+set', 'com_maxfpsUnfocused', '30', '+set', 'com_maxfpsMinimized', '30',
                   '+set', 'cl_motd', '0', '+set', 'net_ip', '127.0.0.1',
                   '+set', 'net_enabled', '1', '+set', 'sv_ladderEnabled', '0',
                   '+set', 'profile_active', 'InstallProbe', '+set', 'logfile', '2',
                   '+exec', 'validation.cfg']
        kwargs = {}
        if os.name == 'nt':
            startup = subprocess.STARTUPINFO()
            startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
            startup.wShowWindow = 0
            kwargs['startupinfo'] = startup
        print(f'START {renderer}', flush=True)
        with (output / f'{renderer}.stdout.log').open('w', encoding='utf-8') as log:
            proc = subprocess.Popen(command, cwd=basepath, stdout=log,
                                    stderr=subprocess.STDOUT, **kwargs)
            deadline = time.monotonic() + args.timeout
            code = None
            while proc.poll() is None and time.monotonic() < deadline:
                live = (output / f'{renderer}.stdout.log').read_text(encoding='utf-8', errors='replace')
                if "ERROR: Couldn't compile shader" in live or 'recursive error after:' in live:
                    code = 'fatal_shader_error'
                    break
                time.sleep(.25)
            if proc.poll() is None:
                code = code or 'timeout'
                proc.terminate()
                try:
                    proc.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait(timeout=10)
            else:
                code = proc.returncode
        text = (output / f'{renderer}.stdout.log').read_text(encoding='utf-8', errors='replace')
        console = game / 'qconsole.log'
        if console.is_file():
            text = console.read_text(encoding='utf-8', errors='replace')
            (output / f'{renderer}.qconsole.log').write_text(text, encoding='utf-8')
        settings = text.split('PROBE_SETTINGS_BEGIN\n', 1)[-1].split('PROBE_SETTINGS_END', 1)[0]
        config = (game / 'q3config.cfg').read_text(errors='replace') if (game / 'q3config.cfg').is_file() else ''
        images = {stage: (game / f'screenshots/{stage}-{renderer}.jpg').is_file()
                  for stage in ('menu', 'track', 'restarted')}
        identity = json.loads(profile.read_text())
        checks = {'normal_exit': code == 0, 'end_marker': 'CLIENT_PROBE_END' in text,
                  'audio_initialized': 'SDL audio initialized.' in text,
                  'renderer_initialized': bool(re.search(r'GL_RENDERER:', text)),
                  'requested_renderer': f'Trying to load "renderer_{renderer}_' in text and
                      f'Trying to load "renderer_{"opengl2" if renderer == "opengl1" else "opengl1"}_' not in text,
                  'renderer_restarted': text.count(f'Trying to load "renderer_{renderer}_') >= 2,
                  'screenshots': all(images.values()),
                  'profile_preserved': identity.get('uuid') == UUID and identity.get('stats', {}).get('kills') == 3,
                  'profile_loaded': UUID in text,
                  'settings_saved': '7.25' in config and 'screenshotJPEG' in config,
                  'settings_reloaded': index == 0 or ('7.25' in settings and 'screenshotJPEG' in settings)}
        result = {'renderer': renderer, 'exit': code, 'checks': checks, 'images': images,
                  'gl_renderer': re.findall(r'GL_RENDERER:([^\r\n]*)', text),
                  'audio_driver': re.findall(r'SDL audio driver is ([^\r\n]*)', text),
                  'passed': all(checks.values())}
        results.append(result)
        (output / 'summary.json').write_text(json.dumps(results, indent=2) + '\n')
        print(json.dumps(result), flush=True)
    return 0 if all(result['passed'] for result in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
