#include "living_village/world.hpp"
#include <algorithm>
#include <cmath>
namespace village {
const char* biome_name(Biome biome) {
    const char* names[] = {"Temperate", "Woodland", "Meadow", "Arid", "Taiga", "Rainforest", "Wetland", "Savanna"};
    const int value = static_cast<int>(biome); return value >= 0 && value < biome_count ? names[value] : "Unknown";
}
const char* geography_name(Geography geography) {
    const char* names[] = {"Ponds", "River", "Island", "Coast", "Highlands", "Lakes", "Peninsula", "Archipelago", "Delta", "Canyon", "Valley", "Plains"};
    const int value = static_cast<int>(geography); return value >= 0 && value < geography_count ? names[value] : "Unknown";
}
void World::generate_terrain() {
    const int tree[] = {140, 280, 65, 55, 240, 350, 120, 80}, rock[] = {50, 45, 35, 90, 75, 25, 25, 55}, bush[] = {65, 90, 95, 30, 50, 120, 100, 65};
    const int type = static_cast<int>(biome);
    for (int y = 0; y < map_size; ++y) for (int x = 0; x < map_size; ++x) {
        const int roll = random(1000); auto& cell = tile(x, y);
        cell.terrain = roll < tree[type] ? Terrain::Tree : roll < tree[type] + rock[type] ? Terrain::Stone : roll < tree[type] + rock[type] + bush[type] ? Terrain::Bush : Terrain::Grass;
        if (std::abs(x - camp.x) + std::abs(y - camp.y) < 7) cell.terrain = Terrain::Grass;
        if (cell.terrain == Terrain::Bush) cell.food = 6;
        if (cell.terrain == Terrain::Tree) cell.wood = 6;
        if (cell.terrain == Terrain::Stone) cell.stone = 8;
    }
    if (geography == Geography::Ponds) {
        // Preserve the original seeded terrain/random stream for default worlds.
        for (int pond = 0; pond < 7; ++pond) {
            const int px = 3 + random(58), py = 3 + random(58);
            for (int y = std::max(0, py - 4); y <= std::min(63, py + 4); ++y)
                for (int x = std::max(0, px - 5); x <= std::min(63, px + 5); ++x)
                    if ((x - px) * (x - px) / 2 + (y - py) * (y - py) < 13 && std::abs(x - camp.x) + std::abs(y - camp.y) > 9)
                        tile(x, y) = {Terrain::Water, 0, 0};
        }
    } else {
        const double phase = (seed % 1000) / 100.0;
        for (int y = 0; y < map_size; ++y) for (int x = 0; x < map_size; ++x) {
            if (std::abs(x - camp.x) + std::abs(y - camp.y) < 10) continue;
            bool water = false;
            if (geography == Geography::River) {
                const double channel = 18 + std::sin(y / 8.0 + phase) * 4;
                water = std::abs(x - channel) < 2.3 && y != 24 && y != 44; // Two land fords.
            } else if (geography == Geography::Island) {
                const double ripple = 1 + std::sin((x + y) / 5.0 + phase) * .06;
                water = (x - 32) * (x - 32) / (25.0 * 25) + (y - 32) * (y - 32) / (23.0 * 23) > ripple;
            } else if (geography == Geography::Coast) water = x > 48 + std::sin(y / 7.0 + phase) * 3;
            else if (geography == Geography::Highlands) {
                const double ridge = x - 32 - (y - 32) * .55;
                if (std::abs(ridge - 12) < 2.5 || std::abs(ridge + 13) < 2) tile(x, y) = {Terrain::Stone, 0, 0, 0, 8};
            }
            if (geography == Geography::Lakes) {
                water = std::pow((x - 15) / 10.0, 2) + std::pow((y - 18) / 14.0, 2) < 1
                    || std::pow((x - 48) / 9.0, 2) + std::pow((y - 44) / 12.0, 2) < 1;
            } else if (geography == Geography::Peninsula) {
                water = y > 12 && std::abs(x - 32 - std::sin(y / 10.0 + phase) * 3) > 19 - y * .18;
            } else if (geography == Geography::Archipelago) {
                water = std::pow((x - 32) / 18.0, 2) + std::pow((y - 32) / 19.0, 2) > 1
                    && std::pow((x - 9) / 7.0, 2) + std::pow((y - 12) / 8.0, 2) > 1
                    && std::pow((x - 54) / 7.0, 2) + std::pow((y - 48) / 9.0, 2) > 1;
            } else if (geography == Geography::Delta) {
                water = (std::abs(x - 13 - std::sin(y / 9.0 + phase) * 3) < 2
                    || (y > 30 && std::abs(x - 13 - (y - 30) * 1.3) < 2)) && y != 24 && y != 48;
            } else if (geography == Geography::Canyon || geography == Geography::Valley) {
                const double channel = 15 + std::sin(y / 10.0 + phase) * 3;
                water = std::abs(x - channel) < 1.8 && y != 24 && y != 44;
                if (geography == Geography::Canyon && std::abs(x - channel) > 3 && std::abs(x - channel) < 6)
                    tile(x, y) = {Terrain::Stone, 0, 0, 0, 8};
            }
            if (water) tile(x, y) = {Terrain::Water, 0, 0};
        }
    }
    for (Point p : {Point{25, 32}, Point{39, 32}, Point{32, 25}, Point{32, 39}}) tile(p.x, p.y) = {Terrain::Bush, 6, 0};
    tile(26, 30) = {Terrain::Tree, 0, 0, 6, 0}; tile(38, 30) = {Terrain::Tree, 0, 0, 6, 0};
    tile(35, 26) = {Terrain::Stone, 0, 0, 0, 8}; tile(29, 38) = {Terrain::Stone, 0, 0, 0, 8};
}
} // namespace village
