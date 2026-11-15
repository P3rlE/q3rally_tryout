# Drag launch RPM guidance

The HUD's **4,000–5,000 RPM** launch band is a recommended range for the
current stock Drag physics on dry asphalt. It is a visual aid, not a scoring
window: entering it adds no torque, bonus or automatic launch. First gear must
still be selected on green.

Calibration used the neutral-launch physics from commit `76076f34`, without
changing its engine, clutch matching, tyre forces or input rules.

## Measurement method

A temporary native harness loaded the compiled cgame DLL and called the actual
production `Pmove` function. A flat trace callback supplied dry asphalt at Z=0,
with stock suspension, mass, drag and friction settings. The car settled in N
for two seconds, then engaged first at the selected starting RPM with full
throttle. Every run used the same subsequent upshift policy: shift at 6,000 RPM,
with a 300 ms initial guard so stored neutral revs could not cause an immediate
shift into second. This automatic policy belongs to the measurement harness;
the game still requires manual shifts.

Each sweep attempted 23 starting values from 750 to 6,250 RPM in 250 RPM steps.
The sweeps used 4, 8 and 16 ms movement commands. Elapsed time was interpolated
at X=14,348 game units, matching the Drag Strip's approximately 402.341 m run.
Reaction time was excluded.

Representative elapsed times, in seconds:

| Starting RPM | 4 ms commands | 8 ms commands | 16 ms commands |
| ---: | ---: | ---: | ---: |
| 750 | — | 12.483 | 12.435 |
| 3,500 | 12.529 | 12.423 | 12.401 |
| 4,000 | 12.523 | 12.418 | 12.398 |
| 4,500 | 12.515 | 12.415 | 12.394 |
| 5,000 | 12.507 | 12.414 | 12.392 |
| 5,500 | 12.531 | 12.422 | 12.392 |
| 6,250 | 12.567 | 12.428 | 12.393 |

The 4 ms harness attempts at 750–2,750 RPM remained stationary and reached the
25 s limit. These synthetic flat-trace failures are excluded from performance
conclusions; they are not evidence that those starts fail in the game. All
23 values completed at 8 and 16 ms, and the proposed HUD band completed at all
three command intervals.

## Choice and limits

The 8 ms sweep has a broad useful plateau around 4,000–5,000 RPM, with its
lowest sampled time at 5,000. The 4 ms sweep also favors 5,000; at 16 ms the
plateau extends above it. A broad band therefore gives a useful target without
claiming a unique optimum or making a few milliseconds into a precise
"perfect launch" threshold. Compare RPM choices within a given command
interval: the full vehicle/contact simulation shows timestep sensitivity.

This does not establish a guaranteed wheelspin threshold. The existing
[launch physics test](../tests/test_drag_launch_physics.py) separately checks
neutral isolation, RPM carryover, bounded flywheel impulse and controlled tyre
slip demand. Launch throttle, later shifts, surfaces and modified physics can
change the result. Recalibrate this guidance after changes to the engine curve,
first-gear coupling, gearing, tyres or stock vehicle settings.

Local validation artifacts are under `output/drag-hud/`: `rpm_sweep.c`,
`rpm_sweep.csv`, `rpm_sweep_4ms.csv` and `rpm_sweep_16ms.csv`. They are local
measurement evidence, not packaged game assets or a committed test dependency.
