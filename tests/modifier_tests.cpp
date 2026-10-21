#include "ultrapool/core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
using namespace up;
namespace {
unsigned checks=0;
void verify(bool ok,const char* text) {++checks;if(!ok){std::fprintf(stderr,"FAIL MODIFIER: %s\n",text);std::exit(1);}}
Game arena(){Game g;g.phase=Phase::Aiming;g.world.pockets_enabled=false;Ball cue;cue.cue=true;cue.p=Vec(-200,210);g.world.add(cue);return g;}
unsigned add(Game& g,BallKey key,int value=1,unsigned level=1,unsigned second=0){Ball b;b.def=builtin(id_of(key));b.def.base_score=value;b.level=static_cast<uint8_t>(level);b.mixed_with=static_cast<uint16_t>(second);b.p=Vec(-150+60*float(g.world.count),0);g.world.add(b);return g.world.last_added;}
void pocket(Game& g,unsigned index,unsigned p=6){++g.world.metrics.ticks;g.world.balls[index].active=false;g.world.events.push(EventKind::Pocketed,index,p,g.world.metrics.ticks,g.world.balls[index].generation);g.dispatch();}
void resolve(Game& g){for(unsigned i=0;i<1800&&g.pending_actions();++i)g.update(tick_dt);verify(!g.pending_actions(),"modifier carriers finish");}
}
unsigned modifier_tests(){
 checks=0;Ball b;b.v=Vec(20,10);set_ball_weight(b,Weight::Heavy);set_ball_locked(b,true);
 verify(b.locked&&b.def.mass==10000&&b.v.length()==0&&std::fabs(b.def.radius-20.4f)<.001f,"lock stops motion and overrides mass without changing weight size");
 set_ball_weight(b,Weight::Light);verify(b.def.mass==10000&&b.unlocked_mass==.5f,"weight changes behind lock retain locked mass");set_ball_locked(b,false);verify(b.def.mass==.5f&&!b.locked,"unlock restores updated weight mass");
 verify(set_ball_shield(b,true)&&b.shielded,"unbroken ball accepts shield");b.shield_broken=true;verify(!set_ball_shield(b,true),"broken shield cannot be reapplied");
 Game g=arena();unsigned a=add(g,BallKey::B_1_BALL,3);g.world.balls[a].def.ability=Respawn;uint32_t generation=g.world.balls[a].generation;Vec old=g.world.balls[a].p;pocket(g,a);
 verify(g.score==3&&g.world.balls[a].active&&g.world.count==2,"Respawn scores and revives same ball rather than spawning a copy");verify(g.world.balls[a].generation==generation&&(g.world.balls[a].p-old).length()>1,"same-ball respawn keeps generation and moves to free position");
 g.world.events.push(EventKind::Pocketed,a,6,g.world.metrics.ticks,generation);g.dispatch();verify(g.score==3,"same tick duplicate pocket is suppressed after respawn");pocket(g,a);verify(g.score==6&&g.world.balls[a].active,"later pocket scores and respawns again");
 g=arena();a=add(g,BallKey::BASKETBALL,1);g.world.balls[a].shielded=true;pocket(g,a,0);
 verify(g.score==2&&g.world.balls[a].active&&g.world.balls[a].shield_broken&&!g.world.balls[a].shielded,"ball shield is consumed while pocket score remains");verify(g.world.balls[a].def.base_score==3,"shield respawn precedes score listener, so Basketball sees its own pocket score");
 verify(!set_ball_shield(g.world.balls[a],true),"shield-broken state survives respawn");pocket(g,a,0);verify(!g.world.balls[a].active&&g.score==8,"second pocket removes shield-broken ball");
 g=arena();a=add(g,BallKey::B_1_BALL,2);g.world.pockets[0].shielded=true;pocket(g,a,0);verify(g.world.balls[a].active&&!g.world.pockets[0].shielded&&g.world.balls[a].shield_broken,"pocket shield protects one eligible ball and is consumed");
 g.world.pockets[0].shielded=true;pocket(g,a,0);verify(!g.world.balls[a].active&&g.world.pockets[0].shielded,"shield-broken ball cannot consume another pocket shield");
 g=arena();a=add(g,BallKey::B_1_BALL,2);g.world.balls[a].shielded=true;g.world.pockets[0].shielded=true;pocket(g,a,0);verify(g.world.pockets[0].shielded,"personal shield takes precedence over pocket shield");
 g=arena();a=add(g,BallKey::BOTTLE,2,3);pocket(g,a);verify(!g.world.pockets[0].shielded&&g.pending_actions(),"Bottle shield and bonus travel before applying");resolve(g);unsigned protected_count=0;int bonus=0;for(const auto& p:g.world.pockets){protected_count+=p.shielded;bonus+=p.bonus;}verify(protected_count==1&&bonus==3,"Bottle shields exactly one random pocket and adds level bonus");
 g=arena();a=add(g,BallKey::SAND,2);g.queue_ball_modifier(a,Modifier::Heavy,3,Vec());verify(g.world.balls[a].def.mass==1,"modifier carrier starts delayed");resolve(g);verify(g.world.balls[a].weight==Weight::Heavy&&g.world.balls[a].def.mass==5&&g.world.balls[a].def.base_score==5&&g.score==3,"weight modifier applies before chained buff and Sand score");
 g=arena();a=add(g,BallKey::SHEEP);g.queue_ball_modifier(a,Modifier::Lock,0,Vec());resolve(g);g.queue_impulse(a,Vec(100,0));verify(!g.world.balls[a].locked&&g.world.balls[a].v.length()==0,"shoot request unlocks immediately before delayed impulse");g.update(2*tick_dt);verify(std::fabs(g.world.balls[a].v.x-100)<.01f,"unlocked impulse uses normal mass after two ticks");
 g=arena();a=add(g,BallKey::FRIDGE,2,2);unsigned other=add(g,BallKey::BUSH);unsigned source=add(g,BallKey::B_1_BALL);pocket(g,source);verify(!g.world.balls[other].locked,"Fridge emits delayed lock carrier");resolve(g);verify(g.world.balls[other].locked&&g.world.balls[other].def.base_score==6,"Fridge locks one other then sends twice level buff through Bush");pocket(g,a);resolve(g);verify(!g.world.balls[other].locked,"Fridge own pocket unlocks remaining locked targets");
 g=arena();a=add(g,BallKey::COOTS);other=add(g,BallKey::FISH);set_ball_weight(g.world.balls[other],Weight::Light);set_ball_locked(g.world.balls[other],true);g.world.events.push(EventKind::Collision,a,other,0);g.dispatch();verify(!g.world.balls[other].locked&&g.world.balls[other].def.mass==.5f,"Coots unlocks contacted ball restoring its light mass");
 g=Game();g.inventory[0]=default_item(id_of(BallKey::SHEEP));g.inventory[0].mixed_with=id_of(BallKey::SCREW);g.inventory_count=1;g.begin_round();verify(g.world.balls[1].locked&&g.world.balls[1].def.mass==10000,"Screw mixed component locks during setup Spawn callback");
 std::printf("Modifier and respawn checks: %u\n",checks);return checks;
}
