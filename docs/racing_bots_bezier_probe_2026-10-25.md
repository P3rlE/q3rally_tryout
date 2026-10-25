# Racing-Bots: Bezier-Pilot auf Downtown

## Versuchsaufbau

Die angehängte Map war `q3r_downtown.map`; eine Countdown-Map ist im lokalen Projekt nicht vorhanden. Für Downtown liegen BSP, Mapquelle und 25 passende lokale Ghosts vor.

Aus `q3r_downtown.bsp` wurden die Entity-Ursprünge der 26 Checkpoints und der Start-/Ziellinie gelesen. `tools/bpd_from_bsp.py` erzeugte daraus 27 BPD-Anker. Die Tangenten folgen dem Winkelhalbierenden der benachbarten Checkpoint-Segmente; die Handle-Länge beträgt ein Drittel des kürzeren Nachbarabstands, maximal 900 Einheiten.

Beide Läufe nutzten denselben aktuellen Linux-Spielmodul-Build, Downtown, vier Bots auf Skill 5, `bot_thinktime 100`, `timescale 1`, Seed 1337, NOS aus und 180 Spielsekunden. Der Ghost-Lauf lud alle 25 Dateien; das Spiel wählte die 53,05-s-Aufnahme `q3r_downtown_tl1_rev0_20260923-184307.ghost`. Der BPD-Lauf hatte keine Ghost-Dateien und lud die erzeugte BPD aus seinem isolierten Serververzeichnis.

## Ergebnisse

| Client | BPD, erste Runde | Ghost, erste Runde |
| --- | ---: | ---: |
| 0 | 101,3 s | 103,9 s |
| 1 | 99,2 s | 90,5 s |
| 2 | 103,3 s | 91,3 s |
| 3 | 96,9 s | 92,2 s |
| **Mittelwert** | **100,2 s** | **94,5 s** |

Alle vier Bots beendeten in beiden Läufen mindestens eine Runde und passierten alle Checkpoints. Der BPD-Lauf war im Mittel 5,7 Sekunden langsamer; seine schnellste Runde lag 6,4 Sekunden hinter der schnellsten Ghost-Runde.

Im Ghost-Lauf wurden zwei Not-Resets und Recovery-Zustände protokolliert. Der BPD-Zweig benutzt diese Recovery-Zustandsmaschine nicht; seine Telemetrie meldet dort deshalb stets `none`. Das ist kein fairer Stabilitätsvergleich. Der BPD-Lauf gab außerdem deutlich häufiger den negativen Fahrbefehl aus; der Messwert trennt Bremsen und Rückwärtsfahren nicht vollständig.

## Einordnung

Der Versuch vergleicht zwei vollständige Implementierungen: die alte Checkpoint-Bezier-Steuerung mit automatisch erzeugten Handles und die neuere gemeinsame Führung mit Ghost-Route. Er isoliert nicht die Qualität der Kurve. Die erzeugte BPD nutzt Checkpoint-Ursprünge als Anker; sie rekonstruiert keine vom Mapper gezeichnete Ideallinie und prüft die Kurve nicht auf Fahrbahnbreite oder Hindernisse.

Damit gewinnt die aktuelle automatische BPD nicht gegen die Ghost-Route. Das reicht noch nicht, um Bezier als Routenformat abzulehnen. Die BPD-Kurve und der alte Bot-Fahrer wurden gemeinsam getestet; außerdem schreibt der alte Fahrer den Kurvenparameter bei Zielnähe in festen 0,1-Schritten weiter.

## Empfehlung

- Ghost-Routen vorerst als leistungsstärkste Referenz behalten.
- Bezier als kompakte, mapperfreundliche statische Route ergänzend unterstützen.
- Ein Refinement-Werkzeug entwickeln, das Ghost- oder BSP-Fahrspuren in BPD-Handles umsetzt, Kurven visualisiert und Fahrbahnkollisionen prüft.
- BPD-Kurven durch dieselbe moderne Routenführung wie Ghosts fahren lassen. Erst dann lässt sich die Linienqualität direkt vergleichen.

