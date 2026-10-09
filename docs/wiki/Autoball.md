# Autoball

**Car football: two teams, one big ball, two goals.** Push the ball into the other team's goal with your car. No weapons, just driving, turbo and the occasional demolition.

---

## 🎮 For Players

### The match

| Phase | What happens |
|---|---|
| **Kick-off** | Everybody is put back on a kick-off spot and the cars are frozen. A 3-2-1 countdown, then the whistle: go. |
| **Live** | Hit the ball. When the whole ball crosses a goal line, the other team scores. |
| **Goal** | Explosion, horn, the crowd goes wild, cars near the goal get blown away. After a few seconds: next kick-off. |
| **End** | Goal limit or time limit. If the score is tied when time runs out, the next goal wins (golden goal). The clock waits while the ball is in the air. |

### Driving tips

- **Turbo** is your jump and your boost. You start with 5 seconds stored; hold the Turbo key to burn it, let go to save the rest. Turbo pickups lie around the arena.
- **Hit the ball off-centre** to aim it. Hitting it straight on just pushes it forward.
- **Demolition:** ram an opponent while burning turbo at 110 km/h or more and their car is wrecked. They respawn after 3 seconds; you get an extra second of turbo.
- **The ball's seams glow** in the colour of the team that touched it last.
- **Your own goal:** the ball counts for whoever's goal it goes into. Own goals happen. Don't panic.

### Points

Personal score (scoreboard) is separate from the team score (goals).

| Action | Points |
|---|---|
| Goal | 100 |
| Assist (teammate touched the ball shortly before your goal) | 50 |
| Save (you stop a ball that was heading into your goal) | 50 |
| Shot (your touch sends the ball towards the opponent goal) | 20 |
| Demolition | 10 |
| Touch | 2 (at most every 2 seconds) |

The scoreboard shows goals, assists, saves and shots for every player.

### HUD and camera

| Command / cvar | Default | Description |
|---|---|---|
| `ballcam` | | Toggles the ball camera: the chase camera turns towards the ball instead of along your car. Bind it to a key: `bind c ballcam`. |
| `cg_autoballCam` | 0 | Ball camera on/off (what `ballcam` toggles). |
| `cg_autoballIndicator` | 1 | Arrow at the screen edge pointing to the ball when it is off-screen, with the distance. |
| `cg_autoballShake` | 1 | Camera shake strength for goal explosions. `0` turns it off, `1.5` for more. |
| `cg_autoballPredict` | 0 | Experimental: predicts your car against the ball. Leave it off online. |

---

## 🖥️ For Server Admins

Autoball is `g_gametype 23`. It is a team mode: it uses `capturelimit` as the **goal limit** and `timelimit` as usual. A ready-made config is in `baseq3r/q3r_autoball.cfg`.

> ⚠️ Autoball needs the Bullet physics backend: `g_scriptedObjectBullet 1` (the default). Without it the ball uses the old solver and feels wrong.

### Match settings

| Cvar | Default | Description |
|---|---|---|
| `capturelimit` | | Goals needed to win (0 = no limit). |
| `timelimit` | | Minutes. Tied at the end: golden goal. |
| `g_autoballKickoffDelay` | 3 | Seconds the cars are frozen at kick-off (0-10). |
| `g_autoballGoalDelay` | 4 | Seconds between a goal and the next kick-off (1-15). |
| `g_autoballStartTurbo` | 5000 | Stored turbo in ms after every spawn. |
| `g_autoballDemoSpeed` | 110 | km/h a turbo-burning car needs for a demolition. |
| `g_autoballGoalPush` | 2200 | Strength of the goal explosion that pushes cars away. |
| `g_autoballWeapons` | 0 | 1 = weapons and weapon pickups stay in the game. Latched (map restart). |

### Ball physics

Changes apply live, so you can tune during a match.

| Cvar | Default | Description |
|---|---|---|
| `g_autoballImpactScale` | 1.6 | How hard car hits launch the ball (0-2). |
| `g_autoballVerticalScale` | 0.45 | Flattens shots: 1 = full upward share of the contact, lower = flatter shots (0-1). |
| `g_autoballLift` | 0.15 | Minimum upward share of every hit, so the ball leaves the ground (0-0.9). |
| `g_autoballMass` | 400 | Ball mass. A car is about 1400. |
| `g_autoballElasticity` | 0.6 | Bounciness (0-1). |

### Mutators

Mutators are **not** saved in the config, so every map starts normal unless your server config sets them. For a local game they are in the Start Server menu: **Balls** (1-3), **Ball Size** (Small 0.7, Normal, Big 1.4, Giant 2) and **Ball Gravity** (Light 0.6, Moon 0.35, Normal, Heavy 1.5).

| Cvar | Default | Description |
|---|---|---|
| `g_autoballBalls` | 1 | Number of match balls (1-3). Extra balls sit beside the centre spot; every ball can score; a goal resets all balls. Latched. |
| `g_autoballBallScale` | 1 | Ball size (0.5-2). `2` = giant ball. Latched. |
| `g_autoballBallGravity` | 1 | Gravity for the ball only (0.1-2). `0.4` = moon ball. Live. |

### Bots

