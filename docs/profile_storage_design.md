# Profil-Speicherdesign

## Profilfelder
| Feld            | Datentyp | Beschreibung |
|-----------------|----------|--------------|
| `kilometer`     | `float`  | Gesamtzahl der gefahrenen Kilometer. |
| `rennen`        | `int`    | Anzahl der absolvierten Rennen. |
| `siege`         | `int`    | Anzahl der gewonnenen Rennen. |
| `verbrauch`     | `float`  | Durchschnittlicher Kraftstoffverbrauch (z. B. Liter/100 km). |
| `rekorde`       | `array`  | Liste mit Strecken-Rekorden; einzelne Einträge bestehen aus Objekten mit Streckenkennung, Zeit und Datum. |

## Speicherort
Profile werden im Konfigurationsverzeichnis des Spiels relativ zu `fs_homepath` abgelegt. Der vollständige Pfad folgt dem Muster:

```
profiles/<playerId>.json
```

Dabei entspricht `<playerId>` der eindeutigen Spielerkennung.

## Versionierung und Migration
Jedes Profil enthält im Wurzelobjekt einen Header `schemaVersion`. Die aktuelle Implementierung startet mit Version `1`. Bei Änderungen an der Struktur wird die Versionsnummer erhöht. Beim Laden wird `schemaVersion` geprüft und ggf. eine Migrationsroutine aufgerufen, die ältere Versionen stufenweise auf die aktuelle Version anhebt. Nicht unterstützte oder unbekannte Versionen werden mit einem Fehlereintrag protokolliert und führen zum Anlegen eines neuen Profils.

## I/O-Grundstruktur (Pseudocode)
```pseudo
function loadProfile(playerId):
    path = fs_homepath + "/profiles/" + playerId + ".json"
    if fileExists(path):
        data = readJson(path)
        if needsMigration(data.schemaVersion):
            data = migrateProfile(data)
        return data
    else:
        return initializeProfile(playerId)

function initializeProfile(playerId):
    profile = {
        schemaVersion: 1,
        playerId: playerId,
        kilometer: 0.0,
        rennen: 0,
        siege: 0,
        verbrauch: 0.0,
        rekorde: []
    }
    saveProfile(profile)
    return profile

function saveProfile(profile):
    path = fs_homepath + "/profiles/" + profile.playerId + ".json"
    ensureDirectoryExists(dirname(path))
    writeJson(path, profile)
```
