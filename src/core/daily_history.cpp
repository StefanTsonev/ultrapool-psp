#include "ultrapool/daily.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
namespace up {
namespace {
constexpr unsigned max_records=65536;
struct Stream {
 FILE* file=nullptr;uint32_t hash=2166136261u;bool ok=true;
 void put(uint32_t value,bool checksum=true){for(unsigned i=0;i<4;++i){unsigned char byte=static_cast<unsigned char>(value>>(i*8));if(std::fputc(byte,file)==EOF)ok=false;if(checksum)hash=(hash^byte)*16777619u;}}
 uint32_t get(bool checksum=true){uint32_t value=0;for(unsigned i=0;i<4;++i){int byte=std::fgetc(file);if(byte==EOF){ok=false;return 0;}value|=uint32_t(byte)<<(i*8);if(checksum)hash=(hash^uint32_t(byte))*16777619u;}return value;}
};
bool read_history(const char* path,DailyHistory& history){
 FILE* file=std::fopen(path,"rb");if(!file)return false;Stream in;in.file=file;
 uint32_t magic=in.get(),version=in.get(),count=in.get();bool valid=magic==0x48445055 && version==1 && count<=max_records;
 DailyHistory candidate;if(valid)for(unsigned i=0;i<count && in.ok;++i){DailyRecord record;record.date=in.get();uint32_t score=in.get();record.score=score<=0x7fffffffu?int(score):int(int64_t(score)-4294967296LL);if(!valid_daily_date(record.date) || (!candidate.records.empty() && candidate.records.back().date>=record.date)){valid=false;break;}candidate.records.push_back(record);}
 uint32_t expected=in.hash,checksum=in.get(false);valid &= in.ok && checksum==expected && std::fgetc(file)==EOF && !std::ferror(file);std::fclose(file);if(!valid)return false;history=candidate;return true;
}
}
const DailyRecord* DailyHistory::find(uint32_t date)const{auto found=std::lower_bound(records.begin(),records.end(),date,[](const DailyRecord& a,uint32_t b){return a.date<b;});return found!=records.end() && found->date==date?&*found:nullptr;}
bool DailyHistory::record(uint32_t date,int score,bool replay){
 if(replay || !valid_daily_date(date))return false;
 auto found=std::lower_bound(records.begin(),records.end(),date,[](const DailyRecord& a,uint32_t b){return a.date<b;});
 if(found!=records.end() && found->date==date){if(found->score==score)return true;found->score=score;}
 else {if(records.size()>=max_records)return false;DailyRecord result;result.date=date;result.score=score;records.insert(found,result);}dirty=true;return true;
}
bool DailyHistory::load(const char* path){if(!path || std::strlen(path)>220)return false;DailyHistory candidate;if(!read_history(path,candidate)){char backup[240];std::snprintf(backup,sizeof(backup),"%s.bak",path);if(!read_history(backup,candidate))return false;}*this=candidate;return true;}
bool DailyHistory::save(const char* path)const{
 if(!path || std::strlen(path)>220 || records.size()>max_records)return false;
 char temporary[240],backup[240];std::snprintf(temporary,sizeof(temporary),"%s.tmp",path);std::snprintf(backup,sizeof(backup),"%s.bak",path);
 FILE* file=std::fopen(temporary,"wb");if(!file)return false;Stream out;out.file=file;out.put(0x48445055);out.put(1);out.put(static_cast<uint32_t>(records.size()));for(const auto& record:records){out.put(record.date);out.put(static_cast<uint32_t>(record.score));}out.put(out.hash,false);bool ok=out.ok;if(std::fflush(file))ok=false;if(std::fclose(file))ok=false;if(!ok){std::remove(temporary);return false;}
 DailyHistory previous;bool had=read_history(path,previous);if(had){std::remove(backup);if(std::rename(path,backup)){std::remove(temporary);return false;}}
 else {FILE* old=std::fopen(path,"rb");if(old){std::fclose(old);if(std::remove(path)){std::remove(temporary);return false;}}}
 if(std::rename(temporary,path)){if(had)std::rename(backup,path);std::remove(temporary);return false;}return true;
}
}