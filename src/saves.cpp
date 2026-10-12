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
    require(std::filesystem::file_size(path) <= 2 * 1024 * 1024);
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open save " + path.string());
    std::ostringstream data; data << in.rdbuf();
    if (in.bad()) throw std::runtime_error("Cannot read save");
    return data.str();
}
void flush_file(const std::filesystem::path& path, const std::string& data) {
    int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
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
}

std::string World::serialize() const {
    std::ostringstream out;
    out << std::setprecision(17) << "LIVING_VILLAGE 3\n" << seed << ' ' << ticks << ' ' << food_stock << '\n';
    out << wood_stock << ' ' << stone_stock << ' ' << auto_develop << ' ' << food_shortage << ' ' << simulation_speed << '\n';
    out << population_growth << ' ' << next_person_id << ' ' << next_migration_day << ' ' << next_birth_day << ' ' << births << ' ' << arrivals << ' ' << deaths << '\n';
    out << rng << '\n';
    for (const auto& cell : tiles) out << static_cast<int>(cell.terrain) << ' ' << cell.food << ' ' << cell.regrowth << ' ' << cell.wood << ' ' << cell.stone
        << ' ' << cell.loose_food << ' ' << cell.loose_wood << ' ' << cell.loose_stone << '\n';
    out << buildings.size() << '\n';
    for (const auto& b : buildings) {
        out << b.id << ' ' << static_cast<int>(b.kind) << ' ' << b.position.x << ' ' << b.position.y << ' ' << b.complete << ' '
            << b.wood_delivered << ' ' << b.stone_delivered << ' ' << b.initiator << ' ' << b.work << ' ' << b.growth << ' '
            << b.fuel << ' ' << b.food << ' ' << std::quoted(b.initiator_name) << ' ' << b.contributors.size();
        for (int id : b.contributors) out << ' ' << id;
        out << '\n';
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
        points(p.path); points(p.known_food); points(p.known_wood); points(p.known_stone);
        out << p.relationships.values.size() << '\n';
        for (const auto& relationship : p.relationships.values) out << relationship.first << ' ' << relationship.second << '\n';
        out << p.history.size() << '\n'; for (const auto& line : p.history) out << std::quoted(line) << '\n';
    }
    out << events.size() << '\n'; for (const auto& message : events) out << std::quoted(message) << '\n';
    const std::string body = out.str(); return body + "CHECKSUM " + std::to_string(checksum(body)) + "\n";
}

World World::deserialize(const std::string& data) {
    require(data.size() <= 2 * 1024 * 1024);
    const auto footer = data.rfind("CHECKSUM "); require(footer != std::string::npos);
    std::istringstream trailer(data.substr(footer + 9)); std::uint64_t expected = 0;
    trailer >> expected; require(!trailer.fail()); trailer >> std::ws; require(trailer.eof());
    require(checksum(data.substr(0, footer)) == expected);
    std::istringstream in(data.substr(0, footer)); std::string magic; int version = 0;
    in >> magic >> version; require(magic == "LIVING_VILLAGE" && version >= 1 && version <= 3);
    std::uint32_t seed = 0; in >> seed; World world(seed);
    in >> world.ticks >> world.food_stock;
    if (version >= 2) in >> world.wood_stock >> world.stone_stock >> world.auto_develop >> world.food_shortage >> world.simulation_speed;
    require(world.simulation_speed == 1 || world.simulation_speed == 2 || world.simulation_speed == 4);
    if (version >= 3) in >> world.population_growth >> world.next_person_id >> world.next_migration_day >> world.next_birth_day >> world.births >> world.arrivals >> world.deaths;
    require(world.next_person_id >= 1 && world.next_person_id <= 1000000000 && world.next_migration_day >= 0 && world.next_migration_day <= 100000000 && world.next_birth_day >= 0 && world.next_birth_day <= 100000000
        && world.births <= 1000000000ULL && world.arrivals <= 1000000000ULL && world.deaths <= 1000000000ULL);
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
            require(b.id == index && kind >= 0 && kind <= 4 && b.initiator >= -1 && b.initiator < (version >= 3 ? world.next_person_id : 20));
            b.kind = static_cast<BuildingKind>(kind); auto cost = building_cost(b.kind);
            require(b.wood_delivered >= 0 && b.wood_delivered <= cost.wood && b.stone_delivered >= 0 && b.stone_delivered <= cost.stone && range(b.work, cost.work)
                && range(b.growth, 45) && range(b.fuel, 270) && b.food >= 0 && b.food <= 12);
            require(world.passable(b.position.x, b.position.y) && world.building_at(b.position) < 0 && (b.kind == BuildingKind::Farm || b.food == 0));
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
    require(!in.fail()); in >> std::ws; require(in.eof()); return world;
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
