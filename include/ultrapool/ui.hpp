#pragma once
#include "ultrapool/core.hpp"
#include <cstdio>
namespace up {
inline bool panel_phase(Phase p) { return p==Phase::Menu || p==Phase::Shop || p==Phase::Lost || p==Phase::RunWon; }
struct Panel { char rows[24][78]{}; };
inline Panel panel(const Game& g) {
    Panel p;
    std::snprintf(p.rows[0],78,"ULTRAPOOL NATIVE - GAMEPLAY BUILD 0.2");
    if (g.phase==Phase::Menu) {
        std::snprintf(p.rows[3],78,"%c CLASSIC RUN - original 2-Ball + 1-Ball + 1-Ball",g.selection==0?'>':' ');
        std::snprintf(p.rows[5],78,"%c ABILITY TRIAL - Bomb, Pinball, Bricks, Gold, Smiley, Seal",g.selection==1?'>':' ');
        std::snprintf(p.rows[8],78,"Meet round targets. Buy and merge balls between rounds.");
        std::snprintf(p.rows[10],78,"Trial is a test build, not an original starting deck.");
        std::snprintf(p.rows[13],78,"Up/down: choose    X: start");
        std::snprintf(p.rows[15],78,"Stick: aim/place   D-pad: fine aim   L/R: power");
        std::snprintf(p.rows[17],78,"X: confirm/shoot   Circle: cancel   Start: pause");
        std::snprintf(p.rows[19],78,"Square: inspect balls   Triangle: end when target met");
    } else if (g.phase==Phase::Shop) {
        std::snprintf(p.rows[2],78,"ROUND %d %s  SCORE %d/%d  MONEY $%d  HP %d",
            g.round,g.last_round_won?"PASSED":"MISSED",g.score,g.target,g.money,g.hp);
        for (unsigned i=0;i<4;++i) {
            unsigned id=g.offers[i];
            if (!id) std::snprintf(p.rows[4+i*2],78,"%c [%u] SOLD",g.selection==i?'>':' ',i+1);
            else std::snprintf(p.rows[4+i*2],78,"%c [%u] %-14s value %d  price $%d",g.selection==i?'>':' ',i+1,
                ball_name(id),builtin(id).base_score,shop_price(id));
        }
        std::snprintf(p.rows[12],78,"%c REROLL $%d",g.selection==4?'>':' ',g.reroll_price());
        std::snprintf(p.rows[14],78,"%c NEXT ROUND %d (Circle also continues)",g.selection==5?'>':' ',g.round+1);
        const OwnedBall& owned=g.inventory[g.inventory_selection];
        std::snprintf(p.rows[16],78,"BUILD %zu/15  selected %u: %s Lv%u value %d",g.inventory_count,
            g.inventory_selection+1,ball_name(owned.id),unsigned(owned.level),owned.score);
        std::snprintf(p.rows[18],78,"Left/right: offer   Up/down: owned ball");
        std::snprintf(p.rows[20],78,"X buy   Square merge into selected   Triangle sell");
        if(g.selection<4 && g.offers[g.selection])
            std::snprintf(p.rows[22],78,"%s",ability_text(builtin(g.offers[g.selection]).ability));
    } else {
        std::snprintf(p.rows[4],78,"%s",g.phase==Phase::RunWon?"TEN ROUNDS COMPLETE!":"RUN OVER");
        std::snprintf(p.rows[7],78,"Round %d  Last score %d/%d  Money $%d",g.round,g.score,g.target,g.money);
        std::snprintf(p.rows[11],78,"X: return to deck selection");
    }
    return p;
}
inline char ball_symbol(unsigned a) {
    const char symbols[]={' ','P','B','$','S','*','A','O','T','F'};
    return symbols[a<=Football?a:0];
}
}