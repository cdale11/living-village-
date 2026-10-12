#!/bin/sh
set -eu
# KNULLI Ports launcher: put this beside livingvillage/ in /userdata/roms/ports.
# All writes stay on the userdata partition. Exit returns to EmulationStation.
PORTS_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
VILLAGE_DIR="$PORTS_DIR/livingvillage"
cd "$VILLAGE_DIR"
mkdir -p logs saves
# Optional platform overrides, e.g. SDL_VIDEODRIVER=kmsdrm. Keep the OS defaults
# until the device's --diagnose-input and video log establish the right backend.
if [ -f config/platform.env ]; then . ./config/platform.env; fi
./bin/living-village --register-port-art "$PORTS_DIR" >logs/art.log 2>&1 || echo "Cover registration skipped; see logs/art.log" >&2
LOG=logs/game.log
if [ -f "$LOG" ]; then mv -f "$LOG" logs/game.previous.log; fi
{
    echo "Living Village C++ startup $(date -u)"
    uname -m
    exec ./bin/living-village --fullscreen --save-dir "$VILLAGE_DIR/saves" "$@"
} >>"$LOG" 2>&1
