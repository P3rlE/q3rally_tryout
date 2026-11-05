import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]


def test_download_catalog_publishes_after_parsing_without_status_change():
    source = (ROOT / "engine/code/client/cl_webgarage.c").read_text()
    start = source.index("        if ( screen == 6 && cls.realtime - webLastDownloadPoll")
    end = source.index("        if ( screen == 6 && cls.realtime - webLastStatusReport", start)
    logic = source[start:end]
    mime_start = source.index("static const wchar_t *Web_ResourceMime(")
    mime_end = source.index("\nstatic cef_resource_handler_t", mime_start)
    harness = r'''
#include <assert.h>
#include <stdarg.h>
#include <wchar.h>
#define UI_WEB_DOWNLOAD_TICK 1
#define UI_WEB_CATALOG_NUMBER 2
static struct { int realtime; } cls;
static int uivm=1, webLastDownloadPoll, webCatalogScreen=6;
static int webCatalogState=2, webCatalogRevision, revision, state=2, published;
static int Cvar_VariableIntegerValue(const char *key) { return state; }
static int VM_Call(int vm,int command,...) {
    if(command==UI_WEB_DOWNLOAD_TICK) { revision=1; return 0; }
    assert(command==UI_WEB_CATALOG_NUMBER); return revision;
}
static void Web_PublishCatalog(int screen) {
    assert(screen==6); published++; webCatalogRevision=revision; webCatalogState=state;
}
static void frame(void) {
    int screen=6;
'''
    harness += logic + "\n}\n" + source[mime_start:mime_end]
    harness += r'''
int main(void) {
    cls.realtime=1000; frame();
    assert(published==1 && state==2 && webCatalogRevision==1);
    cls.realtime+=100; frame();
    assert(published==1);
    state=4; cls.realtime+=100; frame();
    assert(published==2);
    assert(!wcscmp(Web_ResourceMime(L"download-preview/tracks/map.jpg"),L"image/jpeg"));
    assert(!wcscmp(Web_ResourceMime(L"download-preview/tracks/map.jpeg"),L"image/jpeg"));
    assert(!wcscmp(Web_ResourceMime(L"download-preview/tracks/map.png"),L"image/png"));
    assert(!wcscmp(Web_ResourceMime(L"download-preview/tracks/map.webp"),L"image/webp"));
    assert(!Web_ResourceMime(L"download-preview/tracks/map.exe"));
}
'''
    compiler = shutil.which("gcc") or "C:/msys64/mingw64/bin/gcc.exe"
    with tempfile.TemporaryDirectory(dir=ROOT / "tests") as directory:
        sourcefile = pathlib.Path(directory) / "downloads.c"
        binary = pathlib.Path(directory) / "downloads.exe"
        sourcefile.write_text(harness)
        subprocess.run([compiler, str(sourcefile), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=5)
