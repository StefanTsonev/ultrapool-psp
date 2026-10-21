#include "ultrapool/core.hpp"
#include <cstdio>
#include <cstdlib>
using namespace up;
namespace {
unsigned count=0;
void verify(bool condition,const char* text) { ++count;if(!condition) { std::fprintf(stderr,"FAIL DELAY: %s\n",text);std::exit(1); } }
Game arena(unsigned id=1) {
    Game g;g.phase=Phase::Aiming;g.world.pockets_enabled=false;
    Ball cue;cue.cue=true;cue.p=Vec(-200,0);g.world.add(cue);
    Ball ball;ball.def=builtin(id);g.world.add(ball);return g;
}
void resolve(Game& g) {
    for(unsigned i=0;i<1800 && g.pending_actions();++i) g.update(tick_dt);
    verify(!g.pending_actions(),"delayed effects resolve within their lifetime");
}
unsigned pocket_wisps(const Game& g) { unsigned n=0;for(const auto& a:g.actions) n+=a.kind==ActionKind::PocketChange;return n; }
}
unsigned delayed_effect_tests() {
    count=0;Game g=arena();
    verify(g.queue_pocket_change(0,1.5f,-3,Vec()),"queue signed fractional pocket modifier");
    verify(g.world.pockets[0].bonus==0 && g.world.pockets[0].multiplier==2,"pocket unchanged when wisp is created");
    verify(!g.finish_round(),"manual round end waits for pocket wisps");
    for(unsigned i=0;i<80;++i) g.update(tick_dt);
    verify(g.world.pockets[0].bonus==0 && g.pending_actions(),"pocket wisp cannot land before original homing delay");
    resolve(g);verify(g.world.pockets[0].bonus==-3 && g.world.pockets[0].multiplier==3.5f && g.world.pockets[0].score(1)==-7,"wisp applies signed bonus and fractional multiplier once on arrival");
    verify(!g.queue_pocket_change(6,1,1,Vec()),"invalid pocket target rejected");
    g=arena();g.queue_pocket_change(1,1,0,Vec());g.paused=true;g.update(3);
    verify(g.actions[0].age==0 && g.world.metrics.ticks==0,"pause freezes pocket wisp stage and clock");
    g.paused=false;resolve(g);
    g=arena();g.queue_repeated_pocket(6,1,0,Vec());
    verify(pocket_wisps(g)==1,"Pants emits its first modifier immediately");
    for(unsigned i=0;i<17;++i) g.update(tick_dt);
    verify(pocket_wisps(g)==1,"Pants second emission waits a full tenth second");
    g.update(tick_dt);verify(pocket_wisps(g)==2,"Pants emits second wisp after eighteen fixed ticks");
    resolve(g);float total=0;for(const auto& p:g.world.pockets) total+=p.multiplier;
    verify(total==18,"six Pants emissions eventually add six multiplier increments");
    g=arena(30);g.world.balls[1].active=false;g.world.balls[1].level=2;
    Ball dice;dice.def=builtin(19);dice.p=Vec(100,0);g.world.add(dice);
    g.world.events.push(EventKind::Pocketed,1,0,0);g.dispatch();
    verify(g.world.balls[2].def.base_score==6,"another-pocket Dice consumes first deterministic roll before self effect");
    unsigned spawned_value=0;for(const auto& a:g.actions) if(a.kind==ActionKind::Spawn) spawned_value=(a.flags&32)?a.bonus:a.snapshot.def.base_score;
    verify(spawned_value==3,"Slot self effect consumes the next deterministic roll after listeners");
    g=arena(25);g.world.balls[1].active=false;g.snacks={PokerChip,0,0,0};g.world.pockets[0].bonus=4;
    g.world.events.push(EventKind::Pocketed,1,0,0);g.dispatch();
    verify(g.score==12 && g.world.pockets[0].bonus==0 && g.world.pockets[1].bonus==0,"Juggling scores and resets current pocket before any buff lands");
    verify(pocket_wisps(g)==3,"Poker Chip and both adjacent Juggling buffs use separate carriers");
    resolve(g);verify(g.world.pockets[0].bonus==1 && g.world.pockets[1].bonus==6 && g.world.pockets[5].bonus==6,"Poker Chip lands after redistribution without being copied into neighbor bonuses");
    g=Game();g.reset();g.snacks={0,Strawberry,HamSandwich,0};g.begin_round();
    unsigned first=0,second=0;for(const auto& a:g.actions) if(a.kind==ActionKind::PocketChange) {
        verify(a.p.x==-384,"passive source uses original canonical table offset");
        first+=a.p.y==-360;second+=a.p.y==-295;
    }
    verify(first==1 && second==6,"sparse passive slots use original compact visual positions for wisp origins");
    g=arena(9);g.world.balls[1].level=101;g.world.balls[0].v=Vec(1000,0);g.queue_pocket_change(0,1,5,Vec());
    g.world.events.push(EventKind::Wall,1,0,0);g.dispatch();
    verify(g.cap_rewarded && g.money==10 && g.score==100 && g.shots==0 && !g.pending_actions(),"score overflow clears carriers and awards original ten-money bonus once");
    verify(!g.queue_pocket_change(0,1,5,Vec()) && !g.queue_impulse(0,Vec(1,0)),"broken-round state rejects further ability actions");
    for(unsigned i=0;i<12 && g.phase!=Phase::Shop;++i) g.update(.1);
    verify(g.phase==Phase::Shop && g.world.moving(),"overflow finishes round without waiting for unrelated physical motion");
    int settled_money=g.money;verify(!g.finish_round() && g.money==settled_money,"overflow round cannot settle or pay rewards twice");
    g=arena(9);g.world.balls[1].level=100;g.world.events.push(EventKind::Wall,1,0,0);g.dispatch();
    verify(!g.cap_rewarded && g.score==100,"exact score ceiling is valid; only crossing it breaks the round");
    g=arena(1);g.world.balls[1].active=false;dice=Ball();dice.def=builtin(19);dice.falling=true;g.world.add(dice);
    g.world.events.push(EventKind::Pocketed,1,0,0);g.dispatch();
    verify(g.world.balls[2].def.base_score==6,"falling but still-alive listeners retain original event participation");
    std::printf("Delayed-effect checks: %u\n",count);return count;
}
