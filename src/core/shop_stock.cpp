#include "ultrapool/shop_stock.hpp"
#include "ultrapool/daily_catalog.hpp"
#include "ultrapool/snack_stock_catalog.hpp"
#include <algorithm>
namespace up {
namespace {
unsigned roll(Pcg32& random,unsigned level,const ItemSet* sets,unsigned set_count,const unsigned* excluded,unsigned excluded_count,bool exclude_numbers=false,bool common_uncommon=false){
 // Global randf_range and randf have different draw counts in Godot 4.6.
 double value=random.range_unit(),progress=std::max(0.0,std::min(1.0,(double(level)-1)/10));
 double common=12+(8-12)*progress,uncommon=3+(2-3)*progress;
 double rare=common_uncommon?0:-.3+(1+.3)*progress,legendary=common_uncommon?0:-.5+(.25+.5)*progress;
 double sum=common+uncommon+rare+legendary;unsigned rarity=value<=common/sum?0:value<=(common+uncommon)/sum?1:value<=(common+uncommon+rare)/sum?2:3;
 random.global_unit(); // Empty from_set still consumes the probability test.
 unsigned choices[content_count],count=0;
 for(unsigned id:daily_ball_order){const auto& info=ball_info(id);if(!info.can_drop || info.set==ItemSet::NEGATIVE || info.rarity!=rarity)continue;if(exclude_numbers && (info.tags&static_cast<uint32_t>(Tag::TAG_NUMBER)))continue;bool allowed=false,used=false;for(unsigned i=0;i<set_count;++i)allowed|=sets[i]==info.set;for(unsigned i=0;i<excluded_count;++i)used|=excluded[i]==id;if(allowed && !used)choices[count++]=id;}
 return count?choices[random.next()%count]:0;
}
bool any_candidate(ItemSet set,const unsigned* excluded,unsigned excluded_count){
 for(unsigned id:daily_ball_order){const auto& info=ball_info(id);if(!info.can_drop || info.set!=set || info.set==ItemSet::NEGATIVE)continue;bool used=false;for(unsigned i=0;i<excluded_count;++i)used|=id==excluded[i];if(!used)return true;}return false;
}
}
bool generate_shop_stock(Pcg32& random,const ShopStockRequest& request,std::array<OwnedBall,3>& out){
 if(!request.set_count || request.set_count>7)return false;
 Pcg32 preferred(random.next());unsigned selected[3]{};std::array<OwnedBall,3> result{};
 for(unsigned slot=0;slot<3;++slot){unsigned set_index=request.set_count>3?random.next()%request.set_count:preferred.weighted_set(request.set_count);ItemSet chosen=request.sets[set_index];
  if(!any_candidate(chosen,selected,slot))return false;
  for(unsigned attempt=0;attempt<4096 && !selected[slot];++attempt)selected[slot]=roll(random,request.level,&chosen,1,selected,slot);
  if(!selected[slot])return false;
 }
 for(unsigned slot=0;slot<3;++slot){OwnedBall item;item.id=static_cast<uint16_t>(selected[slot]);const auto& primary=ball_info(item.id);
  // Original checks literal NUMBER (not TAG_NUMBER), so numbered primaries can mix.
  if(request.cocktails && item.id!=id_of(BallKey::SCRAP) && item.id!=id_of(BallKey::UPGRADE_BALL)){
   double probability=std::max(0.0,std::min(25.0,(double(request.level)-(primary.rarity==0?4:9))*.5));
   if(primary.rarity<2 && random.bounded(101)<probability){unsigned excluded[]={item.id,id_of(BallKey::SCRAP),id_of(BallKey::UPGRADE_BALL)};item.mixed_with=static_cast<uint16_t>(roll(random,request.level,request.sets.data(),request.set_count,excluded,3,true,true));}
  }
  bool numbered=(primary.tags&static_cast<uint32_t>(Tag::TAG_NUMBER)) || (item.mixed_with && (ball_info(item.mixed_with).tags&static_cast<uint32_t>(Tag::TAG_NUMBER)));
  unsigned level=1;
  if(!numbered && primary.rarity<2){int offset=primary.rarity==0?3:6,offset3=primary.rarity==0?11:16;int probability3=(int(request.level)-offset3)*2,probability2=(int(request.level)-offset)*2;if(int(random.bounded(101))<probability3)level=3;else if(int(random.bounded(101))<probability2)level=2;}
  item.level=static_cast<uint8_t>(level);item.score=(primary.base_score+(item.mixed_with?ball_info(item.mixed_with).base_score:0))*(1<<(level-1));result[slot]=item;
 }
 out=result;return true;
}
bool generate_snack_stock(Pcg32& random,const ShopStockRequest& request,const std::array<uint8_t,4>& owned,std::array<uint8_t,3>& out){
 uint32_t sets=0;for(unsigned i=0;i<request.set_count;++i)sets|=1u<<static_cast<unsigned>(request.sets[i]);
 double progress=std::max(0.0,std::min(1.0,(double(request.level)-1)/10));
 double common=12-4*progress,uncommon=2+2*progress,rare=std::max(0.0,-2+4*progress),total=common+uncommon+rare;
 std::array<uint8_t,3> result{};
 for(unsigned slot=0;slot<3;++slot){double value=random.range_unit();unsigned tier=value<common/total?0:value<common/total+uncommon/total?1:2;unsigned choices[snack_count],count=0;
  for(const auto& rule:snack_stock_rules){const auto& info=snack_info(rule.id);if(!info.can_drop || info.rarity!=tier || (sets&rule.required_sets)!=rule.required_sets || (rule.extra && !request.extra_sets))continue;bool used=false;for(auto id:owned)used|=id==rule.id;for(unsigned j=0;j<slot;++j)used|=result[j]==rule.id;if(!used)choices[count++]=rule.id;}
  if(!count)return false;
  result[slot]=static_cast<uint8_t>(choices[random.next()%count]);
 }
 out=result;return true;
}

unsigned roll_filtered_content_data(Pcg32& random,const ShopStockRequest& request,const unsigned* excluded,unsigned excluded_count,uint32_t required_tags,int rarity_filter){
 random.global_unit(); // Empty preferred set still consumes the original probability test.
 unsigned choices[content_count],count=0;
 for(unsigned id:daily_ball_order){const auto& info=ball_info(id);if(!info.can_drop || info.set==ItemSet::NEGATIVE || (rarity_filter>=0 && info.rarity!=unsigned(rarity_filter)) || (required_tags && !(info.tags&required_tags)))continue;bool used=false,allowed=!request.set_count;for(unsigned i=0;i<excluded_count;++i)used|=excluded[i]==id;for(unsigned i=0;i<request.set_count;++i)allowed|=request.sets[i]==info.set;if(!used && allowed)choices[count++]=id;}
 return count?choices[random.next()%count]:0;
}

unsigned roll_seeded_content_data(Pcg32& random,const ShopStockRequest& request,const unsigned* excluded,unsigned excluded_count,uint32_t required_tags,bool planets){
 int tier=-1;
 if(!planets){double draw=random.range_unit(),t=std::max(0.0,std::min(1.0,(double(request.level)-1)/10));double weights[]={12-4*t,3-t,-.3+1.3*t,-.5+.75*t};double total=weights[0]+weights[1]+weights[2]+weights[3];tier=draw<=weights[0]/total?0:draw<=weights[0]/total+weights[1]/total?1:draw<=weights[0]/total+weights[1]/total+weights[2]/total?2:3;}
 random.global_unit();unsigned choices[content_count],count=0;
 for(unsigned id:daily_ball_order){const auto& info=ball_info(id);if(info.set==ItemSet::NEGATIVE || (!planets && !info.can_drop) || (tier>=0 && info.rarity!=unsigned(tier)) || (required_tags && !(info.tags&required_tags)))continue;bool used=false,allowed=planets || !request.set_count;for(unsigned i=0;i<excluded_count;++i)used|=excluded[i]==id;for(unsigned i=0;i<request.set_count;++i)allowed|=request.sets[i]==info.set;if(!used && allowed)choices[count++]=id;}
 return count?choices[random.next()%count]:0;
}

}