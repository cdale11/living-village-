#!/bin/sh
# Offline, display-independent stability survey. Collapse is a reported outcome,
# not an assertion: difficult climates may fail without player intervention.
set -eu
BINARY=${1:-./build/living-village}
STEPS=${2:-240000}
case "$STEPS" in ''|*[!0-9]*) echo "Steps must be a positive integer" >&2; exit 2 ;; esac
[ "$STEPS" -gt 0 ] && [ "$STEPS" -le 100000000 ] || { echo "Steps out of range" >&2; exit 2; }
for seed in 123 98765; do
    for biome in temperate woodland meadow arid; do
        for layout in ponds river island coast highlands; do
            printf 'seed=%s biome=%s layout=%s\n' "$seed" "$biome" "$layout"
            "$BINARY" --headless --steps "$STEPS" --seed "$seed" --biome "$biome" --layout "$layout"
        done
    done
done
