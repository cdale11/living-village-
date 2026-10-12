# Online neural learning

Every resident has an independent C++ multilayer perceptron (MLP). There is no
Python, downloaded model, GPU compute, server or external ML library. KNULLI is
the handheld target; desktop Linux remains the development/test platform.

## What the model does

The existing decision system proposes actions that are valid for the resident's
age, inventory, available resources and reachable destinations. The MLP predicts
the outcome of each candidate, adding up to **18 utility points** in either
direction. The highest combined score selects the action. Immediate carried-meal
interrupts still protect a severely hungry resident. Execution always checks
physical location, supplies, materials, work and capacity.

This is a contextual outcome learner combined with survival utility rules. It
can change an agent's preferred action among competing candidates. It does not
invent new action types, rewrite game code, guarantee optimal behavior or learn
language. The model's weights change during gameplay; its topology stays fixed.
There is no offline progression or training while the game is paused.

## Network and observations

Each model has **37 inputs → 12 tanh hidden units → 1 tanh outcome value**, with
469 trainable weights/biases. Double-precision weights occupy about 3.7 KiB per
resident, plus a small pending-experience record. Starting residents, newborns
and arrivals receive distinct deterministic initial weights from the world seed
and permanent resident ID. Initialization uses its own small PRNG and does not
consume the world's simulation random stream. Children learn too, within their
age-appropriate candidate actions. Children inherit personality, not a parent's
trained weights.

Nineteen inputs identify the candidate action. Eighteen normalized observations
are hunger, energy, health, loneliness, cooperation, curiosity, family drive,
gather/build/farm skills, carried food, carried materials, shared food per living
resident, outstanding wood demand, outstanding stone demand, spare beds,
Manhattan trip distance, and the candidate's base utility. Values are clamped to
[0, 1]. Plans and shared stock are communal information; resource candidates
still come from local memories or published gathering designations. Exploration
has an unknown destination when scored, so its distance input is zero.

## Learning from actual outcomes

An experience begins when the agent chooses a goal. It includes travel and
execution until the next decision, or death. A pending experience stores its
original inputs, needs, accumulated useful work and elapsed simulated seconds.
An SGD/backpropagation update fits the model to the observed reward:

```
reward = useful work
       + 0.6 * hunger reduction / 45
       + 0.4 * energy gain / 40
       + 0.2 * loneliness reduction / 45
       + 0.8 * health gain / 20
       - min(0.6, elapsed seconds * 0.003)
```

Death adds -1. The final target is clamped to [-1, 1]. Useful work is credited
only after real effects: gathering, harvesting, sharing, transporting needed
materials, construction, refuelling, caregiving and discovering locations.
Cooperation scales communal rewards. Rest, eating and socialization receive
reward through actual changes in needs. Failed pickups receive no work credit,
and long or unproductive trips incur cost. Gathering also gains learned skills,
which remain distinct from neural weights.

Training uses squared prediction error, learning rate 0.025, clipped gradients
and weights bounded to [-3, 3]. Every new decision outcome receives one update, plus one old replay sample
at half the learning rate. A fixed 32-outcome ring replaces older samples;
there is no growing dataset. The pending experience, replay and per-agent RNG
are retained in saves. Last reward/error report the fresh outcome. This is short-horizon feedback: it does not yet propagate
credit through entire construction or family lifetimes, and the reward rules
influence what it learns. Different residents can develop different preferences;
improvement in every village is not promised.

## Inspecting and controlling learning

- **X**, then D-pad left/right: reach the fifth inspector page, personal MLP.
- It shows training count, base utility, the selected learned adjustment, and
  the last training reward/error.
- The three context effects compare the current model's output on its last
  recorded decision with that observation set to zero. These are model
  sensitivities, not exact additive or causal explanations. They can change
  after subsequent training; the displayed selected adjustment is the value
  actually used at decision time.
- **Y**, then left/right: reach neural learning. **A** pauses/resumes training.
  Pausing freezes weights while agents continue using their learned values.
  Experiences ending while training is paused are discarded, not queued.

## Saves and tests

Save version **6** contains each model's weights, learning statistics, pending
inputs, reward components and the village training preference. The existing
atomic writes, checksums and backup recovery remain. Save validation bounds
weights, observations and statistics and rejects non-finite values or a
mismatched network schema. Versions 1–3 load automatically with fresh neural
models; version 4 preserves its trained weights and pending experiences; their established world state is retained. Back up saves before upgrade;
older binaries cannot read version 7.

`make test` includes learned action-selection changes, successful/failed outcome
discrimination, reward reversal/relearning, deterministic backpropagation, independent residents, live
training, invalid-action gates, freeze/resume, exact neural save continuation,
and old-world migration. The 3,500-day population regression also runs with
learning enabled. `--headless --steps 720000` reports total retained training
updates, time and peak RSS. These checks do not establish handheld FPS or battery
impact; measure those on the RG34XX SP's actual SDL backend.

## Bounded experimentation and work practice

Healthy adults (hunger <60, energy >=35, health >=80, shared food at least
one per resident) can sample a candidate action type on one in eight decisions.
That type gains eight utility points if a valid reachable candidate exists.
Unavailable actions remain unavailable, and pressing pause training disables
experimentation. The separate selected trial bonus and count appear on X's sixth
page, alongside replay counts and actual work practice. Trials count selected
bonus actions; a trial does not necessarily change the winning action.
The agent's saved local RNG also chooses replay samples, so continuation is
deterministic within the supported simulation/runtime.

Practice adds up to six utility points for familiar work and creates changing
occupation labels; see [society rules](society.md). An MLP bonus, practice bonus
and experiment bonus remain subordinate to resource/location/action validity.

Separate saved worlds retain separate models and replay buffers. A snapshot
keeps exact neural state as a playable independent branch; renaming a world
does not reset training. Generation/name metadata in version 6 preserves
version 5 learned state.
