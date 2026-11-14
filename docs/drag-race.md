# Drag Race prototype (v0.9 / Tryout)

Drag Race is an experimental mode on the Tryout development line. It does not
change the v0.8 release line or its Ladder/profile contracts.

## Playing a heat

Select **Drag Race** in Offline/Host and choose **Drag Strip**. The prototype
accepts two human drivers, or one driver for solo practice. Bots are disabled.
Both lanes use the same vehicle performance, regardless of the selected body.

For a console start:

```text
set g_gametype 9
map q3r_dragstrip
```

1. Hold the handbrake to stage. The default binding is **Space**. The countdown
   begins once every connected driver is holding it; solo staging starts a
   practice heat immediately.
2. Keep holding through the amber lights. Release on green to launch. You may
   hold the accelerator in advance; it cannot launch the car automatically.
3. Shift manually with **Gear Up / Gear Down** (defaults **Page Up / Page Down**).
   The mode forces manual transmission without changing the saved preference.
4. Stay in your lane and cross the finish. Releasing before green is an
   irrevocable false start. Leaving the lane, resetting/respawning, or leaving
   the driving field after green forfeits the attempt.
5. Review the results and use the normal intermission confirmation for the next
   heat. Each match currently contains one run.

The map is approximately a quarter mile from the equal grid positions to the
finish plane. See [the map contract](drag-race-map.md) for exact coordinates,
geometry checks and rebuild instructions.

## Timing and rules

- **RT:** time from scheduled green to the driver's handbrake-release command.
  This prototype measures the input release, rather than a physical start beam.
- **ET:** time from that release to crossing the finish plane.
- **TOTAL:** RT + ET. The earliest valid finish wins; equal millisecond results
  are tied. Invalid attempts have no result time or finishing rank.
- **KM/H:** interpolated speed at the finish.

The server uses the accepted usercommand timestamp for the release and
interpolates crossing of a common plane between movement updates. The car's
origin is the shared reference for every vehicle body. Millisecond display is
the recording resolution, not a guarantee of sub-frame physical accuracy.
Winner resolution happens after all drivers have moved for a server frame.

Green follows three amber stages with a variable final hold. A disconnect or
switch to spectator before green cancels the countdown and requires restaging.
New drivers cannot join an armed heat. A heat times out after 60 seconds; after
the first valid finish, the other driver has up to ten seconds to finish.

Weapons, holdable items, powerups, clean-sector NOS, slipstream and fuel
consumption are disabled. The prototype does not write race ghosts, award
profile progression or submit Ladder results.

## Scope and validation

Included: separate gametype ID 9, dedicated map filtering, native/WebUI setup,
staging/false starts, manual shifting, lane rules, interpolated finish timing,
live telemetry and Drag results. Existing mode IDs remain stable.

Deferred: an analogue clutch/launch-RPM model, selectable gearing, NOS,
Best-of-three matches with lane swaps, bots, career rewards and Ladder support.

Production-code harnesses cover staging, false starts, queued/future input,
prediction replay, finish order/ties, forfeits, disconnects/spectator changes,
timeouts, map filtering and result formatting. Final driving feel and WAN
fairness still need human testing with two clients; no lag compensation for
round-trip latency is claimed.
