#include "ultrapool/core.hpp"
namespace up {
namespace {
bool supported_cube(unsigned id) {
    return id==id_of(BallKey::BOUNCER) || id==id_of(BallKey::PATCHWORK) || id==id_of(BallKey::DEMON) || id==id_of(BallKey::PAPERWORK) || id==id_of(BallKey::WEIGHTS) || id==id_of(BallKey::CUBERT) || id==id_of(BallKey::GLITCH) || id==id_of(BallKey::CLOCKTOWER) || id==id_of(BallKey::TYRANT) || id==id_of(BallKey::THORNS);
}
EntityRef cube_ref(unsigned slot,unsigned id) {
    EntityRef ref;ref.kind=EntityKind::Cube;ref.index=static_cast<uint16_t>(slot);ref.generation=id;return ref;
}
}
bool Game::set_cube(unsigned slot,unsigned id,unsigned level) {
    if(slot>=cubes.size() || (id && (!supported_cube(id) || level<1 || level>5))) return false;
    for(unsigned i=0;i<cubes.size();++i) if(id && i!=slot && cubes[i].id==id) return false;
    if(cubes[slot].id) listeners.remove(cube_ref(slot,cubes[slot].id));
    cubes[slot].id=static_cast<uint16_t>(id);cubes[slot].level=static_cast<uint8_t>(level);cubes[slot].count=0;
    if(id) listeners.add(cube_ref(slot,id),ball_info(id).events);
    return true;
}
void Game::setup_curses() {
    const unsigned required_levels=curse_levels_for_round(round,difficulty);
    unsigned total=0;for(const auto& cube:cubes)if(cube.id)total+=cube.level;
    // Independent deterministic selection; exact Godot RNG stream parity is pending.
    uint32_t seed=0x554c5452u+(static_cast<unsigned>(round)-1)*1000u;
    while(total<required_levels) {
        unsigned options[15],count=0;
        for(unsigned id=1;id<content_count;++id) {
            const auto& info=ball_info(id);if(info.set!=ItemSet::NEGATIVE || !info.can_drop || !supported_cube(id))continue;
            bool full=false;for(const auto& cube:cubes)if(cube.id==id && cube.level>=5)full=true;
            if(!full)options[count++]=id;
        }
        if(!count)break;
        seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
        unsigned selected=options[seed%count],slot=static_cast<unsigned>(cubes.size());
        for(unsigned i=0;i<cubes.size();++i)if(cubes[i].id==selected){slot=i;break;}
        if(slot<cubes.size()){++cubes[slot].level;++total;}
        else {
            for(unsigned i=0;i<cubes.size();++i)if(!cubes[i].id){slot=i;break;}
            if(slot==cubes.size() || !set_cube(slot,selected,1))break;
            ++total;
        }
    }
}
void Game::reset_cube_counters() { for(auto& cube:cubes) cube.count=0; }
void Game::curse_heavy(Vec origin) {
    unsigned options[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]) && world.balls[i].weight!=Weight::Heavy)options[count++]=i;
    if(count)queue_ball_modifier(options[random()%count],Modifier::Heavy,0,origin);
}
void Game::curse_teleport(unsigned amount,Vec origin) {
    unsigned options[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]))options[count++]=i;
    for(unsigned i=count;i>1;--i){unsigned j=random()%i,t=options[i-1];options[i-1]=options[j];options[j]=t;}
    for(unsigned i=0;i<count && i<amount;++i)queue_ball_modifier(options[i],Modifier::Teleport,0,origin);
}
void Game::run_cube_event(Signal signal,unsigned slot,unsigned pocket,unsigned source) {
    if(cap_rewarded || slot>=cubes.size()) return;
    auto& cube=cubes[slot];if(!supported_cube(cube.id)) return;
    const unsigned level=cube.level;
    const Vec origin(-310,-300+40*static_cast<float>(slot));
    if(signal==Signal::SpawnAnother && cube.id==id_of(BallKey::TYRANT) && source<world.count) {
        if(++cube.count>=7-static_cast<int>(level)) {
            queue_ball_buff(source,-100,origin);cube.count-=7-static_cast<int>(level);
        }
    }
    if(signal==Signal::RoundStart) {
        if(cube.id==id_of(BallKey::THORNS)){plant_pickup(PickupKind::Thorn);plant_pickup(PickupKind::Thorn);}
        if(cube.id==id_of(BallKey::WEIGHTS))curse_heavy(origin);
        if(cube.id==id_of(BallKey::GLITCH))curse_teleport(level,origin);
        if(cube.id==id_of(BallKey::CUBERT))for(int i=0;i<round && i<10;++i)spawn_data(id_of(BallKey::TUNGSTEN_BALL),1,-1,origin,Cubert);
        if(cube.id==id_of(BallKey::BOUNCER)) for(unsigned i=0;i<level;++i) change_random_pocket_lock(true);
        if(cube.id==id_of(BallKey::PATCHWORK)) for(unsigned p=0;p<6;++p) queue_pocket_change(p,-1,0,origin);
        if(cube.id==id_of(BallKey::DEMON)) spawn_data(id_of(BallKey::IMP),1,-2*static_cast<int>(level),origin,Demon);
    }
    if(signal!=Signal::PocketAnother) return;
    if(cube.id==id_of(BallKey::THORNS) && ++cube.count>=6-static_cast<int>(level)) {
        plant_pickup(PickupKind::Thorn);plant_pickup(PickupKind::Thorn);cube.count-=6-static_cast<int>(level);
    }
    if(cube.id==id_of(BallKey::CLOCKTOWER) && ++cube.count>=4) {
        cube.count-=4;
        for(unsigned p=0;p<6;++p) {
            int bonus=world.pockets[p].bonus;
            if(bonus){world.pockets[p].bonus=0;queue_pocket_change((p+1)%6,0,bonus,world.pockets[p].p);}
        }
    }
    if(cube.id==id_of(BallKey::PAPERWORK) && world.pocket(pocket) && ++cube.count>=3) {
        queue_pocket_change(random_pocket_id(),0,-1-static_cast<int>(level),origin);cube.count-=3;
    }
    if(cube.id==id_of(BallKey::WEIGHTS) && ++cube.count>=8-static_cast<int>(level)) {
        curse_heavy(origin);cube.count-=8-static_cast<int>(level);
    }
    if(cube.id==id_of(BallKey::GLITCH) && ++cube.count>=4) {
        curse_teleport(level,origin);cube.count-=4;
    }
    if(cube.id==id_of(BallKey::BOUNCER)) {
        if(++cube.count>=3) {change_random_pocket_lock(false);change_random_pocket_lock(true);cube.count-=3;}
    }
    if(cube.id==id_of(BallKey::PATCHWORK) && cube.count<static_cast<int>(3*level)) {
        if(++cube.count>=static_cast<int>(3*level)) for(unsigned p=0;p<6;++p) queue_pocket_change(p,1,0,origin);
    }
    if(cube.id==id_of(BallKey::DEMON)) {
        const int threshold=8-static_cast<int>(level);
        if(++cube.count>=threshold) {spawn_data(id_of(BallKey::IMP),1,-2*static_cast<int>(level),origin,Demon);cube.count-=threshold;}
    }
}
}
