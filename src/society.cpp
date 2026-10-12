#include "living_village/world.hpp"
#include <algorithm>
#include <cmath>

namespace village {
namespace {
int distance(const Villager& a, const Villager& b) { return static_cast<int>(std::round(std::abs(a.x - b.x) + std::abs(a.y - b.y))); }
}
double World::social_exchange(Villager& p, Villager& other) {
    double reward = 0;
    // Gifts arise from observed hunger and spare carried meals, never shared teleportation.
    if (p.food > 1 && other.food < 4 && other.hunger >= 60 && p.hunger < 70 && (p.cooperation >= .5 || p.relationships[other.id] >= 20)) {
        --p.food; ++other.food; ++help_count; reward += .4 * p.cooperation;
        other.decision_timer = 0;
        p.relationships[other.id] = std::min(100.0, p.relationships[other.id] + 2);
        other.relationships[p.id] = std::min(100.0, other.relationships[p.id] + 5);
        remember(p, "Shared a meal with hungry " + other.name); remember(other, p.name + " helped me through hunger");
        event(p.name + " brings " + other.name + " a spare meal"); practice_work(p, Goal::DeliverFood, 1);
    } else if (!p.child() && !other.child() && p.food >= 3 && other.food <= 1 && other.wood + other.stone >= 2 && p.wood + p.stone < 6 && p.relationships[other.id] > -20 && (wood_stock < 3 || stone_stock < 2)) {
        // Two surplus meals for one carried material; both sides actually retain it.
        p.food -= 2; other.food += 2;
        const bool stone = other.stone > 0 && stone_stock < 2;
        if (stone) { --other.stone; ++p.stone; } else if (other.wood > 0) { --other.wood; ++p.wood; } else { --other.stone; ++p.stone; }
        ++trade_count; reward += .25;
        p.relationships[other.id] = std::min(100.0, p.relationships[other.id] + 2);
        other.relationships[p.id] = std::min(100.0, other.relationships[p.id] + 2);
        remember(p, "Exchanged two meals for a needed material with " + other.name);
        remember(other, "Traded a spare material for food with " + p.name);
        event(p.name + " and " + other.name + " exchange food and materials");
    }
    if (p.relationships[other.id] < 0 && p.hunger < 70 && other.hunger < 70 && p.energy >= 30 && other.energy >= 30 && (p.cooperation + other.cooperation) / 2 >= .4) {
        p.relationships[other.id] = std::min(100.0, p.relationships[other.id] + 5);
        other.relationships[p.id] = std::min(100.0, other.relationships[p.id] + 5);
        ++reconciliations; reward += .2;
        remember(p, "Repaired trust with " + other.name); remember(other, "Repaired trust with " + p.name);
        event(p.name + " and " + other.name + " work on reconciliation");
    }
    return reward;
}
void World::resource_conflict(Villager& p) {
    if (p.hunger < 60 || p.cooperation >= .5 || p.last_conflict_day == day()) return;
    for (auto& other : villagers) {
        const int carried = p.goal == Goal::Forage ? other.food : p.goal == Goal::GatherWood ? other.wood : other.stone;
        if (other.id == p.id || !other.alive() || other.goal != p.goal || carried == 0 || distance(p, other) > 1) continue;
        p.relationships[other.id] = std::max(-100.0, p.relationships[other.id] - 8);
        other.relationships[p.id] = std::max(-100.0, other.relationships[p.id] - 3);
        p.loneliness = std::min(100.0, p.loneliness + 5); p.last_conflict_day = day(); ++conflicts;
        remember(p, "Disputed a depleted resource with " + other.name); remember(other, p.name + " resented my resource claim");
        event(p.name + " and " + other.name + " dispute scarce supplies"); break;
    }
}
BuildingCost technology_cost(int level) { return level == 0 ? BuildingCost{4, 2, 20} : BuildingCost{6, 4, 30}; }
bool World::research_available() const { return technology_level < 2 && knowledge >= (technology_level == 0 ? 8 : 18); }
int World::research_wood_needed(const Building& b) const { return research_available() ? std::max(0, technology_cost(technology_level).wood - b.upgrade_wood) : 0; }
int World::research_stone_needed(const Building& b) const { return research_available() ? std::max(0, technology_cost(technology_level).stone - b.upgrade_stone) : 0; }
bool World::research(Villager& p, Building& b, double dt) {
    if (!std::isfinite(dt) || dt <= 0 || dt > 1 || !b.complete || b.kind != BuildingKind::Workshop || !research_available() || research_wood_needed(b) || research_stone_needed(b)
        || std::abs(p.x - b.position.x) + std::abs(p.y - b.position.y) > .5) return false;
    b.upgrade_work += dt * (.75 + p.build_skill);
    practice_work(p, Goal::Build, dt / 2); p.build_skill = std::min(1.0, p.build_skill + dt * .0005);
    if (p.brain.active) p.brain.progress = std::min(20.0, p.brain.progress + dt * .03);
    if (b.upgrade_work >= technology_cost(technology_level).work) {
        const auto invested = technology_cost(technology_level);
        auto& surplus = tile(b.position.x, b.position.y);
        surplus.loose_wood += std::max(0, b.upgrade_wood - invested.wood); surplus.loose_stone += std::max(0, b.upgrade_stone - invested.stone);
        ++technology_level; b.upgrade_wood = b.upgrade_stone = 0; b.upgrade_work = 0;
        remember(p, technology_level == 1 ? "Developed improved hand tools" : "Developed farm irrigation");
        event(technology_level == 1 ? "Improved tools: gathering and building 10 percent faster" : "Irrigation: farm growth 15 percent faster");
        // Only one communal upgrade is active. Extra staged supplies remain recoverable.
        for (auto& other : buildings) if (other.kind == BuildingKind::Workshop && other.id != b.id) {
            auto& ground = tile(other.position.x, other.position.y);
            ground.loose_wood += other.upgrade_wood; ground.loose_stone += other.upgrade_stone;
            other.upgrade_wood = other.upgrade_stone = 0; other.upgrade_work = 0;
        }
    }
    return true;
}
void World::record_day() {
    DailyRecord record; record.day = day(); record.people = alive_count(); record.children = child_count();
    record.food = food_stock; record.wood = wood_stock; record.stone = stone_stock; record.technology = technology_level;
    record.births = births; record.deaths = deaths; record.trades = trade_count;
    for (const auto& b : buildings) if (b.complete) ++record.buildings;
    if (!statistics.empty() && statistics.back().day == record.day) statistics.back() = record;
    else { if (statistics.size() >= 128) statistics.pop_front(); statistics.push_back(record); }
}
} // namespace village
