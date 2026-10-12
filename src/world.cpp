#include "living_village/world.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace village {
namespace {
Point position(const Villager& p) { return {static_cast<int>(std::round(p.x)), static_cast<int>(std::round(p.y))}; }
int distance(Point a, Point b) { return std::abs(a.x - b.x) + std::abs(a.y - b.y); }
}
int World::random(int maximum) { return static_cast<int>(rng() % static_cast<unsigned>(maximum)); }
World::World(std::uint32_t initial_seed, Biome initial_biome, Geography initial_geography, int starting_population)
    : seed(initial_seed), biome(initial_biome), geography(initial_geography), rng(seed) {
    if (static_cast<int>(biome) < 0 || static_cast<int>(biome) >= biome_count || static_cast<int>(geography) < 0 || static_cast<int>(geography) >= geography_count) throw std::invalid_argument("Unknown biome or geography");
    if (starting_population < 2 || starting_population > population_limit) throw std::invalid_argument("Starting population must be 2 to 20");
    next_person_id = starting_population;
    generate_terrain();
    Building stockpile; stockpile.id = 0; stockpile.kind = BuildingKind::Stockpile; stockpile.position = {33, 32}; stockpile.complete = true;
    Building fire; fire.id = 1; fire.kind = BuildingKind::Campfire; fire.position = camp; fire.complete = true; fire.fuel = 240;
    buildings = {stockpile, fire};
    const char* names[] = {"Mira", "Arin", "Tala", "Bram", "Neri", "Oren", "Lina", "Theo", "Suri", "Kai", "Esme", "Jori", "Ada", "Remy", "Iris", "Ravi", "Fern", "Elio", "Wren", "Cora"};
    for (int i = 0; i < starting_population; ++i) {
        Villager p;
        p.id = i; p.brain.initialize(seed, i); p.name = names[i]; p.x = 31 + i % 3; p.y = 30 + i / 3;
        p.age = 19 + random(24); p.curiosity = 0.3 + random(60) / 100.0;
        p.cooperation = .35 + random(60) / 100.0; p.family_drive = .35 + random(60) / 100.0;
        p.decision_timer = i * .3; p.destination = position(p);
        villagers.push_back(p);
    }
    initialize_ecology();
    event(starting_population == 6 ? "Six villagers arrive at the clearing" : std::to_string(starting_population) + " villagers arrive at the clearing"); record_day();
}
bool World::passable(int x, int y) const {
    return x >= 0 && y >= 0 && x < map_size && y < map_size && tile(x, y).terrain != Terrain::Water;
}
std::vector<Point> World::route(Point from, Point to) const {
    if (!passable(from.x, from.y) || !passable(to.x, to.y) || from == to) return {};
    std::array<int, map_size * map_size> previous; previous.fill(-1);
    std::array<int, map_size * map_size> queue{};
    int head = 0, tail = 0, start = from.y * map_size + from.x, end = to.y * map_size + to.x;
    queue[tail++] = start; previous[start] = start;
    while (head < tail && previous[end] == -1) {
        int cell = queue[head++];
        for (Point d : {Point{1, 0}, Point{-1, 0}, Point{0, 1}, Point{0, -1}}) {
            int x = cell % map_size + d.x, y = cell / map_size + d.y;
            if (!passable(x, y)) continue;
            int next = y * map_size + x;
            if (previous[next] != -1) continue;
            previous[next] = cell; queue[tail++] = next;
        }
    }
    if (previous[end] == -1) return {};
    std::vector<Point> path;
    for (int cell = end; cell != start; cell = previous[cell]) path.push_back({cell % map_size, cell / map_size});
    std::reverse(path.begin(), path.end()); return path;
}
void World::discover(Villager& person) {
    const Point p = position(person);
    auto forget_empty = [&](std::vector<Point>& memory, int kind) {
        memory.erase(std::remove_if(memory.begin(), memory.end(), [&](Point point) {
            const auto& t = tile(point.x, point.y);
            return kind == 0 ? t.terrain != Terrain::Bush && t.loose_food == 0
                : kind == 1 ? t.terrain != Terrain::Tree && t.loose_wood == 0
                : t.stone == 0 && t.loose_stone == 0;
        }), memory.end());
    };
    forget_empty(person.known_food, 0); forget_empty(person.known_wood, 1); forget_empty(person.known_stone, 2);
    for (int y = std::max(0, p.y - 5); y <= std::min(63, p.y + 5); ++y)
        for (int x = std::max(0, p.x - 5); x <= std::min(63, p.x + 5); ++x) {
            const auto& cell = tile(x, y);
            auto learn = [&](std::vector<Point>& memory, bool known, bool loose) {
                if (!known || std::find(memory.begin(), memory.end(), Point{x, y}) != memory.end()) return;
                if (memory.size() >= 48) { if (!loose) return; memory.pop_back(); }
                memory.push_back({x, y});
            };
            learn(person.known_food, cell.terrain == Terrain::Bush || cell.loose_food > 0, cell.loose_food > 0);
            learn(person.known_wood, cell.terrain == Terrain::Tree || cell.loose_wood > 0, cell.loose_wood > 0);
            learn(person.known_stone, cell.stone > 0 || cell.loose_stone > 0, cell.loose_stone > 0);
        }
}
void World::decide(Villager& p) {
    const auto known = p.known_food.size() + p.known_wood.size() + p.known_stone.size();
    discover(p);
    if (p.brain.active) p.brain.progress = std::min(20.0, p.brain.progress + .03 * std::max(0.0, static_cast<double>(p.known_food.size() + p.known_wood.size() + p.known_stone.size()) - static_cast<double>(known)));
    learn(p);
    const Point here = position(p);
    if (p.age < 6) {
        p.path.clear(); p.destination = here; p.target_id = -1; p.action_timer = 0; p.decision_timer = 5;
        p.goal = Goal::Play; p.reason = "Playing safely at home";
        double base = 25 + p.curiosity * 15;
        auto input = observations(p, p.goal, here, base);
        double adjustment = 18 * p.brain.evaluate(input), score = base + adjustment;
        auto child_choice = [&](Goal goal, double utility, const char* why) {
            const auto candidate = observations(p, goal, here, utility);
            const double modifier = 18 * p.brain.evaluate(candidate);
            if (utility + modifier > score) { p.goal = goal; p.reason = why; base = utility; input = candidate; adjustment = modifier; score = utility + modifier; }
        };
        if (p.food > 0 && p.hunger >= 30) child_choice(Goal::Eat, p.hunger * 1.5, "Eating a supplied meal");
        if (p.energy < 75) child_choice(Goal::Rest, (100 - p.energy) * 1.6, "Young child resting at home");
        if (p.hunger >= 25 && p.food == 0) child_choice(Goal::WaitForCare, p.hunger * 1.5, "Too young to forage; needs a caregiver");
        if (p.goal == Goal::Rest) p.target_id = p.home;
        begin_learning(p, input, base, adjustment); p.decision_score = std::max(0.0, score);
        return;
    }
    Goal chosen = Goal::Explore;
    Point destination = here; int target = -1;
    const bool can_experiment = learning_enabled && !p.child() && p.hunger < 60 && p.energy >= 35 && p.health >= 80 && food_stock >= alive_count();
    const int experiment = p.brain.exploration(can_experiment);
    double chosen_exploration = experiment == static_cast<int>(chosen) ? 8 : 0;
    double chosen_base = 25 + p.curiosity * 15;
    BrainInput chosen_input = observations(p, chosen, here, chosen_base);
    double chosen_adjustment = 18 * p.brain.evaluate(chosen_input);
    double highest = chosen_base + chosen_adjustment + chosen_exploration;
    std::string reason = "Curiosity " + std::to_string(static_cast<int>(p.curiosity * 100)) + "; finding resources";
    auto consider = [&](Goal goal, double utility, Point place, const std::string& why, int id = -1) {
        if (!passable(place.x, place.y)) return;
        utility += work_bias(p, goal);
        const double exploratory = experiment == static_cast<int>(goal) ? 8 : 0;
        const auto input = observations(p, goal, place, utility);
        const double adjustment = 18 * p.brain.evaluate(input);
        if (utility + adjustment + exploratory <= highest) return;
        auto path = route(here, place);
        if (!(here == place) && path.empty()) return;
        highest = utility + adjustment + exploratory; chosen_exploration = exploratory; chosen_base = utility; chosen_adjustment = adjustment; chosen_input = input; chosen = goal; destination = place; target = id; reason = why;
    };
    auto resource = [&](const std::vector<Point>& memory, int kind) {
        std::vector<Point> available;
        for (Point point : memory) {
            const auto& cell = tile(point.x, point.y);
            const int amount = kind == 0 ? cell.food + cell.loose_food : kind == 1 ? cell.wood + cell.loose_wood : cell.stone + cell.loose_stone;
            if (amount > 0) available.push_back(point);
        }
        std::stable_sort(available.begin(), available.end(), [&](Point a, Point b) { return distance(here, a) < distance(here, b); });
        for (std::size_t i = 0; i < std::min<std::size_t>(available.size(), 4); ++i)
            if (here == available[i] || !route(here, available[i]).empty()) return available[i];
        return Point{-1, -1};
    };
    const double urgency = p.hunger >= 75 ? 55 : 0;
    const double forage_score = (p.food == 0 ? 45 : 0) + p.hunger * .5 + (food_stock < 12 ? 25 : 0) + urgency;
    if (p.food < 4) {
        consider(Goal::Forage, forage_score, resource(p.known_food, 0), food_stock < 12 ? "Shared food low; gathering known berries" : "Gathering known berries for meals");
        for (const auto& b : buildings) if (b.complete && b.kind == BuildingKind::Farm && b.food > 0)
            consider(Goal::TendFarm, forage_score + 8 + p.farm_skill * 10, b.position, "Ripe farm; hunger and shared food needs", b.id);
    }
    if (p.food > 1 && food_stock < food_capacity())
        consider(Goal::DeliverFood, 55 + p.cooperation * 35 + (food_stock < 8 ? 40 : 0), camp, "Sharing carried food; cooperation " + std::to_string(static_cast<int>(p.cooperation * 100)));
    if (!p.child() && p.wood + p.stone > 0 && ((p.wood > 0 && wood_stock < material_capacity()) || (p.stone > 0 && stone_stock < material_capacity())))
        consider(Goal::StoreMaterials, 70 + p.cooperation * 15, camp, "Returning spare materials to shared stock");
    for (const auto& b : buildings) {
        if (p.child()) break;
        if (b.complete) {
            if (b.condition < 85) {
                const int wood = repair_wood_needed(b), stone = repair_stone_needed(b);
                const double urgency = (100 - b.condition) * .6;
                if ((wood && p.wood) || (stone && p.stone)) consider(Goal::DeliverMaterials, 72 + urgency + p.cooperation * 20, b.position, "Weathered structure needs carried repair supplies", b.id);
                else if ((wood && wood_stock) || (stone && stone_stock)) consider(Goal::TakeMaterials, 64 + urgency + p.cooperation * 20, camp, "Collecting shared maintenance materials", b.id);
                if (p.wood + p.stone < 6) {
                    if (wood > p.wood && wood_stock == 0) consider(Goal::GatherWood, 58 + urgency + p.cooperation * 20, resource(p.known_wood, 1), "Wood needed for worn structures", b.id);
                    if (stone > p.stone && stone_stock == 0) consider(Goal::CollectStone, 58 + urgency + p.cooperation * 20, resource(p.known_stone, 2), "Stone needed for worn structures", b.id);
                }
                if (!wood && !stone) consider(Goal::Build, 65 + urgency + p.cooperation * 20, b.position, "Maintenance supplied; repairing damage", b.id);
                continue;
            }
            if (b.kind == BuildingKind::Workshop && research_available()) {
                const int wood = research_wood_needed(b), stone = research_stone_needed(b);
                if ((wood > 0 && p.wood > 0) || (stone > 0 && p.stone > 0)) consider(Goal::DeliverMaterials, 72 + p.cooperation * 25, b.position, "Workshop needs supplies for new technology", b.id);
                else if ((wood > 0 && wood_stock > 0) || (stone > 0 && stone_stock > 0)) consider(Goal::TakeMaterials, 65 + p.cooperation * 25, camp, "Collecting shared research supplies", b.id);
                if (p.wood + p.stone < 6) {
                    if (wood > p.wood && wood_stock == 0) consider(Goal::GatherWood, 60 + p.cooperation * 25, resource(p.known_wood, 1), "Gathering wood for improved tools", b.id);
                    if (stone > p.stone && stone_stock == 0) consider(Goal::CollectStone, 60 + p.cooperation * 25, resource(p.known_stone, 2), "Gathering stone for improved tools", b.id);
                }
                if (wood == 0 && stone == 0) consider(Goal::Build, 62 + p.cooperation * 25, b.position, "Supplied workshop; developing village technology", b.id);
            }
            if (b.kind == BuildingKind::Campfire && b.fuel <= 30) {
                if (p.wood > 0) consider(Goal::RefuelFire, 88 + p.cooperation * 25, b.position, "Communal fire low; delivering carried wood", b.id);
                else if (wood_stock > 0 && p.wood + p.stone < 6) consider(Goal::TakeMaterials, 62 + p.cooperation * 25, camp, "Collecting one shared wood to refuel fire", b.id);
            }
            continue;
        }
        const std::string need = std::string(building_name(b.kind)) + " needs " + std::to_string(b.wood_needed()) + " wood / " + std::to_string(b.stone_needed()) + " stone";
        const double affinity = b.initiator >= 0 && b.initiator != p.id ? p.relationships[b.initiator] : 0;
        if ((p.wood > 0 && b.wood_needed() > 0) || (p.stone > 0 && b.stone_needed() > 0))
            consider(Goal::DeliverMaterials, 90 + p.cooperation * 35 + affinity * .1, b.position, need + "; helping, affinity " + std::to_string(static_cast<int>(affinity)), b.id);
        else if ((b.wood_needed() > 0 && wood_stock > 0) || (b.stone_needed() > 0 && stone_stock > 0))
            consider(Goal::TakeMaterials, 65 + p.cooperation * 30, camp, need + "; supplies at camp", b.id);
        if (p.wood + p.stone < 6) {
            if (b.wood_needed() > p.wood && wood_stock == 0)
                consider(Goal::GatherWood, 58 + p.cooperation * 30 + p.gather_skill * 8, resource(p.known_wood, 1), need + "; gathering wood", b.id);
            if (b.stone_needed() > p.stone && stone_stock == 0)
                consider(Goal::CollectStone, 57 + p.cooperation * 30 + p.gather_skill * 8, resource(p.known_stone, 2), need + "; gathering stone", b.id);
        }
        if (b.supplied()) consider(Goal::Build, 58 + p.cooperation * 30 + p.build_skill * 15, b.position, std::string(building_name(b.kind)) + " supplied; contributing work", b.id);
    }
    if (!p.child() && wood_stock < 3 && p.wood + p.stone < 6)
        consider(Goal::GatherWood, 40 + p.cooperation * 20, resource(p.known_wood, 1), "Keeping communal fire supplied with wood");
    // Player designations publish local resource information to the village.
    for (Point order : gather_orders) {
        const auto terrain = tile(order.x, order.y).terrain;
        const bool food = terrain == Terrain::Bush;
        if (p.child() && !food) continue;
        if (food ? (p.food >= 4 || food_stock >= food_capacity()) : (p.wood + p.stone >= 6 || (terrain == Terrain::Tree ? wood_stock : stone_stock) >= material_capacity())) continue;
        Point nearest{-1, -1}; int best = std::numeric_limits<int>::max();
        for (int y = std::max(0, order.y - 2); y <= std::min(63, order.y + 2); ++y)
            for (int x = std::max(0, order.x - 2); x <= std::min(63, order.x + 2); ++x) {
                const auto& cell = tile(x, y);
                if (cell.terrain != terrain || (food ? cell.food : terrain == Terrain::Tree ? cell.wood : cell.stone) <= 0) continue;
                if (distance(here, {x, y}) < best) { best = distance(here, {x, y}); nearest = {x, y}; }
            }
        consider(food ? Goal::Forage : terrain == Terrain::Tree ? Goal::GatherWood : Goal::CollectStone,
                 68 + p.cooperation * 25, nearest, "Player gathering area; shared stock has space");
    }
    if (!p.child()) for (const auto& child : villagers) {
        const bool parent = child.parents[0] == p.id || child.parents[1] == p.id;
        const bool known = parent || (p.home >= 0 && p.home == child.home) || distance(here, position(child)) <= 5;
        if (!child.alive() || !child.child() || child.hunger < 40 || !known) continue;
        const double utility = child.hunger * 1.35 + p.cooperation * 30 + (parent ? 20 : 0);
        if (p.food > 0) consider(Goal::Care, utility, position(child), child.name + " needs a meal; " + (parent ? "my child" : "village care"), child.id);
        else if (food_stock > 0) consider(Goal::CollectMeal, utility, camp, "Collecting shared food for " + child.name, child.id);
    }
    if (p.food > 0 || food_stock > 0)
        consider(Goal::Eat, p.hunger * 1.5 + urgency, p.food > 0 ? here : camp, "Hunger " + std::to_string(static_cast<int>(p.hunger)) + "; food available");
    consider(Goal::Rest, (100 - p.energy) * 1.6, camp, "Energy " + std::to_string(static_cast<int>(p.energy)) + "; resting at camp");
    for (const auto& b : buildings) if (b.complete && b.condition > 20 && b.kind == BuildingKind::Shelter) {
        int occupants = 0;
        for (const auto& other : villagers) if (other.id != p.id && other.alive() && other.goal == Goal::Rest && other.target_id == b.id) ++occupants;
        if (occupants < 4) consider(Goal::Rest, (100 - p.energy) * 1.6 + (p.home == b.id ? 12 : 5), b.position, p.home == b.id ? "Energy low; returning to my household" : "Energy low; shelter improves rest", b.id);
    }
    for (const auto& other : villagers) if (other.id != p.id && other.alive() && distance(here, position(other)) <= 8) {
        if (p.food > 1 && other.food < 4 && other.hunger >= 60 && p.hunger < 70 && (p.cooperation >= .5 || p.relationships[other.id] >= 20))
            consider(Goal::Socialize, other.hunger * 1.25 + p.cooperation * 20, position(other), "Helping hungry " + other.name + " with a spare meal", other.id);
        else if (!p.child() && !other.child() && p.food >= 3 && other.food <= 1 && other.wood + other.stone >= 2 && p.wood + p.stone < 6 && (wood_stock < 3 || stone_stock < 2))
            consider(Goal::Socialize, 65 + p.cooperation * 20, position(other), "Trading surplus meals for " + other.name + "'s materials", other.id);
        if (p.relationships[other.id] < 0 && p.hunger < 70 && other.hunger < 70)
            consider(Goal::Socialize, 40 + p.cooperation * 40, position(other), "Trying to reconcile with " + other.name, other.id);
        const bool companionship = p.age >= 18 && other.age >= 18 && p.partner < 0 && std::abs(p.age - other.age) < 10;
        consider(Goal::Socialize, p.loneliness * .8 + p.cooperation * 15 + p.relationships[other.id] * .05 + (companionship ? p.family_drive * 15 : 0),
                 position(other), companionship ? "Seeking companionship with " + other.name : "Loneliness " + std::to_string(static_cast<int>(p.loneliness)) + "; spending time with " + other.name, other.id);
    }
    if (p.child()) consider(Goal::Play, 30 + p.curiosity * 20, here, "Playing and learning near home");
    if (ecology_enabled && (tile(here.x, here.y).fire > .1 || tile(here.x, here.y).surface_water > .5)) {
        for (int radius = 1; radius <= 3; ++radius) for (Point d : {Point{1, 0}, Point{-1, 0}, Point{0, 1}, Point{0, -1}}) {
            Point safe{here.x + d.x * radius, here.y + d.y * radius};
            if (passable(safe.x, safe.y) && tile(safe.x, safe.y).fire < .05 && tile(safe.x, safe.y).surface_water < .3)
                consider(Goal::Explore, 240 - radius, safe, "Escaping dangerous fire or flood");
        }
    }
    if ((chosen == Goal::Explore || chosen == Goal::Play) && chosen_base < 200) {
        Point origin = p.child() && p.home >= 0 ? buildings[p.home].position : here;
        const int radius = p.child() ? 2 : 6;
        for (int attempt = 0; attempt < 12; ++attempt) {
            Point candidate{std::clamp(origin.x + random(radius * 2 + 1) - radius, 0, 63), std::clamp(origin.y + random(radius * 2 + 1) - radius, 0, 63)};
            if (!route(here, candidate).empty()) { destination = candidate; break; }
        }
    }
    p.goal = chosen; p.target_id = target; p.destination = destination; p.reason = reason; p.decision_score = std::max(0.0, highest);
    begin_learning(p, chosen_input, chosen_base, chosen_adjustment);
    p.brain.selected_exploration = chosen_exploration;
    if (chosen_exploration > 0 && p.brain.experiments < 100000000000ULL) ++p.brain.experiments;
    p.path = route(here, destination); p.action_timer = 0; p.decision_timer = 5 + (p.id % population_limit) * .07;
    if (target >= 0 && (chosen == Goal::Build || chosen == Goal::GatherWood || chosen == Goal::CollectStone || chosen == Goal::TakeMaterials || chosen == Goal::DeliverMaterials) && !buildings[target].complete && buildings[target].initiator < 0)
        { buildings[target].initiator = p.id; buildings[target].initiator_name = p.name; }
}
void World::remember(Villager& person, const std::string& experience) {
    if (person.history.size() >= 6) person.history.pop_front();
    person.history.push_back(experience);
}
void World::execute(Villager& p, double dt) {
    if (!p.path.empty()) {
        const auto& ground = tile(position(p).x, position(p).y);
        double movement = dt * 1.6 * (ecology_enabled ? (1 + ground.compaction * .1) * (1 - std::min(.6, ground.surface_water)) : 1);
        for (int edges = 0; !p.path.empty() && movement > 0 && edges < 4; ++edges) {
            const Point next = p.path.front();
            const double dx = next.x - p.x, dy = next.y - p.y, remaining = std::hypot(dx, dy);
            if (remaining <= movement) { p.x = next.x; p.y = next.y; p.path.erase(p.path.begin()); movement -= remaining; if (ecology_enabled) trample(next); }
            else { p.x += dx / remaining * movement; p.y += dy / remaining * movement; movement = 0; }
        }
        return;
    }
    p.action_timer += dt;
    const Point here = position(p);
    auto reward = [&](double amount) { if (p.brain.active) p.brain.progress = std::clamp(p.brain.progress + amount, -20.0, 20.0); };
    auto finish = [&]() { p.decision_timer = 0; p.action_timer = 0; };
    const bool person_target = p.goal == Goal::Socialize || p.goal == Goal::Care || p.goal == Goal::CollectMeal;
    auto* site = p.target_id >= 0 && p.target_id < static_cast<int>(buildings.size()) && !person_target ? &buildings[p.target_id] : nullptr;
    if (p.goal == Goal::Eat && p.action_timer >= 1) {
        bool available = false;
        if (p.food > 0) { --p.food; available = true; }
        else if (here == camp && food_stock > 0) { --food_stock; available = true; }
        if (available) { p.hunger = std::max(0.0, p.hunger - 45); remember(p, "Ate one food"); }
        finish();
    } else if (p.goal == Goal::Rest) {
        double efficiency = here == camp ? 1.8 : .8;
        if (site && site->complete && site->kind == BuildingKind::Shelter && here == site->position && site->condition > 20) {
            efficiency = 1.8 + 1.4 * site->condition / 100;
            if (ecology_enabled) site->condition = std::max(0.0, site->condition - dt * .00003);
        }
        for (const auto& fire : buildings) if (fire.complete && fire.kind == BuildingKind::Campfire && fire.fuel > 0 && distance(here, fire.position) <= 2) { efficiency += .3; break; }
        p.energy = std::min(100.0, p.energy + dt * efficiency);
        if (p.energy >= 96 || p.hunger > 80) finish();
    } else if ((p.goal == Goal::Forage || p.goal == Goal::GatherWood || p.goal == Goal::CollectStone) && p.action_timer >= 2.0 / ((1 + p.gather_skill) * (technology_level >= 1 ? 1.1 : 1))) {
        auto& cell = tile(here.x, here.y);
        int* loose = p.goal == Goal::Forage ? &cell.loose_food : p.goal == Goal::GatherWood ? &cell.loose_wood : &cell.loose_stone;
        int* natural = p.goal == Goal::Forage ? &cell.food : p.goal == Goal::GatherWood ? &cell.wood : &cell.stone;
        int* available = *loose > 0 ? loose : natural;
        int* inventory = p.goal == Goal::Forage ? &p.food : p.goal == Goal::GatherWood ? &p.wood : &p.stone;
        bool space = p.goal == Goal::Forage ? p.food < 4 : p.wood + p.stone < 6;
        if (*available == 0 && space) resource_conflict(p);
        if (*available > 0 && space) {
            --*available; ++*inventory; reward(p.goal == Goal::Forage ? .12 : .10); practice_work(p, p.goal, 1); p.gather_skill = std::min(1.0, p.gather_skill + .005);
            if (ecology_enabled && available == natural && p.goal == Goal::GatherWood) { cell.biomass = std::max(.05, cell.biomass - .08); cell.plant_health = std::max(1.0, cell.plant_health - 1); }
        }
        p.action_timer = 0;
        if (*available == 0 || (p.goal == Goal::Forage ? p.food >= 4 : p.wood + p.stone >= 6)) finish();
    } else if (p.goal == Goal::TendFarm && p.action_timer >= 3 / (1 + p.farm_skill)) {
        if (site && site->complete && here == site->position) {
            int amount = std::min(4 - p.food, site->food);
            site->food -= amount; p.food += amount;
            if (amount > 0) { reward(.12 * amount); practice_work(p, p.goal, amount); p.farm_skill = std::min(1.0, p.farm_skill + .01); remember(p, "Harvested " + std::to_string(amount) + " farm food"); }
        }
        finish();
    } else if (p.goal == Goal::DeliverFood && here == camp && p.action_timer >= 1) {
        int amount = std::min(std::max(0, p.food - 1), food_capacity() - food_stock);
        if (amount > 0) { food_stock += amount; p.food -= amount; reward(.12 * amount * p.cooperation); practice_work(p, p.goal, amount); remember(p, "Shared " + std::to_string(amount) + " food at camp"); }
        finish();
    } else if (p.goal == Goal::StoreMaterials && here == camp && p.action_timer >= 1) {
        int wood = std::min(p.wood, std::max(0, material_capacity() - wood_stock));
        int stone = std::min(p.stone, std::max(0, material_capacity() - stone_stock));
        wood_stock += wood; stone_stock += stone; p.wood -= wood; p.stone -= stone;
        if (wood + stone > 0) { reward(.08 * (wood + stone) * p.cooperation); practice_work(p, p.goal, wood + stone); remember(p, "Stored spare materials at camp"); }
        finish();
    } else if (p.goal == Goal::TakeMaterials && here == camp && p.action_timer >= 1) {
        if (site && site->complete && site->condition < 85) {
            int wood = std::min({std::max(0, repair_wood_needed(*site) - p.wood), wood_stock, 6 - p.wood - p.stone}); wood_stock -= wood; p.wood += wood;
            int stone = std::min({std::max(0, repair_stone_needed(*site) - p.stone), stone_stock, 6 - p.wood - p.stone}); stone_stock -= stone; p.stone += stone; reward(.06 * (wood + stone));
        } else if (site && site->complete && site->kind == BuildingKind::Campfire) {
            int amount = std::min({1, wood_stock, 6 - p.wood - p.stone}); wood_stock -= amount; p.wood += amount; reward(.06 * amount);
        } else if (site && site->complete && site->kind == BuildingKind::Workshop && research_available()) {
            int wood = std::min({std::max(0, research_wood_needed(*site) - p.wood), wood_stock, 6 - p.wood - p.stone});
            wood_stock -= wood; p.wood += wood;
            int stone = std::min({std::max(0, research_stone_needed(*site) - p.stone), stone_stock, 6 - p.wood - p.stone});
            stone_stock -= stone; p.stone += stone; reward(.06 * (wood + stone));
        } else if (site && !site->complete) {
            int wood = std::min({std::max(0, site->wood_needed() - p.wood), wood_stock, 6 - p.wood - p.stone});
            wood_stock -= wood; p.wood += wood;
            int stone = std::min({std::max(0, site->stone_needed() - p.stone), stone_stock, 6 - p.wood - p.stone});
            stone_stock -= stone; p.stone += stone; reward(.06 * (wood + stone));
        }
        finish();
    } else if (p.goal == Goal::RefuelFire && p.action_timer >= 1) {
        if (site && site->complete && site->kind == BuildingKind::Campfire && here == site->position && site->fuel <= 30 && p.wood > 0) {
            --p.wood; site->fuel += 240; reward(.3 * p.cooperation); practice_work(p, p.goal, 1); remember(p, "Carried wood to refuel campfire");
        }
        finish();
    } else if (p.goal == Goal::DeliverMaterials && p.action_timer >= 1) {
        if (site && site->complete && site->condition < 85 && here == site->position) {
            int wood = std::min(p.wood, repair_wood_needed(*site)), stone = std::min(p.stone, repair_stone_needed(*site));
            p.wood -= wood; p.stone -= stone; site->repair_wood += wood; site->repair_stone += stone;
            reward(.12 * (wood + stone)); practice_work(p, p.goal, wood + stone);
        } else if (site && site->complete && site->kind == BuildingKind::Workshop && here == site->position && research_available()) {
            int wood = std::min(p.wood, research_wood_needed(*site)), stone = std::min(p.stone, research_stone_needed(*site));
            site->upgrade_wood += wood; site->upgrade_stone += stone; p.wood -= wood; p.stone -= stone;
            reward(.12 * (wood + stone)); practice_work(p, p.goal, wood + stone);
        } else if (site && !site->complete && here == site->position) {
            int wood = std::min(p.wood, site->wood_needed()), stone = std::min(p.stone, site->stone_needed());
            site->wood_delivered += wood; site->stone_delivered += stone; p.wood -= wood; p.stone -= stone;
            if (wood + stone > 0) {
                reward(.16 * (wood + stone) * p.cooperation); practice_work(p, p.goal, wood + stone);
                site->contribute(p.id);
                remember(p, std::string("Supplied ") + building_name(site->kind));
                event(p.name + " delivers " + std::to_string(wood) + " wood / " + std::to_string(stone) + " stone to " + building_name(site->kind));
                if (site->initiator >= 0 && site->initiator != p.id) {
                    p.relationships[site->initiator] = std::min(100.0, p.relationships[site->initiator] + 1);
                    if (auto* recipient = person(site->initiator)) {
                        recipient->relationships[p.id] = std::min(100.0, recipient->relationships[p.id] + 2);
                        remember(*recipient, p.name + " helped supply my project");
                    }
                }
            }
        }
        finish();
    } else if (p.goal == Goal::Build) {
        if (site && site->complete && site->condition < 85) { if (!(here == site->position) || !repair(p, *site, dt)) finish(); return; }
        if (site && site->complete && site->kind == BuildingKind::Workshop) { if (!research(p, *site, dt)) finish(); return; }
        if (!site || site->complete || !site->supplied() || !(here == site->position)) { finish(); return; }
        reward(dt * .04 * p.cooperation); practice_work(p, p.goal, dt / 2);
        site->work += dt * (.75 + p.build_skill) * (technology_level >= 1 ? 1.1 : 1); p.build_skill = std::min(1.0, p.build_skill + dt * .0005);
        site->contribute(p.id);
        if (site->work >= building_cost(site->kind).work) {
            site->work = building_cost(site->kind).work; site->complete = true;
            if (site->kind == BuildingKind::Campfire) site->fuel = 240;
            reward(.35);
            event(std::string("Completed: ") + building_name(site->kind));
            remember(p, std::string("Finished ") + building_name(site->kind)); finish();
        }
    } else if (p.goal == Goal::CollectMeal && here == camp && p.action_timer >= 1) {
        const auto* child = person(p.target_id);
        if (child && child->alive() && child->child() && child->hunger >= 40 && food_stock > 0 && p.food < 4) { --food_stock; ++p.food; reward(.08); }
        finish();
    } else if (p.goal == Goal::Care && p.action_timer >= 1) {
        if (auto* child = person(p.target_id)) if (child->alive() && child->child() && child->hunger >= 40 && p.food > 0 && distance(here, position(*child)) <= 1) {
            reward(.5 * (.5 + p.cooperation)); practice_work(p, p.goal, 1);
            --p.food; child->hunger = std::max(0.0, child->hunger - 45); child->loneliness = std::max(0.0, child->loneliness - 20);
            child->relationships[p.id] = std::min(100.0, child->relationships[p.id] + 2);
            remember(p, "Brought a meal to " + child->name); remember(*child, p.name + " brought me food");
        }
        finish();
    } else if (p.goal == Goal::Play && p.action_timer >= 3) {
        p.loneliness = std::max(0.0, p.loneliness - 8); if (p.gather_skill < .2) p.gather_skill = std::min(.2, p.gather_skill + .0005); finish();
    } else if (p.goal == Goal::Socialize && p.action_timer >= 3) {
        if (auto* target = person(p.target_id)) {
            auto& other = *target;
            if (other.alive() && distance(here, position(other)) <= 2) {
                const double old = p.relationships[other.id];
                reward(social_exchange(p, other));
                bool comfort = false;
                for (const auto& fire : buildings) if (fire.complete && fire.kind == BuildingKind::Campfire && fire.fuel > 0 && distance(here, fire.position) <= 3) { comfort = true; break; }
                p.loneliness = std::max(0.0, p.loneliness - (comfort ? 45 : 35)); other.loneliness = std::max(0.0, other.loneliness - 20);
                p.relationships[other.id] = std::min(100.0, p.relationships[other.id] + (comfort ? 4 : 3)); other.relationships[p.id] = std::min(100.0, other.relationships[p.id] + 3);
                remember(p, "Spent time with " + other.name);
                if (old < 20 && p.relationships[other.id] >= 20) event(p.name + " befriended " + other.name);
                form_household(p.id, other.id);
            }
        }
        finish();
    }
}
void World::update(double dt) {
    if (!std::isfinite(dt) || dt <= 0 || dt > 1) return;
    ++ticks;
    if (ecology_enabled && ticks % 2400 == 0) recruit_wildlife();
    const double growth = dt * growing_rate();
    if (ecology_enabled && ticks % 10 == 0) update_ecology(dt * 10);
    if (!ecology_enabled) for (auto& cell : tiles) {
        if (cell.terrain == Terrain::Bush && cell.food < 6) {
            cell.regrowth += growth;
            if (cell.regrowth >= 80) { ++cell.food; cell.regrowth -= 80; }
        } else if (cell.terrain == Terrain::Tree && cell.wood < 6) {
            cell.regrowth += growth;
            if (cell.regrowth >= 240) { ++cell.wood; cell.regrowth -= 240; }
        }
    }
    for (auto& b : buildings) if (b.complete) {
        if (!ecology_enabled && b.kind == BuildingKind::Farm && b.food < 12) {
            b.growth += growth * (technology_level >= 2 ? 1.15 : 1);
            if (b.growth >= 45) { b.food += std::min(2, 12 - b.food); b.growth -= 45; }
        }
        if (b.kind == BuildingKind::Campfire) {
            b.fuel = std::max(0.0, b.fuel - dt);
        }
    }
    if (ticks % 50 == 1) { update_homes(); update_needs(); }
    for (auto& p : villagers) {
        next_person_id = std::max(next_person_id, p.id + 1);
        if (!p.alive()) { if (p.death_day == 0) kill(p.id, "Hunger"); continue; }
        if (p.brain.active) p.brain.elapsed = std::min(100000.0, p.brain.elapsed + dt);
        p.hunger = std::min(100.0, p.hunger + dt * (p.age < 6 ? .06 : p.child() ? .09 : .12)); p.energy = std::max(0.0, p.energy - dt * (p.child() ? .025 : .065));
        p.loneliness = std::min(100.0, p.loneliness + dt * .045);
        if (ecology_enabled) {
            const auto& hazard = tile(position(p).x, position(p).y);
            p.health = std::max(0.0, p.health - dt * (hazard.fire * .3 + std::max(0.0, hazard.surface_water - .6) * .06));
            if (!p.alive()) { kill(p.id, hazard.fire > .1 ? "Wildfire" : "Flood"); continue; }
            // Interrupt travel once, but let a stationary meal/rest/repair finish.
            // Replanning every tick used to reset the eating timer in flooded camp.
            if ((hazard.fire > .1 || hazard.surface_water > .5) && p.goal != Goal::Explore && !p.path.empty()) { p.path.clear(); p.decision_timer = 0; }
        }
        if (p.hunger >= 99) p.health = std::max(0.0, p.health - dt * .25);
        else p.health = std::min(100.0, p.health + dt * .02);
        if (!p.alive()) { kill(p.id, "Hunger"); continue; }
        p.decision_timer = std::max(0.0, p.decision_timer - dt);
        // Carrying a meal permits immediate survival action during a long trip.
        if (p.hunger >= 85 && p.food > 0 && p.goal != Goal::Eat) { p.path.clear(); p.decision_timer = 0; }
        if (p.decision_timer <= 0 && p.path.empty()) decide(p);
        execute(p, dt);
    }
    if (ticks % 2400 == 0) { update_population(); record_day(); }
    prune_history();
}
void World::event(const std::string& message) {
    if (event_sequence < 100000000000ULL) ++event_sequence;
    if (chronicle.size() >= 512) chronicle.pop_front();
    chronicle.push_back({ticks, message});
    if (events.size() >= 24) events.pop_front();
    events.push_back("Day " + std::to_string(day()) + ": " + message);
}
double World::daylight() const { return (std::sin(ticks * fixed_step / 240 * 6.283185307 - 1.570796327) + 1) / 2; }
const char* goal_name(Goal goal) {
    switch (goal) {
        case Goal::Explore: return "Exploring"; case Goal::Forage: return "Gathering food";
        case Goal::Eat: return "Getting a meal"; case Goal::Rest: return "Resting";
        case Goal::GatherWood: return "Gathering wood"; case Goal::CollectStone: return "Collecting stone";
        case Goal::DeliverMaterials: return "Delivering materials"; case Goal::TakeMaterials: return "Collecting shared supplies";
        case Goal::Build: return "Building"; case Goal::TendFarm: return "Harvesting farm";
        case Goal::DeliverFood: return "Sharing food"; case Goal::StoreMaterials: return "Storing materials";
        case Goal::Socialize: return "Socializing";
        case Goal::RefuelFire: return "Refuelling campfire";
        case Goal::Arrive: return "Joining the village"; case Goal::Play: return "Playing near home";
        case Goal::Care: return "Bringing a child food"; case Goal::CollectMeal: return "Collecting a care meal";
        case Goal::WaitForCare: return "Waiting for a caregiver";
    }
    return "Unknown";
}
} // namespace village
