#include "living_village/world.hpp"
#include "living_village/ui.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace village;
void check(bool ok,const char* why) { if(!ok) throw std::runtime_error(why); }
int main() {
    try {
        for (int count : {2,6,12,20}) {
            World w(123,Biome::Temperate,Geography::Ponds,count);
            check(w.alive_count()==count && w.next_person_id==count && w.statistics.front().people==count,"Starting population or identities invalid");
            check(World::deserialize(w.serialize()).serialize()==w.serialize(),"Nondefault population save did not roundtrip");
            for(int step=0;step<2000;++step) w.update();
            check(w.alive_count()==count,"Starting population could not survive first session");
        }
        for (int bad : {-1,0,1,21}) { bool rejected=false; try { World w(1,Biome::Temperate,Geography::Ponds,bad); } catch(...) { rejected=true; } check(rejected,"Invalid population accepted"); }
        World w(123); int visible=0;
        for(const auto& a:w.animals) visible += std::abs(a.x-32)<=12 && std::abs(a.y-32)<=6;
        check(visible>=4,"No wildlife in initial camera area");
        Interface ui; ui.view.panel=Panel::Overview; ui.view.overview_page=7; InputState a; a.pressed[static_cast<int>(Button::Confirm)]=true;
        ui.update(a,w,.03); check(ui.view.panel==Panel::Map && ui.view.watched_animal_id==w.animals[0].id && ui.view.camera_x==w.animals[0].x,"Controller cannot watch wildlife");
        InputState movement; movement.x=1; ui.update(movement,w,.03); check(ui.view.watched_animal_id<0,"Camera did not release wildlife");
        ui.view.panel=Panel::NewVillage; ui.view.menu_index=5; ui.view.draft_population=20; ui.update(a,w,.03); check(ui.view.draft_population==2,"Population control failed to wrap");
        ui.view.panel=Panel::Overview; ui.view.overview_page=5; InputState x; x.pressed[static_cast<int>(Button::Inspect)]=true;
        check(ui.update(x,w,.03)==Request::TestFeedback,"Sound/motor test not reachable");
        InputState l1; l1.pressed[static_cast<int>(Button::Previous)]=true; check(ui.update(l1,w,.03)==Request::AudioOutput,"Audio routing not reachable via controller");
        World social(1); social.auto_develop=social.population_growth=social.ecology_enabled=false; social.villagers.resize(2);
        for(auto& p:social.villagers) { p.x=p.y=32; p.hunger=20; p.energy=80; p.cooperation=.8; p.goal=Goal::Socialize; p.decision_timer=9; p.action_timer=3; p.known_food.clear(); p.known_wood.clear(); p.known_stone.clear(); }
        social.tile(5,5)={Terrain::Bush,6,0}; social.villagers[0].known_food.push_back({5,5}); social.villagers[0].target_id=1; social.villagers[1].target_id=-1;
        social.update(); check(std::find(social.villagers[1].known_food.begin(),social.villagers[1].known_food.end(),Point{5,5})!=social.villagers[1].known_food.end(),"Meeting did not convey limited resource memory");
        check(social.food_stock==18 && social.villagers[1].food==0,"Knowledge sharing duplicated resources");
        World distant=social; distant.villagers[1].known_food.clear(); distant.villagers[1].x=50; distant.villagers[0].action_timer=3; distant.villagers[0].goal=Goal::Socialize;
        distant.update(); check(distant.villagers[1].known_food.empty(),"Resource information teleported across map");
        World habitat(4); habitat.animals.clear(); habitat.weather=Weather::Clear;
        for(auto& t:habitat.tiles) { t.terrain=Terrain::Grass; t.moisture=.6; t.biomass=.8; t.nectar=.4; t.fire=0; t.plant_health=100; }
        for(int i=0;i<40 && habitat.animals.empty();++i) habitat.recruit_wildlife();
        check(!habitat.animals.empty(),"Suitable habitat could not be recolonized");
        World hostile(4); hostile.animals.clear(); for(auto& t:hostile.tiles) t.terrain=Terrain::Water;
        for(int i=0;i<40;++i) hostile.recruit_wildlife();
        check(hostile.animals.empty(),"Wildlife appeared in unsuitable habitat");
        std::cout<<"PASS: starting population/survival/IDs/saves, visible wildlife/camera, habitat-gated recruitment, physical knowledge sharing and feedback controls\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
