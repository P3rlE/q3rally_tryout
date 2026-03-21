# Technisches Design: Synthesebasiertes Motorensoundsystem

## Zweck

Dieses Dokument ist der nächste Schritt nach der Architekturentscheidung in `docs/engine_audio_architecture_decision.md`.

Ziel ist ein konkret umsetzbares Design für einen ersten Implementierungszyklus:

- Datenfluss zwischen `cgame` und Client-Audio
- neue Datenstrukturen
- API-Vorschlag
- Modulgrenzen
- MVP-Umfang
- schrittweise Einführung ohne Bruch der bestehenden Sample-SFX-Pipeline

---

## Randbedingungen aus dem vorhandenen Code

Die aktuelle Audioarchitektur bietet bereits die wichtigsten Integrationspunkte:

- `S_RawSamples(...)` kann rohe PCM-Blöcke in den Client-Soundpfad einspeisen.
- `S_AddLoopingSound(...)` und `S_AddRealLoopingSound(...)` zeigen, dass entitätsgebundene Dauersounds pro Frame aus `cgame` angemeldet werden.
- `trap_S_UpdateEntityPosition(...)` und `trap_S_Respatialize(...)` zeigen, dass Positions- und Listenerdaten bereits im bestehenden Soundsystem gepflegt werden.
- `cg_main.c` registriert Q3Rally-spezifische Samples wie `turboSound` und `skidSound`; diese bleiben bestehen und werden nicht durch den Motorsynth ersetzt.
- `cg_playerstate.c` und die Fahrzeugphysik verwalten bereits clientseitig Zustände wie Fuel, RPM, Wheel- und Car-State.

Daraus folgt: Das neue System sollte sich an die bestehende Entitäts- und Listenerlogik anhängen, statt eine komplett separate Audiowelt aufzubauen.

---

## Nicht-Ziele für den ersten Implementierungsschritt

Dieser Entwurf vermeidet im ersten Durchlauf bewusst:

- vollständige Portierung von `engine-sim`
- neue Dateiformate für hochkomplexe Engine-Setups
- Änderungen an Crash-, Skid-, UI- oder Gameplay-Sounds
- harte Abhängigkeit von einem zusätzlichen Audiothread
- vollständige OpenAL/DMA-Neuarchitektur

Das erste Ziel ist ein robuster MVP für Motorsound, nicht ein voll generisches Prozedural-Audio-Framework.

---

## Zielbild des Datenflusses

## Übersicht

```text
Car physics / predicted player state / entity snapshots
        ↓
CG_BuildVehicleAudioState(entityNum)
        ↓
trap_S_UpdateVehicleSynth(entityNum, vehicleAudioState)
        ↓
client sound frontend (snd_main)
        ↓
engine synth manager (snd_enginesynth)
        ↓
PCM block generation per active voice
        ↓
existing raw/stream path in sound backend
        ↓
spatialized playback in current mixer/backend
```

Der entscheidende Entwurfspunkt ist: **`cgame` liefert Zustände, der Soundcode synthetisiert Audio.**

Damit bleibt die Syntheselogik vollständig im nativen Client-Code und muss nicht in die VM verschoben werden.

---

## Datenschnittstelle zwischen `cgame` und Soundsystem

## Neuer Zustandstyp

Vorgeschlagene Struktur im gemeinsamen Header, vorzugsweise in einer kleinen öffentlichen Sound-API-Datei oder in `snd_public.h`:

```c
typedef struct vehicleAudioState_s {
    int     entityNum;
    int     serverTime;
    float   rpm;
    float   throttle;
    float   load;
    float   clutch;
    float   speed;
    float   wheelSlip;
    float   lateralSlip;
    float   damage;
    float   turbo;
    int     gear;
    qboolean airborne;
    qboolean listenerIsInside;
    vec3_t  origin;
    vec3_t  velocity;
    vec3_t  forward;
} vehicleAudioState_t;
```

## Felder und Herkunft

- `rpm`  
  Direkt oder indirekt aus vorhandenem Fahrzeug-/Player-State.
