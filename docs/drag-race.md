# Drag Race prototype (v0.9 / Tryout)

Drag Race is an experimental mode on the Tryout development line. It does not
change the v0.8 release line or its Ladder/profile contracts.

## Playing a heat

Select **Drag Race** in Offline/Host and choose **Drag Strip**. The prototype
accepts two human drivers, or one driver for solo practice. Bots are disabled.
Both lanes use the same vehicle performance, regardless of the selected body.
Neutral revs carry into a brief clutch engagement when first gear is selected.
Throttle and tyre grip affect the launch; there is no fixed perfect-RPM bonus.

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
4. Stay in your lane and cross the finish. Selecting first before green is an
   irrevocable false start. Leaving the lane, resetting/respawning, or leaving
   the driving field after green forfeits the attempt.
5. Review the results and use the normal intermission confirmation for the next
   heat. Each match currently contains one run.

The map is approximately a quarter mile from the equal grid positions to the
finish plane. See [the map contract](drag-race-map.md) for exact coordinates,
geometry checks and rebuild instructions.

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

- **RT:** time from scheduled green to the driver's first-gear input.
  This prototype measures gear engagement, rather than a physical start beam.
- **ET:** time from that input to crossing the finish plane.
- **TOTAL:** RT + ET. The earliest valid finish wins; equal millisecond results
  are tied. Invalid attempts have no result time or finishing rank.
- **KM/H:** interpolated speed at the finish.

The server uses the accepted launch-input timestamp for first gear and
interpolates crossing of a common plane between movement updates. The car's
origin is the shared reference for every vehicle body. Millisecond display is
the recording resolution, not a guarantee of sub-frame physical accuracy.
Winner resolution happens after all drivers have moved for a server frame.

Green follows three amber stages with a variable final hold. A disconnect or
switch to spectator before green cancels the countdown and requires restaging.
New drivers cannot join an armed heat. A heat times out after 60 seconds; after
the first valid finish, the other driver has up to ten seconds to finish.

The Drag Strip has a physical start light above each lane, reusing Timber Run's
`rally_startlight` model and effect. Its three red lamps match the HUD's three
amber stages. Both world lights, the HUD and the distinct green tone use the
same scheduled server green timestamp, even when the `GO!` text arrives later.
A short tone accompanies each countdown stage. Green stays lit for up to four
seconds; the HUD start prompt clears sooner. A driver's false start is shown
in their HUD and does not change the common world lights for the opponent.
Map restart and a cancelled start clear the previous signals. Mapper preview
is silent, and late joining or seeking past green does not replay a stale tone.

Weapons, holdable items, powerups, clean-sector NOS, slipstream and fuel
consumption are disabled. The prototype does not write race ghosts, award
profile progression or submit Ladder results.

## Scope and validation

Included: separate gametype ID 9, dedicated map filtering, native/WebUI setup,
neutral staging, first-gear launch/false starts, manual shifting, lane rules,
interpolated finish timing, live telemetry and Drag results. Existing mode IDs
remain stable.

Deferred: analogue clutch control, selectable gearing, NOS,
Best-of-three matches with lane swaps, bots, career rewards and Ladder support.

Production-code harnesses cover staging, false starts, queued/future input,
prediction replay, finish order/ties, forfeits, disconnects/spectator changes,
timeouts, map filtering and result formatting. Final driving feel and WAN
fairness still need human testing with two clients; no lag compensation for
round-trip latency is claimed.
