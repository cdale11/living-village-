#include "living_village/saves.hpp"
#include <algorithm>
#include <cmath>
#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace village {
namespace {
std::uint64_t checksum(const std::string& text) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char c : text) { hash ^= c; hash *= 1099511628211ULL; }
    return hash;
}
void require(bool valid) { if (!valid) throw std::runtime_error("Invalid or unsupported village save"); }
bool range(double value, double maximum) { return std::isfinite(value) && value >= 0 && value <= maximum; }
Point read_point(std::istream& in) { Point p; in >> p.x >> p.y; require(in.good() && p.x >= 0 && p.x < 64 && p.y >= 0 && p.y < 64); return p; }
std::string read_file(const std::filesystem::path& path) {
    require(std::filesystem::file_size(path) <= save_byte_limit);
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open save " + path.string());
    std::ostringstream data; data << in.rdbuf();
    if (in.bad()) throw std::runtime_error("Cannot read save");
    return data.str();
}
void flush_file(const std::filesystem::path& path, const std::string& data) {
    int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0600);
    if (fd < 0) throw std::runtime_error("Cannot write save " + path.string());
    std::size_t offset = 0;
    while (offset < data.size()) {
        ssize_t count = ::write(fd, data.data() + offset, data.size() - offset);
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) { ::close(fd); throw std::runtime_error("Save write failed"); }
        offset += static_cast<std::size_t>(count);
    }
    const int result = ::fsync(fd);
    const int closed = ::close(fd);
    if (result != 0 || closed != 0) throw std::runtime_error("Save flush failed");
}
void write_animal_brain(std::ostream& out, const Brain& b) {
    out << b.active << ' ' << b.action << ' ' << b.updates << ' ' << b.elapsed << ' ' << b.progress << ' '
        << b.start_hunger << ' ' << b.start_energy << ' ' << b.start_health << ' ' << b.start_loneliness << ' '
        << b.last_reward << ' ' << b.last_prediction << ' ' << b.last_error << ' ' << b.selected_base << ' ' << b.selected_adjustment << '\n';
    for (double value : b.weights) out << value << ' ';
    out << '\n'; for (double value : b.pending) out << value << ' ';
    out << '\n' << b.replay_size << ' ' << b.replay_head << ' ' << b.random_state << ' ' << b.episodes << ' ' << b.replay_updates << ' ' << b.experiments << ' ' << b.selected_exploration << '\n';
    for (int n = 0; n < b.replay_size; ++n) {
        out << b.replay[n].reward;
        for (double value : b.replay[n].input) out << ' ' << value;
        out << '\n';
    }
}
void read_animal_brain(std::istream& in, Brain& b) {
    in >> b.active >> b.action >> b.updates >> b.elapsed >> b.progress >> b.start_hunger >> b.start_energy >> b.start_health >> b.start_loneliness
        >> b.last_reward >> b.last_prediction >> b.last_error >> b.selected_base >> b.selected_adjustment;
    for (double& value : b.weights) in >> value;
    for (double& value : b.pending) in >> value;
    in >> b.replay_size >> b.replay_head >> b.random_state >> b.episodes >> b.replay_updates >> b.experiments >> b.selected_exploration;
    require(b.replay_size >= 0 && b.replay_size <= replay_capacity);
    for (int n = 0; n < b.replay_size; ++n) {
        in >> b.replay[n].reward; for (double& value : b.replay[n].input) in >> value;
    }
    require(!in.fail() && b.valid());
}
}

