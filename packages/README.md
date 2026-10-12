# Ready-to-install ARM64 build

`living-village-knulli-arm64.tar.gz` contains the native executable and KNULLI
Ports launcher. It is built from source milestone **518409e** with Zig 0.13.0,
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
the initial release runs on KNULLI. This update expands **village life and adaptive learning**:
- bounded MLP experience replay and healthy exploration;
- actual-work specialization and changing occupations;
- nearby meal sharing, physical barter, claim disputes and reconciliation;
- supplied workshop crafting for improved tools and irrigation;
- bounded chronicles/daily statistics with controller CSV exports;
- original procedural audio, volume controls and optional SDL vibration;
- seasonal/animated visuals, night lighting and readable work/need indicators;
- save-safe temporary 6–20-agent graphical benchmarks and FPS/work/RSS/battery logs.

The game remains entirely C++, offline, and launched through KNULLI Ports.
**Fn+Start save-and-exit is retained.** Simulation/neural/society tests,
3,500-day regressions, old-save migrations, dummy rendering/audio, controller
input, sanitizer checks and actual-step benchmark smoke tests pass on desktop.
The initial ARM64 release was user-confirmed working. This expanded build still
needs physical FPS, audio/motor, battery and lid validation on the handheld.
See the root [README](../README.md) and [device guide](../docs/knulli.md).

To upgrade an existing installation, replace only
`/userdata/roms/ports/livingvillage/bin/living-village` with the executable in
this archive and ensure it remains executable. Keep your saves and config.
Version 1–4 saves load and upgrade automatically to version 5; older binaries
cannot read newly upgraded saves. A examines tiles/plans and gathering areas,
X opens the villager inspector (left/right for family/MLP/details), and Y opens
the overview (left/right for population/history/learning/exports/sound; A toggles
settings or exports CSV; R1 on the sound page toggles rumble).
Back up the whole saves folder before upgrading.
