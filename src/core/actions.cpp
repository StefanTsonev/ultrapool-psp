#include "ultrapool/core.hpp"
#include <algorithm>
namespace up {
Action &Game::allocate_action() {
  for (auto &a : actions)
    if (a.kind == ActionKind::None) {
      a = Action();
      a.serial = ++action_serial;
      return a;
    }
  actions.emplace_back();
  Action &a = actions.back();
  a.serial = ++action_serial;
  action_high_water =
      std::max(action_high_water, static_cast<unsigned>(actions.size()));
  return a;
}
bool Game::pending_actions() const {
  if (!semantics.empty() || (!cap_rewarded && pickup_holds_pending()))
    return true;
  if(!cap_rewarded)for(unsigned i=1;i<world.count;++i)if(world.balls[i].active && !world.balls[i].passive && !world.balls[i].falling && (world.balls[i].chase_flowers>0 || world.balls[i].rocket_time>0 || world.balls[i].conveyor_time>0 || world.balls[i].snake_time>0 || world.balls[i].chase_pocket_time>0 || world.balls[i].pull_time>0 || world.balls[i].pulled))return true;
  for (const auto &a : actions)
    if (a.kind != ActionKind::None)
      return true;
  return false;
}
void Game::start_wisp(Action &a, ActionKind kind, Vec origin, Vec destination) {
  sound("buff_spawn");
  a.kind = kind;
  a.p = origin;
  a.destination = destination;
  a.due = world.metrics.ticks;
  a.velocity = Vec(-100 + 200 * (random() / 4294967296.0f), -300);
}
bool Game::queue_ball_pocket(unsigned target_index, int multiplier,
                             Vec origin) {
  if (cap_rewarded || target_index >= world.count ||
      !world.balls[target_index].active || world.balls[target_index].cue)
    return false;
  {
    Action &a = allocate_action();
    start_wisp(a, ActionKind::PocketBall, origin, world.balls[target_index].p);
    a.target = target_index;
    a.generation = world.balls[target_index].generation;
    a.multiplier = static_cast<float>(multiplier);
    return true;
  }
}
bool Game::queue_ball_buff(unsigned target_index, int amount, Vec origin,
                           bool permanent, bool is_moss, bool retarget, bool hatch) {
  if (cap_rewarded || target_index >= world.count ||
      !world.balls[target_index].active || world.balls[target_index].cue)
    return false;
  {
    Action &a = allocate_action();
    start_wisp(a, ActionKind::BuffBall, origin, world.balls[target_index].p);
    a.target = target_index;
    a.generation = world.balls[target_index].generation;
    a.bonus = amount;
    a.flags =
        (permanent ? 1u : 0u) | (is_moss ? 2u : 0u) | (retarget ? 4u : 0u) | (hatch ? 8u : 0u);
    return true;
  }
}
bool Game::queue_pocket_change(unsigned pocket, float multiplier, int bonus,
                               Vec origin) {
  if (cap_rewarded || !world.pocket(pocket))
    return false;
  {
    Action &a = allocate_action();
    start_wisp(a, ActionKind::PocketChange, origin, world.pocket(pocket)->p);
    a.target = pocket;
    a.multiplier = multiplier;
    a.bonus = bonus;
    return true;
  }
}
bool Game::queue_repeated_pocket(unsigned count, float multiplier, int bonus,
                                 Vec origin) {
  if (!count || cap_rewarded)
    return false;
  bool first = queue_pocket_change(random_pocket_id(), multiplier, bonus, origin);
  if (count == 1)
    return first;
  {
    Action &a = allocate_action();
    a.kind = ActionKind::RepeatPocket;
    a.p = origin;
    a.repeats = count - 1;
    a.multiplier = multiplier;
    a.bonus = bonus;
    a.due = world.metrics.ticks + 18;
    return first;
  }
}
bool Game::queue_impulse(unsigned target_index, Vec impulse) {
  if (cap_rewarded || target_index >= world.count ||
      !world.balls[target_index].active || world.balls[target_index].passive)
    return false;
  set_ball_locked(world.balls[target_index], false);
  {
    Action &a = allocate_action();
    a.kind = ActionKind::Impulse;
    a.target = target_index;
    a.generation = world.balls[target_index].generation;
    a.impulse = impulse;
    a.due = world.metrics.ticks + 2;
    return true;
  }
}
bool Game::find_spawn_position(Vec &destination, float radius) {
  bool found = false;
  for (unsigned attempt = 0; attempt < 1024 && !found; ++attempt) {
    destination = Vec(-208 + 416 * (random() / 4294967296.0f),
                      -356 + 716 * (random() / 4294967296.0f));
    found = world.valid_placement(destination, radius, max_balls);
    for (std::size_t i = 0; i < world.count && found; ++i)
      if (world.balls[i].active && !world.balls[i].passive && !world.balls[i].energy &&
          (world.balls[i].p - destination).length() < 40)
        found = false;
  }
  if (!found) {
    ++spawn_rejections;
    notice("No free spawn position");
  }
  return found;
}
bool Game::queue_spawn(const Ball &snapshot, Vec origin, unsigned source, bool preserve_owner,int post_weight,bool spawn_locked,bool preserve_snapshot_lock) {
  if (cap_rewarded)
    return false;
  Vec destination;
  if (!find_spawn_position(destination, snapshot.def.radius))
    return false;
  {
    Action &a = allocate_action();
    start_wisp(a, ActionKind::Spawn, origin, destination);
    a.snapshot = snapshot;
    a.flags=(preserve_owner?1u:0u)|(spawn_locked?2u:0u)|(preserve_snapshot_lock?8u:0u)|((post_weight>=0 && post_weight<=3)?(4u|(static_cast<unsigned>(post_weight)<<8)):0u);
    a.target = source;
    return true;
  }
}
bool Game::spawn_snapshot(Ball b, unsigned source, bool identity_prepared, bool preserve_owner, bool setup,bool restore_snapshot_lock,unsigned* spawned_index) {
  unsigned active = 0;
  for (std::size_t i = 1; i < world.count; ++i)
    if (targetable_ball(world.balls[i]))
      ++active;
  if (active >= 30) {
    ++spawn_rejections;
    return false;
  }
  b.base_value = b.def.base_score - b.temp_value;
  if (!identity_prepared && number_ball(b.def.id)) {
    b.def.id = static_cast<uint16_t>(number_id(b.base_value));
    if (b.mixed_with == b.def.id)
      b.mixed_with = 0;
    b.def.ability = builtin(b.def.id).ability;
    b.def.color = builtin(b.def.id).color;
  }
  b.passive=false;b.collision_disabled=false;
  b.active = true;
  b.gone = false;
  b.falling = false;
  b.fall_remaining = 0;
  b.cue = false;
  b.energy=false;b.energy_age=0;b.collision_exception=max_balls;b.exception_generation=0;
  if(!preserve_owner)b.owner = -1;
  b.v = Vec();
  b.chase_flowers=0;b.rocket_time=0;b.conveyor_time=0;b.conveyor_direction=Vec();b.snake_time=0;b.chase_pocket_time=0;b.snake_direction=Vec();
  b.parrot_used=false;b.ghost_used=false;b.pull_time=0;b.pulled=false;b.last_contact=max_balls;b.last_contact_generation=0;
  b.flash = 0.5f;
  b.last_pocket_tick = 0xffffffffu;
  bool stored_lock=restore_snapshot_lock && b.locked;
  set_ball_locked(b, false);
  refresh_initial_weight(b);
  if(stored_lock)set_ball_locked(b,true);
  if (!world.add(b)) {
    ++spawn_rejections;
    return false;
  }
  sound("ball_spawn");
  sync_listeners();
  effect(b.p, 0xff8fd8a2);
  const unsigned spawned=world.last_added;
  if(spawned_index)*spawned_index=spawned;
  world.retain(spawned);
  if(!setup)emit_semantic(Signal::SpawnAnother, 0, spawned);
  spawn_effects(spawned, source);
  world.release(spawned);
  return true;
}
void Game::update_actions() {
  // Work on an entry snapshot: actions created by completion start next tick.
  const uint64_t cutoff = action_serial;
  const unsigned entry_count = static_cast<unsigned>(actions.size());
  for (unsigned i = 0; i < entry_count && i < actions.size() && !cap_rewarded;
       ++i)
    if (actions[i].kind != ActionKind::None && actions[i].serial <= cutoff) {
      Action &a = actions[i];
      if (a.kind == ActionKind::RepeatEffect) {
        if (world.metrics.ticks < a.due)
          continue;
        EffectCall call = a.call;
        a.kind = ActionKind::None;
        run_effect(call);
      } else if(a.kind==ActionKind::Ritual){
        if(world.metrics.ticks<a.due)continue;
        if(!ritual_done){ritual_done=true;ritual_flash=1;progress_track("RITUAL_STRENGTH",ritual_strength,true);progress_track("TOTAL_RITUALS_PERFORMED");}
        if(a.repeats){ritual_nudge();--a.repeats;a.due=world.metrics.ticks+36;}
        else {completed_ritual_strength=ritual_strength;a.kind=ActionKind::None;}
      } else if (a.kind == ActionKind::Impulse) {
        if (world.metrics.ticks < a.due)
          continue;
        if (a.target < world.count) {
          Ball &b = world.balls[a.target];
          if (b.generation == a.generation && b.active)
            b.v += a.impulse * (1 / (b.falling ? 1.0f : b.def.mass));
        }
        a.kind = ActionKind::None;
      } else if(a.kind==ActionKind::RepeatBuff) {
        if(world.metrics.ticks<a.due)continue;
        unsigned exclude=a.target<world.count && world.balls[a.target].generation==a.generation?a.target:max_balls;Vec origin=a.p;
        if(--a.repeats)a.due+=36;else a.kind=ActionKind::None;
        buff_random_other(exclude,1,origin);
      } else if(a.kind==ActionKind::Wait) {
        if(world.metrics.ticks>=a.due)a.kind=ActionKind::None;
      } else if(a.kind==ActionKind::SetupBuild) {
        if(world.metrics.ticks<a.due)continue;
        if(a.flags){a.kind=ActionKind::None;continue;}
        while(a.target<inventory.size() && (!deploys_slot(a.target) || !inventory[a.target].id))++a.target;
        if(a.target>=inventory.size()){a.flags=1;a.due=world.metrics.ticks+18;continue;}
        const unsigned slot=a.target++;const OwnedBall owned=inventory[slot];a.due=world.metrics.ticks+9;
        Ball ball;ball.def=builtin(owned.id);ball.def.base_score=owned.score;ball.level=owned.level;ball.mixed_with=owned.mixed_with;ball.owner=static_cast<int8_t>(slot);ball.p=build_slot_position(slot);
        bool more=false;for(unsigned next_slot=a.target;next_slot<inventory.size();++next_slot)more|=deploys_slot(next_slot) && inventory[next_slot].id;
        if(!more){a.flags=1;a.due=world.metrics.ticks+18;}
        spawn_snapshot(ball,Unsupported,false,true,true);
      } else if(a.kind==ActionKind::Tiger) {
        if(world.metrics.ticks<a.due)continue;
        if(a.flags){a.kind=ActionKind::None;continue;}
        const float strength=static_cast<float>(a.multiplier);
        a.flags=1;a.due+=a.repeats;
        for(unsigned ball_index=1;ball_index<world.count;++ball_index){
          const Ball& ball=world.balls[ball_index];if(!ball.active || ball.passive || ball.falling || ball.energy)continue;
          float direction=(random()/4294967296.0f)*6.283185307f;
          queue_impulse(ball_index,Vec(std::cos(direction),std::sin(direction))*(600*strength));
        }
      } else if(a.kind==ActionKind::RepeatBoom) {
        if(world.metrics.ticks<a.due)continue;
        Vec origin=a.p;
        if(--a.repeats)a.due+=18;else a.kind=ActionKind::None;
        queue_random_boom(origin);
      } else if (a.kind == ActionKind::RepeatPocket) {
        if (world.metrics.ticks < a.due)
          continue;
        // Select the next target when the timer fires, not when the series
        // starts.
        Vec origin = a.p;
        float multiplier = static_cast<float>(a.multiplier);
        int bonus = a.bonus;
        --a.repeats;
        if (a.repeats)
          a.due += 18;
        else
          a.kind = ActionKind::None;
        queue_pocket_change(random_pocket_id(), multiplier, bonus, origin);
      } else if (a.kind == ActionKind::Boom || a.kind == ActionKind::Spawn ||
                 a.kind == ActionKind::PocketChange ||
                 a.kind == ActionKind::BuffBall ||
                 a.kind == ActionKind::PocketBall ||
                 a.kind == ActionKind::TransformBall ||
                 a.kind == ActionKind::ModifierBall ||
                 a.kind == ActionKind::SplitBall) {
        a.age += tick_dt * 1.25f;
        a.velocity = a.velocity * std::pow(0.9f, tick_dt * 60);
        bool arrived = false;
        if (a.homing) {
          if ((a.kind == ActionKind::BuffBall ||
               a.kind == ActionKind::PocketBall ||
               a.kind == ActionKind::TransformBall ||
               a.kind == ActionKind::ModifierBall ||
               a.kind == ActionKind::SplitBall)) {
            if (a.target >= world.count ||
                world.balls[a.target].generation != a.generation ||
                !world.balls[a.target].active) {
              if ((a.kind == ActionKind::BuffBall && (a.flags & 4)) || (a.kind == ActionKind::ModifierBall && (a.flags & 0x80000000u))) {
                unsigned options[max_balls], count = 0;
                for (unsigned j = 1; j < world.count; ++j)
                  if (world.balls[j].active && !world.balls[j].passive && !world.balls[j].energy && !world.balls[j].falling &&
                      world.balls[j].def.id != id_of(BallKey::IMP) &&
                      world.balls[j].def.id != id_of(BallKey::TUNGSTEN_BALL) &&
                      world.balls[j].mixed_with != id_of(BallKey::IMP) &&
                      world.balls[j].mixed_with !=
                          id_of(BallKey::TUNGSTEN_BALL))
                    options[count++] = j;
                if (count) {
                  a.target = options[random() % count];
                  a.generation = world.balls[a.target].generation;
                } else {
                  a.kind = ActionKind::None;
                  continue;
                }
              } else {
                a.kind = ActionKind::None;
                continue;
              }
            }
            a.destination = world.balls[a.target].p;
          }
          a.homing_time += tick_dt;
          Vec delta = a.destination - a.p;
          a.velocity += delta.unit() * (9000 * 1.25f * tick_dt);
          arrived = delta.length() < (a.velocity * tick_dt).length();
        } else {
          // Original spawning stage integrates drift twice per frame.
          a.p += a.velocity * tick_dt;
          if (a.age > 0.6f)
            a.homing = true;
        }
        if (arrived) {
          Action completed = a;
          a.kind = ActionKind::None;
          if(completed.kind==ActionKind::Boom){blast_at(completed.destination);
          } else if (completed.kind == ActionKind::Spawn) {
            completed.snapshot.p = completed.destination;
            unsigned index=0;
            if((completed.flags&16) && passive_count>=passive_balls.size())continue;
            if(spawn_snapshot(completed.snapshot,completed.target,false,(completed.flags&1)!=0,false,(completed.flags&8)!=0,&index)){
              Ball& spawned=world.balls[index];
              if(completed.flags&32)set_ball_score(index,completed.bonus);
              if(completed.flags&16)land_passive(index);
              if(completed.flags&2)set_ball_locked(spawned,true);
              if(completed.flags&4)set_ball_weight(spawned,static_cast<Weight>((completed.flags>>8)&3));
            }
          } else if (completed.kind == ActionKind::BuffBall) {
            if (completed.bonus) {
              sound("buff_land");
              buff_ball(completed.target, completed.bonus, true,
                        (completed.flags & 2) != 0);
              if((completed.flags&8) && world.balls[completed.target].def.base_score>=10){
                const Ball& egg=world.balls[completed.target];
                int side=egg.def.id==id_of(BallKey::EGG)?0:egg.mixed_with==id_of(BallKey::EGG)?1:-1;
                if(side>=0){sound("egg_ball");transform_ball(completed.target,id_of(BallKey::CHICKEN),side);}
              }
              const Ball &b = world.balls[completed.target];
              if ((completed.flags & 1) && b.owner >= 0 &&
                  inventory[b.owner].id) {
                inventory[b.owner].score =
                    std::max(1, inventory[b.owner].score + completed.bonus);
                normalize_numbers(inventory[b.owner]);
              }
            }
          } else if (completed.kind == ActionKind::SplitBall) {
            split_ball(completed.target, completed.multiplier,
                       (completed.flags & 1) != 0);
            if (completed.bonus && world.balls[completed.target].generation ==
                                       completed.generation)
              buff_ball(completed.target, completed.bonus);
          } else if (completed.kind == ActionKind::ModifierBall) {
            Ball &b = world.balls[completed.target];
            Modifier modifier = static_cast<Modifier>(completed.flags & 255u);
            if(modifier==Modifier::Reroll)reroll_body(completed.target);
            if(modifier==Modifier::Level){unsigned level=std::min(5u,unsigned(b.level)+1);emit_semantic(Signal::Upgrade,0,completed.target);b.level=static_cast<uint8_t>(level);}
            if (modifier == Modifier::Lock)
              set_ball_locked(b, true);
            if (modifier == Modifier::Unlock)
              set_ball_locked(b, false);
            if (modifier == Modifier::Shield)
              set_ball_shield(b, true);
            if (modifier == Modifier::Heavy)
              set_ball_weight(b, Weight::Heavy);
            if (modifier == Modifier::Light)
              set_ball_weight(b, Weight::Light);
            if (modifier == Modifier::Star)
              b.star_power = true;
            if (modifier == Modifier::Flame)
              b.flaming = true;
            if (modifier == Modifier::PutOut)
              b.flaming = false;
            if (modifier == Modifier::Fleeting)
              b.fleeting = true;
            if (modifier == Modifier::Teleport) {
              Vec position;
              if(find_spawn_position(position,b.def.radius)) {
                world.relocate(completed.target,position);
                clear_pickup_contacts(completed.target);
              }
            }
            if(modifier==Modifier::Nudge || modifier==Modifier::ShieldNudge) {
              int closest=-1;float distance=0;
              for(unsigned ordinal=0;ordinal<world.pocket_count();++ordinal){const Pocket& p=world.pocket_at(ordinal);if(p.closed)continue;Vec delta=p.p-b.p;float length=delta.dot(delta);if(closest<0 || length<distance){closest=static_cast<int>(world.pocket_id(ordinal));distance=length;}}
              if(closest>=0 && distance>0)queue_impulse(completed.target,(world.pocket(static_cast<unsigned>(closest))->p-b.p).unit()*(300*static_cast<float>(completed.multiplier)));
            }
            if(modifier==Modifier::ShieldNudge)set_ball_shield(b,true);
            if (modifier == Modifier::Shoot) {
              float direction = (random() / 4294967296.0f) * 6.283185307f;
              queue_impulse(completed.target,
                            Vec(std::cos(direction), std::sin(direction)) *
                                (900*static_cast<float>(completed.multiplier)));
            }
            if(modifier==Modifier::TransformRat){
              int rat_side=b.def.id==id_of(BallKey::RAT)?0:b.mixed_with==id_of(BallKey::RAT)?1:-1;
              if(rat_side>=0)transform_ball(completed.target,id_of(BallKey::RAT_KING),rat_side);
              else {before_transform(completed.target);retire_listener(completed.target);sync_listeners();b.flash=.5f;emit_semantic(Signal::TransformAnother,0,completed.target);spawn_effects(completed.target,Unsupported);}
            }
            if (completed.bonus){
              if(modifier==Modifier::Reroll)sound("buff_land");
              buff_ball(completed.target, completed.bonus);
              if(modifier==Modifier::Reroll && b.owner>=0 && inventory[b.owner].id){inventory[b.owner].score=std::max(1,inventory[b.owner].score+completed.bonus);normalize_numbers(inventory[b.owner]);}
            }
          } else if (completed.kind == ActionKind::TransformBall) {
            transform_like(completed.target, completed.snapshot);
            if (completed.bonus)
              buff_ball(completed.target, completed.bonus);
          } else if (completed.kind == ActionKind::PocketBall) {
            Ball &b = world.balls[completed.target];
            b.active = false;
            b.falling = false;
            b.v = Vec();
            retire_listener(completed.target);
            world.events.push(EventKind::Pocketed, completed.target, 6,
                              world.metrics.ticks, b.generation, 0,
                              static_cast<int>(completed.multiplier));
          } else if (Pocket* pocket=world.pocket(completed.target)) {
            if(completed.flags&64){HauntedBall held;held.snapshot=completed.snapshot;held.pocket=completed.target;haunts.push_back(held);++pocket->held_count;}
            else if(completed.flags&32)pocket->suck_scale=static_cast<float>(completed.multiplier);
            else {pocket->multiplier+=static_cast<float>(completed.multiplier);pocket->bonus+=completed.bonus;if(completed.flags&16)pocket->shielded=true;}
            effect(pocket->p,0xff66ffff);
          }
        } else {
          a.p += a.velocity * tick_dt;
          if (a.homing_time > 5)
            a.kind = ActionKind::None;
        }
      }
    }
}
Vec Game::passive_origin(unsigned id) const {
  unsigned dense = 0;
  for (auto snack : snacks)
    if (snack) {
      // Canonical table.tscn desktop PivotV, with its -65 gap inversion.
      if (snack == id)
        return Vec(-384, -360 + 65 * float(dense));
      ++dense;
    }
  return Vec(-384, -360);
}
void Game::respawn_cue() {
  if (!world.count)
    return;
  Ball &cue = world.balls[0];
  float radius = std::sqrt(1 + 9999 * (random() / 4294967296.0f));
  float spawn_angle = (random() / 4294967296.0f) * 6.283185307f;
  cue.p =
      cue_start + Vec(std::cos(spawn_angle), std::sin(spawn_angle)) * radius;
  cue.step_start = cue.p;
  cue.motion_tick = world.metrics.ticks;
  cue.just_respawned = true;
  clear_pickup_contacts(0);
  cue.v = Vec();
  cue.active = true;
  cue.falling = false;
  cue.fall_remaining = 0;
}
bool Game::queue_ball_transform(unsigned index, const Ball &source, int buff,
                                Vec origin) {
  if (cap_rewarded || index >= world.count || !world.balls[index].active ||
      world.balls[index].cue)
    return false;
  {
    Action &a = allocate_action();
    start_wisp(a, ActionKind::TransformBall, origin, world.balls[index].p);
    a.target = index;
    a.generation = world.balls[index].generation;
    a.snapshot = source;
    a.bonus = buff;
    return true;
  }
}

} // namespace up
