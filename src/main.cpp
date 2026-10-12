#include "living_village/input.hpp"
#include "living_village/port_art.hpp"
#include "living_village/feedback.hpp"
#include "living_village/performance.hpp"
#include "living_village/render.hpp"
#include "living_village/saves.hpp"
#include "living_village/ui.hpp"
#include <algorithm>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <memory>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <sys/resource.h>

namespace {
volatile std::sig_atomic_t stop_requested = 0;
void signal_stop(int) { stop_requested = 1; }
struct Options {
    bool ai_diagnostics = false, fullscreen = false, headless = false, no_save = false, new_world = false, diagnose = false, profile = false;
    std::uint32_t seed = 20261011;
    village::Biome biome = village::Biome::Temperate;
    village::Geography geography = village::Geography::Ponds;
    std::string world_name = "New Village";
    int frames = 0, steps = 24000, benchmark_villagers = 0, starting_food = 18, starting_population = 6;
    std::string save_dir = "saves", screenshot, art_ports;
};
std::uint32_t seed_value(const std::string& value) {
    if (value.empty() || value.size() > 10 || value.find_first_not_of("0123456789") != std::string::npos) throw std::runtime_error("Seed must be 0 to 4294967295");
    const auto number = std::stoull(value); if (number > 4294967295ULL) throw std::runtime_error("Seed must be 0 to 4294967295");
    return static_cast<std::uint32_t>(number);
}
Options options(int argc, char** argv) {
    Options result;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() { if (++i >= argc) throw std::runtime_error("Missing value for " + arg); return std::string(argv[i]); };
        if (arg == "--fullscreen") result.fullscreen = true;
        else if (arg == "--ai-diagnostics") result.ai_diagnostics = true;
        else if (arg == "--population") { result.starting_population = std::stoi(value()); if (result.starting_population < 2 || result.starting_population > 20) throw std::runtime_error("Population must be 2 to 20"); }
        else if (arg == "--starting-food") { result.starting_food = std::stoi(value()); if (result.starting_food != 6 && result.starting_food != 18 && result.starting_food != 30) throw std::runtime_error("Starting food: 6, 18 or 30"); }
        else if (arg == "--headless") result.headless = true;
        else if (arg == "--no-save") result.no_save = true;
        else if (arg == "--new") result.new_world = true;
        else if (arg == "--register-port-art") result.art_ports = value();
        else if (arg == "--profile") result.profile = true;
        else if (arg == "--benchmark-villagers") { result.benchmark_villagers = std::stoi(value()); result.no_save = true; result.new_world = true; result.profile = true; }
        else if (arg == "--diagnose-input") result.diagnose = true;
        else if (arg == "--seed") result.seed = seed_value(value());
        else if (arg == "--world-name") { result.world_name = value(); if (!village::valid_world_name(result.world_name)) throw std::runtime_error("Invalid world name"); }
        else if (arg == "--biome") {
            const auto name = value(); bool found = false;
            for (int b = 0; b < village::biome_count; ++b) { std::string label = village::biome_name(static_cast<village::Biome>(b)); for (char& c : label) c = static_cast<char>(std::tolower(c)); if (label == name) { result.biome = static_cast<village::Biome>(b); found = true; } }
            if (!found) throw std::runtime_error("Biome: temperate, woodland, meadow, arid, taiga, rainforest, wetland, savanna");
        } else if (arg == "--layout") {
            const auto name = value(); bool found = false;
            for (int g = 0; g < village::geography_count; ++g) { std::string label = village::geography_name(static_cast<village::Geography>(g)); for (char& c : label) c = static_cast<char>(std::tolower(c)); if (label == name) { result.geography = static_cast<village::Geography>(g); found = true; } }
            if (!found) throw std::runtime_error("Layout: ponds, river, island, coast, highlands, lakes, peninsula, archipelago, delta, canyon, valley, plains");
        }
        else if (arg == "--steps") result.steps = std::stoi(value());
        else if (arg == "--frames") result.frames = std::stoi(value());
        else if (arg == "--save-dir") result.save_dir = value();
        else if (arg == "--screenshot") result.screenshot = value();
        else if (arg == "--help") {
            std::cout << "Living Village C++ / SDL2\n"
                      << "--fullscreen --seed N --save-dir PATH --new --no-save\n"
                      << "--world-name NAME --biome temperate|woodland|meadow|arid|taiga|rainforest|wetland|savanna\n"
                      << "--layout ponds|river|island|coast|highlands|lakes|peninsula|archipelago|delta|canyon|valley|plains\n"
                      << "--population 2..20 --starting-food 6|18|30 --ai-diagnostics (live panel)\n"
                      << "--register-port-art PORTS_DIRECTORY (no SDL needed)\n"
                      << "--headless --steps N   (no display, SDL or save writes)\n"
                      << "--diagnose-input --frames N --screenshot FILE.ppm\n"
                      << "--profile --benchmark-villagers N (6..20, temporary world, no saves)\n";
            std::exit(0);
        } else throw std::runtime_error("Unknown option " + arg);
    }
    if ((result.benchmark_villagers != 0 && (result.benchmark_villagers < 6 || result.benchmark_villagers > 20)) || result.steps < 1 || result.steps > 100000000 || result.frames < 0) throw std::runtime_error("Steps/frames out of range");
    return result;
}
}

