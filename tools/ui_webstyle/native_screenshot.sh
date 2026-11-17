#!/bin/bash
# Screenshot of a native screen in a pinned state, for a side by side check
# against the WebUI (measure_webui_loading.cjs, measure_webui_main.cjs):
#
#   tools/ui_webstyle/native_screenshot.sh <width> <height> <state> <out.png> [seconds]
#
# <state> is "stage,percent,prompt" for the loading screen,
# "main:name,rank,points,quit" for the main menu, "credits:seconds" for the
# end credits, paused that many seconds into the roll, or
# "rivals:search,scroll,rival" for Rivals (rival -1: the first match). The last two come up
# once the loading screen has run through (give it about 40 seconds).
#
# Needs a Linux client build (make BUILD_CLIENT=1), Xvfb, Mesa and ImageMagick's
# import. The game data is this checkout's baseq3r plus the built QVMs.
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD=${BUILD:-$ROOT/engine/build/release-linux-x86_64}
WORK=$(mktemp -d)
trap 'kill $GAME $XVFB 2>/dev/null; rm -rf "$WORK"' EXIT
mkdir -p "$WORK/baseq3r/vm" "$WORK/home"
for entry in "$ROOT"/baseq3r/*; do
    [ "$(basename "$entry")" = vm ] || ln -s "$entry" "$WORK/baseq3r/"
done
cp "$BUILD"/baseq3r/vm/*.qvm "$WORK/baseq3r/vm/"
DISPLAY_NUMBER=${DISPLAY_NUMBER:-91}
Xvfb ":$DISPLAY_NUMBER" -screen 0 "${1}x${2}x24" >/dev/null 2>&1 & XVFB=$!
sleep 1
if [ "${3#main:}" != "$3" ]; then
    STATE=(+set ui_mainMenuPreview "${3#main:}")
elif [ "${3#rivals:}" != "$3" ]; then
    STATE=(+set ui_rivalsPreview "${3#rivals:}")
elif [ "${3#credits:}" != "$3" ]; then
    STATE=(+set ui_creditsPreview "${3#credits:}")
else
    STATE=(+set ui_gfxLoadingStagePause 15000 +set ui_gfxLoadingPreview "$3")
fi
DISPLAY=":$DISPLAY_NUMBER" LIBGL_ALWAYS_SOFTWARE=1 "$BUILD/q3rally.x86_64" \
    +set fs_basepath "$WORK" +set fs_homepath "$WORK/home" +set vm_ui 2 +set vm_cgame 2 \
    +set sv_pure 0 +set r_fullscreen 0 +set r_mode -1 +set r_customwidth "$1" +set r_customheight "$2" \
    +set s_initsound 0 +set com_introPlayed 1 "${STATE[@]}" \
    +set model alpine/red +set rim svt_cobra >"$WORK/log.txt" 2>&1 & GAME=$!
sleep "${5:-16}"
DISPLAY=":$DISPLAY_NUMBER" import -window root "$4"
