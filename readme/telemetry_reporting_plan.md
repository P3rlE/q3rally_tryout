# Q3Rally Telemetry Reporting Plan

This document captures the planned telemetry payloads and delivery mechanics for automated match reporting. It targets the dedicated server build described in this repository and focuses on race-centric modes introduced by Q3Rally alongside the inherited arena game types.

## 1. Kennzahlen nach Spielmodus

All payloads share a common player object structure:

```json
{
  "playerId": "sha256:...",   // hash of cl_guid + name
  "displayName": "Player",
  "team": "red" | "blue" | null,
  "car": "carmodel/skin",
  "normalizedScore": 0.0,
  "rawScore": 0,
  "damageDealt": { "raw": 1234, "normalized": 0.52 },
  "damageTaken": { "raw": 800, "normalized": 0.33 },
  "timeAlive": "PT12M3.542S"
}
```

* **Spielerkennung:** use the server-side `cl_guid` (see `cl_main.c`) salted with the server-provided namespace and hashed with SHA-256 to keep PII out of transit.【F:engine/code/client/cl_main.c†L124-L136】【F:engine/code/client/cl_main.c†L3654-L3746】
* **Zeitformat:** represent all absolute timestamps as RFC 3339 UTC strings and all durations as ISO 8601 durations (`PTmmMss.mmmS`). Game logic already keeps race timers in milliseconds, so conversion happens right before serialisation.【F:engine/code/cgame/cg_rally_hud.c†L602-L625】【F:engine/code/cgame/cg_scoreboard.c†L620-L625】
* **Normalisierung:** map raw metrics to `0.0–1.0` floats per match scope (e.g., divide by session max or configured cap). Where the engine already reports percentages (`accuracy`), normalisation divides by `100`.

### 1.1 Rennen (GT_RACING, GT_RACING_DM)

| Kennzahl | Beschreibung | Normalisierung |
| --- | --- | --- |
| `completedLaps` | Anzahl abgeschlossener Runden | Teilen durch `track.totalLaps` aus der Level-Info.
| `bestLap` | Schnellste Rundenzeit | Dauer als ISO 8601, zusätzlich `normalizedBestLap = bestLapMs / sessionBestLapMs`.
| `totalTime` | Gesamtzeit bis Zielflagge | Dauer; `normalizedTotalTime = winnerTimeMs / playerTimeMs` (>= 1.0 capped auf 1.0).
| `position` | Platzierung laut Scoreboard | `normalizedPosition = 1 - ((position-1)/(gridSize-1))`.
| `startReaction` | Zeit zwischen Grünlicht und Start | Normierung gegen `g_eliminationStartDelay` falls gesetzt.【F:engine/code/game/g_rally_racetools.c†L236-L379】
| `boostUsage` | Prozentualer Anteil der Boost-Zeit | Rohwert / `g_maxBoostMs` (neuer Server-CVar, siehe Abschnitt 3).

Für `GT_RACING_DM` werden außerdem `damageDealt` und `damageTaken` aus dem Scoreboard übernommen.【F:engine/code/cgame/cg_servercmds.c†L122-L145】

### 1.2 Team-Rennen (GT_TEAM_RACING, GT_TEAM_RACING_DM)

Zusätzlich zu den Einzelrenn-Kennzahlen:

* `teamScoreFraction` = Team-Summe `score` / Summe aller Team-Scores.
* `teamDamageShare` = Spieler-Schadensanteil relativ zu seinem Team (`playerDamage / teamDamage`).
* `relaySegments` = Anzahl übergebener Staffeln (für eventuelle Staffelmodi, falls `g_teamRelay` aktiv wird).

### 1.3 Eliminierung & Last Car Standing (GT_ELIMINATION, GT_LCS)

| Kennzahl | Beschreibung | Normalisierung |
| --- | --- | --- |
| `survivalTime` | Dauer bis Eliminierung | Dauer + Anteil relativ zur längsten Überlebenszeit.
| `elimOrder` | Reihenfolge der Ausscheidung | `normalizedElimOrder = 1 - ((elimOrder-1)/(gridSize-1))`.
| `lapsAtElim` | Geschaffte Runden vor Eliminierung | Anteil an `track.totalLaps`.
| `warningResponses` | Reaktionen auf Eliminierungswarnungen | Durchschnittliche Verzögerung / `g_eliminationWarning`.【F:engine/code/game/g_rally_racetools.c†L236-L379】

### 1.4 Demolition Derby (GT_DERBY)

