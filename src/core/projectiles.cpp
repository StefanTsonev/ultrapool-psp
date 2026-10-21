#include "ultrapool/core.hpp"
#include <cmath>
namespace up {
bool Game::spawn_energy(Vec position,Vec impulse,unsigned exception) {
    if(cap_rewarded || !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(impulse.x) || !std::isfinite(impulse.y))return false;
    unsigned live=0;for(unsigned i=0;i<world.count;++i)live+=world.balls[i].active && world.balls[i].energy;
    if(live>=10)return false;
    Ball projectile;projectile.energy=true;projectile.ignore_pockets=true;projectile.def=builtin(id_of(BallKey::ENERGY_BALL));projectile.def.base_score=0;
    projectile.base_value=0;projectile.level=1;projectile.p=position;projectile.v=impulse*(1/projectile.def.mass);
    if(exception<world.count){projectile.collision_exception=static_cast<uint16_t>(exception);projectile.exception_generation=world.balls[exception].generation;}
    if(!world.add(projectile)){++spawn_rejections;return false;}
    return true;
}
}
