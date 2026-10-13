#pragma once

#include "core/vec2.hpp"
#include "core/mat3.hpp"

namespace inc::render { class Renderer; }

namespace inc::game {

// ---------------------------------------------------------------------------
// Triangle — the M2 demo entity.
//
// Bounces off the edges of the virtual canvas (1920x1080), moves with arrow
// keys, rotates with Q/E. Its only job is to prove that input -> sim ->
// render works end to end. It will be replaced by real entities in M3.
// ---------------------------------------------------------------------------
struct Triangle {
    Vec2  position { VIRTUAL_W * 0.5f, VIRTUAL_H * 0.5f };
    Vec2  velocity { 420.0f, 300.0f };   // virtual px/sec
    f32   angle    = 0.0f;               // radians
    f32   spin     = 0.0f;               // rad/sec, from Q/E

    // Physics constants.
    static constexpr f32 ACCEL     = 2400.0f;   // px/sec^2 while holding a key
    static constexpr f32 DRAG      = 0.85f;     // velocity retained per second
    static constexpr f32 MAX_SPEED = 900.0f;    // px/sec
    static constexpr f32 RADIUS    = 40.0f;     // for wall collision

    static constexpr f32 SPIN_SPEED = 3.4f;     // rad/sec

    // Advance one fixed simulation tick.
    void step(f32 dt, bool key_left, bool key_right,
                       bool key_up,   bool key_down,
                       bool key_q,    bool key_e);

    // Submit this entity to the renderer.
    void draw(render::Renderer& r) const;
};

} // namespace inc::game