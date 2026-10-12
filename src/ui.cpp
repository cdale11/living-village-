#include "living_village/ui.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <iomanip>
#include <sstream>

namespace village {
void Interface::cycle(const World& world, int direction) {
    const int count = static_cast<int>(world.villagers.size());
    for (int attempts = 0; attempts < count; ++attempts) {
        view.selected = (view.selected + count + direction) % count;
        if (world.villagers[view.selected].alive()) break;
    }
    view.camera_x = world.villagers[view.selected].x;
    view.camera_y = world.villagers[view.selected].y;
    view.follow = true;
}
Request Interface::update(const InputState& input, const World& world, double elapsed) {
    if (input.exit_requested) return Request::Exit;
    view.selected = std::clamp(view.selected, 0, static_cast<int>(world.villagers.size()) - 1);
    repeat -= elapsed; page_repeat -= elapsed;
    if (view.panel == Panel::SeedEdit || view.panel == Panel::NameEdit) {
        const bool seed = view.panel == Panel::SeedEdit;
        if (input.hit(Button::Back)) { view.panel = view.renaming ? Panel::Saves : Panel::NewVillage; return Request::None; }
        if (input.hit(Button::Inspect)) {
            if (seed) return Request::RandomSeed;
            view.edit_buffer[view.edit_cursor] = ' ';
        }
        if (std::abs(input.x) < .5) page_repeat = 0;
        if (std::abs(input.y) < .5) repeat = 0;
        if (std::abs(input.x) >= .5 && page_repeat <= 0) {
            const int count = static_cast<int>(view.edit_buffer.size());
            view.edit_cursor = (view.edit_cursor + count + (input.x > 0 ? 1 : -1)) % count; page_repeat = .15;
        }
        if (std::abs(input.y) >= .5 && repeat <= 0) {
            const std::string alphabet = seed ? "0123456789" : " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-";
            const auto found = alphabet.find(view.edit_buffer[view.edit_cursor]);
            const int index = found == std::string::npos ? 0 : static_cast<int>(found), count = static_cast<int>(alphabet.size());
            view.edit_buffer[view.edit_cursor] = alphabet[(index + count + (input.y > 0 ? -1 : 1)) % count]; repeat = .12;
        }
        if (input.hit(Button::Confirm)) {
            if (seed) {
                const auto number = std::stoull(view.edit_buffer);
                if (number > 4294967295ULL) { view.status = "Seed must be 0 to 4294967295"; return Request::None; }
                view.draft_seed = static_cast<std::uint32_t>(number); view.panel = Panel::NewVillage;
            } else {
                auto name = view.edit_buffer; const auto begin = name.find_first_not_of(' '), end = name.find_last_not_of(' ');
                name = begin == std::string::npos ? "" : name.substr(begin, end - begin + 1);
                if (!valid_world_name(name)) { view.status = "Enter a world name"; return Request::None; }
                if (view.renaming) { view.pending_name = name; view.panel = Panel::Saves; return Request::RenameSlot; }
                view.draft_name = name; view.panel = Panel::NewVillage;
            }
        }
        return Request::None;
    }
    if (input.hit(Button::Menu)) { view.panel = view.simulation_running() ? Panel::Menu : (view.has_world ? Panel::Map : Panel::Menu); view.menu_index = 0; }
    else if (input.hit(Button::Back)) {
        if (view.panel == Panel::ConfirmDelete) view.panel = Panel::Saves;
        else if (view.panel == Panel::ConfirmCreate) view.panel = Panel::NewVillage;
        else if (view.panel == Panel::Saves) view.panel = view.has_world ? Panel::Menu : Panel::NewVillage;
        else view.panel = (view.panel == Panel::Controls || view.panel == Panel::NewVillage) ? Panel::Menu : view.panel == Panel::Build ? Panel::Tile : (view.has_world ? Panel::Map : Panel::NewVillage);
        view.menu_index = 0;
    } else if (input.hit(Button::Speed) && view.has_world) {
        view.panel = Panel::Speed; view.menu_index = view.speed == 1 ? 0 : view.speed == 2 ? 1 : 2;
    } else if (input.hit(Button::Overview) && view.has_world && view.panel != Panel::Saves && view.panel != Panel::ConfirmDelete) view.panel = view.panel == Panel::Overview ? Panel::Map : Panel::Overview;
    else if (input.hit(Button::Inspect) && view.has_world && view.panel != Panel::Saves && view.panel != Panel::NewVillage && view.panel != Panel::ConfirmDelete) view.panel = view.panel == Panel::Inspector ? Panel::Map : Panel::Inspector;
    if (view.panel == Panel::NewVillage && input.hit(Button::Inspect)) return Request::RandomSeed;
    if (view.panel == Panel::Saves) {
        if (view.has_world && input.hit(Button::Previous)) return Request::CopyWorld;
        if (input.hit(Button::Inspect) && !view.saves.empty()) {
            const auto& slot = view.saves[view.menu_index];
            if (view.has_world && slot.id == view.active_slot) view.status = "Switch worlds before deleting the open village";
            else { view.pending_slot = slot.id; view.pending_name = slot.name; view.panel = Panel::ConfirmDelete; }
            return Request::None;
        }
        if (input.hit(Button::Overview) && !view.saves.empty()) {
            const auto& slot = view.saves[view.menu_index]; view.pending_slot = slot.id;
            view.edit_buffer = slot.name.substr(0, 24); for (char& c : view.edit_buffer) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
            view.edit_buffer.resize(24, ' '); view.edit_cursor = 0; view.renaming = true; view.panel = Panel::NameEdit;
            return Request::None;
        }
    }
    if (view.panel == Panel::Overview && view.overview_page == 5 && input.hit(Button::Next)) return Request::ToggleRumble;
    if (view.has_world && (view.panel == Panel::Map || view.panel == Panel::Inspector || view.panel == Panel::Overview)) {
        if (view.panel == Panel::Overview && view.overview_page == 7 && !world.animals.empty()) {
            const int count = static_cast<int>(world.animals.size());
            if (input.hit(Button::Previous)) view.selected_animal = (view.selected_animal + count - 1) % count;
            if (input.hit(Button::Next)) view.selected_animal = (view.selected_animal + 1) % count;
        } else {
            if (input.hit(Button::Previous)) cycle(world, -1);
            if (input.hit(Button::Next)) cycle(world, 1);
        }
    }
    if (input.hit(Button::ZoomOut)) view.tile_size = std::max(16, view.tile_size - 4);
    if (input.hit(Button::ZoomIn)) view.tile_size = std::min(48, view.tile_size + 4);
    if (std::abs(input.y) < .5) repeat = 0;
    if ((view.panel == Panel::Menu || view.panel == Panel::Speed || view.panel == Panel::Build || view.panel == Panel::NewVillage || view.panel == Panel::Saves) && std::abs(input.y) >= .5 && repeat <= 0) {
        const int count = view.panel == Panel::Speed ? 3 : view.panel == Panel::Menu ? 7 : view.panel == Panel::NewVillage ? 7 : view.panel == Panel::Saves ? std::max(1, static_cast<int>(view.saves.size())) : 6;
        view.menu_index = (view.menu_index + count + (input.y > 0 ? 1 : -1)) % count;
        repeat = .2;
    }
    if (std::abs(input.x) < .5) page_repeat = 0;
    if (view.panel == Panel::NewVillage && std::abs(input.x) >= .5 && page_repeat <= 0) {
        const int direction = input.x > 0 ? 1 : -1;
        if (view.menu_index == 2) view.draft_biome = static_cast<Biome>((static_cast<int>(view.draft_biome) + biome_count + direction) % biome_count);
        if (view.menu_index == 3) view.draft_geography = static_cast<Geography>((static_cast<int>(view.draft_geography) + geography_count + direction) % geography_count);
        if (view.menu_index == 4) view.draft_supplies = (view.draft_supplies + 3 + direction) % 3;
        page_repeat = .2;
    }
    if (view.panel == Panel::Inspector && std::abs(input.x) >= .5 && page_repeat <= 0) {
        view.inspector_page = (view.inspector_page + 6 + (input.x > 0 ? 1 : -1)) % 6;
        page_repeat = .2;
    }
    if (view.panel == Panel::Overview && std::abs(input.x) >= .5 && page_repeat <= 0) {
        view.overview_page = (view.overview_page + 9 + (input.x > 0 ? 1 : -1)) % 9;
        page_repeat = .2;
    }
    if (view.panel == Panel::Map) {
        if (std::abs(input.x) > .05 || std::abs(input.y) > .05) view.follow = false;
        if (view.follow) {
            const auto& p = world.villagers[view.selected];
            const double blend = std::min(1.0, elapsed * 6);
            view.camera_x += (p.x - view.camera_x) * blend;
            view.camera_y += (p.y - view.camera_y) * blend;
        }
        view.camera_x = std::clamp(view.camera_x + input.x * elapsed * 9, 0.0, 63.0);
        view.camera_y = std::clamp(view.camera_y + input.y * elapsed * 9, 0.0, 63.0);
    }
    if (input.hit(Button::Confirm)) {
        if (view.panel == Panel::Menu) {
            switch (view.menu_index) {
                case 0: view.panel = view.has_world ? Panel::Map : Panel::NewVillage; break;
                case 1: return Request::Save;
                case 2: return Request::Load;
                case 3: view.panel = Panel::Controls; break;
                case 4: return Request::Exit;
                case 5:
                    view.panel = Panel::NewVillage; view.menu_index = 0; view.draft_seed = world.seed + 1;
                    view.draft_biome = world.biome; view.draft_geography = world.geography; view.draft_name = "New Village"; break;
                case 6: view.panel = Panel::Saves; view.menu_index = 0; return Request::OpenSaves;
            }
        } else if (view.panel == Panel::Speed) {
            view.speed = 1 << view.menu_index; view.panel = Panel::Map;
        } else if (view.panel == Panel::Map) {
            view.selected_tile = {static_cast<int>(std::round(view.camera_x)), static_cast<int>(std::round(view.camera_y))};
            view.panel = Panel::Tile;
        } else if (view.panel == Panel::Tile && world.can_plan(view.selected_tile)) { view.panel = Panel::Build; view.menu_index = 0; }
        else if (view.panel == Panel::Tile && world.building_at(view.selected_tile) < 0) return Request::Designate;
        else if (view.panel == Panel::Build) return Request::Plan;
        else if (view.panel == Panel::Overview && view.overview_page < 2) return view.overview_page == 0 ? Request::ToggleDevelopment : Request::ToggleGrowth;
        else if (view.panel == Panel::Overview && (view.overview_page == 3 || view.overview_page == 8)) return Request::ToggleLearning;
        else if (view.panel == Panel::Overview && view.overview_page == 4) return Request::ExportHistory;
        else if (view.panel == Panel::Overview && view.overview_page == 5) return Request::Volume;
        else if (view.panel == Panel::NewVillage) {
            if (view.menu_index == 0) { view.edit_buffer = view.draft_name.substr(0, 24); for (char& c : view.edit_buffer) if (c >= 'a' && c <= 'z') c -= 'a' - 'A'; view.edit_buffer.resize(24, ' '); view.edit_cursor = 0; view.renaming = false; view.panel = Panel::NameEdit; }
            else if (view.menu_index == 1) { std::ostringstream number; number << std::setw(10) << std::setfill('0') << view.draft_seed; view.edit_buffer = number.str(); view.edit_cursor = 0; view.renaming = false; view.panel = Panel::SeedEdit; }
            else if (view.menu_index == 2) view.draft_biome = static_cast<Biome>((static_cast<int>(view.draft_biome) + 1) % biome_count);
            else if (view.menu_index == 3) view.draft_geography = static_cast<Geography>((static_cast<int>(view.draft_geography) + 1) % geography_count);
            else if (view.menu_index == 4) view.draft_supplies = (view.draft_supplies + 1) % 3;
            else if (view.menu_index == 5) { if (view.has_world) view.panel = Panel::ConfirmCreate; else return Request::NewVillage; }
            else if (view.menu_index == 6) { view.panel = Panel::Saves; view.menu_index = 0; return Request::OpenSaves; }
        } else if (view.panel == Panel::ConfirmCreate) return Request::NewVillage;
        else if (view.panel == Panel::Saves && !view.saves.empty()) return Request::LoadSlot;
        else if (view.panel == Panel::Saves && view.saves.empty()) { view.panel = Panel::NewVillage; view.menu_index = 0; }
        else if (view.panel == Panel::ConfirmDelete) return Request::DeleteSlot;
    }
    return Request::None;
}
}
