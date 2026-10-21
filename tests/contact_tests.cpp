#include "ultrapool/core.hpp"
#include <cstdio>
#include <cstdlib>
using namespace up;
namespace {
unsigned checks=0;
void verify(bool ok,const char* text) { ++checks;if(!ok) {std::fprintf(stderr,"FAIL CONTACT: %s\n",text);std::exit(1);} }
Game arena() { Game g;g.phase=Phase::Aiming;g.world.pockets_enabled=false;Ball cue;cue.cue=true;cue.p=Vec(-200,210);g.world.add(cue);return g; }
unsigned add(Game& g,BallKey key,unsigned level=1,int value=1,unsigned second=0) { Ball b;b.def=builtin(id_of(key));b.level=static_cast<uint8_t>(level);b.def.base_score=value;b.mixed_with=static_cast<uint16_t>(second);b.p=Vec(-150+float(g.world.count)*60,0);g.world.add(b);return g.world.last_added; }
void hit(Game& g,unsigned a,unsigned b=0,bool wall=false) {g.world.events.push(wall?EventKind::Wall:EventKind::Collision,a,b,g.world.metrics.ticks);g.dispatch();}
void pocket(Game& g,unsigned a,unsigned p=6) { g.world.balls[a].active=false;g.world.events.push(EventKind::Pocketed,a,p,g.world.metrics.ticks);g.dispatch(); }
void resolve(Game& g) {for(unsigned i=0;i<1800 && g.pending_actions();++i) g.update(tick_dt);verify(!g.pending_actions(),"contact carriers finish");}
unsigned carriers(const Game& g,ActionKind kind) {unsigned n=0;for(const auto& a:g.actions)n+=a.kind==kind;return n;}
}
unsigned contact_tests() {
 checks=0;Game g=arena();unsigned a=add(g,BallKey::GOAT,2,3),b=add(g,BallKey::SHEEP,3,4);
 hit(g,a);verify(g.world.balls[a].def.base_score==3,"Goat ignores cue contact");hit(g,b);verify(g.world.balls[0].def.base_score==1,"Sheep ignores cue contact");
 hit(g,a,b);verify(g.world.balls[a].def.base_score==8 && g.world.balls[b].def.base_score==4,"Goat self buff precedes Sheep return buff");
 hit(g,a,0,true);verify(g.world.balls[a].def.base_score==8,"Goat ignores walls");
 g=arena();a=add(g,BallKey::CLOUD,2,1);b=add(g,BallKey::PINBALL,1,3);hit(g,a,0,true);
 verify(!g.world.balls[a].active && g.score==0 && carriers(g,ActionKind::BuffBall)==1,"Cloud can break while still sending its buff");resolve(g);
 verify(g.world.balls[b].def.base_score==5,"Cloud buffs non-number target");
 g=arena();a=add(g,BallKey::CLOUD,2,5);b=add(g,BallKey::PINBALL,1,3);hit(g,a,0,true);g.world.balls[b].active=false;resolve(g);
 verify(g.world.balls[a].def.base_score==6,"Cloud carrier retargets surviving source if original target dies");
 g=arena();a=add(g,BallKey::FISHBOWL,3,1);hit(g,a,0,true);verify(!g.world.balls[a].active && carriers(g,ActionKind::Spawn)==1,"empty Fishbowl breaks and still emits fish");resolve(g);
 verify(g.world.count==3 && g.world.balls[2].def.id==id_of(BallKey::FISH) && g.world.balls[2].level==3 && g.world.balls[2].def.base_score==3,"Fish spawn keeps level and explicit score");
 verify(g.world.balls[2].def.mass==.5f && g.world.balls[2].def.radius>13 && g.world.balls[2].def.radius<14,"Fish starts light");
 g=arena();a=add(g,BallKey::CHICKEN,2,4);pocket(g,a);verify(g.score==4 && carriers(g,ActionKind::Spawn)==1,"Chicken pocket emits egg after scoring");resolve(g);b=2;
 verify(g.world.balls[b].def.id==id_of(BallKey::EGG) && g.world.balls[b].level==2 && g.world.balls[b].def.base_score==2,"Egg spawn explicit level/value");hit(g,b,0,true);
 verify(g.score==8 && !g.world.balls[b].active,"Egg wall uses virtual x2 score");hit(g,b,0,true);verify(g.score==8,"dead egg cannot score a second wall event");
 g=arena();a=add(g,BallKey::EGG,1,3);hit(g,a);verify(g.world.balls[a].active,"Egg does not pocket on ball contact");g.buff_ball(a,1);verify(g.world.balls[a].active && g.world.balls[a].def.base_score==4,"Egg has no BUFF branch in original despite registered mask");
 g=arena();a=add(g,BallKey::MUSHROOM,2,7);b=add(g,BallKey::BUSH);unsigned far=add(g,BallKey::PINBALL);g.world.balls[b].p=Vec(-50,0);g.world.balls[far].p=Vec(200,0);pocket(g,a);resolve(g);
 verify(g.world.balls[b].def.base_score==9 && g.world.balls[far].def.base_score==1,"Mushroom sends full value to nearest, which runs its own Bush handler");
 g=arena();a=add(g,BallKey::INCUBATOR,2,4);b=add(g,BallKey::SHARK_PLUSH,3,1);verify(g.transform_ball(a,id_of(BallKey::PINBALL)),"transform accepted");
 verify(g.world.balls[a].def.base_score==4 && carriers(g,ActionKind::BuffBall)==2,"Incubator before-change and Shark after-change gains remain delayed");resolve(g);
 verify(g.world.balls[a].def.base_score==19,"Incubator +6 and Shark +9 arrive on transformed ball");
 g=arena();a=add(g,BallKey::SHARK_PLUSH,2,4,id_of(BallKey::INCUBATOR));g.transform_ball(a,id_of(BallKey::PINBALL),1);resolve(g);
 verify(g.world.balls[a].def.id==id_of(BallKey::SHARK_PLUSH) && g.world.balls[a].mixed_with==9 && g.world.balls[a].def.base_score==16,"mixed transform triggers retained Shark on self after old Incubator callback");
 g=arena();a=add(g,BallKey::PUMPKIN,2,8,id_of(BallKey::INCUBATOR));hit(g,a,0,true);
 verify(g.world.balls[a].def.id==id_of(BallKey::JACK_O_LANTERN) && g.world.balls[a].mixed_with==id_of(BallKey::INCUBATOR) && g.world.balls[a].def.base_score==10,"Pumpkin replaces only own component at exact threshold");resolve(g);verify(g.world.balls[a].def.base_score==16,"retained mixed Incubator observes Pumpkin transformation");
 g=arena();a=add(g,BallKey::INCUBATOR,1,2);g.world.balls[a].falling=true;g.world.balls[a].fall_remaining=.1f;g.transform_ball(a,id_of(BallKey::PINBALL));resolve(g);verify(g.world.balls[a].def.base_score==2,"transformation carrier cancels when falling target pockets before arrival");

 g=arena();a=add(g,BallKey::WEREWOLF,2,9);b=add(g,BallKey::PINBALL);hit(g,a,b);verify(g.world.balls[a].def.id==id_of(BallKey::WEREWOLF),"wolf ignores object contact");hit(g,a);verify(g.world.balls[a].def.id==id_of(BallKey::WEREWOLF_HUMAN),"cue changes wolf into human");hit(g,a);verify(g.world.balls[a].def.id==id_of(BallKey::WEREWOLF),"second cue contact changes human back");pocket(g,a);verify(g.score==1,"Werewolf physical or virtual pocket scores zero; Pinball previous contact remains");
 g=arena();a=add(g,BallKey::WEREWOLF_HUMAN,2,9);pocket(g,a);verify(g.score==9,"human form pockets for its ordinary value");
 g=arena();a=add(g,BallKey::ZOMBIE,2,5,id_of(BallKey::BUSH));b=add(g,BallKey::PINBALL,4,2);hit(g,a,b);
 verify(g.world.balls[a].def.base_score==9 && g.world.balls[b].def.id==id_of(BallKey::ZOMBIE) && g.world.balls[b].mixed_with==id_of(BallKey::BUSH),"Zombie runs self buff then infects both identities");
 verify(g.world.balls[b].def.base_score==2 && g.world.balls[b].level==4,"infection retains victim score and level");int old=g.world.balls[a].def.base_score;hit(g,a,b);verify(g.world.balls[a].def.base_score==old,"identical ordered Zombie mixture cannot infect itself again");
 g=arena();a=add(g,BallKey::BLUEPRINT,3,7,id_of(BallKey::INCUBATOR));b=add(g,BallKey::SHEEP,1,2,id_of(BallKey::B_1_BALL));hit(g,a,b);
 verify(g.world.balls[a].def.id==id_of(BallKey::SHEEP) && g.world.balls[a].mixed_with==8 && g.world.balls[a].level==3 && g.world.balls[a].def.base_score==8,"Blueprint copies ordered identities, numbers use base, opposite Sheep still hits after transformation");resolve(g);verify(g.world.balls[a].def.base_score==17,"copied Blueprint still receives old Incubator carrier");
 g=arena();a=add(g,BallKey::BUSH,2,7,id_of(BallKey::MOSS));b=add(g,BallKey::FISH);g.transform_like(a,g.world.balls[b]);verify(g.world.balls[a].mixed_with==0 && g.world.balls[a].def.mass==.5f,"copying a single light Fish clears old mixture and applies initial weight");g.transform_ball(a,9);verify(g.world.balls[a].def.mass==.5f,"transforming away from Fish retains weight modifier");
 g=arena();a=add(g,BallKey::FROG,2,5,id_of(BallKey::BUSH));b=add(g,BallKey::INCUBATOR,3,4);pocket(g,a);verify(g.world.balls[b].def.id==id_of(BallKey::INCUBATOR) && carriers(g,ActionKind::TransformBall)==1,"Frog identity is captured in a delayed carrier");resolve(g);verify(g.world.balls[b].def.id==id_of(BallKey::FROG) && g.world.balls[b].mixed_with==id_of(BallKey::BUSH) && g.world.balls[b].level==3 && g.world.balls[b].def.base_score==21,"Frog applies copied identities then buff at recipient level with old Incubator gain");
 g=arena();a=add(g,BallKey::PRINTER,3,4);pocket(g,a);verify(carriers(g,ActionKind::Spawn)==2,"Printer emits exactly two Blueprint carriers");resolve(g);verify(g.world.count==4 && g.world.balls[2].def.id==id_of(BallKey::BLUEPRINT) && g.world.balls[3].def.id==id_of(BallKey::BLUEPRINT) && g.world.balls[2].def.base_score==8 && g.world.balls[2].level==3,"Printer Blueprints have level and power-of-two score");
 std::printf("Contact and transform checks: %u\n",checks);return checks;
}
