# Native screens in the WebUI look

The WebUI (`engine/webui/garage`, shown through CEF on Windows) set the look of
the menus. Screens move back into the game one at a time and must look exactly
like their WebUI version, on every platform and without a browser. The first
one is the graphics loading screen ("Getting ready.", WebUI screen 1).

## How a screen is rebuilt

- **Layout from the CSS.** `q3_ui/ui_rally_gfxlayout.c` computes every box
  of the screen from the rules in `garage.css`, number for number. This covers
  `clamp()` with `vw`/`vh`, `max-width`, the grid columns, margins and their
  collapsing, `min-height` and line wrapping. It works in screen pixels, which
  are CSS pixels at a device pixel ratio of 1. Chrome reserves 15 px for the
  scrollbar (`scrollbar-gutter: stable`), so the page is not centered on the
  screen.
- **Text like Chrome.** `q3_ui/ui_rally_webstyle.c` lays text out with the
  font's linear advances, its GPOS pair kerning, `letter-spacing` after every
  glyph and `text-transform`. Line boxes round ascent and descent to whole
  pixels and put the larger half of the leading below the text.
- **Things Chrome synthesizes:**
  - bold for weight 750 on a regular face (outline emboldened by em/24, as Skia does);
  - italic for `<em>` (skew 1/4);
  - tabular figures for list markers (`::marker { font-variant-numeric: tabular-nums }`).
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

- `tools/ui_webstyle/measure_webui_loading.cjs` measures the WebUI screen in
  Chrome. It records every box and the width of every text run, for ten
  viewports and all states.
- The result is `tests/fixtures/webui_loading_layout.json`.
- `tests/test_native_gfxloading.py` runs the native layout and text
  measurement against that fixture:
  - boxes must match within 0.5 px;
  - text runs within 0.1 px;
  - line breaks must match exactly.
- `tools/ui_webstyle/native_screenshot.sh` takes a screenshot of the real
  client under Xvfb, with Mesa's software renderer. The cvar
  `ui_gfxLoadingPreview "stage,percent,prompt"` pins the state, so the
  screenshot can be laid next to the WebUI screenshot of the same state.

What still differs from the WebUI:
- **Glyph edges.** Chrome positions glyphs on subpixels; the game draws them
  on whole pixels.
- **The car's materials.** The WebUI shows a three.js export with plain
  textures. The game renders the real car with its shaders: see-through glass,
  lit headlights. Camera, field of view, angle and framing are the same
  (`UI_DrawVehicleWebView`).
- **The cursor.** The game draws its own cursor.

## Interplay with the WebUI

While the native loading screen runs, the UI reports `UI_WEB_SCREEN` 15. The
client then starts CEF and loads the page hidden in its screen 1 state, so
the car viewer is already loaded.

For the hand-off, the UI reports the main menu with bit `0x100`. It keeps
drawing its own screen until the browser has shown the page. The client then
sets `cl_webPresented` to 2, and the UI switches to the main menu. The UI
waits for this at most 3 s.

Without CEF (`cl_webUI` 0, or no WebUI build) the UI switches at once.
