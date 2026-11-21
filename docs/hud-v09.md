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
- The icons are drawn by Chrome from the design's SVG (also the weapons:
  `hud_gun`, `hud_mine`, `hud_smoke`)
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
| Racing DM, Team Racing DM | yes: SCORE (frags) in the card instead of the gap, the car's card 330px wide with FRONT and REAR weapon (ammo, name) before the items, a kill feed of the last three kills (6 s) under the order; in teams the frags in the team cells |
| Elimination | yes: place among those still in, LEFT x/y, NEXT OUT (who is last now, how many are through the lap that decides), total time; "Still in" with SAFE/DANGER and the drivers out under it; a big message when someone is out; weapons and kill feed with g_eliminationWeapons |
| Last Car Standing | yes: LEFT x / y and SCORE, match time and the leader; "Still in" with everyone's integrity as a bar (in percent, orange when low), those out under it; weapons and kill feed; a big message when someone is out |
| Derby | yes: SCORE and NO RAM (time left to ram someone), the car from above with the zones that took the hits (OK / WORN / CRITICAL, the zone just hit blinks); "Derby order" with everyone's integrity; a big message when someone is out |
| Deathmatch | yes: frags against the frag limit and the rank ("=3rd" when tied), time left (or played) and the leader; "Frag order" from the scores; weapons and kill feed |
| Team Deathmatch | yes: your team in its colour and your frags, the clock and the frag limit; "Team frag order" with the teams' frags over it and team squares; weapons and kill feed |
| Capture the Flag, Four team CTF | yes: your team in its colour, a line per flag (AT BASE, CARRIED with the carrier when in sight, DROPPED with the time until it returns), the clock; "CTF order" with the teams' captures over it, every driver's captures and team squares; weapons and kill feed |
| Domination | yes: a tile per control point in its owner's colour, CAPTURING with how far (the server sends it while the car is on the point) or the points your team holds, your team and the clock; "Domination order" with the teams' score over it and team squares; markers for the points in the scene (letter, owner, distance; `cg_sigilLocator`); weapons and kill feed |
| King of the Hill | yes: who holds the hill (icon and team in its colour), what goes on there (CAPTURING with how far, CONTESTED, HOLDING, HILL EMPTY) with each team's players on the hill ("2 v 1", red v blue), the next respawn wave and the clock; "KOTH driver order" with the teams' points over it and team squares; weapons and kill feed |
| Autoball | yes: a score bug at the top centre in place of the rear view (RED score, the clock over the state: TIME LEFT, KICK-OFF, GOAL, OVERTIME, BLUE score); your team, the ball (in whose half, how far), your goals and your team's possession (the team of the last touch while the ball is in play, counted from when you joined); "Autoball order" by goals; the goal as a big message ("BLUE SCORES!", scorer and speed); the ball marker out of view clear of the panels (`cg_autoballIndicator`) |
| Drag | yes: the time slip in the card (reaction, 60 ft, 330 ft and 1/8 mile taken by the client from where the car stood staged, the 1/4 mile ET and the trap speed from the server; FOUL, DNF), "YOU" and the lane; "Duel · Heat 2/3" with the lanes L/R and each ET; the tree under it (PRE, STAGE, three ambers, GO per lane, red after a false start); a tachometer across the bottom in place of minimap and cockpit: RPM, the launch window and what to do (WAIT FOR GREEN, SELECT FIRST, SHIFT UP, OVER-REV), gear and speed, 40 segments with the launch window in blue and the rev limit in orange; a "Heat 2 of 3" notification with the score of the match |

The gaps in the race order come from the checkpoints (`cg_race_gap.c`,
tested in `tests/test_race_gap.py`): at the last checkpoint both drivers
have crossed, growing while the one ahead is past the next one. The order
itself is the server's (by the way still to go); where the two disagree
right after an overtake, a place is shown no closer than the one ahead.

All game modes now have the new HUD.

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
