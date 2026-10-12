#include "living_village/world.hpp"
#include <algorithm>
#include <cmath>
namespace village {
bool World::toggle_pin(int id) {
    auto found = std::find(pinned_residents.begin(), pinned_residents.end(), id);
    if (found != pinned_residents.end()) { pinned_residents.erase(found); return true; }
    if (!person(id) || pinned_residents.size() >= 6) return false;
    pinned_residents.push_back(id); return true;
}
bool World::teach(Villager& teacher, Villager& learner) {
    if (!teaching_enabled || teacher.id == learner.id || !teacher.alive() || !learner.alive()
        || teacher.child() || learner.age < 6 || learner.last_lesson_day == day()
        || std::abs(teacher.x-learner.x)+std::abs(teacher.y-learner.y) > 2
        || teacher.hunger >= 60 || learner.hunger >= 60 || teacher.energy < 50 || learner.energy < 50
        || teacher.relationships[learner.id] < 0 || learner.relationships[teacher.id] < 0
        || teacher.cooperation < .5) return false;
    double* source[] = {&teacher.gather_skill, &teacher.build_skill, &teacher.farm_skill};
    double* target[] = {&learner.gather_skill, &learner.build_skill, &learner.farm_skill};
    int skill = -1; double gap = .1;
    for (int i = 0; i < 3; ++i) if (*source[i] - *target[i] > gap) { gap = *source[i]-*target[i]; skill = i; }
    if (skill < 0) return false;
    const char* names[] = {"gathering", "building", "farming"};
    *target[skill] = std::min(*source[skill], *target[skill] + std::min(.005, gap * .02));
    teacher.energy -= .5; learner.energy -= .3;
    learner.last_lesson_day = day(); ++lessons;
    remember(teacher, "Taught " + std::string(names[skill]) + " to " + learner.name);
    remember(learner, "Learned " + std::string(names[skill]) + " from " + teacher.name);
    event(teacher.name + " teaches " + learner.name + " " + names[skill], "Completed nearby social encounter; skill gap, trust, cooperation and spare energy", {static_cast<int>(learner.x),static_cast<int>(learner.y)});
    return true;
}
}
