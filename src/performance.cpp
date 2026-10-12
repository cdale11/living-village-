#include "living_village/performance.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/resource.h>
namespace village {
void Performance::frame(double work_ms, double total_ms, int simulation_steps) {
    if (!std::isfinite(work_ms) || !std::isfinite(total_ms) || work_ms < 0 || total_ms < 0) return;
    work[frames % work.size()] = work_ms; ++frames; elapsed += total_ms; total_work += work_ms;
    steps += static_cast<unsigned>(std::max(0, simulation_steps));
}
std::string Performance::report() const {
    auto samples = work; const auto count = std::min<std::size_t>(frames, samples.size());
    std::sort(samples.begin(), samples.begin() + count);
    const double p95 = count ? samples[(count - 1) * 95 / 100] : 0;
    struct rusage usage{}; getrusage(RUSAGE_SELF, &usage);
    std::ostringstream out; out << std::fixed << std::setprecision(2) << "Performance: frames=" << frames
        << " fps=" << (elapsed > 0 ? frames * 1000 / elapsed : 0) << " mean_work_ms=" << (frames ? total_work / frames : 0)
        << " recent_p95_work_ms=" << p95 << " simulation_steps=" << steps << " peak_rss_kib=" << usage.ru_maxrss;
    return out.str();
}
int battery_percent() {
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator("/sys/class/power_supply", error)) {
        std::ifstream type(entry.path() / "type"); std::string name; type >> name;
        if (name != "Battery") continue;
        std::ifstream capacity(entry.path() / "capacity"); int value = -1; capacity >> value;
        if (!capacity.fail() && value >= 0 && value <= 100) return value;
    }
    return -1;
}
}
