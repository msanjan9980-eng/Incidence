#pragma once

#include "core/vec2.hpp"
#include "core/color.hpp"

namespace inc::render { class Renderer; }

namespace inc::game {

class Character;

// ---------------------------------------------------------------------------
// Drone — the player. KITE-9, a mirror deflector unit.
//
// Shield span and spin speed are now dynamic, sourced from Character stats
// each simulation tick. Dash ability is bound to Space.
// ---------------------------------------------------------------------------
struct Drone {
    // ---- State ---------------------------------------------------------
    Vec2 position { VIRTUAL_W * 0.5f, VIRTUAL_H * 0.5f };
    Vec2 velocity { 0.0f, 0.0f };
    f32  heading     = -HALF_PI;
    f32  spin        = 0.0f;
    f32  shield_span = 2.44f;   // radians, updated from Character each step
    f32  dash_cd     = 0.0f;    // seconds until next dash

    // ---- Geometry -----------------------------------------------------
    static constexpr f32 CORE_RADIUS    = 14.0f;
    static constexpr f32 SHIELD_R_INNER = 50.0f;
    static constexpr f32 SHIELD_R_OUTER = 60.0f;
    static constexpr int SHIELD_FACETS  = 5;

    // ---- Movement tuning ----------------------------------------------
    static constexpr f32 ACCEL     = 2400.0f;
    static constexpr f32 DRAG      = 0.85f;
    static constexpr f32 MAX_SPEED = 700.0f;

    // ---- Simulation ---------------------------------------------------
    void step(f32 dt, const Character& ch,
              bool key_left, bool key_right,
              bool key_up,   bool key_down,
              bool key_q,    bool key_e,
              bool key_dash);

    void draw(render::Renderer& r) const;

    // ---- Geometry helpers ---------------------------------------------
    [[nodiscard]] Vec2 facet_normal(int i) const;
    void facet_angles(int i, f32& out_a0, f32& out_a1) const;
};

} // namespace inc::game