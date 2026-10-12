# Ready-to-install ARM64 build

`living-village-knulli-arm64.tar.gz` contains the native executable and KNULLI
Ports launcher. It is built from source milestone **6c797ae** with Zig 0.13.0,
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
the initial release runs on KNULLI. This update adds **Fn+Start to save and exit**;
the shortcut passes virtual SDL controller tests and still needs physical
confirmation. The OS provides its existing SDL2 runtime.
See the root [README](../README.md) and [device guide](../docs/knulli.md).

To upgrade an existing installation, replace only
`/userdata/roms/ports/livingvillage/bin/living-village` with the executable in
this archive and ensure it remains executable. Keep your saves and config.
