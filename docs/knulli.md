# KNULLI Ports integration

Install `LivingVillage.sh` in `/userdata/roms/ports`, with `livingvillage/` beside it. Refresh game lists or restart EmulationStation. **Ports → LivingVillage** is the entry in the main game browser; no EmulationStation/system settings are patched.

The script derives its install location, enters `livingvillage/`, creates save/log folders, optionally reads `config/platform.env` and executes the binary fullscreen. One previous log is rotated to `logs/game.previous.log`. Saves stay in `/userdata/roms/ports/livingvillage/saves`. Exit returns control to EmulationStation.

## Physical device checks still required

The user confirmed the initial ARM64 package runs on KNULLI. The updated
village-development milestone and Fn+Start shortcut are tested on desktop;
this expanded build still requires physical device confirmation.

Inspect runtime facts over SSH:

```sh
uname -m
getconf GNU_LIBC_VERSION
ls -l /lib/ld-linux-aarch64.so.1
ls /usr/lib/libSDL2* /lib/libSDL2* 2>/dev/null
```

Launch from Ports, then read `livingvillage/logs/game.log`. Startup reports selected/available SDL video backends, renderer and controller name/GUID/mapping mode. Validate movement, every button, zoom, speed, pause, save/load, Fn+Start exit and lid reopening. Check A tile/building plans and gathering areas, X inspector detail pages, and Y autonomous-planning toggle. Keep existing saves; they migrate automatically to version 2.

If video initialization fails, use the error/backend list to choose an override. Do not assume X11. **Only if KMSDRM is available and appropriate for this firmware**, put this in `config/platform.env`:

```sh
export SDL_VIDEODRIVER=kmsdrm
```

This is an optional trusted local shell configuration. SDL otherwise inherits the OS launch environment. The game tries accelerated rendering, then software. Do not start a second fullscreen game over SSH while EmulationStation or an emulator owns the display.

For controller diagnostics, pass `--diagnose-input` to the launcher in an appropriate display session. Desktop virtual-controller tests do not confirm this device's physical button indices.

The ARM64 binary needs `/lib/ld-linux-aarch64.so.1`, glibc 2.17+ and SDL2 named `libSDL2-2.0.so.0` or `libSDL2.so` in the library search path. libc++ and C++ ABI support are linked into the executable. If the OS library lives elsewhere, export a scoped `LD_LIBRARY_PATH` in `platform.env`. Do not copy desktop SDL libraries onto the device.

## Performance and saves

Headless benchmark, without taking over the display:

```sh
cd /userdata/roms/ports/livingvillage
./bin/living-village --headless --steps 720000
```

Graphical frame times and memory must be measured with the actual OS backend. The 30 FPS cap is a target; desktop measurements do not establish handheld performance.

Autosaves run every 30 real seconds, and on menu save, focus/background interruption and clean signal exit. Suspend may freeze the process without notification; autosave limits recent progress loss if firmware sends none. Confirm lid behavior physically. Hard power-off/SIGKILL can lose progress since the latest save.

ROCKNIX has not been tested. Its paths, runtime and Ports/PortMaster conventions need an adapter and validation.
