#pragma once
#include <cstdint>
namespace up {
constexpr unsigned achievement_count=104,profile_stat_count=105;
struct AchievementRule {const char* id; const char* condition; int64_t amount; bool per_round; const char* deck; const char* difficulty; unsigned group,visibility; bool toast,popup; const char* mix_a; const char* mix_b;};
const AchievementRule& achievement_rule(unsigned index);
const char* profile_stat_key(unsigned index);
const char* deck_requirement(unsigned index);
const char* difficulty_requirement(unsigned index);
}
