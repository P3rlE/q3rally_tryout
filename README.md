Q3Rally — *It's damn fast, baby*
===============================

## English

Q3Rally is a standalone game based on ioquake3.

For compiling, see [engine/README.md](engine/README.md).

### Elimination Mode

Elimination races periodically remove the driver at the back of the pack until
only one racer remains. After the start lights go green, the server waits the
duration defined in `g_eliminationStartDelay` before scheduling the first
elimination (default `30000`, i.e., 30 seconds). Once active, eliminations are
triggered every `g_eliminationInterval` milliseconds (default `15000`, or 15
seconds). Drivers receive an on-screen warning `g_eliminationWarning`
milliseconds before each cut (default `5000`, or 5 seconds); set this to `0` if
you want an immediate drop with no countdown.

These CVars can be adjusted in your server configuration to tune pacing for
public servers or competitive events. For a step-by-step server setup walkthrough
see the [Q3Rally Dedicated Server Setup Guide](readme/q3rally_dedicated_server_guide.txt).

### Vehicle balancing

Servers can enforce basic balancing rules for custom vehicles. Two CVars
control the allowed spread of key vehicle attributes:

* `g_vehicleHpMaxRatio` – maximum allowed ratio between the highest and
  lowest horsepower peak among all cars (default `1.2`).
* `g_vehicleHealthMaxRatio` – maximum allowed ratio between the highest
  and lowest maximum health (default `1.5`).

Typical vehicles ship with an `hpPeak` around 320 and `maxHealth` near 100.
Allowing a ratio between `1.0`–`1.5` for horsepower and `1.0`–`2.0` for
health keeps racing competitive while still permitting variety. Admins are
free to tweak these CVars to fit their custom vehicle sets.

### Jukebox soundtrack rotation

Players can swap the level music for a rotating playlist by toggling the jukebox. Activate it with the `/jukebox` console command or bind the "Jukebox" action in the Controls menu to flip it on and off during a race. Once active, the game picks a random starting song and advances to the next track automatically whenever the current one finishes.

Tracks are loaded from `music/jukebox` (e.g., `baseq3/music/jukebox`) and must be `.ogg` files. Up to 128 songs are indexed per session, and the sound system reports a warning if a track is not encoded as 22 kHz stereo. Turning the jukebox off restores the map's original soundtrack.

### Resources

* [Q3Rally Website](http://www.q3rally.com)
* [Q3Rally on ModDB](https://www.moddb.com/games/q3rally)
* [Q3Rally on Discord](https://discord.gg/rX8Sxmh)

### License
The source code (engine directory) is licensed under the GPLv2 or later unless specified otherwise.

The data files (baseq3r directory) do not have have a known license and should be treated as non-commercial / non-free.

---

## Deutsch

Q3Rally ist ein eigenständiges Spiel auf Basis von ioquake3.

Informationen zum Kompilieren findest du in [engine/README.md](engine/README.md).

### Eliminierungsmodus

Im Eliminierungsrennen scheidet in regelmäßigen Abständen der Fahrer am Ende des Feldes aus, bis nur noch ein Racer übrig bleibt. Nachdem die Startampel auf Grün springt, wartet der Server die in `g_eliminationStartDelay` angegebene Dauer, bevor die erste Eliminierung geplant wird (Standard: `30000`, also 30 Sekunden). Danach werden Eliminierungen alle `g_eliminationInterval` Millisekunden ausgelöst (Standard: `15000`, also 15 Sekunden). Fahrer erhalten `g_eliminationWarning` Millisekunden vor jeder Eliminierung eine Warnung auf dem HUD (Standard: `5000`, 5 Sekunden); setze den Wert auf `0`, wenn der Ausschluss sofort und ohne Countdown erfolgen soll.

Diese CVars lassen sich in der Serverkonfiguration anpassen, um das Tempo für öffentliche Server oder Wettbewerbe feinzujustieren. Eine Schritt-für-Schritt-Anleitung zur Servereinrichtung findest du im [Q3Rally Dedicated Server Setup Guide](readme/q3rally_dedicated_server_guide.txt).

### Fahrzeug-Balancing

Server können grundlegende Balancing-Regeln für benutzerdefinierte Fahrzeuge erzwingen. Zwei CVars bestimmen die zulässige Spannweite wichtiger Fahrzeugwerte:

* `g_vehicleHpMaxRatio` – maximales Verhältnis zwischen der höchsten und der niedrigsten Spitzenleistung aller Fahrzeuge (Standard: `1.2`).
* `g_vehicleHealthMaxRatio` – maximales Verhältnis zwischen der höchsten und der niedrigsten maximalen Haltbarkeit (Standard: `1.5`).

Standardfahrzeuge besitzen typischerweise einen `hpPeak` um 320 und `maxHealth` nahe 100. Ein Verhältnis zwischen `1,0`–`1,5` für die Leistung und `1,0`–`2,0` für die Haltbarkeit hält das Feld konkurrenzfähig und erlaubt dennoch Vielfalt. Administratoren können die CVars nach Bedarf an ihre eigenen Fahrzeugpakete anpassen.

### Jukebox-Soundtrack-Rotation

Spieler können die Levelmusik gegen eine rotierende Playlist austauschen, indem sie die Jukebox aktivieren. Nutze dazu den Konsolenbefehl `/jukebox` oder binde die Aktion „Jukebox“ im Steuerungsmenü, um sie während eines Rennens ein- und auszuschalten. Ist die Jukebox aktiv, wählt das Spiel einen zufälligen Starttitel und wechselt automatisch zum nächsten Track, sobald der aktuelle Song endet.

Die Titel werden aus `music/jukebox` (z. B. `baseq3/music/jukebox`) geladen und müssen im `.ogg`-Format vorliegen. Pro Sitzung werden bis zu 128 Songs indiziert; das Soundsystem gibt eine Warnung aus, wenn ein Track nicht als 22-kHz-Stereo kodiert ist. Beim Deaktivieren der Jukebox erklingt wieder der ursprüngliche Soundtrack der Map.

### Ressourcen

* [Q3Rally Website](http://www.q3rally.com)
* [Q3Rally auf ModDB](https://www.moddb.com/games/q3rally)
* [Q3Rally auf Discord](https://discord.gg/rX8Sxmh)

### Lizenz
Der Quellcode (Verzeichnis `engine`) steht unter der GPLv2 oder neuer, sofern nicht anders angegeben.

Für die Spieldateien (Verzeichnis `baseq3r`) ist keine Lizenz bekannt; behandle sie daher als nicht-kommerziell / nicht frei.
