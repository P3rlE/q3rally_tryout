# Synthetisierte Sounds

Sechs Sounds trugen noch Quake-3-Namen und klangen nach Quake 3. Sie werden jetzt synthetisiert,
aus Rauschen und Oszillatoren mit numpy. Aufgenommen oder gesampelt ist nichts:

```
python3 tools/soundbuild/build_sounds.py               # schreibt nach baseq3r/sound/
python3 tools/soundbuild/build_sounds.py --out /tmp/s  # woanders hin
python3 tools/soundbuild/build_sounds.py --wav         # zusätzlich .wav zum Anhören
```

Gebraucht werden numpy und ffmpeg mit libvorbis. Ein Lauf dauert ein paar Sekunden.

| Datei | Länge | Was es ist | Wo |
|---|---|---|---|
| `sound/world/lava1.ogg` | 16,1 s | Dampf zischt in Schüben über tiefem Grollen, dazu einzelne Blasen | `q3r_lavafalls`, 8 Lautsprecher |
| `sound/world/fireloud.ogg` | 6,6 s | Fauchendes Feuer mit Knistern und einzelnen Knallern | `q3r_lavafalls`, 5 Lautsprecher |
| `sound/world/tim_comp_hum3.ogg` | 5,4 s | Netzbrummen mit Obertönen, Lüfter, Relais-Ticks, kurze Rechner-Piepser | `q3r_lavafalls`, 2 Lautsprecher |
| `sound/world/tim_hole.ogg` | 2,5 s | Luftsog in fünf Schüben, die Resonanz fällt in jedem Schub ab | `q3r_lavafalls`, 1 Lautsprecher |
| `sound/weapons/melee/fstrun.ogg` | 2,0 s | Gauntlet beim Feuern: kleiner Zweitakt-Motor | Gauntlet |
| `sound/weapons/melee/fstatck.ogg` | 0,77 s | Gauntlet-Treffer: metallischer Schlag über kurzem Hochdrehen | Gauntlet |

## Was gleich bleibt

- Dateinamen, Abtastrate (22 kHz, das Feuer 44 kHz), Mono, Länge und Bitrate. Die Karten und der Code bleiben unverändert.
- Die Lautheit (RMS) jedes Sounds. Die Zielwerte stehen in `SOUNDS` im Skript, damit die Mischung im Spiel so bleibt wie vorher. Wird ein Sound beim Anheben zu laut, sättigt er weich (tanh), statt zu clippen.

## Nahtlose Loops

Die Umgebungsgeräusche und das Gauntlet-Laufgeräusch laufen in Schleife. Damit es an der Naht nicht klickt:

- Das Rauschen wird per FFT erzeugt und ist damit periodisch.
- Einzelne Ereignisse (Blasen, Knackser, Ticks) laufen über das Dateiende auf den Anfang weiter.
- Töne haben eine ganze Zahl von Schwingungen in der Länge der Datei.
- Filter laufen zweimal durch, der zweite Durchgang startet mit dem Zustand vom Ende.

Geprüft habe ich die Naht über die zweite Ableitung: Der Übergang vom Ende zum Anfang ist nicht größer als normale Stellen im Signal.

## Schon eigene Sounds

Ansager (`sound/feedback/`), Powerup-Sounds (`sound/items/`) und Menü-Sounds (`sound/misc/menu*`, `silence`)
tragen zwar Quake-3-Namen, sind aber eigene Aufnahmen von Q3Rally.
