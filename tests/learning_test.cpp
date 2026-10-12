#include "living_village/saves.hpp"
#include "living_village/ui.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace village;
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
void advance(World& world, int steps) { for (int n = 0; n < steps; ++n) world.update(); }
int main() {
    try {
        Brain brain; brain.initialize(12, 3);
        BrainInput good{}, bad{}; good[1] = 1; bad[4] = 1;
        good[brain_actions] = bad[brain_actions] = .8;
        const auto initial = brain.weights;
        for (int n = 0; n < 800; ++n) { brain.train(good, .9); brain.train(bad, -.9); }
        check(brain.predict(good) > .65 && brain.predict(bad) < -.65 && brain.valid(), "MLP did not distinguish useful and unsuccessful action outcomes");
        check(brain.weights[1] != initial[1] && brain.weights.back() != initial.back(), "Backpropagation did not update both network layers");
        const auto trained = brain.weights;
        BrainInput invalid = good; invalid[2] = 2;
        brain.train(invalid, .9); brain.train(good, std::nan(""));
        check(brain.weights == trained, "Invalid training data changed the model");
        Brain repeat; repeat.initialize(12, 3);
        for (int n = 0; n < 800; ++n) { repeat.train(good, .9); repeat.train(bad, -.9); }
        check(repeat.weights == brain.weights, "Deterministic neural training diverged");
        for (int n = 0; n < 1000; ++n) { brain.train(good, -.9); brain.train(bad, .9); }
        check(brain.predict(good) < -.65 && brain.predict(bad) > .65 && brain.valid(), "Model did not relearn when outcome rewards reversed");
        Brain independent; independent.initialize(12, 4);
        check(independent.weights != initial, "Different villagers share the same initialized model");

        World choices(1); choices.auto_develop = false; choices.population_growth = false; choices.villagers.resize(1);
        auto& chooser = choices.villagers[0]; chooser.x = chooser.y = 32; chooser.hunger = 20; chooser.energy = 100;
        chooser.loneliness = 0; chooser.cooperation = .1; chooser.curiosity = .3; chooser.food = 0; chooser.known_food = {{34, 32}};
        choices.tile(34, 32) = {Terrain::Bush, 6, 0}; choices.wood_stock = 3;
        check(choices.plan(BuildingKind::Farm, {31, 32}), "Learning choice fixture failed");
        auto& farm = choices.buildings.back(); farm.complete = true; farm.food = 12;
        choices.learning_enabled = false;
        World untrained = choices; untrained.update();
        check(untrained.villagers[0].goal == Goal::TendFarm, "Baseline choice did not prefer its higher farm utility");
        const auto forage = choices.observations(chooser, Goal::Forage, {34, 32}, 55);
        const auto harvest = choices.observations(chooser, Goal::TendFarm, {31, 32}, 63);
        for (int n = 0; n < 1000; ++n) { chooser.brain.train(forage, .9); chooser.brain.train(harvest, -.9); }
        choices.update();
        check(chooser.goal == Goal::Forage && chooser.brain.selected_adjustment > 8, "Learned MLP values did not change the actual agent decision");
        choices.learning_enabled = true; advance(choices, 100);
        check(chooser.brain.updates > 2000 && chooser.food > 0, "Live gathering outcomes did not train the model");
        World invalid_choice = choices; auto& child = invalid_choice.villagers[0];
        child.age = 0; child.food = 0; child.path.clear(); child.decision_timer = 0;
        for (auto& weight : child.brain.weights) weight = 3;
        invalid_choice.update();
        check(child.goal != Goal::Build && child.goal != Goal::GatherWood && child.goal != Goal::CollectStone, "Model bypassed child action validity");

        World live(20261011); advance(live, 2400);
        for (const auto& p : live.villagers) if (p.alive()) check(p.brain.updates > 0 && p.brain.valid(), "A living agent did not learn during play");
        check(live.villagers[0].brain.weights != live.villagers[1].brain.weights, "Agents failed to retain independent models");
        const auto snapshot = live.serialize();
        World restored = World::deserialize(snapshot);
        check(restored.serialize() == snapshot, "Neural weights or in-flight learning experience did not round-trip");
        advance(live, 5000); advance(restored, 5000);
        check(live.serialize() == restored.serialize(), "Saved online learning did not resume deterministically");
        live.learning_enabled = false; const auto frozen = live.villagers[0].brain.weights; const auto count = live.villagers[0].brain.updates;
        advance(live, 1000);
        check(live.villagers[0].brain.weights == frozen && live.villagers[0].brain.updates == count, "Paused learning still changed weights");
        check(!World::deserialize(live.serialize()).learning_enabled, "Learning preference was not saved");
        live.learning_enabled = true; advance(live, 1000);
        check(live.villagers[0].brain.updates > count && live.villagers[0].brain.weights != frozen, "Learning failed to resume in a running world");
        World malformed = live; malformed.villagers[0].brain.weights[0] = 4;
        bool rejected = false; try { World::deserialize(malformed.serialize()); } catch (const std::exception&) { rejected = true; }
        check(rejected, "Save validation accepted unbounded model weights");
        malformed = live; malformed.villagers[0].brain.pending[0] = 2;
        rejected = false; try { World::deserialize(malformed.serialize()); } catch (const std::exception&) { rejected = true; }
        check(rejected, "Save validation accepted invalid pending observations");
#ifdef _LIBCPP_VERSION
        const char* fixture = "tests/fixtures/version3-libcxx.lvsave";
#else
        const char* fixture = "tests/fixtures/version3.lvsave";
#endif
        std::ifstream old(fixture); std::ostringstream content; content << old.rdbuf();
        World migrated = World::deserialize(content.str());
        check(migrated.seed == 777 && migrated.ticks == 2500 && migrated.villagers.size() == 6 && migrated.learning_enabled, "Version 3 migration lost world state");
        for (const auto& p : migrated.villagers) check(p.brain.updates == 0 && p.brain.valid(), "Old resident did not receive a valid fresh model");
        check(World::deserialize(migrated.serialize()).serialize() == migrated.serialize(), "Migrated neural state was not saveable");
        advance(migrated, 200); check(migrated.villagers[0].brain.updates > 0, "Migrated agent did not start learning");
        Interface ui; ui.view.panel = Panel::Overview; ui.view.overview_page = 3;
        InputState press; press.pressed[static_cast<int>(Button::Confirm)] = true;
        check(ui.update(press, live, .033) == Request::ToggleLearning, "Controller could not toggle online learning");
        press = {}; press.x = -1; ui.view.panel = Panel::Inspector; ui.view.inspector_page = 0;
        ui.update(press, live, .033); check(ui.view.inspector_page == 7, "Controller inspector did not wrap to new outcomes page");
        ui.update({}, live, .25); ui.view.inspector_page = 5;
        ui.update(press, live, .033); check(ui.view.inspector_page == 4, "Controller could not reach neural inspection");
        std::cout << "PASS: MLP backpropagation, outcome discrimination/relearning, learned action changes, live independent learning, validity gates, freeze/resume, neural persistence/continuation, v3 migration, controller diagnostics\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
