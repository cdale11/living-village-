#pragma once
#include <array>
#include <cstdint>

namespace village {
// Candidate-specific contextual value model: 19 action flags + 18 observations.
constexpr int brain_actions = 19, brain_context = 18;
constexpr int brain_inputs = brain_actions + brain_context, brain_hidden = 12;
constexpr int brain_weights = brain_hidden * (brain_inputs + 1) + brain_hidden + 1;
using BrainInput = std::array<double, brain_inputs>;
constexpr int replay_capacity = 32;
struct Experience { BrainInput input{}; double reward = 0; };
struct Brain {
    std::array<double, brain_weights> weights{};
    BrainInput pending{};
    bool active = false;
    int action = 0;
    std::uint64_t updates = 0;
    double elapsed = 0, progress = 0;
    double start_hunger = 0, start_energy = 0, start_health = 0, start_loneliness = 0;
    double last_reward = 0, last_prediction = 0, last_error = 0;
    double selected_base = 0, selected_adjustment = 0;
    std::array<Experience, replay_capacity> replay{};
    int replay_size = 0, replay_head = 0;
    std::uint32_t random_state = 1;
    std::uint64_t episodes = 0, replay_updates = 0, experiments = 0;
    double selected_exploration = 0;
    std::uint64_t session_predictions = 0, session_updates = 0, session_changed_updates = 0;
    double session_weight_change = 0;
    double evaluate(const BrainInput& input);
    Brain();
    void initialize(std::uint32_t world_seed, int person_id);
    double predict(const BrainInput& input) const;
    void train(const BrainInput& input, double reward, double rate = .025);
    void observe(const BrainInput& input, double reward);
    int exploration(bool allowed);
    bool valid() const;
private:
    std::uint32_t random();
};
} // namespace village
