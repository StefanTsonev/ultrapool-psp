#include "ultrapool/core.hpp"
#include "ultrapool/presentation.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
using namespace up;
namespace {
unsigned checks=0;
void verify(bool value,const char* name) { ++checks; if(!value) { std::fprintf(stderr,"FAIL MIX: %s\n",name); std::exit(1); } }
OwnedBall item(unsigned id,unsigned level=1,int value=1,unsigned mixed=0) {
    OwnedBall b; b.id=static_cast<uint16_t>(id); b.level=static_cast<uint8_t>(level); b.score=value; b.mixed_with=static_cast<uint16_t>(mixed); return b;
}
Game arena(unsigned id,unsigned mixed=0) {
    Game g; g.world=World(); g.world.pockets_enabled=false; g.phase=Phase::Aiming;
    Ball cue; cue.cue=true; cue.p=Vec(-200,0); g.world.add(cue);
    Ball b; b.def=builtin(id); b.mixed_with=static_cast<uint16_t>(mixed); g.world.add(b); return g;
}
void legacy_checkpoint(const char* path) {
    std::vector<unsigned char> bytes;
    auto put=[&](uint32_t value) { for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<unsigned char>(value>>(8*i))); };
    const uint32_t header[]={0x31525055,1,3,30,3,1,0,0x554c5452,0,1,1,2};
    for(auto value:header) put(value);
    for(unsigned i=0;i<15;++i) { put(i==0?9:0); put(1); put(1); }
    for(auto value:{11,12,30,3,0,0,0,1,2,4}) put(value);
    uint32_t hash=2166136261u; for(auto value:bytes) hash=(hash^value)*16777619u; put(hash);
    verify(bytes.size()==272,"legacy fixture matches published 272-byte format");
    FILE* f=std::fopen(path,"wb"); verify(f!=nullptr,"create legacy fixture");
    verify(std::fwrite(bytes.data(),1,bytes.size(),f)==bytes.size(),"write legacy fixture"); std::fclose(f);
}
}
unsigned mixed_ball_tests() {
    checks=0; Game g; g.phase=Phase::Shop;
    for(unsigned first=1;first<content_count;++first) for(unsigned second=1;second<content_count;++second)
        for(unsigned a=1;a<=5;++a) for(unsigned b=1;b<=5;++b) {
            g.inventory={}; g.inventory[0]=item(first,a,3); g.inventory[1]=item(second,b,7);
            g.inventory_count=2; g.cocktail_tickets=1;
            if(first==second) { verify(!g.mix_owned(0,1) && g.cocktail_tickets==1 && g.inventory_count==2,"identical cocktail inputs rejected transactionally"); continue; }
            verify(g.mix_owned(0,1),"distinct unmixed components can mix");
            const OwnedBall& result=g.inventory[0];
            verify(result.level==(a+b+1)/2 && result.score==10,"cocktail sums values and takes ceiling of average levels");
            verify(g.cocktail_tickets==0 && g.inventory_count==1 && !g.inventory[1].id,"mix consumes one ticket and two inputs become one");
            if(number_ball(first) && number_ball(second)) verify(result.id==26 && !result.mixed_with,"two number components collapse into Overflow");
            else verify(result.id==(number_ball(first)?26:first) && result.mixed_with==(number_ball(second)?26:second),"component order retained and number side normalizes to total");
        }
    g.inventory[0]=item(9,2,4,10);g.inventory[1]=item(11);g.inventory_count=2;g.cocktail_tickets=1;
    verify(!g.mix_owned(0,1) && g.inventory_count==2,"already mixed input cannot be mixed again");
    g.inventory[0]=item(9);g.cocktail_tickets=0;
    verify(!g.mix_owned(0,1),"cocktail requires a ticket");
    verify(!g.mix_owned(15,0) && !g.mix_owned(0,0),"invalid and identical inventory slots rejected");
    OwnedBall target=item(11,2,8,12),source=item(12,2,7,11);
    verify(can_combine(source,target),"flipped matching mixed pair combines"); combine_values(source,target);
    verify(target.id==11 && target.mixed_with==12 && target.level==3 && target.score==15,"mixed upgrade preserves destination order and sums value");
    source=item(11,3,2); verify(can_combine(source,target),"same component at matching level can upgrade mixed destination");
    source=item(1,1,4); target=item(2,4,6,9);
    verify(can_combine(source,target),"unmixed number may add value to mixed number destination regardless of level"); combine_values(source,target);
    verify(target.level==4 && target.id==26 && target.mixed_with==9 && target.score==10,"unmixed number merge preserves level and other component");
    source=item(27,2);target=item(9,3,4,10);verify(!can_combine(source,target),"lower-level Upgrade Ball cannot upgrade mixed item");
    source.level=3;verify(can_combine(source,target),"matching-level Upgrade Ball can upgrade mixed item");
    verify(item_price(item(14,2,2,31))==14,"mixed pricing preserves original primary-rarity branch");
    verify(item_price(item(31,2,2,14))==20,"secondary uncommon uses four-money mixed component price");
    g=arena(11,12);g.world.balls[1].active=false;g.world.balls[1].level=2;g.world.balls[1].def.base_score=7;
    g.shots=0;g.world.events.push(EventKind::Pocketed,1,0,0);g.dispatch();
    verify(g.score==14 && g.money==4 && g.shots==2,"mixed pocket scores once then executes Gold and Smiley components");
    g=arena(10,9);g.world.balls[1].level=3;g.world.events.push(EventKind::Wall,1,0,0);g.dispatch();
    verify(g.score==3 && g.world.balls[1].def.base_score==4,"both mixed wall abilities execute at shared level");
    g=arena(1,10);g.world.events.push(EventKind::Wall,1,0,0);g.dispatch();
    verify(g.world.balls[1].def.id==2 && g.world.balls[1].mixed_with==10,"mixed number identity follows temporary value buff");
    Game before=arena(9);before.world.balls[1].active=false;
    Ball ordered;ordered.def=builtin(19);ordered.mixed_with=28;ordered.level=3;before.world.add(ordered);
    Game after=before;after.world.balls[2].def=builtin(28);after.world.balls[2].mixed_with=19;
    before.world.events.push(EventKind::Pocketed,1,0,0);after.world.events.push(EventKind::Pocketed,1,0,0);before.dispatch();after.dispatch();
    verify(before.world.balls[2].def.base_score==after.world.balls[2].def.base_score+3,"Dice and CoolGuy side order changes shared final value");
    g=arena(9);g.world.balls[1].level=3;ordered=Ball();ordered.def=builtin(15);ordered.def.base_score=2;ordered.mixed_with=16;ordered.level=2;g.world.add(ordered);
    Game reverse=g;reverse.world.balls[2].def.ability=Tennis;reverse.world.balls[2].def.id=16;reverse.world.balls[2].mixed_with=15;
    g.world.events.push(EventKind::Wall,1,0,0);reverse.world.events.push(EventKind::Wall,1,0,0);g.dispatch();reverse.dispatch();
    verify(g.world.balls[2].def.base_score==8 && reverse.world.balls[2].def.base_score==4,"ordered score components see value mutations from preceding side");
    g=arena(13,10);g.world.balls[1].def.base_score=7;g.world.events.push(EventKind::Wall,1,0,0);g.dispatch();
    verify(g.score==8 && g.world.balls[1].def.base_score==8,"first-side Bomb death suppresses second-side hit effect");
    g=arena(31,11);g.world.balls[1].active=false;g.world.balls[1].level=2;g.world.balls[1].def.base_score=9;
    g.world.events.push(EventKind::Pocketed,1,0,0);g.dispatch();
    verify(g.money==4 && g.score==18 && g.pending_actions(),"Nesting Doll plus Gold scores and pays before copy arrives");
    for(unsigned i=0;i<1400 && g.pending_actions();++i) g.update(tick_dt);
    verify(g.world.count==3 && g.world.balls[2].mixed_with==11 && g.world.balls[2].level==1 && g.world.balls[2].def.base_score==9,"Nesting copy preserves both components and value while lowering shared level");
    g=Game();g.difficulty=2;g.reset();verify(g.shots==4 && g.target==1,"Malibu original shot penalty and target curve");
    g.round=3;g.begin_round();g.score=g.target;g.finish_round();verify(g.cocktail_tickets==1,"Malibu cocktail ticket earned after winning third round");
    g=Game();g.difficulty=2;g.reset();g.round=3;g.begin_round();g.finish_round();verify(g.cocktail_tickets==0 && g.snack_tickets==0,"loss cannot duplicate snack or cocktail tickets");
    g=Game();g.difficulty=1;g.reset();g.finish_round();verify(g.snack_tickets==0,"first-round loss grants no snack ticket");
    verify(required_score(4,2)==28 && required_score(22,0)==846 && shots_for_round(15,2)==8,"difficulty helpers match original ordinary and endless curves");
    g=Game();g.reset();g.shots=0;g.gain_shots(20);verify(g.shots==15 && g.shot_capacity==20,"shot cap applies to total round capacity including spent shots");
    g.shots=0;g.gain_shots(1);verify(g.shots==0,"exhausted twenty-capacity rack cannot gain a twenty-first shot");
    g=Game();g.difficulty=2;g.reset();g.score=g.target;g.finish_round();g.inventory[0]=item(9,3,11,10);g.cocktail_tickets=4;
    verify(g.save_run("build/mixed-save.sav"),"write mixed checkpoint");Game restored;
    verify(restored.load_run("build/mixed-save.sav") && restored.inventory[0].mixed_with==10 && restored.cocktail_tickets==4 && restored.difficulty==2,"mixed sides tickets and Malibu survive checkpoint roundtrip");
    legacy_checkpoint("build/legacy-mixed-save.sav");
    verify(restored.load_run("build/legacy-mixed-save.sav") && restored.inventory[0].id==9 && !restored.inventory[0].mixed_with && !restored.cocktail_tickets,"legacy checkpoint imports with empty second sides and cocktail tickets");
    std::remove("build/mixed-save.sav");std::remove("build/mixed-save.sav.bak");std::remove("build/legacy-mixed-save.sav");
    std::printf("Mixed-ball checks: %u\n",checks);return checks;
}
