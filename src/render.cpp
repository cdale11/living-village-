#include "living_village/render.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace village {
namespace {
constexpr SDL_Color ink{25, 38, 39, 255}, cream{244, 235, 205, 255}, muted{162, 187, 167, 255};
std::string decimal(double value) {
    std::ostringstream out; out << std::fixed << std::setprecision(2) << value; return out.str();
}
// Original 5x7 bitmap lettering, no font or texture dependency.
const char* glyph(char c) {
    switch (c) {
        case 'A': return "01110100011000111111100011000110001";
        case 'B': return "11110100011000111110100011000111110";
        case 'C': return "01111100001000010000100001000001111";
        case 'D': return "11110100011000110001100011000111110";
        case 'E': return "11111100001000011110100001000011111";
        case 'F': return "11111100001000011110100001000010000";
        case 'G': return "01111100001000010111100011000101111";
        case 'H': return "10001100011000111111100011000110001";
        case 'I': return "11111001000010000100001000010011111";
        case 'J': return "00111000100001000010100101001001100";
        case 'K': return "10001100101010011000101001001010001";
        case 'L': return "10000100001000010000100001000011111";
        case 'M': return "10001110111010110101100011000110001";
        case 'N': return "10001110011010110011100011000110001";
        case 'O': return "01110100011000110001100011000101110";
        case 'P': return "11110100011000111110100001000010000";
        case 'Q': return "01110100011000110001101011001001101";
        case 'R': return "11110100011000111110101001001010001";
        case 'S': return "01111100001000001110000010000111110";
        case 'T': return "11111001000010000100001000010000100";
        case 'U': return "10001100011000110001100011000101110";
        case 'V': return "10001100011000110001100010101000100";
        case 'W': return "10001100011000110101101011101110001";
        case 'X': return "10001100010101000100010101000110001";
        case 'Y': return "10001100010101000100001000010000100";
        case 'Z': return "11111000010001000100010001000011111";
        case '0': return "01110100011001110101110011000101110";
        case '1': return "00100011000010000100001000010001110";
        case '2': return "01110100010000100010001000100011111";
        case '3': return "11110000010000101110000010000111110";
        case '4': return "00010001100101010010111110001000010";
        case '5': return "11111100001000011110000010000111110";
        case '6': return "01110100001000011110100011000101110";
        case '7': return "11111000010001000100010000100001000";
        case '8': return "01110100011000101110100011000101110";
        case '9': return "01110100011000101111000010000101110";
        case ':': return "00000001000010000000001000010000000";
        case '.': return "00000000000000000000000000010000100";
        case '-': return "00000000000000011111000000000000000";
        case '/': return "00001000100001000100010000100010000";
        case '+': return "00000001000010011111001000010000000";
        case '>': return "10000010000001000001000100100010000";
        case '<': return "00001000100010001000001000001000001";
        case '|': return "00100001000010000100001000010000100";
        case '(': return "00010001000100001000010000010000010";
        case ')': return "01000001000001000010000100010001000";
        default: return "00000000000000000000000000000000000";
    }
}
}
void Renderer::rect(int x, int y, int w, int h, SDL_Color color) {
    sdl.SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect bounds{x, y, w, h}; sdl.RenderFillRect(renderer, &bounds);
}
void Renderer::outline(int x, int y, int w, int h, SDL_Color color) {
    sdl.SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect bounds{x, y, w, h}; sdl.RenderDrawRect(renderer, &bounds);
}
void Renderer::text(int x, int y, const std::string& value, SDL_Color color, int scale, int width) {
    const int origin = x;
    for (unsigned char raw : value) {
        if (raw == '\n') { x = origin; y += 9 * scale; continue; }
        if (x + 5 * scale > origin + width) { x = origin; y += 9 * scale; }
        const char* pattern = glyph(static_cast<char>(std::toupper(raw)));
        for (int row = 0; row < 7; ++row)
            for (int col = 0; col < 5; ++col)
                if (pattern[row * 5 + col] == '1') rect(x + col * scale, y + row * scale, scale, scale, color);
        x += 6 * scale;
    }
}
void Renderer::meter(int x, int y, const std::string& label, double value, SDL_Color color) {
    text(x, y, label + " " + std::to_string(static_cast<int>(value)), cream);
    rect(x + 180, y, 160, 12, {47, 60, 54, 255});
    rect(x + 180, y, static_cast<int>(std::clamp(value, 0.0, 100.0) * 1.6), 12, color);
}

