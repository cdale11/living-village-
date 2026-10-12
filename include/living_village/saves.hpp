#pragma once
#include "world.hpp"
#include <filesystem>
namespace village {
constexpr std::size_t save_byte_limit = 8 * 1024 * 1024;
void save_atomic(const World& world, const std::filesystem::path& filename);
World load_save(const std::filesystem::path& filename, bool* used_backup = nullptr);
void export_history(const World& world, const std::filesystem::path& directory);
void write_atomic_text(const std::filesystem::path& filename, const std::string& text);
bool valid_world_name(const std::string& name);
struct SaveSlot {
    std::string id, name;
    std::uint32_t seed = 0;
    int day = 1;
    Biome biome = Biome::Temperate;
    Geography geography = Geography::Ponds;
    bool metadata = false, backup_only = false;
};
class SaveCatalog {
public:
    explicit SaveCatalog(std::filesystem::path directory) : root(std::move(directory)) {}
    std::vector<SaveSlot> list() const;
    std::filesystem::path path(const std::string& id) const;
    bool exists(const std::string& id) const;
    std::string active() const;
    void activate(const std::string& id);
    std::string create(const World& world);
    World load(const std::string& id, bool* used_backup = nullptr) const;
    void rename(const std::string& id, const std::string& name);
    void erase(const std::string& id, const std::string& current_id);
    std::filesystem::path export_directory(const std::string& id) const;
private:
    std::filesystem::path root;
    std::filesystem::path deletion_marker(const std::string& id) const;
    void safe(const std::string& id) const;
};
}
