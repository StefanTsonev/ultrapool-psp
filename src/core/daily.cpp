#include "ultrapool/core.hpp"
#include "ultrapool/daily_catalog.hpp"
#include <algorithm>
#include <cstdio>
#include <ctime>
namespace up {
void Pcg32::reset(uint64_t seed,uint64_t sequence){state=0;increment=(sequence<<1)|1;next();state+=seed;next();}
uint32_t Pcg32::next(){uint64_t previous=state;state=previous*6364136223846793005ULL+increment;unsigned rotation=static_cast<unsigned>(previous>>59);uint32_t bits=static_cast<uint32_t>(((previous>>18)^previous)>>27);return (bits>>rotation)|(bits<<((32-rotation)&31));}
uint32_t Pcg32::bounded(uint32_t bound){if(!bound)return 0;uint32_t minimum=uint32_t(0-bound)%bound;for(;;){uint32_t value=next();if(value>=minimum)return value%bound;}}
float Pcg32::global_unit(){return float(next())/4294967295.0f;}
namespace {unsigned leading_zeros(uint32_t value){unsigned count=0;while(value && !(value&0x80000000u)){++count;value<<=1;}return count;}}
float Pcg32::dense_unit(){uint32_t exponent=next();if(!exponent)return 0;uint32_t mantissa=next()|0x80000001u;return std::ldexp(float(mantissa),-32-int(leading_zeros(exponent)));}
double Pcg32::range_unit(){uint32_t exponent=next();if(!exponent)return 0;uint64_t high=next(),low=next();uint64_t mantissa=(high<<32)|low|0x8000000000000001ULL;return std::ldexp(double(mantissa),-64-int(leading_zeros(exponent)));}
unsigned Pcg32::weighted_set(unsigned count){if(count<2)return 0;float remaining=dense_unit()*float(count+1);for(unsigned i=0;i<count;++i){remaining-=i?1.0f:2.0f;if(remaining<0)return i;}return count-1;}
namespace {
bool leap(unsigned year){return year%4==0 && (year%100!=0 || year%400==0);}
unsigned month_days(unsigned year,unsigned month){const unsigned days[]={31,28,31,30,31,30,31,31,30,31,30,31};return month>=1 && month<=12?days[month-1]+(month==2 && leap(year)):0;}
int year_start(unsigned year){int y=int(year)-1;return 365*y+y/4-y/100+y/400-719162;}
}
bool valid_daily_date(uint32_t date){unsigned y=date/10000,m=(date/100)%100,d=date%100;return y>=1 && y<=9999 && m>=1 && m<=12 && d>=1 && d<=month_days(y,m);}
int daily_serial(uint32_t date){if(!valid_daily_date(date))return -2147483647;unsigned y=date/10000,m=(date/100)%100;int serial=year_start(y)+int(date%100)-1;for(unsigned i=1;i<m;++i)serial+=month_days(y,i);return serial;}
uint32_t daily_date_from_serial(int serial){if(serial<year_start(1) || serial>=year_start(10000))return 0;unsigned low=1,high=10000;while(low+1<high){unsigned middle=(low+high)/2;if(year_start(middle)<=serial)low=middle;else high=middle;}unsigned day=static_cast<unsigned>(serial-year_start(low)),month=1;while(day>=month_days(low,month))day-=month_days(low,month++);return low*10000+month*100+day+1;}
uint32_t daily_today_utc(){std::time_t now=std::time(nullptr);std::tm* utc=std::gmtime(&now);return utc?uint32_t((utc->tm_year+1900)*10000+(utc->tm_mon+1)*100+utc->tm_mday):0;}
void format_daily_date(uint32_t date,char* out,unsigned size){if(out && size)std::snprintf(out,size,"%04u-%02u-%02u",unsigned(date/10000),unsigned((date/100)%100),unsigned(date%100));}
uint32_t daily_seed(uint32_t date){if(!valid_daily_date(date))return 0;uint32_t n=date;n=((n>>16)^n)*73244475u;n=((n>>16)^n)*73244475u;n=(n>>16)^n;return 10000000+n%90000000;}
unsigned daily_round(uint32_t date){if(!valid_daily_date(date))return 0;int monday=(daily_serial(date)+3)%7;if(monday<0)monday+=7;return 6+static_cast<unsigned>(monday);}
bool daily_in_window(uint32_t date,uint32_t today){if(!valid_daily_date(date)||!valid_daily_date(today))return false;int distance=daily_serial(today)-daily_serial(date);return distance>=0 && distance<=7;}
std::array<int,4> daily_thresholds(uint32_t date){int score=required_score(daily_round(date),2);return {{int(std::floor(score*.25+.5)),int(std::floor(score*.5+.5)),score,score*2}};}
unsigned daily_medal(uint32_t date,int score){if(!valid_daily_date(date))return 0;unsigned medal=0;for(int threshold:daily_thresholds(date))medal+=score>=threshold;return medal;}
DailySetup make_daily(uint32_t date){
 DailySetup result;if(!valid_daily_date(date))return result;result.date=date;result.seed=daily_seed(date);result.round=daily_round(date);unsigned level=result.round-1;result.money=level*10;result.snack_tickets=std::min(4u,1+level/3);result.cocktail_tickets=std::min(4u,level/3);
 Pcg32 random(result.seed);ItemSet available[7];std::copy(daily_set_order,daily_set_order+7,available);unsigned remaining=7;
 for(unsigned i=0;i<3;++i){unsigned choice=random.next()%remaining;result.sets[i]=available[choice];for(unsigned j=choice+1;j<remaining;++j)available[j-1]=available[j];--remaining;}
 for(unsigned slot=0;slot<3;++slot){random.next(); // get_random_ball consumes its from-set probability even with an empty preference.
  uint16_t options[content_count];unsigned count=0;
  for(unsigned id:daily_ball_order){const auto& info=ball_info(id);if(!info.can_drop || info.rarity || info.set==ItemSet::NEGATIVE)continue;bool included=false,used=false;for(auto set:result.sets)included|=info.set==set;for(unsigned previous=0;previous<slot;++previous)used|=result.balls[previous]==id;if(included && !used)options[count++]=static_cast<uint16_t>(id);}
  if(!count)return result;
  result.balls[slot]=options[random.next()%count];
 }
 result.valid=true;return result;
}
}