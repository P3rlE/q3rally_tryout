# API-Vertrag: Player Profile v1 (verbindlich)

**Status:** Accepted  
**Gültig ab:** 2026-03-28  
**Version:** `profile.v1`

Dieses Dokument ist der verbindliche Vertrag zwischen **Engine**, **Ladder-Service/PHP-Webservice** und **UI** für Spieler-Profile.

## 1) Verbindlicher API-Vertrag

### 1.1 Identität und Authentifizierung

- **Kanonisches Feld für Identität:** `user_id` (String, nicht leer).
- **Legacy-Alias:** `playerId`.
- **Normative Regel:** Sender SOLLEN `user_id` senden. Empfänger MÜSSEN `user_id` bevorzugen und `playerId` als Legacy-Fallback akzeptieren.

- **Auth-Token für schreibende Endpunkte:**
  - HTTP Header: `Authorization: Bearer <token>`
  - Token wird serverseitig geprüft.
  - Ohne gültiges Token: `401 Unauthorized`.

### 1.2 Profil-Transport im Match-Payload

Profil-Daten werden pro Spieler im Match-Payload transportiert:

```json
{
  "players": [
    {
      "user_id": "abc123",
      "displayName": "RacerOne",
      "profile": {
        "schema_version": "v1",
        "name": "RacerOne",
        "country": "DE",
        "birthdate": "1995-04-17",
        "gender": "unspecified",
        "avatar_ref": "avatars/racerone.png"
      }
    }
  ]
}
```

## 2) Profilschema v1

### 2.1 Felder

| Feld | Typ | Pflicht | Regeln |
|---|---|---:|---|
| `schema_version` | String | ja | Muss `"v1"` sein |
| `name` | String | ja | 1..64 Zeichen |
| `country` | String \\ `null` | nein | ISO-3166-1 alpha-2 empfohlen (z. B. `DE`, `US`) |
| `birthdate` | String \\ `null` | nein | Format `YYYY-MM-DD` |
| `gender` | String \\ `null` | nein | `male`, `female`, `non_binary`, `unspecified`, `other` |
| `avatar_ref` | String \\ `null` | nein | Referenz/URI auf Avatar-Asset |

### 2.2 JSON-Schema (normativ)

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "$id": "q3rally/profile.v1.schema.json",
  "title": "Q3Rally Profile v1",
  "type": "object",
  "additionalProperties": false,
  "required": ["schema_version", "name"],
  "properties": {
    "schema_version": { "const": "v1" },
    "name": { "type": "string", "minLength": 1, "maxLength": 64 },
    "country": { "type": ["string", "null"], "pattern": "^[A-Z]{2}$" },
    "birthdate": { "type": ["string", "null"], "format": "date" },
    "gender": {
      "type": ["string", "null"],
      "enum": ["male", "female", "non_binary", "unspecified", "other", null]
    },
    "avatar_ref": { "type": ["string", "null"], "minLength": 1, "maxLength": 256 }
  }
}
```

## 3) Legacy-Kompatibilität

Alte Profile mit nur Name bleiben lesbar.

### 3.1 Zulässiges Legacy-Format

```json
{ "name": "OldSchoolRacer" }
```

### 3.2 Upgrade-/Leseregel

Wenn ein Profil nur `name` enthält (kein `schema_version`):

- beim Lesen als `v1` interpretieren:
  - `schema_version = "v1"`
  - `country = null`
  - `birthdate = null`
  - `gender = "unspecified"`
  - `avatar_ref = null`
- die Originaldaten bleiben kompatibel lesbar (kein harter Migrationszwang).

## 4) Verteilung der Verantwortung (Engine/PHP/UI)

### Engine
- sendet für neue Clients `user_id` + `profile.schema_version = "v1"`.
- darf zusätzlich `playerId` bis Abschluss der Migration mitsenden.

### PHP/Ladder-Service
- akzeptiert `user_id` und `playerId`.
- nutzt intern kanonisch `user_id`.
- wendet Legacy-Leseregeln aus Abschnitt 3 an.

### UI
- rendert Profile immer gegen das normalisierte v1-Modell.
- zeigt fehlende optionale Daten neutral an (z. B. „—“).

## 5) Versionierung und Change-Policy

- Breaking Changes nur über `schema_version`-Erhöhung (`v2`, …).
- `v1` bleibt für bestehende Clients lesbar.
- Erweiterungen in `v1` nur rückwärtskompatibel (optionale Felder).
