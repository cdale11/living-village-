#include "living_village/world.hpp"
#include <algorithm>
#include <cmath>
namespace village {
bool World::conserved(Point point) const {
    for (Point center : conservation_zones)
        if (std::abs(center.x - point.x) <= 2 && std::abs(center.y - point.y) <= 2) return true;
    return false;
}
bool World::toggle_conservation(Point center) {
    if (!passable(center.x, center.y)) return false;
    auto found = std::find(conservation_zones.begin(), conservation_zones.end(), center);
    if (found != conservation_zones.end()) {
        const auto index = static_cast<std::size_t>(found - conservation_zones.begin());
        conservation_zones.erase(found);
        conservation_baselines.erase(conservation_baselines.begin() + index);
        event("Conservation area removed", "Player lifted natural-harvest and new-plan restrictions", center);
    } else {
        if (conservation_zones.size() >= 16) return false;
        conservation_zones.push_back(center);
        conservation_baselines.push_back(observe_zone(center));
        event("Conservation area established", "Player protects a 5x5 area; natural regrowth and wildlife continue", center);
    }
    for (auto& p : villagers) if (p.alive()) p.decision_timer = 0;
    return true;
}
ZoneObservation World::observe_zone(Point center) const {
    ZoneObservation result; result.recorded = true; result.tick = ticks; int land = 0;
    for (int y = std::max(0,center.y-2); y <= std::min(63,center.y+2); ++y)
        for (int x = std::max(0,center.x-2); x <= std::min(63,center.x+2); ++x) {
            const auto& cell = tile(x,y); if (cell.terrain == Terrain::Water) continue;
            ++land; result.vegetation += cell.biomass * cell.plant_health / 100; result.moisture += cell.moisture;
            result.trees += cell.terrain == Terrain::Tree;
        }
    result.vegetation /= std::max(1,land); result.moisture /= std::max(1,land);
    for (const auto& animal : animals) if (animal.health > 0 && std::abs(animal.x-center.x) <= 2.5 && std::abs(animal.y-center.y) <= 2.5) ++result.wildlife;
    return result;
}
std::uint64_t World::weather_cause() const {
    for (auto it = chronicle.rbegin(); it != chronicle.rend(); ++it) {
        if (ticks - it->tick >= 2400) break;
        if (weather == Weather::Storm && it->message.find("Storm:") == 0) return it->id;
    }
    return 0;
}
}
