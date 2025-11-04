# Scoreboard & Player Stats UI Draft

## Relevante Menüs und Einstiegspunkte
- `engine/ui/hud.txt` lädt das klassische `ui/score.menu` (Scoreboard-Overlay) sowie `ui/teamscore.menu` für Teamvarianten. Diese Ressourcen bestimmen, wann der Code in `cg_scoreboard.c` aktiv wird.
- Spielerinformationen werden zusätzlich über `ui/ingame_player.menu` (aus `engine/ui/ingame.txt`) eingebunden, was den Einstiegspunkt für weiterführende Statistiken bildet.
- Die tatsächliche Darstellung des Scoreboards erfolgt im Client-Modul `engine/code/cgame/cg_scoreboard.c`; alle neuen Tabs greifen hier ein und ersetzen keine bestehenden `.menu`-Dateien.

## Neues Tab-/Sektionen-Layout
| Tab | Inhalt | Zweck |
| --- | --- | --- |
| **Overview** | Teamzuordnung, Gesamtpunkte, aktuelle Position, Ping. | Schneller Überblick über Platzierung und Netzwerkstatus. |
| **Lap Times** | Beste Runde, letzte Runde, Gesamtzeit, absolvierte Checkpoints. | Fokus auf Renn- und Time-Attack-Modi. |
| **Combat** | Verteilte und erlittene Schadenspunkte, Trefferquote, Eliminierungen, Assists. | Für Kampfmodi (DM, Racing-DM, Derby). |
| **Player Stats** | Gesammelte Items, genutzte Boost-Pads, Top-Speed, Accuracy. | Ergänzende Performance-Daten für Renn- und Hybrid-Modi. |

Tabs liegen oberhalb des Tabellenkopfes, teilen sich die Scoreboard-Breite und nutzen farbliche Hervorhebung (sekundäres Blau für aktiv, graue Sekundärfarbe für inaktive Tabs). Die Detailfläche liegt unterhalb der Spielerlisten und spiegelt die Auswahl (`cg.selectedScore`) wider.

## Datenpfad und neue Strukturen
- **Struktur `playerStats_t`** (`cg_local.h`): hält Best-/Letzte Runde, Gesamtzeit, Checkpoints, Schaden, Items, Boosts, Accuracy, Top-Speed, Eliminierungen, Assists.
- **Client-Statusfelder** in `cg_t`: `activeScoreboardTab`, `mockStatsInitialized`, `playerStats[]`, `playerStatsValid[]` kapseln den Tab-Zustand und die Daten pro Client.
- **Neue VM-Cvars** (`cg_main.c`):
  - `cg_scoreboardTab` – ermöglicht das Vorwählen eines Tabs (persistiert, kann auch via Konsole gesetzt werden).
  - `cg_scoreboardMockData` – aktiviert die Mock-Datenbefüllung für Layouttests.

Server-zu-Client-Werte sollen später über Configstrings oder neue Serverkommandos gefüllt werden; die Struktur ist dafür vorbereitet.

## Mock-Daten & Workflow
1. `cg_scoreboardMockData 1` setzen → `CG_ApplyMockScoreboardData` erzeugt sechs Beispiel-Fahrer inkl. Teamfarben, Zeiten, Kampfstats.
2. Optional `cg_scoreboardTab <0-3>` setzen, um Tabs direkt zu testen.
3. Scoreboard öffnen (`TAB`/`Scores`) → Tabs + Detailpanel erscheinen mit Mock-Strings (Farben & Abstände geprüft).
4. Mit `cg_scoreboardMockData 0` wird der Mock-Zustand zurückgesetzt (`cg.numScores = 0`, Stats geleert).

Diese Grundlage erlaubt Designer:innen, Textelemente, Localization-Strings und visuelle Änderungen ohne Serveranbindung zu überprüfen.
