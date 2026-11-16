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
`bg_*` sources, as the cgame does) into a sweep on a flat dry plane. It runs
`pmove_fixed 1` with `pmove_msec 18`, like the game. The car settles in N,
holds the launch RPM for two seconds with an armed engine, then engages first
with full throttle. The harness shifts up at 6,000 RPM (an argument changes
that); the game itself requires manual shifts.

```text
python tools/drag/launch_sweep.py all 16          # every 250 RPM, 16 ms commands
python tools/drag/launch_sweep.py 4500 8 6100     # one launch, 8 ms, shift at 6,100
python tools/drag/launch_sweep.py limiter 16      # flat out in N: time until the engine blows
```

`tests/test_drag_launch_physics.py` runs it at 8, 16 and 33 ms and checks the
shape below. Re-measure after changes to the engine curve, clutch coupling,
gearing, tyres or stock vehicle settings.

## Results

Elapsed time over the quarter mile (14,348 units), in seconds, by launch RPM and
command interval:

| Launch RPM | 8 ms | 16 ms | 33 ms |
| ---: | ---: | ---: | ---: |
| 1,000 | 13.082 | 13.154 | 13.156 |
| 2,000 | 12.794 | 12.907 | 12.913 |
| 3,000 | 12.545 | 12.676 | 12.684 |
| 3,500 | 12.440 | 12.580 | 12.587 |
| 4,000 | 12.366 | **12.518** | **12.528** |
| 4,500 | 12.330 | 12.524 | 12.535 |
| 5,000 | **12.327** | 12.537 | 12.548 |
| 5,500 | 12.337 | 12.564 | 12.576 |
| 6,000 | 12.364 | 12.609 | 12.616 |
| 6,250 | 12.383 | 12.625 | 12.640 |

The best launch lies in 4,000–5,000 RPM at every interval. Low revs cost the
most (bog), the limiter about 0.1 s (smoke). Flat out in N with an armed engine,
it blows after about 2.7 s at every interval.

Absolute times still depend on the command interval (client frame rate): the
whole run is about 0.2 s quicker at 8 ms than at 16 ms. That sensitivity is
older than the launch physics and affects every heat equally only when both
drivers run the same frame rate. Compare launch RPMs within one interval.

Before this change, the wheels had to cross a fixed 1.5 rad/s dead zone within
one movement step. With short steps (high frame rates) a weakly driven wheel
never got going, so launches below 3,000 RPM stood still at 4 ms. Drag now only
stops wheels that turn the wrong way.
