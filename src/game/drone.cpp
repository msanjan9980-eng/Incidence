#include "game/drone.hpp"
#include "game/character.hpp"
#include "render/renderer.hpp"

#include <cmath>

namespace inc::game {

void Drone::step(f32 dt, const Character& ch,
                 bool key_left, bool key_right,
                 bool key_up,   bool key_down,
                 bool key_q,    bool key_e,
                 bool key_dash)
{
    // ---- Read stats ---------------------------------------------------
    shield_span = ch.shield_span_rad();
    f32 spin_speed = ch.spin_speed();
    if (ch.has_skill(SkillId::RotateInstinct)) spin_speed *= 1.25f;

    // ---- Dash cooldown ------------------------------------------------
    if (dash_cd > 0.0f) dash_cd -= dt;

    // ---- Movement -----------------------------------------------------
    f32 ax = 0.0f, ay = 0.0f;
    if (key_left)  ax -= 1.0f;
    if (key_right) ax += 1.0f;
    if (key_up)    ay -= 1.0f;
    if (key_down)  ay += 1.0f;

    const f32 alen = std::sqrt(ax * ax + ay * ay);
    if (alen > 0.0f) { ax /= alen; ay /= alen; }

    velocity.x += ax * ACCEL * dt;
    velocity.y += ay * ACCEL * dt;

    const f32 drag_factor = std::pow(DRAG, dt);
    velocity.x *= drag_factor;
    velocity.y *= drag_factor;

    const f32 speed = velocity.length();
    if (speed > MAX_SPEED) velocity = velocity * (MAX_SPEED / speed);

    // ---- Dash ---------------------------------------------------------
    if (key_dash && dash_cd <= 0.0f) {
        Vec2 dir;
        if (alen > 0.0f) {
            dir = Vec2{ ax, ay };
        } else if (velocity.length_sq() > 100.0f) {
            dir = velocity.normalized();
        } else {
            dir = Vec2::from_angle(heading);
        }
        velocity += dir * ch.dash_impulse();
        dash_cd = ch.dash_cooldown();
    }

    // ---- Integrate ----------------------------------------------------
    position += velocity * dt;

    // ---- Wall bounce --------------------------------------------------
    constexpr f32 R = SHIELD_R_OUTER;
    if (position.x < R)                  { position.x = R;                  velocity.x = -velocity.x; }
    else if (position.x > VIRTUAL_W - R) { position.x = VIRTUAL_W - R;       velocity.x = -velocity.x; }
    if (position.y < R)                  { position.y = R;                  velocity.y = -velocity.y; }
    else if (position.y > VIRTUAL_H - R) { position.y = VIRTUAL_H - R;       velocity.y = -velocity.y; }

    // ---- Rotation -----------------------------------------------------
    if (key_q) spin = -spin_speed;
    else if (key_e) spin = spin_speed;
    else spin *= std::pow(0.02f, dt);

    heading = wrap_pi(heading + spin * dt);
}

void Drone::facet_angles(int i, f32& out_a0, f32& out_a1) const {
    const f32 step = shield_span / static_cast<f32>(SHIELD_FACETS);
    const f32 base = heading - shield_span * 0.5f;
    out_a0 = base + static_cast<f32>(i)     * step;
    out_a1 = base + static_cast<f32>(i + 1) * step;
}

Vec2 Drone::facet_normal(int i) const {
    f32 a0, a1;
    facet_angles(i, a0, a1);
    const f32 mid = (a0 + a1) * 0.5f;
    return Vec2::from_angle(mid);
}

void Drone::draw(render::Renderer& r) const {
    const f32 speed = velocity.length();
    if (speed > 40.0f) {
        Vec2 back = -(velocity * (1.0f / speed));
        for (int i = 0; i < 3; ++i) {
            const f32 d = CORE_RADIUS + 6.0f + f32(i) * 10.0f;
            const u8 a = u8(140.0f * (1.0f - f32(i) / 3.0f));
            r.draw_circle(position + back * d, 5.0f - f32(i),
                          with_alpha(COLOR_ENGINE_GLOW, a), 10);
        }
    }

    for (int i = 0; i < SHIELD_FACETS; ++i) {
        f32 a0, a1;
        facet_angles(i, a0, a1);
        const Vec2 d0 = Vec2::from_angle(a0);
        const Vec2 d1 = Vec2::from_angle(a1);
        const Vec2 pin0  = position + d0 * SHIELD_R_INNER;
        const Vec2 pin1  = position + d1 * SHIELD_R_INNER;
        const Vec2 pout0 = position + d0 * SHIELD_R_OUTER;
        const Vec2 pout1 = position + d1 * SHIELD_R_OUTER;

        r.draw_quad(pin0, pin1, pout1, pout0, COLOR_FACET_FILL);
        r.draw_line(pin0,  pin1,  2.5f, COLOR_FACET_EDGE);
        r.draw_line(pout0, pout1, 3.0f, COLOR_FACET_EDGE);
        r.draw_line(pin0,  pout0, 1.2f, COLOR_FACET_EDGE);
        r.draw_line(pin1,  pout1, 1.2f, COLOR_FACET_EDGE);

        const Vec2 n_dir = facet_normal(i);
        r.draw_line(position + n_dir * (SHIELD_R_OUTER + 3.0f),
                    position + n_dir * (SHIELD_R_OUTER + 16.0f),
                    2.0f, COLOR_FACET_NORMAL);
    }

    r.draw_ring(position, SHIELD_R_OUTER + 18.0f, SHIELD_R_OUTER + 19.0f,
                with_alpha(COLOR_FACET_EDGE, 60), 40);

    r.draw_circle(position, CORE_RADIUS + 8.0f, COLOR_CORE_GLOW, 24);
    r.draw_circle(position, CORE_RADIUS,        COLOR_CORE_FILL, 20);
    r.draw_ring  (position, CORE_RADIUS, CORE_RADIUS + 2.5f, COLOR_CORE_EDGE, 28);
    r.draw_circle(position, 5.0f, COLOR_CORE_INNER, 12);

    const Vec2 hd = Vec2::from_angle(heading);
    r.draw_circle(position + hd * (CORE_RADIUS + 3.0f), 3.5f, COLOR_CORE_INNER, 10);
    r.draw_line(position + hd * (CORE_RADIUS + 6.0f),
                position + hd * (CORE_RADIUS + 14.0f),
                2.0f, COLOR_CORE_INNER);

    if (dash_cd > 0.0f) {
        const f32 frac = clamp(1.0f - dash_cd * 0.6f, 0.0f, 1.0f);
        r.draw_arc_ring(position, CORE_RADIUS + 18.0f, CORE_RADIUS + 22.0f,
                        -HALF_PI, -HALF_PI + frac * TAU,
                        with_alpha(COLOR_CORE_EDGE, 160), 20);
    }
}

} // namespace inc::game