#include "ultrapool/core.hpp"
#include <algorithm>
namespace up {
void Game::queue_nudges(unsigned amount,Vec origin,bool prefer_small) {
    unsigned options[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]))options[count++]=i;
    for(unsigned i=count;i>1;--i)std::swap(options[i-1],options[random()%i]);
    if(prefer_small)std::stable_sort(options,options+count,[this](unsigned a,unsigned b){return world.balls[a].weight<world.balls[b].weight;});
    for(unsigned i=0;i<std::min(amount,count);++i)queue_ball_modifier(options[i],Modifier::Nudge,0,origin);
}
bool Game::queue_boom(Vec origin,Vec destination) {
    if(cap_rewarded || !std::isfinite(destination.x) || !std::isfinite(destination.y))return false;
    Action& action=allocate_action();start_wisp(action,ActionKind::Boom,origin,destination);return true;
}
void Game::queue_random_boom(Vec origin) {
    // Exact topLeftExtended/bottomRightExtended coordinates from the table resource.
    queue_boom(origin,Vec(-268+533*(random()/4294967296.0f),-418+833*(random()/4294967296.0f)));
}
void Game::queue_boom_series(unsigned amount,Vec origin) {
    if(!amount || cap_rewarded)return;
    queue_random_boom(origin);
    if(amount>1){Action& action=allocate_action();action.kind=ActionKind::RepeatBoom;action.p=origin;action.repeats=amount-1;action.due=world.metrics.ticks+18;}
}
void Game::queue_apple_buffs(unsigned count,unsigned exclude,Vec origin) {
    if(cap_rewarded || !count)return;
    buff_random_other(exclude,1,origin);
    if(count>1){Action& action=allocate_action();action.kind=ActionKind::RepeatBuff;action.target=exclude;action.generation=exclude<world.count?world.balls[exclude].generation:0;action.p=origin;action.repeats=count-1;action.due=world.metrics.ticks+36;}
}
void Game::hold_round(unsigned ticks) {
    if(cap_rewarded || !ticks)return;
    Action& action=allocate_action();action.kind=ActionKind::Wait;action.due=world.metrics.ticks+ticks;
}
void Game::queue_build_setup() {
    if(cap_rewarded)return;
    refresh_build_ids();
    Action& action=allocate_action();action.kind=ActionKind::SetupBuild;action.due=world.metrics.ticks+9;
    bool any=false;for(unsigned i=0;i<inventory.size();++i)any|=deploys_slot(i) && inventory[i].id;
    if(!any){action.flags=1;action.due=world.metrics.ticks+18;}
}
void Game::queue_tiger(float strength,bool round_end) {
    if(cap_rewarded)return;
    progress_increment("TIGER_TRIGGERED");progress_track("TOTAL_TIGERS_TRIGGERED");
    Action& action=allocate_action();action.kind=ActionKind::Tiger;
    action.multiplier=strength;action.due=world.metrics.ticks+288;
    action.repeats=round_end?252:108;
    sound("tiger_cut_in");
    effect(Vec(),0xff44bbff,160);
}
void Game::blast_at(Vec position) {
    if(cap_rewarded)return;
    effect(position,0xff6688ff,200);
    for(unsigned i=1;i<world.count;++i){
        const Ball& ball=world.balls[i];if(!ball.active || ball.passive || ball.energy)continue;
        Vec delta=ball.p-position;float distance=delta.length();
        if(distance>0 && distance<=200)queue_impulse(i,delta.unit()*(600*(1-distance/200)));
    }
}
}