- `throttle`  
  Für Last und Schubgeräusch; falls noch nicht clientseitig verfügbar, zunächst aus lokaler Eingabe bzw. heuristisch ableiten.
- `load`  
  Näherung aus Drehzahländerung, Gang, Geschwindigkeit und Gasstellung.
- `clutch`  
  Für Schalt- und Entkopplungsgeräusche; im MVP notfalls heuristisch.
- `speed`  
  Für Distanz-/Drive-by-Mischung und Fallback-Heuristiken.
- `wheelSlip` / `lateralSlip`  
  Für Traktions- und Schubverhalten; Skid-Sample bleibt separat.
- `damage`  
  Für raueren Lauf und Unregelmäßigkeiten.
- `turbo`  
  Für spätere Kompressor-/Blowoff-/Spool-Komponenten.
- `gear`  
  Für Lastmodell und Shift-Transienten.
- `listenerIsInside`  
  Für Innen-/Außenfilterung.
- `origin`, `velocity`, `forward`  
  Für entitätsgebundene Wiedergabe, Doppler und Richtcharakteristik.

---

## API-Vorschlag

## CGame → Client Sound Frontend

Neuer Trap-/Frontend-Aufruf:

```c
void trap_S_UpdateVehicleSynth( const vehicleAudioState_t *state );
```

Alternative mit expliziter Lebensdauersteuerung:

```c
void trap_S_BeginVehicleSynthFrame( void );
void trap_S_UpdateVehicleSynth( const vehicleAudioState_t *state );
void trap_S_EndVehicleSynthFrame( void );
```

### Empfehlung

Die Frame-Version ist sauberer, weil sie das bestehende Muster von `S_ClearLoopingSounds()` + `S_AddLoopingSound()` spiegelt:

1. Frame beginnen
2. alle aktiven Fahrzeuge melden
3. nicht mehr gemeldete Synth-Voices sauber ausfaden

Das reduziert Zombie-Voices und vereinfacht Priorisierung.

---

## Client Sound Frontend → Synth Manager

Neue interne API in `snd_enginesynth.h`:

```c
void S_EngineSynth_Init( void );
void S_EngineSynth_Shutdown( void );
void S_EngineSynth_BeginFrame( int serverTime );
void S_EngineSynth_UpdateVehicle( const vehicleAudioState_t *state );
void S_EngineSynth_EndFrame( void );
void S_EngineSynth_GenerateBlock( int samples );
```

### Aufgabenverteilung

- `snd_main.c`
  - öffentliche API annimmt
  - an Synth-Manager weiterreicht
- `snd_enginesynth.c`
  - Voice-Lebenszyklus
  - Priorisierung
  - Parameter-Glättung
  - PCM-Erzeugung
- `snd_dma.c` / `snd_openal.c`
  - bleibt zunächst nur Empfänger der generierten PCM-Daten

---

## Voice-Management

## Neue Runtime-Strukturen

```c
typedef enum engineSynthQuality_e {
    ESYNTH_QUALITY_FULL,
    ESYNTH_QUALITY_NEAR,
    ESYNTH_QUALITY_FAR
} engineSynthQuality_t;

typedef struct engineSynthVoice_s {
    qboolean            active;
    qboolean            touchedThisFrame;
    int                 entityNum;
    int                 lastUpdateTime;
    engineSynthQuality_t quality;
    vehicleAudioState_t target;
    vehicleAudioState_t smoothed;

    float               phase;
    float               firingPhase;
    float               intakeNoiseState;
    float               exhaustState[4];
    float               mechanicalState[2];
    float               transientGain;
    float               damageJitter;

    int                 rawStreamId;
} engineSynthVoice_t;
```

## Regeln

- Eine Voice pro aktiver Fahrzeug-Entity.
- `touchedThisFrame == qfalse` am Frame-Ende bedeutet: ausfaden und freigeben.
- Parameter werden geglättet, nie hart übernommen.
- `rawStreamId` ist stabil pro Voice oder pro Entity reserviert.

---

## Wahl des Audioausgabepfads

