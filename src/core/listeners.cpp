#include "ultrapool/core.hpp"
namespace up {
bool SemanticQueue::push(const SemanticEvent &event) {
  events.push_back(event);
  if (events.size() > high_water)
    high_water = static_cast<unsigned>(events.size());
  return true;
}
bool SemanticQueue::pop(SemanticEvent &event) {
  if (events.empty())
    return false;
  event = events.front();
  events.pop_front();
  return true;
}
void ListenerRegistry::add(EntityRef ref, uint32_t mask) {
  for (unsigned event = 0; event < signal_count; ++event)
    if (mask & (uint32_t(1) << event)) {
      auto &list = lists[event];
      bool duplicate = false;
      for (unsigned i = 0; i < list.count; ++i)
        duplicate |= list.entities[i] == ref;
      if (duplicate)
        continue;
      if (list.count == list.entities.size()) {
        ++overflows;
        continue;
      }
      list.entities[list.count++] = ref;
    }
}
void ListenerRegistry::remove(EntityRef ref) {
  for (auto &list : lists)
    for (unsigned i = 0; i < list.count; ++i)
      if (list.entities[i] == ref) {
        list.entities[i] = list.entities[--list.count];
        list.entities[list.count] = EntityRef();
        break;
      }
}
namespace {
EntityRef reference(EntityKind kind, unsigned index, uint32_t generation) {
  EntityRef ref;
  ref.kind = kind;
  ref.index = static_cast<uint16_t>(index);
  ref.generation = generation;
  return ref;
}
} // namespace
namespace {
uint32_t snack_mask(unsigned id) { return snack_info(id).events; }
} // namespace
void Game::sync_listeners() {
  // Curse cubes precede snacks and physical balls in original setup order.
  for(unsigned i=0;i<cubes.size();++i) if(cubes[i].id) {
    const auto& cube=cubes[i];
    listeners.add(reference(EntityKind::Cube,i,cube.id),ball_info(cube.id).events);
  }
  // Original round setup registers passives before physical balls.
  for (unsigned i = 0; i < snacks.size(); ++i)
    if (bound_snacks[i] != snacks[i]) {
      listeners.remove(reference(EntityKind::Snack, i, snack_generations[i]));
      ++snack_generations[i];
      if (!snack_generations[i])
        ++snack_generations[i];
      bound_snacks[i] = snacks[i];profile.see_snack(snacks[i]);
      snack_counts[i] = 0;
      if (snacks[i])
        listeners.add(reference(EntityKind::Snack, i, snack_generations[i]),
                      snack_mask(snacks[i]));
    }
  for (unsigned i = 0; i < max_balls; ++i) {
    auto &bound = bound_balls[i];
    if (i >= world.count) {
      if (bound.registered)
        listeners.remove(reference(EntityKind::Ball, i, bound.generation));
      bound = ListenerBinding();
      continue;
    }
    const Ball &b = world.balls[i];if(!b.cue){profile.see_ball(b.def.id);if(b.mixed_with)profile.see_ball(b.mixed_with);}
    uint32_t mask =
        (b.cue || b.energy)
            ? 0
            : ability_signals(component_ability(b, 0)) |
                  (b.mixed_with ? ability_signals(component_ability(b, 1)) : 0);
    bool primary_changed =
        bound.primary != b.def.id &&
        !(number_ball(bound.primary) && number_ball(b.def.id));
    bool secondary_changed =
        bound.secondary != b.mixed_with &&
        !(bound.secondary && b.mixed_with && number_ball(bound.secondary) &&
          number_ball(b.mixed_with));
    if (bound.generation != b.generation || primary_changed ||
        secondary_changed || bound.mask != mask) {
      if (bound.registered)
        listeners.remove(reference(EntityKind::Ball, i, bound.generation));
      bound.registered = false;
    }
    bound.generation = b.generation;
    bound.primary = b.def.id;
    bound.secondary = b.mixed_with;
    bound.mask = mask;
    if (b.active && !b.energy && !bound.registered) {
      listeners.add(reference(EntityKind::Ball, i, b.generation), mask);
      bound.registered = true;
    }
  }
}
void Game::retire_listener(unsigned index) {
  if (index < world.count) {
    listeners.remove(
        reference(EntityKind::Ball, index, world.balls[index].generation));
    bound_balls[index].registered = false;
  }
}
bool Game::live_listener(EntityRef ref) const {
  if (ref.kind == EntityKind::Ball)
    return ref.index < world.count &&
           world.balls[ref.index].generation == ref.generation &&
           world.balls[ref.index].active && !world.balls[ref.index].cue && !world.balls[ref.index].energy;
  if(ref.kind==EntityKind::Cube)
    return ref.index<cubes.size() && cubes[ref.index].id==ref.generation;
  if (ref.kind == EntityKind::Snack)
    return ref.index < snacks.size() && snacks[ref.index] &&
           snack_generations[ref.index] == ref.generation;
  return false;
}
} // namespace up
