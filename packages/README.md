# Ready-to-install ARM64 build

`living-village-knulli-arm64.tar.gz` contains the native C++17 game, Ports launcher,
internal cover art, configuration, documentation and diagnostics. Built from
source milestone **b645382** using Zig 0.13.0 for Cortex-A53 / AArch64 Linux with
a glibc 2.17 baseline. Gameplay requires no Python, model downloads or internet.

Download with GitHub's **Download raw file**, extract into `/userdata/roms/ports/`,
preserving the whole `livingvillage/saves/` folder and customized `config/`, then
launch **Ports → LivingVillage**. Replace launcher, binary, images, scripts and
documentation. Keep `config/audio-device.txt` if customized. Cover registration
runs automatically; a frontend game-list refresh may be needed for cached art.

## This release

- Saved pins for up to six residents, including bounded deceased retention.
  **X → Life History → A** pins/unpins; **Y → Pinned Residents → A** opens records.
  Life histories contain real meaningful events, readable on three rows with scrolling.
- **Y → Social Networks → A** cycles outgoing trust and actual resource memories.
  Strongest trust values have a readable side panel; no knowledge is granted to agents.
- Physical, trust/energy/skill-gated teaching during completed nearby conversations;
  daily learner limits, actual skill gains and recorded teacher/learner experiences.
- **Y → Village Policies** sets food reserve, project priorities, teaching and planning.
  Policies influence utility/planning while preserving survival/resource requirements.
- Inspector **Observed Outcomes** measures collection, goal time including travel,
  empty harvest attempts, unreachable utility candidates and net hunger changes.
  Measurements continue with MLP training frozen.
- Conservation centers compare recorded vegetation/tree baselines with current
  conditions. Old baselines remain unknown; no automatic recovery bonus.
- CSV exports add lives, learning outcomes and conservation reports. Packaged
  **scripts/compare_learning.sh** runs reproducible headless learning/frozen pairs.
- Version-10 saves migrate versions 1–9 without invented life/metric/baseline history.
  Existing director, ecology, CPU workers, artwork, audio/rumble and meters remain.

## Validation and limits

Native regression, controller/SDL smoke checks, save migration/continuation and
serial/four-thread equivalence are checked. GCC and SDK libc++ director/worker
tests exercise delayed training, Observer isolation, frozen models, safety gates,
chunk partition, exceptions and pool cleanup. Package checks verify AArch64 ELF,
matching binary/art and checksum. The 3,500-day native generational regression
completed with 20 living residents. Short desktop checks with 20 residents held
about 30 FPS in serial/four-thread modes; this does not establish device speedup. See [director](../docs/director.md) and
[architecture](../docs/architecture.md) for the implemented scope.

Lives tests verify bounded pinned archives, contact teaching/cooldowns, frozen
metrics, Housing/Maintenance planning, save continuation, v9 migration and controls.
Native regression checks, SDK libc++ tests and address/undefined-behavior checks
passed. The 3,500-day run retained 20 residents (six births, 16 deaths). Twelve
40-day learning/frozen runs across three seeds/two biomes had zero deaths; this
does not establish general learning superiority. A short desktop 20-resident
check held 30.08 FPS, 5.52 ms mean frame work and 9,588 KiB peak RSS.
See [lives and learning](../docs/lives-and-learning.md) for semantics and limits.

Director reward proxies are provisional; actual improvement in emergence has not
been established. Observer stays default. Device speedup, battery use, sound,
motor and new-screen readability require physical verification. Earlier versions
were confirmed to load and exit with Fn+Start on the user's handheld.

```sh
sha256sum -c packages/SHA256SUMS
```
