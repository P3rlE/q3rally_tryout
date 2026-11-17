# Native screens in the WebUI look

The WebUI (`engine/webui/garage`, shown through CEF on Windows) set the look of
the menus. Screens move back into the game one at a time and must look exactly
like their WebUI version, on every platform and without a browser. Done so
far: the graphics loading screen ("Getting ready.", WebUI screen 1), the
main menu ("Ready to race.", WebUI screen 2) and the end credits ("Thank you
for playing.", WebUI screen 3).

## How a screen is rebuilt

- **Layout from the CSS.** `q3_ui/ui_rally_gfxlayout.c` (loading screen),
  `q3_ui/ui_rally_mainlayout.c` (main menu) and
  `q3_ui/ui_rally_creditslayout.c` (end credits) compute every box of the screen
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

- `tools/ui_webstyle/measure_webui_loading.cjs`, `measure_webui_main.cjs` and
  `measure_webui_credits.cjs` measure the WebUI screens in Chrome (the
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
  credits, paused that far into the roll.

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

## Interplay with the WebUI

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
- Driver profile opens the WebUI's driver page with `cl_webUI` 1, otherwise
  the native profile overlay.

## Interplay with the WebUI

While a native screen runs (loading screen, main menu, end credits), the UI reports
`UI_WEB_SCREEN` 15. The client treats it like screen 0: the browser stays
hidden and the game draws. While the game is disconnected, though, the
client starts CEF and loads the page, so the screens that are still WebUI
pages open without a load.

Going from a native screen to a WebUI screen, the browser shows the new page
a few frames later. Until then the client keeps the last native frame on
screen (`CL_WebGarageHoldFrame`, at most 1 s) instead of drawing black.

Without CEF (`cl_webUI` 0, or no WebUI build) nothing of this applies.
