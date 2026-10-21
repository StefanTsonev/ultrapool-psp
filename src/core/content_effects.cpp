#include "ultrapool/core.hpp"
#include <algorithm>
#include <limits>
namespace up {
void Game::content_effect(unsigned source,unsigned ability,unsigned level,Vec origin,unsigned other) {
    if(cap_rewarded || source>=world.count)return;
    Ball& receiver=world.balls[source];
    if(ability==BlackHole){spawn_hole(level);return;}
    if(ability==Wormhole){for(unsigned ordinal=0;ordinal<world.pocket_count();++ordinal)queue_pocket_suck(world.pocket_id(ordinal),level,origin);return;}
    if(ability==Robot){set_ball_locked(receiver,false);receiver.chase_pocket_time=.1f*float(level);return;}
    if(ability==Snake){
        set_ball_locked(receiver,false);
        if(receiver.snake_time<=0){float heading=(random()/4294967296.0f)*6.283185307f;receiver.snake_direction=Vec(std::cos(heading),std::sin(heading));}
        receiver.snake_time=.5f*float(level);return;
    }
    if(ability==RedDwarf){if(pockets_after_last_shot==3)buff_ball(source,static_cast<int>(std::ceil(double(receiver.def.base_score)*.2*level)));return;}
    if(ability==Ufo){
        int closest=closest_target(receiver.p,source);
        if(closest>=0){sound("ufo_ball");Vec delta=world.balls[closest].p-receiver.p;spawn_energy(receiver.p,delta.dot(delta)>0?delta.unit()*(300+300*float(level)):Vec(),source);}
        return;
    }
    if(ability==Tornado){
        sound("tornado_ball");
        for(unsigned i=1;i<world.count;++i){const Ball& b=world.balls[i];if(!b.active || b.passive || b.cue || b.energy)continue;
            Vec delta=Vec()-b.p;queue_impulse(i,delta.dot(delta)>0?delta.unit()*(600*(.5f+.2f*float(std::min(5u,unsigned(receiver.level))))):Vec());
        }
        return;
    }
    if(ability==Tesla){
        unsigned choices[max_balls],count=0;
        for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]))choices[count++]=i;
        std::stable_sort(choices,choices+count,[&](unsigned a,unsigned b){Vec da=world.balls[a].p-receiver.p,db=world.balls[b].p-receiver.p;return da.dot(da)<db.dot(db);});
        if(count)sound("tesla_ball");
        // The reference skips element zero, rather than removing the source by identity.
        for(unsigned n=1;n<std::min(count,level+1);++n){unsigned chosen=choices[n];Vec delta=world.balls[chosen].p-receiver.p;queue_impulse(chosen,delta.dot(delta)>0?delta.unit()*150:Vec());effect_line(receiver.p,world.balls[chosen].p);}
        return;
    }
    if(ability==Asteroid){spawn_data(id_of(BallKey::STAR),level,default_item(id_of(BallKey::STAR),level).score,origin,Unsupported);return;}
    if(ability==Comet){
        int nearest=-1;float distance=0;
        for(unsigned i=1;i<world.count;++i)if(i!=source && targetable_ball(world.balls[i])){
            Vec delta=world.balls[i].p-origin;float squared=delta.dot(delta);
            if(nearest<0 || squared<distance){nearest=static_cast<int>(i);distance=squared;}
        }
        if(nearest>=0){Vec delta=world.balls[nearest].p-origin;spawn_energy(origin,delta.dot(delta)>0?delta.unit()*(500+250*float(level)):Vec(),source);}
        return;
    }
    if(ability==Antenna){
        unsigned count=0;for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]))++count;
        for(unsigned n=count;n<2+2*level;++n)spawn_data(id_of(BallKey::ALIEN),level,static_cast<int>(level),origin,Unsupported);
        return;
    }
    if(ability==Conveyor){
        float flow_angle=(random()/4294967296.0f)*6.283185307f;
        Vec direction(std::cos(flow_angle),std::sin(flow_angle));
        for(unsigned i=1;i<world.count;++i){Ball& b=world.balls[i];if(!b.active || b.passive || b.cue || b.energy)continue;
            set_ball_locked(b,false);b.conveyor_time=.2f*float(level);b.conveyor_direction=direction;
        }
        return;
    }
    if(ability==Alien || ability==Wheel){
        unsigned choices[max_balls],count=0;
        for(unsigned i=1;i<world.count;++i)if(i!=source && targetable_ball(world.balls[i]) && (ability==Wheel || has_tag(world.balls[i],Tag::TAG_FRIEND)))choices[count++]=i;
        if(ability==Alien){if(count)queue_ball_modifier(choices[random()%count],Modifier::Nudge,static_cast<int>(level),receiver.p,.5f*float(level));}
        else {
            for(unsigned n=count;n>1;--n)std::swap(choices[n-1],choices[random()%n]);
            for(unsigned n=0;n<std::min(level,count);++n)queue_ball_modifier(choices[n],Modifier::Shoot,1,origin,1,true);
        }
        return;
    }
    if(ability==Panellet){
        buff_ball(source,static_cast<int>(level));
        unsigned status=random()%3;
        set_ball_locked(receiver,status==0);
        receiver.flaming=status==1;
        set_ball_shield(receiver,status==2);
        const Weight weights[]={Weight::Normal,Weight::Heavy,Weight::Light};
        set_ball_weight(receiver,weights[random()%3]);
        return;
    }
    if(ability==Rainbow){
        // Original uniqueness is an ordered primary/secondary identity pair.
        for(unsigned i=1;i<world.count;++i)if(world.balls[i].active && !world.balls[i].passive && !world.balls[i].energy)
            for(unsigned j=1;j<i;++j)if(world.balls[j].active && !world.balls[j].passive && !world.balls[j].energy && world.balls[i].def.id==world.balls[j].def.id && world.balls[i].mixed_with==world.balls[j].mixed_with)return;
        for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]))queue_ball_buff(i,static_cast<int>(level),origin);
        return;
    }
    if(ability==Onigiri){
        unsigned id=roll_content(id_of(BallKey::ONIGIRI),0,static_cast<uint32_t>(Tag::TAG_FOOD),0,false,0,0);
        if(id)spawn_data(id,level,default_item(id,level).score,origin,Onigiri);
        return;
    }
    if(ability==Friendsphere){
        for(unsigned n=0;n<level && !cap_rewarded;++n){
            unsigned id=roll_content(id_of(BallKey::PET_FOOD),id_of(BallKey::FRIENDSPHERE),static_cast<uint32_t>(Tag::TAG_FRIEND),0,false,id_of(BallKey::TENNIS),-2);
            if(!id)continue;
            Ball friend_ball;friend_ball.def=builtin(id);friend_ball.level=1;friend_ball.def.base_score=default_item(id,1).score;
            if(find_spawn_position(friend_ball.p,friend_ball.def.radius))spawn_snapshot(friend_ball,Friendsphere);
        }
        return;
    }
    if(ability==Log){
        for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]) && (world.balls[i].def.id==id_of(BallKey::LOG) || world.balls[i].mixed_with==id_of(BallKey::LOG)))queue_ball_buff(i,static_cast<int>(2*level),origin);
        return;
    }
    if(ability==Ant){spawn_data(id_of(BallKey::ANT),level,default_item(id_of(BallKey::ANT),level).score,origin,Ant);return;}
    if(ability==Replicator){const OwnedBall& reserve=inventory[10];if(reserve.id && rarity(reserve.id)==0)for(unsigned n=0;n<level+1;++n)spawn_data(reserve.id,reserve.level,reserve.score,origin,Replicator);return;}
    if(ability==Pot){if(other<world.count)buff_ball(source,static_cast<int>(std::ceil(double(level)*world.balls[other].def.base_score/5.0)));return;}
    if(ability==Spirit){
        if(random()%100<10+10*level){int closest=-1;float distance=0;
            for(unsigned ordinal=0;ordinal<world.pocket_count();++ordinal){const Pocket& p=world.pocket_at(ordinal);if(p.closed)continue;Vec delta=p.p-receiver.p;float squared=delta.dot(delta);if(closest<0 || squared<distance){closest=static_cast<int>(world.pocket_id(ordinal));distance=squared;}}
            if(closest>=0 && distance>0)queue_impulse(source,(world.pocket(static_cast<unsigned>(closest))->p-receiver.p).unit()*600);
        }
        return;
    }
    unsigned targets[max_balls],count=0;
    for(unsigned i=1;i<world.count;++i)if(targetable_ball(world.balls[i]))targets[count++]=i;
    if(!count)return;
    if(ability==Peacock){
        std::stable_sort(targets,targets+count,[&](unsigned a,unsigned b){Vec da=world.balls[a].p-receiver.p,db=world.balls[b].p-receiver.p;return da.dot(da)>db.dot(db);});
        for(unsigned n=0;n<std::min(level,count);++n){Vec direction=receiver.p-world.balls[targets[n]].p;queue_impulse(targets[n],direction.dot(direction)>0?direction.unit()*300:Vec());effect_line(receiver.p,world.balls[targets[n]].p);}
        return;
    }
    if(ability==Nest){
        unsigned eggs[max_balls],size=0;for(unsigned n=0;n<count;++n)if(has_ability(world.balls[targets[n]],Egg))eggs[size++]=targets[n];
        for(unsigned n=size;n>1;--n)std::swap(eggs[n-1],eggs[random()%n]);
        for(unsigned n=0;n<std::min(3u,size);++n)queue_ball_buff(eggs[n],static_cast<int>(level),origin,false,false,false,true);
        return;
    }
    if(ability==Turbine){
        unsigned choices[max_balls],size=0;for(unsigned n=0;n<count;++n)if(targets[n]!=source)choices[size++]=targets[n];
        if(size)queue_ball_modifier(choices[random()%size],Modifier::Shoot,static_cast<int>(level),origin,.5f,true);
        return;
    }
    if(ability==ChefHat){
        for(unsigned n=0;n<level;++n){
            unsigned target_index=targets[random()%count];
            switch(random()%3){
            case 0:queue_ball_buff(target_index,1,origin);break;
            case 1:queue_ball_modifier(target_index,Modifier::Shield,0,origin);break;
            case 2:queue_ball_modifier(target_index,Modifier::Flame,0,origin);break;
            }
        }
    }
    if(ability==Meatball){
        const int64_t scaled=static_cast<int64_t>(receiver.def.base_score)*level;
        const int total=static_cast<int>(std::max<int64_t>(0,std::min<int64_t>((scaled+1)/2,std::numeric_limits<int>::max())));
        unsigned cuts[max_balls+1];cuts[0]=0;cuts[count]=static_cast<unsigned>(total);
        for(unsigned n=1;n<count;++n)cuts[n]=random()%(static_cast<uint32_t>(total)+1u);
        std::sort(cuts,cuts+count+1);
        for(unsigned n=0;n<count;++n){int share=static_cast<int>(cuts[n+1]-cuts[n]);if(share>0)queue_ball_buff(targets[n],share,origin);}
    }
}
}
