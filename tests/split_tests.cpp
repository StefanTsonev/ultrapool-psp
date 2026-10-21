#include "ultrapool/core.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
using namespace up;
namespace {
unsigned checks=0;
void verify(bool ok,const char* text){++checks;if(!ok){std::fprintf(stderr,"FAIL SPLIT: %s\n",text);std::exit(1);}}
Game arena(){Game g;g.phase=Phase::Aiming;g.target=1000;g.world.pockets_enabled=false;Ball cue;cue.cue=true;cue.p=Vec(-200,210);g.world.add(cue);return g;}
unsigned add(Game& g,BallKey key,int value=1,unsigned level=1,unsigned second=0){Ball b;b.def=builtin(id_of(key));b.def.base_score=value;b.level=static_cast<uint8_t>(level);b.mixed_with=static_cast<uint16_t>(second);b.p=Vec(-170+55*float(g.world.count),0);g.world.add(b);return g.world.last_added;}
void pocket(Game& g,unsigned a){g.world.balls[a].active=false;g.world.events.push(EventKind::Pocketed,a,6,g.world.metrics.ticks);g.dispatch();}
void resolve(Game& g){for(unsigned i=0;i<2400&&g.pending_actions();++i)g.update(tick_dt);verify(!g.pending_actions(),"split and planet carriers finish");}
}
unsigned split_tests(){
 checks=0;Game g=arena();unsigned a=add(g,BallKey::PINBALL,5,3,id_of(BallKey::MARBLE));Ball& source=g.world.balls[a];source.owner=0;source.shielded=true;source.star_power=true;source.fleeting=true;set_ball_weight(source,Weight::Heavy);set_ball_locked(source,true);Vec position=source.p;g.inventory[0]=default_item(9);g.inventory_count=1;
 verify(g.split_ball(a,.5f),"split succeeds");verify(!g.world.balls[a].active&&g.world.balls[a].gone&&g.world.count==4&&g.score==0,"split destroys source without pocket score and creates two copies immediately");
 for(unsigned i=2;i<4;++i)verify(g.world.balls[i].def.base_score==3&&g.world.balls[i].level==3&&g.world.balls[i].mixed_with==id_of(BallKey::MARBLE)&&g.world.balls[i].owner==-1&&g.world.balls[i].shielded&&g.world.balls[i].star_power&&g.world.balls[i].fleeting&&!g.world.balls[i].locked&&g.world.balls[i].def.mass==5,"copies preserve identities, level and flags, unlock and remain temporary");
 verify(std::fabs((g.world.balls[2].p-position).length()-std::sqrt(200.0f))<.01f&&(g.world.balls[3].p-position).length()==0,"first split copy uses rotated Vector2.ONE times ten; second uses exact source position");verify(g.inventory[0].score==1,"runtime split leaves persistent owned build untouched");
 for(unsigned mode=0;mode<3;++mode){g=arena();a=add(g,mode==2?BallKey::BUSH:BallKey::B_1_BALL,1,1,mode==1?id_of(BallKey::BUSH):mode==2?4:0);g.buff_ball(a,3,false);verify(g.split_ball(a,.5f),"split original reference case");const Ball& copy=g.world.balls[2];verify(copy.def.base_score==(mode==0?5:2)&&copy.temp_value==0&&copy.def.id==(mode==0?5:mode==1?1:id_of(BallKey::BUSH))&&copy.mixed_with==(mode==1?id_of(BallKey::BUSH):mode==2?4:0),"split value and ordered number identities match direct Godot BallItem reference results");}
 g=arena();a=add(g,BallKey::PINBALL,8);set_ball_weight(g.world.balls[a],Weight::Heavy);g.split_ball(a,.5f,true);verify(g.world.balls[2].weight==Weight::Normal&&g.world.balls[2].def.mass==1,"lower-size split lowers heavy to normal");
 g=arena();a=add(g,BallKey::PINBALL,8);g.split_ball(a,.5f,true);verify(g.world.balls[2].weight==Weight::Light&&g.world.balls[2].def.mass==.5f,"lower-size split lowers normal to light");
 g=arena();a=add(g,BallKey::PINBALL,4);g.world.balls[a].falling=true;g.world.balls[a].fall_remaining=10;g.split_ball(a,.5f);verify(g.world.balls[a].active&&g.world.balls[a].falling&&g.world.count==4,"already-falling split target is not broken by original guard");
 g=arena();a=add(g,BallKey::PINBALL,6);g.queue_ball_split(a,.5f,false,Vec());verify(g.world.count==2&&g.world.balls[a].active,"split carrier does not fork before arrival");resolve(g);verify(g.world.count==4&&!g.world.balls[a].active,"split carrier forks on arrival");
 g=arena();a=add(g,BallKey::PINBALL,6);g.queue_ball_split(a,.5f,false,Vec());g.world.balls[a].active=false;resolve(g);verify(g.world.count==2,"split carrier cancels dead recipient");
 g=arena();for(unsigned i=0;i<30;++i)add(g,BallKey::PINBALL,4);verify(g.split_ball(1,.5f)&&g.spawn_rejections==1,"split at thirty-active cap admits one replacement then rejects second");
 g=arena();a=add(g,BallKey::BANANA,2,2);unsigned other=add(g,BallKey::PINBALL,5);pocket(g,a);resolve(g);verify(!g.world.balls[other].active&&g.world.count==5&&g.world.balls[3].def.base_score==2,"level-two Banana splits another ball into two ceil(40 percent) copies");
 g=arena();a=add(g,BallKey::SATURN,2,3);other=add(g,BallKey::STAR,5);add(g,BallKey::PINBALL,8);pocket(g,a);resolve(g);verify(!g.world.balls[other].active&&g.world.count==6&&g.world.balls[4].def.base_score==4&&g.world.balls[3].active,"Saturn split preserves Godot double rounding: ceil(5*(0.2*3)) is four");
 g=arena();a=add(g,BallKey::VENUS,2,2);unsigned first=add(g,BallKey::STAR,1),second=add(g,BallKey::STAR,2);other=add(g,BallKey::PINBALL,3);pocket(g,a);resolve(g);verify(g.world.balls[first].def.base_score==5&&g.world.balls[second].def.base_score==6&&g.world.balls[other].def.base_score==3,"Venus buffs all and only stars by twice level");
 g=arena();a=add(g,BallKey::EARTH,2,1);first=add(g,BallKey::PINBALL,1);second=add(g,BallKey::PINBALL,1);pocket(g,a);resolve(g);verify(g.world.balls[first].star_power&&!g.world.balls[second].star_power,"Earth stars closest non-star up to level");
 g=arena();a=add(g,BallKey::JUPITER,2,1);first=add(g,BallKey::STAR,1);second=add(g,BallKey::STAR,1);other=add(g,BallKey::STAR,1);pocket(g,a);resolve(g);verify(g.world.balls[first].def.mass==5&&g.world.balls[second].def.mass==5&&g.world.balls[first].def.base_score==4&&g.world.balls[other].def.mass==1,"Jupiter makes first two stars heavy and buffs three");
 g=arena();a=add(g,BallKey::PLUTO,2,1);first=add(g,BallKey::STAR,1);pocket(g,a);resolve(g);verify(g.world.balls[first].def.mass==.5f&&g.world.balls[first].def.base_score==2,"Pluto lightens and buffs star");
 g=arena();a=add(g,BallKey::MARS,2,1);first=add(g,BallKey::BUSH,1);g.world.balls[first].star_power=true;pocket(g,a);resolve(g);verify(g.world.balls[first].flaming&&g.world.balls[first].def.base_score==4,"Mars ignites before buff so Bush repeats its buff callback");
 g=arena();a=add(g,BallKey::NEPTUNE,2,1);first=add(g,BallKey::STAR,1);g.world.balls[first].shielded=true;second=add(g,BallKey::STAR,1);pocket(g,a);resolve(g);verify(g.world.balls[second].shielded&&g.world.balls[first].shielded,"Neptune skips already shielded stars");
 g=arena();a=add(g,BallKey::URANUS,2,2);first=add(g,BallKey::STAR,1);pocket(g,a);resolve(g);verify(g.world.balls[first].locked&&g.world.balls[first].def.base_score==9,"Uranus locks one star and buffs four times level");
 g=arena();a=add(g,BallKey::MERCURY,2,1);first=add(g,BallKey::STAR,1);pocket(g,a);resolve(g);verify(g.world.balls[first].def.base_score==2&&std::fabs(g.world.balls[first].v.length()-900)<.1f,"Mercury carrier buffs star and schedules original-strength launch");
 g=arena();a=add(g,BallKey::GIFT,2,2);first=add(g,BallKey::PINBALL,1);second=add(g,BallKey::PINBALL,1,1,id_of(BallKey::MARBLE));other=add(g,BallKey::MARBLE,1);pocket(g,a);resolve(g);verify(g.world.balls[first].def.base_score==3&&g.world.balls[second].def.base_score==5&&g.world.balls[other].def.base_score==3,"Gift counts union of shared component identities without duplicate counting");
 g=arena();a=add(g,BallKey::CHEESE,4,1);first=add(g,BallKey::PINBALL,1);second=add(g,BallKey::MARBLE,2);other=add(g,BallKey::SHEEP,3);pocket(g,a);verify(g.world.balls[first].def.base_score+g.world.balls[second].def.base_score+g.world.balls[other].def.base_score==9&&g.score==4,"Cheese buffs all then rotates values without extra pockets");
 std::printf("Split, planet and identity checks: %u\n",checks);return checks;
}
