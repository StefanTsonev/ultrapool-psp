#include "ultrapool/core.hpp"
#include <cstdio>
#include <cstring>
#include <new>
#include <algorithm>
namespace up {
namespace {
struct Bytes {
    unsigned char data[1024]{}; unsigned size=0,pos=0; bool ok=true;
    void put(uint32_t v) { if(size+4>sizeof(data)) { ok=false; return; } for(unsigned i=0;i<4;++i) data[size++]=static_cast<unsigned char>(v>>(i*8)); }
    uint32_t get() { if(pos+4>size) { ok=false; return 0; } uint32_t v=0; for(unsigned i=0;i<4;++i) v|=uint32_t(data[pos++])<<(8*i); return v; }
    uint32_t hash(unsigned n) const { uint32_t v=2166136261u; for(unsigned i=0;i<n;++i) v=(v^data[i])*16777619u; return v; }
};
bool read(const char* path,Bytes& b) {
    FILE* f=std::fopen(path,"rb"); if(!f) return false;
    b.size=static_cast<unsigned>(std::fread(b.data,1,sizeof(b.data),f));
    bool ok=b.size>=12 && b.size<sizeof(b.data) && std::fgetc(f)==EOF && !std::ferror(f); std::fclose(f);
    if(!ok) return false;
    b.pos=b.size-4; uint32_t checksum=b.get(); b.pos=0;
    return checksum==b.hash(b.size-4);
}
}
bool Game::save_run(const char* path) const {
    if((phase!=Phase::Shop && !(phase==Phase::DailyResult && daily_date)) || !path || std::strlen(path)>220) return false;
    const unsigned version=12; // Persist shop seeds/counters for normal and daily runs.
    Bytes b; b.put(0x31525055); b.put(version);
    b.put(round); b.put(money); b.put(hp); b.put(difficulty); b.put(trial?1:0); b.put(rng);
    b.put(rerolls); b.put(snack_tickets); b.put(last_round_won?1:0); b.put(static_cast<uint32_t>(score));
    for(const auto& slot:inventory) { b.put(slot.id); b.put(slot.level); b.put(slot.score); }
    for(auto offer:offers) b.put(offer);
    for(auto snack:snacks) b.put(snack);
    for(auto offer:snack_offers) b.put(offer);
    for(const auto& slot:inventory) b.put(slot.mixed_with);
    b.put(cocktail_tickets);
    for(const auto& cube:cubes){b.put(cube.id);b.put(cube.level);}
    {b.put(deck);for(auto set:shop_sets)b.put(static_cast<unsigned>(set));}
    for(unsigned i=0;i<offers.size();++i){auto item=offer_item(i);b.put(item.level);b.put(item.score);b.put(item.mixed_with);}
    {b.put(endless?1:0);b.put(seeded_run?1:0);}
    {b.put(daily_date);b.put(daily_replay?1:0);b.put(phase==Phase::DailyResult?1:0);b.put(static_cast<uint32_t>(daily_random.state));b.put(static_cast<uint32_t>(daily_random.state>>32));b.put(daily_rerolls);}
    b.put(daily_rounds_played);
    {b.put(run_seed);b.put(rounds_played);b.put(total_rerolls);}
    b.put(snack_tickets_used);b.put(gacha_rolls);b.put(scrap_rolls);b.put(astrolabe_rolls);b.put(uint32_t(earned_money_total));b.put(uint32_t(uint64_t(earned_money_total)>>32));b.put(win_streak);b.put(planted_flowers);b.put(scrap_transforms);
    b.put(b.hash(b.size)); if(!b.ok) return false;
    char temporary[240],backup[240]; std::snprintf(temporary,sizeof(temporary),"%s.tmp",path); std::snprintf(backup,sizeof(backup),"%s.bak",path);
    FILE* f=std::fopen(temporary,"wb"); if(!f) return false;
    bool ok=std::fwrite(b.data,1,b.size,f)==b.size;
    if(std::fflush(f)!=0) ok=false;
    if(std::fclose(f)!=0) ok=false;
    if(!ok) { std::remove(temporary); return false; }
    // Keep a recoverable previous checkpoint until the new file has been installed.
    FILE* existing=std::fopen(path,"rb"); bool had_previous=existing!=nullptr; if(existing) std::fclose(existing);
    if(had_previous) { std::remove(backup); if(std::rename(path,backup)!=0) { std::remove(temporary); return false; } }
    if(std::rename(temporary,path)!=0) { if(had_previous) std::rename(backup,path); std::remove(temporary); return false; }
    return true;
}
bool Game::load_run(const char* path) {
    if(!path || std::strlen(path)>220) return false;
    Bytes b;
    if(!read(path,b)) { char backup[240]; std::snprintf(backup,sizeof(backup),"%s.bak",path); if(!read(backup,b)) return false; }
    if(b.get()!=0x31525055) return false;
    unsigned version=b.get(); if(version!=1 && version!=2 && version!=3 && version!=4 && version!=5 && version!=6 && version!=7 && version!=8 && version!=9 && version!=10 && version!=11 && version!=12) return false;
    Game* g=new(std::nothrow) Game(*this); if(!g) return false;
    g->round=static_cast<int>(b.get()); g->money=static_cast<int>(b.get()); g->hp=static_cast<int>(b.get());
    g->difficulty=b.get(); uint32_t trial_value=b.get(); g->trial=trial_value==1; g->rng=b.get();
    g->rerolls=b.get(); g->snack_tickets=b.get(); uint32_t won=b.get(); g->last_round_won=won==1; g->score=static_cast<int>(b.get());
    bool valid=g->round>=1 && g->round<=100 && g->money>=0 && g->money<=100000000 && g->hp>=1 && g->hp<=3 && g->difficulty<=5 && trial_value<=1 && won<=1 && g->rerolls<=100000 && g->snack_tickets<=1000 && g->rng!=0;
    g->inventory_count=0;
    for(auto& slot:g->inventory) {
        unsigned id=b.get(),level=b.get(); int value=static_cast<int>(b.get());
        valid &= id<content_count && level>=1 && level<=31 && value>=1 && value<=100000000;
        slot.id=static_cast<uint16_t>(id); slot.mixed_with=0; slot.level=static_cast<uint8_t>(level); slot.score=value;
        if(id) ++g->inventory_count;
    }
    for(auto& offer:g->offers) { unsigned id=b.get(); valid &= id<content_count; offer=static_cast<uint16_t>(id); }
    for(auto& snack:g->snacks) { unsigned id=b.get(); valid &= id<snack_count; snack=static_cast<uint8_t>(id); }
    for(auto& offer:g->snack_offers) { unsigned id=b.get(); valid &= id<snack_count; offer=static_cast<uint8_t>(id); }
    g->endless=g->seeded_run=false;g->profile_open=false;g->daily_date=0;g->daily_replay=g->daily_open=g->daily_save_pending=false;g->daily_rounds_played=0;g->daily_rerolls=0;g->daily_snapshot_available=true;
    g->cocktail_tickets=0;
    if(version>=2) {
        for(auto& slot:g->inventory) {
            unsigned mixed=b.get();
            valid &= mixed<content_count && (!mixed || (slot.id && mixed!=slot.id && !(number_ball(mixed) && number_ball(slot.id))));
            slot.mixed_with=static_cast<uint16_t>(mixed);
        }
        g->cocktail_tickets=b.get(); valid &= g->cocktail_tickets<=1000;
    }
    g->cubes={};
    if(version>=3) for(unsigned i=0;i<g->cubes.size();++i) {
        unsigned id=b.get(),level=b.get();
        valid &= g->set_cube(i,id,level);
    }
    g->deck=0;g->shop_sets={};
    if(version>=4){
        g->deck=b.get();valid &= g->deck<deck_count;
        for(auto& set:g->shop_sets){unsigned value=b.get();valid &= value<=static_cast<unsigned>(ItemSet::TECH) && value!=static_cast<unsigned>(ItemSet::NEGATIVE) && value!=static_cast<unsigned>(ItemSet::BASIC);set=static_cast<ItemSet>(value);}
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<i;++j)valid &= g->shop_sets[i]==ItemSet::NONE || g->shop_sets[i]!=g->shop_sets[j];
        // Daily runs use three seed-selected sets, independently of the normal deck placeholder.
    } else g->shop_sets[0]=ItemSet::CLASSIC;
    g->offer_items={};
    for(unsigned i=0;i<g->offers.size();++i){
        OwnedBall item=g->offers[i]?default_item(g->offers[i]):OwnedBall();
        if(version>=5){unsigned level=b.get();int value=static_cast<int>(b.get());unsigned mixed=b.get();valid &= level>=1 && level<=5 && value>=1 && value<=100000000 && mixed<content_count && (!mixed || (item.id && mixed!=item.id));item.level=static_cast<uint8_t>(level);item.score=value;item.mixed_with=static_cast<uint16_t>(mixed);}
        g->offer_items[i]=item;
    }
    if(version>=6){auto endless_flag=b.get(),seeded_flag=b.get();valid &= endless_flag<=1 && seeded_flag<=1;g->endless=endless_flag==1;g->seeded_run=seeded_flag==1;}
    bool daily_result=false;
    if(version>=7){g->daily_date=b.get();unsigned replay=b.get(),result=b.get();g->daily_replay=replay==1;daily_result=result==1;uint64_t low=b.get(),high=b.get();g->daily_random.state=low|(high<<32);g->daily_rerolls=b.get();valid &= replay<=1 && result<=1 && g->daily_rerolls<=100000 && (g->daily_date?(valid_daily_date(g->daily_date) && g->difficulty==2 && g->round==static_cast<int>(daily_round(g->daily_date))):(version>=9 && !replay && !result));}
    g->daily_rounds_played=version>=8?b.get():g->daily_date?static_cast<unsigned>(g->round-1):0;
    if(g->daily_date)valid &= g->daily_rounds_played>=static_cast<unsigned>(g->round-1) && g->daily_rounds_played<=1000000;
    g->run_seed=g->rng;g->rounds_played=static_cast<unsigned>(g->round-1+(g->last_round_won?1:0));g->total_rerolls=g->rerolls;
    if(version>=9){g->run_seed=b.get();g->rounds_played=b.get();g->total_rerolls=b.get();valid &= g->rounds_played<=1000000 && g->total_rerolls<=1000000;}

