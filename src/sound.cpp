#include "living_village/feedback.hpp"
#include <algorithm>
#include <cmath>
namespace village {
std::vector<std::int16_t> synthesize(Cue cue, int volume, int rate) {
    if (rate < 8000 || rate > 48000) return {};
    volume = std::clamp(volume, 0, 100);
    const double duration = cue == Cue::Confirm ? .075 : cue >= Cue::Birds ? .55 : .25;
    std::vector<std::int16_t> samples(static_cast<std::size_t>(rate * duration));
    double phase = 0;
    for (std::size_t i = 0; i < samples.size(); ++i) {
        const double time = static_cast<double>(i) / rate, part = time / duration;
        double frequency = 600;
        switch (cue) {
            case Cue::Confirm: frequency = 740; break;
            case Cue::Complete: frequency = part < .5 ? 660 : 880; break;
            case Cue::Warning: frequency = part < .5 ? 220 : 165; break;
            case Cue::Birth: frequency = part < .33 ? 523 : part < .66 ? 659 : 784; break;
            case Cue::Birds: frequency = 1800 + 650 * std::sin(time * 43); break;
            case Cue::Rain: frequency = 70 + static_cast<double>((i * 1103515245U + 12345U) % 1700); break;
            case Cue::Fire: frequency = 90 + static_cast<double>((i * 1664525U + 1013904223U) % 700); break;
            case Cue::Trade: frequency = part < .5 ? 440 : 587; break;
        }
        const double envelope = std::min(1.0, time / .008) * std::min(1.0, (duration - time) / .025);
        phase += frequency * 6.283185307179586 / rate;
        const double wave = std::sin(phase) * (cue >= Cue::Birds ? .22 : 1) * (cue == Cue::Birds ? std::max(0.0, std::sin(time * 31)) : 1);
        samples[i] = static_cast<std::int16_t>(wave * envelope * volume * 100);
    }
    return samples;
}
}