int main(int argc, char** argv) {
    using namespace village;
    try {
        const Options config = options(argc, argv);
        if (!config.art_ports.empty()) { register_port_art(config.art_ports); return 0; }
        World world(config.seed, config.biome, config.geography, config.starting_population); world.world_name = config.world_name; world.food_stock = config.starting_food;
        if (config.benchmark_villagers) {
            world.population_growth = false;
            while (static_cast<int>(world.villagers.size()) < config.benchmark_villagers) {
                Villager p; p.id = world.next_person_id++; p.name = "Benchmark " + std::to_string(p.id); p.brain.initialize(world.seed, p.id);
                p.x = 30 + p.id % 5; p.y = 30 + (p.id / 5) % 5; world.villagers.push_back(p);
            }
        }
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
            std::cout << "Ecology: " << world.animals.size() << " animals, " << world.ecology.animal_births << " births, " << world.ecology.animal_deaths << " deaths, " << world.ecology.predations
                      << " predations, " << world.ecology.trees_born << " new trees, " << world.ecology.plants_died << " plants lost, " << world.ecology.repairs << " repairs, " << world.weather_name() << '\n';
            return 0;
        }
        SaveCatalog catalog(config.save_dir);
        Interface ui; bool ready = config.no_save;
        std::string active_id;
        ui.view.draft_population = config.starting_population; ui.view.draft_seed = config.seed; ui.view.draft_biome = config.biome; ui.view.draft_geography = config.geography; ui.view.draft_name = config.world_name; ui.view.draft_supplies = config.starting_food == 6 ? 0 : config.starting_food == 30 ? 2 : 1;
        if (!config.no_save) {
            active_id = catalog.active();
            if (active_id.empty() && catalog.exists("default")) active_id = "default";
            if (active_id.empty()) { const auto slots = catalog.list(); if (!slots.empty()) active_id = slots.front().id; }
            if (!active_id.empty()) {
                try { bool backup = false; world = catalog.load(active_id, &backup); ready = true; ui.view.status = backup ? "Recovered previous save" : "Village loaded"; }
                catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = "Save could not load - choose or create a world"; ui.view.panel = Panel::Saves; }
            }
            if (config.new_world) {
                try {
                    World fresh(config.seed, config.biome, config.geography, config.starting_population); fresh.world_name = config.world_name; fresh.food_stock = config.starting_food;
                    const auto id = catalog.create(fresh); catalog.activate(id); active_id = id; world = std::move(fresh); ready = true;
                } catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = error.what(); ui.view.panel = Panel::Saves; }
            }
        }
        ui.view.has_world = ready; ui.view.active_slot = ready ? active_id : ""; ui.view.saves = config.no_save ? std::vector<SaveSlot>{} : catalog.list();
        if (!ready && ui.view.panel != Panel::Saves) ui.view.panel = Panel::NewVillage;
        ui.view.speed = world.simulation_speed;
        auto save_path = active_id.empty() ? std::filesystem::path(config.save_dir) / "village.lvsave" : catalog.path(active_id);
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
        Renderer graphics(sdl, renderer.get()); Feedback feedback(sdl);
        feedback.play(Cue::Complete,world.volume);
        if (world.rumble_enabled) input.rumble(24000,90);
        ResourceMeter resource_meter; Uint32 last_ambient = sdl.GetTicks();
        std::uint64_t last_feedback = world.event_sequence;
        Uint32 previous = sdl.GetTicks(), last_save = previous;
        double accumulator = 0;
        int frame_count = 0;
        bool running = true, exit_saved = false; Performance performance; const int starting_battery = config.profile ? battery_percent() : -1;
        if (config.profile) std::cout << "Profile start: residents=" << world.alive_count() << " battery_percent=" << starting_battery << "\n";
        std::cout << std::flush;
        auto save = [&]() {
            last_save = sdl.GetTicks();
            if (config.no_save || !ready) return true; // Failed autosaves retry in 30 seconds, not every frame.
            try { save_atomic(world, save_path); ui.view.status = "Village saved"; return true; }
            catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = "SAVE FAILED - CHECK LOG / FREE SPACE"; return false; }
        };
        auto refresh_saves = [&]() {
            ui.view.saves = catalog.list(); ui.view.active_slot = ready ? active_id : "";
            ui.view.menu_index = std::clamp(ui.view.menu_index, 0, std::max(0, static_cast<int>(ui.view.saves.size()) - 1));
        };
        std::mt19937 creation_rng(static_cast<std::uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
        auto reset_view = [&]() { ui = Interface{}; ui.view.has_world = ready; ui.view.active_slot = active_id; ui.view.speed = world.simulation_speed; accumulator = 0; };
        if (config.ai_diagnostics && ready) { ui.view.panel = Panel::Overview; ui.view.overview_page = 3; }
        while (running && !stop_requested) {
            const auto frame_start = std::chrono::steady_clock::now();
            int frame_steps = 0;
            const Uint32 begin = sdl.GetTicks();
            const double elapsed = std::min((begin - previous) / 1000.0, 0.2);
            previous = begin;
            SDL_Event event;
            while (sdl.PollEvent(&event)) {
                if (event.type == SDL_QUIT) running = false;
                const bool benchmark_focus = config.benchmark_villagers && event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST;
                if (benchmark_focus) std::cout << "Benchmark: ignoring window focus loss in temporary diagnostic world\n";
                if (!benchmark_focus && ((event.type == SDL_APP_WILLENTERBACKGROUND || event.type == SDL_APP_DIDENTERBACKGROUND) || (event.type == SDL_WINDOWEVENT && (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST || event.window.event == SDL_WINDOWEVENT_MINIMIZED)))) {
                    if (ui.view.simulation_running()) { ui.view.panel = Panel::Menu; ui.view.menu_index = 0; }
                    save(); accumulator = 0;
                }
                input.handle(event);
            }
            const auto controls = input.poll();
            const auto request = ui.update(controls, world, elapsed);
            if (request != Request::TestFeedback && request != Request::Volume && std::any_of(controls.pressed.begin(), controls.pressed.end(), [](bool held) { return held; })) feedback.play(Cue::Confirm, world.volume);
            if (request != Request::TestFeedback && world.rumble_enabled && (controls.hit(Button::Confirm) || controls.hit(Button::Back))) input.rumble(22000,45);
            world.simulation_speed = ui.view.speed;
            if (request == Request::TestFeedback) {
                feedback.mute(); feedback.play(Cue::Complete,40); const bool motor = input.rumble(42000,250);
                ui.view.status = std::string("Test: sound ") + (feedback.last_queue_ok ? "queued" : "unavailable") + " / motor " + (motor ? "request sent" : "unavailable");
            } else if (request == Request::Save) save();
            else if (request == Request::Load) {
                if (!ready) { ui.view.panel = Panel::Saves; refresh_saves(); }
                else try { world = load_save(save_path); ui.view.speed = world.simulation_speed; ui.view.status = "Village loaded"; accumulator = 0; }
                catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = "LOAD FAILED - CHECK LOG"; }
            } else if (request == Request::Exit) { if (save()) { running = false; exit_saved = true; } }
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
            } else if (request == Request::Volume) {
                feedback.mute(); world.volume = world.volume >= 100 ? 0 : std::min(100, world.volume + 20);
                if (world.volume == 0) feedback.mute(); else feedback.play(Cue::Confirm, world.volume);
            } else if (request == Request::AudioOutput) {
                if (feedback.next_output()) {
                    feedback.play(Cue::Complete,40);
                    try { write_atomic_text("config/audio-device.txt",feedback.output_name + "\n"); ui.view.status = "Audio output selected - X tests speaker and motor"; }
                    catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = "Output changed for this session; preference could not save"; }
                } else ui.view.status = "Output unavailable - L1 tries next / check audio log";
            } else if (request == Request::ToggleRumble) {
                world.rumble_enabled = !world.rumble_enabled; if (world.rumble_enabled) input.rumble(30000,160); else input.rumble(0, 0);
            } else if (request == Request::ExportHistory) {
                try { export_history(world, catalog.export_directory(active_id.empty() ? "temporary" : active_id)); ui.view.status = "History exported to this world's exports folder"; }
                catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = "EXPORT FAILED - CHECK LOG"; }
            } else if (request == Request::RandomSeed) {
                ui.view.draft_seed = creation_rng();
                if (ui.view.panel == Panel::SeedEdit) { std::ostringstream digits; digits << std::setw(10) << std::setfill('0') << ui.view.draft_seed; ui.view.edit_buffer = digits.str(); }
                ui.view.status = "Random seed chosen";
            } else if (request == Request::OpenSaves) {
                try { refresh_saves(); } catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = error.what(); }
            } else if (request == Request::NewVillage) {
                try {
                    World fresh(ui.view.draft_seed, ui.view.draft_biome, ui.view.draft_geography, ui.view.draft_population); fresh.world_name = ui.view.draft_name;
                    const int starting_food[] = {6, 18, 30}; fresh.food_stock = starting_food[ui.view.draft_supplies];
                    if (!save()) throw std::runtime_error("Current world could not save - creation cancelled");
                    std::string id;
                    if (!config.no_save) { id = catalog.create(fresh); catalog.activate(id); }
                    world = std::move(fresh); active_id = id; ready = true;
                    if (!id.empty()) save_path = catalog.path(id);
                    reset_view(); last_save = sdl.GetTicks(); ui.view.status = "New world created - earlier worlds preserved";
                } catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = error.what(); ui.view.panel = Panel::NewVillage; }
            } else if (request == Request::LoadSlot) {
                try {
                    const auto id = ui.view.saves.at(ui.view.menu_index).id;
                    if (ready && id == active_id) { ui.view.panel = Panel::Map; ui.view.status = "This village is already open"; }
                    else {
                        bool backup = false; World loaded = catalog.load(id, &backup);
                        if (!save()) throw std::runtime_error("Current world could not save - switch cancelled");
                        if (!config.no_save) catalog.activate(id);
                        world = std::move(loaded); active_id = id; ready = true; save_path = catalog.path(id); reset_view();
                        last_save = sdl.GetTicks(); ui.view.status = backup ? "World loaded from backup" : "World loaded";
                    }
                } catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = error.what(); }
            } else if (request == Request::CopyWorld) {
                try {
                    if (config.no_save) throw std::runtime_error("Temporary sessions do not create save copies");
                    if (!save()) throw std::runtime_error("Current world could not save");
                    World copy = world; copy.world_name = world.world_name.substr(0, 15) + " Snapshot";
                    catalog.create(copy); refresh_saves(); ui.view.status = "Independent snapshot saved";
                } catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = error.what(); }
            } else if (request == Request::DeleteSlot) {
                try {
                    if (config.no_save) throw std::runtime_error("Temporary sessions do not delete saves");
                    catalog.erase(ui.view.pending_slot, ready ? active_id : ""); ui.view.status = "Save and its backup deleted";
                } catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = error.what(); }
                ui.view.panel = Panel::Saves; refresh_saves();
            } else if (request == Request::RenameSlot) {
                try {
                    if (config.no_save) throw std::runtime_error("Temporary sessions do not rename saves");
                    if (ready && ui.view.pending_slot == active_id) {
                        const auto previous_name = world.world_name; world.world_name = ui.view.pending_name;
                        if (!save()) { world.world_name = previous_name; throw std::runtime_error("Rename could not save"); }
                    } else catalog.rename(ui.view.pending_slot, ui.view.pending_name);
                    ui.view.status = "World renamed";
                } catch (const std::exception& error) { std::cerr << error.what() << '\n'; ui.view.status = error.what(); }
                refresh_saves();
            } else if (request == Request::Designate) {
                ui.view.status = world.designate(ui.view.selected_tile) ? "Gathering area updated" : "Choose a tree, bush or stone - max 16 areas";
            }
            ui.view.selected = std::clamp(ui.view.selected, 0, static_cast<int>(world.villagers.size()) - 1);
            const int watched_id = world.villagers[ui.view.selected].id;
            if (ui.view.simulation_running()) {
                accumulator += elapsed * ui.view.speed;
                int steps = 0;
                while (accumulator >= fixed_step && steps++ < 8) { ++frame_steps; world.update(); accumulator -= fixed_step; }
            } else accumulator = 0;
            bool found = false;
            for (std::size_t index = 0; index < world.villagers.size(); ++index) if (world.villagers[index].id == watched_id) { ui.view.selected = static_cast<int>(index); found = true; break; }
            if (!found) {
                ui.view.selected = 0;
                for (std::size_t index = 0; index < world.villagers.size(); ++index) if (world.villagers[index].alive()) { ui.view.selected = static_cast<int>(index); break; }
            }
            if (last_feedback != world.event_sequence) {
                last_feedback = world.event_sequence;
                if (!world.chronicle.empty()) {
                    const auto& message = world.chronicle.back().message;
                    if (message.find("Completed:") != std::string::npos || message.find("Improved tools:") != std::string::npos || message.find("Irrigation:") != std::string::npos) { feedback.play(Cue::Complete, world.volume); if (world.rumble_enabled) input.rumble(30000,160); }
                    else if (message.find("is born") != std::string::npos) feedback.play(Cue::Birth, world.volume);
                    else if (message.find("exchange food") != std::string::npos) feedback.play(Cue::Trade, world.volume);
                    else if (message.find("died:") != std::string::npos || message.find("running low") != std::string::npos) feedback.play(Cue::Warning, world.volume);
                }
            }
            if (begin - last_save >= 30000) save();
            if (ui.view.simulation_running() && world.volume > 0 && begin - last_ambient >= 6000) {
                feedback.play(world.raining() ? Cue::Rain : world.daylight() > .35 ? Cue::Birds : Cue::Fire,world.volume); last_ambient = begin;
            }
            resource_meter.sample(std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count());
            ui.view.cpu_percent = resource_meter.cpu_percent; ui.view.ram_mib = resource_meter.ram_mib;
            ui.view.audio_output = feedback.output_name.empty() ? "Default system output" : feedback.output_name; ui.view.audio_status = feedback.status; ui.view.rumble_status = input.rumble_status;
            graphics.draw(world, ui.view);
            ++frame_count;
            if (config.frames > 0 && frame_count >= config.frames) {
                if (!config.screenshot.empty()) graphics.screenshot(config.screenshot);
                running = false;
            }
            sdl.RenderPresent(renderer.get());
            const Uint32 render_time = sdl.GetTicks() - begin;
            const auto work_finished = std::chrono::steady_clock::now();
            if (render_time < 33) sdl.Delay(33 - render_time);
            if (config.profile) {
                performance.frame(std::chrono::duration<double, std::milli>(work_finished - frame_start).count(), std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - frame_start).count(), frame_steps);
                if (frame_count % 300 == 0) std::cout << performance.report() << '\n' << std::flush;
            }
        }
        const bool saved = exit_saved || save();
        if (config.profile) std::cout << performance.report() << " battery_start=" << starting_battery << " battery_end=" << battery_percent() << "\n";
        std::cout << "Exited cleanly after " << frame_count << " frames\n";
        return saved ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Living Village: " << error.what() << '\n';
        return 1;
    }
}
