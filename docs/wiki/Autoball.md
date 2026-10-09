# Autoball 🧪

*Development build.* **Car football: two teams, one big ball, two goals.** Drive the ball into the other team's goal. No weapons by default, just driving, turbo and the occasional demolition.

`g_gametype 23` · team mode · `.arena` keyword `q3r_autoball`

---

## 🏁 Playing

### How a match runs

| Phase | What happens |
|---|---|
| **Kick-off** | Every car is put back on a kick-off spot and frozen. A 3-2-1 countdown, then the whistle. Before the first kick-off of a match the camera flies once over the arena. |
| **Live** | Hit the ball. When the **whole ball** crosses a goal line, the other team scores. |
| **Goal** | Explosion, flashing goal lights, horn and crowd. Cars near the goal are blown away. A few seconds later: next kick-off. |
| **End** | Goal limit or time limit. If the score is tied when time runs out, the next goal wins (golden goal). The clock waits while the ball is in the air. Under the final scoreboard: **MVP**, **Top Scorer**, **Best Keeper** and **Hardest Shot**. |

### Tips

* **Turbo** is your boost. You start every life with 5 seconds stored. Hold **Turbo** (default `Shift`) to burn it, let go to keep the rest. Turbo pickups are spread around the arena; they are the only items in Autoball.
* **Aim with the side of your car.** Hitting the ball straight on just pushes it; hitting it off-centre sends it sideways.
* **Demolition:** ram an opponent while burning turbo at **110 km/h** or more and their car is wrecked. They respawn after 3 seconds and you get an extra second of turbo.
* **The ball's seams glow** in the colour of the team that touched it last. Above 100 km/h the ball leaves a trail in that colour; above 150 km/h it catches fire.
* **Own goals count.** A ball in your own goal is a point for the other team, whoever touched it last.

### Points

The team score is the number of goals. Your personal score on the scoreboard:

