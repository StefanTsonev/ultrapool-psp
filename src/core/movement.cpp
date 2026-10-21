#include "ultrapool/core.hpp"
#include <algorithm>
namespace up {
namespace {
Vec unit_or_zero(Vec v){return v.dot(v)>1e-16f?v.unit():Vec();}
}
void Game::update_pursuit() {
    if(cap_rewarded)return;
    for(unsigned i=1;i<world.count;++i){
        Ball& b=world.balls[i];if(!b.active || b.falling || b.energy)continue;
        const bool prior_motion=b.chase_flowers>0 || b.snake_time>0 || b.rocket_time>0 || b.chase_pocket_time>0 || b.conveyor_time>0;
        Vec force;
        if(b.chase_flowers>0){
            b.chase_flowers=std::max(0.0f,b.chase_flowers-tick_dt);
            float best=0;Vec delta;bool found=false;
            for(const auto& p:pickups)if(p.active && p.kind==PickupKind::Flower){Vec d=p.p-b.p;float distance=d.dot(d);if(!found || distance<best){found=true;best=distance;delta=d;}}
            if(found)force=unit_or_zero(delta)*800;
        }else if(b.snake_time>0){
            b.snake_time=std::max(0.0f,b.snake_time-tick_dt);
            Vec away;
            for(unsigned ordinal=0;ordinal<world.pocket_count();++ordinal){const Pocket& p=world.pocket_at(ordinal);Vec delta=b.p-p.p;float squared=delta.dot(delta);if(squared>1e-16f && squared<40000)away+=delta*(1/squared);}
            float rotation=((random()/4294967296.0f)*2-1)*3.141592654f*tick_dt*30;
            float c=std::cos(rotation),s=std::sin(rotation);Vec heading=b.snake_direction;
            b.snake_direction=Vec(heading.x*c-heading.y*s,heading.x*s+heading.y*c);
            force=unit_or_zero(b.snake_direction+unit_or_zero(away))*1000;
        }else if(b.rocket_time>0){
            b.rocket_time=std::max(0.0f,b.rocket_time-tick_dt);force=unit_or_zero(b.v)*2000;
        }else if(b.chase_pocket_time>0){
            b.chase_pocket_time=std::max(0.0f,b.chase_pocket_time-tick_dt);
            int nearest=-1;float best=0;
            for(unsigned ordinal=0;ordinal<world.pocket_count();++ordinal){const Pocket& p=world.pocket_at(ordinal);if(p.closed)continue;Vec delta=p.p-b.p;float distance=delta.dot(delta);if(nearest<0 || distance<best){nearest=static_cast<int>(world.pocket_id(ordinal));best=distance;}}
            if(nearest>=0)force=unit_or_zero(world.pocket(static_cast<unsigned>(nearest))->p-b.p)*1000;
        }else if(b.conveyor_time>0){
            b.conveyor_time=std::max(0.0f,b.conveyor_time-tick_dt);force=unit_or_zero(b.conveyor_direction)*1000;
        }
        if(!prior_motion){
            if(b.pull_time>0){b.pull_time=std::max(0.0f,b.pull_time-tick_dt);force=unit_or_zero(b.pull_target-b.p)*100000;}
            else if(b.pulled){b.pulled=false;set_ball_weight(b,b.weight);}
        }
        b.v+=force*(tick_dt/b.def.mass);
        if(world.speed_limit>0 && b.v.length()>world.speed_limit)b.v=b.v.unit()*world.speed_limit;
    }
}
}
