#include "game/projectile.hpp"
#include "game/drone.hpp"
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

// ---------------------------------------------------------------------------
// Trajectory preview — the signature feature.
//
// Simulates a *copy* of this bullet forward in time, reflecting off walls
// and (using the drone's CURRENT heading) the shield. Draws a dotted line
// of every Nth position. Because the shield reflection uses the current
// heading, rotating with Q/E causes the predicted path to swing in real
// time. That is the moment the game teaches the player reflection geometry.
//
// The simulation matches the real simulation's dt and constants exactly,
// so the ghost line is a truthful prediction of what will happen if the
// player does not change anything.
// ---------------------------------------------------------------------------
void Projectile::draw_predicted(render::Renderer& r, const Drone& drone) const {
    if (!alive) return;

    // Local mutable copies. We never touch this projectile's actual state.
    Vec2 sim_pos = pos;
    Vec2 sim_vel = vel;

    constexpr f32 DT         = FIXED_DT;
    constexpr int MAX_STEPS  = 240;    // 2 seconds of preview
    constexpr int MAX_BOUNCE = 4;
    constexpr f32 DOT_SPACING = 34.0f; // virtual pixels between dots

    int  bounces       = 0;
    f32  since_last    = DOT_SPACING;  // draw a dot on the first step
    bool alive_preview = true;

    for (int i = 0; i < MAX_STEPS && alive_preview; ++i) {
        sim_pos += sim_vel * DT;
        since_last += sim_vel.length() * DT;

        // ---- Wall bounce --------------------------------------------------
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

        // ---- Shield reflection --------------------------------------------
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
                        since_last = DOT_SPACING;   // draw a dot right at the bounce
                    }
                }
            }
        }

        // ---- Core / turret-terminating checks -----------------------------
        {
            Vec2 to_c = sim_pos - drone.position;
            if (to_c.length() < Drone::CORE_RADIUS + radius) {
                // Impact on core — stop here.
                alive_preview = false;
            }
        }

        // ---- Draw a dot at the appropriate spacing ------------------------
        if (since_last >= DOT_SPACING) {
            since_last = 0.0f;
            // Hot dots near the shield, cool dots elsewhere.
            const f32 dist_to_drone = (sim_pos - drone.position).length();
            const Color dot_color = (dist_to_drone < Drone::SHIELD_R_OUTER + 40.0f)
                ? COLOR_GHOST_DOT_HOT
                : COLOR_GHOST_DOT;
            r.draw_circle(sim_pos, 2.2f, dot_color, 6);
        }

        if (bounces >= MAX_BOUNCE + 2) {
            alive_preview = false;
        }
    }
}

} // namespace inc::game