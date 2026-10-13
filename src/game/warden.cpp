#include "game/warden.hpp"
#include "game/room.hpp"
#include "render/renderer.hpp"

#include <cmath>

namespace inc::game {

void Warden::reset() {
    pos           = { 960.0f, 540.0f };
    hp            = hp_max;
    rotation      = 0.0f;
    rot_speed     = 0.3f;
    phase         = 1;
    alive         = true;
    fire_timer    = 0.5f;      // stagger the first shot
    fire_interval = 2.0f;
    phase_flash   = 0.0f;
}

bool Warden::step(f32 dt) {
    if (!alive) return false;

    // ---- Rotation -----------------------------------------------------
    rotation = wrap_pi(rotation + rot_speed * dt);

    // ---- Fire clock ---------------------------------------------------
    fire_timer += dt;
    if (fire_timer >= fire_interval) {
        fire_timer -= fire_interval;
        return true;
    }
    return false;
}

void Warden::facet_endpoints(int i, Vec2& out_a, Vec2& out_b) const {
    const f32 theta = rotation + f32(i) * TAU / f32(FACET_COUNT);
    const f32 a_ang = theta - FACET_HALF_SPAN;
    const f32 b_ang = theta + FACET_HALF_SPAN;
    out_a = pos + Vec2::from_angle(a_ang) * CAGE_RADIUS;
    out_b = pos + Vec2::from_angle(b_ang) * CAGE_RADIUS;
}

Vec2 Warden::facet_normal(int i) const {
    Vec2 a, b;
    facet_endpoints(i, a, b);
    Vec2 ab = b - a;
    Vec2 n  = ab.perp().normalized();
    // The normal must point AWAY from the cage center.
    const Vec2 mid = (a + b) * 0.5f;
    if ((mid - pos).dot(n) < 0.0f) n = -n;
    return n;
}

int Warden::facet_hit_by(Vec2 p, f32 r) const {
    for (int i = 0; i < FACET_COUNT; ++i) {
        Vec2 a, b;
        facet_endpoints(i, a, b);
        Vec2 closest;
        const f32 d = closest_point_on_segment(p, a, b, closest);
        if (d < r + FACET_THICKNESS * 0.5f) {
            return i;
        }
    }
    return -1;
}

void Warden::damage(int amount) {
    if (!alive) return;
    hp -= amount;
    if (hp < 0) hp = 0;

    // ---- Phase transitions -------------------------------------------
    int new_phase = phase;
    if      (hp <= hp_max / 3)     new_phase = 3;
    else if (hp <= 2 * hp_max / 3) new_phase = 2;
    else                           new_phase = 1;

    if (new_phase != phase) {
        phase = new_phase;
        phase_flash = 0.6f;   // flash duration in seconds
        switch (phase) {
            case 2:
                rot_speed     = 0.7f;
                fire_interval = 1.5f;
                break;
            case 3:
                rot_speed     = 0.0f;
                fire_interval = 3.0f;
                break;
            default: break;
        }
    }

    if (hp <= 0) {
        alive = false;
    }
}

void Warden::draw(render::Renderer& r) const {
    if (!alive) return;

    // ---- Cage facets (drawn first, under the core) --------------------
    for (int i = 0; i < FACET_COUNT; ++i) {
        Vec2 a, b;
        facet_endpoints(i, a, b);
        r.draw_line(a, b, 12.0f, COLOR_FACET_FILL);
        r.draw_line(a, b, 5.0f,  COLOR_CORE_EDGE);

        // Small outward-pointing tick on the facet midpoint.
        const Vec2 mid = (a + b) * 0.5f;
        const Vec2 n   = facet_normal(i);
        r.draw_line(mid, mid + n * 10.0f, 2.0f, COLOR_FACET_NORMAL);
    }

    // ---- Core ---------------------------------------------------------
    const f32 core_hp_ratio = f32(hp) / f32(hp_max);
    const Color core_color = mix(COLOR_ENEMY_EDGE, COLOR_CORE_INNER, core_hp_ratio);

    r.draw_circle(pos, CORE_RADIUS + 10.0f, with_alpha(core_color, 60), 22);
    r.draw_circle(pos, CORE_RADIUS, COLOR_CORE_FILL, 22);
    r.draw_ring(pos, CORE_RADIUS - 2.0f, CORE_RADIUS + 4.0f, core_color, 28);
    r.draw_circle(pos, 8.0f, core_color, 14);

    // ---- Phase flash overlay ------------------------------------------
    if (phase_flash > 0.0f) {
        const f32 t = clamp(phase_flash / 0.6f, 0.0f, 1.0f);
        const u8 a = u8(t * 100.0f);
        r.draw_quad({ 0.0f,   0.0f    },
                    { VIRTUAL_W, 0.0f  },
                    { VIRTUAL_W, VIRTUAL_H },
                    { 0.0f,   VIRTUAL_H },
                    with_alpha(COLOR_CORE_INNER, a));
    }
}

void Warden::draw_glow(render::Renderer& r) const {
    if (!alive) return;

    // Big soft halo behind the core.
    const f32 core_hp_ratio = f32(hp) / f32(hp_max);
    const Color core_color = mix(COLOR_ENEMY_EDGE, COLOR_CORE_INNER, core_hp_ratio);
    r.draw_circle(pos, CORE_RADIUS + 26.0f, with_alpha(core_color, 70), 24);
    r.draw_circle(pos, CAGE_RADIUS,          with_alpha(core_color, 30), 32);
}

} // namespace inc::game