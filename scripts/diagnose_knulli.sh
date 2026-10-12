#!/bin/sh
# Run from SSH for headless measurements, or from a released display session
# for graphics/input. Never reads or writes player saves.
set -eu
MODE=${1:-system}
case "$MODE" in system|cpu|graphics|input) ;; *) echo "Usage: $0 [system|cpu|graphics|input]" >&2; exit 2 ;; esac
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
mkdir -p logs
REPORT=$(mktemp "$ROOT/logs/diagnostic-${MODE}-XXXXXX")
echo "Report: $REPORT"
if [ -f config/platform.env ]; then . ./config/platform.env; fi
{
    echo "Living Village diagnostic: $MODE"
    date -u
    uname -smr
    if [ -r /etc/os-release ]; then cat /etc/os-release; fi
    if [ -r /proc/meminfo ]; then
        sed -n '/^MemTotal:/p; /^MemAvailable:/p; /^SwapTotal:/p' /proc/meminfo
    fi
    for supply in /sys/class/power_supply/*; do
        [ -r "$supply/type" ] || continue
        [ "$(cat "$supply/type")" = Battery ] || continue
        for field in status capacity; do
            if [ -r "$supply/$field" ]; then printf 'Battery %s: ' "$field"; cat "$supply/$field"; fi
        done
    done
    if [ -r /proc/bus/input/devices ]; then cat /proc/bus/input/devices; fi
    echo "Configured video backend: ${SDL_VIDEODRIVER:-OS default}"
    echo "Configured audio backend: ${SDL_AUDIODRIVER:-OS default}"
    case "$MODE" in
        system) ;;
        cpu) ./bin/living-village --headless --steps 720000 --benchmark-villagers 20 ;;
        graphics) ./bin/living-village --fullscreen --benchmark-villagers 20 --frames 900 ;;
        input) ./bin/living-village --fullscreen --benchmark-villagers 6 --diagnose-input --frames 1800 ;;
    esac
} >"$REPORT" 2>&1 || {
    result=$?
    cat "$REPORT"
    echo "Diagnostic failed ($result). Report retained: $REPORT" >&2
    exit "$result"
}
cat "$REPORT"
