#include "living_village/saves.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace village;
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
void advance(World& w, int steps) { for (int n = 0; n < steps; ++n) w.update(); }
int main() {
    try {
        Brain brain; brain.initialize(42, 7); BrainInput food{}, stone{};
        food[1] = stone[5] = 1; food[brain_actions] = stone[brain_actions] = .8;
        for (int n = 0; n < 1000; ++n) { brain.observe(food, .9); brain.observe(stone, -.9); }
        check(brain.valid() && brain.replay_size == 32 && brain.replay_head == 16 && brain.episodes == 2000 && brain.replay_updates == 1999 && brain.updates == 3999, "Replay was not bounded or update budget incorrect");
        check(brain.predict(food) > .65 && brain.predict(stone) < -.65, "Replay did not preserve contrasting action outcomes");
        for (int n = 0; n < 1000; ++n) { brain.observe(food, -.9); brain.observe(stone, .9); }
        check(brain.valid() && brain.predict(food) < -.65 && brain.predict(stone) > .65, "Old replay prevented adaptation to changed outcomes");
        const auto before = brain.weights; const auto episodes = brain.episodes; BrainInput invalid{};
        brain.observe(invalid, 1); invalid[0] = invalid[1] = 1; brain.observe(invalid, 1);
        check(brain.weights == before && brain.episodes == episodes, "Invalid action flags entered replay");
        const auto random_before = brain.random_state;
        for (int n = 0; n < 100; ++n) check(brain.exploration(false) == -1, "Disabled exploration selected an action");
        check(brain.random_state == random_before, "Disabled exploration consumed neural RNG");
        int probes = 0; for (int n = 0; n < 1000; ++n) { const int action = brain.exploration(true); check(action >= -1 && action < brain_actions, "Exploration exceeded action bounds"); if (action >= 0) ++probes; }
        check(probes > 60 && probes < 200, "Exploration did not occasionally sample alternatives");

        World urgent(42); urgent.auto_develop = false; urgent.population_growth = false; urgent.villagers.resize(1);
        auto& resident = urgent.villagers[0]; resident.hunger = 90; resident.energy = 10; resident.food = 4;
        urgent.update(); check(resident.brain.selected_exploration == 0, "Hungry/tired resident experimented instead of addressing needs");
        World practicing(22); practicing.auto_develop = false; practicing.population_growth = false; practicing.villagers.resize(1);
        auto& worker = practicing.villagers[0]; worker.x = worker.y = 32;
        for (int n = 0; n < 12; ++n) {
            worker.goal = Goal::Forage; worker.path.clear(); worker.decision_timer = 9; worker.action_timer = 2; worker.food = 0;
            practicing.tile(32, 32) = {Terrain::Bush, 6, 0}; practicing.update();
        }
        check(worker.occupation == Occupation::Forager && worker.practice[0] == 12, "Successful foraging did not create an earned specialization");
        for (int n = 0; n < 20; ++n) {
            worker.goal = Goal::GatherWood; worker.path.clear(); worker.decision_timer = 9; worker.action_timer = 2; worker.wood = worker.stone = 0;
            practicing.tile(32, 32) = {Terrain::Tree, 0, 0, 6, 0}; practicing.update();
        }
        check(worker.occupation == Occupation::Woodcutter && worker.practice[1] == 20 && worker.practice[0] == 12, "Occupation did not respond to a clear change in actual work");
        const auto wood_practice = worker.practice[1];
        for (int n = 0; n < 5; ++n) {
            worker.goal = Goal::GatherWood; worker.path.clear(); worker.decision_timer = 9; worker.action_timer = 2; worker.wood = worker.stone = 0;
            practicing.tile(32, 32) = {}; practicing.update();
        }
        check(worker.practice[1] == wood_practice, "Unsuccessful gathering earned work credit");
        worker.brain.active = false; worker.goal = Goal::Rest; worker.target_id = -1; worker.action_timer = 0; worker.decision_timer = 9;
        check(World::deserialize(practicing.serialize()).serialize() == practicing.serialize(), "Practice and occupation did not save exactly");
        practicing.ticks = 2399; const auto remembered = worker.practice[1]; practicing.update();
        check(std::abs(worker.practice[1] - remembered * .98) < 1e-9, "Practice did not decay with time");

        World live(444); advance(live, 4000);
        for (const auto& p : live.villagers) if (p.alive()) check(p.brain.episodes > 0 && p.brain.replay_size > 0 && p.brain.valid(), "Live agent did not use replay");
        World restored = World::deserialize(live.serialize()); advance(live, 4000); advance(restored, 4000);
        check(live.serialize() == restored.serialize(), "Replay/exploration RNG did not resume deterministically");
        const auto memory = live.villagers[0].brain.replay; const auto updates = live.villagers[0].brain.updates;
        live.learning_enabled = false; advance(live, 500);
        check(live.villagers[0].brain.updates == updates && live.villagers[0].brain.replay[0].input == memory[0].input && live.villagers[0].brain.replay[0].reward == memory[0].reward, "Frozen learner changed replay or weights");
        World corrupt = live; corrupt.villagers[0].brain.replay_size = 33;
        bool rejected = false; try { World::deserialize(corrupt.serialize()); } catch (const std::exception&) { rejected = true; }
        check(rejected, "Out-of-range replay accepted");
#ifdef _LIBCPP_VERSION
        const char* fixture = "tests/fixtures/version4-libcxx.lvsave";
#else
        const char* fixture = "tests/fixtures/version4.lvsave";
#endif
        std::ifstream old(fixture); std::ostringstream content; content << old.rdbuf();
        World upgraded = World::deserialize(content.str());
        check(upgraded.seed == 444 && upgraded.ticks == 4000 && upgraded.villagers[0].brain.updates > 0 && upgraded.villagers[0].brain.replay_size == 0, "Version 4 migration lost learned state or fabricated replay");
        std::istringstream original(content.str()); std::string line;
        while (std::getline(original, line) && line.rfind("0 \"Mira\"", 0) != 0) {}
        for (int n = 0; n < 4; ++n) std::getline(original, line);
        std::istringstream weights(line);
        for (double weight : upgraded.villagers[0].brain.weights) { double expected = 0; weights >> expected; check(weights.good() && weight == expected, "Migration changed previously trained neural weights"); }
        check(World::deserialize(upgraded.serialize()).serialize() == upgraded.serialize(), "Upgraded neural world did not save exactly");
        advance(upgraded, 200); check(upgraded.villagers[0].brain.replay_size > 0, "Upgraded learner did not start replay");
        std::cout << "PASS: bounded replay/update budgets, retained outcomes and reversal, controlled exploration, earned/changing occupations, real-work gates, decay, replay continuation/freeze, trained v4 migration\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
