# The in-game HUD of v0.9

The HUD follows the menus: dark panels with a 1px line and an orange top
edge, the menu fonts, and the centre of the screen left to the race. Each
module sits in the same corner in every game mode; only the content of the
mode card and the standings changes.

| Where | What |
|---|---|
| top left | mode card: place, lap, lap and best time, gap to the car ahead (or the lead) |
| top right | standings: up to six rows around your place |
| top centre | rear view, below it the notification lane (pickups, rewards, achievements, rank, messages) |
| bottom left | jukebox (one line, opened for 15 s on a change of track), the car: integrity, armor, trunk items and weapons |
| bottom right | minimap, slipstream (while active), cockpit: speed, gear, revs, NOS, fuel |

## Design and code

- The design is a set of 1600x900 pages, one per game mode, written by
  `tools/ui_webstyle/hud_design/gen.py` (also the Design canvas "Q3Rally HUD –
  Entwurf v0.9"). `measure_hud_design.py` measures a page in Chrome into
  `tests/fixtures/hud_*_layout.json`.
- `engine/code/cgame/cg_hud_modernlayout.c` lays the modules out in design
  pixels (900 to the screen height) the way Chrome lays out the page;
  `tests/test_native_hud_race.py` holds it against the measurements (0.5 px).
- `engine/code/cgame/cg_hud_modern.c` fills it from the game and draws it,
  scaled by the screen height. The corners follow the screen edges, so 4:3
  and ultrawide keep the same distances from the edges.
- The icons are drawn by Chrome from the design's SVG
  (`build_hud_icons.py` → `baseq3r/menu/art/hud_*.png`).
- The menu fonts have extra atlas sizes for the HUD (up to 4K) and an em dash;
  the italic face carries lowercase too ("1,252 m").

## Game modes

| Mode | v0.9 HUD |
|---|---|
| Race | yes |
| Sprint | yes: the way to the finish instead of the lap, a progress bar with the checkpoints (ticks up to 12), total time |
| Ghost Race | yes: the gap to the ghost at the last checkpoint instead of the place (blue ahead, orange behind), the ghost and its lap in the card; at the right the splits of the lap (sector time and how it went against the ghost) and the sector you are in |
| Team Racing | yes: the race with "YOUR TEAM" in the card, the teams' average time over the order ("--" until the first car is home) and a square in the team colour before every driver |
| all others | the old HUD for now |

`cg_hudStyle 0` shows the old HUD in every mode. The HUD toggles
(`cg_hudShowTimes`, `cg_hudShowLaps`, `cg_hudShowPosition`,
`cg_hudShowOpponentList`, `cg_hudShowSlipstream`, `cg_drawRearView`,
`cg_drawMMap`, `cg_drawStatus`) work as before; `cg_mmap_size` scales the
minimap, `cg_mmap_fov` its zoom.

In these modes the big messages in the middle are in the menu fonts too
(the `Meldungen.dc.html` board): the countdown ("3", "2", "1", "GO!" in the
accent), WRONG WAY, and every centerprint (also target_print) with its first
line big and the next ones in small capitals under it. One at a time:
countdown before wrong way before a centerprint. A spectator gets a panel at
the bottom in the middle: the view (following, track camera, free camera)
and whom it shows. FIRE goes to the next driver, the rear attack key
(`+button12`, default KP INS, shown in the panel) to the next view:
follow, track cameras, free camera, follow. A driver who has finished skips
the free camera.

Still to come: the other game modes.
