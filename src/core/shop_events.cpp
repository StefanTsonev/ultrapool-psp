#include "ultrapool/core.hpp"
#include "ultrapool/shop_stock.hpp"
#include "ultrapool/daily_catalog.hpp"
#include <algorithm>
namespace up {
namespace {
void shop_transform_reaction(Game& game,unsigned token){
    const OwnedBall item=game.shop_participant(token);
    for(unsigned side=0;side<(item.mixed_with?2u:1u);++side)
        if(builtin(side?item.mixed_with:item.id).ability==Incubator)game.shop_buff(token,3*item.level);
}
}
unsigned Game::seeded_content_roll(unsigned& counter,unsigned exclude,unsigned other,uint32_t required_tags,unsigned level,bool planets,bool twice){
 ShopStockRequest request;request.level=level;
 if(!daily_date && (deck==7 || trial)){for(auto set:daily_set_order)if(trial || shop_set_unlocked(profile,set))request.sets[request.set_count++]=set;}
 else for(auto set:shop_sets)if(set!=ItemSet::NONE)request.sets[request.set_count++]=set;
 Pcg32 random(uint64_t(daily_date?daily_seed(daily_date):run_seed)+counter++);
 unsigned excluded[]={exclude,other};unsigned result=roll_seeded_content_data(random,request,excluded,2,required_tags,planets);
 if(twice)result=roll_seeded_content_data(random,request,excluded,2,required_tags,planets);
 if(daily_date)daily_random=random;
 return result;
}
OwnedBall Game::offer_item(unsigned index) const {
    if(index>=offers.size() || !offers[index])return OwnedBall();
    return offer_items[index].id==offers[index]?offer_items[index]:default_item(offers[index]);
}
OwnedBall& Game::shop_participant(unsigned token) {
    if(token<inventory.size())return inventory[token];
    unsigned index=token-static_cast<unsigned>(inventory.size());
    if(offer_items[index].id!=offers[index])offer_items[index]=offer_item(index);
    return offer_items[index];
}
unsigned Game::roll_content(unsigned exclude,unsigned other,uint32_t required_tags,unsigned rarity_level,bool any_set,unsigned exclude_extra,int rarity_filter) {
    if(daily_date && !any_set){
        ShopStockRequest request;request.level=rarity_level;
        for(auto set:shop_sets)if(set!=ItemSet::NONE)request.sets[request.set_count++]=set;
        const unsigned excluded[]={exclude,other,exclude_extra};
        return rarity_filter==-1?roll_seeded_content_data(daily_random,request,excluded,3,required_tags,false):roll_filtered_content_data(daily_random,request,excluded,3,required_tags,rarity_filter);
    }
    const float progress=std::max(0.0f,std::min(1.0f,(float(rarity_level)-1)/10));
    const float weights[]={12-4*progress,3-progress,std::max(0.0f,-.3f+1.3f*progress),std::max(0.0f,-.5f+.75f*progress)};
    float roll=(rarity_filter==-1?random()/4294967296.0f:0)*(weights[0]+weights[1]+weights[2]+weights[3]);unsigned tier=0;
    while(tier<3 && roll>=weights[tier]){roll-=weights[tier];++tier;}
    unsigned options[content_count],count=0;
    for(unsigned id=1;id<content_count;++id){
        const auto& info=ball_info(id);
        if(id==exclude || id==other || id==exclude_extra || !ball_supported(id) || info.set==ItemSet::NEGATIVE || (!any_set && !info.can_drop))continue;
        if(required_tags && !(info.tags&required_tags))continue;
        if(rarity_filter>=0 && info.rarity!=static_cast<unsigned>(rarity_filter))continue;
        if(rarity_filter==-1 && !any_set && info.rarity!=tier)continue;
        bool allowed=any_set || deck==7 || trial;
        for(auto set:shop_sets)allowed|=set!=ItemSet::NONE && set==info.set;
        if(allowed)options[count++]=id;
    }
    if(count)return options[random()%count];
    return 0;
}
void Game::shop_buff(unsigned slot,int amount,bool chain,bool is_moss) {
    if(slot>=18 || !shop_participant(slot).id)return;
    int gaia=0;for(const auto& owned:inventory)if(owned.id && (builtin(owned.id).ability==Gaia || (owned.mixed_with && builtin(owned.mixed_with).ability==Gaia)))gaia+=owned.level;
    amount+=gaia;
    OwnedBall& item=shop_participant(slot);item.score=std::max(1,item.score+amount);normalize_numbers(item);if(slot>=15)offers[slot-15]=item.id;
    if(!chain || amount<=0)return;
    const OwnedBall source=item;
    for(unsigned side=0;side<(source.mixed_with?2u:1u);++side){
        const unsigned ability=builtin(side?source.mixed_with:source.id).ability;
        if(ability==Bush){sound("bush_ball");shop_buff(slot,source.level,false);}
        if(ability==Antimatter)shop_buff(slot,-2*amount,false);
        if(ability==Caterpillar && (daily_date?daily_random.bounded(100):random()%100)<source.level){
            const OwnedBall previous=shop_participant(slot);unsigned incubator=0;
            for(unsigned component=0;component<(previous.mixed_with?2u:1u);++component)if(builtin(component?previous.mixed_with:previous.id).ability==Incubator)incubator+=3*previous.level;
            OwnedBall& transformed=shop_participant(slot);
            if(side)transformed.mixed_with=static_cast<uint16_t>(id_of(BallKey::BUTTERFLY));else transformed.id=static_cast<uint16_t>(id_of(BallKey::BUTTERFLY));
            normalize_numbers(transformed);if(slot>=15)offers[slot-15]=transformed.id;
            if(!seeded_run && !trial)profile.unlock("transform_caterpillar");
            // The old Incubator's carrier arrives after synchronous identity assignment.
            if(incubator)shop_buff(slot,static_cast<int>(incubator));
            run_shop_event(Signal::TransformAnother,slot);
            // Butterfly's Spawn targets table bodies, not persistent shop items.
        }
        if(ability==Moss && !is_moss){
            unsigned eligible[15],count=0;for(unsigned i=0;i<inventory.size();++i)if(i!=slot && inventory[i].id)eligible[count++]=i;
            for(unsigned i=count;i>1;--i)std::swap(eligible[i-1],eligible[random()%i]);
            if(count)shop_buff(eligible[0],static_cast<int>(std::ceil(double(amount)*source.level*.2)),true,true);
        }
    }
}

void Game::run_shop_event(Signal signal,unsigned changed) {
    // Snapshot subscribed slots, then evaluate live item components in stable rack order.
    unsigned slots[18],count=0;
    for(unsigned i=0;i<18;++i){const auto& item=shop_participant(i);if(item.id && ((ball_info(item.id).events|(item.mixed_with?ball_info(item.mixed_with).events:0))&signal_bit(signal)))slots[count++]=i;}
    auto obtain_scrap=[&](){for(auto& item:inventory)if(!item.id){item=default_item(id_of(BallKey::SCRAP));++inventory_count;break;}};
    if(signal==Signal::EnterShop)for(unsigned passive:snacks)if(passive==id_of(SnackKey::TV_DINNER)){obtain_scrap();obtain_scrap();}
    for(unsigned entry=0;entry<count;++entry){
        const unsigned slot=slots[entry];if(!shop_participant(slot).id)continue;
        const OwnedBall snapshot=shop_participant(slot);
        for(unsigned side=0;side<(snapshot.mixed_with?2u:1u);++side){
            const unsigned id=side?snapshot.mixed_with:snapshot.id,ability=builtin(id).ability;
            if((signal==Signal::SellAnother || signal==Signal::Reroll) && ability==Cauldron && shop_participant(slot).score<snapshot.level*5){shop_buff(slot,snapshot.level);sound("cauldron_ball");}
            if(signal==Signal::EnterShop && ability==Battery)shop_buff(slot,-5);
            if(signal==Signal::EnterShop && ability==Amber)shop_buff(slot,snapshot.level);
            if(signal==Signal::EnterShop && ability==Television)for(unsigned i=0;i<snapshot.level;++i)obtain_scrap();
            if(signal==Signal::TransformAnother && ability==SharkPlush && changed<18 && shop_participant(changed).id)shop_buff(changed,3*snapshot.level);
            if(signal==Signal::Upgrade && ability==Battery)shop_buff(slot,snapshot.level);
            if(signal==Signal::Upgrade && ability==Scrap && changed==slot){
                ++scrap_transforms;progress_track("SCRAP_UPGRADED",scrap_transforms,true);
                shop_buff(slot,std::max(0,int(snapshot.level)-1));
                shop_transform_reaction(*this,slot);
                unsigned result=seeded_content_roll(scrap_rolls,snapshot.id,snapshot.mixed_with,static_cast<uint32_t>(Tag::TAG_TECH),static_cast<unsigned>(std::max(0,round-1+(last_round_won?1:0))),false,!side && snapshot.mixed_with);
                if(result){OwnedBall& transformed=shop_participant(slot);if(side)transformed.mixed_with=static_cast<uint16_t>(result);else transformed.id=static_cast<uint16_t>(result);if(transformed.mixed_with==transformed.id)transformed.mixed_with=0;normalize_numbers(transformed);if(slot>=15)offers[slot-15]=transformed.id;sound("scrap_ball");}
                run_shop_event(Signal::TransformAnother,slot);
            }
        }
    }
}
void Game::shop_bought(const OwnedBall& bought,unsigned destination) {
    if(destination>=inventory.size())return;
    const ItemSet primary=shop_sets[0]==ItemSet::NONE?ItemSet::CLASSIC:shop_sets[0];
    const bool same_set=ball_info(bought.id).set==primary;
    for(unsigned passive:snacks)
        if((passive==id_of(SnackKey::SHRIMP) && same_set) || (passive==id_of(SnackKey::FRIED_SHRIMP) && !same_set))shop_buff(destination,1);
    for(unsigned side=0;side<(bought.mixed_with?2u:1u);++side)if(builtin(side?bought.mixed_with:bought.id).ability==Gacha){
        OwnedBall& receiving=inventory[destination];unsigned target_side=receiving.id==id_of(BallKey::GACHA)?0:1;
        shop_buff(destination,bought.level);
        unsigned result=seeded_content_roll(gacha_rolls,id_of(BallKey::GACHA),0,0,8);
        if(result){if(target_side)receiving.mixed_with=static_cast<uint16_t>(result);else receiving.id=static_cast<uint16_t>(result);if(receiving.mixed_with==receiving.id)receiving.mixed_with=0;normalize_numbers(receiving);
            shop_transform_reaction(*this,destination);
            run_shop_event(Signal::TransformAnother,destination);sound("gacha_ball");}
    }
}
void Game::shop_sold(const OwnedBall& sold) {
    for(unsigned side=0;side<(sold.mixed_with?2u:1u);++side){
        unsigned ability=builtin(side?sold.mixed_with:sold.id).ability;
        if(ability==PetFood && inventory[0].id) {
            const auto& front=inventory[0];uint32_t tags=ball_info(front.id).tags|(front.mixed_with?ball_info(front.mixed_with).tags:0);
            shop_buff(0,sold.score+((tags&static_cast<uint32_t>(Tag::TAG_FRIEND))?sold.level:0));
        }
        if(ability==Potion && inventory[0].id) {
            const OwnedBall before=inventory[0];
            shop_transform_reaction(*this,0);
            unsigned first=roll_content(before.id,before.mixed_with,0,static_cast<unsigned>(std::max(0,round-1+(last_round_won?1:0))),false,id_of(BallKey::POTION));
            if(before.mixed_with)first=roll_content(before.id,before.mixed_with,0,static_cast<unsigned>(std::max(0,round-1+(last_round_won?1:0))),false,id_of(BallKey::POTION));
            if(first){inventory[0].id=static_cast<uint16_t>(first);normalize_numbers(inventory[0]);}
            if(inventory[0].mixed_with){unsigned second=roll_content(before.id,before.mixed_with,0,static_cast<unsigned>(std::max(0,round-1+(last_round_won?1:0))),false,id_of(BallKey::POTION));if(second){inventory[0].mixed_with=static_cast<uint16_t>(second);normalize_numbers(inventory[0]);}}
            run_shop_event(Signal::TransformAnother,0);
            shop_buff(0,sold.level);
        }
        if(ability==Piggybank)gain_money(sold.score*std::min(3u,unsigned(sold.level)));
        if(ability==Astrolabe)for(unsigned i=0;i<sold.level;++i){
            unsigned planet=seeded_content_roll(astrolabe_rolls,0,0,static_cast<uint32_t>(Tag::TAG_PLANET)|static_cast<uint32_t>(Tag::TAG_PLANET_),static_cast<unsigned>(std::max(0,round-1+(last_round_won?1:0))),true);
            if(!planet)continue;
            for(auto& slot:inventory)if(!slot.id){slot=default_item(planet,1);++inventory_count;break;}
        }
    }
}
}
