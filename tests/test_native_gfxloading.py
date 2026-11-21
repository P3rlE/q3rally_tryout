"""The native graphics loading screen has to look like the WebUI's screen 1.

tests/fixtures/webui_loading_layout.json holds what Chrome reports for the
WebUI (tools/ui_webstyle/measure_webui_loading.cjs, ten viewports, four
states). The native layout and text measurement run here against it.
"""
import json
import pathlib
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
CODE = ROOT / 'engine/code'
FIXTURE = json.loads((ROOT / 'tests/fixtures/webui_loading_layout.json').read_text())
TITLES = {'s2': 'Preparing the setup menu', 's0': 'Starting up', 's8': 'Ready to race', 'prompt': 'Update available'}

HARNESS = r'''
#include "q3_ui/ui_local.h"
#include "q3_ui/ui_rally_gfxlayout.h"
#include "q3_ui/ui_rally_mainlayout.h"
#include "q3_ui/ui_rally_creditslayout.h"
#include "q3_ui/ui_rally_rivalslayout.h"
#include "q3_ui/ui_rally_demoslayout.h"
#include "q3_ui/ui_rally_maploadlayout.h"
#include "q3_ui/ui_rally_offlinelayout.h"
#include "q3_ui/ui_rally_offlinerulelayout.h"
#include "q3_ui/ui_rally_onlinelayout.h"
#include "q3_ui/ui_rally_driverlayout.h"
#include "q3_ui/ui_rally_configlayout.h"
#include "q3_ui/ui_rally_garagelayout.h"
#include "cgame/cg_hud_panellayout.h"
#include "cgame/cg_hud_modernlayout.h"
#include <stdio.h>
uiStatic_t uis;
static FILE *files[8];
int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode) {
    char path[1024]; FILE *h; int i = 1, length;
    snprintf(path, sizeof(path), "%s/%s", BASE, qpath);
    if (!(h = fopen(path, "rb"))) { *f = 0; return -1; }
    while (files[i]) i++;
    files[i] = h; *f = i;
    fseek(h, 0, SEEK_END); length = ftell(h); fseek(h, 0, SEEK_SET);
    return length;
}
void trap_FS_Read(void *buffer, int length, fileHandle_t f) { fread(buffer, 1, length, files[f]); }
void trap_FS_FCloseFile(fileHandle_t f) { fclose(files[f]); files[f] = 0; }
qhandle_t trap_R_RegisterShaderNoMip(const char *name) { return 1; }
void trap_R_SetColor(const float *rgba) {}
void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t shader) {}
void QDECL Com_Printf(const char *fmt, ...) {}
void QDECL Com_Error(int level, const char *fmt, ...) { abort(); }
void UI_DrawVehicleWebView(float x, float y, float w, float h, playerInfo_t *pi, int time,
                           float yaw, float elevation, float distance) {}
#include "q3_ui/ui_rally_webstyle.c"
#include "q3_ui/ui_rally_frontpage.c"
#include "q3_ui/ui_rally_gfxlayout.c"
#include "q3_ui/ui_rally_mainlayout.c"
#include "q3_ui/ui_rally_creditslayout.c"
#include "q3_ui/ui_rally_rivalslayout.c"
#include "q3_ui/ui_rally_demoslayout.c"
#include "q3_ui/ui_rally_maploadlayout.c"
#include "q3_ui/ui_rally_offlinelayout.c"
#include "q3_ui/ui_rally_offlinerulelayout.c"
#include "q3_ui/ui_rally_onlinelayout.c"
#include "q3_ui/ui_rally_driverlayout.c"
#include "q3_ui/ui_rally_configlayout.c"
#include "q3_ui/ui_rally_garagelayout.c"
#include "cgame/cg_hud_panellayout.c"
#include "cgame/cg_hud_modernlayout.c"

static void rect(const char *name, wsRect_t r) { printf("\"%s\":[%g,%g,%g,%g],", name, r.x, r.y, r.w, r.h); }

int main(void) {
    char command[16], text[256];
    while (scanf("%15s", command) == 1) {
        if (!strcmp(command, "layout")) {
            gfxLayout_t l; float w, h; int prompt, i;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %d %255[^\n]", &w, &h, &prompt, text);
            Q_strncpyz(l.title, text, sizeof(l.title));
            strcpy(l.vehicle, "Alpine"); strcpy(l.status, "Ready for the next rally");
            l.prompt = prompt;
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            GFX_Layout(&l, w, h);
            printf("{");
            rect("left", l.left); rect("right", l.room.box); rect("car", l.room.car);
            rect("update", l.buttons[0]); rect("skip", l.buttons[1]);
            printf("\"h1\":%g,\"status\":[%g,%g],\"title\":%g,\"progress\":[%g,%g,%g],\"percent\":%g,",
                l.h1Y, l.statusX, l.statusY, l.titleY, l.progressX, l.progressY, l.progressW, l.percentY);
            printf("\"prompt\":%g,\"vehicle\":%g,\"statusText\":%g,\"stageX\":%g,\"page\":%g,",
                l.promptY, l.room.vehicleY, l.room.statusY, l.stageX, l.pageHeight);
            printf("\"lines\":[%d,%d,%d,%d],\"stages\":[", l.titleLines.count, l.promptLines.count,
                l.room.vehicleLines.count, l.loadingLabelLines.count);
            for (i = 0; i < GFX_LAYOUT_STAGES; i++) printf("%s%g", i ? "," : "", l.stageY[i]);
            printf("]}\n");
        } else if (!strcmp(command, "main")) {
            /* main <width> <height> <profile summary> */
            mainLayout_t l; float w, h; int i;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %255[^\n]", &w, &h, text);
            Q_strncpyz(l.summary, text, sizeof(l.summary));
            strcpy(l.vehicle, "Alpine"); strcpy(l.status, "Ready for the next rally");
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            MainMenu_Layout(&l, w, h);
            printf("{");
            rect("right", l.room.box); rect("car", l.room.car); rect("garage", l.room.garage);
            rect("dialog", l.dialog); rect("accept", l.accept); rect("cancel", l.cancel);
            printf("\"h1\":%g,\"status\":[%g,%g],\"vehicle\":%g,\"statusText\":%g,\"page\":%g,",
                l.page.h1Y, l.page.statusX, l.page.statusY, l.room.vehicleY, l.room.statusY, l.pageHeight);
            printf("\"summary\":[%g,%g],\"dialogText\":[%g,%g,%g,%g],",
                l.nav[MAIN_NAV_PROFILE].x + l.navTextX, l.summaryY,
                l.dialogX, l.dialogLabelY, l.dialogTitleY, l.dialogTextY);
            printf("\"lines\":[%d,%d,%d],\"buttons\":[", l.summaryLines.count, l.room.vehicleLines.count,
                l.dialogTitleLines.count);
            for (i = 0; i < MAIN_NAV_COUNT; i++) {
                printf("%s[%g,%g,%g,%g]", i ? "," : "", l.nav[i].x, l.nav[i].y, l.nav[i].w, l.nav[i].h);
            }
            printf("]}\n");
        } else if (!strcmp(command, "credits")) {
            /* credits <width> <height> <seconds> <count> <kind>... */
            static creditsLayout_t l; int kinds[CREDITS_MAX_LINES], count, i; float w, h, t;
            scanf("%f %f %f %d", &w, &h, &t, &count);
            for (i = 0; i < count; i++) scanf("%d", &kinds[i]);
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            Credits_Layout(&l, w, h, kinds, count);
            printf("{");
            rect("exit", l.exit); rect("window", l.window);
            printf("\"h1\":%g,\"hint\":%g,\"page\":%g,\"rollHeight\":%g,\"duration\":%g,\"title\":%g,\"lines\":[",
                l.page.h1Y, l.hintY, l.pageHeight, l.rollHeight, l.duration, l.title.size);
            for (i = 0; i < l.count; i++) printf("%s[%g,%g]", i ? "," : "", Credits_LineTop(&l, i, t), l.lineH[i]);
            printf("]}\n");
        } else if (!strcmp(command, "rivals")) {
            /* rivals <width> <height> <count> <scroll> <downloads>, then
             * name|personality|weapon|description|status|author|download status */
            static rivalsLayout_t l; float w, h, scroll; int count, downloads; char line[1024], *f[7]; int i;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %d %f %d\n", &w, &h, &count, &scroll, &downloads);
            fgets(line, sizeof(line), stdin); line[strcspn(line, "\n")] = 0;
            f[0] = line; for (i = 1; i < 7; i++) { f[i] = strchr(f[i - 1], '|'); *f[i]++ = 0; }
            l.downloads = downloads; l.hideScrollbar = qtrue;	/* like the measured Chrome */
            { static char rows[8192]; char *r; fgets(rows, sizeof(rows), stdin); rows[strcspn(rows, "\n")] = 0;
              for (i = 0, r = rows; i < count && r; i++) { l.rowText[i] = r; r = strchr(r, '|'); if (r) *r++ = 0; } }
            Q_strncpyz(l.author, f[5], sizeof(l.author)); Q_strncpyz(l.downloadStatus, f[6], sizeof(l.downloadStatus));
            Q_strncpyz(l.name, f[0], sizeof(l.name)); Q_strncpyz(l.personality, f[1], sizeof(l.personality));
            Q_strncpyz(l.weapon, f[2], sizeof(l.weapon)); Q_strncpyz(l.description, f[3], sizeof(l.description));
            Q_strncpyz(l.status, f[4], sizeof(l.status));
            l.count = count; l.scroll = scroll;
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            Rivals_Layout(&l, w, h);
            printf("{");
            rect("back", l.back); rect("left", l.left); rect("right", l.right); rect("search", l.search);
            rect("list", l.list); rect("car", l.car); rect("type", l.type); rect("install", l.install);
            rect("refresh", l.refresh); rect("cancel", l.cancel); rect("progress", l.progress);
            printf("\"typeLabel\":%g,\"author\":%g,\"downloadStatus\":%g,", l.typeLabelY, l.authorY, l.downloadStatusY);
            printf("\"h1\":%g,\"searchLabel\":%g,\"count\":%g,\"label\":%g,\"name\":%g,\"personality\":%g,"
                "\"weapon\":%g,\"description\":%g,\"status\":%g,\"page\":%g,\"content\":%g,\"scroll\":%g,\"buttons\":[",
                l.page.h1Y, l.searchLabelY, l.countY, l.labelY, l.nameY, l.personalityY, l.weaponY, l.descriptionY,
                l.statusY, l.pageHeight, l.contentHeight, l.scroll);
            for (i = 0; i < count; i++) { wsRect_t b; Rivals_Button(&l, i, &b); printf("%s[%g,%g,%g,%g]", i ? "," : "", b.x, b.y, b.w, b.h); }
            printf("]}\n");
        } else if (!strcmp(command, "demos")) {
            /* demos <width> <height> <count> <scroll>, then details|placeholder|clock|message|pause */
            static demosLayout_t l; float w, h; char line[512], *f[5]; int i;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %d %f\n", &w, &h, &l.count, &l.scroll);
            fgets(line, sizeof(line), stdin); line[strcspn(line, "\n")] = 0;
            f[0] = line; for (i = 1; i < 5; i++) { f[i] = strchr(f[i - 1], '|'); *f[i]++ = 0; }
            Q_strncpyz(l.details, f[0], sizeof(l.details)); Q_strncpyz(l.placeholderText, f[1], sizeof(l.placeholderText));
            l.placeholder = f[1][0] != 0;
            Q_strncpyz(l.clock, f[2], sizeof(l.clock)); Q_strncpyz(l.message, f[3], sizeof(l.message)); Q_strncpyz(l.pause, f[4], sizeof(l.pause));
            l.hideScrollbar = qtrue;
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            Demos_Layout(&l, w, h);
            printf("{");
            rect("back", l.back); rect("left", l.left); rect("right", l.right); rect("list", l.list);
            rect("play", l.play); rect("viewport", l.viewport);
            printf("\"h1\":%g,\"recordings\":%g,\"choose\":%g,\"details\":[%g,%g],\"preview\":%g,\"placeholder\":[%g,%g],"
                "\"clock\":%g,\"message\":%g,\"page\":%g,\"option\":[%g,%g,%g],\"transport\":[",
                l.page.h1Y, l.recordingsY, l.chooseY, l.detailsY, l.detailLines.count * DEMOS_DETAIL_LINE, l.previewY,
                l.placeholderX, l.placeholderY, l.clockY, l.messageX, l.pageHeight, l.optionHeight, l.optionWidth, l.contentHeight);
            for (i = 0; i < 5; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.transport[i].x, l.transport[i].y, l.transport[i].w, l.transport[i].h);
            printf("]}\n");
        } else if (!strcmp(command, "mapload")) {
            /* mapload <width> <height> <image>, then name|server|detail|transfer */
            static mapLoadLayout_t l; float w, h; int image; char line[512], *f[4]; int i;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %d\n", &w, &h, &image);
            fgets(line, sizeof(line), stdin); line[strcspn(line, "\n")] = 0;
            f[0] = line; for (i = 1; i < 4; i++) { f[i] = strchr(f[i - 1], '|'); *f[i]++ = 0; }
            l.image = image;
            Q_strncpyz(l.name, f[0], sizeof(l.name)); Q_strncpyz(l.server, f[1], sizeof(l.server));
            Q_strncpyz(l.detail, f[2], sizeof(l.detail)); Q_strncpyz(l.transfer, f[3], sizeof(l.transfer));
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            MapLoad_Layout(&l, w, h);
            printf("{");
            rect("preview", l.preview); rect("status", l.status); rect("image", l.shot); rect("progress", l.progress);
            rect("cancel", l.cancel);
            printf("\"label\":%g,\"h1\":[%g,%g],\"name\":[%g,%d],\"server\":%g,\"statusLabel\":%g,\"detail\":[%g,%d],"
                "\"transfer\":%g,\"note\":[%g,%d],\"page\":%g}\n",
                l.labelY, l.h1X, l.h1Y, l.nameY, l.nameLines.count, l.serverY, l.statusLabelY, l.detailY, l.detailLines.count,
                l.transferY, l.noteY, l.noteLines.count, l.pageHeight);
        } else if (!strcmp(command, "offline")) {
            /* offline <width> <height> <scroll> <image>, then mapId|name, rows (|), details (|) */
            static offlineLayout_t l; static char head[256], rows[4096], details[2048]; float w, h; int image, i; char *r;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %f %d\n", &w, &h, &l.scroll, &image);
            fgets(head, sizeof(head), stdin); head[strcspn(head, "\n")] = 0;
            fgets(rows, sizeof(rows), stdin); rows[strcspn(rows, "\n")] = 0;
            fgets(details, sizeof(details), stdin); details[strcspn(details, "\n")] = 0;
            r = strchr(head, '|'); *r++ = 0;
            Q_strncpyz(l.mapId, head, sizeof(l.mapId)); Q_strncpyz(l.name, r, sizeof(l.name));
            for (r = rows; *r && l.count < OFFLINE_MAX_ROWS; ) { l.rowText[l.count++] = r; r = strchr(r, '|'); if (!r) break; *r++ = 0; }
            for (r = details; *r && l.details < OFFLINE_MAX_DETAILS; ) { l.detail[l.details++] = r; r = strchr(r, '|'); if (!r) break; *r++ = 0; }
            l.image = image; l.hideScrollbar = qtrue;
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            Offline_Layout(&l, w, h);
            printf("{");
            rect("back", l.back); rect("left", l.left); rect("right", l.right); rect("mode", l.mode); rect("search", l.search);
            rect("list", l.list); rect("image", l.shot); rect("next", l.next);
            printf("\"label\":%g,\"h1\":%g,\"step\":[%g,%g],\"modeLabel\":%g,\"searchLabel\":%g,\"count\":%g,"
                "\"mapId\":%g,\"name\":%g,\"details\":[%g,%g],\"page\":%g,\"buttons\":[",
                l.labelY, l.h1Y, l.stepY, l.stepRulesX, l.modeLabelY, l.searchLabelY, l.countY, l.mapIdY, l.nameY,
                l.detailsY, l.detailsH, l.pageHeight);
            for (i = 0; i < l.count; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.list.x, l.list.y - l.scroll + l.rowY[i], l.rowWidth, l.rowH[i]);
            printf("],\"detailBoxes\":[");
            for (i = 0; i < l.details; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.detailX[i], l.detailY[i], l.detailW, l.detailLines[i].count * l.detailStyle.lineHeight);
            printf("]}\n");
        } else if (!strcmp(command, "rules")) {
            /* rules <width> <height> <options scroll> <players scroll> <hosting>, then title|mode,
               the rule help, the grid help, fields (I:/S:text|...), rows (human~text~kinds;...) */
            static rulesLayout_t l; static char head[256], rhelp[1024], ghelp[1024], fields[2048], rows[4096];
            float w, h; int hosting, i, j; char *r, *next;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %f %f %d\n", &w, &h, &l.optionsScroll, &l.playersScroll, &hosting);
            fgets(head, sizeof(head), stdin); head[strcspn(head, "\n")] = 0;
            fgets(rhelp, sizeof(rhelp), stdin); rhelp[strcspn(rhelp, "\n")] = 0;
            fgets(ghelp, sizeof(ghelp), stdin); ghelp[strcspn(ghelp, "\n")] = 0;
            fgets(fields, sizeof(fields), stdin); fields[strcspn(fields, "\n")] = 0;
            fgets(rows, sizeof(rows), stdin); rows[strcspn(rows, "\n")] = 0;
            r = strchr(head, '|'); *r++ = 0;
            Q_strncpyz(l.title, head, sizeof(l.title)); Q_strncpyz(l.modeName, r, sizeof(l.modeName));
            l.ruleHelp = rhelp; l.gridHelp = ghelp; l.hosting = hosting; l.hideScrollbar = qtrue;
            for (r = fields; *r && l.fields < RULES_MAX_FIELDS; r = next) {
                next = strchr(r, '|'); if (next) *next++ = 0; else next = r + strlen(r);
                l.fieldInput[l.fields] = r[0] == 'I'; l.fieldText[l.fields++] = r + 2;
            }
            for (r = rows; *r && l.rows < RULES_MAX_ROWS; r = next) {
                char *text, *kinds;
                next = strchr(r, ';'); if (next) *next++ = 0; else next = r + strlen(r);
                text = strchr(r, '~'); *text++ = 0; kinds = strchr(text, '~'); *kinds++ = 0;
                l.rowHuman[l.rows] = r[0] == '1'; l.rowText[l.rows] = text;
                for (j = 0; kinds[j] && j < RULES_MAX_PARTS; j++)
                    l.rowKind[l.rows][j] = kinds[j] == 'P' ? RULES_P : kinds[j] == 'A' ? RULES_AVATAR : kinds[j] == 'T' ? RULES_TYPE
                        : kinds[j] == 'C' ? RULES_CAR : kinds[j] == 'B' ? RULES_BOT : RULES_TEAM;
                l.rowParts[l.rows++] = j;
            }
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            Rules_Layout(&l, w, h);
            printf("{");
            rect("back", l.head.back); rect("summary", l.summary); rect("start", l.start); rect("left", l.left); rect("right", l.right);
            rect("options", l.options); rect("players", l.players);
            printf("\"label\":%g,\"h1\":%g,\"step\":[%g,%g],\"summaryLabel\":%g,\"title\":%g,\"raceMode\":%g,"
                "\"rulesH2\":%g,\"ruleHelp\":[%g,%d],\"gridH2\":%g,\"gridHelp\":[%g,%d],\"page\":%g,\"heights\":[%g,%g],\"fields\":[",
                l.head.labelY, l.head.h1Y, l.head.stepY, l.head.stepRulesX, l.summaryLabelY, l.titleY, l.modeY,
                l.rulesH2Y, l.ruleHelpY, l.ruleHelpLines.count, l.gridH2Y, l.gridHelpY, l.gridHelpLines.count, l.pageHeight,
                l.optionsHeight, l.playersHeight);
            for (i = 0; i < l.fields; i++) printf("%s[%g,%g,%g,%g,%g,%g,%g,%g]", i ? "," : "",
                l.options.x + l.fieldBox[i].x, l.options.y - l.optionsScroll + l.fieldBox[i].y, l.fieldBox[i].w, l.fieldBox[i].h,
                l.options.x + l.fieldControl[i].x, l.options.y - l.optionsScroll + l.fieldControl[i].y, l.fieldControl[i].w, l.fieldControl[i].h);
            printf("],\"rows\":[");
            for (i = 0; i < l.rows; i++) {
                float oy = l.players.y - l.playersScroll + l.row[i].y;
                printf("%s{\"row\":[%g,%g,%g,%g],\"parts\":[", i ? "," : "", l.players.x, oy, l.rowWidth, l.row[i].h);
                for (j = 0; j < l.row[i].parts; j++) {
                    rulesItem_t *p = &l.row[i].part[j];
                    printf("%s[%d,%g,%g,%g,%g,%g,%g,%g,%g]", j ? "," : "", p->kind, l.players.x + p->box.x, oy + p->box.y, p->box.w, p->box.h,
                        l.players.x + p->control.x, oy + p->control.y, p->control.w, p->control.h);
                }
                printf("]}");
            }
            printf("]}\n");
        } else if (!strcmp(command, "online")) {
            /* online <width> <height> <list scroll> <picked> <image> <remove> <dialog>, then lines:
               status|count|empty|message|name|address|favorite|dialog text, details (|), rows (name~sub|...) */
            static onlineLayout_t l; static char head[2048], details[1024], rows[8192]; char *field[8], *r, *next;
            float w, h; int picked, image, removable, dialog, i, n;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %f %d %d %d %d\n", &w, &h, &l.listScroll, &picked, &image, &removable, &dialog);
            fgets(head, sizeof(head), stdin); head[strcspn(head, "\n")] = 0;
            fgets(details, sizeof(details), stdin); details[strcspn(details, "\n")] = 0;
            fgets(rows, sizeof(rows), stdin); rows[strcspn(rows, "\n")] = 0;
            for (n = 0, r = head; n < 8; n++) { field[n] = r; r = strchr(r, '|'); if (r) *r++ = 0; else r = head + strlen(head); }
            l.status = field[0]; l.count = field[1]; l.empty = field[2]; l.message = field[3]; l.name = field[4];
            l.address = field[5]; l.favorite = field[6]; l.dialogText = field[7];
            for (n = 0, r = details; n < ONLINE_DETAILS; n++) { l.detail[n] = r; r = strchr(r, '|'); if (r) *r++ = 0; else r = details + strlen(details); }
            for (r = rows; *r && l.rows < ONLINE_MAX_ROWS; r = next) {
                next = strchr(r, '|'); if (next) *next++ = 0; else next = r + strlen(r);
                l.rowName[l.rows] = r; r = strchr(r, '~'); *r++ = 0; l.rowSub[l.rows++] = r;
            }
            l.picked = picked; l.image = image; l.remove = removable; l.dialog = dialog; l.hideScrollbar = qtrue;
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            Online_Layout(&l, w, h);
            printf("{");
            rect("back", l.back); rect("toolbar", l.toolbar); rect("listSection", l.listSection); rect("detailSection", l.detailSection);
            rect("list", l.list); rect("image", l.shot); rect("dialog", l.dialogBox); rect("dialogInput", l.dialogInput);
            printf("\"filters\":[");
            for (i = 0; i < ONLINE_FILTERS; i++) printf("%s[%g,%g,%g,%g,%g,%g,%g,%g]", i ? "," : "", l.filterBox[i].x, l.filterBox[i].y, l.filterBox[i].w, l.filterBox[i].h,
                l.filterControl[i].x, l.filterControl[i].y, l.filterControl[i].w, l.filterControl[i].h);
            printf("],\"toggles\":[");
            for (i = 0; i < ONLINE_TOGGLES; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.toggle[i].x, l.toggle[i].y, l.toggle[i].w, l.toggle[i].h);
            printf("],\"actions\":[");
            for (i = 0; i < ONLINE_ACTIONS; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.action[i].x, l.action[i].y, l.action[i].w, l.action[i].h);
            printf("],\"buttons\":[");
            for (i = 0; i < ONLINE_BUTTONS; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.button[i].x, l.button[i].y, l.button[i].w, l.button[i].h);
            printf("],\"dialogButtons\":[");
            for (i = 0; i < 2; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.dialogButton[i].x, l.dialogButton[i].y, l.dialogButton[i].w, l.dialogButton[i].h);
            printf("],\"cards\":[");
            for (i = 0; i < ONLINE_DETAILS && picked; i++) printf("%s[%g,%g,%g,%g,%g]", i ? "," : "", l.card[i].x, l.card[i].y, l.card[i].w, l.card[i].h, l.cardValueY[i]);
            printf("],\"rows\":[");
            for (i = 0; i < l.rows; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.list.x, l.list.y - l.listScroll + l.rowY[i], l.rowWidth, l.rowH[i]);
            printf("],\"status\":[%g,%d],\"heading\":[%g,%g,%g],\"countY\":%g,\"label\":%g,\"name\":[%g,%d],\"address\":[%g,%d],"
                "\"details\":[%g,%g],\"message\":%g,\"emptyY\":%g,\"page\":%g,\"listHeight\":%g,\"dialogY\":[%g,%g,%g,%g,%g,%g]}\n",
                l.statusY, l.statusLines.count, l.headingY, l.headingPlayersX, l.headingPingX, l.countY, l.labelY, l.nameY, l.nameLinesDetail.count,
                l.addressY, l.addressLines.count, l.detailsY, l.detailsH, l.messageY, l.list.y + l.emptyY, l.pageHeight, l.listHeight,
                l.dialogLabelY, l.dialogH2Y, l.dialogFieldLabelY, l.dialogHelpY, l.dialogErrorY, l.dialogAddressY);
        } else if (!strcmp(command, "config")) {
            /* config <width> <height> <page> <dialog> <scroll>, then lines: title, description, capture action,
               buttons (|), fields (type~label~text|...); \x1e stands for a line feed */
            static configLayout_t l; static char title[256], description[4096], action[256], buttons[2048], fields[16384];
            char *r, *next, *c; float w, h; int dialog, i;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %d %d %f\n", &w, &h, &l.screen, &dialog, &l.pageScroll);
            fgets(title, sizeof(title), stdin); title[strcspn(title, "\n")] = 0;
            fgets(description, sizeof(description), stdin); description[strcspn(description, "\n")] = 0;
            fgets(action, sizeof(action), stdin); action[strcspn(action, "\n")] = 0;
            fgets(buttons, sizeof(buttons), stdin); buttons[strcspn(buttons, "\n")] = 0;
            fgets(fields, sizeof(fields), stdin); fields[strcspn(fields, "\n")] = 0;
            for (c = description; *c; c++) if (*c == '\x1e') *c = '\n';
            for (c = fields; *c; c++) if (*c == '\x1e') *c = '\n';
            l.title = title; l.description = description; l.captureAction = action; l.dialog = dialog;
            for (r = buttons; *r && l.buttons < CONFIG_MAX_BUTTONS; r = next) {
                next = strchr(r, '|'); if (next) *next++ = 0; else next = r + strlen(r);
                l.buttonLabel[l.buttons++] = r;
            }
            for (r = fields; *r && l.fields < CONFIG_MAX_FIELDS; r = next) {
                configField_t *f = &l.field[l.fields++];
                next = strchr(r, '|'); if (next) *next++ = 0; else next = r + strlen(r);
                f->type = atoi(r); r = strchr(r, '~') + 1; f->label = r; r = strchr(r, '~'); *r++ = 0; f->text = r;
                if ((c = strchr(r, '~'))) { *c++ = 0; f->avatar = atoi(c); }
            }
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            Config_Layout(&l, w, h);
            printf("{");
            rect("back", l.back); rect("nav", l.nav); rect("section", l.section); rect("dialog", l.dialogBox); rect("cancel", l.cancel);
            printf("\"buttons\":[");
            for (i = 0; i < l.buttons; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.button[i].x, l.button[i].y, l.button[i].w, l.button[i].h);
            printf("],\"fields\":[");
            for (i = 0; i < l.fields && l.settings; i++) {
                configField_t *f = &l.field[i];
                printf("%s[[%g,%g,%g,%g],[%g,%g,%g,%g],%g,[%g,%g,%g,%g],[%g,%g,%g,%g],%d,[%g,%g,%g,%g],[%g,%g,%g,%g]]", i ? "," : "", f->box.x, f->box.y, f->box.w, f->box.h,
                    f->control.x, f->control.y, f->control.w, f->control.h, f->outputY, f->rebind.x, f->rebind.y, f->rebind.w, f->rebind.h,
                    f->clear.x, f->clear.y, f->clear.w, f->clear.h, f->labelLines.count, f->preview.x, f->preview.y, f->preview.w, f->preview.h,
                    f->image.x, f->image.y, f->image.w, f->image.h);
            }
            printf("],\"description\":[%g,%g],\"page\":%g,\"settings\":%d,\"dialogY\":[%g,%g,%g,%g],\"h2\":[%d,%g]}\n",
                l.descriptionY, l.descriptionLines.count * l.descriptionStyle.lineHeight, l.pageHeight, l.settings, l.dialogLabelY, l.dialogH2Y, l.dialogActionY, l.dialogHelpY,
                l.dialogH2Lines.count, l.h2.size);
        } else if (!strcmp(command, "hud")) {
            /* hud <width> <height> <scroll>, then the toggles' labels (|) */
            static hudPanelLayout_t l; static char labels[4096]; char *r, *next; float w, h; int i;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %f\n", &w, &h, &l.scroll);
            fgets(labels, sizeof(labels), stdin); labels[strcspn(labels, "\n")] = 0;
            for (r = labels; *r && l.items < HUDPANEL_MAX_ITEMS; r = next) {
                next = strchr(r, '|'); if (next) *next++ = 0; else next = r + strlen(r);
                l.itemLabel[l.items++] = r;
            }
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            HUDPanel_Layout(&l, w, h);
            printf("{");
            rect("panel", l.panel); rect("close", l.close); rect("console", l.console); rect("size", l.size); rect("zoom", l.zoom);
            printf("\"items\":[");
            for (i = 0; i < l.items; i++) printf("%s[%g,%d,%g,%g,%g,%g]", i ? "," : "", l.itemY[i], l.itemLines[i].count,
                l.select[i].x, l.select[i].y, l.select[i].w, l.select[i].h);
            printf("],\"y\":[%g,%g,%g,%g,%g,%g,%g,%g,%g],\"text\":%d,\"content\":%g,\"h2\":%g,\"x\":[%g,%g,%g]}\n",
                l.labelY, l.h2Y, l.textY, l.h3Y, l.sizeY, l.sizeOutputY, l.zoomY, l.zoomOutputY, l.hintY,
                l.textLines.count, l.contentHeight, l.h2Size, l.x, l.w, l.emX);
        } else if (!strcmp(command, "hudmsg")) {
            /* hudmsg <view width>, then: title|size|top|sub;sub */
            static mhMessageLayout_t m; static char line[1024]; char *f[4], *r, *item, *more; int n = 0, i;
            memset(&m, 0, sizeof(m));
            scanf("%f\n", &m.viewWidth);
            fgets(line, sizeof(line), stdin); line[strcspn(line, "\n")] = 0;
            for (r = line; n < 4; ) { f[n++] = r; r = strchr(r, '|'); if (!r) break; *r++ = 0; }
            m.title = f[0]; m.titleSize = atof(f[1]); m.top = atof(f[2]);
            for (item = n > 3 ? f[3] : ""; *item && m.subCount < MH_SUB_MAX; item = more) {
                more = strchr(item, ';'); if (more) *more++ = 0; else more = item + strlen(item);
                m.sub[m.subCount++] = item;
            }
            uis.glconfig.vidWidth = m.viewWidth; uis.glconfig.vidHeight = 900;
            MH_MessageLayout(&m);
            printf("{\"title\":[%g,%g,%g,%g],\"titleText\":\"%s\"", m.titleT.box.x, m.titleT.box.y, m.titleT.box.w, m.titleT.box.h, m.titleT.text);
            for (i = 0; i < m.subCount; i++) printf(",\"sub%d\":[%g,%g,%g,%g]", i, m.subT[i].box.x, m.subT[i].box.y, m.subT[i].box.w, m.subT[i].box.h);
            printf(",\"size\":%g}\n", m.titleT.style.size);
        } else if (!strcmp(command, "hudspec")) {
            /* hudspec <view width>, then: label|name|hint|hint */
            static mhSpectatorLayout_t sp; static char line[512]; char *f[4], *r; int n = 0;
            memset(&sp, 0, sizeof(sp));
            scanf("%f\n", &sp.viewWidth);
            fgets(line, sizeof(line), stdin); line[strcspn(line, "\n")] = 0;
            for (r = line; n < 4; ) { f[n++] = r; r = strchr(r, '|'); if (!r) break; *r++ = 0; }
            sp.label = f[0]; sp.name = f[1]; sp.hint1 = f[2]; sp.hint2 = f[3];
            uis.glconfig.vidWidth = sp.viewWidth; uis.glconfig.vidHeight = 900;
            MH_SpectatorLayout(&sp);
#define B(name, b) printf("\"%s\":[%g,%g,%g,%g],", name, (b).x, (b).y, (b).w, (b).h)
            printf("{"); B("box", sp.box); B("label", sp.labelT.box); B("name", sp.nameT.box);
            B("hint1", sp.hint1T.box); B("hint2", sp.hint2T.box); printf("\"nameText\":\"%s\"}\n", sp.nameT.text);
#undef B
        } else if (!strcmp(command, "hudrace")) {
            /* hudrace <view width>, then one line of fields split by |, lists by ; and : */
            static mhRaceLayout_t l; static char line[4096]; static char *f[48]; char *r, *next, *item, *more;
            int n = 0, i;
            memset(&l, 0, sizeof(l));
            scanf("%f\n", &l.viewWidth);
            fgets(line, sizeof(line), stdin); line[strcspn(line, "\n")] = 0;
            for (r = line; n < 48; r = next) {
                next = strchr(r, '|'); f[n++] = r;
                if (!next) break;
                *next++ = 0;
            }
#define FIELD(k) (*f[k] ? f[k] : NULL)
            l.title = f[0]; l.pos = FIELD(1); l.of = f[2]; l.lapLabel = f[3]; l.lapValue = FIELD(4);
            for (item = f[5]; *item && l.kvCount < MH_KV_MAX; item = more) {
                more = strchr(item, ';'); if (more) *more++ = 0; else more = item + strlen(item);
                l.kvKey[l.kvCount] = item; r = strchr(item, ':'); *r = 0; l.kvValue[l.kvCount++] = r + 1;
            }
            l.standings = 1; l.orderTitle = f[6]; l.orderRight = f[7];
            for (item = f[8]; *item && l.rowCount < MH_ROWS_MAX; item = more) {
                more = strchr(item, ';'); if (more) *more++ = 0; else more = item + strlen(item);
                l.rowPos[l.rowCount] = item; r = strchr(item, ':'); *r++ = 0;
                l.rowName[l.rowCount] = r; r = strchr(r, ':'); *r++ = 0; l.rowValue[l.rowCount++] = r;
            }
            l.mirror = atoi(f[9]); l.toast = atoi(f[10]); l.toastIcon = atoi(f[11]);
            l.toastTag = f[12]; l.toastTitle = f[13]; l.toastSub = FIELD(14);
            l.jukebox = atoi(f[15]); l.jukeboxOpen = atoi(f[16]); l.jukeboxLine = f[17]; l.jukeboxTime = f[18];
            l.jukeboxTitle = f[19]; l.jukeboxArtist = f[20]; l.jukeboxState = f[21];
            l.integrity = f[22]; l.armor = FIELD(23);
            l.minimap = atoi(f[24]); l.minimapSize = 176; l.slipstream = atoi(f[25]); l.slipValue = f[26];
            l.speed = f[27]; l.unit = f[28]; l.gear = f[29]; l.nos = f[30]; l.fuel = f[31];
            if (n > 32) l.vehicleWidth = atof(f[32]);
            for (item = n > 33 ? f[33] : ""; *item && l.slotCount < MH_SLOTS_MAX; item = more) {
                more = strchr(item, ';'); if (more) *more++ = 0; else more = item + strlen(item);
                l.slotLabel[l.slotCount] = item; r = strchr(item, ':'); *r++ = 0;
                l.slotValue[l.slotCount] = r; r = strchr(r, ':'); *r++ = 0; l.slotName[l.slotCount++] = r;
            }
            if (n > 34 && *f[34]) {   /* ticks:left:right */
                l.progress = 1; l.progressTicks = atoi(f[34]); r = strchr(f[34], ':'); *r++ = 0;
                l.progressLeft = r; r = strchr(r, ':'); *r++ = 0; l.progressRight = r;
            }
            if (n > 35) l.posLabel = FIELD(35);
            for (i = 0, item = n > 36 ? f[36] : ""; *item && i < MH_KV_MAX; i++, item = more) {   /* value sizes: 17,17,14 */
                more = strchr(item, ','); if (more) *more++ = 0; else more = item + strlen(item);
                l.kvSize[i] = atof(item);
            }
            for (item = n > 37 ? f[37] : ""; *item && l.teamCount < MH_TEAMS_MAX; item = more) {   /* teams: name:value;.. */
                more = strchr(item, ';'); if (more) *more++ = 0; else more = item + strlen(item);
                l.teamName[l.teamCount] = item; r = strchr(item, ':'); *r++ = 0; l.teamValue[l.teamCount++] = r;
            }
            if (n > 38) l.rowSwatch = atoi(f[38]);
            for (item = n > 39 ? f[39] : ""; *item && l.killCount < MH_KILLS_MAX; item = more) {   /* kills: a:weapon:b;.. */
                more = strchr(item, ';'); if (more) *more++ = 0; else more = item + strlen(item);
                l.killA[l.killCount] = item; r = strchr(item, ':'); *r++ = 0;
                l.killWeapon[l.killCount] = r; r = strchr(r, ':'); *r++ = 0; l.killB[l.killCount++] = r;
            }
            if (n > 41) l.posSize = atof(f[41]);
            if (n > 42) l.rowBars = atoi(f[42]);
            if (n > 43 && *f[43]) l.derbyStatus = f[43];
            if (n > 40 && *f[40]) {   /* elimination: label;value;caption */
                l.elim = 1; l.elimLabel = f[40]; r = strchr(f[40], ';'); *r++ = 0;
                l.elimValue = r; r = strchr(r, ';'); *r++ = 0; l.elimCaption = r;
            }
#undef FIELD
            uis.glconfig.vidWidth = l.viewWidth; uis.glconfig.vidHeight = 900;
            MH_RaceLayout(&l);
#define T(name, t) printf("\"%s\":[%g,%g,%g,%g],", name, (t).box.x, (t).box.y, (t).box.w, (t).box.h)
            printf("{");
            rect("card", l.card); rect("sep", l.sep); T("title", l.titleT); T("posLabel", l.posLabelT); T("pos", l.posT); T("of", l.ofT);
            T("lapLabel", l.lapLabelT); T("lapValue", l.lapValueT);
            rect("derbyCar", l.derbyCar); T("derbyStatus", l.derbyStatusT);
            for (i = 0; i < 3; i++) { char k[24]; sprintf(k, "derbySwatch%d", i); rect(k, l.derbySwatch[i]);
                sprintf(k, "derbyLegend%d", i); T(k, l.derbyLegendT[i]); }
            rect("elimBar", l.elimBar); T("elimLabel", l.elimLabelT); T("elimValue", l.elimValueT); T("elimCaption", l.elimCaptionT);
            rect("progressBar", l.progressBar); T("progressLeft", l.progressLeftT); T("progressRight", l.progressRightT);
            for (i = 0; i < l.kvCount; i++) { char k[16]; sprintf(k, "key%d", i); T(k, l.kvKeyT[i]); sprintf(k, "value%d", i); T(k, l.kvValueT[i]); }
            rect("order", l.order); T("orderTitle", l.orderTitleT); T("orderRight", l.orderRightT);
            for (i = 0; i < l.rowCount; i++) { char k[16]; sprintf(k, "row%d", i); rect(k, l.row[i]);
                sprintf(k, "rowPos%d", i); T(k, l.rowPosT[i]); sprintf(k, "rowName%d", i); T(k, l.rowNameT[i]);
                sprintf(k, "rowValue%d", i); T(k, l.rowValueT[i]); sprintf(k, "rowSwatch%d", i); rect(k, l.rowSwatchBox[i]); sprintf(k, "rowBar%d", i); rect(k, l.rowBar[i]); }
            for (i = 0; i < l.killCount; i++) { char k[16]; sprintf(k, "kill%d", i); rect(k, l.killBox[i]);
                sprintf(k, "killA%d", i); T(k, l.killAT[i]); sprintf(k, "killWeapon%d", i); T(k, l.killWeaponT[i]);
                sprintf(k, "killB%d", i); T(k, l.killBT[i]); }
            for (i = 0; i < l.teamCount; i++) { char k[16]; sprintf(k, "team%d", i); rect(k, l.teamCell[i]);
                sprintf(k, "teamName%d", i); T(k, l.teamNameT[i]); sprintf(k, "teamValue%d", i); T(k, l.teamValueT[i]); }
            rect("mirror", l.mirrorBox); rect("toast", l.toastBox); rect("toastIcon", l.toastIconBox);
            T("toastTag", l.toastTagT); T("toastTitle", l.toastTitleT); T("toastSub", l.toastSubT);
            rect("jukebox", l.jukeboxBox); rect("jukeboxIcon", l.jukeboxIcon); rect("jukeboxBar", l.jukeboxBar);
            T("jukeboxLine", l.jukeboxLineT); T("jukeboxTime", l.jukeboxTimeT); T("jukeboxState", l.jukeboxStateT);
            T("jukeboxTitle", l.jukeboxTitleT); T("jukeboxArtist", l.jukeboxArtistT); T("jukeboxModes", l.jukeboxModesT);
            for (i = 0; i < 3; i++) { char k[16]; sprintf(k, "jukeboxButton%d", i); rect(k, l.jukeboxButton[i]); }
            rect("vehicle", l.vehicle); rect("integrityBar", l.integrityBar); T("integrityLabel", l.integrityLabel);
            T("integrity", l.integrityT); T("armorLabel", l.armorLabel); T("armor", l.armorT); rect("vehicleSep", l.vehicleSep);
            for (i = 0; i < l.slotCount; i++) { char k[16]; sprintf(k, "slotLabel%d", i); T(k, l.slotLabelT[i]);
                sprintf(k, "slotIcon%d", i); rect(k, l.slotIcon[i]); sprintf(k, "slotValue%d", i); T(k, l.slotValueT[i]);
                sprintf(k, "slotName%d", i); T(k, l.slotNameT[i]); }
            rect("minimap", l.minimapBox); rect("minimapView", l.minimapView);
            rect("slip", l.slip); rect("slipBar", l.slipBar); T("slipLabel", l.slipLabel); T("slipValue", l.slipValueT);
            rect("cockpit", l.cockpit); rect("gearBox", l.gearBox); rect("rpm", l.rpm); rect("nosBar", l.nosBar); rect("fuelBar", l.fuelBar);
            T("speed", l.speedT); T("unit", l.unitT); T("gearLabel", l.gearLabel); T("gear", l.gearT);
            T("nosLabel", l.nosLabel); T("nosValue", l.nosValue); T("fuelLabel", l.fuelLabel); T("fuelValue", l.fuelValue);
            printf("\"name0\":\"%s\"}\n", l.rowNameT[0].text);
#undef T
        } else if (!strcmp(command, "garage")) {
            /* garage <width> <height> <view3d>, then lines: vehicle, status, profile status,
               the pickers (count index lockedBefore lockedCurrent lockedAfter, five times),
               the slots (used image|load text, four lines) */
            static garageLayout_t l; static char vehicle[128], status[512], profile[512], load[4][128];
            float w, h; int view3d, i, c;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %d\n", &w, &h, &view3d);
            fgets(vehicle, sizeof(vehicle), stdin); vehicle[strcspn(vehicle, "\n")] = 0;
            fgets(status, sizeof(status), stdin); status[strcspn(status, "\n")] = 0;
            fgets(profile, sizeof(profile), stdin); profile[strcspn(profile, "\n")] = 0;
            for (i = 0; i < GARAGE_PICKERS; i++) {
                int a, b, d; scanf("%d %d %d %d %d", &l.picker[i].count, &l.picker[i].index, &a, &b, &d);
                l.picker[i].locked[0] = a; l.picker[i].locked[1] = b; l.picker[i].locked[2] = d;
            }
            scanf("\n");
            for (i = 0; i < GARAGE_SLOTS; i++) {
                int used, image; char *bar;
                fgets(load[i], sizeof(load[i]), stdin); load[i][strcspn(load[i], "\n")] = 0;
                sscanf(load[i], "%d %d", &used, &image); bar = strchr(load[i], '|');
                l.slot[i].used = used; l.slot[i].image = image; l.slot[i].load = bar ? bar + 1 : "Empty";
            }
            l.vehicle = vehicle; l.status = status; l.profileStatus = profile; l.view3d = view3d;
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            Garage_Layout(&l, w, h);
            printf("{");
            rect("back", l.back); rect("showroom", l.showroom); rect("config", l.config); rect("favorites", l.favorites);
            rect("image", l.image); rect("toggle", l.toggle); rect("apply", l.apply); rect("link", l.link); rect("pull", l.pull);
            printf("\"y\":[%g,%g,%g,%g,%g,%g,%g,%g,%g],\"lines\":[%d,%d,%d],\"page\":%g,\"pickers\":[",
                l.showroomLabelY, l.vehicleY, l.configLabelY, l.configH2Y, l.favoritesLabelY, l.favoritesH2Y, l.favoritesTextY,
                l.profileStatusY, l.statusY, l.vehicleLines.count, l.statusLines.count, l.profileStatusLines.count, l.pageHeight);
            for (i = 0; i < GARAGE_PICKERS; i++) {
                garagePicker_t *p = &l.picker[i];
                printf("%s{\"label\":%g,\"box\":[%g,%g,%g,%g],\"prev\":[%g,%g,%g,%g],\"next\":[%g,%g,%g,%g],\"cards\":[", i ? "," : "",
                    p->labelY, p->box.x, p->box.y, p->box.w, p->box.h, p->previous.x, p->previous.y, p->previous.w, p->previous.h,
                    p->next.x, p->next.y, p->next.w, p->next.h);
                for (c = 0; c < 3; c++) {
                    garageCard_t *k = &p->card[c];
                    printf("%s[[%g,%g,%g,%g],[%g,%g,%g,%g],%g,%g,%g,%g,%g]", c ? "," : "", k->box.x, k->box.y, k->box.w, k->box.h,
                        k->image.x, k->image.y, k->image.w, k->image.h, k->nameX, k->nameW, k->nameY, k->lockY, k->countY);
                }
                printf("]}");
            }
            printf("],\"slots\":[");
            for (i = 0; i < GARAGE_SLOTS; i++) {
                garageSlot_t *s = &l.slot[i];
                printf("%s{\"box\":[%g,%g,%g,%g],\"label\":%g,\"picture\":[%g,%g,%g,%g],\"load\":[%g,%g,%g,%g],\"save\":[%g,%g,%g,%g],\"upload\":[%g,%g,%g,%g]}",
                    i ? "," : "", s->box.x, s->box.y, s->box.w, s->box.h, s->labelY, s->picture.x, s->picture.y, s->picture.w, s->picture.h,
                    s->loadButton.x, s->loadButton.y, s->loadButton.w, s->loadButton.h, s->save.x, s->save.y, s->save.w, s->save.h,
                    s->upload.x, s->upload.y, s->upload.w, s->upload.h);
            }
            printf("]}\n");
        } else if (!strcmp(command, "driver")) {
            /* driver <width> <height> <tab> <profile> <avatar> <dialog> <avatars>, then lines:
               name|rank|country|progress text|widest filter option|count|profile line|status,
               the four overview values (|), the stats (|), medals (earned~label~name~text~status|...), profiles (|) */
            static driverLayout_t l; static char head[1024], overview[256], stats[4096], medals[32768], profiles[2048];
            char *field[8], *r, *next; float w, h; int tab, profile, avatar, dialog, i, n;
            memset(&l, 0, sizeof(l));
            scanf("%f %f %d %d %d %d %d\n", &w, &h, &tab, &profile, &avatar, &dialog, &l.avatars);
            fgets(head, sizeof(head), stdin); head[strcspn(head, "\n")] = 0;
            fgets(overview, sizeof(overview), stdin); overview[strcspn(overview, "\n")] = 0;
            fgets(stats, sizeof(stats), stdin); stats[strcspn(stats, "\n")] = 0;
            fgets(medals, sizeof(medals), stdin); medals[strcspn(medals, "\n")] = 0;
            fgets(profiles, sizeof(profiles), stdin); profiles[strcspn(profiles, "\n")] = 0;
            for (n = 0, r = head; n < 8; n++) { field[n] = r; r = strchr(r, '|'); if (r) *r++ = 0; else r = head + strlen(head); }
            l.name = field[0]; l.rank = field[1]; l.country = field[2]; l.progressText = field[3]; l.filterText = field[4];
            l.count = field[5]; l.profileLine = field[6]; l.status = field[7];
            for (n = 0, r = overview; n < 4; n++) { l.overview[n] = r; r = strchr(r, '|'); if (r) *r++ = 0; else r = overview + strlen(overview); }
            for (n = 0, r = stats; n < DRIVER_STATS; n++) { l.stats[n] = r; r = strchr(r, '|'); if (r) *r++ = 0; else r = stats + strlen(stats); }
            for (r = medals; *r && l.medals < DRIVER_MAX_MEDALS; r = next) {
                next = strchr(r, '|'); if (next) *next++ = 0; else next = r + strlen(r);
                l.earned[l.medals] = r[0] == '1'; r += 2;
                l.medalLabel[l.medals] = r; r = strchr(r, '~'); *r++ = 0;
                l.medalName[l.medals] = r; r = strchr(r, '~'); *r++ = 0;
                l.medalText[l.medals] = r; r = strchr(r, '~'); *r++ = 0;
                l.medalStatus[l.medals++] = r;
            }
            for (r = profiles; *r && l.profiles < DRIVER_MAX_PROFILES; r = next) {
                next = strchr(r, '|'); if (next) *next++ = 0; else next = r + strlen(r);
                l.profileName[l.profiles++] = r;
            }
            l.tab = tab; l.profile = profile; l.avatar = avatar; l.dialog = dialog; l.hideScrollbar = qtrue;
            uis.glconfig.vidWidth = w; uis.glconfig.vidHeight = h;
            Driver_Layout(&l, w, h);
            printf("{");
            rect("back", l.back); rect("identity", l.identity); rect("career", l.career); rect("avatar", l.avatarBox);
            rect("filter", l.filter); rect("dialog", l.dialogBox); rect("list", l.list); rect("nameInput", l.nameInput);
            rect("submit", l.submit); rect("close", l.close);
            printf("\"tabs\":[");
            for (i = 0; i < 3; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.tabs[i].x, l.tabs[i].y, l.tabs[i].w, l.tabs[i].h);
            printf("],\"actions\":[");
            for (i = 0; i < 3; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.actions[i].x, l.actions[i].y, l.actions[i].w, l.actions[i].h);
            printf("],\"overview\":[");
            for (i = 0; i < 4; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.overviewCard[i].box.x, l.overviewCard[i].box.y, l.overviewCard[i].box.w, l.overviewCard[i].box.h);
            printf("],\"groups\":[");
            for (i = 0; i < DRIVER_GROUPS; i++) printf("%s[%g,%g,%g,%g,%g]", i ? "," : "", l.group[i].x, l.group[i].y, l.group[i].w, l.group[i].h, l.groupH2Y[i]);
            printf("],\"stats\":[");
            for (i = 0; i < DRIVER_STATS; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.statCard[i].box.x, l.statCard[i].box.y, l.statCard[i].box.w, l.statCard[i].box.h);
            printf("],\"medals\":[");
            for (i = 0; i < l.medals; i++) printf("%s[%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%d,%d]", i ? "," : "", l.medal[i].box.x, l.medal[i].box.y, l.medal[i].box.w, l.medal[i].box.h,
                l.medal[i].medal.x, l.medal[i].labelY, l.medal[i].nameY, l.medal[i].textY, l.medal[i].progressY, l.medal[i].statusY, l.medal[i].name.count, l.medal[i].text.count);
            printf("],\"fields\":[");
            for (i = 0; i < 3; i++) printf("%s[%g,%g,%g,%g,%g]", i ? "," : "", l.field[i].x, l.field[i].y, l.field[i].w, l.field[i].h, l.fieldLabelY[i]);
            printf("],\"avatars\":[");
            for (i = 0; i < l.avatars; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.avatarChoice[i].x, l.avatarChoice[i].y, l.avatarChoice[i].w, l.avatarChoice[i].h);
            printf("],\"profiles\":[");
            for (i = 0; i < l.profiles; i++) printf("%s[%g,%g,%g,%g]", i ? "," : "", l.list.x, l.list.y + l.profileButton[i].y, l.profileButton[i].w, l.profileButton[i].h);
            printf("],\"y\":{\"identity\":%g,\"label\":%g,\"name\":%g,\"rank\":%g,\"country\":%g,\"careerH2\":%g,\"progress\":%g,"
                "\"progressText\":%g,\"category\":%g,\"count\":%g,\"dialogLabel\":%g,\"dialogH2\":%g,\"profileLine\":%g,"
                "\"avatarLabel\":%g,\"create\":%g,\"nameLabel\":%g,\"help\":%g,\"status\":%g},\"identityX\":%g,\"identityW\":%g,"
                "\"page\":%g,\"dialogHeight\":%g,\"listHeight\":%g}\n",
                l.identityY, l.identityLabelY, l.nameY, l.rankY, l.countryY, l.careerH2Y, l.progressY, l.progressTextY, l.categoryY,
                l.countY, l.dialogLabelY, l.dialogH2Y, l.profileLineY, l.avatarLabelY, l.createY, l.nameLabelY, l.helpY, l.statusY,
                l.identityX, l.identityW, l.pageHeight, l.dialogHeight, l.listHeight);
        } else if (!strcmp(command, "width")) {
            wsTextStyle_t style; int font, upper; float size, spacing; char shown[256];
            scanf("%d %f %f %d %255[^\n]", &font, &size, &spacing, &upper, text);
            style.font = font; style.size = size; style.spacing = spacing; style.lineHeight = size; style.uppercase = upper;
            WS_Transform(&style, text, shown, sizeof(shown));
            printf("%g\n", WS_TextWidth(&style, shown, strlen(shown)));
        }
        fflush(stdout);
    }
    return 0;
}
'''