void Renderer::draw(const World& world, const View& view) {
    sdl.SetRenderDrawColor(renderer, 20, 36, 35, 255); sdl.RenderClear(renderer);
    const int size = view.tile_size;
    const int selected_index = std::clamp(view.selected, 0, static_cast<int>(world.villagers.size()) - 1);
    const int selected_id = world.villagers[selected_index].id;
    const int left = static_cast<int>(view.camera_x - 360.0 / size) - 1;
    const int top = static_cast<int>(view.camera_y - 240.0 / size) - 1;
    const auto px = [&](double x) { return static_cast<int>(360 + (x - view.camera_x - .5) * size); };
    const auto py = [&](double y) { return static_cast<int>(240 + (y - view.camera_y - .5) * size); };
    for (int y = std::max(0, top); y < std::min(64, top + 480 / size + 4); ++y) {
        for (int x = std::max(0, left); x < std::min(64, left + 720 / size + 4); ++x) {
            const auto& tile = world.tile(x, y);
            const int sx = px(x), sy = py(y), unit = std::max(1, size / 8);
            const Uint8 shade = static_cast<Uint8>(108 + ((x * 17 + y * 31 + world.seed) % 3) * 5);
            const SDL_Color meadow[] = {{70, shade, 65, 255}, {76, static_cast<Uint8>(shade + 8), 60, 255}, {110, shade, 62, 255}, {140, 157, 151, 255}};
            rect(sx, sy, size + 1, size + 1, meadow[world.season()]);
            rect(sx + unit, sy + size - unit * 2, unit, unit, {91, 138, 77, 255});
            switch (tile.terrain) {
                case Terrain::Tree:
                    rect(sx + size / 2 - unit, sy + size / 2, unit * 2, size / 2, {98, 70, 43, 255});
                    if (tile.wood == 0) break;
                    rect(sx + unit, sy + unit * 2, size - unit * 2, size / 2, {32, 76, 47, 255});
                    rect(sx + unit * 2, sy + unit, size - unit * 4, size / 2, {42, 93, 52, 255});
                    rect(sx + unit * 3, sy, size - unit * 6, unit * 2, {66, 114, 59, 255});
                    break;
                case Terrain::Stone:
                    if (tile.stone == 0) { rect(sx + size / 3, sy + size * 2 / 3, size / 3, unit, {106, 116, 108, 255}); break; }
                    rect(sx + size / 4, sy + size / 3, size / 2, size / 2, {106, 116, 108, 255});
                    rect(sx + size / 3, sy + size / 4, size / 3, size / 4, {169, 177, 156, 255});
                    break;
                case Terrain::Water:
                    rect(sx, sy, size + 1, size + 1, {48, 102, 129, 255});
                    rect(sx + size / 4, sy + size / 3 + static_cast<int>((world.ticks / 8 + x + y) % 3) * unit, size / 2, unit, {75, 132, 150, 255});
                    break;
                case Terrain::Bush:
                    rect(sx + unit, sy + size / 3, size - unit * 2, size / 2, {37, 82, 45, 255});
                    if (tile.food > 0) {
                        rect(sx + unit * 2, sy + size / 2, unit, unit, {197, 80, 94, 255});
                        rect(sx + size / 2 + unit, sy + size / 2 - unit, unit, unit, {214, 107, 116, 255});
                    }
                    break;
                case Terrain::Grass: break;
            }
            if (tile.loose_food + tile.loose_wood + tile.loose_stone > 0) rect(sx + size - unit * 3, sy + size - unit * 3, unit * 2, unit * 2, {220, 190, 124, 255});
        }
    }
    for (Point order : world.gather_orders) {
        outline(px(order.x - 2), py(order.y - 2), size * 5, size * 5, {191, 218, 127, 180});
        outline(px(order.x), py(order.y), size, size, {213, 238, 152, 255});
    }
    for (const auto& b : world.buildings) {
        const int x = px(b.position.x), y = py(b.position.y), u = std::max(1, size / 8);
        if (x < -size || x > 720 || y < -size || y > 480) continue;
        if (!b.complete) {
            rect(x + 1, y + 1, size - 2, size - 2, {116, 94, 63, 255});
            outline(x + 2, y + 2, size - 4, size - 4, {226, 199, 122, 255});
            rect(x + 3, y + size / 2, size - 6, u, {171, 149, 97, 255});
            rect(x + size / 2, y + 3, u, size - 6, {171, 149, 97, 255});
            rect(x + 2, y + size - 4, static_cast<int>((size - 4) * b.work / building_cost(b.kind).work), 2, {169, 215, 135, 255});
            continue;
        }
        switch (b.kind) {
            case BuildingKind::Workshop:
                rect(x + u, y + u, size - u * 2, size - u * 2, {104, 81, 64, 255});
                rect(x + u, y + u, size - u * 2, u * 2, {167, 135, 96, 255});
                rect(x + u * 2, y + u * 3, u, u * 3, {180, 182, 166, 255});
                rect(x + u, y + u * 3, u * 3, u, {180, 182, 166, 255});
                break;
            case BuildingKind::Shelter:
                rect(x + u, y + size / 3, size - 2 * u, size * 2 / 3, {191, 160, 109, 255});
                for (int row = 0; row < size / 2; ++row) rect(x + size / 2 - row, y + row, row * 2 + 1, 1, {116, 80, 54, 255});
                rect(x + size / 2 - u, y + size * 2 / 3, u * 2, size / 3, {65, 61, 46, 255});
                break;
            case BuildingKind::FoodStorage:
                rect(x + u, y + u * 2, size - u * 2, size - u * 2, {139, 102, 69, 255});
                rect(x, y, size, u * 2, {102, 134, 121, 255});
                rect(x + size / 3, y + size / 2, size / 3, u * 2, {206, 93, 107, 255});
                break;
            case BuildingKind::Stockpile:
                rect(x + u, y + u, size - 2 * u, size - 2 * u, {125, 88, 49, 255});
                outline(x + u, y + u, size - 2 * u, size - 2 * u, {209, 166, 94, 255});
                rect(x + size / 2 - 1, y + u, 2, size - 2 * u, {198, 154, 88, 255});
                rect(x + u, y + size / 2 - 1, size - 2 * u, 2, {198, 154, 88, 255});
                break;
            case BuildingKind::Farm:
                rect(x, y, size, size, {119, 86, 56, 255});
                for (int row = 1; row < 4; ++row) {
                    rect(x + u, y + row * size / 4, size - 2 * u, u, {79, 66, 45, 255});
                    if (b.food > 0) for (int col = 1; col < 4; ++col)
                        rect(x + col * size / 4, y + row * size / 4 - u, u, u, {190, 181, 73, 255});
                }
                break;
            case BuildingKind::Campfire:
                rect(x + size / 4, y + size / 2, size / 2, size / 3, {123, 127, 110, 255});
                if (b.fuel > 0) {
                    rect(x + size / 3, y + size / 3, size / 3, size / 3, {199, 93, 45, 255});
                    rect(x + size / 2 - u, y + size / 4, u * 2, size / 3, {255, 190, 80, 255});
                }
                break;
        }
    }
    const SDL_Color shirts[] = {{184, 95, 79, 255}, {84, 145, 193, 255}, {177, 136, 188, 255},
                                {217, 169, 82, 255}, {130, 166, 99, 255}, {126, 177, 174, 255}};
    for (const auto& person : world.villagers) {
        const int x = px(person.x + .5), y = py(person.y + .5);
        if (x < -size || x > 720 + size || y < -size || y > 480 + size) continue;
        if (!person.alive() && person.id != selected_id && world.day() - person.death_day > 3) continue;
        const int unit = std::max(2, size / (person.child() ? 12 : 9));
        rect(x - unit * 2, y + unit * 2, unit * 4, unit, {48, 71, 45, 255});
        if (!person.alive()) {
            if (person.id == selected_id || world.day() - person.death_day <= 3) {
                rect(x - unit, y, unit * 2, unit, {129, 119, 106, 255});
                rect(x, y - unit, unit / 2 + 1, unit * 3, {129, 119, 106, 255});
            }
            continue;
        }
        const int bob = person.path.empty() && (person.goal == Goal::Build || person.goal == Goal::Forage || person.goal == Goal::GatherWood || person.goal == Goal::TendFarm) ? static_cast<int>((world.ticks / 5) % 2) : 0;
        const int walk = person.path.empty() ? bob : static_cast<int>((world.ticks / 3) % 2);
        rect(x - unit, y + unit, unit, unit * (1 + walk), {55, 55, 58, 255});
        rect(x + 1, y + unit, unit, unit * (2 - walk), {55, 55, 58, 255});
        rect(x - unit, y - unit, unit * 2, unit * 2, shirts[person.id % 6]);
        rect(x - unit, y - unit * 3, unit * 2, unit * 2, {237, 198, 141, 255});
        rect(x - unit, y - unit * 3, unit * 2, unit / 2 + 1, {89, 66, 48, 255});
        if (person.wood + person.stone > 0) rect(x + unit, y, unit * 2, unit * 2, {185, 144, 83, 255});
        else if (person.food > 0) rect(x + unit, y, unit, unit * 2, {207, 111, 119, 255});
        if (person.id == selected_id) {
            if (person.hunger >= 75 || person.health < 50) rect(x - unit, y - unit * 5, unit * 2, unit, {222, 120, 83, 255});
            else if (person.goal == Goal::Socialize || person.goal == Goal::Care) rect(x - unit, y - unit * 5, unit * 2, unit, {159, 191, 214, 255});
        }
        if (person.id == selected_id) outline(x - unit * 2 - 2, y - unit * 3 - 2, unit * 4 + 4, unit * 6 + 4, {246, 223, 130, 255});
    }
    const Uint8 darkness = static_cast<Uint8>((1 - world.daylight()) * 65);
    rect(0, 0, 720, 480, {12, 24, 45, darkness});
    if (world.daylight() < .5) for (const auto& b : world.buildings) if (b.complete) {
        const int x = px(b.position.x), y = py(b.position.y);
        if (b.kind == BuildingKind::Campfire && b.fuel > 0) {
            rect(x - size / 2, y - size / 2, size * 2, size * 2, {233, 174, 77, 18});
            rect(x + size / 3, y + size / 3, size / 3, size / 3, {240, 172, 62, 230});
        } else if (b.kind == BuildingKind::Shelter) rect(x + size / 2, y + size / 2, size / 7, size / 6, {233, 193, 92, 220});
    }
    if (world.raining()) for (int i = 0; i < 24; ++i) rect(static_cast<int>((i * 137 + world.ticks * 2) % 720), 54 + static_cast<int>((i * 71 + world.ticks * 3) % 370), 1, 4, {172, 206, 223, 95});
    outline(351, 231, 18, 18, {231, 225, 187, 170});
    rect(0, 0, 720, 54, ink);
    text(12, 8, "LIVING VILLAGE", cream);
    text(210, 8, "DAY " + std::to_string(world.day()), muted);
    text(330, 8, std::to_string(world.alive_count()) + " PEOPLE", muted);
    text(492, 8, std::to_string(view.speed) + "X", cream);
    text(555, 8, view.panel == Panel::Map ? "RUNNING" : "PAUSED", cream);
    text(12, 32, "FOOD " + std::to_string(world.food_stock) + "/" + std::to_string(world.food_capacity()) +
         "  WOOD " + std::to_string(world.wood_stock) + "  STONE " + std::to_string(world.stone_stock) + "  " + world.season_name() + (world.raining() ? " RAIN" : " DRY"), muted);
    rect(0, 426, 720, 54, ink);
    text(12, 433, "A TILE/PLAN  X PERSON  Y VILLAGE  FN+START EXIT", muted);
    text(12, 455, view.status, cream, 2, 690);
    if (view.panel != Panel::Map) {
        rect(0, 54, 720, 372, {8, 19, 21, 155});
        rect(72, 56, 576, 356, {26, 43, 43, 248});
        outline(72, 56, 576, 356, {127, 153, 123, 255});
        switch (view.panel) {
            case Panel::Menu: {
                text(102, 80, "VILLAGE MENU", cream, 3);
                const char* entries[] = {"Resume", "Save village", "Load village", "Controls", "Save and exit", "New village", "Switch previous village"};
                for (int i = 0; i < 7; ++i) {
                    if (view.menu_index == i) rect(96, 122 + i * 32, 528, 28, {63, 89, 73, 255});
                    text(112, 130 + i * 32, std::string(view.menu_index == i ? "> " : "  ") + entries[i], cream);
                }
                text(102, 366, "A SELECT   B BACK", muted);
                break;
            }
            case Panel::Inspector: {
                const auto& p = world.villagers.at(selected_index);
                text(102, 80, p.name + " - AGE " + std::to_string(p.age), cream, 3);
                if (view.inspector_page == 0) {
                    text(102, 122, p.alive() ? std::string(goal_name(p.goal)) + " | SCORE " + std::to_string(static_cast<int>(p.decision_score)) : "Died: " + p.death_reason, muted);
                    meter(102, 154, "Hunger", p.hunger, {196, 116, 68, 255});
                    meter(102, 182, "Energy", p.energy, {126, 166, 99, 255});
                    meter(102, 210, "Health", p.health, {170, 137, 139, 255});
                    text(102, 246, "Carried: F " + std::to_string(p.food) + " W " + std::to_string(p.wood) + " S " + std::to_string(p.stone), muted);
                    text(102, 276, "WHY: " + p.reason, cream, 2, 504);
                    if (p.target_id >= 0 && p.goal != Goal::Socialize && p.goal != Goal::Care && p.goal != Goal::CollectMeal && p.target_id < static_cast<int>(world.buildings.size())) {
                        const auto& b = world.buildings[p.target_id];
                        if (b.initiator >= 0) text(102, 336, "Project owner: " + (b.initiator_name.empty() ? world.person_name(b.initiator) : b.initiator_name), muted);
                    }
                } else if (view.inspector_page == 1) {
                    text(102, 122, "PERSONALITY AND SKILLS", muted);
                    meter(102, 156, "Cooperation", p.cooperation * 100, {126, 166, 99, 255});
                    meter(102, 184, "Loneliness", p.loneliness, {170, 137, 139, 255});
                    text(102, 222, "Gather skill: " + std::to_string(static_cast<int>(p.gather_skill * 100)), cream);
                    text(102, 248, "Build skill: " + std::to_string(static_cast<int>(p.build_skill * 100)), cream);
                    text(102, 274, "Farm skill: " + std::to_string(static_cast<int>(p.farm_skill * 100)), cream);
                    text(102, 338, std::string("Role: ") + occupation_name(p.occupation), cream);
                    text(102, 306, "Known food/wood/stone: " + std::to_string(p.known_food.size()) + "/" + std::to_string(p.known_wood.size()) + "/" + std::to_string(p.known_stone.size()), muted);
                } else if (view.inspector_page == 2) {
                    text(102, 122, "RELATIONSHIPS / EXPERIENCES", muted);
                    int line = 0;
                    for (const auto& other : world.villagers) if (other.alive() && other.id != p.id && line < 5)
                        text(102, 152 + line++ * 24, other.name + ": " + std::to_string(static_cast<int>(p.relationships[other.id])), cream);
                    for (std::size_t i = p.history.size() > 2 ? p.history.size() - 2 : 0, row = 0; i < p.history.size(); ++i, ++row)
                        text(102, 286 + static_cast<int>(row) * 32, p.history[i], muted, 2, 504);
                }
                if (view.inspector_page == 3) {
                    text(102, 122, "HOUSEHOLD AND FAMILY", muted);
                    text(102, 152, p.home >= 0 ? "Home: shelter " + std::to_string(world.buildings[p.home].position.x) + "/" + std::to_string(world.buildings[p.home].position.y) : "Home: camp clearing", cream);
                    text(102, 180, "Partner: " + (p.partner >= 0 ? world.person_name(p.partner) : "None"), cream);
                    for (int i = 0; i < 2; ++i) {
                        const auto* parent = world.person(p.parents[i]);
                        const auto name = p.parent_names[i].empty() ? "Unknown" : p.parent_names[i] + (!parent || !parent->alive() ? " (late)" : "");
                        text(102, 208 + i * 28, "Parent: " + name, cream, 2, 504);
                    }
                    std::string children;
                    for (const auto& child : world.villagers) if (child.alive() && (child.parents[0] == p.id || child.parents[1] == p.id)) {
                        if (!children.empty()) children += " / ";
                        children += child.name;
                    }
                    text(102, 268, "Children: " + (children.empty() ? "None" : children.substr(0, 70)), cream, 2, 504);
                    text(102, 318, "Family drive: " + std::to_string(static_cast<int>(p.family_drive * 100)), muted);
                    text(102, 342, p.alive() ? "Birthday in " + std::to_string(days_per_year - p.age_days) + " days" : "Remembered since day " + std::to_string(p.death_day), muted);
                }
                if (view.inspector_page == 4) {
                    const auto& brain = p.brain;
                    text(102, 122, "PERSONAL MLP - ONLINE LEARNING", muted);
                    text(102, 154, "Updates " + std::to_string(brain.updates) + " | " + (world.learning_enabled ? "Training on" : "Training paused"), cream);
                    text(102, 182, "Base " + decimal(brain.selected_base) + " | Learned " + decimal(brain.selected_adjustment), cream);
                    text(102, 210, "Last reward " + decimal(brain.last_reward) + " | Error " + decimal(brain.last_error), muted);
                    text(102, 240, "Learned context effects:", cream);
                    const char* labels[] = {"Hunger", "Energy", "Health", "Loneliness", "Cooperation", "Curiosity", "Family drive", "Gather skill", "Build skill", "Farm skill", "Carried food", "Materials", "Shared food", "Wood demand", "Stone demand", "Spare beds", "Trip distance", "Base utility"};
                    std::array<std::pair<double, int>, brain_context> effects{};
                    const double prediction = brain.predict(brain.pending);
                    for (int i = 0; i < brain_context; ++i) {
                        auto ablated = brain.pending; ablated[brain_actions + i] = 0;
                        effects[i] = {18 * (prediction - brain.predict(ablated)), i};
                    }
                    std::stable_sort(effects.begin(), effects.end(), [](const auto& a, const auto& b) { return std::abs(a.first) > std::abs(b.first); });
                    for (int i = 0; i < 3; ++i) text(102, 268 + i * 26, std::string(labels[effects[i].second]) + ": " + decimal(effects[i].first), muted);
                }
                if (view.inspector_page == 5) {
                    const auto& brain = p.brain;
                    text(102, 122, std::string("ROLE: ") + occupation_name(p.occupation), cream);
                    text(102, 150, "Replay " + std::to_string(brain.replay_size) + "/32 | Episodes " + std::to_string(brain.episodes), muted);
                    text(102, 176, "Replayed " + std::to_string(brain.replay_updates) + " | Trials " + std::to_string(brain.experiments), muted);
                    text(102, 202, "Selected trial bonus " + decimal(brain.selected_exploration), muted);
                    for (int i = 0; i < work_types; ++i)
                        text(102, 232 + i * 18, std::string(occupation_name(static_cast<Occupation>(i + 1))) + ": " + decimal(p.practice[i]), cream);
                }
                text(102, 366, "LEFT/RIGHT DETAILS   L1/R1 PEOPLE", muted);
                break;
            }
            case Panel::Overview: {
                text(102, 80, view.overview_page == 0 ? "VILLAGE LIFE" : view.overview_page == 1 ? "POPULATION" : view.overview_page == 2 ? "VILLAGE HISTORY" : view.overview_page == 3 ? "NEURAL LEARNING" : view.overview_page == 4 ? "ECONOMY / HISTORY" : "SOUND / VIBRATION", cream, 3);
                if (view.overview_page == 0) {
                    int finished = 0; for (const auto& b : world.buildings) if (b.complete) ++finished;
                    text(102, 126, "Built " + std::to_string(finished) + " | Plans " + std::to_string(world.buildings.size() - finished), muted);
                    text(102, 164, "Residents " + std::to_string(world.alive_count()) + " | Beds " + std::to_string(world.shelter_capacity()), cream);
                    text(102, 208, "A: AUTO PLANS " + std::string(world.auto_develop ? "ON" : "OFF"), cream);
                    text(102, 256, "Left/right: population, log and learning", muted, 2, 504);
                } else if (view.overview_page == 1) {
                    text(102, 126, "Adults " + std::to_string(world.alive_count() - world.child_count()) + " | Children " + std::to_string(world.child_count()), cream);
                    text(102, 164, "Births " + std::to_string(world.births) + " | Arrivals " + std::to_string(world.arrivals), cream);
                    text(102, 202, "Deaths " + std::to_string(world.deaths) + " | Year " + std::to_string(1 + (world.day() - 1) / days_per_year), muted);
                    text(102, 248, "A: NEW BIRTHS/ARRIVALS " + std::string(world.population_growth ? "ON" : "OFF"), cream);
                    text(102, 294, world.growth_status(), muted, 2, 504);
                } else if (view.overview_page == 3) {
                    std::uint64_t total = 0; for (const auto& p : world.villagers) total += p.brain.updates;
                    text(102, 126, "Each villager has a personal MLP", cream);
                    text(102, 160, "37 inputs / 12 hidden / 1 value", muted);
                    text(102, 194, "Retained updates: " + std::to_string(total), cream);
                    text(102, 238, "A: ONLINE LEARNING " + std::string(world.learning_enabled ? "ON" : "PAUSED"), cream);
                    text(102, 284, "Learns from real outcomes with bounded replay and healthy exploration. X shows learning and practiced roles.", muted, 2, 504);
                } else if (view.overview_page == 4) {
                    text(102, 126, "Trades " + std::to_string(world.trade_count) + " | Meals shared " + std::to_string(world.help_count), cream);
                    text(102, 162, "Disputes " + std::to_string(world.conflicts) + " | Repairs " + std::to_string(world.reconciliations), muted);
                    text(102, 198, "Knowledge " + decimal(world.knowledge) + " | Tech " + std::to_string(world.technology_level) + "/2", cream);
                    text(102, 236, "History " + std::to_string(world.chronicle.size()) + " events | " + std::to_string(world.statistics.size()) + " days", muted);
                    text(102, 280, "A: EXPORT HISTORY AND STATISTICS", cream);
                    text(102, 320, "CSV files are kept in saves/exports", muted);
                } else if (view.overview_page == 5) {
                    text(102, 136, "A: SOUND VOLUME " + std::to_string(world.volume), cream);
                    text(102, 186, "R1: VIBRATION " + std::string(world.rumble_enabled ? "ON" : "OFF"), cream);
                    text(102, 250, "Short sounds mark choices and village events. Vibration requires SDL support from the device driver.", muted, 2, 504);
                } else {
                    for (std::size_t i = world.events.size() > 4 ? world.events.size() - 4 : 0, line = 0; i < world.events.size(); ++i, ++line)
                        text(102, 126 + static_cast<int>(line) * 54, world.events[i], cream, 2, 504);
                }
                text(102, 366, "LEFT/RIGHT PAGES   B BACK", muted); break;
            }
            case Panel::NewVillage:
                text(102, 80, "START A NEW VILLAGE", cream, 3);
                text(102, 144, "Your current village is kept in the previous-village save slot.", cream, 2, 504);
                text(102, 222, "You can switch back from the menu.", muted, 2, 504);
                text(102, 314, "A START FRESH   B CANCEL", cream); break;
            case Panel::Tile: {
                const auto at = view.selected_tile;
                const auto& cell = world.tile(at.x, at.y);
                const char* terrain[] = {"Grass", "Tree", "Stone", "Water", "Berry bush"};
                text(102, 80, "TILE " + std::to_string(at.x) + "/" + std::to_string(at.y), cream, 3);
                text(102, 122, terrain[static_cast<int>(cell.terrain)], muted);
                const int id = world.building_at(at);
                if (id >= 0) {
                    const auto& b = world.buildings[id];
                    const auto cost = building_cost(b.kind);
                    text(102, 162, std::string(building_name(b.kind)) + (b.complete ? " - COMPLETE" : " - CONSTRUCTION"), cream);
                    if (!b.complete) {
                        text(102, 202, "Wood " + std::to_string(b.wood_delivered) + "/" + std::to_string(cost.wood) + " | Stone " + std::to_string(b.stone_delivered) + "/" + std::to_string(cost.stone), cream);
                        text(102, 236, "Work " + std::to_string(static_cast<int>(b.work)) + "/" + std::to_string(static_cast<int>(cost.work)), cream);
                        text(102, 278, b.supplied() ? "Materials ready - needs builders" : "Villagers gather and deliver supplies", muted);
                        text(102, 312, "Contributors: " + std::to_string(b.contributors.size()), muted);
                    } else if (b.kind == BuildingKind::Shelter) {
                        text(102, 202, "Four beds - faster rest", cream);
                        text(102, 244, "Residents: " + std::to_string(world.home_occupancy(b.id)) + "/4", muted);
                    }
                    else if (b.kind == BuildingKind::Farm) text(102, 202, "Ripe food " + std::to_string(b.food) + "/12 - renewable crops", cream);
                    else if (b.kind == BuildingKind::FoodStorage) text(102, 202, "Adds 40 to shared food capacity", cream);
                    else if (b.kind == BuildingKind::Stockpile) text(102, 202, "Adds 20 to wood and stone capacity", cream);
                    else text(102, 202, b.fuel > 0 ? "Lit - warmth and social comfort" : "No fuel - needs shared wood", cream);
                } else {
                    text(102, 172, "Food " + std::to_string(cell.food) + " | Wood " + std::to_string(cell.wood) + " | Stone " + std::to_string(cell.stone), cream);
                    if (cell.loose_food + cell.loose_wood + cell.loose_stone > 0) text(102, 198, "Loose F " + std::to_string(cell.loose_food) + " W " + std::to_string(cell.loose_wood) + " S " + std::to_string(cell.loose_stone), muted);
                    const bool resource = cell.terrain == Terrain::Tree || cell.terrain == Terrain::Stone || cell.terrain == Terrain::Bush;
                    const bool designated = std::find(world.gather_orders.begin(), world.gather_orders.end(), at) != world.gather_orders.end();
                    text(102, 232, world.can_plan(at) ? "A: PLACE A CONSTRUCTION PLAN" : resource ? (designated ? "A: CANCEL GATHERING AREA" : "A: DESIGNATE GATHERING AREA") : "Plans need reachable empty grass", muted, 2, 504);
                    if (resource) text(102, 276, "5X5 area - shared priorities, not orders", muted, 2, 504);
                }
                text(102, 354, "B BACK", muted); break;
            }
            case Panel::Build: {
                text(102, 80, "CONSTRUCTION PLAN", cream, 3);
                for (int i = 0; i < 6; ++i) {
                    const auto kind = static_cast<BuildingKind>(i); const auto cost = building_cost(kind);
                    if (i == view.menu_index) rect(96, 122 + i * 36, 528, 32, {63, 89, 73, 255});
                    text(112, 130 + i * 36, std::string(building_name(kind)) + "  W" + std::to_string(cost.wood) + " S" + std::to_string(cost.stone), cream);
                }
                text(102, 366, "A PLAN   B CANCEL - SUPPLIES REQUIRED", muted); break;
            }
            case Panel::Speed: {
                text(102, 80, "SIMULATION SPEED", cream, 3);
                const char* labels[] = {"1X - RELAXED", "2X - BRISK", "4X - FAST"};
                for (int i = 0; i < 3; ++i) {
                    if (i == view.menu_index) rect(96, 140 + i * 54, 528, 36, {63, 89, 73, 255});
                    text(112, 151 + i * 54, labels[i], cream);
                }
                text(102, 354, "A CONFIRM   B BACK", muted);
                break;
            }
            case Panel::Controls:
                text(102, 80, "HANDHELD CONTROLS", cream, 3);
                text(102, 124, "D-PAD: CAMERA / MENU\nA: TILE / PLAN / CONFIRM\nB: BACK / CANCEL\nX: INSPECT   Y: VILLAGE\nL1/R1: CYCLE VILLAGERS\nL2/R2: ZOOM OUT / IN\nSTART: MENU / PAUSE\nSELECT: SPEED", cream);
                text(102, 286, "FN + START: SAVE AND EXIT", cream);
                text(102, 314, "Remap in config/controller.cfg", muted);
                text(102, 354, "B BACK", muted);
                break;
            case Panel::Map: break;
        }
    }
}
void Renderer::screenshot(const std::string& filename) {
    std::vector<Uint8> pixels(720 * 480 * 4);
    if (sdl.RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, pixels.data(), 720 * 4) != 0)
        throw std::runtime_error(sdl.GetError());
    std::ofstream out(filename, std::ios::binary);
    out << "P6\n720 480\n255\n";
    for (std::size_t i = 0; i < pixels.size(); i += 4) out.write(reinterpret_cast<const char*>(pixels.data() + i), 3);
    if (!out) throw std::runtime_error("Screenshot write failed");
}
} // namespace village
