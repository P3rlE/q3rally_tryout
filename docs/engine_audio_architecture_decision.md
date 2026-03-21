# Architekturentscheidung: Synthesebasiertes Motorensoundsystem

## Ziel

Q3Rally soll ein neues Motorensoundsystem erhalten, das den Motorsound **zur Laufzeit synthetisiert** statt feste RPM-Samples zu blenden.

Der erste Architekturentscheid betrifft die Grundrichtung:

1. **Sample-basiertes RPM-Looping weiter ausbauen**
2. **`engine-sim` vollständig in Q3Rally integrieren**
3. **Ein eigenes, leichtgewichtiges Q3Rally-Synthesizersystem bauen, inspiriert von `engine-sim`**

**Entscheidung:** Wir verfolgen Option 3.

---

## Ausgangslage im aktuellen Code

Die bestehende Engine bietet bereits mehrere Bausteine, die für ein prozedurales Soundsystem nützlich sind:

- Das Client-Audio-API unterstützt klassische One-Shot- und Loop-Sounds sowie rohe PCM-Zufuhr über `S_RawSamples(...)`. Das ist der naheliegendste Einstiegspunkt für einen Laufzeit-Synthesizer. 
- In `cgame` werden Fahrzeug- und Welt-Entities bereits kontinuierlich in die Soundwelt eingespeist; Loop-Sounds werden pro Entity im Frame neu angemeldet.
- Q3Rally registriert aktuell fahrzeugbezogene Samples wie Turbo- und Skid-Sounds, aber noch keinen Motor-Synth.
- Die Fahrzeugphysik führt bereits drehzahlbezogene Zustände wie `rpm` im Car-State mit.

Damit ist die technische Grundlage vorhanden, um ein neues Motorsystem **ergänzend** in die bestehende Soundpipeline einzuhängen, ohne die komplette Audioarchitektur ersetzen zu müssen.

---

## Entscheidung

Wir bauen ein **neues Q3Rally-spezifisches Engine-Audio-System**, das:

- den Klang pro Fahrzeug in Echtzeit synthetisiert,
- vorhandene Fahrzeugzustände wie RPM, Last, Gang und Schlupf als Eingänge verwendet,
- die existierende Sample-SFX-Pipeline für andere Sounds weiter nutzt,
- konzeptionell auf Ideen aus `engine-sim` aufsetzt,
- aber **nicht** dessen gesamte Laufzeit- und Tool-Architektur 1:1 übernimmt.

Kurzform:

> **Motor = Synthese**
>
> **Crash / Skid / Turbo / UI / Gameplay = bestehende Sample-SFX**

---

## Warum nicht beim Sample-System bleiben?

Ein ausgebautes RPM-Loop-System hätte zwar den kleinsten Implementierungsaufwand, löst aber die eigentlichen Qualitätsprobleme nicht:

- Starre Übergänge zwischen Sample-Bereichen bleiben hörbar.
- Lastwechsel, Schubbetrieb, Begrenzer und Schaltvorgänge bleiben nur grob approximiert.
- Für jedes Fahrzeug müssten mehrere Loops produziert, gepflegt und sauber abgestimmt werden.
- Fahrzeugcharaktere skalieren content-seitig schlecht, weil jede neue Variante neue Audioassets verlangt.

Für ein Rennspiel mit stark physikgetriebenem Fahrgefühl bringt ein prozeduraler Ansatz langfristig deutlich mehr Nutzen.

---

## Warum nicht `engine-sim` vollständig einbetten?

`engine-sim` ist als Referenz sehr wertvoll, aber eine Vollintegration wäre für Q3Rally voraussichtlich zu schwergewichtig.

### Hauptgründe

- Q3Rally läuft auf einer alten Quake-III-Audioarchitektur mit eigenem Mixer- und Streamingmodell.
- Im Spiel müssen mehrere Fahrzeuge gleichzeitig stabil und günstig berechnet werden.
- Die Runtime-Anforderungen eines Rennspiels unterscheiden sich von einer dedizierten Motor-Audio-Sandbox.
- Eine 1:1-Übernahme würde Build-, Plattform- und Wartungsrisiken erhöhen.

### Konsequenz

Wir übernehmen bevorzugt:

- Modellideen,
- Signalfluss,
- Parameterisierung,
- akustische Teilquellen,

