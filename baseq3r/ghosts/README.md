# Ghost telemetry format

Each map/vehicle combination is stored as a plain text file in `baseq3r/ghosts` with the name pattern
`<mapname>_<vehicle>.ghost`. The file is streamed by the server to advertise the fastest base ghost and can
be downloaded or shipped with the map PK3 so that clients can play it back.

A ghost file is line-based and uses the following header keys:

```
map <mapname without extension>
vehicle <vehicle class or chassis id>
best_time_ms <fastest race time in milliseconds>
frames <number of ghost frames that follow>
```

After the header, every line represents one frame with the fields below separated by spaces:

```
<timeOffsetMs> <originX> <originY> <originZ> <anglesPitch> <anglesYaw> <anglesRoll> <velocityX> <velocityY> <velocityZ> <buttons> <forwardmove> <upmove>
```

* `timeOffsetMs` is the time since the player crossed the start line.
* `origin`, `angles`, and `velocity` match the captured player state.
* `buttons`, `forwardmove`, and `upmove` capture the input state for that frame.

The client validates the `map` and `vehicle` header values against the current map and selected vehicle
before accepting a ghost. Use integer values for time and input columns; positions, angles, and velocities
can be decimals.
