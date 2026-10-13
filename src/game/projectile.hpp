#pragma once

#include "core/vec2.hpp"
#include "core/color.hpp"

namespace inc::render { class Renderer; }

namespace inc::game {

// ---------------------------------------------------------------------------
// Projectile — a straight-flying bullet fired by an enemy.
//
// The player has no gun. Every projectile in INCIDENCE originates from an
// enemy and travels in a straight line at constant speed, until:
//   - it bounces off a wall (up to MAX_WALL_BOUNCES times),
//   - it bounces off the player's shield (unlimited bounces, kills the enemy),
//   - it hits the player's core (damage + dies),
//   - it hits an enemy (damage + dies),
//   - or its lifetime expires.
//
// Bullets are stored in a fixed pool inside World. There are no dynamic
// allocations during gameplay.
// ---------------------------------------------------------------------------
struct Projectile {
    Vec2 pos;
    Vec2 vel;
    f32  radius      = 5.0f;
    f32  age         = 0.0f;
    int  bounces     = 0;      // total bounces (walls + shield)
    bool alive       = false;

    // Design constants.
    static constexpr f32 LIFETIME_S       = 8.0f;
    static constexpr int MAX_WALL_BOUNCES = 4;

    void step(f32 dt);
    void draw(render::Renderer& r) const;
};

} // namespace inc::game