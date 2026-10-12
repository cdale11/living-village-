#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TEMP=$(mktemp -d /tmp/lv-diagnostics-XXXXXX)
trap 'rm -rf "$TEMP"' EXIT HUP INT TERM
INSTALL="$TEMP/installation with spaces"
mkdir -p "$INSTALL/scripts" "$INSTALL/bin" "$INSTALL/config" "$INSTALL/saves"
cp "$ROOT/scripts/diagnose_knulli.sh" "$INSTALL/scripts/"
printf 'preserve me\n' > "$INSTALL/saves/village.lvsave"
cat > "$INSTALL/bin/living-village" <<'STUB'
#!/bin/sh
printf 'Arguments:'
printf ' <%s>' "$@"
printf '\n'
case "$*" in *--headless*) exit 7 ;; esac
STUB
chmod +x "$INSTALL/bin/living-village"
"$INSTALL/scripts/diagnose_knulli.sh" system > "$TEMP/system"
grep -q 'Living Village diagnostic: system' "$TEMP/system"
if grep -q 'Arguments:' "$TEMP/system"; then echo 'System mode launched game' >&2; exit 1; fi
"$INSTALL/scripts/diagnose_knulli.sh" graphics > "$TEMP/graphics"
grep -q '<--benchmark-villagers> <20> <--frames> <900>' "$TEMP/graphics"
"$INSTALL/scripts/diagnose_knulli.sh" input > "$TEMP/input"
grep -q '<--diagnose-input> <--frames> <1800>' "$TEMP/input"
result=0
"$INSTALL/scripts/diagnose_knulli.sh" cpu > "$TEMP/cpu" 2>&1 || result=$?
[ "$result" -eq 7 ]
grep -q 'Diagnostic failed (7)' "$TEMP/cpu"
[ "$(cat "$INSTALL/saves/village.lvsave")" = 'preserve me' ]
[ "$(find "$INSTALL/logs" -type f | wc -l)" -eq 4 ]
result=0
"$INSTALL/scripts/diagnose_knulli.sh" invalid > "$TEMP/invalid" 2>&1 || result=$?
[ "$result" -eq 2 ]
echo 'Diagnostic script modes, failure reporting, spaced paths and save isolation passed'
