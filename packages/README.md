# Ready-to-install ARM64 build

`living-village-knulli-arm64.tar.gz` contains the native C++17 game, Ports launcher,
internal cover art, configuration, documentation and diagnostics. Built from
source milestone **5bfec6a** using Zig 0.13.0 for Cortex-A53 / AArch64 Linux with
a glibc 2.17 baseline. Gameplay requires no Python, model downloads or internet.

Download with GitHub's **Download raw file**, extract into `/userdata/roms/ports/`,
preserving the whole `livingvillage/saves/` folder and customized `config/`, then
launch **Ports → LivingVillage**. Replace launcher, binary, images, scripts and
documentation. Keep `config/audio-device.txt` if customized. Cover registration
runs automatically; a frontend game-list refresh may be needed for cached art.

## This release

- Offline neural director, saved model/replay/pending evaluations, and
  **Off / Observer / Adaptive** modes through **Y → Neural Director → A**.
  Observer is the default. Adaptive applies bounded weather opportunities and
  learns from three-day outcome associations. Diagnostics show proposals,
  interventions, predicted/observed change, training and CPU thread count.
- Persistent CPU pool detects cores, using four total threads on H700 for
  independent tile hydrology. Ordered reduction preserves deterministic state.
  `--threads 1` selects serial execution; workers sleep between batches.
- **Y → Ecology / Weather → A** cycles actual soil-water, vegetation and traffic
  overlays, then returns to the live map. Repeat to return to the normal map.
- Version-8 atomic checksummed saves migrate versions 1–7, including trained
  residents and existing wildlife. Older binaries cannot read version 8.
- Existing population setup, wildlife following, original sprites/ambient audio,
  sound-output tests, SDL/Linux rumble, cover integration and CPU/RAM meters remain.

## Validation and limits

Native regression, controller/SDL smoke checks, save migration/continuation and
serial/four-thread equivalence are checked. GCC and SDK libc++ director/worker
tests exercise delayed training, Observer isolation, frozen models, safety gates,
chunk partition, exceptions and pool cleanup. Package checks verify AArch64 ELF,
matching binary/art and checksum. The 3,500-day native generational regression
completed with 20 living residents. Short desktop checks with 20 residents held
about 30 FPS in serial/four-thread modes; this does not establish device speedup. See [director](../docs/director.md) and
[architecture](../docs/architecture.md) for the implemented scope.

Director reward proxies are provisional; actual improvement in emergence has not
been established. Observer stays default. Device speedup, battery use, sound,
motor and new-screen readability require physical verification. Earlier versions
were confirmed to load and exit with Fn+Start on the user's handheld.

```sh
sha256sum -c packages/SHA256SUMS
```
