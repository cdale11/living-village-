#include "living_village/world.hpp"
#include <algorithm>
#include <cmath>

namespace village {
namespace {
int work_index(Goal goal) {
    switch (goal) {
        case Goal::Forage: return 0;
        case Goal::GatherWood: return 1;
        case Goal::CollectStone: return 2;
        case Goal::TendFarm: return 3;
        case Goal::Build: return 4;
        case Goal::Care: return 5;
        case Goal::DeliverFood: case Goal::StoreMaterials: case Goal::DeliverMaterials: case Goal::RefuelFire: return 6;
        default: return -1;
    }
}
}
const char* occupation_name(Occupation occupation) {
    const char* names[] = {"Generalist", "Forager", "Woodcutter", "Stoneworker", "Farmer", "Builder", "Caregiver", "Carrier"};
    const int value = static_cast<int>(occupation);
    return value >= 0 && value <= work_types ? names[value] : "Unknown";
}
double World::work_bias(const Villager& p, Goal goal) const {
    const int index = work_index(goal); return index < 0 ? 0 : std::min(6.0, p.practice[index] * .06);
}
void World::practice_work(Villager& p, Goal goal, double amount) {
    const int index = work_index(goal);
    if (index < 0 || !std::isfinite(amount) || amount <= 0) return;
    knowledge = std::min(30.0, knowledge + amount * .02);
    p.practice[index] = std::min(100.0, p.practice[index] + amount);
    update_occupation(p);
}
void World::update_occupation(Villager& p) {
    const int best = static_cast<int>(std::max_element(p.practice.begin(), p.practice.end()) - p.practice.begin());
    Occupation next = p.practice[best] >= 8 ? static_cast<Occupation>(best + 1) : Occupation::Generalist;
    const int current = static_cast<int>(p.occupation) - 1;
    if (next == Occupation::Generalist && current >= 0 && p.practice[current] >= 4) return;
    // Require a clear change in practiced work to avoid oscillating job labels.
    if (next != Occupation::Generalist && current >= 0 && best != current && p.practice[best] < p.practice[current] * 1.35) return;
    if (next == p.occupation) return;
    const auto previous = p.occupation; p.occupation = next;
    remember(p, std::string("Work shifted: ") + occupation_name(previous) + " to " + occupation_name(next));
    event(p.name + " becomes a " + occupation_name(next));
}
} // namespace village
