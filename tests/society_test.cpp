#include "living_village/saves.hpp"
#include "living_village/ui.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace village;
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
void advance(World& w, int steps) { for (int n = 0; n < steps; ++n) w.update(); }
World pair() {
    World w(8); w.auto_develop = w.population_growth = false; w.villagers.resize(2);
    for (auto& p : w.villagers) { p.x = p.y = 32; p.hunger = 10; p.energy = 100; p.loneliness = 0; p.cooperation = .8; p.decision_timer = 9; }
    auto& actor = w.villagers[0]; actor.goal = Goal::Socialize; actor.target_id = 1; actor.action_timer = 3;
    return w;
}
int main() {
    try {
        World sharing = pair(); sharing.villagers[0].food = 3; sharing.villagers[1].hunger = 80;
        sharing.update(); check(sharing.help_count == 1 && sharing.villagers[0].food == 2 && sharing.villagers[1].food == 1, "Meal sharing did not physically conserve food");
        World distant = pair(); distant.villagers[0].food = 3; distant.villagers[1].hunger = 80; distant.villagers[1].x = 50;
        distant.update(); check(distant.help_count == 0 && distant.villagers[0].food == 3, "Food was shared across the map");
        World trade = pair(); trade.villagers[0].food = 3; trade.villagers[1].stone = 2;
        trade.update(); check(trade.trade_count == 1 && trade.villagers[0].food == 1 && trade.villagers[1].food == 2 && trade.villagers[0].stone == 1 && trade.villagers[1].stone == 1, "Barter created or lost resources");
        World child_trade = pair(); child_trade.villagers[0].age = 10; child_trade.villagers[0].food = 3; child_trade.villagers[1].stone = 2;
        child_trade.update(); check(child_trade.trade_count == 0 && child_trade.villagers[0].stone == 0, "Child barter bypassed material restrictions");
        World dispute = pair(); auto& claimant = dispute.villagers[0]; claimant.goal = Goal::Forage; claimant.target_id = -1; claimant.action_timer = 2; claimant.hunger = 70; claimant.cooperation = .2;
        dispute.villagers[1].goal = Goal::Forage; dispute.villagers[1].food = 1;
        dispute.tile(32, 32) = {Terrain::Bush, 0, 0}; dispute.update();
        check(dispute.conflicts == 1 && claimant.relationships[1] < 0 && claimant.last_conflict_day == dispute.day(), "Competing claims did not affect real relationships");
        claimant.action_timer = 2; claimant.decision_timer = 9; dispute.update(); check(dispute.conflicts == 1, "Disputes bypassed their daily bound");
        claimant.hunger = 10; claimant.energy = 100; claimant.cooperation = .8; claimant.goal = Goal::Socialize; claimant.target_id = 1; claimant.action_timer = 3; claimant.decision_timer = 9;
        const double old = claimant.relationships[1]; dispute.update();
        check(dispute.reconciliations == 1 && claimant.relationships[1] > old, "Healthy cooperative residents did not repair damaged trust");

        World technology(9); technology.auto_develop = technology.population_growth = false; technology.villagers.resize(1);
        check(technology.plan(BuildingKind::Workshop, {30, 30}), "Workshop placement failed");
        auto& workshop = technology.buildings.back(); workshop.complete = true; workshop.wood_delivered = 12; workshop.stone_delivered = 8; workshop.work = 28;
        auto& engineer = technology.villagers[0]; engineer.x = engineer.y = 30; engineer.goal = Goal::Build; engineer.target_id = workshop.id; engineer.decision_timer = 9;
        check(!technology.research(engineer, workshop, .1), "Technology appeared without knowledge or materials");
        technology.knowledge = 8;
        check(!technology.research(engineer, workshop, .1), "Unsupplied research advanced");
        engineer.wood = 4; engineer.stone = 2; engineer.goal = Goal::DeliverMaterials; engineer.action_timer = 1; technology.update();
        check(workshop.upgrade_wood == 4 && workshop.upgrade_stone == 2 && engineer.wood + engineer.stone == 0, "Research supplies were not physically delivered");
        engineer.goal = Goal::Build; engineer.decision_timer = 9; engineer.action_timer = 0;
        check(technology.research(engineer, workshop, .1) && technology.technology_level == 0, "Research failed to require actual work");
        for (int n = 0; n < 400 && technology.technology_level == 0; ++n) technology.research(engineer, workshop, .1);
        check(technology.technology_level == 1 && workshop.upgrade_wood + workshop.upgrade_stone == 0, "Supplied tools did not complete and consume materials");
        technology.knowledge = 18; workshop.upgrade_wood = 6; workshop.upgrade_stone = 4;
        for (int n = 0; n < 600 && technology.technology_level == 1; ++n) technology.research(engineer, workshop, .1);
        check(technology.technology_level == 2 && !technology.research_available(), "Second technology failed or progressed without a bound");
        World basic(19); basic.auto_develop = basic.population_growth = false; basic.villagers.resize(1);
        basic.tile(32, 32) = {Terrain::Bush, 6, 0}; auto& gatherer = basic.villagers[0]; gatherer.x = gatherer.y = 32; gatherer.goal = Goal::Forage; gatherer.decision_timer = 9; gatherer.action_timer = 1.75;
        World equipped = basic; equipped.technology_level = 1; basic.update(); equipped.update();
        check(basic.villagers[0].food == 0 && equipped.villagers[0].food == 1, "Improved tools did not change gathering time");
        World farm(22); farm.auto_develop = farm.population_growth = false; farm.villagers.clear(); farm.plan(BuildingKind::Farm, {30, 30}); farm.buildings.back().complete = true; farm.buildings.back().growth = 44.8;
        World irrigation = farm; irrigation.technology_level = 2; farm.update(.2); irrigation.update(.2);
        check(irrigation.buildings.back().growth >= farm.buildings.back().growth || irrigation.buildings.back().food > farm.buildings.back().food, "Irrigation slowed crops");
        check(World::deserialize(technology.serialize()).serialize() == technology.serialize(), "Society/technology state did not save exactly");
        for (int n = 0; n < 700; ++n) technology.event("Event \"quoted\", number " + std::to_string(n));
        check(technology.chronicle.size() == 512 && technology.events.size() == 24, "History grew without bounds");
        const auto directory = std::filesystem::temp_directory_path() / "living-village-history-tests";
        export_history(technology, directory);
        std::ifstream history(directory / "village-history.csv"); std::string all((std::istreambuf_iterator<char>(history)), {});
        check(all.find("\"\"quoted\"\"") != std::string::npos && std::filesystem::exists(directory / "residents.csv") && std::filesystem::exists(directory / "village-statistics.csv"), "History export failed or CSV escaping lost quotes");
        std::filesystem::remove_all(directory);
        Interface ui; InputState press; press.pressed[static_cast<int>(Button::Confirm)] = true;
        ui.view.panel = Panel::Overview; ui.view.overview_page = 4; check(ui.update(press, technology, .033) == Request::ExportHistory, "Controller could not export histories");
        ui.view.overview_page = 5; check(ui.update(press, technology, .033) == Request::Volume, "Controller could not change volume");
        press = {}; press.pressed[static_cast<int>(Button::Next)] = true; check(ui.update(press, technology, .033) == Request::ToggleRumble, "Controller could not toggle rumble");
        std::cout << "PASS: physical meal sharing and barter, scarcity conflicts, reconciliation, supplied/time-gated technology, tool/crop effects, bounded histories and escaped CSV exports, controller settings\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