## Fairer Vergleich mit gemeinsamer Fahrsteuerung

Der Folgeversuch nutzt denselben aktuellen Build und dieselbe `G_RaceRoute`-Führung für beide Quellen. Der BPD-Lauf bestätigte `source=bezier route=checkpoint_bezier`; der Ghost-Lauf bestätigte `source=ghost`. Beide Läufe hatten vier Bots auf Skill 5, Seed 1337, kein NOS, `bot_thinktime 100` und 120 Spielsekunden.

| Messwert | Automatische BPD | Ghost |
| --- | ---: | ---: |
| Einzelbot, erste Runde | **129,5 s** | **90,2 s** |
| Zeitdifferenz | **+39,3 s** | Referenz |
| Rückwärts-Fahrbefehl im Einzelbotlauf | 26,1 s | 8,1 s |
| Recovery-Samples im Einzelbotlauf | 399 | 322 |
| Vier Bots: erste Runde abgeschlossen | 0/4 | 3/4 |
| Vier Bots: Rundenzeiten der Finisher | keine | 89,6 s · 91,9 s · 96,8 s |

Der Einzelbotlauf trennt Linienqualität vom Verkehr: Die automatische BPD braucht 43,6 % länger als Ghosts und erzeugt über dreimal so viel Rückwärts-Fahrbefehl. Im Viererfeld blieben drei BPD-Bots am Ende bei Checkpoint 8 hängen; einzeln kommt der Bot dort vorbei. Der Verkehr macht den Mangel also schlimmer, ist aber nicht die Ursache des Zeitunterschieds. Bei Ghosts kam im Viererfeld ein Bot ebenfalls nur bis Checkpoint 22; Recovery und Mehrverkehr bleiben ein eigenes Problem.

Die Ursache des schlechten BPD-Kandidaten ist inzwischen klarer als „falsche Handle-Länge“: Der Generator hat `origin` aus den Map-Entities als Bézier-Anker verwendet. Laut Map-Entitätsdefinition ist `origin` aber der Respawn-Punkt; die Kurvenposition ist eine eigene Eigenschaft (`bezierPos`). Die automatisch erzeugten Anker liegen deshalb teils weit von der tatsächlichen Ghost-Linie entfernt. Bei Downtown beträgt der Abstand zwischen Respawn-Ursprung und nächstem Ghost-Sample an mehreren Checkpoints rund 300–470 Einheiten. Dieser Probelauf bewertet somit nur den fehlerhaften Ansatz „Respawn-Punkte als Streckenanker“, nicht das Bezier-Format an sich.

## Konsequenz

Die Ghost-Route bleibt vorerst die Leistungsreferenz; aus dem fehlerhaft verankerten Roh-Kandidaten folgt keine Ablehnung des Bezier-Formats. Der Fitter verwendet echte Checkpoint-Brushes und Ghost-Gate-Übertritte als Anker. Die neue SVG-Vorschau ergänzt einen ersten Sichtflächen-Check; danach sollte ein Fit mit einer bestätigten sauberen Ghostrunde folgen.

## Ghost-angepasster BPD-Prototyp

`tools/bpd_from_ghost.py` liest die tatsächlichen Checkpoint-Kollisionsbrushes aus der BSP, schneidet die Ghost-Samples in Checkpoint-Reihenfolge mit diesen Brushes und berechnet aus den Gate-Ankern gemeinsame Handles per regularisierter kleinster Quadrate. Das Werkzeug berücksichtigt den Fahrzeugkontakt mit einer einstellbaren 24-Unit-Erweiterung. Die resultierende Linie wurde von `G_RaceRoute` als `checkpoint_bezier` importiert und auf Checkpoint-Reihenfolge geprüft.

