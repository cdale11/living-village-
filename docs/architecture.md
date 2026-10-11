# Architecture

`World` and its villagers are C++ data with no SDL dependency. `world.cpp` contains bounded pathfinding, resource discovery, utility decisions, movement, need changes and ecology. `saves.cpp` serializes the complete state including the PRNG. The headless command and simulation tests use this same simulation.

`main.cpp` owns SDL and feeds simulation steps of 0.1 seconds. Rendering is capped at approximately 30 FPS. Speed 4× advances four times as many simulation steps. Frame stalls are bounded to 0.2 seconds and eight simulation updates per frame, preventing runaway catch-up. Paused panels advance no simulation time.

`input.cpp` reads mapped SDL controllers, raw joysticks and keyboard state. `ui.cpp` translates presses into camera changes, menus and save/load/exit requests. Edge detection prevents held buttons from repeatedly activating actions; menu direction input has a repeat interval. One joystick is selected; reconnection is handled.

`render.cpp` draws visible terrain, original rectangle-based sprites, bitmap text and controller panels at logical 720×480 without external fonts or textures. `sdl.cpp` loads SDL2 dynamically so the ARM cross build can use upstream headers without shipping a desktop SDL build. Missing symbols produce a log error. Stable SDL2 functions are used by the game; virtual joystick functions are used only by desktop tests.

The map is a 4096-tile array. BFS visits each tile at most once using bounded arrays. Resource memory is limited to 48 locations per villager and history to 24 events. Save reads are capped at 2 MiB. Saves permit up to 20 villagers, but this release spawns six and has no population growth. There is no game networking.

Platform initialization and paths stay in the launcher. No OS package installs are performed. ROCKNIX still needs validation and its own installation-path adapter.
