from test_web_config import ROOT, extract, run_c

def test_active_web_screen_requires_explicit_input_context():
 source=(ROOT/'engine/code/client/cl_webgarage.c').read_text()
 harness=r"""
#include <assert.h>
#define KEYCATCH_CONSOLE 1
#define KEYCATCH_UI 2
#define KEYCATCH_CGAME 4
#define CA_ACTIVE 8
#define CA_CONNECTING 3
#define CA_PRIMED 7
#define CA_DISCONNECTED 1
#define UI_WEB_SCREEN 77
static int catcher,hud,cgvm=1,uivm=1,screen=12;
static struct {int state,demoPlayer;} clc;
static int Key_GetCatcher(void){return catcher;}
static int Cvar_VariableIntegerValue(const char *name){return hud;}
static int VM_Call(int vm,int command){return screen;}
"""+screen_codes(source)+extract(source,'static int Web_UIScreenRaw(')+extract(source,'static int Web_UIScreen(')+extract(source,'static int Web_CurrentScreen(')+r"""
int main(void){clc.state=CA_ACTIVE;assert(Web_CurrentScreen()==0);
catcher=KEYCATCH_UI;assert(Web_CurrentScreen()==12);
hud=1;catcher=KEYCATCH_CGAME;assert(Web_CurrentScreen()==13);
catcher|=KEYCATCH_CONSOLE;assert(Web_CurrentScreen()==0);
catcher=0;clc.state=2;assert(Web_CurrentScreen()==0);
/* connecting and loading: the native map loading screen, the browser hides */
clc.state=CA_CONNECTING;assert(Web_CurrentScreen()==0);clc.state=CA_PRIMED;assert(Web_CurrentScreen()==0);
catcher=KEYCATCH_CONSOLE;assert(Web_CurrentScreen()==0);catcher=0;
clc.state=CA_DISCONNECTED;assert(Web_CurrentScreen()==12);
clc.state=CA_ACTIVE;clc.demoPlayer=1;screen=4;assert(Web_CurrentScreen()==4);
/* the native loading screen and main menu: the browser waits hidden */
clc.state=CA_DISCONNECTED;clc.demoPlayer=0;screen=15;assert(Web_CurrentScreen()==0);}
"""
 run_c(harness)


def screen_codes(source):
 start=source.index('#define WEB_SCREEN_PRELOAD')
 return source[start:source.index('\n\n',start)]+'\n'


def test_native_screens_preload_the_browser_and_hold_their_last_frame():
 source=(ROOT/'engine/code/client/cl_webgarage.c').read_text()
 harness=r"""
#include <assert.h>
#define CA_CONNECTING 3
#define CA_PRIMED 7
#define CA_DISCONNECTED 1
#define UI_WEB_SCREEN 77
typedef int qboolean;
#define qfalse 0
#define qtrue 1
static int uivm=1,screen,visible,webChild=1;
static struct {int integer;} frontend={1},*webFrontend=&frontend;
static struct {int state,demoPlayer;} clc={CA_DISCONNECTED,0};
static int VM_Call(int vm,int command){return screen;}
static int IsWindowVisible(int window){return visible;}
"""+screen_codes(source)+extract(source,'static int Web_UIScreenRaw(')+extract(source,'static int Web_UIScreen(')+extract(source,'static qboolean Web_Preloading(')+extract(source,'qboolean CL_WebGarageOwnsScreen(')+hold_frame(source)+r"""
int main(void){
 /* a native screen: drawn by the UI while the page loads hidden */
 screen=15;assert(!CL_WebGarageOwnsScreen()&&Web_Preloading());
 assert(!CL_WebGarageHoldFrame(1000));
 /* a WebUI screen asked for: the native frame stays until the page shows */
 screen=9;assert(CL_WebGarageOwnsScreen()&&!Web_Preloading());
 assert(CL_WebGarageHoldFrame(1016)&&CL_WebGarageHoldFrame(1500));
 visible=1;assert(!CL_WebGarageHoldFrame(1516));
 visible=0;assert(!CL_WebGarageHoldFrame(1532));
 /* a browser that does not come draws black after a second */
 screen=15;assert(!CL_WebGarageHoldFrame(2000));
 screen=9;assert(CL_WebGarageHoldFrame(2500)&&!CL_WebGarageHoldFrame(3000)&&!CL_WebGarageHoldFrame(3016));
 clc.state=8;screen=15;assert(!Web_Preloading());
 screen=0;assert(!CL_WebGarageOwnsScreen());
 frontend.integer=0;screen=2;assert(!CL_WebGarageOwnsScreen());}
"""
 run_c(harness)


def hold_frame(source):
 start=source.index('#define WEB_HOLD_NATIVE_MS')
 return '\n'+source[start:source.index('\n}\n',start)+3]

def test_hud_commands_validate_mode_values_and_open_state():
 source=(ROOT/'engine/code/cgame/cg_hud_core.c').read_text()
 harness=r"""
#include <assert.h>
#define HUDOPT_NUM_ENTRIES 2
#define qfalse 0
#define qtrue 1
typedef struct {int integer;} vmCvar_t;
typedef struct {vmCvar_t *cvar;const char *cvarName;int onValue;} hudToggleEntry_t;
static vmCvar_t cg_hudOptionsOpen,cg_mmap_size,cg_mmap_fov,a,b;
static hudToggleEntry_t hudToggleTable[]={{&a,"a",1},{&b,"b",3}};
static int calls,unavailable;static float last;
static void trap_Cvar_Update(vmCvar_t *v){}
static int CG_HUDOptionsIsOpen(void){return cg_hudOptionsOpen.integer;}
static int HUDEntry_IsUnavail(const hudToggleEntry_t *entry){return unavailable;}
static const char *va(const char *format,int value){return "test";}
static void trap_Cvar_Set(const char *name,const char *value){calls++;}
static void HUDOpt_SetFloatCvar(const char *name,vmCvar_t *v,float value,int integral){calls++;last=value;}
"""+extract(source,'void CG_WebHUDAction(')+r"""
int main(void){CG_WebHUDAction(0,0,1);assert(calls==0);cg_hudOptionsOpen.integer=1;
CG_WebHUDAction(0,0,1);assert(calls==1);
CG_WebHUDAction(0,0,2);CG_WebHUDAction(0,-1,0);CG_WebHUDAction(0,2,0);assert(calls==1);
unavailable=1;CG_WebHUDAction(0,0,0);assert(calls==1);unavailable=0;
CG_WebHUDAction(1,0,40);assert(calls==2&&last>.39f&&last<.41f);
CG_WebHUDAction(1,0,201);CG_WebHUDAction(1,0,41);assert(calls==2);
CG_WebHUDAction(2,0,120);assert(calls==3&&last==120);
CG_WebHUDAction(2,0,125);CG_WebHUDAction(2,0,29);assert(calls==3);}
"""
 run_c(harness)
