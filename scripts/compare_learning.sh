#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
LV_BINARY=${1:-./build/living-village}
if [ "$#" -eq 0 ] && [ -x ./bin/living-village ]; then LV_BINARY=./bin/living-village; fi
LV_STEPS=${2:-96000}
case "$LV_STEPS" in ''|*[!0-9]*) echo "Steps must be an integer" >&2; exit 2;; esac
[ "$LV_STEPS" -ge 2400 ] && [ "$LV_STEPS" -le 96000 ] || { echo "Use 2400..96000 steps (1..40 days)" >&2; exit 2; }
printf '%s\n' 'seed,biome,mode,alive,births,arrivals,deaths,food_units,gather_units,gather_seconds,empty_harvests,unreachable_candidates,lessons'
for lv_seed in 43 44 45; do
  for lv_biome in temperate arid; do
    for lv_mode in learning frozen; do
      set -- --headless --seed "$lv_seed" --biome "$lv_biome" --steps "$LV_STEPS" --director off --no-exploration --threads 1
      if [ "$lv_mode" = frozen ]; then set -- "$@" --freeze-learning; fi
      lv_output=$("$LV_BINARY" "$@")
      printf '%s\n' "$lv_output" | awk -v seed="$lv_seed" -v biome="$lv_biome" -v mode="$lv_mode" '
        /^Headless:/ { for (i=1;i<NF;i++) { if ($(i+1)=="alive") alive=$i; if ($(i+1)=="births,") births=$i; if ($(i+1)=="arrivals,") arrivals=$i; if ($(i+1)=="deaths,") deaths=$i } }
        /^Outcomes:/ { for (i=2;i<=NF;i++) {split($i,a,"="); m[a[1]]=a[2]} }
        END { if (alive=="" || m["food_units"]=="") exit 1; print seed "," biome "," mode "," alive "," births "," arrivals "," deaths "," m["food_units"] "," m["gather_units"] "," m["gather_seconds"] "," m["empty_harvests"] "," m["unreachable_candidates"] "," m["lessons"] }'
    done
  done
done
