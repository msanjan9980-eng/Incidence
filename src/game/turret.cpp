#include "game/turret.hpp"
#include "render/renderer.hpp"

namespace inc::game {

bool Turret::step(f32 dt, Vec2 target_pos) {
    (void)target_pos;   // unused for now; only used by the caller to aim
    if (!alive) return false;

    fire_timer += dt;
    if (fire_timer >= fire_interval) {
        fire_timer -= fire_interval;
        return true;
    }
    return false;
}

void Turret::draw(render::Renderer& r) const {
    if (!alive) return;

    // Outer glow — soft enemy red.
    r.draw_circle(pos, radius + 8.0f, rgba(180, 50, 60, 60), 22);

    // Solid body.
    r.draw_circle(pos, radius, COLOR_ENEMY_FILL, 20);

    // Bright edge ring.
    r.draw_ring(pos, radius, radius + 2.5f, COLOR_ENEMY_EDGE, 24);

    // Targeting eye — bright amber core.
    r.draw_circle(pos, 7.0f, COLOR_ENEMY_CORE, 14);

    // Four spikes at cardinal points (visual character).
    for (int i = 0; i < 4; ++i) {
        const f32 a = TAU * static_cast<f32>(i) / 4.0f;
        const Vec2 dir = Vec2::from_angle(a);
        Vec2 a_pt = pos + dir * (radius + 1.0f);
        Vec2 b_pt = pos + dir * (radius + 9.0f);
        r.draw_line(a_pt, b_pt, 3.0f, COLOR_ENEMY_EDGE);
    }
}

} // namespace inc::game