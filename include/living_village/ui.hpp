#pragma once
#include "input.hpp"
#include "render.hpp"
namespace village {
enum class Request { None, Save, Load, Exit, Plan, ToggleDevelopment, Designate, ToggleGrowth, NewVillage, PreviousVillage, ToggleLearning, Volume, ToggleRumble, ExportHistory };
class Interface {
public:
    View view;
    Request update(const InputState& input, const World& world, double elapsed);
private:
    double repeat = 0;
    double page_repeat = 0;
    void cycle(const World& world, int direction);
};
}
