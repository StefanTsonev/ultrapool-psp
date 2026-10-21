#pragma once
#include "ultrapool/catalog.hpp"
#include <array>
#include <vector>
namespace up {
struct Pcg32 {
    uint64_t state=0,increment=0;
    explicit Pcg32(uint64_t seed=0,uint64_t sequence=1442695040888963407ULL){reset(seed,sequence);}
    void reset(uint64_t seed,uint64_t sequence=1442695040888963407ULL);
    uint32_t next();
    uint32_t bounded(uint32_t bound);
    float global_unit();
    float dense_unit();
    double range_unit();
    unsigned weighted_set(unsigned count);
};
bool valid_daily_date(uint32_t date);
int daily_serial(uint32_t date);
uint32_t daily_date_from_serial(int serial);
uint32_t daily_today_utc();
uint32_t daily_seed(uint32_t date);
unsigned daily_round(uint32_t date);
bool daily_in_window(uint32_t date,uint32_t today);
void format_daily_date(uint32_t date,char* out,unsigned size);
std::array<int,4> daily_thresholds(uint32_t date);
unsigned daily_medal(uint32_t date,int score);
struct DailySetup {
    uint32_t date=0,seed=0;
    unsigned round=0,money=0,snack_tickets=0,cocktail_tickets=0;
    std::array<ItemSet,3> sets{};
    std::array<uint16_t,3> balls{};
    bool valid=false;
};
DailySetup make_daily(uint32_t date);
struct DailyRecord {uint32_t date=0;int score=0;};
struct DailyHistory {
    std::vector<DailyRecord> records;
    bool dirty=false;
    const DailyRecord* find(uint32_t date)const;
    bool record(uint32_t date,int score,bool replay=false);
    bool save(const char* path)const;
    bool load(const char* path);
};
}