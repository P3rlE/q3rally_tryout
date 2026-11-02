"""Install and start a native launcher ZIP without publishing or using real secrets."""
import argparse
import base64
from datetime import datetime, timedelta, timezone
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from launcher.core import Manager, atomic_json
from tools.publish_launcher_catalog import Index, package_record, sign


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--release-index', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    index = Index(args.release_index.resolve())
    artifact, = index.data['artifacts']
    version, target = index.data['version'], artifact['target']
    package = package_record(index, artifact, 'https://example.invalid', 'Q3RallyLauncher')
    key = Ed25519PrivateKey.generate()
    keys = {'package-test': base64.b64encode(key.public_key().public_bytes_raw()).decode()}
    envelope = sign({'schema': 1, 'product': 'Q3Rally', 'sequence': 1,
        'expires': (datetime.now(timezone.utc) + timedelta(days=1)).isoformat(), 'releases': [],
        'launchers': [{'version': version, 'packages': {target: package}}]}, key, 'package-test')
    manager = Manager(output / 'data', keys, product='Q3RallyLauncher')
    shutil.copyfile(index.parent / artifact['filename'], manager.cache / (package['sha256'] + '.part'))
    manager.install(envelope, version, target)
    executable = manager.verify(version, target)
    # An empty trust store makes catalog refresh fail locally before networking.
    config = output / 'offline-config.json'
    atomic_json(config, {'catalog_url': 'https://example.invalid/catalog.json', 'public_keys': {}})
    env = dict(os.environ)
    for name in ('PYTHONHOME', 'PYTHONPATH', 'LD_LIBRARY_PATH', 'TCL_LIBRARY', 'TK_LIBRARY'):
        env.pop(name, None)
    command = [str(executable), '--config', str(config), '--data-dir', str(output / 'app-data')]
    options = {'creationflags': subprocess.CREATE_NO_WINDOW} if os.name == 'nt' else {}
    cli = subprocess.run(command + ['--action', 'list'], env=env, capture_output=True, timeout=40, **options)
    assert cli.returncode == 0, cli.stderr
    if cli.stdout:  # A Windows --windowed executable has no console output.
        assert json.loads(cli.stdout)['available'] == []
    # Check real HTTPS/signature verification through the packaged executable too.
    livehome = output / 'live-app-data'
    live = subprocess.run([str(executable), '--config', str(ROOT / 'launcher/config.json'),
        '--data-dir', str(livehome), '--action', 'list'], env=env, capture_output=True, timeout=60, **options)
    assert live.returncode == 0, live.stderr
    assert (livehome / 'catalog-cache.json').is_file(), 'Packaged launcher could not verify the live HTTPS catalog'
    with (output / 'gui.log').open('wb') as log:
        process = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT, **options)
        try:
            try:
                process.wait(timeout=5)
                raise AssertionError(f'Packaged GUI exited early: {process.returncode}')
            except subprocess.TimeoutExpired:
                pass
            logpath = output / 'app-data/logs/launcher.log'
            assert logpath.is_file(), 'Packaged GUI did not initialize logging'
            assert 'Launcher failed' not in logpath.read_text(errors='replace')
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill(); process.wait(timeout=10)
    atomic_json(output / 'result.json', {'version': version, 'target': target,
        'signed_install_verified': True, 'packaged_cli_started': True,
        'packaged_gui_started': True, 'live_https_catalog_verified': True,
        'external_python_environment_removed': True})
    print(f'{target}: signed install, packaged CLI and GUI startup PASS', flush=True)


if __name__ == '__main__':
    main()
