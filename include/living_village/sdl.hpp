#pragma once
#include <SDL.h>

// SDL is loaded from the OS runtime. This avoids bundling a desktop SDL build
// whose video drivers would be wrong for KNULLI's display stack.
#define VILLAGE_SDL_FUNCTIONS(X) \
    X(Init) X(Quit) X(GetError) X(GetTicks) X(Delay) X(SetHint) \
    X(CreateWindow) X(DestroyWindow) X(CreateRenderer) X(DestroyRenderer) \
    X(RenderSetLogicalSize) X(SetRenderDrawColor) X(SetRenderDrawBlendMode) \
    X(RenderClear) X(RenderFillRect) X(RenderDrawRect) X(RenderDrawLine) X(RenderPresent) \
    X(PollEvent) X(GetKeyboardState) X(GetNumVideoDrivers) X(GetVideoDriver) X(GetCurrentVideoDriver) \
    X(GetRendererInfo) X(NumJoysticks) X(IsGameController) X(GameControllerOpen) \
    X(GameControllerClose) X(GameControllerGetJoystick) X(GameControllerGetButton) \
    X(GameControllerGetAxis) X(GameControllerName) X(GameControllerAddMappingsFromRW) X(GameControllerAddMapping) X(RWFromFile) \
    X(JoystickOpen) X(JoystickClose) X(JoystickName) X(JoystickInstanceID) \
    X(JoystickNumHats) X(JoystickGetHat) X(JoystickNumAxes) X(JoystickGetAxis) \
    X(JoystickNumButtons) X(JoystickGetButton) X(JoystickGetAttached) X(JoystickGetGUID) X(JoystickGetGUIDString) \
    X(CreateTexture) X(DestroyTexture) X(UpdateTexture) X(SetTextureBlendMode) X(SetTextureColorMod) X(RenderCopyEx) \
    X(RenderReadPixels)

#define VILLAGE_SDL_OPTIONAL(X) \
    X(InitSubSystem) X(QuitSubSystem) X(OpenAudioDevice) X(CloseAudioDevice) X(PauseAudioDevice) \
    X(QueueAudio) X(ClearQueuedAudio) X(GetQueuedAudioSize) X(GetCurrentAudioDriver) \
    X(GameControllerRumble) X(JoystickRumble) X(JoystickIsHaptic) X(HapticOpenFromJoystick) \
    X(HapticRumbleSupported) X(HapticRumbleInit) X(HapticRumblePlay) X(HapticRumbleStop) X(HapticClose) \
    X(GetNumAudioDrivers) X(GetAudioDriver) X(GetNumAudioDevices) X(GetAudioDeviceName) \
    X(NewAudioStream) X(FreeAudioStream) X(AudioStreamPut) X(AudioStreamGet) X(AudioStreamAvailable) X(AudioStreamFlush) X(AudioStreamClear)

namespace village {
class SDLApi {
public:
    SDLApi();
    ~SDLApi();
    SDLApi(const SDLApi&) = delete;
    SDLApi& operator=(const SDLApi&) = delete;
#define VILLAGE_DECLARE(name) decltype(&SDL_##name) name = nullptr;
    VILLAGE_SDL_FUNCTIONS(VILLAGE_DECLARE)
    VILLAGE_SDL_OPTIONAL(VILLAGE_DECLARE)
#undef VILLAGE_DECLARE
private:
    void* library = nullptr;
};
}
