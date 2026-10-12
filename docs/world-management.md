# World creation and save management

## Create a world on the handheld

On first launch without a saved world, the game waits at **Create world**. It
never autosaves the unconfirmed preview. From a running village, use
**Start → New village**. Select:

1. **Name** — up to 24 letters, numbers, spaces or hyphens.
2. **Seed** — any unsigned 32-bit number, including zero (`0–4294967295`).
3. **Biome** — temperate, woodland, meadow, arid, taiga, rainforest, wetland or savanna.
4. **Layout** — ponds, river, island, coast, highlands, lakes, peninsula, archipelago, delta, canyon, valley or plains.
5. **Food** — lean (6 meals), standard (18) or generous (30) starting supplies.
6. **Population** — 2 to 20 starting adults (default six); initial identities,
   names, personalities and independent neural models are created for each.
7. **Create this world** — generates and saves a new independent world.
8. **Browse saved worlds** — opens the save manager.

Use D-pad up/down to select a row. A edits the name/seed or cycles biome/layout;
left/right also cycles biome/layout/food/population. The preview shows actual seeded terrain,
with the camp marked. Descriptions explain the selected climate and geography. In text/number editors, left/right moves
between characters and up/down changes the selected character. A accepts,
B cancels, and X clears a name character or chooses a random seed. X on the
creator also chooses a random seed. No keyboard is needed.

Creating a world while another is open has a final confirmation. The old world
is saved first; failure to save cancels the switch. New files never replace an
older world. All neural models, families, inventories and simulation state are
independent. The same seed/settings/generator version reproduce initial worlds
within the supported build/runtime; saved worlds restore actual tiles and state
rather than regenerating them. Cross-architecture floating-point histories are
not promised identical.

## Biomes and geographical layouts

| Biome | Tree / stone / berry tile rates outside the clearing | Plant/crop growth multiplier | Appearance |
| --- | --- | ---: | --- |
| Temperate | 14% / 5% / 6.5% | 1.00 | Original green clearing |
| Woodland | 28% / 4.5% / 9% | 1.10 | Darker forest ground |
| Meadow | 6.5% / 3.5% / 9.5% | 1.05 | Brighter grassland |
| Arid | 5.5% / 9% / 3% | 0.65 | Sandy, sparse landscape |
| Taiga | 24% / 7.5% / 5% | 0.72 | Conifers, cool climate (-9 C offset) |
| Rainforest | 35% / 2.5% / 12% | 1.20 | Warm (+7 C), frequent rain |
| Wetland | 12% / 2.5% / 10% | 1.12 | Initially wetter soil, frequent rain |
| Savanna | 8% / 5.5% / 6.5% | 0.85 | Warm (+6 C), drier grassland |

These rates are initial rolls before geographical masks and starter guarantees.
Season/rain and irrigation modifiers still apply. Arid worlds require more care
with food production, although all options retain a safe central clearing and
reachable starting berry, wood and stone supplies.

- **Ponds:** the original seven seeded pond clusters. Temperate/ponds preserves
  the original generation and initial simulation random stream.
- **River:** a meandering river west of camp, with two land fords connecting banks.
- **Island:** an irregular central island surrounded by water. Newcomers can
  disembark on a reachable shoreline and then walk to camp.
- **Coast:** a winding eastern coastline with inland land available for growth.
- **Highlands:** stone-rich diagonal ridges and passable valleys. This is mineral
  geography, not a new elevation/mountain movement system; stone remains walkable.

- **Lakes:** two broad lakes around a connected camp area.
- **Peninsula:** a long tongue of land narrowing towards the south.
- **Archipelago:** a central habitable island plus separate wildlife islets.
- **Delta:** branching river channels with land crossings.
- **Canyon:** narrow river with walkable stone walls and greater elevation.
- **Valley:** river beside sloping valley terrain.
- **Plains:** continuous land without permanent water; rainfall supplies soil water
  and drinking puddles. Drought is more challenging here.

Eight biomes, twelve layouts and three food bundles provide 288 starting
terrain/food combinations per seed, each with 19 population choices. Food choice changes only initial stock; saved quantities
persist normally, with no continuing artificial resource bonus. The default is six villagers; population can be set to 2–20. Starting resources
remain reachable, but larger groups compete for the same finite supplies.
Population choice affects only world creation, never an existing save.

Water blocks ordinary movement. Layouts and biome effects are actual simulation
state; they are not only names or decorative previews.

## Manage saved worlds

Use **Start → Manage saved worlds**. The list shows four entries per page with
names, seeds, days and selected biome/layout. Up/down navigates across pages.

| Button | Save-manager action |
| --- | --- |
| A | Load selected world; open world resumes without discarding progress |
| Y | Rename selected world using the controller editor |
| X | Request deletion; A confirms in a separate screen, B cancels |
| L1 | Save an independent snapshot of the currently open world |
| B | Return to menu/creator |
| Fn+Start | Save and exit from any manager/editor/confirmation panel |

There are up to **32 named worlds**, plus the two legacy current/previous slots.
You can delete inactive worlds to free space. The open world cannot be deleted:
create/load another world first. Deletion removes the selected primary, backup
and leftover temporary save files. It cannot be undone inside the game. CSV
exports are retained. Interrupted deletion has a durable intent marker so a
leftover backup does not silently restore a deleted world into the catalog.
Names can repeat; stable identifiers keep their files separate and are not reused
under normal management, protecting retained exports from another world's writes.

Loading first validates the target, then saves the village being left, then
switches the active marker. A missing/corrupt target or failed current save keeps
the current world open. Corrupt primary files can recover from validated backups;
backup-only worlds still appear in the manager. If both current and backup fail
at startup, the manager opens without overwriting them, allowing another world
to be loaded/created or the broken inactive entry to be deleted.

## Files and compatibility

```text
saves/
├── active-world.txt
├── next-world.txt
├── village.lvsave                  # legacy original world, if present
├── previous-village.lvsave         # legacy previous world, if present
├── worlds/
│   ├── world-000001.lvsave
│   ├── world-000001.lvsave.bak
│   └── world-000002.lvsave
└── exports/
    ├── world-000001/
    └── world-000002/
```

The marker remembers the selected world on next launch. The counter allocates
stable world IDs. World saves retain atomic writes, flushing, checksums and one
validated backup each. Snapshot copies retain the current seed, time and trained
models so they can be loaded as independent branches. Save names never become
file paths. Other unrelated files are ignored by the manager.

Save format **7** stores name, seed, biome, layout and generator version in
addition to all simulation/neural/history state. Versions **1–6** upgrade
automatically. Existing current/previous files remain browsable without being
silently deleted or moved. Old worlds use their original temperate/ponds setting;
their actual terrain, learned weights, replay and progress remain intact. Older
executables cannot read version 10. Back up/synchronize the **whole saves folder**,
including markers, worlds, backups and exports, before upgrades.

## Desktop/SSH commands

```sh
./build/living-village --new --seed 123 --world-name 'Forest Isle' --biome woodland --layout island --population 12
./build/living-village --headless --seed 123 --biome arid --layout highlands --steps 72000
make worlds-smoke
```

`--new` now creates a separate named save. Generation options apply to a new
world, initial creation defaults or headless/temporary runs; they do not rewrite
a resumed world. `--no-save` and temporary benchmarks do not load/write saved
worlds or permit manager rename/delete/snapshot operations. Manual CSV export
remains an explicit user action. The first-run creator, seed overflow rejection,
CLI-created independent worlds, last-world resume and non-overwriting corrupt
startup are covered by the desktop integration smoke test.
