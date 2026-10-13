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
    const f32 spin_speed = ch.spin_speed();

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
    // ---- Engine trail --------------------------------------------------
    const f32 speed = velocity.length();
    if (speed > 40.0f) {
        Vec2 back = -(velocity * (1.0f / speed));
        Vec2 tip  = position + back * 34.0f;
        r.draw_line(position + back * (CORE_RADIUS + 2.0f), tip,
                    6.0f, COLOR_ENGINE_GLOW);
    }

    // ---- Shield facets -------------------------------------------------
    for (int i = 0; i < SHIELD_FACETS; ++i) {
        f32 a0, a1;
        facet_angles(i, a0, a1);

        const Vec2 dir0 = Vec2::from_angle(a0);
        const Vec2 dir1 = Vec2::from_angle(a1);

        const Vec2 pin0  = position + dir0 * SHIELD_R_INNER;
        const Vec2 pin1  = position + dir1 * SHIELD_R_INNER;
        const Vec2 pout0 = position + dir0 * SHIELD_R_OUTER;
        const Vec2 pout1 = position + dir1 * SHIELD_R_OUTER;

        r.draw_quad(pin0, pin1, pout1, pout0, COLOR_FACET_FILL);

        r.draw_line(pin0,  pin1,  2.5f, COLOR_FACET_EDGE);
        r.draw_line(pout0, pout1, 2.5f, COLOR_FACET_EDGE);
        r.draw_line(pin0,  pout0, 1.2f, COLOR_FACET_EDGE);
        r.draw_line(pin1,  pout1, 1.2f, COLOR_FACET_EDGE);

        const Vec2 n_dir = facet_normal(i);
        r.draw_line(position + n_dir * (SHIELD_R_OUTER + 3.0f),
                    position + n_dir * (SHIELD_R_OUTER + 16.0f),
                    2.0f, COLOR_FACET_NORMAL);
    }

    // ---- Core ---------------------------------------------------------
    r.draw_circle(position, CORE_RADIUS + 8.0f, COLOR_CORE_GLOW, 24);
    r.draw_circle(position, CORE_RADIUS,        COLOR_CORE_FILL, 20);
    r.draw_ring  (position, CORE_RADIUS, CORE_RADIUS + 2.5f, COLOR_CORE_EDGE, 28);
    r.draw_circle(position, 4.5f, COLOR_CORE_INNER, 12);

    // ---- Dash cooldown ring --------------------------------------------
    if (dash_cd > 0.0f) {
        const f32 frac = clamp(1.0f - dash_cd * 0.6f, 0.0f, 1.0f);
        r.draw_arc_ring(position, CORE_RADIUS + 18.0f, CORE_RADIUS + 22.0f,
                        -HALF_PI, -HALF_PI + frac * TAU,
                        with_alpha(COLOR_CORE_EDGE, 160), 20);
    }
}

} // namespace inc::game