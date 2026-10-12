#pragma once
#include "sdl.hpp"
#include <array>
namespace village {
enum class Sprite { Oak, Pine, Rabbit, Deer, Fox, Bee, Person, Shelter, Count };
class Sprites {
public:
    Sprites(SDLApi& api, SDL_Renderer* renderer);
    ~Sprites();
    void draw(Sprite sprite, int x, int y, int width, int height, bool mirror = false, SDL_Color tint = {255,255,255,255});
private:
    SDLApi& sdl; SDL_Renderer* renderer;
    std::array<SDL_Texture*, static_cast<int>(Sprite::Count)> textures{};
};
}
