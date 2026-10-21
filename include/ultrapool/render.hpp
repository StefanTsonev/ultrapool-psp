#pragma once
#include "ultrapool/core.hpp"
namespace up {
struct Screen { float x,y; };
constexpr float table_scale=0.31f;
constexpr float passive_draw_dx=-196,passive_draw_dy=218;
inline Screen project(Vec p) { return Screen{240-p.y*table_scale,128+p.x*table_scale}; }
class Renderer {
public:
    void init();
    void draw(const Game& game, unsigned physics_us, unsigned free_bytes);
    void shutdown();
};
Input read_input();
}