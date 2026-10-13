#include "game/turret.hpp"
#include "game/drone.hpp"
#include "render/renderer.hpp"
#include "render/icons.hpp"

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

    Color fill_c = COLOR_ENEMY_FILL;
    Color edge_c = COLOR_ENEMY_EDGE;
    Color core_c = COLOR_ENEMY_CORE;
    switch (type) {
        case EnemyType::Lancer: fill_c = COLOR_ENEMY_FILL;  edge_c = COLOR_ENEMY_EDGE;  core_c = COLOR_ENEMY_CORE;  break;
        case EnemyType::Banker: fill_c = COLOR_BANKER_FILL; edge_c = COLOR_BANKER_EDGE; core_c = COLOR_BANKER_CORE; break;
        case EnemyType::Lead:   fill_c = COLOR_LEAD_FILL;   edge_c = COLOR_LEAD_EDGE;   core_c = COLOR_LEAD_CORE;   break;
    }

    if (show_telegraph) {
        const f32 tt = clamp((fire_timer - (fire_interval - TELEGRAPH_S)) / TELEGRAPH_S, 0.0f, 1.0f);
        const u8 a = u8(tt * 220.0f);
        r.draw_line(pos, telegraph_target, 1.4f, with_alpha(edge_c, a));
        const Vec2 diff = telegraph_target - pos;
        if (diff.length_sq() > 1.0f) {
            const Vec2 perp = diff.perp().normalized() * 7.0f;
            r.draw_line(telegraph_target - perp, telegraph_target + perp, 1.4f, with_alpha(edge_c, a));
        }
    }

    r.draw_circle(pos, radius + 8.0f, with_alpha(edge_c, 60), 22);
    r.draw_circle(pos, radius,        fill_c, 20);
    r.draw_ring  (pos, radius, radius + 2.5f, edge_c, 24);

    const f32 icon_sz = radius * 1.25f;
    switch (type) {
        case EnemyType::Lancer:
            render::icon_lancer(r, pos, icon_sz, with_alpha(edge_c, 220), core_c); break;
        case EnemyType::Banker:
            render::icon_banker(r, pos, icon_sz, with_alpha(edge_c, 220), core_c); break;
        case EnemyType::Lead:
            render::icon_lead  (r, pos, icon_sz, with_alpha(edge_c, 220), core_c); break;
    }

    const f32 pip_y  = pos.y - radius - 14.0f;
    const f32 pip_sp = 8.0f;
    const f32 pip_x0 = pos.x - pip_sp;
    for (int i = 0; i < 3; ++i) {
        const f32 px = pip_x0 + f32(i) * pip_sp;
        if (i < hp) r.draw_circle({px, pip_y}, 2.5f, core_c, 8);
        else        r.draw_ring  ({px, pip_y}, 2.0f, 3.0f, with_alpha(core_c, 100), 8);
    }
}

} // namespace inc::game