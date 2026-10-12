# Ready-to-install ARM64 build

`living-village-knulli-arm64.tar.gz` contains the native C++17 game, Ports launcher,
internal cover art, configuration, documentation and diagnostics. Built from
source milestone **3ed7023** using Zig 0.13.0 for Cortex-A53 / AArch64 Linux with
a glibc 2.17 baseline. Gameplay requires no Python, model downloads or internet.

Download with GitHub's **Download raw file**, extract into `/userdata/roms/ports/`,
preserving the whole `livingvillage/saves/` folder and customized `config/`, then
launch **Ports → LivingVillage**. Replace launcher, binary, images, scripts and
documentation. Keep `config/audio-device.txt` if customized. Cover registration
runs automatically; a frontend game-list refresh may be needed for cached art.

## This release

- **Map → A tile → Y** toggles a bounded 5x5 conservation area. Natural
  harvesting and new plans are restricted; dropped supplies, existing farms,
  wildlife, weather and natural regrowth continue. No artificial recovery bonus.
- **Y → Village History → up/down** inspects recorded event-time evidence and
  locations. **A** follows an explicitly recorded contributing event when retained.
  Initial links cover weather hazards and construction/material deliveries;
  uninstrumented and older events identify missing evidence.
- **Y → Wildlife Trends** displays current species counts and up to 128 daily
  observations. Historical days without measurements stay blank.
- History CSV includes event IDs, contributors and evidence. Wildlife populations
  have a separate CSV through the existing controller export action.
- Version-9 saves migrate versions 1–8 with trained models and existing world
  state; no historical evidence or populations are invented during migration.
- Existing neural director, CPU workers, ecology overlays, controller support,
  sound/rumble, internal cover art and CPU/RAM meters remain available.

## Validation and limits

Native regression, controller/SDL smoke checks, save migration/continuation and
serial/four-thread equivalence are checked. GCC and SDK libc++ director/worker
tests exercise delayed training, Observer isolation, frozen models, safety gates,
chunk partition, exceptions and pool cleanup. Package checks verify AArch64 ELF,
matching binary/art and checksum. The 3,500-day native generational regression
completed with 20 living residents. Short desktop checks with 20 residents held
about 30 FPS in serial/four-thread modes; this does not establish device speedup. See [director](../docs/director.md) and
[architecture](../docs/architecture.md) for the implemented scope.

Conservation/event/population tests cover physical resource restrictions, loose
supply recovery, area/history limits, trace controls, persistence and v8 migration.
See [observable emergence](../docs/observable-emergence.md) for coverage and limits.

Director reward proxies are provisional; actual improvement in emergence has not
been established. Observer stays default. Device speedup, battery use, sound,
motor and new-screen readability require physical verification. Earlier versions
were confirmed to load and exit with Fn+Start on the user's handheld.

```sh
sha256sum -c packages/SHA256SUMS
```
