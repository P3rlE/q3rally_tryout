"""Run the production Drag command filter across replays and delayed snapshots."""
import pathlib
import re
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]


def test_drag_prediction_first_gear_timing():
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
    assert(cmd.forwardmove==127 && cmd.buttons==BUTTON_HANDBRAKE);
    assert(ps.stats[STAT_GEAR]==0); /* Gas can rev the disconnected engine. */

    car.dragPhase=DRAG_COUNTDOWN; car.dragGreenTime=1000;
    cmd=command(999,qtrue);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,999);
    assert(!cg_dragPrediction.launchTime && cmd.forwardmove==127 && ps.stats[STAT_GEAR]==0);
    cmd=command(1105,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,1105);
    assert(!cg_dragPrediction.launchTime && ps.stats[STAT_GEAR]==0);
    assert(cmd.buttons==BUTTON_HANDBRAKE); /* Gas/brake changes cannot launch. */
    CG_DragPredictFirstGear(&car,&ps,0,1105);
    cmd=command(1105,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,999);
    assert(cmd.forwardmove==127 && ps.commandTime==900 && ps.stats[STAT_GEAR]==0);
    assert(cg_dragPrediction.launchTime==1105); /* future cmd cannot move before green */
    cmd=command(1120,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,1110);
    assert(cmd.forwardmove==127 && ps.commandTime==1105 && ps.stats[STAT_GEAR]==1);
    assert(!(cmd.buttons&(BUTTON_ATTACK|BUTTON_USE_HOLDABLE)));

    /* Fresh render calls must retain the first gear input while the server's
       COUNTDOWN phase is still the most recently delivered status. */
    ps.commandTime=1120;
    cmd=command(1130,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,1130);
    assert(cmd.forwardmove==127 && ps.commandTime==1120);
    assert(cg_dragPrediction.launchTime==1105);

    /* Replaying from an older snapshot clamps to the same first-gear input, not the
       newest command's time. The remaining interval may accelerate. */
    ps.commandTime=900;
    cmd=command(1160,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,1160);
    assert(ps.commandTime==1105 && cmd.forwardmove==127);

    car.dragPhase=DRAG_RUNNING; car.dragLaunchTime=1110;
    ps.commandTime=900; ps.stats[STAT_GEAR]=0;
    cmd=command(1170,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,1170);
    assert(ps.commandTime==1110 && cg_dragPrediction.launchTime==1110 && ps.stats[STAT_GEAR]==1);
    ps.commandTime=1180; ps.stats[STAT_GEAR]=0;
    cmd=command(1190,qtrue);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,1190);
    assert(cmd.forwardmove==127 && (cmd.buttons&BUTTON_HANDBRAKE));
    assert(ps.stats[STAT_GEAR]==0); /* Later manual neutral selection is respected. */

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
    assert(cmd.forwardmove==127 && ps.stats[STAT_GEAR]==0 && !cg_dragPrediction.launchTime);
    assert(!ps.stats[STAT_SLIPSTREAM] && !ps.stats[STAT_HOLDABLE_ITEM] && !ps.powerups[PW_TURBO]);
    CG_DragPredictFirstGear(&car,&ps,0,2115);
    cmd=command(2115,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,0,2115);
    assert(ps.commandTime==2115);
    ps.commandTime=1900;
    ps.stats[STAT_GEAR]=0;
    CG_DragPredictFirstGear(&car,&ps,1,2125);
    cmd=command(2125,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,1,2125);
    assert(ps.commandTime==2125 && cg_dragPrediction.clientNum==1);

    /* An early first gear never turns into a valid launch when green arrives. */
    car.dragGreenTime=3000; ps.commandTime=2900; ps.stats[STAT_GEAR]=0;
    CG_DragPredictFirstGear(&car,&ps,1,2990);
    CG_DragPredictFirstGear(&car,&ps,1,3010);
    cmd=command(3020,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,1,3020);
    assert(cg_dragPrediction.launchTime==2990 && ps.commandTime==2900);
    assert(ps.stats[STAT_GEAR]==0 && cmd.buttons==BUTTON_HANDBRAKE);

    /* A launch inside the minimum reaction time is a server false start, so
       it is never predicted as a launch either; 0.100 s is still legal. */
    car.dragGreenTime=4000; ps.commandTime=3900; ps.stats[STAT_GEAR]=0;
    CG_DragPredictFirstGear(&car,&ps,1,4000+DRAG_MIN_REACTION_MS-1);
    cmd=command(4150,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,1,4150);
    assert(ps.commandTime==3900 && ps.stats[STAT_GEAR]==0 && cmd.buttons==BUTTON_HANDBRAKE);
    car.dragGreenTime=5000; ps.commandTime=4900;
    CG_DragPredictFirstGear(&car,&ps,1,5000+DRAG_MIN_REACTION_MS);
    cmd=command(5150,qfalse);
    CG_DragFilterPredictedCommand(&cmd,&ps,&car,1,5150);
    assert(ps.commandTime==5000+DRAG_MIN_REACTION_MS && ps.stats[STAT_GEAR]==1);
    return 0;
}
'''
    compiler = shutil.which("gcc") or "C:/msys64/mingw64/bin/gcc.exe"
    with tempfile.TemporaryDirectory(dir=ROOT / "tests") as directory:
        c = pathlib.Path(directory) / "drag_prediction.c"
        binary = pathlib.Path(directory) / "drag_prediction.exe"
        c.write_text(harness)
        subprocess.run([compiler, '-DARCH_STRING="test"', str(c), "-o", str(binary), "-lm"], check=True)
        subprocess.run([str(binary)], check=True, timeout=5)
