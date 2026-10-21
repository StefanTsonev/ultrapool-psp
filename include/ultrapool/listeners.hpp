#pragma once
#include <array>
#include <deque>
#include <cstdint>
namespace up {
enum class Signal : uint8_t { Score, PocketAnother, RoundStart, RoundEnd, Shot, Spawn, SpawnAnother, Hit, Wall, Buff, Transform, TransformAnother, ScoreChange, ReachScore, MaxRoll, Pickup, EnterShop, Upgrade, SellAnother, Reroll, SellSelf, Buy, BuyAnother, Pocket, Count };
enum class EntityKind : uint8_t { Ball, Snack, Cube, Shop };
struct EntityRef {
    uint32_t generation=0; uint16_t index=0; EntityKind kind=EntityKind::Ball;
    bool operator==(const EntityRef& b) const { return generation==b.generation && index==b.index && kind==b.kind; }
};
constexpr unsigned signal_count=static_cast<unsigned>(Signal::Count);
constexpr uint32_t signal_bit(Signal signal) { return uint32_t(1)<<static_cast<unsigned>(signal); }
struct ListenerSnapshot { std::array<EntityRef,30> entities{}; unsigned count=0; };
class ListenerRegistry {
    std::array<ListenerSnapshot,signal_count> lists{};
public:
    unsigned overflows=0;
    void add(EntityRef ref,uint32_t mask);
    void remove(EntityRef ref);
    ListenerSnapshot snapshot(Signal signal) const { return lists[static_cast<unsigned>(signal)]; }
    void clear() { for(auto& list:lists) list.count=0; overflows=0; }
};
struct SemanticEvent { Signal signal=Signal::Score; EntityRef source,target; int amount=0,extra=0; uint32_t flags=0; };
class SemanticQueue {
    std::deque<SemanticEvent> events;
public:
    unsigned overflows=0,high_water=0;
    bool push(const SemanticEvent& event);
    bool pop(SemanticEvent& event);
    bool empty() const { return events.empty(); }
    void clear() { events.clear(); }
};
uint32_t ability_signals(unsigned ability);
// Direct callbacks can reach handlers absent from the global subscription list.
uint32_t ability_handlers(unsigned ability);
struct ListenerBinding { uint32_t generation=0,mask=0; uint16_t primary=0,secondary=0; bool registered=false; };
}
