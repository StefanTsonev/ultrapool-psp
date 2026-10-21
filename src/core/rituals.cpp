#include "ultrapool/core.hpp"
#include <algorithm>
namespace up {
void Game::pocket_candle(unsigned strength){
    if(cap_rewarded || candles_pocketed>=5)return;
    ++candles_pocketed;ritual_strength+=strength;ritual_progress=0;
    if(candles_pocketed==5){Action& a=allocate_action();a.kind=ActionKind::Ritual;a.due=world.metrics.ticks+60;a.repeats=ritual_strength;}
}
void Game::ritual_nudge(){
    unsigned options[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]))options[count++]=i;
    // choose_random_balls shuffles the current candidates anew at each pulse.
    for(unsigned i=count;i>1;--i)std::swap(options[i-1],options[random()%i]);
    if(count)queue_ball_modifier(options[0],Modifier::ShieldNudge,0,Vec(),3);
}
void Game::echo_pocket(unsigned receiver,unsigned source,unsigned side,unsigned level){
    if(receiver>=world.count || source>=world.count || world.balls[receiver].parrot_used)return;
    const Ball& pocketed=world.balls[source];unsigned ids[2]={pocketed.def.id,pocketed.mixed_with};
    bool eligible=false;for(unsigned n=0;n<2;++n)if(ids[n] && (ball_info(ids[n]).events&signal_bit(Signal::Pocket)))eligible=true;
    if(!eligible)return;
    world.balls[receiver].parrot_used=true;world.balls[receiver].flash=.5f;
    for(unsigned n=0;n<2;++n)if(ids[n] && (ball_info(ids[n]).events&signal_bit(Signal::Pocket))){
        EffectCall call;call.signal=Signal::Pocket;call.receiver.index=static_cast<uint16_t>(receiver);call.receiver.generation=world.balls[receiver].generation;
        call.ability=ball_info(ids[n]).ability;call.side=static_cast<uint8_t>(side);call.level=static_cast<uint8_t>(level);call.origin=world.balls[receiver].p;
        Action& a=allocate_action();a.kind=ActionKind::RepeatEffect;a.call=call;a.due=world.metrics.ticks+36;
    }
}
void Game::observatory_copy(unsigned source,unsigned level){
    if(source>=world.count || world.balls[source].fleeting)return;
    Ball copy=world.balls[source];copy.def.base_score=static_cast<int>(std::ceil(double(copy.def.base_score)*(.2*level)));copy.base_value=copy.def.base_score;copy.temp_value=0;
    if(!find_spawn_position(copy.p,copy.def.radius))return;
    // The original immediate copy dispatches Spawn at the source level, then sets level one and fleeting.
    unsigned index=0;
    if(spawn_snapshot(copy,Observatory,false,false,false,false,&index)) {Ball& spawned=world.balls[index];spawned.level=1;spawned.fleeting=true;}
}
}
