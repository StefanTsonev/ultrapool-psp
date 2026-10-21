#include "ultrapool/presentation.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
using namespace up;
namespace {
unsigned checks = 0;
void check(bool ok, const char *why) {
  ++checks;
  if (!ok) {
    std::fprintf(stderr, "FAIL AUDIT: %s\n", why);
    std::exit(1);
  }
}
} // namespace
unsigned audit_fix_tests() {
  // More than four times the former capacity, with exact-once completion.
  std::unique_ptr<Game> g_fixture(new Game);Game& g=*g_fixture;
  g.reset();
  g.world = World();
  g.world.pockets_enabled = false;
  g.world.stop_speed = 0;
  Ball cue;
  cue.cue = true;
  g.world.add(cue);
  Ball b;
  b.p = Vec(100, 0);
  b.def.damping = 0;
  g.world.add(b);
  for (unsigned i = 0; i < 300; ++i)
    check(g.queue_impulse(1, Vec(1, 0)), "retain each simultaneous impulse");
  check(g.action_high_water == 300 && g.action_overflows == 0,
        "action storage grows with measured demand");
  g.update(2 * tick_dt);
  check(std::fabs(g.world.balls[1].v.x - 300) < .001f,
        "all 300 impulses execute exactly once");
  g.update(tick_dt);
  check(std::fabs(g.world.balls[1].v.x - 300) < .001f,
        "completed impulses do not replay");
  for (int level : {1, 3}) {
    std::unique_ptr<Game> chain_fixture(new Game);Game& chain=*chain_fixture;
    chain.reset();
    chain.inventory = {};
    chain.inventory_count = 10;
    chain.round = 10;
    chain.difficulty = 2;
    for (unsigned i = 0; i < 10; ++i) {
      OwnedBall x;
      x.id = id_of(i < 5 ? BallKey::GAIA : BallKey::SAND);
      x.mixed_with = id_of(BallKey::BUTTERFLY);
      x.level = level;
      x.score = 1;
      chain.inventory[i] = x;
    }
    chain.snacks[0] = id_of(SnackKey::CROISSANT);
    chain.snacks[1] = id_of(SnackKey::ESPRESSO);
    chain.begin_round();
    for (unsigned i = 0; i < 1200; ++i)
      chain.update(tick_dt);
    check(chain.action_overflows == 0,
          "Butterfly Gaia Sand chain loses no actions");
    check(chain.action_high_water > 64,
          "regression exercises old overflow path");
    std::printf("Chain L%d: score=%d actions-high-water=%u storage=%zu bytes\n",
                level, chain.score, chain.action_high_water,
                chain.action_high_water * sizeof(Action));
  }
  std::unique_ptr<Game> shop_fixture(new Game);Game& shop=*shop_fixture;
  shop.reset();
  shop.phase = Phase::Shop;
  shop.shop_focus = ShopFocus::Market;
  shop.offers[0] = 9;
  int cash = shop.money;
  auto count = shop.inventory_count;
  Input select;
  select.auxiliary = true;
  shop.input(select, .016f);
  check(shop.offer_details && shop.money == cash &&
            shop.inventory_count == count,
        "Select opens non-mutating offer details");
  Input close;
  close.cancel = true;
  shop.input(close, .016f);
  check(!shop.offer_details, "Circle closes offer details");
  shop.shop_focus = ShopFocus::Inventory;
  shop.input(select, .016f);
  check(shop.reorder_pending, "Select gives direct move access");
  shop.roster_details = shop.passive_details = shop.offer_details = true;
  shop.begin_round();
  check(!shop.roster_details && !shop.passive_details && !shop.offer_details,
        "new round clears transient info modals");
  std::unique_ptr<Game> settings_fixture(new Game);Game& settings=*settings_fixture;
  settings.reset();
  settings.paused = true;
  Input right;
  right.right = true;
  int shots = settings.shots;
  float power = settings.power;
  settings.input(right, .016f);
  check(settings.music_percent == 27 && settings.settings_dirty,
        "pause changes music volume independently");
  Input down;
  down.down = true;
  settings.input(down, .2f);
  settings.input(down, .2f);
  settings.input(down, .2f); // release repeat delay
  settings.settings_selection = 1;
  for (unsigned i = 0; i < 5; ++i)
    settings.input(Input(), .05f);
  settings.input(right, .016f);
  check(settings.sfx_percent == 70 && settings.music_percent == 27 &&
            settings.shots == shots && settings.power == power,
        "audio controls leave gameplay unchanged");
  settings.audio_muted = true;
  check(settings.save_settings("build/audit-options.cfg"),
        "save checksummed sound preferences");
  std::unique_ptr<Game> restored_fixture(new Game);Game& restored=*restored_fixture;
  check(restored.load_settings("build/audit-options.cfg") &&
            restored.music_percent == 27 && restored.sfx_percent == 70 &&
            restored.audio_muted,
        "sound preferences survive restart");
  FILE *corrupt = std::fopen("build/audit-options.cfg", "wb");
  check(corrupt != nullptr, "open corrupt settings fixture");
  std::fputs("broken", corrupt);
  std::fclose(corrupt);
  restored.music_percent = 49;
  check(!restored.load_settings("build/audit-options.cfg") &&
            restored.music_percent == 49,
        "corrupt settings do not mutate preferences");
  std::remove("build/audit-options.cfg");
  std::unique_ptr<Game> rat_fixture(new Game);Game& rat=*rat_fixture;
  rat.reset();
  rat.inventory[0] = default_item(id_of(BallKey::RAT));
  rat.inventory[1] = default_item(id_of(BallKey::RAT));
  rat.inventory[2] = default_item(id_of(BallKey::RAT));
  rat.begin_round();
  check(rat.world.balls[1].def.base_score == 3 &&
            rat.world.balls[2].def.base_score == 3 &&
            rat.world.balls[3].def.base_score == 3,
        "Rat setup spawn counts other Rat identities without buffing owners");
  check(rat.inventory[0].score == 1, "Rat spawn bonus remains temporary");
  std::unique_ptr<Game> heal_fixture(new Game);Game& heal=*heal_fixture;
  heal.reset();
  heal.phase = Phase::Shop;
  heal.hp = 1;
  heal.snacks[0] = id_of(SnackKey::LOVE_CAKE);
  check(heal.sell_snack(0) && heal.hp == 2, "Love Cake sale heals one heart");
  heal.snacks[0] = id_of(SnackKey::LOVE_CAKE);
  heal.hp = 3;
  heal.sell_snack(0);
  check(heal.hp == 3, "Love Cake never exceeds maximum HP");
  heal.snacks[0] = id_of(SnackKey::VITAMINS);
  auto prior = heal.inventory_count;
  check(heal.sell_snack(0) && heal.inventory_count == prior + 1 &&
            heal.inventory[3].id == id_of(BallKey::UPGRADE_BALL),
        "Vitamins sale grants one Upgrade Ball");
  std::unique_ptr<Game> menu_fixture(new Game);Game& menu=*menu_fixture;
  menu.phase = Phase::Menu;
  menu.menu_preview_checked = true;
  menu.continue_available = false;
  Input next;
  next.down = true;
  menu.input(next, .016f);
  check(menu.selection != 1, "disabled Continue does not take focus");
  char description[256];
  describe_ability(Pinball, 5, description, sizeof(description));
  check(std::strstr(description, "+5") != nullptr,
        "inspection shows actual level-dependent points");
  describe_ability(Bomb, 3, description, sizeof(description));
  check(std::strstr(description, "24") != nullptr,
        "inspection resolves scaled ability threshold");
  describe_ability(NestingDoll, 1, description, sizeof(description));
  check(std::strstr(description, "no copy") != nullptr,
        "level-one copy exception is explicit");
  std::unique_ptr<Game> snack_info_fixture(new Game);Game& snack_info=*snack_info_fixture;
  snack_info.reset();
  snack_info.phase = Phase::Shop;
  snack_info.snack_bar = true;
  snack_info.shop_focus = ShopFocus::Inventory;
  snack_info.snacks[0] = id_of(SnackKey::OLIVE);
  snack_info.input(Input{}, .016f);
  Input detail;
  detail.confirm = true;
  snack_info.input(detail, .016f);
  check(snack_info.passive_details &&
            snack_info.snacks[0] == id_of(SnackKey::OLIVE),
        "owned snack opens full details without consuming it");
  Input back;
  back.cancel = true;
  snack_info.input(back, .016f);
  check(!snack_info.passive_details && snack_info.snack_bar,
        "closing snack details keeps snack bar");
  snack_info.snack_bar = false;
  snack_info.shop_focus = ShopFocus::Passive;
  Input sell;
  sell.finish = true;
  snack_info.input(sell, .016f);
  check(snack_info.sell_pending && snack_info.snacks[0],
        "main-shop passive sale requires confirmation");
  snack_info.input(back, .016f);
  check(snack_info.snacks[0], "cancel passive sale keeps passive");
  snack_info.shop_focus = ShopFocus::Passive;
  snack_info.input(sell, .016f);
  snack_info.input(detail, .016f);
  check(!snack_info.snacks[0], "Triangle and X sell the selected passive");
  World doors;
  Ball trapped; trapped.p=doors.pockets[0].p;
  doors.add(trapped);
  check(doors.set_pocket_closed(0,true), "close an open pocket");
  doors.capture(0,0);
  check(!doors.balls[0].falling,"closed pocket rejects capture");
  check(doors.set_pocket_closed(0,false) && doors.balls[0].falling,
        "opening pocket immediately captures overlapping object");
  check(doors.set_pocket_closed(0,true) && doors.balls[0].falling,
        "closing does not eject an already captured ball");
  check(!doors.set_pocket_closed(6,false),"invalid pocket is rejected");
  std::unique_ptr<Game> bouncer_fixture(new Game);Game& bouncer=*bouncer_fixture; bouncer.reset(); bouncer.world=World();
  Ball guard_cue; guard_cue.cue=true; bouncer.world.add(guard_cue);
  check(bouncer.set_cube(0,id_of(BallKey::BOUNCER),3),"equip nonphysical Bouncer cube");
  bouncer.start_round_effects();
  unsigned closed=0;for(const auto& pocket:bouncer.world.pockets) closed+=pocket.closed;
  check(closed==3 && bouncer.world.count==1,"Bouncer closes pockets without a physical ball");
  bouncer.run_cube_event(Signal::PocketAnother,0);
  bouncer.run_cube_event(Signal::PocketAnother,0);
  check(bouncer.cubes[0].count==2,"cube counter starts at zero and is independent of ball value");
  bouncer.run_cube_event(Signal::PocketAnother,0);
  closed=0;for(const auto& pocket:bouncer.world.pockets) closed+=pocket.closed;
  check(closed==3 && bouncer.cubes[0].count==0,"Bouncer cycles lock after three pockets");
  std::unique_ptr<Game> patch_fixture(new Game);Game& patch=*patch_fixture;patch.reset();patch.world=World();patch.world.add(guard_cue);
  check(patch.set_cube(0,id_of(BallKey::PATCHWORK),2),"equip Patchwork");
  patch.start_round_effects();for(unsigned tick=0;tick<360;++tick)patch.update(tick_dt);
  for(const auto& pocket:patch.world.pockets) check(pocket.multiplier==1,"Patchwork round-start penalty");
  for(unsigned i=0;i<6;++i) patch.run_cube_event(Signal::PocketAnother,0);
  for(unsigned tick=0;tick<360;++tick)patch.update(tick_dt);
  for(const auto& pocket:patch.world.pockets) check(pocket.multiplier==2,"Patchwork restores multiplier once");
  patch.run_cube_event(Signal::PocketAnother,0);for(unsigned tick=0;tick<360;++tick)patch.update(tick_dt);
  check(patch.cubes[0].count==6 && patch.world.pockets[0].multiplier==2,"Patchwork restoration is capped");
  check(!patch.set_cube(15,id_of(BallKey::DEMON),1) && !patch.set_cube(1,id_of(BallKey::DEMON),6),"reject invalid cube slot and level");
  std::unique_ptr<Game> demon_fixture(new Game);Game& demon=*demon_fixture;demon.reset();demon.world=World();demon.world.add(guard_cue);
  demon.set_cube(0,id_of(BallKey::DEMON),5);demon.start_round_effects();for(unsigned tick=0;tick<360;++tick)demon.update(tick_dt);
  check(demon.world.count==2 && demon.world.balls[1].def.id==id_of(BallKey::IMP) && demon.world.balls[1].def.base_score==-10,"Demon spawns negative Imp at round start");
  for(unsigned i=0;i<3;++i)demon.run_cube_event(Signal::PocketAnother,0);
  for(unsigned tick=0;tick<360;++tick)demon.update(tick_dt);
  check(demon.world.count==3 && demon.cubes[0].count==0,"level-five Demon spawns every three pockets");
  patch.phase=Phase::Shop;
  check(patch.save_run("build/cube-roundtrip.sav"),"cube run saves atomically");
  std::unique_ptr<Game> cube_restored_fixture(new Game);Game& cube_restored=*cube_restored_fixture;cube_restored.reset();
  check(cube_restored.load_run("build/cube-roundtrip.sav") && cube_restored.cubes[0].id==id_of(BallKey::PATCHWORK) && cube_restored.cubes[0].level==2 && cube_restored.cubes[0].count==0,"cube identity and level persist; round counter resets");
  cube_restored.reset();check(!cube_restored.cubes[0].id,"new run clears curse cubes");
  std::remove("build/cube-roundtrip.sav");std::remove("build/cube-roundtrip.sav.bak");
  for(unsigned level=1;level<=5;++level) {
    std::unique_ptr<Game> paperwork_storage(new Game);Game& paperwork=*paperwork_storage;paperwork.reset();paperwork.world=World();paperwork.world.add(guard_cue);
    paperwork.set_cube(0,id_of(BallKey::PAPERWORK),level);
    for(unsigned i=0;i<10;++i)paperwork.run_cube_event(Signal::PocketAnother,0,6);
    check(paperwork.cubes[0].count==0,"Paperwork ignores virtual pockets entirely");
    for(unsigned i=0;i<3;++i)paperwork.run_cube_event(Signal::PocketAnother,0,0);
    for(unsigned t=0;t<360;++t)paperwork.update(tick_dt);
    int sum=0;for(const auto& pocket:paperwork.world.pockets)sum+=pocket.bonus;
    check(sum==-1-static_cast<int>(level),"Paperwork level-scaled penalty after three real pockets");
    std::unique_ptr<Game> weights_storage(new Game);Game& weights=*weights_storage;weights.reset();weights.world=World();weights.world.add(guard_cue);
    Ball object;object.p=Vec(100,100);weights.world.add(object);
    weights.set_cube(0,id_of(BallKey::WEIGHTS),level);weights.start_round_effects();
    for(unsigned t=0;t<360;++t)weights.update(tick_dt);
    check(weights.world.balls[1].weight==Weight::Heavy,"Weights applies heavy through delayed modifier");
    check(!weights.set_cube(1,id_of(BallKey::WEIGHTS),level),"duplicate curse identities are rejected");
    std::unique_ptr<Game> glitch_storage(new Game);Game& glitch=*glitch_storage;glitch.reset();glitch.world=World();glitch.world.add(guard_cue);glitch.world.add(object);
    glitch.set_cube(0,id_of(BallKey::GLITCH),level);glitch.start_round_effects();
    for(unsigned t=0;t<360;++t)glitch.update(tick_dt);
    check((glitch.world.balls[1].p-object.p).length()>1 && glitch.world.balls[1].generation==1,"Glitch relocates without respawn or generation change");
  }
  for(int round : {1,5,10,20}) {
    std::unique_ptr<Game> cubert_storage(new Game);Game& cubert=*cubert_storage;cubert.reset();cubert.world=World();cubert.world.add(guard_cue);cubert.round=round;
    cubert.set_cube(0,id_of(BallKey::CUBERT),1);cubert.start_round_effects();
    for(unsigned t=0;t<360;++t)cubert.update(tick_dt);
    check(cubert.world.count==1+static_cast<unsigned>(round<10?round:10),"Cubert spawns round-scaled capped Tungsten count");
    for(unsigned i=1;i<cubert.world.count;++i)check(cubert.world.balls[i].def.id==id_of(BallKey::TUNGSTEN_BALL) && cubert.world.balls[i].def.base_score==-1 && cubert.world.balls[i].def.mass==60,"Cubert Tungsten preserves negative value and mass");
  }
  ListenerRegistry capacity;
  for(unsigned i=0;i<82;++i){EntityRef ref;ref.index=i;capacity.add(ref,signal_bit(Signal::PocketAnother));}
  check(capacity.snapshot(Signal::PocketAnother).count==30 && capacity.overflows==52,"registry preserves original per-event cap across balls, snacks and cubes");
  for(unsigned level=1;level<=5;++level) {
    std::unique_ptr<Game> holder(new Game);Game& curses=*holder;curses.reset();curses.world=World();curses.world.add(guard_cue);Ball standing;standing.def=builtin(9);standing.p=Vec(0,-100);curses.world.add(standing);
    curses.set_cube(0,id_of(BallKey::THORNS),level);curses.start_round_effects();
    unsigned thorns=0;for(const auto& pickup:curses.pickups)thorns+=pickup.active && pickup.kind==PickupKind::Thorn;
    check(thorns==2,"Thorns produces two hazards at round start");
    for(unsigned i=0;i<6-level;++i)curses.run_cube_event(Signal::PocketAnother,0);
    thorns=0;for(const auto& pickup:curses.pickups)thorns+=pickup.active && pickup.kind==PickupKind::Thorn;
    check(thorns==4 && curses.cubes[0].count==0,"Thorns level-scaled interval and rollover");
    curses.set_cube(0,id_of(BallKey::CLOCKTOWER),level);
    const int bonuses[6]={2,-3,0,7,1,-4};for(unsigned p=0;p<6;++p)curses.world.pockets[p].bonus=bonuses[p];
    for(unsigned i=0;i<4;++i)curses.run_cube_event(Signal::PocketAnother,0);
    for(unsigned t=0;t<360;++t)curses.update(tick_dt);
    for(unsigned p=0;p<6;++p)check(curses.world.pockets[p].bonus==bonuses[(p+5)%6],"Clocktower rotates signed bonuses without changing amounts");
    curses.set_cube(0,id_of(BallKey::TYRANT),level);curses.pickups={};
    Ball spawned;spawned.def=builtin(9);spawned.def.base_score=150;spawned.p=Vec(100,100);
    for(unsigned i=0;i<7-level;++i) {
      spawned.p=Vec(-180+float(i)*60,100);check(curses.queue_spawn(spawned,Vec(),Unsupported),"queue Tyrant spawn fixture");
      for(unsigned t=0;t<360;++t)curses.update(tick_dt);
    }
    for(unsigned t=0;t<1800 && curses.pending_actions();++t)curses.update(tick_dt);
    check(!curses.pending_actions(),"Tyrant nested spawn and penalty carriers finish before assertion");
    check(curses.cubes[0].count==0 && curses.world.balls[curses.world.last_added].def.base_score==50,"Tyrant reacts to real spawn events with delayed -100 buff");
  }
  check(required_score(10,3)==200 && required_score(10,4)==150 && required_score(10,5)==220,"higher difficulty score modifiers match recovered resources");
  check(rounds_for_difficulty(5)==20 && rounds_for_difficulty(4)==10,"All Nighter has twenty rounds");
  for(unsigned difficulty=0;difficulty<6;++difficulty) {
    std::unique_ptr<Game> holder(new Game);Game& progression=*holder;progression.difficulty=difficulty;progression.reset();
    for(unsigned round=1;round<=20;++round) {
      progression.round=round;progression.begin_round();unsigned total=0;
      for(const auto& cube:progression.cubes)if(cube.id){total+=cube.level;check(cube.level<=5 && ball_info(cube.id).can_drop,"automatic curses use full droppable pool and legal levels");}
      check(total==curse_levels_for_round(round,difficulty),"curse awards follow original cumulative interval");
      progression.begin_round();unsigned retry=0;for(const auto& cube:progression.cubes)if(cube.id)retry+=cube.level;
      check(retry==total,"retrying a round does not duplicate curse awards");
    }
  }
  for(int last_round : {10,20}) {
    std::unique_ptr<Game> holder(new Game);Game& allnight=*holder;allnight.difficulty=5;allnight.reset();allnight.round=last_round;allnight.begin_round();allnight.actions={};allnight.phase=Phase::Aiming;allnight.score=allnight.target;
    check(allnight.finish_round(),"All Nighter finish round resolves");
    check(allnight.phase==(last_round==20?Phase::RunWon:Phase::Shop),"All Nighter ends at twenty, not ten");
    if(last_round==10){check(allnight.save_run("build/high-difficulty.sav"),"higher difficulty checkpoint saves");std::unique_ptr<Game> restored(new Game);check(restored->load_run("build/high-difficulty.sav") && restored->difficulty==5,"higher difficulty checkpoint restores");std::remove("build/high-difficulty.sav");std::remove("build/high-difficulty.sav.bak");}
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& chooser=*holder;chooser.phase=Phase::Menu;Input cycle;cycle.inspect=true;
    for(unsigned expected : {1u,2u,3u,5u,0u}){chooser.input(cycle,.016f);check(chooser.difficulty==expected,"menu skips inactive original Dark and Stormy difficulty");}
  }
  for(unsigned selected=0;selected<deck_count;++selected) {
    std::unique_ptr<Game> holder(new Game);Game& run=*holder;run.deck=selected;run.reset();
    check(run.inventory_count==3 && (selected==7?run.world.count>=4:run.world.count==4),"each original deck starts with three physical balls");
    for(unsigned i=0;i<3;++i){check(selected==7?run.inventory[i].id!=id_of(BallKey::GACHA):run.inventory[i].id==deck_info(selected).balls[i],"original ordered starting lineup or Gacha spawn transformation");check(selected==7?run.inventory[i].score==2:run.inventory[i].score==builtin(run.inventory[i].id).base_score,"starter base value or Gacha permanent gain matches source data");}
    run.phase=Phase::Shop;run.money=1000;run.stock_shop();
    if(selected<7)for(unsigned id:run.offers)if(id)check(ball_info(id).set==deck_info(selected).set,"Chill shops remain in selected deck set");
    check(run.save_run("build/deck-roundtrip.sav"),"deck checkpoint saved");
    std::unique_ptr<Game> restored(new Game);check(restored->load_run("build/deck-roundtrip.sav") && restored->deck==selected && restored->shop_sets==run.shop_sets,"deck and chosen shop sets persist together");
    std::remove("build/deck-roundtrip.sav");std::remove("build/deck-roundtrip.sav.bak");
    run.difficulty=1;run.round=3;run.stock_shop();check(selected==7 || run.shop_sets[1]!=ItemSet::NONE,"extra sets introduced after original third round");
    run.round=4;run.stock_shop();check(selected==7 || (run.shop_sets[2]!=ItemSet::NONE && run.shop_sets[0]!=run.shop_sets[1] && run.shop_sets[1]!=run.shop_sets[2]),"three distinct persistent sets by fourth round");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& run=*holder;run.deck=3;run.reset();
    check(run.world.balls[2].ghost_used && run.world.balls[2].shielded,"Spooky starter Ghost gets its once-per-round spawn shield");
    Ball child;child.def=builtin(9);run.queue_spawn(child,Vec(),Unsupported);for(unsigned t=0;t<500;++t)run.update(tick_dt);
    check(run.world.balls[run.world.last_added].def.base_score==2,"Gravestone buffs a real newly spawned ball");
    run.deck=1;run.reset();run.world.pockets_enabled=false;run.pickups={};run.spawn_pickup(PickupKind::Flower,Vec(100,-120));
    run.world.balls[2].active=false;run.world.events.push(EventKind::Pocketed,2,6,0,run.world.balls[2].generation);run.dispatch();
    check(run.world.balls[1].chase_flowers>.49f,"Ladybug gets half-second level-one flower pursuit");
    const Vec before=run.world.balls[1].p;for(unsigned t=0;t<20;++t)run.update(tick_dt);
    check(run.world.balls[1].p.x>before.x+1,"flower pursuit accelerates through stopping threshold");
    for(unsigned t=0;t<100;++t)run.update(tick_dt);check(run.world.balls[1].chase_flowers==0,"pursuit duration expires on fixed ticks");
    run.deck=4;run.reset();run.world.pockets_enabled=false;run.world.balls[1].active=false;run.world.events.push(EventKind::Pocketed,1,6,0,run.world.balls[1].generation);run.dispatch();
    check(run.pending_actions(),"Dog schedules aimed launch after another ball pockets");
    for(unsigned t=0;t<5;++t)run.update(tick_dt);check(run.world.balls[2].v.length()>300,"Dog launches itself with reference impulse strength");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& shop=*holder;shop.deck=2;shop.reset();shop.phase=Phase::Shop;shop.money=100;
    check(shop.combine_owned(0,1),"Tech starters combine");
    check(shop.inventory[1].id!=id_of(BallKey::SCRAP) && (ball_info(shop.inventory[1].id).tags&static_cast<uint32_t>(Tag::TAG_TECH)) && shop.inventory[1].level==2 && shop.inventory[1].score==3,"Scrap merge adds level-1 then becomes Tech without resetting value/level");
    shop.inventory={};shop.inventory[0]=default_item(id_of(BallKey::CAULDRON));shop.inventory[1]=default_item(9);shop.inventory_count=2;
    check(shop.sell(1) && shop.inventory[0].score==2,"sale dispatch buffs surviving Cauldron");
    for(unsigned i=0;i<6;++i){shop.money=100;check(shop.reroll(),"successful reroll dispatches event");}
    check(shop.inventory[0].score==5,"Cauldron stops at original level-one threshold");
    shop.inventory[0]=default_item(id_of(BallKey::BATTERY),2);shop.inventory[1]=default_item(9);shop.inventory[2]=default_item(9);shop.inventory_count=3;
    const int charge=shop.inventory[0].score;check(shop.combine_owned(1,2) && shop.inventory[0].score==charge+2,"Battery reacts to a different owned upgrade");
    shop.inventory={};shop.inventory[0]=default_item(id_of(BallKey::ASTROLABE),3);shop.inventory_count=1;
    check(shop.sell(0) && shop.inventory_count==3,"Astrolabe sale obtains one planet per level using freed slot");
    for(const auto& item:shop.inventory)if(item.id)check((ball_info(item.id).tags&(static_cast<uint32_t>(Tag::TAG_PLANET)|static_cast<uint32_t>(Tag::TAG_PLANET_)))!=0,"Astrolabe obtained records have original planet tags");
    shop.inventory={};shop.inventory[0]=default_item(id_of(BallKey::PIGGYBANK),3);shop.inventory[0].score=7;shop.inventory_count=1;shop.money=0;
    const int sale_price=item_price(shop.inventory[0])/2;check(shop.sell(0) && shop.money==sale_price+21,"Piggybank sale uses reference value multiplier");
    shop.deck=7;shop.reset();check(shop.inventory_count==3,"Gacha starter remains three owned items after transformation");
    for(unsigned i=0;i<3;++i)check(shop.inventory[i].id!=id_of(BallKey::GACHA) && shop.inventory[i].score==2,"Gacha spawn transforms permanently and gains permanent value");
    shop.phase=Phase::Shop;shop.money=100;shop.offers[0]=id_of(BallKey::GACHA);check(shop.buy(0),"Gacha shop purchase succeeds");
    check(shop.inventory[3].id!=id_of(BallKey::GACHA) && shop.inventory[3].score==2,"Gacha buy effect transforms the actual receiving item");
    check(shop.save_run("build/shop-effects.sav"),"shop effects checkpoint saves");std::unique_ptr<Game> restored(new Game);check(restored->load_run("build/shop-effects.sav") && restored->inventory[3].id==shop.inventory[3].id && restored->inventory[3].score==2,"transformed shop item persists");std::remove("build/shop-effects.sav");std::remove("build/shop-effects.sav.bak");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& shop=*holder;shop.deck=7;shop.set_run_seed(0x554c5452,true);shop.reset();shop.phase=Phase::Shop;shop.inventory={};shop.inventory[0]=default_item(id_of(BallKey::CAULDRON));shop.inventory_count=1;shop.money=0;
    check(!shop.reroll() && shop.inventory[0].score==1,"failed reroll cannot trigger Cauldron");
    check(!shop.sell(1) && shop.inventory[0].score==1,"empty-slot sale cannot trigger Cauldron");
    shop.inventory[1]=default_item(id_of(BallKey::GACHA));shop.inventory[1].mixed_with=id_of(BallKey::BRICKS);shop.inventory_count=2;shop.offers[0]=id_of(BallKey::GACHA);shop.inventory_selection=1;shop.money=100;
    check(shop.buy(0,true) && shop.inventory[1].id!=id_of(BallKey::GACHA) && shop.inventory[1].mixed_with==(shop.inventory[1].id==id_of(BallKey::BRICKS)?0:id_of(BallKey::BRICKS)) && shop.inventory[1].level==2 && shop.inventory[1].score==3,"Gacha merge transforms its component, collapses duplicate Bricks, and keeps shared upgraded level/value");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& effects=*holder;effects.reset();effects.world=World();effects.world.pockets_enabled=false;effects.world.stop_speed=0;
    Ball cue;cue.cue=true;cue.p=Vec(0,-100);effects.world.add(cue);
    Ball target_ball;target_ball.def=builtin(9);target_ball.def.damping=0;target_ball.p=Vec(100,0);effects.world.add(target_ball);target_ball.p=Vec(250,0);effects.world.add(target_ball);
    effects.blast_at(Vec());for(unsigned tick=0;tick<3;++tick)effects.update(tick_dt);
    check(std::fabs(effects.world.balls[1].v.x-300)<.01f && effects.world.balls[2].v.length()==0 && effects.world.balls[0].v.length()==0,"blast uses 200 radius and 600 linear falloff without launching cue");
    effects.actions={};effects.world=World();effects.world.pockets_enabled=false;effects.world.add(cue);
    Ball light;light.def=builtin(id_of(BallKey::FISH));light.weight=Weight::Light;light.p=Vec(-100,0);effects.world.add(light);
    Ball heavy;heavy.def=builtin(id_of(BallKey::JUPITER));heavy.weight=Weight::Heavy;heavy.p=Vec(100,0);effects.world.add(heavy);
    Ball normal;normal.def=builtin(9);normal.p=Vec(0,100);effects.world.add(normal);
    effects.queue_nudges(2,Vec(),true);unsigned mask=0;
    for(const auto& action:effects.actions)if(action.kind==ActionKind::ModifierBall && action.flags==static_cast<unsigned>(Modifier::Nudge))mask|=1u<<action.target;
    check(mask==((1u<<1)|(1u<<3)),"Cat preference chooses light and normal targets before heavy without duplicates");
    effects.actions={};effects.queue_boom_series(3,Vec());unsigned booms=0;
    for(const auto& action:effects.actions)booms+=action.kind==ActionKind::Boom;
    check(booms==1,"Tomato series starts one carrier immediately");
    for(unsigned tick=0;tick<18;++tick)effects.update(tick_dt);booms=0;for(const auto& action:effects.actions)booms+=action.kind==ActionKind::Boom;
    check(booms==2,"Tomato emits second carrier after 0.1 simulated seconds");
    for(unsigned tick=0;tick<18;++tick)effects.update(tick_dt);booms=0;unsigned repeats=0;for(const auto& action:effects.actions){booms+=action.kind==ActionKind::Boom;repeats+=action.kind==ActionKind::RepeatBoom;if(action.kind==ActionKind::Boom)check(action.destination.x>=-268 && action.destination.x<=265 && action.destination.y>=-418 && action.destination.y<=415,"Tomato destinations use original extended table bounds");}
    check(booms==3 && repeats==0,"Tomato emits exactly three carriers for level one");
    for(unsigned tick=0;tick<1800 && effects.pending_actions();++tick)effects.update(tick_dt);
    check(!effects.pending_actions(),"explosion carriers and delayed impulses finish without leaks");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& stock=*holder;stock.reset();stock.phase=Phase::Shop;stock.money=100;stock.inventory[0]=default_item(9);stock.inventory[1]=default_item(9);stock.inventory_count=3;
    stock.offers={{static_cast<uint16_t>(id_of(BallKey::CAULDRON)),static_cast<uint16_t>(id_of(BallKey::BATTERY)),9}};stock.offer_items={};
    check(stock.sell(2) && stock.offer_item(0).score==2,"market Cauldron observes an owned sale");
    check(stock.combine_owned(0,1) && stock.offer_item(1).score==2,"market Battery observes an owned upgrade");
    check(stock.save_run("build/market-state.sav"),"mutable stock values checkpoint");std::unique_ptr<Game> restored(new Game);check(restored->load_run("build/market-state.sav") && restored->offer_item(0).score==2 && restored->offer_item(1).score==2,"version-five save restores actual offer values");
    check(restored->buy(0) && restored->inventory[0].id==id_of(BallKey::CAULDRON) && restored->inventory[0].score==2,"purchase preserves buffed offered value");
    std::remove("build/market-state.sav");std::remove("build/market-state.sav.bak");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& shop=*holder;shop.reset();shop.phase=Phase::Shop;shop.money=100;shop.inventory={};shop.inventory_count=0;
    shop.snacks={{static_cast<uint8_t>(SnackKey::SHRIMP),static_cast<uint8_t>(SnackKey::FRIED_SHRIMP),0,0}};
    shop.offers[0]=9;check(shop.buy(0) && shop.inventory[0].score==2,"Shrimp buffs actual main-set purchase once");
    shop.offers[0]=id_of(BallKey::DOG);check(shop.buy(0) && shop.inventory[1].score==2,"Fried Shrimp buffs actual off-set purchase once");
    shop.inventory_selection=0;shop.offers[0]=9;check(shop.buy(0,true) && shop.inventory[0].score==4 && shop.inventory[0].level==2,"purchase passive buffs merged receiver before upgrade dispatch");
    shop.inventory[0]=default_item(id_of(BallKey::CAULDRON));shop.inventory[0].score=1;
    check(shop.sell_snack(0) && shop.inventory[0].score==2,"passive sale dispatches SellAnother to owned Cauldron");
    for(const char* key:{"tech_win","spooky_win","friends_win","food_win"})shop.profile.unlock(key);
    shop.difficulty=1;shop.round=4;shop.update_shop_sets();const auto before=shop.shop_sets;const auto offers_before=shop.offers;
    shop.snacks[0]=id_of(SnackKey::HALO_HALO);check(shop.sell_snack(0),"Halo Halo sale succeeds");
    check(shop.shop_sets[0]==before[0] && shop.offers==offers_before,"Halo Halo preserves main set and existing stock");
    for(unsigned i=1;i<3;++i)for(auto old:before)check(shop.shop_sets[i]!=old,"Halo Halo picks fresh extra sets");
    shop.inventory={};shop.inventory[0]=default_item(id_of(BallKey::DOG));shop.inventory[4]=default_item(id_of(BallKey::PET_FOOD),2);shop.inventory[4].score=3;shop.inventory_count=2;shop.snacks={};
    check(shop.sell(4) && shop.inventory[0].score==6,"Pet Food sale buffs literal front slot with Friend bonus");
    shop.inventory[0]=OwnedBall();shop.inventory[1]=default_item(9);shop.inventory[4]=default_item(id_of(BallKey::PET_FOOD));shop.inventory_count=2;
    check(shop.sell(4) && shop.inventory[1].score==1,"Pet Food does not substitute first occupied slot for empty front");
    shop.set_run_seed(12345678);shop.deck=7;shop.inventory={};shop.inventory[0]=default_item(9,2);shop.inventory[0].mixed_with=10;shop.inventory[4]=default_item(id_of(BallKey::POTION),3);shop.inventory_count=2;
    bool potion_sold=shop.sell(4);if(shop.inventory[0].score!=5)std::fprintf(stderr,"Potion fixture: id=%u mixed=%u score=%d\n",unsigned(shop.inventory[0].id),unsigned(shop.inventory[0].mixed_with),shop.inventory[0].score);
    check(potion_sold && shop.inventory[0].id!=9 && shop.inventory[0].id!=10 && shop.inventory[0].id!=id_of(BallKey::POTION) && shop.inventory[0].level==2 && shop.inventory[0].score==5,"Potion rerolls front identity and preserves level/value before its buff");
    check(!shop.inventory[0].mixed_with || (shop.inventory[0].mixed_with!=9 && shop.inventory[0].mixed_with!=10 && shop.inventory[0].mixed_with!=id_of(BallKey::POTION)),"Potion rerolls the secondary component with original exclusions");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.reset();g.snacks={};g.snacks[0]=id_of(SnackKey::NUTS);g.snacks[1]=id_of(SnackKey::CRUNCHY_TACO);
    g.target=100000;auto pocket=[&](){Ball b;b.def=builtin(9);b.active=true;g.world.add(b);unsigned i=g.world.last_added;g.world.balls[i].active=false;++g.world.metrics.ticks;g.world.events.push(EventKind::Pocketed,i,6,g.world.metrics.ticks);g.dispatch();};
    pocket();g.phase=Phase::Aiming;check(g.shoot(),"passive fixture shoots through public controls");
    check(g.snack_counts[0]==1 && g.snack_counts[1]==0,"Nuts counter persists across shots while Crunchy Taco resets");
    for(unsigned i=0;i<3;++i)pocket();
    const auto queued=g.actions.size();pocket();
    check(g.snack_counts[1]==3 && g.actions.size()==queued,"Crunchy Taco triggers once per shot");
    pocket();check(g.snack_counts[0]==0 && g.actions.size()>queued,"Nuts spawns Scrap every third pocket");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.reset();g.inventory={};g.inventory_count=3;
    g.inventory[0]=default_item(id_of(BallKey::BATTERY));g.inventory[0].score=9;
    g.inventory[1]=default_item(id_of(BallKey::AMBER),2);g.inventory[1].score=3;
    g.inventory[2]=default_item(id_of(BallKey::TV),2);g.snacks={};g.snacks[0]=id_of(SnackKey::TV_DINNER);
    g.actions={};g.world=World();g.phase=Phase::Aiming;g.end_effects_done=true;g.score=g.target;
    check(g.finish_round(),"shop-entry fixture finishes actual round");
    check(g.phase==Phase::Shop && g.inventory[0].score==4 && g.inventory[1].score==5,"shop entry drains Battery by five and grows Amber by level");
    unsigned scraps=0;for(const auto& item:g.inventory)scraps+=item.id==id_of(BallKey::SCRAP);
    check(scraps==4 && g.inventory_count==7,"TV and TV Dinner grant default Scrap into free roster slots");
    check(!g.finish_round() && g.inventory_count==7 && g.inventory[0].score==4,"repeated finish does not replay shop-entry effects");
    check(g.save_run("build/shop-entry-state.sav"),"shop-entry state checkpoints");
    std::unique_ptr<Game> restored(new Game);check(restored->load_run("build/shop-entry-state.sav") && restored->inventory_count==7 && restored->inventory[0].score==4 && restored->inventory[1].score==5,"loading shop does not drain or grant again");
    std::remove("build/shop-entry-state.sav");std::remove("build/shop-entry-state.sav.bak");
    g.inventory={};for(auto& item:g.inventory)item=default_item(9);g.inventory_count=15;
    g.inventory[0]=default_item(id_of(BallKey::BATTERY));g.inventory[0].score=2;g.inventory[1]=default_item(id_of(BallKey::TV),3);
    g.phase=Phase::Aiming;g.end_effects_done=true;g.actions={};g.world=World();g.score=g.target;
    check(g.finish_round() && g.inventory[0].score==1 && g.inventory_count==15,"Battery clamps to one and full roster rejects Scrap grants");
    g.inventory[2]=default_item(id_of(BallKey::GAIA),2);g.inventory[0].score=9;
    g.phase=Phase::Aiming;g.end_effects_done=true;g.actions={};g.world=World();g.score=g.target;
    check(g.finish_round() && g.inventory[0].score==6,"owned Gaia also modifies Battery negative shop buff");

  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.world=World();g.world.pockets_enabled=false;g.world.stop_speed=0;
    Ball cue;cue.cue=true;cue.p=Vec(-200,0);g.world.add(cue);
    Ball b;b.def=builtin(9);b.def.damping=0;b.p=Vec(100,0);g.world.add(b);set_ball_locked(g.world.balls[1],true);
    b.def.mass=2;b.p=Vec(-100,0);g.world.add(b);set_ball_locked(g.world.balls[2],true);
    b.active=false;g.world.add(b);g.queue_tiger(3);
    for(unsigned tick=0;tick<287;++tick)g.update(tick_dt);
    check(g.world.balls[1].locked && g.world.balls[1].v.length()==0,"Tiger waits 1.6 seconds before selecting targets");
    g.update(tick_dt);check(!g.world.balls[1].locked && g.world.balls[1].v.length()==0,"Tiger unlocks before two-frame impulse delay");
    g.update(tick_dt);g.update(tick_dt);
    check(std::fabs(g.world.balls[1].v.length()-1800)<.1f && std::fabs(g.world.balls[2].v.length()-900)<.1f,"Tiger momentum respects strength and mass");
    check(g.world.balls[0].v.length()==0 && !g.world.balls[3].active,"Tiger excludes cue and dead objects");
    g.world.balls[1].v=g.world.balls[2].v=Vec();
    for(unsigned tick=0;tick<106;++tick)g.update(tick_dt);
    check(!g.pending_actions(),"Tiger timeline ends after original 2.2 seconds");
    g.queue_tiger(10,true);for(unsigned tick=0;tick<539;++tick){g.update(tick_dt);g.world.balls[1].v=g.world.balls[2].v=Vec();}
    check(g.pending_actions(),"Tiger Roll preserves three-second round-end hold");g.update(tick_dt);
    check(!g.pending_actions(),"Tiger Roll hold finishes after three seconds");
    g.actions={};g.world.balls[1].def=builtin(id_of(BallKey::TIGER));g.world.balls[1].level=2;g.pocket_effect(1,6);
    bool tiger_hook=false;for(const auto& action:g.actions)tiger_hook|=action.kind==ActionKind::Tiger && action.multiplier==6;
    check(tiger_hook,"Tiger pocket event uses three times component level");
    g.actions={};g.snacks={};g.snacks[0]=id_of(SnackKey::TIGER_ROLL);g.end_round_effects();
    bool roll_hook=false;for(const auto& action:g.actions)roll_hook|=action.kind==ActionKind::Tiger && action.multiplier==10 && action.repeats==252;
    check(roll_hook,"Tiger Roll round-end subscriber schedules strength ten and hold");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.reset();g.actions={};g.world=World();g.target=100000;g.snacks={};g.snacks[0]=id_of(SnackKey::GUMMYWORM);g.inventory={};g.inventory_count=2;
    g.inventory[0]=default_item(id_of(BallKey::DOG),2);g.inventory[0].score=7;g.inventory[0].mixed_with=id_of(BallKey::CAT);
    g.inventory[14]=default_item(id_of(BallKey::STAR),3);g.inventory[14].score=9;
    Ball cue;cue.cue=true;g.world.add(cue);Ball b;b.def=builtin(9);b.active=true;g.world.add(b);
    Ball heavy;heavy.def=builtin(id_of(BallKey::TUNGSTEN_BALL));heavy.p=Vec(100,100);g.world.add(heavy);
    g.world.balls[1].active=false;++g.world.metrics.ticks;g.world.events.push(EventKind::Pocketed,1,6,g.world.metrics.ticks);g.dispatch();
    check(g.snack_counts[0]==0 && g.actions.empty(),"Gummy Worm waits while untargetable objects remain");
    g.world.balls[2].active=false;++g.world.metrics.ticks;g.world.events.push(EventKind::Pocketed,2,6,g.world.metrics.ticks);g.dispatch();
    unsigned spawned=0;bool mixed=false,reserve=false;for(const auto& a:g.actions)if(a.kind==ActionKind::Spawn){++spawned;mixed|=a.snapshot.def.id==id_of(BallKey::DOG) && a.snapshot.mixed_with==id_of(BallKey::CAT) && a.snapshot.level==2 && a.snapshot.def.base_score==7;reserve|=a.snapshot.def.id==id_of(BallKey::STAR) && a.snapshot.level==3 && a.snapshot.def.base_score==9;}
    check(g.snack_counts[0]==1 && spawned==2 && mixed && reserve,"Gummy Worm draws distinct populated build slots including reserve and preserves mixed values");
    const auto queued=g.actions.size();++g.world.metrics.ticks;g.world.events.push(EventKind::Pocketed,2,6,g.world.metrics.ticks);g.dispatch();
    check(g.actions.size()==queued,"Gummy Worm cannot trigger a second time in the round");
    g.actions={};g.snack_counts[0]=0;g.inventory[1]=default_item(id_of(BallKey::BEE));g.inventory[2]=default_item(id_of(BallKey::FISH));g.inventory_count=4;
    ++g.world.metrics.ticks;g.world.events.push(EventKind::Pocketed,2,6,g.world.metrics.ticks);g.dispatch();
    unsigned mask=0,selected=0;for(const auto& a:g.actions)if(a.kind==ActionKind::Spawn){++selected;unsigned bit=a.snapshot.def.id==id_of(BallKey::DOG)?1:a.snapshot.def.id==id_of(BallKey::STAR)?2:a.snapshot.def.id==id_of(BallKey::BEE)?4:8;check(!(mask&bit),"Gummy Worm samples without replacement");mask|=bit;}
    check(selected==3,"Gummy Worm caps draw at three populated build slots");

  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.reset();g.actions={};g.world=World();g.world.pockets_enabled=false;g.target=100000;g.score=12;g.shots=2;g.inventory={};g.inventory_count=3;
    g.inventory[0]=default_item(id_of(BallKey::DOG),2);g.inventory[0].score=7;g.inventory[0].mixed_with=id_of(BallKey::CAT);
    g.inventory[6]=default_item(id_of(BallKey::STAR));g.inventory[14]=default_item(id_of(BallKey::BEE));
    g.snacks={{static_cast<uint8_t>(SnackKey::GUMMYBEAR),static_cast<uint8_t>(SnackKey::SMALL_NACHO),static_cast<uint8_t>(SnackKey::BIG_NACHO),static_cast<uint8_t>(SnackKey::CEREAL)}};
    Ball cue;cue.cue=true;cue.p=Vec(0,210);g.world.add(cue);Ball b;b.def=builtin(9);g.world.add(b);g.world.balls[1].active=false;++g.world.metrics.ticks;g.world.events.push(EventKind::Pocketed,1,6,g.world.metrics.ticks);g.dispatch();
    check(g.snack_counts[0]==1 && g.pending_actions(),"Gummy Bear consumes once-per-round use and schedules rack setup");
    for(unsigned i=0;i<8;++i)g.update(tick_dt);check(!g.world.balls[1].active,"Gummy Bear waits first 0.05-second spawn interval");
    g.update(tick_dt);check(g.world.balls[2].active && g.world.balls[2].owner==0 && g.world.balls[2].mixed_with==id_of(BallKey::CAT) && g.world.balls[2].def.base_score==7,"Gummy Bear restores owned mixed ball at original rack slot");
    for(unsigned i=0;i<9;++i)g.update(tick_dt);
    check(g.world.count==4 && g.world.balls[3].owner==14 && (g.world.balls[3].p-build_slot_position(14)).length()<.01f,"Gummy Bear respects combined Nacho slot filters and 0.05-second spacing");
    for(unsigned i=0;i<18;++i)g.update(tick_dt);
    check(!g.pending_actions() && g.shots==2 && g.score==13,"Gummy Bear finishes setup tail without resetting shots or round score");
    g.world.balls[2].active=false;++g.world.metrics.ticks;g.world.events.push(EventKind::Pocketed,2,6,g.world.metrics.ticks);g.dispatch();
    check(g.inventory[0].score==8,"Cereal permanently buffs the owned item behind a redeployed ball");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.world.pockets_enabled=false;g.target=100000;g.shots=0;
    Ball cue;cue.cue=true;cue.p=Vec(0,210);g.world.add(cue);Ball bunny;bunny.def=builtin(id_of(BallKey::BUNNY));bunny.level=2;bunny.def.base_score=4;bunny.p=Vec(0,-120);g.world.add(bunny);
    g.end_round_effects();g.dispatch();
    check(g.score==12 && g.shots==1 && !g.world.balls[1].active,"Bunny round end virtually pockets at level plus one and grants one shot");
    for(unsigned tick=0;tick<359;++tick)g.update(tick_dt);check(g.pending_actions(),"Bunny holds round end for two seconds");g.update(tick_dt);check(!g.pending_actions(),"Bunny hold expires after two seconds");
    g.actions={};g.world=World();g.world.pockets_enabled=false;g.world.add(cue);Ball timer;timer.def=builtin(id_of(BallKey::EGG_TIMER));timer.level=2;g.world.add(timer);
    for(unsigned i=0;i<5;++i){Ball b;b.def=builtin(1);b.p=Vec(-150+60*float(i),-150);g.world.add(b);}
    g.end_round_effects();unsigned nudges=0;for(const auto& action:g.actions)if(action.kind==ActionKind::ModifierBall){++nudges;check(action.multiplier==4 && action.target!=1,"Egg Timer uses twice-level force and excludes itself");}
    check(nudges==4,"Egg Timer selects at most twice-level live targets");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.world.pockets_enabled=false;g.world.stop_speed=0;Ball cue;cue.cue=true;cue.p=Vec(-200,200);g.world.add(cue);Ball b;b.def=builtin(1);b.def.damping=0;b.p=Vec(100,0);g.world.add(b);
    g.queue_ball_modifier(1,Modifier::Nudge,0,b.p,4);bool impulse=false;
    for(unsigned tick=0;tick<1800 && !impulse;++tick){g.update(tick_dt);for(const auto& action:g.actions)impulse|=action.kind==ActionKind::Impulse;}
    check(impulse,"Egg Timer force reaches delayed impulse after carrier arrival");g.update(tick_dt);g.update(tick_dt);
    check(std::fabs(g.world.balls[1].v.x-1200)<.1f && std::fabs(g.world.balls[1].v.y)<.1f,"nudge uses half-force times 600 toward closest pocket");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.world.pockets_enabled=false;g.target=100000;
    Ball cue;cue.cue=true;cue.p=Vec(0,210);g.world.add(cue);Ball apple;apple.def=builtin(id_of(BallKey::APPLE));apple.level=2;apple.p=Vec(0,-200);g.world.add(apple);
    Ball target_ball;target_ball.def=builtin(id_of(BallKey::DOG));target_ball.p=Vec(100,0);g.world.add(target_ball);
    g.pocket_effect(1,6);unsigned buffs=0;for(const auto& a:g.actions)buffs+=a.kind==ActionKind::BuffBall;
    check(buffs==1,"Apple launches first buff immediately");for(unsigned tick=0;tick<35;++tick)g.update(tick_dt);
    buffs=0;for(const auto& a:g.actions)buffs+=a.kind==ActionKind::BuffBall;check(buffs==1,"Apple waits 0.2 seconds between buffs");g.update(tick_dt);
    buffs=0;for(const auto& a:g.actions)buffs+=a.kind==ActionKind::BuffBall;check(buffs==2,"Apple second buff launches after 36 ticks");
    for(unsigned tick=0;tick<1800 && g.pending_actions();++tick)g.update(tick_dt);
    check(g.world.balls[2].def.base_score==5 && g.world.balls[1].def.base_score==1,"level-two Apple sends four buffs to non-number targets and excludes itself");
    g.inventory={};g.inventory[0]=default_item(id_of(BallKey::DOG));g.inventory[0].score=5;g.inventory[1]=default_item(id_of(BallKey::COMPASS),3);g.inventory_count=2;g.begin_round();
    bool south=false;for(const auto& a:g.actions)south|=a.kind==ActionKind::BuffBall && a.target==1 && a.bonus==6;check(south,"Compass spawn buffs southernmost eligible ball by twice level");
    g.inventory[1]=default_item(id_of(BallKey::LANTERN),2);g.begin_round();
    for(unsigned tick=0;tick<1800 && g.pending_actions();++tick)g.update(tick_dt);
    check(g.world.balls[1].shielded && g.world.balls[1].def.base_score==7,"Lantern spawn grants southernmost shield and level value");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.world.pockets_enabled=false;g.target=100000;Ball cue;cue.cue=true;cue.p=Vec(0,210);g.world.add(cue);
    Ball calc;calc.def=builtin(id_of(BallKey::CALCULATOR));calc.level=3;calc.p=Vec(-100,-100);g.world.add(calc);g.dispatch();
    Ball plain;plain.def=builtin(1);plain.p=Vec(100,100);g.queue_spawn(plain,Vec(100,100),Unsupported);
    for(unsigned tick=0;tick<1800 && g.pending_actions();++tick)g.update(tick_dt);
    check(g.world.balls[1].def.base_score==4,"Calculator gains level value when a different object spawns");
    g.world.balls[1].def=builtin(id_of(BallKey::ROCKET));g.world.balls[1].def.damping=0;g.world.balls[1].level=2;
    g.world.balls[2].active=false;++g.world.metrics.ticks;g.world.events.push(EventKind::Pocketed,2,6,g.world.metrics.ticks);g.dispatch();
    check(std::fabs(g.world.balls[1].rocket_time-.4f)<.0001f && g.pending_actions(),"Rocket starts twice-level 0.2-second thrust timer");
    g.update(tick_dt);check(g.world.balls[1].v.length()==0,"Rocket random shot retains two-frame delay");g.update(tick_dt);
    check(std::fabs(g.world.balls[1].v.length()-60)<.01f,"Rocket initial random momentum is 600 times 0.1");g.update(tick_dt);
    check(std::fabs(g.world.balls[1].v.length()-(60+2000*tick_dt))<.02f,"Rocket accelerates along velocity with original 2000 force");
    for(unsigned tick=0;tick<80;++tick)g.update(tick_dt);check(g.world.balls[1].rocket_time==0,"Rocket thrust timer expires");
    g.world.balls[1].rocket_time=.4f;g.world.balls[1].chase_flowers=.05f;g.update(tick_dt);
    check(std::fabs(g.world.balls[1].rocket_time-.4f)<.0001f,"flower pursuit takes priority over Rocket thrust in mixed balls");
    Ball copy=g.world.balls[1];copy.rocket_time=.3f;copy.chase_flowers=.2f;g.queue_spawn(copy,Vec(),Unsupported);
    for(unsigned tick=0;tick<1800 && g.pending_actions();++tick)g.update(tick_dt);
    check(g.world.balls[g.world.last_added].rocket_time==0 && g.world.balls[g.world.last_added].chase_flowers==0,"newly spawned copies do not inherit node movement timers");

  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.inventory={};g.inventory[0]=default_item(id_of(BallKey::CALCULATOR));g.inventory[1]=default_item(1);g.inventory_count=2;g.begin_round();
    check(g.world.balls[1].def.base_score==1,"initial rack setup suppresses SpawnAnother as in original");
    g.world.balls[1].active=g.world.balls[2].active=false;g.actions={};g.queue_build_setup();
    for(unsigned tick=0;tick<1800 && g.pending_actions();++tick)g.update(tick_dt);
    check(g.world.count==5 && g.world.balls[3].def.base_score==1,"Gummy Bear setup also suppresses SpawnAnother while running normal Spawn effects");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.world.pockets_enabled=false;g.target=100000;Ball cue;cue.cue=true;cue.p=Vec(0,210);g.world.add(cue);Ball b;b.def=builtin(id_of(BallKey::OCTOPUS));b.level=3;b.p=Vec(0,-120);g.world.add(b);g.pocket_effect(1,6);
    unsigned fish=0;for(const auto& a:g.actions)if(a.kind==ActionKind::Spawn){++fish;check(a.snapshot.def.id==id_of(BallKey::FISH) && a.snapshot.level==3 && a.snapshot.def.base_score==3,"Octopus Fish keep level and explicit level value");}check(fish==8,"Octopus always spawns eight Fish");
    g.actions={};g.world.balls[1].def=builtin(id_of(BallKey::MACHINE));g.world.balls[1].level=2;g.pocket_effect(1,6);unsigned scraps=0;
    for(const auto& a:g.actions)if(a.kind==ActionKind::Spawn){++scraps;check(a.snapshot.def.id==id_of(BallKey::SCRAP) && a.snapshot.level==1 && a.snapshot.def.base_score==1,"Machine emits default Scrap instead of inheriting its own level");}check(scraps==3,"Machine spawns level plus one Scrap");
    g.actions={};g.world.balls[1].def=builtin(id_of(BallKey::SKULL));Ball target_ball;target_ball.def=builtin(1);target_ball.p=Vec(10,-120);target_ball.shielded=true;g.world.add(target_ball);target_ball.shielded=false;target_ball.shield_broken=true;target_ball.p=Vec(20,-120);g.world.add(target_ball);target_ball.shield_broken=false;target_ball.p=Vec(30,-120);g.world.add(target_ball);g.pocket_effect(1,6);
    bool recipient=false;for(const auto& a:g.actions)recipient|=a.kind==ActionKind::ModifierBall && a.target==4 && a.bonus==2;check(recipient,"Skull skips shielded and shield-broken closer targets");
    g.actions={};g.world.balls[1].def=builtin(id_of(BallKey::BEANS));g.world.balls[1].level=2;g.buff_ball(1,-1);unsigned booms=0;for(const auto& a:g.actions)booms+=a.kind==ActionKind::Boom;check(booms==1,"Beans reacts even to a negative buff with first explosion carrier");
    for(unsigned tick=0;tick<18;++tick)g.update(tick_dt);booms=0;for(const auto& a:g.actions)booms+=a.kind==ActionKind::Boom;check(booms==2,"Beans emits level carriers at original 0.1-second interval");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.world.pockets_enabled=false;g.world.stop_speed=0;Ball cue;cue.cue=true;cue.p=Vec(0,210);g.world.add(cue);Ball b;b.def=builtin(1);b.def.damping=0;b.p=Vec(100,0);g.world.add(b);
    for(unsigned pocket=0;pocket<6;++pocket)g.world.pockets[pocket].closed=pocket!=5;
    g.queue_ball_modifier(1,Modifier::Nudge,0,b.p);bool impulse=false;
    for(unsigned tick=0;tick<1800 && !impulse;++tick){g.update(tick_dt);for(const auto& a:g.actions)impulse|=a.kind==ActionKind::Impulse;}
    g.update(tick_dt);g.update(tick_dt);check(impulse && std::fabs(g.world.balls[1].v.x+195)<.1f,"nudge skips nearer closed pocket and chooses closest open pocket");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.target=100000;g.world.pockets_enabled=false;g.shop_sets[0]=ItemSet::TECH;Ball cue;cue.cue=true;cue.p=Vec(0,210);g.world.add(cue);
    Ball recycler;recycler.def=builtin(id_of(BallKey::RECYCLER));recycler.level=2;recycler.p=Vec(-100,0);g.world.add(recycler);Ball scrap;scrap.def=builtin(id_of(BallKey::SCRAP));scrap.level=2;scrap.p=Vec(100,0);g.world.add(scrap);Ball battery;battery.def=builtin(id_of(BallKey::BATTERY));battery.p=Vec(0,-200);g.world.add(battery);Ball pocketed;pocketed.def=builtin(1);g.world.add(pocketed);g.world.balls[4].active=false;++g.world.metrics.ticks;g.world.events.push(EventKind::Pocketed,4,6,g.world.metrics.ticks);g.dispatch();
    for(unsigned tick=0;tick<1800 && g.pending_actions();++tick)g.update(tick_dt);
    check(g.world.balls[2].level==3 && g.world.balls[2].def.id!=id_of(BallKey::SCRAP) && has_tag(g.world.balls[2],Tag::TAG_TECH),"Recycler levels and rerolls Scrap through runtime Upgrade event");
    check(g.world.balls[2].def.base_score==5 && g.world.balls[3].def.base_score==2,"Scrap uses old level buff then Recycler value; Battery observes runtime upgrade");
    g.actions={};g.world.balls[1].def=builtin(id_of(BallKey::TOOLBOX));g.world.balls[1].level=2;set_ball_locked(g.world.balls[2],true);set_ball_locked(g.world.balls[3],true);g.pocket_effect(1,6);
    unsigned screws=0,shots=0;for(const auto& a:g.actions){if(a.kind==ActionKind::Spawn){++screws;check(a.snapshot.def.id==id_of(BallKey::SCREW) && a.snapshot.level==2,"Toolbox Screws inherit component level");}if(a.kind==ActionKind::ModifierBall){++shots;check(a.multiplier==2,"Toolbox shot carrier retains level force");}}
    check(screws==2 && shots==2,"Toolbox emits level Screws and chooses distinct locked targets");
  }
  {
    std::unique_ptr<Game> holder(new Game);Game& g=*holder;g.world.pockets_enabled=false;g.world.stop_speed=0;Ball cue;cue.cue=true;cue.p=Vec(-200,200);g.world.add(cue);Ball b;b.def=builtin(1);b.def.damping=0;b.p=Vec(100,0);g.world.add(b);set_ball_locked(g.world.balls[1],true);
    g.queue_ball_modifier(1,Modifier::Shoot,0,b.p,2);bool impulse=false;
    for(unsigned tick=0;tick<1800 && !impulse;++tick){g.update(tick_dt);for(const auto& a:g.actions)impulse|=a.kind==ActionKind::Impulse;}
    check(impulse && !g.world.balls[1].locked,"Toolbox shot unlocks target before delayed impulse");g.update(tick_dt);g.update(tick_dt);
    check(std::fabs(g.world.balls[1].v.length()-1800)<.1f,"level-two Toolbox shot uses 900 times level momentum");
  }
  return checks;
}
