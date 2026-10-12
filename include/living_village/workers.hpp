#pragma once
#include <functional>
#include <cstddef>
namespace village {
// Main thread participates; workers never mutate neighboring tiles or RNGs.
void configure_workers(unsigned total_threads = 0);
unsigned worker_count();
void parallel_tiles(std::size_t count, const std::function<void(std::size_t, std::size_t)>& job);
}
