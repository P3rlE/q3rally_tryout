# Native screens in the WebUI look

The WebUI (`engine/webui/garage`, shown through CEF on Windows) set the look of
the menus. Screens move back into the game one at a time and must look exactly
like their WebUI version, on every platform and without a browser. Done so
far: the graphics loading screen ("Getting ready.", WebUI screen 1), the
main menu ("Ready to race.", WebUI screen 2), the end credits ("Thank you
for playing.", WebUI screen 3), Rivals ("Meet your rivals.", WebUI
screen 5), Downloads ("Expand your rally.", WebUI screen 6) and the demo
archive ("Your replays.", WebUI screen 4) and the map loading screen
("Ready to race.", WebUI screen 14).

## How a screen is rebuilt

- **Layout from the CSS.** `q3_ui/ui_rally_gfxlayout.c` (loading screen),
  `q3_ui/ui_rally_mainlayout.c` (main menu),
  `q3_ui/ui_rally_creditslayout.c` (end credits) and
  `q3_ui/ui_rally_rivalslayout.c` (Rivals) compute every box of the screen
  from the rules in `garage.css`, number for number. What both share, the
  page header, the two-column grid and the showroom with the car, lives in
  `q3_ui/ui_rally_frontpage.c`. This covers
  `clamp()` with `vw`/`vh`, `max-width`, the grid columns, margins and their
  collapsing, `min-height` and line wrapping. It works in screen pixels, which
  are CSS pixels at a device pixel ratio of 1. Chrome reserves 15 px for the
  scrollbar (`scrollbar-gutter: stable`), so the page is not centered on the
  screen. Chrome keeps block lengths in 1/64 px, cut off rather than
  rounded; the main menu's stacked buttons follow that, or nine of them
  would drift by a quarter pixel.
- **Text like Chrome.** `q3_ui/ui_rally_webstyle.c` lays text out with the
  font's linear advances, its standard ligatures (fi, fl, ffi, ffl, ij, IJ,
  gj, `--`; Chrome leaves them out when letter-spacing is set), its GPOS pair
  kerning, `letter-spacing` after every
  glyph and `text-transform`. Line boxes round ascent and descent to whole
  pixels and put the larger half of the leading below the text.
- **Things Chrome synthesizes:**
  - bold for weight 750 on a regular face (outline emboldened by em/24, as Skia does);
  - italic for `<em>` (skew 1/4);
  - tabular figures for list markers (`::marker { font-variant-numeric: tabular-nums }`).
- **Masks.** `mask-image` with a vertical gradient (the credits window fades
  its text in and out over 12% at each end) is `WS_SetFade`: glyphs in a
  fade zone are drawn in one pixel rows, each with the mask's alpha.
- **Backgrounds.**
  - The body stripes are a 1D texture. The shader `tcMod transform`
    (`scripts/q3r_webstyle.shader`) projects the pixel position onto the
    132° gradient line. Like Chrome, the stripes tile at the size of the html
    box, so they repeat below the page content.
  - The showroom glow is a box-relative texture.
  - Chrome's dark `<progress>` is drawn from its own pixels, captured at
    1920×1080.

## Assets

| What | Built by | Where |
| --- | --- | --- |
| Font atlases and metrics | `tools/ui_webstyle/build_ui_fonts.py` (FreeType via `freetype-py`, `fontTools`) | `baseq3r/menu/fonts/*.dat`, `*_<size>.png` |
| Stripes, glow, progress art | `tools/ui_webstyle/build_ui_art.py` | `baseq3r/menu/ui/web_*.png` |

**Font sizes.** Each face comes in a few fixed sizes. The UI picks the next
larger size and scales it down by at most about 1.2. Barlow Condensed carries
only uppercase, because every use of it is uppercased.

**Metrics file.** Each `.dat` holds:
- advances and kerning in font units;
- the glyph rectangles for every size.

The format is documented in the builder.

**Licenses.** The OFL license texts of the three fonts are copied next to the
atlases.

## Checking a screen

