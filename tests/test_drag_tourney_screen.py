"""Draw the Drag tournament bracket and podium with the production cgame code."""
from test_plate_and_menu_regressions import CODE, compile_run, function


def test_drag_tournament_bracket_and_podium():
    racing = (CODE / "cgame/cg_hud_racing.c").read_text()
    tools = (CODE / "cgame/cg_drawtools.c").read_text()
    begin = racing.index("#define DT_TOP")
    harness = r'''
#include "cgame/cg_local.h"
#include <assert.h>
#include <strings.h>
cg_t cg;
cgs_t cgs;
vec4_t colorWhite={1,1,1,1};
vmCvar_t cg_stretch;
static screenPlacement_e cg_horizontalPlacement=PLACE_LEFT, cg_verticalPlacement=PLACE_TOP;
static screenPlacement_e cg_lastHorizontalPlacement, cg_lastVerticalPlacement;
static int strings;
static char labels[128][96];
int QDECL Com_sprintf(char *dest,int size,const char *format,...) {
    int n; va_list ap; va_start(ap,format); n=vsnprintf(dest,size,format,ap); va_end(ap); return n;
}
char *Q_CleanStr(char *text) { return text; }
int Q_stricmp(const char *a,const char *b) { return strcasecmp(a,b); }
void Q_strncpyz(char *dest,const char *src,int size) { assert(size>0); snprintf(dest,size,"%s",src); }
int CG_IngameStringWidth(const char *text,int style,float scale) { return (int)(strlen(text)*14*scale*0.6f); }
'''
    for name in ("CG_SetScreenPlacement", "CG_GetScreenHorizontalPlacement",
                 "CG_GetScreenVerticalPlacement", "CG_AdjustFrom640"):
        harness += function(tools, name) + "\n"
    harness += r'''
void CG_FillRect(float x,float y,float w,float h,const float *color) {
    assert(color && w>=0 && h>=0);
    CG_AdjustFrom640(&x,&y,&w,&h);
    assert(x>=-.01f && y>=-.01f && x+w<=cgs.glconfig.vidWidth+.1f && y+h<=cgs.glconfig.vidHeight+.1f);
}
void CG_DrawIngameString(int x,int y,const char *s,int style,float scale,const float *color) {
    assert(x>=0 && x<=640 && y>=0 && y<480 && scale>0 && color);
    assert(strings<128); snprintf(labels[strings++],96,"%s",s);
}
static int has(const char *label) {
    int i; for(i=0;i<strings;i++) if(!strcmp(labels[i],label)) return 1; return 0;
}
static int count(const char *label) {
    int i, n=0; for(i=0;i<strings;i++) if(!strcmp(labels[i],label)) n++; return n;
}
'''
    # the bracket's bookkeeping, shared with the v0.9 bracket
    bracket = (CODE / "cgame/cg_drag_bracket.c").read_text().replace('#include "cg_drag_bracket.h"', '')
    harness += bracket + "\n"
    harness += racing[begin:] + "\n"
    harness += r'''
static void field4(int played) {
    static const char *names[4]={"Per","Bobby","Carla","Dean"};
    int i;
    memset(&cgs.dragTourney,0,sizeof(cgs.dragTourney));
    cgs.dragTourney.size=4; cgs.dragTourney.bestOf=3; cgs.dragTourney.played=played;
    for(i=0;i<4;i++){ snprintf(cgs.dragTourney.name[i],16,"%s",names[i]); cgs.dragTourney.human[i]=i==0; }
    /* semi 0: Bobby beats Per; semi 1: Carla beats Dean */
    cgs.dragTourney.winner[0]=1; cgs.dragTourney.wins[0][0]=1; cgs.dragTourney.wins[0][1]=2;
    cgs.dragTourney.winner[1]=0; cgs.dragTourney.wins[1][0]=2; cgs.dragTourney.wins[1][1]=0;
    /* third place: Per beats Dean; final: Carla beats Bobby */
    cgs.dragTourney.winner[2]=0; cgs.dragTourney.wins[2][0]=2; cgs.dragTourney.wins[2][1]=1;
    cgs.dragTourney.winner[3]=1; cgs.dragTourney.wins[3][0]=1; cgs.dragTourney.wins[3][1]=2;
    cgs.dragTourney.current[0]=cgs.dragTourney.current[1]=-1;
}

static void draw(void) {
    int i;
    for(i=0;i<60;i++) { cg.time+=100; strings=0; CG_DrawDragTourney(); } /* past the heat results */
    strings=0;
    assert(CG_DrawDragTourney());
}

int main(void) {
    static snapshot_t snap;
    cgs.gametype=GT_DRAG; cg.snap=&snap; cg.clientNum=0;
    snprintf(cgs.clientinfo[0].name,sizeof(cgs.clientinfo[0].name),"Per");
    cgs.glconfig.vidWidth=1920; cgs.glconfig.vidHeight=1080;
    cgs.screenXScale=cgs.screenYScale=2.25f; cgs.screenXScaleStretch=3; cgs.screenYScaleStretch=2.25f;
    cgs.screenXBias=240; cgs.screenYBias=0;

    /* heat results first, then the bracket */
    field4(2); cg.time=1000;
    assert(!CG_DrawDragTourney());
    cg.time+=500; assert(!CG_DrawDragTourney());

    /* before third place: the semi-final losers are in its boxes */
    field4(2); cgs.dragTourney.current[0]=0; cgs.dragTourney.current[1]=3;
    draw();
    assert(has("THIRD PLACE") && count("Per")==2 && count("Dean")==2);
    assert(has("Bobby") && has("Carla") && has("WINNER") && has("HOLD SCOREBOARD FOR THE HEAT RESULTS"));

    /* before the final: a bot match can be skipped */
    field4(3); cgs.dragTourney.current[0]=1; cgs.dragTourney.current[1]=2;
    draw();
    assert(has("FINAL") && strings>0);
    {
        int i, skip=0;
        for(i=0;i<strings;i++) if(strstr(labels[i],"/dragskip")) skip=1;
        assert(skip);
    }

    /* after the final: the podium */
    field4(4);
    draw();
    assert(has("Carla WINS"));
    assert(has("YOU FINISH THIRD") && has("FINAL  2 : 1"));
    assert(has("Carla") && has("Bobby") && has("Per") && !has("Dean"));
    assert(has("1") && has("2") && has("3") && has("FIRE / USE: NEXT TOURNAMENT"));

    /* out in the semi-final of an eight driver field */
    {
        int i;
        memset(&cgs.dragTourney,0,sizeof(cgs.dragTourney));
        cgs.dragTourney.size=8; cgs.dragTourney.bestOf=1; cgs.dragTourney.played=8;
        for(i=0;i<8;i++) snprintf(cgs.dragTourney.name[i],16,"Bot%d",i);
        snprintf(cgs.dragTourney.name[5],16,"Per"); cgs.dragTourney.human[5]=qtrue;
        /* quarter-finals: the first of each pair wins, except Per (5) beats 4 */
        cgs.dragTourney.winner[2]=1;
        /* semi-final 1 (Per vs 6) goes to 6; Per then loses third place */
        cgs.dragTourney.winner[5]=1;
        cgs.dragTourney.winner[6]=0;   /* third: loser(4)=2 vs loser(5)=Per -> 2 */
        draw();
        assert(has("YOU FINISH FOURTH"));
        cgs.dragTourney.winner[2]=0;   /* Per out in the quarter-final */
        draw();
        assert(has("YOU WENT OUT IN THE QUARTER-FINAL"));
    }
    return 0;
}
'''
    compile_run(harness)