| Kennzahl | Beschreibung | Normalisierung |
| --- | --- | --- |
| `knockouts` | Anzahlen eliminierter Gegner | Teilen durch `maxOpponents`.
| `ringOuts` | Selbstverschuldete Eliminierungen | Invers normalisiert: `1 - min(selfRingOuts, limit)/limit`.
| `armorIntegrity` | Durchschnittliche Rest-HP | Durchschnitt / `maxHealth` (aus Fahrzeugdefinition).
| `damageEfficiency` | `damageDealt / damageTaken`, auf 0–1 gescaled über `tanh`.

### 1.5 Deathmatch & Team Deathmatch (GT_DEATHMATCH, GT_TEAM)

| Kennzahl | Beschreibung | Normalisierung |
| --- | --- | --- |
| `frags` | Eliminierungen | Teilen durch höchsten Fragwert im Match.
| `deaths` | Tode | `1 - (deaths / maxDeaths)`.
| `accuracy` | Treffergenauigkeit | `cg.scores[i].accuracy / 100`.【F:engine/code/cgame/cg_servercmds.c†L126-L140】
| `streaks` | Längste Abschussserie | Teilen durch `fraglimit` (oder Session-Max).
| `powerupUptime` | Zeit mit aktiven Power-Ups | Dauer / Matchdauer.

Team-Variante ergänzt `teamScoreFraction` und `teamDamageShare` wie bei Team-Rennen.

### 1.6 Capture the Flag & Varianten (GT_CTF, GT_CTF4)

| Kennzahl | Beschreibung | Normalisierung |
| --- | --- | --- |
| `captures` | Eroberte Flaggen | Teilen durch `capturelimit` oder Match-Max.| 
| `returns` | Zurückgebrachte Flaggen | Teilen durch höchste Anzahl im Match.
| `carrierKills` | Eliminierungen des Flaggen-Trägers | Teilen durch Session-Max.
| `escortTime` | Zeit neben eigenem Carrier | Dauer / Matchdauer.
| `assistCount` | Scoreboard `assistCount` geteilt durch Session-Max.【F:engine/code/cgame/cg_servercmds.c†L126-L140】

### 1.7 Domination (GT_DOMINATION)

| Kennzahl | Beschreibung | Normalisierung |
| --- | --- | --- |
| `controlTicks` | Kontrollierte Zeit-Slots | Teilen durch Gesamtzahl der Slots.
| `contestedEvents` | Anzahl erzwungener Neutralisationen | Teilen durch Session-Max.
| `objectiveDamage` | Schaden an Kontrollpunkten | Schaden / höchster Punkteschaden.

### 1.8 Single Player (GT_SINGLE_PLAYER)

Singleplayer nutzt die Renn-Kennzahlen, ergänzt um `aiDifficulty` und `retryCount` (Anzahl Restarts).

## 2. Web-API Entwurf

### 2.1 Transport & Authentisierung

* **Protokoll:** HTTPS mit TLS 1.2+.
* **Methode:** `POST /v1/matches` für Match-Endberichte; optionale `POST /v1/pings` für Herzschläge.
* **Authentisierung:** statischer API-Key via Header `Authorization: Bearer <key>`. Replay-Schutz über `X-Q3R-Timestamp` (Unix ms) und HMAC-Signatur im Header `X-Q3R-Signature` (`HMAC-SHA256` über Body).

### 2.2 Request-Schema (`/v1/matches`)

```json
{
  "matchId": "srv-20240405-183011-42",
  "server": {
    "name": "Q3Rally EU #1",
    "host": "203.0.113.10:27960",
    "build": "1.3.0",
    "map": "q3r_country01"
  },
  "mode": "GT_RACING",
  "startTime": "2024-04-05T18:30:11Z",
  "endTime": "2024-04-05T18:42:39Z",
  "duration": "PT12M28S",
  "settings": {
    "g_gametype": 141,
    "g_eliminationInterval": 15000,
    "g_vehicleHpMaxRatio": 1.2
  },
  "players": [ { ...player metrics... } ],
  "teams": [
    {
      "team": "red",
      "rawScore": 123,
      "normalizedScore": 0.64,
      "damageDealt": 4200,
      "objectives": {
        "captures": 2,
        "controlTicks": 35
      }
    }
  ],
  "events": [
    {
      "timestamp": "2024-04-05T18:33:15.210Z",
      "type": "lap_completed",
      "playerId": "sha256:...",
      "lap": 2,
      "lapTime": "PT1M12.250S"
    }
  ]
}
```

* `mode` entspricht `gametype_t` Enum im Code (z.B. `GT_RACING`).【F:engine/code/game/bg_public.h†L129-L157】
* `settings` spiegelt relevante CVars wider, damit Analysten Spielparameter nachvollziehen können (z.B. `g_eliminationInterval`).【F:engine/code/game/g_rally_racetools.c†L236-L379】
* `events` sind optional und nur aktivierbar, wenn feineres Tracking gewünscht ist (konfigurierbarer Schalter `sv_telemetryEvents`).

