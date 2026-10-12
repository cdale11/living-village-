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
