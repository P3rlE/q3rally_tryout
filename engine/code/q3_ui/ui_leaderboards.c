#include "ui_local.h"

#define MAX_LEADERBOARD_MAPS 128

static leaderboardEntry_t leaderboardList[MAX_LEADERBOARD_MAPS];
static int leaderboardCount;

static void UI_LoadLeaderboards(void) {
    char filelist[4096];
    int numFiles;
    char *fileptr;
    int i;

    leaderboardCount = 0;
    numFiles = trap_FS_GetFileList("Records/", ".rec", filelist, sizeof(filelist));
    fileptr = filelist;
    for (i = 0; i < numFiles && leaderboardCount < MAX_LEADERBOARD_MAPS; i++) {
        int len = strlen(fileptr);
        leaderboardEntry_t *entry;
        if (len < 4) {
            fileptr += len + 1;
            continue;
        }

        entry = &leaderboardList[leaderboardCount];
        Q_strncpyz(entry->map, fileptr, sizeof(entry->map));
        if (len > 4)
            entry->map[len - 4] = '\0';

        {
            fileHandle_t f;
            char path[MAX_QPATH];
            int size;
            Com_sprintf(path, sizeof(path), "Records/%s", fileptr);
            size = trap_FS_FOpenFile(path, &f, FS_READ);
            if (size >= 0) {
                char buffer[1024];
                if (size > sizeof(buffer) - 1) {
                    size = sizeof(buffer) - 1;
                }
                trap_FS_Read(buffer, size, f);
                trap_FS_FCloseFile(f);
                buffer[size] = '\0';
                // parse first line
                entry->hasSpeed = qfalse;
                if (size > 0) {
                    char name[64], vehicle[64];
                    float time, speed;
                    int count = sscanf(buffer, "\"%63[^\"]\" %f %63s %f", name, &time, vehicle, &speed);
                    Q_strncpyz(entry->name, name, sizeof(entry->name));
                    entry->time = time;
                    Q_strncpyz(entry->vehicle, vehicle, sizeof(entry->vehicle));
                    if (count == 4) {
                        entry->speed = speed;
                        entry->hasSpeed = qtrue;
                    }
                }
            }
        }
        leaderboardCount++;
        fileptr += len + 1;
    }
}

void UI_Leaderboards_MenuInit(void) {
    int i;
    UI_LoadLeaderboards();
    uiInfo.leaderboardCount = leaderboardCount;
    for (i = 0; i < leaderboardCount; i++) {
        uiInfo.leaderboards[i] = leaderboardList[i];
    }
}

int UI_Leaderboards_FeederCount(void) {
    return uiInfo.leaderboardCount;
}

const char *UI_Leaderboards_FeederItemText(int index, int column) {
    leaderboardEntry_t *e;
    if (index < 0 || index >= uiInfo.leaderboardCount)
        return "";
    e = &uiInfo.leaderboards[index];
    switch (column) {
    case 0:
        return e->map;
    case 1:
        return va("%0.3f", e->time);
    case 2:
        return e->name;
    case 3:
        return e->vehicle;
    case 4:
        if (e->hasSpeed) {
            return va("%0.1f", e->speed);
        }
        return "";
    }
    return "";
}

void UI_LeaderboardsMenu( void ) {
    trap_Key_SetCatcher( KEYCATCH_UI );
    Menus_CloseAll();
    Menus_ActivateByName( "leaderboards" );
}
