# Neural director

The director is a small offline C++ MLP (37 inputs, 12 hidden units, 469 weights)
with bounded replay. It scores three opportunities: leave conditions alone,
favor a wet transition, or favor a dry transition. It observes welfare, available
stored/carried food, activity diversity, average soil moisture, population,
wildlife, season and biome. Villagers retain their autonomous decision systems.

Use **Y → left/right to Neural Director → A** to cycle **Observer → Adaptive →
Off**. The diagnostics remain live; Start pauses. Observer is the default for new
and migrated worlds, proposes once per day and never intervenes or trains on its
unexecuted proposals. Off does no daily observation/inference.

Adaptive scores once every three simulated days. A weather opportunity changes
the next daily transition's rain/snow probability by at most eight percentage
points. Temperature still decides rain versus snow. Storm thresholds, weather
intensity, ecology, resource consumption and resident decisions keep their normal
rules. No resource, building, relationship or birth is fabricated. Bias clears on
the following daily observation. Mode changes immediately clear bias and cancel
pending evaluations. There is no intervention while ecology is disabled.

Low food reserves, poor welfare, extinction or a flood alert permit only leaving
conditions alone. Otherwise a bounded personal PRNG occasionally explores a
permitted action. Independent director randomness never consumes agent/ecology
randomness. At most one opportunity is applied per three days.

After three days, the executed decision receives a clamped score based on change
in a weighted proxy: 65% mean welfare (health, energy, hunger), 25% food security,
10% distinct resident activities. A loss of residents receives an additional
penalty. These are provisional, measurable proxies, not a definition or proof of
emergence. Ordinary world variation can influence them. The panel's predicted and
observed changes report association, not causal attribution or calibrated
confidence. Observer proposals have no counterfactual training target.

The existing global neural-learning toggle also freezes director training and
experimentation. Inference continues. Models, replay, mode, counters, PRNG and
pending three-day evaluations persist in checksummed atomic version-8 saves.
Versions 1–7 migrate; older executables cannot read version 8. Back up the whole
saves directory before upgrading. Session inference/update telemetry resets on
load; lifetime outcome/SGD counters persist.

Headless experiments (no display or save writes):

```sh
make director-test
./build/living-village --headless --seed 43 --steps 240000 --director off
./build/living-village --headless --seed 43 --steps 240000 --director observer
./build/living-village --headless --seed 43 --steps 240000 --director adaptive
```

Repeat across seeds and biomes. Off/Observer should have identical simulation
outcomes. Adaptive differences do not automatically imply improvement. Longer
comparison experiments and physical-device performance validation are required
before changing the default to Adaptive or expanding its intervention vocabulary.
Future work: causal records, trajectory-level learning measures, regional
opportunities, richer trend memory and controller tuning. This initial director
has one previous outcome baseline and bounded experience replay; no LLM runtime.
