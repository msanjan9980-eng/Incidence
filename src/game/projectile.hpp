#pragma once

#include "core/vec2.hpp"
#include "core/color.hpp"

namespace inc::render { class Renderer; }

namespace inc::game {

struct Drone;
struct Wall;

// ---------------------------------------------------------------------------
// Projectile — a straight-flying bullet fired by an enemy.
//
// The bullet's survival is governed by `wall_bounces_left`. Each wall or
// screen-edge bounce decrements it; shield bounces do not. When it reaches
// zero, the next wall contact destroys the bullet. Lancer / Lead bullets
// start at 4. Banker bullets start at 8 — they persist through crowded
// geometry and force the player to keep reading ricochet paths.
// ---------------------------------------------------------------------------
struct Projectile {
    Vec2  pos;
    Vec2  vel;
    f32   radius             = 5.0f;
    f32   age                = 0.0f;
    int   bounces            = 0;    // total bounces (for scoring)
    int   wall_bounces_left  = 4;    // decremented per wall bounce
    Color core_color         = COLOR_BULLET_CORE;
    Color glow_color         = COLOR_BULLET_GLOW;
    bool  alive              = false;

    static constexpr f32 LIFETIME_S = 8.0f;

    void step(f32 dt);
    void draw(render::Renderer& r) const;

    void draw_predicted(render::Renderer& r,
                        const Drone& drone,
                        const Wall* walls, int wall_count) const;
};

} // namespace inc::game