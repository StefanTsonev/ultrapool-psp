#pragma once
#include <array>
#include <deque>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include "ultrapool/listeners.hpp"
#include "ultrapool/catalog.hpp"
#include "ultrapool/audio.hpp"
#include "ultrapool/progression.hpp"
#include "ultrapool/daily.hpp"
#include "ultrapool/tutorial.hpp"

namespace up {
constexpr float tick_dt = 1.0f / 180.0f;
constexpr std::size_t max_balls = 64;
struct Vec {
    float x, y;
    Vec(float a=0, float b=0): x(a), y(b) {}
    Vec operator+(Vec b) const { return Vec(x+b.x,y+b.y); }
    Vec operator-(Vec b) const { return Vec(x-b.x,y-b.y); }
    Vec operator*(float s) const { return Vec(x*s,y*s); }
    Vec& operator+=(Vec b) { x+=b.x; y+=b.y; return *this; }
    float dot(Vec b) const { return x*b.x+y*b.y; }
    float length() const { return std::sqrt(dot(*this)); }
    Vec unit() const { float n=length(); return n>1e-8f ? *this*(1/n):Vec(1,0); }
};
struct ScorePopup {Vec p,v;int amount=0;float life=0;bool passive=false;};
struct Definition {
    uint16_t id=0, ability=0;
    float radius=17, mass=1, damping=1.5f;
    int base_score=1;
    uint32_t color=0xffeeeeee;
};
enum class Weight : uint8_t { Light, Normal, Heavy, VeryHeavy };
enum class Modifier : uint8_t { Lock, Unlock, Shield, Heavy, Light, Star, Flame, PutOut, Fleeting, Shoot, Teleport, Nudge, Level, TransformRat, ShieldNudge, Reroll };
struct Ball {
    Vec p, v;
    float visual_rotation=0;
    uint32_t generation=0;
    Definition def;
    int base_value=1,temp_value=0;
    bool active=true, cue=false, energy=false;
    float energy_age=0;uint16_t collision_exception=max_balls;uint32_t exception_generation=0;
    int8_t owner=-1;
    uint16_t mixed_with=0;
    uint8_t level=1, pocket_index=0;
    Vec step_start;uint32_t motion_tick=0;
    bool captured_this_tick=false,just_respawned=false,collect_flowers=false;
    bool falling=false,locked=false,shielded=false,shield_broken=false,star_power=false,flaming=false,fleeting=false,gone=false,ignore_pockets=false;
    Weight weight=Weight::Normal;float unlocked_mass=1;
    uint16_t last_contact=max_balls;uint32_t last_contact_generation=0;
    float pull_time=0;Vec pull_target;bool pulled=false;
    uint32_t last_pocket_tick=0xffffffffu;
    float fall_remaining=0, flash=0, chase_flowers=0,rocket_time=0,conveyor_time=0,snake_time=0,chase_pocket_time=0;Vec conveyor_direction,snake_direction;bool ghost_used=false,parrot_used=false,passive=false,collision_disabled=false;
};
struct CurseCube { uint16_t id=0; uint8_t level=1; int count=0; };
constexpr unsigned max_holes=10;
struct Pocket { Vec p; float radius=24.0832f, multiplier=2; int bonus=0; bool closed=false,shielded=false,closeable=true;float scale=1,suck_scale=1;unsigned held_count=0;
    int score(int value) const { return static_cast<int>(std::floor((value+bonus)*multiplier)); }
};
enum class EventKind : uint8_t { Shot, Collision, Wall, Captured, Pocketed, Settled, RoundEnded };
struct Event {
    EventKind kind=EventKind::Shot;
    uint16_t a=0, b=0;
    uint32_t tick=0, sequence=0, generation_a=0, generation_b=0;
    int multiplier=1;
};
class Events {
    std::array<Event,1024> data;
    std::size_t first=0, count=0;
    uint32_t serial=0;
public:
    uint32_t dropped=0;
    bool push(EventKind kind, unsigned a, unsigned b, uint32_t tick,uint32_t generation_a=0,uint32_t generation_b=0,int multiplier=1);
    bool pop(Event& e);
    void clear() { first=count=0; serial=0; dropped=0; }
};
struct Metrics { uint32_t ticks=0, impacts=0, solver_exhaustions=0; };
class World {
    std::array<uint64_t,max_balls> contacts{};
    std::array<uint8_t,max_balls> wall_contacts{};
    std::array<uint16_t,max_balls> retained{};
public:
    std::array<Ball,max_balls> balls;
    std::size_t count=0;
    unsigned last_added=0;
    std::array<Pocket,6> pockets;
    std::array<Pocket,max_holes> holes;unsigned hole_count=0;
    // IDs 0..5 are table pockets; 6 remains the virtual-pocket sentinel.
    // Spawned holes use IDs 7..16. Ordinals enumerate only real pockets.
    unsigned pocket_count() const {return 6+hole_count;}
    unsigned pocket_id(unsigned ordinal) const {return ordinal<6?ordinal:ordinal+1;}
    Pocket* pocket(unsigned id);
    const Pocket* pocket(unsigned id) const;
    Pocket& pocket_at(unsigned ordinal){return *pocket(pocket_id(ordinal));}
    const Pocket& pocket_at(unsigned ordinal) const{return *pocket(pocket_id(ordinal));}
    bool add_hole(Vec position,float multiplier);
    bool pockets_animating() const;
    void update_pockets(float dt);
    Vec minimum=Vec(-270,-418), maximum=Vec(270,422);
    float restitution=1, stop_speed=10, speed_limit=2500;
    bool pockets_enabled=true;
    Events events;
    Metrics metrics;
    World();
    bool add(const Ball& b);
    void retain(unsigned index){if(index<max_balls && retained[index]<65535)++retained[index];}
    void release(unsigned index){if(index<max_balls && retained[index])--retained[index];}
    bool pair_collides(unsigned a,unsigned b) const;
    bool revive(unsigned index,Vec position);
    bool relocate(unsigned index,Vec position);
    void capture(std::size_t ball, unsigned pocket);
    bool set_pocket_closed(unsigned pocket, bool closed);
    bool moving() const;
    bool energy_active() const;
    bool passive_motion() const;
    bool valid_placement(Vec p, float radius, std::size_t exclude=0) const;
    void step(float dt=tick_dt);
};
struct AimPrediction {
    std::array<Vec,3> points{}; unsigned count=0; int hit=-1;
    Vec cue_velocity,object_velocity;
    // Bounded first-flight guides after impact. These are visual predictions;
    // simulation remains authoritative for abilities, pockets and damping.
    std::array<std::array<Vec,4>,2> travel{};
    std::array<unsigned,2> travel_count{};
};
AimPrediction predict_aim(const World& world,Vec direction,float power);
enum class Phase { Placement, Aiming, Simulating, Won, Lost, Menu, Shop, RunWon, DailyResult };
enum class ShopFocus { Market, Inventory, Actions, Passive };
enum Snack : uint8_t { NoSnack, Olive, Croissant, Chocolate, Cereal, PokerChip, Strawberry, Blackberry, HamSandwich, CrispSandwich, BlackOlive, Muffin, MiniMuffin };
const char* snack_name(unsigned id);
const char* snack_text(unsigned id);
unsigned snack_rarity(unsigned id);
int snack_sell_price(unsigned id);
unsigned snack_counter_goal(unsigned id);
Vec build_slot_position(unsigned slot);
enum Ability : uint16_t { Plain, Pinball, Bricks, Gold, Smiley, Bomb, Seal, Basketball, Tennis, Football, Beach, Dice, Marble, Plushie, Roulette, Pants, Yinyang, Juggling, Number, Upgrade, CoolGuy, Spent, Slot, NestingDoll, Mirror, Bush, Moss, Gaia, Sand, Antimatter, Gingerbread, Butterfly, Donut, Caterpillar, Goat, Sheep, Cloud, Fishbowl, Chicken, Egg, Mushroom, Incubator, SharkPlush, Pumpkin, Werewolf, WerewolfHuman, Zombie, Blueprint, Frog, Printer, Respawn, Bottle, Screw, Fridge, Coots, Fireball, Takoyaki, Astronaut, Star, StarSystem, Galaxy, Sun, Starmap, Banana, Saturn, Mercury, Venus, Earth, Mars, Jupiter, Pluto, Neptune, Uranus, Gift, Cheese, Bee, FlowerBush, Piping, Tracks, Stove, Dandelion, Rose, Sunflower, Window, Vacuum, Bacon, Lettuce, Orange, Oven, Rat, Bouncer, Patchwork, Demon, Paperwork, Weights, Cubert, Glitch, Clocktower, Tyrant, Thorns, Dog, Ghost, Gravestone, Ladybug, Scrap, Cauldron, Battery, Astrolabe, Gacha, Piggybank, Cat, Tomato, PetFood, Potion, Television, Amber, Tiger, Bunny, EggTimer, Apple, Compass, Lantern, Calculator, Rocket, Beans, Octopus, Machine, Skull, Recycler, Toolbox, ChefHat, Panellet, Meatball, Onigiri, Friendsphere, Rainbow, Log, Ant, Nest, Pot, Peacock, Spirit, Replicator, Turbine, Alien, Antenna, Wheel, Conveyor, Energy, Comet, Asteroid, Robot, Snake, Tesla, Ufo, Tornado, RedDwarf, BlackHole, Wormhole, AiBrain, Drill, Bear, Lightbulb, Keyring, Coffin, RatKing, Vampire, Supernova, Trash, Lich, Candle, Observatory, Parrot, HitBall, Luna, Jellyfish, Mimic, Witch, Ceo, Icecube, TimeMachine, Racoon, Gameball, Satellite, Eyeball, Burger, Ooze, Puzzleball, Reaper, Robocook, AbilityCount, Unsupported=65535 };
bool number_ball(unsigned id);
unsigned number_id(int value);
unsigned rarity(unsigned id);
bool shop_eligible(unsigned id,bool extra_sets);
bool ball_supported(unsigned id);
bool snack_supported(unsigned id);
uint32_t ball_tags(const Ball& ball);
bool has_tag(const Ball& ball,Tag tag);
bool is_star(const Ball& ball);
bool targetable_ball(const Ball& ball);
bool physical_pocketable(const Ball& ball);
Definition builtin(unsigned id);
void set_ball_weight(Ball& ball,Weight weight);
void set_ball_locked(Ball& ball,bool locked);
bool set_ball_shield(Ball& ball,bool shielded);
const char* ball_name(unsigned id);
int shop_price(unsigned id);
const char* ability_text(unsigned ability);
void describe_ability(unsigned ability,unsigned level,char* out,std::size_t size);
struct OwnedBall { uint16_t id=0, mixed_with=0; uint8_t level=1; int score=1; };
unsigned component_ability(const Ball& ball,unsigned side);
bool has_ability(const Ball& ball,unsigned ability);
bool has_number(const OwnedBall& item);
void normalize_numbers(OwnedBall& item);
void normalize_numbers(Ball& ball);
bool can_combine(const OwnedBall& source,const OwnedBall& destination);
void combine_values(const OwnedBall& source,OwnedBall& destination);
int item_price(const OwnedBall& item);
struct DeckDefinition { const char* name;const char* key;ItemSet set;std::array<unsigned,3> balls; };
constexpr unsigned deck_count=8;
const DeckDefinition& deck_info(unsigned deck);
bool shop_set_unlocked(const Profile& profile,ItemSet set);
const char* difficulty_name(unsigned difficulty);
int required_score(unsigned round,unsigned difficulty);
int shots_for_round(unsigned round,unsigned difficulty);
unsigned rounds_for_difficulty(unsigned difficulty);
unsigned curse_levels_for_round(unsigned round,unsigned difficulty);
OwnedBall default_item(unsigned id,unsigned level=0);
int stock_price(unsigned id);
struct Effect { Vec p,end;bool line=false,passive=false; float life=0, radius=0; uint32_t color=0xffffffff; };
struct Input {
    float analog_x=0, analog_y=0;
    bool left=false,right=false,up=false,down=false,less=false,more=false;
    bool confirm=false,cancel=false,pause=false,inspect=false,finish=false,auxiliary=false;
};
enum class ActionKind : uint8_t { None, Impulse, Spawn, PocketChange, RepeatPocket, BuffBall, PocketBall, TransformBall, ModifierBall, RepeatEffect, SplitBall, Boom, RepeatBoom, Tiger, SetupBuild, Wait, RepeatBuff, Ritual };
struct EffectCall {
    EntityRef receiver,other;Signal signal=Signal::Score;
    uint16_t ability=0;uint8_t side=0,level=1,pocket=6;
    int amount=0;bool is_moss=false;Vec origin,shot_direction;
};
struct Action {
    ActionKind kind=ActionKind::None;
    uint32_t due=0, generation=0;
    uint64_t serial=0;
    unsigned target=0, repeats=0;
    int bonus=0;
    uint32_t flags=0;
    double multiplier=0;
    Vec impulse, p, velocity, destination;
    Ball snapshot;
    EffectCall call;
    float age=0, homing_time=0;
    bool homing=false;
};
enum class PickupKind : uint8_t { Flower, Oil, Thorn, Launchpad, Stove, Flame };
constexpr unsigned max_pickups=256;
struct Pickup {
    Vec p,direction;uint64_t inside=0;uint32_t generation=0;
    float delay=.3f,scale=0,hold_time=0;
    int power=1,charges=1;uint16_t held=max_balls;uint32_t held_generation=0;
    PickupKind kind=PickupKind::Flower;bool active=false;
};
struct HauntedBall {Ball snapshot;unsigned pocket=6;};
class Game {
    double accumulator=0;
    bool was_moving=false;
    std::array<Definition,226> definitions;
    std::size_t definition_count=0;
    uint32_t rng=0x554c5452;
    float end_delay=0, nav_delay=0;
    Vec cue_start=Vec(0,210);
    std::array<ListenerBinding,max_balls> bound_balls{};
    std::array<uint8_t,4> bound_snacks{};
    std::array<uint32_t,4> snack_generations{};
    void sync_listeners();
    void retire_listener(unsigned index);
    bool live_listener(EntityRef ref) const;
    bool semantic_busy=false;
    void emit_semantic(Signal signal,int amount=0,unsigned source=max_balls);
    void drain_semantic();
    void dispatch_semantic(const SemanticEvent& event);
    void spawn_effects(unsigned index,unsigned source_ability);
    void buff_components(unsigned index,int amount,bool is_moss);
    void run_ball_event(Signal signal,unsigned index,int amount=0,unsigned other=max_balls,unsigned pocket=6,bool is_moss=false,unsigned source_ability=Unsupported,unsigned only_side=2);
    void run_effect(const EffectCall& call);
    void fire_neighbors(unsigned index,unsigned level);
    void destroy_fleeting();
    void update_pickups();
    bool pickup_holds_pending() const;
    void pickup_contact(unsigned pickup,unsigned body);
    void clear_pickup_contacts(unsigned body);
    std::array<uint32_t,max_balls> pickup_body_generations{};
    void run_snack_event(Signal signal,unsigned slot,unsigned source=max_balls,unsigned pocket=6);
    Vec snack_origin(unsigned slot) const;
    void star_planets(Vec origin,int buff);
    void star_south(Vec origin,int buff);
    void score_stars(Vec origin,unsigned count);
    void planet_effect(unsigned ability,unsigned source,unsigned level,Vec origin);
    void gift_effect(unsigned level,Vec origin);
    bool spawn_energy(Vec position,Vec impulse,unsigned exception=max_balls);
    unsigned random_pocket_id();
    bool spawn_hole(unsigned level);
    bool queue_pocket_suck(unsigned id,unsigned level,Vec origin);
    void process_pocket(const Event& event);
    bool consume_ball(unsigned index,int multiplier=1);
    void collection_effect(unsigned source,unsigned ability,unsigned level,Vec origin);
    void release_held(unsigned pocket);
    void summon_effect(unsigned source,unsigned ability,unsigned level,Vec origin,unsigned other=max_balls);
    void queue_scrap_upgrade(int buff,Vec origin);
    void content_effect(unsigned source,unsigned ability,unsigned level,Vec origin,unsigned other=max_balls);
    void shuffle_scores(unsigned level);
    void split_random(unsigned exclude,double fraction,Vec origin);
    void before_transform(unsigned index);
    void mimic_front(unsigned index,unsigned side);
    void witch_front(unsigned index,unsigned level);
    void burger_neighbors(unsigned index);
    void reaper_pull(unsigned index,unsigned level,Vec origin);
    void puzzle_effect(unsigned index,unsigned level,Vec origin);
    void reroll_body(unsigned index);
    void refresh_initial_weight(Ball& ball);
    void spawn_data(unsigned id,unsigned level,int value,Vec origin,unsigned source);
    void grow_ball(unsigned id,int amount,Vec origin,int weight=-1);
    void trigger_pocket_effects(bool fruit_only=false);
    bool find_spawn_position(Vec& destination,float radius);
    int closest_target(Vec origin,unsigned exclude);
    void buff_random_other(unsigned exclude,int amount,Vec origin);
    bool virtual_pocket(unsigned target,int multiplier);
    void score_changed(unsigned index,int amount);
    void buff_others(unsigned exclude,int amount,unsigned quantity,Vec origin,bool is_moss=false);
    int gaia_bonus() const;
    uint32_t random();
    void contact_ability(unsigned a,unsigned b,bool wall);
    void contact_component(unsigned a,unsigned b,bool wall,unsigned ability,unsigned side,unsigned level);
    void pocket_component(unsigned source,unsigned pocket,unsigned ability,unsigned level,Vec origin);
    void add_score(int amount,unsigned source);
    void complete_shot();
    void update_actions();
    void start_wisp(Action& action,ActionKind kind,Vec origin,Vec destination);
    void respawn_cue();
    Vec passive_origin(unsigned id) const;
    bool spawn_snapshot(Ball snapshot,unsigned source,bool identity_prepared=false,bool preserve_owner=false,bool setup=false,bool restore_snapshot_lock=false,unsigned* spawned_index=nullptr);
public:
    World world;
    std::deque<HauntedBall> haunts;
    std::array<EntityRef,8> passive_balls{};unsigned passive_count=0;
    static Vec passive_position(unsigned slot);
    unsigned passive_slot(unsigned body) const;
    bool queue_passive_copy(unsigned source,Vec origin);
    bool land_passive(unsigned body);
    void remove_passive(unsigned body);
    std::array<CurseCube,15> cubes{};
    bool set_cube(unsigned slot,unsigned id,unsigned level);
    void run_cube_event(Signal signal,unsigned slot,unsigned pocket=6,unsigned source=max_balls);
    void curse_heavy(Vec origin);
    void curse_teleport(unsigned amount,Vec origin);
    void reset_cube_counters();
    void setup_curses();
    SoundQueue sounds;
    bool audio_muted=false,debug_hud=false;
    unsigned music_percent=22,sfx_percent=65,settings_selection=0;
    unsigned aim_percent=100,analog_deadzone=30;
    bool analog_enabled=true,tutorial_hints=true,reduced_motion=false;
    bool seed_open=false,tutorial_replay=false,gallery_details=false;
    char seed_text[25]{};unsigned seed_cursor=0,replay_step=0;
    unsigned npc_group=0,npc_serial=0,npc_variant=0;float npc_time=0;
    int64_t earned_money_total=0;unsigned win_streak=0,planted_flowers=0,scrap_transforms=0;bool losing_before_end=false;
    void gain_money(int amount,const char* source="none");
    void react_npc(unsigned group);
    bool menu_overlay_input(const Input& in,float seconds);
    bool settings_input(const Input& in,float seconds);
    bool start_seeded_run();
    void shift_daily_month(int delta);
    void copy_settings_from(const Game& other);
    bool settings_dirty=false;
    bool save_settings(const char* path) const;
    bool load_settings(const char* path);
    void sound(const char* name,float gain=1,float pitch=1){sounds.push(name,gain,pitch);}
    std::array<Pickup,max_pickups> pickups{};
    unsigned pickup_overflows=0,pickup_contacts=0;
    float pickup_sensor_radius=10;
    int spawn_pickup(PickupKind kind,Vec position,int power=1);
    int plant_pickup(PickupKind kind,int power=1);
    void remove_pickup(unsigned index);
    unsigned flower_count() const;
    void spend_flowers(unsigned count);
    void improve_flowers(int amount,bool all=false);
    void grow_biggest_flower(int amount);
    ListenerRegistry listeners;
    SemanticQueue semantics;
    unsigned semantic_yields=0;
    // Stable references during chained emissions; storage grows only with live demand.
    std::deque<Action> actions;
    uint64_t action_serial=0;
    unsigned action_high_water=0;
    Action& allocate_action();
    unsigned action_overflows=0, spawn_rejections=0;
    bool pending_actions() const;
    bool queue_impulse(unsigned target,Vec impulse);
    void queue_nudges(unsigned amount,Vec origin,bool prefer_small);
    void queue_random_boom(Vec origin);
    void queue_boom_series(unsigned amount,Vec origin);
    void queue_tiger(float strength,bool round_end=false);
    bool queue_boom(Vec origin,Vec destination);
    void blast_at(Vec position);
    bool queue_spawn(const Ball& snapshot,Vec origin,unsigned source,bool preserve_owner=false,int post_weight=-1,bool spawn_locked=false,bool preserve_snapshot_lock=false);
    void queue_build_setup();
    void hold_round(unsigned ticks);
    void queue_apple_buffs(unsigned count,unsigned exclude,Vec origin);
    bool queue_ball_pocket(unsigned target,int multiplier,Vec origin);
    bool queue_ball_buff(unsigned target,int amount,Vec origin,bool permanent=false,bool is_moss=false,bool retarget=false,bool hatch=false);
    void buff_ball(unsigned target,int amount,bool chain=true,bool is_moss=false);
    void set_ball_score(unsigned target,int value);
    bool transform_ball(unsigned target,unsigned id,int side=0,bool permanent=false,bool transform_after=false);
    bool transform_like(unsigned target,const Ball& source);
    bool queue_ball_transform(unsigned target,const Ball& source,int buff,Vec origin);
    bool queue_ball_split(unsigned target,double fraction,bool lower_size,Vec origin,int buff=0);
    bool split_ball(unsigned target,double fraction,bool lower_size=false);
    bool queue_ball_modifier(unsigned target,Modifier modifier,int buff,Vec origin,float force=-1,bool retarget=false);
    bool queue_pocket_shield(unsigned pocket,int bonus,Vec origin);
    bool respawn_ball(unsigned target);
    void unlock_all(Vec origin);
    void lock_random_other(unsigned exclude,int buff,Vec origin);
    bool change_random_pocket_lock(bool closed);
    bool queue_pocket_change(unsigned pocket,float multiplier,int bonus,Vec origin);
    bool queue_repeated_pocket(unsigned count,float multiplier,int bonus,Vec origin);
    Phase phase=Phase::Placement;
    bool paused=false, aim_preparing=false;
    bool round_summary=false;float payout_time=0;
    int payout_base=0,payout_bonus=0,payout_shots=0,payout_total=0,round_earned=0,round_start_money=0;
    int payout_base_shown=0,payout_bonus_shown=0,payout_shots_shown=0;
    bool payout_done()const{return payout_base_shown==payout_base && payout_bonus_shown==payout_bonus && payout_shots_shown==payout_shots;}
    std::array<ScorePopup,24> score_popups{};
    float score_display=0,score_flash=0,score_sound_time=0,score_sound_pitch=1;
    void reveal_payout();
    float angle=-1.570796327f, power=0.6f;
    Vec shot_direction=Vec(0,-1);
    unsigned candles_pocketed=0,ritual_strength=0,completed_ritual_strength=0;
    float ritual_progress=0,ritual_flash=0;bool ritual_done=false;
    void pocket_candle(unsigned strength);
    void ritual_nudge();
    void echo_pocket(unsigned receiver,unsigned source,unsigned side,unsigned level);
    void observatory_copy(unsigned source,unsigned level);
    uint32_t pockets_after_last_shot=0;
    int shot_capacity=5;
    void gain_shots(unsigned amount);
    int shots=5, score=0, round=1, target=1, money=0, hp=3;
    unsigned deck=0;std::array<ItemSet,3> shop_sets{};
    void update_shop_sets();
    void reroll_shop_sets();
    ItemSet choose_shop_set();
    void update_pursuit();
    void run_shop_event(Signal signal,unsigned changed=15);
    void shop_bought(const OwnedBall& bought,unsigned destination);
    void shop_sold(const OwnedBall& sold);
    void shop_buff(unsigned slot,int amount,bool chain=true,bool is_moss=false);
    unsigned roll_content(unsigned exclude,unsigned other,uint32_t required_tags,unsigned rarity_level,bool any_set=false,unsigned exclude_extra=0,int rarity_filter=-1);
    Profile profile;
    DailyHistory daily_history;
    Pcg32 daily_random,snack_random;
    unsigned snack_tickets_used=0;
    unsigned gacha_rolls=0,scrap_rolls=0,astrolabe_rolls=0;
    unsigned seeded_content_roll(unsigned& counter,unsigned exclude,unsigned other,uint32_t required_tags,unsigned level,bool planets=false,bool twice=false);
    uint32_t run_seed=0x554c5452;unsigned rounds_played=0,total_rerolls=0;
    void set_run_seed(uint32_t value,bool custom=false);
    uint32_t daily_date=0,daily_today=0,platform_date=0;
    unsigned daily_rounds_played=0,daily_rerolls=0,daily_selection=0,daily_medals_played=0;
    bool daily_result_counted=false;
    bool daily_replay=false,daily_open=false,daily_save_pending=false,daily_snapshot_available=true;
    bool start_daily(uint32_t date,bool replay=false);
    bool choose_daily(uint32_t date,bool replay=false,const char* prefix="ultrapool");
    bool continue_daily(const char* prefix="ultrapool");
    bool flush_daily(const char* prefix="ultrapool");
    bool seeded_run=false,endless=false;
    TutorialState tutorial;
    bool profile_open=false;unsigned profile_tab=0,profile_selection=0;
    unsigned achievement_toast=achievement_count;float achievement_toast_time=0;
    unsigned round_pocketed=0,sevens_in_a_row=0;float progress_timer=0;
    void progress_track(const char* key,int64_t value=1,bool maximum=false,bool per_round=false);
    void progress_snapshot();
    void progress_increment(const char* key);
    void progress_pocket(const Ball& ball,int scored);
    void enter_shop();
    bool continue_endless();
    bool trial=false, last_round_won=false;
    float ui_time=0,ui_rotation=0;
    bool menu_preview_checked=false,continue_available=false;
    int continue_round=0,continue_money=0;std::array<OwnedBall,3> continue_balls{};
    bool offer_details=false;
    bool roster_details=false;bool passive_details=false;unsigned passive_selection=0;
    unsigned selection=0, inspected=0, inventory_selection=0, rerolls=0, shop_action=0;
    ShopFocus shop_focus=ShopFocus::Market;
    bool merge_pending=false, sell_pending=false, reorder_pending=false;
    unsigned reorder_source=0;
    bool owned_merge_pending=false;
    bool combine_owned(unsigned source,unsigned destination);
    bool cap_rewarded=false, chocolate_used=false, cereal_used=false, end_effects_done=false;
    bool snack_bar=false, cocktail_bar=false, mixing_pending=false, save_pending=false;
    unsigned difficulty=0, snack_tickets=0, cocktail_tickets=0;
    bool mix_owned(unsigned first,unsigned second);
    bool can_mix(unsigned first,unsigned second) const;
    std::array<uint8_t,4> snacks{};
    std::array<int,4> snack_counts{};
    bool deploys_slot(unsigned slot) const;
    std::array<uint8_t,3> snack_offers{};
    bool can_merge(unsigned offer,unsigned item) const;
    std::array<OwnedBall,15> inventory;
    std::size_t inventory_count=0;
    std::array<uint16_t,30> round_build_ids{};unsigned round_build_id_count=0;
    void refresh_build_ids();
    std::array<uint16_t,3> offers{};
    std::array<OwnedBall,3> offer_items{};
    OwnedBall offer_item(unsigned index) const;
    OwnedBall& shop_participant(unsigned token);
    std::array<Effect,16> effects;
    char message[80]{}; float message_time=0;
    void notice(const char* text);
    void effect(Vec p,uint32_t color,float radius=30,bool passive=false);
    void effect_line(Vec from,Vec to);
    void begin_round();
    bool finish_round();
    void next_round();
    bool buy(unsigned offer,bool merge=false);
    bool sell(unsigned item);
    bool reroll();
    void stock_shop(bool exclude_previous=false);
    bool move_item(unsigned source,unsigned destination);
    bool has_snack(unsigned id) const;
    bool buy_snack(unsigned offer);
    bool sell_snack(unsigned slot);
    void stock_snacks(bool reseed=false);
    void start_round_effects();
    void end_round_effects();
    void pocket_effect(unsigned source,unsigned pocket);
    void pocket_listeners(unsigned source,unsigned pocket);
    bool save_run(const char* path) const;
    bool load_run(const char* path);
    int reroll_price() const;
    uint32_t slow_frames=0;
    bool load_content(const char* path);
    void reset(bool ability_trial=false);
    void input(const Input& input, float seconds);
    void update(double seconds);
    bool shoot();
    void dispatch();
};
struct Texture {
    uint16_t width=0,height=0;
    uint16_t* pixels=nullptr;
    void* allocation=nullptr;
    Texture() = default;
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    ~Texture();
    void clear();
    std::size_t bytes() const { return std::size_t(width)*height*2; }
};
bool load_texture(const char* filename,Texture& texture);
struct Asset {
    uint16_t width=0,height=0;
    std::array<uint16_t,4096> pixels;
};
// Bounded 64x64 RGBA4444 sprite loader, optional in the geometry prototype.
bool load_sprite(const char* filename, Asset& asset);
bool load_definitions(const char* filename, std::array<Definition,226>& defs, std::size_t& count);
} // namespace up
