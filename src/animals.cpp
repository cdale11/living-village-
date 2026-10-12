#include "living_village/world.hpp"
#include <algorithm>
#include <cmath>

namespace village {
namespace {
struct Biology { double metabolism, speed, adult, lifespan, pregnancy, heat; };
Biology biology(Species species) {
    switch (species) {
        case Species::Rabbit: return {.10, 1.5, 12, 240, 5, 38};
        case Species::Deer: return {.07, 1.2, 50, 800, 18, 38};
        case Species::Fox: return {.09, 1.7, 30, 400, 10, 38};
        case Species::Bee: return {.06, 1.8, 8, 300, 12, 30}; // Entity represents a colony's mobile foragers.
    }
    return {.1, 1, 10, 200, 8, 38};
}
Point location(const Animal& a) { return {static_cast<int>(std::round(a.x)), static_cast<int>(std::round(a.y))}; }
double distance(double x, double y, Point p) { return std::abs(x - p.x) + std::abs(y - p.y); }
BrainInput animal_observations(const Animal& a, AnimalAction action, const Tile& t, double trip, double utility, double temperature) {
    BrainInput input{}; input[static_cast<int>(action)] = 1;
    const std::array<double, brain_context> context{{a.hunger / 100, a.energy / 100, a.health / 100, a.thirst / 100,
        a.metabolism / 1.5, a.immunity, a.fertility, a.mass, a.disease, t.biomass, t.moisture, t.surface_water,
        t.pollination, t.compaction, static_cast<int>(a.species) / 3.0, (temperature + 20) / 70, trip / 16, utility / 300}};
    for (int n = 0; n < brain_context; ++n) input[brain_actions + n] = std::clamp(context[n], 0.0, 1.0);
    return input;
}
}
const char* species_name(Species species) {
    const char* names[] = {"Rabbit", "Deer", "Fox", "Bee colony"}; return names[static_cast<int>(species)];
}
const char* animal_action_name(AnimalAction action) {
    const char* names[] = {"Exploring", "Feeding", "Drinking", "Resting", "Fleeing", "Hunting", "Seeking mate"}; return names[static_cast<int>(action)];
}
bool World::spawn_animal(Species species, Point p, int parent) {
    if (static_cast<int>(species) < 0 || static_cast<int>(species) > 3 || animals.size() >= animal_limit || !passable(p.x, p.y) || next_animal_id >= 1000000000) return false;
    animals.reserve(animal_limit);
    Animal animal; animal.id = next_animal_id++; animal.species = species; animal.parent = parent; animal.x = p.x; animal.y = p.y; animal.destination = p;
    animal.female = eco_random(2) == 0; animal.metabolism = .8 + eco_random(41) / 100.0;
    animal.immunity = .3 + eco_random(41) / 100.0; animal.fertility = .3 + eco_random(41) / 100.0;
    animal.age = parent < 0 ? biology(species).adult + eco_random(20) : 0;
    animal.mass = parent < 0 ? .8 : .15; animal.body_temperature = biology(species).heat;
    animal.brain.initialize(seed ^ 0xa11a1U, animal.id);
    animals.push_back(animal); return true;
}
void World::recruit_wildlife() {
    // Regional dispersal: a scarce species can recolonize a suitable boundary
    // habitat. Weather, local forage and prey determine opportunity, not a scene.
    if (animals.size() >= animal_limit || weather == Weather::Storm || weather == Weather::Heatwave) return;
    std::array<int, 4> counts{}; for (const auto& a : animals) ++counts[static_cast<int>(a.species)];
    for (int species = 0; species < 4; ++species) {
        if (counts[species] >= (species == 2 ? 2 : 4) || eco_random(4) != 0) continue;
        for (int attempt = 0; attempt < 16; ++attempt) {
            const int side = eco_random(4), edge = 2 + eco_random(60);
            Point p = side == 0 ? Point{2,edge} : side == 1 ? Point{61,edge} : side == 2 ? Point{edge,2} : Point{edge,61};
            if (!passable(p.x,p.y)) continue;
            const auto& t = tile(p.x,p.y);
            const bool habitat = t.fire == 0 && t.plant_health > 50 && t.moisture > .18
                && (species == 2 ? counts[0] + counts[1] >= 4 : species == 3 ? t.nectar > .2 : t.biomass > .4);
            if (habitat && spawn_animal(static_cast<Species>(species), p)) {
                event(std::string(species_name(static_cast<Species>(species))) + " arrives in suitable boundary habitat"); break;
            }
        }
    }
}
void World::update_animals(double seconds) {
    const std::size_t initial_count = animals.size();
    // Reserve once to keep parents/references valid when offspring are appended.
    animals.reserve(animal_limit);
    auto die = [&](Animal& a, const char* cause) {
        if (a.health <= 0) return;
        a.health = 0; ++ecology.animal_deaths;
        auto& ground = tile(location(a).x, location(a).y);
        ground.organic = std::min(1.0, ground.organic + a.mass * .2);
        // No instant meat stock: carcasses feed scavengers and soil locally.
        if (a.brain.active && learning_enabled) a.brain.observe(a.brain.pending, -1);
        a.brain.active = false;
        if (ecology.animal_deaths % 8 == 1) event(std::string(species_name(a.species)) + " died: " + cause, std::string("Recorded death condition: ") + cause, location(a));
    };
    for (std::size_t index = 0; index < initial_count; ++index) {
        auto& a = animals[index]; if (a.health <= 0) continue;
        const auto traits = biology(a.species); const auto here = location(a); auto& cell = tile(here.x, here.y);
        a.age += seconds / 240;
        a.mass = std::min(1.0, a.mass + seconds * .00008 * (a.hunger < 70 ? 1 : 0));
        const double metabolism = traits.metabolism * a.metabolism * (.6 + .4 * a.mass);
        a.hunger = std::min(100.0, a.hunger + seconds * metabolism);
        a.thirst = std::min(100.0, a.thirst + seconds * metabolism * (.6 + std::max(0.0, temperature - 20) / 30));
        a.energy = std::max(0.0, a.energy - seconds * .04);
        const double shade = cell.terrain == Terrain::Tree ? 5 : 0;
        const double exposed = temperature - shade;
        a.body_temperature = std::clamp(a.body_temperature + seconds * ((traits.heat - a.body_temperature) * .08 + (exposed - 20) * .002), 20.0, 45.0);
        a.disease = std::clamp(a.disease + seconds * ((cell.disease + (cell.surface_water > .4 ? .1 : 0)) * .0004 - a.immunity * .00015), 0.0, 1.0);
        for (const auto& other : animals) if (other.id != a.id && other.health > 0 && other.species == a.species && distance(a.x, a.y, location(other)) <= 2)
            a.disease = std::min(1.0, a.disease + seconds * other.disease * .00015 * (1 - a.immunity));
        cell.disease = std::min(1.0, cell.disease + seconds * a.disease * .00003);
        const double damage = (a.hunger >= 99 ? .12 : 0) + (a.thirst >= 99 ? .18 : 0) + a.disease * .015 + cell.fire * .4
            + std::max(0.0, std::abs(a.body_temperature - traits.heat) - 3) * .02;
        if (damage > 0) {
            const double next = a.health - seconds * damage;
            if (next <= 0) { die(a, cell.fire > 0 ? "Fire" : a.thirst >= 99 ? "Thirst" : a.hunger >= 99 ? "Hunger" : "Illness"); continue; }
            a.health = next;
        } else a.health = std::min(100.0, a.health + seconds * .025 * a.immunity);
        if (a.age >= traits.lifespan) { die(a, "Old age"); continue; }
        if (a.gestation > 0) {
            a.gestation = std::max(0.0, a.gestation - seconds / 240);
            if (a.gestation == 0 && a.health > 40 && a.hunger < 80) {
                const int litter = a.species == Species::Rabbit ? 2 : 1;
                int born = 0;
                for (int n = 0; n < litter && animals.size() < animal_limit; ++n) if (spawn_animal(a.species, here, a.id)) {
                    auto& child = animals.back();
                    child.metabolism = std::clamp((a.metabolism + a.mate_metabolism) / 2 + (eco_random(11) - 5) / 100.0, .7, 1.3);
                    child.immunity = std::clamp((a.immunity + a.mate_immunity) / 2 + (eco_random(11) - 5) / 100.0, .1, .9);
                    child.fertility = std::clamp((a.fertility + a.mate_fertility) / 2 + (eco_random(11) - 5) / 100.0, .1, .9);
                    child.disease = a.disease * .2; ++ecology.animal_births; ++born;
                }
                a.last_birth_day = day(); a.hunger = std::min(100.0, a.hunger + born * 12);
                if (born) event(std::string(species_name(a.species)) + " offspring born", "Completed gestation; offspring placed near parent #" + std::to_string(a.id), location(a));
            }
        }
        a.decision_timer = std::max(0.0, a.decision_timer - seconds);
        if (a.decision_timer <= 0) {
            if (a.brain.active && learning_enabled) {
                const double outcome = .7 * (a.brain.start_hunger - a.hunger) / 45 + .6 * (a.brain.start_loneliness - a.thirst) / 45
                    + .4 * (a.energy - a.brain.start_energy) / 40 + (a.health - a.brain.start_health) / 20 - .02;
                a.brain.observe(a.brain.pending, outcome);
            }
            a.action = AnimalAction::Wander; a.target_id = -1; a.destination = here; a.reason = "Exploring nearby habitat";
            BrainInput chosen = animal_observations(a, a.action, cell, 0, 20, temperature);
            double base = 20, adjustment = 10 * a.brain.evaluate(chosen), score = base + adjustment;
            auto consider = [&](AnimalAction action, Point p, double utility, const char* reason, int target = -1) {
                if (!passable(p.x, p.y)) return;
                const double trip = distance(a.x, a.y, p);
                if (utility + 10 - trip * 2 <= score) return;
                const auto input = animal_observations(a, action, tile(p.x, p.y), trip, utility, temperature);
                const double bonus = 10 * a.brain.evaluate(input), value = utility + bonus - trip * 2;
                if (value <= score) return;
                // Local wildlife never launches full-map BFS. Check a bounded route
                // only for the final candidate via greedy one-cell movement below.
                score = value; base = utility; adjustment = bonus; chosen = input;
                a.action = action; a.destination = p; a.target_id = target; a.reason = reason;
            };
            consider(AnimalAction::Rest, here, (100 - a.energy) * 1.5, "Energy low; resting in habitat");
            for (int y = std::max(0, here.y - 4); y <= std::min(63, here.y + 4); ++y)
                for (int x = std::max(0, here.x - 4); x <= std::min(63, here.x + 4); ++x) {
                    const auto& t = tile(x, y); const Point p{x, y};
                    if (t.terrain == Terrain::Water) {
                        for (Point d : {Point{1, 0}, Point{-1, 0}, Point{0, 1}, Point{0, -1}})
                            consider(AnimalAction::Drink, {x + d.x, y + d.y}, a.thirst * 1.6, "Thirst; drinking at a water edge");
                    } else if (t.surface_water >= .01 || t.moisture > .3)
                        consider(AnimalAction::Drink, p, a.thirst * (t.surface_water >= .01 ? 1.6 : 1.2), t.surface_water >= .01 ? "Thirst; puddle water" : "Thirst; moisture in plants");
                    if (a.species == Species::Rabbit && (t.biomass > .15 || t.food > 0)) consider(AnimalAction::Feed, p, a.hunger * 1.5, "Hungry; grazing vegetation");
                    if (a.species == Species::Deer && (t.biomass > .2 || t.food > 0)) consider(AnimalAction::Feed, p, a.hunger * 1.5, "Hungry; browsing foliage");
                    if (a.species == Species::Bee && t.terrain == Terrain::Bush && t.plant_health > 20 && t.nectar > .01) consider(AnimalAction::Feed, p, a.hunger * 1.6, "Nectar available; pollinating flowers");
                    if (t.terrain == Terrain::Tree && a.energy < 50) consider(AnimalAction::Rest, p, (100 - a.energy) * 1.5 + 8, "Resting in forest shade");
                }
            for (const auto& other : animals) {
                if (other.id == a.id || other.health <= 0) continue;
                const Point p = location(other); const double trip = distance(a.x, a.y, p);
                if (trip > 7) continue;
                if (a.species == Species::Fox && other.species == Species::Rabbit) consider(AnimalAction::Hunt, p, a.hunger * 1.5 + 12, "Hungry; stalking a nearby rabbit", other.id);
                if ((a.species == Species::Rabbit || a.species == Species::Deer) && other.species == Species::Fox && trip < 4) {
                    const Point flee{std::clamp(here.x + (here.x >= p.x ? 3 : -3), 0, 63), std::clamp(here.y + (here.y >= p.y ? 3 : -3), 0, 63)};
                    consider(AnimalAction::Flee, flee, 160 - trip * 10, "Predator nearby; fleeing", other.id);
                }
                if (other.species == a.species && other.female != a.female && a.age >= traits.adult && other.age >= traits.adult && a.gestation == 0
                    && other.gestation == 0 && a.hunger < 50 && a.thirst < 50 && a.health > 70 && other.health > 70 && a.parent != other.id && other.parent != a.id
                    && (a.parent < 0 || a.parent != other.parent) && day() - a.last_birth_day > 15 && day() - other.last_birth_day > 15 && animals.size() < animal_limit)
                    consider(AnimalAction::Mate, p, 35 + a.fertility * 40, "Healthy adult; seeking compatible mate", other.id);
            }
            if (cell.fire > 0 || cell.surface_water > .45) {
                for (Point d : {Point{1, 0}, Point{-1, 0}, Point{0, 1}, Point{0, -1}}) {
                    Point p{here.x + d.x, here.y + d.y};
                    if (passable(p.x, p.y) && (tile(p.x, p.y).fire < cell.fire || tile(p.x, p.y).surface_water < cell.surface_water)) consider(AnimalAction::Flee, p, 210, "Escaping fire or flood");
                }
            }
            if (a.action == AnimalAction::Wander) for (int attempt = 0; attempt < 8; ++attempt) {
                Point p{std::clamp(here.x + eco_random(7) - 3, 0, 63), std::clamp(here.y + eco_random(7) - 3, 0, 63)};
                if (passable(p.x, p.y)) { a.destination = p; break; }
            }
            a.brain.pending = chosen; a.brain.action = static_cast<int>(a.action); a.brain.active = true;
            a.brain.start_hunger = a.hunger; a.brain.start_energy = a.energy; a.brain.start_health = a.health; a.brain.start_loneliness = a.thirst;
            a.brain.selected_base = base; a.brain.selected_adjustment = adjustment;
            a.decision_timer = 8 + (a.id % 4); a.action_timer = 0;
        }
        // Refresh moving prey/mate coordinates using stable IDs, never vector indices.
        if (a.target_id >= 0 && (a.action == AnimalAction::Hunt || a.action == AnimalAction::Mate))
            for (const auto& other : animals) if (other.id == a.target_id && other.health > 0) { a.destination = location(other); break; }
        const double dx = a.destination.x - a.x, dy = a.destination.y - a.y;
        if (std::hypot(dx, dy) > .2) {
            // Cardinal steps keep land animals out of water even on diagonal trips.
            double mx = 0, my = 0; const double speed = traits.speed * seconds * (a.energy < 20 ? .5 : 1);
            if (std::abs(dx) >= std::abs(dy) && passable(here.x + (dx > 0 ? 1 : -1), here.y)) mx = std::clamp(dx, -speed, speed);
            else if (std::abs(dy) > .1 && passable(here.x, here.y + (dy > 0 ? 1 : -1))) my = std::clamp(dy, -speed, speed);
            else if (std::abs(dx) > .1 && passable(here.x + (dx > 0 ? 1 : -1), here.y)) mx = std::clamp(dx, -speed, speed);
            // At most one tile per ecology update; no skipping across channels.
            a.x = std::clamp(a.x + std::clamp(mx, -1.0, 1.0), 0.0, 63.0); a.y = std::clamp(a.y + std::clamp(my, -1.0, 1.0), 0.0, 63.0);
            if (a.species != Species::Bee) trample(location(a), .15);
            continue;
        }
        auto& habitat = tile(location(a).x, location(a).y); a.action_timer += seconds;
        if (a.action == AnimalAction::Rest) a.energy = std::min(100.0, a.energy + seconds * 2);
        else if (a.action == AnimalAction::Drink && a.action_timer >= 2) {
            bool shore = false;
            for (Point d : {Point{1, 0}, Point{-1, 0}, Point{0, 1}, Point{0, -1}}) {
                const int x = here.x + d.x, y = here.y + d.y;
                if (x >= 0 && y >= 0 && x < 64 && y < 64 && tile(x, y).terrain == Terrain::Water) shore = true;
            }
            if (shore) a.thirst = std::max(0.0, a.thirst - 55);
            else if (habitat.surface_water >= .01) { habitat.surface_water -= .01; ecology.water_used += .01; a.thirst = std::max(0.0, a.thirst - 45); }
            else if (habitat.moisture > .3) { habitat.moisture -= .001; ecology.water_used += .001; a.thirst = std::max(0.0, a.thirst - 20); }
            a.decision_timer = 0;
        } else if (a.action == AnimalAction::Feed && a.action_timer >= 3) {
            double meal = 0;
            if (a.species == Species::Bee && habitat.terrain == Terrain::Bush && habitat.plant_health > 20 && habitat.nectar > .01) {
                const double nectar = std::min(.04, habitat.nectar); habitat.nectar -= nectar;
                habitat.pollination = std::min(1.0, habitat.pollination + .08); meal = nectar * 625;
            } else if (habitat.food > 0) { --habitat.food; meal = 40; habitat.bush_seeds = std::min(1.0, habitat.bush_seeds + .02); }
            else if (habitat.biomass > .1 && habitat.terrain != Terrain::Water && habitat.terrain != Terrain::Stone) {
                const double bite = std::min(.025, habitat.biomass); habitat.biomass -= bite; meal = bite * 1000;
                if (habitat.terrain == Terrain::Tree && a.species == Species::Deer && habitat.plant_age < 20) habitat.plant_health = std::max(0.0, habitat.plant_health - 2);
            }
            a.hunger = std::max(0.0, a.hunger - meal); habitat.organic = std::min(1.0, habitat.organic + meal * .0002); a.decision_timer = 0;
        } else if (a.action == AnimalAction::Hunt && a.action_timer >= 2) {
            for (auto& prey : animals) if (prey.id == a.target_id && prey.species == Species::Rabbit && prey.health > 0 && distance(a.x, a.y, location(prey)) < 1.5) {
                a.hunger = std::max(0.0, a.hunger - 65 * (.5 + prey.mass)); die(prey, "Predation"); ++ecology.predations; break;
            }
            a.decision_timer = 0;
        } else if (a.action == AnimalAction::Mate && a.action_timer >= 3) {
            for (auto& other : animals) if (other.id == a.target_id && other.health > 70 && other.species == a.species && other.female != a.female && distance(a.x, a.y, location(other)) <= 1) {
                Animal& mother = a.female ? a : other; Animal& father = a.female ? other : a;
                if (mother.gestation == 0 && mother.hunger < 60 && mother.thirst < 60 && father.hunger < 60 && father.thirst < 60 && animals.size() < animal_limit) {
                    mother.gestation = traits.pregnancy; mother.mate_metabolism = father.metabolism; mother.mate_immunity = father.immunity; mother.mate_fertility = father.fertility;
                    mother.last_birth_day = father.last_birth_day = day();
                }
                break;
            }
            a.decision_timer = 0;
        }
    }
    animals.erase(std::remove_if(animals.begin(), animals.end(), [](const Animal& a) { return a.health <= 0; }), animals.end());
    // Rare recolonization is conditional on habitat and an actual land boundary.
    // Islands have no artificial spontaneous mammal arrivals.
    if (ticks % 2400 == 0 && day() % 8 == 0 && animals.size() < animal_limit / 2) {
        Point p{eco_random(64), eco_random(2) ? 0 : 63};
        if (passable(p.x, p.y) && tile(p.x, p.y).biomass > .4 && tile(p.x, p.y).moisture > .2) spawn_animal(static_cast<Species>(eco_random(4)), p);
    }
}
} // namespace village
