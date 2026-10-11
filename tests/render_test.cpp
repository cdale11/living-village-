#include "living_village/render.hpp"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace village;
int main() {
    try {
        SDLApi sdl;
        if (sdl.Init(SDL_INIT_VIDEO) != 0) throw std::runtime_error(sdl.GetError());
        auto* window = sdl.CreateWindow("Smoke test", 0, 0, 720, 480, SDL_WINDOW_HIDDEN);
        if (!window) throw std::runtime_error(sdl.GetError());
        auto* renderer = sdl.CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        if (!renderer) throw std::runtime_error(sdl.GetError());
        sdl.RenderSetLogicalSize(renderer, 720, 480);
        sdl.SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        Renderer graphics(sdl, renderer); World world(42); View view;
        for (int n = 0; n < 40; ++n) world.update();
        std::vector<Uint8> pixels(720 * 480 * 4);
        for (Panel panel : {Panel::Map, Panel::Menu, Panel::Inspector, Panel::Overview, Panel::Speed, Panel::Controls}) {
            view.panel = panel;
            graphics.draw(world, view);
            if (sdl.RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, pixels.data(), 720 * 4) != 0) throw std::runtime_error(sdl.GetError());
            const auto extremes = std::minmax_element(pixels.begin(), pixels.end());
            if (*extremes.first == *extremes.second) throw std::runtime_error("Blank render");
            if (panel == Panel::Map) graphics.screenshot("build/map-preview.ppm");
            sdl.RenderPresent(renderer);
        }
        sdl.DestroyRenderer(renderer); sdl.DestroyWindow(window); sdl.Quit();
        std::cout << "PASS: map and five controller panels rendered at 720x480\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
