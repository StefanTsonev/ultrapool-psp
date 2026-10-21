#include "ultrapool/core.hpp"
#include <cstdio>
#include <cstdlib>
using namespace up;
namespace {
unsigned checks=0;
void verify(bool ok,const char* text){++checks;if(!ok){std::fprintf(stderr,"FAIL FIRE: %s\n",text);std::exit(1);}}
Game arena(){Game g;g.phase=Phase::Aiming;g.target=1000;g.world.pockets_enabled=false;Ball cue;cue.cue=true;cue.p=Vec(-200,210);g.world.add(cue);return g;}
unsigned add(Game& g,BallKey key,unsigned level=1,int value=1,unsigned second=0){Ball b;b.def=builtin(id_of(key));b.level=static_cast<uint8_t>(level);b.def.base_score=value;b.mixed_with=static_cast<uint16_t>(second);b.p=Vec(-150+60*float(g.world.count),0);g.world.add(b);return g.world.last_added;}
void wall(Game& g,unsigned a){g.world.events.push(EventKind::Wall,a,0,g.world.metrics.ticks);g.dispatch();}
void pocket(Game& g,unsigned a){g.world.balls[a].active=false;g.world.events.push(EventKind::Pocketed,a,6,g.world.metrics.ticks);g.dispatch();}
void ticks(Game& g,unsigned count){for(unsigned i=0;i<count;++i)g.update(tick_dt);}
void resolve(Game& g){for(unsigned i=0;i<2000&&g.pending_actions();++i)g.update(tick_dt);verify(!g.pending_actions(),"fire actions finish");}
}
unsigned fire_tests(){
 checks=0;Game g=arena();unsigned a=add(g,BallKey::PINBALL,2);g.world.balls[a].flaming=true;wall(g,a);
 verify(g.score==2&&g.pending_actions(),"flaming contact emits one immediate effect and queues a repeat");ticks(g,17);verify(g.score==2,"fire repeat waits full 0.1 seconds");ticks(g,1);verify(g.score==4&&!g.pending_actions(),"repeat executes at tick eighteen exactly once");
 g=arena();a=add(g,BallKey::PINBALL);g.world.balls[a].flaming=true;wall(g,a);g.world.balls[a].level=5;g.world.balls[a].flaming=false;g.transform_ball(a,id_of(BallKey::GOLDEN_BALL));ticks(g,18);
 verify(g.score==2&&g.world.balls[a].def.ability==Gold,"repeat captures original ability and level across identity and fire changes");
 g=arena();a=add(g,BallKey::GOLDEN_BALL,2,3);g.world.balls[a].flaming=true;pocket(g,a);verify(g.money==4&&!g.world.balls[a].flaming,"pocket pays once immediately and extinguishes fire");ticks(g,18);verify(g.money==8&&g.score==3,"pocket repeat still runs on retained dead ball without scoring pocket twice");
 g=Game();g.inventory[0]=default_item(id_of(BallKey::TAKOYAKI));g.inventory[0].mixed_with=id_of(BallKey::CHICKEN);g.inventory_count=1;g.begin_round();unsigned spawns=0;for(const auto& action:g.actions)spawns+=action.kind==ActionKind::Spawn;verify(spawns==1&&g.world.balls[1].flaming,"first Takoyaki component ignites later Chicken during setup Spawn");ticks(g,18);spawns=0;for(const auto& action:g.actions)spawns+=action.kind==ActionKind::Spawn;verify(spawns==2,"later mixed Spawn component observes newly ignited state");
 g=Game();g.inventory[0]=default_item(id_of(BallKey::CHICKEN));g.inventory[0].mixed_with=id_of(BallKey::TAKOYAKI);g.inventory_count=1;g.begin_round();ticks(g,18);spawns=0;for(const auto& action:g.actions)spawns+=action.kind==ActionKind::Spawn;verify(spawns==1,"earlier mixed Spawn component is not retroactively doubled by later ignition");
 g=arena();a=add(g,BallKey::BUSH,2);g.world.balls[a].flaming=true;g.buff_ball(a,3);verify(g.world.balls[a].def.base_score==6,"first Bush buff callback applies before incoming amount");ticks(g,18);verify(g.world.balls[a].def.base_score==8,"delayed Bush callback adds only its captured level, not incoming buff again");
 g=arena();a=add(g,BallKey::PINBALL);unsigned tennis=add(g,BallKey::TENNIS,1,1,id_of(BallKey::BUSH));g.world.balls[tennis].flaming=true;wall(g,a);verify(g.world.balls[tennis].def.base_score==2,"Tennis chain suppression also bypasses Bush on immediate gain");ticks(g,18);verify(g.score==1&&g.world.balls[tennis].def.base_score==3,"flaming Tennis repeats a chain-suppressed gain");
 g=arena();a=add(g,BallKey::GAIA,2,3);g.world.balls[a].flaming=true;unsigned other=add(g,BallKey::B_1_BALL);g.buff_ball(other,1);verify(g.world.balls[other].def.base_score==6,"flaming Gaia contributes twice its level to count effect");
 g=arena();a=add(g,BallKey::INCUBATOR);g.world.balls[a].flaming=true;g.transform_ball(a,id_of(BallKey::GOLDEN_BALL));resolve(g);verify(g.world.balls[a].def.base_score==7,"Incubator repeat retains old transformation callback after identity changes");
 g=arena();a=add(g,BallKey::PINBALL);g.world.balls[a].flaming=true;g.world.balls[a].fleeting=true;wall(g,a);other=add(g,BallKey::B_1_BALL);pocket(g,other);
 verify(!g.world.balls[a].active&&g.world.balls[a].gone&&g.score==2,"another pocket breaks fleeting ball without awarding its value");ticks(g,18);verify(g.score==2,"destroyed fleeting ball cannot execute pending fire repeat");
 g=arena();a=add(g,BallKey::B_1_BALL);g.world.balls[a].fleeting=true;g.world.balls[a].falling=true;g.world.balls[a].fall_remaining=10;other=add(g,BallKey::B_1_BALL);pocket(g,other);verify(g.world.balls[a].active&&!g.world.balls[a].gone,"fleeting cleanup does not break a falling ball");
 g=arena();a=add(g,BallKey::ASTRONAUT,2);other=add(g,BallKey::PINBALL,1,3);g.world.balls[other].star_power=true;g.world.events.push(EventKind::Collision,a,other,0);g.dispatch();verify(g.world.balls[other].def.base_score==5&&g.score==6,"Astronaut buffs first then scores starred recipient; opposite Pinball contact also scores");
 verify(is_star(g.world.balls[other]),"star modifier identifies star ball");Ball star;star.def=builtin(find_ball("STAR"));verify(is_star(star),"original STAR tag counts without modifier");
 g=Game();g.inventory[0]=default_item(id_of(BallKey::FIREBALL));g.inventory[1]=default_item(9);g.inventory[2]=default_item(10);g.inventory[3]=default_item(11);g.inventory_count=4;g.begin_round();unsigned burning=0;for(unsigned i=1;i<g.world.count;++i)burning+=g.world.balls[i].flaming;
 verify(burning==1&&!g.world.balls[1].flaming&&!g.world.balls[4].flaming,"level-one Fireball ignites only one adjacent rack ball, not itself or non-neighbor");
 g=Game();g.inventory[0]=default_item(id_of(BallKey::TAKOYAKI));g.inventory_count=1;g.begin_round();verify(g.world.balls[1].flaming,"Takoyaki ignites on setup Spawn");

 verify((ball_info(id_of(BallKey::CHICKEN)).events&signal_bit(Signal::Spawn)) && (ball_info(id_of(BallKey::CHICKEN)).events&signal_bit(Signal::Pocket)),"Chicken registers spawn and pocket effects from source lists");
 verify(ability_signals(Screw)==signal_bit(Signal::Spawn) && ability_signals(Takoyaki)==signal_bit(Signal::Spawn),"Screw and Takoyaki have no reachable pocket callback");
 for(unsigned id=0;id<content_count;++id) if(ball_supported(id)) verify(ability_signals(builtin(id).ability)==ball_info(id).events,"implemented ability mask agrees with factual resource registration");
 g=arena();a=add(g,BallKey::STAR,1,2);unsigned first=add(g,BallKey::STAR,1,3);unsigned second=add(g,BallKey::STAR,1,5);unsigned far=add(g,BallKey::STAR,1,9);unsigned cool=add(g,BallKey::COOLGUY);pocket(g,a);
 verify(g.score==10&&g.world.balls[first].active&&g.world.balls[second].active&&g.world.balls[far].active,"Star scores two closest other star values without pocketing them");verify(g.world.balls[cool].def.base_score==2,"star scoring does not emit additional pocket notifications");
 g=arena();a=add(g,BallKey::STAR_SYSTEM,3,2);pocket(g,a);resolve(g);verify(g.world.count==5&&g.world.balls[2].def.id==id_of(BallKey::STAR)&&g.world.balls[3].def.id==id_of(BallKey::STAR)&&g.world.balls[4].def.id==id_of(BallKey::STAR)&&g.world.balls[2].level==3&&g.world.balls[2].def.base_score==4,"level-three Star System emits three Stars with default level-scaled values");
 g=arena();a=add(g,BallKey::GALAXY,2,2);pocket(g,a);resolve(g);verify(g.world.count==4&&g.world.balls[2].def.id==id_of(BallKey::STAR_SYSTEM)&&g.world.balls[3].def.id==id_of(BallKey::STAR_SYSTEM)&&g.world.balls[2].level==2,"Galaxy emits Star Systems rather than immediate Stars");
 g=Game();g.inventory[0]=default_item(id_of(BallKey::SUN),2);g.inventory[1]=default_item(id_of(BallKey::EARTH));g.inventory[1].score=3;g.inventory[2]=default_item(9);g.inventory_count=3;g.begin_round();resolve(g);verify(g.world.balls[2].star_power&&g.world.balls[2].def.base_score==5&&!g.world.balls[3].star_power,"Sun star carriers target planets and apply level buff");
 g=Game();g.inventory[0]=default_item(9);g.inventory[1]=default_item(10);g.inventory[2]=default_item(id_of(BallKey::STARMAP),3);g.inventory_count=3;g.begin_round();resolve(g);verify(g.world.balls[1].star_power&&g.world.balls[1].def.base_score==4&&!g.world.balls[2].star_power,"Starmap selects southernmost eligible setup ball and buffs by level");
 std::printf("Fire, star and fleeting checks: %u\n",checks);return checks;
}
