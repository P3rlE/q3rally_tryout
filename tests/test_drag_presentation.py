"""Exercise production Drag map filtering and result formatting."""
import re

from test_plate_and_menu_regressions import CODE, compile_run, function


def test_drag_results_separate_reaction_elapsed_and_total():
    source = (CODE / "cgame/cg_scoreboard.c").read_text()
    columns = re.search(r"typedef enum \{\s*SBCOL_RANK,.*?\} sbColumn_t;", source, re.S).group()
    harness = r'''
#include "cgame/cg_local.h"
#include <assert.h>
void QDECL Com_Printf(const char *fmt, ...) {}
void QDECL Com_Error(int code, const char *fmt, ...) { abort(); }
float AngleNormalize180(float angle) { return angle; }
'''
    harness += columns + "\n"
    harness += function(source, "CG_DragResultText") + "\n"
    harness += function(source, "CG_DragResultStatus") + "\n"
    harness += r'''
int main(void) {
    centity_t car = {0}; char text[32];
    car.dragPhase=DRAG_FINISHED; car.dragGreenTime=10000;
    car.dragLaunchTime=10220; car.dragFinishTime=22565; car.dragTrapSpeed=2005;
    CG_DragResultText(SBCOL_DRAG_REACTION,&car,text,sizeof(text)); assert(!strcmp(text,"0.220"));
    CG_DragResultText(SBCOL_DRAG_ELAPSED,&car,text,sizeof(text)); assert(!strcmp(text,"12.345"));
    CG_DragResultText(SBCOL_TOTALTIME,&car,text,sizeof(text)); assert(!strcmp(text,"12.565"));
    CG_DragResultText(SBCOL_DRAG_TRAP,&car,text,sizeof(text)); assert(!strcmp(text,"200.5"));
    car.dragLaunchTime=10000;
    CG_DragResultText(SBCOL_DRAG_REACTION,&car,text,sizeof(text)); assert(!strcmp(text,"0.000"));
    car.dragPhase=DRAG_FALSE_START;
    CG_DragResultText(SBCOL_TOTALTIME,&car,text,sizeof(text)); assert(!strcmp(text,"-"));
    CG_DragResultText(SBCOL_DRAG_REACTION,&car,text,sizeof(text)); assert(!strcmp(text,"-"));
    CG_DragResultText(SBCOL_DRAG_TRAP,&car,text,sizeof(text)); assert(!strcmp(text,"-"));
    assert(!strcmp(CG_DragResultStatus(car.dragPhase),"FALSE START"));
    car.dragPhase=DRAG_DNF;
    CG_DragResultText(SBCOL_DRAG_ELAPSED,&car,text,sizeof(text)); assert(!strcmp(text,"-"));
    assert(!strcmp(CG_DragResultStatus(car.dragPhase),"DNF"));
    memset(&car,0,sizeof(car));
    CG_DragResultText(SBCOL_DRAG_REACTION,&car,text,sizeof(text)); assert(!strcmp(text,"-"));
    return 0;
}
'''
    compile_run(harness, shared=True)


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


def test_drag_result_headers_render_for_every_visible_metric():
    source = (CODE / "cgame/cg_scoreboard.c").read_text()
    layout = source[source.index("/* Modern scoreboard layout constants */"):
                    source.index("/*\n=================\nCG_IsRacingGametype")]
    harness = r'''
#include "cgame/cg_local.h"
#include <assert.h>
cg_t cg;
cgs_t cgs;
qboolean BG_GametypeIsRace(int mode) { return mode == GT_DRAG; }
qboolean BG_GametypeIsTimedRace(int mode) { return mode == GT_DRAG; }
int CG_RaceLapLimit(void) { return 1; }
'''
    harness += layout + "\n"
    harness += r'''
static int emitted[SBCOL_MAX];
static void CG_DrawModernBackground(int x,int y,int width,int height,float alpha,qboolean header) {}
static void CG_DrawModernText(int x,int y,const char *text,int align,int width,float *color,qboolean bold) {
    int i;
    for(i=0;i<SBCOL_MAX;i++) {
        if(columns[i].visible && !strcmp(columns[i].header,text)) {
            assert(x==columns[i].x && width==columns[i].width && bold);
            if(i!=SBCOL_NAME) assert(align==1);
            emitted[i]++;
        }
    }
}
'''
    for name in ("CG_IsRacingGametype", "CG_IsTeamGametype",
                 "CG_InitScoreboardColumns", "CG_DrawModernHeader"):
        harness += function(source, name) + "\n"
    harness += r'''
int main(void) {
    int i;
    cgs.gametype=GT_DRAG; cg.predictedPlayerState.pm_type=PM_INTERMISSION;
    CG_InitScoreboardColumns();
    assert(!strcmp(columns[SBCOL_DRAG_REACTION].header,"RT (S)"));
    assert(!strcmp(columns[SBCOL_DRAG_ELAPSED].header,"ET (S)"));
    assert(!strcmp(columns[SBCOL_DRAG_TRAP].header,"KM/H"));
    assert(!columns[SBCOL_LAPTIME].visible);
    assert(currentScoreboardWidth<=SCREEN_WIDTH);
    CG_DrawModernHeader(120,1.0f);
    for(i=0;i<SBCOL_MAX;i++) {
        if(columns[i].visible && columns[i].header[0]) assert(emitted[i]==1);
    }
    assert(emitted[SBCOL_DRAG_REACTION]==1);
    assert(emitted[SBCOL_DRAG_ELAPSED]==1);
    assert(emitted[SBCOL_DRAG_TRAP]==1);
    return 0;
}
'''
    compile_run(harness)
