import pathlib, shutil, subprocess, tempfile
ROOT = pathlib.Path(__file__).resolve().parents[1]
def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    pos = opening + 1
    while depth:
        depth += (source[pos] == '{') - (source[pos] == '}')
        pos += 1
    return source[start:pos]
def test_web_profile_mutations_validate_and_preserve_identity():
    source = (ROOT/'engine/code/q3_ui/ui_rally_profiles.c').read_text()
    harness = r'''
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define qtrue 1
#define qfalse 0
#define PROFILE_MAX_UUID 37
#define PROFILE_MAX_NAME 32
#define PROFILE_MAX_AVATAR 128
#define PROFILE_MAX_GENDER 32
#define PROFILE_MAX_BIRTHDATE 16
#define PROFILE_MAX_COUNTRY 8
typedef int qboolean;
typedef struct {char uuid[37],avatar[128],gender[32],birthDate[16],country[8]; int sentinelFavorites;} profile_info_t;
static profile_info_t active={"uuid","gfx/avatars/preset/helmet_01","Male","2000-01-01","DE",123};
static struct {const char *path;} uiAvatarPresets[]={{"gfx/avatars/preset/helmet_01"}};
static int uiAvatarPresetCount=1, saves, activates, creates, exists, ok;
static char payload[1024],message[128];
static void Q_strncpyz(char *out,const char *in,int size){snprintf(out,size,"%s",in);}
#define Com_sprintf snprintf
#define Com_Memcpy memcpy
/* A scanner returning zero models the QVM limitation. Date validation must not depend on it. */
#define sscanf(...) 0
static void trap_Cvar_Set(const char *key,const char *value){if(!strcmp(key,"ui_webdriver_ok"))ok=atoi(value);if(!strcmp(key,"ui_webdriver_message"))Q_strncpyz(message,value,sizeof(message));}
static void trap_Cvar_VariableStringBuffer(const char *key,char *out,int size){Q_strncpyz(out,payload,size);}
static int UI_Profile_FindExisting(const char *name,char *out,int size){if(exists&&out)Q_strncpyz(out,name,size);return exists;}
static int UI_Profile_WriteDefaultFile(const char *name){creates++;return 1;}
static int UI_Profile_ReadData(const char *name,profile_info_t *info,void *stats){return 1;}
static void UI_Profile_ActivateProfile(const char *name){activates++;}
static const profile_info_t *UI_Profile_GetActiveInfo(void){return &active;}
static int UI_Profile_HasActiveProfile(void){return 1;}
static int UI_Profile_SaveActiveInfo(const profile_info_t *info){assert(!strcmp(info->uuid,active.uuid));assert(info->sentinelFavorites==123);active=*info;saves++;return 1;}
'''
    for signature in ['static qboolean UI_Profile_NameIsValid(', 'static void UI_Profile_ParseString(']:
        # Skip forward declarations.
        start = source.index(signature)
        if ';' in source[start:source.index('{',start)]: start = source.index(signature,start+1)
        harness += function(source[start:],signature)
    harness += function(source,'int UI_WebProfileMutation(')
    harness += r'''
int main(void){
strcpy(payload,"{\"uuid\":\"uuid\",\"avatar\":\"gfx/avatars/preset/helmet_01\",\"gender\":\"Female\",\"birthDate\":\"2000-02-29\",\"country\":\"GB\"}");
assert(UI_WebProfileMutation(4)==1&&ok&&saves==1);assert(!strcmp(active.country,"GB"));
strcpy(payload,"{\"uuid\":\"uuid\",\"avatar\":\"gfx/avatars/preset/helmet_01\",\"gender\":\"Male\",\"birthDate\":\"1979-12-22\",\"country\":\"DE\"}");
assert(UI_WebProfileMutation(4)==1&&saves==2);assert(!strcmp(active.birthDate,"1979-12-22"));
strcpy(payload,"{\"uuid\":\"other\"}");assert(!UI_WebProfileMutation(4)&&saves==2);
strcpy(payload,"{\"uuid\":\"uuid\",\"avatar\":\"gfx/avatars/preset/helmet_01\",\"birthDate\":\"2001-02-29\"}");assert(!UI_WebProfileMutation(4)&&saves==2);
strcpy(payload,"{\"name\":\"../bad\"}");assert(!UI_WebProfileMutation(5)&&!creates&&!activates);
strcpy(payload,"{\"name\":\"Driver2\"}");exists=1;assert(!UI_WebProfileMutation(5)&&!creates);
exists=0;assert(UI_WebProfileMutation(5)&&creates==1&&activates==1);
assert(!UI_WebProfileMutation(6)&&activates==1);exists=1;assert(UI_WebProfileMutation(6)&&activates==2);
}
'''
    with tempfile.TemporaryDirectory(dir=ROOT/'tests') as directory:
        c=pathlib.Path(directory)/'profile.c';exe=c.with_suffix('.exe');c.write_text(harness)
        subprocess.run([shutil.which('gcc') or 'C:/msys64/mingw64/bin/gcc.exe',str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True,timeout=5)
