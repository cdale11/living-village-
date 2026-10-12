#include "living_village/ui.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace village {
void Interface::cycle(const World& world, int direction) {
    const int count = static_cast<int>(world.villagers.size());
    view.selected = (view.selected + count + direction) % count;
    view.camera_x = world.villagers[view.selected].x;
    view.camera_y = world.villagers[view.selected].y;
    view.follow = true;
}
Request Interface::update(const InputState& input, const World& world, double elapsed) {
    if (input.exit_requested) return Request::Exit;
    view.selected = std::clamp(view.selected, 0, static_cast<int>(world.villagers.size()) - 1);
    if (input.hit(Button::Menu)) { view.panel = view.panel == Panel::Map ? Panel::Menu : Panel::Map; view.menu_index = 0; }
    else if (input.hit(Button::Back)) {
        view.panel = view.panel == Panel::Controls ? Panel::Menu : Panel::Map;
        view.menu_index = 0;
    } else if (input.hit(Button::Speed)) {
        view.panel = Panel::Speed; view.menu_index = view.speed == 1 ? 0 : view.speed == 2 ? 1 : 2;
    } else if (input.hit(Button::Overview)) view.panel = view.panel == Panel::Overview ? Panel::Map : Panel::Overview;
    else if (input.hit(Button::Inspect)) view.panel = view.panel == Panel::Inspector ? Panel::Map : Panel::Inspector;
    if (input.hit(Button::Previous)) cycle(world, -1);
    if (input.hit(Button::Next)) cycle(world, 1);
    if (input.hit(Button::ZoomOut)) view.tile_size = std::max(16, view.tile_size - 4);
    if (input.hit(Button::ZoomIn)) view.tile_size = std::min(48, view.tile_size + 4);
    repeat -= elapsed;
    if (std::abs(input.y) < .5) repeat = 0;
    if ((view.panel == Panel::Menu || view.panel == Panel::Speed) && std::abs(input.y) >= .5 && repeat <= 0) {
        const int count = view.panel == Panel::Menu ? 5 : 3;
        view.menu_index = (view.menu_index + count + (input.y > 0 ? 1 : -1)) % count;
        repeat = .2;
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
            }
        } else if (view.panel == Panel::Speed) {
            view.speed = 1 << view.menu_index; view.panel = Panel::Map;
        } else if (view.panel == Panel::Map) {
            double closest = std::numeric_limits<double>::max();
            for (std::size_t i = 0; i < world.villagers.size(); ++i) {
                const auto& p = world.villagers[i];
                double distance = std::hypot(p.x - view.camera_x, p.y - view.camera_y);
                if (distance < closest) { closest = distance; view.selected = static_cast<int>(i); }
            }
            view.panel = Panel::Inspector;
        }
    }
    return Request::None;
}
}
