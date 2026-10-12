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
            auto ground = meadow[world.season()];
            if (world.biome == Biome::Arid) ground = {171, static_cast<Uint8>(142 + shade % 12), 87, 255};
            else if (world.biome == Biome::Woodland) { ground.r = static_cast<Uint8>(ground.r * .85); ground.g = static_cast<Uint8>(ground.g * .9); }
            else if (world.biome == Biome::Meadow) ground.g = static_cast<Uint8>(std::min(190, ground.g + 12));
            if (world.biome == Biome::Taiga) ground = {79, 116, 111, 255};
            if (world.biome == Biome::Rainforest) ground = {48, 112, 75, 255};
            if (world.biome == Biome::Wetland) ground = {88, 121, 91, 255};
            if (world.biome == Biome::Savanna) ground = {157, 148, 78, 255};
            rect(sx, sy, size + 1, size + 1, ground);
            if (tile.terrain != Terrain::Water) {
                const int tuft = static_cast<int>((x * 41U + y * 23U + world.seed) % (size - 3));
                rect(sx + tuft, sy + size / 3, 1, 2, {176, 184, 103, 70});
                rect(sx + size / 2, sy + size - 3, 2, 1, {32, 66, 43, 55});
                if (world.biome == Biome::Meadow && (x * 7 + y * 13) % 11 == 0)
                    rect(sx + tuft, sy + size / 2, 2, 2, {231, 211, 147, 210});
            }
            if (tile.compaction > .15) rect(sx + size / 3, sy, size / 3, size, {132, 114, 78, static_cast<Uint8>(tile.compaction * 150)});
            if (tile.biomass > .25) rect(sx + unit, sy + size - unit * 2, unit, unit, {91, 138, 77, 255});
            switch (tile.terrain) {
                case Terrain::Tree:
                    rect(sx + unit * 2, sy + size - unit * 2, size - unit * 2, unit * 2, {15, 38, 29, 85});
                    rect(sx + size / 2 - unit, sy + size / 2, unit * 2, size / 2, {98, 70, 43, 255});
                    if (tile.wood == 0) break;
                    if (tile.plant_age < 20) {
                        rect(sx + size / 3, sy + size / 3, size / 3, size / 3, {68, 115, 58, 255}); break;
                    }
                    if (world.biome == Biome::Taiga) {
                        for (int level = 0; level < 3; ++level) {
                            const int width = unit * (3 + level * 2);
                            rect(sx + (size - width) / 2, sy + unit * (1 + level * 2), width, unit * 2, {35, static_cast<Uint8>(88 - level * 9), 67, 255});
                        }
                        break;
                    }
                    rect(sx + unit, sy + unit * 2, size - unit * 2, size / 2, {32, 76, 47, 255});
                    rect(sx + unit * 2, sy + unit, size - unit * 4, size / 2, {42, 93, 52, 255});
                    rect(sx + unit * 3, sy, size - unit * 6, unit * 2, {66, 114, 59, 255});
                    rect(sx + unit * 2, sy + unit * 2, unit * 2, unit, {104, 143, 69, 160});
                    rect(sx + size - unit * 3, sy + unit * 4, unit, unit * 2, {22, 58, 41, 180});
                    break;
                case Terrain::Stone:
                    if (tile.stone == 0) { rect(sx + size / 3, sy + size * 2 / 3, size / 3, unit, {106, 116, 108, 255}); break; }
                    rect(sx + size / 4, sy + size / 3, size / 2, size / 2, {106, 116, 108, 255});
                    rect(sx + size / 3, sy + size / 4, size / 3, size / 4, {169, 177, 156, 255});
                    break;
                case Terrain::Water:
                    rect(sx, sy, size + 1, size + 1, {48, 102, 129, 255});
                    rect(sx + size / 4, sy + size / 3 + static_cast<int>((world.ticks / 8 + x + y) % 3) * unit, size / 2, unit, {75, 132, 150, 255});
                    for (Point d : {Point{-1,0}, Point{1,0}, Point{0,-1}, Point{0,1}}) {
                        const int nx = x + d.x, ny = y + d.y;
                        if (nx >= 0 && ny >= 0 && nx < map_size && ny < map_size && world.tile(nx,ny).terrain != Terrain::Water) {
                            if (d.x) rect(sx + (d.x > 0 ? size - unit : 0), sy, unit, size, {137, 160, 117, 255});
                            else rect(sx, sy + (d.y > 0 ? size - unit : 0), size, unit, {137, 160, 117, 255});
                        }
                    }
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
            if (tile.surface_water > .15 && tile.terrain != Terrain::Water) rect(sx, sy + size / 2, size, size / 2, {76, 124, 153, static_cast<Uint8>(std::min(180.0, tile.surface_water * 240))});
            if (tile.snowpack > .02 && tile.terrain != Terrain::Water) rect(sx, sy + size / 2, size, size / 2, {216, 227, 222, static_cast<Uint8>(std::min(180.0, tile.snowpack * 600))});
            if (tile.fire > 0) {
                rect(sx + size / 4, sy + size / 4, size / 2, size * 3 / 4, {225, 103, 41, 210});
                rect(sx + size / 3, sy + static_cast<int>(world.ticks / 5 % 3) * unit, size / 3, size / 2, {255, 196, 79, 230});
            }
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
        rect(x + size / 5, y + size - u, size - u, u * 2, {10, 29, 25, 95});
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
                for (int roof = u * 2; roof < size / 2; roof += u * 2)
                    rect(x + size / 2 - roof, y + roof, roof * 2, 1, {161, 110, 67, 255});
                rect(x + size / 2 - u, y + size * 2 / 3, u * 2, size / 3, {65, 61, 46, 255});
                rect(x + u * 2, y + size / 2, u, u, {175, 212, 213, 255});
                rect(x + size - u * 3, y + size / 2, u, u, {175, 212, 213, 255});
                rect(x + size - u * 3, y + u, u, u * 2, {107, 105, 97, 255});
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
        if (b.condition < 85) {
            rect(x + 2, y + size - 3, size - 4, 2, {68, 47, 36, 255});
            rect(x + 2, y + size - 3, static_cast<int>((size - 4) * b.condition / 100), 2, {219, 152, 79, 255});
        }
    }
    for (const auto& animal : world.animals) {
        const int x = px(animal.x + .5), y = py(animal.y + .5), u = std::max(1, size / 10);
        if (x < -size || x > 720 || y < -size || y > 480) continue;
        if (animal.species == Species::Bee) {
            const int buzz = static_cast<int>((world.ticks / 4 + animal.id) % 3) - 1;
            rect(x - u, y + buzz * u, u * 2, u, {236, 191, 69, 255});
            rect(x, y - u + buzz * u, u, u, {222, 234, 211, 255});
        } else {
            const bool deer = animal.species == Species::Deer;
            const SDL_Color coat = deer ? SDL_Color{171, 133, 85, 255} : animal.species == Species::Fox ? SDL_Color{205, 118, 56, 255} : SDL_Color{188, 178, 153, 255};
            rect(x - u * 2, y, u * 4, u * (deer ? 2 : 1), coat);
            rect(x + u, y - u, u * 2, u * 2, coat);
            rect(x + u * 2, y - u * (deer ? 3 : 2), u, u * 2, coat);
            const int stride = static_cast<int>((world.ticks / 5 + animal.id) % 2);
            rect(x - u + stride, y + u, u, u * 2, {77, 70, 56, 255});
            rect(x + u - stride, y + u, u, u * 2, {77, 70, 56, 255});
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
        rect(x - unit * 2, y + unit * 2, unit * 4, unit, {15, 29, 29, 100});
        rect(x - unit, y + unit, unit, unit * (1 + walk), {55, 55, 58, 255});
        rect(x + 1, y + unit, unit, unit * (2 - walk), {55, 55, 58, 255});
        rect(x - unit, y - unit, unit * 2, unit * 2, shirts[person.id % 6]);
        rect(x - unit, y - unit * 3, unit * 2, unit * 2, {237, 198, 141, 255});
        rect(x - unit, y - unit * 3, unit * 2, unit / 2 + 1, {89, 66, 48, 255});
        if (size >= 28) { rect(x, y - unit * 2, 1, 1, {62, 52, 41, 255}); rect(x - unit, y, 1, unit, {239, 221, 167, 170}); }
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
    if (view.panel == Panel::Map) {
        rect(648, 342, 62, 74, {20, 36, 35, 210});
        for (int y = 0; y < 16; ++y) for (int x = 0; x < 16; ++x) {
            const SDL_Color colors[] = {{112,144,89,255},{48,89,64,255},{148,145,133,255},{58,123,153,255},{167,163,91,255}};
            rect(654 + x * 3, 348 + y * 3, 3, 3, colors[static_cast<int>(world.tile(x * 4,y * 4).terrain)]);
        }
        rect(654 + static_cast<int>(view.camera_x / 4) * 3, 348 + static_cast<int>(view.camera_y / 4) * 3, 3, 3, cream);
        text(654, 400, "WORLD", muted, 1);
    }
    outline(351, 231, 18, 18, {231, 225, 187, 170});
    rect(0, 0, 720, 54, ink);
    text(12, 8, "LIVING VILLAGE", cream);
    text(210, 8, "DAY " + std::to_string(world.day()), muted);
    text(330, 8, std::to_string(world.alive_count()) + " PEOPLE", muted);
    text(492, 8, std::to_string(view.speed) + "X", cream);
    text(555, 8, view.simulation_running() ? "RUNNING" : "PAUSED", cream);
    text(12, 32, "FOOD " + std::to_string(world.food_stock) + "/" + std::to_string(world.food_capacity()) +
         "  WOOD " + std::to_string(world.wood_stock) + "  STONE " + std::to_string(world.stone_stock) + "  " + world.season_name() + " " + world.weather_name(), muted);
    rect(0, 426, 720, 54, ink);
    text(12, 433, "A TILE/PLAN  X PERSON  Y VILLAGE  FN+START EXIT", muted);
    text(12, 455, view.status, cream, 2, 690);
    if (view.panel != Panel::Map) {
        rect(0, 54, 720, 372, {8, 19, 21, 155});
        rect(78, 62, 576, 356, {0, 0, 0, 70});
        rect(72, 56, 576, 356, {26, 43, 43, 248});
        rect(74, 58, 572, 48, {33, 54, 52, 255});
        outline(72, 56, 576, 356, {127, 153, 123, 255});
        switch (view.panel) {
            case Panel::Menu: {
                text(102, 80, "VILLAGE MENU", cream, 3);
                const char* entries[] = {"Resume", "Save village", "Load village", "Controls", "Save and exit", "New village", "Manage saved worlds"};
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
                const char* titles[] = {"VILLAGE LIFE", "POPULATION", "VILLAGE HISTORY", "NEURAL LEARNING", "ECONOMY / HISTORY", "SOUND / VIBRATION", "ECOLOGY / WEATHER", "WILDLIFE", "AGENT DIAGNOSTICS"};
                text(102, 80, titles[std::clamp(view.overview_page, 0, 8)], cream, 3);
                if (view.overview_page == 0) {
                    int finished = 0; for (const auto& b : world.buildings) if (b.complete) ++finished;
                    text(102, 122, world.world_name, cream);
                    text(102, 156, "Built " + std::to_string(finished) + " | Plans " + std::to_string(world.buildings.size() - finished), muted);
                    text(102, 190, "Residents " + std::to_string(world.alive_count()) + " | Beds " + std::to_string(world.shelter_capacity()), cream);
                    text(102, 224, "A: AUTO PLANS " + std::string(world.auto_develop ? "ON" : "OFF"), cream);
                    text(102, 268, "Seed " + std::to_string(world.seed), muted);
                    text(102, 302, std::string(biome_name(world.biome)) + " / " + geography_name(world.geography), muted);
                } else if (view.overview_page == 1) {
                    text(102, 126, "Adults " + std::to_string(world.alive_count() - world.child_count()) + " | Children " + std::to_string(world.child_count()), cream);
                    text(102, 164, "Births " + std::to_string(world.births) + " | Arrivals " + std::to_string(world.arrivals), cream);
                    text(102, 202, "Deaths " + std::to_string(world.deaths) + " | Year " + std::to_string(1 + (world.day() - 1) / days_per_year), muted);
                    text(102, 248, "A: NEW BIRTHS/ARRIVALS " + std::string(world.population_growth ? "ON" : "OFF"), cream);
                    text(102, 294, world.growth_status(), muted, 2, 504);
                } else if (view.overview_page == 3) {
                    std::uint64_t predictions = 0, updates = 0, changed = 0, retained = 0, episodes = 0, replay = 0;
                    int models = 0, trained = 0; double error = 0;
                    auto account = [&](const Brain& b) {
                        ++models; predictions += b.session_predictions; updates += b.session_updates;
                        changed += b.session_changed_updates; retained += b.updates; episodes += b.episodes; replay += b.replay_updates;
                        if (b.episodes) { ++trained; error += std::abs(b.last_error); }
                    };
                    for (const auto& p : world.villagers) if (p.alive()) account(p.brain);
                    for (const auto& a : world.animals) account(a.brain);
                    text(102, 118, "LIVE | " + std::to_string(models) + " resident / wildlife MLPs", cream);
                    text(102, 148, "Value predictions this session: " + std::to_string(predictions), muted);
                    text(102, 178, "Training this session: " + std::to_string(updates) + " steps", cream);
                    text(102, 208, "Steps changing weights: " + std::to_string(changed), muted);
                    text(102, 238, "Retained: " + std::to_string(episodes) + " outcomes / " + std::to_string(replay) + " replay", cream);
                    text(102, 268, "Last-outcome mean abs error: " + (trained ? decimal(error / trained) : std::string("NO OUTCOMES YET")), muted);
                    text(102, 298, "A: LEARNING " + std::string(world.learning_enabled ? "ON" : "FROZEN") + " | Saved SGD " + std::to_string(retained), cream);
                    text(102, 328, "Counts since load. Error is value fit, not accuracy.", muted, 1, 504);
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
                } else if (view.overview_page == 6) {
                    double moisture = 0, fertility = 0; int forest = 0, flooded = 0, burning = 0, land = 0;
                    for (const auto& t : world.tiles) if (t.terrain != Terrain::Water) {
                        ++land; moisture += t.moisture; fertility += t.fertility;
                        forest += t.terrain == Terrain::Tree; flooded += t.surface_water > .35; burning += t.fire > .05;
                    }
                    text(102, 122, std::string(world.weather_name()) + " | " + decimal(world.temperature) + " C | Wind " + decimal(world.wind), cream);
                    text(102, 154, "Dry days " + std::to_string(world.dry_days) + " | Flooded " + std::to_string(flooded), muted);
                    text(102, 186, "Soil water " + decimal(moisture / std::max(1, land) * 100) + "% | Nutrients " + decimal(fertility / std::max(1, land) * 100) + "%", cream);
                    text(102, 218, "Trees " + std::to_string(forest) + " | Seedlings born " + std::to_string(world.ecology.trees_born), muted);
                    text(102, 250, "Burning " + std::to_string(burning) + " | Plants lost " + std::to_string(world.ecology.plants_died), cream);
                    text(102, 282, "Wildlife " + std::to_string(world.animals.size()) + " | Predations " + std::to_string(world.ecology.predations), muted);
                    text(102, 314, "Building repairs " + std::to_string(world.ecology.repairs), cream);
                } else if (view.overview_page == 8) {
                    const auto& p = world.villagers[selected_index]; const auto& b = p.brain;
                    text(102, 118, p.name + " | " + (p.alive() ? std::string("LIVE") : "DECEASED"), cream);
                    text(102, 148, "Predictions since load: " + std::to_string(b.session_predictions), muted);
                    text(102, 178, "SGD / weight-changing steps: " + std::to_string(b.session_updates) + "/" + std::to_string(b.session_changed_updates), cream);
                    text(102, 208, "Last step weight delta: " + decimal(b.session_weight_change * 1000) + " x .001", muted);
                    text(102, 238, "Goal value bonus: " + decimal(b.selected_adjustment) + " | Reward: " + decimal(b.last_reward), cream);
                    text(102, 268, "Saved outcomes: " + std::to_string(b.episodes) + " | Replay: " + std::to_string(b.replay_size) + "/32", muted);
                    text(102, 298, !world.learning_enabled ? "TRAINING FROZEN | PREDICTIONS CONTINUE" : b.active ? "PENDING ACTION OUTCOME" : "WAITING FOR NEXT ACTION", cream);
                    text(102, 332, "L1/R1 RESIDENTS | A FREEZE/RESUME TRAINING", muted, 1);
                } else if (view.overview_page == 7) {
                    if (world.animals.empty()) text(102, 146, "No wildlife remains in this habitat", muted);
                    else {
                        const auto& a = world.animals[std::clamp(view.selected_animal, 0, static_cast<int>(world.animals.size()) - 1)];
                        text(102, 120, std::string(species_name(a.species)) + " #" + std::to_string(a.id) + (a.female ? " F" : " M") + " | Age " + decimal(a.age) + " days", cream);
                        text(102, 148, std::string(animal_action_name(a.action)) + " | " + std::to_string(static_cast<int>(a.x)) + "/" + std::to_string(static_cast<int>(a.y)), muted);
                        text(102, 178, "Hunger " + decimal(a.hunger) + " Thirst " + decimal(a.thirst) + " Health " + decimal(a.health), cream);
                        text(102, 208, "Energy " + decimal(a.energy) + " Body " + decimal(a.body_temperature) + " C", muted);
                        text(102, 238, "Disease " + decimal(a.disease * 100) + "% | Pregnancy " + decimal(a.gestation) + " days", cream);
                        text(102, 268, "MLP episodes " + std::to_string(a.brain.episodes) + " | Reward " + decimal(a.brain.last_reward), muted);
                        text(102, 298, a.reason, cream, 2, 504);
                        text(102, 336, "L1/R1 | Session predictions " + std::to_string(a.brain.session_predictions) + " SGD " + std::to_string(a.brain.session_updates), muted, 1);
                    }
                } else {
                    for (std::size_t i = world.events.size() > 4 ? world.events.size() - 4 : 0, line = 0; i < world.events.size(); ++i, ++line)
                        text(102, 126 + static_cast<int>(line) * 54, world.events[i], cream, 2, 504);
                }
                text(102, 366, "LEFT/RIGHT PAGES   B BACK", muted); break;
            }
            case Panel::NewVillage: {
                text(102, 80, "CREATE WORLD", cream, 3);
                const char* supplies[] = {"Lean (6)", "Standard (18)", "Generous (30)"};
                const std::string rows[] = {"Name: " + view.draft_name.substr(0, 18), "Seed: " + std::to_string(view.draft_seed), std::string("Biome: ") + biome_name(view.draft_biome), std::string("Layout: ") + geography_name(view.draft_geography), std::string("Food: ") + supplies[view.draft_supplies], "Create this world", "Browse saved worlds"};
                for (int i = 0; i < 7; ++i) {
                    if (view.menu_index == i) { rect(96, 118 + i * 29, 330, 26, {63, 89, 73, 255}); rect(96, 118 + i * 29, 3, 26, {220, 195, 125, 255}); }
                    text(106, 125 + i * 29, rows[i], cream, 2, 320);
                }
                if (!preview || preview->seed != view.draft_seed || preview->biome != view.draft_biome || preview->geography != view.draft_geography)
                    preview = std::make_unique<World>(view.draft_seed, view.draft_biome, view.draft_geography);
                rect(442, 122, 132, 132, ink);
                for (int y = 0; y < map_size; ++y) for (int x = 0; x < map_size; ++x) {
                    const SDL_Color colors[] = {{107,140,83,255},{43,89,62,255},{160,150,131,255},{64,139,166,255},{161,151,82,255}};
                    rect(444 + x * 2, 124 + y * 2, 2, 2, colors[static_cast<int>(preview->tile(x,y).terrain)]);
                }
                outline(502, 182, 12, 12, cream);
                text(442, 267, "SEED PREVIEW", muted, 1);
                text(442, 290, std::to_string(static_cast<int>(view.draft_biome) + 1) + "/8 BIOMES", cream, 1);
                text(442, 306, std::to_string(static_cast<int>(view.draft_geography) + 1) + "/12 LAYOUTS", cream, 1);
                const char* descriptions[] = {"Balanced woods and mild seasons", "Dense forest with frequent rain", "Open grass and plentiful berries", "Sparse trees, dry soils and heat", "Cool conifer forest and snowy winters", "Lush growth, warmth and heavy rainfall", "Saturated soils and frequent rainfall", "Open woodland with warm, dry spells"};
                text(102, 337, descriptions[static_cast<int>(view.draft_biome)], muted, 1, 504);
                const char* layouts[] = {"Seven seeded pond clusters", "Winding river with two land crossings", "A central island and reachable shores", "An eastern coastline with inland space", "Mineral ridges and broad hills", "Two broad lakes beside the clearing", "Land narrows between two coastal shores", "Main island and remote wildlife islets", "Branching waterways with land crossings", "Stone canyon walls and a narrow river", "Sloping valley beside a river", "Continuous land; rainfall supplies water"};
                text(102, 350, layouts[static_cast<int>(view.draft_geography)], muted, 1, 504);
                text(102, 366, "A SELECT  LEFT/RIGHT CHANGE  X SEED", muted); break;
            }
            case Panel::SeedEdit: case Panel::NameEdit: {
                const bool seed = view.panel == Panel::SeedEdit;
                text(102, 80, seed ? "EDIT WORLD SEED" : "EDIT WORLD NAME", cream, 3);
                text(102, 126, "LEFT/RIGHT MOVE  UP/DOWN CHANGE", muted);
                for (std::size_t i = 0; i < view.edit_buffer.size(); ++i) {
                    if (static_cast<int>(i) == view.edit_cursor) rect(100 + static_cast<int>(i) * 18, 164, 18, 28, {63, 89, 73, 255});
                    text(102 + static_cast<int>(i) * 18, 170, std::string(1, view.edit_buffer[i]), cream, 3);
                }
                text(102, 234, seed ? "Range: 0 to 4294967295" : "Letters, numbers, spaces and hyphens", muted);
                text(102, 282, seed ? "X: RANDOM SEED" : "X: CLEAR SELECTED CHARACTER", muted);
                text(102, 366, "A ACCEPT  B CANCEL", muted); break;
            }
            case Panel::ConfirmCreate:
                text(102, 80, "CREATE THIS WORLD", cream, 3);
                text(102, 136, view.draft_name, cream);
                text(102, 172, "Seed " + std::to_string(view.draft_seed), muted);
                text(102, 208, std::string(biome_name(view.draft_biome)) + " / " + geography_name(view.draft_geography), cream);
                text(102, 260, "Your current world is saved separately. You can switch back from Manage saved worlds.", muted, 2, 504);
                text(102, 366, "A CREATE  B CANCEL", cream); break;
            case Panel::Saves: {
                text(102, 80, "SAVED WORLDS", cream, 3);
                text(442, 84, std::to_string(view.saves.size()) + " SAVES", muted);
                if (view.saves.empty()) { text(102, 152, "No saved worlds", cream); text(102, 208, "A: CREATE A WORLD", muted); }
                const int first = (view.menu_index / 4) * 4;
                for (int row = 0; row < 4 && first + row < static_cast<int>(view.saves.size()); ++row) {
                    const int index = first + row; const auto& slot = view.saves[index];
                    if (view.menu_index == index) rect(96, 120 + row * 48, 528, 44, {63, 89, 73, 255});
                    text(112, 126 + row * 48, (slot.id == view.active_slot && view.has_world ? "OPEN: " : "") + slot.name, cream);
                    text(112, 146 + row * 48, slot.metadata ? "Day " + std::to_string(slot.day) + " | Seed " + std::to_string(slot.seed) : "Header unreadable - load checks backup", muted);
                }
                if (!view.saves.empty()) {
                    const auto& slot = view.saves[view.menu_index];
                    text(102, 320, slot.metadata ? std::string(biome_name(slot.biome)) + " / " + geography_name(slot.geography) + (slot.backup_only ? " | BACKUP ONLY" : "") : "Unknown settings - load checks the backup", muted);
                }
                text(102, 344, "A LOAD  Y NAME  X DELETE", muted);
                text(102, 376, "L1 SNAPSHOT  B BACK", muted); break;
            }
            case Panel::ConfirmDelete:
                text(102, 80, "DELETE SAVED WORLD", cream, 3);
                text(102, 146, view.pending_name, cream);
                text(102, 184, "Save ID: " + view.pending_slot, muted);
                text(102, 230, "Deletes this world's save and backup. Exports are retained. This cannot be undone in the game.", muted, 2, 504);
                text(102, 340, "A DELETE  B CANCEL", cream); break;
            case Panel::Tile: {
                const auto at = view.selected_tile;
                const auto& cell = world.tile(at.x, at.y);
                const char* terrain[] = {"Grass", "Tree", "Stone", "Water", "Berry bush"};
                text(102, 80, "TILE " + std::to_string(at.x) + "/" + std::to_string(at.y), cream, 3);
                text(102, 122, std::string(terrain[static_cast<int>(cell.terrain)]) + " | Elevation " + decimal(cell.elevation), muted);
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
                    else if (b.kind == BuildingKind::Workshop) {
                        text(102, 202, "Technology " + std::to_string(world.technology_level) + "/2 | Knowledge " + decimal(world.knowledge), cream);
                        if (world.technology_level < 2) {
                            const auto upgrade = technology_cost(world.technology_level);
                            text(102, 244, "Supplies W " + std::to_string(b.upgrade_wood) + "/" + std::to_string(upgrade.wood) + " S " + std::to_string(b.upgrade_stone) + "/" + std::to_string(upgrade.stone) + " Work " + decimal(b.upgrade_work) + "/" + std::to_string(static_cast<int>(upgrade.work)), muted);
                        } else text(102, 244, "All technologies researched", muted);
                    }
                    else text(102, 202, b.fuel > 0 ? "Lit - warmth and social comfort" : "No fuel - needs shared wood", cream);
                    if (b.complete) {
                        text(102, 282, "Condition " + decimal(b.condition) + "% | Repair W " + std::to_string(b.repair_wood) + " S " + std::to_string(b.repair_stone), cream);
                        text(102, 314, "Repair work " + decimal(b.repair_work) + "/8", muted);
                    }
                } else {
                    text(102, 172, "Food " + std::to_string(cell.food) + " | Wood " + std::to_string(cell.wood) + " | Stone " + std::to_string(cell.stone), cream);
                    if (cell.loose_food + cell.loose_wood + cell.loose_stone > 0) text(102, 198, "Loose F " + std::to_string(cell.loose_food) + " W " + std::to_string(cell.loose_wood) + " S " + std::to_string(cell.loose_stone), muted);
                    const bool resource = cell.terrain == Terrain::Tree || cell.terrain == Terrain::Stone || cell.terrain == Terrain::Bush;
                    const bool designated = std::find(world.gather_orders.begin(), world.gather_orders.end(), at) != world.gather_orders.end();
                    text(102, 232, world.can_plan(at) ? "A: PLACE A CONSTRUCTION PLAN" : resource ? (designated ? "A: CANCEL GATHERING AREA" : "A: DESIGNATE GATHERING AREA") : "Plans need reachable empty grass", muted, 2, 504);
                    text(102, 278, "Water " + decimal(cell.moisture * 100) + "% | Nutrients " + decimal(cell.fertility * 100) + "%", muted);
                    text(102, 302, "Health " + decimal(cell.plant_health) + " Age " + std::to_string(static_cast<int>(cell.plant_age)) + "d Disease " + decimal(cell.disease * 100) + "%", muted);
                    text(102, 326, "Flood " + decimal(cell.surface_water) + " Erosion " + decimal(cell.erosion * 100) + "% Wear " + decimal(cell.compaction * 100) + "%", muted);
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
