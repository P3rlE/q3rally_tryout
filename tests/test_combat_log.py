"""A combat match written down for the end screen (engine/code/cgame/cg_combat_log.c).

The server sends the scores and every kill; the client keeps who killed whom
and when, the streaks, who went out when and the scores at the end of each
minute.
"""
import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'engine/code/cgame/cg_combat_log.c'

DRIVER = r"""
#include <stdio.h>
#include <string.h>
#include "cg_combat_log.h"

static cgCombatLog_t log_;

int main(void) {
    char cmd[16];
    int a, t, time, i, n, c, m, score[CG_COMBAT_CLIENTS], damage[CG_COMBAT_CLIENTS], team[CG_COMBAT_TEAMS];
    CG_CombatReset(&log_, 1000);
    while (scanf("%15s", cmd) == 1) {
        if (!strcmp(cmd, "kill")) { scanf("%d %d %d", &a, &t, &time); CG_CombatKill(&log_, a, t, time); }
        else if (!strcmp(cmd, "sample")) {
            /* sample <time> <n> client score damage .. <red> <blue> */
            for (i = 0; i < CG_COMBAT_CLIENTS; i++) { score[i] = CG_COMBAT_NONE; damage[i] = 0; }
            scanf("%d %d", &time, &n);
            for (i = 0; i < n; i++) { scanf("%d", &c); scanf("%d %d", &score[c], &damage[c]); }
            for (i = 0; i < CG_COMBAT_TEAMS; i++) team[i] = 0;
            scanf("%d %d", &team[0], &team[1]);
            CG_CombatSample(&log_, time, score, damage, team);
        }
        else if (!strcmp(cmd, "who")) { scanf("%d", &c);
            printf("%d %d %d %d %d\n", log_.kills[c], log_.deaths[c], log_.bestStreak[c], log_.outAt[c], log_.outBy[c]); }
        else if (!strcmp(cmd, "nemesis")) { scanf("%d", &c); n = CG_CombatNemesis(&log_, c, &m); printf("%d %d\n", n, m); }
        else if (!strcmp(cmd, "minute")) { scanf("%d %d", &m, &c);
            printf("%d %d %d %d %d %d\n", log_.minutes, log_.minuteKills[m][c], log_.score[m][c], log_.damage[m][c],
                   CG_CombatPlaceAt(&log_, c, m), CG_CombatLeftAt(&log_, m)); }
        else if (!strcmp(cmd, "team")) { scanf("%d", &m); printf("%d %d\n", log_.team[m][0], log_.team[m][1]); }
    }
    return 0;
}
"""


def run(script):
    with tempfile.TemporaryDirectory() as directory:
        directory = pathlib.Path(directory)
        (directory / 'driver.c').write_text(DRIVER)
        exe = directory / 'driver'
        subprocess.run([shutil.which('gcc') or 'gcc', '-Wall', '-Werror', '-I' + str(SOURCE.parent), '-o', str(exe),
                        str(directory / 'driver.c'), str(SOURCE)], check=True)
        out = subprocess.run([str(exe)], input='\n'.join(script) + '\n', capture_output=True, text=True, check=True)
        return out.stdout.split('\n')[:-1]


def test_kills_deaths_streaks_and_the_nemesis():
    # 1 kills 2 twice and 3 once, then 2 gets 1; 3 falls on its own
    script = ['kill 1 2 5000', 'kill 1 2 9000', 'kill 1 3 12000', 'kill 2 1 15000', 'kill 1 2 70000', 'kill -1 3 80000',
              'who 1', 'who 2', 'who 3', 'nemesis 2', 'nemesis 1', 'nemesis 4']
    assert run(script) == ['4 1 3 14000 2',  # kills, deaths, best streak, out at, out by
                           '1 3 1 69000 1',
                           '0 2 0 79000 -1',
                           '1 3', '2 1', '-1 0']


def test_the_minutes_the_place_and_who_is_left():
    # minute 0 sampled, minute 1 not (carried on), minute 2 again; 3 out after 90 s
    script = ['kill 1 2 5000', 'kill 1 2 30000', 'kill 2 3 91000',
              'sample 20000 3 1 1 100 2 0 40 3 0 10 1 0',
              'sample 58000 3 1 2 220 2 0 60 3 0 20 2 0',
              'sample 150000 3 1 2 300 2 3 400 3 0 20 2 3',
              'minute 0 1', 'minute 0 2', 'minute 1 1', 'minute 1 3', 'minute 2 2', 'minute 2 1', 'team 1', 'team 2']
    # minute, its kills, score, damage, place, who was still in (2 last died at 0:29, 3 at 1:30)
    assert run(script) == ['3 2 2 220 1 2',
                           '3 0 0 60 2 2',
                           '3 0 2 220 1 1',
                           '3 0 0 20 2 1',
                           '3 0 3 400 1 1',
                           '3 0 2 300 2 1',
                           '2 0', '2 3']
