"""The race result for the end screen (engine/code/cgame/cg_race_result.c).

The client collects the laps, checkpoints and finishes the server sends
everybody and turns them into the classification, the gaps, each driver's
best lap and best sector.
"""
import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'engine/code/cgame/cg_race_result.c'

DRIVER = r'''
#include <stdio.h>
#include <string.h>
#include "cg_race_result.h"

static cgRaceResult_t d[8];

int main(void) {
    char cmd[16], text[32];
    int who, a, b, order[8], n, i;
    while (scanf("%15s", cmd) == 1) {
        if (!strcmp(cmd, "start")) { scanf("%d %d", &who, &a); CG_ResultStart(&d[who], a); }
        else if (!strcmp(cmd, "grid")) { scanf("%d %d", &who, &a); d[who].grid = a; }
        else if (!strcmp(cmd, "pos")) { scanf("%d %d", &who, &a); d[who].position = a; }
        else if (!strcmp(cmd, "split")) { scanf("%d %d %d", &who, &a, &b); CG_ResultSplit(&d[who], a, b); }
        else if (!strcmp(cmd, "lap")) { scanf("%d %d", &who, &a); CG_ResultLap(&d[who], a); }
        else if (!strcmp(cmd, "finish")) { scanf("%d %d", &who, &a); CG_ResultFinish(&d[who], a); }
        else if (!strcmp(cmd, "laps")) {
            scanf("%d", &who);
            printf("%d", d[who].lapCount);
            for (i = 0; i < d[who].lapCount; i++) printf(" %d", d[who].laps[i]);
            printf("\n");
        }
        else if (!strcmp(cmd, "best")) {
            scanf("%d", &who);
            printf("%d %d %d %d\n", CG_ResultBestLap(&d[who]), d[who].bestSector, d[who].bestSectorNum, d[who].grid);
        }
        else if (!strcmp(cmd, "order")) {
            n = CG_ResultOrder(d, 8, order);
            for (i = 0; i < n; i++) printf(i ? " %d" : "%d", order[i]);
            printf("\n");
        }
        else if (!strcmp(cmd, "gap")) {
            scanf("%d %d", &a, &who);
            CG_ResultGap(&d[a], &d[who], text, sizeof(text));
            printf("%s\n", text);
        }
        else if (!strcmp(cmd, "diff")) { scanf("%d", &a); CG_ResultDiff(a, text, sizeof(text)); printf("%s\n", text); }
        else if (!strcmp(cmd, "sectors")) {
            /* a sprint: the checkpoints, then each sector of a driver and the place at its end */
            int cps = CG_ResultCheckpoints(d, 8);
            scanf("%d", &who);
            printf("%d %d", cps, CG_ResultReached(&d[who]));
            for (i = 1; i <= cps + 1; i++) printf(" %d/%d", CG_ResultSector(&d[who], i, cps), CG_ResultPlaceAt(d, 8, who, i, cps));
            printf("\n");
        }
        else if (!strcmp(cmd, "time")) { scanf("%d", &a); CG_ResultTime(a, text, sizeof(text)); printf("%s\n", text); }
    }
    return 0;
}
'''


def run(script):
    with tempfile.TemporaryDirectory() as directory:
        directory = pathlib.Path(directory)
        (directory / 'driver.c').write_text(DRIVER)
        exe = directory / 'driver'
        subprocess.run([shutil.which('gcc') or 'gcc', '-I' + str(SOURCE.parent), '-o', str(exe),
                        str(directory / 'driver.c'), str(SOURCE)], check=True)
        out = subprocess.run([str(exe)], input='\n'.join(script) + '\n', capture_output=True, text=True,
                             check=True).stdout
    return out.splitlines()


def test_laps_and_the_best_one():
    script = ['start 0 1000', 'lap 0 54000', 'lap 0 105000', 'finish 0 157500', 'laps 0', 'best 0']
    assert run(script) == ['3 53000 51000 52500', '51000 0 0 0']


def test_nothing_counts_after_the_finish():
    assert run(['start 0 0', 'finish 0 50000', 'lap 0 99000', 'split 0 1 120000', 'laps 0']) == ['1 50000']


def test_the_best_sector_and_where_it_ends():
    # S1 12 s, S2 9 s; the next lap's S1 from the lap line: 11 s
    script = ['start 0 0', 'split 0 1 12000', 'split 0 2 21000', 'lap 0 40000', 'split 0 1 51000', 'best 0']
    assert run(script) == ['40000 9000 2 0']


def test_the_grid_survives_the_start():
    assert run(['grid 0 5', 'start 0 0', 'best 0']) == ['0 0 0 5']


def test_the_classification():
    # 2 and 0 finished (2 first); 1 did three laps, 3 one lap; 4 never started
    script = ['start 0 0', 'start 1 0', 'start 2 0', 'start 3 0',
              'lap 1 10', 'lap 1 20', 'lap 1 30', 'lap 3 10',
              'finish 0 9000', 'finish 2 8000', 'order']
    assert run(script) == ['2 0 1 3']


def test_those_not_home_on_the_same_lap_by_the_servers_position():
    script = ['start 0 0', 'start 1 0', 'pos 0 3', 'pos 1 2', 'lap 0 100', 'lap 1 110', 'order']
    assert run(script) == ['1 0']


def test_the_time_column():
    script = ['start 0 0', 'start 1 0', 'start 2 0', 'finish 0 260238', 'finish 1 261380', 'finish 2 330000',
              'gap 0 0', 'gap 0 1', 'gap 0 2', 'gap 0 3']
    assert run(script) == ['4:20.238', '+1.142', '+1:09.762', 'DNF']


def test_times():
    assert run(['time 51204', 'time 0', 'time 3600000']) == ['0:51.204', '0:00.000', '60:00.000']


def test_a_difference_is_signed():
    assert run(['diff 2070', 'diff -6140', 'diff 0', 'diff 62100', 'diff -61000']) == ['+2.070', '-6.140', '+0.000', '+1:02.100', '-1:01.000']


def test_a_sprint_by_its_sectors():
    # three drivers, two checkpoints. 0 leads to CP1, 1 passes him to CP2 and
    # wins; the finish line's own split is no checkpoint; 2 stops after CP1.
    # 1 crossed the start line first: the split there is overwritten at CP1.
    script = ['start 0 1000', 'start 1 1000', 'start 2 1000',
              'split 1 1 3000',
              'split 0 1 11000', 'split 1 1 12000', 'split 2 1 13000',
              'split 1 2 20000', 'split 0 2 22000',
              'split 1 3 30000', 'finish 1 30000', 'split 0 3 31000', 'finish 0 31000',
              'sectors 0', 'sectors 1', 'sectors 2']
    assert run(script) == ['2 2 10000/1 11000/2 9000/2',
                           '2 2 11000/2 8000/1 10000/1',
                           '2 1 12000/3 0/0 0/0']
