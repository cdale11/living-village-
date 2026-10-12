#include "living_village/saves.hpp"
#include "living_village/ui.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace village;
namespace {
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
World laboratory() {
    World w(42); w.animals.clear(); w.villagers.clear(); w.buildings.clear(); w.auto_develop = w.population_growth = false;
    w.ticks = 10; w.weather = Weather::Clear; w.temperature = 20; w.precipitation = 0; w.wind = 0;
    for (auto& t : w.tiles) { t = {}; t.elevation = .3; t.biomass = .5; }
    return w;
}
double water(const World& w) { double total = 0; for (const auto& t : w.tiles) if (t.terrain != Terrain::Water) total += t.moisture + t.surface_water + t.groundwater + t.snowpack; return total; }
void advance(World& w, int ticks) { for (int n = 0; n < ticks; ++n) w.update(); }
std::uint64_t hash(const std::string& text) { std::uint64_t value = 14695981039346656037ULL; for (unsigned char c : text) { value ^= c; value *= 1099511628211ULL; } return value; }
}
int main() {
    try {
        auto flow = laboratory(); flow.tile(15, 15).surface_water = 1; flow.tile(15, 15).elevation = .9; flow.precipitation = .002;
        const double before = water(flow); flow.update_ecology(1);
        check(flow.tile(15, 15).surface_water < 1 && flow.tile(16, 15).surface_water > 0, "Water did not flow down a head gradient");
        check(std::abs(water(flow) - before - flow.ecology.rain_input + flow.ecology.evaporation + flow.ecology.runoff_out) < 1e-7, "Hydrology created or lost unaccounted water");
        auto snow = laboratory(); snow.temperature = -2; snow.weather = Weather::Snow; snow.precipitation = .0008;
        snow.update_ecology(1); check(snow.tile(15, 15).snowpack > 0, "Cold precipitation did not accumulate snow");
        snow.temperature = 10; snow.precipitation = 0; snow.update_ecology(1); check(snow.tile(15, 15).snowpack == 0, "Warm weather did not melt snow");
        auto compact = laboratory(), porous = laboratory();
        for (auto* w : {&compact, &porous}) { auto& t = w->tile(15, 15); t.moisture = .2; t.surface_water = .3; }
        for (int n = 0; n < 500; ++n) compact.trample({15, 15});
        compact.update_ecology(1); porous.update_ecology(1);
        check(compact.tile(15, 15).moisture < porous.tile(15, 15).moisture && compact.tile(15, 15).compaction > .9, "Traffic did not compact soil and restrict infiltration");
        auto wet = laboratory(), dry = laboratory();
        for (auto* w : {&wet, &dry}) { auto& t = w->tile(15, 15); t.terrain = Terrain::Bush; t.food = 0; }
        dry.tile(15, 15).moisture = .03; dry.tile(15, 15).groundwater = 0;
        for (int n = 0; n < 200; ++n) { wet.update_ecology(1); dry.update_ecology(1); }
        check(wet.tile(15, 15).food > dry.tile(15, 15).food && wet.tile(15, 15).plant_health > dry.tile(15, 15).plant_health, "Drought did not limit growth and stress plants");
        auto forest = laboratory(); forest.ticks = 2400; forest.tile(10, 10).seed_bank = 1;
        forest.update_ecology(1); check(forest.tile(10, 10).terrain == Terrain::Tree && forest.ecology.trees_born == 1, "Seed bank did not produce a seedling in suitable habitat");
        auto crowded = laboratory(); crowded.ticks = 2400; crowded.tile(10, 10).seed_bank = 1;
        for (Point p : {Point{9, 10}, Point{11, 10}, Point{10, 9}, Point{10, 11}}) { crowded.tile(p.x, p.y).terrain = Terrain::Tree; crowded.tile(p.x, p.y).wood = 6; }
        crowded.update_ecology(1); check(crowded.tile(10, 10).terrain == Terrain::Grass, "Dense canopy did not suppress seedling competition");
        forest.ticks = 2410; forest.tile(10, 10).plant_health = 0; forest.tile(10, 10).wood = 3;
        forest.update_ecology(1); check(forest.tile(10, 10).terrain == Terrain::Grass && forest.tile(10, 10).loose_wood == 3 && forest.ecology.plants_died == 1, "Tree mortality lost wood or failed to return litter");
        auto fire = laboratory(); auto& fuel = fire.tile(10, 10); fuel.moisture = .02; fuel.biomass = .9;
        check(fire.ignite({10, 10}) && !fire.ignite({11, 10}), "Fire ignition ignored fuel/moisture");
        const double biomass = fuel.biomass; fire.update_ecology(1);
        check(fuel.biomass < biomass && fuel.plant_health < 100, "Fire did not consume fuel and damage plants");
        fire.weather = Weather::Rain; fire.precipitation = .01; fuel.moisture = .9;
        for (int n = 0; n < 10; ++n) fire.update_ecology(1);
        check(fuel.fire == 0, "Wet weather did not extinguish fire");
        auto grazing = laboratory(); check(grazing.spawn_animal(Species::Rabbit, {10, 10}), "Could not spawn rabbit");
        auto& rabbit = grazing.animals.front(); rabbit.hunger = 90; rabbit.thirst = 0; rabbit.action = AnimalAction::Feed; rabbit.action_timer = 3; rabbit.decision_timer = 10;
        const double grass = grazing.tile(10, 10).biomass; grazing.update_animals(1);
        check(rabbit.hunger < 90 && grazing.tile(10, 10).biomass < grass, "Grazing did not consume biomass");
        auto pollination = laboratory(); pollination.tile(10, 10).terrain = Terrain::Bush; pollination.tile(10, 10).pollination = .2;
        pollination.spawn_animal(Species::Bee, {10, 10}); auto& bee = pollination.animals.front(); bee.hunger = 90; bee.thirst = 0; bee.action = AnimalAction::Feed; bee.action_timer = 3; bee.decision_timer = 10;
        const double nectar = pollination.tile(10, 10).nectar; pollination.update_animals(1);
        check(pollination.tile(10, 10).pollination > .2 && pollination.tile(10, 10).nectar < nectar && bee.hunger < 90, "Bee did not consume nectar and pollinate flowers");
        auto predation = laboratory(); predation.spawn_animal(Species::Fox, {10, 10}); predation.spawn_animal(Species::Rabbit, {10, 10});
        predation.animals[0].hunger = 90; predation.animals[0].thirst = 0; predation.animals[0].action = AnimalAction::Hunt; predation.animals[0].target_id = predation.animals[1].id; predation.animals[0].action_timer = 2; predation.animals[0].decision_timer = 10;
        predation.update_animals(1); check(predation.animals.size() == 1 && predation.ecology.predations == 1 && predation.ecology.animal_deaths == 1 && predation.animals[0].hunger < 90, "Predation did not remove actual prey and feed the fox");
        auto birth = laboratory(); birth.spawn_animal(Species::Rabbit, {10, 10}); birth.animals[0].female = true; birth.animals[0].gestation = .001;
        birth.animals[0].metabolism = .9; birth.animals[0].mate_metabolism = 1.1;
        birth.update_animals(1); check(birth.animals.size() == 3 && birth.ecology.animal_births == 2 && birth.animals[1].parent == birth.animals[0].id && birth.animals[1].age == 0, "Pregnancy did not produce bounded inherited offspring");
        check(birth.animals[1].metabolism >= .95 && birth.animals[1].metabolism <= 1.05, "Metabolic traits did not inherit from both parents");
        while (birth.animals.size() < animal_limit) birth.spawn_animal(Species::Deer, {12, 12});
        check(!birth.spawn_animal(Species::Deer, {12, 12}), "Wildlife population exceeded its bound");
        auto repair = World(9); auto& site = repair.buildings[0]; site.condition = 30;
        check(!repair.repair(repair.villagers[0], site, 1), "Repair completed without materials");
        site.repair_wood = site.repair_stone = 1;
        for (int n = 0; n < 20 && repair.ecology.repairs == 0; ++n) repair.repair(repair.villagers[0], site, 1);
        check(site.condition == 55 && site.repair_wood == 0 && site.repair_stone == 0 && repair.ecology.repairs == 1, "Repair did not consume materials and work");
        check(repair.repair_stone_needed(site) == 0, "Routine patching still exhausted finite quarries");
        World maintenance(90); maintenance.villagers.resize(1); maintenance.auto_develop = maintenance.population_growth = false;
        maintenance.buildings[0].condition = 60; maintenance.food_stock = 30;
        auto& builder = maintenance.villagers[0]; builder.hunger = 0; builder.energy = 100; builder.wood = builder.stone = 1; builder.cooperation = 1; builder.decision_timer = 0;
        advance(maintenance, 300);
        check(maintenance.ecology.repairs > 0 && maintenance.buildings[0].condition > 80, "Agent did not deliver actual maintenance materials and complete repair work");
        auto flood = laboratory(); Building house; house.complete = true; house.position = {10, 10}; flood.buildings.push_back(house); flood.tile(10, 10).surface_water = 2;
        flood.update_ecology(1); check(flood.buildings[0].condition < 100 && flood.tile(10, 10).erosion > 0, "Flood did not weather structures and terrain");
        World living(777), equivalent(777); advance(living, 3000); advance(equivalent, 3000);
        check(living.serialize() == equivalent.serialize(), "Ecology was not deterministic");
        auto restored = World::deserialize(living.serialize()); check(restored.serialize() == living.serialize(), "Ecology and animal brains did not round-trip exactly");
        advance(living, 1000); advance(restored, 1000); check(restored.serialize() == living.serialize(), "Resumed ecology diverged");
        check(std::any_of(living.animals.begin(), living.animals.end(), [](const Animal& a) { return a.brain.updates > 0 && a.brain.replay_size > 0; }), "Wildlife MLPs did not learn from biological outcomes");
        auto frozen = World(77); advance(frozen, 100); frozen.learning_enabled = false;
        const int id = frozen.animals[0].id; const auto weights = frozen.animals[0].brain.weights; advance(frozen, 100);
        for (const auto& a : frozen.animals) if (a.id == id) check(a.brain.weights == weights, "Paused learning changed animal neural weights");
        World maximum(8); maximum.villagers.clear(); maximum.animals.clear(); maximum.next_person_id = population_limit + dead_history_limit;
        BrainInput experience{}; experience[0] = 1;
        for (int i = brain_actions; i < brain_inputs; ++i) experience[i] = .123456789012345;
        for (int i = 0; i < population_limit + dead_history_limit; ++i) {
            Villager p; p.id = i; p.name = "Save stress"; p.brain.initialize(8, i);
            for (int n = 0; n < replay_capacity; ++n) p.brain.observe(experience, .123456789012345);
            p.path.assign(4096, camp);
            if (i >= population_limit) { p.health = 0; p.death_day = 1; p.death_reason = "Hunger"; }
            maximum.villagers.push_back(p);
        }
        for (int i = 0; i < animal_limit; ++i) {
            check(maximum.spawn_animal(Species::Rabbit, camp), "Maximum save population setup failed");
            for (int n = 0; n < replay_capacity; ++n) maximum.animals.back().brain.observe(experience, .123456789012345);
        }
        const auto largest = maximum.serialize();
        check(largest.size() < save_byte_limit && World::deserialize(largest).serialize() == largest, "Bounded full populations/replay/routes exceeded the supported save budget");
        std::string invalid = living.serialize(); const auto start = invalid.find("ECOLOGY 1 "); const auto footer = invalid.rfind("CHECKSUM ");
        std::string body = invalid.substr(0, footer); body.replace(start, 9, "ECOLOGY 9"); invalid = body + "CHECKSUM " + std::to_string(hash(body)) + "\n";
        bool rejected = false; try { World::deserialize(invalid); } catch (const std::exception&) { rejected = true; } check(rejected, "Save accepted an unknown ecology schema");
#ifdef _LIBCPP_VERSION
        std::ifstream old("tests/fixtures/version6-libcxx.lvsave"); std::ostringstream data; data << old.rdbuf();
#else
        std::ifstream old("tests/fixtures/version6.lvsave"); std::ostringstream data; data << old.rdbuf();
#endif
        auto legacy = World::deserialize(data.str()); check(legacy.seed == 600 && legacy.animals.size() > 0 && legacy.world_name == "New Village", "Version 6 did not gain ecology without losing village identity");
        World emergency(123); emergency.ecology_enabled = false; emergency.villagers.resize(1);
        // Exercise hazard interruption with ecology enabled but below its cadence.
        emergency.ecology_enabled = true; emergency.ticks = 1; emergency.tile(32, 32).surface_water = .8;
        for (int y = 29; y <= 35; ++y) for (int x = 29; x <= 35; ++x) emergency.tile(x, y).surface_water = .8;
        emergency.villagers[0].x = emergency.villagers[0].y = 32; emergency.villagers[0].hunger = 90; emergency.villagers[0].food = 1;
        advance(emergency, 15); check(emergency.villagers[0].hunger < 60, "Repeated flood interruption prevented a stationary emergency meal");
        Interface ui; ui.view.panel = Panel::Overview; ui.view.overview_page = 6;
        InputState right; right.x = 1; ui.update(right, living, .3); check(ui.view.overview_page == 7, "Ecology/wildlife pages were unreachable");
        InputState next; next.pressed[static_cast<int>(Button::Next)] = true; ui.update(next, living, .3); check(ui.view.selected_animal == 1, "Controller did not cycle wildlife");
        std::cout << "PASS: water accounting/runoff, snowmelt, compaction, drought, succession, mortality/litter, fire, grazing/nectar/pollination, predation, pregnancy/inheritance/cap, maintenance/materials, flooding, neural learning/freeze, deterministic persistence, migration and controller ecology pages\n";
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
