#pragma once
#include "brain.hpp"
#include <array>
#include <cstdint>
#include <deque>
#include <random>
#include <map>
#include <string>
#include <vector>

namespace village {
constexpr int map_size = 64;
constexpr double fixed_step = 0.1;
constexpr int population_limit = 20, dead_history_limit = 32, days_per_year = 40;
enum class Terrain : int { Grass, Tree, Stone, Water, Bush };
enum class Biome : int { Temperate, Woodland, Meadow, Arid, Taiga, Rainforest, Wetland, Savanna };
enum class Geography : int { Ponds, River, Island, Coast, Highlands, Lakes, Peninsula, Archipelago, Delta, Canyon, Valley, Plains };
constexpr int biome_count = 8, geography_count = 12;
const char* biome_name(Biome biome);
const char* geography_name(Geography geography);
enum class Goal : int { Explore, Forage, Eat, Rest, GatherWood, CollectStone, DeliverMaterials, TakeMaterials, Build, TendFarm, DeliverFood, StoreMaterials, Socialize, RefuelFire, Arrive, Play, Care, CollectMeal, WaitForCare };
struct Tile {
    Terrain terrain = Terrain::Grass; int food = 0; double regrowth = 0;
    int wood = 0, stone = 0; int loose_food = 0, loose_wood = 0, loose_stone = 0;
    double elevation = 0, moisture = .55, surface_water = 0, groundwater = .3;
    double fertility = .7, organic = .5, compaction = 0, erosion = 0;
    double biomass = .7, plant_age = 0, plant_health = 100, disease = 0;
    double seed_bank = 0, pollination = .5, fire = 0, bush_seeds = 0, nectar = .4, snowpack = 0;
};
struct Point { int x = 0, y = 0; bool operator==(const Point& b) const { return x == b.x && y == b.y; } };
constexpr Point camp{32, 32};
enum class BuildingKind : int { Shelter, FoodStorage, Stockpile, Farm, Campfire, Workshop };
enum class Occupation : int { Generalist, Forager, Woodcutter, Stoneworker, Farmer, Builder, Caregiver, Carrier };
const char* occupation_name(Occupation occupation);
constexpr int work_types = 7;
struct Building {
    int id = 0;
    BuildingKind kind = BuildingKind::Shelter;
    Point position;
    bool complete = false;
    int wood_delivered = 0, stone_delivered = 0, initiator = -1;
    double work = 0, growth = 0, fuel = 0;
    int food = 0;
    int upgrade_wood = 0, upgrade_stone = 0;
    double upgrade_work = 0;
    double condition = 100, repair_work = 0;
    int repair_wood = 0, repair_stone = 0;
    std::vector<int> contributors;
    std::string initiator_name;
    void contribute(int person_id);
    int wood_needed() const;
    int stone_needed() const;
    bool supplied() const;
};
struct BuildingCost { int wood, stone; double work; };
BuildingCost building_cost(BuildingKind kind);
BuildingCost technology_cost(int level);
const char* building_name(BuildingKind kind);
struct Relationships {
    std::map<int, double> values;
    double operator[](int id) const;
    double& operator[](int id);
};
struct Villager {
    Brain brain;
    Occupation occupation = Occupation::Generalist;
    std::array<double, work_types> practice{};
    int last_conflict_day = -1000;
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
    Relationships relationships;
    int age_days = 0, home = -1, partner = -1;
    std::array<int, 2> parents{{-1, -1}};
    std::array<std::string, 2> parent_names;
    int last_birth_day = -1000, death_day = 0;
    std::string death_reason;
    double family_drive = 0.5;
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
    bool child() const { return age < 16; }
};

struct HistoryRecord { std::uint64_t tick = 0; std::string message; };
struct DailyRecord {
    int day = 1, people = 0, children = 0, food = 0, wood = 0, stone = 0, buildings = 0, technology = 0;
    std::uint64_t births = 0, deaths = 0, trades = 0;
};
enum class Weather : int { Clear, Rain, Storm, Snow, Heatwave };
enum class Species : int { Rabbit, Deer, Fox, Bee };
enum class AnimalAction : int { Wander, Feed, Drink, Rest, Flee, Hunt, Mate };
constexpr int animal_limit = 48;
const char* species_name(Species species);
const char* animal_action_name(AnimalAction action);
struct Animal {
    int id = 0, parent = -1;
    Species species = Species::Rabbit;
    bool female = false;
    double x = 0, y = 0, age = 0, hunger = 20, thirst = 15, energy = 90, health = 100;
    double mass = .5, metabolism = 1, immunity = .5, fertility = .5, disease = 0;
    double gestation = 0, mate_metabolism = 1, mate_immunity = .5, mate_fertility = .5;
    double body_temperature = 38, decision_timer = 0, action_timer = 0;
    int last_birth_day = -1000, target_id = -1;
    Point destination;
    AnimalAction action = AnimalAction::Wander;
    std::string reason = "Looking for habitat";
    Brain brain;
};
struct EcologyStats {
    std::uint64_t animal_births = 0, animal_deaths = 0, predations = 0;
    std::uint64_t trees_born = 0, plants_died = 0, fires_started = 0, repairs = 0;
    double rain_input = 0, evaporation = 0, runoff_out = 0, water_used = 0;
};
class World {
public:
    explicit World(std::uint32_t seed = 20261011, Biome biome = Biome::Temperate, Geography geography = Geography::Ponds, int starting_population = 6);
    std::uint32_t seed;
    Biome biome = Biome::Temperate;
    Geography geography = Geography::Ponds;
    int generator_version = 1;
    std::string world_name = "Living Village";
    std::uint64_t ticks = 0;
    int food_stock = 18;
    int wood_stock = 0, stone_stock = 0;
    int simulation_speed = 1;
    bool auto_develop = true, food_shortage = false;
    bool population_growth = true;
    bool learning_enabled = true;
    int volume = 40;
    bool rumble_enabled = true;
    double knowledge = 0;
    int technology_level = 0;
    std::uint64_t trade_count = 0, help_count = 0, conflicts = 0, reconciliations = 0, event_sequence = 0;
    std::deque<HistoryRecord> chronicle;
    std::deque<DailyRecord> statistics;
    int next_person_id = 6, next_migration_day = 12, next_birth_day = 8;
    std::uint64_t births = 0, arrivals = 0, deaths = 0;
    // Fixed-size heap storage keeps world copies/save parsing off the small stack.
    std::vector<Tile> tiles = std::vector<Tile>(map_size * map_size);
    std::vector<Villager> villagers;
    std::vector<Building> buildings;
    std::vector<Point> gather_orders;
    std::deque<std::string> events;
    std::mt19937 rng;
    std::mt19937 ecology_rng;
    std::vector<Animal> animals;
    int next_animal_id = 0, dry_days = 0;
    Weather weather = Weather::Clear;
    double temperature = 18, precipitation = 0, wind = .2;
    bool ecology_enabled = true;
    bool flood_alert = false;
    EcologyStats ecology;
    const Tile& tile(int x, int y) const { return tiles.at(y * map_size + x); }
    Tile& tile(int x, int y) { return tiles.at(y * map_size + x); }
    bool passable(int x, int y) const;
    std::vector<Point> route(Point from, Point to) const;
    void update(double dt = fixed_step);
    void recruit_wildlife();
    void event(const std::string& message);
    int day() const { return 1 + static_cast<int>(ticks / 2400); }
    double daylight() const;
    int food_capacity() const;
    int material_capacity() const;
    int shelter_capacity() const;
    int alive_count() const;
    int child_count() const;
    int home_occupancy(int building_id) const;
    Villager* person(int id);
    const Villager* person(int id) const;
    std::string person_name(int id) const;
    bool form_household(int first, int second);
    bool have_child(int first, int second);
    bool welcome_migrant();
    bool supports_newcomer() const;
    bool kill(int id, const std::string& reason);
    std::string growth_status() const;
    int building_at(Point point) const;
    bool can_plan(Point point) const;
    bool plan(BuildingKind kind, Point point, int initiator = -1);
    bool designate(Point point);
    int season() const { return ((day() - 1) / 10) % 4; }
    bool raining() const { return precipitation > 0 && weather != Weather::Snow; }
    const char* weather_name() const;
    double plant_growth(const Tile& cell) const;
    void initialize_ecology(bool wildlife = true);
    void update_ecology(double seconds);
    void update_animals(double seconds);
    void update_weather();
    bool spawn_animal(Species species, Point place, int parent = -1);
    bool ignite(Point place);
    void trample(Point place, double amount = 1);
    int repair_wood_needed(const Building& site) const;
    int repair_stone_needed(const Building& site) const;
    bool repair(Villager& person, Building& site, double dt);
    const char* season_name() const;
    std::string serialize() const;
    static World deserialize(const std::string& data);
    BrainInput observations(const Villager& person, Goal goal, Point destination, double utility) const;
    bool research_available() const;
    int research_wood_needed(const Building& workshop) const;
    int research_stone_needed(const Building& workshop) const;
    bool research(Villager& person, Building& workshop, double dt);
    void record_day();
private:
    void generate_terrain();
    double social_exchange(Villager& person, Villager& other);
    void resource_conflict(Villager& person);
    void learn(Villager& person, double terminal_reward = 0);
    void begin_learning(Villager& person, const BrainInput& input, double base, double adjustment);
    void practice_work(Villager& person, Goal goal, double amount);
    void update_occupation(Villager& person);
    double work_bias(const Villager& person, Goal goal) const;
    void decide(Villager& person);
    void execute(Villager& person, double dt);
    void discover(Villager& person);
    void update_needs();
    void update_homes();
    void update_population();
    void prune_history();
    bool close_family(int first, int second) const;
    std::string new_name(int id) const;
    void remember(Villager& person, const std::string& experience);
    double growing_rate() const;
    int random(int maximum);
    int eco_random(int maximum);
};
const char* goal_name(Goal goal);
} // namespace village
