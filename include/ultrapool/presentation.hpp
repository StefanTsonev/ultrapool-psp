#pragma once
#include "ultrapool/render.hpp"
#include <memory>
namespace up {
struct AchievementText {char title[96]{},description[384]{};};
bool load_achievement_text(const char* filename,std::unique_ptr<AchievementText[]>& out);
struct PrivateTextBank {
    std::vector<std::array<unsigned,2>> offsets;
    std::vector<char> data;
    bool load(const char* filename,unsigned limit);
    const char* title(unsigned id)const;
    const char* text(unsigned id)const;
};
struct Skin {
    PrivateTextBank ball_text,snack_texts,dialogue_text;
    std::array<Texture,5> medals;
    Texture merchant_talk,cocktail_talk;
    std::unique_ptr<AchievementText[]> achievement_text,tutorial_text;
    std::array<Texture,content_count> balls;
    std::array<Texture,snack_count> snacks;
    std::array<Texture,6> pickups;
    std::array<Texture,8> flowers;
    std::array<Texture,4> levels;
    std::array<Texture,3> rarity_stars;
    std::array<Texture,7> tutorial_portraits;
    std::array<Texture,9> set_posters;
    // Original menu symbols converted from the owner's PC installation.
    // Order: achievements, collection, customise, stats, daily, settings, play.
    std::array<Texture,7> menu_icons;
    Texture menu_button;
    std::array<Texture,4> statuses;
    std::array<Texture,2> tickets;
    Texture tiger_cut_in,tiger_glow,pocket_skull;
    Texture table,floor,heart,font,shop_shelves,build_mat,wallet,merchant,logo,cocktail_merchant,snack_merchant,counter,merchant_blink,cocktail_blink;
    std::array<unsigned char,96> font_advance{};
    unsigned loaded=0;
    std::size_t resident=0;
    void load(const char* directory);
};
class Painter {
public:
    virtual ~Painter() = default;
    virtual void rect(float x,float y,float w,float h,uint32_t color)=0;
    virtual void line(float x,float y,float xx,float yy,uint32_t color)=0;
    virtual void circle(float x,float y,float radius,uint32_t color)=0;
    virtual void sprite(const Texture& t,float x,float y,float w,float h,uint32_t tint=0xffffffff)=0;
    // Source coordinates are normalized texture fractions; destination is pixel geometry.
    virtual void sprite_region(const Texture& t,float x,float y,float w,float h,float u0,float v0,float u1,float v1,uint32_t tint=0xffffffff)=0;
    virtual void sprite_rotated(const Texture& t,float cx,float cy,float w,float h,float angle,uint32_t tint=0xffffffff){(void)angle;sprite(t,cx-w/2,cy-h/2,w,h,tint);}
    virtual void sprite_region_rotated(const Texture& t,float x,float y,float w,float h,float cx,float cy,float u0,float v0,float u1,float v1,float angle,uint32_t tint=0xffffffff){(void)cx;(void)cy;(void)angle;sprite_region(t,x,y,w,h,u0,v0,u1,v1,tint);}
    virtual void text(int x,int y,uint32_t color,const char* text)=0;
};
void draw_outlined_ui_text(Painter& painter,const Skin& skin,float x,float y,float height,uint32_t foreground,uint32_t outline,const char* text);
float ui_text_width(const Skin& skin,float height,const char* text);
void draw_ui_text(Painter& painter,const Skin& skin,float x,float y,float height,uint32_t color,const char* text);
struct PortraitPlacement {float x,y,width,height,v_bottom;};
PortraitPlacement shop_portrait_layout(float original_aspect);
Screen shop_slot_position(unsigned index);
unsigned shop_vertical_slot(unsigned index,bool down);
// Offers 0..2, owned slots 3..17, Roll 18, Play 19, 20 unused, passives 21..24.
unsigned snack_neighbor(unsigned node,int dx,int dy);
unsigned shop_neighbor(unsigned node,int dx,int dy,bool inventory_only=false);
void paint_scene(Painter& p,const Skin& skin,const Game& game,unsigned physics_us,unsigned free_bytes,unsigned draw_us=0);
}
