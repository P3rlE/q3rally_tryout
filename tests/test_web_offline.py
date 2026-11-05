import pathlib, shutil, subprocess, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def extract(source,signature):
    start=source.index(signature); opening=source.index('{',start); depth=1; pos=opening+1
    while depth:
        depth+=(source[pos]=='{')-(source[pos]=='}');pos+=1
    return source[start:pos]
def test_offline_bridge_uses_native_rules_and_guards_actions():
    source=(ROOT/'engine/code/q3_ui/ui_rally_startserver.c').read_text()
    harness=r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define ARRAY_LEN(a) (sizeof(a)/sizeof((a)[0]))
#define MTYPE_FIELD 1
#define MTYPE_SPINCONTROL 2
#define MTYPE_RADIOBUTTON 3
#define QMF_HIDDEN 1
#define QMF_GRAYED 2
#define QMF_INACTIVE 4
#define QMF_NUMBERSONLY 8
#define QM_ACTIVATED 1
#define PLAYER_SLOTS 12
typedef struct menucommon_s {int type,flags;const char *name;void(*callback)(void*,int);} menucommon_s;
typedef struct {int nitems;void *items[80];} menuframework_s;
typedef struct {menucommon_s generic;int curvalue;const char **itemnames;} menulist_s;
typedef menulist_s menuradiobutton_s;
typedef struct {menucommon_s generic;struct {char buffer[32];int maxchars;} field;} menufield_s;
typedef struct {char avatar[128];} profile_info_t;
static const profile_info_t *UI_Profile_GetActiveInfo(void){return NULL;}
static struct {menuframework_s *activemenu;int transitionIn,transitionOut;} uis;
static struct {menuframework_s menu;int multiplayer,nummaps,currentmap,numstats;menulist_s gametype,list;menucommon_s next;char maplist[64][28],maplistname[64][28],mapstats[16][128];} s_startserver;
static struct {menuframework_s menu;int multiplayer,gametype;menulist_s playerType[12],playerTeam[12];menucommon_s playerName[12];menufield_s hostname;menulist_s dedicated;char playerNameBuffers[12][16];} s_serveroptions;
static int gametype_remap[16];static const char *gametype_items[17];
static int popped,filtered,updated,advanced,started,changed,allowsBots=1;
static void UI_PopMenu(void){popped++;}
static void StartServer_GametypeEvent(void *item,int event){assert(item==&s_startserver.gametype&&event==1);filtered++;}
static void StartServer_Update(void){updated++;}
static void StartServer_MenuEvent(void *item,int event){assert(item==&s_startserver.next&&event==1);advanced++;}
static void ServerOptions_Start(void){started++;}
static void ServerOptions_SetPlayerItems(void){changed++;}
static int BG_GametypeAllowsBots(int gametype){return allowsBots;}
static int UI_GetNumBots(void){return 2;}
static const char *UI_GetBotInfoByNumber(int index){assert(index>=0&&index<2);return index?"Bee":"Jim";}
static const char *Info_ValueForKey(const char *info,const char *key){return info;}
static void Q_strncpyz(char *out,const char *value,int size){snprintf(out,size,"%s",value);}
static void trap_Cvar_VariableStringBuffer(const char *key,char *out,int size){Q_strncpyz(out,"Test server",size);}
#define Com_sprintf snprintf
'''
    harness+=extract(source,'static int ServerOptions_PlayerSlotForItem(')
    harness+=source[source.index('int UI_WebOfflineScreen( void )'):]
    harness+=r'''
int main(void){
menufield_s laps={{MTYPE_FIELD,QMF_NUMBERSONLY,"Laps",0},{"3",3}};
const char *lengths[]={"Short","Long",0};
menulist_s length={{MTYPE_SPINCONTROL,0,"Length",0},0,lengths};
uis.activemenu=&s_startserver.menu;s_startserver.nummaps=2;
assert(UI_WebOfflineScreen()==9);UI_WebOfflineAction(1,15,0);assert(filtered==1&&s_startserver.gametype.curvalue==15);
UI_WebOfflineAction(1,16,0);assert(filtered==1);
UI_WebOfflineAction(2,1,0);assert(updated==1&&s_startserver.currentmap==1&&s_startserver.list.curvalue==1);
UI_WebOfflineAction(2,2,0);assert(updated==1);UI_WebOfflineAction(3,0,0);assert(advanced==1);
uis.activemenu=&s_serveroptions.menu;s_serveroptions.menu.nitems=3;
s_serveroptions.menu.items[0]=&laps;s_serveroptions.menu.items[1]=&length;s_serveroptions.menu.items[2]=&s_serveroptions.playerType[1];
s_serveroptions.playerType[1].generic.type=MTYPE_SPINCONTROL;s_serveroptions.playerType[1].itemnames=lengths;
assert(UI_WebOfflineScreen()==10);assert(UI_WebOfflineNumber(6,1)==2);
UI_WebOfflineAction(4,0,12);assert(!strcmp(laps.field.buffer,"12"));UI_WebOfflineAction(4,0,1234);assert(!strcmp(laps.field.buffer,"12"));
UI_WebOfflineAction(4,1,1);assert(length.curvalue==1);UI_WebOfflineAction(4,1,2);assert(length.curvalue==1);
length.generic.flags=QMF_GRAYED;UI_WebOfflineAction(4,1,0);assert(length.curvalue==1);
allowsBots=0;UI_WebOfflineAction(4,2,1);assert(s_serveroptions.playerType[1].curvalue==0);
UI_WebOfflineAction(5,1,1);assert(!s_serveroptions.playerNameBuffers[1][0]);allowsBots=1;
UI_WebOfflineAction(4,2,1);UI_WebOfflineAction(5,1,1);assert(!strcmp(s_serveroptions.playerNameBuffers[1],"Bee"));
UI_WebOfflineAction(5,0,1);assert(!s_serveroptions.playerNameBuffers[0][0]);UI_WebOfflineAction(6,0,0);assert(started==1);
s_startserver.nummaps=0;UI_WebOfflineAction(6,0,0);assert(started==1);
s_serveroptions.multiplayer=1;assert(UI_WebOfflineScreen()==10&&UI_WebOfflineNumber(14,0)==1);
s_serveroptions.menu.items[0]=&s_serveroptions.hostname;s_serveroptions.hostname.generic.type=MTYPE_FIELD;s_serveroptions.hostname.field.maxchars=32;
assert(UI_WebOfflineNumber(4,0)==4);UI_WebOfflineAction(7,0,0);assert(!strcmp(s_serveroptions.hostname.field.buffer,"Test server"));
strcpy(s_serveroptions.hostname.field.buffer,"unchanged");s_serveroptions.multiplayer=0;UI_WebOfflineAction(7,0,0);assert(!strcmp(s_serveroptions.hostname.field.buffer,"unchanged"));
}
'''
    with tempfile.TemporaryDirectory(dir=ROOT/'tests') as directory:
        c=pathlib.Path(directory)/'offline.c';exe=c.with_suffix('.exe');c.write_text(harness)
        subprocess.run([shutil.which('gcc') or 'C:/msys64/mingw64/bin/gcc.exe',str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True,timeout=5)
