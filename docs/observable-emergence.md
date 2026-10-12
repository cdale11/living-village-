# Conservation, wildlife history and recorded event evidence

On the map move the camera, press **A** to inspect the central tile, then **Y**
to toggle a 5x5 conservation area centered there. Up to 16 centers are retained.
Turquoise outlines mark areas and their centers. Select that same center and
press Y again to remove it. Overlapping zones remain effective until all covering
centers are removed. Water centers are rejected; edges clip to the map.

Residents avoid natural food, wood and stone harvesting in these areas. The
execution check also blocks previously selected harvesting after protection is
added. Loose dropped supplies remain recoverable. New construction plans and
new harvest orders inside protected areas are rejected. Existing construction,
farms, travel and maintenance continue. Wildlife can still graze, drink and hunt.
Protection provides no growth bonus, extra water, food or animals: recovery
follows existing consumption, succession, soil, weather and fire rules. Protecting
accessible resources can worsen village shortages; choose areas accordingly.

**Y → Wildlife Trends** shows live species counts and up to 128 daily observations
for rabbits, deer, foxes and bee colonies. The chart has a fixed 0–48 scale.
Observations count living entities at the daily boundary; one bee entity is a
colony. Old saved days lack measurements and appear blank, rather than invented
zero populations. New days accumulate observations after migration.

**Y → Village History**, then **D-pad up/down**, browses retained events from newest
to oldest. **A** jumps to the contributing parent when its record is retained.
It shows recorded event-time evidence, a location when available and
one contributing parent event when explicitly linked. Initial coverage includes:

- storm/drought conditions, storm-triggered ignition and hydrology flood alerts;
- structural condition crossing the unsafe threshold;
- material deliveries with the villager's real goal-selection reason, and completed
  construction linked to a retained supplying delivery;
- shared food shortage/recovery thresholds;
- sampled animal deaths and recorded births;
- player establishment/removal of conservation areas.

A parent is one recorded contributor, not a complete causal explanation. Weather
links identify an actual weather context in the same simulated day; other terrain
and water conditions also affect outcomes. Old events and uninstrumented event
types explicitly report missing evidence. Links to expired parent records retain
the ID and report that the record is outside retained history. The chronicle stays
bounded to 512 events. Animal death logging retains its existing one-in-eight
sampling; population observations include all living animals.

**Y → Economy / History → A** exports these recorded fields in
`village-history.csv` and writes `wildlife-populations.csv` alongside the existing
CSV files. Unobserved historical species counts are blank and marked unrecorded.

Conservation, event IDs/evidence/parent links and observations persist in
version-10 atomic saves. Versions 1–9 migrate without generating historical evidence
or populations. Older executables cannot load version 10; preserve the whole saves
folder before upgrading.

```sh
make emergence-test
```

The first milestone exposes resource protection and recorded contributors. Full
causal graphs, conservation outcome comparisons, resident pinning, knowledge and
relationship overlays, and broader learning-outcome evaluation remain planned.
