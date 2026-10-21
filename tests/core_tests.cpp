#include "ultrapool/core.hpp"
#include "ultrapool/presentation.hpp"
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <limits>
using namespace up;
static int assertions=0;
static void check(bool b,const char* name) {
    ++assertions;
    if (!b) { std::fprintf(stderr,"FAIL: %s\n",name); std::exit(1); }
}
static bool near(float a,float b,float eps=0.03f) { return std::fabs(a-b)<eps; }
static World isolated() {
    World w; w.minimum=Vec(-100000,-100000); w.maximum=Vec(100000,100000);
    w.pockets_enabled=false; w.stop_speed=0; w.speed_limit=0; return w;
}
static Ball ball(Vec p,Vec v,float mass=1) {
    Ball b; b.p=p; b.v=v; b.def.mass=mass; b.def.damping=0; return b;
}
static void collisions() {
    World w=isolated(); w.add(ball(Vec(0,0),Vec(100,0))); w.add(ball(Vec(34,0),Vec()));
    w.step(); check(near(w.balls[0].v.x,0),"equal mass stops striker");
    check(near(w.balls[1].v.x,100),"equal mass transfers velocity");
    check(w.metrics.impacts==1,"one contact event");
    w=isolated(); w.add(ball(Vec(0,0),Vec(100,0),1)); w.add(ball(Vec(34,0),Vec(),5));
    w.step(); check(near(w.balls[0].v.x,-66.6667f),"heavy ball reflection");
    check(near(w.balls[1].v.x,33.3333f),"heavy ball velocity");
    check(near(w.balls[0].v.x+5*w.balls[1].v.x,100),"momentum conservation");
    float energy=w.balls[0].v.dot(w.balls[0].v)+5*w.balls[1].v.dot(w.balls[1].v);
    check(near(energy,10000,1),"kinetic energy conservation");
    w=isolated(); w.add(ball(Vec(0,0),Vec(40000,0))); w.add(ball(Vec(100,0),Vec()));
    w.step(); check(near(w.balls[1].v.x,40000),"sweep prevents ball tunneling");
    check(w.balls[0].p.x<w.balls[1].p.x,"sweep preserves ball ordering");
    w=isolated(); w.add(ball(Vec(0,0),Vec(100,20))); w.add(ball(Vec(34,0),Vec()));
    w.step(); check(near(w.balls[0].v.y,20),"tangent velocity retained");
    w=isolated(); w.add(ball(Vec(0,0),Vec(-100,0))); w.add(ball(Vec(34,0),Vec()));
    w.step(); check(near(w.balls[0].v.x,-100),"separating contact has no impulse");
    w=isolated(); w.add(ball(Vec(),Vec())); w.add(ball(Vec(),Vec(),5));
    w.step(); check((w.balls[0].p-w.balls[1].p).length()>=33.999f,"coincident overlap repaired");
    check(!w.moving(),"overlap repair adds no energy");
}
static void walls_pockets_damping() {
    World w=isolated(); w.minimum=Vec(-100,-100); w.maximum=Vec(100,100);
    w.add(ball(Vec(82,0),Vec(1000,0))); w.step();
    check(near(w.balls[0].v.x,-1000),"wall elastic bounce");
    check(w.balls[0].p.x<=83,"wall bounds");
    check(w.balls[0].visual_rotation>0 && w.balls[0].visual_rotation<1,"ball visual spin follows fixed-step travel");
    w=isolated(); w.minimum=Vec(-100,-100); w.maximum=Vec(100,100);
    w.add(ball(Vec(),Vec(100000,0))); w.step();
    check(w.metrics.impacts>=3 && w.metrics.solver_exhaustions==0,"multiple swept wall bounces");
    check(w.balls[0].p.x>=-83 && w.balls[0].p.x<=83,"fast ball contained");
    for (unsigned p=0;p<6;++p) {
        w=World(); Ball b=ball(w.pockets[p].p,Vec()); w.add(b); w.step();
        check(w.balls[0].active && w.balls[0].falling,"capture remains active during falling");
        for(int tick=0;tick<91;++tick) w.step();
        check(!w.balls[0].active,"all six pockets finish after half a second");
        Event e; check(w.events.pop(e) && e.kind==EventKind::Captured,"capture comes before score");
        check(w.events.pop(e) && e.kind==EventKind::Pocketed && e.b==p,"pocket identity event");
    }
    w=isolated(); w.pockets_enabled=true; w.pockets[0].p=Vec(100,0);
    w.add(ball(Vec(),Vec(40000,0))); w.step();
    check(w.balls[0].falling,"swept pocket entry");
    w=World(); Ball cue=ball(w.pockets[0].p,Vec()); cue.cue=true; w.add(cue); w.step();
    check(w.balls[0].falling,"cue uses smaller pocket trigger");
    w=isolated(); Ball b=ball(Vec(),Vec(180,0)); b.def.damping=1.5f; w.add(b); w.step();
    check(near(w.balls[0].v.x,178.5f),"reference damping 1.5 at 180 Hz");
    w.stop_speed=2; w.balls[0].v=Vec(1,0); w.step(); check(!w.moving(),"stop threshold");
}
static void game_events_resources() {
    Events q;
    for (unsigned i=0;i<1024;++i) check(q.push(EventKind::Collision,i,0,3),"event queue capacity");
    check(!q.push(EventKind::Shot,0,0,3) && q.dropped==1,"overflow instrumented");
    Event e;
    for (unsigned i=0;i<1024;++i) check(q.pop(e) && e.sequence==i && e.a==i,"FIFO event ordering");
    check(!q.pop(e),"empty event queue");
    Game a; a.reset();
    check(!a.shoot(),"placement cannot shoot");
    Input confirm; confirm.confirm=true; a.input(confirm,0.016f);
    check(a.phase==Phase::Aiming,"placement confirm");
    check(a.shoot() && a.shots==4,"shot consumes one shot");
    check(a.shoot() && a.shots==3,"original permits another shot while balls are moving");
    check(near(a.world.balls[0].v.length(),1500),"reference impulse mapping");
    a.paused=true; Vec p=a.world.balls[0].p; a.update(1);
    check(near((a.world.balls[0].p-p).length(),0),"pause freezes physics");
    a.paused=false;
    for (int i=0;i<1200 && a.phase==Phase::Simulating;++i) a.update(1.0/60);
    check(a.phase!=Phase::Simulating,"shot settles and returns control");
    check(a.world.events.dropped==0 && a.world.metrics.solver_exhaustions==0,"rack event and solver budgets");
    Game b,c; b.reset(); c.reset(); b.phase=c.phase=Phase::Aiming; b.shoot(); c.shoot();
    for (int i=0;i<60;++i) b.update(1.0/60);
    for (int i=0;i<120;++i) c.update(1.0/120);
    check(b.world.metrics.ticks==c.world.metrics.ticks,"frame independent fixed ticks");
    for (std::size_t i=0;i<b.world.count;++i)
        check(near((b.world.balls[i].p-c.world.balls[i].p).length(),0,0.001f),"frame independent trajectory");
    Game s; s.reset();
    s.world.events.push(EventKind::Pocketed,1,0,1); s.dispatch();
    check(s.score==4,"base score times pocket multiplier");
    check(!s.world.valid_placement(s.world.balls[1].p,17.5f),"placement collision rejected");
    Ball bad; bad.def.mass=0; check(!s.world.add(bad),"zero mass rejected");
    std::array<Definition,226> defs; std::size_t count=0;
    check(load_definitions("assets/definitions.txt",defs,count) && count==content_count-1,"data definitions load");
    Asset asset; check(!load_sprite("not-present.ups",asset),"missing sprite safe");
}
static void scratch_and_resources() {
    Game g; g.reset(); g.phase=Phase::Aiming; g.shoot();
    g.world.balls[0].p=g.world.pockets[0].p;
    g.world.balls[0].v=Vec(100,0);
    g.update(1.0/60);
    // No object is moving, so a scratch should finish directly into placement.
    for(int tick=0;tick<120 && g.phase==Phase::Simulating;++tick) g.update(1.0/180);
    check(g.phase==Phase::Aiming,"scratch returns automatically to aiming");
    check(g.world.balls[0].active,"scratch restores cue");
    check(g.score==0,"scratch earns no points");
    check(g.world.valid_placement(g.world.balls[0].p,17.5f),"scratch finds valid position");
    g.reset(); g.phase=Phase::Aiming; g.shots=1; g.power=0.26f; g.angle=0;
    check(g.shoot(),"last shot starts");
    for (int i=0;i<900 && g.phase==Phase::Simulating;++i) g.update(1.0/60);
    for(int tick=0;tick<240 && g.phase!=Phase::Shop;++tick) g.update(1.0/180);
    check(g.phase==Phase::Shop && g.hp==2 && g.money==11,"missed round costs HP and pays consolation");
    g.reset();
    check(g.shots==5 && g.phase==Phase::Placement,"restart resets round");

    FILE* f=std::fopen("build/synthetic.ups","wb");
    check(f!=nullptr,"create synthetic sprite fixture");
    const unsigned char bytes[]={'U','P','S','1',2,0,1,0,0x8f,0xf0,0xff,0xff};
    std::fwrite(bytes,1,sizeof(bytes),f); std::fclose(f);
    Asset a; check(load_sprite("build/synthetic.ups",a),"native sprite loads");
    check(a.width==2 && a.height==1 && a.pixels[0]==0xf08f,"native sprite dimensions/packing");
    f=std::fopen("build/synthetic.ups","wb");
    std::fwrite(bytes,1,10,f); std::fclose(f);
    check(!load_sprite("build/synthetic.ups",a),"truncated sprite rejected");
    check(a.width==2 && a.pixels[0]==0xf08f,"failed load preserves previous asset");
    f=std::fopen("build/invalid_definitions.txt","w");
    check(f!=nullptr,"create definitions fixture");
    std::fputs("1 17 1 1.5 1 ffffffff 0\n1 17 1 1.5 1 ffffffff 0\n",f); std::fclose(f);
    std::array<Definition,226> defs; std::size_t n=17;
    check(!load_definitions("build/invalid_definitions.txt",defs,n) && n==17,"duplicate definitions rejected atomically");
    check(g.load_content("assets/definitions.txt"),"game data registry loads");
    g.reset(); check(g.world.balls[1].def.id==2,"game uses data-driven definition");
}
static void stress() {
    World w; w.pockets_enabled=false;
    uint32_t rng=42;
    for (int i=0;i<64;++i) {
        rng=rng*1664525u+1013904223u;
        float angle=(rng%6283)/1000.0f;
        Ball b=ball(Vec((i%8)*40-140,(i/8)*40-140),Vec(std::cos(angle),std::sin(angle))*1800,(i%4==0)?5:1);
        b.def.damping=1.5f; w.add(b);
    }
    check(!w.add(Ball()),"bounded instance capacity");
    for (int t=0;t<1800;++t) {
        w.step(); Event e; while(w.events.pop(e)) {}
        for (std::size_t i=0;i<w.count;++i) {
            const Ball& b=w.balls[i];
            check(std::isfinite(b.p.x) && std::isfinite(b.p.y) && std::isfinite(b.v.x) && std::isfinite(b.v.y),"stress finite state");
            check(b.p.x>=w.minimum.x+b.def.radius-0.05f && b.p.x<=w.maximum.x-b.def.radius+0.05f &&
                  b.p.y>=w.minimum.y+b.def.radius-0.05f && b.p.y<=w.maximum.y-b.def.radius+0.05f,"stress wall containment");
        }
    }
    check(w.metrics.solver_exhaustions==0 && w.events.dropped==0,"64-ball stress within budgets");
    check(!w.moving(),"64-ball stress settles");
}
static Game arena(unsigned id) {
    Game g; g.reset(true); g.world=isolated();
    Ball cue=ball(Vec(-200,0),Vec()); cue.cue=true; g.world.add(cue);
    Ball b=ball(Vec(),Vec()); b.def=builtin(id); g.world.add(b);
    g.score=0; g.money=0; return g;
}
static void abilities_and_rounds() {
    Game g=arena(9);
    g.world.events.push(EventKind::Wall,1,0,1); g.dispatch();
    check(g.score==1,"Pinball scores wall hit");
    g.world.events.push(EventKind::Collision,0,1,2); g.dispatch();
    check(g.score==2,"Pinball scores cue collision");
    g=arena(10); g.world.events.push(EventKind::Wall,1,0,1); g.dispatch();
    check(g.world.balls[1].def.base_score==2 && g.score==0,"Bricks gains temporary value");
    g.world.events.push(EventKind::Collision,0,1,2); g.dispatch();
    check(g.world.balls[1].def.base_score==2,"Bricks only reacts to walls");
    g=arena(11); g.world.balls[1].active=false;
    g.world.events.push(EventKind::Pocketed,1,0,1); g.dispatch();
    check(g.score==2 && g.money==2,"Gold pocket scores then pays money");
    g=arena(12); g.shots=0; g.world.balls[1].active=false;
    g.world.events.push(EventKind::Pocketed,1,0,1); g.dispatch();
    check(g.shots==1,"Smiley rescues exhausted shots");
    check(g.world.balls[1].def.ability==Smiley,"Smiley keeps its identity after pocketing");
    g.shots=20; g.world.events.push(EventKind::Pocketed,1,0,2); g.dispatch();
    check(g.shots==20,"Smiley obeys 20 shot cap");
    g=arena(13); Ball other=ball(Vec(100,0),Vec()); g.world.add(other);
    g.world.balls[1].def.base_score=7;
    g.world.events.push(EventKind::Wall,1,0,1); g.dispatch();
    check(!g.world.balls[1].active && g.score==8,"Bomb explodes at reference threshold and scores at x1");
    check(g.world.balls[2].v.length()==0 && g.pending_actions(),"Bomb impulse waits two physics frames");
    g.update(2*tick_dt);
    check(near(g.world.balls[2].v.x,150),"Bomb applies reference radial falloff impulse");
    g=arena(14); g.world.add(other);
    g.world.events.push(EventKind::Collision,0,1,1); g.dispatch();
    check(g.score==0,"Seal does not score cue collision");
    g.world.events.push(EventKind::Collision,1,2,2); g.dispatch();
    check(g.score==1 && g.world.balls[2].v.length()==0,"Seal scores before delayed push");
    g.update(2*tick_dt);
    check(near(g.world.balls[2].v.x,240),"Seal pushes object after two frames");
    g=arena(9);
    for(unsigned id=15;id<=17;++id) { Ball b=ball(Vec(float(id*40),0),Vec()); b.def=builtin(id); g.world.add(b); }
    g.world.balls[1].level=10;
    g.world.events.push(EventKind::Wall,1,0,1); g.dispatch();
    check(g.world.balls[2].def.base_score==4,"Basketball listens to larger SCORE");
    check(g.world.balls[3].def.base_score==1,"Tennis ignores large SCORE");
    check(g.world.balls[4].def.base_score==6,"Football listens to multiples of ten");
    g.world.balls[1].level=1; g.world.events.push(EventKind::Wall,1,0,2); g.dispatch();
    check(g.world.balls[3].def.base_score==2,"Tennis buffs on small score");
    g=arena(9); g.world.balls[0].p=Vec(-34,0); g.world.balls[0].v=Vec(100,0);
    g.world.step(); g.dispatch();
    check(g.score==1,"actual swept collision triggers ability");
    g.world.step(); g.dispatch();
    check(g.score==1,"persistent pair contact does not repeat ability");

    g.reset(); check(g.inventory_count==3 && g.world.count==4,"Classic starts with three owned balls");
    check(g.inventory[0].id==2 && g.inventory[1].id==1 && g.shots==5 && g.target==1,"reference deck/shots/first target");
    g.score=10; g.shots=3;
    check(g.finish_round() && g.phase==Phase::Shop,"target enters shop");
    check(g.money==10,"round reward plus triangular surplus plus unspent shots");
    g.offers[0]=9; g.inventory_selection=0;
    check(g.buy(0) && g.inventory_count==4 && g.money==7,"buy adds persistent ball and charges price");
    g.offers[1]=9; g.inventory_selection=3;
    check(g.buy(1,true) && g.inventory[3].level==2 && g.inventory[3].score==2,"same-level merge upgrades permanent build");
    g.offers[2]=9; int money=g.money;
    check(!g.buy(2,true) && g.money==money,"invalid merge never spends money");
    g.offers[2]=1; g.inventory_selection=1;
    check(g.buy(2,true) && g.inventory[1].id==2 && g.inventory[1].score==2,"number merge changes number and value");
    check(g.sell(3) && g.inventory_count==3,"selling updates persistent inventory");
    check(g.reroll_price()==1,"reference first reroll cost");
    check(g.reroll() && g.rerolls==1 && g.reroll_price()==2,"reroll regenerates offers and scales price");
    g.next_round();
    check(g.round==2 && g.target==6 && g.shots==5 && g.score==0,"next round restores original target/shots");
    check(g.world.balls[2].def.base_score==2,"persistent upgrades survive round transition");
    g.hp=1; g.score=0; check(g.finish_round() && g.phase==Phase::Lost,"last HP loss ends run");
    g.reset(true); check(g.inventory_count==6 && g.world.balls[1].def.ability==Bomb,"ability trial contains real abilities");
    for(int i=1;i<5;++i) check(shop_price(i)==(i==1?3:i<4?4:6),"reference rarity prices");
    g.phase=Phase::Menu; g.selection=0; Input confirm; confirm.confirm=true; g.input(confirm,0.01f);
    check(!g.trial && g.inventory_count==3 && g.phase==Phase::Placement,"normal menu starts original Classic rather than custom ability trial");
}
struct RecordingPainter : Painter {
    unsigned rectangles=0,circles=0,sprites=0,rotated_sprites=0,rotated_regions=0;
    std::vector<std::string> labels;
    void rect(float,float,float,float,uint32_t) override { ++rectangles; }
    void line(float,float,float,float,uint32_t) override {}
    void circle(float,float,float,uint32_t) override { ++circles; }
    void sprite(const Texture&,float,float,float,float,uint32_t) override { ++sprites; }
    void sprite_rotated(const Texture&,float,float,float,float,float,uint32_t) override { ++rotated_sprites; }
    unsigned regions=0;
    void sprite_region(const Texture&,float,float,float,float,float,float,float,float,uint32_t) override { ++regions; }
    void sprite_region_rotated(const Texture&,float,float,float,float,float,float,float,float,float,float,float,uint32_t) override { ++rotated_regions; }
    void text(int,int,uint32_t,const char* value) override { labels.push_back(value); }
    bool contains(const char* needle) const {
        for(const auto& s:labels) if(s.find(needle)!=std::string::npos) return true;
        return false;
    }
};
static void cooldown(Game& g) { Input idle; for(int i=0;i<4;++i) g.input(idle,0.05f); }
static void visual_shop_and_textures() {
    FILE* f=std::fopen("build/synthetic.upt","wb");
    check(f!=nullptr,"create native texture fixture");
    const unsigned char bytes[]={'U','P','T','1',2,0,1,0,0x8f,0xf0,0xff,0xff};
    std::fwrite(bytes,1,sizeof(bytes),f); std::fclose(f);
    Texture t;
    check(load_texture("build/synthetic.upt",t),"UPT1 loads");
    check(t.width==2 && t.height==1 && t.pixels[0]==0xf08f,"RGBA4444 texture pixels");
    check((reinterpret_cast<uintptr_t>(t.pixels)&15)==0,"texture storage aligned for PSP GU");
    f=std::fopen("build/synthetic.upt","wb");
    std::fwrite(bytes,1,10,f); std::fclose(f);
    check(!load_texture("build/synthetic.upt",t) && t.pixels[0]==0xf08f,"truncated texture rejected atomically");
    const unsigned char bad[]={'U','P','T','1',3,0,1,0,0,0,0,0,0,0};
    f=std::fopen("build/synthetic.upt","wb"); std::fwrite(bad,1,sizeof(bad),f); std::fclose(f);
    check(!load_texture("build/synthetic.upt",t),"non power-of-two texture rejected");
    t.clear(); check(!t.pixels && t.bytes()==0,"texture unload releases storage");

    Game g; g.reset(true); g.score=3; g.finish_round();
    Input result_confirm;result_confirm.confirm=true;g.input(result_confirm,.016f);g.input(result_confirm,.016f);
    g.money=30; g.offers[0]=13;
    check(g.shop_focus==ShopFocus::Market,"shop starts at offers");
    Input down; down.down=true; g.input(down,0.016f);
    check(g.shop_focus==ShopFocus::Actions&&g.shop_action==0,"down from left offer selects Roll without a duplicate Sell stop");
    cooldown(g); Input across;across.right=true;g.input(across,0.016f);
    check(g.shop_focus==ShopFocus::Actions&&g.shop_action==1,"right from Roll selects Play");
    Input back; back.cancel=true; g.input(back,0.016f);
    check(g.shop_focus==ShopFocus::Market && g.phase==Phase::Shop,"Circle backs out without leaving shop");
    Input merge; merge.inspect=true; g.input(merge,0.016f);
    check(g.merge_pending && g.shop_focus==ShopFocus::Inventory,"merge enters explicit target selection");
    Skin no_assets;
    RecordingPainter preview; paint_scene(preview,no_assets,g,1,1000);
    check(preview.contains("MERGE Bomb -> Bomb") && preview.contains("Result: level 2"),"merge displays compatible preview");
    int money=g.money; Input confirm; confirm.confirm=true; g.input(confirm,0.016f);
    check(!g.merge_pending && g.inventory[0].level==2 && g.money==money-3,"confirmed merge updates build once");
    g.shop_focus=ShopFocus::Inventory;g.inventory_selection=0;
    Input sell; sell.finish=true; money=g.money; auto count=g.inventory_count;
    g.input(sell,0.016f);
    check(g.sell_pending && g.inventory_count==count && g.money==money,"sell waits for confirmation");
    RecordingPainter sale; paint_scene(sale,no_assets,g,1,1000);
    check(sale.contains("Sell Bomb") && sale.contains(" confirm / ") && sale.contains(" cancel"),"sale displays price and confirmation");
    g.input(back,0.016f);
    check(!g.sell_pending && g.inventory_count==count,"cancel sale preserves build");
    g.shop_focus=ShopFocus::Inventory;g.inventory_selection=0;cooldown(g); g.input(sell,0.016f); g.input(confirm,0.016f);
    check(g.inventory_count==count-1 && !g.sell_pending && g.money>money,"confirmed sale commits");
    cooldown(g); g.input(down,0.016f);
    check(g.shop_focus==ShopFocus::Actions,"actions reachable after sale");
    g.shop_focus=ShopFocus::Actions;g.shop_action=1;cooldown(g);g.input(confirm,0.016f);
    check(g.round==2 && g.phase==Phase::Placement,"focused Next Round button continues");
    Game spinning;spinning.reset(true);Skin spin_skin;
    for(auto& texture:spin_skin.balls){texture.width=texture.height=1;texture.allocation=std::malloc(2);texture.pixels=static_cast<uint16_t*>(texture.allocation);texture.pixels[0]=0xffff;}
    spinning.world.balls[1].mixed_with=spinning.world.balls[2].def.id;
    spinning.world.balls[1].visual_rotation=.7f;spinning.world.balls[2].visual_rotation=.25f;
    RecordingPainter spin_view;paint_scene(spin_view,spin_skin,spinning,0,0);
    check(spin_view.rotated_regions>=2 && spin_view.rotated_sprites>=1,
          "moving mixed and ordinary balls route their surface spin through the renderer");
    g.phase=Phase::Menu;
    RecordingPainter menu; paint_scene(menu,no_assets,g,1,1000);
    check(menu.contains("PLAY CLASSIC") && menu.contains("CONTINUE RUN") && menu.circles>=6,"menu draws play, resume and deck art");
}
static void resolve_actions(Game& game) {
    for(unsigned tick=0;tick<4000 && game.pending_actions();++tick) game.update(tick_dt);
    check(!game.pending_actions(),"deferred ability actions finish within original homing lifetime");
}
static void fidelity_v04() {
    World w=isolated(); w.minimum=Vec(-200,-200); w.maximum=Vec(200,200);
    w.add(ball(Vec(0,100),Vec())); w.balls[0].cue=true; w.balls[0].def.radius=17.5f;
    w.add(ball(Vec(0,0),Vec()));
    AimPrediction a=predict_aim(w,Vec(0,-1),1);
    check(a.hit==1 && a.count==2 && near(a.points[1].y,34.5f),"aim uses combined radii and first contact");
    check(a.travel_count[1]>=2 && a.travel[1][0].y>-18 && a.travel[1][0].y< -17,
          "target guide begins at the ball surface");
    check(near(a.cue_velocity.length(),0) && near(a.object_velocity.y,-2500),"head-on equal-mass preview transfers velocity");
    check(w.balls[0].p.y==100 && w.balls[1].v.length()==0 && w.metrics.ticks==0,"aim prediction never changes simulation state");
    w.pockets_enabled=true; w.pockets[0].p=Vec(0,-100); w.pockets[0].radius=24;
    a=predict_aim(w,Vec(0,-1),1);
    check(a.travel_count[1]==2 && a.travel[1][1].y< -58 && a.travel[1][1].y> -61,
          "object guide ends at open pocket capture boundary");
    w.pockets[0].p=a.travel[1][0]; a=predict_aim(w,Vec(0,-1),1);
    check(a.travel_count[1]==2 && near(a.travel[1][1].y,a.travel[1][0].y),
          "object guide stops immediately when already inside pocket sensor");
    w.pockets_enabled=false;
    w.add(ball(Vec(0,-100),Vec())); a=predict_aim(w,Vec(0,-1),1);
    check(a.travel_count[1]==2 && near(a.travel[1][1].y,-66,0.1f),
          "object guide clips at the next ball's collision surface");
    w.balls[2].active=false; a=predict_aim(w,Vec(0,-1),1);
    check(a.travel_count[1]==3 && near(a.travel[1][1].y,-183,0.1f) && a.travel[1][2].y>0,
          "object guide reflects from the radius-adjusted rail");
    w.balls[1].def.mass=5; a=predict_aim(w,Vec(0,-1),0.5f);
    check(near(a.object_velocity.y,-416.6667f) && near(a.cue_velocity.y,833.3333f),"heavy target preview conserves momentum");
    w.balls[1].def.mass=1; w.balls[1].p=Vec(20,0); a=predict_aim(w,Vec(0,-1),1);
    check(a.hit==1 && near(a.cue_velocity.dot(a.object_velocity),0,1),"cut shot preview separates along tangent and normal");
    check(a.travel_count[0]>=1 && a.travel_count[1]>=1,"cut-shot preview draws both outgoing paths");
    check(near(a.cue_velocity.dot(a.cue_velocity)+a.object_velocity.dot(a.object_velocity),6250000,2),"cut shot preview conserves elastic energy");
    w.minimum=Vec(-200,-100); w.maximum=Vec(200,200); w.balls[0].p=Vec(-50,0); w.balls[1].p=Vec(100,-15);
    a=predict_aim(w,Vec(1,-1),1);
    check(a.count==3 && a.hit==1 && near(a.points[1].y,-82.5f),"aim previews cushion bounce before object hit");
    w.balls[1].active=false; a=predict_aim(w,Vec(1,-1),1);
    check(a.hit==-1 && a.count==3,"aim ignores inactive balls and remains bounded to two segments");
    check(predict_aim(w,Vec(),1).count==0,"zero aim vector is rejected");
    Screen top=project(Vec(-350,0)),bottom=project(Vec(350,0));
    check(top.y>=17 && bottom.y<238,"complete table canvas fits between HUD bars");
    for(const auto& pocket:w.pockets) { Screen v=project(pocket.p); check(v.x>85 && v.x<395 && v.y>17 && v.y<238,"all pocket centers remain inside fitted viewport"); }
    Pocket pock; pock.bonus=-3; pock.multiplier=3;
    check(pock.score(1)==-6,"negative pocket bonus is applied before multiplier");
    pock.multiplier=1.5f; check(pock.score(2)==-2,"fractional pocket score floors negative values");
    World closed; closed.add(ball(closed.pockets[0].p,Vec())); closed.pockets[0].closed=true; closed.capture(0,0);
    check(!closed.balls[0].falling,"closed pocket rejects direct capture");
    closed.step(); check(!closed.balls[0].falling && closed.metrics.solver_exhaustions==0,"closed pocket is excluded from swept candidates");

    Game g; g.reset(); g.inventory_count=15;
    for(unsigned i=0;i<15;++i) { g.inventory[i].id=1; g.inventory[i].score=int(i+1); }
    g.begin_round(); check(g.world.count==11,"only first ten inventory positions deploy");
    g.score=g.target; g.finish_round();
    check(g.move_item(0,14) && g.inventory[14].score==1 && g.inventory[0].score==15,"rack/reserve swap preserves both owned items");
    g.next_round(); check(g.world.balls[1].def.base_score==15 && g.world.count==11,"rack swap changes deployed ball next round");
    g.score=g.target; g.finish_round(); int held=g.money; g.sell(4);
    check(g.inventory[4].id==0 && g.inventory[5].score==6 && g.money>held,"selling preserves sparse slot positions");
    for(unsigned n=0;n<100;++n) {
        g.stock_shop();
        for(unsigned i=0;i<3;++i) { check(shop_eligible(g.offers[i],false),"stock uses reference eligible supported pool"); for(unsigned j=0;j<i;++j) check(g.offers[i]!=g.offers[j],"stock offers are distinct"); }
    }
    check(!shop_eligible(3,false) && !shop_eligible(8,false) && !shop_eligible(15,false),"disabled number/Basketball records are not shop drops");
    g.reset(); g.round=10; g.begin_round(); g.score=0;
    check(g.finish_round() && g.phase==Phase::Shop && g.hp==2,"losing target round does not declare run victory");
    g.next_round(); check(g.round==10,"failed rounds retry the same level rather than advancing");
    g.reset(); g.round=10; g.begin_round(); g.score=g.target;
    check(g.finish_round() && g.phase==Phase::RunWon,"winning target round completes run");
    g.reset(); g.score=g.target; g.finish_round();
    g.inventory[0].id=9; g.inventory[0].level=2; g.inventory[0].score=3;
    g.inventory[1]=g.inventory[0];
    check(g.combine_owned(0,1) && g.inventory[1].level==3 && g.inventory[1].score==6 && !g.inventory[0].id,"owned matching level-two balls upgrade to level three");
    g.inventory[0]=g.inventory[1]; g.inventory[1].level=5;
    check(!g.combine_owned(0,1),"owned upgrade cannot exceed level five");
    g.inventory[0].id=8; g.inventory[0].score=8; g.inventory[1].id=1; g.inventory[1].score=1;
    check(g.combine_owned(0,1) && g.inventory[1].id==26 && g.inventory[1].score==9,"number total above eight becomes Overflow Ball");

    // Focused ability tests submit the same pocket event used by swept physics.
    auto pocket_case=[](unsigned id) {
        Game x; x.reset(); x.world=World(); Ball cue; cue.cue=true; x.world.add(cue);
        Ball b; b.def=builtin(id); b.p=Vec(30,30); b.active=false; x.world.add(b);
        x.world.events.push(EventKind::Pocketed,1,0,0); return x;
    };
    g=pocket_case(18); g.dispatch(); resolve_actions(g); int sum=0,changed=0;
    for(const auto& p:g.world.pockets) { sum+=p.bonus; changed+=p.bonus!=0; }
    check(sum==2 && changed==2,"Beach Ball buffs two different pockets");
    g=pocket_case(23); g.world.balls[1].level=3; g.dispatch(); resolve_actions(g); float mult=0;
    for(const auto& p:g.world.pockets) mult+=p.multiplier;
    check(mult==18,"level-three Pants adds six pocket multiplier increments");
    g=pocket_case(24); g.dispatch(); resolve_actions(g); sum=0; changed=0;
    for(unsigned i=0;i<6;++i) if(g.world.pockets[i].bonus) { sum+=g.world.pockets[i].bonus; ++changed; check(i!=0,"Yinyang excludes the used physical pocket"); }
    check(changed==1 && sum==-2,"level-one Yinyang applies original multiplier tradeoff");
    g=pocket_case(25); g.world.pockets[0].bonus=4; g.dispatch(); resolve_actions(g);
    check(g.score==12 && g.world.pockets[0].bonus==0 && g.world.pockets[1].bonus==6 && g.world.pockets[5].bonus==6,"Juggling scores before distributing its pocket bonus");
    g=pocket_case(11); Ball dice; dice.def=builtin(19); dice.level=2; g.world.add(dice);
    Ball roulette; roulette.def=builtin(22); g.world.add(roulette); Ball plush; plush.def=builtin(21); g.world.add(plush);
    g.world.pockets_enabled=false; g.dispatch(); sum=0; for(const auto& p:g.world.pockets) sum+=p.bonus;
    check(g.world.balls[2].def.base_score>=1 && g.world.balls[2].def.base_score<=12,"Dice reacts to another pocket at its level range");
    check(sum==0 && g.world.balls[4].v.length()==0 && g.pending_actions(),"Roulette pocket buff and Plushie impulse are deferred");
    g.update(2*tick_dt);
    check(g.world.balls[4].v.length()>0,"Plushie restarts motion after two physics frames");
    resolve_actions(g); sum=0;for(const auto& p:g.world.pockets) sum+=p.bonus;
    check(sum==1,"Roulette pocket bonus arrives through its wisp");
    g=pocket_case(1); g.world.pockets[0].bonus=-4; g.dispatch(); check(g.score==-6,"negative scoring is not discarded");
    g.reset(); g.snacks={Olive,Croissant,Strawberry,HamSandwich}; g.begin_round();
    check(g.shots==6 && g.world.balls[1].def.base_score==2,"Olive applies immediately while Croissant creates buff carriers");
    resolve_actions(g);check(g.world.balls[1].def.base_score==3,"Croissant buff arrives after its carrier");
    sum=0; for(const auto& p:g.world.pockets) { sum+=p.bonus; check(p.multiplier==3,"Ham Sandwich increases every multiplier"); }
    check(sum==-15,"Strawberry bonus combines with Ham Sandwich penalties");
    g.reset(); g.snacks={Cereal,PokerChip,Chocolate,0}; g.begin_round();
    g.world.events.push(EventKind::Collision,0,1,0); g.dispatch();
    check(g.world.balls[1].def.base_score==3,"Chocolate buffs first object hit by cue");
    g.world.events.push(EventKind::Collision,0,2,0); g.dispatch();
    check(g.world.balls[2].def.base_score==1,"Chocolate triggers only once per shot");
    g.world.balls[1].active=false; g.world.events.push(EventKind::Pocketed,1,0,0); g.dispatch();
    check(g.inventory[0].score==3 && g.world.pockets[0].bonus==0,"Cereal permanent buff precedes deferred Poker Chip pocket buff");
    resolve_actions(g);check(g.world.pockets[0].bonus==1,"Poker Chip wisp buffs the used pocket");
    g.world.balls[2].active=false; g.world.events.push(EventKind::Pocketed,2,0,0); g.dispatch();
    check(g.inventory[1].score==1,"Cereal permanent buff occurs only once each round");
    g.reset(); g.snacks={Muffin,0,0,0}; g.target=2; g.score=0;
    check(!g.finish_round() && g.pending_actions(),"Muffin delays round end while its target wisp travels");
    resolve_actions(g);check(g.finish_round() && g.last_round_won && g.phase==Phase::Shop,"Muffin can rescue round before final win/loss evaluation");
    g.reset(); g.difficulty=1; g.begin_round(); g.score=g.target; g.finish_round();
    check(g.snack_tickets==1,"Wine Mixer grants first ticket on advancing to round two");
    g.snack_offers={Olive,Chocolate,Cereal}; held=g.money;
    check(g.buy_snack(0) && g.has_snack(Olive) && g.snack_tickets==0 && g.money==held,"snack choice spends ticket rather than money");
    check(!g.buy_snack(0),"snack purchases require a ticket");
    check(g.sell_snack(0) && !g.has_snack(Olive),"owned snack can be sold");
    g.move_item(0,14); g.snacks={Cereal,Chocolate,0,0}; g.snack_tickets=2;
    check(g.save_run("build/fidelity-save.sav"),"save valid shop checkpoint");
    Game restored; check(restored.load_run("build/fidelity-save.sav"),"load valid checkpoint");
    check(restored.phase==Phase::Shop && restored.round==g.round && restored.money==g.money && restored.inventory[14].id==g.inventory[14].id && restored.snacks==g.snacks && restored.offers==g.offers,"save roundtrip preserves sparse ownership/snacks/stock");
    check(restored.snack_tickets==2 && restored.difficulty==1,"save roundtrip preserves difficulty and tickets");
    check(g.save_run("build/fidelity-save.sav"),"second save retains backup");
    FILE* f=std::fopen("build/fidelity-save.sav","wb"); std::fputs("corrupt",f); std::fclose(f);
    check(restored.load_run("build/fidelity-save.sav"),"corrupt primary recovers previous valid checkpoint");
    std::remove("build/fidelity-save.sav.bak"); int before=restored.money;
    check(!restored.load_run("build/fidelity-save.sav") && restored.money==before,"invalid save is rejected without changing game");
    g.phase=Phase::Simulating; check(!g.save_run("build/fidelity-save.sav"),"saving is restricted to durable shop checkpoints");
    std::remove("build/fidelity-save.sav");
    Skin empty; RecordingPainter snack_view; g.phase=Phase::Shop;g.round_summary=false; g.snack_bar=true;
    paint_scene(snack_view,empty,g,0,0);
    check(snack_view.contains("THE SNACK BAR") && snack_view.contains("YOUR SNACKS"),"shared renderer presents snack choice and owned slots");
    g.sell_pending=true; Input cancel; cancel.cancel=true; g.input(cancel,0.016f);
    check(!g.snack_bar && !g.sell_pending && !g.merge_pending,"leaving snack bar cancels its pending sale");
}
static void cancel_fidelity() {
    Game g; g.reset(); Vec original=g.world.balls[0].p;
    Input input; input.analog_x=1; g.input(input,0.05f);
    check(g.phase==Phase::Aiming,"stick prepares aim without placement");
    check((g.world.balls[0].p-original).length()==0,"stick never translates cue before first shot");
    input=Input(); input.cancel=true; input.confirm=true; int shots=g.shots;
    g.input(input,0.016f);
    check(g.phase==Phase::Placement && g.shots==shots,"cancel wins over simultaneous shoot");
    for(int i=0;i<100;++i) { input=Input(); input.analog_y=1; input.right=true; g.input(input,0.05f); }
    check((g.world.balls[0].p-original).length()==0,"cancel and reaim cannot reposition cue");
    input=Input(); input.cancel=true; g.input(input,0.016f);
    input=Input(); input.confirm=true; g.input(input,0.016f);
    check(g.phase==Phase::Aiming && g.shots==shots,"confirm after cancel prepares without firing");
}
static void spawn_and_action_fidelity() {
    Game g=arena(30); g.world.balls[1].active=false; g.world.balls[1].level=3;
    g.world.events.push(EventKind::Pocketed,1,0,0); g.dispatch();
    check(g.pending_actions() && g.world.count==2,"Slot defers spawn until wisp arrives");
    check(!g.finish_round(),"round cannot end before a spawn resolves");
    g.phase=Phase::Simulating;
    for(int i=0;i<1400 && g.pending_actions();++i) g.update(tick_dt);
    check(!g.pending_actions() && g.world.count==3,"Slot wisp creates its ball");
    check(number_ball(g.world.balls[2].def.id) && g.world.balls[2].def.base_score>=1 && g.world.balls[2].def.base_score<=21,"Slot uses original level-dependent number range");
    check(g.world.balls[2].owner==-1,"temporary ball has no permanent inventory owner");
    g=arena(31); g.world.balls[1].active=false; g.world.balls[1].level=4; g.world.balls[1].def.base_score=19;
    g.world.events.push(EventKind::Pocketed,1,0,0); g.dispatch();
    for(int i=0;i<1400 && g.pending_actions();++i) g.update(tick_dt);
    check(g.world.count==3 && g.world.balls[2].level==3 && g.world.balls[2].def.base_score==19,"Nesting Doll keeps value and reduces copy level");
    g=arena(31); g.world.balls[1].active=false; g.world.events.push(EventKind::Pocketed,1,0,0); g.dispatch();
    check(!g.pending_actions(),"level-one Nesting Doll does not copy");
    g.reset(); g.inventory[0].id=32; g.inventory[0].level=3; g.inventory[0].score=11; g.begin_round();
    for(int i=0;i<1400 && g.pending_actions();++i) g.update(tick_dt);
    check(g.world.count==5 && g.world.balls[4].def.id==32 && g.world.balls[4].level==3 && g.world.balls[4].def.base_score==11,"Mirror copies first deployed item with its level");
    check(!g.pending_actions(),"Mirror source marker blocks recursive spawning");
    g=arena(1); g.queue_impulse(1,Vec(120,0)); g.paused=true;
    g.update(2); check(g.world.balls[1].v.length()==0 && g.pending_actions(),"pause freezes deferred impulses");
    g.paused=false; g.update(tick_dt); check(g.world.balls[1].v.length()==0,"first deferred tick has no impulse");
    g.update(tick_dt); check(near(g.world.balls[1].v.x,120),"second deferred tick applies impulse exactly once");
    g=arena(1); g.queue_impulse(1,Vec(120,0)); uint32_t old=g.world.balls[1].generation;
    g.world.balls[1].generation=old+1; g.update(2*tick_dt);
    check(g.world.balls[1].v.length()==0,"stale impulse cannot affect replacement generation");
    g.world.balls[1].def=builtin(9); g.world.events.push(EventKind::Wall,1,0,0,old); g.dispatch();
    check(g.score==0,"stale event cannot affect replacement generation");
    World w=isolated(); for(unsigned i=0;i<64;++i) check(w.add(ball(Vec(i*40.0f,0),Vec())),"fill instance slots");
    for(unsigned i=0;i<200;++i) { unsigned slot=1+i%63; uint32_t prior=w.balls[slot].generation; w.balls[slot].active=false;
        check(w.add(ball(Vec(),Vec())) && w.last_added==slot && w.balls[slot].generation!=prior,"retired slot reuse increments identity generation"); }
    check(w.count==64,"repeated spawns keep bounded instance storage");
    g.reset(); Vec start=g.world.balls[0].p; float distance=(start-Vec(0,210)).length();
    check(distance>=1 && distance<=100,"initial cue uses original area-uniform spawn annulus");
    g.phase=Phase::Simulating; g.world.balls[1].v=Vec(100,0); g.world.balls[0].active=false;
    g.world.events.push(EventKind::Pocketed,0,0,0); g.dispatch();
    check(g.world.balls[0].active && g.world.balls[1].v.length()>0,"cue returns while another ball is still moving");
    check((g.world.balls[0].p-start).length()>=1 && (g.world.balls[0].p-start).length()<=100,"scratch returns around captured initial spawn, matching PlayerBall.start_pos");
    g=arena(1); g.phase=Phase::Simulating; g.world.balls[1].v=Vec(50,0);
    g.world.balls[0].v=Vec(400,0); g.angle=1.570796327f; g.power=0.5f;
    check(g.shoot() && near(g.world.balls[0].v.x,0) && near(g.world.balls[0].v.y,1250),"shot resets moving cue velocity before applying impulse");
    check(near(g.world.balls[1].v.x,50),"mid-motion cue shot preserves other velocities");
    g.world.balls[0].falling=true;
    check(!g.shoot(),"falling cue cannot shoot");
    g=arena(1);
    for(unsigned i=0;i<29;++i) g.world.add(ball(Vec(1000+float(i*50),0),Vec()));
    Ball capped; capped.def=builtin(1); g.queue_spawn(capped,Vec(),Slot);
    for(unsigned i=0;i<1400 && g.pending_actions();++i) g.update(tick_dt);
    check(g.world.count==31 && g.spawn_rejections==1,"spawn completion enforces original thirty-active-ball cap");
    g=arena(1); Ball spawn; spawn.def=builtin(1); check(g.queue_spawn(spawn,Vec(),Slot),"queue capacity test spawn");
    auto before=g.actions; g.paused=true; g.update(2);
    check((g.actions[0].p-before[0].p).length()==0,"pause freezes wisps");
    g.paused=false;
    for(unsigned i=0;i<63;++i) check(g.queue_impulse(1,Vec(1,0)),"fill bounded action storage");
    check(g.queue_impulse(1,Vec(1,0)) && g.action_overflows==0,"65th action is retained instead of dropping an ability");
}
static void cocktail_interface() {
    FILE* fixture=std::fopen("build/mixed-icon.upt","wb"); check(fixture!=nullptr,"create mixed icon fixture");
    const unsigned char pixels[]={'U','P','T','1',2,0,1,0,0x8f,0xf0,0xff,0xff};
    std::fwrite(pixels,1,sizeof(pixels),fixture);std::fclose(fixture);
    Skin skin; check(load_texture("build/mixed-icon.upt",skin.balls[9]),"load synthetic primary half");
    check(load_texture("build/mixed-icon.upt",skin.balls[11]),"load synthetic secondary half");
    Game g; g.reset(); g.phase=Phase::Shop; g.difficulty=2; g.cocktail_tickets=1;
    g.inventory={}; g.inventory_count=2;
    g.inventory[0].id=9;g.inventory[0].level=2;g.inventory[0].score=3;
    g.inventory[1].id=11;g.inventory[1].level=3;g.inventory[1].score=7;
    Input input;input.less=true;g.input(input,0.016f);
    check(g.cocktail_bar && !g.snack_bar,"L from Malibu ball shop opens cocktail bar");
    input=Input();input.confirm=true;g.input(input,0.016f);
    check(g.mixing_pending && g.reorder_source==0,"first cocktail selection stays in inventory until confirmation");
    cooldown(g);cooldown(g);input=Input();input.right=true;g.input(input,0.016f);
    RecordingPainter preview;paint_scene(preview,skin,g,0,0);
    check(preview.contains("THE COCKTAIL BAR") && preview.contains("Lv3 VALUE 10"),"cocktail interface previews original result level and value");
    check(preview.regions==2,"mixed preview renders complementary source texture halves");
    input=Input();input.confirm=true;g.input(input,0.016f);
    check(!g.mixing_pending && g.inventory[0].mixed_with==11 && g.inventory[0].score==10 && g.cocktail_tickets==0,"cocktail UI commits confirmed two-ball mixture");
    g.cocktail_bar=false;g.shop_focus=ShopFocus::Inventory;g.inventory_selection=0;
    RecordingPainter owned;paint_scene(owned,skin,g,0,0);
    check(owned.regions==2 && owned.contains("Pinball + Gold"),"shop shows mixed artwork and both names");
    g.cocktail_bar=true;g.mixing_pending=true;
    input=Input();input.cancel=true;g.input(input,0.016f);
    check(g.cocktail_bar && !g.mixing_pending && g.inventory_count==1,"cancel selection preserves cocktail and item state");
    g.input(input,0.016f);check(!g.cocktail_bar,"second cancel leaves empty cocktail selection");
    std::remove("build/mixed-icon.upt");
}
static void benchmark() {
    World w=isolated();
    for (int i=0;i<32;++i) w.add(ball(Vec((i%8)*45,(i/8)*45),Vec(i*3,10)));
    auto start=std::chrono::steady_clock::now();
    for (int i=0;i<18000;++i) { w.step(); Event e; while(w.events.pop(e)) {} }
    double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::printf("Host benchmark: 32 balls, 18000 ticks, %.2f ms, %.2f us/tick\n",ms,ms*1000/18000);
    std::printf("sizeof(World)=%zu sizeof(Game)=%zu; PSP timing must be measured separately\n",sizeof(World),sizeof(Game));
}
unsigned mixed_ball_tests();
unsigned delayed_effect_tests();
unsigned listener_tests();
unsigned buff_tests();
unsigned contact_tests();
unsigned modifier_tests();
unsigned fire_tests();
unsigned split_tests();
unsigned passive_tests();
unsigned pickup_tests();
unsigned audio_tests();
unsigned ui_refresh_tests();
unsigned menu_features_tests();
unsigned audit_fix_tests();
unsigned content_batch_tests();
unsigned projectile_tests();
unsigned movement_tests();
unsigned hole_tests();
unsigned summon_tests();
unsigned collection_tests();
unsigned ritual_tests();
unsigned passive_body_tests();
unsigned identity_effect_tests();
unsigned score_contact_tests();
unsigned progression_tests();
unsigned daily_tests();
unsigned shop_transform_tests();
int main(int argc,char** argv) {
    std::setvbuf(stdout,nullptr,_IONBF,0);
    if(argc==2 && !std::strcmp(argv[1],"--projectiles")){projectile_tests();return 0;}

    std::puts("Core: collisions");collisions(); walls_pockets_damping(); game_events_resources(); scratch_and_resources(); stress(); abilities_and_rounds();std::puts("Core: visual shop/resources"); visual_shop_and_textures(); fidelity_v04(); cancel_fidelity(); spawn_and_action_fidelity(); assertions+=mixed_ball_tests(); cocktail_interface(); assertions+=delayed_effect_tests(); assertions+=listener_tests(); assertions+=buff_tests(); assertions+=contact_tests(); assertions+=modifier_tests(); assertions+=fire_tests(); assertions+=split_tests(); assertions+=passive_tests(); assertions+=pickup_tests(); assertions+=audio_tests();assertions+=ui_refresh_tests();assertions+=menu_features_tests();assertions+=audit_fix_tests();assertions+=content_batch_tests();assertions+=projectile_tests();assertions+=movement_tests();assertions+=hole_tests();assertions+=summon_tests();assertions+=collection_tests();assertions+=ritual_tests();assertions+=passive_body_tests();assertions+=identity_effect_tests();assertions+=score_contact_tests();assertions+=progression_tests();assertions+=daily_tests(); assertions+=shop_transform_tests(); benchmark();
    std::printf("PASS: %d assertions\n",assertions); return 0;
}
