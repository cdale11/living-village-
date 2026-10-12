# Architecture

`World` and all villagers/buildings are C++ data without SDL dependencies.
`world.cpp` contains resource discovery, utility decisions, action execution,
movement, needs, relationships and ecology. `buildings.cpp` contains building
recipes, capacity/effect queries, placement validation and shared need-driven
planning. `saves.cpp` records full state and migrates version 1 saves to version
2. Headless runs and tests use the same simulation as the graphical game.

`main.cpp` feeds fixed 0.1-second steps. Rendering targets 30 FPS. Speed 4×
advances four times as many steps. Stalls are bounded to 0.2 seconds and at most
eight updates per frame, avoiding unlimited catch-up. UI panels pause the
simulation. Simulation speed and autonomous planning preference are saved.

`input.cpp` reads mapped SDL controllers, raw joysticks and keyboard state,
including Fn+Start exit. `ui.cpp` controls camera, menu and inspection pages and
returns save/load/exit/plan/designation requests. Building requests alter
simulation state in `main.cpp`; UI does not bypass material or work costs.
Directional menus repeat at a bounded rate, while action buttons use edges.

`render.cpp` draws visible terrain, original sprites, buildings, construction
progress, designation borders, bitmap lettering and panels at logical 720×480.
No external art/font library is required. `sdl.cpp` loads the OS SDL2 ABI, so
ARM builds do not ship incompatible desktop video drivers. The library is not
loaded for headless commands. Virtual joystick functions are test-only.

The map is a 4096-tile array. BFS visits each cell once with bounded arrays.
Movement traverses at most four path edges per update. Memory is capped at 48
locations per resource per agent, history at six experiences per agent and 24
shared events. Plans/buildings cap at 64 and gathering areas at 16. Save reads
cap at 2 MiB; population data caps at 20. The default population remains six;
20-agent regression scenarios verify initial scalability without adding births.

Platform paths/optional SDL overrides stay in the Ports launcher. All handheld
writes stay on /userdata; no OS package installs occur. ROCKNIX still needs a
validated platform adapter. There is no game networking.
