#pragma once
#include "ultrapool/core.hpp"
namespace up {
struct ShopStockRequest {
 unsigned level=0,set_count=0;
 std::array<ItemSet,7> sets{};
 bool cocktails=false,extra_sets=false;
};
// Transactional: an unavailable candidate pool leaves the caller's stock unchanged.
bool generate_shop_stock(Pcg32& random,const ShopStockRequest& request,std::array<OwnedBall,3>& out);
bool generate_snack_stock(Pcg32& random,const ShopStockRequest& request,const std::array<uint8_t,4>& owned,std::array<uint8_t,3>& out);
unsigned roll_filtered_content_data(Pcg32& random,const ShopStockRequest& request,const unsigned* excluded,unsigned excluded_count,uint32_t required_tags,int rarity_filter=-2);
unsigned roll_seeded_content_data(Pcg32& random,const ShopStockRequest& request,const unsigned* excluded,unsigned excluded_count,uint32_t required_tags,bool planets=false);
}