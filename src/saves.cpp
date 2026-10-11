#include "living_village/saves.hpp"
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
    out << std::setprecision(17) << "LIVING_VILLAGE 1\n" << seed << ' ' << ticks << ' ' << food_stock << '\n';
    out << rng << '\n';
    for (const auto& cell : tiles) out << static_cast<int>(cell.terrain) << ' ' << cell.food << ' ' << cell.regrowth << '\n';
    out << villagers.size() << '\n';
    for (const auto& person : villagers) {
        out << person.id << ' ' << std::quoted(person.name) << ' ' << person.x << ' ' << person.y << ' ' << person.age << ' '
            << person.hunger << ' ' << person.energy << ' ' << person.health << ' ' << person.curiosity << ' ' << person.food << ' '
            << static_cast<int>(person.goal) << ' ' << person.decision_timer << ' ' << person.action_timer << ' ' << std::quoted(person.reason) << '\n';
        out << person.path.size() << '\n';
        for (Point p : person.path) out << p.x << ' ' << p.y << '\n';
        out << person.known_food.size() << '\n';
        for (Point p : person.known_food) out << p.x << ' ' << p.y << '\n';
    }
    out << events.size() << '\n';
    for (const auto& message : events) out << std::quoted(message) << '\n';
    const std::string body = out.str();
    return body + "CHECKSUM " + std::to_string(checksum(body)) + "\n";
}

World World::deserialize(const std::string& data) {
    require(data.size() <= 2 * 1024 * 1024);
    const auto footer = data.rfind("CHECKSUM ");
    require(footer != std::string::npos);
    std::istringstream trailer(data.substr(footer + 9));
    std::uint64_t expected = 0; trailer >> expected; require(!trailer.fail());
    trailer >> std::ws; require(trailer.eof());
    require(checksum(data.substr(0, footer)) == expected);
    std::istringstream in(data.substr(0, footer));
    std::string magic; int version = 0;
    in >> magic >> version; require(magic == "LIVING_VILLAGE" && version == 1);
    std::uint32_t seed = 0; in >> seed;
    World world(seed);
    in >> world.ticks >> world.food_stock >> world.rng;
    require(world.ticks < 100000000000ULL && world.food_stock >= 0 && world.food_stock <= 100000);
    for (auto& cell : world.tiles) {
        int terrain = 0;
        in >> terrain >> cell.food >> cell.regrowth;
        require(terrain >= 0 && terrain <= 4 && cell.food >= 0 && cell.food <= 6 && range(cell.regrowth, 80));
        cell.terrain = static_cast<Terrain>(terrain);
        require(cell.terrain == Terrain::Bush || cell.food == 0);
    }
    int count = 0; in >> count; require(count > 0 && count <= 20);
    world.villagers.clear();
    for (int index = 0; index < count; ++index) {
        Villager p; int goal = 0;
        in >> p.id >> std::quoted(p.name) >> p.x >> p.y >> p.age >> p.hunger >> p.energy >> p.health >> p.curiosity
           >> p.food >> goal >> p.decision_timer >> p.action_timer >> std::quoted(p.reason);
        require(p.id == index && !p.name.empty() && p.name.size() <= 32 && p.reason.size() <= 200);
        require(range(p.x, 63) && range(p.y, 63) && p.age >= 0 && p.age < 150 && range(p.hunger, 100)
                && range(p.energy, 100) && range(p.health, 100) && range(p.curiosity, 1));
        require(p.food >= 0 && p.food <= 4 && goal >= 0 && goal <= 3 && std::isfinite(p.decision_timer)
                && p.decision_timer >= -1 && p.decision_timer <= 10 && range(p.action_timer, 10000000));
        p.goal = static_cast<Goal>(goal);
        int path_count = 0; in >> path_count; require(path_count >= 0 && path_count <= 4096);
        for (int n = 0; n < path_count; ++n) {
            auto point = read_point(in); require(world.passable(point.x, point.y));
            p.path.push_back(point);
        }
        int memory_count = 0; in >> memory_count; require(memory_count >= 0 && memory_count <= 48);
        for (int n = 0; n < memory_count; ++n) p.known_food.push_back(read_point(in));
        world.villagers.push_back(p);
    }
    int event_count = 0; in >> event_count; require(event_count >= 0 && event_count <= 24);
    world.events.clear();
    for (int n = 0; n < event_count; ++n) {
        std::string message; in >> std::quoted(message); require(message.size() <= 240);
        world.events.push_back(message);
    }
    require(!in.fail()); in >> std::ws; require(in.eof());
    return world;
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
