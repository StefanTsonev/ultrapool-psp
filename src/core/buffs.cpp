#include "ultrapool/core.hpp"
#include <algorithm>
namespace up {
namespace {
bool identity(const Ball& b,BallKey key) { unsigned id=id_of(key);return b.def.id==id || b.mixed_with==id; }
bool targetable(const Ball& b) { return targetable_ball(b); }
void reconcile(Ball& b) { if(b.base_value+b.temp_value!=b.def.base_score) b.base_value=b.def.base_score-b.temp_value; }
}
int Game::gaia_bonus() const {
    int bonus=0;for(unsigned i=1;i<world.count;++i) if(targetable(world.balls[i]) && has_ability(world.balls[i],Gaia)) bonus+=world.balls[i].level*(world.balls[i].flaming?2:1);
    return bonus;
}
void Game::buff_ball(unsigned index,int amount,bool chain,bool is_moss) {
    if(cap_rewarded || index>=world.count) return;
    Ball& b=world.balls[index];if(b.energy){b.active=false;b.gone=true;b.v=Vec();return;}if(b.cue || (b.falling && amount<0)) return;
    if(amount>0) amount+=gaia_bonus();
    if(identity(b,BallKey::IMP) || identity(b,BallKey::TUNGSTEN_BALL) || identity(b,BallKey::COOTS)) return;
    reconcile(b);
    if(chain) buff_components(index,amount,is_moss);
    b.temp_value+=amount;b.def.base_score=b.base_value+b.temp_value;normalize_numbers(b);b.flash=.25f;
    if(chain) score_changed(index,amount);
    if(b.def.base_score<=0 && !b.falling) { b.active=false;b.gone=true;b.v=Vec();retire_listener(index); }
}
void Game::set_ball_score(unsigned index,int value) {
    if(index>=world.count || !world.balls[index].active) return;
    Ball& b=world.balls[index];if(b.energy){if(value>0)buff_ball(index,value);return;}reconcile(b);
    if(value>b.def.base_score) buff_ball(index,value-b.def.base_score);
    else { b.base_value=value;b.temp_value=0;b.def.base_score=value;normalize_numbers(b);b.flash=.25f; }
}
void Game::buff_others(unsigned exclude,int amount,unsigned quantity,Vec origin,bool is_moss) {
    unsigned eligible[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i) if(i!=exclude && targetable(world.balls[i])) eligible[count++]=i;
    for(unsigned i=count;i>1;--i) std::swap(eligible[i-1],eligible[random()%i]);
    for(unsigned i=0;i<std::min(count,quantity);++i) queue_ball_buff(eligible[i],amount,origin,false,is_moss);
}
void Game::buff_components(unsigned index,int amount,bool is_moss) { run_ball_event(Signal::Buff,index,amount,max_balls,6,is_moss); }
void Game::score_changed(unsigned index,int amount) { run_ball_event(Signal::ScoreChange,index,amount); }
bool Game::transform_ball(unsigned index,unsigned id,int side,bool permanent,bool transform_after) {
    if(index>=world.count || id>=content_count || !ball_supported(id) || (side!=0 && side!=1 && side!=-1)) return false;
    Ball& b=world.balls[index];if(b.cue || b.energy) return false;reconcile(b);
    if(has_ability(b,Caterpillar) && id==id_of(BallKey::BUTTERFLY) && !seeded_run && !trial)profile.unlock("transform_caterpillar");
    if(!transform_after)before_transform(index);
    unsigned actual=number_ball(id)?number_id(b.base_value):id;
    if(side==0 || side==-1) { b.def.id=static_cast<uint16_t>(actual);b.def.ability=builtin(actual).ability;b.def.color=builtin(actual).color; }
    if(side==1 || side==-1) b.mixed_with=static_cast<uint16_t>(actual);
    if(b.mixed_with==b.def.id) b.mixed_with=0;
    if(permanent && b.owner>=0 && inventory[b.owner].id) {
        OwnedBall& item=inventory[b.owner];
        if(side==0 || side==-1) item.id=static_cast<uint16_t>(id);
        if(side==1 || side==-1) item.mixed_with=static_cast<uint16_t>(id);
        normalize_numbers(item);
    }
    if(transform_after)run_ball_event(Signal::Transform,index);
    refresh_initial_weight(b);retire_listener(index);sync_listeners();b.flash=.5f;
    emit_semantic(Signal::TransformAnother,0,index);spawn_effects(index,Unsupported);return true;
}
void Game::spawn_effects(unsigned index,unsigned source_ability) { run_ball_event(Signal::Spawn,index,0,max_balls,6,false,source_ability); }
void Game::emit_semantic(Signal signal,int amount,unsigned index) {
    if(cap_rewarded) return;
    SemanticEvent event;event.signal=signal;event.amount=amount;
    if(index<world.count) { event.source.index=static_cast<uint16_t>(index);event.source.generation=world.balls[index].generation; }
    if(!semantics.push(event)) notice("Semantic event capacity exceeded");
    if(!semantic_busy) drain_semantic();
}
void Game::drain_semantic() {
    if(semantic_busy) return;
    semantic_busy=true;SemanticEvent event;unsigned processed=0;
    while(processed<4096 && semantics.pop(event)) { ++processed;dispatch_semantic(event); }
    if(!semantics.empty()) ++semantic_yields;
    semantic_busy=false;
}
void Game::dispatch_semantic(const SemanticEvent& event) {
    if(cap_rewarded) return;
    sync_listeners();const auto snapshot=listeners.snapshot(event.signal);
    for(unsigned listener=0;listener<snapshot.count;++listener) {
        EntityRef ref=snapshot.entities[listener];if(!live_listener(ref)) continue;
        if(event.signal==Signal::MaxRoll && (event.source.index>=world.count || world.balls[event.source.index].generation!=event.source.generation)) continue;
        if(ref.kind==EntityKind::Cube) {
            if(event.signal==Signal::SpawnAnother && (event.source.index>=world.count || world.balls[event.source.index].generation!=event.source.generation))continue;
            run_cube_event(event.signal,ref.index,6,event.source.index);continue;
        }
        if(ref.kind==EntityKind::Snack) { run_snack_event(event.signal,ref.index,event.source.index);continue; }
        if(ref.kind!=EntityKind::Ball) continue;
        if(event.signal==Signal::Spawn) { spawn_effects(ref.index,Unsupported);continue; }
        if(event.signal==Signal::TransformAnother && (event.source.index>=world.count || world.balls[event.source.index].generation!=event.source.generation)) continue;
        run_ball_event(event.signal,ref.index,event.amount,event.source.index);

    }
}
void Game::before_transform(unsigned index) { run_ball_event(Signal::Transform,index); }
int Game::closest_target(Vec origin,unsigned exclude) {
    int closest=-1;float distance=0;unsigned ties=0;
    for(unsigned i=1;i<world.count;++i) if(i!=exclude && targetable(world.balls[i])) {
        Vec delta=world.balls[i].p-origin;float squared=delta.dot(delta);
        if(closest<0 || squared<distance) { closest=static_cast<int>(i);distance=squared;ties=1; }
        else if(squared==distance && random()%++ties==0) closest=static_cast<int>(i);
    }
    return closest;
}
void Game::buff_random_other(unsigned exclude,int amount,Vec origin) {
    unsigned options[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i) if(i!=exclude && targetable(world.balls[i])) options[count++]=i;
    if(count) queue_ball_buff(options[random()%count],amount,origin,false,false,true);
}
void Game::spawn_data(unsigned id,unsigned level,int value,Vec origin,unsigned source) {
    Ball b;b.def=builtin(id);b.level=static_cast<uint8_t>(level);b.def.base_score=value;
    queue_spawn(b,origin,source);
}
bool Game::virtual_pocket(unsigned index,int multiplier) {
    if(cap_rewarded || index>=world.count || !world.balls[index].active || world.balls[index].cue || world.balls[index].last_pocket_tick==world.metrics.ticks) return false;
    Ball& b=world.balls[index];b.active=false;b.falling=false;b.v=Vec();retire_listener(index);
    return world.events.push(EventKind::Pocketed,index,6,world.metrics.ticks,b.generation,0,multiplier);
}

void Game::refresh_initial_weight(Ball& b) {
    // Original applies the special initial weight of each side in order, retaining
    // an existing weight when the new identity has no initial weight override.
    for(unsigned side=0;side<(b.mixed_with?2u:1u);++side) {
        unsigned id=side?b.mixed_with:b.def.id;
        if(id==id_of(BallKey::FISH) || id==id_of(BallKey::PLUTO) || id==id_of(BallKey::ANT)) { set_ball_weight(b,Weight::Light); }
        if(id==id_of(BallKey::JUPITER)) { set_ball_weight(b,Weight::Heavy); }
        if(id==id_of(BallKey::TUNGSTEN_BALL)) { set_ball_weight(b,Weight::VeryHeavy); }
    }
}
bool Game::transform_like(unsigned index,const Ball& source) {
    if(cap_rewarded || index>=world.count || world.balls[index].cue || world.balls[index].energy) return false;
    Ball pattern=source;Ball& b=world.balls[index];reconcile(b);before_transform(index);
    unsigned first=number_ball(pattern.def.id)?number_id(b.base_value):pattern.def.id;
    unsigned second=pattern.mixed_with?(number_ball(pattern.mixed_with)?number_id(b.base_value):pattern.mixed_with):0;
    b.def.id=static_cast<uint16_t>(first);b.def.ability=builtin(first).ability;b.def.color=builtin(first).color;
    b.mixed_with=static_cast<uint16_t>(first==second?0:second);refresh_initial_weight(b);
    retire_listener(index);sync_listeners();b.flash=.5f;
    emit_semantic(Signal::TransformAnother,0,index);spawn_effects(index,Unsupported);return true;
}

}
