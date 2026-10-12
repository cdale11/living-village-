#include "living_village/feedback.hpp"
#include "living_village/input.hpp"
#include "living_village/performance.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace village;
int haptic_starts=0,haptic_stops=0,haptic_closes=0,joystick_closes=0;
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
int main() {
    try {
        for (Cue cue : {Cue::Confirm, Cue::Complete, Cue::Warning, Cue::Birth, Cue::Trade, Cue::Birds, Cue::Rain, Cue::Fire}) {
            const auto sound = synthesize(cue, 40), silent = synthesize(cue, 0);
            check(!sound.empty() && sound.size() < 22050 && std::any_of(sound.begin(), sound.end(), [](int value) { return value != 0; }), "Procedural sound is empty or unbounded");
            check(std::all_of(sound.begin(), sound.end(), [](int value) { return value >= -4000 && value <= 4000; }) && std::all_of(silent.begin(), silent.end(), [](int value) { return value == 0; }), "Volume/mute failed");
        }
        SDLApi sdl; check(sdl.Init(SDL_INIT_EVENTS) == 0, "SDL init failed");
        { Feedback audio(sdl); check(audio.available(), "Dummy audio device did not open"); audio.play(Cue::Complete, 40); check(audio.last_queue_ok, "Negotiated audio conversion did not queue PCM"); audio.mute(); audio.play(Cue::Birds,40); check(audio.last_queue_ok, "Ambient PCM did not queue"); audio.mute(); check(audio.next_output(), "Controller output selection failed"); audio.play(Cue::Complete,40); check(audio.last_queue_ok, "Selected output did not queue PCM"); Input input(sdl); check(!input.rumble(), "No-controller rumble did not fail gracefully"); }
        sdl.NumJoysticks = []() { return 1; }; sdl.IsGameController = [](int) { return SDL_FALSE; };
        sdl.JoystickOpen = [](int) { return reinterpret_cast<SDL_Joystick*>(1); };
        sdl.JoystickClose = [](SDL_Joystick*) { ++joystick_closes; };
        sdl.JoystickName = [](SDL_Joystick*) { return "Haptic lifecycle test"; };
        sdl.JoystickGetGUID = [](SDL_Joystick*) { return SDL_JoystickGUID{}; };
        sdl.JoystickRumble = [](SDL_Joystick*,Uint16,Uint16,Uint32) { return -1; };
        sdl.JoystickIsHaptic = [](SDL_Joystick*) { return 1; };
        sdl.InitSubSystem = [](Uint32 flags) { if(flags==SDL_INIT_HAPTIC) ++haptic_starts; return 0; };
        sdl.QuitSubSystem = [](Uint32 flags) { if(flags==SDL_INIT_HAPTIC) ++haptic_stops; };
        sdl.HapticOpenFromJoystick = [](SDL_Joystick*) { return reinterpret_cast<SDL_Haptic*>(1); };
        sdl.HapticRumbleSupported = [](SDL_Haptic*) { return 1; }; sdl.HapticRumbleInit = [](SDL_Haptic*) { return 0; };
        sdl.HapticRumblePlay = [](SDL_Haptic*,float strength,Uint32 duration) { return strength>0 && duration<=2000 ? 0 : -1; };
        sdl.HapticClose = [](SDL_Haptic*) { ++haptic_closes; };
        { Input motor(sdl); motor.open_devices(); check(motor.rumble(42000,250),"SDL haptic fallback failed"); }
        check(haptic_starts==1 && haptic_stops==1 && haptic_closes==1 && joystick_closes==1,"Haptic subsystem lifetime unbalanced");
        sdl.HapticOpenFromJoystick = [](SDL_Joystick*) -> SDL_Haptic* { return nullptr; };
        { Input failed(sdl); failed.open_devices(); failed.rumble(0,0); }
        check(haptic_starts==2 && haptic_stops==2,"Failed haptic open leaked subsystem");
        sdl.Quit();
        ResourceMeter resource; resource.sample(0); check(resource.ram_mib > 0 && resource.cpu_percent < 0, "Current RSS/CPU baseline invalid"); resource.sample(.5); check(resource.cpu_percent < 0, "Meter was not rate limited"); resource.sample(1.1); check(resource.cpu_percent >= 0, "CPU usage was not sampled");
        Performance metrics; for (int i = 0; i < 400; ++i) metrics.frame(2, 1000.0 / 30, 1);
        check(metrics.report().find("fps=30.00") != std::string::npos && metrics.report().find("mean_work_ms=2.00") != std::string::npos, "Performance counters are incorrect");
        std::cout << "PASS: original PCM synthesis/volume/mute, SDL dummy audio, graceful unsupported rumble, bounded frame metrics\n";
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
