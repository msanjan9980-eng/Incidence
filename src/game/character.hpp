#pragma once

#include "core/math.hpp"

namespace inc::game {

// ---------------------------------------------------------------------------
// Stats — the four numeric parameters that define your character.
//
// Every level grants 1 stat point. Stats start at 3, cap at 15. Total points
// earned across a full run (level 1 -> 20) is 19. You cannot max all four
// stats; the build is the choice.
// ---------------------------------------------------------------------------
struct Stats {
    int arc    = 3;   // shield arc width
    int focus  = 3;   // rotation speed
    int charge = 3;   // reflected bullet speed multiplier
    int drift  = 3;   // dash impulse and cooldown
};

inline constexpr int STAT_MIN = 3;
inline constexpr int STAT_MAX = 15;

inline constexpr int LEVEL_MIN = 1;
inline constexpr int LEVEL_MAX = 20;

// ---------------------------------------------------------------------------
// Character — the player's progression state.
//
// Derived gameplay values (shield span in radians, spin speed, dash impulse,
// etc.) are computed from stats on demand. Nothing is cached; the Character
// is the single source of truth.
// ---------------------------------------------------------------------------
class Character {
public:
    int   level        = LEVEL_MIN;
    int   xp           = 0;
    int   stat_points  = 0;
    int   skill_points = 0;
    Stats stats;

    // ---- Derived values (all read from stats) -------------------------
    // Shield arc width: 100deg at ARC=3, 212.5deg at ARC=15.
    [[nodiscard]] f32 shield_span_rad() const noexcept;

    // Rotation speed in radians / second.
    [[nodiscard]] f32 spin_speed() const noexcept;

    // Multiplier applied to a bullet's speed when it reflects off the shield.
    [[nodiscard]] f32 reflect_multiplier() const noexcept;

    // Dash impulse in pixels / second.
    [[nodiscard]] f32 dash_impulse() const noexcept;

    // Dash cooldown in seconds.
    [[nodiscard]] f32 dash_cooldown() const noexcept;

    // ---- XP / leveling ------------------------------------------------
    // XP required to go from `level` to `level + 1`. Returns 0 at cap.
    [[nodiscard]] static int xp_to_next(int level) noexcept;
    [[nodiscard]] int  xp_to_next() const noexcept { return xp_to_next(level); }
    [[nodiscard]] f32  xp_progress() const noexcept;

    // Awards XP. Returns the number of levels gained (0, 1, or more).
    int add_xp(int amount);

    // Spend a stat point. index: 0=arc 1=focus 2=charge 3=drift.
    // Returns true if a point was successfully spent.
    bool spend_point(int index);
};

} // namespace inc::game