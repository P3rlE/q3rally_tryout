# Fahrerhelme

Die Fahrerköpfe in `baseq3r/models/players/heads` waren die Köpfe der Quake-3-Figuren (Ordner
`heads/quake3/` mit Sarge, Klesk, Sorlag, Xaero, Doom …). Diese Inhalte stammen von id Software und
stehen nicht unter der GPL. Q3Rally hat stattdessen jetzt eigene Rennhelme. Alles kommt aus einem Generator
im Repo:

```
python3 tools/carbuild/helmets/build_helmets.py
python3 tools/carbuild/helmets/build_helmets.py --sheet /tmp/helme.png   # Übersicht aller Icons
```

Gebraucht werden Python 3, numpy und Pillow. Ein Lauf dauert wenige Sekunden.

## Was entsteht

Für jeden Helm in `tools/carbuild/helmets/designs.py`:

| Datei | Inhalt |
|---|---|
| `models/players/heads/<name>.md3` | das Modell |
| `models/players/heads/<name>.skin` | welche Textur auf welche Fläche kommt |
| `models/players/heads/icon_<name>.tga` | Bild für Menü und Garage (128 × 128) |
| `models/players/heads/helmets/<name>.jpg` | die Lackierung |

Dazu kommen die gemeinsamen Teile in `helmets/`: Visiere, Glanz, Gummi, Sturmhaube. Außerdem wird
`scripts/heads.shader` geschrieben. Menü und Garage bauen die Helmliste aus den `.skin`-Dateien. Ein neuer
Helm braucht deshalb nur einen Eintrag in `designs.py`.

## Die Helme

- **Integralhelme mit Visier:** classic, racer, checker, flames, carbon, police, camo, neon, gold,
  lightning, tricolore, stars, sunset, arrow.
- **Offene Helme mit Sturmhaube und Brille:** retro, vintage, moto, pilot.

Standard ist `classic` (`DEFAULT_HEAD` in `cg_local.h` und `ui_local.h`, Vorgabe der Cvar `head`, Garage).
Wer noch einen alten Kopf eingestellt hat, bekommt beim Start `classic`. Das prüft
`UI_HeadExists()` in `ui_atoms.c`.

## Aufbau

- Der `tag_head` der Autos ist um 180 Grad um y gedreht (Achsen -x, y, -z). Die Kopfmodelle liegen
  deshalb wie die alten Köpfe auf dem Kopf und nach hinten gewandt in der Datei, mit dem Hals im Ursprung.
- Der Helm wird aufrecht modelliert, mit x nach vorn und z nach oben. `TagMesh` dreht ihn beim Schreiben
  und skaliert ihn auf 92 %, damit er ins Cockpit passt.
- Die Icons werden wieder aufrecht gerendert.
- Die Schale ist ein Ringnetz. Ring t geht von 0 am unteren Rand bis 1 am Scheitel, der Winkel phi läuft
  um den Kopf.
- Die Gesichtsöffnung ist pro Ring der Bereich zwischen -phi_e(t) und phi_e(t). Ihr Rand bleibt deshalb
  glatt, egal wie viele Ringe es gibt.
- Die Lackierungen werden auf dem 3D-Helm entworfen und nicht auf der flachen Textur. Jedes Texel kennt
  seinen Punkt auf der Schale. Streifen laufen deshalb sauber über den Scheitel, und Flammen und Zahlen
  sitzen an den Seiten.

## Bilder rendern

`tools/carbuild/mdlrender.py` ist ein kleiner Software-Renderer für md3-Modelle. Er macht die Icons und
lässt sich auch für Vorschauen anderer Modelle benutzen.
