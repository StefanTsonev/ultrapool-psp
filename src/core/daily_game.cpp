#include "ultrapool/core.hpp"
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>
namespace up {
bool Game::start_daily(uint32_t date,bool replay){
 const DailySetup setup=make_daily(date);if(!setup.valid)return false;
 std::unique_ptr<Game> next(new(std::nothrow) Game());if(!next)return false;
 next->profile=profile;next->tutorial=tutorial;next->achievement_toast=achievement_toast;next->achievement_toast_time=achievement_toast_time;next->daily_history=daily_history;next->definitions=definitions;next->definition_count=definition_count;
 next->copy_settings_from(*this);next->debug_hud=debug_hud;next->settings_dirty=settings_dirty;
 next->daily_date=date;next->daily_today=daily_today;next->platform_date=platform_date;next->daily_replay=replay;next->daily_random.reset(setup.seed);next->rng=setup.seed;
 next->daily_rounds_played=setup.round-1;next->round=static_cast<int>(setup.round);next->target=required_score(setup.round,2);next->difficulty=2;next->deck=0;next->hp=3;next->money=setup.money;next->snack_tickets=setup.snack_tickets;next->cocktail_tickets=setup.cocktail_tickets;next->shop_sets=setup.sets;
 for(unsigned i=0;i<3;++i)next->inventory[i]=default_item(setup.balls[i]);
 next->inventory_count=3;
 next->phase=Phase::Shop;next->stock_shop();next->stock_snacks();next->progress_snapshot();next->save_pending=true;next->notice(replay?"Daily replay: original result is preserved":"Daily: prepare your build, then play one round");
 *this=*next;return true;
}
bool Game::choose_daily(uint32_t date,bool replay,const char* prefix){
 if(!prefix || std::strlen(prefix)>180 || !profile.daily_unlocked() || !valid_daily_date(date) || (!daily_in_window(date,daily_today) && (replay || !daily_history.find(date))))return false;
 char checkpoint[240];std::snprintf(checkpoint,sizeof(checkpoint),"%s.daily.sav",prefix);
 std::unique_ptr<Game> saved(new(std::nothrow) Game(*this));if(!saved)return false;
 if(!replay && saved->load_run(checkpoint) && saved->daily_date){
  if(!daily_in_window(saved->daily_date,daily_today)){std::remove(checkpoint);char backup[240];std::snprintf(backup,sizeof(backup),"%s.daily.sav.bak",prefix);std::remove(backup);}
  else if(saved->daily_date==date && saved->phase==Phase::Shop){*this=*saved;daily_open=false;return true;}
 }
 if(!replay)if(const DailyRecord* record=daily_history.find(date)){
  char snapshot[240];std::snprintf(snapshot,sizeof(snapshot),"%s.daily-%08u.sav",prefix,unsigned(date));
  if(saved->load_run(snapshot) && saved->daily_date==date && saved->phase==Phase::DailyResult){*this=*saved;daily_open=false;sound("win");return true;}
  // A history result can still be viewed when its optional full-build snapshot is unavailable.
  daily_date=date;score=record->score;round=static_cast<int>(daily_round(date));difficulty=2;target=required_score(round,2);daily_replay=false;daily_snapshot_available=false;daily_open=false;score_display=0;daily_medals_played=0;daily_result_counted=false;phase=Phase::DailyResult;sound("win");return true;
 }
 return start_daily(date,replay || date!=daily_today);
}
bool Game::continue_daily(const char* prefix){
 if(phase!=Phase::DailyResult || !daily_date || !daily_snapshot_available)return false;
 if(daily_save_pending && !flush_daily(prefix))return false;
 ++daily_rounds_played;last_round_won=false;enter_shop();round_summary=false;notice("Daily continuation: the same round and current build");return true;
}
bool Game::flush_daily(const char* prefix){
 if(!prefix || std::strlen(prefix)>180)return false;
 if(daily_save_pending && !daily_replay){char result[240];std::snprintf(result,sizeof(result),"%s.daily-%08u.sav",prefix,unsigned(daily_date));if(!save_run(result))return false;}
 if(daily_history.dirty){char path[240];std::snprintf(path,sizeof(path),"%s.daily",prefix);if(!daily_history.save(path))return false;daily_history.dirty=false;}
 if(daily_save_pending){char checkpoint[240],backup[240];std::snprintf(checkpoint,sizeof(checkpoint),"%s.daily.sav",prefix);std::snprintf(backup,sizeof(backup),"%s.daily.sav.bak",prefix);std::remove(checkpoint);std::remove(backup);daily_save_pending=false;}
 return true;
}
}