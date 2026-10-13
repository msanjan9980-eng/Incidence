#include "game/projectile.hpp"
#include "game/drone.hpp"
#include "game/room.hpp"
#include "render/renderer.hpp"

#include <cmath>

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

    r.draw_circle(pos, radius + 5.0f, COLOR_BULLET_GLOW, 14);
    r.draw_circle(pos, radius, COLOR_BULLET_CORE, 12);

    Vec2 v = vel;
    const f32 speed = v.length();
    if (speed > 1.0f) {
        Vec2 dir = v * (1.0f / speed);
        Vec2 tail_start = pos - dir * (radius + 2.0f);
        Vec2 tail_end   = pos - dir * (radius + 14.0f);
        r.draw_line(tail_start, tail_end, 2.0f, COLOR_BULLET_GLOW);
    }
}

void Projectile::draw_predicted(render::Renderer& r,
                                const Drone& drone,
                                const Wall* walls, int wall_count) const {
    if (!alive) return;

    Vec2 sim_pos = pos;
    Vec2 sim_vel = vel;

    constexpr f32 DT          = FIXED_DT;
    constexpr int MAX_STEPS   = 360;
    constexpr int MAX_BOUNCE  = 6;
    constexpr f32 DOT_SPACING = 34.0f;

    int  bounces       = this->bounces;
    f32  since_last    = DOT_SPACING;
    bool alive_preview = true;

    auto bounce_off = [&](Vec2 wa, Vec2 wb) -> bool {
        Vec2 closest;
        const f32 d = closest_point_on_segment(sim_pos, wa, wb, closest);
        if (d >= radius) return false;

        Vec2 ab = wb - wa;
        Vec2 n  = ab.perp().normalized();
        if ((sim_pos - closest).dot(n) < 0.0f) n = -n;
        if (sim_vel.dot(n) >= 0.0f) return false;

        sim_vel = reflect(sim_vel, n);
        sim_pos = closest + n * (radius + 0.5f);
        ++bounces;
        since_last = DOT_SPACING;
        return true;
    };

    for (int i = 0; i < MAX_STEPS && alive_preview; ++i) {
        sim_pos += sim_vel * DT;
        since_last += sim_vel.length() * DT;

        // ---- Screen edge bounce ------------------------------------------
        if (bounces < MAX_BOUNCE) {
            bool hit = false;
            if (sim_pos.x < radius)              { sim_pos.x = radius;              sim_vel.x = -sim_vel.x; hit = true; }
            else if (sim_pos.x > VIRTUAL_W - radius) { sim_pos.x = VIRTUAL_W - radius; sim_vel.x = -sim_vel.x; hit = true; }
            if (sim_pos.y < radius)              { sim_pos.y = radius;              sim_vel.y = -sim_vel.y; hit = true; }
            else if (sim_pos.y > VIRTUAL_H - radius) { sim_pos.y = VIRTUAL_H - radius; sim_vel.y = -sim_vel.y; hit = true; }
            if (hit) ++bounces;
        } else {
            if (sim_pos.x < 0.0f || sim_pos.x > VIRTUAL_W ||
                sim_pos.y < 0.0f || sim_pos.y > VIRTUAL_H)
            {
                alive_preview = false;
            }
        }

        // ---- Interior wall bounce ----------------------------------------
        if (bounces < MAX_BOUNCE) {
            for (int w = 0; w < wall_count; ++w) {
                bounce_off(walls[w].a, walls[w].b);
            }
        }

        // ---- Shield reflection -------------------------------------------
        {
            Vec2 to_b = sim_pos - drone.position;
            const f32 dist = to_b.length();
            if (dist > Drone::SHIELD_R_INNER - radius &&
                dist < Drone::SHIELD_R_OUTER + radius)
            {
                const f32 b_angle = std::atan2(to_b.y, to_b.x);
                const f32 rel     = wrap_pi(b_angle - drone.heading);
                if (std::fabs(rel) <= Drone::SHIELD_SPAN * 0.5f) {
                    const f32 facet_span = Drone::SHIELD_SPAN / f32(Drone::SHIELD_FACETS);
                    int facet = int((rel + Drone::SHIELD_SPAN * 0.5f) / facet_span);
                    if (facet < 0) facet = 0;
                    if (facet >= Drone::SHIELD_FACETS) facet = Drone::SHIELD_FACETS - 1;

                    const Vec2 n = drone.facet_normal(facet);
                    if (sim_vel.dot(n) < 0.0f) {
                        sim_vel = reflect(sim_vel, n);
                        Vec2 dir_out = (dist < 1e-3f) ? Vec2{1,0} : to_b * (1.0f / dist);
                        sim_pos = drone.position + dir_out * (Drone::SHIELD_R_OUTER + radius + 1.0f);
                        ++bounces;
                        since_last = DOT_SPACING;
                    }
                }
            }
        }

        // ---- Terminate on core impact ------------------------------------
        {
            Vec2 to_c = sim_pos - drone.position;
            if (to_c.length() < Drone::CORE_RADIUS + radius) {
                alive_preview = false;
            }
        }

        // ---- Drop a dot --------------------------------------------------
        if (since_last >= DOT_SPACING) {
            since_last = 0.0f;
            const f32 dist_to_drone = (sim_pos - drone.position).length();
            const Color dot_color = (dist_to_drone < Drone::SHIELD_R_OUTER + 40.0f)
                ? COLOR_GHOST_DOT_HOT
                : COLOR_GHOST_DOT;
            r.draw_circle(sim_pos, 2.4f, dot_color, 6);
        }

        if (bounces >= MAX_BOUNCE + 2) {
            alive_preview = false;
        }
    }
}

} // namespace inc::game