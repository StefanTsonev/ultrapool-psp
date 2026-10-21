#include "ultrapool/core.hpp"
namespace up {
void set_ball_weight(Ball& b,Weight weight) {
    b.weight=weight;float scale=1,mass=1;
    if(weight==Weight::Light) { mass=.5f;scale=.8f; }
    if(weight==Weight::Heavy) { mass=5;scale=1.2f; }
    if(weight==Weight::VeryHeavy) { mass=60;scale=1.3f; }
    b.unlocked_mass=mass;b.def.mass=b.pulled?100:(b.locked?10000:mass);b.def.radius=17*scale;
}
void set_ball_locked(Ball& b,bool locked) {
    if(locked && !b.locked && !b.pulled) b.unlocked_mass=b.def.mass;
    if(locked) { b.def.mass=10000;b.v=Vec(); }
    else if(b.locked) b.def.mass=b.unlocked_mass;
    b.locked=locked;if(b.pulled)b.def.mass=100;
}
bool set_ball_shield(Ball& b,bool shielded) {
    if(b.shield_broken) return false;
    b.shielded=shielded;return true;
}
bool Game::queue_ball_modifier(unsigned index,Modifier modifier,int buff,Vec origin,float force,bool retarget) {
    if(cap_rewarded || index>=world.count || !world.balls[index].active || world.balls[index].cue) return false;
    { Action& a=allocate_action();
        start_wisp(a,ActionKind::ModifierBall,origin,world.balls[index].p);a.target=index;
        a.generation=world.balls[index].generation;a.flags=static_cast<unsigned>(modifier)|(retarget?0x80000000u:0u);a.bonus=buff;a.multiplier=force>=0?force:(modifier==Modifier::Nudge?.65f:1.0f);return true;
    }
}
bool Game::queue_pocket_shield(unsigned index,int bonus,Vec origin) {
    if(cap_rewarded || !world.pocket(index)) return false;
    { Action& a=allocate_action();
        start_wisp(a,ActionKind::PocketChange,origin,world.pocket(index)->p);a.target=index;a.bonus=bonus;a.flags=16;return true;
    }
}
bool Game::respawn_ball(unsigned index) {
    if(cap_rewarded || index>=world.count || world.balls[index].cue) return false;
    Ball& b=world.balls[index];Vec destination;bool found=false;
    for(unsigned attempt=0;attempt<1024 && !found;++attempt) {
        destination=Vec(-208+416*(random()/4294967296.0f),-356+716*(random()/4294967296.0f));
        found=world.valid_placement(destination,b.def.radius,index);
        for(unsigned j=0;j<world.count && found;++j) if(j!=index && world.balls[j].active && !world.balls[j].passive && !world.balls[j].energy && !world.balls[j].falling && (world.balls[j].p-destination).length()<40) found=false;
    }
    if(!found) { ++spawn_rejections;notice("No free respawn position");return false; }
    retire_listener(index);world.revive(index,destination);b.parrot_used=false;clear_pickup_contacts(index);sync_listeners();
    progress_increment("BALLS_RESPAWNED");
    emit_semantic(Signal::SpawnAnother,0,index);spawn_effects(index,Unsupported);return true;
}
namespace {
bool targetable(const Ball& b) { return targetable_ball(b); }
}
void Game::unlock_all(Vec origin) {
    unsigned options[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i) if(targetable(world.balls[i]) && world.balls[i].locked) options[count++]=i;
    for(unsigned i=count;i>1;--i) { unsigned chosen=random()%i,temp=options[i-1];options[i-1]=options[chosen];options[chosen]=temp; }
    for(unsigned i=0;i<count;++i) queue_ball_modifier(options[i],Modifier::Unlock,0,origin);
}
void Game::lock_random_other(unsigned exclude,int buff,Vec origin) {
    unsigned options[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i) if(i!=exclude && targetable(world.balls[i]) && !world.balls[i].locked) options[count++]=i;
    if(count) queue_ball_modifier(options[random()%count],Modifier::Lock,buff,origin);
}
}
