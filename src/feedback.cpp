#include "living_village/feedback.hpp"
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <fstream>
namespace village {
Feedback::Feedback(SDLApi& api) : sdl(api) {
    if (!sdl.InitSubSystem || !sdl.QuitSubSystem || !sdl.OpenAudioDevice || !sdl.CloseAudioDevice || !sdl.PauseAudioDevice || !sdl.QueueAudio || !sdl.ClearQueuedAudio || !sdl.GetQueuedAudioSize) return;
    std::string preferred; { std::ifstream saved("config/audio-device.txt"); char c; while (preferred.size() <= 128 && saved.get(c) && c != '\n') preferred += c; if (preferred.size()>128) preferred.clear(); }
    const bool conversion = sdl.NewAudioStream && sdl.FreeAudioStream && sdl.AudioStreamPut && sdl.AudioStreamGet && sdl.AudioStreamAvailable && sdl.AudioStreamFlush && sdl.AudioStreamClear;
    for (int attempt = 0; attempt < 2 && !device; ++attempt) {
        if (attempt == 1) { if (initialized) sdl.QuitSubSystem(SDL_INIT_AUDIO); initialized = false; ::setenv("SDL_AUDIODRIVER", "alsa", 1); }
        if (sdl.InitSubSystem(SDL_INIT_AUDIO) != 0) { status = std::string("Audio init failed: ") + sdl.GetError(); continue; }
        initialized = true;
        SDL_AudioSpec desired{}; desired.freq = conversion ? 48000 : 22050; desired.format = AUDIO_S16SYS; desired.channels = conversion ? 2 : 1; desired.samples = 1024;
        const int devices = sdl.GetNumAudioDevices ? sdl.GetNumAudioDevices(0) : 0;
        for (int index = -2; index < devices && index < 16 && !device; ++index) {
            if (index == -2 && preferred.empty()) continue;
            const char* name = index == -2 ? preferred.c_str() : index < 0 || !sdl.GetAudioDeviceName ? nullptr : sdl.GetAudioDeviceName(index, 0);
            device = sdl.OpenAudioDevice(name, 0, &desired, &output, conversion ? SDL_AUDIO_ALLOW_ANY_CHANGE : 0);
            if (device) { output_name = name ? name : ""; output_index = index < 0 ? -1 : index; }
            if (!device) status = std::string("Audio device failed: ") + sdl.GetError();
        }
        if (device && conversion) {
            stream = sdl.NewAudioStream(AUDIO_S16SYS, 1, 22050, output.format, output.channels, output.freq);
            if (!stream) { status = std::string("Audio conversion failed: ") + sdl.GetError(); sdl.CloseAudioDevice(device); device = 0; }
        }
    }
    if (device) {
        sdl.PauseAudioDevice(device, 0);
        status = std::string(sdl.GetCurrentAudioDriver ? sdl.GetCurrentAudioDriver() : "SDL") + " " + std::to_string(output.freq) + " Hz / " + std::to_string(output.channels) + " ch";
    }
    std::cout << "Audio: " << status << '\n';
}
Feedback::~Feedback() {
    if (stream) sdl.FreeAudioStream(stream);
    if (device) sdl.CloseAudioDevice(device);
    if (initialized) sdl.QuitSubSystem(SDL_INIT_AUDIO);
}
bool Feedback::next_output() {
    if (!initialized || !sdl.GetNumAudioDevices || !sdl.GetAudioDeviceName) return false;
    const int count = std::min(16,sdl.GetNumAudioDevices(0)); if (count <= 0) return false;
    for (int i=0;i<count;++i) { const char* listed=sdl.GetAudioDeviceName(i,0); if (listed && output_name == listed) output_index=i; }
    output_index = (output_index + 2) % (count + 1) - 1;
    const char* name = output_index < 0 ? nullptr : sdl.GetAudioDeviceName(output_index,0);
    const std::string chosen = name ? name : "";
    mute(); if (stream) { sdl.FreeAudioStream(stream); stream=nullptr; } if (device) { sdl.CloseAudioDevice(device); device=0; }
    const bool conversion = sdl.NewAudioStream && sdl.FreeAudioStream && sdl.AudioStreamPut && sdl.AudioStreamGet && sdl.AudioStreamAvailable && sdl.AudioStreamFlush && sdl.AudioStreamClear;
    SDL_AudioSpec desired{}; desired.freq=conversion?48000:22050; desired.format=AUDIO_S16SYS; desired.channels=conversion?2:1; desired.samples=1024;
    device=sdl.OpenAudioDevice(name,0,&desired,&output,conversion?SDL_AUDIO_ALLOW_ANY_CHANGE:0);
    if (device && conversion) { stream=sdl.NewAudioStream(AUDIO_S16SYS,1,22050,output.format,output.channels,output.freq); if(!stream) { sdl.CloseAudioDevice(device); device=0; } }
    if (!device) { status=std::string("Output failed: ")+sdl.GetError(); output_name=chosen; return false; }
    output_name=chosen; sdl.PauseAudioDevice(device,0);
    status=std::string(sdl.GetCurrentAudioDriver?sdl.GetCurrentAudioDriver():"SDL")+" "+std::to_string(output.freq)+" Hz / "+std::to_string(output.channels)+" ch";
    return true;
}
void Feedback::mute() { last_sound = 0; if (device) sdl.ClearQueuedAudio(device); if (stream) sdl.AudioStreamClear(stream); }
void Feedback::play(Cue cue, int volume) {
    last_queue_ok = false;
    if (!device || volume <= 0) return;
    const Uint32 now = sdl.GetTicks();
    if ((last_sound && now - last_sound < 100) || sdl.GetQueuedAudioSize(device) > 192000) return;
    const auto samples = synthesize(cue, volume);
    if (stream) {
        sdl.AudioStreamClear(stream);
        if (sdl.AudioStreamPut(stream, samples.data(), static_cast<int>(samples.size() * sizeof(std::int16_t))) < 0 || sdl.AudioStreamFlush(stream) < 0) return;
        const int available = sdl.AudioStreamAvailable(stream);
        if (available <= 0 || available > 1024 * 1024) return;
        std::vector<Uint8> converted(static_cast<std::size_t>(available)); const int bytes = sdl.AudioStreamGet(stream, converted.data(), available);
        last_queue_ok = bytes > 0 && sdl.QueueAudio(device, converted.data(), static_cast<Uint32>(bytes)) == 0;
    } else last_queue_ok = sdl.QueueAudio(device, samples.data(), static_cast<Uint32>(samples.size() * sizeof(std::int16_t))) == 0;
    if (last_queue_ok) last_sound = now;
}
}
