#include "living_village/world.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace village {
namespace {
Point position(const Villager& person) { return {static_cast<int>(std::round(person.x)), static_cast<int>(std::round(person.y))}; }
int distance(Point a, Point b) { return std::abs(a.x - b.x) + std::abs(a.y - b.y); }
constexpr Point camp{32, 32};
}
int World::random(int maximum) { return static_cast<int>(rng() % static_cast<unsigned>(maximum)); }

World::World(std::uint32_t initial_seed) : seed(initial_seed), rng(seed) {
    for (int y = 0; y < map_size; ++y) {
        for (int x = 0; x < map_size; ++x) {
            const int roll = random(1000);
            auto& cell = tile(x, y);
            cell.terrain = roll < 140 ? Terrain::Tree : roll < 190 ? Terrain::Stone : roll < 255 ? Terrain::Bush : Terrain::Grass;
            if (distance({x, y}, camp) < 7) cell.terrain = Terrain::Grass;
            if (cell.terrain == Terrain::Bush) cell.food = 6;
        }
    }
    // Seeded ponds make readable obstacles rather than isolated water pixels.
    for (int pond = 0; pond < 7; ++pond) {
        const int px = 3 + random(58), py = 3 + random(58);
        for (int y = std::max(0, py - 4); y <= std::min(63, py + 4); ++y)
            for (int x = std::max(0, px - 5); x <= std::min(63, px + 5); ++x)
                if ((x - px) * (x - px) / 2 + (y - py) * (y - py) < 13 && distance({x, y}, camp) > 9)
                    tile(x, y) = {Terrain::Water, 0, 0};
    }
    // Reliable accessible starting food; future play can still exhaust stocks.
    for (Point p : {Point{25, 32}, Point{39, 32}, Point{32, 25}, Point{32, 39}})
        tile(p.x, p.y) = {Terrain::Bush, 6, 0};
    const char* names[] = {"Mira", "Arin", "Tala", "Bram", "Neri", "Oren"};
    for (int i = 0; i < 6; ++i) {
        Villager person;
        person.id = i; person.name = names[i];
        person.x = 31 + i % 3; person.y = 30 + i / 3;
        person.age = 19 + random(24); person.curiosity = 0.3 + random(60) / 100.0;
        person.decision_timer = i * 0.3;
        villagers.push_back(person);
    }
    event("Six villagers arrive at the clearing");
}

bool World::passable(int x, int y) const {
    return x >= 0 && y >= 0 && x < map_size && y < map_size && tile(x, y).terrain != Terrain::Water;
}

std::vector<Point> World::route(Point from, Point to) const {
    if (!passable(from.x, from.y) || !passable(to.x, to.y) || from == to) return {};
    std::array<int, map_size * map_size> previous;
    previous.fill(-1);
    std::array<int, map_size * map_size> queue{};
    int head = 0, tail = 0;
    int start = from.y * map_size + from.x, end = to.y * map_size + to.x;
    queue[tail++] = start; previous[start] = start;
    while (head < tail && previous[end] == -1) {
        const int cell = queue[head++];
        for (Point d : {Point{1, 0}, Point{-1, 0}, Point{0, 1}, Point{0, -1}}) {
            const int x = cell % map_size + d.x, y = cell / map_size + d.y;
            if (!passable(x, y)) continue;
            const int next = y * map_size + x;
            if (previous[next] != -1) continue;
            previous[next] = cell; queue[tail++] = next;
        }
    }
    if (previous[end] == -1) return {};
    std::vector<Point> path;
    for (int cell = end; cell != start; cell = previous[cell]) path.push_back({cell % map_size, cell / map_size});
    std::reverse(path.begin(), path.end());
    return path;
}

void World::discover(Villager& person) {
    const Point p = position(person);
    for (int y = std::max(0, p.y - 5); y <= std::min(63, p.y + 5); ++y)
        for (int x = std::max(0, p.x - 5); x <= std::min(63, p.x + 5); ++x)
            if (tile(x, y).terrain == Terrain::Bush && person.known_food.size() < 48 &&
                std::find(person.known_food.begin(), person.known_food.end(), Point{x, y}) == person.known_food.end())
                person.known_food.push_back({x, y});
}

