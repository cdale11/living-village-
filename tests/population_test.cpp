#include "living_village/saves.hpp"
#include "living_village/ui.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace village;
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
void advance(World& world, int steps) { for (int n = 0; n < steps; ++n) world.update(); }
void completed_house(World& world, Point at) {
    check(world.plan(BuildingKind::Shelter, at), "House fixture placement failed");
    auto& house = world.buildings.back(); house.complete = true;
    house.wood_delivered = 12; house.stone_delivered = 4; house.work = 24;
}
World family() {
    World world(7); world.auto_develop = false; world.villagers.resize(2);
    completed_house(world, {30, 30}); world.ticks = 7 * 2400;
    for (auto& p : world.villagers) { p.age = 24; p.hunger = 0; p.energy = 100; p.home = 2; p.x = p.y = 30; p.family_drive = 1; p.cooperation = .8; }
    world.villagers[0].relationships[1] = world.villagers[1].relationships[0] = 80;
    check(world.form_household(0, 1), "Healthy trusting adults could not form a household");
    return world;
}
int main(int argc, char** argv) {
    const bool quick = argc > 1 && std::string(argv[1]) == "--quick";
    try {
        World parents = family();
        const int reserve = parents.food_stock;
        check(parents.have_child(0, 1), "Eligible household did not have a child");
        const int baby_id = parents.villagers.back().id;
        check(baby_id == 6 && parents.villagers.back().age == 0 && parents.births == 1 && parents.food_stock == reserve - 2, "Birth did not preserve identity or consume its food cost");
        const auto& baby = parents.villagers.back();
        check(baby.parents == std::array<int, 2>{0, 1} && baby.parent_names[0] == "Mira" && baby.parent_names[1] == "Arin" && baby.cooperation >= .7 && baby.cooperation <= .9, "Family links or inherited personality incorrect");
        const auto after_birth = parents.serialize();
        check(!parents.have_child(0, 1) && parents.serialize() == after_birth, "Birth cooldown failed or a rejected action changed state");
        check(World::deserialize(after_birth).serialize() == after_birth, "Family save changed state");

        World shortage = parents; shortage.ticks = 99 * 2400; shortage.food_stock = 0;
        check(!shortage.have_child(0, 1), "Food shortage did not prevent a birth");
        World full_house = parents; full_house.ticks = 99 * 2400;
        Villager resident; resident.id = full_house.next_person_id++; resident.name = "Resident"; resident.home = 2;
        full_house.villagers.push_back(resident);
        check(!full_house.have_child(0, 1), "Full household did not prevent birth");
        World paused = parents; paused.ticks = 99 * 2400; paused.population_growth = false;
        check(!paused.have_child(0, 1) && !paused.welcome_migrant(), "Population preference did not stop new residents");
        World kin = parents; kin.person(baby_id)->age = 22;
        Villager sibling; sibling.id = kin.next_person_id++; sibling.name = "Sibling"; sibling.home = 2; sibling.parents = {0, 1}; sibling.parent_names = {"Mira", "Arin"};
        const int sibling_id = sibling.id; kin.villagers.push_back(sibling);
        kin.person(baby_id)->relationships[sibling_id] = kin.person(sibling_id)->relationships[baby_id] = 100;
        check(!kin.form_household(baby_id, sibling_id), "Close relatives formed a reproductive household");
        kin.person(0)->partner = kin.person(1)->partner = -1;
        check(!kin.form_household(0, baby_id), "Parent and child formed a household partnership");

        World care = parents; care.kill(1, "Hunger");
        care.person(0)->food = 1; care.person(0)->x = 40; care.person(0)->y = 30; care.person(0)->decision_timer = 0;
        care.person(baby_id)->hunger = 80;
        const int care_stock = care.food_stock;
        care.update();
        check(care.person(0)->goal == Goal::Care && care.person(0)->target_id == baby_id && care.person(0)->food == 1 && care.person(baby_id)->hunger > 79, "Care meal teleported or used an index instead of stable ID");
        advance(care, 100);
        check(care.person(0)->food == 0 && care.person(baby_id)->hunger < 40 && care.food_stock == care_stock, "Physical care did not consume carried food");
        check(care.person(baby_id)->relationships[0] > 60, "Care did not strengthen the child's trust");
        World race = parents;
        for (int id : {0, 1}) { race.person(id)->food = 1; race.person(id)->goal = Goal::Care; race.person(id)->target_id = baby_id; race.person(id)->decision_timer = 5; }
        race.person(baby_id)->hunger = 45; advance(race, 12);
        check(race.person(0)->food + race.person(1)->food == 1 && race.person(baby_id)->hunger < 2, "Two caregivers consumed two meals for one need");

        World ageing = parents; ageing.population_growth = false; ageing.person(baby_id)->age_days = 39;
        ageing.ticks = 8 * 2400 - 1; ageing.update();
        check(ageing.person(baby_id)->age == 1 && ageing.person(baby_id)->age_days == 0, "Child did not age across a birthday");
        ageing.person(baby_id)->age = 15; ageing.person(baby_id)->age_days = 39;
        ageing.person(0)->age = 89; ageing.person(0)->age_days = 39; ageing.person(0)->food = 2; ageing.person(0)->wood = 3;
        ageing.ticks = 9 * 2400 - 1; ageing.update();
        check(!ageing.person(baby_id)->child() && !ageing.person(0)->alive() && ageing.person(0)->death_reason == "Old age", "Working age or old-age mortality failed");
        auto* ancestor = ageing.person(0);
        const auto& dropped = ageing.tile(static_cast<int>(std::round(ancestor->x)), static_cast<int>(std::round(ancestor->y)));
        check(dropped.loose_food == 2 && dropped.loose_wood == 3 && ancestor->food == 0 && ancestor->wood == 0, "Death duplicated or lost carried supplies");
        check(!ageing.kill(0, "Old age") && dropped.loose_wood == 3, "Death deposited inventory more than once");
        check(World::deserialize(ageing.serialize()).serialize() == ageing.serialize(), "Death/ancestry state did not round-trip");

        World arrivals = family(); arrivals.ticks = 11 * 2400;
        const int arrival_food = arrivals.food_stock;
        check(arrivals.welcome_migrant(), "Prosperous village did not accept a newcomer");
        const auto& newcomer = arrivals.villagers.back();
        check(newcomer.id == 6 && newcomer.goal == Goal::Arrive && !newcomer.path.empty() && (newcomer.x == 0 || newcomer.x == 63 || newcomer.y == 0 || newcomer.y == 63) && newcomer.food == 0 && arrivals.food_stock == arrival_food, "Arrival did not walk from the boundary or created food");
        check(!arrivals.welcome_migrant(), "Arrival cooldown failed");
        arrivals.food_stock = 0; arrivals.next_migration_day = 0;
        check(!arrivals.welcome_migrant(), "Food shortage did not stop migration");

        World identities(9); identities.auto_develop = false; identities.population_growth = false; identities.villagers.resize(1);
        for (auto& tile : identities.tiles) tile = {};
        completed_house(identities, {30, 30}); identities.person(0)->relationships[6] = 10;
        identities.buildings[0].initiator = 6; identities.buildings[0].initiator_name = "Former worker";
        for (int n = 0; n < 80; ++n) {
            Villager elder; elder.id = identities.next_person_id++; elder.name = "Elder " + std::to_string(n); elder.age = 90; elder.wood = 1;
            const int id = elder.id; identities.villagers.push_back(elder); identities.buildings[0].contribute(id);
            identities.kill(id, "Old age"); identities.update();
        }
        check(!identities.person(6) && identities.villagers.size() <= 1 + dead_history_limit && identities.buildings[0].contributors.size() == 64, "Archive/contributor history was not bounded");
        int preserved_wood = identities.tile(32, 32).loose_wood + identities.wood_stock;
        for (const auto& p : identities.villagers) preserved_wood += p.wood;
        check(preserved_wood == 80 && identities.person(0)->relationships[6] == 10 && identities.buildings[0].initiator_name == "Former worker", "Retirement lost resources or historical identity");
        identities.population_growth = true; identities.next_migration_day = 0;
        const int fresh_id = identities.next_person_id;
        check(identities.welcome_migrant() && identities.person(fresh_id) && !identities.person(6) && identities.person(0)->relationships[fresh_id] == 0, "Newcomer reused a dead villager's identity or relationships");
        check(World::deserialize(identities.serialize()).serialize() == identities.serialize(), "Sparse identities or bounded history could not be saved");

#ifdef _LIBCPP_VERSION
        const char* fixture = "tests/fixtures/version2-libcxx.lvsave";
#else
        const char* fixture = "tests/fixtures/version2.lvsave";
#endif
        std::ifstream legacy_file(fixture); std::ostringstream legacy; legacy << legacy_file.rdbuf();
        World upgraded = World::deserialize(legacy.str());
        check(upgraded.seed == 123 && upgraded.ticks == 5000 && upgraded.simulation_speed == 4 && !upgraded.auto_develop && upgraded.gather_orders.size() == 1 && upgraded.villagers.size() == 6, "Version 2 migration lost world/settings/designations");
        check(upgraded.buildings.size() > 2 && !upgraded.buildings[2].contributors.empty() && upgraded.villagers[0].home >= 0 && upgraded.next_person_id == 6, "Migration lost construction credit or failed to initialize household identities");
        check(upgraded.serialize().find("LIVING_VILLAGE 7\n") == 0 && World::deserialize(upgraded.serialize()).serialize() == upgraded.serialize(), "Version 2 migration did not produce a valid version 7 save");
        World restored = World::deserialize(parents.serialize()); advance(parents, 24000); advance(restored, 24000);
        check(parents.serialize() == restored.serialize(), "Population save continuation diverged");

        Interface ui; InputState press; press.pressed[static_cast<int>(Button::Confirm)] = true;
        ui.view.panel = Panel::Overview; ui.view.overview_page = 1;
        check(ui.update(press, upgraded, .033) == Request::ToggleGrowth, "Controller could not toggle population growth");
        ui.view.panel = Panel::Menu; ui.view.menu_index = 5;
        check(ui.update(press, upgraded, .033) == Request::None && ui.view.panel == Panel::NewVillage, "New world skipped the confirmation panel");
        ui.view.menu_index = 5;
        check(ui.update(press, upgraded, .033) == Request::None && ui.view.panel == Panel::ConfirmCreate, "Creator skipped confirmation");
        check(ui.update(press, upgraded, .033) == Request::NewVillage, "Controller could not confirm a new village");
        press = {}; press.exit_requested = true;
        check(ui.update(press, upgraded, .033) == Request::Exit, "Fn+Start did not exit the new confirmation panel");

        World generations(20261011);
        const int days = quick ? 300 : 3500;
        for (int elapsed = 0; elapsed < days;) {
            const int batch = std::min(500, days - elapsed); advance(generations, 2400 * batch); elapsed += batch;
            std::cout << "Generations: " << elapsed << " days, " << generations.alive_count() << " alive, " << generations.births << " births, " << generations.arrivals << " arrivals, " << generations.deaths
                << " deaths, " << generations.ecology.repairs << " repairs, food " << generations.food_stock << std::endl;
        }
        check(generations.alive_count() > 0 && generations.alive_count() <= population_limit && generations.births > 0 && generations.arrivals > 0, "Long-run settlement did not sustain population growth");
        // Ecology changes demographic rates; generation replacement must not
        // depend on an arbitrary minimum number of allocated IDs in one seed.
        if (!quick) check(generations.deaths > 0 && std::none_of(generations.villagers.begin(), generations.villagers.end(), [](const Villager& p) { return p.id < 6 && p.alive(); }), "Long-run settlement did not replace its founding generation");
        check(generations.villagers.size() <= population_limit + dead_history_limit && generations.events.size() <= 24, "Population history grew without bounds");
        for (const auto& p : generations.villagers) check(p.relationships.values.size() <= 32 && p.history.size() <= 6, "Person history grew without bounds");
        check(World::deserialize(generations.serialize()).serialize() == generations.serialize(), "Multigeneration save was invalid");
        std::cout << "PASS: households, inherited traits, birth conditions/cooldowns, physical care, ageing, deaths/loot, sparse identities, bounded archives, v2 migration, controller life controls, " << days << " days (" << generations.alive_count() << " alive, " << generations.births << " births, " << generations.deaths << " deaths)\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
