"""The heats of a drag match for the end screen (engine/code/cgame/cg_drag_log.c).

The server sends each car's state and keeps no history for the client; the
client writes every heat down, works out who won it and by how much.
"""
import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'engine/code/cgame/cg_drag_log.c'

DRIVER = r"""
#include <stdio.h>
#include <string.h>
#include "cg_drag_log.h"

static cgDragLog_t log_;

int main(void) {
    char cmd[16];
    int c, st, cd, g, l, f, t, i, m, w, heats[16], n;
    CG_DragLogReset(&log_);
    while (scanf("%15s", cmd) == 1) {
        if (!strcmp(cmd, "status")) { scanf("%d %d %d %d %d %d %d", &c, &st, &cd, &g, &l, &f, &t);
            CG_DragLogStatus(&log_, c, (cgDragState_t)st, cd, g, l, f, t); }
        else if (!strcmp(cmd, "split")) { scanf("%d %d %d %d", &c, &cd, &i, &m); CG_DragLogSplit(&log_, c, cd, i, m); }
        else if (!strcmp(cmd, "heats")) { printf("%d\n", log_.count); }
        else if (!strcmp(cmd, "run")) { scanf("%d %d", &i, &c);
            { const cgDragRun_t *r = CG_DragLogRun(&log_.heat[i], c);
              if (!r) printf("none\n");
              else printf("%d %d %d %d %d %d %d %d\n", r->state, r->rt, r->et, r->total, r->trap, r->split[0], r->split[1], r->split[2]); } }
        else if (!strcmp(cmd, "winner")) { scanf("%d", &i); w = CG_DragLogWinner(&log_.heat[i], &m); printf("%d %d\n", w, m); }
        else if (!strcmp(cmd, "of")) { scanf("%d", &c); n = CG_DragLogHeatsOf(&log_, c, heats, 16);
            for (i = 0; i < n; i++) printf(i ? " %d" : "%d", heats[i]); printf("\n"); }
    }
    return 0;
}
"""
RUNNING, DONE, FOUL, OUT = range(4)


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


def test_a_heat_is_written_down_with_reaction_elapsed_and_total():
    out = run([f'status 3 {RUNNING} 1000 0 0 0 0', f'status 3 {RUNNING} 1000 5000 5412 0 0',
               'split 3 1000 0 1982', 'split 3 1000 1 5104', 'split 3 1000 1 9999',
               f'status 3 {DONE} 1000 5000 5412 16894 1874', 'heats', 'run 0 3'])
    assert out == ['1', f'{DONE} 412 11482 11894 1874 1982 5104 0']


def test_the_first_over_the_line_wins_by_the_gap_in_total_time():
    out = run([f'status 3 {DONE} 1000 5000 5412 16894 1874', f'status 7 {DONE} 1000 5000 5388 16977 1849',
               'winner 0'])
    assert out == ['0 83']


def test_a_foul_or_a_car_that_does_not_finish_loses():
    out = run([f'status 3 {FOUL} 1000 0 900 0 0', f'status 7 {DONE} 1000 5000 5390 17057 1840', 'winner 0',
               f'status 3 {OUT} 2000 6000 6400 0 0', f'status 7 {DONE} 2000 6000 6390 18000 1840', 'winner 1',
               f'status 3 {FOUL} 3000 0 900 0 0', f'status 7 {FOUL} 3000 0 950 0 0', 'winner 2'])
    assert out == ['1 -1', '1 -1', '-1 -1']


def test_a_dead_heat_has_no_winner_and_heats_are_kept_apart_by_their_countdown():
    out = run([f'status 3 {DONE} 1000 5000 5400 17000 1800', f'status 7 {DONE} 1000 5000 5300 17000 1800', 'winner 0',
               f'status 3 {DONE} 30000 34000 34400 46000 1800', f'status 9 {DONE} 30000 34000 34300 46100 1800',
               'heats', 'of 3', 'of 7', 'of 9', 'run 1 7'])
    assert out == ['-1 -1', '2', '0 1', '0', '1', 'none']


def test_alone_on_the_strip_a_finished_run_wins_its_heat():
    out = run([f'status 3 {DONE} 1000 5000 5400 17000 1800', 'winner 0', f'status 3 {FOUL} 2000 0 900 0 0', 'winner 1'])
    assert out == ['0 -1', '-1 -1']


def test_a_car_staged_and_gone_before_green_was_never_in_the_heat():
    out = run([f'status 9 {RUNNING} 1000 0 0 0 0', f'status 3 {DONE} 1000 5000 5400 17000 1800', 'winner 0',
               f'status 9 {RUNNING} 2000 6000 6300 0 0', f'status 3 {DONE} 2000 6000 6400 18000 1800', 'winner 1'])
    assert out == ['1 -1', '-1 -1']
