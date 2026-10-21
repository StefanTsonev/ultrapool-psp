#include "ultrapool/core.hpp"
#include <cstdio>
#include <cstdlib>
#include <memory>
using namespace up;
namespace {
unsigned checks=0;
void verify(bool ok,const char* message){++checks;if(!ok){std::fprintf(stderr,"FAIL CONTENT BATCH: %s\n",message);std::exit(1);}}
void arena(Game& g,unsigned skip=0){g=Game();g.phase=Phase::Aiming;g.target=1000000;g.world.pockets_enabled=false;Ball cue;cue.cue=true;cue.p=Vec(0,210);g.world.add(cue);g.set_run_seed(0x554c5452u+skip*7919u);g.shop_sets={};g.offers={};g.offer_items={};}
unsigned add(Game& g,BallKey key,unsigned level=1,int value=1,Vec p=Vec(0,-120)){Ball b;b.def=builtin(id_of(key));b.def.base_score=value;b.level=static_cast<uint8_t>(level);b.p=p;g.world.add(b);return g.world.last_added;}
void pocket(Game& g,unsigned index){g.world.balls[index].active=false;g.world.balls[index].v=Vec();++g.world.metrics.ticks;g.world.events.push(EventKind::Pocketed,index,6,g.world.metrics.ticks);g.dispatch();}
void resolve(Game& g){for(unsigned n=0;n<2400 && g.pending_actions();++n)g.update(tick_dt);verify(!g.pending_actions(),"content carriers finish");}
unsigned buffs(const Game& g){unsigned n=0;for(const auto& a:g.actions)n+=a.kind==ActionKind::BuffBall;return n;}
}
unsigned content_batch_tests(){
 checks=0;std::unique_ptr<Game> holder(new Game);Game& g=*holder;
 arena(g);g.daily_date=20261020;g.shop_sets={{ItemSet::FOOD,ItemSet::FRIENDS,ItemSet::CLASSIC}};g.daily_random.reset(7654321);unsigned daily_food=add(g,BallKey::ONIGIRI,3,1);pocket(g,daily_food);unsigned daily_spawns=0;for(const auto& action:g.actions)if(action.kind==ActionKind::Spawn){++daily_spawns;verify(action.snapshot.def.id==id_of(BallKey::BOTTLE) && action.snapshot.level==3,"real daily Onigiri pocket emits Godot-predicted Bottle at source level");}verify(daily_spawns==1,"daily Onigiri pocket emits exactly one content carrier");
 arena(g);g.daily_date=20261020;g.shop_sets[0]=ItemSet::TECH;g.daily_random.reset(7657892);unsigned daily_friend=add(g,BallKey::FRIENDSPHERE,1,1);pocket(g,daily_friend);verify(g.world.count==3 && g.world.balls[2].def.id==id_of(BallKey::ROBOT) && g.world.balls[2].level==1,"real daily Friendsphere pocket spawns Godot-predicted Robot");
 unsigned chef_outcomes=0,panellet_statuses=0,panellet_weights=0;
 for(unsigned seed=0;seed<40;++seed){
  arena(g,seed);unsigned chef=add(g,BallKey::CHEF_HAT,3,3);unsigned other=add(g,BallKey::B_1_BALL,1,1,Vec(100,0));pocket(g,other);unsigned actions=0;
  for(const auto& a:g.actions)if(a.kind==ActionKind::BuffBall || a.kind==ActionKind::ModifierBall){++actions;verify(a.target==chef,"Chef Hat can choose itself and excludes inactive/cue objects");if(a.kind==ActionKind::BuffBall){verify(a.bonus==1,"Chef Hat buff is exactly one");chef_outcomes|=1;}else{verify(a.bonus==0,"Chef Hat status does not add value");verify(a.flags==static_cast<unsigned>(Modifier::Shield) || a.flags==static_cast<unsigned>(Modifier::Flame),"Chef Hat selects only original status effects");chef_outcomes|=a.flags==static_cast<unsigned>(Modifier::Shield)?2:4;}}
  verify(actions==3,"Chef Hat emits level choices with replacement");
  arena(g,seed);unsigned panel=add(g,BallKey::PANELLET,2,1);set_ball_locked(g.world.balls[panel],true);g.world.balls[panel].shielded=true;other=add(g,BallKey::B_1_BALL,1,1,Vec(100,0));pocket(g,other);const Ball& b=g.world.balls[panel];
  verify(b.def.base_score==3,"Panellet adds component level value");verify(unsigned(b.locked)+unsigned(b.flaming)+unsigned(b.shielded)==1,"Panellet replaces status with exactly one lock/flame/shield");verify(b.weight!=Weight::VeryHeavy,"Panellet chooses only normal/heavy/light weight");panellet_statuses|=b.locked?1:b.flaming?2:4;panellet_weights|=1u<<static_cast<unsigned>(b.weight);
 }
 verify(chef_outcomes==7 && panellet_statuses==7 && panellet_weights==7,"random content exercises all original outcomes");
 for(unsigned recipients=0;recipients<=8;++recipients){
  arena(g,recipients*3);unsigned meat=add(g,BallKey::MEATBALL,3,17);
  for(unsigned n=0;n<recipients;++n)add(g,BallKey::BREAD,1,1,Vec(-180+float(n)*45,-240));pocket(g,meat);int total=0;unsigned count=0;
  for(const auto& a:g.actions)if(a.kind==ActionKind::BuffBall){total+=a.bonus;++count;verify(a.bonus>0 && a.target!=meat,"Meatball queues only positive shares to active recipients");}
  verify(total==(recipients?26:0) && count<=recipients,"Meatball preserves rounded half-value-times-level total across random partition");resolve(g);
 }
 arena(g);unsigned rainbow=add(g,BallKey::RAINBOW,2,2);add(g,BallKey::DOG,1,1,Vec(-100,0));add(g,BallKey::CAT,1,1,Vec(100,0));pocket(g,rainbow);verify(buffs(g)==2,"Rainbow buffs distinct surviving identities");resolve(g);verify(g.world.balls[2].def.base_score==3 && g.world.balls[3].def.base_score==3,"Rainbow applies level value to each target");
 arena(g);rainbow=add(g,BallKey::RAINBOW,2,2);add(g,BallKey::DOG);add(g,BallKey::DOG,1,1,Vec(100,0));pocket(g,rainbow);verify(buffs(g)==0,"Rainbow rejects repeated identities");
 arena(g);rainbow=add(g,BallKey::RAINBOW,2,2);unsigned first=add(g,BallKey::DOG);g.world.balls[first].mixed_with=id_of(BallKey::CAT);unsigned second=add(g,BallKey::CAT,1,1,Vec(100,0));g.world.balls[second].mixed_with=id_of(BallKey::DOG);pocket(g,rainbow);verify(buffs(g)==2,"Rainbow mixed uniqueness preserves original component order");
 arena(g);rainbow=add(g,BallKey::RAINBOW,2,2);add(g,BallKey::DOG);add(g,BallKey::TUNGSTEN_BALL,1,1,Vec(100,0));add(g,BallKey::TUNGSTEN_BALL,1,1,Vec(-100,0));pocket(g,rainbow);verify(buffs(g)==0,"untargetable duplicates block Rainbow uniqueness");
 g=Game();g.inventory[0]=default_item(id_of(BallKey::RAINBOW),2);g.inventory[1]=default_item(id_of(BallKey::DOG));g.inventory_count=2;g.begin_round();resolve(g);verify(g.world.balls[1].def.base_score==6 && g.world.balls[2].def.base_score==3,"Rainbow also runs through real Spawn dispatch");
 for(unsigned seed=0;seed<24;++seed){
  arena(g,seed);g.shop_sets[0]=ItemSet::FOOD;unsigned onigiri=add(g,BallKey::ONIGIRI,3,1);pocket(g,onigiri);unsigned spawns=0;
  for(const auto& a:g.actions)if(a.kind==ActionKind::Spawn){++spawns;const auto& info=ball_info(a.snapshot.def.id);verify(info.rarity==0 && (info.tags&static_cast<uint32_t>(Tag::TAG_FOOD)) && a.snapshot.def.id!=id_of(BallKey::ONIGIRI),"Onigiri strictly chooses a common Food identity excluding itself");verify(a.snapshot.level==3 && a.snapshot.def.base_score==default_item(a.snapshot.def.id,3).score,"Onigiri preserves level and selected default value");}
  verify(spawns==1,"Onigiri emits one carrier rather than level carriers");
 }
 arena(g);g.shop_sets[0]=ItemSet::TECH;unsigned onigiri=add(g,BallKey::ONIGIRI,2,1);pocket(g,onigiri);verify(g.actions.empty(),"Onigiri does not escape chosen sets when no common Food is eligible");
 arena(g);g.shop_sets[0]=ItemSet::FRIENDS;unsigned sphere=add(g,BallKey::FRIENDSPHERE,3,1);pocket(g,sphere);verify(g.world.count==5,"Friendsphere spawns level friends immediately");
 for(unsigned i=2;i<g.world.count;++i){const Ball& b=g.world.balls[i];verify(has_tag(b,Tag::TAG_FRIEND) && b.def.id!=id_of(BallKey::PET_FOOD) && b.def.id!=id_of(BallKey::FRIENDSPHERE) && b.def.id!=id_of(BallKey::TENNIS),"Friendsphere excludes original prohibited identities");verify(b.level==1 && b.def.base_score==default_item(b.def.id,1).score && b.owner==-1,"Friendsphere uses level-one temporary friends with default value");}
 arena(g);unsigned nest=add(g,BallKey::NEST,2,1);unsigned egg=add(g,BallKey::EGG,2,9,Vec(100,-180));g.world.balls[egg].mixed_with=id_of(BallKey::FLOWER_BUSH);g.world.balls[egg].owner=0;g.inventory[0]=default_item(id_of(BallKey::EGG),2);g.inventory[0].score=9;unsigned other=add(g,BallKey::B_1_BALL,1,1,Vec(-100,0));pocket(g,other);resolve(g);
 verify(g.world.balls[egg].def.id==id_of(BallKey::CHICKEN) && g.world.balls[egg].mixed_with==id_of(BallKey::FLOWER_BUSH) && g.world.balls[egg].level==2 && g.world.balls[egg].def.base_score==11,"Nest hatches only Egg component at threshold while preserving mixed value and level");
 verify(g.flower_count()+g.pickup_contacts==4,"hatching replays both mixed Spawn components; unchanged Flower Bush creates four flowers");verify(g.inventory[0].id==id_of(BallKey::EGG) && g.inventory[0].level==2 && g.inventory[0].score==9,"Nest hatching remains temporary");
 unsigned eggs_after=0;for(unsigned i=1;i<g.world.count;++i)eggs_after+=g.world.balls[i].active && g.world.balls[i].def.id==id_of(BallKey::EGG);verify(eggs_after==1,"new Chicken executes its Egg spawning effect");
 arena(g);add(g,BallKey::NEST,3,1);for(unsigned n=0;n<4;++n)add(g,BallKey::EGG,1,1,Vec(-150+100*float(n),-200));other=add(g,BallKey::B_1_BALL);pocket(g,other);verify(buffs(g)==3,"Nest buffs at most three distinct Eggs");
 arena(g);egg=add(g,BallKey::EGG,1,9);g.queue_ball_buff(egg,2,g.world.balls[egg].p);resolve(g);verify(g.world.balls[egg].def.id==id_of(BallKey::EGG),"ordinary Egg buffs do not invent Nest hatching");
 arena(g);unsigned ant=add(g,BallKey::ANT,3,10);pocket(g,ant);unsigned ant_spawns=0;for(const auto& a:g.actions)if(a.kind==ActionKind::Spawn){++ant_spawns;verify(a.snapshot.level==3 && a.snapshot.def.base_score==4,"Ant replacement uses default level value rather than pocketed runtime value");}resolve(g);verify(ant_spawns==1 && g.world.balls[g.world.last_added].weight==Weight::Light,"Ant replacement remains Light");
 arena(g);unsigned log=add(g,BallKey::LOG,2,1);add(g,BallKey::LOG,1,1,Vec(-100,0));unsigned mixed_log=add(g,BallKey::DOG,1,1,Vec(100,0));g.world.balls[mixed_log].mixed_with=id_of(BallKey::LOG);add(g,BallKey::DOG,1,1,Vec(0,-250));pocket(g,log);verify(buffs(g)==2,"Log recognizes primary and mixed surviving Log components");resolve(g);verify(g.world.balls[2].def.base_score==5 && g.world.balls[3].def.base_score==5 && g.world.balls[4].def.base_score==1,"Log buffs only matching live objects");
 arena(g);unsigned replicator=add(g,BallKey::REPLICATOR,2,1);g.inventory[10]=default_item(id_of(BallKey::DOG),2);g.inventory[10].score=7;g.inventory[10].mixed_with=id_of(BallKey::CAT);pocket(g,replicator);unsigned copies=0;for(const auto& a:g.actions)if(a.kind==ActionKind::Spawn){++copies;verify(a.snapshot.def.id==id_of(BallKey::DOG) && !a.snapshot.mixed_with && a.snapshot.level==2 && a.snapshot.def.base_score==7,"Replicator uses original primary-only reserve spawning semantics");}verify(copies==3,"Replicator emits one plus level copies");
 arena(g);replicator=add(g,BallKey::REPLICATOR,2,1);g.inventory[11]=default_item(id_of(BallKey::DOG));pocket(g,replicator);verify(g.actions.empty(),"Replicator requires literal first reserve slot");
 arena(g);replicator=add(g,BallKey::REPLICATOR,2,1);g.inventory[10]=default_item(id_of(BallKey::B_2_BALL));pocket(g,replicator);verify(g.actions.empty(),"Replicator rejects non-common first reserve");
 arena(g);unsigned pot=add(g,BallKey::POT,2,2);other=add(g,BallKey::BREAD,1,7,Vec(100,0));pocket(g,other);verify(g.world.balls[pot].def.base_score==5,"Pot adds ceiling of pocketed raw value times level divided by five");
 arena(g);unsigned peacock=add(g,BallKey::PEACOCK,2,1,Vec());unsigned near=add(g,BallKey::BREAD,1,1,Vec(50,0));unsigned far_x=add(g,BallKey::BREAD,1,1,Vec(200,0));unsigned far_y=add(g,BallKey::BREAD,1,1,Vec(0,-250));other=add(g,BallKey::B_1_BALL,1,1,Vec(0,100));for(unsigned i=1;i<g.world.count;++i)g.world.balls[i].def.damping=0;pocket(g,other);unsigned pulls=0;for(const auto& a:g.actions)if(a.kind==ActionKind::Impulse){++pulls;verify(a.target==far_x || a.target==far_y,"Peacock chooses farthest distinct objects");}g.update(tick_dt);g.update(tick_dt);verify(pulls==2 && g.world.balls[near].v.length()==0 && std::fabs(g.world.balls[far_x].v.x+300)<.1f && std::fabs(g.world.balls[far_y].v.y-300)<.1f,"Peacock pulls selected objects toward itself at 300 momentum");
 arena(g);peacock=add(g,BallKey::PEACOCK,1,1);set_ball_locked(g.world.balls[peacock],true);other=add(g,BallKey::B_1_BALL,1,1,Vec(100,0));pocket(g,other);bool zero_pull=false;for(const auto& a:g.actions)zero_pull|=a.kind==ActionKind::Impulse && a.target==peacock && a.impulse.length()==0;verify(zero_pull && !g.world.balls[peacock].locked,"Peacock self-selection unlocks even with zero-direction impulse");
 arena(g);unsigned spirit=add(g,BallKey::SPIRIT,9,1,Vec(100,0));for(unsigned p=0;p<6;++p)g.world.pockets[p].closed=p!=5;other=add(g,BallKey::B_1_BALL,1,1,Vec(0,-200));pocket(g,other);bool spirit_shot=false;for(const auto& a:g.actions)spirit_shot|=a.kind==ActionKind::Impulse && a.target==spirit && std::fabs(a.impulse.x+600)<.01f;verify(spirit_shot,"Spirit level-nine certainty targets closest open pocket");
 arena(g);add(g,BallKey::SPIRIT,9,1);for(auto& p:g.world.pockets)p.closed=true;other=add(g,BallKey::B_1_BALL);pocket(g,other);verify(g.actions.empty(),"Spirit safely handles all pockets closed");
 arena(g);unsigned turbine=add(g,BallKey::TURBINE,2,1);unsigned old_target=add(g,BallKey::BREAD,1,1,Vec(100,0));other=add(g,BallKey::B_1_BALL,1,1,Vec(-100,0));g.world.balls[turbine].def.damping=0;pocket(g,other);g.world.balls[old_target].active=false;unsigned replacement=add(g,BallKey::DOG,1,1,Vec(100,100));g.world.balls[replacement].def.damping=0;bool launched=false;unsigned shot_target=max_balls;
 for(unsigned n=0;n<1800 && !launched;++n){g.update(tick_dt);for(const auto& a:g.actions)if(a.kind==ActionKind::Impulse){launched=true;shot_target=a.target;verify(a.target!=old_target && std::fabs(a.impulse.length()-450)<.1f,"Turbine carrier retargets and preserves half-force shot");}}
 verify(launched && shot_target<g.world.count && g.world.balls[shot_target].def.base_score==3 && g.world.balls[old_target].def.base_score==1,"Turbine applies level value to new live recipient rather than dead target");g.update(tick_dt);g.update(tick_dt);verify(std::fabs(g.world.balls[shot_target].v.length()-450)<.1f,"retargeted shot still decodes Shoot modifier correctly");
 // Motion-content batch: real Pocket dispatch, mixed Friend tags, population and timed forces.
 for(unsigned lv=1;lv<=5;++lv){
  arena(g);unsigned antenna=add(g,BallKey::ANTENNA,lv);add(g,BallKey::BREAD);add(g,BallKey::TUNGSTEN_BALL);pocket(g,antenna);unsigned spawned=0;
  for(const auto& action:g.actions)if(action.kind==ActionKind::Spawn){++spawned;verify(action.snapshot.def.id==id_of(BallKey::ALIEN) && action.snapshot.level==lv && action.snapshot.def.base_score==int(lv),"Antenna carriers preserve Alien identity, level and explicit score");}
  verify(spawned==1+2*lv,"Antenna counts surviving targetable objects and excludes Tungsten");
 }
 arena(g);unsigned antenna=add(g,BallKey::ANTENNA);for(unsigned i=0;i<5;++i)add(g,BallKey::BREAD);pocket(g,antenna);verify(g.actions.empty(),"Antenna never removes excess balls");
 arena(g);unsigned alien=add(g,BallKey::ALIEN,3);unsigned friend_ball=add(g,BallKey::BREAD,1,1,Vec(100,0));g.world.balls[friend_ball].mixed_with=id_of(BallKey::DOG);add(g,BallKey::BREAD,1,1,Vec(-100,0));pocket(g,alien);unsigned nudges=0;
 for(const auto& action:g.actions)if(action.kind==ActionKind::ModifierBall){++nudges;verify(action.target==friend_ball && action.flags==static_cast<unsigned>(Modifier::Nudge) && action.bonus==3 && std::fabs(action.multiplier-1.5)<.001f,"Alien recognizes mixed Friend tags and uses half-level nudge force plus level buff");}
 verify(nudges==1,"Alien nudges exactly one friend");
 arena(g);alien=add(g,BallKey::ALIEN);add(g,BallKey::BREAD);pocket(g,alien);verify(g.actions.empty(),"Alien safely handles no Friend recipient");
 arena(g);unsigned wheel=add(g,BallKey::WHEEL,3);for(unsigned i=0;i<5;++i)add(g,BallKey::BREAD,1,1,Vec(-180+80*float(i),0));pocket(g,wheel);unsigned launches=0;unsigned long long chosen=0;
 for(const auto& action:g.actions)if(action.kind==ActionKind::ModifierBall){++launches;verify(!(chosen&(1ull<<action.target)),"Wheel chooses distinct initial recipients");chosen|=1ull<<action.target;verify(action.bonus==1 && action.multiplier==1 && action.flags==(static_cast<unsigned>(Modifier::Shoot)|0x80000000u),"Wheel carries +1 value, full random shot and retarget permission");}
 verify(launches==3,"Wheel emits level carriers capped by population");
 arena(g);wheel=add(g,BallKey::WHEEL,5);add(g,BallKey::BREAD);pocket(g,wheel);verify(g.actions.size()==1,"Wheel does not duplicate targets to fill missing recipients");
 arena(g);unsigned conveyor=add(g,BallKey::CONVEYOR,2);unsigned normal=add(g,BallKey::BREAD,1,1,Vec(-100,0));unsigned heavy=add(g,BallKey::TUNGSTEN_BALL,1,1,Vec(100,0));set_ball_locked(g.world.balls[normal],true);unsigned falling=add(g,BallKey::BREAD,1,1,Vec(0,-300));g.world.balls[falling].falling=true;
 pocket(g,conveyor);Vec flow=g.world.balls[normal].conveyor_direction;
 verify(!g.world.balls[normal].locked && std::fabs(g.world.balls[normal].conveyor_time-.4f)<.00001f,"Conveyor unlocks recipients and sets level duration");
 verify(std::fabs(flow.length()-1)<.00001f && (flow-g.world.balls[heavy].conveyor_direction).length()==0 && g.world.balls[heavy].conveyor_time>0,"Conveyor includes untargetable Tungsten with shared normalized direction");
 verify(g.world.balls[0].conveyor_time==0 && g.world.balls[conveyor].conveyor_time==0 && g.world.balls[falling].conveyor_time>0,"Conveyor excludes cue/pocketed source but sets paused movement on alive falling balls");
 g.world.balls[normal].def.damping=g.world.balls[heavy].def.damping=0;g.update(tick_dt);
 verify(std::fabs(g.world.balls[normal].v.length()-1000*tick_dt)<.001f && std::fabs(g.world.balls[heavy].v.length()-1000*tick_dt/60)<.001f,"Conveyor integrates original 1000 force with inverse mass each tick");
 verify(g.pending_actions(),"Conveyor duration prevents premature round settlement");
 g.world.balls[normal].chase_flowers=.1f;float held=g.world.balls[normal].conveyor_time;g.update(tick_dt);verify(g.world.balls[normal].conveyor_time==held,"Flower chase retains original priority and pauses conveyor timer");
 g.world.balls[normal].chase_flowers=0;g.world.balls[normal].rocket_time=.1f;g.update(tick_dt);verify(g.world.balls[normal].conveyor_time==held,"Rocket retains priority over conveyor");
 g.world.balls[normal].rocket_time=0;for(unsigned n=0;n<100;++n)g.update(tick_dt);verify(g.world.balls[normal].conveyor_time==0 && g.world.balls[heavy].conveyor_time==0,"Conveyor force expires at bounded fixed-step duration");
 g.world.balls[normal].conveyor_time=1;g.world.balls[normal].conveyor_direction=Vec(1,0);g.world.revive(normal,Vec(-100,0));verify(g.world.balls[normal].conveyor_time==0 && g.world.balls[normal].conveyor_direction.length()==0,"revival clears old Conveyor force just as original ball initialization does");
 std::printf("Grouped content checks: %u\n",checks);return checks;
}
