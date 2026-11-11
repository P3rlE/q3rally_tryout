"""Verify an extracted WebUI ZIP and run bounded Windows native startup probes."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.build_webui_package import hashes, verify_common
from tools.release_assets import safe_name


def extract(archive, output):
    with zipfile.ZipFile(archive) as bundle:
        members = bundle.infolist()
        roots = set()
        for item in members:
            name = safe_name(item.filename.rstrip('/'))
            if (item.external_attr >> 16) & 0o170000 == 0o120000:
                raise ValueError('Package symlinks are not supported')
            roots.add(name.split('/')[0])
        if len(roots) != 1: raise ValueError('Expected one package root')
        bundle.extractall(output)
    return output / roots.pop()


def validate(root):
    manifest = json.loads((root / 'webui-build.json').read_text())
    actual = hashes(root); actual.pop('webui-build.json')
    if actual != manifest['files']:
        raise ValueError('Package has missing, extra or modified files')
    # Recheck the canonical bytes independently of the native package manifest.
    common = json.loads((root / 'common-manifest.json').read_text())
    if common['revision'] != manifest['revision']:
        raise ValueError('Package/common revision mismatch')
    for name, info in common['files'].items():
        if actual.get(name) != info: raise ValueError(f'Changed common asset: {name}')
    for name in ('libcef.dll','chrome_elf.dll','v8_context_snapshot.bin','icudtl.dat',
                 'resources.pak','locales/en-US.pak','licenses/CEF-LICENSE.txt',
                 'licenses/CURL-COPYING.txt','Start-WebUI.cmd','Start-Native.cmd',
                 'webui/garage/vehicleShowroom.bundle.js','webui/garage/THREE-LICENSE.txt',
                 'webui/garage/fonts/Barlow-Regular.ttf','webui/garage/fonts/ShareTechMono-OFL.txt'):
        if not (root / name).is_file(): raise ValueError(f'Missing required runtime file: {name}')
    with zipfile.ZipFile(root / 'baseq3r/qvms.pk3') as bundle:
        if set(bundle.namelist()) != {'vm/cgame.qvm','vm/qagame.qvm','vm/ui.qvm'}:
            raise ValueError('Unexpected QVM package contents')
    if any((root/'baseq3r').glob('vm/*')): raise ValueError('Loose QVMs in test package')
    return manifest


def run_native(root, output, first_run=False):
    executable = root / 'q3rally.x86_64.exe'
    env = os.environ.copy()
    # Do not mask missing MinGW/CEF dependencies with developer tools on PATH.
    system_root = os.environ['SystemRoot']
    env['PATH'] = os.pathsep.join([str(root),str(Path(system_root)/'System32'),system_root])
    flags = subprocess.CREATE_NO_WINDOW
    version = subprocess.run([str(executable),'--version'],env=env,cwd=root,
        capture_output=True,timeout=30,creationflags=flags)
    (output/'version.log').write_bytes(version.stdout+version.stderr)
    if version.returncode != 0 or b'Q3Rally v0.9.0-webui' not in version.stdout+version.stderr:
        raise ValueError(f'Packaged executable/version probe failed: {version.returncode}')
    home = output/'home'; profiles=home/'baseq3r/profiles'; profiles.mkdir(parents=True)
    if not first_run: (profiles/'WebUIPackageProbe.json').write_text(json.dumps({
        'profileVersion':2,'identity':{'uuid':'12345678-1234-4234-8234-123456789abc',
        'name':'WebUIPackageProbe','info':{'country':'DE'}},
        'webLink':{'state':'unlinked','accountId':None,'profileId':None},
        'progress':{'epoch':'v0.8-local-1','authority':'local','currentRank':0,
        'highestRank':0,'stats':{'playerScore':0}}}))
    (home/'baseq3r/webui-probe.cfg').write_text(
        'echo WEBUI_PACKAGE_PROBE_BEGIN\nwait 180\n'
        'echo WEBUI_FULLSCREEN_BEGIN\nset r_fullscreen 1\nvid_restart\nwait 180\n'
        'echo WEBUI_WINDOWED_BEGIN\nset r_fullscreen 0\nvid_restart\nwait 180\n'
        'echo WEBUI_PACKAGE_PROBE_END\nquit\n')
    command=[str(executable),'+set','fs_basepath',str(root),'+set','fs_homepath',str(home),
        '+set','logfile','2','+set','net_enabled','0','+set','cl_motd','0',
        '+set','sv_ladderEnabled','0','+set','r_fullscreen','0','+set','r_mode','-1',
        '+set','r_customwidth','1100','+set','r_customheight','720','+set','cl_renderer','opengl1',
        '+set','s_initsound','0','+set','in_mouse','0','+set','in_nograb','0',
        '+set','com_maxfps','30','+set','com_maxfpsUnfocused','30',
        '+set','cl_webUI','1','+exec','webui-probe.cfg']
    if not first_run: command[1:1]=['+set','profile_active','WebUIPackageProbe','+set','ladder_wizard_dismissed_profiles','WebUIPackageProbe']
    with (output/'native.log').open('wb') as log:
        process=subprocess.Popen(command,env=env,cwd=root,stdout=log,stderr=subprocess.STDOUT,
            stdin=subprocess.DEVNULL,creationflags=flags)
        try: process.wait(timeout=180)
        except subprocess.TimeoutExpired:
            # Only the test process tree spawned here; not other running games.
            subprocess.run(['taskkill','/PID',str(process.pid),'/T','/F'],capture_output=True)
            process.wait(timeout=15)
            raise ValueError('Packaged CEF client startup timed out')
    text=(output/'native.log').read_text(errors='replace')
    console=home/'baseq3r/qconsole.log'
    if console.exists(): text += console.read_text(errors='replace')
    if process.returncode != 0: raise ValueError(f'Packaged client exited with {process.returncode}')
    if first_run and 'Web UI: registration step 17.' not in text:
        raise ValueError('First-run driver wizard was not published to the WebUI')
    for marker in ('WEBUI_PACKAGE_PROBE_BEGIN','Web UI: bundled frontend loaded (HTTP 200).','WEBUI_PACKAGE_PROBE_END'):
        if marker not in text: raise ValueError(f'Missing native startup marker: {marker}')
    # Each renderer restart destroys the SDL parent. Require a fresh browser
    # attachment and successful document load in both replacement windows.
    for begin, end, mode in (
        ('WEBUI_FULLSCREEN_BEGIN','WEBUI_WINDOWED_BEGIN','fullscreen'),
        ('WEBUI_WINDOWED_BEGIN','WEBUI_PACKAGE_PROBE_END','windowed'),
    ):
        section = text.split(begin, 1)[1].split(end, 1)[0]
        if f'Web UI: browser attached ({mode},' not in section or 'Web UI: bundled frontend loaded (HTTP 200).' not in section:
            raise ValueError(f'WebUI did not recover after the {mode} renderer restart')
    if re.search(r'(^|\n)(?:\^1)?(?:ERROR:|FATAL)',text): raise ValueError('Native startup reported an error')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--archive',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args(); output=args.output.resolve(); output.mkdir(parents=True,exist_ok=False)
    result={'success':False}
    try:
        root=extract(args.archive.resolve(),output/'extracted')
        manifest=validate(root)
        if os.name != 'nt': raise ValueError('Native startup validation requires Windows')
        run_native(root,output)
        first_run=output/'first-run';first_run.mkdir()
        run_native(root,first_run,first_run=True)
        result.update(success=True,revision=manifest['revision'],cef=manifest['cef']['version'])
    except Exception as error:
        result['error']=str(error)
        raise
    finally:
        (output/'result.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
