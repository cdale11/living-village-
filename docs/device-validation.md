# RG34XX SP validation and performance

The initial ARM64 release was user-confirmed to launch on KNULLI. Later expansions
are desktop-tested and cross compiled. Physical validation of the expanded build
remains required; no handheld FPS or battery result is claimed.

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
2. Use X's six detail pages and Y's six overview pages. Place a workshop using A;
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
