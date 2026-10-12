#include "living_village/workers.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
using namespace village;
int main() {
    configure_workers(4);
    if (worker_count() != 4) throw std::runtime_error("Four-thread pool unavailable");
    std::array<unsigned, 4096> visits{};
    for (int run = 0; run < 50; ++run) parallel_tiles(visits.size(), [&](std::size_t begin, std::size_t end) {
        for (auto i = begin; i < end; ++i) ++visits[i];
    });
    for (auto count : visits) if (count != 50) throw std::runtime_error("Chunk duplicated or omitted");
    bool caught = false;
    try { parallel_tiles(4096, [](std::size_t begin, std::size_t) { if (begin != 0) throw std::runtime_error("Worker failure"); }); }
    catch (const std::runtime_error&) { caught = true; }
    if (!caught) throw std::runtime_error("Worker failure lost");
    parallel_tiles(4096, [](std::size_t, std::size_t) {});
    configure_workers(1);
    if (worker_count() != 1) throw std::runtime_error("Pool shutdown failed");
    std::cout << "Worker partition, reuse, exception recovery and shutdown passed\n";
}
