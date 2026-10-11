#include "living_village/saves.hpp"
#include "living_village/ui.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

using namespace village;
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void advance(World& world, int steps) { for (int n = 0; n < steps; ++n) world.update(); }

int main() {
    try {
        World world(42), same(42);
        advance(world, 6000); advance(same, 6000);
        check(world.serialize() == same.serialize(), "Seed determinism failed");
        World restored = World::deserialize(world.serialize());
        check(world.serialize() == restored.serialize(), "Save round trip changed state");
        advance(world, 600); advance(restored, 600);
        check(world.serialize() == restored.serialize(), "Restored random generator diverged");
        std::string corrupt = world.serialize(); corrupt[20] ^= 1;
        bool rejected = false;
        try { World::deserialize(corrupt); } catch (const std::exception&) { rejected = true; }
        check(rejected, "Corrupt save was accepted");

        World hungry(1);
        auto& eater = hungry.villagers[0];
        eater.hunger = 95; eater.food = 1; eater.energy = 100;
        advance(hungry, 15);
        check(eater.hunger < 60 && eater.food == 0, "Hungry agent did not consume carried food");
        World tired(1);
        auto& sleeper = tired.villagers[0];
        sleeper.x = sleeper.y = 32; sleeper.energy = 5; sleeper.hunger = 0;
        advance(tired, 20);
        check(sleeper.goal == Goal::Rest && sleeper.energy > 8, "Tired agent did not rest");

        World harvest(1);
        harvest.villagers.resize(1); harvest.food_stock = 0;
        for (auto& tile : harvest.tiles) tile = {};
        harvest.tile(32, 32) = {Terrain::Bush, 1, 0};
        auto& picker = harvest.villagers[0]; picker.x = picker.y = 32; picker.hunger = 0;
        advance(harvest, 25);
        check(picker.food == 1 && harvest.tile(32, 32).food == 0, "Harvest duplicated or lost a resource");

        World obstacles(1);
        for (auto& tile : obstacles.tiles) tile = {};
        for (int y = 0; y < 63; ++y) obstacles.tile(2, y).terrain = Terrain::Water;
        auto path = obstacles.route({1, 1}, {3, 1});
        check(path.size() > 100 && path.back() == Point{3, 1}, "Route failed to avoid obstacle");
        for (auto p : path) check(obstacles.passable(p.x, p.y), "Path crosses water");

        World shortage(9); shortage.food_stock = 0;
        for (auto& tile : shortage.tiles) tile = {};
        advance(shortage, 18000);
        for (const auto& p : shortage.villagers) check(!p.alive(), "Severe shortage did not cause collapse");

        const auto folder = std::filesystem::temp_directory_path() / ("living-village-test-" + std::to_string(getpid()));
        std::filesystem::create_directories(folder);
        const auto filename = folder / "test.lvsave";
        save_atomic(world, filename); advance(world, 10); save_atomic(world, filename);
        check(load_save(filename).serialize() == world.serialize(), "Atomic save did not restore state");
        { std::ofstream bad(filename); bad << "interrupted"; }
        bool backup = false; load_save(filename, &backup);
        check(backup, "Backup recovery failed");
        save_atomic(world, filename);
        check(load_save(filename).serialize() == world.serialize(), "Saving after recovery failed");
        std::filesystem::remove_all(folder);

        Interface menu;
        InputState press; press.pressed[static_cast<int>(Button::Menu)] = true;
        menu.update(press, world, .033); check(menu.view.panel == Panel::Menu, "Controller menu did not open");
        press = {}; press.y = 1; menu.update(press, world, .033);
        press = {}; press.pressed[static_cast<int>(Button::Confirm)] = true;
        check(menu.update(press, world, .033) == Request::Save, "Controller save menu did not activate");
        menu.view.panel = Panel::Menu; menu.view.menu_index = 4;
        check(menu.update(press, world, .033) == Request::Exit, "Controller exit did not activate");

        World long_run(123);
        advance(long_run, 2400 * 300);
        for (const auto& p : long_run.villagers) {
            check(p.alive(), "Reasonable starting conditions failed long-term survival");
            check(p.known_food.size() <= 48 && p.path.size() <= 4096, "Agent memory grew beyond bounds");
        }
        check(long_run.events.size() <= 24, "Event history grew beyond bounds");
        check(World::deserialize(long_run.serialize()).serialize() == long_run.serialize(), "Long-run save failed");
        std::cout << "PASS: determinism, save continuation, corruption/backup, eating, rest, resource accounting, obstacles, shortages, controller menus, 300 days\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