| Einzelbot, gleiche Steuerung | Rundenzeit | Rückwärtsbefehl | Recovery-Samples | Resets |
| --- | ---: | ---: | ---: | ---: |
| Roh-BPD aus Respawn-Ursprüngen | 129,5 s | 26,1 s | 399 | 1 |
| Ghost-angepasste BPD | **97,7 s** | **14,4 s** | **47** | **0** |
| Ghost-Route | 90,2 s | 8,1 s | 322 | 1 |

Die angepasste BPD ist im Einzelbot 31,8 s schneller als die grobe BPD und bleibt 7,5 s hinter der Ghost-Referenz. Der Ghost-angepasste Lauf hatte keine Rückwärtsbewegung und keinen Not-Reset. Die Probe-Ghostdatei selbst ist als `clean 0` markiert, daher bleibt sie ein vorläufiger Fit.

Im Viererfeld beendeten alle BPD-Bots die erste Runde. Die Zeiten lagen bei **92,7 / 106,0 / 96,6 / 101,0 s** (Mittel 99,1 s); es gab keine Resets und 35–74 Recovery-Samples pro Bot. Das ist ein deutlicher Fortschritt gegenüber der Roh-BPD, bei der im Viererfeld kein Bot die erste Runde beendete. Ein Bot hatte einen kurzen Recovery-Rejoin. Ghosts sind im Mittel weiterhin schneller, aber einer der vier Ghost-Bots blieb im Vergleichslauf vor dem Ziel hängen.

Der Fit nutzte 729 Ghost-Beobachtungen und erreichte 85,9 Einheiten RMS-Abstand, maximal 294,1. Zusammen mit der neuen Preview deckt die Prüfung nun auch ab, ob der Routenverlauf näherungsweise über sichtbaren Straßenflächen liegt. Die volle Fahrbahnbreite, Kollisionsbrushes und Hindernisse sind weiterhin nicht validiert. Als Nächstes ist ein Fit mit einer bestätigten sauberen Ghostrunde sinnvoll.

## Sichtflächen-Preview und Qualitätscheck

`tools/bpd_from_ghost.py` erzeugt nun zusätzlich eine SVG-Vorschau. Sie legt die Ghostspur (blau), den gefitteten BPD-Verlauf (orange), die Gate-Anker (grün) und Samples ohne erkannte Straßenfläche (pink) über die aus der BSP triangulierten sichtbaren Straßenmaterialien.

Für Downtown wurden 286 Straßenoberflächen mit 4.058 Dreiecken erkannt. Alle **732/732** BPD-Abtastpunkte liegen innerhalb der projizierten Dreiecksflächen und maximal 96 Höhen-Einheiten von einer Oberfläche entfernt. Das ist ein positiver Strecken-Plausibilitätscheck, aber noch keine Prüfung von Kollisionsbrushes, Fahrbahnbreite, Hindernissen oder Fahrzeugverhalten. Die Höhen-Toleranz kann mehrstöckige Straßen und Geometrie nahe der Strecke fälschlich als Treffer zählen.

Vorschau: `dist/bot-racing-input/downtown-ghost-fit/baseq3r/maps/q3r_downtown.svg`.

## Präzisions-Ghost von 2026-10-25

Die neue Aufnahme `q3r_downtown_tl1_rev0_20261025-171955.ghost` ist `clean=1`, `track_length=1`, `track_reversed=0` und 100,25 s schnell. Sie enthält 778 Frames. Der Fitter erkannte erneut alle 27 Gate-Übertritte und erzeugte 27 BPD-Anker. Mit 828 Ghost-Beobachtungen beträgt der Fit-Abstand 92,3 Einheiten RMS und maximal 261,1; 754 von 761 Routenstichproben (99,1 %) liegen auf den erkannten Straßenflächen bei 96 Einheiten Höhentoleranz.

