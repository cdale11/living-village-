#include "living_village/world.hpp"
#include "living_village/ui.hpp"
#include <iostream>
#include <stdexcept>
using namespace village;
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    World w(67); w.ecology_enabled = false; w.auto_develop = false; w.population_growth = false;
    w.villagers.resize(1);
    Point source{31,31}; auto& t = w.tile(source.x, source.y); t.terrain = Terrain::Tree; t.wood = 6;
    check(w.toggle_conservation(source) && w.conserved({33,33}) && !w.conserved({34,33}), "Area bounds incorrect");
    auto& p = w.villagers.front();
    for (auto& other : w.villagers) other.decision_timer = 10;
    auto gather = [&] {
        p.brain.active = false; p.outcomes.pending_measured = false;
        p.x = source.x; p.y = source.y; p.goal = Goal::GatherWood; p.path.clear(); p.destination = source;
        p.target_id = -1; p.action_timer = 0; p.decision_timer = 5;
        for (int n = 0; n < 21; ++n) w.update();
    };
    gather(); check(p.wood == 0 && t.wood == 6, "Protected natural resources harvested");
    t.loose_wood = 1; gather(); check(p.wood == 1 && t.loose_wood == 0 && t.wood == 6, "Loose supplies blocked or duplicated");
    check(w.toggle_conservation(source), "Area cannot be removed");
    gather(); check(p.wood == 2 && t.wood == 5, "Harvest did not resume");
    Point build{36,32}; w.tile(build.x,build.y).terrain = Terrain::Grass;
    check(w.can_plan(build), "Fixture cannot build"); w.toggle_conservation(build);
    check(!w.can_plan(build) && !w.plan(BuildingKind::Farm,build), "New plans bypass conservation");
    w.record_day(); check(w.statistics.back().wildlife_recorded, "Daily wildlife observation missing");
    int recorded = 0; for (int count : w.statistics.back().wildlife) recorded += count;
    check(recorded == static_cast<int>(w.animals.size()), "Species counts wrong");
    w.weather = Weather::Storm; w.event("Storm: test weather", "Recorded weather selection");
    const auto cause = w.chronicle.back().id;
    Point fire{48,48}; auto& fuel = w.tile(fire.x,fire.y); fuel.terrain = Terrain::Grass; fuel.moisture = .05; fuel.biomass = .8;
    check(w.ignite(fire,cause), "Dry fuel did not ignite");
    check(w.chronicle.back().parent == cause && w.chronicle.back().location == fire && !w.chronicle.back().evidence.empty(), "Ignition evidence missing");
    const auto data = w.serialize(); auto restored = World::deserialize(data);
    check(restored.serialize() == data, "Emergence save mismatch");
    for (int n = 0; n < 100; ++n) { w.update(); restored.update(); }
    check(w.serialize() == restored.serialize(), "Emergence continuation diverged");
    auto legacy = data.substr(0, data.find("EMERGENCE 1 "));
    legacy.replace(0, legacy.find('\n') + 1, "LIVING_VILLAGE 8\n");
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : legacy) { hash ^= c; hash *= 1099511628211ULL; }
    auto migrated = World::deserialize(legacy + "CHECKSUM " + std::to_string(hash) + "\n");
    check(migrated.conservation_zones.empty() && !migrated.statistics.back().wildlife_recorded && migrated.chronicle.back().evidence.empty(), "Migration invented records");
    World bounded(9);
    for (int n = 0; n < 16; ++n) { Point center{n+2,2}; bounded.tile(center.x,center.y) = Tile{}; check(bounded.toggle_conservation(center), "Area limit reached early"); }
    check(!bounded.toggle_conservation({32,32}), "Area cap exceeded");
    for (int n = 0; n < 600; ++n) bounded.event("Bounded history", "Recorded fixture");
    check(bounded.chronicle.size() == 512 && World::deserialize(bounded.serialize()).serialize() == bounded.serialize(), "Bounded records failed persistence");
    Interface trace; trace.view.panel = Panel::Overview; trace.view.overview_page = 2;
    InputState confirm; confirm.pressed[static_cast<int>(Button::Confirm)] = true;
    trace.update(confirm,w,.033);
    check(trace.view.history_offset > 0 && w.chronicle[w.chronicle.size()-1-trace.view.history_offset].id == cause, "Cannot trace contributing event");
    Interface ui; ui.view.panel = Panel::Tile; InputState press; press.pressed[static_cast<int>(Button::Overview)] = true;
    check(ui.update(press,w,.033) == Request::Conservation, "Controller cannot protect area");
    ui.view.panel = Panel::Overview; ui.view.overview_page = 2; press = {}; press.y = -1;
    ui.update(press,w,.033); check(ui.view.history_offset == 1, "Event inspector cannot navigate");
    std::cout << "Conservation, physical harvesting, wildlife records, causal evidence, migration and controller inspection passed\n";
}
