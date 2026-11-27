"""Exercise production Drag map filtering."""
import re

from test_plate_and_menu_regressions import CODE, compile_run, function


def test_drag_maps_require_their_own_arena_type():
    source = (CODE / "q3_ui/ui_rally_startserver.c").read_text()
    names = re.search(r"static const struct \{\s*const char\* name;.*?\} gametype_bitnames\[\] = \{.*?\n\};", source, re.S).group()
    harness = r'''
#include "q3_ui/ui_local.h"
#include <assert.h>
void QDECL Com_Printf(const char *fmt, ...) {}
void QDECL Com_Error(int code, const char *fmt, ...) { abort(); }
float AngleNormalize180(float angle) { return angle; }
'''
    harness += names + "\n" + function(source, "GametypeBits") + "\n"
    harness += r'''
int main(void) {
    assert(GametypeBits("q3r_drag") == (1 << GT_DRAG));
    assert(GametypeBits("drag") == (1 << GT_DRAG));
    assert(!(GametypeBits("q3r_sprint q3r_racing") & (1 << GT_DRAG)));
    assert(!(GametypeBits("q3r_drag") & ((1 << GT_SPRINT) | (1 << GT_RACING))));
    return 0;
}
'''
    compile_run(harness, shared=True)
