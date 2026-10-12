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
        conservation_zones.erase(found);
        event("Conservation area removed", "Player lifted natural-harvest and new-plan restrictions", center);
    } else {
        if (conservation_zones.size() >= 16) return false;
        conservation_zones.push_back(center);
        event("Conservation area established", "Player protects a 5x5 area; natural regrowth and wildlife continue", center);
    }
    for (auto& p : villagers) if (p.alive()) p.decision_timer = 0;
    return true;
}
std::uint64_t World::weather_cause() const {
    for (auto it = chronicle.rbegin(); it != chronicle.rend(); ++it) {
        if (ticks - it->tick >= 2400) break;
        if (weather == Weather::Storm && it->message.find("Storm:") == 0) return it->id;
    }
    return 0;
}
}
