#include "living_village/render.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace village {
namespace {
constexpr SDL_Color ink{25, 38, 39, 255}, cream{244, 235, 205, 255}, muted{162, 187, 167, 255};
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
    const int left = static_cast<int>(view.camera_x - 360.0 / size) - 1;
    const int top = static_cast<int>(view.camera_y - 240.0 / size) - 1;
    const auto px = [&](double x) { return static_cast<int>(360 + (x - view.camera_x) * size); };
    const auto py = [&](double y) { return static_cast<int>(240 + (y - view.camera_y) * size); };
    for (int y = std::max(0, top); y < std::min(64, top + 480 / size + 4); ++y) {
        for (int x = std::max(0, left); x < std::min(64, left + 720 / size + 4); ++x) {
            const auto& tile = world.tile(x, y);
            const int sx = px(x), sy = py(y), unit = std::max(1, size / 8);
            const Uint8 shade = static_cast<Uint8>(108 + ((x * 17 + y * 31 + world.seed) % 3) * 5);
            rect(sx, sy, size + 1, size + 1, {70, shade, 65, 255});
            rect(sx + unit, sy + size - unit * 2, unit, unit, {91, 138, 77, 255});
            switch (tile.terrain) {
                case Terrain::Tree:
                    rect(sx + size / 2 - unit, sy + size / 2, unit * 2, size / 2, {98, 70, 43, 255});
                    rect(sx + unit, sy + unit * 2, size - unit * 2, size / 2, {32, 76, 47, 255});
                    rect(sx + unit * 2, sy + unit, size - unit * 4, size / 2, {42, 93, 52, 255});
                    rect(sx + unit * 3, sy, size - unit * 6, unit * 2, {66, 114, 59, 255});
                    break;
                case Terrain::Stone:
                    rect(sx + size / 4, sy + size / 3, size / 2, size / 2, {106, 116, 108, 255});
                    rect(sx + size / 3, sy + size / 4, size / 3, size / 4, {169, 177, 156, 255});
                    break;
                case Terrain::Water:
                    rect(sx, sy, size + 1, size + 1, {48, 102, 129, 255});
                    rect(sx + size / 4, sy + size / 2, size / 2, unit, {75, 132, 150, 255});
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
        }
    }
    // Camp is real simulation state: food access and efficient rest happen here.
    const int camp_x = px(32), camp_y = py(32);
    rect(camp_x - size, camp_y - size, size, size, {127, 99, 65, 255});
    rect(camp_x - size + 3, camp_y - size + 3, size - 6, size - 6, {203, 174, 116, 255});
    rect(camp_x - size / 2 - 3, camp_y - size / 2, 6, size / 2, {77, 65, 46, 255});
    rect(camp_x + size, camp_y, size / 2, size / 2, {132, 85, 45, 255});
    outline(camp_x + size, camp_y, size / 2, size / 2, {210, 162, 97, 255});
    rect(camp_x + size / 3, camp_y + size / 3, size / 3, size / 3, {197, 93, 45, 255});
    rect(camp_x + size / 2 - 2, camp_y + size / 4, 4, size / 3, {255, 187, 78, 255});
    const SDL_Color shirts[] = {{184, 95, 79, 255}, {84, 145, 193, 255}, {177, 136, 188, 255},
                                {217, 169, 82, 255}, {130, 166, 99, 255}, {126, 177, 174, 255}};
    for (const auto& person : world.villagers) {
        const int x = px(person.x + .5), y = py(person.y + .5);
        if (x < -size || x > 720 + size || y < -size || y > 480 + size) continue;
        const int unit = std::max(2, size / 9);
        rect(x - unit * 2, y + unit * 2, unit * 4, unit, {48, 71, 45, 255});
        if (!person.alive()) { rect(x - unit * 2, y, unit * 4, unit, {129, 119, 106, 255}); continue; }
        const int walk = person.path.empty() ? 0 : static_cast<int>((world.ticks / 3) % 2);
        rect(x - unit, y + unit, unit, unit * (1 + walk), {55, 55, 58, 255});
        rect(x + 1, y + unit, unit, unit * (2 - walk), {55, 55, 58, 255});
        rect(x - unit, y - unit, unit * 2, unit * 2, shirts[person.id % 6]);
        rect(x - unit, y - unit * 3, unit * 2, unit * 2, {237, 198, 141, 255});
        rect(x - unit, y - unit * 3, unit * 2, unit / 2 + 1, {89, 66, 48, 255});
        if (person.id == view.selected) outline(x - unit * 2 - 2, y - unit * 3 - 2, unit * 4 + 4, unit * 6 + 4, {246, 223, 130, 255});
    }
    const Uint8 darkness = static_cast<Uint8>((1 - world.daylight()) * 65);
    rect(0, 0, 720, 480, {12, 24, 45, darkness});
    outline(351, 231, 18, 18, {231, 225, 187, 170});
    rect(0, 0, 720, 38, ink);
    text(12, 12, "LIVING VILLAGE", cream);
    text(210, 12, "DAY " + std::to_string(world.day()), muted);
    text(330, 12, "FOOD " + std::to_string(world.food_stock), muted);
    text(492, 12, std::to_string(view.speed) + "X", cream);
    text(555, 12, view.panel == Panel::Map ? "RUNNING" : "PAUSED", cream);
    rect(0, 426, 720, 54, ink);
    text(12, 433, "X INSPECT  Y VILLAGE  START MENU  SELECT SPEED", muted);
    text(12, 455, view.status, cream, 2, 690);
    if (view.panel != Panel::Map) {
        rect(0, 38, 720, 388, {8, 19, 21, 155});
        rect(72, 56, 576, 356, {26, 43, 43, 248});
        outline(72, 56, 576, 356, {127, 153, 123, 255});
        switch (view.panel) {
            case Panel::Menu: {
                text(102, 80, "VILLAGE MENU", cream, 3);
                const char* entries[] = {"Resume", "Save village", "Load village", "Controls", "Save and exit"};
                for (int i = 0; i < 5; ++i) {
                    if (view.menu_index == i) rect(96, 128 + i * 40, 528, 32, {63, 89, 73, 255});
                    text(112, 138 + i * 40, std::string(view.menu_index == i ? "> " : "  ") + entries[i], cream);
                }
                break;
            }
            case Panel::Inspector: {
                const auto& p = world.villagers.at(view.selected);
                text(102, 80, p.name + " - AGE " + std::to_string(p.age), cream, 3);
                text(102, 122, p.alive() ? goal_name(p.goal) : "Died from hunger", muted);
                meter(102, 158, "Hunger", p.hunger, {196, 116, 68, 255});
                meter(102, 186, "Energy", p.energy, {126, 166, 99, 255});
                meter(102, 214, "Health", p.health, {170, 137, 139, 255});
                text(102, 250, "Carried food " + std::to_string(p.food) + " | Known bushes " + std::to_string(p.known_food.size()), muted);
                text(102, 280, "WHY: " + p.reason, cream, 2, 504);
                text(102, 354, "L1/R1 VILLAGERS   B BACK", muted);
                break;
            }
            case Panel::Overview:
                text(102, 80, "VILLAGE LIFE", cream, 3);
                text(102, 124, "Camp food: " + std::to_string(world.food_stock) + " | Day " + std::to_string(world.day()), muted);
                for (std::size_t i = world.events.size() > 5 ? world.events.size() - 5 : 0, line = 0; i < world.events.size(); ++i, ++line)
                    text(102, 168 + static_cast<int>(line) * 36, world.events[i], cream, 2, 504);
                break;
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
                text(102, 124, "D-PAD: CAMERA / MENU\nA: SELECT / CONFIRM\nB: BACK / CANCEL\nX: INSPECT   Y: VILLAGE\nL1/R1: CYCLE VILLAGERS\nL2/R2: ZOOM OUT / IN\nSTART: MENU / PAUSE\nSELECT: SPEED", cream);
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
