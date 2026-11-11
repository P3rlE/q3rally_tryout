# Embedded web garage pilot

The Windows x64 pilot embeds a CEF child window in the SDL game window.
The bundled HTML, CSS, JavaScript and vehicle images live in
`engine/webui/garage`. CEF serves the assets directly from the game directory
under the reserved origin `https://q3rally.invalid/`. No network server or
website connection is required.

## Build and run

Use the matching Windows x64 CEF SDK and runtime. The pilot was built against
CEF `150.0.23+g4ccc4ca+chromium-150.0.7871.287`. Add the following arguments to
the usual Windows x64 engine build. Keep this build separate from a regular
build so the two clients do not share compiled objects.

```sh
make release PLATFORM=mingw32 ARCH=x86_64 BUILD_DIR=build-cef \
  USE_CEF_GARAGE=1 CEF_DIR=/path/to/unpacked/cef-sdk
```

Put the CEF Release DLLs, snapshot and Vulkan metadata next to the game
executable. Copy the contents of the CEF Resources directory, including
`icudtl.dat`, the PAK files and the `locales` directory, beside the executable
as well. Include the CEF license. Copy `engine/webui/garage` to
`webui/garage` beside the executable, and copy the three TTF files and their
OFL notices from `launcher/assets/fonts` into `webui/garage/fonts`.

The showroom exporter, vehicle display labels and viewer now live in this
repository. No website checkout is required. Install the pinned dependencies
with `npm ci --prefix engine/webui` and
`python -m pip install -r engine/webui/requirements.txt`. Export into a fresh
directory; the game PK3 is the source of geometry, skins and preview images.

```sh
python engine/webui/build_showroom.py /path/to/game/webui/garage /path/to/game/baseq3r/pak0.pk3
```

Start Q3Rally in windowed mode and run `webgarage` from the console while
disconnected. The startup arguments `+set r_fullscreen 0 +webgarage` open the
pilot directly. Pick any vehicle and preview paint and wheels. In 3D view,
drag to rotate and scroll to zoom. Locked cars and paints display their
required profile rank and remain available for preview. Apply setup changes
the active model and wheels after the engine checks the profile rank, the
bundled catalog, and both game skins. Install the matching UI QVM alongside
the engine so the web garage can read the active profile's rank.
Back to game menu returns to the native menu.

## Timing comparison

While the CEF child is visible, the engine skips drawing the covered native
menu. To compare with the previous behavior, add these startup arguments:

```text
+set logfile 2 +set cl_webGarageTiming 1 +set cl_webGarageRenderBehind 1
```

Repeat with `cl_webGarageRenderBehind 0`. When the garage closes, the game
console and `baseq3r/qconsole.log` report the number of skipped screen updates,
engine frame intervals and CEF message-pump time. These numbers measure the
game loop, not browser compositor frame rate.

## Pilot boundaries

The build flag is opt-in. The child window tests HTML rendering and native
interaction in windowed and fullscreen modes. Renderer restarts recreate the
browser under the replacement SDL window. The local bridge validates setup choices against the bundled catalog and
profile unlocks before applying them. The CEF sandbox is disabled for this MinGW pilot. It is intended
for local testing with bundled documents only.

## Main menu, Rivals and Downloads

With `cl_webUI 1`, the embedded frontend opens for graphics loading, the main
menu, credits, the demo archive, Rivals and Downloads. Other menus use their
native UI. On return to a native menu, the client restores SDL input focus,
clears held keys and routes mouse positions through the UI's absolute position
entry point.

Rivals reads the existing bot descriptions and vehicles from the native menu
and previews their vehicles with the local showroom assets. Downloads uses the
existing native catalog and transfer service. Search and category filtering
happen locally. Refresh, install and cancel commands use validated native item
indices. Preview images are served from the download manager's local preview
cache. Completed installations retain the restart requirement.

The focused mouse regression also runs with the CEF build flag enabled to cover
native menu input after the browser closes.

## Complete setups and favorites

The embedded garage includes vehicle, paint, wheels, driver and license plate.
Legacy vehicle settings entry points open this same garage in a CEF build.
Four saved setups use the active local profile's existing favorite slots.
Older four-field favorites remain readable and use the currently selected
plate when no plate was saved.

Save here persists the preview in the local driver profile. Upload to web
profile sends a saved slot through the linked device token. Load web setup and
favorites imports the central profile's four slots and previews its vehicle
setup. Apply setup commits a preview to the game after the normal asset and
unlock checks.

