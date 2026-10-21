#include "ultrapool/progression.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include "ultrapool/daily_catalog.hpp"
namespace up {
namespace {
unsigned stat_index(const char* key){if(key)for(unsigned i=0;i<profile_stat_count;++i)if(!std::strcmp(key,profile_stat_key(i)))return i;return profile_stat_count;}
unsigned achievement_index(const char* key){if(key)for(unsigned i=0;i<achievement_count;++i)if(!std::strcmp(key,achievement_rule(i).id))return i;return achievement_count;}
const char* decks[]={"1_CLASSIC","2_NATURE","3_TECH","4_SPOOKY","5_FRIENDS","6_FOOD","7_SPACE","GACHA"};
int64_t sum(int64_t a,int64_t b){const auto hi=std::numeric_limits<int64_t>::max(),lo=std::numeric_limits<int64_t>::min();return b>0 && a>hi-b?hi:b<0 && a<lo-b?lo:a+b;}
struct Bytes {
 unsigned char data[4096]{};unsigned size=0,pos=0;bool ok=true;
 void put(uint64_t v,unsigned n){if(size+n>sizeof(data)){ok=false;return;}for(unsigned i=0;i<n;++i)data[size++]=static_cast<unsigned char>(v>>(8*i));}
 uint64_t get(unsigned n){if(pos+n>size){ok=false;return 0;}uint64_t v=0;for(unsigned i=0;i<n;++i)v|=uint64_t(data[pos++])<<(8*i);return v;}
 uint32_t hash(unsigned n)const{uint32_t h=2166136261u;for(unsigned i=0;i<n;++i)h=(h^data[i])*16777619u;return h;}
};
bool read_profile(const char* path,Profile& result){
 FILE* f=std::fopen(path,"rb");if(!f)return false;
 Bytes b;b.size=static_cast<unsigned>(std::fread(b.data,1,sizeof(b.data),f));bool ok=b.size>=24 && b.size<sizeof(b.data) && std::fgetc(f)==EOF && !std::ferror(f);std::fclose(f);if(!ok)return false;
 b.pos=b.size-4;uint32_t checksum=static_cast<uint32_t>(b.get(4));if(checksum!=b.hash(b.size-4))return false;b.pos=0;
   if(b.get(4)!=0x46525055)return false;
   unsigned version=static_cast<unsigned>(b.get(4));
   if(version!=1 && version!=2 && version!=3)return false;
 unsigned stats=static_cast<unsigned>(b.get(2)),achievements=static_cast<unsigned>(b.get(2)),balls=static_cast<unsigned>(b.get(2)),snacks=static_cast<unsigned>(b.get(2));
 if(stats>profile_stat_count || achievements>achievement_count || balls>content_count || snacks>snack_count)return false;
 Profile candidate;for(unsigned i=0;i<stats;++i)candidate.stats[i]=static_cast<int64_t>(b.get(8));
 for(unsigned i=0;i<achievements;++i){auto v=b.get(1);ok&=v<=1;candidate.unlocked[i]=static_cast<uint8_t>(v);}
 for(auto& w:candidate.wins){auto v=b.get(1);ok&=v<64;w=static_cast<uint8_t>(v);}
 for(unsigned i=0;i<balls;++i){auto v=b.get(1);ok&=v<=1;candidate.seen_balls[i]=static_cast<uint8_t>(v);}
 for(unsigned i=0;i<snacks;++i){auto v=b.get(1);ok&=v<=1;candidate.seen_snacks[i]=static_cast<uint8_t>(v);}
 if(version>=2)candidate.tutorial_groups=b.get(8);
 if(version>=3)for(auto& seen:candidate.seen_achievements){auto v=b.get(1);ok&=v<=1;seen=static_cast<uint8_t>(v);}
 if(!ok || !b.ok || b.pos!=b.size-4)return false;
 result=candidate;return true;
}
}
int64_t Profile::stat(const char* key)const{unsigned i=stat_index(key);return i<profile_stat_count?stats[i]:0;}
int64_t Profile::round_stat(const char* key)const{unsigned i=stat_index(key);return i<profile_stat_count?round_stats[i]:0;}
bool Profile::has(const char* key)const{if(key && !*key)return true;unsigned i=achievement_index(key);return i<achievement_count && unlocked[i];}
bool Profile::unlock(const char* key,bool silent){unsigned i=achievement_index(key);if(i>=achievement_count || unlocked[i])return false;unlocked[i]=1;last_unlock=i;++unlock_serial;dirty=true;
 if(!silent && achievement_rule(i).toast && toast_count<toast_queue.size()){toast_queue[(toast_head+toast_count)%toast_queue.size()]=static_cast<uint8_t>(i);++toast_count;}return true;
}
unsigned Profile::take_toast(){if(!toast_count)return achievement_count;unsigned result=toast_queue[toast_head];toast_head=(toast_head+1)%toast_queue.size();--toast_count;return result;}
bool Profile::has_unseen_achievements()const{for(unsigned i=0;i<achievement_count;++i)if(unlocked[i] && !seen_achievements[i])return true;return false;}
void Profile::mark_achievements_seen(){bool changed=false;for(unsigned i=0;i<achievement_count;++i)if(unlocked[i] && !seen_achievements[i]){seen_achievements[i]=1;changed=true;}if(changed)dirty=true;}
void Profile::track(const char* key,int64_t value,bool maximum,bool per_round){
 unsigned index=stat_index(key);if(index>=profile_stat_count)return;
 int64_t next=maximum?std::max(stats[index],value):sum(stats[index],value);if(next!=stats[index]){stats[index]=next;dirty=true;}
 int64_t compare=stats[index];if(per_round){if(value<=round_stats[index])return;round_stats[index]=value;compare=value;}
 for(unsigned i=0;i<achievement_count;++i){const auto& rule=achievement_rule(i);if(rule.per_round==per_round && !std::strcmp(rule.condition,key) && compare>=rule.amount)unlock(rule.id);}
}
bool Profile::completed(unsigned deck,unsigned difficulty)const{return deck<8 && difficulty<6 && (wins[deck]&(1u<<difficulty));}
void Profile::record_win(unsigned deck,unsigned difficulty){
 if(deck>=8 || difficulty>=6)return;
 wins[deck]|=static_cast<uint8_t>(1u<<difficulty);dirty=true;
 track("WIN-GAME");char key[100];std::snprintf(key,sizeof(key),"WIN-DECK-%s",decks[deck]);track(key);std::snprintf(key,sizeof(key),"WIN-DIFFICULTY-diff_%u",difficulty+1);track(key);std::snprintf(key,sizeof(key),"WIN-DECK-DIFFICULTY-%s-diff_%u",decks[deck],difficulty+1);track(key);
 for(unsigned i=0;i<achievement_count;++i){const auto& rule=achievement_rule(i);if(!*rule.deck && !*rule.difficulty)continue;bool met=false;
  for(unsigned d=0;d<8;++d)for(unsigned m=0;m<6;++m){char diff[16];std::snprintf(diff,sizeof(diff),"diff_%u",m+1);if((!*rule.deck || !std::strcmp(rule.deck,decks[d])) && (!*rule.difficulty || !std::strcmp(rule.difficulty,diff)) && completed(d,m))met=true;}
  if(met)unlock(rule.id);
 }
}
void Profile::see_ball(unsigned id){if(id>0 && id<content_count && !seen_balls[id]){seen_balls[id]=1;dirty=true;check_gallery();}}
void Profile::see_snack(unsigned id){if(id>0 && id<snack_count && !seen_snacks[id]){seen_snacks[id]=1;dirty=true;check_gallery();}}
void Profile::mix(unsigned a,unsigned b){track("TOTAL_MIXES_OBTAINED");for(unsigned i=0;i<achievement_count;++i){const auto& rule=achievement_rule(i);if(!*rule.mix_a)continue;unsigned x=find_ball(rule.mix_a),y=find_ball(rule.mix_b);if((a==x && b==y)||(a==y && b==x))unlock(rule.id);}}
void Profile::check_gallery(){
 unsigned total[3]={},seen[3]={};const uint32_t planet=static_cast<uint32_t>(Tag::TAG_PLANET)|static_cast<uint32_t>(Tag::TAG_PLANET_);
 for(unsigned i=1;i<content_count;++i){const auto& b=ball_info(i);if(!b.can_drop && !(b.tags&planet))continue;unsigned cat=b.set==ItemSet::NEGATIVE?2:0;++total[cat];seen[cat]+=seen_balls[i]!=0;}
 for(unsigned i=1;i<snack_count;++i){const auto& s=snack_info(i);if(!s.can_drop && !(s.tags&planet))continue;++total[1];seen[1]+=seen_snacks[i]!=0;}
 const char* ids[]={"unlock_all_balls","unlock_all_passives","unlock_all_cubes"};bool all=true;
 for(unsigned i=0;i<3;++i){if(total[i] && seen[i]>=total[i])unlock(ids[i]);all&=has(ids[i]);}if(all)unlock("unlock_all_gallery");
}
unsigned Profile::achievement_total()const{unsigned n=0;for(auto a:unlocked)n+=a!=0;return n;}
unsigned Profile::visible_achievements()const{unsigned n=0;for(unsigned i=0;i<achievement_count;++i){auto visibility=achievement_rule(i).visibility;n+=visibility==0 || (visibility==1 && unlocked[i]);}return n;}
unsigned Profile::visible_achievement(unsigned position)const{for(unsigned i=0;i<achievement_count;++i){auto visibility=achievement_rule(i).visibility;if(visibility==0 || (visibility==1 && unlocked[i])){if(!position--)return i;}}return achievement_count;}
unsigned Profile::gallery_count(unsigned kind)const{
 if(kind>2)return 0;
 unsigned found=0,count=kind==1?snack_count:content_count;
 const uint32_t planet=static_cast<uint32_t>(Tag::TAG_PLANET)|static_cast<uint32_t>(Tag::TAG_PLANET_);
 for(unsigned i=1;i<count;++i){const auto& info=kind==1?snack_info(i):ball_info(i);if(!info.can_drop && !(info.tags&planet))continue;if(kind!=1 && ((info.set==ItemSet::NEGATIVE)!=(kind==2)))continue;++found;}
 return found;
}
unsigned Profile::gallery_id(unsigned kind,unsigned position)const{
 if(kind>2)return 0;
 struct Sorted {
  std::array<std::array<unsigned,content_count>,3> ids{};unsigned counts[3]{};
  Sorted(){
   const uint32_t planet=static_cast<uint32_t>(Tag::TAG_PLANET)|static_cast<uint32_t>(Tag::TAG_PLANET_);
   for(unsigned k=0;k<3;++k){unsigned total=k==1?snack_count:content_count;
    for(unsigned i=1;i<total;++i){const auto& info=k==1?snack_info(i):ball_info(i);if(!info.can_drop && !(info.tags&planet))continue;if(k!=1 && ((info.set==ItemSet::NEGATIVE)!=(k==2)))continue;ids[k][counts[k]++]=i;}
    std::sort(ids[k].begin(),ids[k].begin()+counts[k],[k](unsigned a,unsigned b){
     const auto& x=k==1?snack_info(a):ball_info(a);const auto& y=k==1?snack_info(b):ball_info(b);
     if(k==0 && x.set!=y.set)return unsigned(x.set)<unsigned(y.set);
     if(x.rarity!=y.rarity)return x.rarity<y.rarity;
     if(k==1)return a<b;
     unsigned ar=content_count,br=content_count,n=0;for(unsigned id:daily_ball_order){if(id==a)ar=n;if(id==b)br=n;++n;}return ar<br;
    });
   }
  }
 };
 static const Sorted sorted;
 return position<sorted.counts[kind]?sorted.ids[kind][position]:0;
}
bool Profile::save(const char* path)const{
 if(!path || std::strlen(path)>220)return false;
 Bytes b;b.put(0x46525055,4);b.put(3,4);b.put(profile_stat_count,2);b.put(achievement_count,2);b.put(content_count,2);b.put(snack_count,2);
 for(auto v:stats)b.put(static_cast<uint64_t>(v),8);
 for(auto v:unlocked)b.put(v,1);
 for(auto v:wins)b.put(v,1);
 for(auto v:seen_balls)b.put(v,1);
 for(auto v:seen_snacks)b.put(v,1);
 b.put(tutorial_groups,8);
 for(auto v:seen_achievements)b.put(v,1);
 b.put(b.hash(b.size),4);if(!b.ok)return false;
 char temporary[240],backup[240];std::snprintf(temporary,sizeof(temporary),"%s.tmp",path);std::snprintf(backup,sizeof(backup),"%s.bak",path);FILE* f=std::fopen(temporary,"wb");if(!f)return false;bool ok=std::fwrite(b.data,1,b.size,f)==b.size;if(std::fflush(f))ok=false;if(std::fclose(f))ok=false;if(!ok){std::remove(temporary);return false;}
 Profile previous;bool had=read_profile(path,previous);
 if(had){std::remove(backup);if(std::rename(path,backup)){std::remove(temporary);return false;}}
 else { // A corrupt main must not replace the valid recovery backup.
  FILE* old=std::fopen(path,"rb");if(old){std::fclose(old);if(std::remove(path)){std::remove(temporary);return false;}}
 }
 if(std::rename(temporary,path)){if(had)std::rename(backup,path);std::remove(temporary);return false;}return true;
}
bool Profile::load(const char* path){if(!path || std::strlen(path)>220)return false;Profile candidate;if(!read_profile(path,candidate)){char backup[240];std::snprintf(backup,sizeof(backup),"%s.bak",path);if(!read_profile(backup,candidate))return false;}*this=candidate;
 for(unsigned i=0;i<achievement_count;++i){const auto& rule=achievement_rule(i);if(*rule.condition && stat(rule.condition)>=rule.amount)unlock(rule.id);}
 check_gallery();return true;}
}
