#include "living_village/saves.hpp"
#include "living_village/ui.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
using namespace village;
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
void advance(World& w, int steps) { for (int n = 0; n < steps; ++n) w.update(); }
std::string contents(const std::filesystem::path& path) { std::ifstream in(path); return {(std::istreambuf_iterator<char>(in)), {}}; }
int main() {
    std::filesystem::path directory;
    try {
        for (int b = 0; b < biome_count; ++b) for (int g = 0; g < geography_count; ++g) {
            World first(123, static_cast<Biome>(b), static_cast<Geography>(g)), second(123, static_cast<Biome>(b), static_cast<Geography>(g));
            check(first.serialize() == second.serialize(), "Same seed and generation settings differed");
            check(first.passable(32, 32) && !first.route(camp, {25, 32}).empty() && !first.route(camp, {35, 26}).empty(), "Layout isolated the starting supplies");
            advance(first, 2000); check(first.alive_count() == 6, "A biome/layout failed the initial survival smoke");
            check(World::deserialize(first.serialize()).serialize() == first.serialize(), "Generation settings or world did not round-trip");
        }
        World forest(12, Biome::Woodland), desert(12, Biome::Arid), river(12, Biome::Temperate, Geography::River), island(12, Biome::Meadow, Geography::Island);
        int trees_forest = 0, trees_desert = 0, water = 0;
        for (int i = 0; i < 4096; ++i) { trees_forest += forest.tiles[i].terrain == Terrain::Tree; trees_desert += desert.tiles[i].terrain == Terrain::Tree; water += island.tiles[i].terrain == Terrain::Water; }
        check(trees_forest > trees_desert * 3 && water > 1000 && island.tile(0, 0).terrain == Terrain::Water, "Biome or island options were decorative");
        check(!river.route({5, 5}, camp).empty(), "River fords did not connect the banks");
        World greener(22, Biome::Woodland), drier(22, Biome::Arid);
        for (auto* w : {&greener, &drier}) { w->auto_develop = w->population_growth = false; w->villagers.clear(); w->tile(1, 1) = {Terrain::Bush, 0, 0}; advance(*w, 100); }
        check(greener.tile(1, 1).regrowth > drier.tile(1, 1).regrowth, "Biome did not affect regrowth");
        island.plan(BuildingKind::Shelter, {30, 30}); island.plan(BuildingKind::Shelter, {34, 30});
        for (auto& house : island.buildings) if (house.kind == BuildingKind::Shelter) { house.complete = true; house.wood_delivered = 12; house.stone_delivered = 4; house.work = 24; }
        island.food_stock = 30; island.ticks = 11 * 2400;
        check(island.welcome_migrant() && island.villagers.back().goal == Goal::Arrive && !island.villagers.back().path.empty(), "Island could not accept an arriving resident at its shore");

        char temporary[] = "/tmp/lv-world-management-XXXXXX";
        char* created = mkdtemp(temporary); check(created != nullptr, "Could not create private test directory"); directory = created;
        SaveCatalog catalog(directory);
        check(catalog.list().empty() && catalog.active().empty(), "Empty catalog fabricated saved worlds");
        World old(44); save_atomic(old, directory / "village.lvsave"); save_atomic(World(45), directory / "previous-village.lvsave");
        check(catalog.list().size() == 2 && catalog.load("default").seed == 44, "Legacy current/previous saves were hidden");
        World named(123, Biome::Woodland, Geography::Island); named.world_name = "Green Haven"; advance(named, 50);
        const auto id = catalog.create(named); catalog.activate(id);
        check(id == "world-000001" && catalog.active() == id && catalog.load(id).serialize() == named.serialize(), "Named creation/activation changed world state");
        check(contents(directory / "village.lvsave") == old.serialize(), "Creating a world overwrote the original save");
        catalog.rename(id, "Renamed Haven"); auto renamed = catalog.load(id);
        check(renamed.world_name == "Renamed Haven" && renamed.ticks == named.ticks && renamed.villagers[0].brain.weights == named.villagers[0].brain.weights, "Rename discarded simulation/neural progress");
        const auto list = catalog.list(); check(list.back().name == "Renamed Haven" && list.back().seed == 123 && list.back().biome == Biome::Woodland, "Catalog metadata did not describe the saved world");
        std::ofstream(catalog.path(id)) << "corrupt";
        bool backup = false; auto recovered = catalog.load(id, &backup);
        check(backup && recovered.serialize() == named.serialize(), "Named save did not recover its backup");
        save_atomic(recovered, catalog.path(id)); std::filesystem::remove(catalog.path(id));
        check(catalog.exists(id) && catalog.list().back().backup_only, "Backup-only world disappeared from the manager");
        save_atomic(named, catalog.path(id));
        bool rejected = false; try { catalog.erase(id, id); } catch (const std::exception&) { rejected = true; }
        check(rejected && catalog.exists(id), "Manager deleted the open world");
        const auto copy_id = catalog.create(named);
        check(copy_id != id && catalog.load(copy_id).serialize() == named.serialize(), "Snapshot copy reused an identifier or changed state");
        const auto exports = catalog.export_directory(id); export_history(named, exports);
        catalog.erase(id, copy_id);
        check(!catalog.exists(id) && !std::filesystem::exists(catalog.path(id).string() + ".bak") && std::filesystem::exists(exports / "residents.csv"), "Deletion resurrected through backup or removed exports");
        const auto later_id = catalog.create(named); check(later_id == "world-000003", "Deleted IDs were reused and could overwrite retained exports");
        write_atomic_text(directory / "worlds" / (".delete-" + later_id), "DELETE\n");
        check(!catalog.exists(later_id), "Interrupted deletion exposed a leftover primary/backup"); catalog.erase(later_id, copy_id);
        rejected = false; try { catalog.path("../../other"); } catch (const std::exception&) { rejected = true; } check(rejected, "Save identifier allowed traversal");
        const auto victim = directory / "victim"; std::ofstream(victim) << "KEEP";
        std::filesystem::create_symlink(victim, directory / "worlds/world-000010.lvsave");
        rejected = false; try { catalog.load("world-000010"); } catch (const std::exception&) { rejected = true; }
        check(rejected && contents(victim) == "KEEP", "Linked save accessed or changed another file"); std::filesystem::remove(directory / "worlds/world-000010.lvsave");
        std::filesystem::create_symlink(victim, directory / "unsafe.lvsave.tmp");
        rejected = false; try { save_atomic(named, directory / "unsafe.lvsave"); } catch (const std::exception&) { rejected = true; }
        check(rejected && contents(victim) == "KEEP", "Temporary save followed a symlink"); std::filesystem::remove(directory / "unsafe.lvsave.tmp");
        while (catalog.list().size() < 34) catalog.create(named);
        rejected = false; try { catalog.create(named); } catch (const std::exception&) { rejected = true; } check(rejected, "Catalog exceeded its 32 named-world bound");
#ifdef _LIBCPP_VERSION
        const char* fixture = "tests/fixtures/version5-libcxx.lvsave";
#else
        const char* fixture = "tests/fixtures/version5.lvsave";
#endif
        World migrated = World::deserialize(contents(fixture));
        check(migrated.seed == 555 && migrated.ticks == 4000 && migrated.villagers[0].brain.replay_size > 0 && !migrated.statistics.empty() && migrated.biome == Biome::Temperate && migrated.geography == Geography::Ponds, "Version 5 migration lost learned/history/generation state");
        check(World::deserialize(migrated.serialize()).serialize() == migrated.serialize(), "Migrated world did not save exactly");

        Interface ui; InputState press; press.pressed[static_cast<int>(Button::Confirm)] = true;
        ui.view.panel = Panel::NewVillage; ui.view.has_world = false; ui.view.menu_index = 1;
        ui.update(press, named, .033); check(ui.view.panel == Panel::SeedEdit, "Controller could not enter seed editor");
        ui.view.edit_buffer = "4294967296"; ui.update(press, named, .033); check(ui.view.panel == Panel::SeedEdit, "Overflowing seed was accepted");
        ui.view.edit_buffer = "4294967295"; ui.update(press, named, .033); check(ui.view.draft_seed == 4294967295U && ui.view.panel == Panel::NewVillage, "Maximum unsigned seed failed");
        ui.view.menu_index = 2; ui.update(press, named, .033); check(ui.view.draft_biome == Biome::Woodland, "Controller could not select biome");
        ui.view.menu_index = 3; ui.update(press, named, .033); check(ui.view.draft_geography == Geography::River, "Controller could not select layout");
        ui.view.menu_index = 5; check(ui.update(press, named, .033) == Request::NewVillage, "First-world creation was not reachable");
        ui.view.has_world = true; ui.view.panel = Panel::Saves; ui.view.saves = {{copy_id, "Existing World"}}; ui.view.active_slot = "default"; ui.view.menu_index = 0;
        InputState delete_press = press; delete_press.pressed[static_cast<int>(Button::Inspect)] = true;
        check(ui.update(delete_press, named, .033) == Request::None && ui.view.panel == Panel::ConfirmDelete, "Delete and confirm chord skipped confirmation");
        check(ui.update(press, named, .033) == Request::DeleteSlot, "Controller could not confirm deletion");
        ui.view.panel = Panel::Saves; ui.view.active_slot = copy_id; ui.update(delete_press, named, .033);
        check(ui.view.panel == Panel::Saves, "Controller offered deletion of the open world");
        for (Panel panel : {Panel::NewVillage, Panel::SeedEdit, Panel::NameEdit, Panel::ConfirmCreate, Panel::Saves, Panel::ConfirmDelete}) { ui.view.panel = panel; InputState exit; exit.exit_requested = true; check(ui.update(exit, named, .033) == Request::Exit, "Fn+Start failed in management UI"); }
        std::filesystem::remove_all(directory);
        std::cout << "PASS: seeded biome/layout generation, resource/climate differences, river crossings and island arrivals; named/legacy saves, rename/copy/recovery/deletion, bounded slots, path/link safety, v5 migration and controller creation/confirmation\n";
    } catch (const std::exception& error) { if (!directory.empty()) std::filesystem::remove_all(directory); std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
