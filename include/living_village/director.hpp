#pragma once
#include "brain.hpp"
#include <string>
namespace village {
enum class DirectorMode { Off, Observer, Adaptive };
struct Director {
    DirectorMode mode = DirectorMode::Observer;
    Brain brain;
    int action = 0, bias = 0;
    std::uint64_t decisions = 0, interventions = 0, pending_tick = 0;
    double baseline = 0, expected = 0, outcome = 0;
    int baseline_population = 0;
    bool pending = false;
    std::string reason = "Awaiting first daily observation";
    const char* mode_name() const;
    const char* action_name() const;
};
}
