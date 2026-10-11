// Actual SDL virtual joystick test, runs without display or physical controller.
#include "living_village/input.hpp"
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

using namespace village;
void check(bool valid, const char* message) { if (!valid) throw std::runtime_error(message); }
int main() {
    try {
        SDLApi sdl;
        check(sdl.Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) == 0, "SDL input init failed");
        void* lib = dlopen("libSDL2-2.0.so.0", RTLD_NOW);
        auto attach = reinterpret_cast<decltype(&SDL_JoystickAttachVirtual)>(dlsym(lib, "SDL_JoystickAttachVirtual"));
        auto set_button = reinterpret_cast<decltype(&SDL_JoystickSetVirtualButton)>(dlsym(lib, "SDL_JoystickSetVirtualButton"));
        auto set_hat = reinterpret_cast<decltype(&SDL_JoystickSetVirtualHat)>(dlsym(lib, "SDL_JoystickSetVirtualHat"));
        auto detach = reinterpret_cast<decltype(&SDL_JoystickDetachVirtual)>(dlsym(lib, "SDL_JoystickDetachVirtual"));
        auto pump = reinterpret_cast<decltype(&SDL_PumpEvents)>(dlsym(lib, "SDL_PumpEvents"));
        check(attach && set_button && set_hat && detach && pump, "Tests require SDL 2.0.14+ virtual joystick support");
        int index = attach(SDL_JOYSTICK_TYPE_GAMECONTROLLER, 6, 16, 1);
        check(index >= 0, "Virtual joystick creation failed");
        auto* virtual_pad = sdl.JoystickOpen(index);
        char guid[40]; sdl.JoystickGetGUIDString(sdl.JoystickGetGUID(virtual_pad), guid, sizeof(guid));
        const std::string mapping = std::string(guid) + ",Village Test,a:b0,b:b1,x:b2,y:b3,back:b8,start:b9,leftshoulder:b4,rightshoulder:b5,dpup:h0.1,dpright:h0.2,dpdown:h0.4,dpleft:h0.8,leftx:a0,lefty:a1,lefttrigger:a2,righttrigger:a3,";
        check(sdl.GameControllerAddMapping(mapping.c_str()) >= 0, "Virtual mapping failed");
        {
            Input input(sdl); input.open_devices(); pump(); input.poll();
            set_button(virtual_pad, 9, 1); set_hat(virtual_pad, 0, SDL_HAT_RIGHT); pump();
            auto state = input.poll();
            check(state.hit(Button::Menu) && state.x > .5, "Mapped Start or D-pad failed");
            check(!input.poll().hit(Button::Menu), "Held button repeated as a press");
            set_button(virtual_pad, 9, 0); set_button(virtual_pad, 0, 1); pump();
            check(input.poll().hit(Button::Confirm), "Mapped A did not confirm");
        }
        const std::string config = "/tmp/living-village-input-" + std::to_string(getpid()) + ".cfg";
        { std::ofstream file(config); file << "raw_mode=1\nmenu=12\n"; }
        {
            Input input(sdl); input.load_config(config); input.open_devices();
            set_button(virtual_pad, 0, 0); set_button(virtual_pad, 12, 1); set_hat(virtual_pad, 0, SDL_HAT_UP); pump();
            auto state = input.poll();
            check(state.hit(Button::Menu) && state.y < -.5, "Raw remapping or D-pad failed");
        }
        unlink(config.c_str());
        sdl.JoystickClose(virtual_pad); detach(index); dlclose(lib); sdl.Quit();
        std::cout << "PASS: SDL mapped buttons/D-pad, edge detection, raw controller remapping\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
