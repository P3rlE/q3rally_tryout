# Laternen-Modelle

Vier Modelle für Laternen in den Maps, als `misc_model` gesetzt (q3map2 backt sie in die
BSP). Erzeugt von `tools/mapobjects/make_lanterns.py` (Python 3 + numpy), Shader in
`scripts/q3r_lanterns.shader`, Dateien unter `models/mapobjects/q3r_lanterns/`.

| Modell | Was | Ursprung | Maße (Units) |
|---|---|---|---|
| `lantern.md3` | Hängelaterne: Ring, Kaminhaube, überstehendes Dach, nach unten schmaler werdendes Glas mit Eisenrahmen, Boden mit Spitze, Flamme innen | Oberkante des Rings, hängt nach unten | 22 breit, 48 hoch |
| `lantern_post.md3` | Gusseiserner Laternenpfahl mit Sockel, Ringen, Kugelkopf, geschwungenem Arm mit Zierschnecke; die Laterne hängt an zwei Kettengliedern | Boden, Arm zeigt nach +x | 160 hoch, Laterne 34 vom Pfahl |
| `lantern_wall.md3` | Wandarm mit Platte, Strebe und Laterne | Wandfläche, Arm nach +x | Arm 22, Laterne hängt bis 51 unter den Ansatz |
| `streetlamp.md3` | Natriumdampf-Straßenlaterne: konischer Mast, Schwanenhals, flacher Leuchtenkopf mit glühender Unterseite | Boden, Arm nach +x | 328 hoch, Kopf 88 vom Mast, Glas bei z 316 |

Die Ausrichtung setzt der Schlüssel `angle` (0 = Arm nach +x).

## Licht und Kollision

Die Modelle leuchten selbst (Glas und Flamme flackern leicht über `rgbGen wave noise`,
die Linse der Straßenlaterne glüht), werfen aber kein Licht. Daneben gehört eine
`light`-Entity:

- Laternenpfahl: bei (34, 0, 106) relativ zum Ursprung, warm (1 0.72 0.38), 190–220.
- Wandlaterne: bei (22, 0, -33) relativ zum Wandansatz.
- Straßenlaterne: unter dem Kopf, bei (88, 0, 290), Natriumgelb (1 0.72 0.38), 320.

`misc_model` kollidiert nicht. Wo Autos gegen den Pfahl fahren können, kommt ein
unsichtbarer, fester Brush um den Schaft (Laternenpfahl 10 × 10, Straßenlaterne 12 × 12).
Arm und Laterne hängen über Autohöhe.

## Wo sie stehen

- `q3r_david4` (Jungle Opening): 12 Laternenpfähle an den Erdwegen, 2 an der
  Quad-Platte, 4 Wandlaternen an den Ruinentoren.
- `q3r_ctf4_03` (Industrial Crossfire): 24 Straßenlaternen an den Kaistraßen.

`shaderlist.txt` muss `q3r_lanterns` enthalten, damit q3map2 die Shader kennt.
