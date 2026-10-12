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
enum class Goal : int { Explore, Forage, Eat, Rest, GatherWood, CollectStone, DeliverMaterials, TakeMaterials, Build, TendFarm, DeliverFood, StoreMaterials, Socialize, RefuelFire };
struct Tile { Terrain terrain = Terrain::Grass; int food = 0; double regrowth = 0; int wood = 0, stone = 0; };
struct Point { int x = 0, y = 0; bool operator==(const Point& b) const { return x == b.x && y == b.y; } };
constexpr Point camp{32, 32};
enum class BuildingKind : int { Shelter, FoodStorage, Stockpile, Farm, Campfire };
struct Building {
    int id = 0;
    BuildingKind kind = BuildingKind::Shelter;
    Point position;
    bool complete = false;
    int wood_delivered = 0, stone_delivered = 0, initiator = -1;
    double work = 0, growth = 0, fuel = 0;
    int food = 0;
    std::uint32_t contributors = 0;
    int wood_needed() const;
    int stone_needed() const;
    bool supplied() const;
};
struct BuildingCost { int wood, stone; double work; };
BuildingCost building_cost(BuildingKind kind);
const char* building_name(BuildingKind kind);
struct Villager {
    int id = 0;
    std::string name;
    double x = 32, y = 32;
    int age = 22;
    double hunger = 15, energy = 90, health = 100, curiosity = 0.5;
    int food = 0;
    int wood = 0, stone = 0;
    double cooperation = 0.5, loneliness = 10;
    double decision_score = 0;
    double gather_skill = 0, build_skill = 0, farm_skill = 0;
    std::array<double, 20> relationships{};
    std::deque<std::string> history;
    Goal goal = Goal::Explore;
    int target_id = -1;
    Point destination{32, 32};
    double decision_timer = 0, action_timer = 0;
    std::vector<Point> path;
    std::vector<Point> known_food;
    std::vector<Point> known_wood, known_stone;
    std::string reason = "Looking for nearby resources";
    bool alive() const { return health > 0; }
};

class World {
public:
    explicit World(std::uint32_t seed = 20261011);
    std::uint32_t seed;
    std::uint64_t ticks = 0;
    int food_stock = 18;
    int wood_stock = 0, stone_stock = 0;
    int simulation_speed = 1;
    bool auto_develop = true, food_shortage = false;
    std::array<Tile, map_size * map_size> tiles{};
    std::vector<Villager> villagers;
    std::vector<Building> buildings;
    std::vector<Point> gather_orders;
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
    int food_capacity() const;
    int material_capacity() const;
    int shelter_capacity() const;
    int alive_count() const;
    int building_at(Point point) const;
    bool can_plan(Point point) const;
    bool plan(BuildingKind kind, Point point, int initiator = -1);
    bool designate(Point point);
    int season() const { return ((day() - 1) / 10) % 4; }
    bool raining() const { return (seed + static_cast<unsigned>(day()) * 17) % 7 < 2; }
    const char* season_name() const;
    std::string serialize() const;
    static World deserialize(const std::string& data);
private:
    void decide(Villager& person);
    void execute(Villager& person, double dt);
    void discover(Villager& person);
    void update_needs();
    void remember(Villager& person, const std::string& experience);
    double growing_rate() const;
    int random(int maximum);
};
const char* goal_name(Goal goal);
} // namespace village
