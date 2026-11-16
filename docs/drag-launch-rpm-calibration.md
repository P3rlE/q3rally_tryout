# Drag launch RPM guidance

The HUD's **4,000–5,000 RPM** launch band is the sweet spot of the Drag launch
physics on dry asphalt. It is a visual aid, not a scoring window: entering it
adds no torque or bonus, and first gear must still be selected on green. What
makes the band matter is the physics around it (Drag only, see
[the mode notes](drag-race.md#launch-bog-and-smoke)):

- below 3,600 RPM a loaded engine bogs (torque falls towards 30 % at idle);
- a heavier flywheel dump (0.8 kg m²) spins the rear tyres, and past 245 qu/s
  of slip part of the drive goes up in smoke.

The constants are the `DRAG_*` block at the top of
`engine/code/game/bg_wheel_forces.c`.

## Measuring

`tools/drag/launch_sweep.py` compiles the production movement code (the shared
`bg_*` sources, as the cgame does) into a sweep on a flat dry plane. Like the
game with `pmove_fixed 1` and `pmove_msec 18`, it rounds every command time up
to a whole 18 ms step, so the physics steps do not depend on the frame rate.
The car settles in N, holds the launch RPM for two seconds with an armed
engine, then engages first with full throttle. The harness shifts up at 6,000
RPM (an argument changes that); the game itself requires manual shifts.

```text
python tools/drag/launch_sweep.py all 16          # every 250 RPM, 16 ms commands
python tools/drag/launch_sweep.py 4500 8 6100     # one launch, 8 ms, shift at 6,100
python tools/drag/launch_sweep.py limiter 16      # flat out in N: time until the engine blows
```

`tests/test_drag_launch_physics.py` runs it at 8, 16 and 33 ms and checks the
shape below. Re-measure after changes to the engine curve, clutch coupling,
gearing, tyres or stock vehicle settings.

## Results

Elapsed time over the quarter mile (14,348 units), in seconds:

| Launch RPM | ET | vs. best |
| ---: | ---: | ---: |
| 1,000 | 13.161 | +0.61 |
| 2,000 | 12.917 | +0.36 |
| 3,000 | 12.697 | +0.14 |
| 3,500 | 12.585 | +0.03 |
| 4,000 | **12.555** | best |
| 4,500 | 12.568 | +0.01 |
| 5,000 | 12.587 | +0.03 |
| 5,500 | 12.624 | +0.07 |
| 6,000 | 12.677 | +0.12 |
| 6,250 | 12.689 | +0.13 |

Commands every 4, 8, 11, 16, 25 or 33 ms (250 to 30 frames per second) give the
same times within 0.01 s: with `pmove_fixed` the frame rate does not change the
physics. The Ladder requires `pmove_fixed 1` and `pmove_msec 18`. Flat out in N
with an armed engine, it blows after about 2.7 s.

An earlier calibration fed unrounded command times into the movement code and
therefore reported times that depended on the command interval; the game never
moves that way with `pmove_fixed 1`.

Servers that turn `pmove_fixed` off step the physics by the client's frame
time. For them, wheels no longer need to cross the 1.5 rad/s dead zone within
one step in Drag (only wheels turning the wrong way are stopped), so even short
steps launch from low revs.
