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
        world.plan(BuildingKind::Workshop, {36, 32});
        auto& workshop = world.buildings.back(); workshop.complete = true; workshop.wood_delivered = 12; workshop.stone_delivered = 8; workshop.work = 28;
        view.selected_tile = {36, 32};
        for (int n = 0; n < 1000; ++n) world.update();
        std::vector<Uint8> pixels(720 * 480 * 4);
        for (Panel panel : {Panel::Map, Panel::Menu, Panel::Inspector, Panel::Overview, Panel::Speed, Panel::Controls, Panel::Tile, Panel::Build, Panel::NewVillage, Panel::SeedEdit, Panel::NameEdit, Panel::ConfirmCreate, Panel::Saves, Panel::ConfirmDelete}) {
            if (panel == Panel::SeedEdit) view.edit_buffer = "4294967295";
            if (panel == Panel::NameEdit) view.edit_buffer = "LIVING VILLAGE           ";
            if (panel == Panel::NewVillage) view.menu_index = 4;
            if (panel == Panel::ConfirmDelete) { view.pending_name = "Old village"; view.pending_slot = "world-000002"; }
            if (panel == Panel::Saves) { view.menu_index = 0; view.saves = {{"world-000001", "First Village", 123, 42, Biome::Woodland, Geography::Island, true, false}, {"world-000002", "Desert Town", 42, 11, Biome::Arid, Geography::Highlands, true, true}}; view.active_slot = "world-000001"; }
            view.panel = panel; if (panel == Panel::Build) view.menu_index = 5;
            graphics.draw(world, view);
            if (sdl.RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_RGBA32, pixels.data(), 720 * 4) != 0) throw std::runtime_error(sdl.GetError());
            bool variation = false;
            for (std::size_t i = 4; i < pixels.size(); i += 4)
                if (pixels[i] != pixels[0] || pixels[i + 1] != pixels[1] || pixels[i + 2] != pixels[2]) { variation = true; break; }
            if (!variation) throw std::runtime_error("Blank render");
            if (panel == Panel::Map) graphics.screenshot("build/map-preview.ppm");
            if (panel == Panel::Saves) graphics.screenshot("build/saves-preview.ppm");
            if (panel == Panel::NewVillage) graphics.screenshot("build/creator-preview.ppm");
            if (panel == Panel::Build) graphics.screenshot("build/build-preview.ppm");
            if (panel == Panel::Tile) graphics.screenshot("build/tile-preview.ppm");
            if (panel == Panel::Inspector) for (int page = 1; page < 6; ++page) {
                view.inspector_page = page; graphics.draw(world, view);
                if (page == 3) graphics.screenshot("build/family-preview.ppm");
                if (page == 4) graphics.screenshot("build/learning-preview.ppm");
                if (page == 5) graphics.screenshot("build/practice-preview.ppm");
            }
            if (panel == Panel::Overview) for (int page = 1; page < 9; ++page) {
                view.overview_page = page; graphics.draw(world, view);
                if (page == 1) graphics.screenshot("build/population-preview.ppm");
                if (page == 4) graphics.screenshot("build/economy-preview.ppm");
                if (page == 5) graphics.screenshot("build/sound-preview.ppm");
                if (page == 6) graphics.screenshot("build/ecology-preview.ppm");
                if (page == 3) graphics.screenshot("build/diagnostics-preview.ppm");
                if (page == 8) graphics.screenshot("build/agent-diagnostics-preview.ppm");
                if (page == 7) graphics.screenshot("build/wildlife-preview.ppm");
            }
            sdl.RenderPresent(renderer);
        }
        sdl.DestroyRenderer(renderer); sdl.DestroyWindow(window); sdl.Quit();
        std::cout << "PASS: map, buildings, planning, inspection and controller panels at 720x480\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
