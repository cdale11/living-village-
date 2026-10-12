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
        world.plan(BuildingKind::Farm, {30, 32});
        auto& farm = world.buildings.back(); farm.complete = true; farm.food = 6;
        world.plan(BuildingKind::Shelter, {34, 32});
        auto& house = world.buildings.back(); house.complete = true; house.wood_delivered = 12; house.stone_delivered = 4; house.work = 24;
        world.ticks = 7 * 2400;
        for (int id : {0, 1}) { auto* p = world.person(id); p->age = 24; p->hunger = 0; p->home = house.id; p->family_drive = .8; }
        world.plan(BuildingKind::Shelter, {34, 30});
        auto& spare_house = world.buildings.back(); spare_house.complete = true; spare_house.wood_delivered = 12; spare_house.stone_delivered = 4; spare_house.work = 24;
        world.person(0)->relationships[1] = world.person(1)->relationships[0] = 80;
        if (!world.form_household(0, 1) || !world.have_child(0, 1)) throw std::runtime_error("Family render fixture failed");
        view.selected_tile = {34, 32};
        for (int n = 0; n < 1000; ++n) world.update();
        std::vector<Uint8> pixels(720 * 480 * 4);
        for (Panel panel : {Panel::Map, Panel::Menu, Panel::Inspector, Panel::Overview, Panel::Speed, Panel::Controls, Panel::Tile, Panel::Build, Panel::NewVillage}) {
            view.panel = panel;
            graphics.draw(world, view);
            if (sdl.RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, pixels.data(), 720 * 4) != 0) throw std::runtime_error(sdl.GetError());
            bool variation = false;
            for (std::size_t i = 4; i < pixels.size(); i += 4)
                if (pixels[i] != pixels[0] || pixels[i + 1] != pixels[1] || pixels[i + 2] != pixels[2]) { variation = true; break; }
            if (!variation) throw std::runtime_error("Blank render");
            if (panel == Panel::Map) graphics.screenshot("build/map-preview.ppm");
            if (panel == Panel::Build) graphics.screenshot("build/build-preview.ppm");
            if (panel == Panel::Inspector) for (int page = 1; page < 5; ++page) {
                view.inspector_page = page; graphics.draw(world, view);
                if (page == 3) graphics.screenshot("build/family-preview.ppm");
                if (page == 4) graphics.screenshot("build/learning-preview.ppm");
            }
            if (panel == Panel::Overview) for (int page = 1; page < 4; ++page) {
                view.overview_page = page; graphics.draw(world, view);
                if (page == 1) graphics.screenshot("build/population-preview.ppm");
            }
            sdl.RenderPresent(renderer);
        }
        sdl.DestroyRenderer(renderer); sdl.DestroyWindow(window); sdl.Quit();
        std::cout << "PASS: map, buildings, planning, inspection and controller panels at 720x480\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
