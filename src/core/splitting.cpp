#include "ultrapool/core.hpp"
#include <algorithm>
namespace up {
namespace {
bool targetable(const Ball& b) { return targetable_ball(b); }
bool shares_identity(const Ball& a,const Ball& b) {
    return a.def.id==b.def.id || (a.mixed_with && a.mixed_with==b.def.id) || (b.mixed_with && (a.def.id==b.mixed_with || (a.mixed_with && a.mixed_with==b.mixed_with)));
}
}
bool Game::queue_ball_split(unsigned index,double fraction,bool lower,Vec origin,int buff) {
    if(cap_rewarded || index>=world.count || !world.balls[index].active || world.balls[index].cue || world.balls[index].energy || !std::isfinite(fraction) || fraction<0) return false;
    { Action& a=allocate_action();
        start_wisp(a,ActionKind::SplitBall,origin,world.balls[index].p);a.target=index;a.generation=world.balls[index].generation;
        a.multiplier=fraction;a.flags=lower?1:0;a.bonus=buff;return true;
    }
}
bool Game::split_ball(unsigned index,double fraction,bool lower) {
    if(cap_rewarded || index>=world.count || !world.balls[index].active || world.balls[index].cue || world.balls[index].energy || !std::isfinite(fraction) || fraction<0) return false;
    Ball& original=world.balls[index];
    if(original.base_value+original.temp_value!=original.def.base_score) original.base_value=original.def.base_score-original.temp_value;
    int value=static_cast<int>(std::ceil(original.def.base_score*fraction));Vec origin=original.p;
    if(!original.falling) { original.active=false;original.gone=true;original.v=Vec();retire_listener(index); }
    if(lower) set_ball_weight(original,original.weight==Weight::Heavy?Weight::Normal:Weight::Light);
    Ball copy=original;set_ball_locked(copy,false);
    // Ball.set_item reassigns only the primary identity using the old base.
    if(number_ball(copy.def.id)) copy.def.id=static_cast<uint16_t>(number_id(copy.base_value));
    if(copy.def.id==copy.mixed_with) copy.mixed_with=0;
    if(number_ball(copy.def.id) && !copy.mixed_with) value+=copy.temp_value;
    copy.base_value=value;copy.temp_value=0;copy.def.base_score=value;
    if(number_ball(copy.def.id) && !copy.mixed_with) copy.def.id=static_cast<uint16_t>(number_id(value));
    copy.def.ability=builtin(copy.def.id).ability;copy.def.color=builtin(copy.def.id).color;
    float direction=(random()/4294967296.0f)*6.283185307f;
    copy.p=origin+Vec(10*(std::cos(direction)-std::sin(direction)),10*(std::sin(direction)+std::cos(direction)));
    bool first=spawn_snapshot(copy,Unsupported,true);if(first) set_ball_locked(world.balls[world.last_added],false);
    copy.p=origin;bool second=spawn_snapshot(copy,Unsupported,true);if(second) set_ball_locked(world.balls[world.last_added],false);
    effect(origin,0xffa4e8a2);return first || second;
}
void Game::split_random(unsigned exclude,double fraction,Vec origin) {
    unsigned choices[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i) if(i!=exclude && targetable(world.balls[i])) choices[count++]=i;
    if(count) queue_ball_split(choices[random()%count],fraction,false,origin);
}
void Game::gift_effect(unsigned level,Vec origin) {
    for(unsigned i=1;i<world.count;++i) if(targetable(world.balls[i])) {
        unsigned count=0;for(unsigned j=1;j<world.count;++j) if(targetable(world.balls[j]) && shares_identity(world.balls[i],world.balls[j])) ++count;
        if(count>=2) queue_ball_buff(i,static_cast<int>((count-1)*level),origin);
    }
}
void Game::shuffle_scores(unsigned level) {
    unsigned choices[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i) if(world.balls[i].active && !world.balls[i].passive && !world.balls[i].energy) choices[count++]=i;
    for(unsigned i=0;i<count;++i) buff_ball(choices[i],level);
    for(unsigned i=count;i>1;--i) std::swap(choices[i-1],choices[random()%i]);
    if(!count) return;
    int first=world.balls[choices[0]].def.base_score;
    for(unsigned i=0;i+1<count;++i) set_ball_score(choices[i],world.balls[choices[i+1]].def.base_score);
    set_ball_score(choices[count-1],first);
}
void Game::planet_effect(unsigned ability,unsigned source,unsigned level,Vec origin) {
    unsigned choices[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i) if(targetable(world.balls[i])) {
        const Ball& b=world.balls[i];
        if(ability==Earth?!is_star(b):(is_star(b) && (ability!=Neptune || !b.shielded))) choices[count++]=i;
    }
    if(ability==Earth) {
        Vec position=world.balls[source].p;
        std::sort(choices,choices+count,[&](unsigned a,unsigned b){Vec x=world.balls[a].p-position,y=world.balls[b].p-position;return x.dot(x)<y.dot(y);});
    } else if(ability==Saturn || ability==Mercury || ability==Mars || ability==Uranus)
        for(unsigned i=count;i>1;--i) std::swap(choices[i-1],choices[random()%i]);
    if(ability==Saturn) { if(count) queue_ball_split(choices[0],.2*level,false,origin);return; }
    unsigned limit=ability==Venus?count:ability==Uranus?1:(ability==Mercury || ability==Jupiter || ability==Pluto)?2*level:level;
    for(unsigned i=0;i<std::min(count,limit);++i) {
        unsigned index=choices[i];
        if(ability==Venus) queue_ball_buff(index,2*level,origin);
        if(ability==Earth) queue_ball_modifier(index,Modifier::Star,0,origin);
        if(ability==Mercury) queue_ball_modifier(index,Modifier::Shoot,1,origin);
        if(ability==Mars) queue_ball_modifier(index,Modifier::Flame,1,origin);
        if(ability==Jupiter) queue_ball_modifier(index,Modifier::Heavy,3,origin);
        if(ability==Pluto) queue_ball_modifier(index,Modifier::Light,1,origin);
        if(ability==Neptune) queue_ball_modifier(index,Modifier::Shield,0,origin);
        if(ability==Uranus) queue_ball_modifier(index,Modifier::Lock,4*level,origin);
    }
}
}
