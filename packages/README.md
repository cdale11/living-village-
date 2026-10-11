# Ready-to-install ARM64 build

`living-village-knulli-arm64.tar.gz` contains the native executable and KNULLI
Ports launcher. It is built from source milestone **15accc0** with Zig 0.13.0,
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
architecture and glibc symbol baseline have been inspected. **Physical KNULLI
execution has not been verified.** The OS must provide its existing SDL2 runtime.
See the root [README](../README.md) and [device guide](../docs/knulli.md).
