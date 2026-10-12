#pragma once
#include "sdl.hpp"
#include "rumble.hpp"
#include <array>
#include <string>

namespace village {
enum class Button : int { Confirm, Back, Inspect, Overview, Previous, Next, ZoomOut, ZoomIn, Menu, Speed, Hotkey, Count };
constexpr int button_count = static_cast<int>(Button::Count);
struct InputState {
    float x = 0, y = 0;
    std::array<bool, button_count> held{}, pressed{};
    bool exit_requested = false;
    bool hit(Button button) const { return pressed[static_cast<int>(button)]; }
};
class Input {
public:
    explicit Input(SDLApi& sdl);
    ~Input();
    void load_config(const std::string& filename);
    void open_devices();
    void handle(const SDL_Event& event);
    InputState poll();
    bool rumble(Uint16 strength = 8000, Uint32 duration = 120);
    std::string rumble_status = "Not tested";
    bool diagnostic = false;
    std::string device_name = "Keyboard";
private:
    SDLApi& sdl;
    SDL_GameController* controller = nullptr;
    SDL_Joystick* joystick = nullptr;
    SDL_Haptic* haptic = nullptr;
    LinuxRumble linux_rumble;
    std::array<bool, button_count> last{};
    bool last_exit_combo = false;
    std::array<int, button_count> raw_buttons{{0, 1, 2, 3, 4, 5, 6, 7, 9, 8, -1}};
    std::array<int, button_count> mapped_buttons{{SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B,
        SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, -1, -1, SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_BACK, SDL_CONTROLLER_BUTTON_GUIDE}};
    int hotkey_start = -1;
    int axis_x = 0, axis_y = 1, hat_index = 0;
    bool raw_mode = false;
    void close_devices();
};
}
