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
