# Racing-Gamemode – Verbesserungen und offene Punkte

## Verbesserungsvorschläge

1. **Teamzeit am HUD konsistent berechnen** ✅
   Die Teamzeit berücksichtigt jetzt standardmäßig `finishRaceTime - startRaceTime` (bzw. den laufenden Split) und fällt nur bei Sprintstrecken (`laplimit <= 1`) auf die Lap-Uhr zurück. Spieler ohne gültigen Rennstart werden aus der Durchschnittsberechnung ausgeschlossen, um Null-Divisionen und verzerrte Werte zu verhindern.【F:engine/code/cgame/cg_rally_hud2.c†L108-L151】

2. **Stabilere Positions-Tie-Breaker** ✅
   Der Positionsvergleich nutzt jetzt eine 1-Unit-Toleranz (`RALLY_POSITION_DIST_EPSILON`), bevor Splits (`lastCheckpointTime`) und
   Restdistanzen als Tie-Breaker herangezogen werden. Fahrer mit fehlendem Marker (Sentinel `1 << 30`) fallen automatisch hinter
   Konkurrenten mit gültigen Daten zurück, womit Gleichstandssituationen deterministischer aufgelöst werden.【F:engine/code/game/g_rally_racetools.c†L16-L75】【F:engine/code/game/g_rally_racetools.c†L451-L520】

3. **Startaufstellung fehlertoleranter machen**  
   Fehlen ausreichend `info_player_start`-Marker, fällt die Spawnlogik auf Deathmatch-Spawns zurück – inklusive Telefrag-Risiko. Eine Verbesserung wäre, Fahrzeuge in die Zuschauerrolle zu versetzen, bis ein Grid-Platz frei wird, oder zusätzliche Grid-Slots dynamisch entlang der Startlinie zu erzeugen, anstatt die Runde mit chaotischen Spawns zu beginnen.【F:engine/code/game/g_rally_racetools.c†L802-L829】

## Bekannte Issues

* ~~**Team-Zeit Anzeige inkorrekt:** Wie oben beschrieben, summiert der HUD-Code nur die letzte Runde pro Teammitglied und zeigt daher falsche Zeiten an.~~ Behoben: Der HUD-Code ermittelt die Teamzeit inzwischen über den gesamten Rennlauf.【F:engine/code/cgame/cg_rally_hud2.c†L108-L151】
* **Fallback-Spawns gefährden faire Starts:** Fehlende Grid-Marker zwingen Spieler auf Deathmatch-Spawns, obwohl der Kommentar bereits auf eine alternative Lösung hinweist (`FIXME`). Das erzeugt Telefrag-Gefahr und sollte kurzfristig adressiert werden.【F:engine/code/game/g_rally_racetools.c†L818-L829】
