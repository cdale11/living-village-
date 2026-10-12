#include "living_village/world.hpp"
#include "living_village/ui.hpp"
#include <stdexcept>
#include <iostream>
using namespace village;
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
int main() {
    World w(90); w.auto_develop = false;
    auto& teacher = w.villagers[0]; auto& student = w.villagers[1];
    teacher.gather_skill = .8; student.gather_skill = .1; teacher.cooperation = 1;
    teacher.hunger = student.hunger = 10; teacher.energy = student.energy = 95;
    teacher.x = 30; student.x = 40;
    check(!w.teach(teacher,student), "Remote teaching occurred");
    student.x = teacher.x; student.y = teacher.y;
    check(w.teach(teacher,student) && student.gather_skill > .1 && teacher.energy < 95, "Physical teaching absent");
    check(!w.teach(teacher,student) && w.lessons == 1 && student.life.size() == 1, "Daily lesson cap failed");
    check(w.toggle_pin(teacher.id), "Pin failed");
    w.toggle_conservation({39,32}); check(w.conservation_baselines[0].recorded, "Protection baseline missing");
    const auto data = w.serialize(); auto loaded = World::deserialize(data);
    check(loaded.serialize() == data, "Lives/policies/metrics save mismatch");
    for (int n = 0; n < 2000; ++n) { w.update(); loaded.update(); }
    check(w.serialize() == loaded.serialize(), "Saved continuation diverged");
    World frozen(51); frozen.learning_enabled = false; frozen.exploration_enabled = false;
    for (int n = 0; n < 5000; ++n) frozen.update();
    std::uint64_t observed = 0, gathered = 0;
    for (const auto& p : frozen.villagers) { check(p.brain.updates == 0, "Frozen model trained"); for (auto n : p.outcomes.intervals) observed += n; for (auto n : p.outcomes.units) gathered += n; }
    check(observed > 0 && gathered > 0, "Frozen execution outcomes were not observed");
    World housing(71); housing.food_stock = 0; housing.construction_priority = 2; housing.update();
    check(housing.buildings.size() == 3 && housing.buildings.back().kind == BuildingKind::Shelter, "Housing priority did not influence planning");
    World maintenance(71); maintenance.construction_priority = 3; maintenance.buildings[0].condition = 30; maintenance.update();
    check(maintenance.buildings.size() == 2, "Maintenance priority planned new work while repairs pending");
    World pins(43); check(pins.toggle_pin(0) && pins.kill(0,"Fixture"), "Cannot pin deceased history");
    for (int n = 0; n < 34; ++n) { Villager p; p.id = pins.next_person_id++; p.name = "Archive " + std::to_string(p.id); p.health = 0; p.death_day = 1; p.death_reason = "Fixture"; pins.villagers.push_back(p); ++pins.deaths; }
    pins.update(); check(pins.person(0) && !pins.person(0)->alive() && static_cast<int>(pins.villagers.size())-pins.alive_count() == dead_history_limit, "Pinned archive not retained within cap");
    check(World::deserialize(pins.serialize()).pinned_residents[0] == 0, "Pinned archive cannot restore");
    auto legacy = data.substr(0,data.find("LIVES 1 ")); legacy.replace(0,legacy.find('\n')+1,"LIVING_VILLAGE 9\n");
    std::uint64_t hash = 14695981039346656037ULL; for (unsigned char c : legacy) { hash ^= c; hash *= 1099511628211ULL; }
    auto migrated = World::deserialize(legacy + "CHECKSUM " + std::to_string(hash) + "\n");
    check(migrated.pinned_residents.empty() && migrated.villagers[0].life.empty() && !migrated.conservation_baselines[0].recorded, "Migration invented history/baselines");
    Interface ui; ui.view.panel = Panel::Inspector; ui.view.inspector_page = 6;
    InputState press; press.pressed[static_cast<int>(Button::Confirm)] = true;
    check(ui.update(press,w,.03) == Request::Pin, "Controller pin failed");
    ui.view.panel = Panel::Overview; ui.view.overview_page = 12;
    check(ui.update(press,w,.03) == Request::FoodReserve, "Policy controller failed");
    ui.view.overview_page = 11; ui.update(press,w,.03);
    check(ui.view.ecology_overlay == 4 && ui.view.panel == Panel::Map, "Social overlay failed");
    w.food_reserve = 36; check(w.food_target() <= w.food_capacity(), "Policy target exceeds storage");
    std::cout << "Lives, pins/bounded archives, physical teaching, frozen outcome metrics, policies, migration and overlays passed\n";
}
