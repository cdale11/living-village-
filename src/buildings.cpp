#include "living_village/world.hpp"
#include <algorithm>

namespace village {
BuildingCost building_cost(BuildingKind kind) {
    switch (kind) {
        case BuildingKind::Workshop: return {12, 8, 28};
        case BuildingKind::Shelter: return {12, 4, 24};
        case BuildingKind::FoodStorage: return {10, 2, 18};
        case BuildingKind::Stockpile: return {8, 4, 16};
        case BuildingKind::Farm: return {6, 2, 12};
        case BuildingKind::Campfire: return {4, 2, 8};
    }
    return {0, 0, 0};
}
const char* building_name(BuildingKind kind) {
    switch (kind) {
        case BuildingKind::Workshop: return "Workshop";
        case BuildingKind::Shelter: return "Shelter";
        case BuildingKind::FoodStorage: return "Food storage";
        case BuildingKind::Stockpile: return "Stockpile";
        case BuildingKind::Farm: return "Farm plot";
        case BuildingKind::Campfire: return "Campfire";
    }
    return "Unknown";
}
int Building::wood_needed() const { return complete ? 0 : std::max(0, building_cost(kind).wood - wood_delivered); }
int Building::stone_needed() const { return complete ? 0 : std::max(0, building_cost(kind).stone - stone_delivered); }
bool Building::supplied() const { return wood_needed() == 0 && stone_needed() == 0; }
int World::food_capacity() const {
    int capacity = 30;
    for (const auto& b : buildings) if (b.complete && b.condition > 20 && b.kind == BuildingKind::FoodStorage) capacity += 40;
    return capacity;
}
int World::material_capacity() const {
    int capacity = 12;
    for (const auto& b : buildings) if (b.complete && b.condition > 20 && b.kind == BuildingKind::Stockpile) capacity += 20;
    return capacity;
}
int World::shelter_capacity() const {
    int capacity = 0;
    for (const auto& b : buildings) if (b.complete && b.condition > 20 && b.kind == BuildingKind::Shelter) capacity += 4;
    return capacity;
}
int World::alive_count() const { return static_cast<int>(std::count_if(villagers.begin(), villagers.end(), [](const Villager& p) { return p.alive(); })); }
int World::building_at(Point point) const {
    for (const auto& b : buildings) if (b.position == point) return b.id;
    return -1;
}
bool World::can_plan(Point point) const {
    if (!passable(point.x, point.y) || tile(point.x, point.y).terrain != Terrain::Grass || point == camp || building_at(point) >= 0 || buildings.size() >= 64) return false;
    return !route(camp, point).empty();
}
bool World::plan(BuildingKind kind, Point point, int initiator) {
    if (static_cast<int>(kind) < 0 || static_cast<int>(kind) > 5 || initiator < -1 || (initiator >= 0 && !person(initiator)) || !can_plan(point)) return false;
    Building site;
    site.id = static_cast<int>(buildings.size()); site.kind = kind; site.position = point; site.initiator = initiator;
    if (initiator >= 0) site.initiator_name = person_name(initiator);
    buildings.push_back(site);
    event(std::string("New plan: ") + building_name(kind));
    return true;
}
bool World::designate(Point point) {
    if (!passable(point.x, point.y)) return false;
    const auto terrain = tile(point.x, point.y).terrain;
    if (terrain != Terrain::Tree && terrain != Terrain::Stone && terrain != Terrain::Bush) return false;
    auto existing = std::find(gather_orders.begin(), gather_orders.end(), point);
    if (existing != gather_orders.end()) { gather_orders.erase(existing); return true; }
    if (gather_orders.size() >= 16) return false;
    gather_orders.push_back(point);
    event("Gathering area designated"); return true;
}
void World::update_needs() {
    if (!food_shortage && food_stock < 4) { food_shortage = true; event("Shared food is running low"); }
    else if (food_shortage && food_stock >= 12) { food_shortage = false; event("Food stocks have recovered"); }
    if (!auto_develop || alive_count() == 0) return;
    int pending = 0, future_shelter = shelter_capacity(), farms = 0, stores = 0;
    for (const auto& b : buildings) {
        if (!b.complete) { ++pending; if (b.kind == BuildingKind::Shelter) future_shelter += 4; }
        if (b.kind == BuildingKind::Farm) ++farms;
        if (b.kind == BuildingKind::FoodStorage) ++stores;
    }
    if (pending >= 2) return;
    const int wanted_shelter = std::min(population_limit, alive_count() + (population_growth && food_stock >= std::max(12, alive_count() * 2) ? 4 : 0));
    BuildingKind needed;
    if (food_stock < 12 && farms < (alive_count() + 2) / 3) needed = BuildingKind::Farm;
    else if (future_shelter < wanted_shelter) needed = BuildingKind::Shelter;
    else if (food_stock >= food_capacity() * 3 / 4 && stores == 0) needed = BuildingKind::FoodStorage;
    else if (knowledge >= 8 && food_stock >= alive_count() * 2 && std::none_of(buildings.begin(), buildings.end(), [](const Building& b) { return b.kind == BuildingKind::Workshop; })) needed = BuildingKind::Workshop;
    else return;
    // Search a bounded ring around camp; request arises from current needs.
    for (int radius = 2; radius <= 6; ++radius)
        for (int y = camp.y - radius; y <= camp.y + radius; ++y)
            for (int x = camp.x - radius; x <= camp.x + radius; ++x)
                if (std::max(std::abs(x - camp.x), std::abs(y - camp.y)) == radius && can_plan({x, y})) {
                    plan(needed, {x, y}); return;
                }
}
double World::growing_rate() const {
    const double seasonal[] = {1.0, 1.2, 0.9, 0.5};
    const double climate[] = {1.0, 1.1, 1.05, .65};
    return seasonal[season()] * (raining() ? 1.15 : 1.0) * climate[static_cast<int>(biome)];
}
const char* World::season_name() const {
    const char* names[] = {"Spring", "Summer", "Autumn", "Winter"}; return names[season()];
}
} // namespace village
