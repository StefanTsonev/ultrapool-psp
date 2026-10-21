#include "ultrapool/core.hpp"
#include <algorithm>
#include <limits>
namespace up {
namespace {
struct CollectionSnapshot {
    World& world;EntityRef refs[max_balls];unsigned count=0;
    explicit CollectionSnapshot(World& w):world(w){
        for(unsigned i=1;i<w.count;++i){const Ball& b=w.balls[i];
            if(b.cue || b.energy || b.passive || (!b.active && b.last_pocket_tick!=w.metrics.ticks))continue;
            EntityRef ref;ref.kind=EntityKind::Ball;ref.index=static_cast<uint16_t>(i);ref.generation=b.generation;refs[count++]=ref;w.retain(i);
        }
    }
    ~CollectionSnapshot(){for(unsigned i=0;i<count;++i)world.release(refs[i].index);}
};
}
void Game::collection_effect(unsigned source,unsigned ability,unsigned level,Vec origin){
    if(cap_rewarded || source>=world.count)return;
    if(ability==Lich){
        unsigned id=random_pocket_id();const Pocket* pocket=world.pocket(id);if(!pocket)return;
        Ball snapshot=world.balls[source];snapshot.owner=-1;snapshot.def.base_score=static_cast<int>(std::ceil(double(snapshot.def.base_score)*level/5.0));snapshot.base_value=snapshot.def.base_score;snapshot.temp_value=0;
        Action& a=allocate_action();start_wisp(a,ActionKind::PocketChange,origin,pocket->p);a.target=id;a.flags=64;a.snapshot=snapshot;return;
    }
    if(ability==Trash){
        CollectionSnapshot snapshot(world);long long total=0;
        for(unsigned n=0;n<snapshot.count && !cap_rewarded;++n){const EntityRef& ref=snapshot.refs[n];if(world.balls[ref.index].generation!=ref.generation)continue;
            const Ball& scrap=world.balls[ref.index];if(scrap.def.id!=id_of(BallKey::SCRAP) && scrap.mixed_with!=id_of(BallKey::SCRAP))continue;
            total+=scrap.def.base_score;consume_ball(ref.index);
        }
        if(total>0 && !cap_rewarded)spawn_data(id_of(BallKey::SCRAP),level,static_cast<int>(std::min(total,static_cast<long long>(std::numeric_limits<int>::max()))),origin,Unsupported);
    }
}
void Game::release_held(unsigned id){
    Pocket* p=world.pocket(id);if(!p || !p->held_count)return;
    for(auto it=haunts.begin();it!=haunts.end();){
        if(it->pocket!=id){++it;continue;}
        Ball stored=it->snapshot;it=haunts.erase(it);if(p->held_count)--p->held_count;
        queue_spawn(stored,p->p,Unsupported,false,-1,false,true);
    }
}
}
