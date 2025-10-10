# Update-Prüfung in Q3Rally

Diese Notiz beschreibt, wie die im Client integrierte Update-Prüfung funktioniert.

## Auslöser des Checks

Sobald die UI initialisiert wird (`CL_InitUI`), wird `CL_UpdateRequestLatest` aufgerufen. Dadurch startet die eigentliche Update-Prüfung immer beim Übergang ins Hauptmenü bzw. wenn die UI neu geladen wird.【F:engine/code/client/cl_ui.c†L2145-L2189】

## HTTP-Anfrage

Die Funktion `CL_UpdateRequestLatest` sorgt dafür, dass die benötigten Cvars existieren und prüft zunächst, ob `cl_updateCheck` aktiviert ist. Anschließend initialisiert sie (falls notwendig) die cURL-Handles, setzt das Ziel (`cl_updateEndpoint`) sowie weitere Optionen und reiht den Request in den Multi-Handle ein. Gleichzeitig wird der Status auf `checking` gesetzt.【F:engine/code/client/cl_ui.c†L480-L503】【F:engine/code/client/cl_ui.c†L956-L1032】

Standardmäßig verweist `cl_updateEndpoint` auf `https://ladder.q3rally.com/api/v1/version`. Der Wert lässt sich aber per Cvar oder über die `q3config.cfg` überschreiben, falls ein eigener Server genutzt werden soll.【F:engine/code/client/cl_ui.c†L493-L503】

### Erwartete Serverantwort

Der Client akzeptiert zwei Formate:

* **JSON** – bevorzugt. Der Service kann die Felder `latest`, `latestVersion` oder `version` für die Versionsnummer setzen. Optional lassen sich `downloadUrl` bzw. `url` sowie `message` oder `notes` übermitteln.【F:engine/code/client/cl_ui.c†L680-L714】
* **Plaintext-Fallback** – Zeile 1 enthält die Versionsnummer, Zeile 2 (optional) einen Download-Link, alle folgenden Zeilen bilden die Nachricht, wobei Zeilenumbrüche automatisch zu Leerzeichen normalisiert werden.【F:engine/code/client/cl_ui.c†L716-L756】

Damit der Check funktioniert, muss der Server einen per HTTPS erreichbaren GET-Endpunkt bereitstellen, der eines der oben genannten Formate liefert. Ein Minimalbeispiel für eine JSON-Antwort könnte folgendermaßen aussehen:

```json
{
  "latest": "v0.7",
  "downloadUrl": "https://downloads.example.com/q3rally-v0.7.zip",
  "message": "Bugfix-Release mit verbesserten Streckenzeiten."
}
```

Statische Hosting-Varianten (z. B. eine kleine JSON-Datei auf einem CDN) sind ausreichend – serverseitige Logik ist nicht erforderlich, solange die Datei aktualisiert wird, sobald eine neue Version veröffentlicht ist. Wer zusätzliche Metadaten bereitstellen möchte, kann weitere Felder hinzufügen; sie werden vom Client ignoriert.【F:engine/code/client/cl_ui.c†L653-L790】

## Verarbeitung der Antwort

In jedem Frame ruft `CL_Frame` die Pump-Funktion `CL_UpdatePumpRequest` auf. Diese kümmert sich darum, den cURL-Transfer voranzutreiben und reagiert auf Fehler (z. B. fehlende Verbindung oder HTTP-Status ungleich 200). Sobald der Download abgeschlossen ist, wird `CL_UpdateHandleResponse` ausgeführt.【F:engine/code/client/cl_main.c†L2974-L2995】【F:engine/code/client/cl_ui.c†L894-L952】

`CL_UpdateHandleResponse` wertet den Inhalt aus: `CL_UpdateParseResponse` extrahiert Version, Download-Link und optional eine Nachricht entweder aus JSON (Felder `latest`, `url`, `message`) oder aus einem Fallback-Textformat. Danach vergleicht `CL_UpdateCompareVersions` die Remote-Version mit der lokalen `PRODUCT_VERSION`. Ist die entfernte Version neuer, wird der Status `outdated` gesetzt, ansonsten `up_to_date`. Fehlende oder ungültige Antworten führen zu `error` mitsamt Meldung.【F:engine/code/client/cl_ui.c†L653-L790】

Die aktuelle lokale Versionsnummer ist in `q_shared.h` als `PRODUCT_VERSION` definiert; sie wird gegen den vom Server gelieferten Wert geprüft.【F:engine/code/qcommon/q_shared.h†L57-L74】

## Darstellung im UI

Das UI fragt regelmäßig die Status- und Detail-Cvars (`cl_updateStatus`, `cl_updateLatest`, `cl_updateUrl`, `cl_updateMessage`) ab. Sobald der Status `outdated` lautet, baut `UI_MaybeShowUpdateDialog` ein Popup mit Version, Nachricht und Download-Link und zeigt es im Hauptmenü an. Das Popup erscheint nur einmal pro Sitzungsstart, bis der Status wieder einen anderen Wert annimmt.【F:engine/code/q3_ui/ui_menu.c†L175-L217】【F:engine/code/q3_ui/ui_menu.c†L500-L527】

## Zusammenfassung des Entscheidungsweges

1. UI-Initialisierung triggert `CL_UpdateRequestLatest`.
2. cURL lädt die Versionsinformationen vom konfigurierten Endpoint.
3. `CL_UpdateHandleResponse` analysiert die Antwort und vergleicht sie mit `PRODUCT_VERSION`.
4. Bei abweichender (neuere) Version setzt der Client den Status `outdated` und das Hauptmenü zeigt ein Popup an.
