#include "ultrapool/core.hpp"
#include <algorithm>
#include <limits>

namespace up {
bool Events::push(EventKind kind, unsigned a, unsigned b, uint32_t tick,uint32_t ga,uint32_t gb,int multiplier) {
    if (count==data.size()) { ++dropped; return false; }
    Event e; e.kind=kind; e.a=static_cast<uint16_t>(a); e.b=static_cast<uint16_t>(b);
    e.tick=tick; e.sequence=serial++; e.generation_a=ga; e.generation_b=gb; e.multiplier=multiplier;
    data[(first+count)%data.size()]=e; ++count; return true;
}
bool Events::pop(Event& e) {
    if (!count) return false;
    e=data[first]; first=(first+1)%data.size(); --count; return true;
}
World::World() {
    const Vec centers[6]={Vec(-250,-400),Vec(250,-400),Vec(252,0),
                          Vec(250,400),Vec(-250,400),Vec(-250,0)};
    for (unsigned i=0;i<6;++i) pockets[i].p=centers[i];
}
bool World::add(const Ball& b) {
    if (b.def.mass<=0 || b.def.radius<=0 || !std::isfinite(b.def.mass) || !std::isfinite(b.def.radius)) return false;
    std::size_t slot=count;
    if(slot==max_balls) {
        for(slot=1;slot<count;++slot) if(!balls[slot].active && !balls[slot].falling && !retained[slot]) break;
        if(slot==count) return false;
    }
    uint32_t generation=balls[slot].generation+1;
    if(!generation) ++generation;
    balls[slot]=b; balls[slot].generation=generation;balls[slot].step_start=b.p;balls[slot].motion_tick=metrics.ticks;balls[slot].captured_this_tick=false;
    balls[slot].base_value=balls[slot].def.base_score-balls[slot].temp_value;
    contacts[slot]=0; wall_contacts[slot]=0;
    for(auto& mask:contacts) mask&=~(uint64_t(1)<<slot);
    if(slot==count) ++count;
    last_added=static_cast<unsigned>(slot); return true;
}
bool World::pair_collides(unsigned a,unsigned b) const {
    if(a>=count || b>=count)return false;
    const Ball& first=balls[a];const Ball& second=balls[b];
    if(first.collision_disabled || second.collision_disabled)return false;
    if(first.energy && second.energy)return false;
    if((first.energy && second.cue) || (second.energy && first.cue))return false;
    if(first.energy && first.collision_exception==b && first.exception_generation==second.generation)return false;
    if(second.energy && second.collision_exception==a && second.exception_generation==first.generation)return false;
    return true;
}
bool World::revive(unsigned index,Vec position) {
    if(index>=count) return false;
    Ball& b=balls[index];b.p=position;b.step_start=position;b.motion_tick=metrics.ticks;b.captured_this_tick=false;b.v=Vec();b.active=true;b.visual_rotation=0;b.pull_time=0;b.pulled=false;b.def.mass=b.locked?10000:b.unlocked_mass;b.collision_disabled=false;b.gone=false;b.falling=false;b.fall_remaining=0;b.flash=.5f;b.conveyor_time=0;b.conveyor_direction=Vec();
    contacts[index]=0;wall_contacts[index]=0;
    for(auto& mask:contacts) mask&=~(uint64_t(1)<<index);
    return true;
}
bool World::relocate(unsigned index,Vec position) {
    if(index>=count || !balls[index].active || balls[index].falling || !std::isfinite(position.x) || !std::isfinite(position.y)) return false;
    Ball& b=balls[index];b.p=position;b.step_start=position;b.motion_tick=metrics.ticks;
    contacts[index]=0;wall_contacts[index]=0;
    for(auto& mask:contacts)mask&=~(uint64_t(1)<<index);
    return true;
}
bool World::moving() const {
    for (std::size_t i=0;i<count;++i)
        if (balls[i].active && !balls[i].passive && !balls[i].energy && (balls[i].falling || balls[i].v.dot(balls[i].v)>0)) return true;
    return false;
}
bool World::passive_motion() const {
    for(unsigned i=1;i<count;++i){const Ball& b=balls[i];if(b.active && b.passive && (b.v.dot(b.v)>0 || b.chase_flowers>0 || b.rocket_time>0 || b.snake_time>0 || b.chase_pocket_time>0 || b.conveyor_time>0 || b.pull_time>0 || b.pulled))return true;}
    return false;
}
bool World::energy_active() const {
    for(unsigned i=0;i<count;++i)if(balls[i].active && balls[i].energy)return true;
    return false;
}
bool World::valid_placement(Vec p,float r,std::size_t exclude) const {
    if (p.x<minimum.x+r || p.x>maximum.x-r || p.y<minimum.y+r || p.y>maximum.y-r) return false;
    for (std::size_t i=0;i<count;++i) if (i!=exclude && balls[i].active && !balls[i].passive && !balls[i].falling && !balls[i].energy) {
        Vec d=p-balls[i].p; float sum=r+balls[i].def.radius+0.1f;
        if (d.dot(d)<sum*sum) return false;
    }
    if (pockets_enabled) for (const auto& pocket:pockets) {
        if(pocket.closed) continue;
        Vec d=p-pocket.p;
        float capture=pocket.radius+r;
        if (d.dot(d)<capture*capture) return false;
    }
    return true;
}
// Earliest entry into a circle for relative linear motion. Tangencies do not
// produce impulses. Overlap is handled separately by inverse-mass correction.
static float circle_time(Vec p,Vec v,float radius,float limit) {
    float a=v.dot(v), b=p.dot(v), c=p.dot(p)-radius*radius;
    if (c<=0) return b< -1e-5f ? 0.0f : std::numeric_limits<float>::infinity();
    if (a<1e-10f || b>=0) return std::numeric_limits<float>::infinity();
    float discriminant=b*b-a*c;
    if (discriminant<0) return std::numeric_limits<float>::infinity();
    float t=(-b-std::sqrt(discriminant))/a;
    return t>=0 && t<=limit ? t:std::numeric_limits<float>::infinity();
}
bool World::set_pocket_closed(unsigned id, bool closed) {
    Pocket* p=pocket(id);if(!p || p->closed==closed)return false;
    p->closed=closed;
    if(!closed && pockets_enabled)for(unsigned i=0;i<count;++i){
        const Ball& b=balls[i];if(!physical_pocketable(b) || b.cue)continue;
        Vec delta=b.p-p->p;float radius=p->radius*p->scale*p->suck_scale+b.def.radius;
        if(delta.dot(delta)<radius*radius)capture(i,id);
    }
    return true;
}
void World::capture(std::size_t index,unsigned id) {
    Pocket* p=pocket(id);if(index>=count || !p || p->closed)return;
    Ball& b=balls[index];if(!physical_pocketable(b))return;
    if(b.energy){b.active=false;b.gone=true;b.v=Vec();return;}
    b.falling=true;b.captured_this_tick=true;b.pocket_index=static_cast<uint8_t>(id);b.fall_remaining=.5f;
    float speed=b.v.length();b.v=speed>0?b.v*(500/speed):Vec();
    events.push(EventKind::Captured,static_cast<unsigned>(index),id,metrics.ticks,b.generation);
}
void World::step(float dt) {
    if (!(dt>0) || !std::isfinite(dt)) return;
    ++metrics.ticks;update_pockets(dt);
    // Falling is still active motion, but no longer participates in contacts.
    for (std::size_t i=0;i<count;++i) {
        Ball& b=balls[i];b.step_start=b.p;b.motion_tick=metrics.ticks;b.captured_this_tick=false; b.flash=std::max(0.0f,b.flash-dt);
        if(b.active && b.energy){
            b.energy_age+=dt;
            if(b.energy_age>=1.0f-0.000001f && b.v.dot(b.v)<100){b.active=false;b.gone=true;b.v=Vec();}
        }
        if (b.active && b.falling) {
            const Pocket* captured=pocket(b.pocket_index);
            if(!captured){b.active=false;b.falling=false;b.v=Vec();continue;}
            Vec force=(captured->p-b.p)*1000;
            b.v=(b.v+force*dt)*std::max(0.0f,1-32*dt);
            b.p+=b.v*dt; b.fall_remaining-=dt;
            if (b.fall_remaining<=0.000001f) {
                b.active=false; b.falling=false; b.v=Vec();
                events.push(EventKind::Pocketed,static_cast<unsigned>(i),b.pocket_index,metrics.ticks,b.generation);
            }
        }
        for (std::size_t j=i+1;j<count;++j) {
            Vec d=b.p-balls[j].p; float r=b.def.radius+balls[j].def.radius+0.3f;
            if (!b.active || b.falling || !balls[j].active || balls[j].falling || d.dot(d)>r*r) {
                contacts[i]&=~(uint64_t(1)<<j); contacts[j]&=~(uint64_t(1)<<i);
            }
        }
        if (b.p.x>minimum.x+b.def.radius+0.3f) wall_contacts[i]&=~1u;
        if (b.p.x<maximum.x-b.def.radius-0.3f) wall_contacts[i]&=~2u;
        if (b.p.y>minimum.y+b.def.radius+0.3f) wall_contacts[i]&=~4u;
        if (b.p.y<maximum.y-b.def.radius-0.3f) wall_contacts[i]&=~8u;
    }
    // Godot-style linear damping at the beginning of each fixed tick.
    for (std::size_t i=0;i<count;++i) if (balls[i].active && !balls[i].falling)
    {
        float speed=balls[i].v.length();
        if (speed_limit>0 && speed>speed_limit) balls[i].v=balls[i].v*(speed_limit/speed);
        balls[i].v=balls[i].v*std::max(0.0f,1-balls[i].def.damping*dt);
    }
    // Repair initial overlaps without adding kinetic energy.
    for (int pass=0;pass<4;++pass) for (std::size_t i=0;i<count;++i) {
        Ball& a=balls[i]; if (!a.active || a.falling || a.collision_disabled) continue;
        if(!a.passive){
        a.p.x=std::max(minimum.x+a.def.radius,std::min(maximum.x-a.def.radius,a.p.x));
        a.p.y=std::max(minimum.y+a.def.radius,std::min(maximum.y-a.def.radius,a.p.y));}
        for (std::size_t j=i+1;j<count;++j) {
            Ball& b=balls[j]; if (!b.active || b.falling || !pair_collides(static_cast<unsigned>(i),static_cast<unsigned>(j))) continue;
            Vec delta=b.p-a.p; float length=delta.length(), sum=a.def.radius+b.def.radius;
            if (length<sum-0.001f) {
                Vec n=length>1e-6f ? delta*(1/length):Vec(1,0);
                float ia=1/a.def.mass, ib=1/b.def.mass, depth=sum-length+0.001f;
                a.p+=n*(-depth*ia/(ia+ib)); b.p+=n*(depth*ib/(ia+ib));
            }
        }
    }
    float left=dt;
    unsigned iteration=0;
    for (;left>1e-8f && iteration<128;++iteration) {
        float earliest=left;
        int kind=-1, hit_a=-1, hit_b=-1;
        Vec wall_normal;
        auto choose=[&](float t,int k,int a,int b,Vec n=Vec()) {
            if (t<=earliest && (kind<0 || t<earliest-1e-8f)) {
                earliest=t; kind=k; hit_a=a; hit_b=b; wall_normal=n;
            }
        };
        for (std::size_t i=0;i<count;++i) {
            const Ball& a=balls[i]; if (!a.active || a.falling || a.collision_disabled) continue;
            if (pockets_enabled && physical_pocketable(a)) for (unsigned ordinal=0;ordinal<pocket_count();++ordinal) {
                const Pocket& p=pocket_at(ordinal);if(p.closed || p.scale<=0)continue;
                Vec d=a.p-p.p;
                float capture=(a.cue?11.0f:p.radius*p.suck_scale)*p.scale+a.def.radius;
                float t=d.dot(d)<=capture*capture?0:circle_time(d,a.v,capture,left);
                choose(t,2,static_cast<int>(i),static_cast<int>(pocket_id(ordinal)));
            }
            if (a.v.x< -1e-6f) choose(std::max(0.0f,(minimum.x+a.def.radius-a.p.x)/a.v.x),1,static_cast<int>(i),0,Vec(1,0));
            if (a.v.x> 1e-6f) choose(std::max(0.0f,(maximum.x-a.def.radius-a.p.x)/a.v.x),1,static_cast<int>(i),0,Vec(-1,0));
            if (a.v.y< -1e-6f) choose(std::max(0.0f,(minimum.y+a.def.radius-a.p.y)/a.v.y),1,static_cast<int>(i),0,Vec(0,1));
            if (a.v.y> 1e-6f) choose(std::max(0.0f,(maximum.y-a.def.radius-a.p.y)/a.v.y),1,static_cast<int>(i),0,Vec(0,-1));
            for (std::size_t j=i+1;j<count;++j) if (balls[j].active && !balls[j].falling && pair_collides(static_cast<unsigned>(i),static_cast<unsigned>(j)))
                choose(circle_time(a.p-balls[j].p,a.v-balls[j].v,
                    a.def.radius+balls[j].def.radius,left),0,static_cast<int>(i),static_cast<int>(j));
        }
        for (std::size_t i=0;i<count;++i) if (balls[i].active && !balls[i].falling) balls[i].p+=balls[i].v*earliest;
        left-=earliest;
        if (kind<0) break;
        Ball& a=balls[hit_a];
        if (kind==2) {
            capture(static_cast<std::size_t>(hit_a),static_cast<unsigned>(hit_b));
        } else if (kind==1) {
            float vn=a.v.dot(wall_normal);
            if (vn<0) a.v+=wall_normal*(-(1+restitution)*vn);
            a.p+=wall_normal*0.0001f;
            uint8_t bit=wall_normal.x>0 ? 1:wall_normal.x<0 ? 2:wall_normal.y>0 ? 4:8;
            if (!(wall_contacts[hit_a]&bit)) events.push(EventKind::Wall,hit_a,0,metrics.ticks,a.generation);
            wall_contacts[hit_a]|=bit;
            ++metrics.impacts;
        } else {
            Ball& b=balls[hit_b]; Vec n=(b.p-a.p).unit();
            float relative=(a.v-b.v).dot(n), ia=1/a.def.mass, ib=1/b.def.mass;
            if (relative>0) {
                float impulse=(1+restitution)*relative/(ia+ib);
                a.v+=n*(-impulse*ia); b.v+=n*(impulse*ib);
                if (!(contacts[hit_a]&(uint64_t(1)<<hit_b)))
                    events.push(EventKind::Collision,hit_a,hit_b,metrics.ticks,a.generation,b.generation);
                contacts[hit_a]|=uint64_t(1)<<hit_b; contacts[hit_b]|=uint64_t(1)<<hit_a;
                ++metrics.impacts;
            }
        }
    }
    // Never advance through unprocessed collisions on overload. Surface the
    // conservative time loss in telemetry rather than permitting tunneling.
    if (left>1e-8f && iteration==128) ++metrics.solver_exhaustions;
    for (std::size_t i=0;i<count;++i) if (balls[i].active && !balls[i].falling && !balls[i].energy &&
        balls[i].v.dot(balls[i].v)<stop_speed*stop_speed && balls[i].chase_flowers<=0 && balls[i].rocket_time<=0 && balls[i].conveyor_time<=0 && balls[i].snake_time<=0 && balls[i].chase_pocket_time<=0 && balls[i].pull_time<=0) balls[i].v=Vec();
    for(std::size_t i=0;i<count;++i) {
        Ball& b=balls[i];
        if(!b.active || b.falling || b.energy) continue;
        float distance=(b.p-b.step_start).length();
        if(distance>0.0001f) {
            b.visual_rotation+=distance/std::max(1.0f,b.def.radius);
            if(b.visual_rotation>6.2831853f || b.visual_rotation< -6.2831853f)
                b.visual_rotation=std::fmod(b.visual_rotation,6.2831853f);
        }
    }
}
}