def compile_harness(folder, base=None):
    source = folder / 'harness.c'
    source.write_text(HARNESS.replace('BASE', json.dumps(str(base or ROOT / 'baseq3r'))))
    exe = folder / 'harness'
    command = [shutil.which('gcc') or 'gcc', '-DARCH_STRING="test"', '-I' + str(CODE), '-w', str(source),
               str(CODE / 'qcommon/q_shared.c'), str(CODE / 'qcommon/q_math.c'), '-o', str(exe), '-lm']
    subprocess.run(command, check=True)
    return exe


def run(exe, commands):
    # the native text is Latin-1, like the game's strings
    result = subprocess.run([str(exe)], input='\n'.join(commands) + '\n', capture_output=True, encoding='latin-1',
                            check=True, timeout=60)
    return result.stdout.splitlines()


def close(mine, chrome, tolerance=0.5):
    return all(abs(a - b) <= tolerance for a, b in zip(mine, chrome))


def test_native_layout_matches_the_webui_at_every_size():
    keys = sorted(FIXTURE)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        commands = []
        for key in keys:
            size, state = key.split('_')
            width, height = size.split('x')
            commands.append(f'layout {width} {height} {int(state == "prompt")} {TITLES[state]}')
        output = [json.loads(line) for line in run(exe, commands)]
    for key, mine in zip(keys, output):
        chrome = FIXTURE[key]['els']
        problems = []

        def check(name, value, reference):
            if not close(value, reference):
                problems.append((name, value, reference))

        for name in ('left', 'right', 'car'):
            check(name, mine[name], chrome[name])
        check('h1', [mine['h1']], [chrome['h1'][1]])
        check('status', mine['status'], chrome['status'][:2])
        check('title', [mine['title']], [chrome['title'][1]])
        check('percent', [mine['percent']], [chrome['percent'][1]])
        check('progress', mine['progress'], chrome['progress'][:3])
        check('vehicle', [mine['vehicle']], [chrome['vehicle'][1]])
        check('statusText', [mine['statusText']], [chrome['statusText'][1]])
        check('stages', mine['stages'], [stage[1] for stage in chrome['stages']])
        check('stageX', [mine['stageX']], [chrome['stages'][0][0]])
        check('page', [mine['page']], [FIXTURE[key]['height']])
        if key.endswith('prompt'):
            check('prompt', [mine['prompt']], [chrome['prompt'][1]])
            check('update', mine['update'], chrome['update'])
            check('skip', mine['skip'], chrome['skip'])
        lines = {(text['text'], text['family']): len(text['lines']) for text in FIXTURE[key]['texts']}
        assert mine['lines'][0] == lines[(TITLES[key.split('_')[1]], 'Barlow Condensed')], key
        assert mine['lines'][2] == lines[('Alpine', 'Barlow Condensed')], key
        assert mine['lines'][3] == lines[('01 / PREPARING YOUR RALLY', 'Share Tech Mono')], key
        if key.endswith('prompt'):
            assert mine['lines'][1] == lines[('A newer version of Q3Rally is available.', 'Barlow')], key
        assert not problems, (key, problems)


