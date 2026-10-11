#pragma once
#include <array>
#include <cstdint>
#include <deque>
#include <random>
#include <string>
#include <vector>

namespace village {
constexpr int map_size = 64;
constexpr double fixed_step = 0.1;
enum class Terrain : int { Grass, Tree, Stone, Water, Bush };
enum class Goal : int { Explore, Forage, Eat, Rest };
struct Tile { Terrain terrain = Terrain::Grass; int food = 0; double regrowth = 0; };
struct Point { int x = 0, y = 0; bool operator==(const Point& b) const { return x == b.x && y == b.y; } };
struct Villager {
    int id = 0;
    std::string name;
    double x = 32, y = 32;
    int age = 22;
    double hunger = 15, energy = 90, health = 100, curiosity = 0.5;
    int food = 0;
    Goal goal = Goal::Explore;
    double decision_timer = 0, action_timer = 0;
    std::vector<Point> path;
    std::vector<Point> known_food;
    std::string reason = "Looking for nearby resources";
    bool alive() const { return health > 0; }
};

class World {
public:
    explicit World(std::uint32_t seed = 20261011);
    std::uint32_t seed;
    std::uint64_t ticks = 0;
    int food_stock = 18;
    std::array<Tile, map_size * map_size> tiles{};
    std::vector<Villager> villagers;
    std::deque<std::string> events;
    std::mt19937 rng;
    const Tile& tile(int x, int y) const { return tiles.at(y * map_size + x); }
    Tile& tile(int x, int y) { return tiles.at(y * map_size + x); }
    bool passable(int x, int y) const;
    std::vector<Point> route(Point from, Point to) const;
    void update(double dt = fixed_step);
    void event(const std::string& message);
    int day() const { return 1 + static_cast<int>(ticks / 2400); }
    double daylight() const;
    std::string serialize() const;
    static World deserialize(const std::string& data);
private:
    void decide(Villager& person);
    void execute(Villager& person, double dt);
    void discover(Villager& person);
    int random(int maximum);
};
const char* goal_name(Goal goal);
} // namespace village
