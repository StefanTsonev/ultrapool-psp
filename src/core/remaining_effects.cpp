#include "ultrapool/core.hpp"
#include <algorithm>
namespace up {
void Game::burger_neighbors(unsigned index){
    int owner=world.balls[index].owner;if(owner<0)owner=14;
    int row=0;while((row+1)*(row+2)/2<=owner)++row;int column=owner-row*(row+1)/2;
    for(unsigned i=1;i<world.count;++i){int peer=world.balls[i].owner;if(peer<0 || peer>=15 || i==index)continue;
        int r=0;while((r+1)*(r+2)/2<=peer)++r;int c=peer-r*(r+1)/2;
        if((r==row && std::abs(c-column)==1) || (r==row-1 && (c==column-1 || c==column)) || (r==row+1 && (c==column || c==column+1)))queue_ball_modifier(i,Modifier::Heavy,world.balls[index].level,world.balls[index].p);
    }
}
void Game::puzzle_effect(unsigned index,unsigned level,Vec origin){
    unsigned locked[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]) && world.balls[i].locked)locked[count++]=i;
    for(unsigned i=count;i>1;--i)std::swap(locked[i-1],locked[random()%i]);
    if(count)queue_ball_modifier(locked[0],Modifier::Unlock,2*static_cast<int>(level),origin);
    lock_random_other(max_balls,2*static_cast<int>(level),origin);world.balls[index].flash=.5f;
}
void Game::reaper_pull(unsigned index,unsigned level,Vec origin){
    const Ball& source=world.balls[index];unsigned other=source.last_contact;
    if(other>=world.count)return;
    Ball& recipient=world.balls[other];if(recipient.generation!=source.last_contact_generation || !recipient.active || recipient.gone || recipient.last_pocket_tick==world.metrics.ticks)return;
    recipient.pull_time=static_cast<float>(level);recipient.pull_target=origin;recipient.pulled=true;set_ball_locked(recipient,false);recipient.def.mass=100;
}
}
