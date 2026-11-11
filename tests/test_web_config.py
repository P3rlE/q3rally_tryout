import pathlib,shutil,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def extract(source,signature):
 start=source.index(signature);pos=source.index('{',start)+1;depth=1
 while depth:depth+=(source[pos]=='{')-(source[pos]=='}');pos+=1
 return source[start:pos]
def run_c(harness):
 with tempfile.TemporaryDirectory(dir=ROOT/'tests') as directory:
  c=pathlib.Path(directory)/'config.c';exe=c.with_suffix('.exe');c.write_text(harness)
  subprocess.run([shutil.which('gcc') or 'C:/msys64/mingw64/bin/gcc.exe',str(c),'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True,timeout=5)
def test_config_bridge_validates_values_and_invokes_native_callbacks():
 source=(ROOT/'engine/code/q3_ui/ui_setup.c').read_text()
 harness=r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
#define Q_strncpyz(dest,src,size) snprintf(dest,size,"%s",src)
#define MTYPE_PTEXT 1
#define MTYPE_SPINCONTROL 2
#define MTYPE_RADIOBUTTON 3
#define MTYPE_SLIDER 4
#define MTYPE_ACTION 5
#define MTYPE_FIELD 6
#define MTYPE_BITMAP 7
#define MTYPE_SCROLLLIST 8
#define QMF_HIDDEN 1
#define QMF_INACTIVE 2
#define QMF_GRAYED 4
#define QM_ACTIVATED 1
#define K_ESCAPE 27
#define ID_CUSTOMIZEPLAYER 10
typedef struct {int type,flags,id;void(*callback)(void*,int);} menucommon_s;
typedef struct {menucommon_s generic;int curvalue;} menulist_s;
typedef menulist_s menuradiobutton_s;
typedef struct {menucommon_s generic;float minvalue,maxvalue,curvalue;} menuslider_s;
typedef struct {menucommon_s generic;struct {char buffer[64];int maxchars,cursor,scroll;}field;}menufield_s;
typedef struct {int(*key)(int);} menuframework_s;
static struct{int transitionIn,transitionOut;}uis;
static int page=2,calls,back,bind,cleared,driver,search;
static menuframework_s menu;static void *current;
static menucommon_s *WebConfig_Item(int index,int *out){*out=page;return index==0?current:0;}
static menuframework_s *WebConfig_Menu(int *out){*out=page;return &menu;}
static int WebConfig_Choices(menucommon_s *item){return 3;}
static void callback(void *item,int event){assert(event==1);calls++;}
static void UI_WebDriverMenu(void){driver++;}
static void UI_WebProfileWizardBack(void){back+=10;}
static int Menu_DefaultKey(menuframework_s *menu,int key){assert(key==27);back++;return 0;}
static void UI_WebControlsBind(int index,int key){bind++;if(key==-1)cleared++;}
static void trap_Cvar_VariableStringBuffer(const char *key,char *out,int size){strcpy(out,"accelerate");}
static void UI_WebControlsSearch(const char *text){assert(!strcmp(text,"accelerate"));search++;}
'''
 harness+=extract(source,'void UI_WebConfigAction(')
 harness+=r'''
int main(void){
menulist_s list={{MTYPE_SPINCONTROL,0,0,callback},0};menuslider_s slider={{MTYPE_SLIDER,0,0,callback},0.05f,0.75f,0.1f};menucommon_s button={MTYPE_PTEXT,0,0,callback};
current=&list;UI_WebConfigAction(1,0,2);assert(list.curvalue==2&&calls==1);UI_WebConfigAction(1,0,3);assert(list.curvalue==2&&calls==1);
list.generic.flags=QMF_GRAYED;UI_WebConfigAction(1,0,1);assert(calls==1);list.generic.flags=0;
current=&slider;UI_WebConfigAction(2,0,75);assert(slider.curvalue==0.75f&&calls==2);UI_WebConfigAction(2,0,76);assert(calls==2);UI_WebConfigAction(2,0,-1);assert(calls==2);
current=&button;UI_WebConfigAction(0,0,0);assert(calls==3);page=0;button.id=ID_CUSTOMIZEPLAYER;UI_WebConfigAction(0,0,0);assert(driver==1&&calls==3);
button.type=MTYPE_ACTION;page=1;UI_WebConfigAction(3,0,100);UI_WebConfigAction(5,0,0);assert(bind==2&&cleared==1);
button.type=MTYPE_FIELD;UI_WebConfigAction(4,0,0);assert(search==1);UI_WebConfigAction(6,0,0);assert(back==1);
UI_WebConfigAction(0,99,0);assert(calls==3);
button.type=MTYPE_BITMAP;UI_WebConfigAction(0,0,0);assert(calls==4);
current=&list;page=14;list.generic.type=MTYPE_SCROLLLIST;UI_WebConfigAction(7,0,1);assert(list.curvalue==1&&calls==5);
UI_WebConfigAction(7,0,3);assert(calls==5);page=2;UI_WebConfigAction(7,0,0);assert(calls==5);
{menufield_s field={{MTYPE_FIELD,0,0,0},{{0},5,0,0}};current=&field;page=17;
UI_WebConfigAction(4,0,0);assert(!strcmp(field.field.buffer,"accel")&&field.field.cursor==5);
field.generic.flags=QMF_INACTIVE;UI_WebConfigAction(4,0,0);assert(!strcmp(field.field.buffer,"accel"));
UI_WebConfigAction(6,0,0);assert(back==11);
page=2;field.generic.flags=0;field.field.buffer[0]=0;UI_WebConfigAction(4,0,0);assert(!field.field.buffer[0]);
page=21;UI_WebConfigAction(4,0,0);assert(!strcmp(field.field.buffer,"accel"));UI_WebConfigAction(6,0,0);assert(back==12);}
}
'''
 run_c(harness)
def test_web_binding_edits_do_not_clear_live_keys_before_save():
 source=(ROOT/'engine/code/q3_ui/ui_rally_controls.c').read_text()
 harness=r'''
#include <assert.h>
#define qfalse 0
#define qtrue 1
typedef int qboolean;
typedef struct {char *label;int id,bind1,bind2;}bind_t;
static bind_t g_bindings[]={{"accelerate",1,'w',-1},{"brake",2,'s',-1},{0,0,0,0}};
static struct{int changesmade,waitingforkey;}s_controls;
static int clears,updates;static int s_webControlsStaged=1;
static void trap_Key_SetBinding(int key,const char *value){clears++;}
static void Controls_Update(void){updates++;}
'''
 harness+=extract(source,'static qboolean Controls_ApplyBindingChange(')
 harness+=r'''
int main(void){assert(Controls_ApplyBindingChange(1,'e'));assert(g_bindings[0].bind1=='e'&&clears==0&&updates==1&&s_controls.changesmade);assert(Controls_ApplyBindingChange(1,-1));assert(g_bindings[0].bind1==-1&&clears==0);g_bindings[0].bind1='w';s_webControlsStaged=0;assert(Controls_ApplyBindingChange(1,'e'));assert(clears>0);}
'''
 run_c(harness)