Bots play Autoball: the best placed car of a team attacks, the car closest to its own goal defends, the rest wait behind the ball and collect turbo. Humans count for the roles too, so bots fill the gaps around you. Higher bot skill = better aim and more turbo use. The map needs an `.aas` file like for every other mode (see [Bot Navigation](Bot-Navigation)).

### Balance data

With `g_log` on (default) the server writes Autoball events to `games.log`. With `g_autoballStats 1` it also writes a sample every second (ball speed, height, territory, turbo stock). Then:

`python3 tools/autoball/analyze_log.py games.log`

prints goals, kick-off goals, shots, saves, demolitions, turbo stock and hints like "Lots of demolitions: raise g_autoballDemoSpeed". Compare several matches and change one cvar at a time.

### Testing with lag

The ball is simulated on the server only, so ping matters. To test it on your own machine, start the map with cheats (`devmap q3r_autoball_test`) and set `cl_packetdelay 50` and `sv_packetdelay 50`. That is about 100 ms ping. `net_dropsim 2` adds 2 % packet loss.

### Debug and test commands

| Command | Description |
|---|---|
| `g_autoballDebug 1` | Prints touches, shots, saves and demolition checks. `2` also prints bot role changes. |
| `ball_info` | Ball position and speed, match state; with debug on also every car's speed. |
| `ball_spawn_at <x> <y> <z>` | Server: spawns a ball (for testing on any map). |
| `ball_goal_add <red\|blue> <x1> <y1> <z1> <x2> <y2> <z2>` | Server: adds a goal box (team = defender). |
| `ball_kick <vx> <vy> <vz>` | Server: kicks the ball (units/s; 35.66 units = 1 m). |
| `ball_touch <client>` / `ball_turbo <client> <ms>` | Server: fake a touch / give turbo. |
| `ball_spawn` / `ball_reset` / `ball_remove` | Client, cheats: test balls in front of you. |

---

## 🗺️ For Mappers

An Autoball map needs a ball, two goals, team spawns and some turbo pickups. A complete example is generated by `tools/autoball/make_test_arena.py` (`q3r_autoball_test`).

| Entity | Description |
|---|---|
| `autoball_ball` | The ball. Its origin is the kick-off spot (centre of the field). |
| `autoball_goal` | **Brush entity** (use `common/trigger`): the goal volume. One per team. |
| `team_CTF_redplayer` / `team_CTF_blueplayer` | Kick-off spots. All cars are put here at every kick-off. |
| `team_CTF_redspawn` / `team_CTF_bluespawn` | Respawn spots after a demolition. |
| `rally_item_turbo` | Turbo pickup. Only turbo pickups stay in Autoball (unless `g_autoballWeapons 1`). |

| Key on `autoball_goal` | Default | Description |
|---|---|---|
| `team` | red | Team that **defends** this goal. A ball entering the red goal counts for blue. |

| Key on `autoball_ball` | Default | Description |
|---|---|---|
| `radius` | 75 | Ball radius in units. The default model is made for 75. |
| `mass` | 400 | Ball mass. |
| `elasticity` | 0.6 | Bounciness 0-1. |
| `friction` / `rolling_friction` | 0.4 / 0.02 | Surface grip / how fast a rolling ball slows down. |
| `vehicle_impact_scale` | 1.0 | Shot strength 0-2 (multiplied with `g_autoballImpactScale`). |
| `vehicle_vertical_scale` | 0.45 | Flattens car hits (0-1). |
| `vehicle_lift` | 0.15 | Minimum upward share of a car hit. |
| `max_speed` | 3000 | Speed cap in units/s. |
| `model` | models/autoball/ball.md3 | Ball model. |

> ⚠️ Start the goal brush **one ball radius (75 units) behind the goal line**. The game checks the ball's centre, so the whole ball has to be over the line.

- **The goal volume is not solid.** The engine treats `autoball_goal` like a trigger, so the ball rolls into it.
- **Close the arena**, including a ceiling. Bullet treats the sky as solid, and the ball should bounce off everything.
- **Ramps along the walls** (45° boards) let the ball and cars come off the wall instead of getting stuck in the corner.
- **Turbo pickups:** a few big ones (`count` 5000, `wait` about 10) in the corners and small ones (`count` 1500, `wait` about 4) along the sides work well.
- **`.arena` keyword:** `q3r_autoball`. Entity gametype key: `autoball`.
- **Bots need an `.aas` file**, built with bspc like for every other map.

> ⚠️ The console says `AUTOBALL: this map has … red / … blue autoball_goal` when a ball or a goal is missing.

---

## 🛠️ Assets

All Autoball assets are generated by scripts, so they can be rebuilt or changed:

| Script | Makes |
|---|---|
| `tools/autoball/make_ball.py` | `models/autoball/ball.md3`, `ball.tga`, `ball_glow.tga` (shader in `scripts/autoball.shader`) |
| `tools/autoball/make_sounds.py` | `sound/autoball/*.ogg`: ball hits, bounce, goal horn, whistle, crowd cheer (faded copy of `sound/world/crowds.ogg`) |
| `tools/autoball/make_test_arena.py` | `maps/q3r_autoball_test.map` |
| `tools/autoball/analyze_log.py` | Balance report from `games.log` |
