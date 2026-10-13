#pragma once

#include "core/vec2.hpp"
#include "core/color.hpp"

namespace inc::render { class Renderer; }

namespace inc::game {

// ---------------------------------------------------------------------------
// Drone — the player. KITE-9, a mirror deflector unit.
//
// The drone has two visual layers:
//   1. A CORE at the center — dark glass sphere, the vulnerability. If a
//      projectile touches the core, the drone dies. Small, central, exposed.
//   2. A SHIELD ARC of 5 flat mirror facets — the deflector surface. Each
//      facet is a straight chord with a known normal. Bullets bounce off.
//
// The shield arc spans only ~140 degrees, so there is always a 220-degree
// gap that the player must keep pointed away from incoming fire. That gap
// is the entire tactical game.
// ---------------------------------------------------------------------------
struct Drone {
    // ---- State -----------------------------------------------------------
    Vec2 position { VIRTUAL_W * 0.5f, VIRTUAL_H * 0.5f };
    Vec2 velocity { 0.0f, 0.0f };
    f32  heading  = -HALF_PI;    // radians; where the shield arc is centered
    f32  spin     = 0.0f;         // current spin rate (rad/s) from Q/E

    // ---- Geometry --------------------------------------------------------
    static constexpr f32 CORE_RADIUS    = 14.0f;
    static constexpr f32 SHIELD_R_INNER = 50.0f;
    static constexpr f32 SHIELD_R_OUTER = 60.0f;
    static constexpr f32 SHIELD_SPAN    = 2.44f;    // ~140 degrees in radians
    static constexpr int SHIELD_FACETS  = 5;

    // ---- Movement tuning -------------------------------------------------
    static constexpr f32 ACCEL      = 2400.0f;    // px/sec^2
    static constexpr f32 DRAG       = 0.85f;      // retained per second
    static constexpr f32 MAX_SPEED  = 700.0f;     // px/sec
    static constexpr f32 SPIN_SPEED = 3.4f;       // rad/sec, when Q or E held

    // ---- Simulation ------------------------------------------------------
    void step(f32 dt,
              bool key_left, bool key_right,
              bool key_up,   bool key_down,
              bool key_q,    bool key_e);

    // ---- Rendering -------------------------------------------------------
    void draw(render::Renderer& r) const;

    // ---- Geometry helpers ------------------------------------------------

    // World-space outward normal of facet i (0..SHIELD_FACETS-1).
    [[nodiscard]] Vec2 facet_normal(int i) const;

    // Angles (in world space) of facet i's start and end edges.
    void facet_angles(int i, f32& out_a0, f32& out_a1) const;
};

} // namespace inc::game