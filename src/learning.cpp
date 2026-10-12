#include "living_village/world.hpp"
#include <algorithm>
#include <cmath>

namespace village {
BrainInput World::observations(const Villager& p, Goal goal, Point destination, double utility) const {
    BrainInput input{};
    input[static_cast<int>(goal)] = 1;
    int wood_need = 0, stone_need = 0;
    for (const auto& b : buildings) if (!b.complete) { wood_need += b.wood_needed(); stone_need += b.stone_needed(); }
    const double trip = std::abs(p.x - destination.x) + std::abs(p.y - destination.y);
    const std::array<double, brain_context> context{{
        p.hunger / 100, p.energy / 100, p.health / 100, p.loneliness / 100,
        p.cooperation, p.curiosity, p.family_drive, p.gather_skill, p.build_skill, p.farm_skill,
        p.food / 4.0, (p.wood + p.stone) / 6.0,
        food_stock / static_cast<double>(std::max(1, alive_count()) * 4),
        wood_need / 24.0, stone_need / 12.0,
        std::max(0, shelter_capacity() - alive_count()) / 8.0, trip / 64.0, utility / 300.0
    }};
    for (int i = 0; i < brain_context; ++i) input[brain_actions + i] = std::clamp(context[i], 0.0, 1.0);
    return input;
}
void World::learn(Villager& p, double terminal_reward) {
    auto& brain = p.brain;
    if (!brain.active) return;
    const double outcome = brain.progress + terminal_reward
        + .6 * (brain.start_hunger - p.hunger) / 45
        + .4 * (p.energy - brain.start_energy) / 40
        + .2 * (brain.start_loneliness - p.loneliness) / 45
        + .8 * (p.health - brain.start_health) / 20
        - std::min(.6, brain.elapsed * .003);
    if (learning_enabled) brain.train(brain.pending, outcome);
    brain.active = false;
}
void World::begin_learning(Villager& p, const BrainInput& input, double base, double adjustment) {
    auto& brain = p.brain;
    brain.pending = input; brain.active = true; brain.action = static_cast<int>(p.goal);
    brain.elapsed = brain.progress = 0;
    brain.start_hunger = p.hunger; brain.start_energy = p.energy;
    brain.start_health = p.health; brain.start_loneliness = p.loneliness;
    brain.selected_base = base; brain.selected_adjustment = adjustment;
}
} // namespace village
