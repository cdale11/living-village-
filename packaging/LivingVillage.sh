#!/bin/sh
BASE=/userdata/roms/ports/livingvillage
cd "$BASE" || exit 1
mkdir -p "$BASE/logs" "$BASE/saves"
export PYTHONPATH="$BASE/src"
exec python3 -m living_village.main --fullscreen >>"$BASE/logs/game.log" 2>&1
