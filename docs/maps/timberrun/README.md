# Timber Run (q3r_timberrun)

Sprint von A nach B auf Schotter durch einen Herbstwald, früh am Morgen. Vom
Holzlagerplatz auf dem Rücken hinunter ins Bachtal und wieder hinauf zur Wiese.
Erzeugt von `tools/maptools/make_timberrun.py`. Konzept: Dokument „Timber Run –
Sprint-Strecke“.

Stand: **Stufe 1** (fahrbarer Rohbau, noch ohne Wald).

![Layout](timberrun_layout.png)

## Strecke

Länge 48.448 Einheiten, geschätzte Fahrzeit 88 Sekunden, Start auf z 1200, Ziel
auf z 1032, tiefster Punkt in der Furt (z 629). Kleinster Kurvenradius 451,
größte Steigung 17 % (Abfahrt der großen Kuppe). Alles Schotter (`SURF_GRAVEL`).

| Abschnitt | Länge | Charakter |
|---|---|---|
| Holzlagerplatz | 1.500 | Start, breit, offen |
| Forststraße | 6.300 | schnell, sanfte Bögen, zwei kleine Sprungkuppen |
| Hangkehren | 9.350 | drei Kehren den Hang hinunter |
| Bachtal und Furt | 6.050 | Gefälle zum Bach, Furt, Gegenanstieg |
| Fichtendickicht | 7.400 | eng (halbe Breite 152), Kurve an Kurve |
| Kahlschlag | 7.750 | breit und offen, lange schnelle Bögen |
| Hohlweg | 4.700 | bergauf zwischen Erdböschungen |
| Kuppe und Wiese | 4.500 | große Sprungkuppe, dann offen bergab |
| Ziel | 900 | Zielbogen auf der Wiese |

- **Sprungkuppen:** Die Auffahrt ist am Fuß ausgerundet, die Kante oben nicht,
  damit die Autos abheben. Klein: 7 % hinauf, 11 % hinunter, 28 hoch. Groß: 10 %
  hinauf, 16 % hinunter, 60 hoch. Sie liegen dort, wo die Straße in der Nähe der
  gewünschten Stelle am geradesten ist.
- **Furt:** Der Bach kreuzt die Straße im rechten Winkel. Das Wasser steht 12
  tief über 416 Einheiten Straße, ohne Quergefälle. Der Boden dort ist
  `SURF_GRAVEL` + `SURF_WET` (immer nasser Grip). Das Wasser ist eine ebene,
  nicht solide Fläche; der Boden liegt überall sonst darüber.
- **Hohlweg:** Böschungen bis 150 hoch auf beiden Seiten, egal wie das Gelände
  liegt.
- **Grenzen:** unsichtbare Wände auf beiden Seiten, 130 hinter dem Bankett, und
  quer an beiden Enden. Zwischen den Kehren rücken sie zusammen.
- **Checkpoints:** 24, vom Generator so gesetzt, dass die Bot-Route (`.bpd`,
  eine Bézier-Kurve je Abschnitt) höchstens 64 Einheiten von der Mittellinie
  abweicht und nie mehr als 2.048 Einheiten zwischen zwei Checkpoints liegen.
- **Wetter:** Regen, 40 %. Kein Schnee.

## Geprüft

- **Kompilieren:** kein Leck; BSP 12,7 MB. Gelände aus 33.220 Dreiecken.
- **AAS:** aus der Ersatzkarte gebaut (`docs/maps/aas_proxy.md`), 37.190 Areas,
  lädt ohne Fehler. bspc braucht dafür vier Minuten.
- **Bots, Sprint (`g_gametype 7`), 4 Bots, Stufe 4, ein Lauf:** Alle nutzen die
  BPD-Route (`G_RaceRoute: source=bpd`, 24 von 24 Griffen). Drei kamen ins Ziel,
  nach 111, 113 und 122 Sekunden; der vierte (Mini) war beim Rennende noch
  unterwegs.
- **Bilder:** alle Abschnitte in der Linux-Engine angesehen (Start, Kuppen,
  Kehren, Furt, Dickicht, Kahlschlag, Hohlweg, Ziel).

Noch offen für den Spieltest mit Menschen: Fahrzeit, Sprungweite der Kuppen,
Breite im Dickicht.

## Stufen

1. **Fahrbarer Rohbau** (dieser Stand): Gelände, Fahrbahn, Kuppen, Furt, Grenzen,
   Start, Ziel, Checkpoints, Bot-Route, AAS, Kurvenschilder.
2. Wald, Nebel, Licht, Prüfung der Bildrate.
3. Details, Sound, Wetter, Levelshot.

## Bauen

```text
python tools/maptools/make_timberrun.py
q3map2 -game quake3 -fs_basegame baseq3r -fs_basepath <repo> -meta -custinfoparms <map>
q3map2 -game quake3 -fs_basegame baseq3r -fs_basepath <repo> -vis -saveprt <map>
q3map2 -game quake3 -fs_basegame baseq3r -fs_basepath <repo> -light -custinfoparms -fast -shade -filter -super 2 -bounce 1 <map>
bspc -forcesidesvisible -optimize -bsp2aas <bsp>
```

Der Generator schreibt die Map, `maps/q3r_timberrun_index.pcx` (Bodenschichten),
`maps/q3r_timberrun.bpd`, `scripts/q3r_timberrun.shader` und `.arena`, die
Kamerafahrt in `intro_routes/`, die Texturen in `textures/q3r_timberrun/` sowie
`timberrun_layout.png` und `validation.json` hier. `shaderlist.txt` muss
`q3r_timberrun` enthalten.

Die AAS entsteht aus der Ersatzkarte:

```text
python tools/maptools/make_aas_proxy.py baseq3r/maps/q3r_timberrun.map
q3map2 -game quake3 -fs_basegame baseq3r -fs_basepath <repo> -bsp -custinfoparms baseq3r/maps/q3r_timberrun_aasproxy.map
bspc -forcesidesvisible -optimize -bsp2aas baseq3r/maps/q3r_timberrun_aasproxy.bsp
python tools/maptools/make_aas_proxy.py --checksum baseq3r/maps/q3r_timberrun_aasproxy.aas baseq3r/maps/q3r_timberrun.bsp
```

Der Himmel (`trainyard`) und die Bodentexturen sind in Stufe 1 Platzhalter.
