#pragma once

#include "core/math.hpp"
#include "game/skill.hpp"

namespace inc::game {

struct Stats {
    int arc    = 3;
    int focus  = 3;
    int charge = 3;
    int drift  = 3;
};

inline constexpr int STAT_MIN = 3;
inline constexpr int STAT_MAX = 15;

inline constexpr int LEVEL_MIN = 1;
inline constexpr int LEVEL_MAX = 20;

class Character {
public:
    int   level        = LEVEL_MIN;
    int   xp           = 0;
    int   stat_points  = 0;
    int   skill_points = 0;
    Stats stats;

    u16   learned_skills = 0;

    [[nodiscard]] f32 shield_span_rad() const noexcept;
    [[nodiscard]] f32 spin_speed() const noexcept;
    [[nodiscard]] f32 reflect_multiplier() const noexcept;
    [[nodiscard]] f32 dash_impulse() const noexcept;
    [[nodiscard]] f32 dash_cooldown() const noexcept;

    [[nodiscard]] static int xp_to_next(int level) noexcept;
    [[nodiscard]] int  xp_to_next() const noexcept { return xp_to_next(level); }
    [[nodiscard]] f32  xp_progress() const noexcept;

    int add_xp(int amount);
    bool spend_point(int index);

    [[nodiscard]] bool has_skill(SkillId id) const noexcept {
        return (learned_skills & (1u << int(id))) != 0;
    }

    [[nodiscard]] bool can_learn(SkillId id) const noexcept;
    bool learn_skill(SkillId id);
};

} // namespace inc::game