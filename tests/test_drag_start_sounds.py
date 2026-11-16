"""Exercise the production Drag sound transitions and shipped PCM assets."""
import struct
import wave

from test_plate_and_menu_regressions import CODE, ROOT, compile_run, function


def test_drag_start_tones_follow_transitions_without_replaying_stale_starts():
    source = (CODE / "cgame/cg_rally_racetools.c").read_text()
    begin = source.index("static int s_dragSoundHeat;")
    end = source.index("void CG_ResetDragStartSounds", begin)
    harness = r'''
#include "cgame/cg_local.h"
#include <assert.h>
cg_t cg;
cgs_t cgs;
vmCvar_t cg_startLightPreview;
static int heat, announced, reds, emitted, countdowns, greens;
static qboolean green;
int CG_DragStartLightState(int *r,qboolean *g,int *t) { *r=reds; *g=green; *t=announced; return heat; }
void trap_S_StartLocalSound(sfxHandle_t sound,int channel) {
    assert(channel==CHAN_ANNOUNCER);
    if(sound==17) countdowns++;
    else { assert(sound==23); greens++; }
    emitted++;
}
'''
    harness += source[begin:end]
    harness += function(source, "CG_ResetDragStartSounds") + "\n"
    harness += function(source, "CG_UpdateDragStartSounds") + "\n"
    harness += r'''
/* heat: countdown start; at: announced green time, 0 before green. */
static void frame(int time,int target,int at,int r,qboolean g) {
    cg.time=time; heat=target; announced=at; reds=r; green=g;
    CG_UpdateDragStartSounds();
}
static void reset(void) {
    CG_ResetDragStartSounds(); emitted=countdowns=greens=0;
    cgs.gametype=GT_DRAG; cg_startLightPreview.integer=0;
}
int main(void) {
    static snapshot_t snapshot;
    cg.snap=&snapshot;
    cgs.media.dragCountdownSound=17; cgs.media.dragGreenSound=23;
    reset();
    frame(5000,5000,0,1,qfalse); assert(countdowns==1);
    frame(5000,5000,0,1,qfalse); assert(countdowns==1); /* stereo */
    frame(5999,5000,0,1,qfalse); assert(countdowns==1);
    frame(6000,5000,0,2,qfalse); assert(countdowns==2);
    frame(7000,5000,0,3,qfalse); assert(countdowns==3);
    frame(9999,5000,0,3,qfalse); assert(emitted==3); /* unknown final wait */
    frame(10000,5000,9950,0,qtrue); assert(greens==1); /* announced green */
    frame(10000,5000,9950,0,qtrue); frame(10001,5000,9950,0,qtrue);
    frame(14001,5000,9950,0,qfalse); assert(emitted==4);

    reset();
    frame(10001,5000,10000,0,qtrue); frame(10020,5000,10000,0,qtrue);
    assert(!emitted); /* new viewer after green */
    frame(15000,5000,10000,0,qfalse); assert(!emitted);

    reset();
    frame(9990,5000,0,3,qfalse);
    frame(10250,5000,10000,0,qtrue); assert(greens==1);
    reset();
    frame(9990,5000,0,3,qfalse);
    frame(10251,5000,10000,0,qtrue); assert(!greens); /* pause/forward seek */

    reset();
    frame(9000,5000,0,3,qfalse);
    frame(8500,5000,0,2,qfalse); /* backwards seek */
    frame(9000,5000,0,3,qfalse); frame(10000,5000,10000,0,qtrue);
    assert(countdowns==1 && !greens);
    frame(17000,17000,0,1,qfalse); assert(countdowns==2); /* next heat */

    reset();
    frame(5000,5000,0,1,qfalse);
    frame(5100,0,0,0,qfalse); /* cancelled */
    frame(6000,6000,0,1,qfalse); assert(countdowns==2);
    CG_ResetDragStartSounds(); /* map_restart */
    frame(6000,6000,0,1,qfalse); assert(countdowns==3);

    reset();
    cg_startLightPreview.integer=4;
    frame(5000,5000,0,1,qfalse); frame(10000,5000,10000,0,qtrue);
    assert(!emitted);
    cg_startLightPreview.integer=0;
    frame(10010,5000,10000,0,qtrue); assert(!emitted);
    reset(); cgs.gametype=GT_RACING;
    frame(5000,5000,0,1,qfalse); assert(!emitted);
    reset(); cg.snap=NULL;
    frame(5000,5000,0,1,qfalse); assert(!emitted);
    return 0;
}
'''
    compile_run(harness)


def test_drag_signal_assets_are_short_distinct_mono_pcm_tones():
    for name, duration, frequency in (("drag_countdown", .11, 660), ("drag_green", .27, 1320)):
        with wave.open(str(ROOT / "baseq3r/sound/feedback" / (name + ".wav")), "rb") as sound:
            assert sound.getnchannels() == 1
            assert sound.getsampwidth() == 2
            assert sound.getframerate() == 22050
            assert sound.getcomptype() == "NONE"
            assert abs(sound.getnframes() / sound.getframerate() - duration) < .0001
            samples = struct.unpack("<" + "h" * sound.getnframes(), sound.readframes(sound.getnframes()))
        assert samples[0] == samples[-1] == 0  # faded to avoid clicks
        assert 5000 < max(samples) < 10000  # audible headroom, no clipping
        crossings = sum(a <= 0 < b for a, b in zip(samples, samples[1:]))
        assert abs(crossings / duration - frequency) < 10


def test_drag_audio_updates_in_main_frame_independently_of_hud_and_world_rendering():
    source = (CODE / "cgame/cg_view.c").read_text()
    frame = function(source, "CG_DrawActiveFrame")
    assert frame.count("CG_UpdateDragStartSounds();") == 1
    assert frame.index("CG_ProcessSnapshots();") < frame.index("CG_UpdateDragStartSounds();")
    assert frame.index("CG_UpdateDragStartSounds();") < frame.index("CG_PredictPlayerState();")
    assert frame.index("CG_UpdateDragStartSounds();") < frame.index("if ( !cg.hyperspace )")
