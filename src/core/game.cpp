#include "ultrapool/core.hpp"
#include "ultrapool/presentation.hpp"
#include "ultrapool/shop_stock.hpp"
#include "ultrapool/daily_catalog.hpp"
#include <algorithm>
#include <new>
#include <cstdio>
#include <cstring>
#include <ctime>
namespace up {
Vec build_slot_position(unsigned slot) {
    static const Vec positions[]={Vec(0,-120),Vec(-25,-155),Vec(25,-155),Vec(-50,-190),
        Vec(0,-190),Vec(50,-190),Vec(-75,-225),Vec(-25,-225),Vec(25,-225),Vec(75,-225),
        Vec(-100,-260),Vec(-50,-260),Vec(0,-260),Vec(50,-260),Vec(100,-260)};
    return positions[slot<15?slot:0];
}

void Game::set_run_seed(uint32_t value,bool custom){run_seed=value;rng=value?value:0x554c5452;seeded_run=custom;}
uint32_t Game::random() { if(daily_date)return daily_random.next();rng^=rng<<13; rng^=rng>>17; rng^=rng<<5; return rng; }
bool Game::load_content(const char* path) { return load_definitions(path,definitions,definition_count); }
void Game::notice(const char* text) { std::snprintf(message,sizeof(message),"%s",text); message_time=3; }
void Game::effect(Vec p,uint32_t color,float radius,bool passive) {
    for (auto& e:effects) if (e.life<=0) { e.p=p; e.line=false;e.passive=passive;e.color=color; e.radius=radius; e.life=0.5f; break; }
}
void Game::effect_line(Vec from,Vec to) {
    for(auto& e:effects)if(e.life<=0){e.p=from;e.end=to;e.line=true;e.passive=false;e.color=0xffefffff;e.life=.3f;break;}
}
void Game::reset(bool ability_trial) {
    daily_date=0;daily_open=daily_replay=daily_save_pending=false;
    // Match the source's datetime-string hash seed; no proprietary code is used.
    if(!seeded_run){
    std::time_t now=std::time(nullptr);std::tm* local=std::localtime(&now);char date[32]{};
    if(local)std::strftime(date,sizeof(date),"%Y-%m-%dT%H:%M:%S",local);
    run_seed=5381;for(const unsigned char* c=reinterpret_cast<const unsigned char*>(date);*c;++c)run_seed=run_seed*33+*c;
    }
    earned_money_total=0;win_streak=planted_flowers=scrap_transforms=0;seed_open=gallery_details=false;
    rounds_played=total_rerolls=snack_tickets_used=gacha_rolls=scrap_rolls=astrolabe_rolls=0;
    endless=false;profile_open=false;
    cubes={}; offer_items={};shop_sets={};if(deck>=deck_count)deck=0; trial=ability_trial; money=0; hp=3; round=1; inventory_count=0;
    last_round_won=false; cocktail_tickets=0; cocktail_bar=mixing_pending=false; inventory={}; snacks={}; snack_offers={}; snack_tickets=0; cap_rewarded=false; save_pending=false;
    slow_frames=0; set_run_seed(run_seed,seeded_run); message[0]=0; message_time=0;
    for (auto& e:effects) e.life=0;
    const unsigned trial_ids[]={13,9,10,11,12,14};
    for(unsigned i=0;i<(trial?6u:3u);++i) {
        unsigned id=trial?trial_ids[i]:deck_info(deck).balls[i];
        inventory[inventory_count++]=default_item(id);
    }
    update_shop_sets();
    begin_round();
}
void Game::begin_round() {
    profile.round_stats={};round_pocketed=sevens_in_a_row=0;
    refresh_build_ids();
    round_summary=false;payout_time=0;score_popups={};score_display=score_flash=score_sound_time=0;score_sound_pitch=1;
    offer_details=roster_details=passive_details=false;
    world=World();haunts.clear();passive_balls={};passive_count=0;candles_pocketed=ritual_strength=completed_ritual_strength=0;ritual_progress=ritual_flash=0;ritual_done=false;reset_cube_counters();pickups={};pickup_body_generations={};pickup_overflows=pickup_contacts=0; semantic_busy=false;semantics.clear();semantics.overflows=semantic_yields=0; listeners.clear(); bound_balls={};bound_snacks={};snack_generations={}; actions={}; action_serial=0;action_high_water=0;action_overflows=spawn_rejections=0; accumulator=0; was_moving=false; paused=false; aim_preparing=false;
    end_delay=0; selection=0; inspected=0; nav_delay=0; phase=Phase::Placement;
    shots=shots_for_round(round,difficulty); shot_capacity=shots; score=0; target=required_score(round,difficulty);
    power=0.6f; angle=-1.570796327f; cap_rewarded=false; cereal_used=chocolate_used=end_effects_done=false;snack_counts={};
    Ball cue; cue.cue=true; cue.def=builtin(0); cue.def.radius=17.5f; cue.p=Vec(0,210); world.add(cue);
    for (std::size_t i=0;i<inventory.size();++i) {
        if(!deploys_slot(static_cast<unsigned>(i)) || !inventory[i].id) continue;
        Ball b; b.owner=static_cast<int8_t>(i); b.def=builtin(inventory[i].id);
        for (std::size_t d=0;d<definition_count;++d) if (definitions[d].id==b.def.id) b.def=definitions[d];
        b.def.base_score=inventory[i].score; b.mixed_with=inventory[i].mixed_with; b.level=inventory[i].level; b.p=build_slot_position(static_cast<unsigned>(i)); refresh_initial_weight(b);
        world.add(b);
    }
    cue_start=Vec(0,210); respawn_cue(); cue_start=world.balls[0].p;
    losing_before_end=false;round_start_money=money;setup_curses();sync_listeners(); start_round_effects();progress_snapshot();
    notice(trial ? "ABILITY TRIAL: hit Bomb/Pinball. {square} inspect" : "Score target, then {triangle} end");
}
int Game::reroll_price() const { if(daily_date)return 2; unsigned n=std::min(rerolls+1,20u); return static_cast<int>(std::floor(n+std::pow(1.5f,float(n))/32+0.5f)); }
void Game::stock_shop(bool exclude_previous) {
    (void)exclude_previous; // Source intentionally ignores previous offers during rerolls.
    update_shop_sets();
    ShopStockRequest request;
    request.level=static_cast<unsigned>(round-1+(last_round_won?1:0));
    request.cocktails=difficulty>=2;
    if(!daily_date && (deck==7 || trial)) {
        for(auto set:daily_set_order)if(trial || shop_set_unlocked(profile,set))request.sets[request.set_count++]=set;
    } else for(auto set:shop_sets)if(set!=ItemSet::NONE)request.sets[request.set_count++]=set;
    Pcg32 normal_random(uint64_t(run_seed)+rounds_played+total_rerolls);
    if(daily_date)daily_random.reset(uint64_t(daily_seed(daily_date))+daily_rounds_played+daily_rerolls);
    auto next=offer_items;
    if(!generate_shop_stock(daily_date?daily_random:normal_random,request,next)) {
        notice("Shop pool unavailable");return;
    }
    snack_random=daily_date?daily_random:normal_random;
    offer_items=next;
    for(unsigned i=0;i<offers.size();++i)offers[i]=offer_items[i].id;
}
bool Game::reroll() {
    if (phase!=Phase::Shop || money<reroll_price()) { notice("Not enough money to reroll"); return false; }
    sound("slide");money-=reroll_price(); ++rerolls;if(daily_date)++daily_rerolls;else ++total_rerolls;
    run_shop_event(Signal::Reroll);stock_shop(true);react_npc(rerolls>=5?4:3);for(unsigned id:offers)if(id && rarity(id)>=2){react_npc(rarity(id)==3?9:8);break;}
    save_pending=true; notice("Shop rerolled"); return true;
}
int shop_price(unsigned id) { static const int prices[]={3,4,6,9}; return prices[rarity(id)]; }
bool Game::can_merge(unsigned offer,unsigned item) const {
    if(offer>=offers.size() || !offers[offer] || item>=inventory.size() || !inventory[item].id) return false;
    OwnedBall source=offer_item(offer);
    return can_combine(source,inventory[item]);
}
bool Game::buy(unsigned offer,bool merge) {
    if (phase!=Phase::Shop || offer>=offers.size() || !offers[offer]) return false;
    const OwnedBall bought=offer_item(offer); int cost=item_price(bought);
    if (money<cost) { notice("Not enough money"); return false; }
    unsigned destination=15;
    if (merge) {
        if (inventory_selection>=inventory.size() || !inventory[inventory_selection].id) return false;
        OwnedBall& dest=inventory[inventory_selection];
        if (!can_merge(offer,inventory_selection)) { notice("Merge needs matching ball and level"); return false; }
        OwnedBall source=bought; combine_values(source,dest);destination=inventory_selection;
        notice("Merged: permanent value / ability upgrade");
    } else {
        if (inventory_count==inventory.size()) { notice("Inventory full: sell or merge a ball"); return false; }
        OwnedBall b=bought;
        for(unsigned i=0;i<inventory.size();++i)if(!inventory[i].id){inventory[i]=b;destination=i;break;}
        ++inventory_count; notice("Ball added to your build");
    }
    react_npc(rarity(bought.id)==3?9:1);sound("coin");sound("drop_ball");money-=cost; offers[offer]=0;run_shop_event(Signal::BuyAnother,destination);shop_bought(bought,destination);if(merge)run_shop_event(Signal::Upgrade,destination);tutorial.trigger("shop-play",profile.tutorial_groups); save_pending=true; return true;
}
bool Game::sell(unsigned item) {
    if (phase!=Phase::Shop || item>=inventory.size() || !inventory[item].id) { notice("Select an owned ball"); return false; }
    const OwnedBall sold=inventory[item];gain_money(item_price(sold)/2);
    sound("coin_sell");inventory[item]=OwnedBall(); --inventory_count;run_shop_event(Signal::SellAnother,item);shop_sold(sold);
    save_pending=true; react_npc(2);notice("Ball sold"); return true;
}
bool Game::finish_round() {
    if(phase!=Phase::Aiming && phase!=Phase::Placement && !(cap_rewarded && phase==Phase::Simulating)) return false;
    if(!cap_rewarded && (world.moving() || pending_actions())) return false;
    if(cap_rewarded) end_effects_done=true;
    if(!end_effects_done) {
        losing_before_end=score<target;end_effects_done=true; end_round_effects(); dispatch();
        if(world.moving() || pending_actions()) { phase=Phase::Simulating; was_moving=true; return false; }
    }
    if(daily_date){
        last_round_won=false;round_earned=money-round_start_money;phase=Phase::DailyResult;daily_save_pending=true;save_pending=false;
        daily_history.record(daily_date,score,daily_replay);progress_track("MAX-SCORE",score,true);progress_track("MAX-MONEY-HELD",money,true);progress_track("MAX-ROUND",round+1,true);progress_track("TOTAL-MONEY-EARNED",round_earned);progress_snapshot();
        score_display=0;daily_medals_played=0;daily_result_counted=false;sound("win");return true;
    }
    payout_base=payout_bonus=payout_shots=payout_total=0;
    payout_base_shown=payout_bonus_shown=payout_shots_shown=0;payout_time=0;
    last_round_won=score>=target;win_streak=last_round_won?win_streak+1:0;
    if(last_round_won && losing_before_end && !seeded_run && !trial)profile.unlock("win_after_round_end");
    sound(last_round_won?"win":"lose");
    if (last_round_won) {
        int surplus=score-target, bonus=0;
        while (surplus>=bonus+1) { ++bonus; surplus-=bonus; }
        payout_base=4;payout_bonus=bonus;payout_shots=shots;payout_total=4+bonus+shots;gain_money(payout_total);
        notice("Target met: +4, surplus bonus, unspent shots");
    } else {
        --hp;sound("heartbreak");
        if (hp>0) { payout_base=10+round;payout_total=payout_base;gain_money(payout_total); }
        notice("Target missed: lost one HP");
    }
    round_earned=money-round_start_money;
    progress_track("MAX-SCORE",score,true);progress_track("MAX-MONEY-HELD",money,true);
    progress_track("MAX-ROUND",round+1,true);progress_track("TOTAL-MONEY-EARNED",round_earned);
    if(last_round_won)progress_track("MAX_ROUND_REACHED",round+1,true);
    else if(round==1 && !seeded_run && !trial)profile.unlock("lose_round_1");
    progress_snapshot();
    if (hp<=0) { phase=Phase::Lost; return true; }
    if (!endless && round==static_cast<int>(rounds_for_difficulty(difficulty)) && last_round_won) { if(!seeded_run && !trial)profile.record_win(deck,difficulty);phase=Phase::RunWon; return true; }
    enter_shop();return true;
}
void Game::enter_shop(){
    if(!daily_date)++rounds_played;
    phase=Phase::Shop; selection=0; inventory_selection=0; rerolls=0;
    shop_focus=ShopFocus::Market; shop_action=0; merge_pending=sell_pending=reorder_pending=owned_merge_pending=false;
    stock_shop();react_npc(round==1?14:!last_round_won?5:round>=int(rounds_for_difficulty(difficulty))?7:win_streak>=5?6:0); snack_bar=cocktail_bar=mixing_pending=false;
    if(last_round_won && (difficulty || trial) && round%2==1){++snack_tickets;tutorial.trigger("tickets-1",profile.tutorial_groups);}
    if(last_round_won && difficulty>=2 && (round%10==3 || round%10==6 || round%10==9)) ++cocktail_tickets;
    stock_snacks();run_shop_event(Signal::EnterShop);round_summary=true;save_pending=true;progress_snapshot();
}
void Game::reveal_payout(){payout_base_shown=payout_base;payout_bonus_shown=payout_bonus;payout_shots_shown=payout_shots;payout_time=10000;}
void Game::next_round() { if (phase==Phase::Shop) { if(last_round_won) ++round; begin_round(); } }
void Game::input(const Input& in,float seconds) {
    if(menu_overlay_input(in,seconds))return;
    if(tutorial.enabled && tutorial.active<tutorial_count){if(in.confirm){uint64_t before=profile.tutorial_groups;if(tutorial.dismiss(profile.tutorial_groups) && before!=profile.tutorial_groups)profile.dirty=true;}return;}
    if (in.pause) {
        paused=!paused; aim_preparing=false;
        if(phase==Phase::Aiming) phase=Phase::Placement;
    }
    if(settings_input(in,seconds))return;
    if(round_summary && phase==Phase::Shop){
        if(in.confirm || in.finish){if(!payout_done()){reveal_payout();sound("coin");}else{round_summary=false;sound("enter_shop");}}
        return;
    }
    if(in.confirm) sound("button_pos");
    if(in.cancel) sound("button_neg");
    float dt=std::max(0.0f,std::min(seconds,0.05f));
    nav_delay=std::max(0.0f,nav_delay-dt);
    if(phase==Phase::DailyResult){
        if(in.finish){continue_daily();return;}
        if(in.confirm || in.cancel){phase=Phase::Menu;selection=0;menu_preview_checked=false;}return;
    }
    if (phase==Phase::Menu) {
        if(daily_open){
            if(in.finish)daily_selection=0;
            if(in.cancel){daily_open=false;return;}
            if(nav_delay<=0 && (in.less||in.more)){shift_daily_month(in.more?1:-1);nav_delay=.18f;}
            if(nav_delay<=0 && (in.left||in.right||in.up||in.down)){int change=in.left?1:in.right?-1:in.up?7:-7;daily_selection=static_cast<unsigned>(std::max(0,std::min(3650,int(daily_selection)+change)));nav_delay=.15f;}
            uint32_t date=daily_date_from_serial(daily_serial(daily_today)-int(daily_selection));
            if(in.confirm || in.inspect){if(!choose_daily(date,in.inspect))notice("Daily play is available for the last eight days");}
            return;
        }
        if(in.more && nav_delay<=0 && !profile_open){if(!profile.daily_unlocked())notice("Daily challenges unlock after one win");else{daily_today=platform_date?platform_date:daily_today_utc();daily_selection=0;daily_open=valid_daily_date(daily_today);if(!daily_open)notice("Set the PSP date to use daily challenges");}nav_delay=.18f;return;}
        if(profile_open){
            if(in.confirm && profile_tab>=2){gallery_details=true;return;}
            if(in.confirm && profile_tab==0){tutorial_replay=true;replay_step=0;return;}
            if(in.cancel || in.auxiliary){profile_open=false;return;}
            if(nav_delay<=0 && (in.less || in.more)){profile_tab=(profile_tab+(in.more?1:4))%5;profile_selection=0;if(profile_tab==1)profile.mark_achievements_seen();nav_delay=.18f;}
            unsigned count=profile_tab==1?profile.visible_achievements():profile_tab>=2?profile.gallery_count(profile_tab-2):1;
            if(nav_delay<=0 && (in.left||in.right||in.up||in.down)){
                int stride=profile_tab>=2 && (in.up||in.down)?6:1;
                int next=int(profile_selection)+(in.left||in.up?-stride:stride);profile_selection=static_cast<unsigned>(std::max(0,std::min(int(std::max(1u,count))-1,next)));nav_delay=.14f;
            }return;
        }
        if(in.less){seed_open=true;seed_cursor=0;message_time=0;return;}
        if(in.auxiliary){profile_open=true;profile_tab=profile_selection=0;return;}
        if(!menu_preview_checked){
            menu_preview_checked=true;Game* preview=new(std::nothrow) Game();
            continue_available=preview&&preview->load_run("ultrapool.sav");
            if(continue_available){continue_round=preview->round;continue_money=preview->money;for(unsigned i=0;i<3;++i)continue_balls[i]=preview->inventory[i];}
            delete preview;
        }
        if(selection>6)selection=0;
        if(nav_delay<=0 && (in.left||in.right)){
            if(selection>=2)selection=2+(selection-2+(in.right?1:4))%5;
            else deck=(deck+(in.right?1:deck_count-1))%deck_count;
            nav_delay=.18f;
        }
        if(in.inspect) { difficulty=(difficulty+1)%6; if(difficulty==4)++difficulty; notice(difficulty_name(difficulty)); }
        if(in.finish) { if(!load_run("ultrapool.sav")) notice("No valid saved shop checkpoint"); return; }
        if(nav_delay<=0 && (in.up||in.down)){
            if(selection>=2)selection=in.up&&continue_available?1:0;
            else if(selection==0)selection=in.down?(continue_available?1:2):2;
            else selection=in.down?2:0;
            nav_delay=.18f;
        }
        if(in.cancel && selection>=2){selection=0;return;}
        if (in.confirm) {
            if(selection>=2){
                if(selection==2){paused=true;settings_selection=0;return;}
                if(selection==6){
                    if(!profile.daily_unlocked())notice("Daily challenges unlock after one win");
                    else{daily_today=platform_date?platform_date:daily_today_utc();daily_selection=0;daily_open=valid_daily_date(daily_today);if(!daily_open)notice("Set the PSP date to use daily challenges");}
                    return;
                }
                profile_open=true;profile_selection=0;profile_tab=selection==3?1:selection==4?2:0;
                if(profile_tab==1)profile.mark_achievements_seen();
                return;
            }
            if(selection==1) { if(!load_run("ultrapool.sav")) notice("No valid saved shop checkpoint"); }
            else if(!profile.has(deck_requirement(deck)))notice("Deck locked: win with the preceding deck");
            else if(!profile.has(difficulty_requirement(difficulty)))notice("Difficulty locked: win the preceding difficulty");
            else {seeded_run=false;seed_text[0]=0;reset(false);}
        }
        return;
    }
    if(phase==Phase::RunWon && in.finish){continue_endless();return;}
    if (phase==Phase::Lost || phase==Phase::RunWon || phase==Phase::Won) {
        if (in.confirm || in.cancel) { phase=Phase::Menu; selection=0;menu_preview_checked=false; } return;
    }
    if (phase==Phase::Shop) {
        if(offer_details){
            if(in.confirm||in.cancel||in.auxiliary)offer_details=false;
            else if(in.inspect){offer_details=false;merge_pending=true;shop_focus=ShopFocus::Inventory;}
            return;
        }
        if(in.auxiliary && !roster_details && !passive_details && !snack_bar && !cocktail_bar && !merge_pending && !owned_merge_pending && !reorder_pending && !sell_pending){
            if(shop_focus==ShopFocus::Market && offers[selection])offer_details=true;
            else if(shop_focus==ShopFocus::Inventory && inventory[inventory_selection].id){reorder_source=inventory_selection;reorder_pending=true;notice("Choose a slot; {cross} moves / swaps, {circle} cancels.");}
            else if(shop_focus==ShopFocus::Passive)passive_details=true;
            return;
        }
        if(roster_details){
            if(in.cancel||in.confirm){roster_details=false;return;}
            if(in.finish){roster_details=false;sell_pending=true;return;}
            if(in.inspect||in.auxiliary){roster_details=false;reorder_source=inventory_selection;reorder_pending=true;notice("Choose a slot; {cross} swaps, {circle} cancels.");return;}
            if(nav_delay<=0&&(in.left||in.right||in.up||in.down)){
                unsigned node=3+inventory_selection;
                for(unsigned step=0;step<15;++step){unsigned next=shop_neighbor(node,in.right?1:in.left?-1:0,in.down?1:in.up?-1:0,true);if(next==node)break;node=next;if(inventory[node-3].id){inventory_selection=node-3;break;}}
                nav_delay=.18f;
            }
            return;
        }
        if(passive_details){if(in.cancel||in.confirm||in.inspect)passive_details=false;return;}

        if((in.less || in.more) && nav_delay<=0) {
            nav_delay=0.25f;
            if(!difficulty && !trial) { notice("Bars open on higher difficulties"); return; }
            unsigned bars=difficulty>=2?3:2;
            unsigned bar=cocktail_bar?2:snack_bar?1:0;
            bar=(bar+(in.more?1:bars-1))%bars;
            snack_bar=bar==1; cocktail_bar=bar==2; mixing_pending=false;if(cocktail_bar)react_npc(cocktail_tickets?11:15);else npc_time=0;
            selection=inventory_selection=0;
            merge_pending=sell_pending=reorder_pending=owned_merge_pending=false; shop_focus=ShopFocus::Market; return;
        }
        if(cocktail_bar) {
            if(in.cancel) {
                if(mixing_pending) mixing_pending=false;
                else { cocktail_bar=false; shop_focus=ShopFocus::Market; }
                return;
            }
            if(nav_delay<=0 && (in.left || in.right || in.up || in.down)) {
                int column=int(inventory_selection%5),row=int(inventory_selection/5);
                column=std::max(0,std::min(4,column+(in.right?1:in.left?-1:0)));row=std::max(0,std::min(2,row+(in.down?1:in.up?-1:0)));inventory_selection=unsigned(row*5+column);nav_delay=.18f;
            }
            if(in.confirm) {
                if(mixing_pending) { if(mix_owned(reorder_source,inventory_selection)) mixing_pending=false; }
                else if(inventory[inventory_selection].id && !inventory[inventory_selection].mixed_with) { reorder_source=inventory_selection; mixing_pending=true; }
                else notice("Choose an unmixed owned ball");
            }
            return;
        }
        if(snack_bar) {
            if(in.cancel) { snack_bar=false; sell_pending=merge_pending=reorder_pending=owned_merge_pending=false; shop_focus=ShopFocus::Market; selection=inventory_selection=0; return; }
            if(nav_delay<=0&&(in.left||in.right||in.up||in.down)){
                unsigned node=shop_focus==ShopFocus::Market?selection:3+inventory_selection;
                unsigned next=snack_neighbor(node,in.right?1:in.left?-1:0,in.down?1:in.up?-1:0);
                if(next<3){shop_focus=ShopFocus::Market;selection=next;}else{shop_focus=ShopFocus::Inventory;inventory_selection=next-3;}
                sell_pending=false;nav_delay=.18f;
            }
            if(in.finish && shop_focus==ShopFocus::Inventory && snacks[inventory_selection]) sell_pending=true;
            if(in.confirm) {
                if(sell_pending) { sell_snack(inventory_selection); sell_pending=false; }
                else if(shop_focus==ShopFocus::Market) buy_snack(selection);
                else if(shop_focus==ShopFocus::Inventory && snacks[inventory_selection]){passive_selection=inventory_selection;passive_details=true;}
            }
            return;
        }
        if (in.cancel) {
            merge_pending=sell_pending=reorder_pending=owned_merge_pending=false; shop_focus=ShopFocus::Market;
            notice("{dpad} moves between offers, build and buttons."); return;
        }
        if(nav_delay<=0&&(in.left||in.right||in.up||in.down)) {
            bool choosing_target=merge_pending||reorder_pending||owned_merge_pending;
            unsigned node=shop_focus==ShopFocus::Market?selection:shop_focus==ShopFocus::Inventory?3+inventory_selection:shop_focus==ShopFocus::Passive?21+passive_selection:shop_action==0?18:shop_action==1?19:20;
            unsigned next=shop_neighbor(node,in.right?1:in.left?-1:0,in.down?1:in.up?-1:0,choosing_target);
            if(next<3){shop_focus=ShopFocus::Market;selection=next;}
            else if(next<18){shop_focus=ShopFocus::Inventory;inventory_selection=next-3;}
            else if(next<21){shop_focus=ShopFocus::Actions;shop_action=next==18?0:next==19?1:2;}else {shop_focus=ShopFocus::Passive;passive_selection=next-21;}
            sell_pending=false;nav_delay=.18f;
        }
        if(shop_focus==ShopFocus::Passive){
            if(in.finish && snacks[passive_selection]){sell_pending=true;return;}
            if(in.confirm && sell_pending){sell_snack(passive_selection);sell_pending=false;return;}
            if(in.confirm||in.inspect){passive_details=true;return;}
        }
        if(in.inspect && shop_focus==ShopFocus::Inventory && inventory[inventory_selection].id) {
            reorder_source=inventory_selection; owned_merge_pending=true; reorder_pending=merge_pending=sell_pending=false;
            notice("Choose a matching owned ball; {cross} merges, {circle} cancels.");
        }
        if (in.inspect && shop_focus==ShopFocus::Market && offers[selection]) {
            merge_pending=true; sell_pending=false; shop_focus=ShopFocus::Inventory;
            notice("Choose the receiving ball, then X to merge.");
        }
        if (in.finish && shop_focus==ShopFocus::Inventory && inventory[inventory_selection].id && !reorder_pending && !owned_merge_pending) {
            sell_pending=true; merge_pending=false;
            notice("Sell highlighted ball? {cross} confirms; {circle} cancels.");
        }
        if (in.confirm) {
            if(owned_merge_pending) { if(combine_owned(reorder_source,inventory_selection)) owned_merge_pending=false; }
            else if(sell_pending) { if(sell(inventory_selection)) sell_pending=false; }
            else if(merge_pending) {
                if(buy(selection,true)) { merge_pending=false; shop_focus=ShopFocus::Market; }
            } else if(shop_focus==ShopFocus::Market) buy(selection);
            else if(shop_focus==ShopFocus::Actions) {
                if(shop_action==0)reroll();
                else if(shop_action==1)next_round();
                else if(inventory[inventory_selection].id){shop_focus=ShopFocus::Inventory;sell_pending=true;notice("Sell highlighted ball? {cross} confirms; {circle} cancels.");}
                else notice("Choose an owned ball to sell first.");
            }
            else if(shop_focus==ShopFocus::Inventory) {
                if(reorder_pending) { move_item(reorder_source,inventory_selection); reorder_pending=false; }
                else if(inventory[inventory_selection].id) { roster_details=true; }
            }
        }
        return;
    }
    if(in.inspect&&world.count){do{inspected=(inspected+1)%static_cast<unsigned>(world.count);}while(inspected && (!world.balls[inspected].active || world.balls[inspected].energy));return;}
    if(inspected){
        if(in.cancel||in.confirm){inspected=0;return;}
        return;
    }

    if (in.finish) { if (score>=target) finish_round(); else notice("Need target score before ending early"); return; }
    // Placement is retained as the internal idle/cancelled-shot state.
    // Original PlayerBall cancels shot preparation; it never grants ball-in-hand.
    if (phase==Phase::Placement || !aim_preparing) {
        if (in.cancel) return;
        bool aim_input=in.analog_x*in.analog_x+in.analog_y*in.analog_y>0.09f ||
            in.left || in.right || in.less || in.more;
        if (in.confirm) { aim_preparing=true; if(phase!=Phase::Simulating) phase=Phase::Aiming; return; }
        if (!aim_input) return;
        aim_preparing=true; if(phase!=Phase::Simulating) phase=Phase::Aiming;
    }
    if (in.cancel) { aim_preparing=false; if(phase!=Phase::Simulating) phase=Phase::Placement; return; }
    if (analog_enabled && in.analog_x*in.analog_x+in.analog_y*in.analog_y>float(analog_deadzone*analog_deadzone)/10000) angle=std::atan2(-in.analog_x,in.analog_y);
    angle+=((in.right?1:0)-(in.left?1:0))*0.24f*dt*float(aim_percent)/100;
    power=std::max(0.26f,std::min(1.0f,power+((in.more?1:0)-(in.less?1:0))*0.5f*dt));
    if (in.confirm) shoot();
}
bool Game::shoot() {
    if (paused || (phase!=Phase::Aiming && phase!=Phase::Simulating) || shots<=0 || !world.count || !world.balls[0].active || world.balls[0].falling || cap_rewarded) return false;
    Ball& cue=world.balls[0]; cue.v=Vec(std::cos(angle),std::sin(angle))*(2500*power/cue.def.mass);
    sound("player_ball_shoot");
    --shots; pockets_after_last_shot=0;aim_preparing=false; chocolate_used=false; end_delay=0; phase=Phase::Simulating; was_moving=true;
    shot_direction=Vec(std::cos(angle),std::sin(angle));
    world.events.push(EventKind::Shot,0,0,world.metrics.ticks);
    emit_semantic(Signal::Shot,0,0);
    return true;
}
void Game::add_score(int amount,unsigned source) {
    if (amount==0 || cap_rewarded) return;
    const int previous_score=score;
    int ceiling=daily_date?std::max(daily_thresholds(daily_date)[3]*10,100):std::max(target*10,100);
    long long total=static_cast<long long>(score)+amount;
    if(total>ceiling && !cap_rewarded) { gain_money(10); shots=0; cap_rewarded=true; actions={};semantics.clear(); notice("MAX SCORE: +10 money, round ending"); }
    score=static_cast<int>(std::max(-1000000000LL,std::min(total,static_cast<long long>(ceiling))));
    progress_track("ROUND_SCORE",score,true,true);
    if(cap_rewarded) return;
    score_sound_pitch=score_sound_time>0?std::min(4.0f,score_sound_pitch+.1f):1.0f;score_sound_time=2;score_flash=1;
    sound("score",1,score_sound_pitch); // Original score_base player has no stream.
    if(source<world.count){ScorePopup* popup=&score_popups[0];for(auto& candidate:score_popups)if(candidate.life<popup->life)popup=&candidate;popup->p=world.balls[source].p;popup->v=(Vec()-popup->p).unit()*400;popup->amount=amount;popup->life=1;popup->passive=passive_slot(source)<8;}
    if (source<world.count) effect(world.balls[source].p,0xff66ffff,30,passive_slot(source)<8);
    emit_semantic(Signal::Score,amount,source);
    if(score>=target && previous_score<target)emit_semantic(Signal::ReachScore,0,source);
}
void Game::contact_ability(unsigned index,unsigned other,bool wall) { run_ball_event(wall?Signal::Wall:Signal::Hit,index,0,other); }
void Game::contact_component(unsigned index,unsigned other,bool wall,unsigned a,unsigned side,unsigned level) {
    if (index>=world.count || cap_rewarded) return;
    Ball& b=world.balls[index]; if (b.cue || b.collision_disabled || !b.active || b.falling) return;
    if(!wall && other<world.count) {
        Ball& hit=world.balls[other];
        if((a==Werewolf || a==WerewolfHuman) && hit.cue) transform_ball(index,id_of(a==Werewolf?BallKey::WEREWOLF_HUMAN:BallKey::WEREWOLF),side);
        if(a==Asteroid && hit.energy)content_effect(index,Asteroid,level,b.p);
        bool different=b.def.id!=hit.def.id || b.mixed_with!=hit.mixed_with;
        if(a==Zombie && !hit.cue && different && hit.def.base_score<b.def.base_score) { buff_ball(index,level);Ball pattern=b;transform_like(other,pattern); }
        if(a==Blueprint && !hit.cue && hit.def.id!=id_of(BallKey::ENERGY_BALL) && different) { Ball pattern=hit;transform_like(index,pattern); }
    }
    if(a==Racoon && !wall && other<world.count && !world.balls[other].cue && world.balls[other].weight<b.weight){
        int gain=static_cast<int>(std::ceil(double(world.balls[other].def.base_score)*level*.5));
        progress_increment("RACOON_EATEN");
        world.retain(index);consume_ball(other);buff_ball(index,gain);world.release(index);b.flash=.5f;
    }
    if(a==Astronaut && !wall && other<world.count && !world.balls[other].cue) { buff_ball(other,level);if(is_star(world.balls[other])) add_score(world.balls[other].def.base_score,other); }
    if(a==Coots && !wall && other<world.count) set_ball_locked(world.balls[other],false);
    if(a==Cloud) { buff_ball(index,-1);buff_random_other(index,level,b.p); }
    if(a==Fishbowl) { buff_ball(index,-1);spawn_data(id_of(BallKey::FISH),level,level,b.p,Fishbowl); }
    if(a==Goat && !wall && other<world.count && !world.balls[other].cue) buff_ball(index,level);
    if(a==Sheep && !wall && other<world.count && !world.balls[other].cue) buff_ball(other,level);
    if(a==Pumpkin) { buff_ball(index,level);if(b.def.base_score>=static_cast<int>(5*level)) transform_ball(index,id_of(BallKey::JACK_O_LANTERN),side); }
    if(a==Egg && wall) virtual_pocket(index,2);
    if (a==Pinball) { add_score(level,index); b.flash=0.25f; notice("PINBALL: contact scored"); }
    if (a==Marble && !wall && other<world.count && has_ability(world.balls[other],Marble)) { buff_ball(index,2*level); }
    if (a==Bricks && wall) { buff_ball(index,level); notice("BRICKS: wall buff increased value"); }
    if (a==Seal && !wall && other<world.count && !world.balls[other].cue && world.balls[other].active && !world.balls[other].falling) {
        sound("SealSFX");Ball& target_ball=world.balls[other]; add_score(level,index);
        queue_impulse(other,(target_ball.p-b.p).unit()*float(180+60*level));
        b.flash=0.25f; notice("SEAL: score and push");
    }
    if (a==Bomb) {
        buff_ball(index,level);
        if (b.def.base_score>=static_cast<int>(8*level)) {
            sound("BombSFX");Vec origin=b.p; int value=b.def.base_score; b.active=false; b.v=Vec(); retire_listener(index);
            for (std::size_t i=0;i<world.count;++i) {
                Ball& target_ball=world.balls[i]; if (!target_ball.active || target_ball.passive || target_ball.falling || target_ball.energy) continue;
                Vec delta=target_ball.p-origin; float length=delta.length();
                if (length<=200) queue_impulse(static_cast<unsigned>(i),delta.unit()*(300*level*(1-length/200)));
            }
            (void)value; world.events.push(EventKind::Pocketed,index,6,world.metrics.ticks,b.generation); effect(origin,0xff6688ff,200); notice("BOMB exploded: nearby balls launched");
        }
    }
}
void Game::dispatch() {
    sync_listeners();
    Event e; unsigned processed=0;
    while (processed++<4096 && world.events.pop(e)) {
        if(e.generation_a && (e.a>=world.count || world.balls[e.a].generation!=e.generation_a)) continue;
        if(e.generation_b && (e.b>=world.count || world.balls[e.b].generation!=e.generation_b)) continue;
        if (e.kind==EventKind::Collision && e.a<world.count && e.b<world.count) {
            if(!world.balls[e.b].cue){world.balls[e.a].last_contact=e.b;world.balls[e.a].last_contact_generation=world.balls[e.b].generation;}
            if(!world.balls[e.a].cue){world.balls[e.b].last_contact=e.a;world.balls[e.b].last_contact_generation=world.balls[e.a].generation;}
            float speed=std::max(world.balls[e.a].v.length(),world.balls[e.b].v.length());
            sound("BallOnBall",std::min(1.0f,speed/1000),1+std::min(.1f,speed/10000));
            if(has_snack(Chocolate) && !chocolate_used) {
                unsigned hit_index=e.a==0?e.b:e.b==0?e.a:0;
                if(hit_index && hit_index<world.count && world.balls[hit_index].active && !world.balls[hit_index].falling) {
                    buff_ball(hit_index,1); chocolate_used=true;
                }
            }
            contact_ability(e.a,e.b,false); contact_ability(e.b,e.a,false); }
        if(e.kind==EventKind::Captured)sound("ball_pocket_gravity");
        if (e.kind==EventKind::Wall) { if(e.a<world.count) sound("BallOnWall",std::min(1.0f,world.balls[e.a].v.length()/1500));contact_ability(e.a,0,true); }
        if(e.kind==EventKind::Pocketed)process_pocket(e);
    }
}
bool Game::has_snack(unsigned id) const {
    for(auto snack:snacks) if(snack==id) return true;
    return false;
}
void Game::stock_snacks(bool reseed) {
    snack_offers={};if(!snack_tickets)return;
    ShopStockRequest request;request.level=static_cast<unsigned>(round-1+(last_round_won?1:0));
    if(!daily_date && (deck==7 || trial)){for(auto set:daily_set_order)if(trial || shop_set_unlocked(profile,set))request.sets[request.set_count++]=set;}
    else for(auto set:shop_sets)if(set!=ItemSet::NONE)request.sets[request.set_count++]=set;
    request.extra_sets=!daily_date && deck!=7 && difficulty && request.set_count>=3;
    if(reseed){snack_random.reset(uint64_t(daily_date?daily_seed(daily_date):run_seed)+request.level+snack_tickets_used);if(daily_date)daily_random=snack_random;}
    if(!generate_snack_stock(daily_date?daily_random:snack_random,request,snacks,snack_offers))notice("No eligible snack trio");
}

bool Game::buy_snack(unsigned offer) {
    if(phase!=Phase::Shop || offer>=3 || !snack_offers[offer] || !snack_tickets) { notice("Need a snack ticket; earn tickets as rounds advance"); return false; }
    for(auto& slot:snacks) if(!slot) {
        slot=snack_offers[offer]; --snack_tickets;++snack_tickets_used; stock_snacks(true); save_pending=true;
        notice("Snack chosen: active for the rest of this run"); return true;
    }
    notice("Four snack slots full: sell one first"); return false;
}
bool Game::sell_snack(unsigned slot) {
    if(phase!=Phase::Shop || slot>=4 || !snacks[slot]) return false;
    unsigned sold=snacks[slot];
    gain_money(snack_sell_price(sold));snacks[slot]=0;run_shop_event(Signal::SellAnother);
    if(sold==id_of(SnackKey::HALO_HALO)){reroll_shop_sets();react_npc(13);}
    if(sold==id_of(SnackKey::LOVE_CAKE))hp=std::min(3,hp+1);
    bool obtained=false;
    if(sold==id_of(SnackKey::VITAMINS))for(auto& item:inventory)if(!item.id){item=default_item(id_of(BallKey::UPGRADE_BALL),1);++inventory_count;obtained=true;break;}
    sound("coin_sell");save_pending=true;
    notice(sold==id_of(SnackKey::LOVE_CAKE)?"Love Cake sold: healed one heart":sold==id_of(SnackKey::VITAMINS)?(obtained?"Vitamins sold: Upgrade Ball added":"Vitamins sold: no free ball slot"):"Snack sold");return true;
}
void Game::start_round_effects() {
    sync_listeners();const auto snapshot=listeners.snapshot(Signal::RoundStart);
    for(unsigned listener=0;listener<snapshot.count;++listener) {
        EntityRef ref=snapshot.entities[listener];if(!live_listener(ref)) continue;
        if(ref.kind==EntityKind::Cube) { run_cube_event(Signal::RoundStart,ref.index);continue; }
        if(ref.kind==EntityKind::Ball) { run_ball_event(Signal::RoundStart,ref.index);continue; }
        if(ref.kind!=EntityKind::Snack) continue;
        run_snack_event(Signal::RoundStart,ref.index);

    }
    emit_semantic(Signal::Spawn);

}
void Game::end_round_effects() {
    sync_listeners();const auto snapshot=listeners.snapshot(Signal::RoundEnd);
    for(unsigned listener=0;listener<snapshot.count;++listener) {
        EntityRef ref=snapshot.entities[listener];if(!live_listener(ref))continue;
        if(ref.kind==EntityKind::Ball){run_ball_event(Signal::RoundEnd,ref.index);continue;}
        if(ref.kind!=EntityKind::Snack)continue;
        unsigned snack=snacks[ref.index];if(snack==id_of(SnackKey::TIGER_ROLL)){queue_tiger(10,true);continue;}if(snack!=Muffin && snack!=MiniMuffin) continue;
        unsigned eligible[max_balls],n=0;int best=0;
        for(unsigned i=1;i<world.count;++i) {
            const Ball& b=world.balls[i];if(!b.active || b.passive || b.falling) continue;
            bool better=!n || (snack==Muffin?b.def.base_score>best:b.def.base_score<best);
            if(better) { n=0;best=b.def.base_score; }
            if(b.def.base_score==best) eligible[n++]=i;
        }
        if(n) queue_ball_pocket(eligible[random()%n],2,passive_origin(snack));
    }
}
bool Game::combine_owned(unsigned a,unsigned b) {
    if(phase!=Phase::Shop || a>=15 || b>=15 || a==b || !inventory[a].id || !inventory[b].id) return false;
    OwnedBall& source=inventory[a]; OwnedBall& target_ball=inventory[b];
    if(!can_combine(source,target_ball)) { notice("Needs compatible type and level"); return false; }
    combine_values(source,target_ball);
    source=OwnedBall(); --inventory_count;run_shop_event(Signal::Upgrade,b); save_pending=true; notice("Owned balls merged; permanent value and level kept"); return true;
}
bool Game::move_item(unsigned a,unsigned b) {
    if(phase!=Phase::Shop || a>=inventory.size() || b>=inventory.size() || !inventory[a].id) return false;
    std::swap(inventory[a],inventory[b]); save_pending=true; notice("Build rearranged: rack follows your Nacho passives"); return true;
}
void Game::pocket_effect(unsigned source,unsigned pocket) { run_ball_event(Signal::Pocket,source,0,max_balls,pocket); }
void Game::pocket_component(unsigned source,unsigned pocket,unsigned a,unsigned level,Vec origin) {
    if(cap_rewarded) return;
    Ball& b=world.balls[source];
    if(a==Reaper){if(Pocket* p=world.pocket(pocket))p->multiplier+=1;reaper_pull(source,level,origin);}
    if(a==Eyeball){bool other=false;for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]) && has_ability(world.balls[i],Eyeball)){other=true;break;}if(!other)for(unsigned i=0;i<2;++i)spawn_data(id_of(BallKey::EYEBALL),level,b.def.base_score+static_cast<int>(level),origin,Eyeball);}
    if(a==Jellyfish)queue_passive_copy(source,origin);
    if(a==Candle)pocket_candle(level);
    if(a==Trash || a==Lich)collection_effect(source,a,level,origin);
    if(a==AiBrain || a==Drill || a==Bear || a==Lightbulb || a==Keyring || a==RatKing || a==Supernova)summon_effect(source,a,level,origin);
    if(a==Meatball || a==Onigiri || a==Friendsphere || a==Rainbow || a==Log || a==Ant || a==Replicator || a==Alien || a==Antenna || a==Wheel || a==Conveyor || a==Comet || a==Asteroid || a==Tornado || a==BlackHole || a==Wormhole)content_effect(source,a,level,origin);
    if(a==Toolbox){
        unsigned locked[max_balls],count=0;for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]) && world.balls[i].locked)locked[count++]=i;
        for(unsigned i=count;i>1;--i)std::swap(locked[i-1],locked[random()%i]);
        for(unsigned i=0;i<level;++i){spawn_data(id_of(BallKey::SCREW),level,default_item(id_of(BallKey::SCREW),level).score,origin,Toolbox);if(i<count)queue_ball_modifier(locked[i],Modifier::Shoot,0,origin,float(level));}
    }
    if(a==Octopus)for(unsigned i=0;i<8;++i)spawn_data(id_of(BallKey::FISH),level,level,b.p,Octopus);
    if(a==Machine)for(unsigned i=0;i<level+1;++i)spawn_data(id_of(BallKey::SCRAP),1,default_item(id_of(BallKey::SCRAP)).score,origin,Machine);
    if(a==Skull){
        int chosen=-1;float distance=0;unsigned ties=0;
        for(unsigned i=1;i<world.count;++i){const Ball& candidate=world.balls[i];if(i==source || !targetable_ball(candidate) || candidate.shielded || candidate.shield_broken)continue;Vec delta=candidate.p-origin;float squared=delta.dot(delta);
            if(chosen<0 || squared<distance){chosen=static_cast<int>(i);distance=squared;ties=1;}else if(squared==distance && random()%++ties==0)chosen=static_cast<int>(i);
        }
        if(chosen>=0)queue_ball_modifier(static_cast<unsigned>(chosen),Modifier::Shield,level,origin);
    }
    if(a==Apple)queue_apple_buffs(2*level,source,origin);
    if(a==Tiger)queue_tiger(3.0f*float(level));
    if(a==Cat){sound("cat_ball");queue_nudges(level*2,origin,true);}
    if(a==Tomato)queue_boom_series(level*3,origin);
    if(a==Bacon) grow_ball(id_of(BallKey::BREAD),2*level,b.p,static_cast<int>(Weight::Heavy));
    if(a==Lettuce) grow_ball(id_of(BallKey::BREAD),level,b.p,static_cast<int>(Weight::Light));
    if(a==Orange) grow_ball(id_of(BallKey::ORANGE_JUICE),static_cast<int>(std::ceil(b.def.base_score*.5*level)),origin);
    if(a==Clocktower)queue_pocket_change(random_pocket_id(),0,-5*static_cast<int>(level),origin);
    if(a==FlowerBush) for(unsigned i=0;i<2*level;++i) plant_pickup(PickupKind::Flower);
    if(a==Piping) for(unsigned i=0;i<2*level;++i) plant_pickup(PickupKind::Oil);
    if(a==Tracks) for(unsigned i=0;i<level;++i) plant_pickup(PickupKind::Launchpad);
    if(a==Stove) for(unsigned i=0;i<level;++i) plant_pickup(PickupKind::Flame);
    if(a==Dandelion) for(int i=0;i<b.def.base_score;++i) plant_pickup(PickupKind::Flower);
    if(a==Rose) grow_biggest_flower(3*level);
    if(a==Sunflower) spend_flowers(3*level);
    if(a==Window) { for(unsigned i=1;i<world.count;++i) if(targetable_ball(world.balls[i])) queue_ball_buff(i,level,origin);improve_flowers(level,true); }
    if(a==Banana) split_random(source,.2*level,origin);
    if(a==Saturn || a==Mercury || a==Venus || a==Earth || a==Mars || a==Jupiter || a==Pluto || a==Neptune || a==Uranus) planet_effect(a,source,level,origin);
    if(a==Gift) gift_effect(level,origin);
    if(a==Cheese) shuffle_scores(level);
    if(a==Star) score_stars(origin,2*level);
    if(a==StarSystem || a==Galaxy) for(unsigned i=0;i<level;++i) {
        unsigned id=id_of(a==StarSystem?BallKey::STAR:BallKey::STAR_SYSTEM);
        spawn_data(id,level,default_item(id,level).score,origin,a);
    }
    if(a==Takoyaki) b.flaming=true;
    if(a==Respawn) respawn_ball(source);
    if(a==Screw) set_ball_locked(b,true);
    if(a==Fridge) unlock_all(origin);
    if(a==Bottle) queue_pocket_shield(random_pocket_id(),level,origin);
    if(a==Frog) { int nearest=closest_target(origin,source);if(nearest>=0) queue_ball_transform(static_cast<unsigned>(nearest),b,level,origin); }
    if(a==Printer) for(unsigned i=0;i<2;++i) spawn_data(id_of(BallKey::BLUEPRINT),level,1<<std::min(level,20u),origin,Printer);
    if(a==Chicken) spawn_data(id_of(BallKey::EGG),level,level,origin,Chicken);
    if(a==Mushroom) { int nearest=closest_target(origin,source);if(nearest>=0) queue_ball_buff(static_cast<unsigned>(nearest),b.def.base_score,origin); }
    if(a==Slot) {
        const unsigned effect_level=std::max(1u,std::min(5u,level));
        const int value=1+random()%(7*effect_level);
        Ball spawn;spawn.def=builtin(id_of(BallKey::B_1_BALL));spawn.def.base_score=0;
        if(queue_spawn(spawn,origin,Slot))for(auto& carrier:actions)if(carrier.serial==action_serial){carrier.flags|=32;carrier.bonus=value;break;}
    }
    if(a==NestingDoll && level>1) { Ball copy=b; copy.level=static_cast<uint8_t>(level-1); queue_spawn(copy,origin,NestingDoll); }
    if(a==Butterfly) for(unsigned i=1;i<world.count;++i) if(targetable_ball(world.balls[i])) queue_ball_buff(i,level,origin);
    if(a==Gold) { gain_money(2*level,"coin"); notice("GOLD: pocket earned money"); }
    if(a==Smiley) { gain_shots(level); notice("SMILEY: gained extra shots"); }
    if(a==Beach) {
        unsigned count=world.pocket_count(),first=random()%count,second=(first+1+random()%(count-1))%count;
        queue_pocket_change(world.pocket_id(first),0,level,origin);queue_pocket_change(world.pocket_id(second),0,level,origin);
    }
    if(a==Pants) queue_repeated_pocket(2*level,1,0,origin);
    if(a==Yinyang){unsigned count=world.pocket_count(),ordinal=world.pocket(pocket)?((pocket<6?pocket:pocket-1)+1+random()%(count-1))%count:random()%count;queue_pocket_change(world.pocket_id(ordinal),1,int(level)-3,origin);}
    if(a==Juggling){
        Pocket* current=world.pocket(pocket);int bonus=current?current->bonus:0;if(current)current->bonus=0;
        if(pocket<6)for(unsigned p:{(pocket+5)%6,(pocket+1)%6})queue_pocket_change(p,0,bonus+2*level,origin);
        else for(unsigned n=0;n<2;++n)queue_pocket_change(random_pocket_id(),0,bonus+2*level,origin);
    }
}
void Game::pocket_listeners(unsigned source,unsigned pocket) {
    if(pockets_after_last_shot<0xffffffffu)++pockets_after_last_shot;
    destroy_fleeting();sync_listeners();const auto snapshot=listeners.snapshot(Signal::PocketAnother);
    for(unsigned listener=0;listener<snapshot.count;++listener) {
        EntityRef ref=snapshot.entities[listener];if(!live_listener(ref)) continue;
        if(ref.kind==EntityKind::Cube) { run_cube_event(Signal::PocketAnother,ref.index,pocket);continue; }
        if(ref.kind==EntityKind::Snack) {
            run_snack_event(Signal::PocketAnother,ref.index,source,pocket);
            continue;
        }
        unsigned i=ref.index;if(i==source) continue;
        run_ball_event(Signal::PocketAnother,i,0,source,pocket);

    }
}
void Game::complete_shot() {
    was_moving=false; phase=Phase::Aiming;
    world.events.push(EventKind::Settled,0,0,world.metrics.ticks); dispatch();
}
void Game::update(double seconds) {
    if(!std::isfinite(seconds)||seconds<0)return;
    if(seed_open||tutorial_replay||gallery_details)return;
    npc_time=std::max(0.0f,npc_time-float(std::min(seconds,.1)));
    if (paused) return;
    if(tutorial.enabled){
        tutorial.menu_time=phase==Phase::Menu?tutorial.menu_time+static_cast<float>(std::min(seconds,.1)):0;
        if(tutorial.menu_time>=1 && tutorial.active==tutorial_count && phase==Phase::Menu && profile.daily_unlocked() && !profile_open && !daily_open)tutorial.trigger("daily-1",profile.tutorial_groups);
        if(tutorial.active==tutorial_count && !inspected && !offer_details && !roster_details && !passive_details){
            if((phase==Phase::Placement || phase==Phase::Aiming) && world.count){for(const auto& cube:cubes)if(cube.id){tutorial.trigger("first-cube",profile.tutorial_groups);break;}tutorial.trigger("intro-1",profile.tutorial_groups);if(round>=2)tutorial.trigger("shots-1",profile.tutorial_groups);if(shots==1 && rounds_played>=1)tutorial.trigger("last-shot",profile.tutorial_groups);}
            if(phase==Phase::Shop && round_summary && !last_round_won && hp>0)tutorial.trigger("lose-round-1",profile.tutorial_groups);
            if(phase==Phase::Shop && !round_summary){tutorial.trigger("shop-intro-1",profile.tutorial_groups);if(rounds_played>1)tutorial.trigger("shop-reserve-1",profile.tutorial_groups);if(daily_date)tutorial.trigger("shop-daily-1",profile.tutorial_groups);if(snack_bar && snack_tickets)tutorial.trigger("passives-1",profile.tutorial_groups);if(cocktail_bar && cocktail_tickets)tutorial.trigger("mix-1",profile.tutorial_groups);}
        }
        tutorial.update(static_cast<float>(std::min(seconds,.1)));if(tutorial.showing()){if(!tutorial.announced){sound("speech_popup");tutorial.announced=true;}return;}
    }

    if(achievement_toast_time>0){achievement_toast_time=std::max(0.0f,achievement_toast_time-static_cast<float>(std::min(seconds,.1)));if(achievement_toast_time==0)achievement_toast=achievement_count;}
    if(achievement_toast_time==0){unsigned next=profile.take_toast();if(next<achievement_count){achievement_toast=next;achievement_toast_time=3.25f;sound("achievement_popup");}}

    progress_timer+=static_cast<float>(std::min(seconds,.1));if(progress_timer>=.25f){progress_timer=0;progress_snapshot();}
    ui_rotation=std::fmod(ui_rotation+static_cast<float>(std::min(seconds,.1)),31.415926535f);
    ui_time+=static_cast<float>(std::min(seconds,.1));if(ui_time>=6)ui_time-=6;
    if(phase==Phase::DailyResult){
        float step=static_cast<float>(std::min(seconds,.1))*std::max(1.0f,float(score)/8+25);
        score_display=score_display<float(score)?std::min(float(score),score_display+step):std::max(float(score),score_display-step);
        const auto thresholds=daily_thresholds(daily_date);const char* cues[]={"bronze_medal","silver_medal","gold_medal","diamond_medal"};
        while(daily_medals_played<4 && score_display>=thresholds[daily_medals_played])sound(cues[daily_medals_played++]);
        if(score_display==float(score) && !daily_result_counted){sound("coin_win");daily_result_counted=true;}
        return;
    }
    if(round_summary && phase==Phase::Shop){
        payout_time+=static_cast<float>(std::min(seconds,.1));
        int base=payout_time>=.7f?payout_base:0;
        float bonus_seconds=std::min(3.0f,float(payout_bonus)/6);
        int bonus=payout_bonus?std::min(payout_bonus,std::max(0,int((payout_time-1.1f)*payout_bonus/std::max(.01f,bonus_seconds)))):0;
        int paidShots=bonus==payout_bonus?std::min(payout_shots,std::max(0,int((payout_time-1.1f-bonus_seconds)*3.333333f))):0;
        if(base!=payout_base_shown)sound(last_round_won?"coin_win":"coin_lose");
        if(bonus!=payout_bonus_shown)sound("coin_uppitchable");
        if(paidShots!=payout_shots_shown)sound("coin_extra_shots");
        payout_base_shown=base;payout_bonus_shown=bonus;payout_shots_shown=paidShots;return;
    }
    float visual_dt=static_cast<float>(std::min(seconds,.1));
    score_display+=(float(score)-score_display)*std::min(1.0f,12*visual_dt);score_flash=std::max(0.0f,score_flash-visual_dt*6);score_sound_time=std::max(0.0f,score_sound_time-visual_dt);
    for(auto& popup:score_popups)if(popup.life>0){popup.life=std::max(0.0f,popup.life-visual_dt);popup.p+=popup.v*visual_dt;popup.v=popup.v*std::pow(.9f,visual_dt*60);}
    if(!semantics.empty()) { drain_semantic();if(!semantics.empty()) return; }
    if (seconds>0.1) { seconds=0.1; ++slow_frames; }
    message_time=std::max(0.0f,message_time-static_cast<float>(seconds));
    for (auto& e:effects) e.life=std::max(0.0f,e.life-static_cast<float>(seconds));
    if (phase!=Phase::Simulating && phase!=Phase::Aiming && phase!=Phase::Placement) return;
    accumulator+=seconds;
    while (accumulator+1e-10>=tick_dt) {
        ritual_progress=std::min(1.0f,ritual_progress+tick_dt*3);
        ritual_flash=std::max(0.0f,ritual_flash-tick_dt);
        bool simulating=phase==Phase::Simulating;
        if (simulating || pending_actions() || world.energy_active() || world.passive_motion() || world.pockets_animating()) { update_pursuit();world.step(); dispatch(); update_actions(); }
        else for(unsigned i=0;i<world.count;++i) { Ball& b=world.balls[i];b.step_start=b.p;b.motion_tick=world.metrics.ticks;b.captured_this_tick=false;b.flash=std::max(0.0f,b.flash-tick_dt); }
        update_pickups();
        if(pending_actions() || world.moving()) { phase=Phase::Simulating; was_moving=true; }
        accumulator-=tick_dt;
        if (was_moving && !world.moving() && !pending_actions()) complete_shot();
        bool remaining=false;
        for (std::size_t i=1;i<world.count;++i) remaining|=world.balls[i].active && !world.balls[i].passive && !world.balls[i].energy;
        if (cap_rewarded || ((shots==0 || !remaining) && !world.moving() && !pending_actions())) {
            end_delay+=tick_dt;
            if (end_delay>=1.0f) { finish_round(); accumulator=0; break; }
        } else end_delay=0;
    }
}
}
