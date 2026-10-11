#include "living_village/sdl.hpp"
#include <dlfcn.h>
#include <stdexcept>
#include <string>

namespace village {
SDLApi::SDLApi() {
    for (const char* name : {"libSDL2-2.0.so.0", "libSDL2.so"}) {
        library = dlopen(name, RTLD_NOW | RTLD_LOCAL);
        if (library) break;
    }
    if (!library) throw std::runtime_error("SDL2 runtime could not be loaded: " + std::string(dlerror()));
    try {
#define VILLAGE_LOAD(name) name = reinterpret_cast<decltype(name)>(dlsym(library, "SDL_" #name)); \
        if (!name) throw std::runtime_error("SDL2 is missing SDL_" #name);
        VILLAGE_SDL_FUNCTIONS(VILLAGE_LOAD)
#undef VILLAGE_LOAD
    } catch (...) { dlclose(library); library = nullptr; throw; }
}
SDLApi::~SDLApi() { if (library) dlclose(library); }
}
