# Drag Race prototype (v0.9 / Tryout)

Drag Race is an experimental mode on the Tryout development line. It does not
change the v0.8 release line or its Ladder/profile contracts.

## Playing a heat

Select **Drag Race** in Offline/Host and choose one of the strips: **Desert
Dragway**, **Night Drag** (eighth mile) or **Airfield Drag**. Race a human or a
bot in the second lane, or practise solo. Both lanes use the same vehicle
performance, regardless of the selected body. Neutral revs carry into a clutch
engagement when first gear is selected, and the launch RPM decides how cleanly
the car gets away (see [launch, bog and smoke](#launch-bog-and-smoke)).

For a console start:

```text
set g_gametype 9
map q3r_dragstrip
```

1. The car starts in **N**. Use the accelerator to build and hold your chosen
   launch RPM. With a keyboard, tap the accelerator to regulate the revs. The
   countdown begins once every connected driver is revving in neutral; solo
   staging starts a practice heat immediately.
2. Stay in N through the amber lights. On green, select first with **Gear Up**
   to launch. Throttle changes while waiting do not count as a false start.
3. Shift manually with **Gear Up / Gear Down** (defaults **Page Up / Page Down**).
   The mode forces manual transmission without changing the saved preference.
4. Stay in your lane and cross the finish. Selecting first before green, or
   less than 0.100 s after it, is an irrevocable false start. Leaving the lane, resetting/respawning, or leaving
   the driving field after green forfeits the attempt.
5. A match is the best of `g_dragBestOf` heats (1, 3, 5 or 7; 3 by default,
   "Heats / match" in the start menu). After each heat the result stays on
   screen for four seconds, then both cars are back on the grid for the next
   heat, without a map reload. Use the normal intermission confirmation for the
   next match.

## Matches

- A duel ends as soon as a driver has won the majority of the heats (2 of 3,
  3 of 5, 4 of 7). The HUD shows "HEAT 2 OF 3" and the score above the Drag
  panel; after each heat the centre print gives the heat winner and the score.
- A heat without a winner (an exact tie, or both drivers invalid) is run again.
  After twice the number of heats the match ends; the driver with more heat
  wins takes it, otherwise nobody.
- A driver who leaves during the match forfeits it. The match ends on the heats
  won so far: the driver who stayed wins it if ahead.
- Solo practice runs the number of heats.
- New drivers join before the first heat of a match or once it is over; in
  between they spectate.
- After a match against a bot, the next match is against another bot with the
  same skill (not in tournament mode). The swap happens when the intermission
  ends, so the results still show the bot that was raced.
- Between heats both cars are taken off the strip before they respawn, so each
  gets a free start spot.

## Tournament

`g_dragTournament 1` (server options: "Tournament: 4 / 8 / 16 drivers")
turns the strip into a single elimination of `g_dragTournamentSize` drivers.

- The field is drawn when the first human is on the server: every human present
  (up to the size), filled up with different bots whose skill is `g_spSkill`
  ±1. The draw is random.
- Each pairing is a normal match of `g_dragBestOf` heats. A knockout needs a
  winner: if a match ends without one, a coin decides.
- The losers of the semi-finals race for third place before the final, so a
  tournament has as many matches as drivers.
- Only the two drivers of the pairing are on the strip; everyone else
  spectates. The server adds and kicks the bots as the bracket needs them.
- Bot against bot is driven. A spectator can skip it with `/dragskip` (this
  match) or `/dragskip all` (all bot matches up to the next one with a human);
  the result is drawn from the bots' skill with the same launch, reaction and
  false-start model the bots drive with.
- A human who leaves before or during his match forfeits it.
- Between the matches the intermission shows the heat results for a few
  seconds, then the bracket with the third place match under the winner
  (holding the scoreboard key brings the results back). After the final it
  shows the podium and where the player finished; firing starts the next
  tournament, which opens with "NEW DRAG TOURNAMENT". The client gets the bracket as `dragTourney <size> <bestOf> <played>
  <a> <b> {"name" human}×size {winner winsA winsB}×played`.
- The bracket lives in the cvars `g_dragTourneyA/B/R` across the map restarts
  between matches. Loading another map, or the restart after the final,
  starts a new tournament. The server log gets `DragTourney:` lines.
- Ladder and career: each match is reported and scored like any other match;
  a match between two bots is not reported. A local human who wins the
  tournament gets 20 career points on top.

Desert Dragway and Airfield Drag are approximately a quarter mile from the
equal grid positions to the finish plane, Night Drag an eighth mile. See
[the map contract](drag-race-map.md) for exact coordinates,
geometry checks and rebuild instructions.

## Launch, bog and smoke

The physics below are Drag only; other modes drive as before.

- **Too low:** below 3,600 RPM a loaded engine bogs. Its torque falls towards
  30 % at idle, so a launch from the cellar climbs out slowly. The car always
  gets going and the run stays valid; it just costs time (about +0.6 s from
  1,000 RPM, +0.15 s from 3,000 RPM). Shifting up far too early drops into the
  same bog.
- **Too high:** the heavier clutch dump spins the rear tyres. Past their grip,
  part of the drive goes up in smoke instead of pulling (about +0.1 s from the
  limiter).
- **Sweet spot:** 4,000–5,000 RPM, shown by the HUD band. See
  [the calibration](drag-launch-rpm-calibration.md).

### Engine damage

Once the lights start, the engine can be over-revved:

- Time in the red zone from 6,000 RPM builds stress, shown as a heat bar under
  the tachometer with "OVER-REV! ENGINE HEATING". Below the red zone it cools
  at half that rate. About 2.7 s on the limiter blows the engine, also in N
  while waiting for green: hold the band, not the limiter.
- The limiter only cuts the engine's own drive. When the road speed spins the
  engine past 7,600 RPM, it blows at once: a downshift at speed, or hanging on
  the limiter instead of shifting up.
- A blown engine makes no power and smokes, with a bang. The attempt ends as a
  DNF with the reason ("too long on the limiter" or "over-revved").

The HUD lights **SHIFT UP** from 5,900 RPM; shifting at about 6,000 RPM is
best. Before the lights, staging on the limiter cannot hurt the engine.

## Bots

A bot can race the second lane. Its skill sets a plan for each heat: the revs
it holds in N, its reaction time and its shift points. The server then drives
it frame by frame: it launches at green plus its reaction, shifts on its plan
and steers along its own lane. Measured on the dedicated server:

| Skill | Reaction | Launch | Mistakes |
| --- | --- | --- | --- |
| 1 | about 0.30–0.49 s | often bogs or smokes | now and then jumps the light (false start) |
| 3 | about 0.17–0.32 s | mostly in the band | rare |
| 5 | about 0.13–0.22 s | in the band | none |

Bots stay out of the red zone while waiting for green. Drag bots need no
navigation, so the strips need no AAS file: on Drag maps the bot AI does not
think, it only sends the usercmds the server prepares.

Each finished heat logs `DragResult:` lines (RT, ET, total, trap speed, bot)
and every disqualification a `DragDisqualify:` line with its reason.

## Drag HUD and launch guide

The dedicated Drag display focuses on engine RPM, gear, speed and race timing.
Before launch, the tachometer highlights a recommended RPM band and indicates
whether to build, hold or ease the throttle. This is a driving aid; it does not
add a launch bonus, change the clutch or tyre physics, or select a gear for you.
The 4,000–5,000 RPM recommendation is calibrated for the current stock car
physics on dry asphalt. It marks a broad useful launch range, not one exact
perfect RPM value or a promise for modified physics and other surfaces.

The green RPM band and the start lights have separate meanings: keep the revs
in the band while waiting, then select first only when the **start light** turns
green. Selecting first during amber is still a false start, even with the RPM
cursor inside the recommended band. Once driving, the tachometer shows engine
speed and the rev limiter rather than the neutral launch recommendation.

Integrity, armour, NOS and fuel indicators are omitted because those systems do
not affect Drag heats. Checkpoint arrows, the racing line and lap counters are
also omitted on the straight strip. The solo 1/1 order list is hidden; a duel
can still show both drivers through the opponent-list option. Times, the main
status HUD, minimap and rear-view mirror retain their applicable HUD toggles,
and saved preferences remain available in other modes.

## Timing and rules

- **RT:** time from green to the driver's first-gear input.
  This prototype measures gear engagement, rather than a physical start beam.
  An RT under **0.100 s** is a false start, as in sprint athletics: nobody
  reacts that fast, so such a launch was anticipated or automated.
- **ET:** time from that input to crossing the finish plane.
- **TOTAL:** RT + ET. The earliest valid finish wins; equal millisecond results
  are tied. Invalid attempts have no result time or finishing rank.
- **KM/H:** interpolated speed at the finish.

The server uses the accepted launch-input timestamp for first gear and
interpolates crossing of a common plane between movement updates. A gearUp
timestamp older than the movement the server has already received from that
client is moved up to that movement: the client sends both together, so an
older stamp can only be a backdated launch. The car's
origin is the shared reference for every vehicle body. Millisecond display is
the recording resolution, not a guarantee of sub-frame physical accuracy.
Winner resolution happens after all drivers have moved for a server frame.

Green follows three amber stages with a variable final hold. The server keeps
the scheduled green to itself. Green is the server frame in which it happens;
the snapshot of that frame is the first to tell clients, so RT is measured from
the moment drivers can see green. Any first-gear request before that frame is a
false start, whatever its timestamp. A disconnect or
switch to spectator before green cancels the countdown and requires restaging.
New drivers cannot join an armed heat. A heat times out after 60 seconds; after
the first valid finish, the other driver has up to ten seconds to finish.

Every drag strip has a physical start light above each lane, reusing Timber Run's
`rally_startlight` model and effect. Its three red lamps match the HUD's three
amber stages. Both world lights, the HUD and the tones follow the same
published countdown start (one more red per second, the third held through
the variable wait) and switch to green with the announced green time, even when
the `GO!` text arrives later. A short tone accompanies each countdown stage. Green stays lit for up to four
seconds; the HUD start prompt clears sooner. A driver's false start is shown
in their HUD and does not change the common world lights for the opponent.
Map restart and a cancelled start clear the previous signals. Mapper preview
is silent, and late joining or seeking past green does not replay a stale tone.

Weapons, holdable items, powerups, clean-sector NOS, slipstream and fuel
consumption are disabled. Drag does not write lap ghosts.

## Ladder and career (v0.9)

Standard-rule matches are reported as `GT_DRAG`, including regular offline
games, once per match with each driver's **best heat** (`settings.dragBestOf`,
`players[].dragHeatWins`). The leaderboard ranks valid finishes by
**TOTAL = RT + ET**, not ET alone. The match goes to the driver who alone won
the most heats; positions follow heat wins, then the best heat. A driver can
therefore win the match with a slower best heat.
Reaction, elapsed time, finish speed and result status remain separate facts.
False starts and DNF stay visible as invalid results with no recorded time or
finishing rank. A bot's result is stored with the match (`isBot`) and can win
the heat, but bots never enter leaderboards or get a profile. Armed participants remain in the report after a disconnect or
switch to spectator; spectators and unarmed drivers do not become participants.

Solo practice records valid best times and completed runs without a competitive
win, loss or career-point award. A match with two drivers, human or bot as in
the other modes, uses the existing race awards once for the whole match: 12 points for the valid winner and 6 for a valid second
place, with the existing per-award online bonus and win achievements. An invalid
opponent can lose a duel but receives no placement points. Exact total-time ties
have no winner or result-point award. Repeated submission cannot award a heat
twice. Cheats, slow motion and modified physics retain the standard Ladder gate.

Drag has independent profile counters: `dragWins`, `dragCompleted` (valid
finishes only), `dragBestMs`, `dragBestReactionMs`, and `dragBestElapsedMs`.
A best reaction must survive later slower starts. Finishes with an RT under
0.100 s are false starts: the game never reports them as finished, the dedicated
server's and both Ladder services' validation reject them, and the Ladder's
leaderboard leaves out older records that would now be false starts. Drag results do not update Sprint wins or lap records.
Existing profile identities and progress are preserved when these additive
fields are saved by the game or menu.

## Scope and validation

Included: separate gametype ID 9, dedicated map filtering, native/WebUI setup,
neutral staging, first-gear launch/false starts, manual shifting, lane rules,
interpolated finish timing, live telemetry, Drag results, Ladder time rankings
and duel career awards. Existing mode IDs remain stable.

Since then: launch physics with bog and smoke, engine damage, bots.

Deferred: analogue clutch control, selectable gearing, NOS,
Best-of-three matches with lane swaps.

Production-code harnesses cover staging, false starts, queued/future input,
prediction replay, finish order/ties, forfeits, disconnects/spectator changes,
timeouts, map filtering and result formatting. Final driving feel and WAN
fairness still need human testing with two clients; no lag compensation for
round-trip latency is claimed.
