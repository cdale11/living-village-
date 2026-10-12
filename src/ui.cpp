#include "living_village/ui.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

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
    if (input.hit(Button::Menu)) { view.panel = view.panel == Panel::Map ? Panel::Menu : Panel::Map; view.menu_index = 0; }
    else if (input.hit(Button::Back)) {
        view.panel = (view.panel == Panel::Controls || view.panel == Panel::NewVillage) ? Panel::Menu : view.panel == Panel::Build ? Panel::Tile : Panel::Map;
        view.menu_index = 0;
    } else if (input.hit(Button::Speed)) {
        view.panel = Panel::Speed; view.menu_index = view.speed == 1 ? 0 : view.speed == 2 ? 1 : 2;
    } else if (input.hit(Button::Overview)) view.panel = view.panel == Panel::Overview ? Panel::Map : Panel::Overview;
    else if (input.hit(Button::Inspect)) view.panel = view.panel == Panel::Inspector ? Panel::Map : Panel::Inspector;
    if (view.panel == Panel::Overview && view.overview_page == 5 && input.hit(Button::Next)) return Request::ToggleRumble;
    if (input.hit(Button::Previous)) cycle(world, -1);
    if (input.hit(Button::Next)) cycle(world, 1);
    if (input.hit(Button::ZoomOut)) view.tile_size = std::max(16, view.tile_size - 4);
    if (input.hit(Button::ZoomIn)) view.tile_size = std::min(48, view.tile_size + 4);
    repeat -= elapsed;
    if (std::abs(input.y) < .5) repeat = 0;
    if ((view.panel == Panel::Menu || view.panel == Panel::Speed || view.panel == Panel::Build) && std::abs(input.y) >= .5 && repeat <= 0) {
        const int count = view.panel == Panel::Speed ? 3 : view.panel == Panel::Menu ? 7 : 6;
        view.menu_index = (view.menu_index + count + (input.y > 0 ? 1 : -1)) % count;
        repeat = .2;
    }
    page_repeat -= elapsed;
    if (std::abs(input.x) < .5) page_repeat = 0;
    if (view.panel == Panel::Inspector && std::abs(input.x) >= .5 && page_repeat <= 0) {
        view.inspector_page = (view.inspector_page + 6 + (input.x > 0 ? 1 : -1)) % 6;
        page_repeat = .2;
    }
    if (view.panel == Panel::Overview && std::abs(input.x) >= .5 && page_repeat <= 0) {
        view.overview_page = (view.overview_page + 6 + (input.x > 0 ? 1 : -1)) % 6;
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
                case 0: view.panel = Panel::Map; break;
                case 1: return Request::Save;
                case 2: return Request::Load;
                case 3: view.panel = Panel::Controls; break;
                case 4: return Request::Exit;
                case 5: view.panel = Panel::NewVillage; break;
                case 6: return Request::PreviousVillage;
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
        else if (view.panel == Panel::Overview && view.overview_page == 3) return Request::ToggleLearning;
        else if (view.panel == Panel::Overview && view.overview_page == 4) return Request::ExportHistory;
        else if (view.panel == Panel::Overview && view.overview_page == 5) return Request::Volume;
        else if (view.panel == Panel::NewVillage) return Request::NewVillage;
    }
    return Request::None;
}
}
