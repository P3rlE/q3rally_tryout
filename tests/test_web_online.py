import pathlib, shutil, subprocess, tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def test_online_address_actions_survive_reordering_and_reject_commands():
    source=(ROOT/'engine/code/q3_ui/ui_rally_servers.c').read_text()
    harness=r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#define MAX_ADDRESSLENGTH 64
#define MAX_LISTBOXITEMS 128
#define MAX_FAVORITESERVERS 16
#define UIAS_NUM_SOURCES 8
#define UIAS_FAVORITES 7
#define GAMES_NUM_GAMES 14
#define SORT_NUM_SORTS 5
#define QM_ACTIVATED 1
#define EXEC_APPEND 1
#define qtrue 1
#define qfalse 0
typedef int qboolean;
typedef struct {int id;} menucommon_s;
typedef struct {menucommon_s generic;int curvalue,numitems;} control_t;
typedef control_t menuradiobutton_s;
typedef struct {char adrstr[64],hostname[42],mapname[64];int numclients,humanclients,maxclients,pingtime,gametype;} servernode_t;
static struct {control_t list,master,gametype,sortkey,showfull,showempty,onlyhumans,back;struct{char *string;}status;struct{servernode_t *servernode;}table[128];int refreshservers,numfavoriteaddresses;char favoriteaddresses[16][64];int menu;} g_arenaservers;
static struct {void *activemenu;int transitionIn,transitionOut,realtime,frametime;}uis;
static int g_servertype,g_numfavoriteservers,starts,stops,saves,removed,hosted,ticks,events;
static char address[128],message[128],command[256];
static servernode_t a={"127.0.0.1:27960","Server A","q3r_country01",2,1,8,20,0}, b={"127.0.0.1:27961","Server B","q3r_nightcity",3,2,8,30,0};
static const char *master_items[8],*servertype_items[14],*sortkey_items[5];
#define Q_stricmp _stricmp
static void Q_strncpyz(char *out,const char *in,int size){snprintf(out,size,"%s",in);}
static void trap_Cvar_Set(const char *key,const char *value){if(!strcmp(key,"ui_webonline_message"))Q_strncpyz(message,value,sizeof(message));}
static void trap_Cvar_VariableStringBuffer(const char *key,char *out,int size){Q_strncpyz(out,!strcmp(key,"ui_webonline_address")?address:message,size);}
static const char *va(const char *fmt,...){static char text[512];va_list ap;va_start(ap,fmt);vsnprintf(text,sizeof(text),fmt,ap);va_end(ap);return text;}
static void trap_Cmd_ExecuteText(int when,const char *text){Q_strncpyz(command,text,sizeof(command));}
static void ArenaServers_StopRefresh(void){stops++;g_arenaservers.refreshservers=0;}
static void ArenaServers_StartRefresh(void){starts++;g_arenaservers.refreshservers=1;}
static void ArenaServers_DoRefresh(void){ticks++;}
static void ArenaServers_SaveChanges(void){saves++;}
static void ArenaServers_UpdateMenu(void){}
static void ArenaServers_UpdatePicture(void){}
static const char *ArenaServers_GametypeShortName(int type){return "Race";}
static void ArenaServers_Event(void *item,int event){assert(event==QM_ACTIVATED);events++;}
static void ArenaServers_AddToFavorites(void){Q_strncpyz(g_arenaservers.favoriteaddresses[g_arenaservers.numfavoriteaddresses++],g_arenaservers.table[g_arenaservers.list.curvalue].servernode->adrstr,64);g_numfavoriteservers++;}
static void ArenaServers_RemoveNow(void){removed++;g_arenaservers.numfavoriteaddresses=0;g_numfavoriteservers=0;}
static void UI_StartServerMenu(int multiplayer){assert(multiplayer);hosted++;}
'''
    harness+=source[source.index('static char webOnlineSelected['):]
    harness+=r'''
int main(void){
int old;
uis.activemenu=&g_arenaservers.menu;g_arenaservers.list.numitems=2;g_arenaservers.table[0].servernode=&a;g_arenaservers.table[1].servernode=&b;
assert(WebOnline_AddressValid("example.com:27960"));assert(WebOnline_AddressValid("127.0.0.1"));assert(WebOnline_AddressValid("[::1]:27960"));assert(WebOnline_AddressValid("2001:db8::1"));
assert(!WebOnline_AddressValid(""));assert(!WebOnline_AddressValid("host:0"));assert(!WebOnline_AddressValid("host:65536"));assert(!WebOnline_AddressValid("host:"));assert(!WebOnline_AddressValid("[::1"));assert(!WebOnline_AddressValid("x;quit"));assert(!WebOnline_AddressValid("x\""));assert(!WebOnline_AddressValid("x\nquit"));assert(!WebOnline_AddressValid("host:hello"));
strcpy(address,b.adrstr);UI_WebOnlineAction(5,0,0);assert(g_arenaservers.list.curvalue==1);
g_arenaservers.table[0].servernode=&b;g_arenaservers.table[1].servernode=&a;UI_WebOnlineTick(1234);assert(g_arenaservers.list.curvalue==0&&uis.realtime==1234);
UI_WebOnlineAction(6,0,0);assert(strstr(command,"27961")&&!strstr(command,"27960"));
UI_WebOnlineAction(7,0,0);assert(g_numfavoriteservers==1&&WebOnline_Favorite(b.adrstr));UI_WebOnlineAction(7,0,0);assert(g_numfavoriteservers==1);
g_servertype=0;UI_WebOnlineAction(8,0,0);assert(!removed);g_servertype=7;UI_WebOnlineAction(8,0,0);assert(removed==1&&saves);
strcpy(address,"no-longer-listed.example:27960");command[0]=0;UI_WebOnlineAction(6,0,0);assert(!command[0]&&strstr(message,"no longer"));
strcpy(address,"127.0.0.1:27962");UI_WebOnlineAction(9,0,0);assert(strstr(command,"27962"));
command[0]=0;strcpy(address,"127.0.0.1;quit");UI_WebOnlineAction(9,0,0);assert(!command[0]);
old=events;UI_WebOnlineAction(1,8,0);UI_WebOnlineAction(2,14,0);UI_WebOnlineAction(3,5,0);UI_WebOnlineAction(4,0,2);assert(events==old);
UI_WebOnlineAction(1,7,0);assert(events==old+1&&g_arenaservers.master.curvalue==7);UI_WebOnlineAction(4,1,1);assert(g_arenaservers.showempty.curvalue==1);
UI_WebOnlineAction(10,0,0);assert(starts==1);old=ticks;UI_WebOnlineTick(1234);assert(ticks==old+1);old=webOnlineRevision;UI_WebOnlineTick(1234);assert(old==(int)webOnlineRevision);a.pingtime++;UI_WebOnlineTick(1234);assert(old!=(int)webOnlineRevision);
UI_WebOnlineAction(11,0,0);assert(!g_arenaservers.refreshservers);UI_WebOnlineAction(12,0,0);assert(hosted==1);
uis.activemenu=0;old=events;UI_WebOnlineAction(1,0,0);assert(events==old);
}
'''
    with tempfile.TemporaryDirectory(dir=ROOT/'tests') as directory:
        c=pathlib.Path(directory)/'online.c';exe=c.with_suffix('.exe');c.write_text(harness)
        subprocess.run([shutil.which('gcc') or 'C:/msys64/mingw64/bin/gcc.exe',str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True,timeout=5)
