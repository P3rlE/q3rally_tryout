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
        /* zones <time> <n> owner..; held <minute>; longest <time>; goal <time> <team> <scorer> <km/h>; goals */
        else if (!strcmp(cmd, "zones")) { int own[CG_COMBAT_ZONES]; scanf("%d %d", &time, &n);
            for (i = 0; i < n; i++) { scanf("%d", &own[i]); }
            CG_CombatZones(&log_, time, own, n); }
        else if (!strcmp(cmd, "pause")) CG_CombatZonesPause(&log_);
        else if (!strcmp(cmd, "held")) { scanf("%d", &m);
            printf("%d %d %d %d\n", log_.minutes, log_.held[m][0], log_.held[m][1], log_.held[m][2]); }
        else if (!strcmp(cmd, "longest")) { int z, tm; scanf("%d", &time); n = CG_CombatLongest(&log_, time, &z, &tm);
            printf("%d %d %d\n", n, z, tm); }
        else if (!strcmp(cmd, "goal")) { int tm, sc, sp; scanf("%d %d %d %d", &time, &tm, &sc, &sp);
            CG_CombatGoal(&log_, time, tm, sc, sp); }
        else if (!strcmp(cmd, "goals")) { printf("%d", log_.goals);
            for (i = 0; i < CG_COMBAT_GOALS && i < log_.goals; i++) { printf(" %d:%d:%d:%d", log_.goalAt[i], log_.goalTeam[i],
                log_.goalScorer[i], log_.goalSpeed[i]); }
            printf("\n"); }
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


def test_the_hill_held_minute_by_minute_and_the_longest_reign():
    # red takes the hill at 0:00 (the match starts at 1000), blue at 1:30, nobody at 2:00
    script = ['zones 1000 1 0', 'zones 31000 1 0', 'zones 91000 1 1', 'zones 121000 1 -1',
              'held 0', 'held 1', 'longest 121000']
    assert run(script) == ['2 60000 0 0', '2 30000 30000 0', '90000 0 0']


def test_an_open_hold_counts_for_the_longest_and_a_pause_stops_the_clock():
    # blue holds zone B from 0:10; a warmup pause between 0:20 and 0:40 does not count
    script = ['zones 1000 2 0 -1', 'zones 11000 2 0 1', 'zones 21000 2 0 1', 'pause', 'zones 41000 2 0 1',
              'zones 51000 2 0 1', 'held 0', 'longest 51000']
    # red: 0..20 and 40..50 = 30 s, blue: 10..20 and 40..50 = 20 s; the longest: red's zone A, still open
    assert run(script) == ['1 30000 20000 0', '50000 0 0']


def test_the_scores_carry_on_over_minutes_the_hill_already_counted():
    # the hill pushes the minutes on; a sample in minute 3 still fills 1 and 2 with minute 0's score
    script = ['sample 20000 1 1 5 0 4 2', 'zones 1000 1 0', 'zones 200000 1 0', 'sample 190000 1 1 9 0 7 3',
              'team 1', 'team 2', 'team 3']
    assert run(script) == ['4 2', '4 2', '7 3']


def test_the_goals_the_last_ones_kept():
    script = [f'goal {1000 + i * 30000} {i % 2} {i} {80 + i}' for i in range(17)] + ['goals']
    out = run(script)[0].split()
    assert out[0] == '17'
    assert out[1] == '30000:1:1:81' and out[-1] == '480000:0:16:96' and len(out) == 17
