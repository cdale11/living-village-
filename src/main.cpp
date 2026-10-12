#include "living_village/input.hpp"
#include "living_village/render.hpp"
#include "living_village/saves.hpp"
#include "living_village/ui.hpp"
#include <algorithm>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sys/resource.h>

namespace {
volatile std::sig_atomic_t stop_requested = 0;
void signal_stop(int) { stop_requested = 1; }
struct Options {
    bool fullscreen = false, headless = false, no_save = false, new_world = false, diagnose = false;
    std::uint32_t seed = 20261011;
    int frames = 0, steps = 24000;
    std::string save_dir = "saves", screenshot;
};
Options options(int argc, char** argv) {
    Options result;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() { if (++i >= argc) throw std::runtime_error("Missing value for " + arg); return std::string(argv[i]); };
        if (arg == "--fullscreen") result.fullscreen = true;
        else if (arg == "--headless") result.headless = true;
        else if (arg == "--no-save") result.no_save = true;
        else if (arg == "--new") result.new_world = true;
        else if (arg == "--diagnose-input") result.diagnose = true;
        else if (arg == "--seed") result.seed = static_cast<std::uint32_t>(std::stoul(value()));
        else if (arg == "--steps") result.steps = std::stoi(value());
        else if (arg == "--frames") result.frames = std::stoi(value());
        else if (arg == "--save-dir") result.save_dir = value();
        else if (arg == "--screenshot") result.screenshot = value();
        else if (arg == "--help") {
            std::cout << "Living Village C++ / SDL2\n"
                      << "--fullscreen --seed N --save-dir PATH --new --no-save\n"
                      << "--headless --steps N   (no display, SDL or save writes)\n"
                      << "--diagnose-input --frames N --screenshot FILE.ppm\n";
            std::exit(0);
        } else throw std::runtime_error("Unknown option " + arg);
    }
    if (result.steps < 1 || result.steps > 100000000 || result.frames < 0) throw std::runtime_error("Steps/frames out of range");
    return result;
}
}

