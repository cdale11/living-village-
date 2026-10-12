# Living Village roadmap

## Agreed direction — 12 October 2026

Prioritize deeper interactions between existing systems and make their causes
and consequences visible. The player should be able to observe a problem,
understand autonomous responses, influence conditions and watch the outcome.
The following work is planned, not implemented. Preserve offline C++ gameplay,
controller-only operation, bounded simulation costs and existing saves.

## Next release: observable emergence

1. **Explain events and follow residents.** Record bounded causal links from real
   simulation events and decisions; let the player inspect contributing causes.
   Add relationship and knowledge overlays, pinned residents, family/friendship
   views and biographies drawn from recorded experiences. Do not invent causal
   explanations when the underlying evidence was not recorded.
2. **Expose wildlife activity and population changes.** Show distinguishable
   feeding, drinking, sheltering, nesting, parenting and fleeing where those
   behaviors are implemented. Provide species population trends, births, deaths
   and habitat limitations, with controller-accessible inspection.
3. **Connect conservation to ecological recovery.** Add conservation zones and
   consequential grazing, regrowth and habitat recovery. Show resource flows
   and habitat health so the player can inspect the effects of intervention.
4. **Measure learning outcomes.** Extend diagnostics beyond inference/update
   counts to failed journeys, food per trip, survival and recovery after changing
   conditions. Add controlled headless comparisons of learning versus frozen
   models across multiple seeds. Weight changes alone are not proof of better
   decisions; improve feedback and delayed rewards before enlarging networks.
5. **Polish and validate on the handheld throughout.** Improve text sizing,
   contextual button hints, information hierarchy, zoom-specific detail,
   construction stages and environmental sound. Check readability, sustained
   frame times, CPU/RAM, battery impact, suspend behavior, audio and vibration
   on the actual device. Desktop results do not satisfy physical validation.

Acceptance focus: the player can trace a recorded ecological problem, inspect
how villagers respond, apply a conservation intervention and observe its actual
consequences. New records and overlays remain bounded; save migration and
headless regression checks accompany the systems they change.

## Subsequent priorities, in order

1. **More consequential ecology and persistent landscape history.** Distinct
   plant species, forest succession, seed dispersal and habitat preferences;
   woodland loss affects moisture, shelter and food. Abandoned farms regrow,
   travel creates paths, neglect leaves ruins, and grazing/erosion leave visible
   traces. Expand the next release's grazing and recovery mechanisms.
2. **Connected water and food survival.** Drinking water, wells, physical water
   hauling, food spoilage and preservation connect hydrology and storage to
   settlement decisions. Balance these needs against autonomous survival.
3. **Collective planning and teaching.** Residents propose projects from observed
   needs, contribute according to trust/resources and abandon unsuccessful
   plans. Teaching requires contact and time; skills and local knowledge can
   persist or disappear across generations.
4. **Broader player influence.** Gathering restrictions, construction priorities
   and food-reserve targets extend conservation zones while retaining autonomous
   resident decisions.
5. **Continued presentation improvements.** Recognizable activities, richer
   environmental audio and clearer maps/menus follow the behavior they expose.

Defer larger maps, additional biome choices and a sprawling technology tree
until these interactions are stronger. Retain survival safeguards while allowing
useful agent experimentation. No scripted narrative substitutes for emergence.

## Implemented baseline and outstanding validation

| Priority | Implemented scope | Verification still needed |
| --- | --- | --- |
| Handheld optimization/validation | Bounded AI/replay/data, temporary 6–20-agent benchmark, graphical FPS/work/RSS logging, battery readings where exposed, suspend save/pause paths | Actual device FPS, total OS memory, battery drain and lid behavior |
| Sound and vibration | Original procedural tones, configurable/mutable volume, optional SDL rumble, controller settings saved | Physical audio routing and motor exposure |
| Visual polish | Seasonal ground, animated water/work, readable need markers, night windows/fire glow, rain and workshop sprites | Small-screen appearance and actual backend speed |
| Specialization | Successful-work practice, changing professions, decaying history and utility preferences | Long-term balancing across more player villages |
| Richer society | Nearby hunger assistance, resource-claim disputes, reconciliation, existing bereavement and cooperative construction/care | Balancing across varied worlds |
| Economy/progression | Physical food/material barter and two supplied/work-gated workshop technologies | Expanded markets and technology remain optional future depth |
| Village history | Bounded chronicle, daily population/resource metrics, controller CSV exports | User feedback on reporting |

