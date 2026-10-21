#include "ultrapool/core.hpp"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <cstdio>
namespace up {
void Game::copy_settings_from(const Game& other){
 music_percent=other.music_percent;sfx_percent=other.sfx_percent;audio_muted=other.audio_muted;
 aim_percent=other.aim_percent;analog_deadzone=other.analog_deadzone;analog_enabled=other.analog_enabled;
 tutorial_hints=other.tutorial_hints;reduced_motion=other.reduced_motion;settings_dirty=other.settings_dirty;
}
void Game::gain_money(int amount,const char* source){
 if(amount<=0)return;
 money+=amount;earned_money_total+=amount;
 progress_track("MAX-MONEY-EARNED",earned_money_total,true);
 progress_track("MAX-MONEY-HELD",money,true);
 if(source && std::strcmp(source,"none"))sound(source);
}
void Game::react_npc(unsigned group){
 static const unsigned counts[]={4,3,3,3,3,3,3,1,3,2,2,1,1,1,2,1};
 if(group>=sizeof(counts)/sizeof(counts[0]))return;
 unsigned next=++npc_serial%counts[group];
 if(group==npc_group && next==npc_variant && counts[group]>1)next=(next+1)%counts[group];
 npc_group=group;npc_variant=next;npc_time=4.25f;sound("speech_popup",.5f);
}
bool Game::start_seeded_run(){
 if(!*seed_text){notice("Enter a seed first");return false;}
 if(!std::strcmp(seed_text,"ludwig")){deck=0;difficulty=3;}
 if(!profile.has(deck_requirement(deck)) || !profile.has(difficulty_requirement(difficulty))){notice("Selected deck or difficulty is locked");return false;}
 const char* v=seed_text;if(*v=='-')++v;bool numeric=*v!=0;
 for(const char* c=v;*c;++c)numeric&=*c>='0'&&*c<='9';
 uint32_t value=0;
 if(numeric){for(;*v;++v)value=value*10+unsigned(*v-'0');if(seed_text[0]=='-')value=0u-value;}
 else{value=5381;for(const unsigned char* c=reinterpret_cast<const unsigned char*>(seed_text);*c;++c)value=value*33+*c;value+=69;}
 set_run_seed(value,true);seed_open=false;reset(false);return true;
}
bool Game::menu_overlay_input(const Input& in,float seconds){
 if(!seed_open && !tutorial_replay && !gallery_details)return false;
 float dt=std::max(0.0f,std::min(seconds,.05f));nav_delay=std::max(0.0f,nav_delay-dt);
 if(tutorial_replay){
  if(in.cancel||in.pause){tutorial_replay=false;return true;}
  if(nav_delay<=0 && (in.left||in.right||in.up||in.down||in.more||in.less||in.confirm)){
   int delta=in.left||in.up||in.less?-1:1;
   if(in.more||in.less){unsigned group=tutorial_rules[replay_step].group;for(unsigned n=0;n<tutorial_count;++n){replay_step=(replay_step+tutorial_count+delta)%tutorial_count;if(tutorial_rules[replay_step].group!=group)break;}}
   else replay_step=(replay_step+tutorial_count+delta)%tutorial_count;
   nav_delay=.16f;sound("button_pos");
  }return true;
 }
 if(gallery_details){
  if(in.cancel||in.confirm||in.pause){gallery_details=false;return true;}
  if(nav_delay<=0 && (in.left||in.right)){unsigned count=profile.gallery_count(profile_tab-2);if(count)profile_selection=(profile_selection+count+(in.right?1:-1))%count;nav_delay=.16f;}
  return true;
 }
 if(in.cancel){seed_open=false;return true;}
 if(in.pause){start_seeded_run();return true;}
 if(nav_delay<=0 && (in.left||in.right||in.up||in.down)){
  unsigned row=seed_cursor/6,col=seed_cursor%6;
  if(in.left)col=(col+5)%6;
  if(in.right)col=(col+1)%6;
  if(in.up)row=(row+6)%7;
  if(in.down)row=(row+1)%7;
  seed_cursor=row*6+col;nav_delay=.16f;
 }
 if(in.inspect){unsigned n=unsigned(std::strlen(seed_text));if(n)seed_text[n-1]=0;}
 if(in.confirm){
  static const char keys[]="abcdefghijklmnopqrstuvwxyz0123456789- ";
  unsigned n=unsigned(std::strlen(seed_text));
  if(seed_cursor<38 && n<sizeof(seed_text)-1){seed_text[n]=keys[seed_cursor];seed_text[n+1]=0;}
  else if(seed_cursor==38){if(n)seed_text[n-1]=0;}
  else if(seed_cursor==39)seed_text[0]=0;
  else if(seed_cursor==40)start_seeded_run();
  else if(seed_cursor==41)seed_open=false;
  sound("button_pos");
 }return true;
}
bool Game::settings_input(const Input& in,float seconds){
 if(!paused)return false;
 float dt=std::max(0.0f,std::min(seconds,.05f));nav_delay=std::max(0.0f,nav_delay-dt);
 if(in.cancel){paused=false;return true;}
 if(settings_selection>9)settings_selection=0;
 if(nav_delay<=0 && (in.up||in.down)){settings_selection=(settings_selection+(in.down?1:9))%10;nav_delay=.16f;}
 if(in.inspect){audio_muted=!audio_muted;settings_dirty=true;}
 if(nav_delay<=0 && (in.left||in.right||in.confirm)){
  int delta=in.left?-1:1;unsigned i=settings_selection;
  if(i==0 || i==1){unsigned& v=i?sfx_percent:music_percent;v=unsigned(std::max(0,std::min(100,int(v)+5*delta)));}
  if(i==2)audio_muted=!audio_muted;
  if(i==3)aim_percent=unsigned(std::max(25,std::min(200,int(aim_percent)+25*delta)));
  if(i==4)analog_enabled=!analog_enabled;
  if(i==5)analog_deadzone=unsigned(std::max(10,std::min(60,int(analog_deadzone)+5*delta)));
  if(i==6){tutorial_hints=!tutorial_hints;tutorial.enabled=tutorial_hints;if(!tutorial_hints)tutorial.active=tutorial_count;}
  if(i==7)reduced_motion=!reduced_motion;
  if(i==1)sound("BallOnBall",.6f);
  if(i==8 && in.confirm){paused=false;tutorial_replay=true;replay_step=0;return true;}
  if(i==9 && in.confirm){paused=false;phase=Phase::Menu;selection=0;seed_open=gallery_details=profile_open=daily_open=false;menu_preview_checked=false;save_pending=false;notice("Resume returns to your last shop checkpoint");return true;}
  settings_dirty=true;nav_delay=.12f;
 }return true;
}
void Game::shift_daily_month(int delta){
 uint32_t chosen=daily_date_from_serial(daily_serial(daily_today)-int(daily_selection));
 int y=int(chosen/10000),m=int(chosen/100)%100,d=int(chosen%100);
 m+=delta;if(m<1){m=12;--y;}if(m>12){m=1;++y;}
 uint32_t shifted=unsigned(y*10000+m*100+d);
 while(d>1 && !valid_daily_date(shifted))shifted=unsigned(y*10000+m*100+--d);
 if(valid_daily_date(shifted))daily_selection=unsigned(std::max(0,daily_serial(daily_today)-daily_serial(shifted)));
}
}
