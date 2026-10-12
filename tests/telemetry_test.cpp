#include "living_village/world.hpp"
#include "living_village/ui.hpp"
#include <iostream>
#include <stdexcept>
using namespace village;
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
int main() {
    try {
        Brain b; BrainInput input{}; input[0] = 1;
        const auto original = b.weights; b.predict(input);
        check(b.session_predictions == 0, "Inspector queries counted as agent decisions");
        b.evaluate(input); check(b.session_predictions == 1 && original == b.weights, "Inference trained or was not counted");
        b.observe(input, .8); check(b.session_updates == 1 && b.session_changed_updates == 1 && b.session_weight_change > 0 && b.weights != original, "Training did not record actual weight changes");
        b.observe(input, -.8); check(b.session_updates == 3 && b.session_changed_updates == 3 && b.episodes == 2 && b.replay_updates == 1, "Replay/fresh training telemetry wrong");
        World world(123); for (int i = 0; i < 1200; ++i) world.update();
        std::uint64_t predictions = 0, trained = 0; for (const auto& p : world.villagers) { predictions += p.brain.session_predictions; trained += p.brain.session_updates; }
        check(predictions > 0 && trained > 0, "Real autonomous decisions/training were not counted");
        World restored = World::deserialize(world.serialize()); check(restored.serialize() == world.serialize(), "Telemetry changed persistent state");
        for (const auto& p : restored.villagers) check(p.brain.session_predictions == 0 && p.brain.session_updates == 0, "Session counters did not reset at load");
        restored.learning_enabled = false; for (int i = 0; i < 1200; ++i) restored.update();
        predictions = 0; for (const auto& p : restored.villagers) { predictions += p.brain.session_predictions; check(p.brain.session_updates == 0, "Frozen model trained"); }
        check(predictions > 0, "Frozen models stopped predicting");
        Interface ui; InputState press; press.pressed[static_cast<int>(Button::Confirm)] = true;
        ui.view.panel = Panel::Overview; ui.view.overview_page = 8; check(ui.update(press, world, .03) == Request::ToggleLearning, "Diagnostics cannot freeze learning via controller");
        check(ui.view.simulation_running(), "Agent diagnostics paused simulation");
        press = {}; press.pressed[static_cast<int>(Button::Menu)] = true; ui.update(press, world, .03);
        check(ui.view.panel == Panel::Menu && !ui.view.simulation_running(), "Start did not pause live diagnostics");
        ui.view.panel = Panel::Overview; ui.view.overview_page = 3; check(ui.view.simulation_running(), "Global diagnostics paused simulation");
        ui.view.overview_page = 0; check(!ui.view.simulation_running(), "Ordinary information page unexpectedly runs simulation");
        press = {}; press.pressed[static_cast<int>(Button::Confirm)] = true;
        ui.view.panel = Panel::NewVillage; ui.view.menu_index = 4; ui.update(press, world, .03); check(ui.view.draft_supplies == 2, "Controller cannot select starting supplies");
        std::cout << "PASS: actual inference/SGD/weight-change telemetry, replay, session reset, frozen inference, diagnostic and supply controls\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
