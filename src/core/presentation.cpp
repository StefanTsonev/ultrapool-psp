#include "ultrapool/presentation.hpp"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
namespace up {
static constexpr uint32_t white = 0xffe6eaf5, gold = 0xff72c5ff,
                          muted = 0xffa7a5b5, green = 0xff8fd8a2,
                          red = 0xff8892f2, ink = 0xff221e2b;
void Skin::load(const char *directory) {
  loaded = 0;
  resident = 0;
  char prose_path[256];
  std::snprintf(prose_path,sizeof(prose_path),"%s/ball_text.uat",directory);ball_text.load(prose_path,content_count);
  std::snprintf(prose_path,sizeof(prose_path),"%s/snack_text.uat",directory);snack_texts.load(prose_path,snack_count);
  std::snprintf(prose_path,sizeof(prose_path),"%s/dialogue_text.uat",directory);dialogue_text.load(prose_path,64);
  char path[256];
  std::snprintf(path,sizeof(path),"%s/achievement_text.uat",directory);
  achievement_text.reset();load_achievement_text(path,achievement_text);
  std::snprintf(path,sizeof(path),"%s/tutorial_text.uat",directory);tutorial_text.reset();load_achievement_text(path,tutorial_text);
  for (unsigned i = 0; i < balls.size(); ++i) {
    std::snprintf(path, sizeof(path), "%s/ball_%u.upt", directory, i);
    if (load_texture(path, balls[i])) {
      ++loaded;
      resident += balls[i].bytes();
    }
  }
  for (unsigned i = 1; i < snacks.size(); ++i) {
    std::snprintf(path, sizeof(path), "%s/snack_%u.upt", directory, i);
    if (load_texture(path, snacks[i])) {
      ++loaded;
      resident += snacks[i].bytes();
    }
  }
  for (unsigned i = 0; i < pickups.size(); ++i) {
    std::snprintf(path, sizeof(path), "%s/pickup_%u.upt", directory, i);
    if (load_texture(path, pickups[i])) {
      ++loaded;
      resident += pickups[i].bytes();
    }
  }
  for (unsigned i = 0; i < flowers.size(); ++i) {
    std::snprintf(path, sizeof(path), "%s/flower_%u.upt", directory, i);
    if (load_texture(path, flowers[i])) {
      ++loaded;
      resident += flowers[i].bytes();
    }
  }
  for (unsigned i = 0; i < levels.size(); ++i) {
    std::snprintf(path, sizeof(path), "%s/level_%u.upt", directory, i + 2);
    if (load_texture(path, levels[i])) {
      ++loaded;
      resident += levels[i].bytes();
    }
  }
  for(unsigned i=0;i<rarity_stars.size();++i){
    std::snprintf(path,sizeof(path),"%s/rarity_%u.upt",directory,i+1);
    if(load_texture(path,rarity_stars[i])){++loaded;resident+=rarity_stars[i].bytes();}
  }
  for(unsigned i=0;i<tutorial_portraits.size();++i){std::snprintf(path,sizeof(path),"%s/tutorial_portrait_%u.upt",directory,i);if(load_texture(path,tutorial_portraits[i])){++loaded;resident+=tutorial_portraits[i].bytes();}}
  for(unsigned i=0;i<set_posters.size();++i){std::snprintf(path,sizeof(path),"%s/set_poster_%u.upt",directory,i);if(load_texture(path,set_posters[i])){++loaded;resident+=set_posters[i].bytes();}}
  const char *menu_icon_names[]={"menu_achievements","menu_collection","menu_customise","menu_stats","menu_daily","menu_settings","menu_play"};
  for(unsigned i=0;i<menu_icons.size();++i){std::snprintf(path,sizeof(path),"%s/%s.upt",directory,menu_icon_names[i]);if(load_texture(path,menu_icons[i])){++loaded;resident+=menu_icons[i].bytes();}}
  std::snprintf(path,sizeof(path),"%s/menu_button.upt",directory);if(load_texture(path,menu_button)){++loaded;resident+=menu_button.bytes();}
  for (unsigned i = 0; i < statuses.size(); ++i) {
    std::snprintf(path, sizeof(path), "%s/status_%u.upt", directory, i);
    if (load_texture(path, statuses[i])) {
      ++loaded;
      resident += statuses[i].bytes();
    }
  }
  for (unsigned i = 0; i < tickets.size(); ++i) {
    std::snprintf(path, sizeof(path), "%s/ticket_%u.upt", directory, i);
    if (load_texture(path, tickets[i])) {
      ++loaded;
      resident += tickets[i].bytes();
    }
  }
  for(unsigned i=0;i<medals.size();++i){std::snprintf(path,sizeof(path),"%s/medal_%u.upt",directory,i);if(load_texture(path,medals[i])){++loaded;resident+=medals[i].bytes();}}
  std::snprintf(path,sizeof(path),"%s/shop_merchant_talk.upt",directory);if(load_texture(path,merchant_talk)){++loaded;resident+=merchant_talk.bytes();}
  std::snprintf(path,sizeof(path),"%s/cocktail_merchant_talk.upt",directory);if(load_texture(path,cocktail_talk)){++loaded;resident+=cocktail_talk.bytes();}
  const char *names[] = {"table",
                         "shop_floor",
                         "heart",
                         "font_jua",
                         "shop_shelves",
                         "build_mat",
                         "wallet",
                         "shop_merchant",
                         "menu_logo",
                         "cocktail_merchant",
                         "snack_merchant",
                         "shop_counter",
                         "shop_merchant_blink",
                         "cocktail_merchant_blink", "tiger_cut_in", "tiger_glow", "pocket_skull"};
  Texture *textures[] = {&table,          &floor,
                         &heart,          &font,
                         &shop_shelves,   &build_mat,
                         &wallet,         &merchant,
                         &logo,           &cocktail_merchant,
                         &snack_merchant, &counter,
                         &merchant_blink, &cocktail_blink, &tiger_cut_in, &tiger_glow, &pocket_skull};
  for (unsigned i = 0; i < sizeof(names)/sizeof(names[0]); ++i) {
    std::snprintf(path, sizeof(path), "%s/%s.upt", directory, names[i]);
    if (load_texture(path, *textures[i])) {
      ++loaded;
      resident += textures[i]->bytes();
    }
  }
  if (font.pixels && font.width == 256 && font.height == 128)
    for (unsigned i = 0; i < 96; ++i)
      font_advance[i] =
          static_cast<unsigned char>(font.pixels[127 * 256 + i] & 15);
}
static void text(Painter &p, int x, int y, uint32_t color, const char *format,
                 ...) {
  char value[256];
  va_list args;
  va_start(args, format);
  std::vsnprintf(value, sizeof(value), format, args);
  va_end(args);
  p.text(x, y, color, value);
}
namespace {
// Tokens keep prompts ASCII in source and independent of the asset font.
struct ButtonToken {const char* token;unsigned shape;};
static const ButtonToken button_tokens[]={{"{cross}",0},{"{circle}",1},{"{square}",2},{"{triangle}",3},{"{L}",4},{"{R}",5},{"{start}",6},{"{select}",7},{"{dpad}",8},{"{lr}",9},{"{ud}",10}};
static unsigned button_token(const char* text,unsigned& length){
  if(*text!='{'){length=0;return 99;}
  for(const auto& token:button_tokens){unsigned n=unsigned(std::strlen(token.token));if(!std::strncmp(text,token.token,n)){length=n;return token.shape;}}
  length=0;return 99;
}
static float button_width(unsigned shape,float h){return h*(shape==6?2.7f:shape==7?3.0f:1.1f);}
static void button_icon(Painter& p,const Skin& skin,float x,float y,float h,uint32_t color,unsigned shape){
  float a=x+h*.13f,b=x+h*.83f,t=y+h*.16f,d=y+h*.84f,c=x+h*.48f,m=y+h*.5f;
  if(shape==0){p.line(a,t,b,d,color);p.line(a,d,b,t,color);}
  else if(shape==1){for(unsigned i=0;i<20;++i){float u=i*6.2831853f/20,v=(i+1)*6.2831853f/20;p.line(c+std::cos(u)*h*.36f,m+std::sin(u)*h*.36f,c+std::cos(v)*h*.36f,m+std::sin(v)*h*.36f,color);}}
  else if(shape==2){p.line(a,t,b,t,color);p.line(b,t,b,d,color);p.line(b,d,a,d,color);p.line(a,d,a,t,color);}
  else if(shape==3){p.line(c,t,b,d,color);p.line(b,d,a,d,color);p.line(a,d,c,t,color);}
  else if(shape>=8){
    if(shape!=10){p.line(a,m,b,m,color);p.line(a,m,a+h*.2f,m-h*.2f,color);p.line(a,m,a+h*.2f,m+h*.2f,color);p.line(b,m,b-h*.2f,m-h*.2f,color);p.line(b,m,b-h*.2f,m+h*.2f,color);}
    if(shape!=9){p.line(c,t,c,d,color);p.line(c,t,c-h*.2f,t+h*.2f,color);p.line(c,t,c+h*.2f,t+h*.2f,color);p.line(c,d,c-h*.2f,d-h*.2f,color);p.line(c,d,c+h*.2f,d-h*.2f,color);}
  }else{
    const char* name=shape==4?"L":shape==5?"R":shape==6?"START":"SELECT";
    float w=button_width(shape,h)-h*.13f;
    p.line(x,y+h*.08f,x+w,y+h*.08f,color);p.line(x+w,y+h*.08f,x+w,y+h*.96f,color);p.line(x+w,y+h*.96f,x,y+h*.96f,color);p.line(x,y+h*.96f,x,y+h*.08f,color);
    float th=h*.67f,tw=ui_text_width(skin,th,name);if(tw>w-2){th*=std::max(0.1f,(w-2)/tw);tw=ui_text_width(skin,th,name);}draw_ui_text(p,skin,x+(w-tw)/2,y+(h-th)/2,th,color,name);
  }
}
}
void draw_ui_text(Painter &p,const Skin &skin,float x,float y,float height,uint32_t color,const char* value){
  if(!skin.font.pixels && !std::strchr(value,'{')){p.text(int(x),int(y),color,value);return;}
  const float scale=height/20;
  for(const char* c=value;*c;){
    unsigned n=0,shape=button_token(c,n);
    if(n){button_icon(p,skin,x,y,height,color,shape);x+=button_width(shape,height);c+=n;continue;}
    unsigned char ch=static_cast<unsigned char>(*c++);unsigned slot=ch>=32 && ch<128?ch-32:95;
    if(!skin.font.pixels){const char* first=c-1;unsigned count=1;while(first[count] && count<255){unsigned skip=0;button_token(first+count,skip);if(skip)break;++count;}char plain[256];std::memcpy(plain,first,count);plain[count]=0;p.text(int(x),int(y),color,plain);x+=6*count;c=first+count;continue;}
    if(ch!=' ')p.sprite_region(skin.font,x,y,16*scale,20*scale,(float(slot%16)*16+.5f)/256,(float((slot/16)*20)+.5f)/128,(float(slot%16+1)*16-.5f)/256,(float((slot/16+1)*20)-.5f)/128,color);
    x+=(skin.font_advance[slot]?skin.font_advance[slot]:10)*scale;
  }
}
static void ui_text(Painter &p, const Skin &skin, float x, float y,
                    float height, uint32_t color, const char *format, ...) {
  char value[256];
  va_list args;
  va_start(args, format);
  std::vsnprintf(value, sizeof(value), format, args);
  va_end(args);
  draw_ui_text(p, skin, x, y, height, color, value);
}
static void diamond(Painter &p, float x, float y, int radius, uint32_t color,
                    float fraction = 1) {
  int top = radius - int(2 * radius * std::max(0.0f, std::min(1.0f, fraction)));
  for (int row = top; row <= radius; ++row) {
    int half = radius - std::abs(row);
    p.rect(x - half, y + row, float(half * 2 + 1), 1, color);
  }
}
static void dotted_slot(Painter &p, float x, float y, float radius,
                        uint32_t color) {
  for (int i = 0; i < 12; ++i) {
    float a = i * 6.2831853f / 12, b = a + .25f;
    p.line(x + std::cos(a) * radius, y + std::sin(a) * radius,
           x + std::cos(b) * radius, y + std::sin(b) * radius, color);
  }
}
Screen shop_slot_position(unsigned i) {
  static const Screen positions[] = {
      {343, 180}, {328, 154}, {358, 154}, {313, 128}, {343, 128},
      {373, 128}, {298, 102}, {328, 102}, {358, 102}, {388, 102},
      {406, 170}, {430, 170}, {454, 170}, {406, 196}, {430, 196}};
  return positions[i < 15 ? i : 0];
}
unsigned shop_vertical_slot(unsigned index, bool down) {
  if (index >= 15)
    return 0;
  Screen from = shop_slot_position(index);
  unsigned best = index;
  float distance = 1e9f;
  for (unsigned i = 0; i < 15; ++i) {
    if ((index < 10) != (i < 10))
      continue;
    Screen to = shop_slot_position(i);
    float dy = to.y - from.y;
    if ((down ? dy > 1 : dy < -1)) {
      float dx = to.x - from.x;
      float cost = dy * dy + dx * dx * 2;
      if (cost < distance) {
        distance = cost;
        best = i;
      }
    }
  }
  return best;
}
static Screen shop_node_position(unsigned node) {
  if (node >= 21)
    return {424 + 25.0f * ((node - 21) % 2), 91 + 26.0f * ((node - 21) / 2)};
  if (node < 3)
    return {36 + 70.0f * node, 142};
  if (node < 18)
    return shop_slot_position(node - 3);
  if (node == 18)
    return {108, 187};
  if (node == 19)
    return {346, 223};
  return {32, 187};
}
static void menu_nine_patch(Painter& p,const Skin& skin,float x,float y,float w,float h,uint32_t tint){
  if(!skin.menu_button.pixels){p.rect(x+1,y+2,w,h,ink);p.rect(x,y,w,h,tint);return;}
  const float cut=.125f,bx=std::min(8.0f,w*cut),by=std::min(8.0f,h*cut);
  const float xs[]={x,x+bx,x+w-bx,x+w},ys[]={y,y+by,y+h-by,y+h};
  const float us[]={0,cut,1-cut,1},vs[]={0,cut,1-cut,1};
  for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)
    p.sprite_region(skin.menu_button,xs[col],ys[row],xs[col+1]-xs[col],ys[row+1]-ys[row],us[col],vs[row],us[col+1],vs[row+1],tint);
}
unsigned shop_neighbor(unsigned node, int dx, int dy, bool inventory_only) {
  if (!inventory_only && dy == 0) {
    if (dx > 0 && node == 20)
      return 18;
    if (dx > 0 && node == 18)
      return 19;
    if (dx < 0 && node == 19)
      return 18;
    if (dx < 0 && node == 18)
      return 18;
  }
  Screen from = shop_node_position(node);
  unsigned best = node;
  float cost = 1e12f;
  for (unsigned i = 0; i < 25; ++i) {
    if (i == 20)
      continue;
    if (inventory_only && (i < 3 || i >= 18))
      continue;
    Screen to = shop_node_position(i);
    float x = to.x - from.x, y = to.y - from.y;
    float forward = x * dx + y * dy, side = x * dy - y * dx;
    if (forward <= 1 || forward < std::abs(side) * .5f)
      continue;
    float candidate = forward * forward + side * side * 4;
    if (candidate < cost) {
      cost = candidate;
      best = i;
    }
  }
  return best;
}
unsigned snack_neighbor(unsigned node, int dx, int dy) {
  static const Screen pos[] = {{36, 165}, {106, 165}, {176, 165}, {272, 91},
                               {412, 91}, {272, 153}, {412, 153}};
  if (node >= 7)
    node = 0;
  unsigned best = node;
  float cost = 1e12f;
  for (unsigned i = 0; i < 7; ++i) {
    float x = pos[i].x - pos[node].x, y = pos[i].y - pos[node].y,
          forward = x * dx + y * dy, side = x * dy - y * dx;
    if (forward <= 1 || forward < std::abs(side) * .5f)
      continue;
    float c = forward * forward + side * side * 4;
    if (c < cost) {
      cost = c;
      best = i;
    }
  }
  return best;
}
static void selection_ring(Painter &p, float x, float y, float radius,
                           uint32_t color) {
  for (unsigned i = 0; i < 32; ++i) {
    float a = i * 6.2831853f / 32, b = (i + 1) * 6.2831853f / 32;
    p.line(x + std::cos(a) * radius, y + std::sin(a) * radius,
           x + std::cos(b) * radius, y + std::sin(b) * radius, color);
  }
}
static void selection_outline(Painter &p, float x, float y, float radius,
                              uint32_t color, float time) {
  const float a = .18f + time * 1.0471975512f, c = std::cos(a), s = std::sin(a);
  float px[4], py[4];
  for (unsigned i = 0; i < 4; ++i) {
    float xx = (i == 0 || i == 3 ? -radius : radius),
          yy = (i < 2 ? -radius : radius);
    px[i] = x + xx * c - yy * s;
    py[i] = y + xx * s + yy * c;
  }
  for (unsigned i = 0; i < 4; ++i)
    p.line(px[i], py[i], px[(i + 1) % 4], py[(i + 1) % 4], color);
}
float ui_text_width(const Skin& skin,float height,const char* value){
  float width=0;
  for(const char* c=value;*c;){unsigned n=0,shape=button_token(c,n);if(n){width+=button_width(shape,height);c+=n;continue;}unsigned char ch=static_cast<unsigned char>(*c++);unsigned slot=ch>=32 && ch<128?ch-32:95;width+=skin.font.pixels?(skin.font_advance[slot]?skin.font_advance[slot]:10)*height/20:6;}
  return width;
}
static void centered(Painter &p, const Skin &skin, float cx, float y,
                     float height, uint32_t color, const char *format, ...) {
  char value[256];
  va_list args;
  va_start(args, format);
  std::vsnprintf(value, sizeof(value), format, args);
  va_end(args);
  float width = ui_text_width(skin, height, value);
  draw_ui_text(p, skin, cx - width * .5f, y, height, color, value);
}
static void frame(Painter &p, float x, float y, float w, float h,
                  uint32_t border, uint32_t fill) {
  p.rect(x, y, w, h, border);
  p.rect(x + 1, y + 1, w - 2, h - 2, fill);
}
void draw_outlined_ui_text(Painter &p, const Skin &skin, float x, float y,
                           float height, uint32_t foreground, uint32_t outline,
                           const char *value) {
  // A one-pixel four-direction stroke reuses the existing atlas; no texture
  // allocation.
  draw_ui_text(p, skin, x - 1, y, height, outline, value);
  draw_ui_text(p, skin, x + 1, y, height, outline, value);
  draw_ui_text(p, skin, x, y - 1, height, outline, value);
  draw_ui_text(p, skin, x, y + 1, height, outline, value);
  draw_ui_text(p, skin, x, y, height, foreground, value);
}
static void label(Painter &p, const Skin &skin, float cx, float y, float height,
                  uint32_t foreground, uint32_t stroke, const char *format,
                  ...) {
  char value[256];
  va_list args;
  va_start(args, format);
  std::vsnprintf(value, sizeof(value), format, args);
  va_end(args);
  draw_outlined_ui_text(p, skin, cx - ui_text_width(skin, height, value) * .5f,
                        y, height, foreground, stroke, value);
}
static void compact(char *out, std::size_t size, int64_t value) {
  double n = std::fabs(double(value));
  const char *suffix = "";
  double divisor = 1;
  if (n >= 1000000000) {
    suffix = "B";
    divisor = 1000000000;
  } else if (n >= 1000000) {
    suffix = "M";
    divisor = 1000000;
  } else if (n >= 10000) {
    suffix = "K";
    divisor = 1000;
  }
  if (divisor == 1)
    std::snprintf(out, size, "%lld", static_cast<long long>(value));
  else
    std::snprintf(out, size, "%.1f%s", value / divisor, suffix);
}
static void small_number(char *out, std::size_t size, int value) {
  if (std::abs(double(value)) >= 1000000)
    std::snprintf(out, size, "%.0fM", value / 1000000.0);
  else if (std::abs(double(value)) >= 1000)
    std::snprintf(out, size, "%.0fK", value / 1000.0);
  else
    std::snprintf(out, size, "%d", value);
}
static void fit(Painter &p, const Skin &skin, float cx, float y, float width,
                float height, uint32_t color, const char *format, ...) {
  char value[192];
  va_list args;
  va_start(args, format);
  std::vsnprintf(value, sizeof(value), format, args);
  va_end(args);
  float measured = ui_text_width(skin, height, value);
  if (measured > width)
    height *= width / measured;
  centered(p, skin, cx, y, height, color, "%s", value);
}
static void paragraph(Painter &p, const Skin &skin, float x, float y,
                      float width, unsigned rows, const char *value,
                      uint32_t color = white, float height = 14,
                      unsigned skip = 0, bool stroke = false, bool align_center = false) {
  const char *c = value;
  unsigned line = 0, drawn = 0;
  while (*c && drawn < rows) {
    char part[192];
    unsigned count = 0, last_space = 0;
    while (c[count] && count < 190) {
      part[count] = c[count];
      part[count + 1] = 0;
      if (ui_text_width(skin, height, part) > width && count)
        break;
      if (c[count] == ' ')
        last_space = count;
      ++count;
    }
    if (c[count] && last_space)
      count = last_space;
    if (!count)
      count = 1;
    std::memcpy(part, c, count);
    part[count] = 0;
    if (line++ >= skip) {
      float line_x=align_center?x+(width-ui_text_width(skin,height,part))*.5f:x;
      if (stroke)
        draw_outlined_ui_text(p, skin, line_x, y + drawn * (height + 1), height,
                              color, ink, part);
      else
        draw_ui_text(p, skin, line_x, y + drawn * (height + 1), height, color, part);
      ++drawn;
    }
    c += count;
    while (*c == ' ')
      ++c;
  }
}
static void action(Painter &p, const Skin &skin, float cx, float y, float width,
                   bool selected, const char *value, bool enabled = true) {
  char line[256];
  std::snprintf(line, sizeof(line), selected ? "> %s <" : "%s", value);
  float height = 17, measure = ui_text_width(skin, height, line);
  if (measure > width)
    height *= width / measure;
  label(p, skin, cx, y, height,
        !enabled   ? muted
        : selected ? gold
                   : green,
        ink, "%s", line);
  if (selected)
    p.line(cx - width * .35f, y + height + 2, cx + width * .35f, y + height + 2,
           gold);
}
static void footer(Painter &p, const Skin &skin, const char *top,
                   const char *bottom = nullptr) {
  p.rect(0, 237, 480, 35, ink);
  fit(p, skin, 240, 238, 462, 14, white, "%s", top);
  if (bottom && *bottom)
    fit(p, skin, 240, 254, 462, 13, muted, "%s", bottom);
}
static void track(Painter &p, const Skin &skin, int round, float x, float y,
                  float step, const Game *game = nullptr) {
  for (int i = 0; i < 10; ++i) {
    float px = x + float(i) * step;
    if (i < 9)
      p.line(px + 3, y, px + step - 3, y, 0xffa6aaa7);
    p.circle(px, y, i == std::min(round - 1, 9) ? 4 : 2.5f,
             i < round ? 0xff57994e : 0xffa6aaa7);
    if (game) {
      bool snack = (game->difficulty || game->trial) && ((i + 1) % 2 == 1);
      bool mix = game->difficulty >= 2 &&
                 ((i + 1) == 3 || (i + 1) == 6 || (i + 1) == 9);
      if (snack && skin.tickets[0].pixels)
        p.sprite(skin.tickets[0], px - (mix ? 9 : 4), y - 5, 8, 10, 0xff444444);
      if (mix && skin.tickets[1].pixels)
        p.sprite(skin.tickets[1], px + (snack ? 1 : -4), y - 5, 8, 10,
                 0xff444444);
    }
  }
  (void)skin;
}
static void ball_icon(Painter &p, const Skin &skin, unsigned id, float x,
                      float y, float size, uint32_t tint = 0xffffffff) {
  p.circle(x + size * 0.5f + 1, y + size * 0.5f + 2, size * 0.49f, 0x77000000);
  if (id < skin.balls.size() && skin.balls[id].pixels)
    p.sprite(skin.balls[id], x, y, size, size, tint);
  else {
    p.circle(x + size * 0.5f, y + size * 0.5f, size * 0.48f, builtin(id).color);
    p.circle(x + size * 0.34f, y + size * 0.3f, size * 0.1f, 0x66ffffff);
  }
}
static void mixed_icon(Painter &p, const Skin &skin, unsigned id,
                       unsigned second, float x, float y, float size,
                       uint32_t tint = 0xffffffff,float rotation=0) {
  if (!second || id >= skin.balls.size() || second >= skin.balls.size() ||
      !skin.balls[id].pixels || !skin.balls[second].pixels) {
    if(rotation!=0 && id<skin.balls.size() && skin.balls[id].pixels)
      p.sprite_rotated(skin.balls[id],x+size*.5f,y+size*.5f,size,size,rotation,tint);
    else ball_icon(p, skin, id, x, y, size, tint);
    if (second)
      p.line(x + size * 0.5f, y, x + size * 0.5f, y + size, gold);
    return;
  }
  p.circle(x + size * 0.5f + 1, y + size * 0.5f + 2, size * 0.49f, 0x77000000);
  if(rotation==0) {
    p.sprite_region(skin.balls[id], x, y, size * .5f, size, 0, 0, .5f, 1, tint);
    p.sprite_region(skin.balls[second], x + size * .5f, y, size * .5f, size, .5f,
                    0, 1, 1, tint);
  } else {
    p.sprite_region_rotated(skin.balls[id],x,y,size*.5f,size,x+size*.5f,y+size*.5f,0,0,.5f,1,rotation,tint);
    p.sprite_region_rotated(skin.balls[second],x+size*.5f,y,size*.5f,size,x+size*.5f,y+size*.5f,.5f,0,1,1,rotation,tint);
  }
}
static void item_name(char *buffer, std::size_t capacity,
                      const OwnedBall &item) {
  if (item.mixed_with)
    std::snprintf(buffer, capacity, "%s + %s", ball_name(item.id),
                  ball_name(item.mixed_with));
  else
    std::snprintf(buffer, capacity, "%s",
                  item.id ? ball_name(item.id) : "Empty");
}
static void item_description(char *out, std::size_t size,
                             const OwnedBall &item) {
  char first[256], second[256];
  describe_ability(builtin(item.id).ability, item.level, first, sizeof(first));
  if (!out || !size)
    return;
  std::size_t used = 0;
  out[0] = 0;
  auto append = [&](const char *value) {
    while (*value && used + 1 < size)
      out[used++] = *value++;
    out[used] = 0;
  };
  if (item.mixed_with) {
    describe_ability(builtin(item.mixed_with).ability, item.level, second,
                     sizeof(second));
    append(ball_name(item.id));
    append(": ");
    append(first);
    append(" / ");
    append(ball_name(item.mixed_with));
    append(": ");
    append(second);
  } else
    append(first);
}
static void floor(Painter &p, const Skin &skin) {
  p.rect(0, 0, 480, 272, ink);
  if (skin.floor.pixels)
    for (int y = 0; y < 272; y += 128)
      for (int x = 0; x < 480; x += 128)
        p.sprite(skin.floor, float(x), float(y), 128, 128, 0xff555555);
}
static void coin(Painter &p, float x, float y) {
  p.circle(x, y, 5, 0xff35618e);
  p.circle(x, y, 3.5f, gold);
  p.line(x, y - 2, x, y + 2, 0xff357493);
}
static void header(Painter &p, const Skin &skin, const Game &g,
                   const char *title) {
  p.rect(0, 0, 480, 30, ink);
  ui_text(p, skin, 10, 4, 16, white, "%s", title);
  char cash[24];
  compact(cash, sizeof(cash), g.money);
  coin(p, 358, 12);
  fit(p, skin, 391, 4, 54, 15, gold, "%s$", cash);
  for (int i = 0; i < 3; ++i) {
    float x = 430 + float(i) * 15;
    if (i < g.hp && skin.heart.pixels)
      p.sprite(skin.heart, x, 6, 12, 12);
    else
      p.circle(x + 6, 12, 3, i < g.hp ? red : 0xff49414d);
  }
  if(g.endless || g.daily_date)fit(p,skin,273,5,145,14,muted,"Round %d",g.round);
  else fit(p, skin, 273, 5, 145, 14, muted, "Round %d / %u", g.round, rounds_for_difficulty(g.difficulty));
}
static void snack_icon(Painter &p, const Skin &skin, unsigned id, float x,
                       float y, float size) {
  if (id < skin.snacks.size() && skin.snacks[id].pixels)
    p.sprite(skin.snacks[id], x, y, size, size);
  else {
    p.circle(x + size / 2, y + size / 2, size * 0.4f, gold);
    p.circle(x + size / 2 - 3, y + size / 2 - 3, 2, ink);
  }
}
PortraitPlacement shop_portrait_layout(float original_aspect) {
  const float full_height = 150, top = 32, counter_top = 149;
  return {108 - full_height * original_aspect * .5f, top,
          full_height * original_aspect, counter_top - top,
          (counter_top - top) / full_height};
}
static void bar_background(Painter &p, const Skin &skin, const Game& g,
                           const Texture &character) {
  floor(p, skin);
  if (skin.shop_shelves.pixels)
    p.sprite(skin.shop_shelves, 0, 30, 216, 210, 0xff8099b5);
  if (character.pixels) {
    // Ratios measured from the original trimmed, aligned source canvases.
    float aspect =
        (&character == &skin.merchant || &character == &skin.merchant_blink || &character == &skin.merchant_talk)
            ? 485.0f / 1126
        : (&character == &skin.cocktail_merchant ||
           &character == &skin.cocktail_blink || &character == &skin.cocktail_talk)
            ? 741.0f / 989
            : 493.0f / 838;
    PortraitPlacement place = shop_portrait_layout(aspect);
    float bob=g.reduced_motion?0:std::sin(g.ui_rotation*(g.npc_time>0?6:3))*.6f;
    p.sprite_region(character,place.x,place.y+bob,place.width,place.height-bob,0,0,1,place.v_bottom);
  }
  for (int i = 0; i < 3; ++i)
    if (skin.counter.pixels)
      p.sprite(skin.counter, float(i * 72), 149, 72, 88);
  if (skin.build_mat.pixels)
    p.sprite(skin.build_mat, 218, 34, 254, 201);
  else
    p.rect(218, 34, 254, 201, 0xffe9e9e9);
}
static const char *rarity_name(unsigned rarity) {
  static const char *names[] = {"Common", "Uncommon", "Rare", "Legendary"};
  return names[std::min(rarity, 3u)];
}
// Colors from the original EffectManager rarity palette (ABGR).
static uint32_t rarity_color(unsigned tier) {
  static const uint32_t colors[]={0xff919191,0xffeda464,0xff6f2fd1,0xff0a46c7,0xff47a578};
  return colors[std::min(tier,4u)];
}
static void rarity_star(Painter& p,const Skin& skin,const Game& g,float x,float y,float size,unsigned tier){
 if(!tier)return;
 const auto& texture=skin.rarity_stars[tier==4?1:std::min(tier-1,2u)];if(!texture.pixels)return;
 uint32_t base=rarity_color(tier);float flash=.5f+std::fabs(std::sin(g.ui_rotation*2.8f))*.25f;
 uint32_t color=0;for(unsigned channel=0;channel<3;++channel){unsigned c=(base>>(8*channel))&255;color|=uint32_t(c+(255-c)*flash)<<(8*channel);}
 color|=uint32_t(tier==1?128:tier==2 || tier==4?191:255)<<24;
 float scale=.805f+std::sin(g.ui_rotation*5)*.04f;
 p.sprite_rotated(texture,x,y,size*scale,size*scale,g.ui_rotation,color);
}
static void rarity_caption(Painter& p,const Skin& skin,float x,float y,unsigned tier) {
  centered(p,skin,x,y,10,rarity_color(tier),tier==4?"Mixed":rarity_name(tier));
}
static void snack_shop(Painter &p, const Skin &skin, const Game &g) {
  bar_background(p, skin, g, skin.snack_merchant);
  header(p, skin, g, "THE SNACK BAR");
  fit(p, skin, 30, 49, 50, 13, white, "Tickets");
  label(p, skin, 30, 66, 17, gold, ink, "%u", g.snack_tickets);
  for (unsigned i = 0; i < 3; ++i) {
    float x = 36 + 70.0f * i;
    unsigned id = g.snack_offers[i];
    bool selected = g.shop_focus == ShopFocus::Market && g.selection == i;
    if(id)rarity_star(p,skin,g,x,165,49,snack_rarity(id));
    if (selected)
      selection_outline(p, x, 165, 12, white, g.ui_time);
    snack_icon(p, skin, id, x - 15, 150, 30);
    if(id)rarity_caption(p,skin,x,134,snack_rarity(id));
  }
  unsigned id = g.shop_focus == ShopFocus::Market
                    ? g.snack_offers[g.selection]
                    : g.snacks[g.inventory_selection];
  fit(p, skin, 108, 190, 200, 15, id?rarity_color(snack_rarity(id)):gold, "%s", snack_name(id));
  label(p, skin, 108, 211, 15, g.snack_tickets ? green : muted, ink, "%s",
        g.snack_tickets ? "{cross} choose" : "No tickets");
  centered(p, skin, 345, 44, 17, 0xff282828, "YOUR SNACKS");
  for (unsigned i = 0; i < 4; ++i) {
    float x = 272 + (i % 2) * 140.0f, y = 91 + (i / 2) * 62.0f;
    bool selected =
        g.shop_focus == ShopFocus::Inventory && g.inventory_selection == i;
    if (selected)
      selection_outline(p, x, y, 12, 0xff906b39, g.ui_time);
    if (g.snacks[i])
      snack_icon(p, skin, g.snacks[i], x - 14, y - 14, 28);
    else
      centered(p, skin, x, y - 7, 14, 0xff777777, "Empty");
    fit(p, skin, x, y + 19, 122, 13, 0xff282828, "%s",
        g.snacks[i] ? snack_name(g.snacks[i]) : "");
  }
  if (g.sell_pending) {
    char price[24];
    compact(price, sizeof(price), snack_sell_price(id));
    fit(p, skin, 345, 205, 236, 15, 0xff3333a0, "Sell %s for %s$?",
        snack_name(id), price);
  } else
    paragraph(p, skin, 230, 195, 230, 2, snack_text(id), 0xff333333, 13);
  footer(p, skin,
         g.sell_pending ? "{cross} confirm sale   {circle} cancel"
         : g.shop_focus == ShopFocus::Inventory
             ? "{dpad} select   {cross} details   {triangle} sell"
             : "{dpad} select   {cross} choose",
         g.message_time > 0 ? g.message : "{L} / {R} change bar   {circle} back");
}
static void owned_badges(Painter &p, const Skin &skin, const OwnedBall &ball,
                         float x, float y) {
  char value[24];
  small_number(value, sizeof(value), ball.score);
  const float width = std::max(10.0f, ui_text_width(skin, 10, value) + 3);
  p.circle(x + 8, y + 7, width * .5f, 0xff181818);
  label(p, skin, x + 8, y + 2, 10, white, 0xff181818, "%s", value);
  if (ball.level > 1 && ball.level <= 5) {
    const Texture &spark = skin.levels[ball.level - 2];
    if (spark.pixels)
      p.sprite(spark, x - 15, y - 15, 12, 12);
    else
      label(p, skin, x - 10, y - 17, 9, gold, ink, "L%u", unsigned(ball.level));
  }
}
static const char *set_name(ItemSet set) {
  switch (set) {
  case ItemSet::CLASSIC:
    return "Classic";
  case ItemSet::FOOD:
    return "Food";
  case ItemSet::FRIENDS:
    return "Friends";
  case ItemSet::NATURE:
    return "Nature";
  case ItemSet::SPACE:
    return "Space";
  case ItemSet::SPOOKY:
    return "Spooky";
  case ItemSet::TECH:
    return "Tech";
  case ItemSet::NEGATIVE:
    return "Negative";
  default:
    return "Basic";
  }
}
static void cocktail_shop(Painter &p, const Skin &skin, const Game &g) {
  bar_background(p, skin, g,
                 g.ui_time > 5.82f && skin.cocktail_blink.pixels
                     ? skin.cocktail_blink
                     : g.npc_time>0 && std::fmod(g.npc_time,.22f)<.12f && skin.cocktail_talk.pixels?skin.cocktail_talk:skin.cocktail_merchant);
  header(p, skin, g, "THE COCKTAIL BAR");
  fit(p, skin, 30, 49, 50, 13, white, "Tickets");
  label(p, skin, 30, 66, 17, gold, ink, "%u", g.cocktail_tickets);
  const OwnedBall &left =
      g.inventory[g.mixing_pending ? g.reorder_source : g.inventory_selection];
  const OwnedBall &right = g.inventory[g.inventory_selection];
  bool ready = g.mixing_pending &&
               g.can_mix(g.reorder_source, g.inventory_selection) &&
               g.cocktail_tickets;
  if (left.id)
    mixed_icon(p, skin, left.id, left.mixed_with, 20, 151, 32);
  if (g.mixing_pending && right.id)
    mixed_icon(p, skin, right.id, right.mixed_with, 92, 151, 32);
  label(p, skin, 72, 158, 20, gold, ink, "+");
  label(p, skin, 143, 158, 20, gold, ink, ">");
  if (ready) {
    mixed_icon(p, skin, left.id, right.id, 165, 151, 32);
    label(p, skin, 181, 191, 14, green, ink, "%d", left.score + right.score);
  } else
    label(p, skin, 181, 157, 22, muted, ink, "?");
  if (ready)
    fit(p, skin, 108, 210, 200, 14, green, "Lv%u VALUE %d",
        unsigned(left.level + right.level + 1) / 2, left.score + right.score);
  centered(p, skin, 345, 43, 16, 0xff282828, "CHOOSE BALLS TO MIX");
  for (unsigned i = 0; i < 15; ++i) {
    float x = 246 + (i % 5) * 47.0f, y = 84 + (i / 5) * 41.0f;
    const OwnedBall &ball = g.inventory[i];
    if (i == g.inventory_selection)
      selection_outline(p, x, y, 14, 0xff906b39, g.ui_time);
    if (g.mixing_pending && i == g.reorder_source)
      selection_ring(p, x, y, 18, 0xff36a95b);
    if (ball.id) {
      mixed_icon(p, skin, ball.id, ball.mixed_with, x - 12, y - 12, 24);
      owned_badges(p, skin, ball, x, y);
    } else
      dotted_slot(p, x, y, 10, 0xff777777);
  }
  const char *status = !g.cocktail_tickets ? "No mix tickets"
                       : !g.mixing_pending ? "Choose first ball"
                       : ready             ? "{cross} mix these balls"
                                           : "Choose a different unmixed ball";
  fit(p, skin, 345, 192, 236, 15,
      !g.cocktail_tickets ? 0xff777777
      : ready             ? 0xff30883d
                          : 0xff333333,
      "%s", status);
  fit(p, skin, 345, 213, 230, 14, 0xff333333, "%s",
      right.id ? ball_name(right.id) : "Empty slot");
  footer(p, skin,
         g.mixing_pending ? "{dpad} target   {cross} mix   {circle} cancel"
                          : "{dpad} select   {cross} first ball   {circle} back",
         g.message_time > 0 ? g.message : "{L} / {R} change bar");
}
static void shop(Painter &p, const Skin &skin, const Game &g) {
  if (g.cocktail_bar) {
    cocktail_shop(p, skin, g);
    return;
  }
  if (g.snack_bar) {
    snack_shop(p, skin, g);
    return;
  }
  bar_background(p, skin, g,
                 g.ui_time > 5.82f && skin.merchant_blink.pixels
                     ? skin.merchant_blink
                     : g.npc_time>0 && std::fmod(g.npc_time,.22f)<.12f && skin.merchant_talk.pixels?skin.merchant_talk:skin.merchant);
  header(p, skin, g, "THE SHOP");
  if(g.daily_date){char date[20];format_daily_date(g.daily_date,date,sizeof(date));fit(p,skin,338,46,232,14,gold,"Daily %s",date);}
  else track(p, skin, g.round, 236, 46, 24, &g);
  const ItemSet poster_sets[]={ItemSet::CLASSIC,ItemSet::FOOD,ItemSet::FRIENDS,ItemSet::NATURE,ItemSet::SPACE,ItemSet::SPOOKY,ItemSet::TECH};
  unsigned poster_count=(!g.daily_date && (g.deck==7 || g.trial))?1:(!g.daily_date && !g.difficulty?1:3);
  for(unsigned i=0;i<poster_count;++i){unsigned art=8;if(!g.daily_date && (g.deck==7 || g.trial))art=7;else for(unsigned k=0;k<7;++k)if(g.shop_sets[i]==poster_sets[k])art=k;
    const Texture& poster=skin.set_posters[art];float w=i?26.0f:30.0f,h=w*128.0f/112.0f,cx=i==0?44.0f:i==1?16.0f:72.0f;
    if(poster.pixels)p.sprite(poster,cx-w/2,51,w,h,0xffb3bdff);
    else {const char* names[]={"CLASSIC","FOOD","FRIENDS","NATURE","SPACE","SPOOKY","TECH","ALL","?"};fit(p,skin,cx,64,w,8,white,"%s",names[art]);}
  }
  centered(p, skin, 343, 60, 16, 0xff282828, "YOUR BUILD");
  for (unsigned i = 0; i < 3; ++i) {
    float x = 36 + 70.0f * i;
    unsigned id = g.offers[i];
    bool selected = (g.shop_focus == ShopFocus::Market || g.merge_pending) &&
                    g.selection == i;
    if(id)rarity_star(p,skin,g,x,141,57,g.offer_item(i).mixed_with?4:rarity(id));
    if (selected)
      selection_outline(p, x, 141, 18, white, g.ui_time);
    if (id) {
      rarity_caption(p,skin,x,104,g.offer_item(i).mixed_with?4:rarity(id));
      ball_icon(p, skin, id, x - 16, 125, 32);owned_badges(p,skin,g.offer_item(i),x,141);
      label(p, skin, x, 158, 14, g.money >= item_price(g.offer_item(i)) ? white : red, ink,
            "%d$", item_price(g.offer_item(i)));
    } else {
      dotted_slot(p, x, 141, 13, muted);
      label(p, skin, x, 156, 12, muted, ink, "Sold");
    }
  }
  char roll[40];
  std::snprintf(roll, sizeof(roll), "Roll %d$", g.reroll_price());

  action(p, skin, 108, 179, 108,
         g.shop_focus == ShopFocus::Actions && g.shop_action == 0, roll);
  if (skin.wallet.pixels)
    p.sprite(skin.wallet, 233, 109, 30, 30);
  char cash[24];
  compact(cash, sizeof(cash), g.money);
  label(p, skin, 248, 143, 14, 0xff294329, 0xfff5f5f5, "%s$", cash);
  for (unsigned i = 0; i < 2; ++i)
    if ((i == 0 && (g.difficulty || g.trial)) ||
        (i == 1 && g.difficulty >= 2)) {
      float y = 164 + i * 20.0f;
      if (skin.tickets[i].pixels)
        p.sprite(skin.tickets[i], 233, y, 16, 16, 0xff333333);
      label(p, skin, 261, y, 14, 0xff294329, 0xfff5f5f5, "%u",
            i ? g.cocktail_tickets : g.snack_tickets);
    }
  for (unsigned i = 0; i < 15; ++i) {
    Screen point = shop_slot_position(i);
    const OwnedBall &ball = g.inventory[i];
    bool selected =
        g.shop_focus == ShopFocus::Inventory && g.inventory_selection == i;
    if ((g.reorder_pending || g.owned_merge_pending) && i == g.reorder_source)
      selection_ring(p, point.x, point.y, 14, 0xff36a95b);
    if (selected) {
      selection_outline(p, point.x, point.y, 13, 0xff906b39, g.ui_time);
    }
    if (ball.id && ((g.merge_pending && g.can_merge(g.selection, i)) ||
                    (g.owned_merge_pending && i != g.reorder_source &&
                     can_combine(g.inventory[g.reorder_source], ball))))
      selection_outline(p, point.x, point.y, 14, 0xff36a95b, g.ui_time);
    if (ball.id) {
      mixed_icon(p, skin, ball.id, ball.mixed_with, point.x - 11, point.y - 11,
                 22);
      owned_badges(p, skin, ball, point.x, point.y);
      if (i < 10 && !g.deploys_slot(i))
        dotted_slot(p, point.x, point.y, 13, 0xff777777);
    } else
      dotted_slot(p, point.x, point.y, 9,
                  g.deploys_slot(i) ? 0xff777777 : 0xffaaaaaa);
  }
  centered(p, skin, 432, 143, 13, 0xff555555,
           g.deploys_slot(10) ? "IN PLAY" : "RESERVE");
  centered(p, skin, 433, 65, 12, 0xff555555, "PASSIVES");
  for (unsigned i = 0; i < 4; ++i) {
    float x = 414 + (i % 2) * 25.0f, y = 81 + (i / 2) * 26.0f;
    if (g.snacks[i])
      snack_icon(p, skin, g.snacks[i], x, y, 20);
    else
      dotted_slot(p, x + 10, y + 10, 8, 0xffaaaaaa);
    if (g.shop_focus == ShopFocus::Passive && g.passive_selection == i)
      selection_outline(p, x + 10, y + 10, 12, 0xff906b39, g.ui_time);
  }
  action(p, skin, 346, 214, 100,
         g.shop_focus == ShopFocus::Actions && g.shop_action == 1, "Play");
  const OwnedBall &owned = g.inventory[g.inventory_selection];
  unsigned offer = g.offers[g.selection];
  char name[80];
  item_name(name, sizeof(name), owned);
  if (g.shop_focus == ShopFocus::Market) {
    fit(p, skin, 108, 196, 200, 13, gold, "%s",
        offer ? ball_name(offer) : "Sold out");
    if (offer) {
      char summary[512];
      item_description(summary, sizeof(summary), g.offer_item(g.selection));
      paragraph(p, skin, 10, 210, 197, 2, summary, white, 12, 0, true);
    }
  } else if (g.shop_focus == ShopFocus::Inventory) {
    fit(p, skin, 108, 196, 200, 13, owned.id?rarity_color(owned.mixed_with?4:rarity(owned.id)):gold, "%s", name);
    char ability[512];
    item_description(ability, sizeof(ability), owned);
    if (owned.id)
      paragraph(p, skin, 10, 210, 197, 2, ability, white, 12, 0, true);
  }
  if (g.shop_focus == ShopFocus::Passive) {
    fit(p, skin, 108, 196, 200, 14, gold, "%s",
        snack_name(g.snacks[g.passive_selection]));
    label(p, skin, 108, 216, 14, white, ink, "{cross} details");
  }
  const char *hint = "{dpad} move   {cross} confirm   {L} / {R} bar";
  char details[192] = {};
  if (g.sell_pending && g.shop_focus == ShopFocus::Passive) {
    std::snprintf(details, sizeof(details), "Sell %s for %d$?",
                  snack_name(g.snacks[g.passive_selection]),
                  snack_sell_price(g.snacks[g.passive_selection]));
    hint = "{cross} confirm sale   {circle} cancel";
  } else if (g.sell_pending) {
    char price[24];
    compact(price, sizeof(price), item_price(owned) / 2);
    std::snprintf(details, sizeof(details),
                  "Sell %s Lv%u for $%s? {cross} confirm / {circle} cancel", name,
                  unsigned(owned.level), price);
    hint = "SALE CONFIRMATION";
  } else if (g.merge_pending) {
    bool compatible = g.can_merge(g.selection, g.inventory_selection);
    std::snprintf(details, sizeof(details), "MERGE %s -> %s    %s",
                  ball_name(offer), ball_name(owned.id),
                  compatible ? "READY" : "NOT COMPATIBLE");
    char result[96];
    std::snprintf(
        result, sizeof(result),
        compatible ? "Result: level %u, value %d. {cross} confirm / {circle} cancel"
                   : "{circle} cancel / choose a matching ball",
        [&]() {
          OwnedBall result = owned;
          if (compatible)
            combine_values(g.offer_item(g.selection), result);
          return unsigned(result.level);
        }(),
        owned.score + g.offer_item(g.selection).score);
    footer(p, skin, details, result);
    return;
  } else if (g.owned_merge_pending) {
    std::snprintf(details, sizeof(details), "OWNED MERGE: %s -> %s",
                  ball_name(g.inventory[g.reorder_source].id), name);
    bool compatible = g.reorder_source != g.inventory_selection &&
                      can_combine(g.inventory[g.reorder_source], owned);
    if (compatible) {
      OwnedBall result = owned;
      combine_values(g.inventory[g.reorder_source], result);
      std::snprintf(details, sizeof(details), "MERGE -> level %u, value %d",
                    unsigned(result.level), result.score);
    } else
      std::snprintf(details, sizeof(details),
                    "MERGE: choose a matching ball and level");
    hint = compatible ? "{cross} confirm merge   {circle} cancel"
                      : "{circle} cancel / green outlines show valid targets";
  } else if (g.reorder_pending) {
    std::snprintf(details, sizeof(details), "Move %s to slot %u",
                  ball_name(g.inventory[g.reorder_source].id),
                  g.inventory_selection + 1);
    hint = "{cross} move / swap   {circle} cancel";
  } else if (g.shop_focus == ShopFocus::Inventory) {
    std::snprintf(details, sizeof(details),
                  "{cross} details   {square} merge   {triangle} sell");
  } else if (g.shop_focus == ShopFocus::Passive) {
    std::snprintf(details, sizeof(details), "{cross} details   {triangle} sell");
    hint = "{L} / {R} change bar";
  } else if (g.shop_focus == ShopFocus::Actions) {
    std::snprintf(details, sizeof(details),
                  g.shop_action == 0 ? "{cross} reroll offers" : "{cross} play next round");
    hint = "{dpad} move   {L} / {R} bar";
  }
  if (g.shop_focus == ShopFocus::Inventory && !g.merge_pending &&
      !g.owned_merge_pending && !g.reorder_pending && !g.sell_pending)
    hint = "{select} move / swap   {L} / {R} bar";
  if (g.shop_focus == ShopFocus::Market && !g.merge_pending)
    std::snprintf(details, sizeof(details),
                  "{cross} buy   {square} merge   {select} details");
  footer(p, skin, *details ? details : hint,
         g.message_time > 0
             ? g.message
             : (*details ? hint : "{cross} details   {square} merge   {triangle} sell"));
}
static void menu(Painter &p, const Skin &skin, const Game &g) {
  floor(p, skin);
  if (skin.table.pixels) p.sprite(skin.table,16,6,448,224);
  if (skin.logo.pixels) p.sprite(skin.logo,104,42,70,105);
  else centered(p,skin,141,77,24,gold,"ULTRAPOOL");
  const ItemSet poster_sets[]={ItemSet::CLASSIC,ItemSet::FOOD,ItemSet::FRIENDS,
      ItemSet::NATURE,ItemSet::SPACE,ItemSet::SPOOKY,ItemSet::TECH};
  unsigned poster=8;
  if(g.deck==7)poster=7;
  else for(unsigned i=0;i<7;++i)if(deck_info(g.deck).set==poster_sets[i])poster=i;
  if(poster<skin.set_posters.size() && skin.set_posters[poster].pixels)
    p.sprite(skin.set_posters[poster],77,158,28,32);
  const float build_x[]={113,139,126},build_y[]={156,156,178};
  for(unsigned i=0;i<3;++i)
    ball_icon(p,skin,deck_info(g.deck).balls[i],build_x[i],build_y[i],22);
  const float cx=334;
  bool run_enabled=g.profile.has(deck_requirement(g.deck))&&g.profile.has(difficulty_requirement(g.difficulty));
  menu_nine_patch(p,skin,244,173,180,28,run_enabled?0xff55b653:0xff777783);
  if(g.selection==0){p.line(246,175,422,175,gold);p.line(246,199,422,199,gold);p.line(246,175,246,199,gold);p.line(422,175,422,199,gold);}
  if(skin.menu_icons[6].pixels)p.sprite(skin.menu_icons[6],254,179,16,16);
  fit(p,skin,346,179,140,17,white,run_enabled?"PLAY %s":"LOCKED: %s",deck_info(g.deck).name);
  menu_nine_patch(p,skin,244,43,180,27,g.continue_available?0xff54735b:0xff646464);
  if(g.selection==1){p.line(246,45,422,45,gold);p.line(246,68,422,68,gold);}
  centered(p,skin,cx,49,16,g.continue_available?white:muted,"CONTINUE RUN");
  if(g.continue_available){
    char cash[24];compact(cash,sizeof(cash),g.continue_money);
    fit(p,skin,cx,75,174,12,white,"Round %d   %s$",g.continue_round,cash);
    for(unsigned i=0;i<3;++i)if(g.continue_balls[i].id)
      mixed_icon(p,skin,g.continue_balls[i].id,g.continue_balls[i].mixed_with,307+21.0f*i,91,15);
  }else centered(p,skin,cx,80,12,muted,"No saved run");
  label(p,skin,cx,111,15,white,ink,"%s",difficulty_name(g.difficulty));
  const unsigned icon_order[]={5,0,1,3,4};
  const char* names[]={"SETTINGS","ACHIEVEMENTS","COLLECTION","RECORDS","DAILY"};
  const uint32_t colors[]={0xff557cda,0xffc248d8,0xffe08d42,0xffa73cce,0xffd7a331};
  for(unsigned i=0;i<5;++i){
    float x=247+36.0f*i,y=137;
    menu_nine_patch(p,skin,x,y,30,29,colors[i]);
    if(skin.menu_icons[icon_order[i]].pixels)p.sprite(skin.menu_icons[icon_order[i]],x+6,y+5,18,18);
    if(g.selection==i+2){p.line(x-2,y-2,x+32,y-2,gold);p.line(x-2,y+31,x+32,y+31,gold);p.line(x-2,y-2,x-2,y+31,gold);p.line(x+32,y-2,x+32,y+31,gold);}
  }
  if(g.profile.has_unseen_achievements())fit(p,skin,298,126,34,9,gold,"NEW!");
  footer(p,skin,"{dpad} choose   {cross} confirm   {square} difficulty",
      g.message_time>0?g.message:g.selection>=2&&g.selection<7?names[g.selection-2]:"{L} seed   {lr} deck   {triangle} resume");
}
static void table(Painter &p, const Skin &skin, const Game &g,
                  bool modal = false) {
  p.rect(0, 0, 480, 272, ink);
  if (skin.table.pixels)
    p.sprite(skin.table, 85, 19.5f, 310, 217);
  else {
    p.rect(85, 19.5f, 310, 217, 0xff31496f);
    p.rect(100, 40, 280, 176, 0xff35552b);
  }
  if(g.candles_pocketed && (!g.ritual_done || g.ritual_flash>0)){
    const Vec points[6]={Vec(101,-103),Vec(0,209),Vec(-102,-104),Vec(163,89),Vec(-163,89),Vec(101,-103)};
    uint32_t color=g.ritual_done?0x90ffffff:0x50203d24;
    for(unsigned i=0;i<g.candles_pocketed;++i){Vec end=points[i+1];if(i+1==g.candles_pocketed)end=points[i]+(end-points[i])*g.ritual_progress;Screen a=project(points[i]),b=project(end);p.line(a.x,a.y,b.x,b.y,color);}
  }
  float progress =
      std::max(0.0f, std::min(1.0f, g.score_display / std::max(1, g.target)));
  diamond(p, 225, 128, 39, 0x443c6a3e);
  diamond(p, 225, 128, 37, 0x704e9a50, progress);
  float overflow_progress =
      std::max(0.0f, std::min(1.0f, g.score_display /
                                        float(std::max(g.target * 10, 100))));
  for (int row = 42 - int(84 * overflow_progress); row <= 42; ++row) {
    int half = 42 - std::abs(row);
    p.rect(float(225 - half), float(128 + row), 2, 1, 0xffc0e7b9);
    p.rect(float(225 + half - 1), float(128 + row), 2, 1, 0xffc0e7b9);
  }
  const float dx[] = {225, 267, 225, 183, 225}, dy[] = {86, 128, 170, 128, 86};
  for (unsigned i = 0; i < 4; ++i)
    p.line(dx[i], dy[i], dx[i + 1], dy[i + 1],
           g.score_flash > 0 ? white : 0xff92bc85);
  char display[24], target[24];
  compact(display, sizeof(display), int64_t(g.score_display + .5f));
  compact(target, sizeof(target), g.target);
  fit(p, skin, 225, 108, 67, 24, 0xffe3f0c4, "%s", display);
  fit(p, skin, 225, 136, 62, 15, 0xffe3f0c4, "/%s", target);
  for (unsigned ordinal=0;ordinal<g.world.pocket_count();++ordinal) {
    const Pocket& pocket=g.world.pocket_at(ordinal);
    Screen v = project(pocket.p);
    if (pocket.shielded) {
      p.line(v.x - 9, v.y - 8, v.x + 9, v.y - 8, green);
      p.line(v.x - 9, v.y - 8, v.x, v.y + 8, green);
      p.line(v.x + 9, v.y - 8, v.x, v.y + 8, green);
    }
    if(pocket.suck_scale>1)p.circle(v.x,v.y,pocket.radius*pocket.scale*pocket.suck_scale*table_scale,0x40302535);
    p.circle(v.x,v.y,pocket.radius*pocket.scale*table_scale,0xff141019);
    if(pocket.held_count){if(skin.pocket_skull.pixels)p.sprite(skin.pocket_skull,v.x-5,v.y-5,10,10);else {p.circle(v.x,v.y-1,3,white);p.rect(v.x-2,v.y+1,4,3,white);}}
    if(pocket.closed) {
      p.line(v.x-7,v.y-7,v.x+7,v.y+7,red);
      p.line(v.x-7,v.y+7,v.x+7,v.y-7,red);
    }
    float label_y=ordinal<6?(v.y<128?22.0f:218.0f):v.y+pocket.radius*pocket.scale*table_scale+2;
    char multi[24], bonus[24], tag[56];
    compact(multi, sizeof(multi), int64_t(pocket.multiplier));
    compact(bonus, sizeof(bonus), pocket.bonus);
    std::snprintf(tag, sizeof(tag), pocket.bonus ? "x%s %s%s" : "x%s", multi,
                  pocket.bonus > 0 ? "+" : "", bonus);
    float h=ordinal<6?15.0f:11.0f,measure=ui_text_width(skin,h,tag);
    if (measure > 80)
      h *= 80 / measure;
    label(p, skin, v.x, label_y, h, pocket.bonus < 0 ? red : 0xffd6f5db, ink,
          "%s", tag);
  }
  for (const auto &drop : g.pickups)
    if (drop.active) {
      Screen v = project(drop.p);
      unsigned kind = static_cast<unsigned>(drop.kind);
      float world_size = kind == 3 ? 28.0f : kind == 4 ? 52.0f : 36.0f;
      float size = std::max(2.0f, world_size * drop.scale * table_scale);
      const Texture &texture =
          kind == 0 ? skin.flowers[(std::max(1, drop.power) - 1) %
                                   skin.flowers.size()]
                    : skin.pickups[kind];
      uint32_t tint = 0xffffffff;
      if (kind == 1) {
        float alpha = .85f * (drop.charges == 4 ? 1 : .85f * drop.charges / 3);
        tint = (uint32_t(std::max(.55f, std::min(1.0f, alpha)) * 255) << 24) |
               0x00ffffff;
      }
      if (kind == 4)
        tint = 0xc0b4c9fd;
      if (texture.pixels)
        p.sprite(texture, v.x - size * .5f, v.y - size * .5f, size, size, tint);
      else
        p.circle(v.x, v.y, size * .35f,
                 kind == 0   ? green
                 : kind == 1 ? ink
                 : kind == 2 ? red
                             : gold);
      if (kind == 0 && drop.power > 1)
        text(p, int(v.x - 3), int(v.y - 3), white, "%d", drop.power);
      if (kind == 3) {
        Screen end = project(drop.p + drop.direction * 24);
        p.line(v.x, v.y, end.x, end.y, white);
        p.circle(end.x, end.y, 1.5f, gold);
      }
    }
  for (const auto &e : g.effects)
    if (e.life > 0) {
      Screen v = project(e.p);if(e.passive){v.x+=passive_draw_dx;v.y+=passive_draw_dy;}
      if(e.line){Screen end=project(e.end);uint32_t tint=(e.color&0x00ffffffu)|(static_cast<uint32_t>(255*std::min(1.0f,e.life/.3f))<<24);p.line(v.x,v.y,end.x,end.y,tint);continue;}
      float r = e.radius * (1 - e.life / 0.5f) * table_scale;
      for (int j = 0; j < 20; ++j) {
        float a = j * 6.2831853f / 20, b = (j + 1) * 6.2831853f / 20;
        p.line(v.x + std::cos(a) * r, v.y + std::sin(a) * r,
               v.x + std::cos(b) * r, v.y + std::sin(b) * r, e.color);
      }
    }
  for (std::size_t i = 0; i < g.world.count; ++i) {
    const Ball &b = g.world.balls[i];
    if (!b.active)
      continue;
    Screen v = project(b.p);
    if(g.passive_slot(static_cast<unsigned>(i))<8){v.x+=passive_draw_dx;v.y+=passive_draw_dy;}
    if(has_ability(b,Reaper) && b.last_contact<g.world.count){const Ball& peer=g.world.balls[b.last_contact];if(peer.active && !peer.gone && peer.generation==b.last_contact_generation){Screen end=project(peer.p);if(g.passive_slot(b.last_contact)<8){end.x+=passive_draw_dx;end.y+=passive_draw_dy;}Vec delta(end.x-v.x,end.y-v.y);Vec normal=delta.dot(delta)>0?Vec(-delta.y,delta.x).unit():Vec();for(int stroke=-1;stroke<=1;++stroke)p.line(v.x+normal.x*stroke,v.y+normal.y*stroke,end.x+normal.x*stroke,end.y+normal.y*stroke,0x64000000);}}
    float scale = b.falling ? std::max(0.1f, b.fall_remaining * 2) : 1;
    float d = b.def.radius * (2 * table_scale) * scale;
    if(b.energy){
      p.circle(v.x,v.y,d*.64f,0xffc2fbff);
      p.circle(v.x,v.y,d*.44f,0xffffffff);
      continue;
    }
    if (g.inspected == i && i)
      p.circle(v.x, v.y, d * 0.5f + 2, gold);
    mixed_icon(p, skin, b.def.id, b.mixed_with, v.x - d * 0.5f, v.y - d * 0.5f,
               d, b.flash > 0 ? 0xffffe6ff : 0xffffffff,b.visual_rotation);
    if (b.flaming && skin.statuses[0].pixels)
      p.sprite(skin.statuses[0], v.x - 4, v.y - d * .5f - 9, 8, 10);
    else if (b.flaming) {
      p.circle(v.x, v.y - d * .5f - 3, 2.5f, red);
      p.circle(v.x, v.y - d * .5f - 3, 1, gold);
    }
    if (b.shielded && skin.statuses[1].pixels)
      p.sprite(skin.statuses[1], v.x - d * .7f, v.y - d * .7f, d * 1.4f,
               d * 1.4f, 0xffffdfb5);
    else if (b.shielded)
      selection_ring(p, v.x, v.y, d * .5f + 2, green);
    if (b.fleeting)
      dotted_slot(p, v.x, v.y, d * .5f + 3, muted);
    if (b.locked && skin.statuses[2].pixels)
      p.sprite(skin.statuses[2], v.x + d * .5f - 2, v.y - 4, 7, 8);
    else if (b.locked) {
      float x = v.x + d * .5f, y = v.y - 4;
      p.rect(x, y, 4, 4, gold);
      p.line(x, y, x, y - 3, gold);
      p.line(x, y - 3, x + 3, y - 3, gold);
      p.line(x + 3, y - 3, x + 3, y, gold);
    }
    if (b.star_power && skin.statuses[3].pixels)
      p.sprite(skin.statuses[3], v.x - d * .5f - 7, v.y - 4, 8, 8);
    else if (b.star_power) {
      float x = v.x - d * .5f - 3, y = v.y - 3;
      for (int j = 0; j < 5; ++j) {
        float a = j * 6.2831853f / 5 - 1.5708f,
              z = (j + 2) * 6.2831853f / 5 - 1.5708f;
        p.line(x + std::cos(a) * 3, y + std::sin(a) * 3, x + std::cos(z) * 3,
               y + std::sin(z) * 3, gold);
      }
    }
    if(!b.cue && b.level>1 && b.level<=5 && !(number_ball(b.def.id) && !b.mixed_with)){
      const Texture& spark=skin.levels[b.level-2];float size=std::max(5.0f,d*.5f);
      if(spark.pixels)p.sprite(spark,v.x-d*.7f,v.y-d*.45f,size,size);
    }
    bool crowded = false;
    for (std::size_t j = 0; j < g.world.count; ++j)
      if (j != i && g.world.balls[j].active) {
        Screen n = project(g.world.balls[j].p);if(g.passive_slot(static_cast<unsigned>(j))<8){n.x+=passive_draw_dx;n.y+=passive_draw_dy;}
        float x = n.x - v.x, y = n.y - v.y;
        if (x * x + y * y < 225) {
          crowded = true;
          break;
        }
      }
    if (!b.cue && !b.falling && b.def.base_score > 1 &&
        (!crowded || g.inspected == i)) {
      char value[24];
      compact(value, sizeof(value), b.def.base_score);
      label(p, skin, v.x + 8, g.passive_slot(static_cast<unsigned>(i))<8?v.y-12:v.y+5, 11, white, ink, "%s", value);
    }
  }
  unsigned visible_carriers = 0;
  for (const auto &a : g.actions)
    if (a.kind == ActionKind::Boom || a.kind == ActionKind::Spawn || a.kind == ActionKind::PocketChange ||
        a.kind == ActionKind::BuffBall || a.kind == ActionKind::PocketBall ||
        a.kind == ActionKind::TransformBall ||
        a.kind == ActionKind::ModifierBall || a.kind == ActionKind::SplitBall) {
      if (visible_carriers++ >= 64)
        break;
      Screen point = project(a.p);
      float passive_shift=0;
      if(a.kind==ActionKind::Spawn && (a.flags&16)){float initial=(a.impulse-a.destination).length();passive_shift=initial>0?std::max(0.0f,std::min(1.0f,1-(a.p-a.destination).length()/initial)):1;point.x+=passive_draw_dx*passive_shift;point.y+=passive_draw_dy*passive_shift;}
      p.circle(point.x, point.y, 2, green);
      Screen tail = project(a.p - a.velocity * 0.025f);tail.x+=passive_draw_dx*passive_shift;tail.y+=passive_draw_dy*passive_shift;
      p.line(tail.x, tail.y, point.x, point.y, green);
    }
  if (g.aim_preparing &&
      (g.phase == Phase::Aiming || g.phase == Phase::Simulating)) {
    AimPrediction aim = predict_aim(
        g.world, Vec(std::cos(g.angle), std::sin(g.angle)), g.power);
    for (unsigned i = 1; i < aim.count; ++i) {
      Screen a = project(aim.points[i - 1]), b = project(aim.points[i]);
      p.line(a.x, a.y, b.x, b.y, white);
    }
    if (aim.hit >= 0) {
      Screen v = project(aim.points[aim.count - 1]);
      for (int j = 0; j < 24; ++j) {
        float a = j * 6.2831853f / 24, b = (j + 1) * 6.2831853f / 24,
              r = 17.5f * table_scale;
        p.line(v.x + std::cos(a) * r, v.y + std::sin(a) * r,
               v.x + std::cos(b) * r, v.y + std::sin(b) * r, white);
      }
      for (unsigned k = 0; k < 2; ++k) {
        for(unsigned i=1;i<aim.travel_count[k];++i) {
          Vec from=aim.travel[k][i-1],to=aim.travel[k][i];
          float length=(to-from).length(); unsigned dots=std::min(80u,unsigned(length/9.0f));
          for(unsigned j=0;j<=dots;++j) {
            if((j%2u)!=0) continue;
            Vec point=from+(to-from)*(dots?float(j)/dots:0.0f);
            Screen s = project(point);
          p.circle(s.x, s.y, k ? 1.0f : .65f, k ? white : 0xffaac98e);
          }
        }
      }
    }
  }
  if (!modal)
    for (unsigned i = 0; i < g.snacks.size(); ++i)
      if (g.snacks[i]) {
        float x = 10 + (i % 2) * 33.0f, y = 65 + (i / 2) * 52.0f;
        snack_icon(p, skin, g.snacks[i], x, y, 24);
        unsigned goal = snack_counter_goal(g.snacks[i]);
        if (goal) {
          char n[24];
          compact(n, sizeof(n), g.snack_counts[i] < 0 ? 1 : g.snack_counts[i]);
          label(p, skin, x + 12, y + 25, 12, green, ink, "%s/%u", n, goal);
          p.rect(x, y + 41, 24, 2, 0xff444444);
          p.rect(
              x, y + 41,
              24 * std::min(1.0f, float(std::max(0, g.snack_counts[i])) / goal),
              2, green);
        }
      }
  if(!modal)for(unsigned i=0;i<g.cubes.size();++i)if(g.cubes[i].id) {
    const auto& cube=g.cubes[i];const float x=399+26.0f*(i%3),y=43+38.0f*(i/3);
    const Texture& face=skin.balls[cube.id];
    p.rect(x+2,y+2,20,20,0x66000000);
    if(face.pixels)p.sprite(face,x,y,20,20);
    else p.rect(x,y,20,20,red);
    if(cube.level>1 && skin.levels[cube.level-2].pixels)p.sprite(skin.levels[cube.level-2],x+13,y-3,9,9);
    label(p,skin,x+10,y+22,11,white,ink,"%d",cube.count);
  }
  Screen points[24];
  int64_t amounts[24] = {};bool passive_groups[24]={};
  unsigned groups = 0;
  for (const auto &popup : g.score_popups)
    if (popup.life > 0) {
      Screen point = project(popup.p);if(popup.passive){point.x+=passive_draw_dx*popup.life;point.y+=passive_draw_dy*popup.life;}
      unsigned group = 0;
      for (; group < groups; ++group) {
        float gx = point.x - points[group].x, gy = point.y - points[group].y;
        if (gx * gx + gy * gy < 625)
          break;
      }
      if (group == groups)
        points[groups++] = point;
      amounts[group] += popup.amount;passive_groups[group]|=popup.passive;
    }
  for (unsigned i = 0; i < groups; ++i) {
    char value[24];
    compact(value, sizeof(value), amounts[i]);
    label(p, skin, std::max(passive_groups[i]?16.0f:111.0f, std::min(passive_groups[i]?464.0f:369.0f, points[i].x)),
          std::max(passive_groups[i]?24.0f:50.0f, std::min(passive_groups[i]?235.0f:196.0f, points[i].y - 17)), 17, white, ink,
          "%s", value);
  }
  p.rect(0, 0, 480, 20, ink);
  if(g.daily_date)ui_text(p,skin,8,2,15,gold,"DAILY R%d",g.round);
  else if(g.endless)ui_text(p,skin,8,2,15,gold,"R%d",g.round);
  else ui_text(p, skin, 8, 2, 15, gold, "R%d/%u", g.round, rounds_for_difficulty(g.difficulty));
  char score[24], target_value[24];
  compact(score, sizeof(score), g.score);
  compact(target_value, sizeof(target_value), g.target);
  fit(p, skin, 125, 2, 148, 15, white, "%s / %s", score, target_value);
  for (int i = 0; i < std::min(g.shots, 8); ++i)
    p.circle(212.0f + i * 8, 9, 2.3f, green);
  if (g.shots > 8)
    fit(p, skin, 297, 2, 48, 14, green, "+%d", g.shots - 8);
  char money[24];
  compact(money, sizeof(money), g.money);
  coin(p, 358, 10);
  fit(p, skin, 391, 2, 54, 15, gold, "%s$", money);
  for (int i = 0; i < std::min(g.hp, 3); ++i) {
    if (skin.heart.pixels)
      p.sprite(skin.heart, 430.0f + i * 15, 4, 12, 12);
    else
      p.circle(436.0f + i * 15, 10, 4, red);
  }
  if (modal)
    return;
  if (g.inspected && g.inspected < g.world.count) {
    const Ball &ball = g.world.balls[g.inspected];
    p.rect(0, 20, 480, 217, 0xb0000000);
    frame(p, 91, 36, 378, 192, 0xffaaaba5, 0xffe7e6e2);
    mixed_icon(p, skin, ball.def.id, ball.mixed_with, 107, 57, 44);
    OwnedBall item;
    item.id = ball.def.id;
    item.mixed_with = ball.mixed_with;
    char name[96];
    item_name(name, sizeof(name), item);
    fit(p, skin, 310, 44, 288, 18, 0xff222222, "%s", name);
    char value[24];
    compact(value, sizeof(value), ball.def.base_score);
    fit(p, skin, 132, 111, 66, 14, 0xff333333, "Level %u",
        unsigned(ball.level));
    fit(p, skin, 132, 132, 66, 14, 0xff333333, "Value %s", value);
    char body[512];
    char first[256], second[256];
    describe_ability(ball.def.ability, ball.level, first, sizeof(first));
    describe_ability(builtin(ball.mixed_with).ability, ball.level, second,
                     sizeof(second));
    std::snprintf(body, sizeof(body), ball.mixed_with ? "%s / %s" : "%s", first,
                  second);
    paragraph(p, skin, 169, 77, 283, 9, body, 0xff333333, 15);
    footer(p, skin, "{square} next ball   {cross} / {circle} close");
  } else {
    p.rect(0, 238, 480, 34, ink);
    p.rect(10, 244, 84, 6, 0xff645869);
    p.rect(10, 244, 84 * g.power, 6, gold);
    ui_text(p, skin, 101, 240, 14, white, "%d%%", int(g.power * 100));
    fit(p, skin, 300, 239, 298, 14, white, "%s",
        g.phase == Phase::Simulating ? "Balls moving"
        : g.aim_preparing            ? "{cross} shoot   {L} / {R} power"
                                     : "{cross} aim   {L} / {R} power");
    fit(p, skin, 240, 255, 462, 13, muted,
        "{square} inspect   {triangle} end round   {start} pause");
  }
}
static void round_result(Painter &p, const Skin &skin, const Game &g) {
  table(p, skin, g, true);
  p.rect(0, 20, 480, 252, 0xa8000000);
  // The original result uses celebratory rays behind a large score card.
  // Keep rays outside the card so they add color without hurting readability.
  for(unsigned i=0;i<32;++i){
    float angle=(float(i)+.5f)*6.2831853f/32.0f,dx=std::cos(angle),dy=std::sin(angle);
    float scale=std::min(240.0f/std::max(.001f,std::fabs(dx)),136.0f/std::max(.001f,std::fabs(dy)));
    float ex=240+dx*scale,ey=136+dy*scale;
    if((i&1)==0){uint32_t ray=g.last_round_won?0x70428fe0:0x703f607d;p.line(240,136,ex,ey,ray);p.line(241,136,ex+dy,ey-dx,ray);}
  }
  menu_nine_patch(p,skin,57,23,366,219,0xffe8e5dd);
  centered(p, skin, 240, 29, 23, g.last_round_won ? 0xff4a7c38 : 0xff3333b5,
           g.last_round_won ? "ROUND WON" : "ROUND LOST");
  char score[24], target[24], earned[24];
  compact(score, sizeof(score), g.score);
  compact(target, sizeof(target), g.target);
  int paid = g.payout_base_shown + g.payout_bonus_shown + g.payout_shots_shown;
  compact(earned, sizeof(earned),
          int64_t(g.round_earned) - g.payout_total + paid);
  track(p,skin,g.round,130,62,24,&g);
  p.rect(79,77,322,48,0xff73847b);
  p.line(82,79,398,79,0xffa6b4a8);p.line(82,123,398,123,0xff4e635a);
  centered(p,skin,240,78,15,white,"SCORE");
  menu_nine_patch(p,skin,173,97,134,23,0xfff5f2ec);
  fit(p, skin, 240, 98, 122, 20, 0xff222222, "%s / %s", score, target);
  p.rect(79,131,322,42,0xff4aa8c7);
  p.line(82,133,398,133,0xff88e0ff);p.line(82,171,398,171,0xff327598);
  ui_text(p, skin, 91, 143, 16, white, "MONEY EARNED");
  menu_nine_patch(p,skin,292,136,98,32,0xfff5f2ec);
  fit(p, skin, 341, 143, 83, 21, 0xff407c36, "+%s$", earned);
  char base[24], bonus[24], shots[24];
  compact(base, sizeof(base), g.payout_base_shown);
  compact(bonus, sizeof(bonus), g.payout_bonus_shown);
  compact(shots, sizeof(shots), g.payout_shots_shown);
  fit(p, skin, 145, 176, 91, 12, 0xff444444, "Round %s$", base);
  fit(p, skin, 240, 176, 91, 12, 0xff444444, "Bonus %s$", bonus);
  fit(p, skin, 335, 176, 91, 12, 0xff444444, "Shots %s$", shots);
  char wallet[24], other[24];
  compact(wallet, sizeof(wallet), g.money);
  compact(other, sizeof(other), g.round_earned - g.payout_total);
  fit(p, skin, 240, 188, 274, 12, 0xff444444, "Other %s$    Wallet %s$",
      other, wallet);
  menu_nine_patch(p,skin,82,204,316,34,0xff55b653);
  centered(p,skin,240,211,17,0xffffffff,
           g.payout_done()?"{cross} CONTINUE":"{cross} COUNT ALL");
}
static void daily_menu_scene(Painter& p,const Skin& skin,const Game& g){
 floor(p,skin);centered(p,skin,240,10,22,gold,"DAILY CHALLENGE");
 uint32_t selected=daily_date_from_serial(daily_serial(g.daily_today)-int(g.daily_selection));
 unsigned year=selected/10000,month=(selected/100)%100;
 const char* months[]={"January","February","March","April","May","June","July","August","September","October","November","December"};
 fit(p,skin,159,39,272,17,white,"%s %u",months[month-1],year);
 const char* days[]={"Su","Mo","Tu","We","Th","Fr","Sa"};
 for(unsigned col=0;col<7;++col)centered(p,skin,35+39.0f*col,65,12,muted,days[col]);
 uint32_t first=year*10000+month*100+1;
 unsigned start=unsigned((daily_serial(first)+4)%7+7)%7;
 for(unsigned day=1;day<=31;++day){
  uint32_t date=year*10000+month*100+day;if(!valid_daily_date(date))break;
  unsigned cell=start+day-1;float x=35+39.0f*(cell%7),y=86+21.0f*(cell/7);
  bool enabled=daily_in_window(date,g.daily_today),focus=date==selected;
  if(focus){menu_nine_patch(p,skin,x-17,y-3,34,20,0xff52794a);}
  centered(p,skin,x,y,13,focus?gold:enabled?white:muted,"%u",day);
  if(const auto* record=g.daily_history.find(date)){
   unsigned medal=daily_medal(date,record->score);
   if(skin.medals[medal].pixels)p.sprite(skin.medals[medal],x+7,y+9,9,9);else p.circle(x+10,y+14,2,gold);
  }
 }
 p.line(309,40,309,222,muted);
 char date_text[20];format_daily_date(selected,date_text,sizeof(date_text));
 fit(p,skin,391,46,148,14,white,"%s",date_text);
 auto setup=make_daily(selected);fit(p,skin,391,69,148,14,gold,"Round %u",setup.round);
 for(unsigned i=0;i<3;++i)ball_icon(p,skin,setup.balls[i],329+39.0f*i,96,28);
 fit(p,skin,391,136,148,13,white,"%u$  %u snack  %u mix",setup.money,setup.snack_tickets,setup.cocktail_tickets);
 if(const auto* record=g.daily_history.find(selected)){
  unsigned medal=daily_medal(selected,record->score);if(skin.medals[medal].pixels)p.sprite(skin.medals[medal],369,160,42,42);
  fit(p,skin,391,210,148,13,gold,"Score %d",record->score);
 }else fit(p,skin,391,177,148,12,muted,"%s",daily_in_window(selected,g.daily_today)?"Ready to play":"Outside play window");
 footer(p,skin,"{dpad} date   {L} / {R} month   {circle} back",
      g.message_time>0?g.message:"{cross} open / play   {square} practice   {triangle} today");
}
static void daily_result_scene(Painter& p,const Skin& skin,const Game& g){
 floor(p,skin);char date[20];format_daily_date(g.daily_date,date,sizeof(date));centered(p,skin,240,12,24,gold,"DAILY RESULT");
 fit(p,skin,240,45,440,15,white,"%s%s",date,g.daily_replay?"  REPLAY":"");
 fit(p,skin,278,79,332,25,white,"%d",int(g.score_display));const char* names[]={"NO MEDAL","BRONZE","SILVER","GOLD","DIAMOND"};unsigned medal=daily_medal(g.daily_date,int(g.score_display));
 const uint32_t colors[]={muted,0xff598abd,0xffddd6ca,gold,0xffebd875};
 if(skin.medals[medal].pixels){float scale=g.reduced_motion?1:1+std::sin(g.ui_rotation*3)*.025f;p.sprite(skin.medals[medal],63-26*(scale-1),66-26*(scale-1),52*scale,52*scale);}
 centered(p,skin,278,111,19,colors[medal],names[medal]);
 auto thresholds=daily_thresholds(g.daily_date);p.rect(48,150,384,5,0xff534548);p.rect(48,150,384*std::max(0.0f,std::min(1.0f,g.score_display/thresholds[3])),5,colors[medal]);
 for(unsigned i=0;i<4;++i){float x=48+384*float(thresholds[i])/thresholds[3];p.circle(x,152.5f,5,medal>i?colors[i+1]:muted);char value[20];std::snprintf(value,sizeof(value),"%d",thresholds[i]);centered(p,skin,x,166,12,white,value);}
 action(p,skin,240,199,290,true,"{cross}  Return to menu");
 footer(p,skin,g.daily_snapshot_available?"{triangle} continue with this build":"Result snapshot unavailable",g.daily_replay?"Replay leaves the recorded result unchanged":"Offline daily result saved on this PSP");
}
static void profile_scene(Painter& p,const Skin& skin,const Game& g){
 floor(p,skin);const char* tabs[]={"RECORDS","ACHIEVEMENTS","BALLS","PASSIVES","CUBES"};
 centered(p,skin,240,12,23,gold,tabs[g.profile_tab]);
 if(g.profile.has_unseen_achievements())fit(p,skin,424,12,54,13,0xffd34051,"NEW!");
 if(g.profile_tab==0){
  fit(p,skin,240,49,430,17,white,"Wins %lld    Achievements %u / %u",static_cast<long long>(g.profile.stat("WIN-GAME")),g.profile.achievement_total(),achievement_count);
  fit(p,skin,240,76,430,16,white,"Best score %lld    Highest round %lld",static_cast<long long>(g.profile.stat("MAX-SCORE")),static_cast<long long>(g.profile.stat("MAX_ROUND_REACHED")));
  for(unsigned d=0;d<8;++d){int x=22+int(d%4)*116,y=111+int(d/4)*51;
   fit(p,skin,float(x+48),float(y),106,13,g.profile.has(deck_requirement(d))?white:muted,"%s",deck_info(d).name);
   for(unsigned m=0,n=0;m<6;++m){if(m==4)continue;uint32_t color=g.profile.completed(d,m)?gold:0xff55504e;p.circle(float(x+8+n++*18),float(y+25),4,color);}
  }
  fit(p,skin,240,224,440,13,white,"Daily %s    Endless %s",g.profile.daily_unlocked()?"unlocked":"needs 1 win",g.profile.endless_unlocked()?"unlocked":"needs 3 wins");
 }else if(g.profile_tab==1){
  unsigned count=g.profile.visible_achievements(),start=(g.profile_selection/5)*5;
  for(unsigned row=0;row<5 && start+row<count;++row){unsigned i=g.profile.visible_achievement(start+row);const auto& rule=achievement_rule(i);int y=49+int(row)*35;
   char title[100];std::snprintf(title,sizeof(title),"%s",rule.id);for(char* c=title;*c;++c)if(*c=='_' || *c=='-')*c=' ';
   const AchievementText* original=skin.achievement_text?&skin.achievement_text[i]:nullptr;
   if(original && *original->title)std::snprintf(title,sizeof(title),"%s",original->title);
   if(start+row==g.profile_selection)p.line(20,float(y+26),460,float(y+26),gold);
   fit(p,skin,226,float(y),397,15,g.profile.unlocked[i]?gold:white,"%s",title);
   if(original && *original->description)fit(p,skin,240,float(y+17),430,11,muted,"%s",original->description);
   else if(*rule.condition)fit(p,skin,240,float(y+17),430,11,muted,"%lld / %lld%s",static_cast<long long>(g.profile.stat(rule.condition)),static_cast<long long>(rule.amount),rule.per_round?"  in one round":"");
   else fit(p,skin,240,float(y+17),430,11,muted,"%s",g.profile.unlocked[i]?"Unlocked":"Special challenge");
  }
  fit(p,skin,240,228,430,12,muted,"%u / %u",std::min(count,g.profile_selection+1),count);
 }else{
  unsigned kind=g.profile_tab-2,count=g.profile.gallery_count(kind),start=(g.profile_selection/18)*18;
  for(unsigned cell=0;cell<18 && start+cell<count;++cell){unsigned id=g.profile.gallery_id(kind,start+cell);bool seen=kind==1?g.profile.seen_snacks[id]!=0:g.profile.seen_balls[id]!=0;float x=70+float(cell%6)*68,y=65+float(cell/6)*43;
   if(start+cell==g.profile_selection){p.circle(x,y,19,0xffa7e9ff);p.circle(x,y,17,0xff473a35);}
   if(seen){if(kind==1)snack_icon(p,skin,id,x-14,y-14,28);else ball_icon(p,skin,id,x-14,y-14,28);}else centered(p,skin,x,y-12,22,muted,"?");
  }
  unsigned id=g.profile.gallery_id(kind,g.profile_selection);bool seen=id && (kind==1?g.profile.seen_snacks[id]!=0:g.profile.seen_balls[id]!=0);
  if(seen){
   const auto& prose=kind==1?skin.snack_texts:skin.ball_text;
   const auto& info=kind==1?snack_info(id):ball_info(id);
   fit(p,skin,240,173,435,16,gold,"%s",*prose.title(id)?prose.title(id):kind==1?snack_name(id):ball_name(id));
   fit(p,skin,240,191,435,11,muted,"%s  /  %s",set_name(info.set),rarity_name(info.rarity));
   char fallback[512];if(kind==1)std::snprintf(fallback,sizeof(fallback),"%s",snack_text(id));else describe_ability(builtin(id).ability,1,fallback,sizeof(fallback));
   paragraph(p,skin,27,207,426,2,*prose.text(id)?prose.text(id):fallback,white,12);
  }
  else fit(p,skin,240,190,430,16,muted,"Undiscovered");
 }
 footer(p,skin,"{L} / {R} category   {dpad} browse   {circle} close",g.profile_tab>=2?"{cross} full description":g.profile_tab==0?"{cross} how to play":"Local achievements are saved on this PSP");
}
static void seed_scene(Painter& p,const Skin& skin,const Game& g){
 p.rect(0,0,480,272,ink);
 centered(p,skin,240,10,23,gold,"SEEDED RUN");
 fit(p,skin,240,40,440,14,white,"%s / %s",deck_info(g.deck).name,difficulty_name(g.difficulty));
 menu_nine_patch(p,skin,35,61,410,26,0xffe8e5dd);
 fit(p,skin,240,66,390,17,ink,"%s_",g.seed_text);
 static const char keys[]="abcdefghijklmnopqrstuvwxyz0123456789- ";
 const char* special[]={"DEL","CLEAR","PLAY","BACK"};
 for(unsigned key=0;key<42;++key){
  float x=45+66.0f*(key%6),y=96+17.0f*(key/6);bool focused=key==g.seed_cursor;
  if(focused)menu_nine_patch(p,skin,x-24,y-1,48,17,0xff56894a);
  if(key<38)centered(p,skin,x,y,13,focused?gold:white,"%c",keys[key]==' '?'_':keys[key]);
  else centered(p,skin,x,y,11,focused?gold:white,"%s",special[key-38]);
 }
 footer(p,skin,"{dpad} choose   {cross} enter   {square} delete   {start} play",
     g.message_time>0?g.message:"{circle} back   Seeded runs do not award achievements");
}
static void replay_scene(Painter& p,const Skin& skin,const Game& g){
 p.rect(0,0,480,272,ink);
 centered(p,skin,240,9,23,gold,"HOW TO PLAY");
 const auto& rule=tutorial_rules[g.replay_step];const auto* original=skin.tutorial_text?&skin.tutorial_text[g.replay_step]:nullptr;
 char topic[96];std::snprintf(topic,sizeof(topic),"%s",rule.id);for(char* c=topic;*c;++c)if(*c=='-')*c=' ';
 fit(p,skin,240,39,440,14,white,"%u / %u   %s",g.replay_step+1,tutorial_count,topic);
 menu_nine_patch(p,skin,18,64,444,162,0xffe8e5dd);
 bool portrait=rule.portrait && rule.portrait<=skin.tutorial_portraits.size() && skin.tutorial_portraits[rule.portrait-1].pixels;
 if(portrait)p.sprite(skin.tutorial_portraits[rule.portrait-1],30,86,90,81);
 paragraph(p,skin,portrait?136.0f:32.0f,79,portrait?307.0f:414.0f,9,original && *original->description?original->description:"Convert your original tutorial text to see this lesson.",ink,14,0,false,true);
 footer(p,skin,"{lr} / {cross} next   {L} / {R} chapter   {circle} close",
       "Tutorial replay keeps your current run and tutorial progress");
}
static void gallery_detail_scene(Painter& p,const Skin& skin,const Game& g){
 p.rect(0,0,480,272,ink);
 unsigned kind=g.profile_tab-2,id=g.profile.gallery_id(kind,g.profile_selection);
 bool seen=id && (kind==1?g.profile.seen_snacks[id]!=0:g.profile.seen_balls[id]!=0);
 const auto& bank=kind==1?skin.snack_texts:skin.ball_text;
 menu_nine_patch(p,skin,18,29,444,198,0xffe8e5dd);
 if(!seen){centered(p,skin,240,90,30,ink,"?");centered(p,skin,240,143,18,ink,"Undiscovered");}
 else{
  if(kind==1)snack_icon(p,skin,id,39,65,64);else ball_icon(p,skin,id,39,65,64);
  const auto& info=kind==1?snack_info(id):ball_info(id);
  fit(p,skin,282,43,326,20,ink,"%s",*bank.title(id)?bank.title(id):kind==1?snack_name(id):ball_name(id));
  fit(p,skin,282,73,326,13,ink,"%s / %s",set_name(info.set),rarity_name(info.rarity));
  char fallback[512];if(kind==1)std::snprintf(fallback,sizeof(fallback),"%s",snack_text(id));else describe_ability(builtin(id).ability,1,fallback,sizeof(fallback));
  paragraph(p,skin,123,99,318,8,*bank.text(id)?bank.text(id):fallback,ink,14,0,false,true);
  fit(p,skin,72,148,86,12,ink,"Value %d",info.base_score);
  if(kind!=1)fit(p,skin,72,169,86,12,ink,"Level %d",info.start_level);
 }
 footer(p,skin,"{lr} previous / next   {cross} / {circle} close","Collection descriptions show the original starting-level effect");
}
struct TutorialFocus {float x=0,y=0,w=0,h=0;bool circle=false,valid=false;};
static TutorialFocus tutorial_focus(const Game& g,const char* id){
    TutorialFocus f;auto box=[&](float x,float y,float w,float h){f.x=x;f.y=y;f.w=w;f.h=h;f.valid=true;};
    auto ball=[&](Screen p,float radius){box(p.x-radius,p.y-radius,2*radius,2*radius);f.circle=true;};
    if((!std::strcmp(id,"intro-2") || !std::strcmp(id,"intro-3")) && g.world.count)ball(project(g.world.balls[0].p),20);
    else if(!std::strcmp(id,"pocket-2"))box(185,104,80,30);
    else if(!std::strcmp(id,"pocket-3") || !std::strcmp(id,"intro-4"))box(191,133,68,22);
    else if(!std::strcmp(id,"shots-1") || !std::strcmp(id,"last-shot"))box(203,1,92,17);
    else if(!std::strncmp(id,"lose-round-",11))box(425,1,51,18);
    else if(!std::strcmp(id,"first-cube")){for(unsigned n=0;n<g.cubes.size();++n)if(g.cubes[n].id){box(397+26.0f*(n%3),41+38.0f*(n/3),24,24);break;}}
    else if(!std::strcmp(id,"shop-play"))box(290,207,112,28);
    else if(!std::strcmp(id,"shop-intro-2") || !std::strcmp(id,"shop-intro-4"))box(268,74,125,126);
    else if(!std::strcmp(id,"shop-intro-3") || !std::strcmp(id,"shop-daily-3"))box(14,102,188,74);
    else if(!std::strncmp(id,"shop-reserve-",13))box(415,151,48,82);
    else if(!std::strcmp(id,"shop-daily-2"))box(230,109,52,95);
    else if(!std::strcmp(id,"mix-1"))box(231,61,234,101);
    else if(!std::strncmp(id,"tickets-",8))box(230,163,52,23);
    else if(!std::strcmp(id,"passives-2"))box(15,127,188,59);
    else if(!std::strcmp(id,"passives-3"))box(220,70,252,125);
    else if(!std::strcmp(id,"daily-1"))box(8,246,464,16);
    else if(!std::strcmp(id,"pocket-1") || !std::strcmp(id,"pocket") || !std::strcmp(id,"pocket-mult")){
        const Ball* latest=nullptr;for(unsigned n=1;n<g.world.count;++n){const Ball& b=g.world.balls[n];if(b.last_pocket_tick!=0xffffffffu && (!latest || b.last_pocket_tick>latest->last_pocket_tick))latest=&b;}
        if(latest){const Pocket* pocket=g.world.pocket(latest->pocket_index);if(pocket)ball(project(pocket->p),14);}
    }
    return f;
}
static void tutorial_spotlight(Painter& p,const TutorialFocus& f){
    const uint32_t dim=0x66000000;
    if(!f.valid){p.rect(0,0,480,272,dim);return;}
    const float x=std::max(0.0f,f.x),y=std::max(0.0f,f.y),right=std::min(480.0f,f.x+f.w),bottom=std::min(272.0f,f.y+f.h);
    if(y>0)p.rect(0,0,480,y,dim);
    if(bottom<272)p.rect(0,bottom,480,272-bottom,dim);
    if(x>0)p.rect(0,y,x,bottom-y,dim);
    if(right<480)p.rect(right,y,480-right,bottom-y,dim);
    if(f.circle)selection_ring(p,f.x+f.w/2,f.y+f.h/2,f.w/2,gold);
    else{p.line(f.x,f.y,f.x+f.w,f.y,gold);p.line(f.x+f.w,f.y,f.x+f.w,f.y+f.h,gold);p.line(f.x+f.w,f.y+f.h,f.x,f.y+f.h,gold);p.line(f.x,f.y+f.h,f.x,f.y,gold);}
}
void paint_scene(Painter &p, const Skin &skin, const Game &g,
                 unsigned physics_us, unsigned free_bytes, unsigned draw_us) {
  if (g.phase == Phase::Menu)
    {if(g.daily_open)daily_menu_scene(p,skin,g);else if(g.profile_open)profile_scene(p,skin,g);else menu(p, skin, g);}
  else if(g.phase==Phase::DailyResult)daily_result_scene(p,skin,g);
  else if (g.phase == Phase::Shop) {
    if (g.round_summary)
      round_result(p, skin, g);
    else {
      shop(p, skin, g);
      if (g.offer_details) {
        unsigned id = g.offers[g.selection];
        const auto &info = ball_info(id);
        OwnedBall offer = g.offer_item(g.selection);
        p.rect(0, 0, 480, 272, 0xb0000000);
        frame(p, 48, 30, 384, 205, 0xffa6ada0, 0xffe7e6e2);
        ball_icon(p, skin, id, 65, 70, 48);
        fit(p, skin, 263, 43, 300, 20, 0xff333333, "%s", ball_name(id));
        fit(p, skin, 267, 70, 282, 13, 0xff555555, "%s / %s",
            rarity_name(info.rarity), set_name(info.set));
        fit(p, skin, 90, 131, 74, 13, 0xff333333, "Level %u",
            unsigned(offer.level));
        fit(p, skin, 90, 151, 74, 13, 0xff333333, "Value %d", offer.score);
        fit(p, skin, 90, 172, 74, 14,
            g.money >= item_price(offer) ? 0xff36783c : 0xff3333bb, "Price %d$",
            item_price(offer));
        char body[512];
        item_description(body, sizeof(body), offer);
        paragraph(p, skin, 139, 98, 274, 7, body, 0xff333333, 14);
        footer(p, skin, "{cross} / {circle} close   {square} choose merge target");
      }
      if (g.roster_details) {
        const OwnedBall &ball = g.inventory[g.inventory_selection];
        char name[96], value[24], body[512];
        item_name(name, sizeof(name), ball);
        std::snprintf(value, sizeof(value), "%d", ball.score);
        p.rect(0, 0, 480, 272, 0xb0000000);
        frame(p, 48, 30, 384, 205, 0xffa6ada0, 0xffe7e6e2);
        mixed_icon(p, skin, ball.id, ball.mixed_with, 65, 70, 48);
        fit(p, skin, 263, 43, 300, 20, 0xff333333, "%s", name);
        char metadata[128];
        if (ball.mixed_with &&
            ball_info(ball.id).set != ball_info(ball.mixed_with).set)
          std::snprintf(metadata, sizeof(metadata), "Mixed / %s + %s",
                        set_name(ball_info(ball.id).set),
                        set_name(ball_info(ball.mixed_with).set));
        else
          std::snprintf(metadata, sizeof(metadata), "%s / %s",
                        ball.mixed_with ? "Mixed"
                                        : rarity_name(rarity(ball.id)),
                        set_name(ball_info(ball.id).set));
        fit(p, skin, 267, 70, 282, 13, 0xff555555, "%s", metadata);
        fit(p, skin, 90, 131, 74, 14, 0xff333333, "Level %u",
            unsigned(ball.level));
        fit(p, skin, 90, 151, 74, 14, 0xff333333, "Value %s", value);
        item_description(body, sizeof(body), ball);
        paragraph(p, skin, 139, 98, 274, 7, body, 0xff333333, 14);
        footer(p, skin, "{dpad} browse   {cross} / {circle} close   {select} move / swap");
      }
      if (g.passive_details) {
        p.rect(0, 0, 480, 272, 0xb0000000);
        frame(p, 90, 39, 300, 190, 0xffa6ada0, 0xffe7e6e2);
        unsigned id = g.snacks[g.passive_selection];
        snack_icon(p, skin, id, 111, 66, 40);
        fit(p, skin, 270, 52, 210, 19, 0xff333333, "%s", snack_name(id));
        paragraph(p, skin, 169, 91, 204, 7, snack_text(id), 0xff333333, 15);
        centered(p,skin,240,201,15,0xff36783c,"{cross} / {circle} close");
        footer(p,skin,"{cross} / {circle} close");
      }
    }
  } else if (g.phase == Phase::Lost || g.phase == Phase::RunWon) {
    floor(p, skin);
    if (skin.table.pixels)
      p.sprite(skin.table, 42, 18, 396, 218);
    p.rect(42, 18, 396, 218, 0x88000000);
    menu_nine_patch(p,skin,71,31,338,201,0xffe8e5dd);
    centered(p, skin, 240, 48, 24,
             g.phase == Phase::RunWon ? 0xff4a7c38 : 0xff3333b5,
             g.phase == Phase::RunWon ? "RUN COMPLETE" : "RUN OVER");
    char score[24], money[24], earned[24];
    compact(score, sizeof(score), g.score);
    compact(money, sizeof(money), g.money);
    compact(earned, sizeof(earned), g.round_earned);
    if(g.endless)fit(p,skin,240,91,252,18,0xff333333,"Round %d   Score %s",g.round,score);
    else fit(p, skin, 240, 91, 252, 18, 0xff333333, "Round %d / %u   Score %s",
        g.round, rounds_for_difficulty(g.difficulty), score);
    fit(p, skin, 240, 121, 252, 18, 0xff333333, "Wallet %s$", money);
    fit(p, skin, 240, 151, 252, 18, 0xff407c36, "Money earned +%s$", earned);
    menu_nine_patch(p,skin,98,182,284,28,0xff55b653);
    centered(p,skin,240,187,16,white,"{cross} RETURN TO MENU");
    if(g.phase==Phase::RunWon && g.profile.endless_unlocked())fit(p,skin,240,211,258,13,0xff407c36,"{triangle}  Continue in endless");
  } else
    table(p, skin, g);
  if(g.phase==Phase::Simulating || g.phase==Phase::Aiming || g.phase==Phase::Placement) {
    for(const auto& a:g.actions)if(a.kind==ActionKind::Tiger){
      const float elapsed=a.flags?1.6f+(float(a.repeats)-float(a.due-g.world.metrics.ticks))*tick_dt:1.6f-float(a.due-g.world.metrics.ticks)*tick_dt;
      if(elapsed<0 || elapsed>1.604f)continue;
      float alpha=elapsed<.0667f?elapsed/.0667f*.36f:elapsed<1.433f?.36f+.23f*(elapsed-.0667f)/1.3663f:.59f*std::max(0.0f,(1.5923f-elapsed)/.1593f);
      float height=elapsed<1.433f?125+75*elapsed/1.433f:200+100*(elapsed-1.433f)/.17f;
      if(skin.tiger_glow.pixels){float glow=std::max(0.0f,1-std::fabs(elapsed-.8667f)/.6667f);float size=64+190*std::max(0.0f,(elapsed-.2333f)/1.3f);p.sprite(skin.tiger_glow,240-size/2,136-size/2,size,size,(uint32_t(glow*100)<<24)|0x0066ccff);}
      if(skin.tiger_cut_in.pixels){float width=height*(1756.0f/1996.0f);p.sprite(skin.tiger_cut_in,240-width/2,136-height/2,width,height,(uint32_t(std::max(0.0f,std::min(1.0f,alpha))*255)<<24)|0x00ffffff);}
    }
  }
  if(g.phase==Phase::Shop && g.npc_time>0 && !g.round_summary && !g.offer_details && !g.roster_details && !g.passive_details && !g.paused){
   unsigned row=g.npc_group*4+g.npc_variant;
   const char* line=skin.dialogue_text.text(row);if(!*line)line=skin.dialogue_text.text(g.npc_group*4);
   if(*line){
    char full[256];std::snprintf(full,sizeof(full),"%s",line);
    const char* token=std::strstr(full,"{value}");
    if(token){char tail[256];std::snprintf(tail,sizeof(tail),"%s",token+7);unsigned value=g.cocktail_tickets;
     if(g.npc_group==15){value=1;while(value<10){unsigned n=(unsigned(g.round)+value)%10;if(n==3||n==6||n==9)break;++value;}}
     std::snprintf(full+(token-full),sizeof(full)-unsigned(token-full),"%u%s",value,tail);}
    if(const char* set_token=std::strstr(full,"{set}")){char tail[256];std::snprintf(tail,sizeof(tail),"%s",set_token+5);std::snprintf(full+(set_token-full),sizeof(full)-unsigned(set_token-full),"%s%s",set_name(g.shop_sets[1]==ItemSet::NONE?g.shop_sets[0]:g.shop_sets[1]),tail);}
    unsigned length=unsigned(std::strlen(full)),page=length>70?unsigned((4.25f-g.npc_time)/2.1f):0,start=page?std::min(70u,length):0;
    char visible[80];unsigned n=std::min(70u,length-start);std::memcpy(visible,full+start,n);visible[n]=0;
    p.rect(0,253,480,19,ink);fit(p,skin,240,255,462,12,0xffb6dccb,"%s",visible);
   }
  }
  if(g.achievement_toast<achievement_count && g.achievement_toast_time>0 && !g.paused){
    const auto& rule=achievement_rule(g.achievement_toast);
    float elapsed=3.25f-g.achievement_toast_time;
    float y=g.reduced_motion?24:24-std::max(0.0f,1-elapsed/.25f)*78+std::sin(elapsed*5)*1.4f;
    if(!g.reduced_motion && g.achievement_toast_time<.25f)y-=(1-g.achievement_toast_time/.25f)*78;
    uint32_t panel=rule.visibility==1?0xffffe3f1:0xfff5f5f5;
    p.rect(88,y+3,306,68,0x66000000);frame(p,87,y,306,68,0xff555555,panel);
    char title[100];std::snprintf(title,sizeof(title),"%s",rule.id);for(char* c=title;*c;++c)if(*c=='_' || *c=='-')*c=' ';
    const AchievementText* original=skin.achievement_text?&skin.achievement_text[g.achievement_toast]:nullptr;
    const char* heading=original && *original->title?original->title:rule.visibility==1?"SECRET ACHIEVEMENT":"ACHIEVEMENT UNLOCKED";
    if(!original || !*original->description)fit(p,skin,240,y+5,290,12,0xff333333,"%s",heading);
    if(original && *original->description){
      fit(p,skin,240,y+3,290,10,0xff36783c,"ACHIEVEMENT UNLOCKED");
      fit(p,skin,240,y+17,288,14,0xff333333,"%s",original->title);
      paragraph(p,skin,98,y+35,284,2,original->description,0xff333333,11,0,false,true);
    }
    else fit(p,skin,240,y+24,290,14,0xff333333,"%s",title);
  }
  if(g.tutorial.enabled && g.tutorial.showing()){
    const auto& rule=tutorial_rules[g.tutorial.active];const TutorialFocus focus=rule.highlight?tutorial_focus(g,rule.id):TutorialFocus();tutorial_spotlight(p,focus);
    float panel_x=20,panel_y=167,panel_w=440,panel_h=91;bool side=false;
    if(focus.valid && focus.circle && focus.y>=150)panel_y=34;
    if(focus.valid && !focus.circle && focus.y>=30){
        if(focus.x>=218){panel_x=10;panel_y=62;panel_w=198;panel_h=184;side=true;}
        else if(focus.x+focus.w<=218){panel_x=228;panel_y=62;panel_w=242;panel_h=184;side=true;}
        else if(focus.y>=155)panel_y=34;
    }
    frame(p,panel_x,panel_y,panel_w,panel_h,0xff555555,0xfff5f5f5);
    const AchievementText* original=skin.tutorial_text?&skin.tutorial_text[g.tutorial.active]:nullptr;
    const float speaking_time=std::max(0.0f,g.tutorial.elapsed-rule.delay);
    const bool portrait=rule.portrait && rule.portrait<=skin.tutorial_portraits.size() && skin.tutorial_portraits[rule.portrait-1].pixels;
    if(portrait)p.sprite_rotated(skin.tutorial_portraits[rule.portrait-1],side?panel_x+panel_w/2:panel_x+44,panel_y+43,80,80*224.0f/248.0f,g.reduced_motion?0:std::sin(speaking_time*4)*.045f);
    char visible[384];std::snprintf(visible,sizeof(visible),"%s",original && *original->description?original->description:rule.id);
    unsigned revealed=std::min<unsigned>(static_cast<unsigned>(std::strlen(visible)),static_cast<unsigned>(speaking_time/.015f));
    // Never show a token's source spelling while the typewriter is inside it.
    for(unsigned i=0;i<revealed;++i)if(visible[i]=='{'){unsigned n=0;button_token(visible+i,n);if(n && i+n>revealed){revealed=i;break;}if(n)i+=n-1;}
    visible[revealed]=0;
    float text_x=panel_x+(side?12:portrait?82:12),text_y=panel_y+(side && portrait?86:8),text_w=panel_w-(side?24:portrait?94:24);
    unsigned rows=side?static_cast<unsigned>((panel_y+panel_h-25-text_y)/13):4;
    paragraph(p,skin,text_x,text_y,text_w,rows,visible,0xff333333,12,0,false,true);
    fit(p,skin,panel_x+panel_w/2,panel_y+panel_h-17,panel_w-24,11,0xff333333,"%s",g.tutorial.ready()?"{cross} continue":"Read on...");
  }
  if (g.paused) {
    p.rect(0,0,480,272,0xac000000);
    menu_nine_patch(p,skin,58,22,364,212,0xffe8e5dd);
    centered(p,skin,240,31,22,ink,"SETTINGS");
    const char* rows[]={"Music","Sound effects","Sound","Fine aim speed","Analog aiming","Analog dead zone","Tutorial hints","Reduced motion","How to play","Return to menu"};
    for(unsigned i=0;i<10;++i){
     float y=59+16.0f*i;
     if(g.settings_selection==i)p.line(79,y+15,401,y+15,0xff36783c);
     ui_text(p,skin,81,y,13,g.settings_selection==i?0xff36783c:ink,"%s",rows[i]);
     if(i<2)fit(p,skin,366,y,72,13,ink,"%u%%",i?g.sfx_percent:g.music_percent);
     else if(i==3)fit(p,skin,366,y,72,13,ink,"%u%%",g.aim_percent);
     else if(i==5)fit(p,skin,366,y,72,13,ink,"%u%%",g.analog_deadzone);
     else if(i>=8)fit(p,skin,366,y,72,13,ink,"{cross}");
     else fit(p,skin,366,y,72,13,ink,"%s",(i==2?!g.audio_muted:i==4?g.analog_enabled:i==6?g.tutorial_hints:g.reduced_motion)?"ON":"OFF");
    }
    centered(p,skin,240,223,10,ink,"{ud} select   {lr} adjust   {cross} select");
    char seed_label[96];std::snprintf(seed_label,sizeof(seed_label),"Seed %u%s",unsigned(g.run_seed),g.seeded_run?" / SEEDED PRACTICE":"");
    footer(p,skin,"{square} mute   {start} / {circle} return",g.phase==Phase::Menu?"Settings are saved automatically":seed_label);
  }
  if(g.seed_open)seed_scene(p,skin,g);
  if(g.tutorial_replay)replay_scene(p,skin,g);
  if(g.gallery_details)gallery_detail_scene(p,skin,g);
  if (g.debug_hud && !g.profile_open && !g.daily_open && g.phase!=Phase::DailyResult) {
    p.rect(0, 264, 480, 8, ink);
    text(p, 6, 264, muted, "CPU %uus DRAW %uus FREE %uK SOLVER %u DROP %u",
         physics_us, draw_us, free_bytes / 1024,
         unsigned(g.world.metrics.solver_exhaustions),
         unsigned(g.world.events.dropped));
  }
}
} // namespace up

