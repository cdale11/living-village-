#include "living_village/brain.hpp"
#include <algorithm>
#include <cmath>

namespace village {
namespace {
constexpr int output_offset = brain_hidden * (brain_inputs + 1);
std::array<double, brain_hidden> activate(const Brain& brain, const BrainInput& input) {
    std::array<double, brain_hidden> hidden{};
    for (int h = 0; h < brain_hidden; ++h) {
        const int offset = h * (brain_inputs + 1);
        double sum = brain.weights[offset + brain_inputs];
        for (int i = 0; i < brain_inputs; ++i) sum += brain.weights[offset + i] * input[i];
        hidden[h] = std::tanh(sum);
    }
    return hidden;
}
}
Brain::Brain() { initialize(1, 0); }
void Brain::initialize(std::uint32_t world_seed, int person_id) {
    // Local PRNG: initialization never consumes the world's ecology/action RNG.
    std::uint32_t state = world_seed ^ (static_cast<std::uint32_t>(person_id) * 2654435761U) ^ 0x9e3779b9U;
    if (state == 0) state = 1;
    for (auto& weight : weights) {
        state ^= state << 13; state ^= state >> 17; state ^= state << 5;
        weight = (static_cast<double>(state % 20001) / 10000 - 1) * .08;
    }
    random_state = state; replay = {}; replay_size = replay_head = 0;
    episodes = replay_updates = experiments = 0; selected_exploration = 0;
    session_predictions = session_updates = session_changed_updates = 0; session_weight_change = 0;
    pending.fill(0); active = false; action = 0; updates = 0;
    elapsed = progress = start_hunger = start_energy = start_health = start_loneliness = 0;
    last_reward = last_prediction = last_error = selected_base = selected_adjustment = 0;
}
double Brain::evaluate(const BrainInput& input) {
    if (session_predictions < 100000000000ULL) ++session_predictions;
    return predict(input);
}
double Brain::predict(const BrainInput& input) const {
    const auto hidden = activate(*this, input);
    double sum = weights.back();
    for (int h = 0; h < brain_hidden; ++h) sum += weights[output_offset + h] * hidden[h];
    return std::tanh(sum);
}
void Brain::train(const BrainInput& input, double reward, double rate) {
    if (!std::isfinite(reward) || !std::isfinite(rate) || rate <= 0 || rate > .025) return;
    for (double value : input) if (!std::isfinite(value) || std::abs(value) > 1) return;
    const auto hidden = activate(*this, input);
    const double prediction = predict(input);
    const double target = std::clamp(reward, -1.0, 1.0);
    const double delta = std::clamp((prediction - target) * (1 - prediction * prediction), -1.0, 1.0);
    double weight_change = 0;
    auto adjust = [&](int index, double gradient) {
        const double before = weights[index];
        weights[index] = std::clamp(weights[index] - rate * std::clamp(gradient, -1.0, 1.0), -3.0, 3.0);
        weight_change += std::abs(weights[index] - before);
    };
    // Hidden gradient must use output weights from before the update.
    for (int h = 0; h < brain_hidden; ++h) {
        const double hidden_delta = delta * weights[output_offset + h] * (1 - hidden[h] * hidden[h]);
        const int offset = h * (brain_inputs + 1);
        for (int i = 0; i < brain_inputs; ++i) adjust(offset + i, hidden_delta * input[i]);
        adjust(offset + brain_inputs, hidden_delta);
    }
    for (int h = 0; h < brain_hidden; ++h) adjust(output_offset + h, delta * hidden[h]);
    adjust(brain_weights - 1, delta);
    session_weight_change = weight_change;
    if (session_updates < 100000000000ULL) ++session_updates;
    if (weight_change > 0 && session_changed_updates < 100000000000ULL) ++session_changed_updates;
    if (updates < 100000000000ULL) ++updates;
    last_prediction = prediction; last_reward = target; last_error = target - prediction;
}
std::uint32_t Brain::random() {
    random_state ^= random_state << 13; random_state ^= random_state >> 17; random_state ^= random_state << 5;
    return random_state;
}
int Brain::exploration(bool allowed) {
    if (!allowed || random() % 8 != 0) return -1;
    return static_cast<int>(random() % brain_actions);
}
void Brain::observe(const BrainInput& input, double reward) {
    if (!std::isfinite(reward)) return;
    int flags = 0;
    for (int i = 0; i < brain_inputs; ++i) {
        const double value = input[i];
        if (!std::isfinite(value) || value < 0 || value > 1) return;
        if (i < brain_actions) { if (value != 0 && value != 1) return; flags += static_cast<int>(value); }
    }
    if (flags != 1) return;
    train(input, reward);
    const double prediction = last_prediction, error = last_error, target = last_reward;
    // One old sample per new outcome, at half rate. No frame-based training loop.
    if (replay_size > 0) {
        const auto& old = replay[random() % static_cast<unsigned>(replay_size)];
        train(old.input, old.reward, .0125);
        if (replay_updates < 100000000000ULL) ++replay_updates;
    }
    last_prediction = prediction; last_error = error; last_reward = target;
    replay[replay_head] = {input, std::clamp(reward, -1.0, 1.0)};
    replay_head = (replay_head + 1) % replay_capacity;
    replay_size = std::min(replay_capacity, replay_size + 1);
    if (episodes < 100000000000ULL) ++episodes;
}
bool Brain::valid() const {
    auto bounded = [](double value, double maximum) { return std::isfinite(value) && std::abs(value) <= maximum; };
    for (double weight : weights) if (!bounded(weight, 3)) return false;
    for (double value : pending) if (!bounded(value, 1)) return false;
    if (replay_size < 0 || replay_size > replay_capacity || replay_head < 0 || replay_head >= replay_capacity
        || (replay_size < replay_capacity && replay_head != replay_size) || random_state == 0
        || episodes > 100000000000ULL || replay_updates > 100000000000ULL || experiments > 100000000000ULL
        || !bounded(selected_exploration, 8) || selected_exploration < 0) return false;
    for (int n = 0; n < replay_size; ++n) {
        if (!bounded(replay[n].reward, 1)) return false;
        int flags = 0;
        for (int i = 0; i < brain_inputs; ++i) {
            if (!bounded(replay[n].input[i], 1) || replay[n].input[i] < 0) return false;
            if (i < brain_actions) { if (replay[n].input[i] != 0 && replay[n].input[i] != 1) return false; flags += static_cast<int>(replay[n].input[i]); }
        }
        if (flags != 1) return false;
    }
    if (active) {
        for (int i = 0; i < brain_actions; ++i) if (pending[i] != (i == action ? 1.0 : 0.0)) return false;
    }
    return action >= 0 && action < brain_actions && updates <= 100000000000ULL
        && bounded(elapsed, 100000) && elapsed >= 0 && bounded(progress, 20)
        && bounded(start_hunger, 100) && start_hunger >= 0 && bounded(start_energy, 100) && start_energy >= 0
        && bounded(start_health, 100) && start_health >= 0 && bounded(start_loneliness, 100) && start_loneliness >= 0
        && bounded(last_reward, 1) && bounded(last_prediction, 1) && bounded(last_error, 2)
        && bounded(selected_base, 300) && selected_base >= 0 && bounded(selected_adjustment, 18);
}
} // namespace village
