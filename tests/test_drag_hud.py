"""Exercise Drag guidance, authoritative timing and the actual HUD draw calls."""
from test_plate_and_menu_regressions import CODE, compile_run, function


def test_drag_hud_guidance_timing_and_rendering():
    racing = (CODE / "cgame/cg_hud_racing.c").read_text()
    tools = (CODE / "cgame/cg_drawtools.c").read_text()
    units = (CODE / "cgame/cg_rally_tools.c").read_text()
    begin = racing.index("/* Drag Race telemetry.")
    end = racing.index("/* End Drag Race telemetry. */", begin)
    harness = r'''
#include "cgame/cg_local.h"
#include <assert.h>
cg_t cg;
cgs_t cgs;
centity_t cg_entities[MAX_GENTITIES];
vmCvar_t cg_drawStatus, cg_hudShowTimes, cg_metricUnits, cg_stretch;
vec4_t colorWhite={1,1,1,1};
float CP_M_2_QU=35.6613f;
static screenPlacement_e cg_horizontalPlacement=PLACE_LEFT, cg_verticalPlacement=PLACE_TOP;
static screenPlacement_e cg_lastHorizontalPlacement, cg_lastVerticalPlacement;
static int strings, rectangles;
static char labels[64][64];
static int labelY[64];
int QDECL Com_sprintf(char *dest,int size,const char *format,...) {
    int n; va_list ap; va_start(ap,format); n=vsnprintf(dest,size,format,ap); va_end(ap); return n;
}
void Q_strncpyz(char *dest,const char *src,int size) {
    assert(size>0); snprintf(dest,size,"%s",src);
}
'''
    for name in ("CG_SetScreenPlacement", "CG_GetScreenHorizontalPlacement",
                 "CG_GetScreenVerticalPlacement", "CG_AdjustFrom640"):
        harness += function(tools, name) + "\n"
    harness += function(units, "Q3VelocityToRL") + "\n"
    harness += r'''
void CG_FillRect(float x,float y,float w,float h,const float *color) {
    assert(color && w>=0 && h>=0);
    CG_AdjustFrom640(&x,&y,&w,&h);
    assert(x>=-.01f && y>=-.01f && x+w<=cgs.glconfig.vidWidth+.1f && y+h<=cgs.glconfig.vidHeight+.1f);
    rectangles++;
}
void CG_DrawIngameString(int x,int y,const char *s,int style,float scale,const float *color) {
    assert(x>=0 && x<=640 && y>=0 && y<480 && scale>0 && color);
    assert(strings<64); snprintf(labels[strings],64,"%s",s); labelY[strings++]=y;
}
static int hasLabel(const char *label) {
    int i; for(i=0;i<strings;i++) if(!strcmp(labels[i],label)) return 1; return 0;
}
static void resolution(int width,int height) {
    float scale=width/640.0f;
    if(height/480.0f<scale) scale=height/480.0f;
    cgs.glconfig.vidWidth=width; cgs.glconfig.vidHeight=height;
    cgs.screenXScale=cgs.screenYScale=scale;
    cgs.screenXScaleStretch=width/640.0f; cgs.screenYScaleStretch=height/480.0f;
    cgs.screenXBias=(width-640*scale)*.5f; cgs.screenYBias=(height-480*scale)*.5f;
}
'''
    harness += racing[begin:end] + "\n"
    harness += r'''
int main(void) {
    static snapshot_t snap;
    cgDragHudState_t state;
    centity_t *car=&cg_entities[0];
    playerState_t *ps=&cg.predictedPlayerState;
    int phase,i;
    const int widths[]={640,960,1280,1920,800};
    const int heights[]={480,720,720,1080,600};
    cg.snap=&snap; cg.validPPS=qtrue; ps->clientNum=snap.ps.clientNum=0;
    cgs.gametype=GT_DRAG; cg_drawStatus.integer=1; cg_hudShowTimes.integer=1;
    cg_metricUnits.integer=1;
    assert(CG_DragHudLaunchBand(DRAG_HUD_LAUNCH_RPM_MIN-1)==-1);
    assert(CG_DragHudLaunchBand(DRAG_HUD_LAUNCH_RPM_MIN)==0);
    assert(CG_DragHudLaunchBand(DRAG_HUD_LAUNCH_RPM_MAX)==0);
    assert(CG_DragHudLaunchBand(DRAG_HUD_LAUNCH_RPM_MAX+1)==1);
    assert(CG_DragHudRpmFraction(-1)==0 && CG_DragHudRpmFraction(CP_RPM_MAX+1)==1);

    car->dragPhase=DRAG_COUNTDOWN; car->dragGreenTime=5000;
    ps->stats[STAT_GEAR]=0; ps->stats[STAT_RPM]=4500;
    CG_BuildDragHudState(ps,car,4999,&state);
    assert(state.launchGuide && state.launchBand==0);
    assert(!strcmp(state.hintText,"HOLD REVS") && !strcmp(state.statusText,"WAIT FOR GREEN"));
    assert(state.reactionMs==-1 && state.elapsedMs==-1 && state.totalMs==-1);
    CG_BuildDragHudState(ps,car,5000,&state);
    assert(!strcmp(state.statusText,"SELECT FIRST"));
    ps->stats[STAT_RPM]=1000; CG_BuildDragHudState(ps,car,4000,&state);
    assert(!strcmp(state.hintText,"BUILD REVS"));
    ps->stats[STAT_RPM]=6000; CG_BuildDragHudState(ps,car,4000,&state);
    assert(!strcmp(state.hintText,"EASE OFF"));
    ps->stats[STAT_GEAR]=1; CG_BuildDragHudState(ps,car,5010,&state);
    assert(!state.launchGuide && !strcmp(state.statusText,"LAUNCHING"));

    car->dragPhase=DRAG_RUNNING; car->dragLaunchTime=5123;
    CG_BuildDragHudState(ps,car,6000,&state);
    assert(state.reactionMs==123 && state.elapsedMs==877 && state.totalMs==1000);
    assert(!state.launchGuide);
    car->dragPhase=DRAG_FINISHED; car->dragFinishTime=18000;
    CG_BuildDragHudState(ps,car,25000,&state);
    assert(state.reactionMs==123 && state.elapsedMs==12877 && state.totalMs==13000);
    for(phase=DRAG_FALSE_START;phase<=DRAG_DNF;phase++) {
        car->dragPhase=phase; CG_BuildDragHudState(ps,car,25000,&state);
        assert(!state.launchGuide && state.reactionMs==-1 && state.elapsedMs==-1 && state.totalMs==-1);
    }

    car->dragPhase=DRAG_COUNTDOWN; car->dragLaunchTime=car->dragFinishTime=0;
    ps->stats[STAT_GEAR]=0; ps->stats[STAT_RPM]=4500;
    snap.ps.stats[STAT_RPM]=1000; /* The gauge must use responsive predicted RPM. */
    cg.time=4000;
    for(i=0;i<5;i++) {
        resolution(widths[i],heights[i]); strings=rectangles=0;
        CG_SetScreenPlacement(PLACE_LEFT,PLACE_TOP);
        CG_DrawDragTelemetryHud();
        assert(rectangles>10 && hasLabel("4500") && hasLabel("HOLD REVS"));
        assert(hasLabel("WAIT FOR GREEN") && !hasLabel("SELECT FIRST"));
        assert(hasLabel("4000-5000 RPM") && hasLabel("GEAR") && hasLabel("N") && hasLabel("KM/H"));
        assert(hasLabel("RT") && hasLabel("ET") && hasLabel("TOTAL"));
        assert(!hasLabel("NOS") && !hasLabel("FUEL") && !hasLabel("ARMOR") && !hasLabel("INTEGRITY"));
        assert(CG_GetScreenHorizontalPlacement()==PLACE_LEFT && CG_GetScreenVerticalPlacement()==PLACE_TOP);
    }
    cg_metricUnits.integer=0; cg_hudShowTimes.integer=0; strings=rectangles=0;
    CG_DrawDragTelemetryHud();
    assert(hasLabel("MPH") && !hasLabel("RT") && !hasLabel("ET") && !hasLabel("TOTAL"));
    car->dragPhase=DRAG_RUNNING; car->dragLaunchTime=5123; cg.time=6000;
    ps->stats[STAT_GEAR]=2; strings=rectangles=0;
    CG_DrawDragTelemetryHud();
    assert(!hasLabel("4000-5000 RPM") && !hasLabel("HOLD REVS") && hasLabel("RUNNING"));
    cg_drawStatus.integer=0; strings=rectangles=0; CG_DrawDragTelemetryHud();
    assert(!strings && !rectangles);
    cg_drawStatus.integer=1; cgs.gametype=GT_RACING; CG_DrawDragTelemetryHud();
    assert(!strings && !rectangles);
    return 0;
}
'''
    compile_run(harness)
