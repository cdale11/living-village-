# Live AI diagnostics

Press **Y**, then navigate left/right to **Neural learning** (page 4 of 9) or
**Agent diagnostics** (page 9). These two pages keep the fixed-step simulation
running. **Start** pauses it in the main menu; focus/lid background events still
save and pause. Other information pages pause simulation as before.

## Village and wildlife totals

- **Value predictions this session:** real MLP evaluations during candidate
  action scoring. These are reward/value estimates, not future-world rollouts.
  Inspector sensitivity probes and training's internal forward pass are excluded.
- **Training this session:** successful SGD steps, including bounded replay.
- **Steps changing weights:** SGD steps with nonzero measured weight differences.
  Increasing values confirm learning altered actual model parameters.
- **Retained outcomes / replay:** saved lifetime counts from the current models.
- **Last-outcome mean absolute error:** mean absolute reward prediction residual
  for models with an observed outcome. It is a current sample across models,
  not an accuracy percentage or a historical validation average. It can rise
  when circumstances change. Before any outcomes it reads **No outcomes yet**.
- **Saved SGD:** lifetime training steps restored from saves.

Totals include living residents and current wildlife. They can decrease when
models leave through deaths; these are current-model totals, not a world ledger.
Session counters start at zero on load and are intentionally not saved. The
underlying learned weights, outcomes and replay histories remain persistent.

## Individual diagnostics

On page 9, **L1/R1** chooses a resident. The page displays that resident's real
prediction count, training/weight-changing steps, last SGD absolute weight delta,
chosen goal's neural bonus, most recent reward, replay occupancy and whether an
action outcome is pending. Counters advance when decisions/outcomes occur, not
on every frame. A deceased resident's model remains inactive.

On **Wildlife**, L1/R1 selects an animal/colony; its session prediction and SGD
counts are shown alongside biology, current action and lifetime outcomes.

**A** on either diagnostic page freezes/resumes training. Frozen agents still
predict using their current weights, while no new SGD weight updates occur.
A model can have zero session training despite retained learned weights after
loading. Pending actions need an outcome before learning occurs.

Desktop diagnostics launch (temporary world, no save changes):

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/living-village \
  --benchmark-villagers 6 --ai-diagnostics --frames 120
```

Regression tests check inference without weight modification, nonzero actual
SGD changes, replay steps, paused-training inference and session reset on load.
