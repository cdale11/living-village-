# RG34XX SP validation and performance

The initial ARM64 release was user-confirmed to launch on KNULLI. Later expansions
are desktop-tested and cross compiled. Physical validation of the expanded build
remains required; no handheld FPS or battery result is claimed.

## Packaged diagnostic reports

The archive includes an offline, read-only system inspection and temporary-world
measurement script. From the installed game directory:

```sh
./scripts/diagnose_knulli.sh system
./scripts/diagnose_knulli.sh cpu
./scripts/diagnose_knulli.sh graphics
./scripts/diagnose_knulli.sh input
```

Each run writes a separate report under `logs/`, prints its path and preserves
failure output. Reports include OS/architecture, available system memory, exposed
battery status, Linux input devices and configured SDL overrides. They do not
read player saves or print the full environment. `system` opens no display;
`cpu` advances 300 days with 20 starting residents; `graphics` measures 900 frames;
`input` allows 1800 frames of raw button/hat diagnostics. Fn+Start can end these
graphical runs early. Use `graphics`/`input` only after EmulationStation or the
current game has released the display. They use temporary worlds and never
load/write player saves. Platform overrides use the same trusted
`config/platform.env` as the launcher. No package installation or root filesystem
changes are performed. Send the report files when reporting device problems.

For a desktop/headless ecological survey:

```sh
make world-soak
# Optional steps per world (default 100 days):
./scripts/soak_worlds.sh ./build/living-village 240000
```

This surveys two seeds across all 20 biome/layout combinations, reports actual
survivors, construction, learning, elapsed time and peak process RSS, and fails
on execution errors. A collapsed difficult world is a measured outcome, not a
test failure or an exception. No player saves or display are accessed. CI runs
a shorter 10-day survey; long surveys are a separate development command.

The pre-ecology October 12, 2026 desktop survey completed all 40 runs at 240,000 steps each
(100 elapsed simulation days), with 15–19 living residents at the end and a
maximum reported process RSS of 2,832 KiB. These are two sampled seeds on the
desktop build, not a guarantee for every seed or handheld performance figures.

## Temporary performance world

After quitting other fullscreen games, launch from a session where the firmware
has released the display, or append these options to the Ports launcher:

```sh
cd /userdata/roms/ports/livingvillage
./bin/living-village --fullscreen --benchmark-villagers 20 --frames 900
```

This uses a fresh temporary 20-resident world, automatically enables `--profile`
and disables save loading/writing. It does not replace the player's village. Window focus loss alone does not pause
this explicit temporary diagnostic run; minimize/background events and ordinary
play still pause. Ensure the reported simulation-step count increases.
Thirty seconds at the 30 FPS cap is a useful first rendering/AI measurement.
For ordinary play with measurement, use `--profile` alone; it retains normal saves.
The launcher logs output to `logs/game.log`.

Every 300 frames and on exit, the log reports actual FPS, average frame work time,
recent 95th-percentile work time, simulation steps and peak process RSS. Frame work
includes simulation, rendering and other work before the frame-rate sleep. Recent
percentiles use a fixed 300-frame buffer. These are process measurements, not total
OS RAM. Battery percentage is recorded at start/end when the firmware exposes a
Battery power-supply capacity file; -1 means unavailable. Use longer sessions to
assess battery drain and note brightness, volume, Wi-Fi, charging and simulation
speed. Percentage differences alone are not calibrated energy measurements.

Headless CPU/memory check without opening the display:

```sh
./bin/living-village --headless --steps 720000 --benchmark-villagers 20
```

Headless speed does not establish graphical FPS or battery life.

## Functional device checks

1. Open Ports → LivingVillage; verify camera, selection, zoom and every controller
   menu. Fn+Start must save and return to EmulationStation from every panel.
2. Use X's six detail pages and Y's eight overview pages. Place a workshop using A;
   watch deliveries, crafting and technology effects. Verify histories export.
3. Change volume with A on Y's sound page; cycle through mute and higher levels.
   R1 toggles vibration. Original short PCM tones use SDL's OS audio backend;
   missing audio is logged and the game continues. Vibration requires an SDL
   controller/joystick driver exposing the motor; unsupported rumble is ignored.
4. Close/reopen the lid while villagers are moving. Focus loss, minimize and
   SDL background events save and pause; Start resumes. Firmware may suspend
   without an SDL event, so check actual behavior. Thirty-second autosaves still
   limit recent progress loss if no notification is delivered.
5. Resume upgraded saves, confirm neural learning continues, then save/exit and
   launch again. Preserve backups. Hard power-off/SIGKILL can lose progress since
   the last successful save; normal signal/exit paths save atomically.

KNULLI is the sole handheld distribution target. No system partition changes or
OS package installation are required. Inspect the actual SDL backend before
setting platform overrides; do not assume a desktop video driver.

Validate first-run world creation, all seed/name editor controls, biome/layout
choices, save switching, rename, snapshot and confirmed inactive deletion on
the handheld. Use synthetic worlds when checking deletion. Details are in
[world-management.md](world-management.md).

For the ecology build, also inspect Y's weather/wildlife pages, L1/R1 animal
cycling, tile biology and building condition. Observe repairs, runoff, grazing
and rain/snow/fire sprites. Benchmark with wildlife present; previous performance
measurements predate the richer ecology and do not establish its device speed.
