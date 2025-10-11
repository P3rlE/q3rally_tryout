# PHP Ladder Webservice

Dieses Verzeichnis enthält einen minimalen Ladder-Endpunkt, der sich auf typischem Webspace mit PHP-Unterstützung betreiben lässt. Der Service benötigt keine zusätzlichen Bibliotheken und speichert eingehende Matches als JSON-Dateien im Unterordner `data/`.

## Deployment
1. Den gesamten Inhalt dieses Ordners (`index.php` und den leeren Ordner `data/`) auf den gewünschten Webspace hochladen.
2. Sicherstellen, dass PHP 8.0 oder neuer aktiviert ist und der Webserver Schreibrechte für den Ordner `data/` besitzt. Bei Bedarf dem Ordner über das Hosting-Panel Schreibrechte gewähren.
3. Nach dem Upload ist die API unter der URL des Webspaces erreichbar, zum Beispiel `https://example.com/ladder/index.php`.

> Hinweis: Viele Hoster setzen `index.php` automatisch als Startdatei. Liegt der Ordner direkt im Document-Root, kann die Basis-URL z. B. `https://example.com/` sein. Sonst ggf. den Unterordner an die URL anhängen.

## API-Übersicht

* **GET `/version`** – Liefert Informationen für den im Spiel integrierten Update-Check. Die Daten stammen aus `data/version.json` (oder optional `data/version.txt`).
* **POST `/matches`** – Speichert ein Match. Erwartet das JSON, das der Q3Rally-Server erzeugt (inklusive `matchId`). Bereits vorhandene IDs werden ignoriert und mit HTTP 200 quittiert.
* **GET `/matches`** – Liefert eine Liste aller gespeicherten Matches (neueste zuerst). Optional können `mode`, `limit` und `offset` als Query-Parameter gesetzt werden.
* **GET `/matches/{matchId}`** – Gibt das vollständige JSON zu einer Match-ID zurück.
* **DELETE `/matches/{matchId}`** – Löscht ein Match dauerhaft.

### Update-Informationen pflegen

Die Update-Prüfung des Spiels erwartet mindestens eine Versionsnummer, optional ergänzt um Download-Link und Hinweistext. Die Daten lassen sich auf zwei Arten hinterlegen:

1. **JSON (`data/version.json`)** – Standardweg. Beispiel:
   ```json
   {
     "latest": "v0.7",
     "downloadUrl": "https://downloads.example.com/q3rally-v0.7.zip",
     "message": "Bugfix-Release mit verbesserten Streckenzeiten."
   }
   ```
2. **Plaintext (`data/version.txt`)** – Für sehr einfache Setups. Zeile 1 = Version, Zeile 2 (optional) = Download-Link, restliche Zeilen = Nachricht.

> **Hinweis zu den Pfaden:** Alle Dateien, die den Versionsendpunkt konfigurieren, müssen im Unterordner `data/` neben `index.php` liegen. Das betrifft insbesondere `version.json` bzw. `version.txt`. Eine Datei `version.password` wird vom aktuellen PHP-Endpoint nicht ausgewertet – sollte später einmal eine Authentifizierung nötig sein, müsste sie separat in der API implementiert werden.

Fehlen beide Dateien, liefert der Endpunkt automatisch einen Platzhalter (`0.0.0`), sodass der Client zumindest mit "up to date" antwortet.

### Beispiel-Aufrufe
```bash
# Match speichern
curl -X POST https://example.com/ladder/index.php/matches \
     -H "Content-Type: application/json" \
     -d @match.json

# Letzte Matches anzeigen
curl https://example.com/ladder/index.php/matches?limit=10

# Einzelnes Match abrufen
curl https://example.com/ladder/index.php/matches/srv-20240405-183011-42

# Match löschen
curl -X DELETE https://example.com/ladder/index.php/matches/srv-20240405-183011-42
```

## Datenablage
Jedes Match wird als einzelne JSON-Datei unter `data/<matchId>.json` abgelegt. So lässt sich der Ordner bei Bedarf sichern oder in andere Systeme importieren. Der Service fügt automatisch einen Zeitstempel `receivedAt` hinzu, um Listen sortieren zu können.

## Backup & Wartung
* Regelmäßig den Ordner `data/` sichern.
* Bei sehr vielen Matches kann die Dateibasis unübersichtlich werden; für große Installationen empfiehlt sich langfristig dennoch eine vollwertige Datenbank.

## Fehlerbehandlung
Fehlerhafte Anfragen werden als JSON im Format `{ "error": "..." }` beantwortet. Stimmt etwas mit den Dateirechten nicht, liefert der Service HTTP-Status 500.

