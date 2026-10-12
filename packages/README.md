# Ready-to-install ARM64 build

`living-village-knulli-arm64.tar.gz` contains the native executable and KNULLI
Ports launcher. It is built from source milestone **ffc48f2** with Zig 0.13.0,
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
the initial release runs on KNULLI. This update adds **online neural learning**:
every resident has an independent 37-input, 12-hidden-unit C++ MLP trained during
play from actual outcomes. Learned values influence valid action choices; weights
and pending experiences persist in saves. X's fifth page shows model effects;
Y's fourth page freezes/resumes training. No network or ML runtime is required.
KNULLI is the handheld target. **Fn+Start save-and-exit is retained.**
Neural adaptation, reward reversal, old-save migration, deterministic resumption,
controller/rendering, sanitizer checks and learning-enabled 3,500-day regressions
pass on desktop with GCC and the SDK's C++ runtime. The expanded ARM64 build
still needs physical confirmation. The OS provides its existing SDL2 runtime.
See the root [README](../README.md) and [device guide](../docs/knulli.md).

To upgrade an existing installation, replace only
`/userdata/roms/ports/livingvillage/bin/living-village` with the executable in
this archive and ensure it remains executable. Keep your saves and config.
Version 1, 2 and 3 saves load and upgrade automatically to version 4; older binaries
cannot read newly upgraded saves. A examines tiles/plans and gathering areas,
X opens the villager inspector (left/right for family/MLP/details), and Y opens
the overview (left/right for population/history/learning, A toggles settings).
Back up the whole saves folder before upgrading.
