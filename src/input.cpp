#include "living_village/input.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace village {
Input::Input(SDLApi& api) : sdl(api) {}
Input::~Input() { close_devices(); }
void Input::load_config(const std::string& filename) {
    std::ifstream file(filename);
    const char* names[] = {"confirm", "back", "inspect", "overview", "previous", "next", "zoom_out", "zoom_in", "menu", "speed", "hotkey"};
    for (std::string line; std::getline(file, line);) {
        line = line.substr(0, line.find('#'));
        std::replace(line.begin(), line.end(), '=', ' ');
        std::istringstream words(line); std::string key; int value = 0;
        if (!(words >> key >> value)) continue;
        if (key == "raw_mode") raw_mode = value != 0;
        else if (key == "hotkey_start" && value >= -1 && value < 64) hotkey_start = value;
        else if (key == "axis_x" && value >= -1 && value < 32) axis_x = value;
        else if (key == "axis_y" && value >= -1 && value < 32) axis_y = value;
        else if (key == "hat" && value >= -1 && value < 8) hat_index = value;
        else for (int i = 0; i < button_count; ++i) {
            if (key == names[i] && value >= -1 && value < 64) raw_buttons[i] = value;
            if (key == std::string("mapped_") + names[i] && value >= -1 && value < SDL_CONTROLLER_BUTTON_MAX) mapped_buttons[i] = value;
        }
    }
}
void Input::close_devices() {
    if (controller) sdl.GameControllerClose(controller);
    else if (joystick) sdl.JoystickClose(joystick);
    controller = nullptr; joystick = nullptr;
    last.fill(false);
    last_exit_combo = false;
}
void Input::open_devices() {
    close_devices(); device_name = "Keyboard";
    for (int index = 0; index < sdl.NumJoysticks(); ++index) {
        if (!raw_mode && sdl.IsGameController(index)) {
            controller = sdl.GameControllerOpen(index);
            if (controller) joystick = sdl.GameControllerGetJoystick(controller);
        } else joystick = sdl.JoystickOpen(index);
        if (!joystick) continue;
        const char* name = controller ? sdl.GameControllerName(controller) : sdl.JoystickName(joystick);
        device_name = name ? name : "Unknown controller";
        char guid[40]; sdl.JoystickGetGUIDString(sdl.JoystickGetGUID(joystick), guid, sizeof(guid));
        std::cout << "Input: " << device_name << " GUID=" << guid << " mode=" << (controller ? "SDL mapped" : "raw joystick") << '\n';
        return;
    }
}
void Input::handle(const SDL_Event& event) {
    if (event.type == SDL_JOYDEVICEADDED && !joystick) open_devices();
    if (event.type == SDL_JOYDEVICEREMOVED && joystick && event.jdevice.which == sdl.JoystickInstanceID(joystick)) open_devices();
    if (diagnostic && event.type == SDL_JOYBUTTONDOWN) std::cout << "Raw button " << static_cast<int>(event.jbutton.button) << std::endl;
    if (diagnostic && event.type == SDL_JOYHATMOTION) std::cout << "Hat " << static_cast<int>(event.jhat.hat) << " value=" << static_cast<int>(event.jhat.value) << std::endl;
}
InputState Input::poll() {
    InputState state;
    const auto* keys = sdl.GetKeyboardState(nullptr);
    state.x = static_cast<float>((keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) - (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]));
    state.y = static_cast<float>((keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]) - (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]));
    const SDL_Scancode keyboard[] = {SDL_SCANCODE_RETURN, SDL_SCANCODE_ESCAPE, SDL_SCANCODE_X, SDL_SCANCODE_Y,
        SDL_SCANCODE_Q, SDL_SCANCODE_E, SDL_SCANCODE_MINUS, SDL_SCANCODE_EQUALS, SDL_SCANCODE_SPACE, SDL_SCANCODE_TAB, SDL_SCANCODE_F10};
    for (int i = 0; i < button_count; ++i) state.held[i] = keys[keyboard[i]];
    bool exit_combo = keys[SDL_SCANCODE_F10] && keys[SDL_SCANCODE_SPACE];
    if (joystick && sdl.JoystickGetAttached(joystick)) {
        const bool rg34xx = device_name == "Anbernic RG34XX-SP Controller" || device_name == "Anbernic RG34XX Controller";
        const int fn_index = raw_buttons[static_cast<int>(Button::Hotkey)] >= 0 ? raw_buttons[static_cast<int>(Button::Hotkey)] : rg34xx ? 11 : -1;
        const int start_index = hotkey_start >= 0 ? hotkey_start : rg34xx ? 10 : raw_buttons[static_cast<int>(Button::Menu)];
        const auto raw_held = [&](int index) { return index >= 0 && index < sdl.JoystickNumButtons(joystick) && sdl.JoystickGetButton(joystick, index); };
        exit_combo = exit_combo || (raw_held(fn_index) && raw_held(start_index));
        if (controller) {
            state.x += sdl.GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) - sdl.GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
            state.y += sdl.GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) - sdl.GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP);
            const int x = sdl.GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
            const int y = sdl.GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
            if (std::abs(x) > 10000) state.x += x / 32768.f;
            if (std::abs(y) > 10000) state.y += y / 32768.f;
            for (int i = 0; i < button_count; ++i)
                if (mapped_buttons[i] >= 0) state.held[i] = state.held[i] || sdl.GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(mapped_buttons[i]));
            state.held[static_cast<int>(Button::ZoomOut)] = state.held[static_cast<int>(Button::ZoomOut)] || sdl.GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16000;
            state.held[static_cast<int>(Button::ZoomIn)] = state.held[static_cast<int>(Button::ZoomIn)] || sdl.GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 16000;
            exit_combo = exit_combo || (sdl.GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START) &&
                mapped_buttons[static_cast<int>(Button::Hotkey)] >= 0 && sdl.GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(mapped_buttons[static_cast<int>(Button::Hotkey)])));
        } else {
            if (hat_index >= 0 && hat_index < sdl.JoystickNumHats(joystick)) {
                const auto hat = sdl.JoystickGetHat(joystick, hat_index);
                state.x += static_cast<float>(!!(hat & SDL_HAT_RIGHT) - !!(hat & SDL_HAT_LEFT));
                state.y += static_cast<float>(!!(hat & SDL_HAT_DOWN) - !!(hat & SDL_HAT_UP));
            }
            if (axis_x >= 0 && axis_x < sdl.JoystickNumAxes(joystick)) {
                int value = sdl.JoystickGetAxis(joystick, axis_x);
                if (std::abs(value) > 10000) state.x += value / 32768.f;
            }
            if (axis_y >= 0 && axis_y < sdl.JoystickNumAxes(joystick)) {
                int value = sdl.JoystickGetAxis(joystick, axis_y);
                if (std::abs(value) > 10000) state.y += value / 32768.f;
            }
            for (int i = 0; i < button_count; ++i)
                if (raw_buttons[i] >= 0 && raw_buttons[i] < sdl.JoystickNumButtons(joystick)) state.held[i] = state.held[i] || sdl.JoystickGetButton(joystick, raw_buttons[i]);
        }
    }
    state.x = std::clamp(state.x, -1.f, 1.f); state.y = std::clamp(state.y, -1.f, 1.f);
    for (int i = 0; i < button_count; ++i) state.pressed[i] = state.held[i] && !last[i];
    state.exit_requested = exit_combo && !last_exit_combo;
    if (exit_combo) state.pressed[static_cast<int>(Button::Menu)] = false;
    last_exit_combo = exit_combo;
    last = state.held;
    return state;
}
bool Input::rumble(Uint16 strength, Uint32 duration) {
    if (controller && sdl.GameControllerRumble) return sdl.GameControllerRumble(controller, strength, strength, duration) == 0;
    if (joystick && sdl.JoystickRumble) return sdl.JoystickRumble(joystick, strength, strength, duration) == 0;
    return false;
}
} // namespace village
