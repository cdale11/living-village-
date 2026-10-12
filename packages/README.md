# Ready-to-install ARM64 build

`living-village-knulli-arm64.tar.gz` contains the native C++17 executable,
KNULLI Ports launcher, configuration, documentation and diagnostic script.
Gameplay is built from source milestone **2510994**, using Zig 0.13.0 for
Cortex-A53 / AArch64 Linux with a glibc 2.17 baseline.

Download the archive using GitHub's **Download raw file**, extract into
`/userdata/roms/ports/`, make `LivingVillage.sh` and
`livingvillage/bin/living-village` executable, then refresh EmulationStation's
game lists. Open **Ports → LivingVillage**. See the root [README](../README.md).

This release adds:

- Rabbits, deer, foxes and bee colonies with biological needs, inherited traits,
  pregnancy, growth, disease, mortality and individual online MLP learning.
- Soil moisture/nutrients, finite nectar, grazing, pollination, seed dispersal,
  canopy competition, forest recruitment, litter and plant mortality.
- Daily weather, snowmelt, groundwater, conservative runoff, floods, drought and fire.
- Terrain erosion, worn paths and building condition; villagers physically
  deliver materials and perform maintenance work.
- Ecology/weather and wildlife inspection, map effects, ecology/wildlife CSV exports.
- Version 7 saves with version 1–6 migration and preserved resident neural state.

Existing world creation/save management, households, adaptive work, society,
technology, audio and optional rumble remain. Fn+Start saves and exits from
all panels. The package also includes `scripts/diagnose_knulli.sh` for system,
CPU, graphics and input reports in `logs/`, using temporary worlds.

Desktop ecology tests, water accounting, biological/maintenance tests, migration,
maximum save budgets, neural learning, save continuation, controller/rendering,
first-boot/CLI smoke tests, sanitizer checks and 40 ten-day biome/layout surveys
pass. The 3,500-day regression ended with 20 residents, 26 arrivals, 16 deaths
and 2,210 repairs, replacing the founding generation. Routine maintenance uses
renewable wood; severe structural damage also requires stone. The SDK runtime passes ecology, world management and 300-day survival checks.
A short desktop 20-resident/wildlife rendering test measured 30.19 FPS at the
30 FPS cap; this is not a physical handheld result. The ARM64 ELF and glibc symbol
baseline have been inspected. The user confirmed the original game launches on
KNULLI; **this ecology build still requires physical validation**.

Verify the archive from the repository root:

```sh
sha256sum -c packages/SHA256SUMS
```

For an upgrade, back up the **whole saves folder**, then replace the executable
and optionally update documentation/scripts. Preserve customized `config/` and
all save files, nested worlds, backups, markers and exports. Versions 1–6 load
and upgrade to 7; older binaries cannot read newly upgraded saves.

Y → left/right reaches Ecology/weather and Wildlife after the sound page.
L1/R1 cycles animals on Wildlife. A inspects soil/plant state or building
condition. [Ecology documentation](../docs/ecology.md) describes rules and limits.
