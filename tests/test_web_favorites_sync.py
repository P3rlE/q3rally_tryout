import json
import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]


def test_cloud_favorites_are_validated_before_profile_handoff():
    source = (ROOT / "engine/code/client/cl_downloads.c").read_text()

    def function(name):
        start = source.index("static ", source.index(name) - 30)
        end = source.index("\n}", start) + 2
        return source[start:end]

    harness = r"""
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define JSON_IMPLEMENTATION
#include "json.h"
typedef int qboolean;
#define qfalse 0
#define qtrue 1
#define Com_Memset memset
#define Com_sprintf snprintf
typedef struct { char *buf; size_t bufLen; } clDlRequest_t;
static char payload[1600];
static void Cvar_Set(const char *key,const char *value) {
    assert(!strcmp(key,"ui_webprofile_favorites"));
    snprintf(payload,sizeof(payload),"%s",value);
}
""" + function("CL_ProfileAssetId(") + "\n" + function("CL_ProfileReceiveFavorites(")
    harness += r"""
static void receive(char *json) {
    clDlRequest_t req={json,strlen(json)};
    CL_ProfileReceiveFavorites(&req,"12345678-1234-4234-8234-123456789abc");
}
int main(void) {
"""
    favorite = dict(slot=2, vehicle="alpine", paint="red", wheels="svt_cobra",
                    head="doom", plate="usa_california")

    def receive(value):
        return "receive(" + json.dumps(json.dumps(value)) + ");\n"

    harness += receive({"favorites": [favorite]})
    harness += r"""
    assert(strstr(payload," - - - - - - - - - - alpine red svt_cobra doom usa_california - - - - -"));
    strcpy(payload,"unchanged");
"""
    harness += receive({"favorites": [dict(favorite, slot=4)]})
    harness += 'assert(!strcmp(payload,"unchanged"));\n'
    harness += receive({"favorites": [dict(favorite, vehicle="../alpine")]})
    harness += 'assert(!strcmp(payload,"unchanged"));\n'
    harness += receive({})
    harness += 'assert(!strcmp(payload,"unchanged"));\n'
    harness += receive({"favorites": []})
    harness += 'assert(strstr(payload," - - - - - - - - - - - - - - - - - - - -"));\n}\n'
    compiler = shutil.which("gcc") or "C:/msys64/mingw64/bin/gcc.exe"
    with tempfile.TemporaryDirectory(dir=ROOT / "tests") as directory:
        sourcefile = pathlib.Path(directory) / "sync.c"
        binary = pathlib.Path(directory) / "sync.exe"
        sourcefile.write_text(harness)
        subprocess.run([compiler, "-I" + str(ROOT / "engine/code/qcommon"),
                        str(sourcefile), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=5)


def test_profile_settings_keep_favorites_and_plate_after_garage_handoff():
    source = (ROOT / "engine/code/q3_ui/ui_playersettings.c").read_text()

    def function(name):
        start = source.index("static ", source.index(name) - 30)
        return source[start:source.index("\n}", start) + 2]

    harness = r"""
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef int qboolean;
#define qtrue 1
#define qfalse 0
#define MAX_QPATH 64
#define PROFILE_MAX_FAVORITE_CARS 4
#define Com_sprintf snprintf
#define Com_Memset memset
#define Q_strncpyz(dst,src,size) snprintf(dst,size,"%s",src)
typedef struct {char model[64],skin[64],rim[64],head[64],plate[64];} favorite_t;
typedef struct {favorite_t favoriteCars[4];} profile_info_t;
static qboolean PlayerSettings_TryProfileFavorite(const char *name,char *model,char *skin,char *rim,char *head) {
    if(strcmp(name,"favoritecar1")) return qfalse;
    strcpy(model,"viper"); strcpy(skin,"rally27"); strcpy(rim,"svt_cobra"); strcpy(head,"doom");
    return qtrue;
}
static qboolean GetValuesFromFavorite(const char *name,char *model,char *skin,char *rim,char *head) {
    return qtrue;
}
""" + function("PlayerSettings_GetFavoriteValues(") + "\n" + function("PlayerSettings_CopyFavoritesToProfile(")
    harness += r"""
int main(void) {
    profile_info_t info={0};
    strcpy(info.favoriteCars[0].plate,"usa_california");
    PlayerSettings_CopyFavoritesToProfile(&info);
    assert(!strcmp(info.favoriteCars[0].model,"viper"));
    assert(!strcmp(info.favoriteCars[0].skin,"rally27"));
    assert(!strcmp(info.favoriteCars[0].plate,"usa_california"));
    assert(!info.favoriteCars[1].model[0]);
}
"""
    compiler = shutil.which("gcc") or "C:/msys64/mingw64/bin/gcc.exe"
    with tempfile.TemporaryDirectory(dir=ROOT / "tests") as directory:
        sourcefile = pathlib.Path(directory) / "settings.c"
        binary = pathlib.Path(directory) / "settings.exe"
        sourcefile.write_text(harness)
        subprocess.run([compiler, str(sourcefile), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=5)


def test_loading_web_favorites_preserves_unset_local_slots_and_profile_owner():
    source = (ROOT / "engine/code/q3_ui/ui_rally_profiles.c").read_text()
    start = source.index("int UI_WebImportFavorites(")
    function = source[start:source.index("\n}", start) + 2]
    harness = r"""
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define PROFILE_MAX_FAVORITE_CARS 4
#define PROFILE_MAX_FAVORITE_FIELD 64
#define Q_strncpyz(dst,src,size) snprintf(dst,size,"%s",src)
#define Q_stricmp strcmp
typedef struct {char model[64],skin[64],rim[64],head[64],plate[64];} favorite_t;
typedef struct {char uuid[37];favorite_t favoriteCars[4];} profile_info_t;
static profile_info_t active;
static const char *incoming;
static int writes;
static const profile_info_t *UI_Profile_GetActiveInfo(void) {return &active;}
static void trap_Cvar_VariableStringBuffer(const char *key,char *out,int size) {snprintf(out,size,"%s",incoming);}
static void trap_Cvar_Set(const char *key,const char *value) {}
static char *COM_Parse(char **cursor) {char *value=strtok(*cursor," ");*cursor=NULL;return value?value:"";}
static int UI_Profile_SaveActiveInfo(const profile_info_t *info) {active=*info;writes++;return 1;}
static void UI_Profile_SetFavoriteCvars(const profile_info_t *info) {}
""" + function
    harness += r"""
int main(void) {
    strcpy(active.uuid,"12345678-1234-4234-8234-123456789abc");
    strcpy(active.favoriteCars[1].model,"camaro");
    strcpy(active.favoriteCars[1].plate,"eu_france");
    incoming="12345678-1234-4234-8234-123456789abc alpine red svt_cobra doom usa_california - - - - - - - - - - - - - - -";
    assert(UI_WebImportFavorites()==1 && writes==1);
    assert(!strcmp(active.favoriteCars[0].model,"alpine"));
    assert(!strcmp(active.favoriteCars[1].model,"camaro"));
    assert(!strcmp(active.favoriteCars[1].plate,"eu_france"));
    incoming="other-profile alpine blue svt_cobra doom usa_california - - - - - - - - - - - - - - -";
    assert(UI_WebImportFavorites()==0 && writes==1);
    assert(!strcmp(active.favoriteCars[0].skin,"red"));
}
"""
    compiler = shutil.which("gcc") or "C:/msys64/mingw64/bin/gcc.exe"
    with tempfile.TemporaryDirectory(dir=ROOT / "tests") as directory:
        sourcefile = pathlib.Path(directory) / "import.c"
        binary = pathlib.Path(directory) / "import.exe"
        sourcefile.write_text(harness)
        subprocess.run([compiler, str(sourcefile), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=5)
