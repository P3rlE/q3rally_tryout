import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]


def test_web_main_finishes_native_transition_before_opening_submenu():
    source = (ROOT / "engine/code/q3_ui/ui_menu.c").read_text()
    function = source[source.index("void UI_WebMainAction("):]
    harness = r'''
#include <assert.h>
#define qtrue 1
#define qfalse 0
enum { ID_SINGLEPLAYER=1, ID_MULTIPLAYER, ID_SETUP, ID_RIVALS, ID_DEMOS, ID_DOWNLOADS };
static struct { int transitionIn, transitionOut; float tFrac; } uis;
static int screen=1, opened, finished;
static int UI_WebMainScreen(void) { return screen; }
static void MainMenu_RunTransition(float value) { assert(value==1.0f); finished++; }
static void ready(void) { assert(!uis.transitionIn && !uis.transitionOut && uis.tFrac==1.0f); }
static void MainMenu_ChangeMenu(int id) { ready(); opened=id; }
static void UI_WebDriverMenu(void) { ready(); opened=7; }
static void UI_ProfileOverlay_Open(int ignored) { ready(); opened=7; }
static int UI_Profile_HasActiveProfile(void) { return 1; }
static void UI_PlayerStatsMenu(void) { ready(); opened=8; }
static void MainMenu_ExitAction(int accepted) { ready(); opened=accepted ? 13 : 0; }
static void UI_ConfirmMenu(const char *text,int ignored,void (*callback)(int)) { ready(); opened=9; }
'''
    harness += function
    harness += r'''
int main(void) {
    int action;
    for(action=1;action<=8;action++) {
        uis.transitionIn=1234; uis.transitionOut=4321; uis.tFrac=0.25f;
        UI_WebMainAction(action);
        assert(opened==action);
    }
    assert(finished==8);
    screen=0; uis.transitionIn=1234;
    UI_WebMainAction(3);
    assert(uis.transitionIn==1234 && finished==8);
}
'''
    compiler = shutil.which("gcc") or "C:/msys64/mingw64/bin/gcc.exe"
    with tempfile.TemporaryDirectory(dir=ROOT / "tests") as directory:
        sourcefile = pathlib.Path(directory) / "handoff.c"
        binary = pathlib.Path(directory) / "handoff.exe"
        sourcefile.write_text(harness)
        subprocess.run([compiler, str(sourcefile), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=5)
