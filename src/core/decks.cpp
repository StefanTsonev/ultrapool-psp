#include "ultrapool/core.hpp"
#include "ultrapool/daily_catalog.hpp"
#include <algorithm>
namespace up {
const DeckDefinition& deck_info(unsigned deck) {
    static const DeckDefinition definitions[]={
        {"CLASSIC","1_CLASSIC",ItemSet::CLASSIC,{{id_of(BallKey::B_2_BALL),id_of(BallKey::B_1_BALL),id_of(BallKey::B_1_BALL)}}},
        {"NATURE","2_NATURE",ItemSet::NATURE,{{id_of(BallKey::LADYBUG),id_of(BallKey::APPLE),id_of(BallKey::FLOWER_BUSH)}}},
        {"TECH","3_TECH",ItemSet::TECH,{{id_of(BallKey::SCRAP),id_of(BallKey::SCRAP),id_of(BallKey::SCRAP)}}},
        {"SPOOKY","4_SPOOKY",ItemSet::SPOOKY,{{id_of(BallKey::GRAVESTONE),id_of(BallKey::GHOST),id_of(BallKey::CAULDRON)}}},
        {"FRIENDS","5_FRIENDS",ItemSet::FRIENDS,{{id_of(BallKey::FISHBOWL),id_of(BallKey::DOG),id_of(BallKey::CAT)}}},
        {"FOOD","6_FOOD",ItemSet::FOOD,{{id_of(BallKey::TOMATO),id_of(BallKey::BACON),id_of(BallKey::LETTUCE)}}},
        {"SPACE","7_SPACE",ItemSet::SPACE,{{id_of(BallKey::ASTROLABE),id_of(BallKey::STAR),id_of(BallKey::STAR)}}},
        {"GACHA","GACHA",ItemSet::NONE,{{id_of(BallKey::GACHA),id_of(BallKey::GACHA),id_of(BallKey::GACHA)}}}
    };
    return definitions[deck<deck_count?deck:0];
}
bool shop_set_unlocked(const Profile& profile,ItemSet set) {
    switch(set){
    case ItemSet::CLASSIC:case ItemSet::NATURE:case ItemSet::TECH:return true;
    case ItemSet::SPOOKY:return profile.has("tech_win");
    case ItemSet::FRIENDS:return profile.has("spooky_win");
    case ItemSet::FOOD:return profile.has("friends_win");
    case ItemSet::SPACE:return profile.has("food_win");
    default:return false;
    }
}
void Game::update_shop_sets() {
    if(daily_date)return;
    if(deck>=deck_count)deck=0;
    if(deck==7 || trial)return;
    if(shop_sets[0]==ItemSet::NONE)shop_sets[0]=deck_info(deck).set;
    if(!difficulty)return;
    Pcg32 set_random(run_seed);
    unsigned desired=static_cast<unsigned>(std::max(1,std::min(3,round-1+(last_round_won?1:0))));
    for(unsigned slot=1;slot<desired;++slot)if(shop_sets[slot]==ItemSet::NONE) {
        ItemSet choices[7];unsigned count=0;
        for(unsigned candidate=0;candidate<7;++candidate) {
            ItemSet set=daily_set_order[candidate];if(!shop_set_unlocked(profile,set))continue;bool used=false;
            for(auto existing:shop_sets)used|=existing==set;
            if(!used)choices[count++]=set;
        }
        if(count)shop_sets[slot]=choices[set_random.next()%count];
    }
}
void Game::reroll_shop_sets() {
    if(daily_date)return;
    if(!difficulty || deck==7 || trial)return;
    unsigned available=0;for(auto set:daily_set_order)available+=shop_set_unlocked(profile,set);
    if(available<=4)return;
    const auto previous=shop_sets;shop_sets={};shop_sets[0]=deck_info(deck).set;
    const unsigned desired=static_cast<unsigned>(std::max(1,std::min(3,round-1+(last_round_won?1:0))));
    for(unsigned slot=1;slot<desired;++slot){
        ItemSet options[7];unsigned count=0;
        for(unsigned i=0;i<7;++i){ItemSet candidate=daily_set_order[i];if(!shop_set_unlocked(profile,candidate))continue;bool used=false;for(auto old:previous)used|=old==candidate;for(auto selected:shop_sets)used|=selected==candidate;if(!used)options[count++]=candidate;}
        if(count)shop_sets[slot]=options[random()%count];
    }
}
ItemSet Game::choose_shop_set() {
    if(!daily_date && (deck==7 || trial)){ItemSet choices[7];unsigned count=0;for(auto set:daily_set_order)if(trial || shop_set_unlocked(profile,set))choices[count++]=set;return count?choices[random()%count]:ItemSet::CLASSIC;}
    unsigned count=0;for(auto set:shop_sets)count+=set!=ItemSet::NONE;
    if(!count)return deck_info(deck).set;
    unsigned roll=random()%(count+1);return shop_sets[roll<2?0:roll-1];
}
}