The website changes are in the website checkout with schema-v5.sql. Apply the
schema migration and deploy the PHP endpoint, data and Astro page together.


### Driver profile

The embedded main menu now has one Driver profile entry, containing Overview,
Stats and Achievements tabs. Profile snapshots are read locally; opening this
screen does not submit or change profile statistics. Rank thresholds and earned
achievement flags come from the existing QVM rules. The asset catalog includes
preset avatars and achievement medals. Unsupported custom avatar paths remain
text-only. Edit driver and Select / create profile use embedded web dialogs backed by
the existing QVM profile writer and activation path. Edits preserve UUID, stats
and favorites; a profile UUID check prevents applying edits to a changed active
profile. The editor supports preset avatars, gender, birth date and country.
Custom avatars already stored in the profile are preserved. Profile creation
rejects existing names; selection checks that the chosen file is readable. Web profile linking remains available through the Garage action.

Validation: native CEF/QVM build, existing profile/favorites/menu/download/mouse
regressions, and browser smoke checks for tab navigation, rank progress, game-mode
statistics, native earned flags, no active profile, garage handoff and 390px layout.


### Online browser and hosting

The web frontend now exposes the native server browser, sources, gametype and
occupancy filters, sorting, local search, live discovery, favorite persistence,
and direct connections. Actions identify servers by address rather than a row
index, so reordered or vanished rows cannot redirect a connection/favorite action.
Server responses are rendered as text. Direct addresses are bounded and validated
before use by the native command path. Removing a favorite has a web confirmation.

Discovery receives the engine realtime explicitly: CEF suppresses native UI drawing,
which otherwise also leaves the legacy browser clock frozen. Snapshots are published
only when the native state changes; selected addresses survive ping/sort updates.
Map names now retain MAX_QPATH characters instead of the legacy 11-character limit.

Host a server reuses the web track/rules/grid screens with the native multiplayer
configuration, including hostname and dedicated mode. The connection/map loading
screen remains native pending its migration.

Validation: native CEF/QVM build; server-address guards and selection across reordered
rows; favorite actions and refresh clock; offline/profile/favorites/menu/download/mouse
regressions; browser smoke checks including mobile, direct dialog, favorites, scanning,
and hosting. An isolated loopback dedicated server was discovered as a favorite,
selected with a map preview, and joined successfully from the CEF server browser.


### Configuration

The embedded Config hub and its Controls, Graphics (including Effects, Lighting
and driver information), Display, Sound, Network, Game options and Q3R options
screens expose the existing native widgets and callbacks. Driver opens the web
Driver Profile. Native disabled rows, mode dependencies, presets, staged renderer
settings and sound Apply/restart behavior remain authoritative. Changing
fullscreen and applying recreates the embedded WebUI after the renderer restart.

Control bindings use a capture dialog, named-key conversion in the client, the
native conflict check and native save/discard/cancel callbacks. Web binding edits
stage changes without clearing live key bindings before Save. Search uses the
native control search. Confirmation menus remain native state with web rendering.

Validation: CEF/QVM build, config callback/value bounds and binding-staging tests,
existing profile/favorites/offline/online/menu/download/mouse regression suite,
and browser checks for categories, choices, sliders, binding capture cancellation,
confirmation actions, disabled rows and mobile layout.

Manual CEF check: opened the config hub, graphics and controls; captured a key
conflict, rejected it, then tested replacing a binding and discarding changes.
Reopening Controls showed the original W binding. Changing a sound slider exposed
the native Apply action. The isolated test window was closed normally afterward.


### In-game menus and console

The web Config adapter now renders Pause, Add/remove bots, team selection,
team orders and server information, retaining their native callbacks. Resume
returns through the native menu stack. Online matches continue while the menu
is open. Native save/discard confirmation remains available from in-game Config.

Shift+H exposes a web sidebar over the running game. Native HUD entries supply
choices and mode availability; the cgame validates each write and minimap range.
The browser child window is clipped to the sidebar, leaving the game visible.
Escape, Shift+H and the close button close this panel. Input capture is limited
to explicitly open menus; the console takes priority over the browser.

The native console uses a dark backdrop, orange accents and a bundled fixed-cell
Share Tech Mono atlas. History, completion and input remain native. The atlas
can be regenerated with engine/webui/build_console_font.py; its OFL license is
included alongside the generated assets in baseq3r/fonts.

Validation: Windows CEF engine and QVM build; 20 regression tests plus 11 subtests;
headless browser checks for Pause, HUD/sidebar, disabled choices, mobile sizing
and return to the main menu. Live in-game mouse/console QA remains to be done.
The existing Windows x64 CEF pilot limitation remains in place.


