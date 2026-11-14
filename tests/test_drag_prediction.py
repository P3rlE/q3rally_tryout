"""Run the production Drag command filter across replays and delayed snapshots."""
import pathlib
import re
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]


def test_drag_prediction_release_timing():
    source = (ROOT / "engine/code/cgame/cg_predict.c").read_text()
    start = source.index("typedef struct {\n\tint clientNum;\n\tint greenTime;")
    function = source.index("static void CG_DragFilterPredictedCommand(", start)
    masked = re.sub(r"/\*.*?\*/|//[^\n]*", lambda match: " " * len(match.group()), source, flags=re.S)
    opening = masked.index("{", function)
    depth, end = 1, opening + 1
    while depth:
        depth += (masked[end] == "{") - (masked[end] == "}")
        end += 1
    header = (ROOT / "engine/code/cgame/cg_local.h").as_posix()
    harness = '#include "' + header + '"\n#include <assert.h>\n' + source[start:end]
    harness += r'''
static usercmd_t command(int stamp, qboolean brake) {
    usercmd_t cmd;
    memset(&cmd,0,sizeof(cmd));
    cmd.serverTime=stamp; cmd.forwardmove=127;
    cmd.buttons=(brake ? BUTTON_HANDBRAKE : 0)|BUTTON_ATTACK|BUTTON_USE_HOLDABLE;
    return cmd;
}
int main(void) {
    static playerState_t ps;
    static centity_t car;
    usercmd_t cmd;
    int phase;

    ps.commandTime=900;
    cmd=command(950,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,950);
    assert(cmd.forwardmove==0 && cmd.buttons==BUTTON_HANDBRAKE);

    car.dragPhase=DRAG_COUNTDOWN; car.dragGreenTime=1000;
    cmd=command(999,qtrue);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,999);
    assert(!cg_dragPrediction.launchTime && cmd.forwardmove==0);
    cmd=command(1005,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,999);
    assert(cmd.forwardmove==0 && ps.commandTime==900);
    assert(cg_dragPrediction.launchTime==1005); /* future cmd cannot move before green */
    cmd=command(1020,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,1010);
    assert(cmd.forwardmove==127 && ps.commandTime==1005);
    assert(!(cmd.buttons&(BUTTON_ATTACK|BUTTON_USE_HOLDABLE)));

    /* Fresh render calls must retain the first release while the server's
       COUNTDOWN phase is still the most recently delivered status. */
    ps.commandTime=1020;
    cmd=command(1030,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,1030);
    assert(cmd.forwardmove==127 && ps.commandTime==1020);
    assert(cg_dragPrediction.launchTime==1005);

    /* Replaying from an older snapshot clamps to the same release, not the
       newest command's time. The remaining interval may accelerate. */
    ps.commandTime=900;
    cmd=command(1060,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,1060);
    assert(ps.commandTime==1005 && cmd.forwardmove==127);

    car.dragPhase=DRAG_RUNNING; car.dragLaunchTime=1010;
    ps.commandTime=900;
    cmd=command(1070,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,1070);
    assert(ps.commandTime==1010 && cg_dragPrediction.launchTime==1010);
    ps.commandTime=1080;
    cmd=command(1090,qtrue);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,1090);
    assert(cmd.forwardmove==127 && (cmd.buttons&BUTTON_HANDBRAKE));

    for(phase=DRAG_FINISHED;phase<=DRAG_DNF;phase++) {
        car.dragPhase=phase;
        cmd=command(1200,qfalse);
        CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,1200);
        assert(cmd.forwardmove==0 && cmd.buttons==BUTTON_HANDBRAKE);
    }

    /* Restaging/new heats and following a different client discard the old
       predicted launch. Authoritative inventory restrictions apply too. */
    car.dragPhase=DRAG_COUNTDOWN; car.dragGreenTime=2000; car.dragLaunchTime=0;
    ps.commandTime=1900;
    ps.stats[STAT_SLIPSTREAM]=255; ps.stats[STAT_HOLDABLE_ITEM]=1;
    ps.powerups[PW_TURBO]=9999;
    cmd=command(2000,qtrue);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,2000);
    assert(cmd.forwardmove==0 && !cg_dragPrediction.launchTime);
    assert(!ps.stats[STAT_SLIPSTREAM] && !ps.stats[STAT_HOLDABLE_ITEM] && !ps.powerups[PW_TURBO]);
    cmd=command(2015,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,2015);
    assert(ps.commandTime==2015);
    ps.commandTime=1900;
    cmd=command(2025,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,1,2025);
    assert(ps.commandTime==2025 && cg_dragPrediction.clientNum==1);
    return 0;
}
'''
    compiler = shutil.which("gcc") or "C:/msys64/mingw64/bin/gcc.exe"
    with tempfile.TemporaryDirectory(dir=ROOT / "tests") as directory:
        c = pathlib.Path(directory) / "drag_prediction.c"
        binary = pathlib.Path(directory) / "drag_prediction.exe"
        c.write_text(harness)
        subprocess.run([compiler, str(c), "-o", str(binary), "-lm"], check=True)
        subprocess.run([str(binary)], check=True, timeout=5)
