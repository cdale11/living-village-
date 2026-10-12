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

The archive includes original Ports cover art. Registration runs on launch; to install its game-list metadata immediately, run `/userdata/roms/ports/livingvillage/scripts/register_art.sh`, then **Update Games Lists** in EmulationStation. See [artwork setup](docs/port-art.md).

The user confirmed the initial ARM64 release runs on KNULLI. The user also confirmed the ecology game loads and Fn+Start exits correctly. This customization/graphics/diagnostics update is cross compiled and tested on desktop; its new screens, artwork and performance still need physical validation. The Fn+Start shortcut remains covered by virtual-controller tests. See [device checks](docs/knulli.md).

The archive also includes `livingvillage/scripts/diagnose_knulli.sh`. Run it with
`system`, `cpu`, `graphics` or `input` to create an offline diagnostic report in
`logs/` using temporary worlds. See [device validation](docs/device-validation.md)
for display-session requirements and the 40-world headless survey command.

## What you can play

- Create seeded 64×64 worlds with eight biomes, twelve geographical layouts and three starting-food bundles.
- Manage up to 32 named worlds: load, rename, snapshot and confirm deletion of old saves.
- Observe six named villagers gathering food, wood and stone, building, farming, eating, resting and socializing.
- Watch shared needs produce real construction plans, material deliveries and cooperative work.
- Observe rabbits, deer, foxes and bee colonies with inherited traits, biological needs, reproduction and individual learned preferences.
- Watch soil water, runoff, snowmelt, forest succession, grazing, pollination, drought, floods and wildfire affect the settlement.
- Maintain weathered buildings through physical material delivery and repair work; walking wears visible paths.
- Plan shelters, food storage, stockpiles, farm plots and campfires; designate gathering areas.
- Inspect actual goals, utility scores, needs, skills, relationships and recent experiences.
- Watch every villager train a personal C++ MLP from action outcomes with bounded experience replay and controlled exploration while the game runs. Inspect learned score effects or pause training through the controller.
- Pan and zoom; cycle villagers; view the event log.
- Pause through controller menus and choose 1×, 2× or 4× speed.
- Save, load and resume. Autosaves run every 30 real seconds.

This release adds online neural learning to autonomous survival, village development, households, controller menus and persistent saves. Each resident's MLP learns from actual action outcomes and changes candidate scores within the simulation's survival and validity rules. Buildings consume delivered resources and work; farms renew food, shelters improve rest, and storage expands capacity. Seasonal/rain effects change resource growth. Households form from mutual relationships; children inherit traits and receive physically delivered meals. Conditional immigration, ageing and deaths allow generational turnover, with up to 20 living villagers. Residents acquire and change work specializations, share meals, barter surplus food/materials, dispute claims and reconcile. Workshops unlock two supplied technology upgrades. Original sound effects and optional SDL rumble are configurable. There is no offline progression.

To upgrade, replace only `livingvillage/bin/living-village` and keep saves/config. Version 1–6 saves upgrade automatically to version 7, preserving existing state. Older binaries cannot read version 7 saves. Keep a separate backup before upgrading.

See [ecology and wildlife](docs/ecology.md) for biological rules, hazard mechanics,
maintenance and model limits. Y's last two pages show Ecology/weather and Wildlife;
L1/R1 cycles individual animals on Wildlife. A tile inspection shows soil/plant
state or structure condition. No offline progression or internet is required.

## Planning and inspecting

Move the camera reticle to a tile and press **A**. On empty grass, press A again to choose one of six buildings, then A to place the plan. Villagers gather and physically deliver the required materials before building. On a tree, bush or stone tile, A toggles a 5×5 gathering area; survival and competing village needs still affect their choices.

