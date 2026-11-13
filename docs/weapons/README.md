# Waffen: „Gebaut, nicht gekauft“

Die Waffenmodelle aus Quake 3 und OpenArena sind durch eigene Modelle ersetzt.
Jede Waffe ist ein Umbau aus Autoteilen und steht auf derselben Dachplatte
(Gunmetal-Basis, orange Schnellwechselplatte, vier Schrauben). Alle teilen sich
einen Texturatlas; geglüht wird nur dort, wo der Schuss herkommt.

| Taste | Anzeigename | Früher | Umbau aus | Dreht sich |
|---|---|---|---|---|
| 1 | Harvester Saw | Chainsaw | Sägekopf einer Holzerntemaschine auf Hydraulikarm | Lüfterrad hinten (beim Sägen), Kette läuft immer |
| 2 | Spotlight MG | Machinegun | Rallye-Zusatzscheinwerfer mit Dreifachrotor | Rotor |
| 3 | Twin Shotgun | Shotgun | Sportauspuff: ovaler Dämpfer, zwei schräge Endrohre, gelochtes Hitzeblech | – |
| 4 | Drum Launcher | Grenade Launcher | gelochte Bremsscheibe als Trommel, Bremssattel als Verschluss | Bremsscheibe |
| 5 | Quad Launcher | Rocket Launcher | vier Endrohre im Carbonkasten, blau angelaufene Spitzen, Hitzeschild | – |
| 6 | Spark Gun | Lightning Gun | Zündspule im Käfig, zwei Zündkerzen, orange Zündkabel | – (Funke flackert) |
| 7 | Turbo Railgun | Railgun | Turbolader hinten, zwei Carbonschienen mit Spulenringen, mintgrünes Glühen | Verdichterrad |
| 8 | Plasma Cooler | Plasma Gun | Ladeluftkühler mit Endkästen, Düse mit blauen Sichtfenstern, Silikonschlauch | – |
| 9 | BFG Blower | BFG10K | Roots-Kompressor, Hutze nach vorn als Mündung, grüner Kern | Riemenscheibe |
| 0 | Nitro Flamer | Flame Thrower | liegende Druckflasche mit Q3R-Band, Druckminderer, Nachbrennerdüse, blaue Zündflamme | – |

Nur die Anzeigenamen sind neu (Pickup-Meldung, Waffenwahl, Steuerungsmenü,
Bot-Lieblingswaffe, Bot-Chat, zwei Todesmeldungen). Entity-Klassennamen,
Waffennummern und Botdateien bleiben gleich; `give` und `BG_FindItem` kennen
die alten Namen weiterhin (`give Railgun` funktioniert also noch).

## Dateien

- `tools/carbuild/prim.py`: Grundkörper für kleine md3-Modelle (Box, Zylinder,
  Ring, Loft, Schlauch, Bohrung, Kreuzquads) mit automatischen Atlas-UVs.
- `tools/carbuild/weapons/atlas.py`: der gemeinsame Atlas `parts.tga`
  (512 × 512, 4 × 4 Materialfelder, eins davon geteilt in vier Glühfarben),
  dazu `chrome.tga`, `lampflash.tga`, `chain.tga`, `spark.tga`, `pilot.tga`.
- `tools/carbuild/weapons/build_weapons.py`: baut alle Waffen und
  `scripts/q3r_weapons.shader`:

  ```
  python3 tools/carbuild/weapons/build_weapons.py            # alle
  python3 tools/carbuild/weapons/build_weapons.py spark_gun  # nur eine
  ```

Pro Waffe entstehen `<waffe>.md3` (mit `tag_flash`, bei drehenden Teilen auch
`tag_barrel`) und, wo sich etwas dreht, `<waffe>_barrel.md3`. Die alten
Mündungsfeuer (`<waffe>_flash.md3`) bleiben. Die alten LOD-Stufen
(`*_1.md3`, `*_2.md3`) der Waffenkörper sind gelöscht, sonst würde die Engine
auf Entfernung wieder die alten Modelle zeigen.

Modellraum: x nach vorn (Mündung), y nach links, z nach oben, Ursprung auf
`tag_weapon` des Autos. Länge 24 bis 32 Einheiten, höchstens gut 10 hoch,
500 bis 1.300 Dreiecke pro Waffe.