### Map loading

Screen 14 covers connecting, challenge, game state, map resources and priming.
It reads client connection/configstring state directly, without reentering the
cgame VM during CG_INIT. CG_LoadingString publishes its current resource label.
SCR_UpdateScreen pumps the browser during synchronous loading so the main-frame
stall does not freeze the loading page. The native connection presentation stays
available when the web pilot is disabled or unavailable.

The local catalog supplies track images with a text fallback for unknown maps.
Transfers show actual bytes and percent where the size is known; map resources
use indeterminate progress instead of an invented percentage. Cancel queues the
native disconnect command; synchronous resource loading must return before the
command buffer can process it. The console retains priority.

Validation: Windows CEF/QVM build, 20 tests and 11 subtests, browser checks for
map preview, unknown tracks, local/server labels, determinate/indeterminate
progress, mobile width and exit back to the main menu. Live map-load QA remains
for the test build; the Windows-only pilot restrictions still apply.


## Reproducible Windows x64 Actions test package

### Driver setup and offline registration

The first-run driver wizard and the standalone offline Ladder registration use
the shared web form renderer (Config pages 17-20 and 16 respectively). Their
native menus retain profile creation, validation, protected key handling,
registration callbacks, retry, cancellation and per-profile dismissal behavior.
This is offline result registration, not website account creation or linking.

Only visible, enabled fields accept text updates, bounded by their native field
limits. Profile Back uses the wizard's native Back action; the mandatory name
step cannot be dismissed. A native revision derived from step/status/flags
refreshes asynchronous results without polling and rebuilding the HTML form
while the user is typing. Registration actions sit below the fields.

The browser tests cover the setup steps, country selection, pending/retry and
completion UI. Native ZIP validation also starts with a separate empty home and
requires the first-run wizard to be published, including across fullscreen
renderer restarts. No live registration is submitted by these tests.

`.github/workflows/webui-windows.yml` builds the opt-in test package without
publishing releases or changing the regular release workflow. It runs manually,
on WebUI integration branches, and on relevant pull requests.

The Linux `common` job builds QVMs once using the source commit timestamp and
UTC, audits tracked game assets, creates the canonical pak0.pk3/qvms.pk3 and
exports the whole local showroom. Exact npm versions and integrities are in
engine/webui/package-lock.json; Python image export uses requirements.txt.
Vehicle labels and the MD3 exporter were brought over from the Q3Rally website
and are maintained here, so future website changes cannot silently alter a game
build. All runtime HTML/scripts, fonts and license notices are staged locally.

The Windows job verifies/downloads the CEF SDK pinned by URL, size and SHA-256
in engine/webui/cef.lock.json. It builds into a separate `build-webui` directory
with static Schannel curl. The client linker uses a response file for its object
list to avoid Windows command-line limits with the longer build directory.
Only runtime DLLs, snapshots, resources/locales and licenses are copied from the
SDK; import libraries and bootstrap/example executables are excluded.

The native package consumes the common PK3s byte-for-byte; it cannot regenerate
them per platform. Both common and final manifests record source provenance,
file sizes and SHA-256 digests. This structure can be reused by later platform
jobs; it does not yet change v0.8 release packaging or fix already-published
platform-specific packages.

Checks cover archive traversal/tampering, canonical QVM preservation, missing
CEF resources, browser rendering and menu transitions. The Windows validation
extracts the actual ZIP into a clean directory, checks every digest, strips
build tools from PATH, checks --version and starts the CEF frontend with an
isolated profile and no game networking. A bundled-page load marker and bounded
native config script are required, and results/logs are uploaded with the ZIP.
The script switches windowed to fullscreen and back with vid_restart and requires
a new browser attachment in the actual SDL mode plus HTTP 200 in each stage.
Browser smoke tests run the bundled files with software WebGL and save images.
These startup checks do not replace interactive fullscreen/input/audio tests.

Download the `Q3Rally-WebUI-windows-x86_64-<commit>` artifact from a successful
run and extract its inner ZIP completely. Run `Start-WebUI.cmd`; no Python,
Node, Chrome or website checkout is needed. `Start-Native.cmd` selects native
menus in the same test build. Both use an isolated LOCALAPPDATA/Q3Rally-WebUI
home per source revision and disable server Ladder reporting by default.
CEF remains the existing sandbox-disabled Windows-only pilot; this workflow
is an integration milestone, not production security or cross-platform signoff.
