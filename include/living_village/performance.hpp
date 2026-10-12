#pragma once
#include <array>
#include <string>
namespace village {
class Performance {
public:
    void frame(double work_ms, double total_ms, int simulation_steps);
    std::string report() const;
private:
    std::array<double, 300> work{};
    unsigned frames = 0;
    unsigned long long steps = 0;
    double elapsed = 0, total_work = 0;
};
int battery_percent();
}

namespace village {
class ResourceMeter {
public:
    void sample(double wall_seconds);
    double cpu_percent = -1, ram_mib = -1;
private:
    double last_wall = -1, last_cpu = 0;
};
}
