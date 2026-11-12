from test_web_config import ROOT, extract, run_c


def test_only_current_local_screen_can_reveal_the_browser():
    source = (ROOT / 'engine/code/client/cl_webgarage.c').read_text()
    harness = r"""
#include <assert.h>
#include <wchar.h>
#define CEF_CALLBACK
#define qtrue 1
#define qfalse 0
typedef int cef_display_handler_t;
typedef struct { const wchar_t *str; } cef_string_t;
typedef struct { struct { void (*release)(void *); } base; } cef_frame_t;
typedef struct browser { cef_frame_t *(*get_main_frame)(struct browser *); } cef_browser_t;
static int local=1,current=12,webPresentedScreen,releases;
static cef_frame_t frame;
static void release(void *ignored){releases++;}
static cef_frame_t *get_frame(cef_browser_t *browser){return &frame;}
static int Web_IsLocalFrame(cef_frame_t *ignored){return local;}
static int Web_CurrentScreen(void){return current;}
""" + extract(source, 'static void CEF_CALLBACK Web_TitleChanged(') + r"""
int main(void){
    cef_browser_t browser={get_frame};cef_string_t title={L"Q3Rally screen 12"};
    frame.base.release=release;
    Web_TitleChanged(0,&browser,&title);assert(webPresentedScreen==12&&releases==1);
    webPresentedScreen=0;current=7;
    Web_TitleChanged(0,&browser,&title);assert(!webPresentedScreen);
    current=12;local=0;
    Web_TitleChanged(0,&browser,&title);assert(!webPresentedScreen);
    local=1;title.str=L"Q3Rally | Garage";
    Web_TitleChanged(0,&browser,&title);assert(!webPresentedScreen&&releases==4);
}
"""
    run_c(harness)



def test_opening_escape_does_not_reach_the_new_pause_page():
    source=(ROOT/'engine/code/client/cl_webgarage.c').read_text()
    harness=r"""
#include <assert.h>
#define CEF_CALLBACK
#define VK_ESCAPE 27
#define KEYEVENT_KEYUP 2
#define qfalse 0
typedef int cef_keyboard_handler_t;
typedef int cef_browser_t;
typedef int cef_event_handle_t;
typedef struct { int windows_key_code,type; } cef_key_event_t;
static int webOpeningEscape;
static struct {int integer;} *webTiming;
static int Web_CurrentScreen(void){return 12;}
static int Web_Visible(void){return 1;}
static void Com_Printf(const char *format,...){}
"""+extract(source,'static int CEF_CALLBACK Web_PreKey(')+r"""
int main(void){
    cef_key_event_t event={27,0};
    webOpeningEscape=1;
    assert(Web_PreKey(0,0,&event,0,0)==1);
    assert(Web_PreKey(0,0,&event,0,0)==1);
    event.windows_key_code=65;assert(Web_PreKey(0,0,&event,0,0)==0);
    event.windows_key_code=27;event.type=KEYEVENT_KEYUP;
    assert(Web_PreKey(0,0,&event,0,0)==1&&!webOpeningEscape);
    event.type=0;assert(Web_PreKey(0,0,&event,0,0)==0);
}
"""
    run_c(harness)


def test_web_owned_screen_ticks_without_drawing_native_menu():
    source=(ROOT/'engine/code/client/cl_scrn.c').read_text()
    harness=r"""
#include <assert.h>
#define USE_CEF_GARAGE
#define UI_WEB_TICK 1
#define UI_REFRESH 2
static int owned,command,uivm=1;
static struct {int realtime;} cls;
static int CL_WebGarageOwnsScreen(void){return owned;}
static void VM_Call(int vm,int call,int time){assert(vm==1&&time==123);command=call;}
"""+extract(source,'static void SCR_RefreshUI(')+r"""
int main(void){
 cls.realtime=123;owned=1;SCR_RefreshUI();assert(command==UI_WEB_TICK);
 owned=0;SCR_RefreshUI();assert(command==UI_REFRESH);
}
"""
    run_c(harness)



def test_native_render_ownership_does_not_depend_on_browser_visibility():
    source=(ROOT/'engine/code/client/cl_webgarage.c').read_text()
    harness=r"""
#include <assert.h>
#define qfalse 0
#define CA_CONNECTING 3
#define CA_PRIMED 7
#define UI_WEB_SCREEN 77
typedef int qboolean;
static struct {int integer;} frontend;
static struct {int state,demoPlayer;} clc;
static struct {int integer;} *webFrontend;
static int uivm=1,screen;
static int VM_Call(int vm,int command){return screen;}
""".replace('static struct {int integer;} *webFrontend;','static __typeof__(frontend) *webFrontend;')+extract(source,'qboolean CL_WebGarageOwnsScreen(')+r"""
int main(void){
 webFrontend=&frontend;frontend.integer=1;screen=12;clc.state=8;
 assert(CL_WebGarageOwnsScreen());
 screen=0;assert(!CL_WebGarageOwnsScreen());
 clc.state=CA_CONNECTING;assert(CL_WebGarageOwnsScreen());
 frontend.integer=0;assert(!CL_WebGarageOwnsScreen());
 frontend.integer=1;uivm=0;assert(!CL_WebGarageOwnsScreen());
}
"""
    run_c(harness)
