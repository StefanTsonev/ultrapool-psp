#include "ultrapool/core.hpp"
#include <cstdio>
#include <cstdlib>
#include <memory>
using namespace up;
namespace {
unsigned checks;
void expect(bool good,const char* text){++checks;if(std::getenv("ULTRAPOOL_TRACE_TESTS"))std::fprintf(stderr,"Projectile check %u: %s\n",checks,text);if(!good){std::fprintf(stderr,"FAIL PROJECTILES: %s\n",text);std::exit(1);}}
bool near(float a,float b,float epsilon=.02f){return std::fabs(a-b)<epsilon;}
Ball body(Vec p,Vec v=Vec()){Ball b;b.def=builtin(id_of(BallKey::BREAD));b.def.damping=0;b.p=p;b.v=v;return b;}
Ball energy(Vec p,Vec v=Vec()){Ball b=body(p,v);b.def=builtin(id_of(BallKey::ENERGY_BALL));b.def.damping=0;b.energy=true;b.ignore_pockets=true;b.base_value=0;return b;}
void arena(Game& g){g=Game();g.phase=Phase::Aiming;g.target=1000000;g.world.pockets_enabled=false;Ball cue=body(Vec(0,300));cue.cue=true;g.world.add(cue);}
unsigned add(Game& g,BallKey key,Vec p,unsigned lv=1){Ball b=body(p);b.def=builtin(id_of(key));b.level=static_cast<uint8_t>(lv);g.world.add(b);return g.world.last_added;}
void pocket(Game& g,unsigned i){Ball& b=g.world.balls[i];b.active=false;b.v=Vec();++g.world.metrics.ticks;g.world.events.push(EventKind::Pocketed,i,6,g.world.metrics.ticks,b.generation);g.dispatch();}
unsigned projectiles(const Game& g){unsigned count=0;for(unsigned i=0;i<g.world.count;++i)count+=g.world.balls[i].active && g.world.balls[i].energy;return count;}
unsigned first_energy(const Game& g){for(unsigned i=0;i<g.world.count;++i)if(g.world.balls[i].active && g.world.balls[i].energy)return i;return max_balls;}
unsigned star_carriers(const Game& g,unsigned lv){unsigned count=0;for(const auto& a:g.actions)if(a.kind==ActionKind::Spawn && a.snapshot.def.id==id_of(BallKey::STAR)){++count;expect(a.snapshot.level==lv && a.snapshot.def.base_score==default_item(id_of(BallKey::STAR),lv).score,"Asteroid Star retains component level and default value");}return count;}
}
unsigned projectile_tests(){
 checks=0;std::unique_ptr<World> physics(new World);World& w=*physics;
 w.pockets_enabled=false;w.stop_speed=0;w.speed_limit=0;
 Ball cue=body(Vec());cue.cue=true;w.add(cue);w.add(energy(Vec(),Vec(100,0)));w.step();expect(near(w.balls[0].p.x,0) && near(w.balls[1].v.x,100),"energy ignores cue overlap and impulse");
 expect(!w.pair_collides(0,1) && !w.pair_collides(1,0),"cue exception is symmetric");
 w=World();w.pockets_enabled=false;w.add(body(Vec()));Ball projectile=energy(Vec(),Vec(100,0));projectile.collision_exception=0;projectile.exception_generation=w.balls[0].generation;w.add(projectile);w.step();expect(near(w.balls[0].p.x,0) && near(w.balls[1].p.x,100*tick_dt),"source exception suppresses overlap repair and sweep");
 ++w.balls[0].generation;expect(w.pair_collides(0,1),"source exception does not attach to a replacement generation");
 w=World();w.pockets_enabled=false;w.stop_speed=0;w.speed_limit=0;w.add(energy(Vec(-100,0),Vec(40000,0)));w.add(body(Vec()));w.step();expect(near(w.balls[1].v.x,40000,.1f) && near(w.balls[0].v.x,0),"energy swept contact transfers physical momentum without tunneling");
 Event event;expect(w.events.pop(event) && event.kind==EventKind::Collision,"energy-to-ball contact emits ordinary HIT event");
 w=World();w.pockets_enabled=false;w.add(energy(Vec(-17,0),Vec(100,0)));w.add(energy(Vec(17,0),Vec(-100,0)));w.step();expect(near(w.balls[0].v.x,100) && near(w.balls[1].v.x,-100) && !w.events.pop(event),"energy projectiles pass through each other according to original layer masks");
 w=World();w.pockets_enabled=false;w.add(energy(Vec()));for(unsigned n=0;n<179;++n)w.step();expect(w.balls[0].active,"resting energy survives its one-second grace period");w.step();expect(!w.balls[0].active && w.balls[0].gone && !w.events.pop(event),"resting energy expires silently at grace-period end");
 w=World();w.pockets_enabled=false;w.add(energy(Vec(),Vec(5,0)));w.step();expect(near(w.balls[0].v.x,5),"energy bypasses ordinary low-speed stopping before grace expiry");
 w.balls[0].energy_age=2;w.balls[0].v=Vec(100,0);w.step();expect(w.balls[0].active,"one second is not a hard projectile lifetime");w.balls[0].v=Vec(9,0);w.step();expect(!w.balls[0].active,"aged projectile dies when speed falls below ten");
 w=World();w.add(energy(w.pockets[0].p,Vec(1,0)));w.step();expect(w.balls[0].active && !w.balls[0].falling && !w.events.pop(event),"energy cannot enter physical pockets during a run (inherited can_be_pocketed guard)");
 w=World();for(auto& p:w.pockets)p.closed=true;w.add(energy(w.pockets[0].p));w.set_pocket_closed(0,false);expect(w.balls[0].active && !w.events.pop(event),"opening a pocket does not capture overlapping energy");
 std::unique_ptr<Game> game(new Game);Game& g=*game;
 arena(g);unsigned comet=add(g,BallKey::COMET,Vec(-100,0),3);unsigned close=add(g,BallKey::BREAD,Vec(0,0));add(g,BallKey::BREAD,Vec(0,200));add(g,BallKey::TUNGSTEN_BALL,Vec(-80,0));unsigned calculator=add(g,BallKey::CALCULATOR,Vec(150,-200));int value=g.world.balls[calculator].def.base_score;pocket(g,comet);
 expect(projectiles(g)==1,"Comet produces exactly one immediate energy projectile");unsigned shot=first_energy(g);const Ball& e=g.world.balls[shot];expect(e.owner==-1 && e.def.base_score==0 && e.level==1 && e.def.mass==1 && e.def.radius==17,"energy has independent temporary identity, zero score and original physics dimensions");expect(near(e.v.x,1250) && near(e.v.y,0),"Comet chooses closest targetable ball and uses 500+250 times level momentum");expect(e.collision_exception==comet && e.exception_generation==g.world.balls[comet].generation,"Comet excludes its source generation");expect(g.world.balls[calculator].def.base_score==value,"energy does not emit normal SpawnAnother or Calculator buffs");expect(!targetable_ball(e),"energy is excluded from ordinary targeting");
 AimPrediction aim=predict_aim(g.world,Vec(0,-1),1);expect(aim.hit!=int(shot),"cue prediction ignores energy");
 g.world.balls[shot].active=false;g.world.balls[close].active=false;
 arena(g);comet=add(g,BallKey::COMET,Vec());pocket(g,comet);expect(projectiles(g)==0,"Comet emits no projectile without a live target");
 arena(g);comet=add(g,BallKey::COMET,Vec(-100,0),1);add(g,BallKey::BREAD,Vec(0,0));for(unsigned n=0;n<14;++n)pocket(g,comet);expect(projectiles(g)==10,"projectile cap is ten independently of normal spawn population");
 arena(g);comet=add(g,BallKey::COMET,Vec(-100,0),2);add(g,BallKey::BREAD,Vec(0,0));g.world.balls[comet].flaming=true;pocket(g,comet);for(unsigned n=0;n<19;++n)g.update(tick_dt);expect(projectiles(g)==2,"flaming Comet repeats energy generation through delayed effect queue");
 arena(g);unsigned asteroid=add(g,BallKey::ASTEROID,Vec(0,0),4);g.world.add(energy(Vec(-34,0),Vec(100,0)));g.world.step();g.dispatch();expect(star_carriers(g,4)==1,"Asteroid hit by energy queues one Star");
 arena(g);asteroid=add(g,BallKey::BREAD,Vec(),2);g.world.balls[asteroid].mixed_with=id_of(BallKey::ASTEROID);g.world.add(energy(Vec(-34,0),Vec(100,0)));g.world.step();g.dispatch();expect(star_carriers(g,2)==1,"mixed secondary Asteroid receives energy HIT effect");
 arena(g);asteroid=add(g,BallKey::ASTEROID,Vec(),2);add(g,BallKey::BREAD,Vec(-34,0));g.world.balls[2].v=Vec(100,0);g.world.step();g.dispatch();expect(star_carriers(g,2)==0,"ordinary collision does not trigger Asteroid energy-only effect");
 pocket(g,asteroid);expect(star_carriers(g,2)==1,"Asteroid also spawns Star on Pocket");
 arena(g);comet=add(g,BallKey::COMET,Vec(-100,0),2);g.world.balls[comet].mixed_with=id_of(BallKey::ASTEROID);add(g,BallKey::BREAD,Vec());pocket(g,comet);expect(projectiles(g)==1 && star_carriers(g,2)==1,"mixed Comet/Asteroid executes both Pocket components");
 arena(g);unsigned blueprint=add(g,BallKey::BLUEPRINT,Vec());g.world.add(energy(Vec(-34,0),Vec(100,0)));g.world.step();g.dispatch();expect(g.world.balls[blueprint].def.id==id_of(BallKey::BLUEPRINT),"Blueprint refuses energy identity");
 arena(g);unsigned astronaut=add(g,BallKey::ASTRONAUT,Vec());g.world.add(energy(Vec(-34,0),Vec(100,0)));shot=g.world.last_added;g.world.step();g.dispatch();expect(!g.world.balls[shot].active && g.world.balls[shot].def.base_score==0,"inherited zero-value upgrade breaks energy without granting value (Astronaut HIT)");
 arena(g);unsigned rainbow=add(g,BallKey::RAINBOW,Vec(-100,-100),2);unsigned bread=add(g,BallKey::BREAD,Vec(100,-100));g.world.add(energy(Vec(0,100)));g.world.add(energy(Vec(100,100)));pocket(g,rainbow);unsigned buffs=0;for(const auto& a:g.actions)if(a.kind==ActionKind::BuffBall){++buffs;expect(a.target==bread,"global value buffs never select energy");}expect(buffs==1,"duplicate energy does not block Rainbow identity uniqueness");
 arena(g);unsigned conveyor=add(g,BallKey::CONVEYOR,Vec());g.world.add(energy(Vec(-100,0)));shot=g.world.last_added;pocket(g,conveyor);expect(g.world.balls[shot].conveyor_time==0,"Conveyor main-object list excludes energy");expect(!g.world.moving() && g.world.energy_active() && !g.pending_actions(),"energy lifecycle runs independently of main-ball movement and carrier settlement");g.update(tick_dt);expect(g.world.balls[shot].energy_age>0,"energy still updates while aiming and no main ball moves");
 arena(g);g.world.add(energy(Vec(-100,0),Vec(100,0)));shot=g.world.last_added;int oil=g.spawn_pickup(PickupKind::Oil,Vec(-100,0));g.update(tick_dt);expect(oil>=0 && g.pickups[oil].charges==4,"energy does not consume table pickup charges");
 for(unsigned n=1;n<max_balls-1;++n){Ball old=body(Vec());old.active=false;g.world.add(old);}unsigned retired=g.world.balls[shot].generation;g.world.balls[shot].active=false;Ball replacement=body(Vec());g.world.add(replacement);expect(g.world.balls[g.world.last_added].generation>retired && !g.world.balls[g.world.last_added].energy,"reused projectile storage resets classification and advances generation");
 std::printf("Projectile checks: %u\n",checks);return checks;
}