int main(int argc, char** argv) {
    using namespace village;
    try {
        const Options config = options(argc, argv);
        World world(config.seed);
        if (config.headless) {
            auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < config.steps; ++i) world.update();
            const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            struct rusage usage{}; getrusage(RUSAGE_SELF, &usage);
            int survivors = 0; for (const auto& p : world.villagers) if (p.alive()) ++survivors;
            int built = 0; for (const auto& b : world.buildings) if (b.complete) ++built;
            std::uint64_t training_updates = 0; for (const auto& p : world.villagers) training_updates += p.brain.updates;
            std::cout << "Headless: " << config.steps << " steps, day " << world.day() << ", " << survivors
                      << " alive (" << world.child_count() << " children), " << world.births << " births, " << world.arrivals << " arrivals, " << world.deaths << " deaths, " << built << "/" << world.buildings.size() << " buildings complete, " << seconds << " CPU wall seconds, " << training_updates << " retained learning updates, peak RSS " << usage.ru_maxrss << " KiB\n";
            return 0;
        }
        const auto save_path = std::filesystem::path(config.save_dir) / "village.lvsave";
        Interface ui;
        if (!config.new_world && !config.no_save && (std::filesystem::exists(save_path) || std::filesystem::exists(save_path.string() + ".bak"))) {
            bool backup = false;
            world = load_save(save_path, &backup);
            ui.view.status = backup ? "Recovered previous save" : "Village loaded";
        }
        ui.view.speed = world.simulation_speed;
        std::signal(SIGINT, signal_stop); std::signal(SIGTERM, signal_stop); std::signal(SIGHUP, signal_stop);
        SDLApi sdl;
        sdl.SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        sdl.SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
        if (sdl.Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) throw std::runtime_error(sdl.GetError());
        struct Cleanup { SDLApi& sdl; ~Cleanup() { sdl.Quit(); } } cleanup{sdl};
        std::cout << "SDL video backend: " << sdl.GetCurrentVideoDriver() << "\nAvailable backends:";
        for (int i = 0; i < sdl.GetNumVideoDrivers(); ++i) std::cout << ' ' << sdl.GetVideoDriver(i);
        std::cout << '\n';
        const Uint32 flags = config.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_RESIZABLE;
        auto window = std::unique_ptr<SDL_Window, decltype(sdl.DestroyWindow)>(sdl.CreateWindow("Living Village", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 720, 480, flags), sdl.DestroyWindow);
        if (!window) throw std::runtime_error("Window creation failed: " + std::string(sdl.GetError()));
        SDL_Renderer* raw_renderer = sdl.CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED);
        if (!raw_renderer) raw_renderer = sdl.CreateRenderer(window.get(), -1, SDL_RENDERER_SOFTWARE);
        auto renderer = std::unique_ptr<SDL_Renderer, decltype(sdl.DestroyRenderer)>(raw_renderer, sdl.DestroyRenderer);
        if (!renderer) throw std::runtime_error("Renderer creation failed: " + std::string(sdl.GetError()));
        if (sdl.RenderSetLogicalSize(renderer.get(), 720, 480) != 0) throw std::runtime_error(sdl.GetError());
        sdl.SetRenderDrawBlendMode(renderer.get(), SDL_BLENDMODE_BLEND);
        SDL_RendererInfo renderer_info{}; sdl.GetRendererInfo(renderer.get(), &renderer_info);
        std::cout << "Renderer: " << renderer_info.name << '\n';
        if (auto* mappings = sdl.RWFromFile("config/gamecontrollerdb.txt", "r")) sdl.GameControllerAddMappingsFromRW(mappings, 1);
        Input input(sdl); input.load_config("config/controller.cfg"); input.diagnostic = config.diagnose; input.open_devices();
        Renderer graphics(sdl, renderer.get());
        Uint32 previous = sdl.GetTicks(), last_save = previous;
        double accumulator = 0;
        int frame_count = 0;
        bool running = true;
        std::cout << std::flush;
        auto save = [&]() {
            if (config.no_save) return true;
            last_save = sdl.GetTicks(); // Failed autosaves retry in 30 seconds, not every frame.
            try { save_atomic(world, save_path); ui.view.status = "Village saved"; return true; }
            catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = "SAVE FAILED - CHECK LOG / FREE SPACE"; return false; }
        };
        while (running && !stop_requested) {
            const Uint32 begin = sdl.GetTicks();
            const double elapsed = std::min((begin - previous) / 1000.0, 0.2);
            previous = begin;
            SDL_Event event;
            while (sdl.PollEvent(&event)) {
                if (event.type == SDL_QUIT) running = false;
                if (event.type == SDL_APP_WILLENTERBACKGROUND || (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)) {
                    ui.view.panel = Panel::Menu; ui.view.menu_index = 0; save(); accumulator = 0;
                }
                input.handle(event);
            }
            const auto request = ui.update(input.poll(), world, elapsed);
            world.simulation_speed = ui.view.speed;
            if (request == Request::Save) save();
            else if (request == Request::Load) {
                try { world = load_save(save_path); ui.view.speed = world.simulation_speed; ui.view.status = "Village loaded"; accumulator = 0; }
                catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = "LOAD FAILED - CHECK LOG"; }
            } else if (request == Request::Exit) { if (save()) running = false; }
            else if (request == Request::Plan) {
                if (world.plan(static_cast<BuildingKind>(ui.view.menu_index), ui.view.selected_tile)) {
                    ui.view.status = "Plan placed - villagers supply and build it"; ui.view.panel = Panel::Map;
                } else ui.view.status = "Plan needs reachable empty grass";
            } else if (request == Request::ToggleDevelopment) {
                world.auto_develop = !world.auto_develop;
                ui.view.status = world.auto_develop ? "Autonomous development enabled" : "Only player construction plans enabled";
            } else if (request == Request::ToggleGrowth) {
                world.population_growth = !world.population_growth;
                ui.view.status = world.population_growth ? "New births and arrivals enabled" : "New births and arrivals paused";
            } else if (request == Request::ToggleLearning) {
                world.learning_enabled = !world.learning_enabled;
                ui.view.status = world.learning_enabled ? "Online neural learning enabled" : "Learning paused - learned decisions retained";
            } else if (request == Request::NewVillage) {
                try {
                    if (!config.no_save) save_atomic(world, std::filesystem::path(config.save_dir) / "previous-village.lvsave");
                    world = World(world.seed + 1); ui = Interface{};
                    if (save()) ui.view.status = "New village - earlier village archived";
                } catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = "COULD NOT ARCHIVE - KEPT CURRENT VILLAGE"; }
            } else if (request == Request::PreviousVillage) {
                try {
                    const auto archived_path = std::filesystem::path(config.save_dir) / "previous-village.lvsave";
                    World archived = load_save(archived_path);
                    if (!config.no_save) save_atomic(world, archived_path);
                    world = std::move(archived); ui = Interface{}; ui.view.speed = world.simulation_speed;
                    if (save()) ui.view.status = "Switched to previous village";
                } catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = "NO PREVIOUS VILLAGE AVAILABLE"; }
            } else if (request == Request::Designate) {
                ui.view.status = world.designate(ui.view.selected_tile) ? "Gathering area updated" : "Choose a tree, bush or stone - max 16 areas";
            }
            ui.view.selected = std::clamp(ui.view.selected, 0, static_cast<int>(world.villagers.size()) - 1);
            const int watched_id = world.villagers[ui.view.selected].id;
            if (ui.view.panel == Panel::Map) {
                accumulator += elapsed * ui.view.speed;
                int steps = 0;
                while (accumulator >= fixed_step && steps++ < 8) { world.update(); accumulator -= fixed_step; }
            } else accumulator = 0;
            bool found = false;
            for (std::size_t index = 0; index < world.villagers.size(); ++index) if (world.villagers[index].id == watched_id) { ui.view.selected = static_cast<int>(index); found = true; break; }
            if (!found) {
                ui.view.selected = 0;
                for (std::size_t index = 0; index < world.villagers.size(); ++index) if (world.villagers[index].alive()) { ui.view.selected = static_cast<int>(index); break; }
            }
            if (begin - last_save >= 30000) save();
            graphics.draw(world, ui.view);
            ++frame_count;
            if (config.frames > 0 && frame_count >= config.frames) {
                if (!config.screenshot.empty()) graphics.screenshot(config.screenshot);
                running = false;
            }
            sdl.RenderPresent(renderer.get());
            const Uint32 render_time = sdl.GetTicks() - begin;
            if (render_time < 33) sdl.Delay(33 - render_time);
        }
        const bool saved = save();
        std::cout << "Exited cleanly after " << frame_count << " frames\n";
        return saved ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Living Village: " << error.what() << '\n';
        return 1;
    }
}
