"""Fetch and verify the exact Windows x64 CEF SDK selected by the WebUI lock."""
import argparse
import hashlib
import json
from pathlib import Path
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]

def verify(archive, lock):
    if archive.stat().st_size != lock['size']:
        raise ValueError('CEF archive size mismatch')
    with archive.open('rb') as stream:
        if hashlib.file_digest(stream, 'sha256').hexdigest() != lock['sha256']:
            raise ValueError('CEF archive SHA-256 mismatch')

def extract(archive, destination, lock):
    name = lock['archive'].removesuffix('.tar.bz2')
    with tarfile.open(archive, 'r:bz2') as bundle:
        members = bundle.getmembers()
        for item in members:
            parts = item.name.split('/')
            if parts[0] != name or any(p in ('', '.', '..') or ':' in p for p in parts) or '\\' in item.name:
                raise ValueError(f'Unsafe CEF archive path: {item.name}')
            if not (item.isfile() or item.isdir()):
                raise ValueError(f'Unexpected CEF archive entry: {item.name}')
        bundle.extractall(destination, members=members, filter='data')
    sdk = destination / name
    if lock['version'] not in (sdk / 'README.txt').read_text():
        raise ValueError('CEF SDK version does not match lock')
    return sdk

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--archive', type=Path, help='Verify an existing archive instead of downloading')
    args = p.parse_args()
    lock = json.loads((ROOT / 'engine/webui/cef.lock.json').read_text())
    args.output.mkdir(parents=True, exist_ok=False)
    archive = args.archive or args.output / lock['archive']
    if not args.archive:
        request = urllib.request.Request(lock['url'], headers={'User-Agent':'Q3Rally-WebUI-build'})
        with urllib.request.urlopen(request, timeout=120) as src, archive.open('wb') as dst:
            while block := src.read(1024 * 1024): dst.write(block)
    verify(archive, lock)
    sdk = extract(archive, args.output, lock)
    (args.output / 'sdk-path.txt').write_text(str(sdk.resolve()) + '\n')
    print(sdk)

if __name__ == '__main__': main()
