# Living Village — Milestone A

A playable top-down prototype with a seeded 64×64 terrain map, six autonomous wandering villagers, camera movement, zoom and pause. The display-independent simulation runs at a fixed 10 Hz; rendering targets 30 FPS.

## Desktop

Requires Python 3.10+ and Pygame 2:

```sh
PYTHONPATH=src python3 -m living_village.main
```

Headless smoke test (no Pygame needed):

```sh
PYTHONPATH=src python3 -m unittest discover -s tests -v
```

## KNULLI

Copy this repository's contents to `/userdata/roms/ports/livingvillage/`. Copy `packaging/LivingVillage.sh` to `/userdata/roms/ports/LivingVillage.sh` and make it executable. Refresh Ports in EmulationStation. Logs go to `livingvillage/logs/game.log`. The launcher uses the OS Python and Pygame, without changing the system partition.

SDL video backend and controller button indices must be verified on the RG34XX SP. This project has not yet run on that device.

## Controls

D-pad or left stick pans. Start pauses. Shoulder buttons zoom. Keyboard arrows/WASD pan, Enter/Space pauses, +/- zoom, Escape exits.

## Scope

This first milestone implements wandering only. Hunger, resources, construction, saving, inspection and remapping are planned for later milestones.
