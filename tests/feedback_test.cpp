#include "living_village/feedback.hpp"
#include "living_village/input.hpp"
#include "living_village/performance.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace village;
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
int main() {
    try {
        for (Cue cue : {Cue::Confirm, Cue::Complete, Cue::Warning, Cue::Birth, Cue::Trade}) {
            const auto sound = synthesize(cue, 40), silent = synthesize(cue, 0);
            check(!sound.empty() && sound.size() < 22050 && std::any_of(sound.begin(), sound.end(), [](int value) { return value != 0; }), "Procedural sound is empty or unbounded");
            check(std::all_of(sound.begin(), sound.end(), [](int value) { return value >= -4000 && value <= 4000; }) && std::all_of(silent.begin(), silent.end(), [](int value) { return value == 0; }), "Volume/mute failed");
        }
        SDLApi sdl; check(sdl.Init(SDL_INIT_EVENTS) == 0, "SDL init failed");
        { Feedback audio(sdl); check(audio.available(), "Dummy audio device did not open"); audio.play(Cue::Complete, 40); audio.mute(); Input input(sdl); check(!input.rumble(), "No-controller rumble did not fail gracefully"); }
        sdl.Quit();
        Performance metrics; for (int i = 0; i < 400; ++i) metrics.frame(2, 1000.0 / 30, 1);
        check(metrics.report().find("fps=30.00") != std::string::npos && metrics.report().find("mean_work_ms=2.00") != std::string::npos, "Performance counters are incorrect");
        std::cout << "PASS: original PCM synthesis/volume/mute, SDL dummy audio, graceful unsupported rumble, bounded frame metrics\n";
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
