#include "ultrapool/core.hpp"
#include <limits>
#include <algorithm>
namespace up {
static void predict_travel(const World& w, Vec origin, Vec velocity, float radius,
                           int excluded, bool cue_ball, bool pocketable,
                           std::array<Vec,4>& points, unsigned& count) {
    const float speed=velocity.length();
    if(speed<1.0f) return;
    Vec d=velocity*(1.0f/speed);
    float remaining=std::min(600.0f, speed*0.15f);
    Vec from=origin+d*(radius+0.5f);
    points[count++]=from;
    for(unsigned leg=0;leg<3 && remaining>0.1f;++leg) {
        float distance=remaining; int axis=-1, hit=-1, pocket_hit=-1;
        for(unsigned k=0;k<2;++k) {
            float component=k?d.y:d.x, coordinate=k?from.y:from.x;
            if(std::fabs(component)<1e-7f) continue;
            float edge=component>0?(k?w.maximum.y:w.maximum.x)-radius:
                                      (k?w.minimum.y:w.minimum.x)+radius;
            float t=(edge-coordinate)/component;
            if(t>0.01f && t<distance) { distance=t; axis=int(k); hit=-1; }
        }
        if(w.pockets_enabled && pocketable) for(unsigned ordinal=0;ordinal<w.pocket_count();++ordinal) {
            const Pocket& pocket=w.pocket_at(ordinal);
            if(pocket.closed || pocket.scale<=0) continue;
            Vec relative=from-pocket.p;
            float pocket_capture=((cue_ball?11.0f:pocket.radius*pocket.suck_scale)*pocket.scale)+radius;
            float along=relative.dot(d);
            float c=relative.dot(relative)-pocket_capture*pocket_capture;
            float t=0;
            if(c>0) {
                float disc=along*along-c;
                if(disc<0) continue;
                t=-along-std::sqrt(disc);
                if(t<=0.01f) continue;
            }
            if(t<distance) { distance=t; axis=-1; hit=-1; pocket_hit=int(ordinal); }
        }
        for(std::size_t i=1;i<w.count;++i) {
            if(int(i)==excluded) continue;
            const Ball& b=w.balls[i];
            if(!b.active || b.passive || b.falling || b.energy || b.collision_disabled) continue;
            Vec relative=from-b.p; float along=relative.dot(d);
            float combined=radius+b.def.radius;
            float disc=along*along-relative.dot(relative)+combined*combined;
            if(disc<0) continue;
            float t=-along-std::sqrt(disc);
            if(t>0.01f && t<distance) { distance=t; hit=int(i); axis=-1; }
        }
        Vec end=from+d*distance;
        if(count<points.size()) points[count++]=end;
        remaining-=distance;
        if(hit>=0 || pocket_hit>=0 || axis<0) break;
        if(axis==0) d.x=-d.x; else d.y=-d.y;
        from=end+d*0.01f;
    }
}
AimPrediction predict_aim(const World& w,Vec direction,float power) {
    AimPrediction a;
    if(!w.count || !w.balls[0].active || direction.dot(direction)<1e-10f) return a;
    Vec d=direction.unit(),from=w.balls[0].p;
    const Ball& cue=w.balls[0];
    a.points[a.count++]=from;
    for(unsigned leg=0;leg<2;++leg) {
        float distance=std::numeric_limits<float>::infinity(); int hit=-1,axis=-1;
        for(unsigned k=0;k<2;++k) {
            float component=k?d.y:d.x,origin=k?from.y:from.x;
            if(std::fabs(component)<1e-7f) continue;
            float edge=component>0?(k?w.maximum.y:w.maximum.x)-cue.def.radius:
                (k?w.minimum.y:w.minimum.x)+cue.def.radius;
            float t=(edge-origin)/component;
            if(t>0.0001f && t<distance) { distance=t; axis=int(k); }
        }
        for(std::size_t i=1;i<w.count;++i) {
            const Ball& b=w.balls[i]; if(!b.active || b.passive || b.falling || b.energy) continue;
            Vec relative=from-b.p; float along=relative.dot(d);
            float radius=cue.def.radius+b.def.radius;
            float discriminant=along*along-relative.dot(relative)+radius*radius;
            if(discriminant<0) continue;
            float t=-along-std::sqrt(discriminant);
            if(t>0.0001f && t<distance) { distance=t; hit=int(i); }
        }
        if(!std::isfinite(distance)) break;
        Vec contact=from+d*distance; a.points[a.count++]=contact;
        if(hit>=0) {
            a.hit=hit; Vec n=(w.balls[hit].p-contact).unit();
            Vec velocity=d*(2500*std::max(0.0f,std::min(power,1.0f))/cue.def.mass);
            float inv1=1/cue.def.mass,inv2=1/w.balls[hit].def.mass;
            float impulse=2*std::max(0.0f,velocity.dot(n))/(inv1+inv2);
            a.cue_velocity=velocity-n*(impulse*inv1); a.object_velocity=n*(impulse*inv2);
            predict_travel(w,contact,a.cue_velocity,cue.def.radius,hit,
                           cue.cue,physical_pocketable(cue),a.travel[0],a.travel_count[0]);
            predict_travel(w,w.balls[hit].p,a.object_velocity,w.balls[hit].def.radius,hit,
                           w.balls[hit].cue,physical_pocketable(w.balls[hit]),a.travel[1],a.travel_count[1]);
            break;
        }
        if(axis==0) d.x=-d.x; else d.y=-d.y;
        from=contact+d*0.01f;
    }
    return a;
}
}
