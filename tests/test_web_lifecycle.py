from test_web_config import ROOT, extract, run_c


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