| Action | Points |
|---|:---:|
| Goal | 100 |
| Assist (you touched the ball shortly before a teammate's goal) | 50 |
| Save (you stop a ball that was heading into your goal) | 50 |
| Shot (your touch sends the ball towards the opponent goal) | 20 |
| Demolition | 10 |
| Touch | 2 (at most every 2 s) |

The scoreboard shows goals, assists, saves and shots for every player. On the [ladder](Ladder-for-Players), Autoball has its own leaderboard and career stats (wins, matches, goals).

### HUD and camera

* A **score bug** at the top shows both scores and the match clock (`+` = golden goal time).
* An **arrow** at the screen edge points to the ball when it is off-screen, with its distance.
* `ballcam` toggles the **ball camera**: the chase camera looks at the ball instead of along your car. Bind it to a key, e.g. `\bind c ballcam`.
* **Spectators** and wrecked cars waiting to respawn get a **TV camera**: it follows the ball from the sideline and cuts to a camera in the goal when a shot is on its way and during the goal celebration. Follow a player as usual with Fire.

All Autoball client settings are listed in [Client Settings → Autoball](Client-Settings#autoball-).

### Bots

Bots play Autoball. The best placed car of a team attacks, the car closest to its own goal defends, and the rest wait behind the ball and collect turbo. Humans count for these roles too, so bots fill the gaps around you. Higher bot skill means better aim and more turbo use; on skill 1–2 they are noticeably easier.

---

## 🎲 Mutators

Variants for fun matches. In the **Create Server** menu they appear under *Goal Limit*; on a dedicated server set the cvars in the config.

| Menu | Cvar | Choices |
|---|---|---|
| Balls | `g_autoballBalls` | 1, 2 or 3 balls. Extra balls start beside the centre spot, every ball can score, a goal resets all of them. |
| Ball Size | `g_autoballBallScale` | Small 0.7 · Normal 1 · Big 1.4 · Giant 2 |
| Ball Gravity | `g_autoballBallGravity` | Moon 0.35 · Light 0.6 · Normal 1 · Heavy 1.5 (only the ball) |

Mutators are **not saved** between sessions, so every map starts normal unless a config sets them. Details in [Server Configuration → Autoball](Server-Configuration#autoball-).

---

## 🖥️ Running an Autoball server

Start with the ready-made config:

```
q3rally-server.x86_64 +exec q3r_autoball.cfg
```

* `capturelimit` is the **goal limit**, `timelimit` works as usual.
* Autoball needs the Bullet physics backend: `g_scriptedObjectBullet 1` (the default).
* All Autoball cvars (match, ball physics, mutators) are in [Server Configuration → Autoball](Server-Configuration#autoball-).

### Balance data

With `g_log` on (the default) the server writes every goal, shot, save, assist and demolition to `games.log`. With `g_autoballStats 1` it also writes a sample every second: ball speed and height, which half the ball is in, turbo stock and car speed per team. The repository has a script that turns this into a report:

```
python3 tools/autoball/analyze_log.py games.log
```

It lists the goals and prints numbers like kick-off goals, goals per shot, demolitions per minute and turbo stock, plus hints such as *"Lots of demolitions: raise g_autoballDemoSpeed"*. Compare several matches and change one cvar at a time.

### Testing with lag

The ball only exists on the server, so ping matters. To try it on your own machine, start the map with cheats and delay the packets:

```
\devmap q3r_autoball_test
\cl_packetdelay 50
\sv_packetdelay 50
```

That is roughly 100 ms ping. `\net_dropsim 2` adds 2 % packet loss.

### Test and debug commands

| Command | Description |
|---|---|
| `g_autoballDebug 1` | Prints touches, shots, saves and demolition checks. `2` also prints bot role changes. |
| `ball_info` | Ball position and speed, match state. With debug on also every car's speed. |
| `ball_spawn_at <x> <y> <z>` | Server: spawns a ball, to test on any map |
| `ball_goal_add <red\|blue> <x1> <y1> <z1> <x2> <y2> <z2>` | Server: adds a goal box (team = defender) |
| `ball_kick <vx> <vy> <vz>` | Server: kicks the ball (units/s, 35.66 units = 1 m) |
| `ball_touch <client>` / `ball_turbo <client> <ms>` | Server: fake a touch / give turbo |
| `ball_spawn` / `ball_reset` / `ball_remove` | Client, needs cheats: test balls in front of your car |

---

## 🛠️ Building an Autoball map

An Autoball arena needs a ball, two goals, team spawns and turbo pickups. The entities and keys are in [Game Mode Entities → Autoball](Game-Mode-Entities#autoball-). Some advice for the layout:

* **Close the arena completely**, including a ceiling. The physics treats the sky as solid, and the ball should bounce off everything.
* **Ramps along the walls** (45° boards) let the ball and the cars come off the wall instead of getting stuck in a corner.
* **Goal mouths** about 1024 units wide and 320 high work well with the default ball (radius 75).
* **Turbo pickups:** a few big ones (`count` 5000, `wait` about 10) in the corners and small ones (`count` 1500, `wait` about 4) along the sides.
* **Bots** need an `.aas` file like on every other map (see [Bot Navigation](Bot-Navigation)).

`tools/autoball/make_test_arena.py` in the repository generates `q3r_autoball_test`, a plain test arena that shows all of this.

### Assets

The ball, its textures and the Autoball sounds are generated by scripts in `tools/autoball/`, so they can be rebuilt or changed:

| Script | Makes |
|---|---|
| `make_ball.py` | `models/autoball/ball.md3`, `ball.tga`, `ball_glow.tga` (shader in `scripts/autoball.shader`) |
| `make_sounds.py` | `sound/autoball/*.ogg`: ball hits, bounce, goal horn, whistle, crowd cheer |
| `make_test_arena.py` | `maps/q3r_autoball_test.map` |
| `analyze_log.py` | Balance report from `games.log` |