std::string World::serialize() const {
    std::ostringstream out;
    out << std::setprecision(17) << "LIVING_VILLAGE 8\n" << seed << ' ' << ticks << ' ' << food_stock << '\n';
    out << "GENERATION " << generator_version << ' ' << static_cast<int>(biome) << ' ' << static_cast<int>(geography) << ' ' << std::quoted(world_name) << '\n';
    out << wood_stock << ' ' << stone_stock << ' ' << auto_develop << ' ' << food_shortage << ' ' << simulation_speed << '\n';
    out << population_growth << ' ' << next_person_id << ' ' << next_migration_day << ' ' << next_birth_day << ' ' << births << ' ' << arrivals << ' ' << deaths << '\n';
    out << learning_enabled << " NEURAL 2 " << brain_inputs << ' ' << brain_hidden << ' ' << brain_weights << ' ' << replay_capacity << ' ' << work_types << '\n';
    out << volume << ' ' << rumble_enabled << ' ' << knowledge << ' ' << technology_level << ' '
        << trade_count << ' ' << help_count << ' ' << conflicts << ' ' << reconciliations << ' ' << event_sequence << '\n';
    out << rng << '\n';
    for (const auto& cell : tiles) out << static_cast<int>(cell.terrain) << ' ' << cell.food << ' ' << cell.regrowth << ' ' << cell.wood << ' ' << cell.stone
        << ' ' << cell.loose_food << ' ' << cell.loose_wood << ' ' << cell.loose_stone << '\n';
    out << buildings.size() << '\n';
    for (const auto& b : buildings) {
        out << b.id << ' ' << static_cast<int>(b.kind) << ' ' << b.position.x << ' ' << b.position.y << ' ' << b.complete << ' '
            << b.wood_delivered << ' ' << b.stone_delivered << ' ' << b.initiator << ' ' << b.work << ' ' << b.growth << ' '
            << b.fuel << ' ' << b.food << ' ' << std::quoted(b.initiator_name) << ' ' << b.contributors.size();
        for (int id : b.contributors) out << ' ' << id;
        out << ' ' << b.upgrade_wood << ' ' << b.upgrade_stone << ' ' << b.upgrade_work
            << ' ' << b.condition << ' ' << b.repair_wood << ' ' << b.repair_stone << ' ' << b.repair_work << '\n';
    }
    out << gather_orders.size() << '\n';
    for (Point point : gather_orders) out << point.x << ' ' << point.y << '\n';
    out << villagers.size() << '\n';
    auto points = [&](const std::vector<Point>& list) {
        out << list.size() << '\n'; for (Point point : list) out << point.x << ' ' << point.y << '\n';
    };
    for (const auto& p : villagers) {
        out << p.id << ' ' << std::quoted(p.name) << ' ' << p.x << ' ' << p.y << ' ' << p.age << ' '
            << p.hunger << ' ' << p.energy << ' ' << p.health << ' ' << p.curiosity << ' ' << p.food << ' '
            << static_cast<int>(p.goal) << ' ' << p.decision_timer << ' ' << p.action_timer << ' ' << std::quoted(p.reason) << '\n';
        out << p.wood << ' ' << p.stone << ' ' << p.cooperation << ' ' << p.loneliness << ' ' << p.gather_skill << ' ' << p.build_skill << ' ' << p.farm_skill << ' ' << p.decision_score << ' '
            << p.target_id << ' ' << p.destination.x << ' ' << p.destination.y << '\n';
        out << p.age_days << ' ' << p.home << ' ' << p.partner << ' ' << p.parents[0] << ' ' << p.parents[1] << ' '
            << std::quoted(p.parent_names[0]) << ' ' << std::quoted(p.parent_names[1]) << ' ' << p.last_birth_day << ' '
            << p.death_day << ' ' << std::quoted(p.death_reason) << ' ' << p.family_drive << '\n';
        const auto& brain = p.brain;
        require(brain.replay_size >= 0 && brain.replay_size <= replay_capacity);
        out << brain.active << ' ' << brain.action << ' ' << brain.updates << ' ' << brain.elapsed << ' ' << brain.progress << ' '
            << brain.start_hunger << ' ' << brain.start_energy << ' ' << brain.start_health << ' ' << brain.start_loneliness << ' '
            << brain.last_reward << ' ' << brain.last_prediction << ' ' << brain.last_error << ' ' << brain.selected_base << ' ' << brain.selected_adjustment << '\n';
        for (double weight : brain.weights) out << weight << ' ';
        out << '\n';
        for (double value : brain.pending) out << value << ' ';
        out << '\n';
        out << brain.replay_size << ' ' << brain.replay_head << ' ' << brain.random_state << ' ' << brain.episodes << ' '
            << brain.replay_updates << ' ' << brain.experiments << ' ' << brain.selected_exploration << '\n';
        for (int n = 0; n < brain.replay_size; ++n) {
            out << brain.replay[n].reward;
            for (double value : brain.replay[n].input) out << ' ' << value;
            out << '\n';
        }
        out << p.last_conflict_day << ' ' << static_cast<int>(p.occupation);
        for (double value : p.practice) out << ' ' << value;
        out << '\n';
        points(p.path); points(p.known_food); points(p.known_wood); points(p.known_stone);
        out << p.relationships.values.size() << '\n';
        for (const auto& relationship : p.relationships.values) out << relationship.first << ' ' << relationship.second << '\n';
        out << p.history.size() << '\n'; for (const auto& line : p.history) out << std::quoted(line) << '\n';
    }
    out << events.size() << '\n'; for (const auto& message : events) out << std::quoted(message) << '\n';
    out << chronicle.size() << '\n';
    for (const auto& record : chronicle) out << record.tick << ' ' << std::quoted(record.message) << '\n';
    out << statistics.size() << '\n';
    for (const auto& r : statistics) out << r.day << ' ' << r.people << ' ' << r.children << ' ' << r.food << ' ' << r.wood << ' ' << r.stone << ' ' << r.buildings << ' ' << r.technology << ' ' << r.births << ' ' << r.deaths << ' ' << r.trades << '\n';
    out << "ECOLOGY 1 " << ecology_enabled << ' ' << static_cast<int>(weather) << ' ' << temperature << ' ' << precipitation << ' ' << wind << ' ' << dry_days << ' ' << next_animal_id << ' ' << flood_alert << '\n';
    out << ecology.animal_births << ' ' << ecology.animal_deaths << ' ' << ecology.predations << ' ' << ecology.trees_born << ' ' << ecology.plants_died << ' ' << ecology.fires_started << ' ' << ecology.repairs
        << ' ' << ecology.rain_input << ' ' << ecology.evaporation << ' ' << ecology.runoff_out << ' ' << ecology.water_used << '\n' << ecology_rng << '\n';
    for (const auto& t : tiles) out << t.elevation << ' ' << t.moisture << ' ' << t.surface_water << ' ' << t.groundwater << ' ' << t.fertility << ' ' << t.organic << ' '
        << t.compaction << ' ' << t.erosion << ' ' << t.biomass << ' ' << t.plant_age << ' ' << t.plant_health << ' ' << t.disease << ' ' << t.seed_bank << ' ' << t.pollination << ' ' << t.fire << ' ' << t.bush_seeds << ' ' << t.nectar << ' ' << t.snowpack << '\n';
    out << animals.size() << '\n';
    for (const auto& a : animals) {
        out << a.id << ' ' << a.parent << ' ' << static_cast<int>(a.species) << ' ' << a.female << ' ' << a.x << ' ' << a.y << ' ' << a.age << ' ' << a.hunger << ' ' << a.thirst << ' ' << a.energy << ' ' << a.health << ' '
            << a.mass << ' ' << a.metabolism << ' ' << a.immunity << ' ' << a.fertility << ' ' << a.disease << ' ' << a.gestation << ' ' << a.mate_metabolism << ' ' << a.mate_immunity << ' ' << a.mate_fertility << ' '
            << a.body_temperature << ' ' << a.decision_timer << ' ' << a.action_timer << ' ' << a.last_birth_day << ' ' << a.target_id << ' ' << a.destination.x << ' ' << a.destination.y << ' ' << static_cast<int>(a.action) << ' ' << std::quoted(a.reason) << '\n';
        write_animal_brain(out, a.brain);
    }
    const auto& d = director;
    out << "DIRECTOR 1 " << static_cast<int>(d.mode) << ' ' << d.action << ' ' << d.bias << ' ' << d.decisions << ' ' << d.interventions << ' ' << d.pending_tick << ' ' << d.baseline << ' ' << d.expected << ' ' << d.outcome << ' ' << d.baseline_population << ' ' << d.pending << ' ' << std::quoted(d.reason) << '\n';
    write_animal_brain(out, d.brain);
    const std::string body = out.str(); return body + "CHECKSUM " + std::to_string(checksum(body)) + "\n";
}

