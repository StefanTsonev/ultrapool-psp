#include "ultrapool/core.hpp"
#include <algorithm>
namespace up {
Pocket* World::pocket(unsigned id){return id<6?&pockets[id]:(id>=7 && id<7+hole_count)?&holes[id-7]:nullptr;}
const Pocket* World::pocket(unsigned id)const{return id<6?&pockets[id]:(id>=7 && id<7+hole_count)?&holes[id-7]:nullptr;}
bool World::add_hole(Vec position,float multiplier){
    if(hole_count>=max_holes || !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(multiplier))return false;
    Pocket h;h.p=position;h.multiplier=multiplier;h.closeable=false;h.scale=0;holes[hole_count++]=h;return true;
}
bool World::pockets_animating()const{
    for(unsigned i=0;i<pocket_count();++i){const Pocket& p=pocket_at(i);if(p.scale<1 || p.suck_scale>1)return true;}return false;
}
void World::update_pockets(float dt){
    if(!(dt>0) || !std::isfinite(dt))return;
    for(unsigned i=0;i<pocket_count();++i){Pocket& p=pocket_at(i);
        if(p.scale<1){p.scale+=(1-p.scale)*std::min(1.0f,12*dt);if(p.scale>.95f)p.scale=1;}
        // Original Vector2.move_toward(ONE, dt*5) reduces each equal component by 5/sqrt(2).
        if(p.suck_scale>1)p.suck_scale=std::max(1.0f,p.suck_scale-dt*3.535533906f);
    }
}
unsigned Game::random_pocket_id(){return world.pocket_id(random()%world.pocket_count());}
bool Game::spawn_hole(unsigned level){
    if(cap_rewarded || phase==Phase::Shop || world.hole_count>=max_holes)return false;
    for(unsigned attempt=0;attempt<1024;++attempt){
        Vec position(-208+416*(random()/4294967296.0f),-356+716*(random()/4294967296.0f));bool clear=true;
        for(unsigned i=0;i<world.count && clear;++i)if(world.balls[i].active && !world.balls[i].passive && !world.balls[i].energy && (world.balls[i].p-position).length()<40)clear=false;
        if(clear)return world.add_hole(position,float(level+1));
    }
    ++spawn_rejections;notice("No free black-hole position");return false;
}
bool Game::queue_pocket_suck(unsigned id,unsigned level,Vec origin){
    const Pocket* p=world.pocket(id);if(cap_rewarded || !p || p->closed)return false;
    Action& a=allocate_action();start_wisp(a,ActionKind::PocketChange,origin,p->p);a.target=id;a.flags=32;a.multiplier=2+level;return true;
}
}
