#include "ultrapool/core.hpp"
#include "special_content_reference_vectors.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace up;
namespace {
unsigned checks=0;
void verify(bool value,const char* text) { ++checks;if(!value) { std::fprintf(stderr,"FAIL BUFF: %s\n",text);std::exit(1); } }
Game arena() { Game g;g.phase=Phase::Aiming;g.world.pockets_enabled=false;Ball cue;cue.cue=true;cue.p=Vec(-200,210);g.world.add(cue);return g; }
unsigned add(Game& g,unsigned id,unsigned level=1,int score=1,unsigned second=0) {
    Ball b;b.def=builtin(id);b.def.base_score=score;b.level=static_cast<uint8_t>(level);b.mixed_with=static_cast<uint16_t>(second);
    b.p=Vec(float((g.world.count%5)*50)-100,float((g.world.count/5)*50));g.world.add(b);return g.world.last_added;
}
unsigned add(Game& g,BallKey key,unsigned level=1,int score=1,unsigned second=0) { return add(g,id_of(key),level,score,second); }
void resolve(Game& g) { for(unsigned i=0;i<1800 && g.pending_actions();++i) g.update(tick_dt);verify(!g.pending_actions(),"buff carriers and queued reactions resolve"); }
}
unsigned buff_tests() {
    checks=0;
    {const special_content_reference::Vector* chosen=nullptr;for(const auto& reference:special_content_reference::vectors)if(reference.kind==0 && reference.selected==id_of(BallKey::INCUBATOR)){chosen=&reference;break;}verify(chosen!=nullptr,"independent Godot vectors include a Gacha-to-Incubator case");Game transformed;transformed.run_seed=chosen->seed;transformed.shop_sets[0]=ItemSet::SPOOKY;transformed.inventory[0]=default_item(id_of(BallKey::GACHA));transformed.inventory_count=1;transformed.begin_round();resolve(transformed);verify(transformed.world.balls[1].def.id==id_of(BallKey::INCUBATOR) && transformed.gacha_rolls==1,"real round Spawn transforms Gacha to independently predicted Incubator");verify(transformed.world.balls[1].def.base_score==5,"Gacha fires Transform on newly acquired Incubator and applies its three-point buff");verify(transformed.inventory[0].id==id_of(BallKey::INCUBATOR) && transformed.inventory[0].score==2,"Gacha identity and one-point permanent gain persist; Incubator buff stays temporary");Game shop;shop.phase=Phase::Shop;shop.run_seed=chosen->seed;shop.shop_sets[0]=ItemSet::SPOOKY;shop.money=100;shop.offers[0]=id_of(BallKey::GACHA);verify(shop.buy(0) && shop.inventory[0].id==id_of(BallKey::INCUBATOR) && shop.inventory[0].score==5,"shop Gacha fires new Incubator transform reaction into persistent value");Game shark;shark.phase=Phase::Shop;shark.run_seed=chosen->seed;shark.shop_sets[0]=ItemSet::SPOOKY;shark.money=100;shark.inventory[1]=default_item(id_of(BallKey::SHARK_PLUSH),2);shark.inventory_count=1;shark.offers[0]=id_of(BallKey::GACHA);verify(shark.buy(0) && shark.inventory[0].score==11,"shop Gacha dispatches TransformAnother to level-two Shark Plush");}
    verify(content_count==223 && snack_count==52,"complete unique ball and passive identity catalogs loaded");
    for(unsigned i=0;i<content_count;++i) verify(find_ball(ball_info(i).key)==i,"ball identifier lookup preserves native catalog identity");
    for(unsigned i=1;i<snack_count;++i) verify(find_snack(snack_info(i).key)==i,"passive identifier lookup preserves native catalog identity");
    verify(find_ball("not-a-real-ball")==content_count && find_snack(nullptr)==snack_count,"unknown identities do not alias cue or empty passive");
    verify(id_of(BallKey::MIRROR)==32 && id_of(SnackKey::MINI_MUFFIN)==12,"previous checkpoint IDs remain stable");
    verify(ball_supported(id_of(BallKey::GACHA)) && shop_eligible(id_of(BallKey::GACHA),true),"Gacha with implemented spawn/buy transformation can enter eligible shops");
    verify(shop_eligible(id_of(BallKey::BUSH),true) && !shop_eligible(id_of(BallKey::BUSH),false),"extra-set Bush uses original set eligibility");
    verify(!shop_eligible(id_of(BallKey::BUTTERFLY),true) && !snack_info(id_of(SnackKey::TWO_OLIVES)).can_drop,"disabled original records remain out of stock");
    verify(ball_info(id_of(BallKey::ANTIMATTER)).base_score==-1 && ball_info(id_of(BallKey::IMP)).base_score==-2,"negative base values preserved in metadata");
    verify(default_item(id_of(BallKey::MAZE)).level==3 && default_item(id_of(BallKey::MAZE)).score==4,"original level-three resource default scales starting value");
    Ball tags;tags.def=builtin(id_of(BallKey::BUSH));tags.mixed_with=id_of(BallKey::GINGERBREAD);
    verify(has_tag(tags,Tag::TAG_NATURE) && has_tag(tags,Tag::TAG_FOOD) && has_tag(tags,Tag::TAG_FRIEND) && has_tag(tags,Tag::TAG_BREAD),"mixed component tags form a complete union");
    SemanticQueue queue;SemanticEvent event;
    for(unsigned i=0;i<512;++i) { event.amount=i;verify(queue.push(event),"semantic queue accepts original capacity"); }
    event.amount=512;verify(queue.push(event) && queue.overflows==0,"semantic delivery grows past former limit without dropping events");
    for(unsigned i=0;i<513;++i) { SemanticEvent next;verify(queue.pop(next) && next.amount==int(i),"semantic propagation preserves FIFO order"); }
    verify(queue.empty(),"semantic queue drains completely");
    Game g=arena();unsigned index=add(g,BallKey::BUSH,2);g.buff_ball(index,3);
    verify(g.world.balls[index].def.base_score==6 && g.world.balls[index].base_value==1 && g.world.balls[index].temp_value==5,"Bush adds level without recursive self-trigger and stores temporary gain separately");
    g=arena();index=add(g,BallKey::BUSH,2,1,id_of(BallKey::SAND));add(g,BallKey::GAIA,3,3);g.buff_ball(index,1);
    verify(g.world.balls[index].def.base_score==10 && g.score==8,"Gaia affects outer and Bush buffs; Sand scores only chained outer amount");
    g=arena();index=add(g,BallKey::ANTIMATTER,1,20);g.buff_ball(index,4);
    verify(g.world.balls[index].active && g.world.balls[index].def.base_score==16,"Antimatter subtracts twice received amount before outer gain");
    g=arena();index=add(g,BallKey::B_1_BALL);g.buff_ball(index,-2,false);
    verify(!g.world.balls[index].active && g.score==0,"non-falling zero-value break retires without pocket score");
    g=arena();index=add(g,BallKey::SAND,1,5);g.world.balls[index].falling=true;g.buff_ball(index,-3);
    verify(g.world.balls[index].def.base_score==5,"falling balls ignore negative buffs before callbacks");
    for(auto key:{BallKey::IMP,BallKey::TUNGSTEN_BALL,BallKey::COOTS}) {
        g=arena();index=add(g,key,1,5);g.buff_ball(index,4);verify(g.world.balls[index].def.base_score==5,"original immune identity rejects buff");
    }
    g=arena();index=add(g,BallKey::MOSS);unsigned other=add(g,BallKey::MOSS);g.buff_ball(index,3);resolve(g);
    verify(g.world.balls[index].def.base_score==4 && g.world.balls[other].def.base_score==2,"Moss shares rounded incoming fraction and suppresses Moss-to-Moss recursion");
    g=arena();index=add(g,BallKey::BUSH,3,1,id_of(BallKey::MOSS));other=add(g,BallKey::B_1_BALL);g.buff_ball(index,1);resolve(g);
    verify(g.world.balls[index].def.base_score==5 && g.world.balls[other].def.base_score==2,"Moss shares incoming amount rather than Bush's augmented resulting value");
    g=arena();index=add(g,BallKey::GINGERBREAD);g.buff_ball(index,1);
    verify(g.world.balls[index].v.length()==0 && g.pending_actions(),"Gingerbread launch waits two physics frames");g.update(2*tick_dt);
    verify(g.world.balls[index].v.length()>449 && g.world.balls[index].v.length()<451,"level-one Gingerbread uses verified 450 impulse");
    g=arena();index=add(g,BallKey::SAND);g.set_ball_score(index,5);
    verify(g.world.balls[index].def.base_score==5 && g.score==4,"increasing assignment travels through buff/score-change chain");
    g.set_ball_score(index,2);verify(g.world.balls[index].base_value==2 && g.world.balls[index].temp_value==0 && g.score==4,"decreasing assignment replaces base and clears temporary value without positive score callback");
    g=arena();index=add(g,BallKey::DICE,1,1,id_of(BallKey::BUSH));other=add(g,BallKey::B_1_BALL);g.world.balls[other].active=false;
    g.world.events.push(EventKind::Pocketed,other,0,0);g.dispatch();verify(g.world.balls[index].def.base_score==7,"Dice's upward assignment allows mixed Bush to exceed rolled value");
    g=arena();index=add(g,BallKey::B_1_BALL);g.world.balls[index].owner=0;g.inventory[0].id=1;g.inventory[0].score=1;add(g,BallKey::GAIA,2,3);
    g.queue_ball_buff(index,1,Vec(),true);resolve(g);
    verify(g.world.balls[index].def.base_score==4 && g.world.balls[index].base_value==1 && g.inventory[0].score==2,"permanent wisp stores raw gain in owner while Gaia gain remains temporary");
    g=arena();index=add(g,BallKey::B_1_BALL,2,3,id_of(BallKey::SAND));g.world.balls[index].owner=0;g.inventory[0].id=1;g.inventory[0].score=3;
    g.buff_ball(index,2,false);verify(g.transform_ball(index,id_of(BallKey::BUSH),0),"transform primary component");
    verify(g.world.balls[index].base_value==3 && g.world.balls[index].temp_value==2 && g.world.balls[index].mixed_with==id_of(BallKey::SAND) && g.inventory[0].id==1,"temporary transform preserves shared value state and untouched component without changing owner");
    verify(g.transform_ball(index,id_of(BallKey::MOSS),0,true) && g.inventory[0].id==id_of(BallKey::MOSS),"permanent transform updates corresponding owned component");
    g=arena();index=add(g,BallKey::CATERPILLAR,5);other=add(g,BallKey::B_1_BALL);
    for(unsigned i=0;i<200 && g.world.balls[index].def.id==id_of(BallKey::CATERPILLAR);++i) g.buff_ball(index,1);
    verify(g.world.balls[index].def.id==id_of(BallKey::BUTTERFLY) && g.pending_actions(),"Caterpillar's verified buff probability can transform and trigger Butterfly spawn effect");resolve(g);
    verify(g.world.balls[other].def.base_score==6,"transformed level-five Butterfly creates all-target spawn buffs");
    g=arena();index=add(g,BallKey::DONUT,2);for(unsigned i=0;i<4;++i) add(g,BallKey::B_1_BALL);
    other=add(g,BallKey::B_1_BALL);g.world.balls[other].active=false;g.world.events.push(EventKind::Pocketed,other,0,0);g.dispatch();resolve(g);
    unsigned buffed=0;for(unsigned i=2;i<other;++i) buffed+=g.world.balls[i].def.base_score==2;
    verify(buffed==4 && g.world.balls[index].def.base_score==1,"Donut buffs distinct other eligible balls and excludes itself");
    g=arena();index=add(g,9);add(g,16,1,1,id_of(BallKey::SAND));g.target=1000;
    g.world.events.push(EventKind::Wall,index,0,0);g.dispatch();for(unsigned i=0;i<10 && !g.cap_rewarded;++i) g.update(tick_dt);
    verify(!g.cap_rewarded && g.score==1 && g.world.balls[2].def.base_score==2 && g.semantics.empty(),"Tennis score gain suppresses buff chaining and cannot create a Sand feedback loop");
    std::printf("Catalog and buff checks: %u\n",checks);return checks;
}
