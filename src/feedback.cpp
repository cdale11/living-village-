#include "living_village/feedback.hpp"
#include <iostream>
namespace village {
Feedback::Feedback(SDLApi& api) : sdl(api) {
    if (!sdl.InitSubSystem || !sdl.QuitSubSystem || !sdl.OpenAudioDevice || !sdl.CloseAudioDevice || !sdl.PauseAudioDevice || !sdl.QueueAudio || !sdl.ClearQueuedAudio || !sdl.GetQueuedAudioSize) return;
    if (sdl.InitSubSystem(SDL_INIT_AUDIO) != 0) { std::cout << "Audio unavailable: " << sdl.GetError() << '\n'; return; }
    initialized = true;
    SDL_AudioSpec desired{}, actual{}; desired.freq = 22050; desired.format = AUDIO_S16SYS; desired.channels = 1; desired.samples = 512;
    device = sdl.OpenAudioDevice(nullptr, 0, &desired, &actual, 0);
    if (!device) { std::cout << "Audio device unavailable: " << sdl.GetError() << '\n'; return; }
    sdl.PauseAudioDevice(device, 0);
    std::cout << "Audio: " << (sdl.GetCurrentAudioDriver ? sdl.GetCurrentAudioDriver() : "SDL") << " 22050 Hz mono\n";
}
Feedback::~Feedback() { if (device) sdl.CloseAudioDevice(device); if (initialized) sdl.QuitSubSystem(SDL_INIT_AUDIO); }
void Feedback::mute() { if (device) sdl.ClearQueuedAudio(device); }
void Feedback::play(Cue cue, int volume) {
    if (!device || volume <= 0) return;
    const Uint32 now = sdl.GetTicks();
    if ((last_sound && now - last_sound < 100) || sdl.GetQueuedAudioSize(device) > 22050) return;
    const auto samples = synthesize(cue, volume);
    if (sdl.QueueAudio(device, samples.data(), static_cast<Uint32>(samples.size() * sizeof(std::int16_t))) == 0) last_sound = now;
}
}