## MVP-Entscheidung: Raw-Stream pro aktiver Voice

Für den ersten Schritt sollte die Ausgabe über `S_RawSamples(...)` erfolgen.

### Vorteile

- nutzt vorhandenen PCM-Einspeisepunkt
- vermeidet invasive Änderungen an `sfx_t` / `channel_t`
- ermöglicht schnelles Prototyping

### Nachteile

- Stream-Management muss sauber geplant werden
- echte Integration in das Loop-/Channel-System kommt erst später

### Konkret vorgeschlagen

- reservierter Streambereich für Fahrzeug-Synths
- z. B. `MAX_CLIENTS` dedizierte Streams oder ein kleiner fester Pool
- Stream-ID aus `entityNum` oder Voice-Slot ableiten

Wenn sich später zeigt, dass dieser Pfad für Mehrfahrzeug-Rennen zu unflexibel ist, kann in einem zweiten Schritt eine echte prozedurale Kanalart ergänzt werden.

---

## Synth-Modell für den MVP

Der MVP muss hörbar gut, aber mathematisch überschaubar bleiben.

## Layer 1: Combustion Pulse

- Pulse-Frequenz aus `rpm`
- Amplitude aus `load` und `throttle`
- leichter Jitter bei Damage oder Limiter

Formelidee:

- firing frequency ≈ `(rpm / 60.0) * combustion_events_per_rev`

## Layer 2: Exhaust Resonator

- 2 bis 4 gekoppelte Resonatoren / Bandpasszüge
- Parameter abhängig von Fahrzeugprofil und RPM
- Hauptquelle für „Charakter“ im Außenmix

## Layer 3: Intake / Broadband Noise

- noise-basierte Quelle
- Gain steigt mit `throttle`
- Innenansicht bekommt mehr Anteil

## Layer 4: Mechanical Texture

- hochfrequente, schwächere Komponente
- Stärke mit RPM und Schaden skalieren

## Layer 5: Transienten

Im MVP nur zwei Ereignisse:

- **Shift cut** bei Gangwechsel
- **Limiter flutter** nahe Redline

Backfire, Turbochirp und Fehlzündungen können in Phase 2 folgen.

---

## Fahrzeugprofile / Content-Format

Für den ersten Codezyklus genügt ein kleiner fest definierter Datensatz im Code, etwa:

```c
typedef struct engineSynthProfile_s {
    const char *name;
    int   cylinders;
    float idleRpm;
    float redlineRpm;
    float pulseSharpness;
    float exhaustResonance;
    float intakeGain;
    float mechanicalGain;
    float limiterRange;
} engineSynthProfile_t;
```

### Einführungspfad

1. Zunächst ein globales Default-Profil für alle Fahrzeuge.
2. Danach ein kleines Mapping pro Fahrzeugklasse.
3. Erst später externe Datenbeschreibung pro Fahrzeug.

Damit vermeiden wir eine zu frühe Content-Pipeline-Diskussion.

---

## CGame-Integration im Detail

## Neue Hilfsfunktion

Vorgeschlagen in `cg_ents.c` oder einem neuen `cg_vehicle_audio.c`:

```c
static void CG_BuildVehicleAudioState( centity_t *cent, vehicleAudioState_t *out );
```

## Verantwortlichkeiten

- Fahrzeugposition und Geschwindigkeit aus `cent->lerpOrigin` / Snapshotdaten
- Lokales Fahrzeug bekommt reichhaltigere Daten aus `cg.car` und `cg.predictedPlayerState`
- Remote-Fahrzeuge verwenden nur replizierte oder heuristisch ableitbare Daten
- Innen-/Außenflag über Kamera- oder Chase-View bestimmen

## Aufrufzeitpunkt

Empfohlen pro Render-/CGame-Frame:

1. `trap_S_BeginVehicleSynthFrame()`
2. alle relevanten Fahrzeuge sammeln und melden
3. `trap_S_EndVehicleSynthFrame()`

Naheliegende Aufruforte:

