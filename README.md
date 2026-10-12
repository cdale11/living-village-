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

The user confirmed the initial ARM64 release runs on KNULLI. This village-development update is cross compiled and tested on desktop, and still needs physical validation on the handheld. The Fn+Start shortcut remains covered by virtual-controller tests. See [device checks](docs/knulli.md).

## What you can play

- Explore a seeded 64×64 world with grass, trees, berry bushes, stone and ponds.
- Observe six named villagers gathering food, wood and stone, building, farming, eating, resting and socializing.
- Watch shared needs produce real construction plans, material deliveries and cooperative work.
- Plan shelters, food storage, stockpiles, farm plots and campfires; designate gathering areas.
- Inspect actual goals, utility scores, needs, skills, relationships and recent experiences.
- Pan and zoom; cycle villagers; view the event log.
- Pause through controller menus and choose 1×, 2× or 4× speed.
- Save, load and resume. Autosaves run every 30 real seconds.

This release implements village development (Milestone C) on top of survival, controller menus and persistent saves. Buildings consume delivered resources and work; farms renew food, shelters improve rest, and storage expands capacity. Seasonal/rain effects change resource growth. Births, families, trade, advanced occupations, audio and rumble remain future milestones. There is no offline progression.

To upgrade, replace only `livingvillage/bin/living-village` and keep saves/config. Version 1 saves load and upgrade automatically; version 2 saves preserve buildings, skills, relationships and player designations. Older binaries cannot read version 2 saves.

## Planning and inspecting

Move the camera reticle to a tile and press **A**. On empty grass, press A again to choose a building, then A to place the plan. Villagers gather and physically deliver the required materials before building. On a tree, bush or stone tile, A toggles a 5×5 gathering area; survival and competing village needs still affect their choices.

**X** inspects the selected villager; left/right on the D-pad cycles needs/reasons, skills/personality, and relationships/experiences. **L1/R1** switches villagers and follows them. **Y** opens the village overview; A there toggles autonomous new construction plans. Existing plans still receive work when automatic planning is disabled. Trees display stumps when depleted, and construction tiles show work progress.

## Handheld controls

| Input | Action |
| --- | --- |
| D-pad / left stick | Pan camera / navigate menu |
| A | Inspect tile / plan building or gathering area / confirm |
| B | Close panel / back |
| X | Inspect villager; D-pad left/right changes detail page |
| Y | Village overview / event log |
| L1 / R1 | Previous / next villager; follow camera |
| L2 / R2 | Zoom out / in |
| Start | Pause menu / resume |
| Fn + Start | Save and exit to EmulationStation |
| Select | Simulation speed menu |

SDL's mapped controller API is used when available. Unrecognized devices use joystick hats/axes and indices in `config/controller.cfg`. Fallback indices are configurable examples, not confirmed RG34XX mappings. Move the D-pad to leave camera following. [Mapping guide](docs/controls.md).

Fn+Start works from any panel, in either press order. SDL Guide+Start is supported, with a fallback to KNULLI's RG34XX-SP button profile (Fn 11, Start 10). `hotkey` and `hotkey_start` override this pair if your mapping differs. On update, replacing just `livingvillage/bin/living-village` enables the shortcut with an existing default config; preserve your saves and customized configuration.

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

The headless command advances 300 days and reports survivors, buildings, time and peak RSS, making no save writes. Normal play defaults to `./saves`; use `--save-dir PATH` to change it. `--new` starts over and replaces the current save on the next save, retaining the old one as backup. `--no-save` disables loading/writing for temporary sessions. See `--help` for diagnostics.

## ARM64 package build

On an x86 Linux development machine, download Zig **0.13.0** from <https://ziglang.org/download/>, extract it, then run:

```sh
ZIG=/path/to/zig ./scripts/build_arm64.sh
./scripts/package_knulli.sh
```

Output: `dist/living-village-knulli-arm64.tar.gz`. Targets Cortex-A53, AArch64 Linux and glibc 2.17+, linking the C++ library into the executable. It loads KNULLI's SDL2 runtime. Cache stays in `build-arm64/`. GitHub Actions also builds and uploads a package artifact.

## Persistence and verification

A versioned text save with checksum records random generator, terrain/resources, time, buildings/progress/invested materials, crops/fuel, inventories, skills, relationships, experiences, goals/paths/memories, designations, simulation settings and events. Writes use `fsync`, same-directory rename, directory flush and a validated `.bak` previous save. Corrupt saves fall back to backup. SIGINT/SIGTERM/SIGHUP request clean save/exit; focus/background events save and pause. SIGKILL or power loss can lose progress since the latest autosave.

Verified on x86 Linux: C++ build, SDL dummy/software rendering, virtual controller/Fn+Start mapping, deterministic save continuation, corruption recovery, old-save migration, resource budgets, physical cooperative delivery, material-gated construction, farm/shelter/storage effects, seasons, gathering designations, 20 active villagers and 300 days with six survivors. A desktop headless benchmark ran 720,000 steps in about 2.6 seconds with about 5.6 MiB peak RSS. These are **not handheld performance results**.

[Architecture](docs/architecture.md) · [Simulation rules](docs/simulation-design.md) · [KNULLI checks](docs/knulli.md)

Original code and generated graphics: MIT. SDL2 headers: zlib, notice in `vendor/SDL2/LICENSE.txt`. The ARM64 binary includes LLVM libc++/ABI (Apache 2.0 with LLVM exceptions) and Zig runtime (MIT); notices are shipped in the package's `licenses/` directory.
