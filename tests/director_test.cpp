#include "living_village/world.hpp"
#include "living_village/ui.hpp"
#include "living_village/workers.hpp"
#include <iostream>
#include <stdexcept>
using namespace village;
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    Interface ui; World input_world(43); ui.view.panel = Panel::Overview; ui.view.overview_page = 9;
    InputState press; press.pressed[static_cast<int>(Button::Confirm)] = true;
    check(ui.update(press, input_world, .033) == Request::DirectorMode, "Controller cannot change director mode");
    check(ui.view.simulation_running(), "Director diagnostics pause simulation");
    ui.view.overview_page = 6; ui.update(press, input_world, .033);
    check(ui.view.ecology_overlay == 1 && ui.view.panel == Panel::Map, "Ecology overlay cannot be selected");
    World serial(17), parallel = serial;
    for (int n = 0; n < 4800; ++n) serial.update();
    configure_workers(4);
    for (int n = 0; n < 4800; ++n) parallel.update();
    check(serial.serialize() == parallel.serialize(), "Parallel ecology changed deterministic state");
    configure_workers(1);
    World off(43), observer = off;
    off.director.mode = DirectorMode::Off;
    for (int n = 0; n < 9600; ++n) { off.update(); observer.update(); }
    check(off.rng == observer.rng && off.ecology_rng == observer.ecology_rng, "Observer altered simulation RNG");
    check(off.food_stock == observer.food_stock && off.alive_count() == observer.alive_count(), "Observer altered outcomes");
    check(observer.director.decisions == 4 && observer.director.brain.updates == 0, "Observer trained on proposals");
    World adaptive(45); adaptive.director.mode = DirectorMode::Adaptive;
    for (int n = 0; n < 2400; ++n) adaptive.update();
    check(adaptive.director.pending && adaptive.director.brain.valid(), "No pending adaptive evaluation");
    // Construct a genuine version-7 body by omitting the new extension.
    auto legacy = adaptive.serialize(); legacy.replace(0, legacy.find('\n') + 1, "LIVING_VILLAGE 7\n");
    legacy = legacy.substr(0, legacy.find("DIRECTOR 1 "));
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : legacy) { hash ^= c; hash *= 1099511628211ULL; }
    legacy += "CHECKSUM " + std::to_string(hash) + "\n";
    auto migrated = World::deserialize(legacy);
    check(migrated.director.mode == DirectorMode::Observer && migrated.ticks == adaptive.ticks, "Version-7 migration failed");
    auto restored = World::deserialize(adaptive.serialize());
    check(restored.serialize() == adaptive.serialize(), "Director save mismatch");
    for (int n = 0; n < 7200; ++n) { adaptive.update(); restored.update(); }
    check(adaptive.serialize() == restored.serialize(), "Continuation diverged after director restore");
    check(adaptive.director.brain.episodes == 1 && adaptive.director.brain.updates > 0, "Delayed learning absent");
    check(std::abs(adaptive.director.bias) <= 8, "Intervention budget exceeded");
    adaptive.cycle_director();
    check(adaptive.director.mode == DirectorMode::Off && !adaptive.director.pending && adaptive.director.bias == 0, "Mode switch retained intervention");
    World frozen(45); frozen.director.mode = DirectorMode::Adaptive; frozen.learning_enabled = false;
    for (int n = 0; n < 9600; ++n) frozen.update();
    check(frozen.director.brain.updates == 0, "Frozen director trained");
    World shortage(3); shortage.director.mode = DirectorMode::Adaptive; shortage.food_stock = 0;
    for (auto& p : shortage.villagers) { p.food = 0; p.health = 40; }
    shortage.update_director();
    check(shortage.director.action == 0 && shortage.director.bias == 0, "Safeguard allowed pressure");
    std::cout << "Director observer isolation, delayed learning, persistence, freeze and safeguards passed\n";
}
