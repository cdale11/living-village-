#pragma once
#include "world.hpp"
#include <filesystem>
namespace village {
void save_atomic(const World& world, const std::filesystem::path& filename);
World load_save(const std::filesystem::path& filename, bool* used_backup = nullptr);
void export_history(const World& world, const std::filesystem::path& directory);
}
