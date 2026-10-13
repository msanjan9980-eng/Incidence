#pragma once

#include "core/vec2.hpp"
#include "core/color.hpp"

namespace inc::render { class Renderer; }

namespace inc::game {

struct Drone;

// ---------------------------------------------------------------------------
// Warden of Mirror — the first boss.
//
// A rotating hexagonal cage of six flat mirror facets surrounds a luminous
// core. The cage is too large to reflect a bullet through from any angle
// except the gaps between facets. To damage the Warden, the player must
// reflect an incoming projectile and thread it through a gap into the core.
//
// Three phases, selected by HP thresholds:
//   Phase 1 (100..67): slow rotation (0.3 rad/s), single aimed shot every 2.0s
//   Phase 2 (66..34):  fast rotation (0.7 rad/s), twin predictive shots every 1.5s
//   Phase 3 (33..1):   cage stops rotating, radial 8-shot burst every 3.0s
//
// On death, `alive` becomes false and World handles the victory sequence.
// ---------------------------------------------------------------------------
struct Warden {
    Vec2  pos        { 960.0f, 540.0f };
    int   hp_max     = 100;
    int   hp         = 100;
    f32   rotation   = 0.0f;      // current cage angle
    f32   rot_speed  = 0.3f;      // rad/s, phase-dependent
    int   phase      = 1;
    bool  alive      = true;

    f32   fire_timer      = 0.0f;
    f32   fire_interval   = 2.0f;
    f32   phase_flash     = 0.0f; // > 0 → white flash overlay

    static constexpr int FACET_COUNT     = 6;
    static constexpr f32 CORE_RADIUS     = 40.0f;
    static constexpr f32 CAGE_RADIUS     = 130.0f;
    static constexpr f32 FACET_HALF_SPAN = 0.35f;   // ~20 degrees
    static constexpr f32 FACET_THICKNESS = 8.0f;

    void reset();

    // Advance rotation + fire clock. Returns true on the tick the Warden
    // wants to fire; the caller then spawns bullets based on `phase`.
    bool step(f32 dt);

    // Facet geometry: endpoints of facet i's mirror segment.
    void facet_endpoints(int i, Vec2& out_a, Vec2& out_b) const;

    // Returns the index (0..5) of the facet the point (p, radius r) is
    // currently colliding with, or -1 if it's in a gap.
    int facet_hit_by(Vec2 p, f32 r) const;

    // Returns the outward-facing unit normal of facet i.
    Vec2 facet_normal(int i) const;

    void damage(int amount);

    void draw(render::Renderer& r) const;
    void draw_glow(render::Renderer& r) const;
};

} // namespace inc::game