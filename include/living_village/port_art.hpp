#pragma once
#include <filesystem>
namespace village {
// Registers only LivingVillage.sh; preserves unrelated frontend metadata.
void register_port_art(const std::filesystem::path& ports);
}
