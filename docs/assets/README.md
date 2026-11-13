# Ungenutzte Assets finden

`tools/assets/find_unused_assets.py` ermittelt, welche Bilder, Modelle und
Sounds in `baseq3r` das Spiel tatsächlich lädt:

```
python3 tools/assets/find_unused_assets.py . unused.json
```

Als benutzt gilt, was erreichbar ist über

- die Karten (BSP): Shader- und Fog-Lump, Entity-Werte (Modelle, Sounds, Remaps), Levelshots;
- die Shader-Skripte der benutzten Shader (`map`, `clampMap`, `animMap`, `videoMap`, `skyParms`);
  bei doppelt definierten Shadern zählt die Fassung, die der Renderer nimmt
  (die im alphabetisch ersten Skript, innerhalb einer Datei die erste);
- md3-Oberflächen und Skins benutzter Modelle, samt LOD-Stufen (`_1`, `_2`) und den
  Teilen, die der Code anhängt (`_flash.md3`, `_barrel.md3`, `_hand.md3`);
- Zeichenketten im Code, auch als Muster (`"models/players/%s/body.md3"`) und
  Shader-Namen ohne Pfad (`"rocketExplosion"`);
- Text-Konfigurationen (Arenen, cfg, Skins);
- ganze Ordner, die das Spiel zur Laufzeit auflistet (`models/players`, `music`, `fonts`).

Getrennt ausgewiesen: Bilder, die nur noch in einer Map-Quelle (`.map`) stehen oder
nur als Editorbild (`qer_editorimage`) dienen.

Grenzen: Sounds werden über Spielertabellen und zusammengesetzte Namen geladen, die
das Skript nicht sicher erkennt; die Sound-Liste ist deshalb nur ein Hinweis. Modelle,
die q3map2 in eine Karte einbackt, stehen nur in der Map-Quelle.

## Aufräumen 11/2026

Entfernt wurden 187 ungenutzte Dateien, die aus Quake 3 oder OpenArena stammen
(174 Bilder, 13 Modelle): 141 identisch mit OpenArena, 17 bearbeitete
OpenArena-Dateien, 20 sehr wahrscheinlich aus Quake 3, 9 weitere in Quake-3-Ordnern.
Die alten Waffentexturen fallen mit dem Waffen-PR weg.

Geprüft im Spiel: alle 35 Karten vorher und nachher geladen (mit allen Waffen),
keine neue Meldung über fehlende Bilder, Modelle oder Sounds.

Nicht entfernt: ungenutzte Q3Rally-eigene Dateien und solche ohne Quake-3- oder
OpenArena-Bezug (Mapobjekte wie Fahrzeuge, Pylonen, Laternen, Texturen für neue
Karten), Map-Quellen-Texturen, Editorbilder und alle Sounds.