- `tools/ui_webstyle/measure_webui_loading.cjs`, `measure_webui_main.cjs`,
  `measure_webui_credits.cjs`, `measure_webui_rivals.cjs`,
  `measure_webui_downloads.cjs`, `measure_webui_demos.cjs`,
  `measure_webui_maploading.cjs`, `measure_webui_offline.cjs`,
  `measure_webui_offline_rules.cjs`, `measure_webui_online.cjs` and
  `measure_webui_driver.cjs` measure the WebUI screens in Chrome (the
  credits with the roll paused at fixed times). They record every box and the width
  of every text run, for several viewports and all states (main menu: with
  and without a profile, quit dialog open).
- The results are `tests/fixtures/webui_loading_layout.json`,
  `webui_main_layout.json` and `webui_credits_layout.json`.
- `tests/test_native_gfxloading.py`, `test_native_mainmenu.py` and
  `test_native_credits.py` run the native layout and text measurement
  against them:
  - boxes must match within 0.5 px;
  - text runs within 0.1 px;
  - line breaks must match exactly.
- `tools/ui_webstyle/native_screenshot.sh` takes a screenshot of the real
  client under Xvfb, with Mesa's software renderer. A cvar pins the state, so
  the screenshot can be laid next to the WebUI screenshot of the same state:
  `ui_gfxLoadingPreview "stage,percent,prompt"` for the loading screen,
  `ui_mainMenuPreview "name,rank,points,quit"` for the main menu (it also
  skips the profile wizards), `ui_creditsPreview <seconds>` for the end
  credits, paused that far into the roll, `ui_rivalsPreview
  "search,scroll,rival"` for Rivals, `ui_downloadsPreview
  "state,progress,type,search,item"` for Downloads (catalog from
  `ui_dl_indexpath`), `ui_demosPreview "selected,scroll"` for the demo
  archive.

What still differs from the WebUI:
- **Glyph edges.** Chrome positions glyphs on subpixels; the game draws them
  on whole pixels.
- **The car's materials.** The WebUI shows a three.js export with plain
  textures. The game renders the real car with its shaders: see-through glass,
  lit headlights. Camera, field of view, angle and framing are the same
  (`UI_DrawVehicleWebView`).
- **The cursor.** The game draws its own cursor.

## The end credits

- The lines are the table in `q3_ui/ui_credits.c`, as the client published
  them to the page: titles as h2, accent-colored lines as labels.
- They roll up through the window at 38 px/s, from just below it until the
  last line has left it; then the game quits, as on the page.
- Any key quits at once; the mouse only through the "Exit game" button.

## Rivals

- The list, the search and the profile follow library.js in its rivals mode:
  the search matches name, model, description and personality; the selected
  rival stays while it still matches, else the first match is selected.
- The list scrolls with the wheel (100 px a notch), the scrollbar's arrows
  and track, and by dragging its thumb. Chrome's dark scrollbar, as CEF
  shows it on Windows, is captured in `menu/ui/web_scrollbar.png`.
- The search field takes the keyboard after a click; Escape goes back to the
  main menu, the arrow keys move the selection.
- The rival's car can be turned and zoomed like the main menu's, starting
  at 2.1 radii as on the page.
- The page shows no weapon icons: its icon table does not know the weapon
  names of 2026, so the native screen leaves them out too.

## Downloads

- The WebUI shows Downloads on the same page as Rivals (library.js mode 6);
  `ui_rally_rivalslayout.c` lays out both, `ui_rally_library.c` holds what
  they share: search, list, scrollbar, disabled buttons.
- The type filter, the search (name, author, type) and the selection by id
  follow library.js. Install, Refresh and Cancel act as the page's buttons
  did (`UI_WebCatalogAction`), disabled at opacity .45 like in Chrome.
- The <select>'s option list is the system's popup in CEF; the native one
  is a plain list below the box.
- Previews are drawn 4:3 (levelshots); the UI cannot read an image's size.

## Demo archive