Every software item above is an implemented initial version. The list does not
claim a polished final game or physical measurements. ROCKNIX was explicitly
removed from scope. Online C++ MLP learning, experience replay and controlled
experimentation support these systems without network/model dependencies.

World management now includes first-run name/seed/biome/layout selection,
independent named saves, loading, renaming, snapshot branches and confirmed
inactive deletion with backup handling. Old current/previous files and trained
models migrate without regeneration. These controller flows still need physical
button/display validation on the handheld.

Validation tooling now includes a packaged KNULLI system/CPU/graphics/input
report script with no player-save access, and a two-seed, all-biome/layout
headless soak command. These make the remaining device checks reproducible;
they do not replace physical measurements. CI runs a short ecological survey.

The ecology expansion adds plant/soil biology, forest recruitment and mortality,
four wildlife types with inherited traits and learned preferences, finite nectar
and pollination, hydrology/snowmelt, floods/fire, weathering/traffic and actual
building maintenance. See [ecology](ecology.md) for rules and limitations.
Handheld performance and long-term ecological balancing remain validation work.

World creation now offers eight ecological biomes, twelve geographic layouts,
three starting food budgets and a cached seeded map preview. Live neural and
individual diagnostics report real inference, SGD, weight changes and outcomes,
with explicit session/lifetime distinctions. Terrain/shoreline/sprite details,
panel framing and a compact minimap improve readability. An original cover and
native atomic Ports metadata registration are packaged without replacing other
games. Physical validation of these new screens and theme integration remains.

The next handheld revision bundles artwork inside the game and registers it
automatically; adds 2–20 starting residents, nearby wildlife, animal-follow camera,
habitat-dependent regional dispersal, contact-based resource knowledge exchange,
live event/route cues and original cached pixel textures. It negotiates audio
formats, provides output selection and tests, supports SDL haptics/Linux FF, and
samples CPU/current RSS in the header. The device log confirms ALSA and controller
FF capability, but audible output, felt pulses and the new visuals still require
physical confirmation. No scripted emergence or automatic power-off progression
was introduced.

## Neural director milestone

Implemented initial offline MLP director with Off/Observer/Adaptive modes,
bounded weather opportunities, delayed three-day outcome learning, saved replay
and controller diagnostics. Observer remains the default. See [director](director.md)
for intervention limits, provisional reward proxies and evaluation commands.
Next director work follows causal event tracking and outcome evaluation: richer
trend memory, multi-seed comparisons and additional constrained opportunities.
Actual benefit to emergence and handheld cost remain validation work.

The CPU milestone adds a persistent detected-core worker pool (four total threads
on H700) for independent tile hydrology calculations with ordered reductions.
The first observation overlays expose real soil water, vegetation condition and
traffic compaction. These are initial steps toward the broader emergence tools;
causal event links, conservation rules and species population histories remain.

## Observable emergence milestone

Implemented bounded 5x5 conservation areas with actual natural-harvest/new-plan
restrictions; controller event-evidence inspection with explicit contributing
links for instrumented events; daily species counts/charts and CSV export.
Version-10 saves retain these records and migrate old worlds without invented
history. See [observable emergence](observable-emergence.md) for limits. Remaining:
full causal graphs, conservation outcome comparisons, pinned residents,
knowledge/relationship overlays and learning-outcome evaluation.

## Lives and outcomes milestone

Implemented pinned residents with bounded deceased retention, real life records,
outgoing trust/resource-memory views, physical skill teaching, reserve/project
policies, independent behavioral outcome metrics and paired headless learning
comparisons. Conservation centers expose measured baselines/current conditions.
See [lives and learning](lives-and-learning.md) for semantics and limits.
Remaining depth: full causal graphs, controlled conservation experiments, richer
plant species/succession, drinking-water/food preservation systems, collective
project proposals/abandonment, and broader long-term learning evaluation.
