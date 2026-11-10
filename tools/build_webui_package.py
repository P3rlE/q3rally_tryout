"""Build common WebUI data once, then assemble an isolated Windows x64 test package.

Neither operation requires a website checkout. This does not publish a release.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / 'engine/webui'))
from make_release import make_asset_pk3, runtime_plan, sha256, zip_info, QVMS
from tools.release_assets import audit, inventory, safe_name
from build_showroom import build as build_showroom

VERSION = '0.9.0-webui'

def files(root):
    for path in sorted(root.rglob('*')):
        if path.is_symlink(): raise ValueError(f'Symlink in package: {path}')
        if path.is_file(): yield path

def hashes(root):
    return {p.relative_to(root).as_posix(): {'sha256':sha256(p), 'size':p.stat().st_size} for p in files(root)}

def write_json(path, data):
    path.write_text(json.dumps(data, sort_keys=True, indent=2) + '\n', encoding='utf-8')

def validate_provenance(revision, epoch):
    if not re.fullmatch(r'[0-9a-f]{40}', revision) or epoch <= 0:
        raise ValueError('Expected full source commit SHA and positive SOURCE_DATE_EPOCH')

def common(assets, vm, output, revision, epoch):
    validate_provenance(revision, epoch)
    output.mkdir(parents=True, exist_ok=False)
    policy = json.loads((ROOT / 'tools/release_policy.json').read_text())
    policy['roots'].append('fonts')
    selected, _ = inventory(assets, policy)
    if 'default.cfg' not in selected or 'scripts/cars.shader' not in selected:
        raise ValueError('Incomplete game asset source: default.cfg/cars.shader missing')
    report = audit(selected)
    write_json(output / 'asset-audit.json', report)
    if report['errors']:
        raise ValueError('Game asset audit failed; see asset-audit.json')
    base = output / 'baseq3r'; base.mkdir()
    make_asset_pk3(base / 'pak0.pk3', selected)
    with zipfile.ZipFile(base / 'qvms.pk3', 'w', allowZip64=False) as archive:
        for name in QVMS:
            data = (vm / name).read_bytes()
            if data[:4] not in (b'\x44\x14\x72\x12', b'\x45\x14\x72\x12'):
                raise ValueError(f'Not a QVM: {name}')
            archive.writestr(zip_info('vm/' + name), data)
    build_showroom(output / 'webui/garage', base / 'pak0.pk3')
    # License bundle from the same tracked source as the native build.
    licenses = output / 'licenses'; licenses.mkdir()
    shutil.copyfile(ROOT / 'engine/docs/COPYING.txt', licenses / 'COPYING-engine.txt')
    for vendor in sorted((ROOT / 'engine/code').iterdir()):
        if vendor.is_dir():
            for name in ('COPYING','COPYING.txt','LICENSE','LICENSE.txt','LICENSE.md'):
                if (vendor / name).is_file():
                    target = licenses / vendor.name / name
                    target.parent.mkdir(exist_ok=True)
                    shutil.copyfile(vendor / name, target)
    write_json(output / 'common-manifest.json', {'schema':1, 'version':VERSION,
        'revision':revision, 'source_date_epoch':epoch, 'files':hashes(output)})
    return output

def verify_common(root, revision):
    manifest = json.loads((root / 'common-manifest.json').read_text())
    if manifest['revision'] != revision or manifest['version'] != VERSION:
        raise ValueError('Common data and native source revision/version differ')
    expected = manifest['files']
    actual = hashes(root)
    actual.pop('common-manifest.json', None)
    if actual != expected: raise ValueError('Common data has missing, extra or modified files')
    return manifest

def copy_cef(sdk, target):
    (target / 'licenses').mkdir(parents=True, exist_ok=True)
    lock = json.loads((ROOT / 'engine/webui/cef.lock.json').read_text())
    if lock['version'] not in (sdk / 'README.txt').read_text():
        raise ValueError('CEF SDK version mismatch')
    for name in ('libcef.dll','chrome_elf.dll','v8_context_snapshot.bin'):
        if not (sdk / 'Release' / name).is_file(): raise ValueError(f'Missing CEF runtime: {name}')
    for name in ('icudtl.dat','resources.pak','chrome_100_percent.pak','chrome_200_percent.pak','locales/en-US.pak'):
        if not (sdk / 'Resources' / name).is_file(): raise ValueError(f'Missing CEF resource: {name}')
    for source in files(sdk / 'Release'):
        if source.suffix.lower() in ('.dll','.bin','.json'):
            shutil.copyfile(source, target / source.name)
    for source in files(sdk / 'Resources'):
        dest = target / source.relative_to(sdk / 'Resources')
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, dest)
    shutil.copyfile(sdk / 'LICENSE.txt', target / 'licenses/CEF-LICENSE.txt')
    shutil.copyfile(sdk / 'README.txt', target / 'licenses/CEF-README.txt')
    return lock

def package(common_root, build, sdk, output, revision):
    manifest = verify_common(common_root, revision)
    output.mkdir(parents=True, exist_ok=False)
    target = output / ('Q3Rally-WebUI-' + revision[:12] + '-windows-x86_64')
    target.mkdir()
    with tempfile.TemporaryDirectory() as directory:
        vm = Path(directory)
        with zipfile.ZipFile(common_root / 'baseq3r/qvms.pk3') as archive:
            for name in QVMS: (vm / name).write_bytes(archive.read('vm/' + name))
        runtime, _ = runtime_plan('windows-x86_64', build, VERSION, vm)
        for item in runtime:
            if item.name.startswith('baseq3r/vm/'): continue
            dest = target / safe_name(item.name)
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(item.path, dest)
    client = target / 'q3rally.x86_64.exe'
    if b'cl_webUI' not in client.read_bytes(): raise ValueError('Native client was not built with CEF')
    for source in files(common_root):
        dest = target / source.relative_to(common_root)
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, dest)
    lock = copy_cef(sdk, target)
    # Workflow supplies this explicitly from the verified curl source archive.
    staged_license = build / 'CURL-COPYING.txt'
    if not staged_license.is_file(): raise ValueError('Missing static curl license')
    shutil.copyfile(staged_license, target / 'licenses/CURL-COPYING.txt')
    for mode, enabled in (('WebUI',1),('Native',0)):
        command = ('@echo off\ncd /d "%~dp0"\n'
            '"%~dp0q3rally.x86_64.exe" +set fs_basepath "%~dp0." '
            f'+set fs_homepath "%LOCALAPPDATA%\\Q3Rally-WebUI\\{revision[:12]}" '
            f'+set cl_webUI {enabled} +set r_fullscreen 0 +set r_mode -1 '
            '+set r_customwidth 1280 +set r_customheight 800 +set sv_ladderEnabled 0\n')
        (target / f'Start-{mode}.cmd').write_bytes(command.replace('\n','\r\n').encode())
    (target / 'README-WebUI.txt').write_text(
        'Q3Rally v0.9 WebUI development test package\n\n'
        'Extract the whole ZIP, then run Start-WebUI.cmd. No Python, Node, browser or website checkout is required.\n'
        'Start-Native.cmd selects the legacy menus in the same test build.\n'
        'Settings use an isolated Q3Rally-WebUI directory under LOCALAPPDATA. Production profiles are untouched.\n'
        'This is the Windows x64 CEF pilot, not an official release. The CEF sandbox is disabled.\n'
        'Interactive fullscreen/input QA, other platforms and production security review remain separate work.\n'
        'CEF and third-party license notices are bundled under licenses/.\n'
        f'Source: {revision}\nCEF: {lock["version"]}\n', encoding='utf-8')
    write_json(target / 'webui-build.json', {'schema':1, 'version':VERSION,
        'revision':revision, 'source_date_epoch':manifest['source_date_epoch'],
        'cef':lock, 'files':hashes(target)})
    archive_path = output / (target.name + '.zip')
    with zipfile.ZipFile(archive_path, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for source in files(target):
            archive.writestr(zip_info(target.name + '/' + source.relative_to(target).as_posix()), source.read_bytes())
    (output / 'SHA256SUMS').write_text(sha256(archive_path) + '  ' + archive_path.name + '\n')
    print(archive_path)
    return target

def main():
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest='command', required=True)
    c = sub.add_parser('common')
    c.add_argument('--assets', type=Path, default=ROOT/'baseq3r')
    c.add_argument('--vm-dir', type=Path, required=True)
    c.add_argument('--output', type=Path, required=True)
    c.add_argument('--revision', required=True)
    c.add_argument('--epoch', type=int, required=True)
    n = sub.add_parser('package')
    for name in ('common','build','cef','output'): n.add_argument('--'+name, type=Path, required=True)
    n.add_argument('--revision', required=True)
    args = p.parse_args()
    if args.command == 'common': common(args.assets.resolve(),args.vm_dir.resolve(),args.output.resolve(),args.revision,args.epoch)
    else: package(args.common.resolve(),args.build.resolve(),args.cef.resolve(),args.output.resolve(),args.revision)

if __name__ == '__main__': main()
