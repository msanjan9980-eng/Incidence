#include "game/turret.hpp"
#include "game/drone.hpp"
#include "render/renderer.hpp"

#include <cmath>

namespace inc::game {

bool Turret::step(f32 dt, const Drone& drone, Vec2& out_target) {
    if (!alive) return false;

    // ---- Pick the aim target --------------------------------------------
    Vec2 to_drone = drone.position - pos;
    const f32 dist = to_drone.length();
    if (dist < 1e-3f) {
        out_target = drone.position;
    } else if (type == EnemyType::Lead) {
        // Iteratively predict where the drone will be when the bullet
        // reaches it. Two iterations converge to sub-pixel accuracy.
        f32 flight = dist / bullet_speed;
        Vec2 predicted = drone.position;
        for (int i = 0; i < 2; ++i) {
            predicted = drone.position + drone.velocity * flight;
            flight = (predicted - pos).length() / bullet_speed;
        }
        out_target = predicted;
    } else {
        out_target = drone.position;
    }

    // ---- Advance the firing clock ---------------------------------------
    fire_timer += dt;
    if (fire_timer >= fire_interval) {
        fire_timer -= fire_interval;
        return true;
    }
    return false;
}

void Turret::draw(render::Renderer& r, bool show_telegraph, Vec2 telegraph_target) const {
    if (!alive) return;

    // ---- Palette by type — initialized to Lancer's palette to silence
    //      the compiler's "maybe uninitialized" warning, then overwritten
    //      below. The switch is exhaustive but GCC can't always prove it.
    Color fill_c = COLOR_ENEMY_FILL;
    Color edge_c = COLOR_ENEMY_EDGE;
    Color core_c = COLOR_ENEMY_CORE;

    switch (type) {
        case EnemyType::Lancer:
            fill_c = COLOR_ENEMY_FILL;  edge_c = COLOR_ENEMY_EDGE;  core_c = COLOR_ENEMY_CORE;  break;
        case EnemyType::Banker:
            fill_c = COLOR_BANKER_FILL; edge_c = COLOR_BANKER_EDGE; core_c = COLOR_BANKER_CORE; break;
        case EnemyType::Lead:
            fill_c = COLOR_LEAD_FILL;   edge_c = COLOR_LEAD_EDGE;   core_c = COLOR_LEAD_CORE;   break;
    }

    // ---- Telegraph line -------------------------------------------------
    if (show_telegraph) {
        const f32 t = clamp((fire_timer - (fire_interval - TELEGRAPH_S)) / TELEGRAPH_S,
                            0.0f, 1.0f);
        const u8 a = u8(t * 220.0f);
        r.draw_line(pos, telegraph_target, 1.4f, with_alpha(edge_c, a));

        // Small cross at the target end so the aim point is legible.
        const Vec2 diff = telegraph_target - pos;
        if (diff.length_sq() > 1.0f) {
            const Vec2 perp = diff.perp().normalized() * 7.0f;
            r.draw_line(telegraph_target - perp, telegraph_target + perp,
                        1.4f, with_alpha(edge_c, a));
        }
    }

    // ---- Body -----------------------------------------------------------
    r.draw_circle(pos, radius + 8.0f, with_alpha(edge_c, 60), 22);
    r.draw_circle(pos, radius,        fill_c, 20);
    r.draw_ring  (pos, radius, radius + 2.5f, edge_c, 24);
    r.draw_circle(pos, 7.0f,          core_c, 14);

    // ---- Per-type character ---------------------------------------------
    switch (type) {
        case EnemyType::Lancer:
            for (int i = 0; i < 4; ++i) {
                const f32 a = TAU * f32(i) / 4.0f;
                const Vec2 dir = Vec2::from_angle(a);
                r.draw_line(pos + dir * (radius + 1.0f),
                            pos + dir * (radius + 9.0f),
                            3.0f, edge_c);
            }
            break;

        case EnemyType::Banker:
            for (int i = 0; i < 4; ++i) {
                const f32 a0 = TAU * f32(i)     / 4.0f + PI * 0.25f;
                const f32 a1 = TAU * f32(i + 1) / 4.0f + PI * 0.25f;
                r.draw_line(pos + Vec2::from_angle(a0) * (radius - 5.0f),
                            pos + Vec2::from_angle(a1) * (radius - 5.0f),
                            2.6f, edge_c);
            }
            break;

        case EnemyType::Lead:
            r.draw_ring(pos, radius + 5.0f,  radius + 7.0f,  with_alpha(edge_c, 200), 20);
            r.draw_ring(pos, radius + 11.0f, radius + 12.5f, with_alpha(edge_c, 130), 20);
            break;
    }
}

} // namespace inc::game