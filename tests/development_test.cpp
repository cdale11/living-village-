#include "living_village/saves.hpp"
#include "living_village/ui.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace village;
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
void advance(World& w, int steps) { for (int n = 0; n < steps; ++n) w.update(); }
void clearing(World& w) { w.auto_develop = false; for (auto& tile : w.tiles) tile = {}; }
int available_materials(const World& w) {
    int amount = w.wood_stock + w.stone_stock;
    for (const auto& p : w.villagers) amount += p.wood + p.stone;
    for (const auto& b : w.buildings) if (!b.complete) amount += b.wood_delivered + b.stone_delivered;
    return amount;
}
int main() {
    try {
        World unsupplied(11); clearing(unsupplied); unsupplied.villagers.resize(1);
        check(unsupplied.plan(BuildingKind::Shelter, {30, 30}), "Valid shelter plan rejected");
        auto& blocked = unsupplied.buildings.back(); blocked.wood_delivered = 12;
        auto& builder = unsupplied.villagers[0];
        builder.x = 30; builder.y = 30; builder.goal = Goal::Build; builder.target_id = blocked.id; builder.decision_timer = 5;
        advance(unsupplied, 200);
        check(!blocked.complete && blocked.work == 0 && blocked.stone_delivered == 0, "Construction advanced without required stone");

        World cooperative(11); clearing(cooperative); cooperative.villagers.resize(2); cooperative.food_stock = 18;
        cooperative.plan(BuildingKind::Shelter, {30, 30}, 0);
        auto& helper = cooperative.villagers[1]; helper.x = 34; helper.y = 32; helper.hunger = 0; helper.energy = 100; helper.wood = 6; helper.cooperation = 1; helper.decision_timer = 0;
        cooperative.update();
        check(helper.goal == Goal::DeliverMaterials && helper.wood == 6 && cooperative.buildings.back().wood_delivered == 0, "Delivery was not a chosen physical trip");
        advance(cooperative, 100);
        check(cooperative.buildings.back().wood_delivered == 6 && helper.wood == 0, "Helper did not deliver real wood");
        check(cooperative.villagers[0].relationships[1] > 0 && helper.relationships[0] > 0, "Useful help did not affect relationships");
        check(available_materials(cooperative) == 6, "Delivery duplicated or destroyed wood");

        World construction(22); clearing(construction);
        construction.wood_stock = 12; construction.stone_stock = 4;
        construction.plan(BuildingKind::Shelter, {32, 30});
        check(available_materials(construction) == 16, "Initial materials incorrect");
        advance(construction, 500);
        const auto& completed = construction.buildings.back();
        check(completed.complete && completed.work == building_cost(completed.kind).work, "Supplied agents did not finish construction");
        check(available_materials(construction) == 0 && completed.wood_delivered == 12 && completed.stone_delivered == 4, "Construction did not consume its exact material cost");
        check(completed.contributors.size() >= 2, "Construction lacked multiple contributors");
        check(construction.shelter_capacity() == 4, "Shelter did not add beds");

        World farm(7); clearing(farm); farm.villagers.resize(1);
        farm.plan(BuildingKind::Farm, {30, 30}); auto& plot = farm.buildings.back();
        plot.complete = true; plot.wood_delivered = 6; plot.stone_delivered = 2; plot.work = 12;
        farm.villagers[0].health = 0;
        advance(farm, 1500);
        check(plot.food > 0 && plot.food <= 12, "Farm did not produce renewable food");
        auto& farmer = farm.villagers[0]; farmer.health = 100; farmer.hunger = 0; farmer.food = 0;
        farmer.death_day = 0; farmer.death_reason.clear();
        farmer.goal = Goal::TendFarm; farmer.target_id = plot.id; farmer.x = 30; farmer.y = 30; farmer.decision_timer = 5;
        int crop_before = plot.food;
        advance(farm, 32);
        check(farmer.food > 0 && plot.food < crop_before && farmer.farm_skill > 0, "Harvest did not move crops into inventory or teach skill");
        World shelter(7); clearing(shelter); shelter.villagers.resize(1);
        shelter.plan(BuildingKind::Shelter, {30, 30}); shelter.buildings.back().complete = true;
        auto& sleeper = shelter.villagers[0]; sleeper.x = 30; sleeper.y = 30; sleeper.energy = 1; sleeper.goal = Goal::Rest; sleeper.target_id = 2; sleeper.decision_timer = 5;
        advance(shelter, 10); check(sleeper.energy > 4, "Shelter did not improve rest");

        World capacities(6); clearing(capacities);
        capacities.plan(BuildingKind::FoodStorage, {30, 30}); capacities.buildings.back().complete = true;
        capacities.plan(BuildingKind::Stockpile, {31, 30}); capacities.buildings.back().complete = true;
        check(capacities.food_capacity() == 70 && capacities.material_capacity() == 52, "Storage buildings have no capacity effect");
        World fire(3); clearing(fire); fire.villagers.resize(1); fire.buildings[1].fuel = 0;
        auto& carrier = fire.villagers[0]; carrier.x = 40; carrier.y = 32; carrier.wood = 1; carrier.hunger = 0; carrier.energy = 100;
        fire.update();
        check(carrier.goal == Goal::RefuelFire && carrier.wood == 1 && fire.buildings[1].fuel == 0, "Fire fuel teleported instead of being delivered");
        advance(fire, 70);
        check(carrier.wood == 0 && fire.buildings[1].fuel > 200, "Villager did not physically consume wood to refuel");
        World designated(3); clearing(designated); designated.villagers.resize(1);
        designated.tile(50, 50) = {Terrain::Tree, 0, 0, 6, 0};
        auto& gatherer = designated.villagers[0]; gatherer.x = 10; gatherer.y = 10; gatherer.hunger = 0;
        check(designated.designate({50, 50}), "Gathering area was not accepted");
        designated.update();
        check(gatherer.goal == Goal::GatherWood && !gatherer.path.empty() && gatherer.known_wood.empty(), "Shared designation did not inform utility choice");
        advance(designated, 550);
        check(gatherer.wood > 0 && gatherer.wood + designated.tile(50, 50).wood == 6, "Designated gathering did not conserve resources");
        check(World::deserialize(designated.serialize()).gather_orders == designated.gather_orders, "Gathering designations were not saved");
        check(designated.designate({50, 50}) && designated.gather_orders.empty(), "Gathering area could not be cancelled");

        World spring(3), winter(3); clearing(spring); clearing(winter);
        for (auto& p : spring.villagers) p.health = 0;
        for (auto& p : winter.villagers) p.health = 0;
        spring.tile(20, 20) = {Terrain::Bush, 0, 0}; winter.tile(20, 20) = {Terrain::Bush, 0, 0};
        winter.ticks = 30 * 2400;
        advance(spring, 1000); advance(winter, 1000);
        const auto& spring_crop = spring.tile(20, 20); const auto& winter_crop = winter.tile(20, 20);
        check(spring_crop.food * 80 + spring_crop.regrowth > winter_crop.food * 80 + winter_crop.regrowth, "Winter did not slow resource regeneration");

#ifdef _LIBCPP_VERSION
        std::ifstream legacy_file("tests/fixtures/version1-libcxx.lvsave", std::ios::binary);
#else
        std::ifstream legacy_file("tests/fixtures/version1.lvsave", std::ios::binary);
#endif
        std::ostringstream legacy; legacy << legacy_file.rdbuf();
        World migrated = World::deserialize(legacy.str());
        check(migrated.villagers.size() == 6 && migrated.food_stock == 18 && migrated.buildings.size() == 2, "Version 1 migration lost the village");
        std::istringstream old_header(legacy.str()); std::string magic; int version = 0; std::uint32_t old_seed = 0; std::uint64_t old_ticks = 0; int old_food = 0; std::mt19937 old_rng;
        old_header >> magic >> version >> old_seed >> old_ticks >> old_food >> old_rng;
        check(migrated.seed == old_seed && migrated.ticks == old_ticks && migrated.rng == old_rng, "Migration changed seed/time/random state");
        check(migrated.villagers[0].name == "Mira" && migrated.villagers[0].x == 31 && migrated.villagers[0].y == 30 && migrated.villagers[0].hunger == 15 && migrated.villagers[0].energy == 90, "Migration changed existing villager needs/position");
        check(migrated.serialize().find("LIVING_VILLAGE 10\n") == 0, "Migrated save did not upgrade to version 10");
        check(World::deserialize(migrated.serialize()).serialize() == migrated.serialize(), "Migrated save did not round-trip");
        advance(migrated, 3000); check(migrated.alive_count() == 6 && migrated.buildings.size() > 2, "Migrated village failed to develop");
        construction.simulation_speed = 4;
        World restored = World::deserialize(construction.serialize());
        check(restored.simulation_speed == 4 && !restored.auto_develop, "Simulation settings were not saved");
        advance(construction, 1000); advance(restored, 1000);
        check(construction.serialize() == restored.serialize(), "Development save continuation diverged");

        World village(123); village.population_growth = false; advance(village, 24000);
        check(village.alive_count() == 6 && village.shelter_capacity() >= 6, "Autonomous village failed to provide shelter");
        int contributors = 0;
        for (const auto& b : village.buildings) if (b.complete && !b.contributors.empty()) ++contributors;
        check(contributors >= 3, "Shared needs did not produce cooperative development");
        World farm_need(123); farm_need.food_stock = 0; advance(farm_need, 24000);
        int farms = 0; for (const auto& b : farm_need.buildings) if (b.complete && b.kind == BuildingKind::Farm) ++farms;
        check(farms > 0 && farm_need.alive_count() == 6, "Food shortage did not produce working farms");
        World twenty(42);
        for (int id = 6; id < 20; ++id) {
            Villager p; p.id = id; p.name = "Settler " + std::to_string(id); p.x = 32; p.y = 32; twenty.villagers.push_back(p);
        }
        advance(twenty, 12000);
        check(twenty.alive_count() == 20, "Twenty-agent village did not survive");
        check(World::deserialize(twenty.serialize()).serialize() == twenty.serialize(), "Twenty-agent save did not round-trip");

        Interface ui; InputState input; input.pressed[static_cast<int>(Button::Confirm)] = true;
        ui.view.camera_x = 30; ui.view.camera_y = 30;
        ui.update(input, village, .033); check(ui.view.panel == Panel::Tile, "Controller did not inspect tile");
        World blank(1); clearing(blank);
        ui.update(input, blank, .033); check(ui.view.panel == Panel::Build, "Controller did not open construction choices");
        check(ui.update(input, blank, .033) == Request::Plan, "Controller did not confirm a plan");
        check(!blank.plan(BuildingKind::Farm, camp) && !blank.plan(BuildingKind::Farm, {-1, 20}), "Invalid plan placement accepted");
        std::cout << "PASS: material gates/accounting, physical cooperation, construction, farms, shelter/storage, designations, seasons, v1 migration, deterministic continuation, autonomous development, 20 villagers, controller planning\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
