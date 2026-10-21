#include "ultrapool/core.hpp"
#include <algorithm>
namespace up {
void Game::refresh_build_ids(){
    round_build_ids={};round_build_id_count=0;
    for(const OwnedBall& item:inventory)if(item.id)for(unsigned side=0;side<(item.mixed_with?2u:1u);++side){
        uint16_t id=side?item.mixed_with:item.id;bool seen=false;
        for(unsigned n=0;n<round_build_id_count;++n)seen|=round_build_ids[n]==id;
        if(!seen && round_build_id_count<round_build_ids.size())round_build_ids[round_build_id_count++]=id;
    }
}
void Game::queue_scrap_upgrade(int buff,Vec origin){
    unsigned choices[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i){const Ball& b=world.balls[i];if(b.active && !b.energy && (b.def.id==id_of(BallKey::SCRAP) || b.mixed_with==id_of(BallKey::SCRAP)))choices[count++]=i;}
    if(count){queue_ball_modifier(choices[random()%count],Modifier::Level,buff,origin);progress_track("SCRAP_UPGRADED");}
}
void Game::summon_effect(unsigned source,unsigned ability,unsigned level,Vec origin,unsigned other){
    if(cap_rewarded || source>=world.count)return;
    Ball& b=world.balls[source];
    if(ability==AiBrain){
        uint16_t choices[30];unsigned count=0;
        for(unsigned n=0;n<round_build_id_count;++n){unsigned id=round_build_ids[n];if(id!=id_of(BallKey::AI_BRAIN) && !number_ball(id))choices[count++]=static_cast<uint16_t>(id);}
        for(unsigned n=count;n>1;--n)std::swap(choices[n-1],choices[random()%n]);
        unsigned id=count?choices[0]:id_of(BallKey::SCRAP);Ball spawned;spawned.def=builtin(id);spawned.level=static_cast<uint8_t>(level);
        spawned.mixed_with=count>1?choices[1]:0;
        spawned.def.base_score=default_item(id,level).score+(spawned.mixed_with?default_item(spawned.mixed_with,level).score:0);
        queue_spawn(spawned,origin,Unsupported);return;
    }
    if(ability==Drill){Ball spawned;spawned.def=builtin(id_of(BallKey::DRILL));spawned.level=static_cast<uint8_t>(level);spawned.def.base_score=b.def.base_score+3*static_cast<int>(level);sound("drill_ball");queue_spawn(spawned,origin,Unsupported,false,-1,true);return;}
    if(ability==Bear){Ball spawned;spawned.def=builtin(id_of(BallKey::BEAR));spawned.level=static_cast<uint8_t>(level);spawned.def.base_score=default_item(spawned.def.id,level).score;int weight=static_cast<int>(random()%3);queue_spawn(spawned,origin,Bear,false,weight);return;}
    if(ability==Lightbulb){sound("lightbulb_ball");queue_scrap_upgrade(0,origin);spawn_data(id_of(BallKey::SCRAP),level,default_item(id_of(BallKey::SCRAP),level).score,origin,Unsupported);return;}
    if(ability==Coffin){if(other<world.count && world.balls[other].def.id!=id_of(BallKey::SPIRIT) && world.balls[other].mixed_with!=id_of(BallKey::SPIRIT))spawn_data(id_of(BallKey::SPIRIT),level,default_item(id_of(BallKey::SPIRIT),level).score,origin,Unsupported);return;}
    if(ability==RatKing){
        unsigned choices[max_balls],count=0;for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]) && (world.balls[i].def.id==id_of(BallKey::RAT) || world.balls[i].mixed_with==id_of(BallKey::RAT)))choices[count++]=i;
        if(count)queue_ball_modifier(choices[random()%count],Modifier::TransformRat,b.def.base_score,origin);
        return;
    }
    if(ability==Keyring){
        unsigned choices[max_balls],count=0;for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]) && world.balls[i].locked)choices[count++]=i;
        for(unsigned n=count;n>1;--n)std::swap(choices[n-1],choices[random()%n]);
        for(unsigned n=0;n<std::min(level,count);++n)queue_ball_modifier(choices[n],Modifier::Unlock,static_cast<int>(level),origin);
        return;
    }
    if(ability==Vampire){
        int highest=-1,value=0;unsigned ties=0;
        for(unsigned i=1;i<world.count;++i)if(i!=source && targetable_ball(world.balls[i])){int candidate=world.balls[i].def.base_score;if(highest<0 || candidate>value){highest=static_cast<int>(i);value=candidate;ties=1;}else if(candidate==value && random()%++ties==0)highest=static_cast<int>(i);}
        int stolen=std::min(2*static_cast<int>(level),value-1);
        if(highest>=0 && stolen>0){buff_ball(static_cast<unsigned>(highest),-stolen);queue_ball_buff(source,stolen,world.balls[highest].p);}
        return;
    }
    if(ability==Supernova){
        EntityRef choices[max_balls];unsigned count=0;
        for(unsigned i=1;i<world.count;++i){const Ball& candidate=world.balls[i];if(i==source || !targetable_ball(candidate) || has_ability(candidate,Supernova))continue;
            uint32_t mask=ball_info(candidate.def.id).events|(candidate.mixed_with?ball_info(candidate.mixed_with).events:0);
            if(mask&signal_bit(Signal::Pocket)){EntityRef ref;ref.kind=EntityKind::Ball;ref.index=static_cast<uint16_t>(i);ref.generation=candidate.generation;choices[count++]=ref;}
        }
        for(unsigned n=count;n>1;--n)std::swap(choices[n-1],choices[random()%n]);
        for(unsigned n=0;n<std::min(level,count) && !cap_rewarded;++n){const EntityRef& ref=choices[n];if(live_listener(ref))run_ball_event(Signal::Pocket,ref.index);}
    }
}
}