Die Ghost-Quelle selbst liegt mit 778/778 Samples über den erkannten Straßenflächen. Der geglättete BPD-Fit erreicht 754/761; sieben Samples liegen in zwei Kurvenabschnitten außerhalb der erkannten Flächen. Ein höheres Handle-Limit (1200 statt 900) verbessert den RMS-Wert nur leicht (92,3 auf 91,9) und beseitigt diese Stellen nicht. Das deutet auf einen Genauigkeitsverlust des aktuellen gemeinsamen Bezier-Fits in diesen Kurven hin; die pinken Punkte sind in der SVG markiert.

Die Standarddatei wurde als `dist/bot-racing-input/downtown-precision-fit/baseq3r/maps/q3r_downtown.bpd` ausgegeben; die neue Vorschau liegt daneben als `q3r_downtown.svg`. Ein Spielbenchmark konnte in der aktuellen Windows-Umgebung nicht starten: Der Benchmark versucht, den Linux-Dedicated-Server auszuführen (WinError 193), und WSL verweigert den Zugriff (`Wsl/E_ACCESSDENIED`). Daher gibt es für diesen Kandidaten noch keine Bot-Rundenzeit.

### Manueller Windows-Lauf

Im anschließenden Spieler-Test lud das Spiel die BPD-Datei und fand keine Ghost-Datei. Der Bot kam laut Ergebnisbildschirm in **1:33,800** ins Ziel; der Spieler in **1:34,050**. Der Nutzer beobachtete bei schneller Kurvenfahrt deutliche Abweichungen von der Route und Wandkontakte.

Der Log zeigt jedoch `G_RaceRoute: source=checkpoint route=fallback`, nicht `source=bezier`. Der lokale `qagame.qvm` wurde um 14:00 Uhr gebaut, während der gemeinsame Bezier-Importer im aktuellen Workspace später geändert wurde. Der Ghost-Vergleich lief dagegen mit `source=ghost`. Damit vergleichen die beiden manuellen Läufe unterschiedliche Bot-Führungen; die Zeitdifferenz beweist nicht, dass die BPD-Linie allein schneller ist. Die Beobachtung der späten Kurvenbremsung passt zur alten direkten Bezier-Steuerung, die lokale Krümmung ohne den neueren Lookahead-Bremsweg auswertet.

### Neuer Windows-QVM-Lauf und Startankerfehler

Im nächsten Windows-Build wurde der gemeinsame Importer aktiv und erkannte `27/27` Handles. Er verwarf den Pfad bei der Checkpoint-Reihenfolge. Ursache war der BPD-Anker 27: Der bisherige Fitter mittelte die Grid-Startposition `(8064, 976, 180)` mit dem Ziellinien-Übertritt. Dadurch lag der Anker bei `y≈850`, außerhalb des Start-/Ziellinien-Triggers. Die erste tatsächliche Startlinienquerung liegt bei `t≈1039 ms, y≈731`; der letzte Übertritt bei `t≈100234 ms, y≈725`.

`tools/bpd_from_ghost.py` verwendet jetzt den Mittelwert dieser beiden echten Gate-Übertritte und fittet die geschlossene Runde ab der ersten Startlinienquerung. Der korrigierte Kandidat liegt unter `dist/bot-racing-input/downtown-precision-fit-gateanchor/baseq3r/maps/q3r_downtown.bpd`. Eine Offline-Simulation der Runtime-Kurvenabtastung und Checkpointreihenfolge findet nun alle 27 Gates. Das ist noch keine Ingame-Bestätigung; der nächste Windows-Lauf muss `Bezier ready` und `source=bezier route=checkpoint_bezier` melden.

### Windows-Lauf mit korrigierter BPD und Telemetrie

Der nächste Lauf bestätigte den Bezier-Pfad im Spiel: `Bezier ready waypoints=762` und `source=bezier route=checkpoint_bezier`. Tobias beendete die Runde in 1:45,650, der Spieler in 1:46,250. Die Datei `downtown_bezier_20261025.jsonl` enthält 1.056 Samples für die erste Bot-Runde.

