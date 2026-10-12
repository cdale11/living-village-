# Ready-to-install ARM64 build

`living-village-knulli-arm64.tar.gz` contains the native executable and KNULLI
Ports launcher. Gameplay is built from source milestone **12c5d5e**, with
diagnostic scripts/documentation from **12340f7**, using Zig 0.13.0,
targeting Cortex-A53, AArch64 Linux, glibc 2.17+. All gameplay code is C++17.

Download the archive using GitHub's **Download raw file**, then extract into
`/userdata/roms/ports/`, make `LivingVillage.sh` and
`livingvillage/bin/living-village` executable, and refresh EmulationStation's
game lists. Launch **Ports → LivingVillage**.

From the repository root, verify archive integrity with:

```sh
sha256sum -c packages/SHA256SUMS
```

Desktop simulation and SDL input/rendering tests pass. The binary's ARM64
architecture and glibc symbol baseline have been inspected. The user confirmed
the initial release runs on KNULLI. This update adds **world creation and save management**:
- first-run/controller naming and full unsigned 32-bit seed editing;
- four real biomes and five geographical layouts, including island shore arrivals;
- up to 32 independent named worlds, last-world resume and legacy slot access;
- load, rename, snapshot copies and confirmed deletion of inactive saves;
- backup-only recovery, corrupt-startup protection and retained exports;
- persistent generation/name settings with version 1–5 migration.

All previous adaptive-learning, society, economy, workshop, audio/rumble and
performance features remain. **Fn+Start save-and-exit works from management
and editor panels.** Controller/rendering, all 20 generation combinations,
file-management/migration tests, sanitizer checks, first-boot/CLI smoke tests
and the 3,500-day simulation regressions pass on desktop with GCC and the SDK
runtime. The ARM64 binary is cross compiled; this expanded build still needs
physical button/display/performance validation on the handheld.
See the root [README](../README.md) and [device guide](../docs/knulli.md).

The archive now includes `livingvillage/scripts/diagnose_knulli.sh` for system,
CPU, graphics and input reports in `logs/`. These measurements use temporary
worlds and leave player saves untouched. All 40 two-seed biome/layout desktop
surveys completed 100 days with 15–19 survivors; this is not a device benchmark.
Copy the new `scripts/` directory as well when upgrading just the executable.

To upgrade an existing installation, replace only
`/userdata/roms/ports/livingvillage/bin/living-village` with the executable in
this archive and ensure it remains executable. Keep your saves and config.
Version 1–5 saves load and upgrade automatically to version 6; older binaries
cannot read newly upgraded saves. A examines tiles/plans and gathering areas,
X opens the villager inspector (left/right for family/MLP/details), and Y opens
the overview (left/right for population/history/learning/exports/sound; A toggles
settings or exports CSV; R1 on the sound page toggles rumble).
Back up the whole saves folder before upgrading. Keep all its nested worlds,
backups, active/counter markers and exports. Start → New village selects
name/seed/biome/layout; Start → Manage saved worlds loads, renames, snapshots
and confirms inactive deletion. The first run waits for world configuration.
