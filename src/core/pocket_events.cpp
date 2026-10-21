#include "ultrapool/core.hpp"
namespace up {
namespace {
struct PocketReference {World& world;unsigned index;PocketReference(World& w,unsigned i):world(w),index(i){world.retain(index);}~PocketReference(){world.release(index);}};
}
void Game::process_pocket(const Event& e){
    if(cap_rewarded || e.a>=world.count || (e.generation_a && world.balls[e.a].generation!=e.generation_a))return;
    PocketReference reference(world,e.a);

    Ball& b=world.balls[e.a];
    if(b.energy)return;
    if(world.pocket(e.b)){sound("ball_pocket");sound("ball_pocket_rattle");}
    if (b.cue) { respawn_cue(); notice("Cue returned automatically"); return; }
    if(b.last_pocket_tick==e.tick) return;
    b.last_pocket_tick=e.tick;
    Pocket* scored_pocket=world.pocket(e.b);
    int pocket_score=has_ability(b,Werewolf)?0:(scored_pocket?scored_pocket->score(b.def.base_score):b.def.base_score)*e.multiplier;
    retire_listener(e.a);
    bool protected_ball=b.shielded;
    bool protected_pocket=scored_pocket && scored_pocket->shielded && !b.shield_broken;
    if(protected_ball || protected_pocket) {
        if(protected_ball) b.shielded=false;else scored_pocket->shielded=false;
        b.shield_broken=true;respawn_ball(e.a);
    }
    progress_pocket(b,pocket_score);
    add_score(pocket_score,e.a);
    if(cap_rewarded) return;
    if(b.passive)remove_passive(e.a);
    // Global.ball_pocketed notifies surviving listeners before self effects.
    pocket_listeners(e.a,e.b);
    pocket_effect(e.a,e.b);
    b.flaming=false;
    if(scored_pocket)release_held(e.b);
        
}
bool Game::consume_ball(unsigned index,int multiplier){
    if(cap_rewarded || index>=world.count)return false;
    Ball& b=world.balls[index];if(!b.active || b.cue || b.last_pocket_tick==world.metrics.ticks)return false;
    b.active=false;b.falling=false;b.v=Vec();
    Event e;e.kind=EventKind::Pocketed;e.a=static_cast<uint16_t>(index);e.b=6;e.tick=world.metrics.ticks;e.generation_a=b.generation;e.multiplier=multiplier;
    process_pocket(e);return true;
}
}
