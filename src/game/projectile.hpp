#pragma once

#include "core/vec2.hpp"
#include "core/color.hpp"

namespace inc::render { class Renderer; }

namespace inc::game {

struct Drone;

// ---------------------------------------------------------------------------
// Projectile — a straight-flying bullet fired by an enemy.
// ---------------------------------------------------------------------------
struct Projectile {
    Vec2 pos;
    Vec2 vel;
    f32  radius      = 5.0f;
    f32  age         = 0.0f;
    int  bounces     = 0;
    bool alive       = false;

    static constexpr f32 LIFETIME_S       = 8.0f;
    static constexpr int MAX_WALL_BOUNCES = 4;

    void step(f32 dt);
    void draw(render::Renderer& r) const;

    // M5: Draw a dotted ghost line showing where this bullet will go,
    // including reflections off walls and the current drone shield.
    // Called once per frame from World::draw.
    void draw_predicted(render::Renderer& r, const Drone& drone) const;
};

} // namespace inc::game