World World::deserialize(const std::string& data) {
    require(data.size() <= save_byte_limit);
    const auto footer = data.rfind("CHECKSUM "); require(footer != std::string::npos);
    std::istringstream trailer(data.substr(footer + 9)); std::uint64_t expected = 0;
    trailer >> expected; require(!trailer.fail()); trailer >> std::ws; require(trailer.eof());
    require(checksum(data.substr(0, footer)) == expected);
    std::istringstream in(data.substr(0, footer)); std::string magic; int version = 0;
    in >> magic >> version; require(magic == "LIVING_VILLAGE" && version >= 1 && version <= 8);
    std::uint32_t seed = 0; in >> seed; World world(seed); world.chronicle.clear(); world.statistics.clear(); world.event_sequence = 0;
    in >> world.ticks >> world.food_stock;
    if (version >= 6) {
        std::string label; int biome = 0, geography = 0;
        in >> label >> world.generator_version >> biome >> geography >> std::quoted(world.world_name);
        require(label == "GENERATION" && world.generator_version == 1 && biome >= 0 && biome < biome_count && geography >= 0 && geography < geography_count
            && valid_world_name(world.world_name));
        world.biome = static_cast<Biome>(biome); world.geography = static_cast<Geography>(geography);
    }
    if (version >= 2) in >> world.wood_stock >> world.stone_stock >> world.auto_develop >> world.food_shortage >> world.simulation_speed;
    require(world.simulation_speed == 1 || world.simulation_speed == 2 || world.simulation_speed == 4);
    if (version >= 3) in >> world.population_growth >> world.next_person_id >> world.next_migration_day >> world.next_birth_day >> world.births >> world.arrivals >> world.deaths;
    require(world.next_person_id >= 1 && world.next_person_id <= 1000000000 && world.next_migration_day >= 0 && world.next_migration_day <= 100000000 && world.next_birth_day >= 0 && world.next_birth_day <= 100000000
        && world.births <= 1000000000ULL && world.arrivals <= 1000000000ULL && world.deaths <= 1000000000ULL);
    if (version >= 4) {
        std::string neural; int schema = 0, inputs = 0, hidden = 0, weights = 0;
        in >> world.learning_enabled >> neural >> schema >> inputs >> hidden >> weights;
        require(neural == "NEURAL" && schema == (version >= 5 ? 2 : 1) && inputs == brain_inputs && hidden == brain_hidden && weights == brain_weights);
        if (version >= 5) { int replay = 0, work = 0; in >> replay >> work; require(replay == replay_capacity && work == work_types); }
    }
    if (version >= 5) {
        in >> world.volume >> world.rumble_enabled >> world.knowledge >> world.technology_level >> world.trade_count >> world.help_count >> world.conflicts >> world.reconciliations >> world.event_sequence;
        require(world.volume >= 0 && world.volume <= 100 && range(world.knowledge, 30) && world.technology_level >= 0 && world.technology_level <= 2
            && world.trade_count <= 100000000000ULL && world.help_count <= 100000000000ULL && world.conflicts <= 100000000000ULL && world.reconciliations <= 100000000000ULL && world.event_sequence <= 100000000000ULL);
    }
    in >> world.rng;
    require(world.ticks < 100000000000ULL && world.food_stock >= 0 && world.food_stock <= 3000 && world.wood_stock >= 0 && world.wood_stock <= 3000 && world.stone_stock >= 0 && world.stone_stock <= 3000);
    for (auto& cell : world.tiles) {
        int terrain = 0; in >> terrain >> cell.food >> cell.regrowth;
        require(terrain >= 0 && terrain <= 4 && cell.food >= 0 && cell.food <= 6 && range(cell.regrowth, terrain == 1 ? 240 : 80)); cell.terrain = static_cast<Terrain>(terrain);
        if (version >= 2) in >> cell.wood >> cell.stone;
        else { cell.wood = cell.terrain == Terrain::Tree ? 6 : 0; cell.stone = cell.terrain == Terrain::Stone ? 8 : 0; }
        if (version >= 3) in >> cell.loose_food >> cell.loose_wood >> cell.loose_stone;
        require(cell.wood >= 0 && cell.wood <= 6 && cell.stone >= 0 && cell.stone <= 8 && cell.loose_food >= 0 && cell.loose_food <= 1000000 && cell.loose_wood >= 0 && cell.loose_wood <= 1000000 && cell.loose_stone >= 0 && cell.loose_stone <= 1000000);
        require((cell.terrain == Terrain::Bush || cell.food == 0) && (cell.terrain == Terrain::Tree || cell.wood == 0) && (cell.terrain == Terrain::Stone || cell.stone == 0));
    }
    if (version >= 2) {
        int count = 0; in >> count; require(count >= 0 && count <= 64); world.buildings.clear();
        for (int index = 0; index < count; ++index) {
            Building b; int kind = 0; in >> b.id >> kind; b.position = read_point(in);
            in >> b.complete >> b.wood_delivered >> b.stone_delivered >> b.initiator >> b.work >> b.growth >> b.fuel >> b.food;
            if (version >= 3) {
                int contributors = 0; in >> std::quoted(b.initiator_name) >> contributors; require(b.initiator_name.size() <= 32 && contributors >= 0 && contributors <= 64);
                for (int n = 0; n < contributors; ++n) { int id = 0; in >> id; require(id >= 0 && id < world.next_person_id && (b.contributors.empty() || b.contributors.back() < id)); b.contributors.push_back(id); }
            } else {
                std::uint32_t mask = 0; in >> mask; require((mask >> 20) == 0);
                for (int id = 0; id < 20; ++id) if (mask & (1U << id)) b.contributors.push_back(id);
            }
            if (version >= 5) in >> b.upgrade_wood >> b.upgrade_stone >> b.upgrade_work;
            if (version >= 7) {
                in >> b.condition >> b.repair_wood >> b.repair_stone >> b.repair_work;
                require(range(b.condition, 100) && b.repair_wood >= 0 && b.repair_wood <= 1 && b.repair_stone >= 0 && b.repair_stone <= 1 && range(b.repair_work, 8)
                    && (b.complete || (b.repair_wood == 0 && b.repair_stone == 0 && b.repair_work == 0)));
            }
            require(b.id == index && kind >= 0 && kind <= (version >= 5 ? 5 : 4) && b.initiator >= -1 && b.initiator < (version >= 3 ? world.next_person_id : 20));
            b.kind = static_cast<BuildingKind>(kind); auto cost = building_cost(b.kind);
            require(b.wood_delivered >= 0 && b.wood_delivered <= cost.wood && b.stone_delivered >= 0 && b.stone_delivered <= cost.stone && range(b.work, cost.work)
                && range(b.growth, 45) && range(b.fuel, 270) && b.food >= 0 && b.food <= 12);
            require(world.passable(b.position.x, b.position.y) && world.building_at(b.position) < 0 && (b.kind == BuildingKind::Farm || b.food == 0));
            require(b.upgrade_wood >= 0 && b.upgrade_wood <= 6 && b.upgrade_stone >= 0 && b.upgrade_stone <= 4 && range(b.upgrade_work, 30)
                && ((b.kind == BuildingKind::Workshop && b.complete) || (b.upgrade_wood == 0 && b.upgrade_stone == 0 && b.upgrade_work == 0)));
            require(b.complete || b.work == 0 || b.supplied()); world.buildings.push_back(b);
        }
        int orders = 0; in >> orders; require(orders >= 0 && orders <= 16);
        for (int n = 0; n < orders; ++n) {
            Point point = read_point(in); const auto terrain = world.tile(point.x, point.y).terrain;
            require(world.passable(point.x, point.y) && (terrain == Terrain::Tree || terrain == Terrain::Stone || terrain == Terrain::Bush)
                && std::find(world.gather_orders.begin(), world.gather_orders.end(), point) == world.gather_orders.end()); world.gather_orders.push_back(point);
        }
    }
    int count = 0; in >> count; require(count > 0 && count <= (version >= 3 ? population_limit + dead_history_limit : 20)); world.villagers.clear();
    if (version < 3) { world.next_person_id = count; world.next_migration_day = world.day() + 12; world.next_birth_day = world.day() + 8; }
    auto points = [&](std::vector<Point>& list, int maximum, bool walkable) {
        int amount = 0; in >> amount; require(amount >= 0 && amount <= maximum);
        for (int n = 0; n < amount; ++n) { Point point = read_point(in); require(!walkable || world.passable(point.x, point.y)); list.push_back(point); }
    };
    for (int index = 0; index < count; ++index) {
        Villager p; int goal = 0;
        in >> p.id >> std::quoted(p.name) >> p.x >> p.y >> p.age >> p.hunger >> p.energy >> p.health >> p.curiosity >> p.food >> goal >> p.decision_timer >> p.action_timer >> std::quoted(p.reason);
        require(p.id >= 0 && p.id < world.next_person_id && (version >= 3 || p.id == index) && !world.person(p.id) && !p.name.empty() && p.name.size() <= 32 && p.reason.size() <= 200);
        require(range(p.x, 63) && range(p.y, 63) && p.age >= 0 && p.age < 150 && range(p.hunger, 100) && range(p.energy, 100) && range(p.health, 100) && range(p.curiosity, 1));
        require(p.food >= 0 && p.food <= 4 && goal >= 0 && goal <= (version == 1 ? 3 : version == 2 ? 13 : 18) && std::isfinite(p.decision_timer) && p.decision_timer >= -1 && p.decision_timer <= 10 && range(p.action_timer, 10000000));
        p.goal = static_cast<Goal>(goal);
        if (version >= 2) {
            in >> p.wood >> p.stone >> p.cooperation >> p.loneliness >> p.gather_skill >> p.build_skill >> p.farm_skill >> p.decision_score >> p.target_id; p.destination = read_point(in);
            require(p.wood >= 0 && p.stone >= 0 && p.wood + p.stone <= 6 && range(p.cooperation, 1) && range(p.loneliness, 100) && range(p.gather_skill, 1) && range(p.build_skill, 1) && range(p.farm_skill, 1) && range(p.decision_score, 300) && p.target_id >= -1);
            const bool individual = p.goal == Goal::Socialize || p.goal == Goal::Care || p.goal == Goal::CollectMeal;
            require(p.target_id < (individual ? world.next_person_id : static_cast<int>(world.buildings.size())));
        }
        if (version >= 3) {
            in >> p.age_days >> p.home >> p.partner >> p.parents[0] >> p.parents[1] >> std::quoted(p.parent_names[0]) >> std::quoted(p.parent_names[1]) >> p.last_birth_day >> p.death_day >> std::quoted(p.death_reason) >> p.family_drive;
            require(p.age_days >= 0 && p.age_days < days_per_year && p.home >= -1 && p.home < static_cast<int>(world.buildings.size()) && p.partner >= -1 && p.partner < world.next_person_id && p.partner != p.id
                && p.parent_names[0].size() <= 32 && p.parent_names[1].size() <= 32 && p.last_birth_day >= -1000 && p.last_birth_day <= world.day() && p.death_day >= 0 && p.death_day <= world.day() && p.death_reason.size() <= 32 && range(p.family_drive, 1));
            for (int parent : p.parents) require(parent >= -1 && parent < p.id);
            require(p.parents[0] < 0 || p.parents[0] != p.parents[1]);
            if (p.home >= 0) require(world.buildings[p.home].complete && world.buildings[p.home].kind == BuildingKind::Shelter);
            require(p.alive() ? (p.death_day == 0 && p.death_reason.empty()) : (p.death_day > 0 && !p.death_reason.empty() && p.food == 0 && p.wood == 0 && p.stone == 0));
        }
        if (version >= 4) {
            p.brain.initialize(world.seed, p.id);
            auto& brain = p.brain;
            in >> brain.active >> brain.action >> brain.updates >> brain.elapsed >> brain.progress
                >> brain.start_hunger >> brain.start_energy >> brain.start_health >> brain.start_loneliness
                >> brain.last_reward >> brain.last_prediction >> brain.last_error >> brain.selected_base >> brain.selected_adjustment;
            for (auto& weight : brain.weights) in >> weight;
            for (auto& value : brain.pending) in >> value;
            if (version >= 5) {
                in >> brain.replay_size >> brain.replay_head >> brain.random_state >> brain.episodes >> brain.replay_updates >> brain.experiments >> brain.selected_exploration;
                require(brain.replay_size >= 0 && brain.replay_size <= replay_capacity);
                for (int n = 0; n < brain.replay_size; ++n) {
                    in >> brain.replay[n].reward;
                    for (double& value : brain.replay[n].input) in >> value;
                }
                int occupation = 0; in >> p.last_conflict_day >> occupation; require(p.last_conflict_day >= -1000 && p.last_conflict_day <= world.day()); require(occupation >= 0 && occupation <= work_types);
                p.occupation = static_cast<Occupation>(occupation);
                for (double& practice : p.practice) { in >> practice; require(range(practice, 100)); }
            }
            require(!in.fail() && brain.valid() && (!brain.active || (p.alive() && brain.action == goal)));
        } else p.brain.initialize(world.seed, p.id);
        points(p.path, 4096, true); points(p.known_food, 48, false);
        if (version >= 2) {
            points(p.known_wood, 48, false); points(p.known_stone, 48, false);
            if (version >= 3) {
                int relationships = 0; in >> relationships; require(relationships >= 0 && relationships <= 32);
                for (int n = 0; n < relationships; ++n) {
                    int id = 0; double affinity = 0; in >> id >> affinity;
                    require(id >= 0 && id < world.next_person_id && !p.relationships.values.count(id) && std::isfinite(affinity) && affinity >= -100 && affinity <= 100); p.relationships.values[id] = affinity;
                }
            } else for (int id = 0; id < 20; ++id) { double affinity = 0; in >> affinity; require(std::isfinite(affinity) && affinity >= -100 && affinity <= 100); if (affinity != 0 && id < count) p.relationships.values[id] = affinity; }
            int history = 0; in >> history; require(history >= 0 && history <= 6);
            for (int n = 0; n < history; ++n) { std::string line; in >> std::quoted(line); require(line.size() <= 200); p.history.push_back(line); }
        } else p.destination = p.path.empty() ? Point{static_cast<int>(std::round(p.x)), static_cast<int>(std::round(p.y))} : p.path.back();
        world.villagers.push_back(p);
    }
    if (version < 3) {
        for (auto& b : world.buildings) {
            require(b.initiator < count); if (b.initiator >= 0) b.initiator_name = world.person_name(b.initiator);
            for (int id : b.contributors) require(id < count);
        }
        for (auto& p : world.villagers) if (!p.alive()) {
            auto& ground = world.tile(static_cast<int>(std::round(p.x)), static_cast<int>(std::round(p.y)));
            ground.loose_food += p.food; ground.loose_wood += p.wood; ground.loose_stone += p.stone; p.food = p.wood = p.stone = 0;
            p.death_day = world.day(); p.death_reason = "Hunger"; ++world.deaths;
        }
        world.update_homes();
    } else for (const auto& p : world.villagers) if (p.alive() && p.partner >= 0) {
        const auto* partner = world.person(p.partner); require(partner && partner->alive() && partner->partner == p.id && p.age >= 18 && partner->age >= 18 && !world.close_family(p.id, partner->id));
    }
    require(world.alive_count() <= population_limit && static_cast<int>(world.villagers.size()) - world.alive_count() <= dead_history_limit);
    require(world.food_stock <= world.food_capacity() && world.wood_stock <= world.material_capacity() && world.stone_stock <= world.material_capacity());
    int event_count = 0; in >> event_count; require(event_count >= 0 && event_count <= 24); world.events.clear();
    for (int n = 0; n < event_count; ++n) { std::string message; in >> std::quoted(message); require(message.size() <= 240); world.events.push_back(message); }
    if (version >= 5) {
        int records = 0; in >> records; require(records >= 0 && records <= 512);
        for (int n = 0; n < records; ++n) {
            HistoryRecord r; in >> r.tick >> std::quoted(r.message);
            require(r.tick <= world.ticks && r.message.size() <= 240 && (world.chronicle.empty() || world.chronicle.back().tick <= r.tick)); world.chronicle.push_back(r);
        }
        int days = 0; in >> days; require(days >= 0 && days <= 128);
        for (int n = 0; n < days; ++n) {
            DailyRecord r; in >> r.day >> r.people >> r.children >> r.food >> r.wood >> r.stone >> r.buildings >> r.technology >> r.births >> r.deaths >> r.trades;
            require(r.day >= 1 && r.day <= world.day() && r.people >= 0 && r.people <= 20 && r.children >= 0 && r.children <= r.people
                && r.food >= 0 && r.food <= 3000 && r.wood >= 0 && r.wood <= 3000 && r.stone >= 0 && r.stone <= 3000 && r.buildings >= 0 && r.buildings <= 64
                && r.technology >= 0 && r.technology <= world.technology_level && r.births <= world.births && r.deaths <= world.deaths && r.trades <= world.trade_count
                && (world.statistics.empty() || world.statistics.back().day < r.day)); world.statistics.push_back(r);
        }
    } else {
        world.record_day();
        for (const auto& message : world.events) world.chronicle.push_back({world.ticks, "Imported log: " + message.substr(0, 226)});
        world.event_sequence = world.chronicle.size();
    }
    if (version >= 7) {
        std::string label; int schema = 0, weather = 0;
        in >> label >> schema >> world.ecology_enabled >> weather >> world.temperature >> world.precipitation >> world.wind >> world.dry_days >> world.next_animal_id >> world.flood_alert;
        require(label == "ECOLOGY" && schema == 1 && weather >= 0 && weather <= 4 && std::isfinite(world.temperature) && world.temperature >= -20 && world.temperature <= 50
            && range(world.precipitation, .01) && range(world.wind, 1) && world.dry_days >= 0 && world.dry_days <= 1000 && world.next_animal_id >= 0 && world.next_animal_id <= 1000000000);
        world.weather = static_cast<Weather>(weather); auto& e = world.ecology;
        in >> e.animal_births >> e.animal_deaths >> e.predations >> e.trees_born >> e.plants_died >> e.fires_started >> e.repairs >> e.rain_input >> e.evaporation >> e.runoff_out >> e.water_used >> world.ecology_rng;
        require(e.animal_births <= 100000000000ULL && e.animal_deaths <= 100000000000ULL && e.predations <= e.animal_deaths && e.trees_born <= 100000000000ULL
            && e.plants_died <= 100000000000ULL && e.fires_started <= 100000000000ULL && e.repairs <= 100000000000ULL && range(e.rain_input, 1e12) && range(e.evaporation, 1e12) && range(e.runoff_out, 1e12) && range(e.water_used, 1e12));
        for (auto& t : world.tiles) {
            in >> t.elevation >> t.moisture >> t.surface_water >> t.groundwater >> t.fertility >> t.organic >> t.compaction >> t.erosion >> t.biomass >> t.plant_age >> t.plant_health >> t.disease >> t.seed_bank >> t.pollination >> t.fire >> t.bush_seeds >> t.nectar >> t.snowpack;
            require(range(t.elevation, 2) && range(t.moisture, 1) && range(t.surface_water, 4) && range(t.groundwater, 1) && range(t.fertility, 1) && range(t.organic, 1)
                && range(t.compaction, 1) && range(t.erosion, 1) && range(t.biomass, 1) && range(t.plant_age, 100000) && range(t.plant_health, 100) && range(t.disease, 1)
                && range(t.seed_bank, 1) && range(t.pollination, 1) && range(t.fire, 1) && range(t.bush_seeds, 1) && range(t.nectar, 1) && range(t.snowpack, 4));
        }
        int count = 0; in >> count; require(count >= 0 && count <= animal_limit); world.animals.clear();
        for (int n = 0; n < count; ++n) {
            Animal a; int species = 0, action = 0;
            in >> a.id >> a.parent >> species >> a.female >> a.x >> a.y >> a.age >> a.hunger >> a.thirst >> a.energy >> a.health >> a.mass >> a.metabolism >> a.immunity >> a.fertility >> a.disease
                >> a.gestation >> a.mate_metabolism >> a.mate_immunity >> a.mate_fertility >> a.body_temperature >> a.decision_timer >> a.action_timer >> a.last_birth_day >> a.target_id;
            a.destination = read_point(in); in >> action >> std::quoted(a.reason);
            require(a.id >= 0 && a.id < world.next_animal_id && (world.animals.empty() || world.animals.back().id < a.id) && a.parent >= -1 && a.parent < a.id && species >= 0 && species <= 3 && action >= 0 && action <= 6
                && range(a.x, 63) && range(a.y, 63) && range(a.age, 100000) && range(a.hunger, 100) && range(a.thirst, 100) && range(a.energy, 100) && a.health > 0 && range(a.health, 100)
                && range(a.mass, 1) && a.metabolism >= .7 && range(a.metabolism, 1.3) && range(a.immunity, 1) && range(a.fertility, 1) && range(a.disease, 1) && range(a.gestation, 18)
                && a.mate_metabolism >= .7 && range(a.mate_metabolism, 1.3) && range(a.mate_immunity, 1) && range(a.mate_fertility, 1) && a.body_temperature >= 20 && range(a.body_temperature, 45)
                && range(a.decision_timer, 11) && range(a.action_timer, 10000000) && a.last_birth_day >= -1000 && a.last_birth_day <= world.day() && a.target_id >= -1 && a.target_id < world.next_animal_id && a.reason.size() <= 200);
            a.species = static_cast<Species>(species); a.action = static_cast<AnimalAction>(action);
            read_animal_brain(in, a.brain); require(!a.brain.active || a.brain.action == action);
            world.animals.push_back(a);
        }
    } else world.initialize_ecology();
    if (version >= 8) {
        auto& d = world.director; std::string label; int schema = 0, mode = 0;
        in >> label >> schema >> mode >> d.action >> d.bias >> d.decisions >> d.interventions >> d.pending_tick >> d.baseline >> d.expected >> d.outcome >> d.baseline_population >> d.pending >> std::quoted(d.reason);
        require(label == "DIRECTOR" && schema == 1 && mode >= 0 && mode <= 2 && d.action >= 0 && d.action <= 2 && (d.bias == 0 || d.bias == 8 || d.bias == -8)
            && d.pending_tick <= world.ticks && range(d.baseline, 1) && std::isfinite(d.expected) && std::abs(d.expected) <= 1 && std::isfinite(d.outcome) && std::abs(d.outcome) <= 1
            && d.baseline_population >= 0 && d.baseline_population <= population_limit && d.reason.size() <= 200 && d.interventions <= d.decisions && d.decisions <= 100000000000ULL);
        d.mode = static_cast<DirectorMode>(mode); read_animal_brain(in, d.brain);
        require(d.pending == d.brain.active && (!d.pending || (mode == 2 && d.brain.action == d.action)) && (mode == 2 || d.bias == 0));
    }
    require(!in.fail()); in >> std::ws; require(in.eof()); return world;
}

