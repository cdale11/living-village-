#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BINARY=${1:-./build/living-village}
CHECK_DIR=$(mktemp -d)
trap 'rm -rf "$CHECK_DIR"' EXIT HUP INT TERM
printf '%s\n' 'Existing player save must remain unchanged' > "$CHECK_DIR/village.lvsave"
cp "$CHECK_DIR/village.lvsave" "$CHECK_DIR/original"
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$BINARY" --benchmark-villagers 20 --frames 30 --save-dir "$CHECK_DIR" > "$CHECK_DIR/run.log" 2>&1
if ! grep -Eq 'simulation_steps=[1-9][0-9]*' "$CHECK_DIR/run.log"; then
    cat "$CHECK_DIR/run.log"
    echo 'FAIL: graphical benchmark did not actually simulate agents' >&2
    exit 1
fi
cmp "$CHECK_DIR/village.lvsave" "$CHECK_DIR/original"
"$BINARY" --headless --steps 1000 --benchmark-villagers 20 > "$CHECK_DIR/headless.log"
grep -Eq '20 alive' "$CHECK_DIR/headless.log"
echo 'PASS: interactive dummy benchmark actually simulates 20 agents and preserves player saves; headless smoke'
