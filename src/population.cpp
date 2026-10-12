#include "living_village/world.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace village {
double Relationships::operator[](int id) const {
    auto found = values.find(id); return found == values.end() ? 0 : found->second;
}
double& Relationships::operator[](int id) {
    if (values.find(id) == values.end() && values.size() >= 32) {
        auto weakest = std::min_element(values.begin(), values.end(), [](const auto& a, const auto& b) { return std::abs(a.second) < std::abs(b.second); });
        values.erase(weakest);
    }
    return values[id];
}
void Building::contribute(int id) {
    if (contributors.size() < 64 && std::find(contributors.begin(), contributors.end(), id) == contributors.end()) {
        contributors.push_back(id); std::sort(contributors.begin(), contributors.end());
    }
}
Villager* World::person(int id) {
    for (auto& p : villagers) if (p.id == id) return &p;
    return nullptr;
}
const Villager* World::person(int id) const {
    for (const auto& p : villagers) if (p.id == id) return &p;
    return nullptr;
}
std::string World::person_name(int id) const {
    const auto* p = person(id); return p ? p->name : "Earlier resident";
}
int World::child_count() const {
    return static_cast<int>(std::count_if(villagers.begin(), villagers.end(), [](const Villager& p) { return p.alive() && p.child(); }));
}
int World::home_occupancy(int home) const {
    if (home < 0) return 0;
    return static_cast<int>(std::count_if(villagers.begin(), villagers.end(), [home](const Villager& p) { return p.alive() && p.home == home; }));
}
void World::update_homes() {
    for (auto& p : villagers) {
        if (!p.alive()) continue;
        if (!p.child() && p.parents[0] >= 0 && p.partner < 0 && p.home >= 0) {
            const auto* parent = person(p.parents[0]);
            if (parent && parent->alive() && parent->home == p.home && home_occupancy(p.home) >= 4) {
                for (const auto& b : buildings) if (b.complete && b.kind == BuildingKind::Shelter && b.id != p.home && home_occupancy(b.id) < 3) { p.home = b.id; remember(p, "Moved to a home of my own"); break; }
            }
        }
        if (p.home >= 0 && p.home < static_cast<int>(buildings.size()) && buildings[p.home].complete && buildings[p.home].kind == BuildingKind::Shelter) continue;
        p.home = -1;
        const auto* partner = person(p.partner);
        if (partner && partner->alive() && partner->home >= 0 && home_occupancy(partner->home) < 4) { p.home = partner->home; continue; }
        for (const auto& b : buildings) if (b.complete && b.kind == BuildingKind::Shelter && home_occupancy(b.id) < 4) { p.home = b.id; break; }
    }
    // Couples can move into a spare home, freeing room to raise children.
    for (auto& p : villagers) {
        auto* partner = person(p.partner);
        if (!p.alive() || !partner || !partner->alive() || p.id > partner->id) continue;
        if (p.home == partner->home && p.home >= 0 && home_occupancy(p.home) < 4) continue;
        bool dependents = false;
        for (const auto& child : villagers) if (child.alive() && child.child() && (child.parents[0] == p.id || child.parents[1] == p.id)) dependents = true;
        if (dependents) continue;
        for (const auto& b : buildings) if (b.complete && b.kind == BuildingKind::Shelter && home_occupancy(b.id) <= 1) {
            p.home = partner->home = b.id; break;
        }
    }
}
bool World::close_family(int first, int second) const {
    auto ancestors = [&](int id) {
        std::vector<int> result{id};
        for (int level = 0, begin = 0, end = 1; level < 2; ++level) {
            for (int i = begin; i < end; ++i) if (const auto* p = person(result[i]))
                for (int parent : p->parents) if (parent >= 0 && std::find(result.begin(), result.end(), parent) == result.end()) result.push_back(parent);
            begin = end; end = static_cast<int>(result.size());
        }
        return result;
    };
    const auto a = ancestors(first), b = ancestors(second);
    for (int id : a) if (std::find(b.begin(), b.end(), id) != b.end()) return true;
    return false;
}
bool World::form_household(int first, int second) {
    auto* a = person(first); auto* b = person(second);
    if (!a || !b || first == second || !a->alive() || !b->alive() || a->age < 18 || b->age < 18 || a->partner >= 0 || b->partner >= 0
        || a->family_drive < .3 || b->family_drive < .3 || a->relationships[second] < 45 || b->relationships[first] < 45 || close_family(first, second)) return false;
    int home = -1;
    for (const auto& house : buildings) if (house.complete && house.kind == BuildingKind::Shelter) {
        const int already_here = (a->home == house.id ? 1 : 0) + (b->home == house.id ? 1 : 0);
        if (home_occupancy(house.id) - already_here <= 2) { home = house.id; break; }
    }
    if (home < 0) return false;
    a->partner = second; b->partner = first; a->home = b->home = home;
    remember(*a, "Formed a household with " + b->name); remember(*b, "Formed a household with " + a->name);
    event(a->name + " and " + b->name + " form a household"); return true;
}
std::string World::new_name(int id) const {
    const char* names[] = {"Cora", "Lio", "Sana", "Elin", "Jori", "Ada", "Finn", "Rhea", "Ivo", "Una", "Kian", "Zia", "Rumi", "Esra", "Noel", "Tavi", "Lena", "Ari", "Theo", "Nala", "Eli", "Suri", "Milo", "Anya"};
    const int offset = std::max(0, id - 6);
    return std::string(names[offset % 24]) + (offset < 24 ? "" : " " + std::to_string(offset / 24 + 1));
}
bool World::have_child(int first, int second) {
    auto* a = person(first); auto* b = person(second);
    if (!population_growth || next_person_id >= 1000000000 || !a || !b || first == second || !a->alive() || !b->alive()
        || a->partner != second || b->partner != first || a->age < 18 || b->age < 18 || a->age > 45 || b->age > 45
        || a->health < 75 || b->health < 75 || a->hunger > 50 || b->hunger > 50 || close_family(first, second)
        || alive_count() >= population_limit || shelter_capacity() <= alive_count() || day() < next_birth_day || day() - a->last_birth_day < 80 || day() - b->last_birth_day < 80
        || a->home < 0 || a->home >= static_cast<int>(buildings.size()) || a->home != b->home || home_occupancy(a->home) >= 4 || food_stock < std::max(12, (alive_count() + 1) * 2)) return false;
    const auto& house = buildings[a->home];
    if (!house.complete || house.kind != BuildingKind::Shelter) return false;
    Villager baby;
    baby.id = next_person_id++; baby.brain.initialize(seed, baby.id); baby.name = new_name(baby.id); baby.age = 0; baby.hunger = 0; baby.home = house.id;
    baby.x = house.position.x; baby.y = house.position.y; baby.destination = house.position;
    baby.parents = {first, second}; baby.parent_names = {a->name, b->name};
    auto inherit = [&](double av, double bv) { return std::clamp((av + bv) / 2 + (random(21) - 10) / 100.0, .2, .95); };
    baby.cooperation = inherit(a->cooperation, b->cooperation); baby.curiosity = inherit(a->curiosity, b->curiosity); baby.family_drive = inherit(a->family_drive, b->family_drive);
    baby.relationships[first] = baby.relationships[second] = 60;
    a->relationships[baby.id] = b->relationships[baby.id] = 80;
    a->last_birth_day = b->last_birth_day = day();
    a->energy = std::max(0.0, a->energy - 8); b->energy = std::max(0.0, b->energy - 8);
    remember(*a, baby.name + " joined our family"); remember(*b, baby.name + " joined our family");
    food_stock -= 2; ++births; next_birth_day = day() + 4;
    villagers.push_back(baby); event(baby.name + " is born to " + baby.parent_names[0] + " and " + baby.parent_names[1]);
    prune_history(); return true;
}
std::string World::growth_status() const {
    if (!population_growth) return "New births and arrivals paused";
    if (alive_count() == 0) return "Village has no surviving residents";
    if (alive_count() >= population_limit) return "Population limit of 20 reached";
    if (shelter_capacity() <= alive_count()) return "Needs spare shelter";
    if (food_stock < std::max(12, (alive_count() + 1) * 3)) return "Needs a shared food reserve";
    for (const auto& p : villagers) if (p.alive() && (p.hunger > 65 || p.health < 60)) return "Residents need better health and food";
    return "Food and housing support newcomers";
}
bool World::supports_newcomer() const {
    if (!population_growth || alive_count() == 0 || alive_count() >= population_limit || shelter_capacity() <= alive_count() || food_stock < std::max(12, (alive_count() + 1) * 3)) return false;
    for (const auto& p : villagers) if (p.alive() && (p.hunger > 65 || p.health < 60)) return false;
    return true;
}
bool World::welcome_migrant() {
    if (!supports_newcomer() || next_person_id >= 1000000000 || day() < next_migration_day) return false;
    Point entry{-1, -1}; std::vector<Point> path;
    // Bounded choices along the world edge. Newcomers walk into the village.
    const int offset = random(64);
    for (int attempt = 0; attempt < 64; ++attempt) {
        const int coordinate = (offset + attempt) % 64;
        const Point candidates[] = {{coordinate, 0}, {63, coordinate}, {coordinate, 63}, {0, coordinate}};
        entry = candidates[(seed + static_cast<unsigned>(attempt)) % 4]; path = route(entry, camp);
        if (!path.empty()) break;
    }
    if (path.empty()) return false;
    int home = -1, best = std::numeric_limits<int>::max();
    for (const auto& b : buildings) if (b.complete && b.kind == BuildingKind::Shelter && home_occupancy(b.id) < 4) {
        int score = home_occupancy(b.id);
        for (const auto& p : villagers) if (p.alive() && p.home == b.id && p.partner >= 0) score += 10;
        if (score < best) { best = score; home = b.id; }
    }
    if (home < 0) return false;
    Villager newcomer;
    newcomer.id = next_person_id++; newcomer.brain.initialize(seed, newcomer.id); newcomer.name = new_name(newcomer.id); newcomer.age = 18 + random(18);
    newcomer.home = home; newcomer.x = entry.x; newcomer.y = entry.y; newcomer.path = path; newcomer.destination = camp;
    newcomer.goal = Goal::Arrive; newcomer.reason = "Village has spare shelter and food"; newcomer.hunger = 10; newcomer.energy = 75;
    newcomer.curiosity = .3 + random(60) / 100.0; newcomer.cooperation = .35 + random(60) / 100.0; newcomer.family_drive = .35 + random(60) / 100.0;
    newcomer.gather_skill = random(16) / 100.0; newcomer.build_skill = random(11) / 100.0;
    remember(newcomer, "Joined a village with spare shelter");
    begin_learning(newcomer, observations(newcomer, Goal::Arrive, camp, 0), 0, 0);
    villagers.push_back(newcomer); ++arrivals; next_migration_day = day() + 12;
    event(newcomer.name + " arrives seeking a home"); prune_history(); return true;
}
bool World::kill(int id, const std::string& reason) {
    auto* p = person(id); if (!p || p->death_day != 0) return false;
    const Point at{static_cast<int>(std::round(p->x)), static_cast<int>(std::round(p->y))};
    auto& ground = tile(at.x, at.y); ground.loose_food += p->food; ground.loose_wood += p->wood; ground.loose_stone += p->stone;
    p->health = 0; learn(*p, -1);
    p->food = p->wood = p->stone = 0; p->death_day = day(); p->death_reason = reason; p->path.clear();
    ++deaths; remember(*p, "Died: " + reason); event(p->name + " died: " + reason);
    const std::string name = p->name;
    for (auto& other : villagers) if (other.alive()) {
        if (other.partner == id) { other.partner = -1; other.loneliness = std::min(100.0, other.loneliness + 20); remember(other, "Lost my partner " + name); }
        else if (other.parents[0] == id || other.parents[1] == id) { other.loneliness = std::min(100.0, other.loneliness + 12); remember(other, "Lost my parent " + name); }
    }
    return true;
}
void World::prune_history() {
    int dead = static_cast<int>(villagers.size()) - alive_count();
    while (dead > dead_history_limit) {
        auto oldest = villagers.end();
        for (auto it = villagers.begin(); it != villagers.end(); ++it)
            if (!it->alive() && (oldest == villagers.end() || it->death_day < oldest->death_day || (it->death_day == oldest->death_day && it->id < oldest->id))) oldest = it;
        if (oldest == villagers.end()) break;
        villagers.erase(oldest); --dead;
    }
}
void World::update_population() {
    update_homes();
    for (auto& p : villagers) if (p.alive()) {
        for (double& practice : p.practice) practice *= .98;
        update_occupation(p);
        if (++p.age_days >= days_per_year) {
            p.age_days = 0; ++p.age;
            if (p.age == 16) { remember(p, "Grew old enough for village work"); event(p.name + " reaches working age"); }
        }
        if (p.age >= 90 || (p.age >= 70 && random(100000) < (p.age - 70) * 3)) kill(p.id, "Old age");
    }
    if (population_growth && alive_count() > 0 && alive_count() < population_limit) {
        if (day() >= next_birth_day) {
            // Births are opportunities conditioned on a real household and economy.
            std::vector<std::pair<int, int>> candidates;
            for (const auto& p : villagers) if (p.alive() && p.partner > p.id) candidates.push_back({p.id, p.partner});
            for (const auto& pair : candidates) {
                const auto* a = person(pair.first); const auto* b = person(pair.second);
                if (a && b && random(100) < 4 + static_cast<int>((a->family_drive + b->family_drive) * 4) && have_child(pair.first, pair.second)) break;
            }
        }
        if (day() >= next_migration_day && random(4) == 0) welcome_migrant();
    }
    prune_history();
}
} // namespace village
