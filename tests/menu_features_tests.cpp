#include "ultrapool/presentation.hpp"
#include <memory>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>
using namespace up;
namespace {unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"FAIL FEATURE BATCH: %s\n",why);std::exit(1);}}
void clean(const char* path){std::remove(path);char backup[256];std::snprintf(backup,sizeof(backup),"%s.bak",path);std::remove(backup);}
}
unsigned menu_features_tests(){
 checks=0;
 {std::unique_ptr<Game> p(new Game);Game& g=*p;g.phase=Phase::Menu;g.menu_preview_checked=true;
  Input open;open.less=true;g.input(open,.016f);check(g.seed_open,"L opens seed editor");
  g.seed_cursor=0;Input key;key.confirm=true;g.input(key,.016f);check(!std::strcmp(g.seed_text,"a"),"seed keyboard enters character");
  key=Input();key.inspect=true;g.input(key,.016f);check(!*g.seed_text,"Square deletes seed character");
  check(!g.start_seeded_run() && g.phase==Phase::Menu,"empty seed cannot start a run");
  std::strcpy(g.seed_text,"12345");check(g.start_seeded_run() && g.seeded_run && g.run_seed==12345,"numeric seed reaches gameplay without achievements");
  auto stats=g.profile.stats;g.progress_track("WIN-GAME");check(g.profile.stats==stats,"seed entry preserves seeded achievement exclusion");
  g.phase=Phase::Menu;g.seed_open=true;std::strcpy(g.seed_text,"pool");
  uint32_t expected=5381;for(const char* c="pool";*c;++c)expected=expected*33+unsigned(*c);expected+=69;
  check(g.start_seeded_run() && g.run_seed==expected,"text seed uses recovered DJB2 plus 69 rule");
  g.phase=Phase::Menu;g.seed_open=true;std::strcpy(g.seed_text,"-1");check(g.start_seeded_run() && g.run_seed==0xffffffffu,"signed numeric seed preserves native low 32 bits");
  g.phase=Phase::Menu;g.selection=0;g.menu_preview_checked=true;g.input(Input(),.05f);g.input(open,.05f);Input close;close.cancel=true;g.input(close,.05f);check(!g.seed_open,"Circle cancels seed entry");
 }
 {std::unique_ptr<Game> p(new Game);Game& g=*p;
  g.music_percent=40;g.sfx_percent=85;g.aim_percent=50;g.analog_deadzone=20;g.analog_enabled=false;g.tutorial_hints=false;g.reduced_motion=true;
  const char* path="build/feature-settings.cfg";clean(path);check(g.save_settings(path),"expanded PSP settings write");
  Game restored;check(restored.load_settings(path) && restored.aim_percent==50 && restored.analog_deadzone==20 && !restored.analog_enabled && !restored.tutorial_hints && restored.reduced_motion,"expanded settings survive restart");
  unsigned char old[20]{};uint32_t words[]={0x31435055,20,70,0};for(unsigned n=0;n<4;++n)for(unsigned b=0;b<4;++b)old[n*4+b]=static_cast<unsigned char>(words[n]>>(8*b));
  uint32_t hash=2166136261u;for(unsigned n=0;n<16;++n)hash=(hash^old[n])*16777619u;for(unsigned b=0;b<4;++b)old[16+b]=static_cast<unsigned char>(hash>>(8*b));
  FILE* f=std::fopen(path,"wb");check(f!=nullptr,"old settings fixture opens");std::fwrite(old,1,20,f);std::fclose(f);
  check(restored.load_settings(path) && restored.aim_percent==100 && restored.analog_enabled && restored.music_percent==20,"old settings load with original default controls");clean(path);
  g.reset();g.tutorial.enabled=false;g.phase=Phase::Aiming;g.aim_preparing=true;float before=g.angle;Input right;right.right=true;g.input(right,.05f);
  check(std::fabs(g.angle-before-.006f)<.00001f,"50 percent fine aim scales original turn speed");
  before=g.angle;Input analog;analog.analog_x=1;g.input(analog,.05f);check(g.angle==before,"analog-disable setting prevents stick drift");
  g.tutorial_replay=true;g.replay_step=0;uint64_t groups=g.profile.tutorial_groups;Input next;next.confirm=true;g.input(next,.05f);
  check(g.replay_step==1 && g.profile.tutorial_groups==groups,"tutorial replay browses without resetting saved progress");
  auto tick=g.world.metrics.ticks;g.update(.05);check(g.world.metrics.ticks==tick,"help overlay pauses gameplay");
  Input close;close.cancel=true;g.input(close,.05f);check(!g.tutorial_replay,"tutorial replay closes safely");
 }
 {std::unique_ptr<Game> p(new Game);Game& g=*p;g.daily_today=20240331;g.shift_daily_month(-1);check(daily_date_from_serial(daily_serial(g.daily_today)-int(g.daily_selection))==20240229,"calendar clamps month jump to leap day");
  g.daily_today=20250331;g.daily_selection=0;g.shift_daily_month(-1);check(daily_date_from_serial(daily_serial(g.daily_today)-int(g.daily_selection))==20250228,"calendar handles non-leap February");
  g.daily_today=20260115;g.daily_selection=0;g.shift_daily_month(-1);check(daily_date_from_serial(daily_serial(g.daily_today)-int(g.daily_selection))==20251215,"calendar crosses year boundary");
 }
 {std::unique_ptr<Game> p(new Game);Game& g=*p;g.reset();g.tutorial.enabled=false;g.gain_money(12);g.money-=7;g.gain_money(5);
  check(g.earned_money_total==17 && g.profile.stat("MAX-MONEY-EARNED")==17,"income record counts gross earnings rather than wallet balance");
  for(unsigned n=0;n<102;++n)g.spawn_pickup(PickupKind::Flower,Vec(float(n),0),1);
  check(g.planted_flowers==102 && g.profile.stat("FLOWERS_PLANTED")==102 && g.profile.has("flowers_planted_100"),"flower achievement counts cap-merged plantings");
  g.phase=Phase::Shop;g.scrap_transforms=7;g.win_streak=4;const char* path="build/feature-run.sav";clean(path);
  check(g.save_run(path),"new run counter format saves");Game loaded;loaded.aim_percent=75;
  check(loaded.load_run(path) && loaded.earned_money_total==17 && loaded.planted_flowers==102 && loaded.scrap_transforms==7 && loaded.win_streak==4 && loaded.aim_percent==75,"run counters persist without overwriting device settings");clean(path);
  auto digest=[](const char* name){FILE* f=std::fopen(name,"rb");uint32_t h=2166136261u;int c;while((c=std::fgetc(f))!=EOF)h=(h^unsigned(c))*16777619u;std::fclose(f);return h;};
  check(g.save_run(path),"dialogue RNG baseline checkpoint");uint32_t before=digest(path);
  g.react_npc(1);unsigned first=g.npc_variant;g.react_npc(1);check(g.save_run(path),"dialogue RNG comparison checkpoint");
  check(digest(path)==before && g.npc_variant!=first && g.npc_time>0,"dialogue does not change saved RNG or repeat consecutively");clean(path);
 }
 {PrivateTextBank bank;const char* path="build/feature-prose.uat";
  const unsigned char valid[]={'U','T','X','1',1,0,0,0,1,0,4,0,5,0,'T','e','s','t','H','e','l','l','o'};
  FILE* f=std::fopen(path,"wb");check(f!=nullptr,"synthetic text bank opens");std::fwrite(valid,1,sizeof(valid),f);std::fclose(f);
  check(bank.load(path,content_count) && !std::strcmp(bank.title(1),"Test") && !std::strcmp(bank.text(1),"Hello"),"bounded private prose loader parses synthetic text");
  f=std::fopen(path,"wb");std::fwrite(valid,1,sizeof(valid)-1,f);std::fclose(f);
  check(!bank.load(path,content_count) && !std::strcmp(bank.title(1),"Test"),"malformed bank cannot replace previously loaded prose");std::remove(path);
 }

 {Profile p;unsigned count=p.gallery_count(0);check(count>100,"gallery retains original eligible entries");
  bool used[content_count]{};unsigned last_set=0,last_rarity=0;
  for(unsigned n=0;n<count;++n){unsigned id=p.gallery_id(0,n);const auto& info=ball_info(id);
   check(id>0 && !used[id],"gallery sorting retains every identity exactly once");used[id]=true;
   if(n && unsigned(info.set)==last_set)check(info.rarity>=last_rarity,"rarity order is ascending within each set");
   else if(n)check(unsigned(info.set)>=last_set,"gallery groups original ball sets");
   last_set=unsigned(info.set);last_rarity=info.rarity;
  }
 }
 std::printf("PSP feature batch checks: %u\n",checks);return checks;
}
