#include "living_village/saves.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace village {
namespace {
bool named(const std::string& id) {
    if (id.size() != 12 || id.substr(0, 6) != "world-") return false;
    for (std::size_t i = 6; i < id.size(); ++i) if (id[i] < '0' || id[i] > '9') return false;
    return id != "world-000000";
}
void no_link(const std::filesystem::path& path) {
    if (std::filesystem::is_symlink(std::filesystem::symlink_status(path))) throw std::runtime_error("Save manager refuses a linked file or directory");
}
bool regular(const std::filesystem::path& path) { return std::filesystem::is_regular_file(std::filesystem::symlink_status(path)); }
std::string short_line(std::istream& in) {
    std::string result; char c;
    while (in.get(c) && c != '\n') { if (result.size() >= 512) throw std::runtime_error("Oversized save header"); result += c; }
    return result;
}
SaveSlot describe(const std::filesystem::path& path, const std::string& id) {
    SaveSlot info; info.id = id;
    info.name = id == "default" ? "Original village" : id == "previous" ? "Previous village" : "World " + id.substr(6);
    info.backup_only = !regular(path);
    try {
        std::ifstream in(info.backup_only ? path.string() + ".bak" : path.string());
        std::istringstream header(short_line(in)); std::string magic; int version = 0; header >> magic >> version;
        if (magic != "LIVING_VILLAGE" || version < 1 || version > 7) return info;
        std::istringstream state(short_line(in)); std::uint64_t ticks = 0; int food = 0; state >> info.seed >> ticks >> food;
        if (state.fail() || ticks >= 100000000000ULL) return info;
        info.day = 1 + static_cast<int>(ticks / 2400);
        if (version >= 6) {
            std::istringstream generation(short_line(in)); std::string label, title; int schema = 0, biome = 0, geography = 0;
            generation >> label >> schema >> biome >> geography >> std::quoted(title);
            if (generation.fail() || label != "GENERATION" || schema != 1 || biome < 0 || biome >= biome_count || geography < 0 || geography >= geography_count || !valid_world_name(title)) return info;
            info.biome = static_cast<Biome>(biome); info.geography = static_cast<Geography>(geography); info.name = title;
        }
        info.metadata = true;
    } catch (const std::exception&) {}
    return info;
}
}
bool valid_world_name(const std::string& name) {
    if (name.empty() || name.size() > 24 || name.front() == ' ' || name.back() == ' ') return false;
    for (char c : name) if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == ' ' || c == '-')) return false;
    return true;
}
std::filesystem::path SaveCatalog::path(const std::string& id) const {
    if (id == "default") return root / "village.lvsave";
    if (id == "previous") return root / "previous-village.lvsave";
    if (!named(id)) throw std::runtime_error("Invalid save identifier");
    return root / "worlds" / (id + ".lvsave");
}
std::filesystem::path SaveCatalog::deletion_marker(const std::string& id) const { const auto file = path(id); return file.parent_path() / (".delete-" + id); }
void SaveCatalog::safe(const std::string& id) const {
    const auto file = path(id); if (named(id)) no_link(root / "worlds");
    no_link(file); no_link(file.string() + ".bak"); no_link(file.string() + ".tmp"); no_link(file.string() + ".bak.tmp");
    no_link(deletion_marker(id));
}
bool SaveCatalog::exists(const std::string& id) const {
    safe(id); if (std::filesystem::exists(deletion_marker(id))) return false;
    const auto file = path(id); return regular(file) || regular(file.string() + ".bak");
}
std::vector<SaveSlot> SaveCatalog::list() const {
    std::vector<SaveSlot> result;
    for (const auto& id : {std::string("default"), std::string("previous")}) if (exists(id)) result.push_back(describe(path(id), id));
    no_link(root / "worlds");
    if (!std::filesystem::exists(root / "worlds")) return result;
    std::vector<std::string> ids; int inspected = 0;
    for (const auto& entry : std::filesystem::directory_iterator(root / "worlds")) {
        if (++inspected > 4096) throw std::runtime_error("Too many files in the worlds folder");
        const auto filename = entry.path().filename().string(); const auto id = filename.substr(0, std::min<std::size_t>(12, filename.size()));
        if (!named(id) || (filename != id + ".lvsave" && filename != id + ".lvsave.bak")) continue;
        if (std::find(ids.begin(), ids.end(), id) == ids.end() && exists(id)) ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end()); if (ids.size() > 32) ids.resize(32);
    for (const auto& id : ids) result.push_back(describe(path(id), id));
    return result;
}
std::string SaveCatalog::active() const {
    no_link(root / "active-world.txt"); std::ifstream in(root / "active-world.txt");
    std::string id; try { id = short_line(in); } catch (const std::exception&) { return {}; }
    if ((id == "default" || id == "previous" || named(id)) && exists(id)) return id;
    return {};
}
void SaveCatalog::activate(const std::string& id) {
    if (!exists(id)) throw std::runtime_error("Save no longer exists");
    no_link(root / "active-world.txt"); write_atomic_text(root / "active-world.txt", id + "\n");
}
std::string SaveCatalog::create(const World& world) {
    if (!valid_world_name(world.world_name)) throw std::runtime_error("Use letters, numbers, spaces or hyphens in the world name");
    const auto slots = list();
    if (std::count_if(slots.begin(), slots.end(), [](const SaveSlot& slot) { return named(slot.id); }) >= 32) throw std::runtime_error("32 worlds already saved - delete an inactive world first");
    no_link(root / "next-world.txt");
    int next = 1; std::ifstream counter(root / "next-world.txt"); std::string digits;
    if (counter) {
        digits = short_line(counter);
        if (digits.empty() || digits.size() > 7 || digits.find_first_not_of("0123456789") != std::string::npos) throw std::runtime_error("Invalid next-world counter");
        next = std::stoi(digits); if (next < 1 || next > 1000000) throw std::runtime_error("Invalid next-world counter");
    }
    for (int number = next; number <= 999999; ++number) {
        std::ostringstream id; id << "world-" << std::setw(6) << std::setfill('0') << number;
        const auto key = id.str(); safe(key);
        if (std::filesystem::exists(path(key)) || std::filesystem::exists(path(key).string() + ".bak") || std::filesystem::exists(deletion_marker(key)) || std::filesystem::exists(root / "exports" / key)) continue;
        write_atomic_text(root / "next-world.txt", std::to_string(number + 1) + "\n");
        save_atomic(world, path(key)); return key;
    }
    throw std::runtime_error("No world identifiers remain");
}
World SaveCatalog::load(const std::string& id, bool* backup) const { if (!exists(id)) throw std::runtime_error("Save no longer exists"); return load_save(path(id), backup); }
void SaveCatalog::rename(const std::string& id, const std::string& name) {
    if (!valid_world_name(name)) throw std::runtime_error("Invalid world name");
    World world = load(id); world.world_name = name; save_atomic(world, path(id));
}
void SaveCatalog::erase(const std::string& id, const std::string& current_id) {
    if (id == current_id) throw std::runtime_error("Switch worlds before deleting the open village");
    safe(id); const auto file = path(id), marker = deletion_marker(id);
    // Durable intent hides interrupted deletions even if a backup survives a crash.
    write_atomic_text(marker, "DELETE " + id + "\n");
    for (const std::string suffix : {"", ".bak", ".tmp", ".bak.tmp"}) std::filesystem::remove(file.string() + suffix);
    no_link(root / "active-world.txt");
    std::ifstream active_file(root / "active-world.txt"); std::string selected; std::getline(active_file, selected); active_file.close();
    if (selected == id) std::filesystem::remove(root / "active-world.txt");
    write_atomic_text(marker, "DELETED " + id + "\n"); // Flush deletions before clearing their intent.
    std::filesystem::remove(marker);
}
std::filesystem::path SaveCatalog::export_directory(const std::string& id) const {
    if (id != "temporary") path(id); // Validate identifier; never derive paths from display names.
    return root / "exports" / id;
}
} // namespace village
