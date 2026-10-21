#pragma once
#include "ultrapool/catalog.hpp"
#include "ultrapool/progression_catalog.hpp"
#include <array>
namespace up {
struct Profile {
    std::array<int64_t,profile_stat_count> stats{},round_stats{};
    std::array<uint8_t,achievement_count> unlocked{};
    std::array<uint8_t,achievement_count> seen_achievements{};
    std::array<uint8_t,8> wins{};
    std::array<uint8_t,content_count> seen_balls{};
    std::array<uint8_t,snack_count> seen_snacks{};
    uint64_t tutorial_groups=0;
    bool dirty=false;
    unsigned last_unlock=achievement_count,unlock_serial=0;
    int64_t stat(const char* key)const;
    int64_t round_stat(const char* key)const;
    void track(const char* key,int64_t value=1,bool maximum=false,bool per_round=false);
    bool unlock(const char* key,bool silent=false);
    std::array<uint8_t,achievement_count> toast_queue{};
    unsigned toast_head=0,toast_count=0;
    unsigned take_toast();
    bool has_unseen_achievements()const;
    void mark_achievements_seen();
    bool has(const char* key)const;
    bool completed(unsigned deck,unsigned difficulty)const;
    void record_win(unsigned deck,unsigned difficulty);
    void see_ball(unsigned id);
    void see_snack(unsigned id);
    void mix(unsigned first,unsigned second);
    void check_gallery();
    unsigned achievement_total()const;
    unsigned visible_achievements()const;
    unsigned visible_achievement(unsigned position)const;
    unsigned gallery_count(unsigned kind)const;
    unsigned gallery_id(unsigned kind,unsigned position)const;
    bool daily_unlocked()const{return stat("WIN-GAME")>0;}
    bool endless_unlocked()const{return stat("WIN-GAME")>2;}
    bool save(const char* path)const;
    bool load(const char* path);
};
}
