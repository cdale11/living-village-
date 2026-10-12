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
    pending.fill(0); active = false; action = 0; updates = 0;
    elapsed = progress = start_hunger = start_energy = start_health = start_loneliness = 0;
    last_reward = last_prediction = last_error = selected_base = selected_adjustment = 0;
}
double Brain::predict(const BrainInput& input) const {
    const auto hidden = activate(*this, input);
    double sum = weights.back();
    for (int h = 0; h < brain_hidden; ++h) sum += weights[output_offset + h] * hidden[h];
    return std::tanh(sum);
}
void Brain::train(const BrainInput& input, double reward) {
    if (!std::isfinite(reward)) return;
    for (double value : input) if (!std::isfinite(value) || std::abs(value) > 1) return;
    const auto hidden = activate(*this, input);
    const double prediction = predict(input);
    const double target = std::clamp(reward, -1.0, 1.0);
    const double delta = std::clamp((prediction - target) * (1 - prediction * prediction), -1.0, 1.0);
    auto adjust = [&](int index, double gradient) {
        weights[index] = std::clamp(weights[index] - .025 * std::clamp(gradient, -1.0, 1.0), -3.0, 3.0);
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
    if (updates < 100000000000ULL) ++updates;
    last_prediction = prediction; last_reward = target; last_error = target - prediction;
}
bool Brain::valid() const {
    auto bounded = [](double value, double maximum) { return std::isfinite(value) && std::abs(value) <= maximum; };
    for (double weight : weights) if (!bounded(weight, 3)) return false;
    for (double value : pending) if (!bounded(value, 1)) return false;
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
