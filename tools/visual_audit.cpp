// Audit-only capture of the unchanged shared renderer. Not linked into game builds.
#include "ultrapool/presentation.hpp"
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdio>
using namespace up;
static Skin skin;
static std::string output_directory="build/visual-audit-decks/";
static std::vector<const Texture*> textures;
struct Capture:Painter {
 std::ofstream out;bool first=true;
 Capture(const char* name):out(output_directory+name+".json"){out<<"[";}
 ~Capture(){out<<"]";}
 void sep(){if(!first)out<<",";first=false;}
 void rect(float x,float y,float w,float h,uint32_t c)override{sep();out<<"[\"rect\","<<x<<","<<y<<","<<w<<","<<h<<","<<c<<"]";}
 void line(float x,float y,float xx,float yy,uint32_t c)override{sep();out<<"[\"line\","<<x<<","<<y<<","<<xx<<","<<yy<<","<<c<<"]";}
 void circle(float x,float y,float r,uint32_t c)override{sep();out<<"[\"circle\","<<x<<","<<y<<","<<r<<","<<c<<"]";}
 void sprite(const Texture&t,float x,float y,float w,float h,uint32_t c)override{sprite_region(t,x,y,w,h,0,0,1,1,c);}
 void sprite_region(const Texture&t,float x,float y,float w,float h,float a,float b,float c,float d,uint32_t tint)override{
  unsigned id=0;for(;id<textures.size()&&textures[id]!=&t;++id){}
  if(id==textures.size()){textures.push_back(&t);char name[128];std::snprintf(name,sizeof(name),"%stexture-%u.raw",output_directory.c_str(),id);std::ofstream f(name,std::ios::binary);uint16_t dims[]={t.width,t.height};f.write((char*)dims,4);f.write((char*)t.pixels,t.width*t.height*2);}
  sep();out<<"[\"sprite\","<<id<<","<<x<<","<<y<<","<<w<<","<<h<<","<<a<<","<<b<<","<<c<<","<<d<<","<<tint<<"]";
 }
 void text(int x,int y,uint32_t color,const char* value)override{draw_ui_text(*this,skin,float(x),float(y),12,color,value);}
};
static void snap(const char* name,Game& g){Capture p(name);paint_scene(p,skin,g,0,0);std::puts(name);}
static Game base(){Game g;g.reset(true);g.load_run("build/psp-visuals-test/ultrapool.sav");g.phase=Phase::Shop;g.round_summary=false;g.paused=false;g.debug_hud=false;g.message_time=0;g.money=30;g.shop_focus=ShopFocus::Market;g.selection=0;g.offers={{9,11,30}};g.continue_available=true;g.menu_preview_checked=true;g.continue_round=g.round;g.continue_money=g.money;for(unsigned i=0;i<3;++i)g.continue_balls[i]=g.inventory[i];return g;}
int main(int argc,char** argv){if(argc>1){output_directory=argv[1];if(output_directory.back()!='/')output_directory+='/';}skin.load("assets/private");Game g=base();
 for(unsigned i=0;i<2;++i){g.phase=Phase::Menu;g.selection=i;g.difficulty=i;snap(i?"02-menu-continue":"01-menu-classic",g);}
 g.difficulty=2;snap("03-menu-difficulty",g);g.continue_available=false;g.notice("No valid saved shop checkpoint");snap("04-menu-save-error",g);
 g=base();snap("05-shop-offer",g);g.selection=1;snap("06-shop-middle-offer",g);g.selection=2;snap("07-shop-right-offer",g);
 g.shop_focus=ShopFocus::Inventory;g.inventory_selection=0;snap("08-shop-rack",g);g.inventory_selection=14;snap("09-shop-reserve",g);
 g.shop_focus=ShopFocus::Actions;for(unsigned i=0;i<3;++i){g.shop_action=i;snap(i==0?"10-shop-roll":i==1?"11-shop-play":"12-shop-sell-button",g);}
 g=base();g.offers={{0,0,0}};snap("13-shop-sold-out",g);g=base();g.money=0;g.notice("Not enough money");snap("14-shop-unaffordable",g);
 g=base();for(auto&b:g.inventory)b=OwnedBall();g.inventory_count=0;g.shop_focus=ShopFocus::Inventory;snap("15-shop-empty-build",g);
 g=base();g.shop_focus=ShopFocus::Inventory;g.inventory[0].id=9;g.inventory[0].level=1;g.merge_pending=true;snap("16-shop-merge-compatible",g);g.inventory_selection=1;snap("17-shop-merge-incompatible",g);
 g=base();g.shop_focus=ShopFocus::Inventory;g.sell_pending=true;snap("18-shop-sale-confirm",g);g.sell_pending=false;g.reorder_pending=true;g.reorder_source=0;snap("19-shop-move-target",g);g.reorder_pending=false;g.owned_merge_pending=true;snap("20-shop-owned-merge",g);
 unsigned longest=0;for(unsigned i=1;i<content_count;++i)if(std::strlen(ball_name(i))>std::strlen(ball_name(longest)))longest=i;
 g=base();g.inventory[0].id=longest;g.inventory[0].mixed_with=longest;g.inventory[0].score=123456;g.inventory[0].level=99;g.shop_focus=ShopFocus::Inventory;g.money=99999999;snap("21-shop-long-name-values",g);g.sell_pending=true;snap("22-shop-long-sale",g);
 g=base();g.snack_bar=true;g.snack_tickets=1;snap("23-snack-offer",g);g.snack_tickets=0;snap("24-snack-no-tickets",g);g.shop_focus=ShopFocus::Inventory;snap("25-snack-owned",g);g.sell_pending=true;snap("26-snack-sale",g);g.sell_pending=false;g.snacks={{0,0,0,0}};snap("27-snack-empty",g);
 g=base();g.cocktail_bar=true;g.cocktail_tickets=1;snap("28-cocktail-first",g);g.mixing_pending=true;g.reorder_source=0;g.inventory_selection=1;snap("29-cocktail-result",g);g.inventory_selection=0;snap("30-cocktail-incompatible",g);g.cocktail_tickets=0;snap("31-cocktail-no-tickets",g);
 g=base();g.next_round();snap("32-table-placement",g);g.phase=Phase::Aiming;g.aim_preparing=true;g.angle=0;snap("33-table-aiming",g);g.phase=Phase::Simulating;g.aim_preparing=false;snap("34-table-simulating",g);
 g.phase=Phase::Aiming;g.inspected=1;snap("35-table-inspection",g);g.world.balls[1].mixed_with=longest;g.world.balls[1].def=builtin(longest);snap("36-table-long-mixed-inspection",g);
 g.inspected=0;g.score=g.target=12345678;g.score_display=12345678;g.shots=40;g.money=99999999;for(auto&p:g.world.pockets){p.bonus=-12345;p.multiplier=1234;}snap("37-table-large-values",g);
 g=base();g.next_round();for(auto&b:g.world.balls)if(b.active){b.flaming=b.shielded=b.fleeting=b.locked=true;b.star_power=1;}for(unsigned i=0;i<g.score_popups.size();++i){g.score_popups[i].life=1;g.score_popups[i].p=g.world.balls[1].p;g.score_popups[i].amount=100+i;}snap("38-table-effects-crowded",g);
 g=base();g.next_round();g.paused=true;snap("39-pause-table",g);g.audio_muted=true;snap("40-pause-muted",g);g=base();g.paused=true;snap("41-pause-shop",g);
 g=base();g.next_round();g.score=23;g.shots=3;g.phase=Phase::Shop;g.round_summary=true;g.last_round_won=true;g.payout_base=4;g.payout_bonus=3;g.payout_shots=3;g.payout_total=g.round_earned=10;g.money=40;g.payout_base_shown=g.payout_bonus_shown=g.payout_shots_shown=0;snap("42-result-counting",g);g.reveal_payout();snap("43-result-complete",g);
 g=base();g.next_round();g.score=0;g.phase=Phase::Shop;g.round_summary=true;g.last_round_won=false;g.hp=2;g.payout_base=13;g.payout_bonus=g.payout_shots=0;g.payout_total=g.round_earned=13;g.money=43;g.reveal_payout();snap("44-result-lost-round",g);
 g.score=12345678;g.target=1234567;g.round_earned=g.payout_base_shown=g.payout_bonus_shown=g.payout_shots_shown=1234567;snap("45-result-large-values",g);
 g=base();g.phase=Phase::RunWon;snap("46-run-complete",g);g.phase=Phase::Lost;snap("47-run-over",g);g.money=g.score=99999999;g.round=10;snap("48-run-over-large-values",g);
 g=base();g.next_round();g.phase=Phase::Aiming;g.aim_preparing=true;{Vec delta=g.world.balls[1].p-g.world.balls[0].p;g.angle=std::atan2(delta.y,delta.x);}snap("49-table-object-trajectory",g);
 g=base();g.next_round();for(unsigned i=0;i<6;++i)g.spawn_pickup(static_cast<PickupKind>(i),Vec(-180+70*i,0),int(i+1));for(auto&p:g.pickups)if(p.active)p.scale=1;snap("50-table-pickups",g);
 g.snacks={{static_cast<uint8_t>(SnackKey::CEREAL),static_cast<uint8_t>(SnackKey::AFTER_EIGHT),static_cast<uint8_t>(SnackKey::SOYSAUCE),static_cast<uint8_t>(SnackKey::FORTUNE_COOKIE)}};for(unsigned i=0;i<4;++i)g.snack_counts[i]=99;snap("51-table-snack-counters",g);
 g=base();g.next_round();g.phase=Phase::Won;snap("52-intermediate-won-phase",g);
 g=base();g.shop_focus=ShopFocus::Passive;g.passive_details=true;snap("53-shop-passive-details",g);g.passive_details=false;g.ui_time=5.9f;g.shop_focus=ShopFocus::Market;snap("54-shop-blink",g);g.cocktail_bar=true;snap("55-cocktail-blink",g);
 g=base();g.shop_focus=ShopFocus::Inventory;g.inventory[0].id=9;g.inventory[0].mixed_with=11;g.roster_details=true;snap("56-owned-mixed-abilities",g);
 std::printf("Longest ball name: %u %s\n",longest,ball_name(longest));
 g=base();g.offer_details=true;snap("57-offer-details",g);g.offer_details=false;g.paused=true;g.settings_selection=1;snap("58-audio-settings",g);
 g=base();for(unsigned i=0;i<5;++i)g.inventory[i].level=i+1;snap("59-level-sparks",g);
 g.snack_bar=true;g.shop_focus=ShopFocus::Inventory;g.inventory_selection=0;g.passive_selection=0;g.passive_details=true;snap("60-owned-snack-details",g);
 g.snack_bar=g.passive_details=false;g.shop_focus=ShopFocus::Passive;g.sell_pending=true;snap("61-main-passive-sale",g);
 g=base();g.difficulty=5;g.round=12;g.next_round();for(unsigned tick=0;tick<720;++tick)g.update(tick_dt);snap("62-all-nighter-curses",g);
 g=base();g.difficulty=3;g.round=3;g.next_round();for(unsigned tick=0;tick<720;++tick)g.update(tick_dt);snap("63-bloody-mary-curses",g);
 for(unsigned deck=0;deck<deck_count;++deck){g=base();g.deck=deck;g.phase=Phase::Menu;char name[64];std::snprintf(name,sizeof(name),"%02u-menu-deck-%u",64+deck,deck);snap(name,g);}
 g=base();g.deck=3;g.reset();for(unsigned tick=0;tick<180;++tick)g.update(tick_dt);snap("72-spooky-start",g);
 g=base();g.deck=1;g.reset();for(unsigned tick=0;tick<180;++tick)g.update(tick_dt);snap("73-nature-start",g);
 g=base();g.deck=7;g.reset();for(unsigned tick=0;tick<360;++tick)g.update(tick_dt);snap("74-gacha-start-transformed",g);
 g=base();g.inventory[0]=default_item(id_of(BallKey::CAULDRON));g.inventory[0].score=4;g.phase=Phase::Shop;snap("75-cauldron-shop-counter",g);
 g=base();g.deck=5;g.reset();g.world.balls[1].active=false;g.world.events.push(EventKind::Pocketed,1,6,g.world.metrics.ticks,g.world.balls[1].generation);g.dispatch();for(unsigned tick=0;tick<36;++tick)g.update(tick_dt);snap("76-tomato-explosion-carriers",g);
 g=base();g.offers[0]=id_of(BallKey::CAULDRON);g.offer_items={};g.run_shop_event(Signal::Reroll);g.run_shop_event(Signal::Reroll);snap("77-buffed-market-value",g);
 g=base();g.next_round();g.queue_tiger(3);for(unsigned tick=0;tick<160;++tick)g.update(tick_dt);snap("78-tiger-cut-in",g);

 g=base();g.phase=Phase::Menu;g.seed_open=true;std::strcpy(g.seed_text,"pool123");snap("79-seed-keyboard",g);
 g=base();g.phase=Phase::Menu;g.tutorial_replay=true;g.replay_step=0;snap("80-tutorial-replay",g);
 g=base();g.phase=Phase::Menu;g.daily_open=true;g.daily_today=20261021;g.daily_history.record(20261020,1000,false);snap("81-daily-calendar",g);
 g=base();g.phase=Phase::Menu;g.profile_open=true;g.profile_tab=2;for(unsigned i=1;i<content_count;++i)g.profile.seen_balls[i]=1;g.gallery_details=true;snap("82-full-collection-description",g);
 g=base();g.react_npc(1);snap("83-bartender-dialogue",g);
 g=base();g.paused=true;g.settings_selection=7;snap("84-expanded-psp-settings",g);
 g=base();g.achievement_toast=1;g.achievement_toast_time=2.5f;snap("85-original-achievement-title",g);

}
