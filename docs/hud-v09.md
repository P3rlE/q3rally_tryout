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

## End screens

The race (Racing) ends on its own screen in the intermission (the
`EndRacing.dc.html` board), in place of the scoreboard; TAB shows the
scoreboard while held.

- At the left: your place ("P2 / 8"), your time and the gap to the winner,
  best lap, top speed, places gained (from the grid), average lap, and your
  laps as bars: what each lost to your best one (the last twelve).
- In the middle: the podium, second, winner, third, each with the driver's
  own car turning in 3D (`CG_DrawShowroomCar`: body, paint, wheels,
  plate), then the highlights: the fastest lap, the best sector (between
  two checkpoints) and the biggest gain from the grid.
- At the right: the classification, the time (the winner's total, the
  others' gap, DNF) and every driver's best lap, the fastest in purple.
  Over 8 drivers the rows are lower; over 16 the first 15 and yours.
- At the bottom: FIRE READY, TAB DETAILS, ESC MENU, and how many drivers
  are ready (bots do not count). Q3 waits for them; there is no next-map
  clock.

The figures come from what the server sends every client during the race
(start, laps, checkpoints, finish, positions), collected by
`cg_race_result.c` (tested in `tests/test_race_result.py`). Who joins during
a race sees what happened from then on. A spectator sees the winner's race.
A screen narrower than 16:9 shows the page scaled down.

A team race (Team Racing) ends on the same page for the teams
(`EndTeamRacing.dc.html`), ranked by their average time as the server ranks
them (`level.teamTimes`):

- At the left: your team's place ("P1 / 3") in the team's colour, its
  average and the margin (won by, or to the winner), then your place, your
  time, how far you were under or over your team's average and your best
  lap; instead of the lap chart the team averages: a row a team, a dot a
  driver home on the time axis and the average as a tick, with the gap to
  the winners.
- The podium of teams, each card in the team's colour with its best
  finisher's car and the drivers under it (their places); YOUR TEAM on
  yours. Highlights: the fastest lap, the team player (the furthest under
  the team's average) and the biggest gain.
- The classification with the teams' averages over it and each driver's
  team as a square.

A drag race ends on its own page (`EndDrag.dc.html`), the duel:

- At the left: the match ("2-1 / 3", the best of), whether you won it and
  your quickest run with how much quicker or slower it was than your
  rival's; best reaction, top trap speed, best total and fouls; your heats
  as bars against your quickest (a foul an orange line).
- In the middle a time slip a lane of the deciding heat (the last one
  with a winner): the car, the lane, reaction, 60 ft, 330 ft, 1/8 mile,
  1/4 mile and trap speed, the better figure of each line in blue; the
  winner's slip with WIN and the accent on top. Practice alone: one slip.
  Highlights: the best reaction and the quickest run of the match.
- At the right the heats: who won each, the elapsed time, the margin over
  the line (FOUL, DNF, SOLO, or RERUN for a heat without a winner).

The server sends only each car's current state; the client writes every
heat down as it runs (`cg_drag_log.c`, tested in `tests/test_drag_log.py`),
with the 60 ft, 330 ft and 1/8 mile splits the HUD measures, now for both
lanes.

A drag tournament (`g_dragTournament 1`) shows the match's own end screen
first; five seconds later (TAB holds the heat results) the tournament:

- Between the matches the bracket (`Tourney.dc.html`, `Tourney16.dc.html`):
  at the left the match up next, big, who rides it (YOU, BOT), your way
  through the rounds (won, lost, up next, the ones to come) and, when two
  bots are up next, the hint to `/dragskip`. At the right a column a
  round, a card a match: the drivers with the heats they won, the winner
  white, the one out grey, bots in grey type with BOT, your lines lit, the
  match up next outlined with UP NEXT; the winner's card and third place
  after the final. 16 drivers get lower lines.
- After the final (`TourneyEnd.dc.html`), the end screen's page for the
  whole tournament: your place of the field, your last match and how it
  went, matches and heats won and lost, your way; the podium (champion,
  finalist, third) with their cars and scores; a clean sweep and the most
  heats won as highlights; every match with its winner, heats and the one
  beaten.

The bracket's bookkeeping (rounds, pairings, a driver's way, the places)
is shared with the classic bracket in `cg_drag_bracket.c`, tested in
`tests/test_drag_bracket.py`.

## The TAB table

Held with TAB (`+scores`), the game dimmed behind it and the HUD out of the
way (`TabRacing`, `TabTeamRacing`, `TabDeathmatch`, `TabCTF` on the
canvas). It no longer comes up on its own when you are wrecked; between
maps it shows by itself in the modes that have no end screen yet, with how
many drivers are ready in place of the clock.

- Over it: the mode, the map with the lap or the limit and how many drive,
  at the right the race time or the clock.
- Races (Racing, Sprint, Ghost, Elimination, Racing DM, Drag): place,
  driver with the car, lap, gap to the leader (LEADER, +1 LAP, OUT, DNF, a
  flag once home), last and best lap (the fastest in purple), ping; one-lap
  races show the time instead of the laps, Drag RT, ET, total, km/h and the
  status, Racing DM the frags. Team races put the teams over it (by their
  drivers' average place) and a square in the team's colour before each
  name.
- Combat (Deathmatch, Derby, Last Car Standing): the score big, the damage
  dealt and taken; those out in grey.
- Teams (TDM, CTF, CTF4, Domination, KOTH, Autoball): a panel a team side
  by side (four two by two), its score in the head, the mode's columns:
  captures, assists and defends; hold time and hill kills; goals, assists,
  saves and shots; or the damage.
- Your row is lit, bots show BOT in place of a ping, a ping from 100 on is
  orange. Under it the spectators and the server with how full it is.

Still to come: the end screens of Sprint, Ghost and the combat and team modes.
