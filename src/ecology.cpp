#include "living_village/world.hpp"
#include "living_village/workers.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace village {
namespace {
constexpr std::array<Point, 4> directions{{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
bool inside(int x, int y) { return x >= 0 && y >= 0 && x < map_size && y < map_size; }
double bounded(double value) { return std::clamp(value, 0.0, 1.0); }
}
int World::eco_random(int maximum) { return static_cast<int>(ecology_rng() % static_cast<unsigned>(maximum)); }
const char* World::weather_name() const {
    const char* names[] = {"Clear", "Rain", "Storm", "Snow", "Heatwave"};
    return names[static_cast<int>(weather)];
}
void World::update_weather() {
    const double seasonal[] = {16, 27, 13, 3};
    const double warmth[] = {0, 0, 0, 9, -9, 7, 2, 6};
    const int rainfall[] = {30, 42, 30, 14, 26, 65, 55, 22};
    temperature = seasonal[season()] + warmth[static_cast<int>(biome)] + eco_random(9) - 4;
    const int rain_chance = rainfall[static_cast<int>(biome)] + (director.mode == DirectorMode::Adaptive ? director.bias : 0);
    const int roll = eco_random(100);
    weather = roll < rain_chance ? (temperature <= 2 ? Weather::Snow : roll < 5 ? Weather::Storm : Weather::Rain)
        : temperature >= 30 && roll > 85 ? Weather::Heatwave : Weather::Clear;
    precipitation = weather == Weather::Storm ? .006 : weather == Weather::Rain ? .002 : weather == Weather::Snow ? .0008 : 0;
    wind = weather == Weather::Storm ? .8 : .1 + eco_random(40) / 100.0;
    dry_days = precipitation > 0 ? 0 : std::min(1000, dry_days + 1);
    if (weather == Weather::Storm) event("Storm: runoff and exposed structures at risk");
    else if (dry_days == 6) event("Drought: dry soil slows growth and raises fire risk");
    else if (weather == Weather::Heatwave) event("Heatwave: wildlife needs water and shade");
}
void World::initialize_ecology(bool wildlife) {
    ecology_rng.seed(seed ^ 0xec0109U); animals.clear(); next_animal_id = 0; ecology = {}; dry_days = 0; flood_alert = false;
    update_weather();
    for (int y = 0; y < map_size; ++y) for (int x = 0; x < map_size; ++x) {
        auto& t = tile(x, y);
        const bool water = t.terrain == Terrain::Water;
        // Basin slope plus two broad hills; no elevation changes alter passability.
        t.elevation = water ? .05 : .3 + .15 * std::sin((x + seed % 17) / 12.0) * std::cos(y / 15.0)
            + (geography == Geography::Highlands ? std::abs(x - 32 - (y - 32) * .55) / 150.0 : 0);
        const double moisture[] = {.6, .6, .6, .32, .5, .78, .88, .42};
        t.moisture = water ? 1 : moisture[static_cast<int>(biome)];
        if (geography == Geography::Canyon) t.elevation += std::min(.45, std::abs(x - 15) / 80.0);
        if (geography == Geography::Valley) t.elevation += std::abs(x - 32) / 100.0;
        t.surface_water = water ? .7 : 0; t.groundwater = water ? .8 : .35;
        t.fertility = biome == Biome::Arid ? .45 : .75;
        t.organic = biome == Biome::Woodland ? .65 : .45;
        t.compaction = t.erosion = t.disease = t.fire = t.bush_seeds = 0;
        t.biomass = water || t.terrain == Terrain::Stone ? 0 : .7;
        t.plant_age = t.terrain == Terrain::Tree ? 40 + eco_random(120) : eco_random(20);
        t.plant_health = 100; t.seed_bank = t.terrain == Terrain::Tree || t.terrain == Terrain::Bush ? .3 : 0;
        t.pollination = .6;
        t.nectar = t.terrain == Terrain::Bush ? .4 : 0; t.snowpack = 0;
    }
    if (!wildlife) return;
    const int counts[] = {8, 4, 2, 4};
    for (int species = 0; species < 4; ++species) for (int n = 0; n < counts[species]; ++n)
        for (int attempt = 0; attempt < 64; ++attempt) {
            Point p{3 + eco_random(58), 3 + eco_random(58)};
            const bool nearby = species < 2 && n < 2 && attempt < 32;
            if (nearby) p = {camp.x + (n == 0 ? -1 : 1) * (7 + eco_random(3)), camp.y - 3 + eco_random(7)};
            if ((!nearby && std::abs(p.x - camp.x) + std::abs(p.y - camp.y) < 12) || !passable(p.x, p.y)) continue;
            if (spawn_animal(static_cast<Species>(species), p)) break;
        }
}
double World::plant_growth(const Tile& cell) const {
    const double water = std::clamp(cell.moisture * 2, .08, 1.25);
    const double warmth = std::clamp(1 - std::abs(temperature - 21) / 40, .2, 1.0);
    return water * warmth * (.5 + .5 * cell.fertility) * (cell.plant_health / 100) * (1 - .4 * cell.compaction);
}
void World::trample(Point p, double amount) {
    if (!inside(p.x, p.y) || tile(p.x, p.y).terrain == Terrain::Water) return;
    auto& cell = tile(p.x, p.y);
    cell.compaction = bounded(cell.compaction + .002 * amount);
    cell.biomass = std::max(0.0, cell.biomass - .001 * amount);
}
bool World::ignite(Point place) {
    if (!inside(place.x, place.y)) return false;
    auto& cell = tile(place.x, place.y);
    if (cell.terrain == Terrain::Water || cell.moisture > .4 || cell.biomass < .2 || cell.fire > 0) return false;
    cell.fire = .25; ++ecology.fires_started; event("Wildfire ignites in dry vegetation"); return true;
}
void World::update_ecology(double seconds) {
    if (!ecology_enabled || seconds <= 0 || seconds > 10) return;
    if (ticks % 2400 == 0) {
        update_weather();
        if (weather == Weather::Storm || dry_days >= 8) {
            for (int attempt = 0; attempt < 12; ++attempt) {
                Point place{eco_random(64), eco_random(64)};
                if (eco_random(weather == Weather::Storm ? 8 : 40) == 0 && ignite(place)) break;
            }
        }
    }
    // Two fixed arrays: synchronous conservative transfers cannot duplicate water.
    std::array<double, map_size * map_size> runoff{}, groundwater{};
    int flooded = 0;
    std::array<double, map_size * map_size> rain_added{}, evaporated{};
    parallel_tiles(tiles.size(), [&](std::size_t first, std::size_t last) {
    for (std::size_t i = first; i < last; ++i) {
        auto& t = tiles[i];
        if (t.terrain == Terrain::Water) {
            // Permanent water bodies are explicit external reservoirs.
            t.surface_water = .7; t.moisture = 1; t.groundwater = .8; continue;
        }
        const double rain = precipitation * seconds;
        if (weather == Weather::Snow && temperature <= 2) t.snowpack += rain;
        else t.surface_water += rain;
        rain_added[i] = rain;
        const double melt = std::min(t.snowpack, std::max(0.0, temperature) * .0005 * seconds);
        t.snowpack -= melt; t.surface_water += melt;
        double infiltration = std::min({t.surface_water, (1 - t.moisture) * .2, seconds * .003 * (1 - .8 * t.compaction)});
        t.surface_water -= infiltration; t.moisture += infiltration;
        const double percolation = std::min(t.moisture, std::max(0.0, t.moisture - .65) * .008 * seconds);
        t.moisture -= percolation; t.groundwater += percolation;
        const double capillary = std::min(t.groundwater, std::max(0.0, .5 - t.moisture) * .0008 * seconds);
        t.groundwater -= capillary; t.moisture += capillary;
        const double evap = seconds * .00013 * std::max(.2, temperature / 20) * (1 + wind) * (t.terrain == Terrain::Tree ? .65 : 1);
        const double surface_evap = std::min(t.surface_water, evap); t.surface_water -= surface_evap;
        const double soil_evap = std::min(t.moisture, evap * .45); t.moisture -= soil_evap;
        const double transpiration = std::min(t.moisture, seconds * .00002 * t.biomass * plant_growth(t)); t.moisture -= transpiration;
        evaporated[i] = surface_evap + soil_evap + transpiration;
    }
    });
    // Fixed tile order preserves the exact serial floating-point accumulation.
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        ecology.rain_input += rain_added[i]; ecology.evaporation += evaporated[i];
    }
    for (int y = 0; y < map_size; ++y) for (int x = 0; x < map_size; ++x) {
        const int i = y * map_size + x; const auto& t = tiles[i];
        if (t.terrain == Terrain::Water) continue;
        double remaining = t.surface_water, ground_remaining = t.groundwater;
        for (Point d : directions) {
            const int nx = x + d.x, ny = y + d.y;
            if (!inside(nx, ny)) {
                const double loss = std::min(remaining, t.surface_water * .04 * seconds);
                runoff[i] -= loss; remaining -= loss; ecology.runoff_out += loss; continue;
            }
            const int j = ny * map_size + nx; const auto& neighbor = tiles[j];
            const double drop = t.elevation + t.surface_water - neighbor.elevation - neighbor.surface_water;
            const double flow = std::min(remaining, std::max(0.0, drop) * .06 * seconds);
            runoff[i] -= flow; remaining -= flow;
            if (neighbor.terrain == Terrain::Water) ecology.runoff_out += flow; else runoff[j] += flow;
            const double seep = std::min(ground_remaining, std::max(0.0, t.groundwater - neighbor.groundwater) * .002 * seconds);
            groundwater[i] -= seep; ground_remaining -= seep;
            if (neighbor.terrain == Terrain::Water) ecology.runoff_out += seep; else groundwater[j] += seep;
        }
    }
    for (int y = 0; y < map_size; ++y) for (int x = 0; x < map_size; ++x) {
        const int index = y * map_size + x; auto& t = tiles[index];
        if (t.terrain == Terrain::Water) continue;
        t.surface_water = std::max(0.0, t.surface_water + runoff[index]);
        t.groundwater = std::max(0.0, t.groundwater + groundwater[index]);
        const double overflow = std::max(0.0, t.surface_water - 4) + std::max(0.0, t.groundwater - 1) + std::max(0.0, t.snowpack - 4);
        ecology.runoff_out += overflow; t.surface_water = std::min(4.0, t.surface_water); t.groundwater = std::min(1.0, t.groundwater); t.snowpack = std::min(4.0, t.snowpack);
        flooded += t.surface_water > .35;
        const double wash = std::max(0.0, -runoff[index]) * (.05 + .2 * (1 - t.biomass));
        const double weathering = seconds * .000001 * (weather == Weather::Snow ? 3 : 1) * (t.terrain == Terrain::Stone ? 1 : .1);
        t.erosion = bounded(t.erosion + wash + weathering);
        t.elevation = std::max(0.0, t.elevation - wash * .01 - weathering * .01);
        t.fertility = bounded(t.fertility - wash * .015 + weathering * .2);
        t.compaction = std::max(0.0, t.compaction - seconds * .00001 * (1 + t.moisture));
        t.pollination = std::max(.2, t.pollination - seconds * .0001);
        const double decomposition = std::min(t.organic, seconds * .00003 * (.1 + t.moisture) * std::max(.2, temperature / 20));
        t.organic -= decomposition; t.fertility = bounded(t.fertility + decomposition * .4);
        t.plant_age = std::min(100000.0, t.plant_age + seconds / 240);
        const double stress = (t.moisture < .1 ? .012 : 0) + (t.surface_water > .35 ? .02 : 0) + t.disease * .006;
        t.plant_health = std::clamp(t.plant_health + seconds * (.004 * plant_growth(t) - stress), 0.0, 100.0);
        t.disease = bounded(t.disease + seconds * (t.moisture > .85 && t.biomass > .7 ? .00004 : -.00002));
        if (t.fire > 0) {
            t.fire = bounded(t.fire + seconds * (.025 * t.biomass - .06 * t.moisture - (raining() ? .04 : 0)));
            const double burned = std::min(t.biomass, t.fire * seconds * .03);
            t.biomass -= burned; t.organic = bounded(t.organic + burned * .15); t.plant_health = std::max(0.0, t.plant_health - t.fire * seconds * .4);
            if (t.biomass < .02) t.fire = 0;
            if (t.fire > .15 && eco_random(30) == 0) {
                Point d = directions[eco_random(4)]; const int nx = x + d.x, ny = y + d.y;
                if (inside(nx, ny)) { auto& other = tile(nx, ny); if (other.moisture < .3 && other.biomass > .2 && other.terrain != Terrain::Water) other.fire = std::max(other.fire, .15); }
            }
        }
        const double production = seconds * growing_rate() * plant_growth(t);
        t.nectar = t.terrain == Terrain::Bush ? bounded(t.nectar + production * .001) : 0;
        if (t.terrain != Terrain::Stone) t.biomass = bounded(t.biomass + production * .0007 * (1 - t.biomass) - seconds * .00004);
        if (t.terrain == Terrain::Tree) {
            t.organic = bounded(t.organic + seconds * .00002 * t.biomass);
            if (t.wood < 6 && t.plant_health > 20) { t.regrowth += production; if (t.regrowth >= 240) { ++t.wood; t.regrowth -= 240; t.biomass = bounded(t.biomass + .04); } }
            if (t.plant_age > 20 && t.wood >= 3 && ticks % 2400 == 0) {
                Point d = directions[eco_random(4)]; const int nx = x + d.x, ny = y + d.y;
                if (inside(nx, ny)) tile(nx, ny).seed_bank = bounded(tile(nx, ny).seed_bank + .12);
            }
        } else if (t.terrain == Terrain::Bush && t.food < 6 && t.plant_health > 20) {
            t.regrowth += production * (.6 + .4 * t.pollination);
            if (t.regrowth >= 80) { ++t.food; t.regrowth -= 80; t.fertility = std::max(0.0, t.fertility - .0003); }
        }
        if ((t.terrain == Terrain::Tree || t.terrain == Terrain::Bush) && (t.plant_health <= 0 || (t.terrain == Terrain::Tree && t.plant_age > 1800))) {
            t.loose_wood += t.wood; t.wood = t.food = 0; t.terrain = Terrain::Grass; t.regrowth = 0; t.organic = bounded(t.organic + .3);
            t.biomass = .1; t.plant_age = 0; t.plant_health = 50; t.nectar = 0; ++ecology.plants_died;
            gather_orders.erase(std::remove(gather_orders.begin(), gather_orders.end(), Point{x, y}), gather_orders.end());
        }
        int canopy = 0;
        if (t.terrain == Terrain::Grass && t.seed_bank > .6) for (Point d : directions) {
            const int nx = x + d.x, ny = y + d.y;
            if (inside(nx, ny) && tile(nx, ny).terrain == Terrain::Tree) ++canopy;
        }
        // Dense neighboring crowns suppress new trees, leaving understory gaps.
        if (t.terrain == Terrain::Grass && t.seed_bank > .6 && canopy < 3 && t.moisture > .25 && t.fertility > .3 && t.compaction < .2
            && std::abs(x - camp.x) + std::abs(y - camp.y) > 10 && building_at({x, y}) < 0 && ticks % 2400 == 0) {
            t.terrain = Terrain::Tree; t.wood = 1; t.regrowth = 0; t.biomass = .15; t.plant_age = 0; t.plant_health = 75; t.seed_bank = 0; ++ecology.trees_born;
        }
        t.seed_bank = std::max(0.0, t.seed_bank - seconds * .000002);
        if (t.terrain == Terrain::Bush && t.food > 0 && t.pollination > .5 && ticks % 2400 == 0) {
            Point d = directions[eco_random(4)]; const int nx = x + d.x, ny = y + d.y;
            if (inside(nx, ny)) tile(nx, ny).bush_seeds = bounded(tile(nx, ny).bush_seeds + .08);
        }
        if (t.terrain == Terrain::Grass && t.bush_seeds > .6 && t.moisture > .25 && t.fertility > .3 && t.compaction < .2
            && std::abs(x - camp.x) + std::abs(y - camp.y) > 10 && building_at({x, y}) < 0 && ticks % 2400 == 0) {
            t.terrain = Terrain::Bush; t.food = 0; t.regrowth = 0; t.biomass = .2; t.plant_age = 0; t.plant_health = 75; t.bush_seeds = 0;
        }
        t.bush_seeds = std::max(0.0, t.bush_seeds - seconds * .000002);
        if (t.terrain == Terrain::Stone && t.stone > 0 && t.erosion > .5 && ticks % 2400 == 0) { --t.stone; ++t.loose_stone; }
    }
    if (!flood_alert && flooded > 64) { flood_alert = true; event("Flooding: low ground is waterlogged"); }
    else if (flood_alert && flooded < 16) { flood_alert = false; event("Floodwaters have receded"); }
    for (auto& b : buildings) if (b.complete) {
        const auto& t = tile(b.position.x, b.position.y);
        const double flood = std::max(0.0, t.surface_water - .25);
        const double wear = seconds / 240 * (.04 + (raining() ? .12 : 0) + (weather == Weather::Storm ? .35 : 0));
        const double damage = wear + seconds * (flood * .02 + t.fire * .12);
        const double before = b.condition; b.condition = std::max(0.0, b.condition - damage);
        if (before > 20 && b.condition <= 20) event(std::string(building_name(b.kind)) + " unsafe: needs repair");
        if (b.kind == BuildingKind::Farm && b.food < 12 && b.condition > 20) {
            b.growth += seconds * growing_rate() * plant_growth(t) * (b.condition / 100) * (technology_level >= 2 ? 1.15 : 1);
            if (b.growth >= 45) { b.food += std::min(2, 12 - b.food); b.growth -= 45; }
        }
        if (flood > .2 || t.fire > .1) { b.food = std::max(0, b.food - 1); b.fuel = std::max(0.0, b.fuel - seconds * 2); }
    }
    // Unsafe storage spills actual resources onto the communal tile.
    auto& cache = tile(camp.x, camp.y);
    const int food = std::max(0, food_stock - food_capacity()), wood = std::max(0, wood_stock - material_capacity()), stone = std::max(0, stone_stock - material_capacity());
    cache.loose_food += food; cache.loose_wood += wood; cache.loose_stone += stone;
    food_stock -= food; wood_stock -= wood; stone_stock -= stone;
    update_animals(seconds);
}
int World::repair_wood_needed(const Building& b) const { return b.complete && b.condition < 85 ? std::max(0, 1 - b.repair_wood) : 0; }
int World::repair_stone_needed(const Building& b) const { return b.complete && b.condition < 50 && b.kind != BuildingKind::Farm && b.kind != BuildingKind::Campfire ? std::max(0, 1 - b.repair_stone) : 0; }
bool World::repair(Villager& p, Building& b, double dt) {
    if (!b.complete || b.condition >= 85 || repair_wood_needed(b) || repair_stone_needed(b)) return false;
    b.repair_work += dt * (.75 + p.build_skill); practice_work(p, Goal::Build, dt / 2);
    if (p.brain.active) p.brain.progress = std::min(20.0, p.brain.progress + dt * .04);
    if (b.repair_work >= 8) {
        b.condition = std::min(100.0, b.condition + 25); b.repair_work = 0; b.repair_wood = b.repair_stone = 0;
        ++ecology.repairs; event(std::string("Repaired: ") + building_name(b.kind));
    }
    return true;
}
} // namespace village