void World::decide(Villager& person) {
    discover(person);
    const Point current = position(person);
    Goal chosen = Goal::Explore;
    Point destination = current;
    const double eat_score = person.hunger * 1.5;
    const double rest_score = (100 - person.energy) * 1.6;
    const double forage_score = (person.food == 0 ? 45 : 0) + person.hunger * 0.5 + (food_stock < 12 ? 25 : 0);
    const double explore_score = 25 + person.curiosity * 15;
    double highest = explore_score;
    person.reason = "Curiosity " + std::to_string(static_cast<int>(person.curiosity * 100)) + "; looking for resources";
    if (forage_score > highest && person.food < 4) {
        int nearest = std::numeric_limits<int>::max();
        for (Point resource : person.known_food) {
            if (tile(resource.x, resource.y).food <= 0) continue;
            const int d = distance(current, resource);
            if (d < nearest) { nearest = d; destination = resource; }
        }
        if (nearest != std::numeric_limits<int>::max()) {
            highest = forage_score; chosen = Goal::Forage;
            person.reason = food_stock < 12 ? "Village food is low; known berry bush" : "Known berries; filling food inventory";
        }
    }
    if (eat_score > highest && (person.food > 0 || food_stock > 0)) {
        highest = eat_score; chosen = Goal::Eat;
        destination = person.food > 0 ? current : camp;
        person.reason = "Hunger " + std::to_string(static_cast<int>(person.hunger)) + "; food available";
    }
    if (rest_score > highest) {
        chosen = Goal::Rest; destination = camp;
        person.reason = "Energy " + std::to_string(static_cast<int>(person.energy)) + "; camp improves rest";
    }
    if (chosen == Goal::Explore) {
        // Bounded attempts: inaccessible ponds never trap the decision loop.
        for (int attempt = 0; attempt < 12; ++attempt) {
            Point candidate{std::clamp(current.x + random(13) - 6, 0, 63), std::clamp(current.y + random(13) - 6, 0, 63)};
            auto path = route(current, candidate);
            if (!path.empty()) { destination = candidate; break; }
        }
    }
    person.goal = chosen;
    person.path = route(current, destination);
    person.action_timer = 0;
    person.decision_timer = 5 + person.id * 0.07;
}

void World::execute(Villager& person, double dt) {
    if (!person.path.empty()) {
        Point next = person.path.front();
        const double dx = next.x - person.x, dy = next.y - person.y;
        const double remaining = std::hypot(dx, dy);
        const double movement = dt * 1.6;
        if (remaining <= movement) {
            person.x = next.x; person.y = next.y;
            person.path.erase(person.path.begin());
        } else {
            person.x += dx / remaining * movement; person.y += dy / remaining * movement;
        }
        return;
    }
    person.action_timer += dt;
    const Point current = position(person);
    if (person.goal == Goal::Eat && person.action_timer >= 1) {
        bool available = false;
        if (person.food > 0) { --person.food; available = true; }
        else if (current == camp && food_stock > 0) { --food_stock; available = true; }
        if (available) person.hunger = std::max(0.0, person.hunger - 45);
        person.decision_timer = 0;
    } else if (person.goal == Goal::Rest) {
        person.energy = std::min(100.0, person.energy + dt * (current == camp ? 2.5 : 0.8));
        if (person.energy >= 96 || person.hunger > 80) person.decision_timer = 0;
    } else if (person.goal == Goal::Forage && person.action_timer >= 2) {
        auto& resource = tile(current.x, current.y);
        if (resource.food > 0 && person.food < 4) { --resource.food; ++person.food; }
        person.action_timer = 0;
        if (resource.food == 0 || person.food >= 4) person.decision_timer = 0;
    }
    // A useful physical transfer: excess carried berries enter the camp stock only on arrival.
    if (current == camp && person.food > 1 && food_stock < 30) {
        int amount = std::min(person.food - 1, 30 - food_stock);
        food_stock += amount; person.food -= amount;
        event(person.name + " shares " + std::to_string(amount) + " food at camp");
    }
}

void World::update(double dt) {
    if (!std::isfinite(dt) || dt <= 0 || dt > 1) return;
    ++ticks;
    for (auto& cell : tiles) {
        if (cell.terrain != Terrain::Bush || cell.food >= 6) continue;
        cell.regrowth += dt;
        if (cell.regrowth >= 80) { ++cell.food; cell.regrowth -= 80; }
    }
    for (auto& person : villagers) {
        if (!person.alive()) continue;
        person.hunger = std::min(100.0, person.hunger + dt * 0.12);
        person.energy = std::max(0.0, person.energy - dt * 0.065);
        if (person.hunger >= 99) person.health = std::max(0.0, person.health - dt * 0.25);
        else person.health = std::min(100.0, person.health + dt * 0.02);
        if (!person.alive()) { event(person.name + " died from hunger"); person.path.clear(); continue; }
        person.decision_timer = std::max(0.0, person.decision_timer - dt);
        if (person.decision_timer <= 0 && person.path.empty()) decide(person);
        execute(person, dt);
    }
}
void World::event(const std::string& message) {
    if (events.size() >= 24) events.pop_front();
    events.push_back("Day " + std::to_string(day()) + ": " + message);
}
double World::daylight() const { return (std::sin(ticks * fixed_step / 240 * 6.283185307 - 1.570796327) + 1) / 2; }
const char* goal_name(Goal goal) {
    switch (goal) {
        case Goal::Explore: return "Exploring";
        case Goal::Forage: return "Gathering food";
        case Goal::Eat: return "Getting a meal";
        case Goal::Rest: return "Resting at camp";
    }
    return "Unknown";
}
} // namespace village
