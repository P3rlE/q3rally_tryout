#!/bin/bash
# Screenshot of a native screen in a pinned state, for a side by side check
# against the WebUI screenshots (the WebUI itself is in the git history):
#
#   tools/ui_webstyle/native_screenshot.sh <width> <height> <state> <out.png> [seconds]
#
# <state> is "stage,percent,prompt" for the loading screen,
# "main:name,rank,points,quit" for the main menu, "credits:seconds" for the
# end credits, paused that many seconds into the roll, or
# "rivals:search,scroll,rival" for Rivals (rival -1: the first match) or
# "downloads:state,progress,type,search,item" for Downloads with the catalog
# tests/fixtures/downloads_index.json (type 0-3: all, tracks, vehicles, skins;
# item -1: the first match), or "demos:selected,scroll" for the demo archive
# with the recordings of tests/fixtures/demo_names.json (empty files), or
# "offline:mode,search,selected,scroll[,pagescroll[,2]]" for the offline setup
# (step 1, or step 2 with ",2": scroll is then the starting grid's), or
# "driver:tab,filter,pagescroll,dialog" (Per of tests/fixtures/profiles active)
# or "nodriver:..." (no profile) for the driver page, or
# "online:row,search,listscroll,pagescroll,dialog" for the server browser with
# the favorites 127.0.0.1:27961-27963 (dedicated servers started beforehand), or
# "config:presses[,capture]" for the settings (the setup page, then the buttons
# with these item indices pressed, "2,3"; "c<item>" opens the key capture for
# that item) or "race:presses" for the race control pages over a race on
# q3r_sk1 (the pause page, then the presses; give it about 150 seconds). The last ones come up
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
elif [ "${3#demos:}" != "$3" ]; then
    mkdir -p "$WORK/home/baseq3r/demos"
    python3 -c 'import json,os,sys
for name, size in json.load(open(sys.argv[1])):
    with open(os.path.join(sys.argv[2], name), "wb") as f: f.truncate(size)' \
        "$ROOT/tests/fixtures/demo_names.json" "$WORK/home/baseq3r/demos"
    STATE=(+set ui_demosPreview "${3#demos:}")
elif [ "${3#downloads:}" != "$3" ]; then
    mkdir -p "$WORK/home/baseq3r"
    cp "$ROOT/tests/fixtures/downloads_index.json" "$WORK/home/baseq3r/downloads_index.json"
    # installed, as the measurement marks them: empty packages of those ids
    python3 -c 'import sys,zipfile;[zipfile.ZipFile(f,"w").close() for f in sys.argv[1:]]' \
        "$WORK/home/baseq3r/trk_lava_falls_rev.pk3" "$WORK/home/baseq3r/trk_dirty_fun_2.pk3"
    STATE=(+set ui_downloadsPreview "${3#downloads:}" +set ui_dl_indexpath downloads_index.json)
elif [ "${3#driver:}" != "$3" ] || [ "${3#nodriver:}" != "$3" ]; then
    # the driver page: Per active (tests/fixtures/profiles), or no profile at all
    mkdir -p "$WORK/home/baseq3r/profiles"
    cp "$ROOT"/tests/fixtures/profiles/*.json "$WORK/home/baseq3r/profiles/"
    if [ "${3#driver:}" != "$3" ]; then
        STATE=(+set ui_driverPreview "${3#driver:}" +set profile_active Per)
    else
        STATE=(+set ui_driverPreview "${3#nodriver:}" +set profile_active "")
        rm "$WORK"/home/baseq3r/profiles/*.json
    fi
elif [ "${3#online:}" != "$3" ]; then
    # the favorites: three local servers (start them first) and one that does not answer
    STATE=(+set ui_onlinePreview "${3#online:}" +set ui_browserMaster 7 +set net_port 27990 +set server1 127.0.0.1:27961
           +set server2 127.0.0.1:27962 +set server3 127.0.0.1:27963 +set server4 127.0.0.1:27969)
elif [ "${3#config:}" != "$3" ]; then
    mkdir -p "$WORK/home/baseq3r/profiles"
    cp "$ROOT"/tests/fixtures/profiles/*.json "$WORK/home/baseq3r/profiles/"
    PRESSES=${3#config:}
    CAPTURE=$(echo "$PRESSES" | tr ',' '\n' | sed -n 's/^c//p')
    PRESSES=$(echo "$PRESSES" | tr ',' '\n' | grep -v '^c' | paste -sd, -)
    STATE=(+set ui_configPreview "${PRESSES:-0}" +set ui_configCapture "${CAPTURE:-}" +set profile_active Per)
elif [ "${3#race:}" != "$3" ]; then
    mkdir -p "$WORK/home/baseq3r/profiles"
    cp "$ROOT"/tests/fixtures/profiles/*.json "$WORK/home/baseq3r/profiles/"
    {
        echo "devmap q3r_sk1"; echo "wait 400"; echo "togglemenu"; echo "wait 100"
        for press in $(echo "${3#race:}" | tr ',' ' '); do echo "set ui_configPress $press"; echo "wait 100"; done
    } >"$WORK/home/baseq3r/race.cfg"
    STATE=(+set vm_game 2 +set profile_active Per +set g_gametype 0 +set bot_minplayers 3 +exec race.cfg)
elif [ "${3#offline:}" != "$3" ]; then
    STATE=(+set ui_offlinePreview "${3#offline:}")
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