Der Routenindex läuft vorwärts, aber der Sollkurs ändert sich an mehreren Checkpointübergängen innerhalb von 50–100 ms um 70–80 Grad. Dabei springt die Routenabweichung beispielsweise am Checkpoint 2 von 30 auf 75 Einheiten. Im bisherigen Code wurde bei 90 Grad abrupt vom angepeilten Gate auf die Tangente nach dem Gate umgeschaltet. Das passt zu den beobachteten Lenkwinkelsprüngen.

Die erste Runde lag bei 6,5 % der Samples mehr als 250 Einheiten neben der Route. Es gab 9 `off_route_rejoin`-Auslösungen und insgesamt 103 Samples im Zustand `rejoin_route`; die stärksten Abweichungen lagen unter anderem bei Checkpoint 18 und 23. Diese Kurvenabweichungen bleiben getrennt vom Gate-Ziel-Übergang zu untersuchen.

Der erste Korrekturschritt in `engine/code/game/ai_dmnet.c` ersetzt den harten 90-Grad-Wechsel durch einen Winkelblend: Ab 60 Grad wird zunehmend zur Ausfahrtsrichtung übergeblendet, ab 140 Grad vollständig. Das ist eine gezielte Änderung zur Glättung der beobachteten Sollkurs-Sprünge und braucht noch einen neuen Windows-Lauf zur Bestätigung.

Der darauffolgende Windows-Lauf enthielt `gate_target_blend`-Ereignisse und bestätigt damit, dass dieser Stand aktiv war. Tobias beendete die Runde in 1:42,400, der Spieler in 1:42,550. Die Telemetrie zeigt bei Checkpoint 16 einen nahezu vollständigen Stillstand: Solltempo 57,6, Isttempo unter 7, `driveCommand=0` für rund drei Sekunden. Die Beschleunigungstotzone von 80 Einheiten verhinderte das Anfahren, und dieselbe Schwelle ließ die Stuck-Erkennung keinen Vorwärtswunsch erkennen. Bei Checkpoint 22 blieb der Bot zusätzlich während eines `rejoin_route`-Manövers etwa eine Sekunde fast stehen.

Der aktuelle Stand reduziert die Beschleunigungstotzone unter 160 Geschwindigkeitseinheiten auf 20 und verwendet dieselbe Schwelle für den Stuck-Timer. Dadurch sollte der Bot bei niedrigem Kurventempo weiter Gas geben und bei ausbleibendem Fortschritt in die vorgesehene Recovery wechseln. Der nächste Lauf muss prüfen, ob beide beobachteten Stillstände damit verschwinden.

## Laufdaten

- BPD: `dist/bot-racing-runs/downtown-bezier-auto-4bots-180/result.json`
- Ghost: `dist/bot-racing-runs/downtown-ghost-4bots-180/result.json`
- Generierte Datei: `dist/bot-racing-input/downtown-auto-bpd/baseq3r/maps/q3r_downtown.bpd`
- Fairer BPD-Lauf: `dist/bot-racing-runs/downtown-bpd-common-120/result.json`
- Fairer Ghost-Lauf: `dist/bot-racing-runs/downtown-ghost-common-120/result.json`
- Einzelbot BPD: `dist/bot-racing-runs/downtown-bpd-single-180/result.json`
- Einzelbot Ghost: `dist/bot-racing-runs/downtown-ghost-single-180/result.json`
- Ghost-angepasster BPD-Lauf (Einzelbot): `dist/bot-racing-runs/downtown-bpd-ghost-fit-single-180/result.json`
- Ghost-angepasster BPD-Lauf (vier Bots): `dist/bot-racing-runs/downtown-bpd-ghost-fit-4bots-180/result.json`
- Ghost-angepasste BPD-Datei: `dist/bot-racing-input/downtown-ghost-fit/baseq3r/maps/q3r_downtown.bpd`
