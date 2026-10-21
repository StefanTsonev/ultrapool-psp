#include "ultrapool/core.hpp"
#include <algorithm>
#include <limits>
namespace up {
unsigned component_ability(const Ball& ball,unsigned side) {
    if(side==0) return ball.def.ability;
    if(ball.mixed_with) return builtin(ball.mixed_with).ability;
    return Plain;
}
bool has_ability(const Ball& b,unsigned ability) {
    return b.def.ability==ability || (b.mixed_with && builtin(b.mixed_with).ability==ability);
}
bool has_number(const OwnedBall& item) { return number_ball(item.id) || (item.mixed_with && number_ball(item.mixed_with)); }
void normalize_numbers(OwnedBall& item) {
    bool first=number_ball(item.id),second=item.mixed_with && number_ball(item.mixed_with);
    if(first) item.id=static_cast<uint16_t>(number_id(item.score));
    if(second) item.mixed_with=first?0:static_cast<uint16_t>(number_id(item.score));
    if(item.id==item.mixed_with) item.mixed_with=0;
}
void normalize_numbers(Ball& ball) {
    OwnedBall item; item.id=ball.def.id; item.mixed_with=ball.mixed_with; item.score=ball.def.base_score;
    normalize_numbers(item); ball.def.id=item.id; ball.mixed_with=item.mixed_with;
    if(number_ball(item.id)) ball.def.ability=builtin(item.id).ability;
}
bool can_combine(const OwnedBall& source,const OwnedBall& destination) {
    if(!source.id || !destination.id) return false;
    if(destination.level>=5 && !(has_number(source) && has_number(destination))) return false;
    if((source.id==27 || source.mixed_with==27) && source.level>=destination.level) return true;
    if(source.mixed_with) return destination.mixed_with && source.level==destination.level &&
        ((source.id==destination.id && source.mixed_with==destination.mixed_with) ||
         (source.id==destination.mixed_with && source.mixed_with==destination.id));
    if(has_number(source) && has_number(destination)) return true;
    return source.level==destination.level && destination.level<5 &&
        (source.id==destination.id || source.id==destination.mixed_with);
}
void combine_values(const OwnedBall& source,OwnedBall& destination) {
    destination.score+=source.score;
    if(!number_ball(source.id) || source.mixed_with) ++destination.level;
    normalize_numbers(destination);
}
int item_price(const OwnedBall& item) {
    int price=shop_price(item.id);
    // Preserve the recovered pricing rule: secondary uncommon is special;
    // otherwise the primary rarity controls this part of the mixed price.
    if(item.mixed_with) price+=rarity(item.mixed_with)==1?4:rarity(item.id)==2?6:rarity(item.id)==3?9:3;
    return price*(1<<std::min(20u,unsigned(item.level?item.level-1:0)));
}
bool Game::can_mix(unsigned first,unsigned second) const {
    return phase==Phase::Shop && first<inventory.size() && second<inventory.size() && first!=second &&
        cocktail_tickets && inventory[first].id && inventory[second].id &&
        !inventory[first].mixed_with && !inventory[second].mixed_with && inventory[first].id!=inventory[second].id;
}
bool Game::mix_owned(unsigned first,unsigned second) {
    if(!can_mix(first,second)) { notice("Mix needs a ticket and two different unmixed balls"); return false; }
    sound("mix_balls");
    OwnedBall result=inventory[first]; result.mixed_with=inventory[second].id;
    result.level=static_cast<uint8_t>((unsigned(result.level)+inventory[second].level+1)/2);
    result.score+=inventory[second].score; normalize_numbers(result);
    if(!seeded_run && !trial)profile.mix(result.id,result.mixed_with);
    inventory[first]=result; inventory[second]=OwnedBall(); --inventory_count; --cocktail_tickets;
    react_npc(12);sound("mix_balls");save_pending=true; notice("Cocktail made: both abilities share level and value"); return true;
}
OwnedBall default_item(unsigned id,unsigned level) {
    OwnedBall item;item.id=static_cast<uint16_t>(id);const auto& info=ball_info(id);
    item.level=static_cast<uint8_t>(level?level:std::max(1,info.start_level));
    item.score=info.base_score*(1<<std::min(20u,unsigned(item.level-1)));return item;
}
int stock_price(unsigned id) { return item_price(default_item(id)); }
void Game::gain_shots(unsigned amount) {
    for(unsigned i=0;i<amount && shot_capacity<20;++i) { ++shot_capacity; shots=std::min(20,shots+1); }
}
const char* difficulty_name(unsigned difficulty) {
    const char* names[]={"CHILL NIGHT","WINE MIXER","PINEAPPLE MALIBU","BLOODY MARY","DARK & STORMY","ALL NIGHTER"}; return names[std::min(difficulty,5u)];
}
int required_score(unsigned round,unsigned difficulty) {
    unsigned extra=round>21?round-21:0;
    unsigned adjusted=round;
    if(extra) adjusted+=extra<27?(1u<<(extra-1)):100000000u;
    const double multipliers[]={1.6,1.7,1.8,2.0,1.5,2.2};
    double score=double(adjusted)*adjusted*multipliers[std::min(difficulty,5u)];
    return static_cast<int>(std::min(score,double(std::numeric_limits<int>::max()/10)));
}
unsigned rounds_for_difficulty(unsigned difficulty) { return difficulty==5?20:10; }
unsigned curse_levels_for_round(unsigned round,unsigned difficulty) {
    return difficulty==3?round/4:difficulty>=4?round/3:0;
}
int shots_for_round(unsigned round,unsigned difficulty) {
    const int allotment[]={5,5,6,6,7,7,8,8,9,9};
    return allotment[std::min(round?round-1:0,9u)]-(difficulty>=2?1:0);
}
}
