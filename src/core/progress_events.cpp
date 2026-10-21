#include "ultrapool/core.hpp"
#include <algorithm>
#include <cstring>
namespace up {
void Game::progress_track(const char* key,int64_t value,bool maximum,bool per_round){if(!seeded_run && !trial)profile.track(key,value,maximum,per_round);}
void Game::progress_increment(const char* key){progress_track(key,profile.round_stat(key)+1,true,true);}
void Game::progress_pocket(const Ball& b,int scored){
    if(tutorial.enabled){tutorial.trigger("pocket-1",profile.tutorial_groups);if(rounds_played>=2)tutorial.trigger("pocket-mult",profile.tutorial_groups);}
    if(seeded_run || trial)return;
    profile.track("STAT_BALL_POCKETED");++round_pocketed;profile.track("ROUND_POCKETED",round_pocketed,true,true);
    profile.track("STAT_BALLS_POCKETED_SINGLE_SHOT",int64_t(pockets_after_last_shot)+1,true);
    auto total=profile.stat("STAT_BALL_POCKETED");if(total==100)profile.unlock("pocket_100_balls");if(total==1000)profile.unlock("pocket_1000_balls");
    if(scored>=100)profile.unlock("pocket_100_score");
    if(scored>=1000)profile.unlock("pocket_1000_score");
    const BallKey identities[]={BallKey::EGG,BallKey::RAT,BallKey::ANT,BallKey::ALIEN,BallKey::FROG};
    const char* counters[]={"EGGS_POCKETED_ROUND","RATS_POCKETED","ANTS_POCKETED","ALIENS_POCKETED","FROGS_POCKETED"};
    for(unsigned i=0;i<5;++i)if(b.def.id==id_of(identities[i]) || b.mixed_with==id_of(identities[i])){
        unsigned stat=0;for(;stat<profile_stat_count;++stat)if(!std::strcmp(profile_stat_key(stat),counters[i]))break;
        if(stat<profile_stat_count)profile.track(counters[i],profile.round_stats[stat]+1,true,true);
    }
    if(b.flaming){unsigned stat=0;for(;stat<profile_stat_count;++stat)if(!std::strcmp(profile_stat_key(stat),"POCKETED_BURNING"))break;if(stat<profile_stat_count)profile.track("POCKETED_BURNING",profile.round_stats[stat]+1,true,true);}
    if(b.def.id==id_of(BallKey::SATELLITE) || b.mixed_with==id_of(BallKey::SATELLITE))profile.unlock("pocket_satellite");
    if(b.def.id==id_of(BallKey::BREAD) || b.mixed_with==id_of(BallKey::BREAD))profile.track("POCKET_BREAD_SCORE",b.def.base_score,true);
    if(b.def.base_score==7){if(++sevens_in_a_row==3)profile.unlock("slots_and_daggers");}else sevens_in_a_row=0;
}
void Game::progress_snapshot(){
    unsigned planets=0,zombies=0,locked=0;int largest_number=0;
    for(const auto& item:inventory)if(item.id){
        profile.see_ball(item.id);if(item.mixed_with)profile.see_ball(item.mixed_with);
        uint32_t tags=ball_info(item.id).tags|(item.mixed_with?ball_info(item.mixed_with).tags:0);
        if(tags&static_cast<uint32_t>(Tag::TAG_PLANET))++planets;
        if(tags&static_cast<uint32_t>(Tag::TAG_NUMBER))largest_number=std::max(largest_number,item.score);
        if((item.id==id_of(BallKey::BUTTERFLY)||item.mixed_with==id_of(BallKey::BUTTERFLY)) && !seeded_run && !trial)profile.unlock("have_butterfly");
    }
    for(unsigned id:snacks)if(id)profile.see_snack(id);
    for(const auto& cube:cubes)if(cube.id)profile.see_ball(cube.id);
    for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i])){zombies+=has_ability(world.balls[i],Zombie);locked+=world.balls[i].locked;}
    progress_track("PLANET_COUNT",planets,true);progress_track("OVERFLOW_SIZE",largest_number,true);
    progress_track("ZOMBIE_COUNT",zombies,true);progress_track("LOCKED_BALLS",locked,true);
    for(unsigned i=0;i<world.pocket_count();++i){const auto& p=world.pocket_at(i);progress_track("POCKET_MULT",static_cast<int64_t>(std::floor(p.multiplier)),true);progress_track("POCKET_EXTRA",p.bonus,true);}
    progress_track("BLACKHOLE_COUNT",world.hole_count,true);
    int largest_flower=0;for(const auto& p:pickups)if(p.active && p.kind==PickupKind::Flower)largest_flower=std::max(largest_flower,p.power);
    progress_track("FLOWER_AMOUNT",flower_count(),true);progress_track("FLOWER_COUNT",flower_count(),true,true);progress_track("FLOWER_SIZE",largest_flower,true);
}
bool Game::continue_endless(){
    if(phase!=Phase::RunWon)return false;
    if(!profile.endless_unlocked()){notice("Endless unlocks after three wins");return false;}
    endless=true;enter_shop();round_summary=false;notice("Endless: continue with your current build");return true;
}
}