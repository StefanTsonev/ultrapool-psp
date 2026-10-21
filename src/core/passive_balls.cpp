#include "ultrapool/core.hpp"
namespace up {
Vec Game::passive_position(unsigned slot){
    // Table's ordered passive spot coordinates, including the parent offset.
    static const Vec positions[8]={Vec(-368,25),Vec(-368,-25),Vec(-400,0),Vec(-432,25),Vec(-432,-25),Vec(-464,0),Vec(-496,25),Vec(-496,-25)};
    return positions[slot<8?slot:7];
}
unsigned Game::passive_slot(unsigned body)const{
    if(body>=world.count)return 8;
    for(unsigned i=0;i<passive_count;++i)if(passive_balls[i].index==body && passive_balls[i].generation==world.balls[body].generation)return i;
    return 8;
}
bool Game::queue_passive_copy(unsigned source,Vec origin){
    if(cap_rewarded || source>=world.count || passive_count>=passive_balls.size())return false;
    const Ball& b=world.balls[source];
    Action& a=allocate_action();start_wisp(a,ActionKind::Spawn,origin,passive_position(passive_count));
    a.snapshot=b;a.snapshot.owner=-1;a.snapshot.base_value=b.def.base_score;a.snapshot.temp_value=0;
    a.flags=16;a.target=Jellyfish;a.impulse=origin;return true;
}
bool Game::land_passive(unsigned body){
    if(body>=world.count || passive_count>=passive_balls.size())return false;
    Ball& b=world.balls[body];b.passive=true;b.collision_disabled=true;b.flash=.5f;
    EntityRef ref;ref.kind=EntityKind::Ball;ref.index=static_cast<uint16_t>(body);ref.generation=b.generation;passive_balls[passive_count++]=ref;
    for(unsigned i=0;i<passive_count;++i){const EntityRef& item=passive_balls[i];if(item.index<world.count && world.balls[item.index].generation==item.generation && world.balls[item.index].active)world.balls[item.index].p=passive_position(i);}
    clear_pickup_contacts(body);effect(b.p,0xff8fd8a2,30,true);return true;
}
void Game::remove_passive(unsigned body){
    unsigned slot=passive_slot(body);if(slot>=passive_count)return;
    for(unsigned i=slot+1;i<passive_count;++i)passive_balls[i-1]=passive_balls[i];
    passive_balls[--passive_count]=EntityRef();
    for(unsigned i=0;i<passive_count;++i){const EntityRef& item=passive_balls[i];if(item.index<world.count && world.balls[item.index].generation==item.generation && world.balls[item.index].active)world.balls[item.index].p=passive_position(i);}
}
}