### 2.3 Response-Schema

```json
{
  "matchId": "srv-20240405-183011-42",
  "status": "accepted",
  "ingestedAt": "2024-04-05T18:42:40Z",
  "nextPollAfter": 0,
  "warnings": [
    { "code": "metrics/unknown", "detail": "Field settings.g_vehicleHpMaxRatio ignored" }
  ]
}
```

* `status` Werte: `accepted`, `queued`, `rejected`.
* Bei `rejected` enthält Antwort `errors` mit Feldpfaden.
* HTTP-Status 202 für `queued`, 200 für `accepted`, 400 für Schemafehler, 401/403 für Auth-Probleme, 429 für Rate Limits.

### 2.4 Fehler- und Retry-Strategie

* Server speichert letzte 10 Payloads im lokalen Spool (`telemetry/outbox`).
* Bei HTTP 5xx oder Netzwerkfehlern: Exponentielles Backoff (Start 5 s, Cap 5 min) und erneuter POST.
* Ein `202 queued` verlangt Polling über `GET /v1/matches/{matchId}` (optional) – nicht aktiv standardmäßig.

## 3. Serverkonfiguration

### 3.1 Neue CVars

| CVar | Default | Beschreibung |
| --- | --- | --- |
| `sv_telemetryEnabled` | `0` | Globaler Schalter – nur wenn `1`, werden Payloads erzeugt und versendet.|
| `sv_telemetryUrl` | `` | HTTPS-Endpoint (z.B. `https://telemetry.example.com/v1/matches`).|
| `sv_telemetryApiKey` | `` | Secret für `Authorization` Header; gespeichert als latched CVar (nicht im `serverinfo`).|
| `sv_telemetryTimeoutMs` | `5000` | Netzwerk-Timeout für POSTs.|
| `sv_telemetryEvents` | `0` | Aktiviert das optionale `events`-Array.|
| `sv_telemetryMaxBatch` | `8` | Anzahl gespeicherter Match-Reports im Spool.|
| `sv_telemetryNamespace` | `"default"` | Salt für Spielerhashes, um GUID-Kollisionen zwischen Communities zu vermeiden.|
| `sv_telemetryMaxBoostMs` | `15000` | Obergrenze zur Normalisierung von Boost-Zeiten (verwendet bei Renn-Kennzahlen).|

Die neuen CVars folgen dem bestehenden Muster der Server-seitigen Steuerung (vgl. `sv_enableRankings` & `sv_rankingsActive`).【F:engine/code/server/sv_rankings.c†L58-L137】

### 3.2 Config-Datei

* Ergänzung des `server_example.cfg` um kommentierte Defaults sowie Empfehlung, Secrets in eine separate Datei `telemetry_secrets.cfg` auszulagern, die via `exec telemetry_secrets.cfg` eingebunden wird.
* Deployment-Dokumentation verlinkt diese Datei und weist auf Dateirechte hin (nur Server-User lesen).

### 3.3 Aktivierungsablauf

1. Betreiber trägt Ziel-URL und API-Key in `telemetry_secrets.cfg` ein.
2. Aktiviert Telemetrie mit `seta sv_telemetryEnabled "1"` und optional `sv_telemetryEvents`.
3. Server initialisiert Outbox beim Start, generiert Namespaces (`sv_telemetryNamespace`) und beginnt ab dem nächsten Match mit POSTs.
4. Health-Monitor im Spiel (neuer Befehl `telemetryStatus`) listet letzte Antwortcodes.

### 3.4 Sicherheitsaspekte

* `sv_telemetryApiKey` wird als latched, write-only CVar umgesetzt, analog zu `rconPassword`, sodass sie nicht über `serverinfo` oder Status-Responses geleakt werden (Server code already guards sensitive CVars).【F:engine/code/server/sv_client.c†L66-L66】【F:engine/code/server/sv_ccmds.c†L174-L199】
* Logging reduziert: Erfolgsmeldungen nur auf Debug-Level, Fehler inkl. HTTP-Status auf Warn-Level.
* Optionale IP-Allowlist (`sv_telemetryAllowedIPs`) kann später ergänzt werden.

---

Diese Planung liefert eine konsistente Datenbasis über alle Spielmodi hinweg, berücksichtigt bestehende Engine-Datenstrukturen und beschreibt Infrastruktur-Erweiterungen, die mit den vorhandenen Server-Konfigurationsmechanismen vereinbar sind.
