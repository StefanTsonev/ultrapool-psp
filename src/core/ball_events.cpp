#include "ultrapool/core.hpp"
#include <algorithm>
namespace up {
bool Game::change_random_pocket_lock(bool closed) {
    unsigned candidates[6], count=0;
    for(unsigned i=0;i<6;++i) if(world.pockets[i].closed!=closed) candidates[count++]=i;
    return count && world.set_pocket_closed(candidates[random()%count],closed);
}
bool is_star(const Ball& b) { return b.star_power || has_tag(b,Tag::TAG_STAR); }
void Game::run_ball_event(Signal signal,unsigned index,int amount,unsigned other,unsigned pocket,bool is_moss,unsigned source_ability,unsigned only_side) {
    if(cap_rewarded || index>=world.count || world.balls[index].cue || world.balls[index].energy || world.balls[index].gone) return;
    for(unsigned side=0;side<(world.balls[index].mixed_with?2u:1u);++side) {
        if(only_side<2 && side!=only_side) continue;
        Ball& b=world.balls[index];unsigned ability=component_ability(b,side);
        if(!(ability_handlers(ability)&signal_bit(signal)) || (signal==Signal::Spawn && ability==source_ability)) continue;
        EffectCall call;call.signal=signal;call.receiver.index=static_cast<uint16_t>(index);call.receiver.generation=b.generation;
        call.other.index=static_cast<uint16_t>(other);if(other<world.count) call.other.generation=world.balls[other].generation;
        call.ability=static_cast<uint16_t>(ability);call.side=static_cast<uint8_t>(side);call.level=b.level;
        call.pocket=static_cast<uint8_t>(pocket);call.amount=amount;call.is_moss=is_moss;call.shot_direction=shot_direction;
        call.origin=signal==Signal::Pocket && world.pocket(pocket)?world.pocket(pocket)->p:b.p;
        bool repeat=b.flaming;run_effect(call);
        if(repeat && !cap_rewarded) {
            Action& a=allocate_action();
            a.kind=ActionKind::RepeatEffect;a.due=world.metrics.ticks+18;a.call=call;
        }
    }
}
void Game::run_effect(const EffectCall& call) {
    unsigned index=call.receiver.index;
    if(cap_rewarded || index>=world.count || world.balls[index].generation!=call.receiver.generation || world.balls[index].gone) return;
    Ball& b=world.balls[index];unsigned ability=call.ability,level=call.level,side=call.side,other=call.other.index;
    if(call.signal==Signal::Shot){
        if(!b.active)return;
        if(ability==Parrot)b.parrot_used=false;
        if(ability==HitBall){float direction=(random()/4294967296.0f)*6.283185307f;queue_impulse(index,Vec(std::cos(direction),std::sin(direction))*600);}
        if(ability==Luna)queue_impulse(index,call.shot_direction*(600*(.7f+.4f*level)));
        return;
    }
    if(call.signal==Signal::ReachScore && ability==Gameball){if(b.active){consume_ball(index,2);gain_money(3*static_cast<int>(level),"coin");}return;}
    if(call.signal==Signal::RoundEnd) {
        if(!b.active || b.falling)return;
        if(ability==Bunny){hold_round(360);virtual_pocket(index,static_cast<int>(level+1));gain_shots(1);}
        if(ability==EggTimer){
            unsigned selected=0;
            for(unsigned target_index=1;target_index<world.count && selected<2*level;++target_index)
                if(target_index!=index && targetable_ball(world.balls[target_index])){queue_ball_modifier(target_index,Modifier::Nudge,0,b.p,2.0f*float(level));++selected;}
            if(selected)hold_round(360);
        }
        return;
    }
    if(call.signal==Signal::Hit || call.signal==Signal::Wall) {
        if(call.signal==Signal::Hit && (other>=world.count || world.balls[other].generation!=call.other.generation)) return;
        contact_component(index,other,call.signal==Signal::Wall,ability,side,level);return;
    }
    if(call.signal==Signal::Spawn && ability==FlowerBush) sound("flowerbush_ball");
    if(call.signal==Signal::RoundStart && ability==Fireball) sound("fireball_ball");
    if(call.signal==Signal::Buff && ability==Bush && call.amount>0 && !call.is_moss) sound("bush_ball");
    if(call.signal==Signal::Buff && ability==Egg) sound("egg_ball");
    if(call.signal==Signal::Transform && ability==Zombie) sound("zombie_ball");
    if(call.signal==Signal::Pocket) { if(ability==FlowerBush) sound("flowerbush_ball");pocket_component(index,call.pocket,ability,level,call.origin);return; }
    if(call.signal==Signal::Buff) {
        int amount=call.amount;
        if(ability==Beans)queue_boom_series(level,b.p);
        if(ability==Bee && amount>0) plant_pickup(PickupKind::Flower,level);
        if(ability==Bush && amount>0) buff_ball(index,level,false);
        if(ability==Antimatter && amount>0) buff_ball(index,-amount*2,false);
        if(ability==Gingerbread) {
            float direction=(random()/4294967296.0f)*6.283185307f;
            queue_impulse(index,Vec(std::cos(direction),std::sin(direction))*(600*(.5f+.25f*level)));
        }
        if(ability==Moss && !call.is_moss) buff_others(index,static_cast<int>(std::ceil(amount*(level*.2f))),1,b.p,true);
        if(ability==Caterpillar && random()%100<level) transform_ball(index,id_of(BallKey::BUTTERFLY),side);
    }
    if(call.signal==Signal::Pickup && ability==Vacuum) buff_ball(index,level);
    if(call.signal==Signal::ScoreChange && ability==Robocook && phase!=Phase::Shop) {
        // The source invokes this directly, despite only subscribing RoboCook to Buff/PocketAnother.
        while(b.def.base_score>3) {
            int previous=b.def.base_score;buff_ball(index,-3,false);
            // Falling and immune mixed identities can reject a debit. Never reproduce the source's infinite loop.
            if(b.def.base_score>=previous)break;
            unsigned candidates[max_balls],count=0;int lowest=0;
            for(unsigned i=1;i<world.count;++i)if(i!=index && targetable_ball(world.balls[i])) {
                int value=world.balls[i].def.base_score;
                if(!count || value<lowest){lowest=value;count=0;}
                if(value==lowest)candidates[count++]=i;
            }
            if(count)queue_ball_modifier(candidates[random()%count],Modifier::Nudge,0,call.origin,.5f);
        }
    }
    if(call.signal==Signal::ScoreChange && ability==Sand && call.amount>0) add_score(call.amount*level,index);
    if(call.signal==Signal::Transform && ability==Incubator) queue_ball_buff(index,3*level,b.p);
    if(call.signal==Signal::TransformAnother && ability==SharkPlush && other<world.count && world.balls[other].generation==call.other.generation) queue_ball_buff(other,3*level,b.p);
    if(call.signal==Signal::Spawn) {
        if(ability==Burger)burger_neighbors(index);
        if(ability==Ooze)buff_ball(index,static_cast<int>(std::ceil(double(b.def.base_score)*(.2*level))));
        if(ability==Rainbow)content_effect(index,ability,level,b.p);
        if(ability==Compass || ability==Lantern){
            int south=-1;for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]) && (south<0 || world.balls[i].p.y>world.balls[south].p.y))south=static_cast<int>(i);
            if(south>=0){if(ability==Compass)queue_ball_buff(static_cast<unsigned>(south),2*level,b.p);else queue_ball_modifier(static_cast<unsigned>(south),Modifier::Shield,level,b.p);}
        }
        if(ability==Gacha){
            buff_ball(index,level);
            if(b.owner>=0 && inventory[b.owner].id)inventory[b.owner].score+=level;
            unsigned result=seeded_content_roll(gacha_rolls,id_of(BallKey::GACHA),0,0,8);
            if(result){sound("gacha_ball");transform_ball(index,result,side,true,true);}
        }
        if(ability==Ghost && !b.ghost_used){b.ghost_used=true;set_ball_shield(b,true);sound("ghost_ball");}
        if(ability==FlowerBush) for(unsigned i=0;i<2*level;++i) plant_pickup(PickupKind::Flower);
        if(ability==Piping) for(unsigned i=0;i<2*level;++i) plant_pickup(PickupKind::Oil);
        if(ability==Stove) for(unsigned i=0;i<level;++i) plant_pickup(PickupKind::Flame);
        if(ability==Chicken) spawn_data(id_of(BallKey::EGG),level,level,b.p,Chicken);
        if(ability==Sun) star_planets(b.p,level);
        if(ability==Starmap) star_south(b.p,level);
        if(ability==Takoyaki) b.flaming=true;
        if(ability==Screw) set_ball_locked(b,true);
        if(ability==Rat) {
            int count=0;for(unsigned j=1;j<world.count;++j)if(j!=index && targetable_ball(world.balls[j])){
                const Ball& rat_peer=world.balls[j];
                count+=(rat_peer.def.id==id_of(BallKey::RAT)||rat_peer.mixed_with==id_of(BallKey::RAT));
                count+=(rat_peer.def.id==id_of(BallKey::RAT_KING)||rat_peer.mixed_with==id_of(BallKey::RAT_KING));
            }
            if(count){sound("rat_ball");buff_ball(index,count*level);}
        }
        if(ability==Mirror) for(unsigned j=1;j<world.count;++j) if(world.balls[j].owner==0) {
            Ball copy=world.balls[j];copy.level=static_cast<uint8_t>(level);queue_spawn(copy,b.p,Mirror);break;
        }
        if(ability==Butterfly) for(unsigned j=1;j<world.count;++j) {
            const Ball& recipient=world.balls[j];
            if(recipient.active && !recipient.passive && !recipient.cue && !recipient.energy && !recipient.falling && recipient.def.id!=id_of(BallKey::IMP) && recipient.mixed_with!=id_of(BallKey::IMP) && recipient.def.id!=id_of(BallKey::TUNGSTEN_BALL) && recipient.mixed_with!=id_of(BallKey::TUNGSTEN_BALL)) queue_ball_buff(j,level,b.p);
        }
    }
    if(call.signal==Signal::SpawnAnother && ability==Gravestone && index!=other && other<world.count && world.balls[other].generation==call.other.generation)queue_ball_buff(other,level,b.p);
    if(call.signal==Signal::Upgrade && b.active && !b.falling && other<world.count && world.balls[other].generation==call.other.generation){
        if(ability==Battery)buff_ball(index,level);
        if(ability==Scrap && index==other){++scrap_transforms;progress_track("SCRAP_UPGRADED",scrap_transforms,true);buff_ball(index,level);unsigned result=seeded_content_roll(scrap_rolls,b.def.id,b.mixed_with,static_cast<uint32_t>(Tag::TAG_TECH),round?round-1:0,false,!side && b.mixed_with);if(result){sound("scrap_ball");transform_ball(index,result,static_cast<int>(side));}}
    }
    if(call.signal==Signal::SpawnAnother && ability==Calculator && index!=other && b.active && !b.falling && other<world.count && world.balls[other].generation==call.other.generation)buff_ball(index,level);
    if(call.signal==Signal::RoundStart){
        if(ability==Fireball)fire_neighbors(index,level);
        if(ability==Mimic)mimic_front(index,side);
        if(ability==Witch)witch_front(index,level);
        if(ability==Ceo)queue_repeated_pocket(level,-1,0,b.p);
        if(ability==Icecube)queue_pocket_shield(random_pocket_id(),level,b.p);
    }

    if(call.signal==Signal::Score) {
        if(!b.active) return;
        int gain=0;if(ability==Basketball && call.amount>b.def.base_score) gain=2*level;
        if(ability==Tennis && call.amount<=static_cast<int>(2*level)) gain=level;
        if(ability==Football && call.amount%10==0) gain=5*level;
        if(gain) buff_ball(index,gain,ability!=Tennis);
    }
    if(call.signal==Signal::PocketAnother) {
        if(!b.active || index==other) return;
        if(ability==ChefHat || ability==Panellet || ability==Nest || ability==Pot || ability==Peacock || ability==Spirit || ability==Turbine || ability==Robot || ability==Snake || ability==Tesla || ability==Ufo || ability==RedDwarf)content_effect(index,ability,level,b.p,other);
        if(ability==Puzzleball)puzzle_effect(index,level,b.p);
        if(ability==TimeMachine && other<world.count && world.balls[other].level>1){Ball copy=world.balls[other];copy.level=1;copy.def.base_score=static_cast<int>(level);copy.base_value=copy.def.base_score;copy.temp_value=0;queue_spawn(copy,b.p,TimeMachine);b.flash=.5f;}
        if(ability==Jellyfish){b.flash=.5f;add_score(static_cast<int>(level),index);}
        if(ability==Observatory)observatory_copy(other,level);
        if(ability==Parrot)echo_pocket(index,other,side,level);
        if(ability==Recycler)queue_scrap_upgrade(static_cast<int>(level),b.p);
        if(ability==Coffin || ability==Vampire)summon_effect(index,ability,level,b.p,other);
        if(ability==RatKing && b.def.id!=id_of(BallKey::RAT) && b.mixed_with!=id_of(BallKey::RAT))buff_ball(index,static_cast<int>(level));
        if(ability==Beans)queue_boom_series(level,b.p);
        if(ability==Rocket){float direction=(random()/4294967296.0f)*6.283185307f;queue_impulse(index,Vec(std::cos(direction),std::sin(direction))*60);b.rocket_time=.2f*float(level);}
        if(ability==Ladybug){set_ball_locked(b,false);b.chase_flowers=level*.5f;}
        if(ability==Dog){int closest=closest_target(b.p,index);if(closest>=0){sound("dog_ball");queue_impulse(index,(world.balls[closest].p-b.p).unit()*(600*(.5f+.25f*level)));}}
        if(ability==Oven && other<world.count && !has_tag(world.balls[other],Tag::TAG_BREAD)) grow_ball(id_of(BallKey::BREAD),static_cast<int>(std::ceil(world.balls[other].def.base_score*.2*level)),b.p);
        if(ability==Fridge) lock_random_other(index,2*level,b.p);
        if(ability==Dice) {
            unsigned roll=1+random()%(6*level);progress_track("MAX_DICE_ROLL",roll,true);progress_increment("DICE_ROLLED_ROUND");
            if(roll>=static_cast<unsigned>(std::floor(6*level*.85))) emit_semantic(Signal::MaxRoll,0,index);
            set_ball_score(index,static_cast<int>(roll));
        }
        if(ability==CoolGuy) buff_ball(index,level);
        if(ability==Donut) buff_others(index,1,2*level,b.p);
        if(ability==Caterpillar && random()%100<level) transform_ball(index,id_of(BallKey::BUTTERFLY),side);
        if(ability==Roulette) queue_pocket_change(random_pocket_id(),0,level,b.p);
        if(ability==Plushie) {
            float direction=(random()/4294967296.0f)*6.283185307f;
            queue_impulse(index,Vec(std::cos(direction),std::sin(direction))*(600*(.5f+level*.25f)));b.flash=.25f;
        }
        normalize_numbers(b);
    }
}
void Game::fire_neighbors(unsigned index,unsigned level) {
    int owner=world.balls[index].owner;if(owner<0 || owner>=15) return;
    int row=0;while((row+1)*(row+2)/2<=owner) ++row;
    int column=owner-row*(row+1)/2;unsigned options[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i) {
        int neighbor=world.balls[i].owner;if(neighbor<0 || neighbor>=15 || i==index) continue;
        int nr=0;while((nr+1)*(nr+2)/2<=neighbor) ++nr;
        int nc=neighbor-nr*(nr+1)/2;
        if((nr==row && std::abs(nc-column)==1) || (nr==row-1 && (nc==column-1 || nc==column)) || (nr==row+1 && (nc==column || nc==column+1))) options[count++]=i;
    }
    for(unsigned i=count;i>1;--i) std::swap(options[i-1],options[random()%i]);
    for(unsigned i=0;i<std::min(count,level);++i) world.balls[options[i]].flaming=true;
}
void Game::destroy_fleeting() {
    for(unsigned i=1;i<world.count;++i) {
        Ball& b=world.balls[i];if(b.active && !b.passive && !b.falling && b.fleeting) { b.active=false;b.gone=true;b.v=Vec();retire_listener(i); }
    }
}
namespace {
bool targetable_star_effect(const Ball& b) { return targetable_ball(b); }
}
void Game::star_planets(Vec origin,int buff) {
    for(unsigned i=1;i<world.count;++i) if(targetable_star_effect(world.balls[i]) && (has_tag(world.balls[i],Tag::TAG_PLANET) || has_tag(world.balls[i],Tag::TAG_PLANET_))) queue_ball_modifier(i,Modifier::Star,buff,origin);
}
void Game::star_south(Vec origin,int buff) {
    int chosen=-1;for(unsigned i=1;i<world.count;++i) if(targetable_star_effect(world.balls[i]) && (chosen<0 || world.balls[i].p.y>world.balls[chosen].p.y)) chosen=static_cast<int>(i);
    if(chosen>=0) queue_ball_modifier(static_cast<unsigned>(chosen),Modifier::Star,buff,origin);
}
void Game::score_stars(Vec origin,unsigned count) {
    unsigned eligible[max_balls],size=0;
    for(unsigned i=1;i<world.count;++i) if(targetable_star_effect(world.balls[i]) && is_star(world.balls[i])) eligible[size++]=i;
    std::sort(eligible,eligible+size,[&](unsigned a,unsigned b) { Vec x=world.balls[a].p-origin,y=world.balls[b].p-origin;return x.dot(x)<y.dot(y); });
    if(size && count) sound("star_ball");
    progress_track("SCORED_STARS_COUNT",std::min(size,count),true);
    for(unsigned i=0;i<std::min(size,count);++i) { unsigned index=eligible[i];world.balls[index].flash=.25f;add_score(world.balls[index].def.base_score,index); }
}

}
