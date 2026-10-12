#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
BINARY=${1:-./build/living-village}
CHECK_DIR=$(mktemp -d)
trap 'rm -rf "$CHECK_DIR"' EXIT HUP INT TERM
export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
"$BINARY" --save-dir "$CHECK_DIR/worlds-test" --frames 3 > "$CHECK_DIR/start.log" 2>&1
test ! -e "$CHECK_DIR/worlds-test/village.lvsave"
test ! -e "$CHECK_DIR/worlds-test/worlds"
"$BINARY" --new --seed 42 --biome woodland --layout island --world-name 'Island One' --save-dir "$CHECK_DIR/worlds-test" --frames 3 > "$CHECK_DIR/first.log" 2>&1
grep -Eq '^GENERATION 1 1 2 "Island One"$' "$CHECK_DIR/worlds-test/worlds/world-000001.lvsave"
cp "$CHECK_DIR/worlds-test/worlds/world-000001.lvsave" "$CHECK_DIR/first-save"
"$BINARY" --new --seed 99 --biome arid --layout coast --world-name 'Coast Two' --save-dir "$CHECK_DIR/worlds-test" --frames 3 > "$CHECK_DIR/second.log" 2>&1
cmp "$CHECK_DIR/first-save" "$CHECK_DIR/worlds-test/worlds/world-000001.lvsave"
grep -Eq '^world-000002$' "$CHECK_DIR/worlds-test/active-world.txt"
"$BINARY" --save-dir "$CHECK_DIR/worlds-test" --frames 3 > "$CHECK_DIR/resume.log" 2>&1
grep -Eq '^GENERATION 1 3 3 "Coast Two"$' "$CHECK_DIR/worlds-test/worlds/world-000002.lvsave"
printf '%s\n' 'broken current save' > "$CHECK_DIR/worlds-test/worlds/world-000002.lvsave"
printf '%s\n' 'broken backup' > "$CHECK_DIR/worlds-test/worlds/world-000002.lvsave.bak"
"$BINARY" --save-dir "$CHECK_DIR/worlds-test" --frames 3 > "$CHECK_DIR/corrupt.log" 2>&1
grep -Eq '^broken current save$' "$CHECK_DIR/worlds-test/worlds/world-000002.lvsave"
if "$BINARY" --headless --seed 4294967296 --steps 1 > "$CHECK_DIR/overflow.log" 2>&1; then
    echo 'FAIL: overflowing CLI seed accepted' >&2
    exit 1
fi
"$BINARY" --new --seed 7 --biome taiga --layout canyon --starting-food 6 --population 12 --ai-diagnostics --save-dir "$CHECK_DIR/new-options" --frames 12 > "$CHECK_DIR/options.log" 2>&1
grep -Eq '^GENERATION 1 4 9 ' "$CHECK_DIR/new-options/worlds/world-000001.lvsave"
"$BINARY" --benchmark-villagers 6 --ai-diagnostics --frames 12 > "$CHECK_DIR/live.log" 2>&1
grep -Eq 'simulation_steps=[1-9][0-9]*' "$CHECK_DIR/live.log"
"$BINARY" --headless --steps 1 --population 20 > "$CHECK_DIR/population.log" 2>&1
grep -q ", 20 alive" "$CHECK_DIR/population.log"
mkdir -p "$CHECK_DIR/ports/livingvillage/images"
cp assets/cover.png "$CHECK_DIR/ports/livingvillage/images/cover.png"
printf '%s\n' '#!/bin/sh' > "$CHECK_DIR/ports/LivingVillage.sh"
"$BINARY" --register-port-art "$CHECK_DIR/ports" > "$CHECK_DIR/art.log" 2>&1
grep -q './livingvillage/images/cover.png' "$CHECK_DIR/ports/gamelist.xml"
echo 'PASS: first boot waits for world settings; CLI generation preserves older worlds; active slot resumes; corrupt saves are not overwritten; seed overflow rejected'
