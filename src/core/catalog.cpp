#include "ultrapool/core.hpp"
#include <cstring>
namespace up {
namespace {
const ContentInfo balls[]={
    {"PLAYER","Cue","data/balls_data/player_ball.tres",0u,0xfff0f0f0u,1,1,0,ItemSet::NONE,false,true,false,Plain,true,0u},
    {"1-BALL","1-Ball","data/balls_data/1-ball.tres",130u,0xff62cbd2u,1,1,0,ItemSet::CLASSIC,true,false,true,Plain,true,0u},
    {"2-BALL","2-Ball","data/balls_data/2-ball.tres",130u,0xffd38863u,2,1,1,ItemSet::CLASSIC,true,false,true,Plain,true,0u},
    {"3-BALL","3-Ball","data/balls_data/3-ball.tres",130u,0xff6d62d2u,3,1,1,ItemSet::CLASSIC,false,false,true,Plain,true,0u},
    {"4-BALL","4-Ball","data/balls_data/4-ball.tres",130u,0xffd2629fu,4,1,2,ItemSet::CLASSIC,true,false,true,Plain,true,0u},
    {"5-BALL","5-Ball","data/balls_data/5-ball.tres",130u,0xff62a0d2u,5,1,2,ItemSet::CLASSIC,false,false,true,Plain,true,0u},
    {"6-BALL","6-Ball","data/balls_data/6-ball.tres",130u,0xff33b10cu,6,1,2,ItemSet::CLASSIC,false,false,true,Plain,true,0u},
    {"7-BALL","7-Ball","data/balls_data/7-ball.tres",130u,0xff7b33b0u,7,1,2,ItemSet::CLASSIC,false,false,true,Plain,true,0u},
    {"8-BALL","8-Ball","data/balls_data/8-ball.tres",130u,0xff353535u,8,1,3,ItemSet::CLASSIC,false,false,true,Plain,true,0u},
    {"PINBALL","Pinball","data/balls_data/pinball.tres",2u,0xffadadadu,1,1,0,ItemSet::CLASSIC,true,true,true,Pinball,true,384u},
    {"BRICKS","Bricks","data/balls_data/bricks.tres",2u,0xff466c98u,1,1,0,ItemSet::CLASSIC,true,true,true,Bricks,true,256u},
    {"GOLDEN-BALL","Gold","data/balls_data/gold.tres",2u,0xff3abdfdu,1,1,0,ItemSet::CLASSIC,true,true,true,Gold,true,8388608u},
    {"SMILEY","Smiley","data/balls_data/smiley.tres",6u,0xff5be9f3u,1,1,1,ItemSet::CLASSIC,true,true,true,Smiley,true,8388608u},
    {"BOMB","Bomb","data/balls_data/bomb.tres",16384u,0xff332e5cu,1,1,0,ItemSet::TECH,true,true,true,Bomb,true,384u},
    {"SEAL","Seal","data/balls_data/seal.tres",16u,0xffb2c0c7u,2,1,1,ItemSet::FRIENDS,true,true,true,Seal,true,128u},
    {"BASKETBALL","Basketball","data/balls_data/basketball.tres",4098u,0xff566cb4u,2,1,2,ItemSet::CLASSIC,false,true,true,Basketball,true,1u},
    {"TENNIS","Tennis","data/balls_data/tennis_ball.tres",4096u,0xff5bf3c5u,1,1,1,ItemSet::FRIENDS,true,true,true,Tennis,true,1u},
    {"FOOTBALL","Football","data/balls_data/football.tres",4096u,0xff5a5a5au,1,1,1,ItemSet::NONE,false,true,true,Football,true,1u},
    {"BEACH-BALL","Beach Ball","data/balls_data/beach_ball.tres",2u,0xff9a9e9au,1,1,0,ItemSet::CLASSIC,true,true,true,Beach,true,8388608u},
    {"DICE","Dice","data/balls_data/dice.tres",2u,0xffd0d0d0u,1,1,0,ItemSet::CLASSIC,true,true,true,Dice,true,2u},
    {"MARBLE","Marble","data/balls_data/marble.tres",2u,0xffdfc574u,1,1,0,ItemSet::CLASSIC,true,true,true,Marble,true,128u},
    {"PLUSHIE","Plushie","data/balls_data/plushie.tres",18u,0xffa395cbu,2,1,0,ItemSet::CLASSIC,true,true,true,Plushie,true,2u},
    {"ROULETTE","Roulette","data/balls_data/roulette.tres",16386u,0xff2c3347u,1,1,1,ItemSet::CLASSIC,true,true,true,Roulette,true,2u},
    {"PANTS","Pants","data/balls_data/pants.tres",2u,0xffd27962u,3,1,3,ItemSet::CLASSIC,true,true,true,Pants,true,8388608u},
    {"YINYANG","Yinyang","data/balls_data/yinyang.tres",2u,0xff5a5a5au,2,1,1,ItemSet::CLASSIC,true,true,true,Yinyang,true,8388608u},
    {"JUGGLING-BALL","Juggling Ball","data/balls_data/juggling-ball.tres",4098u,0xff445b9eu,2,1,2,ItemSet::CLASSIC,true,true,true,Juggling,true,8388608u},
    {"OVERFLOW-BALL","Overflow Ball","data/balls_data/overflow-ball.tres",130u,0xff353535u,1,1,3,ItemSet::CLASSIC,false,false,true,Number,true,0u},
    {"UPGRADE-BALL","Upgrade Ball","data/balls_data/upgrade-ball.tres",2u,0xfff3d75bu,1,1,1,ItemSet::CLASSIC,true,true,true,Upgrade,true,0u},
    {"COOLGUY","CoolGuy","data/balls_data/coolguy.tres",6u,0xff5bc5f3u,1,1,1,ItemSet::CLASSIC,true,true,true,CoolGuy,true,2u},
    {"blank-stare","Spent Smiley","data/balls_data/spent_smiley.tres",4u,0xfff3d75bu,1,1,0,ItemSet::CLASSIC,false,true,true,Spent,true,32u},
    {"SLOT","Slot","data/balls_data/slot.tres",2u,0xffd0d0d0u,1,1,0,ItemSet::CLASSIC,true,true,true,Slot,true,8388608u},
    {"NESTING-DOLL","Nesting Doll","data/balls_data/nesting_doll.tres",18u,0xff3939b5u,2,1,2,ItemSet::CLASSIC,true,true,true,NestingDoll,true,8388608u},
    {"MIRROR","Mirror","data/balls_data/mirror_ball.tres",2u,0xffbfc3eeu,2,1,2,ItemSet::CLASSIC,true,true,true,Mirror,true,32u},
    {"AI-BRAIN","Ai Brain","data/balls_data/ai_brain.tres",16384u,0xffffeca0u,3,1,2,ItemSet::TECH,true,true,true,AiBrain,true,8388608u},
    {"ALARM-CLOCK","Alarm Clock","data/balls_data/negative_balls/alarm_clock.tres",0u,0xff6d62d2u,1,1,0,ItemSet::NEGATIVE,false,false,true,Unsupported,false,0u},
    {"ALIEN","Alien","data/balls_data/alien.tres",1040u,0xff799255u,1,1,0,ItemSet::SPACE,true,false,true,Alien,true,8388608u},
    {"AMBER","Amber","data/balls_data/amber.tres",32u,0xff2a8bdeu,1,1,0,ItemSet::NATURE,false,true,true,Amber,true,65536u},
    {"ANT","Ant","data/balls_data/ant.tres",16u,0xff565044u,1,1,1,ItemSet::FRIENDS,true,true,true,Ant,true,8388608u},
    {"ANTENNA","Antenna","data/balls_data/antenna.tres",17408u,0xff292829u,1,1,2,ItemSet::SPACE,true,false,true,Antenna,true,8388608u},
    {"ANTIMATTER","Antimatter","data/balls_data/antimatter.tres",1024u,0xff000000u,-1,1,1,ItemSet::SPACE,false,true,true,Antimatter,true,512u},
    {"APPLE","Apple","data/balls_data/apple.tres",40u,0xff1c41f8u,1,1,0,ItemSet::NATURE,true,true,true,Apple,true,8388608u},
    {"ASH","Ash","data/balls_data/ash.tres",0u,0xff000000u,1,1,0,ItemSet::FOOD,false,true,true,Unsupported,false,0u},
    {"ASTEROID","Asteroid","data/balls_data/asteroid.tres",1024u,0xffad98a5u,1,1,0,ItemSet::SPACE,true,false,true,Asteroid,true,8388736u},
    {"ASTROLABE","Astrolabe","data/balls_data/astrolabe.tres",17408u,0xff6b899du,1,1,0,ItemSet::SPACE,true,true,true,Astrolabe,true,1048576u},
    {"ASTRONAUT","Astronaut","data/balls_data/astronaut.tres",1040u,0xffe6e7e6u,1,1,1,ItemSet::SPACE,true,false,true,Astronaut,true,128u},
    {"BACON","Bacon","data/balls_data/bacon.tres",8u,0xff4f55c2u,1,1,0,ItemSet::FOOD,true,true,true,Bacon,true,8388608u},
    {"BANANA","Banana","data/balls_data/banana.tres",40u,0xff3fb5bbu,1,1,1,ItemSet::FOOD,true,true,true,Banana,true,8388608u},
    {"BATTERY","Battery","data/balls_data/battery.tres",16384u,0xff5686aeu,1,1,0,ItemSet::TECH,true,true,true,Battery,true,196608u},
    {"BEANS","Beans","data/balls_data/beans.tres",8u,0xff757c32u,1,1,1,ItemSet::FOOD,true,true,true,Beans,true,514u},
    {"BEAR","Bear","data/balls_data/bear.tres",16u,0xff39485fu,1,1,0,ItemSet::FRIENDS,true,true,true,Bear,true,8388608u},
    {"BEE","Bee","data/balls_data/bee.tres",48u,0xff5be9f3u,1,1,1,ItemSet::NATURE,true,true,true,Bee,true,512u},
    {"BIG_BANG","Big Bang","data/balls_data/big_bang.tres",9248u,0xffdf9745u,2,1,1,ItemSet::SPACE,false,true,true,Unsupported,false,0u},
    {"BLACK-HOLE","Black Hole","data/balls_data/black-hole.tres",1024u,0xff000000u,1,1,1,ItemSet::SPACE,true,true,true,BlackHole,true,8388608u},
    {"BLUEPRINT","Blueprint","data/balls_data/blueprint.tres",16384u,0xffa66b37u,2,1,0,ItemSet::TECH,false,true,true,Blueprint,true,128u},
    {"BOTTLE","Bottle","data/balls_data/bottle.tres",8u,0xffcdc247u,1,1,0,ItemSet::FOOD,true,true,true,Bottle,true,8388608u},
    {"BOUNCER","Bouncer","data/balls_data/negative_balls/bouncer.tres",0u,0xff6d62d2u,1,1,0,ItemSet::NEGATIVE,true,false,true,Bouncer,true,6u},
    {"BOXING-GLOVE","Boxing Glove","data/balls_data/boxing_glove.tres",4096u,0xff2422adu,1,1,0,ItemSet::FRIENDS,false,true,true,Unsupported,false,256u},
    {"BREAD","Bread","data/balls_data/bread.tres",9u,0xff3d6888u,1,1,0,ItemSet::FOOD,false,true,true,Plain,true,0u},
    {"BUNNY","Bunny","data/balls_data/bunny.tres",16u,0xffa7bbd9u,1,1,1,ItemSet::FRIENDS,true,false,true,Bunny,true,8u},
    {"BURGER","Burger","data/balls_data/burger.tres",8u,0xff4990c7u,1,1,1,ItemSet::FOOD,false,true,true,Burger,true,32u},
    {"BUSH","Bush","data/balls_data/bush.tres",32u,0xff30a45bu,1,1,1,ItemSet::NATURE,true,true,true,Bush,true,512u},
    {"BUTTERFLY","Butterfly","data/balls_data/butterfly.tres",48u,0xff829dcbu,2,1,0,ItemSet::FRIENDS,false,true,true,Butterfly,true,8388640u},
    {"CALCULATOR","Calculator","data/balls_data/calculator.tres",16384u,0xff4b4b4bu,1,1,1,ItemSet::TECH,true,true,true,Calculator,true,64u},
    {"CANDLE","Candle","data/balls_data/candle.tres",2048u,0xffb8d0d8u,1,1,0,ItemSet::SPOOKY,true,true,true,Candle,true,8388608u},
    {"CAT","Cat","data/balls_data/cat-ball.tres",16u,0xff66a2cfu,1,1,0,ItemSet::FRIENDS,true,true,true,Cat,true,8388608u},
    {"CATERPILLAR","Caterpillar","data/balls_data/caterpillar.tres",16u,0xff319760u,1,1,0,ItemSet::FRIENDS,true,true,true,Caterpillar,true,514u},
    {"CAULDRON","Cauldron","data/balls_data/cauldron.tres",2048u,0xff68748eu,1,1,0,ItemSet::SPOOKY,true,true,true,Cauldron,true,786432u},
    {"CEO","Ceo","data/balls_data/negative_balls/ceo.tres",64u,0xff6d62d2u,1,1,0,ItemSet::NEGATIVE,false,false,true,Ceo,true,4u},
    {"CHEESE","Cheese","data/balls_data/cheese.tres",8u,0xff4dd8f0u,2,1,1,ItemSet::FOOD,true,true,true,Cheese,true,8388608u},
    {"CHEF-HAT","Chef Hat","data/balls_data/chef_hat.tres",8u,0xff959595u,3,1,3,ItemSet::FOOD,true,true,true,ChefHat,true,2u},
    {"CHICKEN","Chicken","data/balls_data/chicken.tres",48u,0xff627cd2u,2,1,1,ItemSet::NATURE,true,true,true,Chicken,true,8388640u},
    {"CLOCKTOWER","Clocktower","data/balls_data/negative_balls/clocktower.tres",0u,0xff6d62d2u,1,1,0,ItemSet::NEGATIVE,true,false,true,Clocktower,true,34u},
    {"CLOUD","Cloud","data/balls_data/cloud.tres",32u,0xfff6c85du,3,1,0,ItemSet::NATURE,true,true,true,Cloud,true,384u},
    {"COFFIN","Coffin","data/balls_data/coffin.tres",2048u,0xff3d6888u,1,1,1,ItemSet::SPOOKY,true,true,true,Coffin,true,2u},
    {"COMET","Comet","data/balls_data/comet.tres",9216u,0xff62cbd2u,1,1,0,ItemSet::SPACE,true,false,true,Comet,true,8388608u},
    {"COMPASS","Compass","data/balls_data/compass.tres",16416u,0xff4f84b6u,1,1,0,ItemSet::NATURE,true,true,true,Compass,true,32u},
    {"CONSTELLATION","Constellation","data/balls_data/constellation.tres",1024u,0xff62cbd2u,1,1,2,ItemSet::SPACE,false,false,true,Unsupported,false,32u},
    {"CONVEYOR","Conveyor","data/balls_data/conveyor.tres",16384u,0xff737373u,1,1,1,ItemSet::TECH,true,true,true,Conveyor,true,8388608u},
    {"COOTS","Coots","data/balls_data/coots.tres",16u,0xff66a2cfu,1,1,0,ItemSet::FRIENDS,false,true,true,Coots,true,128u},
    {"CROQUETA","Croqueta","data/balls_data/croqueta.tres",40u,0xff383bffu,1,1,0,ItemSet::FOOD,false,true,true,Unsupported,false,0u},
    {"CUBERT","Cubert","data/balls_data/negative_balls/cubert.tres",0u,0xff6d62d2u,1,1,0,ItemSet::NEGATIVE,false,false,true,Cubert,true,6u},
    {"DANDELION","Dandelion","data/balls_data/dandelion.tres",32u,0xffb2bdc0u,1,1,2,ItemSet::NATURE,true,true,true,Dandelion,true,8388608u},
    {"DEMON","Demon","data/balls_data/negative_balls/demon.tres",64u,0xff6d62d2u,1,1,0,ItemSet::NEGATIVE,true,false,true,Demon,true,6u},
    {"DISCO","Disco","data/balls_data/disco_ball.tres",16386u,0xffb3b3c9u,1,1,2,ItemSet::CLASSIC,false,true,true,Unsupported,false,0u},
    {"DOG","Dog","data/balls_data/dog.tres",16u,0xff365d7cu,1,1,0,ItemSet::FRIENDS,true,true,true,Dog,true,2u},
    {"DONUT","Donut","data/balls_data/donut.tres",8u,0xffc27adau,1,1,1,ItemSet::FOOD,true,true,true,Donut,true,2u},
    {"DRILL","Drill","data/balls_data/drill.tres",16384u,0xff248289u,1,1,1,ItemSet::TECH,true,true,true,Drill,true,8388608u},
    {"EARTH","Earth","data/balls_data/earth.tres",1280u,0xff9e4b2du,1,1,0,ItemSet::SPACE,true,true,true,Earth,true,8388608u},
    {"EGG","Egg","data/balls_data/egg.tres",40u,0xff829dcbu,2,1,0,ItemSet::NATURE,true,true,true,Egg,true,768u},
    {"EGG-TIMER","Egg Timer","data/balls_data/egg-timer.tres",16392u,0xffadadadu,1,1,1,ItemSet::FOOD,true,true,true,EggTimer,true,8388616u},
    {"ENERGY-BALL","Energy Ball","data/balls_data/energy-ball.tres",0u,0xffffffffu,0,0,0,ItemSet::NONE,false,true,true,Energy,true,0u},
    {"EYEBALL","Eyeball","data/balls_data/eyeball.tres",2048u,0xffa4a4d6u,1,1,0,ItemSet::SPOOKY,true,true,true,Eyeball,true,8388608u},
    {"FIREBALL","Fireball","data/balls_data/fireball.tres",40u,0xff5572e0u,1,1,2,ItemSet::FOOD,true,true,true,Fireball,true,4u},
    {"FISH","Fish","data/balls_data/fish.tres",24u,0xff78b3dau,1,1,0,ItemSet::NONE,false,true,true,Plain,true,0u},
    {"FISHBOWL","Fishbowl","data/balls_data/fishbowl.tres",16u,0xffdd9157u,2,1,0,ItemSet::FRIENDS,true,true,true,Fishbowl,true,384u},
    {"FLOWER-BUSH","Flower Bush","data/balls_data/flower_bush.tres",32u,0xffdf93e1u,1,1,0,ItemSet::NATURE,true,true,true,FlowerBush,true,8388640u},
    {"FORGETMENOT","Forgetmenot","data/balls_data/forget-me-not.tres",0u,0xff000000u,1,1,2,ItemSet::NATURE,false,true,true,Unsupported,false,0u},
    {"FRIDGE","Fridge","data/balls_data/fridge.tres",16384u,0xffdaae6eu,1,1,1,ItemSet::FOOD,true,true,true,Fridge,true,8388610u},
    {"FRIENDSPHERE","Friendsphere","data/balls_data/friend-sphere.tres",0u,0xffd28862u,1,1,0,ItemSet::FRIENDS,false,true,true,Friendsphere,true,8388608u},
    {"FROG","Frog","data/balls_data/frog.tres",16u,0xff7ab893u,1,1,1,ItemSet::FRIENDS,true,true,true,Frog,true,8388608u},
    {"GACHA","Gacha","data/balls_data/gacha_ball.tres",2u,0xffdfdabcu,1,1,1,ItemSet::CLASSIC,true,true,true,Gacha,true,2097184u},
    {"GAIA","Gaia","data/balls_data/gaia.tres",32u,0xff186c68u,3,1,3,ItemSet::NATURE,true,true,true,Gaia,true,0u},
    {"GALAXY","Galaxy","data/balls_data/galaxy.tres",1024u,0xff9f9494u,2,1,2,ItemSet::SPACE,true,false,true,Galaxy,true,8388608u},
    {"GAMEBALL","Gameball","data/balls_data/gameball.tres",16384u,0xffafc4c1u,1,1,1,ItemSet::TECH,false,true,true,Gameball,true,8192u},
    {"GHOST","Ghost","data/balls_data/ghost.tres",2064u,0xffb0b0b0u,1,1,0,ItemSet::SPOOKY,true,true,true,Ghost,true,32u},
    {"GIFT","Gift","data/balls_data/gift.tres",2064u,0xff565044u,1,1,0,ItemSet::SPOOKY,false,true,true,Gift,true,8388608u},
    {"GINGERBREAD","Gingerbread","data/balls_data/gingerbread-man.tres",25u,0xff5588b1u,2,1,0,ItemSet::FOOD,true,true,true,Gingerbread,true,512u},
    {"GLITCH","Glitch","data/balls_data/negative_balls/glitch.tres",64u,0xff6d62d2u,1,1,0,ItemSet::NEGATIVE,true,false,true,Glitch,true,6u},
    {"GOAT","Goat","data/balls_data/goat.tres",16u,0xff8699abu,1,1,0,ItemSet::FRIENDS,true,true,true,Goat,true,128u},
    {"GRAVESTONE","Gravestone","data/balls_data/gravestone.tres",2048u,0xff72767cu,1,1,0,ItemSet::SPOOKY,true,true,true,Gravestone,true,64u},
    {"GREEN-APPLE","Green Apple","data/balls_data/green_apple.tres",8u,0xff57811cu,1,1,0,ItemSet::NONE,false,true,true,Unsupported,false,0u},
    {"HERMIT-CRAB","Hermit Crab","data/balls_data/hermit_crab.tres",16u,0xffa3a3a3u,1,1,1,ItemSet::FRIENDS,false,false,true,Unsupported,false,0u},
    {"HIT-BALL","Hit Ball","data/balls_data/hit-ball.tres",0u,0xffd5a99du,1,1,0,ItemSet::BASIC,false,true,true,HitBall,true,16u},
    {"ICECUBE","Icecube","data/balls_data/icecube.tres",40u,0xffcdc247u,1,1,1,ItemSet::FOOD,false,true,true,Icecube,true,4u},
    {"IMP","Imp","data/balls_data/imp.tres",0u,0xff2a2f99u,-2,1,0,ItemSet::NEGATIVE,false,true,true,Plain,true,0u},
    {"INCUBATOR","Incubator","data/balls_data/incubator.tres",18432u,0xff68748eu,1,1,1,ItemSet::SPOOKY,true,true,true,Incubator,true,1024u},
    {"JACK-O-LANTERN","Jack O Lantern","data/balls_data/jackolantern.tres",2056u,0xff1c82f8u,1,1,0,ItemSet::SPOOKY,false,true,true,Plain,true,0u},
    {"JELLYFISH","Jellyfish","data/balls_data/jellyfish.tres",16u,0xffc26d69u,1,1,2,ItemSet::FRIENDS,true,false,true,Jellyfish,true,8388610u},
    {"JUPITER","Jupiter","data/balls_data/jupiter.tres",1280u,0xff6388c4u,1,1,0,ItemSet::SPACE,false,true,true,Jupiter,true,8388608u},
    {"KEYRING","Keyring","data/balls_data/keyring.tres",16384u,0xff9b9b9bu,3,1,0,ItemSet::TECH,false,true,true,Keyring,true,8388608u},
    {"LADYBUG","Ladybug","data/balls_data/ladybug.tres",48u,0xff3844cfu,1,1,0,ItemSet::NATURE,true,true,true,Ladybug,true,2u},
    {"LANTERN","Lantern","data/balls_data/lantern.tres",2048u,0xff31688au,1,1,1,ItemSet::SPOOKY,true,true,true,Lantern,true,32u},
    {"LETTUCE","Lettuce","data/balls_data/lettuce.tres",40u,0xff469668u,1,1,0,ItemSet::FOOD,true,true,true,Lettuce,true,8388608u},
    {"LICH","Lich","data/balls_data/lich.tres",2048u,0xffafa66eu,3,1,3,ItemSet::SPOOKY,true,true,true,Lich,true,8388608u},
    {"LIGHTBULB","Lightbulb","data/balls_data/lightbulb.tres",16384u,0xff7ed3e5u,1,1,0,ItemSet::TECH,true,true,true,Lightbulb,true,8388608u},
    {"LOCKED","Locked","data/balls_data/locked.tres",0u,0xff515151u,1,1,0,ItemSet::NONE,false,true,true,Unsupported,false,0u},
    {"LOCKED-CUBE","Locked Cube","data/balls_data/locked_cube.tres",0u,0xff515151u,1,1,0,ItemSet::NONE,false,true,true,Unsupported,false,0u},
    {"LOCKED-PASSIVE","Locked Passive","data/balls_data/locked_passive.tres",0u,0xff515151u,1,1,0,ItemSet::NONE,false,true,true,Unsupported,false,0u},
    {"LOG","Log","data/balls_data/log.tres",32u,0xff3f5486u,1,1,0,ItemSet::NATURE,true,true,true,Log,true,8388608u},
    {"LUNA","Luna","data/balls_data/luna.tres",1024u,0xffc5c2b8u,2,1,0,ItemSet::SPACE,true,false,true,Luna,true,16u},
    {"MACHINE","Machine","data/balls_data/machine.tres",16384u,0xff4e5153u,1,1,0,ItemSet::TECH,true,true,true,Machine,true,8388608u},
    {"MARS","Mars","data/balls_data/mars.tres",1280u,0xff3251ccu,1,1,0,ItemSet::SPACE,false,true,true,Mars,true,8388608u},
    {"MAZE","Maze","data/balls_data/negative_balls/maze.tres",128u,0xff6d62d2u,1,3,3,ItemSet::NEGATIVE,false,false,true,Unsupported,false,0u},
    {"MEATBALL","Meatball","data/balls_data/meatball.tres",8u,0xff2e5879u,2,1,2,ItemSet::FOOD,true,true,true,Meatball,true,8388608u},
    {"MERCURY","Mercury","data/balls_data/mercury.tres",1280u,0xff93a291u,1,1,0,ItemSet::SPACE,false,true,true,Mercury,true,8388608u},
    {"MIMIC","Mimic","data/balls_data/mimic.tres",2048u,0xff900d3fu,1,1,1,ItemSet::SPOOKY,true,true,true,Mimic,true,4u},
    {"MOSS","Moss","data/balls_data/moss.tres",32u,0xff79914fu,1,1,1,ItemSet::NATURE,true,true,true,Moss,true,512u},
    {"MUSHROOM","Mushroom","data/balls_data/mushroom.tres",40u,0xff5a5acfu,1,1,1,ItemSet::NATURE,true,true,true,Mushroom,true,8388608u},
    {"NEPTUNE","Neptune","data/balls_data/neptune.tres",1280u,0xffc36c52u,1,1,0,ItemSet::SPACE,false,true,true,Neptune,true,8388608u},
    {"NEST","Nest","data/balls_data/nest.tres",32u,0xff627cd2u,2,1,2,ItemSet::NATURE,true,true,true,Nest,true,2u},
    {"OBSERVATORY","Observatory","data/balls_data/observatory.tres",17408u,0xffe6e7e6u,2,1,2,ItemSet::SPACE,true,false,true,Observatory,true,2u},
    {"OCTOPUS","Octopus","data/balls_data/octopus.tres",16u,0xff365d7cu,1,1,2,ItemSet::FRIENDS,true,true,true,Octopus,true,8388608u},
    {"ONIGIRI","Onigiri","data/balls_data/onigiri.tres",8u,0xffe1e1e1u,1,1,0,ItemSet::FOOD,true,true,true,Onigiri,true,8388608u},
    {"OOZE","Ooze","data/balls_data/ooze.tres",2048u,0xff62b416u,1,1,1,ItemSet::SPOOKY,true,true,true,Ooze,true,32u},
    {"ORANGE","Orange","data/balls_data/orange.tres",40u,0xff259cf8u,1,1,0,ItemSet::FOOD,true,true,true,Orange,true,8388608u},
    {"ORANGE-JUICE","Orange Juice","data/balls_data/orange_juice.tres",8u,0xff259cf8u,1,1,0,ItemSet::FOOD,false,false,true,Plain,true,0u},
    {"ORBIT","Orbit","data/balls_data/orbit.tres",9248u,0xffdf9745u,2,1,1,ItemSet::SPACE,false,true,true,Unsupported,false,0u},
    {"OVEN","Oven","data/balls_data/oven.tres",16384u,0xff35383au,2,1,2,ItemSet::FOOD,true,true,true,Oven,true,2u},
    {"PANELLET","Panellet","data/balls_data/panellet.tres",8u,0xff4ea8e2u,1,1,1,ItemSet::FOOD,true,true,true,Panellet,true,2u},
    {"PAPERWORK","Paperwork","data/balls_data/negative_balls/paperwork.tres",0u,0xff6d62d2u,1,1,0,ItemSet::NEGATIVE,true,false,true,Paperwork,true,2u},
    {"PARROT","Parrot","data/balls_data/parrot.tres",16u,0xff6462d2u,2,1,2,ItemSet::FRIENDS,true,true,true,Parrot,true,18u},
    {"PATCHWORK","Patchwork","data/balls_data/negative_balls/patchwork.tres",0u,0xff6d62d2u,1,1,0,ItemSet::NEGATIVE,true,false,true,Patchwork,true,6u},
    {"PEACOCK","Peacock","data/balls_data/peacock.tres",16u,0xff66965cu,1,1,1,ItemSet::FRIENDS,true,true,true,Peacock,true,2u},
    {"PENDING","Pending","data/balls_data/pending.tres",0u,0xff515151u,1,1,0,ItemSet::NONE,false,true,true,Unsupported,false,0u},
    {"PET_FOOD","Pet Food","data/balls_data/pet_food.tres",8u,0xffa3a3a3u,1,1,0,ItemSet::FRIENDS,true,false,true,PetFood,true,1048576u},
    {"PIGGYBANK","Piggybank","data/balls_data/piggybank.tres",16u,0xffcd84dau,1,1,0,ItemSet::FRIENDS,false,true,true,Piggybank,true,1048576u},
    {"PIPING","Piping","data/balls_data/piping.tres",16384u,0xff33373au,1,1,0,ItemSet::TECH,true,true,true,Piping,true,8388640u},
    {"PLUTO","Pluto","data/balls_data/pluto.tres",1536u,0xff978786u,1,1,0,ItemSet::SPACE,false,true,true,Pluto,true,8388608u},
    {"POT","Pot","data/balls_data/pot.tres",16392u,0xff9b9b9bu,2,1,2,ItemSet::FOOD,true,true,true,Pot,true,2u},
    {"POTION","Potion","data/balls_data/potion.tres",2056u,0xff59ac70u,1,1,0,ItemSet::SPOOKY,true,true,true,Potion,true,1048576u},
    {"PRINTER","Printer","data/balls_data/printer.tres",16384u,0xff8a8a8au,1,1,2,ItemSet::TECH,true,true,true,Printer,true,8388608u},
    {"PUMPKIN","Pumpkin","data/balls_data/pumpkin.tres",2056u,0xff1c82f8u,1,1,0,ItemSet::SPOOKY,true,true,true,Pumpkin,true,384u},
    {"PUZZLEBALL","Puzzleball","data/balls_data/puzzleball.tres",16384u,0xff3a3a3au,1,1,2,ItemSet::TECH,true,true,true,Puzzleball,true,2u},
    {"RACOON","Racoon","data/balls_data/racoon.tres",16u,0xffa3a3a3u,1,1,1,ItemSet::FRIENDS,true,false,true,Racoon,true,128u},
    {"RAINBOW","Rainbow","data/balls_data/rainbow.tres",32u,0xff6666ffu,2,1,2,ItemSet::NATURE,true,true,true,Rainbow,true,8388640u},
    {"RAT","Rat","data/balls_data/rat.tres",2064u,0xff565044u,1,1,0,ItemSet::SPOOKY,true,true,true,Rat,true,32u},
    {"RAT-KING","Rat King","data/balls_data/rat_king.tres",2064u,0xff565044u,1,1,2,ItemSet::SPOOKY,true,true,true,RatKing,true,8388610u},
    {"REAPER","Reaper","data/balls_data/reaper.tres",2048u,0xff4a4642u,2,1,2,ItemSet::SPOOKY,true,true,true,Reaper,true,8388608u},
    {"RECYCLER","Recycler","data/balls_data/recycler.tres",16384u,0xffdededeu,2,1,2,ItemSet::TECH,true,true,true,Recycler,true,2u},
    {"RED-APPLE","Red Apple","data/balls_data/red_apple.tres",8u,0xff1c41f8u,1,1,0,ItemSet::NONE,false,true,true,Unsupported,false,8388608u},
    {"RED_DWARF","Red Dwarf","data/balls_data/red_dwarf.tres",9248u,0xff2b2bedu,2,1,1,ItemSet::SPACE,false,true,true,RedDwarf,true,2u},
    {"REPLICATOR","Replicator","data/balls_data/replicator.tres",17408u,0xff6462d2u,1,1,1,ItemSet::SPACE,true,true,true,Replicator,true,8388608u},
    {"ROBOCOOK","Robocook","data/balls_data/robocook.tres",8u,0xff959595u,3,1,2,ItemSet::FOOD,false,true,true,Robocook,true,514u},
    {"ROBOT","Robot","data/balls_data/robot.tres",16400u,0xff8a8a8au,2,1,1,ItemSet::TECH,true,true,true,Robot,true,2u},
    {"ROCKET","Rocket","data/balls_data/rocket.tres",17408u,0xffa3a3a3u,1,1,0,ItemSet::SPACE,true,false,true,Rocket,true,66u},
    {"ROSE","Rose","data/balls_data/rose.tres",32u,0xff000000u,1,1,0,ItemSet::NATURE,true,true,true,Rose,true,8388608u},
    {"SAFE","Safe","data/balls_data/safe.tres",16384u,0xff7c7c7cu,1,1,1,ItemSet::SPACE,false,false,true,Unsupported,false,0u},
    {"SAND","Sand","data/balls_data/sand.tres",32u,0xff2a8bdeu,1,1,0,ItemSet::NATURE,true,true,true,Sand,true,4096u},
    {"SATELLITE","Satellite","data/balls_data/satellite.tres",17408u,0xff767676u,3,1,0,ItemSet::SPACE,true,true,true,Satellite,true,0u},
    {"SATURN","Saturn","data/balls_data/saturn.tres",1280u,0xff6388c4u,1,1,0,ItemSet::SPACE,false,true,true,Saturn,true,8388608u},
    {"SCRAP","Scrap","data/balls_data/scrap.tres",16384u,0xff363c42u,1,1,0,ItemSet::TECH,true,true,true,Scrap,true,131072u},
    {"SCREW","Screw","data/balls_data/screw.tres",16384u,0xff9b9b9bu,4,1,0,ItemSet::TECH,true,true,true,Screw,true,32u},
    {"SHARK_PLUSH","Shark Plush","data/balls_data/blahaj.tres",16u,0xff826f36u,2,1,2,ItemSet::FRIENDS,true,true,true,SharkPlush,true,2048u},
    {"SHEEP","Sheep","data/balls_data/sheep.tres",16u,0xffc2d0d0u,1,1,0,ItemSet::FRIENDS,true,true,true,Sheep,true,128u},
    {"SKULL","Skull","data/balls_data/skull.tres",2048u,0xffa0c4c4u,1,1,1,ItemSet::SPOOKY,true,true,true,Skull,true,8388608u},
    {"SNAKE","Snake","data/balls_data/snake.tres",16u,0xff579271u,1,1,0,ItemSet::FRIENDS,true,true,true,Snake,true,2u},
    {"SPIRIT","Spirit","data/balls_data/spirit.tres",2048u,0xffd1c19du,1,1,0,ItemSet::SPOOKY,true,true,true,Spirit,true,2u},
    {"STAR","Star","data/balls_data/star.tres",9216u,0xff62cbd2u,1,1,0,ItemSet::SPACE,true,false,true,Star,true,8388608u},
    {"STARMAP","Starmap","data/balls_data/starmap.tres",17408u,0xff31688au,1,1,1,ItemSet::SPACE,true,true,true,Starmap,true,32u},
    {"STAR_SYSTEM","Star System","data/balls_data/star_system.tres",1024u,0xff9c6230u,1,1,1,ItemSet::SPACE,true,false,true,StarSystem,true,8388608u},
    {"STONKS","Stonks","data/balls_data/stonks.tres",4u,0xffd34b5cu,1,1,0,ItemSet::CLASSIC,false,true,true,Unsupported,false,1048576u},
    {"STOVE","Stove","data/balls_data/stove.tres",16384u,0xff5572e0u,1,1,0,ItemSet::FOOD,true,true,true,Stove,true,8388640u},
    {"SUN","Sun","data/balls_data/sun.tres",9248u,0xff85c7f2u,2,1,2,ItemSet::SPACE,true,true,true,Sun,true,32u},
    {"SUNFLOWER","Sunflower","data/balls_data/sunflower.tres",32u,0xff2a8bdeu,1,1,1,ItemSet::NATURE,true,true,true,Sunflower,true,8388608u},
    {"SUPERNOVA","Supernova","data/balls_data/supernova.tres",1024u,0xff493133u,3,1,3,ItemSet::SPACE,true,true,true,Supernova,true,8388608u},
    {"TAKOYAKI","Takoyaki","data/balls_data/takoyaki.tres",8u,0xff7088a7u,2,1,0,ItemSet::FOOD,true,true,true,Takoyaki,true,32u},
    {"TESLA-BALL","Tesla Ball","data/balls_data/tesla.tres",16384u,0xffd781ccu,1,1,0,ItemSet::TECH,true,true,true,Tesla,true,2u},
    {"THORNS","Thorns","data/balls_data/negative_balls/thorns.tres",64u,0xff6d62d2u,1,1,0,ItemSet::NEGATIVE,true,false,true,Thorns,true,6u},
    {"TIGER","Tiger","data/balls_data/tiger.tres",16u,0xff5488cdu,3,1,3,ItemSet::FRIENDS,true,false,true,Tiger,true,8388608u},
    {"TIME_MACHINE","Time Machine","data/balls_data/time_machine.tres",16384u,0xff394291u,2,1,3,ItemSet::TECH,true,true,true,TimeMachine,true,2u},
    {"TOMATO","Tomato","data/balls_data/tomato.tres",40u,0xff383bffu,1,1,0,ItemSet::FOOD,true,true,true,Tomato,true,8388608u},
    {"TOOLBOX","Toolbox","data/balls_data/toolbox.tres",16384u,0xff39479eu,1,1,1,ItemSet::TECH,true,true,true,Toolbox,true,8388608u},
    {"TORNADO","Tornado","data/balls_data/tornado.tres",32u,0xffa3855du,2,1,2,ItemSet::NATURE,true,true,true,Tornado,true,8388608u},
    {"TRACKS","Tracks","data/balls_data/tracks.tres",16384u,0xff737373u,1,1,1,ItemSet::TECH,false,true,true,Tracks,true,8388608u},
    {"TRASH","Trash","data/balls_data/trash.tres",16384u,0xff282828u,1,1,2,ItemSet::TECH,false,true,true,Trash,true,8388608u},
    {"TREASURE","Treasure","data/balls_data/treasure.tres",0u,0xff496588u,1,1,2,ItemSet::BASIC,false,true,true,Unsupported,false,0u},
    {"TUNGSTEN-BALL","Tungsten Ball","data/balls_data/tungsten_ball.tres",0u,0xff938278u,0,1,0,ItemSet::NEGATIVE,false,true,true,Plain,true,0u},
    {"TURBINE","Turbine","data/balls_data/turbine.tres",16384u,0xffbdbdbdu,1,1,1,ItemSet::TECH,true,true,true,Turbine,true,2u},
    {"TV","Tv","data/balls_data/tv.tres",16384u,0xff36465du,1,1,1,ItemSet::TECH,true,true,true,Television,true,65536u},
    {"TYRANT","Tyrant","data/balls_data/negative_balls/tyrant.tres",0u,0xff6d62d2u,1,1,0,ItemSet::NEGATIVE,true,false,true,Tyrant,true,64u},
    {"UFO","Ufo","data/balls_data/ufo.tres",17408u,0xff799255u,1,1,1,ItemSet::SPACE,true,false,true,Ufo,true,2u},
    {"URANUS","Uranus","data/balls_data/uranus.tres",1280u,0xffdda494u,1,1,0,ItemSet::SPACE,false,true,true,Uranus,true,8388608u},
    {"VACUUM","Vacuum","data/balls_data/vacuum.tres",16400u,0xff535353u,1,1,1,ItemSet::TECH,false,true,true,Vacuum,true,32768u},
    {"VAMPIRE","Vampire","data/balls_data/vampire.tres",2048u,0xff4a4642u,2,1,1,ItemSet::SPOOKY,true,true,true,Vampire,true,2u},
    {"VENUS","Venus","data/balls_data/venus.tres",1280u,0xff369eeau,1,1,0,ItemSet::SPACE,false,true,true,Venus,true,8388608u},
    {"WEIGHTS","Weights","data/balls_data/negative_balls/weight.tres",0u,0xff6d62d2u,1,1,0,ItemSet::NEGATIVE,true,false,true,Weights,true,6u},
    {"WEREWOLF","Werewolf","data/balls_data/werewolf.tres",2048u,0xff4a4642u,2,1,2,ItemSet::SPOOKY,false,true,true,Werewolf,true,128u},
    {"WEREWOLF_HUMAN","Werewolf Human","data/balls_data/werewolf_human.tres",2048u,0xff4a4642u,2,1,2,ItemSet::SPOOKY,false,true,true,WerewolfHuman,true,128u},
    {"WHEEL","Wheel","data/balls_data/wheel.tres",16384u,0xff282828u,1,1,0,ItemSet::TECH,true,true,true,Wheel,true,8388608u},
    {"WINDOW","Window","data/balls_data/window.tres",32u,0xffc7b8b2u,1,1,1,ItemSet::NATURE,true,true,true,Window,true,8388608u},
    {"WITCH","Witch","data/balls_data/witch.tres",2048u,0xff868042u,2,1,2,ItemSet::SPOOKY,true,true,true,Witch,true,4u},
    {"WORMHOLE","Wormhole","data/balls_data/wormhole.tres",1024u,0xffc079c3u,1,1,1,ItemSet::SPACE,true,true,true,Wormhole,true,8388608u},
    {"ZOMBIE","Zombie","data/balls_data/zombie.tres",2048u,0xff21772cu,2,1,2,ItemSet::SPOOKY,true,true,true,Zombie,true,128u},
};
const ContentInfo snacks[]={
    {"","Empty","",0u,0xff000000u,1,1,0,ItemSet::NONE,false,false,false,Unsupported,false,0u},
    {"OLIVE","Olive","data/passives_data/olive.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,4u},
    {"CROISSANT","Croissant","data/passives_data/croissant.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,4u},
    {"CHOCOLATE","Chocolate","data/passives_data/chocolate.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,4u},
    {"CEREAL","Cereal","data/passives_data/cereal.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"POKER-CHIP","Poker Chip","data/passives_data/poker_chip.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"STRAWBERRY","Strawberry","data/passives_data/strawberry.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,4u},
    {"BLACKBERRY","Blackberry","data/passives_data/blackberry.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,4u},
    {"HAM-SANDWITCH","Ham Sandwich","data/passives_data/ham_sandwitch.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,4u},
    {"CRISP-SANDWITCH","Crisp Sandwich","data/passives_data/crisp_sandwitch.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,4u},
    {"BLACK-OLIVE","Black Olive","data/passives_data/black_olive.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,4u},
    {"MUFFIN","Muffin","data/passives_data/muffin.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,8u},
    {"MINI-MUFFIN","Mini Muffin","data/passives_data/mini-muffin.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,8u},
    {"8-BAGEL","8 Bagel","data/passives_data/8-bagel.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"AFTER-EIGHT","After Eight","data/passives_data/after_eight.tres",0u,0xff000000u,1,1,2,ItemSet::NONE,true,true,true,Plain,true,6u},
    {"BIG-NACHO","Big Nacho","data/passives_data/big_nacho.tres",0u,0xff000000u,1,1,2,ItemSet::NONE,true,true,true,Plain,true,0u},
    {"BLUE-CHEESE","Blue Cheese","data/passives_data/blue_cheese.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,4u},
    {"BRAVAS","Bravas","data/passives_data/bravas.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"CLASSIC_TACO","Classic Taco","data/passives_data/classic_taco.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,18u},
    {"CRISPS","Crisps","data/passives_data/crisps.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"CRUNCHY_TACO","Crunchy Taco","data/passives_data/crunchy_taco.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,18u},
    {"DAISY-CUPCAKE","Daisy Cupcake","data/passives_data/daisy_cupcake.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"EDAMAME","Edamame","data/passives_data/edamame.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"ESPRESSO","Espresso","data/passives_data/espresso.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,4u},
    {"FISH_FINGER","Fish Finger","data/passives_data/fish_finger.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"FORTUNE-COOKIE","Fortune Cookie","data/passives_data/fortune-cookie.tres",0u,0xff000000u,1,1,2,ItemSet::NONE,true,true,true,Plain,true,16384u},
    {"FRIED-SHRIMP","Fried Shrimp","data/passives_data/fried_shrimp.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,4194304u},
    {"FRUIT-SALAD","Fruit Salad","data/passives_data/fruit_salad.tres",0u,0xff000000u,1,1,2,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"GUMMYBEAR","Gummybear","data/passives_data/gummybear.tres",0u,0xff000000u,1,1,2,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"GUMMYWORM","Gummyworm","data/passives_data/gummyworm.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"HALO-HALO","Halo Halo","data/passives_data/halohalo.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,0u},
    {"JASMINE-TEA","Jasmine Tea","data/passives_data/jasmine_tea.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,4u},
    {"KEY-LIME","Key Lime","data/passives_data/key_lime.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"LOTUS-ROOT","Lotus Root","data/passives_data/lotus_root.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,0u},
    {"LOVE-CAKE","Love Cake","data/passives_data/love_cake.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,1048576u},
    {"MOONCAKE","Mooncake","data/passives_data/mooncake.tres",0u,0xff000000u,1,1,2,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"NUTS","Nuts","data/passives_data/nuts.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"RAINBOW_TACO","Rainbow Taco","data/passives_data/rainbow_taco.tres",0u,0xff000000u,1,1,2,ItemSet::NONE,true,true,true,Plain,true,18u},
    {"SAUSAGE_ROLL","Sausage Roll","data/passives_data/sausage_roll.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"SCRAMBLED-EGGS","Scrambled Eggs","data/passives_data/scrambled_eggs.tres",0u,0xff000000u,1,1,2,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"SHRIMP","Shrimp","data/passives_data/shrimp.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,4194304u},
    {"SMALL-NACHO","Small Nacho","data/passives_data/small_nacho.tres",0u,0xff000000u,1,1,2,ItemSet::NONE,true,true,true,Plain,true,4u},
    {"SOYSAUCE","Soysauce","data/passives_data/soysauce.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"STAR_COOKIE","Star Cookie","data/passives_data/star_cookie.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"SUNFLOWER-CUPCAKE","Sunflower Cupcake","data/passives_data/sunflower_cupcake.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"TIGER-ROLL","Tiger Roll","data/passives_data/tiger_roll.tres",0u,0xff000000u,1,1,2,ItemSet::NONE,true,true,true,Plain,true,8u},
    {"TOAST","Toast","data/passives_data/toast.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,2u},
    {"TV-DINNER","Tv Dinner","data/passives_data/tv_dinner.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,65536u},
    {"TWO-OLIVES","Two Olives","data/passives_data/two_olives.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,false,true,true,Unsupported,false,0u},
    {"VEGGIE_TACO","Veggie Taco","data/passives_data/veggie_taco.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,18u},
    {"VITAMINS","Vitamins","data/passives_data/vitamins.tres",0u,0xff000000u,1,1,1,ItemSet::NONE,true,true,true,Plain,true,1048576u},
    {"WINGS","Wings","data/passives_data/wings.tres",0u,0xff000000u,1,1,0,ItemSet::NONE,true,true,true,Plain,true,4u},
};
}
const ContentInfo& ball_info(unsigned id) { return balls[id<content_count?id:0]; }
const ContentInfo& snack_info(unsigned id) { return snacks[id<snack_count?id:0]; }
unsigned find_ball(const char* key) { if(key) for(unsigned i=0;i<content_count;++i) if(std::strcmp(key,balls[i].key)==0) return i; return content_count; }
unsigned find_snack(const char* key) { if(key) for(unsigned i=1;i<snack_count;++i) if(std::strcmp(key,snacks[i].key)==0) return i; return snack_count; }
static_assert(signal_bit(Signal::Score)==1u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::PocketAnother)==2u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::RoundStart)==4u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::RoundEnd)==8u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::Shot)==16u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::Spawn)==32u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::SpawnAnother)==64u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::Hit)==128u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::Wall)==256u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::Buff)==512u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::Transform)==1024u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::TransformAnother)==2048u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::ScoreChange)==4096u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::ReachScore)==8192u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::MaxRoll)==16384u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::Pickup)==32768u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::EnterShop)==65536u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::Upgrade)==131072u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::SellAnother)==262144u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::Reroll)==524288u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::SellSelf)==1048576u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::Buy)==2097152u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::BuyAnother)==4194304u, "Regenerate catalog after signal schema change");
static_assert(signal_bit(Signal::Pocket)==8388608u, "Regenerate catalog after signal schema change");
uint32_t ability_signals(unsigned ability) {
    if(ability==AiBrain) return 8388608u;
    if(ability==Alien) return 8388608u;
    if(ability==Amber) return 65536u;
    if(ability==Ant) return 8388608u;
    if(ability==Antenna) return 8388608u;
    if(ability==Antimatter) return 512u;
    if(ability==Apple) return 8388608u;
    if(ability==Asteroid) return 8388736u;
    if(ability==Astrolabe) return 1048576u;
    if(ability==Astronaut) return 128u;
    if(ability==Bacon) return 8388608u;
    if(ability==Banana) return 8388608u;
    if(ability==Basketball) return 1u;
    if(ability==Battery) return 196608u;
    if(ability==Beach) return 8388608u;
    if(ability==Beans) return 514u;
    if(ability==Bear) return 8388608u;
    if(ability==Bee) return 512u;
    if(ability==BlackHole) return 8388608u;
    if(ability==Blueprint) return 128u;
    if(ability==Bomb) return 384u;
    if(ability==Bottle) return 8388608u;
    if(ability==Bouncer) return 6u;
    if(ability==Bricks) return 256u;
    if(ability==Bunny) return 8u;
    if(ability==Burger) return 32u;
    if(ability==Bush) return 512u;
    if(ability==Butterfly) return 8388640u;
    if(ability==Calculator) return 64u;
    if(ability==Candle) return 8388608u;
    if(ability==Cat) return 8388608u;
    if(ability==Caterpillar) return 514u;
    if(ability==Cauldron) return 786432u;
    if(ability==Ceo) return 4u;
    if(ability==Cheese) return 8388608u;
    if(ability==ChefHat) return 2u;
    if(ability==Chicken) return 8388640u;
    if(ability==Clocktower) return 34u;
    if(ability==Cloud) return 384u;
    if(ability==Coffin) return 2u;
    if(ability==Comet) return 8388608u;
    if(ability==Compass) return 32u;
    if(ability==Conveyor) return 8388608u;
    if(ability==CoolGuy) return 2u;
    if(ability==Coots) return 128u;
    if(ability==Cubert) return 6u;
    if(ability==Dandelion) return 8388608u;
    if(ability==Demon) return 6u;
    if(ability==Dice) return 2u;
    if(ability==Dog) return 2u;
    if(ability==Donut) return 2u;
    if(ability==Drill) return 8388608u;
    if(ability==Earth) return 8388608u;
    if(ability==Egg) return 768u;
    if(ability==EggTimer) return 8388616u;
    if(ability==Energy) return 0u;
    if(ability==Eyeball) return 8388608u;
    if(ability==Fireball) return 4u;
    if(ability==Fishbowl) return 384u;
    if(ability==FlowerBush) return 8388640u;
    if(ability==Football) return 1u;
    if(ability==Fridge) return 8388610u;
    if(ability==Friendsphere) return 8388608u;
    if(ability==Frog) return 8388608u;
    if(ability==Gacha) return 2097184u;
    if(ability==Gaia) return 0u;
    if(ability==Galaxy) return 8388608u;
    if(ability==Gameball) return 8192u;
    if(ability==Ghost) return 32u;
    if(ability==Gift) return 8388608u;
    if(ability==Gingerbread) return 512u;
    if(ability==Glitch) return 6u;
    if(ability==Goat) return 128u;
    if(ability==Gold) return 8388608u;
    if(ability==Gravestone) return 64u;
    if(ability==HitBall) return 16u;
    if(ability==Icecube) return 4u;
    if(ability==Incubator) return 1024u;
    if(ability==Jellyfish) return 8388610u;
    if(ability==Juggling) return 8388608u;
    if(ability==Jupiter) return 8388608u;
    if(ability==Keyring) return 8388608u;
    if(ability==Ladybug) return 2u;
    if(ability==Lantern) return 32u;
    if(ability==Lettuce) return 8388608u;
    if(ability==Lich) return 8388608u;
    if(ability==Lightbulb) return 8388608u;
    if(ability==Log) return 8388608u;
    if(ability==Luna) return 16u;
    if(ability==Machine) return 8388608u;
    if(ability==Marble) return 128u;
    if(ability==Mars) return 8388608u;
    if(ability==Meatball) return 8388608u;
    if(ability==Mercury) return 8388608u;
    if(ability==Mimic) return 4u;
    if(ability==Mirror) return 32u;
    if(ability==Moss) return 512u;
    if(ability==Mushroom) return 8388608u;
    if(ability==Neptune) return 8388608u;
    if(ability==Nest) return 2u;
    if(ability==NestingDoll) return 8388608u;
    if(ability==Number) return 0u;
    if(ability==Observatory) return 2u;
    if(ability==Octopus) return 8388608u;
    if(ability==Onigiri) return 8388608u;
    if(ability==Ooze) return 32u;
    if(ability==Orange) return 8388608u;
    if(ability==Oven) return 2u;
    if(ability==Panellet) return 2u;
    if(ability==Pants) return 8388608u;
    if(ability==Paperwork) return 2u;
    if(ability==Parrot) return 18u;
    if(ability==Patchwork) return 6u;
    if(ability==Peacock) return 2u;
    if(ability==PetFood) return 1048576u;
    if(ability==Piggybank) return 1048576u;
    if(ability==Pinball) return 384u;
    if(ability==Piping) return 8388640u;
    if(ability==Plain) return 0u;
    if(ability==Plushie) return 2u;
    if(ability==Pluto) return 8388608u;
    if(ability==Pot) return 2u;
    if(ability==Potion) return 1048576u;
    if(ability==Printer) return 8388608u;
    if(ability==Pumpkin) return 384u;
    if(ability==Puzzleball) return 2u;
    if(ability==Racoon) return 128u;
    if(ability==Rainbow) return 8388640u;
    if(ability==Rat) return 32u;
    if(ability==RatKing) return 8388610u;
    if(ability==Reaper) return 8388608u;
    if(ability==Recycler) return 2u;
    if(ability==RedDwarf) return 2u;
    if(ability==Replicator) return 8388608u;
    if(ability==Robocook) return 514u;
    if(ability==Robot) return 2u;
    if(ability==Rocket) return 66u;
    if(ability==Rose) return 8388608u;
    if(ability==Roulette) return 2u;
    if(ability==Sand) return 4096u;
    if(ability==Satellite) return 0u;
    if(ability==Saturn) return 8388608u;
    if(ability==Scrap) return 131072u;
    if(ability==Screw) return 32u;
    if(ability==Seal) return 128u;
    if(ability==SharkPlush) return 2048u;
    if(ability==Sheep) return 128u;
    if(ability==Skull) return 8388608u;
    if(ability==Slot) return 8388608u;
    if(ability==Smiley) return 8388608u;
    if(ability==Snake) return 2u;
    if(ability==Spent) return 32u;
    if(ability==Spirit) return 2u;
    if(ability==Star) return 8388608u;
    if(ability==StarSystem) return 8388608u;
    if(ability==Starmap) return 32u;
    if(ability==Stove) return 8388640u;
    if(ability==Sun) return 32u;
    if(ability==Sunflower) return 8388608u;
    if(ability==Supernova) return 8388608u;
    if(ability==Takoyaki) return 32u;
    if(ability==Television) return 65536u;
    if(ability==Tennis) return 1u;
    if(ability==Tesla) return 2u;
    if(ability==Thorns) return 6u;
    if(ability==Tiger) return 8388608u;
    if(ability==TimeMachine) return 2u;
    if(ability==Tomato) return 8388608u;
    if(ability==Toolbox) return 8388608u;
    if(ability==Tornado) return 8388608u;
    if(ability==Tracks) return 8388608u;
    if(ability==Trash) return 8388608u;
    if(ability==Turbine) return 2u;
    if(ability==Tyrant) return 64u;
    if(ability==Ufo) return 2u;
    if(ability==Upgrade) return 0u;
    if(ability==Uranus) return 8388608u;
    if(ability==Vacuum) return 32768u;
    if(ability==Vampire) return 2u;
    if(ability==Venus) return 8388608u;
    if(ability==Weights) return 6u;
    if(ability==Werewolf) return 128u;
    if(ability==WerewolfHuman) return 128u;
    if(ability==Wheel) return 8388608u;
    if(ability==Window) return 8388608u;
    if(ability==Witch) return 4u;
    if(ability==Wormhole) return 8388608u;
    if(ability==Yinyang) return 8388608u;
    if(ability==Zombie) return 128u;
    if(ability==Respawn) return signal_bit(Signal::Pocket); // Internal unused-source helper, no stock binding.
    return 0;
}
uint32_t ability_handlers(unsigned ability) {
    if(ability==Robocook) return ability_signals(ability)|4096u;
    return ability_signals(ability);
}
}