- in oder nahe `CG_EntityEffects(...)` für entitätsbasierte Sounds
- alternativ ein dedizierter Pass nach Entity-Update und vor Sound-Respatialization

### Empfehlung

Ein dedizierter Pass ist sauberer als das Verstecken in `CG_EntityEffects(...)`, weil:

- der Motor-Synth kein klassischer Sample-Loop ist
- Priorisierung über alle Fahrzeuge zentral leichter fällt
- lokale und entfernte Fahrzeuge konsistenter behandelt werden können

---

## Priorisierung und Qualitätsstufen

## Prioritätsmetrik

Pro Fahrzeug ein Score aus:

- Entfernung zum Listener
- Sichtbarkeit / Relevanz
- Relativgeschwindigkeit
- ob eigenes Fahrzeug
- ob Gegner unmittelbar in der Nähe

Beispiel:

```text
score = own_vehicle_bonus
      + distance_weight
      + relative_speed_weight
      + visibility_weight
```

## Qualitätszuordnung

- bestes Fahrzeug: `FULL`
- nächste 3 bis 5 Fahrzeuge: `NEAR`
- restliche aktive Fahrzeuge im Budget: `FAR`
- darüber hinaus: keine Voice oder kompletter Mute

### Beispielbudget für MVP

- 1× `FULL`
- 4× `NEAR`
- 4× `FAR`

Das Budget soll als CVar konfigurierbar werden, sobald das System stabil läuft.

---

## Parameter-Glättung und Update-Raten

Motorparameter dürfen nicht 1:1 aus Snapshots übernommen werden.

### Regeln

- RPM und Load: exponentielle Glättung
- Gangwechsel: explizite Event-Erkennung
- plötzliche Snapshotlücken: kurzer Hold statt harter Nullung
- Voice-Ausblendung: mindestens 50 bis 150 ms Ramp-Down

Damit vermeiden wir Knackser und unnatürliche Modulation.

---

## MVP-Implementierungsplan

## Phase 1: Frontend-Schnittstelle

- neue `vehicleAudioState_t`-Struktur definieren
- neue Trap-/Frontend-Funktionen anlegen
- Stub-Implementierung im Soundcode

## Phase 2: Minimaler Synth

- Voice-Manager mit einem globalen Default-Profil
- Combustion + Exhaust + Intake
- Ausgabe über Raw-Streams
- nur lokales Fahrzeug aktivieren

## Phase 3: Mehrfahrzeug-Unterstützung

- Priorisierung
- Qualitätsstufen
- Remote-Fahrzeuge mit reduzierten Parametern

## Phase 4: Zusatzeffekte

- Shift cut
- Limiter flutter
- Damage roughness
- Innen-/Außenmischung

---

## Offene Fragen

Diese Punkte sollten vor der ersten Runtime-Implementierung geklärt werden:

1. Welche Fahrzeugparameter sind für Remote-Fahrzeuge tatsächlich repliziert?
2. Soll `throttle` netzwerkseitig übertragen oder heuristisch rekonstruiert werden?
3. Reicht `S_RawSamples(...)` für mehrere parallele Motor-Streams stabil aus?
4. Soll die Voice-Priorisierung in `cgame` oder im nativen Soundcode final entschieden werden?
5. Wird mittelfristig eine eigene prozedurale Kanalart benötigt?

---

## Empfohlene nächste Codeänderung

Die nächste konkrete Implementierung sollte **noch keinen echten DSP-Synth** enthalten, sondern zuerst die Schnittstelle und den Datenfluss etablieren:

- `vehicleAudioState_t` definieren
- `trap_S_BeginVehicleSynthFrame()` / `trap_S_UpdateVehicleSynth()` / `trap_S_EndVehicleSynthFrame()` ergänzen
- Stub-Manager in `snd_main.c` / neuem `snd_enginesynth.c`
- `cgame` meldet zunächst nur das lokale Fahrzeug mit RPM, Position und Geschwindigkeit

Damit können wir den End-to-End-Pfad verifizieren, bevor wir tief in die Klangmodellierung einsteigen.
