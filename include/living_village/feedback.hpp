#pragma once
#include "sdl.hpp"
#include <cstdint>
#include <vector>
namespace village {
enum class Cue { Confirm, Complete, Warning, Birth, Trade };
std::vector<std::int16_t> synthesize(Cue cue, int volume, int rate = 22050);
class Feedback {
public:
    explicit Feedback(SDLApi& sdl);
    ~Feedback();
    void play(Cue cue, int volume);
    void mute();
    bool available() const { return device != 0; }
private:
    SDLApi& sdl;
    SDL_AudioDeviceID device = 0;
    bool initialized = false;
    Uint32 last_sound = 0;
};
}
