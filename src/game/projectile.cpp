#include "game/projectile.hpp"
#include "render/renderer.hpp"

namespace inc::game {

void Projectile::step(f32 dt) {
    if (!alive) return;
    pos += vel * dt;
    age += dt;
    if (age >= LIFETIME_S) {
        alive = false;
    }
}

void Projectile::draw(render::Renderer& r) const {
    if (!alive) return;

    // Outer glow — warm orange, semi-transparent.
    r.draw_circle(pos, radius + 5.0f, COLOR_BULLET_GLOW, 14);

    // Bright core — pale yellow.
    r.draw_circle(pos, radius, COLOR_BULLET_CORE, 12);

    // Short motion trail — a tapering line in the direction of travel.
    Vec2 v = vel;
    const f32 speed = v.length();
    if (speed > 1.0f) {
        Vec2 dir = v * (1.0f / speed);
        Vec2 tail_start = pos - dir * (radius + 2.0f);
        Vec2 tail_end   = pos - dir * (radius + 14.0f);
        r.draw_line(tail_start, tail_end, 2.0f, COLOR_BULLET_GLOW);
    }
}

} // namespace inc::game