#include "living_village/world.hpp"
#include <algorithm>
#include <cmath>
namespace village {
const char* Director::mode_name() const {
    return mode == DirectorMode::Off ? "OFF" : mode == DirectorMode::Observer ? "OBSERVER" : "ADAPTIVE";
}
const char* Director::action_name() const {
    return action == 1 ? "Favor wet transition" : action == 2 ? "Favor dry transition" : "Leave conditions alone";
}
namespace {
struct Snapshot { double welfare = 0, diversity = 0, moisture = 0, food = 0; int population = 0; };
Snapshot snapshot(const World& w) {
    Snapshot s; s.population = w.alive_count();
    std::array<bool, brain_actions> activities{};
    for (const auto& p : w.villagers) if (p.alive()) {
        s.welfare += (p.health + p.energy + 100 - p.hunger) / 300.;
        activities[static_cast<int>(p.goal)] = true;
    }
    s.welfare /= std::max(1, s.population);
    for (bool active : activities) s.diversity += active ? 1. / brain_actions : 0;
    for (const auto& t : w.tiles) s.moisture += t.moisture / w.tiles.size();
    int food = w.food_stock;
    for (const auto& p : w.villagers) if (p.alive()) food += p.food;
    for (const auto& b : w.buildings) food += b.food;
    s.food = std::clamp(food / (6. * std::max(1, s.population)), 0., 1.);
    return s;
}
double quality(const Snapshot& s) { return .65 * s.welfare + .25 * s.food + .1 * s.diversity; }
}
void World::cycle_director() {
    director.mode = static_cast<DirectorMode>((static_cast<int>(director.mode) + 1) % 3);
    director.pending = false; director.bias = 0; director.brain.active = false;
    director.reason = "Mode changed; pending evaluation cancelled";
    event(std::string("Director mode: ") + director.mode_name());
}
void World::update_director() {
    auto& d = director;
    if (d.mode == DirectorMode::Off || !ecology_enabled) {
        d.bias = 0; d.pending = false; d.brain.active = false; return;
    }
    const auto s = snapshot(*this);
    if (d.pending && ticks - d.pending_tick < 7200) { d.bias = 0; return; }
    if (d.pending) {
        // Association over a three-day window, not a causal proof.
        d.outcome = std::clamp(quality(s) - d.baseline
            - .15 * std::max(0, d.baseline_population - s.population), -1., 1.);
        if (learning_enabled) d.brain.observe(d.brain.pending, d.outcome);
        d.pending = false; d.brain.active = false;
    }
    BrainInput input{};
    input[brain_actions] = s.welfare; input[brain_actions + 1] = s.food;
    input[brain_actions + 2] = s.diversity; input[brain_actions + 3] = s.moisture;
    input[brain_actions + 4] = s.population / double(population_limit);
    input[brain_actions + 5] = animals.size() / double(animal_limit);
    input[brain_actions + 6] = season() / 3.;
    input[brain_actions + 7] = static_cast<int>(biome) / 7.;
    const bool safe = s.population > 0 && s.welfare > .6 && s.food > .2 && !flood_alert;
    int chosen = 0; double best = -10;
    for (int a = 0; a < 3; ++a) {
        input[a] = 1;
        const double prediction = d.brain.evaluate(input);
        double score = prediction * .15 + (a == 0 ? .04 : -.02);
        if (a == 1) score += std::max(0., .45 - s.moisture) * .3;
        if (a == 2) score += std::max(0., s.moisture - .75) * .3;
        if ((a == 0 || safe) && score > best) { chosen = a; best = score; d.expected = prediction; }
        input[a] = 0;
    }
    // Rare, safe experiments use only the director's saved PRNG.
    bool experimenting = false;
    if (d.mode == DirectorMode::Adaptive && learning_enabled && exploration_enabled && safe) {
        const int experiment = d.brain.exploration(true);
        if (experiment >= 0) { chosen = experiment % 3; experimenting = true; ++d.brain.experiments; }
    }
    d.action = chosen; input[chosen] = 1;
    d.expected = d.brain.predict(input); ++d.decisions;
    d.reason = !safe ? "Low reserves or welfare: recovery safeguard"
        : experimenting ? "Safe experiment; outcome not yet known"
        : chosen == 0 ? "Observe autonomous development"
        : "Soil water " + std::to_string(static_cast<int>(s.moisture * 100)) + "%; contextual score favors opportunity";
    d.bias = 0;
    if (d.mode == DirectorMode::Adaptive) {
        d.bias = chosen == 1 ? 8 : chosen == 2 ? -8 : 0;
        if (chosen != 0) { ++d.interventions; event(std::string("Director: ") + d.action_name() + " (+/-8 rain chance for this transition)"); }
        d.pending = true; d.pending_tick = ticks; d.baseline = quality(s);
        d.baseline_population = s.population; d.brain.pending = input;
        d.brain.action = chosen; d.brain.active = true;
    }
}
}