void write_atomic_text(const std::filesystem::path& filename, const std::string& text) {
    auto directory = filename.parent_path(); if (directory.empty()) directory = ".";
    std::filesystem::create_directories(directory);
    flush_file(filename.string() + ".tmp", text); std::filesystem::rename(filename.string() + ".tmp", filename);
    const int fd = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd < 0) throw std::runtime_error("Cannot flush save metadata directory");
    const int result = ::fsync(fd); ::close(fd); if (result != 0) throw std::runtime_error("Save metadata flush failed");
}

void save_atomic(const World& world, const std::filesystem::path& filename) {
    auto directory = filename.parent_path();
    if (directory.empty()) directory = ".";
    std::filesystem::create_directories(directory);
    const auto temporary = filename.string() + ".tmp";
    flush_file(temporary, world.serialize());
    if (std::filesystem::exists(filename)) {
        // Preserve only a valid previous save; a recovered backup must not be replaced with corrupt data.
        std::string old;
        bool valid = false;
        try { old = read_file(filename); World::deserialize(old); valid = true; } catch (const std::exception&) {}
        if (valid) {
            const auto backup = filename.string() + ".bak";
            flush_file(backup + ".tmp", old);
            std::filesystem::rename(backup + ".tmp", backup);
        }
    }
    std::filesystem::rename(temporary, filename);
    int directory_fd = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY);
    if (directory_fd < 0) throw std::runtime_error("Cannot open save directory for flush");
    int result = ::fsync(directory_fd);
    ::close(directory_fd);
    if (result != 0) throw std::runtime_error("Save directory flush failed");
}

