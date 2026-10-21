#include "ultrapool/core.hpp"
namespace up {
namespace {
void assign_side(Ball& b,unsigned id,unsigned side){
    if(side)b.mixed_with=static_cast<uint16_t>(id);
    else {b.def.id=static_cast<uint16_t>(id);b.def.ability=builtin(id).ability;b.def.color=builtin(id).color;}
    normalize_numbers(b);
}
}
void Game::mimic_front(unsigned index,unsigned side){
    unsigned front=max_balls;for(unsigned i=1;i<world.count;++i)if(world.balls[i].owner==0){front=i;break;}
    if(front==max_balls || index>=world.count)return;
    Ball pattern=world.balls[front];Ball& b=world.balls[index];
    if(b.base_value+b.temp_value!=b.def.base_score)b.base_value=b.def.base_score-b.temp_value;
    before_transform(index);
    if(!side){assign_side(b,pattern.def.id,0);if(!b.mixed_with && pattern.mixed_with)assign_side(b,pattern.mixed_with,1);}
    else assign_side(b,pattern.mixed_with?pattern.mixed_with:pattern.def.id,1);
    refresh_initial_weight(b);retire_listener(index);sync_listeners();b.flash=.5f;
    emit_semantic(Signal::TransformAnother,0,index);
    // Round setup emits the shared Spawn pass later; do not run it early here.
}
void Game::witch_front(unsigned index,unsigned level){
    // Despite the helper's name, the recovered rule explicitly selects build index four.
    for(unsigned i=1;i<world.count;++i)if(world.balls[i].owner==4){queue_ball_modifier(i,Modifier::Reroll,static_cast<int>(level),world.balls[index].p);break;}
}
void Game::reroll_body(unsigned index){
    if(index>=world.count)return;
    Ball& b=world.balls[index];const unsigned old_primary=b.def.id,old_secondary=b.mixed_with;
    before_transform(index);
    unsigned next=roll_content(old_primary,old_secondary,0,round?round-1:0);
    if(b.mixed_with)next=roll_content(old_primary,old_secondary,0,round?round-1:0);
    if(next)assign_side(b,next,0);
    if(b.owner>=0 && inventory[b.owner].id && next){inventory[b.owner].id=static_cast<uint16_t>(next);normalize_numbers(inventory[b.owner]);}
    if(b.mixed_with){
        next=roll_content(old_primary,old_secondary,0,round?round-1:0);
        if(next){assign_side(b,next,1);if(b.owner>=0 && inventory[b.owner].id){inventory[b.owner].mixed_with=static_cast<uint16_t>(next);normalize_numbers(inventory[b.owner]);}}
    }
    refresh_initial_weight(b);retire_listener(index);sync_listeners();b.flash=.5f;
    // Reroll's source ordering differs from ordinary transformations.
    spawn_effects(index,Unsupported);emit_semantic(Signal::TransformAnother,0,index);
}
}
