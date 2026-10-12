# Ready-to-install ARM64 build

`living-village-knulli-arm64.tar.gz` contains the native C++17 executable,
KNULLI Ports launcher, original cover art, controller configuration,
documentation and diagnostic/cover-registration scripts.
Gameplay is built from source milestone **f36d6c5**, using Zig 0.13.0 for
Cortex-A53 / AArch64 Linux with a glibc 2.17 baseline.

Download using GitHub's **Download raw file**, extract into
`/userdata/roms/ports/`, then refresh EmulationStation's game list and launch
**Ports → LivingVillage**. Keep the whole saves folder and customized config
when upgrading. Existing ecology saves retain their learned models and state.

This update adds:

- **8 biomes:** temperate, woodland, meadow, arid, taiga, rainforest, wetland,
  savanna, with real vegetation, soil, temperature and rainfall differences.
- **12 layouts:** ponds, river, island, coast, highlands, lakes, peninsula,
  archipelago, delta, canyon, valley and plains.
- Three starting food budgets, seeded terrain preview and profile descriptions
  in the controller-operated creator: **Start → New village**.
- Improved shoreline, tree, cottage and character details, panel framing,
  biome palettes and a compact world minimap.
- Live **Y → Neural learning** and **Agent diagnostics** pages showing real
  candidate-value predictions, SGD steps, steps that changed weights, outcomes,
  replay, reward fit and training freeze/resume. **Start pauses** these pages.
- Original 600x800 Ports cover and a native metadata registrar that preserves
  other entries and backs up `gamelist.xml` before atomic replacement.

The launcher registers the artwork automatically. To register immediately
without starting the game:

```sh
/userdata/roms/ports/livingvillage/scripts/register_art.sh
```

Then use **Update Games Lists** in EmulationStation. Themes need an image-enabled
view. See [artwork registration](../docs/port-art.md) for frontend caching and
[AI diagnostics](../docs/ai-diagnostics.md) for metric interpretation.

Verified on desktop Linux:

- All simulation, development, population, learning, adaptation, society,
  ecology, world-management, controller, rendering, audio and launcher tests.
- 192 seed/biome/layout cases over ten days each: 6–7 villagers surviving.
- 3,500-day generational regression: 20 residents, 26 arrivals, 16 deaths,
  2,210 repairs, and replacement of the founding generation.
- New inference/SGD/weight-change telemetry, frozen inference, session reset,
  controller pause, metadata preservation/idempotency/backup/error handling.
- GCC and SDK libc++ coverage for telemetry, world management and registration;
  AddressSanitizer/UBSan checks for telemetry and artwork registration.
- Short software-rendered desktop benchmark: 20 residents, 30.19 FPS at the
  30 FPS cap, 4.62 ms mean frame work, 5.91 ms recent p95, 8,928 KiB peak RSS.
  These are not handheld performance measurements.
- ARM64 ELF/glibc baseline, packaged binary/art matching and archive checksum.

The user confirmed the earlier ecology release loads and Fn+Start exits on
KNULLI. **This new update's creator, visuals, live diagnostics, cover/theme
integration and handheld performance still need physical validation.**

Verify from the repository root:

```sh
sha256sum -c packages/SHA256SUMS
```

Version 7 saves remain supported, including versions 1–6 migration. Expanded
biome/layout saves require this build; earlier builds may reject those profiles.
No Python, model download, internet or extra image/XML library is required on
KNULLI. All installed game data and metadata remain under `/userdata`.