- `q3_ui/ui_rally_demoslayout.c` lays it out: the listbox (`<select size>`,
  options of 31.8px, the checked one #713110), the details in .9rem that
  break even inside a name (`overflow-wrap: anywhere`), the viewport and the
  transport.
- A recording plays embedded (`demopreview`) as before. The UI names the
  viewport in `ui_demoRect` ("x y w h", screen pixels); the cgame renders
  the demo there (`cg_view.c`), as it did for the WebUI's `cl_webDemoRect`.
- Like the page: a double click or Play starts, Space pauses, the left and
  right arrows seek 10 s, Escape stops and goes back. The transport is
  disabled (opacity .45) while nothing plays or the demo seeks.
- `WS_SetClipX` cuts text at the sides, for the options in the listbox.

## Map loading

- `UI_DrawConnectScreen` draws it while connecting, loading and downloading
  (`q3_ui/ui_connect.c`, layout in `ui_rally_maploadlayout.c`). The page is
  written in px, its sections are #161616 with a 2px #ff5a00 top.
- It shows what `Web_PublishMapLoading` sent the page: the levelshot (cover,
  16:9), the track's title, the server ("Local race" for localhost), the
  state, a download with its progress, the connection attempts.
- Without a download the bar runs like Chrome's indeterminate <progress>.
- Escape cancels, as before (`UI_KeyConnect`); the Cancel button is drawn as
  on the page, there is no cursor during loading.
- The client no longer hands this screen to the browser
  (`Web_CurrentScreen`, `CL_WebGarageOwnsScreen`).

## Offline, step 1 (track and mode)

- `q3_ui/ui_rally_offline.c` draws it over the old start server menu
  (`Offline_InstallTrack`), layout in `ui_rally_offlinelayout.c`. It reads
  the same model the page read (`UI_WebOfflineNumber`, `UI_WebOfflineChar`)
  and acts through `UI_WebOfflineAction`: back, mode, track, next.
- From 64rem x 44rem the page fills the viewport and only the track list
  scrolls; the image takes what the details leave. Below that the page
  flows and scrolls as a whole, with a scrollbar in the gutter.
- Like the page: a new mode clears the search, the search matches title and
  id, the arrows and the D-pad pick the previous or next track, Enter goes
  to "Race setup", Escape goes back.
- Levelshots are drawn with `object-fit: cover`; their size comes from the
  file's header (JPEG, TGA, PNG), most are 16:9, a few square.
- `ui_offlineDump 1` prints the model as JSON (tests/fixtures/offline_model.json),
  `ui_offlinePreview "mode,search,track,scroll[,pagescroll]"` opens the
  screen in that state (native_screenshot.sh `offline:`).

## Offline, step 2 (rules and drivers)

- `q3_ui/ui_rally_offlinerules.c` draws it over the old server options menu
  (`Offline_InstallRules`), layout in `ui_rally_offlinerulelayout.c`. Like
  `options()` in offline.js: the summary with "Start race", the rules (the
  controls without a slot) and a row per slot (type, the bot's car and
  select, the team).
- The rows are laid out as the page's flex rows: labels `flex: 1` with a
  min-width, the bot select `flex: 2` from its padding, lines wrap when the
  minimums do not fit. A select is as tall as Chrome's menulist: the font's
  ascent and descent, each rounded, a pixel above and below.
- Selects open a plain option list (up to 20 rows, the wheel scrolls it);
  numbers are typed into the field and set on Enter or a click elsewhere,
  as the page's change event. Bot cars are the vehicles' `icon_<paint>`,
  the same pictures as the page's previews.
- Chrome breaks lines after a hyphen between digits or letters
  ("4000-5000"); `WS_Wrap` does too.
- `ui_offlinePreview "mode,search,track,gridscroll,pagescroll,2"` opens step 2
  (native_screenshot.sh `offline:`); `ui_offlineDump 1` then prints its
  model (tests/fixtures/offline_rules_model.json).

## The server browser (Online)

- `q3_ui/ui_rally_online.c` draws it over the old arena servers menu
  (`Online_Install`), layout in `ui_rally_onlinelayout.c`. It reads the model
  the page read (`UI_WebOnlineNumber`, `UI_WebOnlineChar`) and acts through
  `UI_WebOnlineAction`; the address actions (select, connect, favorite,
  remove, direct) pass the address in `ui_webonline_address`, as before.
- Like online.js: the search matches name, map, address and mode; "Count
  humans only" shows humans in the players column; a server without humans
  shows an empty "Human players" (the page writes nothing for a 0);
  "Remove favorite" only with the Favorites source, behind its dialog.
- The page flows and scrolls as a whole; the list scrolls from 38rem. The
  dialogs are centred in the viewport (without the scrollbar gutter) over
  a #000b backdrop.
- Checkboxes are Chrome's in the dark scheme with the accent colour, drawn
  pixel for pixel from a screenshot (no texture). Chrome starts a select's
  text 4px inside its padding; all native selects do that now.
- `ui_onlineDump 1` prints the model (tests/fixtures/online_model.json),
  `ui_onlinePreview "row,search,listscroll,pagescroll,dialog"` opens the
  screen in that state (native_screenshot.sh `online:`, with three local
  dedicated servers as favorites).

## The driver page

- `q3_ui/ui_rally_driver.c` draws it over `s_webDriverMenu`
  (`Driver_Install`), layout in `ui_rally_driverlayout.c`: the tabs, the
  overview, the stats, the achievements and the two dialogs (edit driver,
  select / create profile). It reads what the page was fed (the active
  profile, `UI_WebDriverValue`, `UI_WebAchievementChar`) and changes
  profiles through `UI_WebDriverAction` 4-6 with `ui_webdriver_payload`,
  as the page did; the status line shows `ui_webdriver_message`.
- The achievement cards float the medal right: their lines beside it are
  4rem shorter. A locked medal is the category's `_locked` picture at .45
  (the page greys the tier's picture with a filter).
- The category filter is Chrome's own select in the system font: its width
  follows the widest option (143.781px for "All achievements"); the text is
  drawn in Barlow.
- The date field takes digits (mm, dd, yyyy) like Chrome's; the calendar
  picker is not there. Values of 0 ms show the em dash, drawn as a bar (the
  fonts are Latin-1).
- The condensed face now carries its lowercase (the cards' h3) and the bold
  face 18.72 and 25.6px: the condensed data grew to 82 KB, the menu font
  loader reads up to 128 KB.
- `ui_driverDump 1` prints the model (tests/fixtures/driver_model.json),
  `ui_driverPreview "tab,filter,pagescroll,dialog"` opens the page in that
  state (native_screenshot.sh `driver:` with tests/fixtures/profiles, or
  `nodriver:`).

## The settings and race control

- `q3_ui/ui_rally_config.c` draws every config page (setup, controls,
  graphics, display, sound, network, game and Q3Rally options, renderer
  information, confirmations, pause, bots, team orders, server information,
  and the wizards: offline Ladder registration 16, first start 17-20, demo
  save 21)
  in place of the old menus' drawing and keys (`Config_Native` in
  ui_atoms.c), layout in `ui_rally_configlayout.c`. Like config.js it reads
  `UI_WebConfigNumber` / `UI_WebConfigChar` and acts through
  `UI_WebConfigAction`; the old menus stay underneath and keep their
  callbacks.
- The wizard pages are config.js's `.registration-screen`: 70rem wide, a
  smaller h1, the description at line-height 1.7 and 58rem, the settings
  with 2rem padding and a 2px accent top, the buttons below them (Next, Join
  ladder, Register, Retry, OK, Done, Start racing and Save in the accent).
  Back is hidden on the name and ready steps. The avatar select shows its
  preview (the avatar's own shader at 6rem, or "No avatar selected").
  Typing in a field sends the text with each key (`UI_WebConfigAction` 4).
- One renderer for every page, as on the page: buttons in a flex row
  (without Back, and without "Resume race" on the pause page, which is the
  header button there), the other items in the auto-fit grid: selects,
  sliders (range input with its value), the control search, key bindings
  (rebind and Clear), the text list (team orders, a pre).
- A confirmation page (`UI_WebConfigNumber( 8 )`) is the confirm dialog over
  the page. Rebind opens the key capture dialog: the next key, mouse
  button, wheel or pad input binds (`UI_WebConfigAction` 3), Escape and
  Cancel cancel, the console key is ignored.
- Chrome draws the range input without the page's border and background: an
  8px track with a grey border, accent up to a 16px disc. A disabled select
  looks like an enabled one (the page sets its colours). The pre has no
  font of its own on the page (the browser's monospace); it is drawn in
  Barlow.
- `ui_configDump 1` prints each page (tests/fixtures/config_model.json),
  `ui_configPreview "2,3"` opens the setup page and presses those items,
  (`ui_configPreview demosave` the demo save page),
  `ui_configPress` ("item", or "action:item:value") / `ui_configCapture`
  act on the current page or open the key capture (native_screenshot.sh
  `config:` and `race:`; without a profile the first-start wizard opens).

## The HUD options (cgame)

- Shift+H in a race opens `cgame/cg_hud_panel.c`, the WebUI's `#hud-screen`
  (session.js): the panel fixed at the right (clamp(21rem, 42vw, 36rem),
  inside the page's 15px scrollbar gutter, #111 with a 3px accent at its
  left), scrolling on its own. Layout in `cgame/cg_hud_panellayout.c`,
  tested against Chrome (tests/test_native_hud_options.py).
- The cgame builds `q3_ui/ui_rally_webstyle.c` in as well (`CGAME` picks
  the cgame's glconfig and white shader); the text styles, buttons,
  selects, range inputs and the scrollbar now live there for both.
- The toggles come from the table in cg_hud_core.c through
  `CG_WebHUDNumber` / `CG_WebHUDChar` / `CG_WebHUDAction`, as for CEF. The
  client no longer reports screen 13, so CEF stays hidden in the race.
- The cgame's cursor reaches the sides of a wide screen like the UI's
  (-bias .. 640 + bias). Escape (the client clears the cgame key catcher)
  and Shift+H close the panel.

## The garage

- `q3_ui/ui_rally_garage.c` is the WebUI's `#garage` (garage.js), opened by
  `UI_GarageMenu` (the main menu's Garage, the driver page): the showroom
  with the paint's picture (object-fit: contain, a drop shadow) or the 3D
  view (`UI_DrawVehicleWebView`, drag to rotate, wheel to zoom, Reset view),
  "Make it yours" with five carousels, Apply setup, the web profile buttons,
  the status lines and the four favorite slots. Layout in
  `ui_rally_garagelayout.c`, tested against Chrome
  (tests/test_native_garage.py, ten viewports, three setups).
- The catalog is found the way engine/webui/build_showroom.py builds
  catalog.json (`ui_rally_garagedata.c`): vehicles with a body.md3, their
  paints and the wheels and drivers by `<key>.skin` beside an `icon_<key>`,
  the plates by their pictures; names from game/garage_unlocks.h,
  `ui_rally_garage_labels.h` (catalog_labels.json's names that are not the
  file name in title case) and the title case. The pictures are the game's
  icons, the ones the WebUI's previews were made from.
- What the client did for the page the UI does itself: Apply sets model, rim,
  head and plate after the same checks, Save here goes through
  `UI_WebSaveFavorite`, the web profile through `ui_webprofile_action`
  (link, sync, favorite), and `ui_webprofile_setup` / `ui_webprofile_status`
  come back into the page.
- The carousel arrows and the ellipsis are characters past Latin-1: the body
  faces carry "‹ › …" at 0x9C-0x9E (build_ui_fonts.py), barlow_bold also at
  32px for the arrows.
- `ui_configPreview garage` opens the garage from the main menu for
  screenshots.

## The main menu

- The buttons behave like the page's: a click activates, Tab and the arrow
  keys move a white focus outline (`:focus-visible`), Enter presses the
  focused button. There are no hover styles, as on the page. Escape does
  nothing, this is the top menu.
- Quit opens the page's confirm dialog over its backdrop; "Keep racing" has
  the focus, Escape closes it, "Show credits and exit" runs the credits.
- The car can be turned and zoomed like the WebUI's OrbitControls: a drag
  turns by 360° per height of the view, the elevation stays between −10° and
  67.5°, the wheel zooms by 0.95 per notch between 2.1 and 7 radii.
- Driver profile opens the native driver page (below).

## Interplay with the WebUI

While a native screen runs (loading screen, main menu, end credits, demo
archive, Rivals, Downloads, offline, online, driver page, settings; map
loading has no UI screen code), the UI reports
`UI_WEB_SCREEN` 15. The client treats it like screen 0: the browser stays
hidden and the game draws. While the game is disconnected, though, the
client starts CEF and loads the page, so the screens that are still WebUI
pages open without a load.

Going from a native screen to a WebUI screen, the browser shows the new page
a few frames later. Until then the client keeps the last native frame on
screen (`CL_WebGarageHoldFrame`, at most 1 s) instead of drawing black.

Without CEF (`cl_webUI` 0, or no WebUI build) nothing of this applies.