def font_of(text):
    if text['family'] == 'Share Tech Mono':
        return 0
    if text['family'] == 'Barlow Condensed':
        return 4 if text['italic'] else 3
    return 2 if text['weight'] >= 600 else 1


def test_text_runs_measure_like_chrome():
    """Advances, pair kerning, letter-spacing and text-transform."""
    runs = {}
    for key, data in FIXTURE.items():
        for text in data['texts']:
            if len(text['lines']) == 1:
                runs[(font_of(text), text['size'], text['spacing'], text['uppercase'], text['text'])] = text['lines'][0][2]
    keys = sorted(runs)
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        widths = run(exe, [f'width {f} {s} {sp} {int(u)} {t}' for f, s, sp, u, t in keys])
    assert len(keys) > 40
    for key, width in zip(keys, widths):
        assert abs(float(width) - runs[key]) < 0.1, (key, width, runs[key])


def test_font_atlases_and_art_are_complete():
    fonts = ROOT / 'baseq3r/menu/fonts'
    for name in ('sharetechmono', 'barlow', 'barlow_bold', 'barlowcondensed', 'barlowcondensed_italic'):
        data = (fonts / f'{name}.dat').read_bytes()
        magic, version, upm, ascent, descent, glyphs, kerns, sizes = struct.unpack_from('<4s7i', data)
        assert magic == b'Q3UF' and version == 2 and glyphs <= 224 and kerns <= 4096 and 1 <= sizes <= 16  # WS_MAX_SIZES
        offset = 32 + glyphs * 8 + kerns * 8
        ligatures, = struct.unpack_from('<i', data, offset)
        assert 0 <= ligatures <= 16
        offset += 4 + ligatures * 8
        for _ in range(sizes):
            size100, width, height, image = struct.unpack_from('<3i64s', data, offset)
            offset += 76 + glyphs * 24
            path = ROOT / 'baseq3r' / (image.split(b'\0')[0].decode() + '.png')
            assert path.is_file(), path
            assert path.read_bytes()[16:24] == struct.pack('>2I', width, height), path
        assert offset == len(data)
    shaders = (ROOT / 'baseq3r/scripts/q3r_webstyle.shader').read_text()
    for art in ('web_stripes', 'web_showroom', 'web_progress_track', 'web_progress_fill'):
        assert f'menu/ui/{art}\n' in shaders
        assert (ROOT / f'baseq3r/menu/ui/{art}.png').is_file()
    for license in ('Barlow-OFL.txt', 'BarlowCondensed-OFL.txt', 'ShareTechMono-OFL.txt'):
        assert (fonts / license).is_file()


