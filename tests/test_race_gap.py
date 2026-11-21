"""The gaps in the race order (engine/code/cgame/cg_race_gap.c).

The server reports every checkpoint a driver crosses. The gap of a driver to
the one ahead is taken at the last checkpoint both have crossed and grows
while the one ahead is past the next one, so it neither sticks at an old
value nor shows a driver who is far behind as close.
"""
import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'engine/code/cgame/cg_race_gap.c'

DRIVER = r'''
#include <stdio.h>
#include <string.h>
#include "cg_race_gap.h"

static cgRaceSplitHistory_t h[2];

int main(void) {
    char cmd[16];
    while (scanf("%15s", cmd) == 1) {
        if (!strcmp(cmd, "add")) {
            int who, lap, cp, time;
            scanf("%d %d %d %d", &who, &lap, &cp, &time);
            CG_RaceSplitAdd(&h[who], lap, cp, time);
        } else if (!strcmp(cmd, "gap")) {
            int aheadFinish, behindFinish, now, gap = 12345, found;
            scanf("%d %d %d", &aheadFinish, &behindFinish, &now);
            found = CG_RaceGap(&h[0], aheadFinish, &h[1], behindFinish, now, &gap);
            printf("%d %d\n", found, found ? gap : 0);
        } else if (!strcmp(cmd, "latest")) {
            int who;
            const cgRaceSplit_t *s;
            scanf("%d", &who);
            s = CG_RaceSplitAt(&h[who], 0);
            printf("%d %d %d %d\n", h[who].count, s->lap, s->checkpoint, s->time);
        } else if (!strcmp(cmd, "reset")) {
            memset(h, 0, sizeof(h));
        }
    }
    return 0;
}
'''

AHEAD, BEHIND = 0, 1


def gaps(script):
    """Runs the lines of `script`; returns what each "gap" or "latest" printed."""
    with tempfile.TemporaryDirectory() as directory:
        directory = pathlib.Path(directory)
        (directory / 'driver.c').write_text(DRIVER)
        exe = directory / 'driver'
        subprocess.run([shutil.which('gcc') or 'gcc', '-I' + str(SOURCE.parent), '-o', str(exe),
                        str(directory / 'driver.c'), str(SOURCE)], check=True)
        out = subprocess.run([str(exe)], input='\n'.join(script) + '\n', capture_output=True, text=True,
                             check=True).stdout
    return [tuple(int(v) for v in line.split()) for line in out.splitlines()]


def test_the_gap_at_the_last_common_checkpoint():
    # 0.5 s behind at checkpoint 1, the one ahead not yet at checkpoint 2
    assert gaps([f'add {AHEAD} 1 1 1000', f'add {BEHIND} 1 1 1500', 'gap 0 0 1600']) == [(1, 500)]


def test_the_gap_grows_while_the_one_ahead_is_past_the_next_checkpoint():
    # the one ahead crossed checkpoint 2 a second ago: the one behind is at least a second behind
    script = [f'add {AHEAD} 1 1 1000', f'add {BEHIND} 1 1 1500', f'add {AHEAD} 1 2 2000']
    assert gaps(script + ['gap 0 0 2200', 'gap 0 0 3000']) == [(1, 500), (1, 1000)]


def test_a_driver_who_was_ahead_at_the_checkpoint_has_a_negative_gap():
    # placed behind on the road, but he crossed checkpoint 1 first: the HUD shows "--"
    assert gaps([f'add {AHEAD} 1 1 1500', f'add {BEHIND} 1 1 1000', 'gap 0 0 1600']) == [(1, -500)]


def test_newer_checkpoints_of_the_one_behind_do_not_make_him_slower():
    # he is past checkpoint 2 already, the one "ahead" is not: no growing
    script = [f'add {AHEAD} 1 1 1000', f'add {BEHIND} 1 1 900', f'add {BEHIND} 1 2 1800', 'gap 0 0 9000']
    assert gaps(script) == [(1, -100)]


def test_finished_drivers_by_their_finish_times():
    script = [f'add {AHEAD} 3 1 1000', f'add {BEHIND} 3 1 1100', 'gap 50000 51250 60000']
    assert gaps(script) == [(1, 1250)]


def test_the_one_ahead_is_home():
    # home at 5 s, the one behind still racing at 9 s: at least 4 s behind
    script = [f'add {AHEAD} 1 1 1000', f'add {BEHIND} 1 1 1100', 'gap 5000 0 9000']
    assert gaps(script) == [(1, 4000)]


def test_before_the_first_checkpoint():
    assert gaps([f'add {AHEAD} 1 1 1000', 'gap 0 0 1500']) == [(1, 500)]
    assert gaps(['gap 0 0 1500']) == [(0, 0)]


def test_home_before_the_one_ahead_has_no_gap():
    assert gaps([f'add {AHEAD} 1 1 1000', f'add {BEHIND} 1 1 900', 'gap 0 4000 5000']) == [(0, 0)]


def test_the_history_keeps_the_last_32_and_replaces_a_repeat():
    script = [f'add {AHEAD} 1 {cp} {cp * 100}' for cp in range(1, 41)]
    script += [f'latest {AHEAD}', f'add {AHEAD} 1 40 9999', f'latest {AHEAD}']
    assert gaps(script) == [(32, 1, 40, 4000), (32, 1, 40, 9999)]
