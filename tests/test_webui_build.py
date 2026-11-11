"""Build-boundary tests: tampering, archive traversal, provenance and runtime closure."""
import json
from pathlib import Path
import tarfile
import io
import zipfile
import pytest
from tools import build_webui_package as build
from tools.fetch_cef import verify, extract as extract_cef
from tools.validate_webui_package import extract as extract_package
from build_showroom import extract_assets

SHA='a'*40

def put(root,name,data=b'x'):
    p=root/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data);return p

def sdk(root):
    lock=json.loads((build.ROOT/'engine/webui/cef.lock.json').read_text())
    put(root,'README.txt',lock['version'].encode());put(root,'LICENSE.txt',b'CEF license')
    for name in ('libcef.dll','chrome_elf.dll','v8_context_snapshot.bin','libEGL.dll'):
        put(root,'Release/'+name)
    put(root,'Release/libcef.lib',b'DO NOT SHIP SDK')
    put(root,'Release/bootstrap.exe',b'DO NOT SHIP EXAMPLES')
    for name in ('icudtl.dat','resources.pak','chrome_100_percent.pak','chrome_200_percent.pak','locales/en-US.pak'):
        put(root,'Resources/'+name)
    return root

def test_common_manifest_rejects_modified_extra_or_wrong_revision(tmp_path):
    put(tmp_path,'baseq3r/pak0.pk3',b'canonical bytes')
    build.write_json(tmp_path/'common-manifest.json',{'revision':SHA,'version':build.VERSION,'files':build.hashes(tmp_path)})
    assert build.verify_common(tmp_path,SHA)['revision']==SHA
    with pytest.raises(ValueError):build.verify_common(tmp_path,'b'*40)
    put(tmp_path,'extra.txt')
    with pytest.raises(ValueError):build.verify_common(tmp_path,SHA)
    (tmp_path/'extra.txt').unlink();put(tmp_path,'baseq3r/pak0.pk3',b'changed')
    with pytest.raises(ValueError):build.verify_common(tmp_path,SHA)

def test_cef_runtime_includes_resources_and_license_but_not_sdk(tmp_path):
    source=sdk(tmp_path/'sdk');target=tmp_path/'package';(target/'licenses').mkdir(parents=True)
    build.copy_cef(source,target)
    assert (target/'locales/en-US.pak').is_file()
    assert (target/'licenses/CEF-LICENSE.txt').read_bytes()==b'CEF license'
    assert not (target/'libcef.lib').exists()
    assert not (target/'bootstrap.exe').exists()
    (source/'Resources/icudtl.dat').unlink()
    with pytest.raises(ValueError,match='icudtl'):build.copy_cef(source,target)

def test_cef_archive_digest_checked_before_extract(tmp_path):
    p=put(tmp_path,'bad.tar.bz2',b'corrupt')
    with pytest.raises(ValueError,match='SHA-256'):verify(p,{'size':7,'sha256':'0'*64})

def test_asset_export_rejects_traversal_and_symlinks(tmp_path):
    for name,mode in [('models/players/../../escape',0),('models/players/link',0o120777)]:
        p=tmp_path/'assets.pk3'
        with zipfile.ZipFile(p,'w') as z:
            item=zipfile.ZipInfo(name);item.external_attr=mode<<16;z.writestr(item,b'x')
        with pytest.raises(ValueError):extract_assets(p,tmp_path/'out')
    assert not (tmp_path/'escape').exists()

def test_test_package_extraction_rejects_traversal(tmp_path):
    p=tmp_path/'bad.zip'
    with zipfile.ZipFile(p,'w') as z:z.writestr('game/../escape',b'x')
    with pytest.raises(ValueError):extract_package(p,tmp_path/'out')

def test_cef_extraction_rejects_links_before_writing(tmp_path):
    p=tmp_path/'cef.tar.bz2'
    with tarfile.open(p,'w:bz2') as z:
        link=tarfile.TarInfo('sdk/link');link.type=tarfile.SYMTYPE;link.linkname='/etc'
        z.addfile(link)
    with pytest.raises(ValueError):extract_cef(p,tmp_path/'out',{'archive':'sdk.tar.bz2'})

def test_package_uses_canonical_qvms_and_rejects_version_mismatch(tmp_path):
    shared=tmp_path/'common';put(shared,'baseq3r/pak0.pk3',b'canonical assets')
    qvms=shared/'baseq3r/qvms.pk3'
    with zipfile.ZipFile(qvms,'w') as z:
        for name in build.QVMS:z.writestr('vm/'+name,b'QVM canonical '+name.encode())
    build.write_json(shared/'common-manifest.json',{'schema':1,'revision':SHA,'version':build.VERSION,'source_date_epoch':1,'files':build.hashes(shared)})
    native=tmp_path/'native'
    for name in ('q3rally.x86_64.exe','q3rally-server.x86_64.exe','renderer_opengl1_x86_64.dll','renderer_opengl2_x86_64.dll','SDL2.dll','CURL-COPYING.txt'):
        put(native,name,('Q3Rally v'+build.VERSION+'\0 cl_webUI').encode())
    source=sdk(tmp_path/'sdk')
    target=build.package(shared,native,source,tmp_path/'output',SHA)
    assert (target/'baseq3r/qvms.pk3').read_bytes()==qvms.read_bytes()
    assert not (target/'baseq3r/vm').exists()
    assert (target/'baseq3r/pak0.pk3').read_bytes()==b'canonical assets'
    assert 'LOCALAPPDATA' in (target/'Start-WebUI.cmd').read_text()
    assert '+set cl_webUI 1 +set r_fullscreen 1 +set r_mode -2' in (target/'Start-WebUI.cmd').read_text()
    assert '+set cl_webUI 0 +set r_fullscreen 1 +set r_mode -2' in (target/'Start-Native.cmd').read_text()
    assert '+set cl_webUI 1 +set r_fullscreen 0 +set r_mode -1' in (target/'Start-Windowed.cmd').read_text()
    put(native,'q3rally.x86_64.exe',b'Q3Rally v0.8.4\0 cl_webUI')
    with pytest.raises(ValueError,match='version'):build.package(shared,native,source,tmp_path/'wrong',SHA)