void export_history(const World& world, const std::filesystem::path& directory) {
    std::filesystem::create_directories(directory);
    auto quoted = [](const std::string& value) { std::string result = "\""; for (char c : value) { if (c == '\"') result += '\"'; result += c; } return result + '\"'; };
    std::ostringstream events, days, people, habitat, wildlife;
    events << "seed,tick,day,event\n";
    for (const auto& r : world.chronicle) events << world.seed << ',' << r.tick << ',' << 1 + r.tick / 2400 << ',' << quoted(r.message) << '\n';
    days << "seed,day,population,children,food,wood,stone,buildings,technology,births,deaths,trades\n";
    for (const auto& r : world.statistics) days << world.seed << ',' << r.day << ',' << r.people << ',' << r.children << ',' << r.food << ',' << r.wood << ',' << r.stone << ',' << r.buildings << ',' << r.technology << ',' << r.births << ',' << r.deaths << ',' << r.trades << '\n';
    people << "seed,id,name,age,alive,role,home,partner,neural_updates,episodes,replayed,experiments,death_reason\n";
    for (const auto& p : world.villagers) people << world.seed << ',' << p.id << ',' << quoted(p.name) << ',' << p.age << ',' << p.alive() << ',' << quoted(occupation_name(p.occupation)) << ',' << p.home << ',' << p.partner << ',' << p.brain.updates << ',' << p.brain.episodes << ',' << p.brain.replay_updates << ',' << p.brain.experiments << ',' << quoted(p.death_reason) << '\n';
    habitat << "seed,day,x,y,terrain,elevation,soil_water,surface_water,groundwater,snowpack,fertility,organic,compaction,erosion,biomass,plant_age,plant_health,disease,tree_seeds,bush_seeds,nectar,pollination,fire\n";
    for (int y = 0; y < map_size; ++y) for (int x = 0; x < map_size; ++x) {
        const auto& t = world.tile(x, y); habitat << world.seed << ',' << world.day() << ',' << x << ',' << y << ',' << static_cast<int>(t.terrain) << ',' << t.elevation << ',' << t.moisture << ',' << t.surface_water << ',' << t.groundwater << ',' << t.snowpack << ',' << t.fertility << ',' << t.organic << ',' << t.compaction << ',' << t.erosion << ',' << t.biomass << ',' << t.plant_age << ',' << t.plant_health << ',' << t.disease << ',' << t.seed_bank << ',' << t.bush_seeds << ',' << t.nectar << ',' << t.pollination << ',' << t.fire << '\n';
    }
    wildlife << "seed,day,id,parent,species,female,x,y,age,hunger,thirst,energy,health,mass,metabolism,immunity,fertility,disease,gestation,body_temperature,activity,reason,neural_updates,episodes\n";
    for (const auto& a : world.animals) wildlife << world.seed << ',' << world.day() << ',' << a.id << ',' << a.parent << ',' << quoted(species_name(a.species)) << ',' << a.female << ',' << a.x << ',' << a.y << ',' << a.age << ',' << a.hunger << ',' << a.thirst << ',' << a.energy << ',' << a.health << ',' << a.mass << ',' << a.metabolism << ',' << a.immunity << ',' << a.fertility << ',' << a.disease << ',' << a.gestation << ',' << a.body_temperature << ',' << quoted(animal_action_name(a.action)) << ',' << quoted(a.reason) << ',' << a.brain.updates << ',' << a.brain.episodes << '\n';
    const std::pair<const char*, std::string> files[] = {{"village-history.csv", events.str()}, {"village-statistics.csv", days.str()}, {"residents.csv", people.str()}, {"ecology.csv", habitat.str()}, {"wildlife.csv", wildlife.str()}};
    for (const auto& file : files) { const auto path = directory / file.first; flush_file(path.string() + ".tmp", file.second); std::filesystem::rename(path.string() + ".tmp", path); }
    const int fd = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd < 0) throw std::runtime_error("Cannot flush history directory");
    const int result = ::fsync(fd); ::close(fd); if (result != 0) throw std::runtime_error("History export flush failed");
}

World load_save(const std::filesystem::path& filename, bool* used_backup) {
    if (used_backup) *used_backup = false;
    try { return World::deserialize(read_file(filename)); }
    catch (const std::exception&) {
        World backup = World::deserialize(read_file(filename.string() + ".bak"));
        if (used_backup) *used_backup = true;
        return backup;
    }
}
} // namespace village
