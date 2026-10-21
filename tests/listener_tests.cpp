#include "ultrapool/core.hpp"
#include <cstdio>
#include <cstdlib>
using namespace up;
namespace {
unsigned checks=0;
void verify(bool condition,const char* name) { ++checks;if(!condition) { std::fprintf(stderr,"FAIL LISTENER: %s\n",name);std::exit(1); } }
EntityRef ref(unsigned index,uint32_t generation=1,EntityKind kind=EntityKind::Ball) { EntityRef r;r.index=static_cast<uint16_t>(index);r.generation=generation;r.kind=kind;return r; }
void resolve(Game& g) { for(unsigned i=0;i<1800 && g.pending_actions();++i) g.update(tick_dt);verify(!g.pending_actions(),"target carriers resolve or cancel within lifetime"); }
void add_cue(Game& g) { Ball cue;cue.cue=true;cue.p=Vec(-200,0);g.world.add(cue);g.phase=Phase::Aiming;g.world.pockets_enabled=false; }
void add_ball(Game& g,unsigned id,unsigned mixed=0) { Ball b;b.def=builtin(id);b.mixed_with=static_cast<uint16_t>(mixed);b.p=Vec(float(g.world.count*50),0);g.world.add(b); }
}
unsigned listener_tests() {
    checks=0;ListenerRegistry registry;uint32_t score=signal_bit(Signal::Score),pocket=signal_bit(Signal::PocketAnother);
    registry.add(ref(1),score|pocket);registry.add(ref(2),score);registry.add(ref(3),score);
    auto before=registry.snapshot(Signal::Score);registry.remove(ref(1));auto after=registry.snapshot(Signal::Score);
    verify(before.count==3 && before.entities[0].index==1 && before.entities[1].index==2 && before.entities[2].index==3,"dispatch snapshot retains registration order after mutation");
    verify(after.count==2 && after.entities[0].index==3 && after.entities[1].index==2,"unregister swaps final entry into removed slot");
    verify(registry.snapshot(Signal::PocketAnother).count==0,"unregister removes all event subscriptions");
    registry.add(ref(4),score);verify(registry.snapshot(Signal::Score).entities[2].index==4,"new listener appends after surviving entries");
    registry.add(ref(4),score);verify(registry.snapshot(Signal::Score).count==3,"duplicate subscription cannot double-fire legal registration");
    registry.clear();for(unsigned i=0;i<31;++i) registry.add(ref(i),score);
    verify(registry.snapshot(Signal::Score).count==30 && registry.overflows==1,"original thirty-listener cap is explicit and instrumented");
    registry.remove(ref(12));registry.add(ref(31),score);
    verify(registry.snapshot(Signal::Score).count==30 && registry.snapshot(Signal::Score).entities[12].index==29 && registry.snapshot(Signal::Score).entities[29].index==31,"freed slot preserves swap order and next registration appends");
    registry.clear();registry.add(ref(7,2),score);registry.remove(ref(7,1));verify(registry.snapshot(Signal::Score).count==1,"stale generation cannot unregister replacement entity");
    registry.add(ref(7,2,EntityKind::Snack),score);verify(registry.snapshot(Signal::Score).count==2,"ball and passive indices have distinct identity namespaces");
    Game g;add_cue(g);add_ball(g,19);add_ball(g,19);add_ball(g,19);g.dispatch();
    auto list=g.listeners.snapshot(Signal::PocketAnother);verify(list.count==3 && list.entities[0].index==1 && list.entities[2].index==3,"physical listeners initially register in deployment order");
    g.world.balls[1].active=false;g.world.events.push(EventKind::Pocketed,1,0,0);g.dispatch();list=g.listeners.snapshot(Signal::PocketAnother);
    verify(list.count==2 && list.entities[0].index==3 && list.entities[1].index==2,"actual pocket lifecycle retires listener with original swap removal");
    g.world.balls[1].active=true;g.dispatch();list=g.listeners.snapshot(Signal::PocketAnother);
    verify(list.count==3 && list.entities[2].index==1,"reactivated instance rejoins at the end without changing generation");
    g=Game();add_cue(g);add_ball(g,1,19);add_ball(g,19);g.dispatch();g.world.balls[1].def.base_score=3;normalize_numbers(g.world.balls[1]);g.dispatch();
    list=g.listeners.snapshot(Signal::PocketAnother);verify(list.entities[0].index==1 && list.entities[1].index==2,"number value normalization does not spuriously reregister mixed listeners");
    g.world.balls[1].def=builtin(28);g.dispatch();list=g.listeners.snapshot(Signal::PocketAnother);
    verify(list.entities[0].index==2 && list.entities[1].index==1,"identity transformation reregisters after surviving listeners");
    g=Game();add_cue(g);add_ball(g,19);add_ball(g,1);g.world.balls[2].active=false;g.world.balls[2].owner=0;
    g.inventory[0].id=1;g.inventory[0].score=1;g.snacks={PokerChip,Cereal,0,0};g.world.events.push(EventKind::Pocketed,2,0,0);g.dispatch();
    list=g.listeners.snapshot(Signal::PocketAnother);
    verify(list.count==3 && list.entities[0].kind==EntityKind::Snack && list.entities[1].kind==EntityKind::Snack && list.entities[2].kind==EntityKind::Ball,"passive subscriptions precede physical listener subscriptions");
    verify(g.world.balls[1].def.base_score==5 && g.inventory[0].score==2,"passive carrier consumes RNG before Dice and Cereal buffs correct owner");
    g=Game();g.reset();g.snacks={Croissant,0,0,0};g.inventory[0].id=9;g.inventory[0].score=2;g.inventory[1].id=32;g.inventory[1].score=2;g.inventory[1].level=3;g.begin_round();
    unsigned copy_value=0;for(const auto& a:g.actions) if(a.kind==ActionKind::Spawn) copy_value=a.snapshot.def.base_score;
    verify(copy_value==2 && g.world.balls[1].def.base_score==2,"Mirror snapshots original value while earlier Croissant buffs are still traveling");
    resolve(g);verify(g.world.balls[1].def.base_score==3 && g.world.balls[g.world.count-1].def.base_score==2,"Croissant targets setup balls but does not retroactively buff later Mirror copy");
    g=Game();add_cue(g);add_ball(g,1);g.queue_ball_buff(1,4,Vec());
    verify(g.world.balls[1].def.base_score==1,"ball-target buff is deferred at creation");
    resolve(g);verify(g.world.balls[1].def.base_score==5 && g.world.balls[1].def.id==5,"ball-target landing updates value and number identity");
    g=Game();add_cue(g);add_ball(g,1);g.queue_ball_buff(1,4,Vec());++g.world.balls[1].generation;
    resolve(g);verify(g.world.balls[1].def.base_score==1,"stale buff cannot modify replacement physical generation");
    g=Game();add_cue(g);add_ball(g,1);g.queue_ball_buff(1,4,Vec());g.world.balls[1].active=false;
    for(unsigned i=0;i<80;++i) g.update(tick_dt);
    verify(g.pending_actions(),"dead-target carrier preserves original initial spawning stage before validation");
    resolve(g);verify(g.world.balls[1].def.base_score==1,"dead-target buff cancels when homing begins");
    g=Game();add_cue(g);add_ball(g,1);g.world.balls[1].v=Vec(300,0);g.queue_ball_buff(1,2,Vec());
    resolve(g);verify(g.world.balls[1].def.base_score==3 && g.world.balls[1].p.x>100,"buff carrier follows moving target before landing");
    g=Game();add_cue(g);add_ball(g,11);g.world.balls[1].def.base_score=3;g.queue_ball_pocket(1,2,Vec());
    verify(g.world.balls[1].active && g.score==0,"virtual pocket carrier does not unalive or score target at creation");
    resolve(g);verify(!g.world.balls[1].active && g.score==6 && g.money==2,"virtual pocket multiplier scores once and invokes target Gold effect");
    g=Game();g.reset();g.snacks={Muffin,MiniMuffin,0,0};g.inventory={};g.inventory[0].id=11;g.inventory[0].score=3;g.begin_round();g.target=100;
    verify(!g.finish_round() && g.world.balls[1].active,"both Muffin selectors leave the shared only target alive while carriers travel");
    resolve(g);verify(g.score==6 && g.money==2,"two carriers aimed at one target cannot pocket or pay it twice");
    g=Game();add_cue(g);add_ball(g,1);g.end_effects_done=true;g.shots=1;
    verify(g.shoot(),"delayed round-end effects do not permanently disable further original shooting");
    g=Game();add_cue(g);add_ball(g,1);add_ball(g,1);g.world.balls[1].active=false;g.world.balls[2].owner=0;g.inventory[0].id=1;g.snacks={Cereal,0,0,0};
    g.world.events.push(EventKind::Pocketed,1,0,0);g.dispatch();g.world.balls[2].active=false;g.world.events.push(EventKind::Pocketed,2,0,1);g.dispatch();
    verify(g.cereal_used && g.inventory[0].score==2,"temporary pocket leaves Cereal available for the first later owned ball");
    std::printf("Listener checks: %u\n",checks);return checks;
}
