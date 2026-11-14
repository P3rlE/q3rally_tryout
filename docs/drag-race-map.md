# Drag Strip prototype map

`baseq3r/maps/q3r_dragstrip.map` is the small test course for the v0.9
Drag Race prototype. `baseq3r/scripts/q3r_dragstrip.arena` advertises only
`q3r_drag`; this is not a v0.8 race or a replacement for existing Sprint maps.

The course uses existing repository textures and shaders. Both lanes have the
same flat asphalt collision surface. White markings use the existing nonsolid
`q3r_redrock/paint_white` shader and do not add bumps or change tyre friction.
There are no weapons, pickups, bot routes or AAS file.

## Geometry contract

All driving is along positive X, at road height Z=0. The two numbered
`info_player_start` entities are `(0, -160, 28)` and `(0, 160, 28)`, facing
angle 0. The start positions have identical X and Z coordinates.

| Entity | Centre X | Y bounds | Z bounds |
| --- | ---: | --- | --- |
| `rally_start` | 100 | -320 to 320 | -8 to 192 |
| `rally_checkpoint`, number 1 | 7174 | -320 to 320 | -8 to 192 |
| `rally_finish` | 14348 | -320 to 320 | -8 to 192 |

The gate brushes are eight units thick. Their bounds, rather than an explicit
origin key, define their centres. The intermediate checkpoint is silent and
keeps the existing Sprint route and countdown infrastructure usable.

Drag timing measures the finish plane from each car's starting reference
position at X=0. With the default `CP_M_2_QU = (350 / 32.2) / 0.3048`,
the 14348-unit run is approximately 402.341 metres. This is a quarter mile
rounded to the map grid. The Sprint route's gate-to-gate length starts at
X=100, so it is approximately 2.804 metres shorter than that timed distance.

Lane centres are Y=-160 and Y=160. The middle marking has no physical barrier.
Roadside barriers start at Y=-384 and Y=384; both lanes have equal room.
The main straight extends past the finish to X=19072, leaving approximately
132.47 metres of braking area before the end barrier. Start and finish
gantries clear the road by 256 units. The intermission camera overlooks the
start area.

## Rebuild

Use Q3Map2 with the repository's current custom surface flags. The example
below is PowerShell, run from the repository root; change `$q3map` if needed.

```powershell
$q3map = '..\Q3RallyRadiant\install\q3map2.exe'
$repo = (Get-Location).Path
$map = Join-Path $repo 'baseq3r\maps\q3r_dragstrip.map'
& $q3map -game quake3 -fs_basegame baseq3r -fs_basepath $repo -threads 8 -meta -custinfoparms $map
& $q3map -game quake3 -fs_basegame baseq3r -fs_basepath $repo -threads 8 -vis -saveprt $map
& $q3map -game quake3 -fs_basegame baseq3r -fs_basepath $repo -threads 8 -light -custinfoparms -fast -filter -super 2 -bounce 1 $map
```

The runtime artifact is `baseq3r/maps/q3r_dragstrip.bsp`. Compiler `.prt`,
`.srf` and light intermediates are not runtime assets. Run the prototype with
the Drag Race mode (`g_gametype 9`) and `map q3r_dragstrip`.

The checked-in BSP was compiled with NetRadiant Q3Map2 2.5.17n
(`401547a`, 2026-11-03), using the three commands above. BSP, VIS and light
completed without leaks. Existing shared shader-list warnings about
`nomipmaps` and `q3map2_q3r_nightcity.shader` remain unrelated to this map.
Compiled collision checks passed for 918 road samples, car-box sweeps along
both lanes, and a sweep across the centre paint. The compiled asphalt surface
flag is `0x200000`; the paint has no solid contents. Both spawn positions and
all three gate centres were also checked in the compiled BSP. In-game driving
and visual checks remain necessary.

## Manual course checks

1. Spawn one driver in each numbered position; check both face the finish.
2. Cross the midpoint and finish through each lane and verify the same gates.
3. Drive across the painted centre and start/finish markings; they should not
   deflect the car or change grip.
4. Confirm the finish gantry does not stop the car and the runoff can be used
   to brake before the end barrier.
5. Repeat the same launch and shift sequence in both lanes and compare times.