def test_font_data_of_version_1_still_shows_text():
    """A game whose menu/fonts predate the ligatures (version 1) must still
    draw its text, only without ligatures."""
    fonts = ROOT / 'baseq3r/menu/fonts'
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        base = pathlib.Path(directory) / 'base'
        (base / 'menu/fonts').mkdir(parents=True)
        for name in ('sharetechmono', 'barlow', 'barlow_bold', 'barlowcondensed', 'barlowcondensed_italic'):
            data = bytearray((fonts / f'{name}.dat').read_bytes())
            magic, version, upm, ascent, descent, glyphs, kerns, sizes = struct.unpack_from('<4s7i', data)
            offset = 32 + glyphs * 8 + kerns * 8
            ligatures, = struct.unpack_from('<i', data, offset)
            del data[offset:offset + 4 + ligatures * 8]
            struct.pack_into('<i', data, 4, 1)
            (base / f'menu/fonts/{name}.dat').write_bytes(bytes(data))
        exe = compile_harness(pathlib.Path(directory), base)
        old, = run(exe, ['width 1 16 0 0 Heijsters'])
    with tempfile.TemporaryDirectory(dir=ROOT / 'tests') as directory:
        exe = compile_harness(pathlib.Path(directory))
        new, = run(exe, ['width 1 16 0 0 Heijsters'])
    assert float(old) > 0 and float(new) > 0 and float(old) != float(new)
