#include "ultrapool/core.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
namespace up {
// Locally assigned IDs; behavior constants are documented in MILESTONE2.md.
Definition builtin(unsigned id) {
  const auto &info = ball_info(id);
  Definition d;
  d.id = static_cast<uint16_t>(id < content_count ? id : 0);
  d.color = info.color;
  d.base_score = info.base_score;
  d.ability = info.ability;
  if (id == 0)
    d.radius = 17.5f;
  if (id == id_of(BallKey::FISH) || id == id_of(BallKey::PLUTO) ||
      id == id_of(BallKey::ANT)) {
    d.mass = .5f;
    d.radius *= .8f;
  }
  if (id == id_of(BallKey::JUPITER)) {
    d.mass = 5;
    d.radius *= 1.2f;
  }
  if (id == id_of(BallKey::TUNGSTEN_BALL)) {
    d.mass = 60;
    d.radius *= 1.3f;
  }
  return d;
}
bool physical_pocketable(const Ball& b){
  return b.active && !b.falling && !b.collision_disabled && !b.ignore_pockets && b.def.id!=id_of(BallKey::COOTS) && b.mixed_with!=id_of(BallKey::COOTS) && b.def.id!=id_of(BallKey::SATELLITE) && b.mixed_with!=id_of(BallKey::SATELLITE) && b.def.id!=id_of(BallKey::ENERGY_BALL) && b.mixed_with!=id_of(BallKey::ENERGY_BALL);
}
bool targetable_ball(const Ball &b) {
  return b.active && !b.passive && !b.cue && !b.energy && !b.falling && b.def.id != id_of(BallKey::IMP) &&
         b.mixed_with != id_of(BallKey::IMP) &&
         b.def.id != id_of(BallKey::TUNGSTEN_BALL) &&
         b.mixed_with != id_of(BallKey::TUNGSTEN_BALL);
}
const char *ball_name(unsigned id) { return ball_info(id).name; }
bool ball_supported(unsigned id) {
  return id < content_count && ball_info(id).implemented;
}
bool snack_supported(unsigned id) {
  return id < snack_count && snack_info(id).implemented;
}
uint32_t ball_tags(const Ball &b) {
  return ball_info(b.def.id).tags |
         (b.mixed_with ? ball_info(b.mixed_with).tags : 0);
}
bool has_tag(const Ball &b, Tag tag) {
  return (ball_tags(b) & static_cast<uint32_t>(tag)) != 0;
}
const char *ability_text(unsigned a) {
  static const char *descriptions[] = {
      "Pocket for value x pocket multiplier",
      "Ball/wall hit: score +level",
      "Wall hit: ball value +level (this round)",
      "Pocket: gain 2 x level money",
      "Pocket: gain level extra shots",
      "Hit: value +level; at 8 x level, explode",
      "Hit an object: score +level and push it",
      "Score above own value: value +2 x level",
      "Score <=2 x level: value +level",
      "Score divisible by 10: value +5 x level",
      "Pocket: give two different pockets +level score",
      "Any pocket: reroll value from 1 to 6 x level",
      "Hit another Marble: own value +2 x level",
      "Another pocket: launch in a random direction",
      "Any pocket: give a random pocket +level score",
      "Pocket: add +1 multiplier, 2 x level times",
      "Pocket: another pocket +1 mult, level-3 score",
      "Pocket: move bonus to adjacent pockets, +2 x level",
      "Combine with any number ball; values add",
      "Upgrade a ball of equal or lower level",
      "Any pocket: own value +level",
      "Smiley has spent its extra shots this round",
      "Pocket: spawn a number ball worth 1 to 7 x level",
      "Pocket: spawn a copy one level lower, if level >1",
      "Round start: copy first deployed ball at own level",
      "Receiving a buff: gain an extra +level",
      "Receiving a buff: share ceil(amount x level / 5)",
      "All positive buffs gain +level",
      "Positive value change: score amount x level",
      "Receiving a positive buff: lose twice that amount",
      "Receiving a buff: launch randomly",
      "Pocket: every other ball gains +level",
      "Another pocket: buff 2 x level other balls by +1",
      "Buff: level percent chance to become Butterfly",
      "Hit another ball: gain +level value",
      "Hit another ball: give it +level value",
      "Hit or wall: lose 1; buff another ball by level",
      "Hit or wall: lose 1 and spawn a light Fish",
      "Pocket: spawn an Egg worth level",
      "Wall: pocket itself with x2 multiplier",
      "Pocket: give its value to the closest ball",
      "Transform: send self a +3 x level buff",
      "Any transformation: buff that ball by 3 x level",
      "Hit or wall: gain level; transform at 5 x level",
      "Cue hit: become Human; wolf form pockets for zero",
      "Cue hit: become Werewolf",
      "Weaker different ball hit: gain level and infect it",
      "Hit another object ball: copy its identities",
      "Pocket: transform closest ball into Frog; buff level",
      "Pocket: spawn two Blueprints worth 2^level",
      "Pocket: respawn itself at a free table position",
      "Pocket: shield one pocket and add level bonus",
      "Spawn: lock itself",
      "Another pocket: lock/buff one other; own pocket: unlock all",
      "Hit: unlock the other ball; immune to value buffs",
      "Round start: ignite level adjacent rack balls",
      "Spawn: ignite itself; effects repeat",
      "Hit: buff object ball; score its value if a star",
      "Pocket: score 2 x level closest remaining stars",
      "Pocket: spawn level Stars",
      "Pocket: spawn level Star Systems",
      "Spawn: give planets a star and +level value",
      "Spawn: star and buff the southernmost ball",
      "Pocket: split another ball into two at 20% x level",
      "Pocket: split one random star at 20% x level",
      "Pocket: launch 2 x level stars and buff each +1",
      "Pocket: buff all stars by 2 x level",
      "Pocket: star level nearest non-star balls",
      "Pocket: ignite level stars and buff each +1",
      "Pocket: make 2 x level stars heavy and buff +3",
      "Pocket: make 2 x level stars light and buff +1",
      "Pocket: shield level unshielded stars",
      "Pocket: lock one star and buff 4 x level",
      "Pocket: buff shared identities by match count x level",
      "Pocket: buff all by level, then shuffle values",
      "Positive buff: plant a flower with power level",
      "Spawn/pocket: plant 2 x level flowers",
      "Spawn/pocket: create 2 x level oil pools",
      "Pocket: create level launchpads",
      "Spawn/pocket: create level flame pickups",
      "Pocket: plant flowers equal to value",
      "Pocket: grow biggest flower by 3 x level",
      "Pocket: spend 3 x level flowers to buff nearby balls",
      "Pocket: buff all balls and improve all flowers",
      "Touch a pickup: buff self by level",
      "Pocket: grow bread by 2 x level and make it heavy",
      "Pocket: grow bread by level and make it light",
      "Pocket: grow Orange Juice by half value x level, rounded up",
      "Another non-bread pocket: grow bread by 20 percent value x level"};
  if(a==PetFood)return "Sell: front slot gains this value; add level if it contains a Friend";
  if(a==Potion)return "Sell: reroll both front-slot abilities, then give it +level value";
  if(a==Cat)return "Pocket: nudge 2 x level balls toward pockets, preferring lighter balls";
  if(a==Tomato)return "Pocket: launch 3 x level random explosion carriers, 0.1 seconds apart";
  if(a==Scrap)return "Upgrade self: gain level-1 value and transform into a Tech ball";
  if(a==Cauldron)return "Sale or shop reroll: gain level value while below 5 x level";
  if(a==Battery)return "Shop entry: lose 5 value; any shop upgrade: gain level value";
  if(a==Television)return "Shop entry: obtain level Scrap balls";
  if(a==Log)return "Pocket: all surviving Logs gain 2 x level value";
  if(a==Ant)return "Pocket: spawn another Ant at this level";
  if(a==Nest)return "Another pocket: up to 3 Eggs gain level value; hatch at 10";
  if(a==Pot)return "Another pocket: gain ceil(its value x level / 5)";
  if(a==Peacock)return "Another pocket: pull level farthest balls toward self";
  if(a==Spirit)return "Another pocket: 10% + 10% x level chance to shoot toward pocket";
  if(a==Replicator)return "Pocket: spawn level + 1 copies of common first reserve";
  if(a==Trash)return "Pocket: collect and pocket all Scrap; summon one with their total value";
  if(a==Gameball)return "Reach target score: pocket self at x2 and gain 3 x level money";
  if(a==Satellite)return "Cannot enter physical pockets; virtual pocket effects still apply";
  if(a==Eyeball)return "Pocket last targetable Eyeball: spawn two at value +level";
  if(a==Burger)return "Spawn: nearby build slots gain Heavy weight and value +level";
  if(a==Ooze)return "Spawn: gain ceil(own value x 20% x level) value";
  if(a==Puzzleball)return "Another pocket: unlock one locked ball and lock one unlocked ball; buff each 2 x level";
  if(a==Reaper)return "Pocket: +1 pocket multiplier; pull last contacted living object toward pocket";
  if(a==Mimic)return "Round start: copy front build identity on this side, keeping value and level";
  if(a==Witch)return "Round start: permanently reroll build slot five and buff it by level";
  if(a==Ceo)return "Round start: subtract one multiplier from a random pocket, level times";
  if(a==Icecube)return "Round start: shield a random pocket and add level score";
  if(a==TimeMachine)return "Another level 2+ pocket: summon its level-one copy with value equal to own level";
  if(a==Racoon)return "Hit a lighter object: pocket it and gain ceil(its value x level / 2)";
  if(a==Jellyfish)return "Another pocket: score +level. Pocket: collect a passive copy (8 slots)";
  if(a==Candle)return "Pocket five Candles: shield and launch random balls, once per total level";
  if(a==Observatory)return "Another pocket: fleeting level-one copy with 20% x level value";
  if(a==Parrot)return "First Pocket effect each shot: echo at own level after 0.2 seconds";
  if(a==HitBall)return "Each shot: launch in a random direction";
  if(a==Luna)return "Each shot: launch along cue aim at 0.7 + 0.4 x level strength";
  if(a==Lich)return "Pocket: haunt a pocket with a copy; next non-cue capture releases it";
  if(a==AiBrain)return "Pocket: summon up to two unique build identities as one mixed ball";
  if(a==Drill)return "Pocket: respawn locked with current value +3 x level";
  if(a==Bear)return "Pocket: return with a random light, normal or heavy weight";
  if(a==Lightbulb)return "Pocket: upgrade a Scrap and spawn one at this level";
  if(a==Keyring)return "Pocket: unlock level locked balls; each gains level value";
  if(a==Coffin)return "Another non-Spirit pockets: spawn a Spirit at this level";
  if(a==RatKing)return "Another pocket: +level unless mixed with Rat; pocket: promote a Rat";
  if(a==Vampire)return "Another pocket: drain up to 2 x level from the highest-value ball";
  if(a==Supernova)return "Pocket: trigger level other balls' pocket abilities";
  if(a==BlackHole)return "Pocket: create an extra hole with level+1 multiplier (maximum ten)";
  if(a==Wormhole)return "Pocket: briefly expand every open pocket's capture area";
  if(a==Robocook)return "Value change: spend 3 value repeatedly to nudge the lowest-value other ball";
  if(a==Robot)return "Another pocket: chase the nearest open pocket for 0.1 x level seconds";
  if(a==Snake)return "Another pocket: slither away from pockets for 0.5 x level seconds";
  if(a==Tesla)return "Another pocket: push level nearby balls outward";
  if(a==Ufo)return "Another pocket: fire energy toward the nearest ball";
  if(a==Tornado)return "Pocket: pull all surviving balls toward the table center";
  if(a==RedDwarf)return "Third pocket after a shot: gain 20% x level of current value";
  if(a==Energy)return "Temporary energy projectile; no value or pocket score";
  if(a==Comet)return "Pocket: fire energy toward the nearest ball";
  if(a==Asteroid)return "Pocket or hit by energy: spawn a Star at this level";
  if(a==Alien)return "Pocket: nudge a Friend toward a pocket; add level value";
  if(a==Antenna)return "Pocket: fill the table to 2 + 2 x level with Aliens";
  if(a==Wheel)return "Pocket: launch level different balls; each gains +1 value";
  if(a==Conveyor)return "Pocket: push all balls together for 0.2 x level seconds";
  if(a==Turbine)return "Another pocket: launch another ball and add level value";
  if(a==ChefHat)return "Another pocket: level random +1, shield or flame effects";
  if(a==Panellet)return "Another pocket: gain level value; random status and weight";
  if(a==Meatball)return "Pocket: randomly share half value x level across active balls";
  if(a==Onigiri)return "Pocket: spawn a common Food ball at this level";
  if(a==Friendsphere)return "Pocket: immediately spawn level random Friend balls";
  if(a==Rainbow)return "Spawn/pocket: all gain level value if all active identities differ";
  if(a==Recycler)return "Another pocket: a random Scrap gains a level and level value";
  if(a==Toolbox)return "Pocket: spawn level Screws and shoot up to level locked balls";
  if(a==Beans)return "Buff or another pocket: launch level random explosions";
  if(a==Octopus)return "Pocket: spawn 8 Fish at this level and level value";
  if(a==Machine)return "Pocket: spawn level + 1 default Scrap balls";
  if(a==Skull)return "Pocket: closest unshielded ball gains shield and level value";
  if(a==Calculator)return "Another ball spawns: gain level value";
  if(a==Rocket)return "Another ball pockets: random impulse then thrust for 0.2s x level";
  if(a==Apple)return "Pocket: 2 x level random +1 buffs, 0.2 seconds apart";
  if(a==Compass)return "Spawn: southernmost ball gains 2 x level value";
  if(a==Lantern)return "Spawn: southernmost ball gains shield and level value";
  if(a==Bunny)return "Round end: pocket self at level + 1 multiplier; gain 1 shot";
  if(a==EggTimer)return "Round end: nudge up to 2 x level other balls at force 2 x level";
  if(a==Tiger)return "Pocket: after 1.6s, launch all balls at strength 3 x level";
  if(a==Amber)return "Shop entry: gain level value";
  if(a==Astrolabe)return "Sell: obtain level random planet balls";
  if(a==Gacha)return "Spawn/buy: gain level value and transform into a random ball";
  if(a==Piggybank)return "Sell: also earn value times min(level,3) money";
  if(a==Dog)return "Another pocket: shoot toward nearest ball at 0.5+0.25 x level power";
  if(a==Ghost)return "First Spawn of this body: gain a shield";
  if(a==Gravestone)return "Another spawn: give the spawned ball +level value";
  if(a==Ladybug)return "Another pocket: chase flowers for level/2 seconds";
  if (a == Clocktower) return "Curse cube: every 4 pockets, move pocket bonuses clockwise";
  if (a == Tyrant) return "Curse cube: every 7-level spawns, give the new ball -100 value";
  if (a == Thorns) return "Curse cube: two thorns at start and every 6-level pockets";
  if (a == Paperwork) return "Curse cube: every 3 real pockets, one pocket loses 1+level bonus";
  if (a == Weights) return "Curse cube: make a ball heavy at start and every 8-level pockets";
  if (a == Cubert) return "Curse cube: spawn min(round,10) Tungsten balls at start";
  if (a == Glitch) return "Curse cube: teleport level balls at start and every 4 pockets";
  if (a == Patchwork) return "Curse cube: -1 pocket multiplier until 3 x level pockets";
  if (a == Demon) return "Curse cube: creates Imps at round start and every 8-level pockets";
  if (a == Bouncer)
    return "Curse cube: close level pockets; every 3 pockets switch a lock";
  if (a == Rat)
    return "Spawn: gain level value for each other Rat or Rat King";
  return a <= Oven ? descriptions[a] : "Ability not implemented yet";
}
namespace {
void replace_description(char *text, std::size_t size, const char *phrase,
                         const char *replacement) {
  char result[512];
  std::size_t used = 0, n = std::strlen(phrase), r = std::strlen(replacement);
  for (const char *c = text; *c && used + 1 < sizeof(result);) {
    if (std::strncmp(c, phrase, n) == 0) {
      for (std::size_t i = 0; i < r && used + 1 < sizeof(result); ++i)
        result[used++] = replacement[i];
      c += n;
    } else
      result[used++] = *c++;
  }
  result[used] = 0;
  std::snprintf(text, size, "%s", result);
}
} // namespace
void describe_ability(unsigned ability, unsigned level, char *out,
                      std::size_t size) {
  if (!out || !size)
    return;
  level = std::max(1u, level);
  char text[512], number[48];
  if (ability == Upgrade) {
    std::snprintf(out, size, "Combine: upgrade a ball of level %u or lower",
                  level);
    return;
  }
  if (ability == Mirror) {
    std::snprintf(out, size,
                  "Round start: copy first deployed ball at level %u", level);
    return;
  }
  if (ability == NestingDoll) {
    if (level > 1)
      std::snprintf(out, size, "Pocket: spawn a copy at level %u", level - 1);
    else
      std::snprintf(out, size, "Pocket: no copy at level 1");
    return;
  }
  std::snprintf(text, sizeof(text), "%s", ability_text(ability));
  std::snprintf(number, sizeof(number), "%u%%", 20 * level);
  replace_description(text, sizeof(text), "20% x level", number);
  for (unsigned factor = 8; factor >= 2; --factor) {
    char phrase[24];
    std::snprintf(phrase, sizeof(phrase), "%u x level", factor);
    std::snprintf(number, sizeof(number), "%u", factor * level);
    replace_description(text, sizeof(text), phrase, number);
  }
  std::snprintf(number, sizeof(number), "%u", 1u << std::min(level, 20u));
  replace_description(text, sizeof(text), "2^level", number);
  std::snprintf(number, sizeof(number), "%d", int(level) - 3);
  replace_description(text, sizeof(text), "level-3", number);
  std::snprintf(number, sizeof(number), "%u", level);
  replace_description(text, sizeof(text), "level", number);
  std::snprintf(out, size, "%s", text);
}
bool number_ball(unsigned id) {
  return id < content_count &&
         (ball_info(id).tags & static_cast<uint32_t>(Tag::TAG_NUMBER)) != 0;
}
unsigned number_id(int value) {
  return value <= 0 || value > 8 ? 26u : static_cast<unsigned>(value);
}
unsigned rarity(unsigned id) { return ball_info(id).rarity; }
bool shop_eligible(unsigned id, bool extra_sets) {
  if (!id || !ball_supported(id))
    return false;
  const auto &info = ball_info(id);
  return info.can_drop && info.set != ItemSet::NEGATIVE &&
         (extra_sets || info.set == ItemSet::CLASSIC);
}
const char *snack_name(unsigned id) { return snack_info(id).name; }
const char *snack_text(unsigned id) {
  static const char *text[] = {
      "Empty snack slot",
      "Round start: gain one extra shot",
      "Round start: every deployed ball gains +1",
      "First ball hit by the cue each shot gains +1",
      "First owned ball pocketed each round gains +1 permanently",
      "Pocket a ball: its pocket gains +1 score",
      "Round start: a random pocket gains +3 score",
      "Round start: a random pocket +1 mult, -5 score",
      "Round start: all pockets +1 multiplier, -3 score",
      "Round start: all pockets -1 multiplier, +5 score",
      "Round start: lose one shot and gain $5",
      "Round end: pocket the highest-valued ball",
      "Round end: pocket the lowest-valued ball"};
  if (id < 13)
    return text[id];
  switch (static_cast<SnackKey>(id)) {
  case SnackKey::SHRIMP:
    return "Buy a ball from the main shop set: it gains +1 value";
  case SnackKey::FRIED_SHRIMP:
    return "Buy a ball from another shop set: it gains +1 value";
  case SnackKey::HALO_HALO:
    return "Sell: replace extra offered sets; keep the main deck set";
  case SnackKey::LOVE_CAKE:
    return "Sell: heal one heart, up to your maximum";
  case SnackKey::VITAMINS:
    return "Sell: obtain a level 1 Upgrade Ball if a build slot is free";
  case SnackKey::FORTUNE_COOKIE:
    return "High Dice roll: split it; up to ten times each round";
  case SnackKey::MOONCAKE:
    return "Eighth pocket: trigger all remaining pocket abilities";
  case SnackKey::FRUIT_SALAD:
    return "Pocket fruit: trigger one remaining fruit ball's pocket abilities";
  case SnackKey::TOAST:
    return "Pocket a non-bread ball: grow bread by +1 or create Bread";
  case SnackKey::SAUSAGE_ROLL:
    return "Every pocket: grow a Screw by +2 or create one";
  case SnackKey::BLUE_CHEESE:
    return "Round start: spawn one Rat";
  case SnackKey::B_8_BAGEL:
    return "Pocket value 8: a random remaining ball gains +8";
  case SnackKey::LOTUS_ROOT:
    return "Flower cap becomes five; further planting grows existing flowers";
  case SnackKey::JASMINE_TEA:
    return "Cue picks up flowers and sends their buff to nearest object ball";
  case SnackKey::DAISY_CUPCAKE:
    return "Every pocket: plant one flower";
  case SnackKey::SUNFLOWER_CUPCAKE:
    return "Every pocket: spend one flower to buff nearest object ball";
  case SnackKey::SOYSAUCE:
    return "Every three pockets: create an oil pool";
  case SnackKey::VEGGIE_TACO:
    return "Third pocket each shot: plant five flowers";
  case SnackKey::AFTER_EIGHT:
    return "Start: -1 all pocket multipliers. After 8 pockets: +2 all";
  case SnackKey::ESPRESSO:
    return "Round start: southernmost ball gains +3";
  case SnackKey::EDAMAME:
    return "Every 3 pockets: make a random non-light ball light";
  case SnackKey::GUMMYBEAR:
    return "Once per round, clear table: redeploy your rack.";
  case SnackKey::GUMMYWORM:
    return "Once per round, clear table: spawn up to 3 random build balls.";
  case SnackKey::TIGER_ROLL:
    return "Round end: Tiger launches all balls at strength 10.";
  case SnackKey::TV_DINNER:
    return "Shop entry: obtain 2 Scrap balls.";
  case SnackKey::NUTS:
    return "Every 3 pockets: spawn a Scrap.";
  case SnackKey::CRUNCHY_TACO:
    return "3 pockets in one shot: spawn level 3 Scrap, value 8.";
  case SnackKey::STAR_COOKIE:
    return "Every 5 pockets: spawn a level 2 Star";
  case SnackKey::FISH_FINGER:
    return "Every pocket: launch a random Fish and buff it +1";
  case SnackKey::SCRAMBLED_EGGS:
    return "Pocket an Egg: its value buffs the nearest ball";
  case SnackKey::KEY_LIME:
    return "Every 3 pockets: unlock one locked ball and buff +2";
  case SnackKey::BRAVAS:
    return "Every 2 pockets: buff one random ball +1";
  case SnackKey::CRISPS:
    return "Every 5 pockets: spawn first reserve's common primary ball";
  case SnackKey::CLASSIC_TACO:
    return "Third pocket in each shot: all pockets gain +2 bonus";
  case SnackKey::RAINBOW_TACO:
    return "Tenth pocket in each shot: all pockets gain +1 multiplier";
  case SnackKey::SMALL_NACHO:
    return "Only first 6 slots deploy; their starting values are buffed by "
           "themselves";
  case SnackKey::BIG_NACHO:
    return "All 15 build slots deploy, including the usual reserve";
  case SnackKey::WINGS:
    return "Cue cannot fall into pockets and has 20 percent less damping";
  default:
    return "Passive not implemented yet";
  }
}
unsigned snack_rarity(unsigned id) { return snack_info(id).rarity; }
int snack_sell_price(unsigned id) { if(!id)return 0;static const int base[]={3,4,6,9};unsigned tier=std::min(snack_rarity(id),3u);return (base[tier]*(1<<tier))/2; }

} // namespace up