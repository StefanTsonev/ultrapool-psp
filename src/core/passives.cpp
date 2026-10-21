#include "ultrapool/core.hpp"
#include <algorithm>
namespace up {
namespace {
bool targetable(const Ball& b) { return targetable_ball(b); }
bool identity(const Ball& b,BallKey key) { return b.def.id==id_of(key) || b.mixed_with==id_of(key); }
}
bool Game::deploys_slot(unsigned slot) const {
    if(slot>=inventory.size()) return false;
    if(slot>9) return has_snack(id_of(SnackKey::BIG_NACHO));
    if(slot>5 && has_snack(id_of(SnackKey::SMALL_NACHO))) return false;
    return true;
}
Vec Game::snack_origin(unsigned slot) const {
    unsigned dense=0;for(unsigned i=0;i<slot && i<snacks.size();++i) dense+=snacks[i]!=0;
    return Vec(-384,-360+65*float(dense));
}
unsigned snack_counter_goal(unsigned id) {
    SnackKey key=static_cast<SnackKey>(id);
    if(key==SnackKey::CEREAL) return 1;
    if(key==SnackKey::NUTS || key==SnackKey::CRUNCHY_TACO) return 3;
    if(key==SnackKey::BRAVAS) return 2;
    if(key==SnackKey::EDAMAME || key==SnackKey::KEY_LIME || key==SnackKey::CLASSIC_TACO || key==SnackKey::SOYSAUCE || key==SnackKey::VEGGIE_TACO) return 3;
    if(key==SnackKey::STAR_COOKIE || key==SnackKey::CRISPS) return 5;
    if(key==SnackKey::AFTER_EIGHT) return 8;
    if(key==SnackKey::RAINBOW_TACO || key==SnackKey::FORTUNE_COOKIE) return 10;
    if(key==SnackKey::MOONCAKE) return 8;
    return 0;
}
void Game::run_snack_event(Signal signal,unsigned slot,unsigned source,unsigned pocket) {
    if(cap_rewarded || slot>=snacks.size() || !snacks[slot]) return;
    SnackKey key=static_cast<SnackKey>(snacks[slot]);int& counter=snack_counts[slot];Vec origin=snack_origin(slot);
    if(signal==Signal::MaxRoll) {
        if(key==SnackKey::FORTUNE_COOKIE && counter<10 && source<world.count && !world.balls[source].cue && identity(world.balls[source],BallKey::DICE)) {
            queue_ball_split(source,.5,false,origin);++counter;
        }
        return;
    }
    if(signal==Signal::Shot) {
        if(key==SnackKey::CLASSIC_TACO || key==SnackKey::RAINBOW_TACO || key==SnackKey::VEGGIE_TACO || key==SnackKey::CRUNCHY_TACO) counter=0;
        return;
    }
    if(signal==Signal::RoundStart) {
        if(key==SnackKey::OLIVE) gain_shots(1);
        if(key==SnackKey::BLUE_CHEESE) spawn_data(id_of(BallKey::RAT),1,default_item(id_of(BallKey::RAT)).score,origin,Unsupported);
        if(key==SnackKey::BLACK_OLIVE) { shots=std::max(0,shots-1);gain_money(5,"coin"); }
        if(key==SnackKey::CROISSANT) for(unsigned i=1;i<world.count;++i) if(targetable(world.balls[i])) queue_ball_buff(i,1,origin);
        if(key==SnackKey::STRAWBERRY) queue_pocket_change(random_pocket_id(),0,3,origin);
        if(key==SnackKey::BLACKBERRY) queue_pocket_change(random_pocket_id(),1,-5,origin);
        if(key==SnackKey::HAM_SANDWITCH || key==SnackKey::CRISP_SANDWITCH || key==SnackKey::AFTER_EIGHT)
            for(unsigned p=0;p<6;++p) queue_pocket_change(p,key==SnackKey::HAM_SANDWITCH?1.0f:-1.0f,key==SnackKey::HAM_SANDWITCH?-3:key==SnackKey::CRISP_SANDWITCH?5:0,origin);
        if(key==SnackKey::ESPRESSO) {
            int south=-1;for(unsigned i=1;i<world.count;++i) if(targetable(world.balls[i]) && (south<0 || world.balls[i].p.y>world.balls[south].p.y)) south=static_cast<int>(i);
            if(south>=0) queue_ball_buff(static_cast<unsigned>(south),3,origin);
        }
        if(key==SnackKey::SMALL_NACHO) for(unsigned i=1;i<world.count;++i) if(world.balls[i].owner>=0 && world.balls[i].owner<6 && targetable(world.balls[i])) queue_ball_buff(i,world.balls[i].def.base_score,origin);
        if(key==SnackKey::JASMINE_TEA && world.count) world.balls[0].collect_flowers=true;
        if(key==SnackKey::WINGS && world.count) { world.balls[0].ignore_pockets=true;world.balls[0].def.damping=1.5f*.8f; }
        return;
    }
    if(signal!=Signal::PocketAnother || source>=world.count) return;
    Ball& pocketed=world.balls[source];
    if((key==SnackKey::GUMMYWORM || key==SnackKey::GUMMYBEAR) && counter==0) {
        bool empty=true;for(unsigned i=1;i<world.count;++i)empty&=!world.balls[i].active || world.balls[i].passive || world.balls[i].falling || world.balls[i].energy;
        if(empty && key==SnackKey::GUMMYBEAR){++counter;queue_build_setup();}
        else if(empty){
            unsigned slots[15],count=0;for(unsigned i=0;i<inventory.size();++i)if(inventory[i].id)slots[count++]=i;
            for(unsigned i=count;i>1;--i)std::swap(slots[i-1],slots[random()%i]);
            for(unsigned i=0;i<std::min(3u,count);++i){const auto& owned=inventory[slots[i]];Ball copy;copy.def=builtin(owned.id);copy.level=owned.level;copy.def.base_score=owned.score;copy.mixed_with=owned.mixed_with;copy.owner=static_cast<int8_t>(slots[i]);queue_spawn(copy,origin,Unsupported,true);}
            ++counter;
        }
    }
    if(key==SnackKey::NUTS && ++counter>=3) { counter=0;spawn_data(id_of(BallKey::SCRAP),1,default_item(id_of(BallKey::SCRAP)).score,origin,Unsupported); }
    if(key==SnackKey::CRUNCHY_TACO && counter<3 && ++counter==3) spawn_data(id_of(BallKey::SCRAP),3,8,origin,Unsupported);
    if(key==SnackKey::TOAST && !has_tag(pocketed,Tag::TAG_BREAD)) grow_ball(id_of(BallKey::BREAD),1,origin);
    if(key==SnackKey::SAUSAGE_ROLL) grow_ball(id_of(BallKey::SCREW),2,origin);
    if(key==SnackKey::B_8_BAGEL && pocketed.def.base_score==8) {
        unsigned choices[max_balls],count=0;
        // Original upgrade_number selects any targetable ball despite its name.
        for(unsigned i=1;i<world.count;++i) if(targetable(world.balls[i])) choices[count++]=i;
        if(count) queue_ball_buff(choices[random()%count],8,origin,false,false,true);
    }
    if(key==SnackKey::MOONCAKE && counter<8 && ++counter==8) trigger_pocket_effects();
    if(key==SnackKey::FRUIT_SALAD && (identity(pocketed,BallKey::APPLE) || identity(pocketed,BallKey::ORANGE) || identity(pocketed,BallKey::BANANA))) trigger_pocket_effects(true);
    if(key==SnackKey::DAISY_CUPCAKE) plant_pickup(PickupKind::Flower);
    if(key==SnackKey::SUNFLOWER_CUPCAKE) spend_flowers(1);
    if(key==SnackKey::SOYSAUCE && ++counter>=3) { counter=0;plant_pickup(PickupKind::Oil); }
    if(key==SnackKey::VEGGIE_TACO && counter<3 && ++counter==3) for(unsigned i=0;i<5;++i) plant_pickup(PickupKind::Flower);
    if(key==SnackKey::POKER_CHIP && world.pocket(pocket)) queue_pocket_change(pocket,0,1,origin);
    if(key==SnackKey::CEREAL && counter==0 && pocketed.owner>=0 && inventory[pocketed.owner].id) {
        buff_ball(source,1,false);OwnedBall& owned=inventory[pocketed.owner];owned.score=std::max(1,owned.score+1);normalize_numbers(owned);
        counter=-1;cereal_used=true;notice("CEREAL: owned ball permanently gained +1");
    }
    if(key==SnackKey::AFTER_EIGHT && counter<8 && ++counter==8) for(unsigned p=0;p<6;++p) queue_pocket_change(p,2,0,origin);
    if(key==SnackKey::CLASSIC_TACO && counter<3 && ++counter==3) for(unsigned p=0;p<world.pocket_count();++p) queue_pocket_change(world.pocket_id(p),0,2,origin);
    if(key==SnackKey::RAINBOW_TACO && counter<10 && ++counter==10) for(unsigned p=0;p<world.pocket_count();++p) queue_pocket_change(world.pocket_id(p),1,0,origin);
    if(key==SnackKey::STAR_COOKIE && ++counter>=5) { counter=0;spawn_data(id_of(BallKey::STAR),2,default_item(id_of(BallKey::STAR),2).score,origin,Unsupported); }
    if(key==SnackKey::CRISPS && ++counter>=5) {
        counter=0;const OwnedBall& reserve=inventory[10];
        if(reserve.id && rarity(reserve.id)==0) spawn_data(reserve.id,reserve.level,reserve.score,origin,Unsupported);
    }
    if(key==SnackKey::BRAVAS && ++counter>=2) { counter=0;buff_others(max_balls,1,1,origin); }
    if(key==SnackKey::SCRAMBLED_EGGS && identity(pocketed,BallKey::EGG)) {
        int nearest=closest_target(pocketed.p,source);if(nearest>=0) queue_ball_buff(static_cast<unsigned>(nearest),pocketed.def.base_score,pocketed.p);
    }
    if(key==SnackKey::EDAMAME && ++counter>=3) {
        counter=0;unsigned choices[max_balls],count=0;
        for(unsigned i=1;i<world.count;++i) if(targetable(world.balls[i]) && world.balls[i].weight!=Weight::Light) choices[count++]=i;
        if(count) queue_ball_modifier(choices[random()%count],Modifier::Light,0,origin);
    }
    if(key==SnackKey::KEY_LIME && ++counter>=3) {
        counter=0;unsigned choices[max_balls],count=0;
        for(unsigned i=1;i<world.count;++i) if(targetable(world.balls[i]) && world.balls[i].locked) choices[count++]=i;
        if(count) queue_ball_modifier(choices[random()%count],Modifier::Unlock,2,origin);
    }
    if(key==SnackKey::FISH_FINGER) {
        unsigned choices[max_balls],count=0;for(unsigned i=1;i<world.count;++i) if(targetable(world.balls[i]) && identity(world.balls[i],BallKey::FISH)) choices[count++]=i;
        if(count) queue_ball_modifier(choices[random()%count],Modifier::Shoot,1,origin);
    }
}
void Game::grow_ball(unsigned id,int amount,Vec origin,int weight) {
    if(cap_rewarded) return;
    unsigned choices[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i) {
        const Ball& b=world.balls[i];
        if(targetable_ball(b) && (b.def.id==id || b.mixed_with==id || (id==id_of(BallKey::BREAD) && has_tag(b,Tag::TAG_BREAD)))) choices[count++]=i;
    }
    if(count) {
        unsigned recipient=choices[random()%count];
        if(weight==static_cast<int>(Weight::Light) || weight==static_cast<int>(Weight::Heavy)) queue_ball_modifier(recipient,weight==static_cast<int>(Weight::Light)?Modifier::Light:Modifier::Heavy,amount,origin);
        else queue_ball_buff(recipient,amount,origin);
        return;
    }
    Ball grown;grown.def=builtin(id);grown.level=1;grown.def.base_score=amount;
    if(find_spawn_position(grown.p,grown.def.radius) && spawn_snapshot(grown,Unsupported) && weight>=0) set_ball_weight(world.balls[world.last_added],static_cast<Weight>(weight));
}

void Game::trigger_pocket_effects(bool fruit_only) {
    EntityRef candidates[max_balls];unsigned count=0;
    for(unsigned i=1;i<world.count;++i) {
        const Ball& b=world.balls[i];
        if(!targetable_ball(b) || (fruit_only && !identity(b,BallKey::APPLE) && !identity(b,BallKey::ORANGE) && !identity(b,BallKey::BANANA))) continue;
        EntityRef ref;ref.kind=EntityKind::Ball;ref.index=static_cast<uint16_t>(i);ref.generation=b.generation;candidates[count++]=ref;
    }
    if(fruit_only) {
        if(count) { EntityRef ref=candidates[random()%count];run_ball_event(Signal::Pocket,ref.index); }
        return;
    }
    // Original Mooncake snapshots both lists, then runs every primary before any secondary.
    bool selected[2][max_balls]{};
    for(unsigned i=0;i<count;++i) for(unsigned side=0;side<2;++side)
        selected[side][i]=(side==0 || world.balls[candidates[i].index].mixed_with) && (ability_signals(component_ability(world.balls[candidates[i].index],side))&signal_bit(Signal::Pocket));
    for(unsigned side=0;side<2;++side) for(unsigned i=0;i<count;++i) if(selected[side][i] && live_listener(candidates[i]))
        run_ball_event(Signal::Pocket,candidates[i].index,0,max_balls,6,false,Unsupported,side);
}

}
