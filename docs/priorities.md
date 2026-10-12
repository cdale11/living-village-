# Next-priority deliverable status

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
