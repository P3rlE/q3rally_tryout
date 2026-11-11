from test_web_config import ROOT, extract, run_c
from tools.validate_webui_package import legacy_gdi_only


def test_only_known_gdi_11_limits_skip_the_gl2_menu_probe():
    assert legacy_gdi_only('GL_RENDERER: GDI Generic\nGL_VERSION: 1.1.0\n')
    assert not legacy_gdi_only('GL_RENDERER: Intel\nGL_VERSION: 3.2.0\n')
    assert not legacy_gdi_only('GL_RENDERER: GDI Generic\nGL_VERSION: 3.2.0\n')
    assert not legacy_gdi_only('GL_RENDERER: Intel\nGL_VERSION: 1.1.0\n')
    assert not legacy_gdi_only('GL_RENDERER: GDI Generic\n')


def test_video_restart_drains_browser_before_releasing_restart_guard():
    source=(ROOT/'engine/code/client/cl_webgarage.c').read_text()
    harness=r'''
#include <assert.h>
#define qtrue 1
#define qfalse 0
static int webInitialized,webVideoRestart,webOpenRequested=1,webCloseRequested=1;
static void *webBrowser=(void*)1;
static int closes,pumps,sleeps;
static void Web_Close(void){assert(webVideoRestart);closes++;}
static void cef_do_message_loop_work(void){assert(webVideoRestart);if(++pumps==2)webBrowser=0;}
static void Sleep(int ms){assert(ms==1);sleeps++;}
static void Com_Printf(const char *text){}
'''+extract(source,'void CL_WebGarageBeforeVideoRestart(')+r'''
int main(void){
CL_WebGarageBeforeVideoRestart();assert(!closes&&!pumps&&webOpenRequested);
webInitialized=1;CL_WebGarageBeforeVideoRestart();
assert(closes==1&&pumps==2&&sleeps==2&&!webBrowser);
assert(!webVideoRestart&&!webOpenRequested&&!webCloseRequested);
CL_WebGarageBeforeVideoRestart();assert(pumps==2&&!webVideoRestart);
}
'''
    run_c(harness)


def test_local_menu_test_endpoint_is_explicit_and_port_bounded():
    source=(ROOT/'engine/code/client/cl_webgarage.c').read_text()
    harness=r'''
#include <assert.h>
#include <string.h>
typedef unsigned long DWORD;
static const char *value="";
static DWORD GetEnvironmentVariableA(const char *name,char *out,DWORD capacity){
assert(!strcmp(name,"Q3RALLY_WEBUI_TEST_CDP_PORT"));
if(strlen(value)<capacity)strcpy(out,value);return strlen(value);}
'''+extract(source,'static int Web_TestDebugPort(')+r'''
int main(void){assert(!Web_TestDebugPort());
value="1023";assert(!Web_TestDebugPort());value="65536";assert(!Web_TestDebugPort());
value="1234x";assert(!Web_TestDebugPort());value="123456789";assert(!Web_TestDebugPort());
value="60000";assert(Web_TestDebugPort()==60000);
}
'''
    run_c(harness)


def test_restart_close_clears_keys_without_calling_sdl_focus_or_cursor():
    source=(ROOT/'engine/code/client/cl_webgarage.c').read_text()
    harness=r'''
#include <assert.h>
#define CEF_CALLBACK
#define qfalse 0
#define SDL_DISABLE 0
typedef void cef_life_span_handler_t;
typedef int SDL_Window;
typedef struct {struct {void(*release)(void*);}base;}cef_browser_t;
static void release(void *p){}
static cef_browser_t browser={{release}},*webBrowser=&browser;
static int webChild,webParent=1,webVideoRestart=1,keys,focus,cursor;
static int webDemoRectValid,webDemoRegion,webHUDPublished,webHUDRectValid,webHUDRegion,webHUDAction;
static int webConfigRead,webConfigWrite;
static void Web_PrintTiming(void){}
static int IsWindow(int window){return 1;}
static SDL_Window *IN_GetWindow(void){static SDL_Window window;return &window;}
static void SetFocus(int window){focus++;}
static void SDL_RaiseWindow(SDL_Window *window){focus++;}
static void SDL_SetWindowInputFocus(SDL_Window *window){focus++;}
static void Key_ClearStates(void){keys++;}
static void Cvar_Set(const char *name,const char *value){}
static void SDL_ShowCursor(int state){cursor++;}
'''+extract(source,'static void CEF_CALLBACK Web_BeforeClose(')+r'''
int main(void){
Web_BeforeClose(0,0);assert(keys==1&&!focus&&!cursor&&!webBrowser&&!webParent);
webVideoRestart=0;webParent=1;webBrowser=&browser;Web_BeforeClose(0,0);
assert(keys==2&&focus==3&&cursor==1&&!webBrowser);
}
'''
    run_c(harness)