**X** inspects the selected villager; left/right on the D-pad cycles needs/reasons, skills/personality, relationships/experiences, household/family, neural learning, and replay/work practice. **L1/R1** switches villagers and follows them. **Y** opens the village overview; A there toggles autonomous new construction plans. Left/right cycles village, population and event pages; A on the population page toggles births/arrivals. The fourth overview page shows live AI diagnostics and controls online neural training; the ninth shows individual agent diagnostics. These two pages keep simulation running; Start pauses. the fifth exports history/statistics; the sixth controls volume (A) and vibration (R1). Existing plans still receive work when automatic planning is disabled. Start opens world creation and Manage saved worlds. First launch waits for name, seed, biome, layout and starting-food selection, with an actual seeded terrain preview. Existing current/previous saves appear alongside named worlds. Trees display stumps when depleted, and construction tiles show work progress.

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
make feedback-test               # PCM and dummy audio/rumble fallback
make game-smoke                  # real AI steps + save-safe temporary benchmark
make worlds-smoke                # initial creator, new slots, resume and corrupt-startup protection
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/living-village --benchmark-villagers 20 --frames 90
./build/living-village --headless --steps 720000
```

The headless command advances 300 days and reports survivors, buildings, time and peak RSS, making no save writes. Normal play defaults to `./saves`; use `--save-dir PATH` to change it. `--new` creates a separate named save and preserves earlier worlds; `--seed`, `--biome`, `--layout` and `--world-name` configure it. `--no-save` disables loading/writing for temporary sessions. See `--help` for diagnostics.

## ARM64 package build

On an x86 Linux development machine, download Zig **0.13.0** from <https://ziglang.org/download/>, extract it, then run:

```sh
ZIG=/path/to/zig ./scripts/build_arm64.sh
./scripts/package_knulli.sh
```

Output: `dist/living-village-knulli-arm64.tar.gz`. Targets Cortex-A53, AArch64 Linux and glibc 2.17+, linking the C++ library into the executable. It loads KNULLI's SDL2 runtime. Cache stays in `build-arm64/`. GitHub Actions also builds and uploads a package artifact.

## Persistence and verification

A versioned text save with checksum records world name/generation settings, neural weights and pending learning experiences, random generator, terrain/resources, time, buildings/progress/invested materials, crops/fuel, inventories, skills, relationships, experiences, goals/paths/memories, designations, simulation settings and events. Writes use `fsync`, same-directory rename, directory flush and a validated `.bak` previous save. Corrupt saves fall back to backup. SIGINT/SIGTERM/SIGHUP request clean save/exit; focus/background events save and pause. SIGKILL or power loss can lose progress since the latest autosave.

Verified on x86 Linux: GCC and LLVM libc++ simulation suites, SDL dummy rendering, virtual controller/Fn+Start mapping, old-save migration, deterministic continuation, corruption recovery, physical resource delivery and child care, construction, households, births, immigration, identity safety and bounded history. A 3,500-day regression finishes with 20 survivors after births and deaths. Neural regressions verify learned action changes, reward reversal, bounded replay, live updates, training pause/resume and exact learning continuation. Society tests cover earned occupations, physical barter/sharing, conflicts/reconciliation, workshop costs/effects, and escaped CSV exports. Original PCM, dummy SDL audio and temporary save-safe graphical benchmarks are tested. Address/undefined-behavior sanitizer checks pass. The ARM64 executable is cross compiled; these are **not handheld performance results**.

[Live AI diagnostics](docs/ai-diagnostics.md) · [Ports cover art](docs/port-art.md) · [World creation/save management](docs/world-management.md) · [Priority status](docs/priorities.md) · [Society and technology](docs/society.md) · [Device measurement](docs/device-validation.md) · [Neural learning](docs/neural-learning.md) · [Population rules](docs/population.md) · [Architecture](docs/architecture.md) · [Simulation rules](docs/simulation-design.md) · [KNULLI checks](docs/knulli.md)

Original code, generated graphics and procedural audio: MIT. SDL2 headers: zlib, notice in `vendor/SDL2/LICENSE.txt`. The ARM64 binary includes LLVM libc++/ABI (Apache 2.0 with LLVM exceptions) and Zig runtime (MIT); notices are shipped in the package's `licenses/` directory.
