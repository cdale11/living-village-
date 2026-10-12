#pragma once
#include "sdl.hpp"
#include "world.hpp"
#include "saves.hpp"
#include <memory>

namespace village {
enum class Panel { Map, Menu, Inspector, Overview, Speed, Controls, Tile, Build, NewVillage, SeedEdit, NameEdit, ConfirmCreate, Saves, ConfirmDelete };
struct View {
    double camera_x = 32, camera_y = 32;
    int tile_size = 28, selected = 0, menu_index = 0, speed = 1;
    bool simulation_running() const { return has_world && (panel == Panel::Map || (panel == Panel::Overview && (overview_page == 3 || overview_page == 8))); }
    bool follow = false;
    int inspector_page = 0;
    int overview_page = 0;
    int selected_animal = 0;
    Point selected_tile{32, 32};
    Panel panel = Panel::Map;
    bool has_world = true;
    std::string active_slot, pending_slot, pending_name;
    std::vector<SaveSlot> saves;
    std::uint32_t draft_seed = 20261011;
    Biome draft_biome = Biome::Temperate;
    Geography draft_geography = Geography::Ponds;
    std::string draft_name = "New Village", edit_buffer;
    int edit_cursor = 0;
    int draft_supplies = 1;
    bool renaming = false;
    std::string status = "A living village begins";
};
class Renderer {
public:
    Renderer(SDLApi& sdl, SDL_Renderer* renderer) : sdl(sdl), renderer(renderer) {}
    void draw(const World& world, const View& view);
    void screenshot(const std::string& filename);
private:
    std::unique_ptr<World> preview;
    SDLApi& sdl;
    SDL_Renderer* renderer;
    void rect(int x, int y, int w, int h, SDL_Color color);
    void outline(int x, int y, int w, int h, SDL_Color color);
    void text(int x, int y, const std::string& text, SDL_Color color, int scale = 2, int width = 700);
    void meter(int x, int y, const std::string& label, double value, SDL_Color color);
};
}
