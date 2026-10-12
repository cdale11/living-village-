# Lives, networks, teaching, policies and outcome evaluation

## Follow residents

The inspector's Life History page records up to 64 meaningful experiences per
resident, with their actual simulation ticks. Routine eating, harvesting and
ordinary visits stay in the small recent history rather than flooding the life
book. Birth, family, loss, migration, construction, teaching and reconciliation
can appear when those experiences actually happen. Older saves have no invented
biographies: collection starts after upgrade.

A pins/unpins a resident on this page; six pins are allowed. The Pinned Residents
village page opens these records. Pinned deceased residents remain within the
existing 32-person deceased archive, replacing other archived records as needed.
Pins do not add living population or expand the archive. Up/down scrolls life
records; left/right changes inspector pages. Family and strongest relationships
remain inspectable. This is a bounded record, not a complete lifetime journal.

## Networks and memories

Social Networks cycles outgoing trust and resource-memory map views for the
selected resident. L1/R1 changes the resident. Positive trust is green, negative
trust is red; labels show the stored outgoing value. Relationships can be
asymmetric. Resource markers represent the selected resident's finite memories:
pink food, green wood, grey stone; depleted memories are dim. These views do not
reveal additional locations to agents or change their decisions.

## Teaching through contact

Completed social encounters can teach one gathering/building/farming skill.
The teacher must be an adult with cooperation at least .5; both participants must
be alive, within two tiles, not distrust one another, have hunger below 60 and
energy at least 50. Learners must be at least six years old. The strongest skill
gap greater than .1 determines the lesson. The learner gains at most .005, never
exceeding the teacher; teacher/learner consume .5/.3 energy. Each learner receives
at most one lesson per simulated day. Lessons require an actual social encounter
in ordinary execution. No neural weights are copied. Teaching is opportunistic,
not a new scheduled occupation. Village Policies can disable it.

## Player policies

Food reserve targets cycle 6/12/24/36 and are capped by actual storage capacity.
Targets influence foraging/delivery utility and farm planning; hungry people can
still eat. Project priority is Balanced, Food, Housing or Maintenance. Food and
Housing boost work on matching projects; Housing prefers shelter when residents
lack beds. Maintenance boosts repair work and pauses new automatic projects while
existing structures need repairs. Policies require existing resources/time and
preserve agent goal selection. Autonomous planning can also be toggled here.

Conservation areas now record vegetation, moisture, tree and local wildlife
baselines when established. Inspect their center to compare baseline and current
conditions. Old areas have unknown baselines. Weather, grazing and other changes
also contribute: this comparison is not proof that conservation caused recovery.

## Observe behavioral outcomes

Each resident retains goal time, actual collected units, completed measured
**decision intervals**, empty harvest attempts, unreachable candidates and net
hunger/health changes over completed measured intervals. Counts continue with
MLP training frozen. Gathering time includes travel while that goal is selected.
An interval is not a complete journey or task: agents can reconsider a goal.
Unreachable candidates are failed route checks during utility candidate selection,
not trips that were physically made or every pathfinding call.
Units are actual resources collected; they are not inferred from inventory deltas.
These metrics begin after upgrade; pre-upgrade pending intervals are not credited
as newly measured intervals. Archived residents may eventually be removed, so
aggregating retained residents is not an unlimited village lifetime total.

The existing controller export writes `resident-lives.csv`, `learning-outcomes.csv`
and `conservation.csv` alongside previous reports. Repeated resident-wide totals
in per-goal outcome rows should not be summed across goals. Unknown conservation
baselines are blank.

```sh
make lives-test
./scripts/compare_learning.sh ./build/living-village 96000 > /tmp/learning.csv
```

The comparison runs three seeds in Temperate and Arid worlds, learning/frozen
pairs, up to 40 days each. Both modes disable the director and neural exploration;
training alone differs initially. Birth/arrival/death counts distinguish population
growth from survival. Practice, teaching and physical rules continue
in both. Initial models are identically seeded; the frozen baseline is initially
untrained. Divergent decisions can change later environment/random draws, so this
is a whole-simulation comparison, not identical weather playback. Results cannot
establish general superiority from a few seeds. The headless options never load
or write player saves. **--freeze-learning** and **--no-exploration** require headless.

Version-10 atomic saves retain pins, measured life records, metrics, policy,
teaching cooldowns and conservation baselines. Versions 1–9 migrate without
inventing old observations; older binaries cannot read version 10. Preserve the
whole save folder and custom configuration before upgrading.