aber bauen die eigentliche Runtime-Integration als **kleinere, Q3Rally-native Implementierung**.

---

## Zielarchitektur

## 1. Fahrzeugzustand als Audio-Eingang

Pro relevanter Fahrzeug-Entity wird ein kompakter Audiozustand benötigt, mindestens mit:

- `rpm`
- `throttle`
- `engine_load`
- `gear`
- `clutch`
- `speed`
- `wheel_slip`
- `damage`
- `listener_distance`
- `camera_mode` (Innen/Außen)

Dieser Zustand soll clientseitig aus bereits vorhandenen oder leicht ergänzbaren Daten in `cgame` abgeleitet werden.

## 2. Synthesizer pro Fahrzeug

Für jedes hörbare Fahrzeug existiert eine Synth-Instanz mit eigenem DSP-Zustand:

- Phasenlage / firing state
- Resonator- und Filterzustände
- Transientenstatus (z. B. Shift-Cut, Backfire)
- Distanz-/Perspektivmischung

## 3. Audioausgabe in den bestehenden Mixer

Der Synth erzeugt kleine PCM-Blöcke und speist diese in die bestehende Soundengine ein.

Bevorzugte Integrationsrichtung:

- erst über einen Raw-/Streaming-Pfad,
- später optional über eine dedizierte prozedurale Soundquellenklasse.

---

## Inhaltlicher Entwurf des Synths

Das System soll nicht mit festen Loops arbeiten, sondern mit mehreren Teilquellen:

1. **Combustion Pulse Layer**  
   Grundcharakter aus Zündereignissen und Drehzahl.

2. **Exhaust Layer**  
   Resonanter, lastabhängiger Auspuffanteil.

3. **Intake Layer**  
   Ansauggeräusch, stärker bei Last und Innenperspektive.

4. **Mechanical Layer**  
   Ventiltrieb, Getriebezirpen, rauere Texturanteile.

5. **Transient Layer**  
   Schalten, Begrenzer, Fehlzündungen, Gaswegnahme.

6. **Damage Layer**  
   rauerer Lauf, Unregelmäßigkeiten, Aussetzer.

Dieses Schichtmodell ist nah genug an `engine-sim`, um davon zu profitieren, bleibt aber klein genug für Q3Rally.

---

## Performance-Strategie

Das System muss von Anfang an auf mehrere Fahrzeuge ausgelegt sein.

### Qualitätsstufen

- **Stufe A:** Eigenes Fahrzeug, volle Qualität
- **Stufe B:** Nahe Gegner, reduzierte Synthtiefe
- **Stufe C:** Entfernte Gegner, vereinfachtes Modell

### Weitere Regeln

- feste maximale Zahl aktiver High-Quality-Stimmen
- keine Heap-Allokationen im Audiopfad
- kleine feste Ringbuffer
- vorallozierte Voice-Strukturen

---

## Auswirkungen auf die Codebasis

Die Entscheidung bedeutet zunächst **Dokumentation und Schnittstellendesign**, noch keine große Audio-Refaktorierung.

Voraussichtlich betroffene Bereiche in späteren Schritten:

- `engine/code/client/snd_public.h`
- `engine/code/client/snd_main.c`
- `engine/code/client/snd_dma.c`
- `engine/code/client/snd_openal.c`
- `engine/code/cgame/cg_ents.c`
- `engine/code/cgame/cg_playerstate.c`
- `engine/code/cgame/cg_local.h`

Zusätzlich wird ein neues clientseitiges Modul für Motor-Audio nötig sein, z. B.:

- `engine/code/client/snd_enginesynth.h`
- `engine/code/client/snd_enginesynth.c`

---

## Ergebnis dieses ersten Schritts

Mit dieser Entscheidung legen wir fest:

- **Kein weiterer Ausbau eines reinen Sample-Loop-Systems**
- **Keine Vollportierung von `engine-sim`**
- **Ja zu einem Q3Rally-eigenen Laufzeit-Synthesizer mit `engine-sim` als Vorbild**

Das ist die Grundlage für den nächsten Schritt: ein konkretes technisches Design mit Datenfluss, API-Vorschlag, Modulgrenzen und MVP-Scope. Dieses Folgedesign ist in `docs/engine_audio_technical_design.md` festgehalten.
