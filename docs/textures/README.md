# Eigene Karten- und Effekt-Texturen

Die Karten benutzten noch 129 Bilder, die mit hoher Wahrscheinlichkeit aus Quake 3 stammen
(Herkunftsklasse C der Asset-Prüfung, siehe `docs/assets/README.md`). Dazu kamen 27 Effektbilder
(Powerup-Spiegelungen, Feuer, Blitze, Rail-Effekte, Ladebildschirm) und drei Modelle. Alles davon ist
jetzt durch eigene Inhalte ersetzt. Alle Pixel kommen aus einem Generator im Repo:

```
python3 tools/texbuild/build_textures.py                  # alle Texturen nach baseq3r/
python3 tools/texbuild/build_textures.py --only light     # nur Rezepte, deren Name passt
python3 tools/texbuild/build_textures.py --out /tmp/x --sheet /tmp/x/vergleich.jpg --orig /pfad/zu/alten/texturen
```

Ein kompletter Lauf dauert etwa zehn Minuten. Die meiste Zeit brauchen die beiden CTF-Flaggen (1024×2048).
Gebraucht werden Python 3, numpy, scipy und Pillow.

## Was gleich bleibt

- **Dateiname und Bildgröße.** Die BSPs und Shader verweisen weiter auf dieselben Pfade, keine Karte muss neu kompiliert werden.
- **Alpha-Kanal mit derselben Aufgabe.** Bei Gittern, Zäunen und Streben ist Alpha der Ausschnitt (`alphaFunc`). Bei glänzenden Platten (`panel_shiny`, `zinc_shiny`, `grill` usw.) steuert Alpha, wie viel Spiegelung durchscheint.
- **Mittlere Farbe.** `tools/texbuild/targets.json` enthält pro Textur nur die mittlere Farbe des alten Bildes (drei Zahlen). Der Generator gleicht jede neue Textur daran an, damit Beleuchtung und Stimmung der Karten erhalten bleiben. Die Skybox `env/xnight2_*` ist davon ausgenommen, sonst würden die Sterne eingefärbt.
- **Lage der Lichter.** Leuchttexturen und ihre Glüh-Ebenen (`*.blend.jpg`, `*_glow`, `runwayb`, `tilefloor7_owfx`, `bluemetalsupport2clight.glow` …) werden aus denselben Formen gezeichnet und liegen deshalb genau übereinander.

## Aufbau

| Datei | Inhalt |
|---|---|
| `tex.py` | Werkzeugkasten: kachelbares Rauschen (FFT), Zellrauschen, Distanzfelder für Formen, Beleuchtung aus Höhenkarten, Speichern |
| `common.py` | Materialien (abgenutztes Blech, Rost, Riffelblech, Steinlagen) und die Rezept-Registrierung |
| `r_metal.py` | Metallwände, Böden, Leisten, Rohrwände, Säulen, Eisenwände |
| `r_light.py` | Lampen und ihre Glüh-Ebenen, glänzende Paneele mit Glühen |
| `r_cutout.py` | Gitter, Zaun, X- und Z-Streben, Kabel, Monitorrahmen |
| `r_stone.py` | Stein- und Ziegelwände, Putz, das große Wandbild in `q3r_ctf_01` |
| `r_nature.py` | Fels, Erde, Wolkenschichten |
| `r_special.py` | Skybox, Spiegelungs-Maps, Wasser, Teleporter, CTF-Flaggen, Modelltexturen |
| `r_effects.py` | Powerup- und Item-Spiegelungen, Feuer, Blitze, Rail-Kern und -Scheibe, Sonne, Ladebildschirm-Vignette |
| `build_models.py` | Eigene Geometrie für die CTF-Flaggen (`models/flags/*_flag.md3`) und den Blitzeinschlag (`models/weaphits/crackle.md3`), gebaut mit `tools/carbuild/prim.py` |

Die Modelle baut ein eigener Aufruf: `python3 tools/texbuild/build_models.py`. Die Flächen- und
Shadernamen sind dieselben wie vorher, damit wehendes Tuch, Chrom-Stange und Flammen-Emblem weiter
funktionieren. Das Tuch hat die alte Größe und Texturrichtung, das Emblem in der Kugel ist ein Rad.

Jedes Rezept ist eine Funktion mit `@recipe`, die `{Pfad: (Farbe, Alpha oder None)}` zurückgibt.
Zufall ist überall mit festen Seeds gesetzt, ein neuer Lauf ergibt also dieselben Bilder.

## Eigene Motive statt der alten

- Das Bodengitter `rusty_pentagrate` zeigt eine Felge mit fünf Speichen.
- Das zehnteilige Wandbild `gothic_cath/graycathflesh*` in `q3r_ctf_01` ist ein Spitzbogen aus Metall mit Lenkrad-Emblem, Mittelrippe und Messingbügeln.
- `iron01_p` trägt Radnaben statt Schädeln.
- Die CTF-Flaggen zeigen ein Lenkrad (blau) und eine karierte Raute (rot).
- Die Modelltexturen der Lampen (`spotlamp`, `gratetorch2b`) und des Schädels sind neu gezeichnet. Die Ausschnitte passen zu den UV-Flächen der Modelle, die direkt aus den MD3-Dateien gelesen werden.

## Bewusst nicht ersetzt

- `models/gibs/gibs.tga` trägt nur einen Quake-3-Namen. Inhaltlich ist es Q3Rallys eigener Trümmer-Atlas (Lenkrad, Lüfter, Schläuche).
- `textures/effects/tinfx.tga` (OpenArena) und `baslt4_1_2k/_4k` (ohne Quake-3-Bezug) bleiben, wie sie sind.
- Die alte Shotgun-Textur (`shotgun.tga`, `shotgun_spec.jpg`) und ihr Shader sind gelöscht. Seit den neuen Waffen benutzt sie kein Modell mehr.
