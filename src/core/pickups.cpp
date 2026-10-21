#include "ultrapool/core.hpp"
#include <algorithm>
namespace up {
namespace {
float growth(int flower_power) { float value=flower_power/2.0f;return value<=1?1:3-8/(value+3); }
float segment_distance_squared(Vec start,Vec end,Vec p) { Vec d=end-start;float size=d.dot(d),t=size>0?std::max(0.0f,std::min(1.0f,(p-start).dot(d)/size)):0;Vec delta=start+d*t-p;return delta.dot(delta); }
}
unsigned Game::flower_count() const { unsigned count=0;for(const auto& p:pickups) count+=p.active && p.kind==PickupKind::Flower;return count; }
int Game::spawn_pickup(PickupKind kind,Vec position,int flower_power) {
    if(static_cast<unsigned>(kind)>5 || cap_rewarded || phase==Phase::Shop || !std::isfinite(position.x) || !std::isfinite(position.y) || flower_power<1) return -1;
    unsigned limit=has_snack(id_of(SnackKey::LOTUS_ROOT))?5:20;
    if(kind==PickupKind::Flower && flower_count()>=limit) {
        unsigned choices[max_pickups],count=0;for(unsigned i=0;i<pickups.size();++i) if(pickups[i].active && pickups[i].kind==PickupKind::Flower) choices[count++]=i;
        if(!count) return -1;
        unsigned chosen=choices[random()%count];pickups[chosen].power+=flower_power;++planted_flowers;progress_track("FLOWERS_PLANTED",planted_flowers,true);return static_cast<int>(chosen);
    }
    for(unsigned i=0;i<pickups.size();++i) if(!pickups[i].active) {
        uint32_t generation=pickups[i].generation+1;if(!generation) ++generation;
        if(kind==PickupKind::Flower || kind==PickupKind::Flame) pickup_sensor_radius=18;
        if(kind==PickupKind::Stove) pickup_sensor_radius=32;
        Pickup p;p.active=true;p.kind=kind;p.p=position;p.power=flower_power;p.generation=generation;
        if(kind==PickupKind::Oil) p.charges=4;
        if(kind==PickupKind::Launchpad) { float direction=(random()/4294967296.0f)*6.283185307f;p.direction=Vec(std::cos(direction),std::sin(direction)); }
        pickups[i]=p;if(kind==PickupKind::Flower){++planted_flowers;progress_track("FLOWERS_PLANTED",planted_flowers,true);}return static_cast<int>(i);
    }
    ++pickup_overflows;notice("Table pickup capacity exceeded");return -1;
}
int Game::plant_pickup(PickupKind kind,int flower_power) {
    unsigned limit=has_snack(id_of(SnackKey::LOTUS_ROOT))?5:20;
    if(kind==PickupKind::Flower && flower_count()>=limit) return spawn_pickup(kind,Vec(),flower_power);
    Vec position;bool found=false;
    for(unsigned attempt=0;attempt<1024 && !found;++attempt) {
        position=Vec(-208+416*(random()/4294967296.0f),-356+716*(random()/4294967296.0f));found=true;
        for(unsigned i=0;i<world.count && found;++i) if(world.balls[i].active && !world.balls[i].passive && !world.balls[i].energy && !world.balls[i].falling && (world.balls[i].p-position).length()<40) found=false;
    }
    if(!found) { ++spawn_rejections;return -1; }
    return spawn_pickup(kind,position,flower_power);
}
void Game::remove_pickup(unsigned index) { if(index<pickups.size()) pickups[index].active=false; }
void Game::clear_pickup_contacts(unsigned body) { if(body<max_balls) for(auto& p:pickups) p.inside&=~(uint64_t(1)<<body); }
bool Game::pickup_holds_pending() const { for(const auto& p:pickups) if(p.active && p.held<max_balls) return true;return false; }
void Game::improve_flowers(int amount,bool all) {
    unsigned choices[max_pickups],count=0;
    for(unsigned i=0;i<pickups.size();++i) if(pickups[i].active && pickups[i].kind==PickupKind::Flower) choices[count++]=i;
    if(all) for(unsigned i=0;i<count;++i) pickups[choices[i]].power+=amount;
    else if(count) pickups[choices[random()%count]].power+=amount;
}
void Game::grow_biggest_flower(int amount) {
    int biggest=-1;for(unsigned i=0;i<pickups.size();++i) if(pickups[i].active && pickups[i].kind==PickupKind::Flower && (biggest<0 || pickups[i].power>pickups[biggest].power)) biggest=static_cast<int>(i);
    if(biggest>=0) pickups[biggest].power+=amount;else plant_pickup(PickupKind::Flower,amount);
}
void Game::spend_flowers(unsigned count) {
    unsigned choices[max_pickups],size=0;for(unsigned i=0;i<pickups.size();++i) if(pickups[i].active && pickups[i].kind==PickupKind::Flower) choices[size++]=i;
    for(unsigned i=size;i>1;--i) std::swap(choices[i-1],choices[random()%i]);
    for(unsigned i=0;i<std::min(count,size);++i) { Pickup p=pickups[choices[i]];remove_pickup(choices[i]);int closest=closest_target(p.p,max_balls);if(closest>=0) queue_ball_buff(static_cast<unsigned>(closest),p.power,p.p); }
}
void Game::pickup_contact(unsigned index,unsigned body) {
    Pickup& p=pickups[index];Ball& b=world.balls[body];++pickup_contacts;
    bool card=ball_info(b.def.id).has_card && !b.cue;
    if(p.kind==PickupKind::Flower) {
        if(card) { sound("flowerbush_ball");int flower_power=p.power;remove_pickup(index);buff_ball(body,flower_power); }
        else if(b.cue && b.collect_flowers) { int flower_power=p.power;Vec origin=p.p;remove_pickup(index);int closest=closest_target(origin,max_balls);if(closest>=0) queue_ball_buff(static_cast<unsigned>(closest),flower_power,origin); }
    }
    if(p.kind==PickupKind::Oil) {
        if((b.cue && b.just_respawned) || b.v.length()==0) return;
        queue_impulse(body,b.v.unit()*360);if(--p.charges<=0) remove_pickup(index);
    }
    if(p.kind==PickupKind::Thorn) { b.v=b.v*.1f;remove_pickup(index); }
    if(p.kind==PickupKind::Launchpad && p.held==max_balls) {
        if(b.cue) return;
        b.v=b.v*.1f;p.held=static_cast<uint16_t>(body);p.held_generation=b.generation;p.hold_time=1;
    }
    if(p.kind==PickupKind::Stove && card) b.flaming=true;
    if(p.kind==PickupKind::Flame) { if(card) b.flaming=true;remove_pickup(index); }
    run_ball_event(Signal::Pickup,body);
}
void Game::update_pickups() {
    if(cap_rewarded) return;
    for(unsigned i=0;i<world.count;++i) if(pickup_body_generations[i]!=world.balls[i].generation) { clear_pickup_contacts(i);pickup_body_generations[i]=world.balls[i].generation; }
    std::array<uint32_t,max_pickups> ready{};for(unsigned i=0;i<pickups.size();++i) if(pickups[i].active) ready[i]=pickups[i].generation;
    for(unsigned i=0;i<pickups.size();++i) {
        Pickup& p=pickups[i];if(!p.active || p.generation!=ready[i]) continue;
        float desired=p.kind==PickupKind::Flower?growth(p.power):1;p.scale+=(desired-p.scale)*std::min(1.0f,.2f*tick_dt*60);
        if(p.held<max_balls) {
            if(p.held>=world.count || !world.balls[p.held].active || world.balls[p.held].generation!=p.held_generation) p.held=max_balls;
            else {
                Ball& held=world.balls[p.held];held.p+=(p.p-held.p)*std::min(1.0f,.2f*tick_dt*60);p.hold_time-=tick_dt;
                if(p.hold_time<=.000001f) { queue_impulse(p.held,p.direction*1200);remove_pickup(i);continue; }
            }
        }
        if(p.delay>0) { p.delay-=tick_dt;if(p.delay>.000001f) continue;p.delay=0; }
        for(unsigned j=0;j<world.count;++j) {
            Ball& b=world.balls[j];uint64_t bit=uint64_t(1)<<j;
            if(!b.active || b.collision_disabled || b.energy || (b.falling && !b.captured_this_tick)) { p.inside&=~bit;continue; }
            float radius=pickup_sensor_radius*p.scale+b.def.radius;Vec delta=b.p-p.p;
            bool inside=delta.dot(delta)<=radius*radius,was_inside=(p.inside&bit)!=0;
            Vec start=b.motion_tick==world.metrics.ticks?b.step_start:b.p;
            bool touched=inside || segment_distance_squared(start,b.p,p.p)<=radius*radius;
            if(inside) p.inside|=bit;else p.inside&=~bit;
            if(!was_inside && touched) { pickup_contact(i,j);if(!p.active || p.generation!=ready[i] || cap_rewarded) break; }
        }
    }
    for(unsigned i=0;i<world.count;++i) world.balls[i].just_respawned=false;
}
}

