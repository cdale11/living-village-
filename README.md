# Living Village

A C++17 / SDL2 village prototype for the Anbernic RG34XX SP running KNULLI. The game runs offline as a native ARM64 executable with no Python dependency.

## Install on KNULLI — no compilation on the handheld

1. Download [`packages/living-village-knulli-arm64.tar.gz`](packages/living-village-knulli-arm64.tar.gz). On GitHub's file page, use **Download raw file**.
2. Extract into `/userdata/roms/ports/`, preserving this layout:

   ```text
   /userdata/roms/ports/
   ├── LivingVillage.sh
   └── livingvillage/
       ├── bin/living-village
       ├── config/controller.cfg
       ├── saves/
       └── logs/
   ```

3. Using SSH, run:

   ```sh
   chmod +x /userdata/roms/ports/LivingVillage.sh
   chmod +x /userdata/roms/ports/livingvillage/bin/living-village
   ```

4. Refresh game lists in EmulationStation (or restart it). Open **Ports → LivingVillage** from KNULLI's game browser. It opens fullscreen. Use **Start → Save and exit** to return to EmulationStation.

To copy over SSH, replace `DEVICE_IP` with the handheld address:

```sh
scp packages/living-village-knulli-arm64.tar.gz root@DEVICE_IP:/userdata/
ssh root@DEVICE_IP 'tar -xzf /userdata/living-village-knulli-arm64.tar.gz -C /userdata/roms/ports && chmod +x /userdata/roms/ports/LivingVillage.sh /userdata/roms/ports/livingvillage/bin/living-village'
```

All files and saves stay on `/userdata`; installation does not modify the system partition. On upgrades, replace the launcher and `bin/`, preserving your `saves/` and customized `config/`. The archive contains empty save/log directories, with no save file.

**Device validation is still required.** The ARM64 ELF has been cross compiled and inspected, but has not been executed on an RG34XX SP. SDL backend, controller mapping, lid behavior and frame rate require physical testing. See [device checks](docs/knulli.md).

## What you can play

- Explore a seeded 64×64 world with grass, trees, berry bushes, stone and ponds.
- Observe six named villagers independently exploring, gathering berries, eating and resting.
- Inspect actual goals, needs, resource memories and decision reasons.
- Pan and zoom; cycle villagers; view the event log.
- Pause through controller menus and choose 1×, 2× or 4× speed.
- Save, load and resume. Autosaves run every 30 real seconds.

This is the first playable C++ release, with basic survival added to Milestone A. Construction, farming, wood/stone gathering, relationships, weather effects, births, audio and rumble remain future milestones. The camp provides shared food access and improved rest; berries regenerate. There is no offline progression.

## Handheld controls

| Input | Action |
| --- | --- |
| D-pad / left stick | Pan camera / navigate menu |
| A | Select nearest villager / confirm |
| B | Close panel / back |
| X | Inspect selected villager |
| Y | Village overview / event log |
| L1 / R1 | Previous / next villager; follow camera |
| L2 / R2 | Zoom out / in |
| Start | Pause menu / resume |
| Select | Simulation speed menu |

SDL's mapped controller API is used when available. Unrecognized devices use joystick hats/axes and indices in `config/controller.cfg`. Fallback indices are configurable examples, not confirmed RG34XX mappings. Move the D-pad to leave camera following. [Mapping guide](docs/controls.md).

Desktop: arrows/WASD pan, Enter confirms, Escape backs out, X/Y inspect/overview, Q/E cycle villagers, -/+ zoom, Space opens pause and Tab opens speed. Quit via the menu or window close.

## Desktop build and tests

Requires C++17 and Make. SDL2 **runtime** is needed for graphics/input tests; development headers are vendored under their zlib license. SDL_ttf and SDL_image are unnecessary.

```sh
make -j2
./scripts/run_desktop.sh
make test                        # no display or SDL runtime required
make input-test                  # SDL 2.0.14+ virtual controller test
make render-test                 # dummy display, no desktop required
./build/living-village --headless --steps 720000
```

The headless command advances 300 days and reports time and peak RSS, making no save writes. Normal play defaults to `./saves`; use `--save-dir PATH` to change it. `--new` starts over and replaces the current save on the next save, retaining the old one as backup. `--no-save` disables loading/writing for temporary sessions. See `--help` for diagnostics.

## ARM64 package build

On an x86 Linux development machine, download Zig **0.13.0** from <https://ziglang.org/download/>, extract it, then run:

```sh
ZIG=/path/to/zig ./scripts/build_arm64.sh
./scripts/package_knulli.sh
```

Output: `dist/living-village-knulli-arm64.tar.gz`. Targets Cortex-A53, AArch64 Linux and glibc 2.17+, linking the C++ library into the executable. It loads KNULLI's SDL2 runtime. Cache stays in `build-arm64/`. GitHub Actions also builds and uploads a package artifact.

## Persistence and verification

A versioned text save with checksum records random generator, terrain/resources, time, inventories, goals, paths, known resources and events. Writes use `fsync`, same-directory rename, directory flush and a validated `.bak` previous save. Corrupt saves fall back to backup. SIGINT/SIGTERM/SIGHUP request clean save/exit; focus/background events save and pause. SIGKILL or power loss can lose progress since the latest autosave.

Verified on x86 Linux: C++ build, SDL dummy/software rendering, virtual controller mapping, deterministic save continuation, corruption recovery, food accounting, obstacle routing, shortage collapse and 300 days with six survivors. A desktop headless benchmark ran 720,000 steps in about 3.7 seconds with about 1.7 MiB peak RSS. These are **not handheld performance results**.

[Architecture](docs/architecture.md) · [Simulation rules](docs/simulation-design.md) · [KNULLI checks](docs/knulli.md)

Original code and generated graphics: MIT. SDL2 headers: zlib, notice in `vendor/SDL2/LICENSE.txt`. The ARM64 binary includes LLVM libc++/ABI (Apache 2.0 with LLVM exceptions) and Zig runtime (MIT); notices are shipped in the package's `licenses/` directory.
