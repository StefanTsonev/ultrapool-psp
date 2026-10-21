#pragma once
#include "ultrapool/tutorial_catalog.hpp"
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cmath>
namespace up {
struct TutorialState {
    bool enabled=false,announced=false;
    uint64_t seen=0;
    unsigned active=tutorial_count;
    float elapsed=0,menu_time=0;
    bool showing()const{return active<tutorial_count && elapsed>=tutorial_rules[active].delay;}
    bool ready()const{return showing() && elapsed>=tutorial_rules[active].delay+tutorial_rules[active].duration;}
    bool trigger(const char* id,uint64_t completed){
        if(!enabled || active<tutorial_count)return false;
        for(unsigned i=0;i<tutorial_count;++i)if(!std::strcmp(id,tutorial_rules[i].id)){
            const auto& rule=tutorial_rules[i];
            if(!rule.manual && ((seen&(uint64_t(1)<<i)) || (completed&(uint64_t(1)<<rule.group))))return false;
            active=i;elapsed=0;announced=false;return true;
        }return false;
    }
    void update(float dt){if(enabled && active<tutorial_count && std::isfinite(dt) && dt>=0)elapsed+=dt;}
    bool dismiss(uint64_t& completed){
        if(!ready())return false;
        const unsigned previous=active;const auto& rule=tutorial_rules[previous];
        if(!rule.manual){seen|=uint64_t(1)<<previous;if(rule.complete)completed|=uint64_t(1)<<rule.group;}
        active=tutorial_count;elapsed=0;
        for(unsigned i=0;i<tutorial_count;++i)if(!std::strcmp(tutorial_rules[i].condition,rule.id) && tutorial_rules[i].manual==rule.manual){trigger(tutorial_rules[i].id,completed);break;}
        return true;
    }
};
}
