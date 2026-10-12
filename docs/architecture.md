# Architecture

`World` and all villagers/buildings are C++ data without SDL dependencies.
`world.cpp` contains resource discovery, utility decisions, action execution,
movement, needs, relationships and ecology. `buildings.cpp` contains building
recipes, capacity/effect queries, placement validation and shared need-driven
planning. `population.cpp` handles homes, households, births, arrivals, ageing
and deaths. `brain.cpp` implements the per-resident MLP and bounded SGD;
`learning.cpp` extracts observations and credits action outcomes. `saves.cpp`
records full state and migrates version 1/2/3/4/5/6 saves to version 7. Headless runs and tests use the same simulation as the graphical game.

`ecology.cpp` updates weather, hydrology, plant/soil biology, hazards and maintenance
every ten fixed ticks. `animals.cpp` updates bounded wildlife, reproduction and
personal neural action preferences on that cadence. Tile storage is a fixed-size
heap vector so world copies and parsing do not consume large stack frames.
Ecology uses its own persisted PRNG; legacy villager random streams and trained
models are preserved. Wildlife remains capped at 48, with 32 replay entries each.

`main.cpp` feeds fixed 0.1-second steps. Rendering targets 30 FPS. Speed 4×
advances four times as many steps. Stalls are bounded to 0.2 seconds and at most
eight updates per frame, avoiding unlimited catch-up. UI panels pause the
simulation. Simulation speed and autonomous planning preference are saved.

`input.cpp` reads mapped SDL controllers, raw joysticks and keyboard state,
including Fn+Start exit. `ui.cpp` controls camera, menu and inspection pages and
returns save/load/exit/plan/designation and learning-toggle requests. Building requests alter
simulation state in `main.cpp`; UI does not bypass material or work costs.
Neural models persist weights and one pending experience each, with a fixed 32-outcome replay buffer. Directional menus repeat at a bounded rate, while action buttons use edges.

`render.cpp` draws visible terrain, original sprites, buildings, construction
progress, designation borders, bitmap lettering and panels at logical 720×480.
No external art/font library is required. `sdl.cpp` loads the OS SDL2 ABI, so
ARM builds do not ship incompatible desktop video drivers. The library is not
loaded for headless commands. Virtual joystick functions are test-only.

The map is a 4096-tile array. BFS visits each cell once with bounded arrays.
Movement traverses at most four path edges per update. Memory is capped at 48
locations per resource per agent, history at six experiences per agent and 24
shared events. Plans/buildings cap at 64 and gathering areas at 16. Save reads
cap at 8 MiB; population caps at 20 living and 32 archived deceased villagers. Stable IDs
never reuse deceased identities. Sparse relationships cap at 32 entries and
construction contributor histories at 64. The default population begins at six;
3,500-day regression tests cover generational turnover.

Platform paths/optional SDL overrides stay in the Ports launcher. All handheld
writes stay on /userdata; no OS package installs occur. There is no game networking.

`occupations.cpp` records actual work and decaying specialization. `society.cpp`
handles physical social exchange, claim disputes, workshop technology and daily
statistics. Chronicles cap at 512 entries and daily statistics at 128.
`sound.cpp` generates original PCM; `feedback.cpp` queues bounded optional SDL
audio and input handles optional rumble. `performance.cpp` uses a fixed frame
sample buffer and read-only battery queries. Headless runs initialize none of
the SDL/audio paths.

`generation.cpp` generates explicit biome/geography settings, preserving the
original default algorithm. `slots.cpp` manages bounded named/legacy worlds,
stable IDs, active markers, rename/snapshot and confirmed inactive deletion.
Controller world/seed/name editors live in `ui.cpp`; they do not alter a running
world until creation/loading succeeds. Autosaving never writes an unconfirmed
first-run preview or a corrupt startup placeholder.

`port_art.cpp` handles frontend XML registration before SDL initialization. It
uses bounded structural scanning and verbatim edits of only our entry, backed-up
atomic metadata writes and a registration lock. Brain `evaluate()` tracks real
decision inference separately from side-effect-free inspector `predict()`;
training records actual weight delta. These diagnostic session fields do not
affect decisions, world RNG or serialized simulation state. Two overview pages
keep simulation active to permit live monitoring.

`sprites.cpp` builds original small SDL textures from
hand-authored pixel data once, and renders them without image codecs. `rumble.cpp`
provides bounded Linux evdev force feedback as a fallback to SDL. Runtime resource
meters sample process CPU/current RSS at one-second intervals and never enter
the save or simulation state. Artwork is installed inside the game folder.