    g->snack_tickets_used=0;for(auto id:g->snacks)g->snack_tickets_used+=id!=0;
    if(version>=10){g->snack_tickets_used=b.get();valid &= g->snack_tickets_used<=1000000;}
    g->gacha_rolls=g->scrap_rolls=g->astrolabe_rolls=0;
    if(version>=11){g->gacha_rolls=b.get();g->scrap_rolls=b.get();g->astrolabe_rolls=b.get();valid &= g->gacha_rolls<=1000000 && g->scrap_rolls<=1000000 && g->astrolabe_rolls<=1000000;}
    g->earned_money_total=g->money;g->win_streak=g->planted_flowers=g->scrap_transforms=0;
    if(version>=12){uint64_t low=b.get(),high=b.get();uint64_t earned=low|(high<<32);valid &= earned<=1000000000000000ULL;g->earned_money_total=static_cast<int64_t>(earned);g->win_streak=b.get();g->planted_flowers=b.get();g->scrap_transforms=b.get();valid &= g->win_streak<=100000 && g->planted_flowers<=100000000 && g->scrap_transforms<=100000000;}
    if(!g->daily_date)valid &= g->deck==7 || g->shop_sets[0]==deck_info(g->deck).set;
    valid &= b.ok && b.pos==b.size-4;
    if(valid) {
        g->audio_muted=audio_muted;g->music_percent=music_percent;g->sfx_percent=sfx_percent;g->settings_dirty=settings_dirty;
        g->offer_details=g->roster_details=g->passive_details=false;
        g->round_summary=false;g->score_popups={};g->score_display=daily_result?0:float(g->score);g->daily_medals_played=0;g->daily_result_counted=false;g->phase=daily_result?Phase::DailyResult:Phase::Shop;g->pickups={};g->pickup_body_generations={};g->snack_counts={};g->semantics.clear();g->semantic_busy=false; g->listeners.clear();g->bound_balls={};g->bound_snacks={};g->snack_generations={}; g->world=World(); g->haunts.clear();g->passive_balls={};g->passive_count=0;g->candles_pocketed=g->ritual_strength=g->completed_ritual_strength=0;g->ritual_progress=g->ritual_flash=0;g->ritual_done=false; g->effects={}; g->actions={}; g->paused=false; g->aim_preparing=false;
        g->selection=g->inventory_selection=g->shop_action=0; g->shop_focus=ShopFocus::Market;
        g->merge_pending=g->sell_pending=g->reorder_pending=g->owned_merge_pending=g->snack_bar=g->cocktail_bar=g->mixing_pending=false;
        g->accumulator=0; g->nav_delay=0; g->save_pending=false;
        g->target=required_score(g->round,g->difficulty);
        if(!daily_result)g->react_npc(10);
        g->notice("Saved shop restored. Your build and offers are intact."); *this=*g;
    }
    delete g; return valid;
}
bool Game::save_settings(const char* path) const {
    if(!path || std::strlen(path)>220 || music_percent>100 || sfx_percent>100 || aim_percent<25 || aim_percent>200 || analog_deadzone<10 || analog_deadzone>60)return false;
    Bytes b;b.put(0x31435055);b.put(music_percent);b.put(sfx_percent);b.put(audio_muted?1:0);b.put(aim_percent);b.put(analog_deadzone);b.put(analog_enabled?1:0);b.put(tutorial_hints?1:0);b.put(reduced_motion?1:0);b.put(b.hash(b.size));
    char temporary[240],backup[240];std::snprintf(temporary,sizeof(temporary),"%s.tmp",path);std::snprintf(backup,sizeof(backup),"%s.bak",path);
    FILE* f=std::fopen(temporary,"wb");if(!f)return false;
    bool ok=std::fwrite(b.data,1,b.size,f)==b.size;if(std::fflush(f)!=0)ok=false;if(std::fclose(f)!=0)ok=false;
    if(!ok){std::remove(temporary);return false;}
    FILE* existing=std::fopen(path,"rb");bool had=existing!=nullptr;if(existing)std::fclose(existing);
    if(had){std::remove(backup);if(std::rename(path,backup)!=0){std::remove(temporary);return false;}}
    if(std::rename(temporary,path)!=0){if(had)std::rename(backup,path);std::remove(temporary);return false;}
    return true;
}
bool Game::load_settings(const char* path) {
    if(!path || std::strlen(path)>220)return false;
    Bytes b;if(!read(path,b)){char backup[240];std::snprintf(backup,sizeof(backup),"%s.bak",path);if(!read(backup,b))return false;}
    if((b.size!=20 && b.size!=40) || b.get()!=0x31435055)return false;
    unsigned music=b.get(),sfx=b.get(),mute=b.get();if(!b.ok||music>100||sfx>100||mute>1)return false;
    unsigned aim=100,dead=30,analog=1,hints=1,motion=0;
    if(b.size==40){aim=b.get();dead=b.get();analog=b.get();hints=b.get();motion=b.get();if(!b.ok||aim<25||aim>200||dead<10||dead>60||analog>1||hints>1||motion>1)return false;}
    music_percent=music;sfx_percent=sfx;audio_muted=mute!=0;aim_percent=aim;analog_deadzone=dead;analog_enabled=analog!=0;tutorial_hints=hints!=0;reduced_motion=motion!=0;settings_dirty=false;return true;
}

}
