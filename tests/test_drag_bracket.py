"""The drag tournament's bracket (engine/code/cgame/cg_drag_bracket.c).

Matches in play order: the rounds up to the semi-finals, third place
(size - 2), the final (size - 1); the server sends which of the two won each.
"""
import pathlib
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'engine/code/cgame/cg_drag_bracket.c'

DRIVER = r"""
#include <stdio.h>
#include <string.h>
#include "cg_drag_bracket.h"

int main(void) {
    cgDragBracket_t b;
    int size, played, winner[16], i, a, c, path[8], n;
    char cmd[16];
    while (scanf("%15s", cmd) == 1) {
        if (!strcmp(cmd, "build")) { scanf("%d %d", &size, &played); for (i = 0; i < played; i++) scanf("%d", &winner[i]);
            CG_BracketBuild(&b, size, played, winner); printf("%d\n", b.rounds); }
        else if (!strcmp(cmd, "match")) { scanf("%d", &i); CG_BracketEntrants(&b, i, &a, &c);
            printf("%d %d %s %s %d\n", a, c, CG_BracketMatchName(&b, i), CG_BracketMatchShort(&b, i), CG_BracketRound(&b, i)); }
        else if (!strcmp(cmd, "path")) { scanf("%d", &i); n = CG_BracketPath(&b, i, path, 8);
            for (a = 0; a < n; a++) printf(a ? " %d" : "%d", path[a]); printf("\n"); }
        else if (!strcmp(cmd, "place")) { scanf("%d", &i); printf("%d\n", CG_BracketPlace(&b, i)); }
        else if (!strcmp(cmd, "head")) { scanf("%d", &i); printf("%s\n", CG_BracketRoundHead(&b, i)); }
    }
    return 0;
}
"""


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


def test_eight_drivers_after_the_quarter_finals():
    # QF: 0 beats 1, 3 beats 2, 4 beats 5, 7 beats 6
    out = run(['build 8 4 0 1 0 1', 'match 4', 'match 5', 'match 6', 'match 7', 'head 0', 'head 1', 'head 2'])
    assert out == ['3', '0 3 SEMI-FINAL SF 1', '4 7 SEMI-FINAL SF 1', '-1 -1 THIRD PLACE 3RD 2', '-1 -1 FINAL F 2',
                   'QUARTER-FINALS', 'SEMI-FINALS', 'FINAL']


def test_a_drivers_way_ends_with_the_match_it_waits_for_or_where_it_went_out():
    out = run(['build 8 4 0 1 0 1', 'path 0', 'path 1', 'path 6'])
    assert out == ['3', '0 4', '0', '3']


def test_a_lost_semi_final_goes_on_to_third_place_and_the_places_come_out_at_the_end():
    # semis: 0 beats 3, 7 beats 4; third: 4 beats 3; final: 7 beats 0
    out = run(['build 8 8 0 1 0 1 0 1 1 1', 'match 6', 'match 7', 'path 3', 'path 0',
               'place 7', 'place 0', 'place 4', 'place 3', 'place 1', 'place 5'])
    assert out == ['3', '3 4 THIRD PLACE 3RD 2', '0 7 FINAL F 2', '1 4 6', '0 4 7', '1', '2', '3', '4', '5', '5']


def test_no_places_before_the_final():
    out = run(['build 8 7 0 1 0 1 0 1 1', 'place 7', 'path 7'])
    assert out == ['3', '0', '3 5 7']


def test_sixteen_and_four_drivers():
    out = run(['build 16 8 0 0 0 0 0 0 0 0', 'match 8', 'match 11', 'match 14', 'head 0', 'place 0', 'path 2',
               'build 4 0', 'match 0', 'match 2', 'match 3', 'head 0', 'head 1'])
    assert out == ['4', '0 2 QUARTER-FINAL QF 1', '12 14 QUARTER-FINAL QF 1', '-1 -1 THIRD PLACE 3RD 3', 'ROUND OF 16', '0',
                   '1 8', '2', '0 1 SEMI-FINAL SF 0', '-1 -1 THIRD PLACE 3RD 1', '-1 -1 FINAL F 1', 'SEMI-FINALS', 'FINAL']
