#include "game/world.hpp"
#include "render/renderer.hpp"
#include "platform/platform.hpp"

#include <cmath>

namespace inc::game {

// ---------------------------------------------------------------------------
// Construction / reset
// ---------------------------------------------------------------------------

World::World() {
    reset();
}

void World::reset() {
    // Reset player.
    drone_.position = { VIRTUAL_W * 0.5f, VIRTUAL_H * 0.5f };
    drone_.velocity = { 0.0f, 0.0f };
    drone_.heading  = -HALF_PI;
    drone_.spin     = 0.0f;

    player_hp_ = 3;
    kills_     = 0;

    // Clear all projectiles.
    for (auto& p : projectiles_) p.alive = false;

    // Clear turrets and re-place them.
    for (auto& t : turrets_) t.alive = false;
    turret_count_ = 0;

    spawn_turret({ 300.0f, 260.0f });
    spawn_turret({ VIRTUAL_W - 300.0f, 260.0f });
    spawn_turret({ VIRTUAL_W * 0.5f, VIRTUAL_H - 220.0f });
}

void World::spawn_turret(Vec2 pos) {
    if (turret_count_ >= 8) return;
    Turret& t = turrets_[turret_count_++];
    t.pos            = pos;
    t.hp             = 3;
    t.fire_timer     = 0.0f;
    t.fire_interval  = 2.0f;
    t.bullet_speed   = 350.0f;
    t.alive          = true;
}

void World::spawn_bullet(Vec2 pos, Vec2 dir, f32 speed) {
    for (auto& p : projectiles_) {
        if (!p.alive) {
            p.pos     = pos;
            p.vel     = dir * speed;
            p.age     = 0.0f;
            p.bounces = 0;
            p.alive   = true;
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------------

void World::step(f32 dt, const platform::InputState& input) {
    const auto kc = [&](platform::Key k) {
        return input.key_down[static_cast<int>(k)];
    };

    // ---- 1. Player --------------------------------------------------------
    drone_.step(dt,
                kc(platform::Key::Left),  kc(platform::Key::Right),
                kc(platform::Key::Up),    kc(platform::Key::Down),
                kc(platform::Key::Q),     kc(platform::Key::E));

    // ---- 2. Turrets fire --------------------------------------------------
    for (int i = 0; i < turret_count_; ++i) {
        Turret& t = turrets_[i];
        if (!t.alive) continue;

        if (t.step(dt, drone_.position)) {
            Vec2 to_drone = drone_.position - t.pos;
            const f32 dist = to_drone.length();
            if (dist > 1e-3f) {
                Vec2 dir = to_drone * (1.0f / dist);
                spawn_bullet(t.pos + dir * (t.radius + 4.0f), dir, t.bullet_speed);
            }
        }
    }

    // ---- 3. Projectiles ---------------------------------------------------
    for (auto& p : projectiles_) {
        if (!p.alive) continue;
        p.step(dt);
        resolve_collisions(p);
    }
}

// ---------------------------------------------------------------------------
// Collision + reflection
// ---------------------------------------------------------------------------

bool World::resolve_collisions(Projectile& p) {
    // ---- Wall bounces (up to MAX_WALL_BOUNCES) --------------------------
    if (p.bounces < Projectile::MAX_WALL_BOUNCES) {
        bool bounced = false;
        if (p.pos.x < p.radius)              { p.pos.x = p.radius;              p.vel.x = -p.vel.x; bounced = true; }
        else if (p.pos.x > VIRTUAL_W - p.radius) { p.pos.x = VIRTUAL_W - p.radius; p.vel.x = -p.vel.x; bounced = true; }
        if (p.pos.y < p.radius)              { p.pos.y = p.radius;              p.vel.y = -p.vel.y; bounced = true; }
        else if (p.pos.y > VIRTUAL_H - p.radius) { p.pos.y = VIRTUAL_H - p.radius; p.vel.y = -p.vel.y; bounced = true; }
        if (bounced) ++p.bounces;
    } else {
        // Out of wall bounces — kill on wall contact.
        if (p.pos.x < 0.0f || p.pos.x > VIRTUAL_W ||
            p.pos.y < 0.0f || p.pos.y > VIRTUAL_H)
        {
            p.alive = false;
            return true;
        }
    }

    // ---- Shield reflection ----------------------------------------------
    // The shield is an arc of flat facets between R_INNER and R_OUTER. A
    // bullet whose position falls in that annulus AND whose angle falls
    // within the arc's span is reflected off the facet it hit.
    {
        Vec2 to_bullet = p.pos - drone_.position;
        const f32 dist  = to_bullet.length();

        if (dist > Drone::SHIELD_R_INNER - p.radius &&
            dist < Drone::SHIELD_R_OUTER + p.radius)
        {
            const f32 bullet_angle = std::atan2(to_bullet.y, to_bullet.x);
            const f32 relative     = wrap_pi(bullet_angle - drone_.heading);

            if (std::fabs(relative) <= Drone::SHIELD_SPAN * 0.5f) {
                // Which facet did we hit?
                const f32 facet_span = Drone::SHIELD_SPAN / static_cast<f32>(Drone::SHIELD_FACETS);
                int facet = static_cast<int>((relative + Drone::SHIELD_SPAN * 0.5f) / facet_span);
                if (facet < 0) facet = 0;
                if (facet >= Drone::SHIELD_FACETS) facet = Drone::SHIELD_FACETS - 1;

                const Vec2 n = drone_.facet_normal(facet);

                // Only reflect bullets that are moving TOWARD the facet.
                if (p.vel.dot(n) < 0.0f) {
                    // incidence angle in degrees, for the HUD:
                    const f32 cos_i = clamp(-p.vel.normalized().dot(n), -1.0f, 1.0f);
                    last_incidence_deg_ = rad_to_deg(std::acos(cos_i));

                    // THE equation. v' = v - 2(v . n) n
                    p.vel = reflect(p.vel, n);

                    // Push the bullet back outside the shield so next tick
                    // doesn't re-collide with the same facet.
                    Vec2 dir_out = to_bullet * (1.0f / (dist < 1e-3f ? 1.0f : dist));
                    p.pos = drone_.position + dir_out * (Drone::SHIELD_R_OUTER + p.radius + 1.0f);

                    ++p.bounces;
                    return false;
                }
            }
        }
    }

    // ---- Core hit --------------------------------------------------------
    {
        Vec2 to_bullet = p.pos - drone_.position;
        if (to_bullet.length() < Drone::CORE_RADIUS + p.radius) {
            p.alive = false;
            player_hp_--;
            if (player_hp_ <= 0) {
                reset();
            }
            return true;
        }
    }

    // ---- Turret hits -----------------------------------------------------
    for (int i = 0; i < turret_count_; ++i) {
        Turret& t = turrets_[i];
        if (!t.alive) continue;
        Vec2 delta = p.pos - t.pos;
        if (delta.length() < t.radius + p.radius) {
            p.alive = false;
            t.hp--;
            if (t.hp <= 0) {
                t.alive = false;
                kills_++;
            }
            return true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void World::draw(render::Renderer& r) const {
    // ---- Room boundary (a thin frame around the virtual canvas) ---------
    constexpr f32 W = VIRTUAL_W;
    constexpr f32 H = VIRTUAL_H;
    constexpr f32 T = 3.0f;
    r.draw_line({ T, T }, { W - T, T }, T, COLOR_WALL_EDGE);
    r.draw_line({ W - T, T }, { W - T, H - T }, T, COLOR_WALL_EDGE);
    r.draw_line({ W - T, H - T }, { T, H - T }, T, COLOR_WALL_EDGE);
    r.draw_line({ T, H - T }, { T, T }, T, COLOR_WALL_EDGE);

    // ---- Turrets --------------------------------------------------------
    for (int i = 0; i < turret_count_; ++i) {
        turrets_[i].draw(r);
    }

    // ---- Projectiles ----------------------------------------------------
    for (const auto& p : projectiles_) {
        p.draw(r);
    }

    // ---- Drone ----------------------------------------------------------
    drone_.draw(r);

    // ---- HUD ------------------------------------------------------------
    // Top-left: HP as small cyan squares.
    for (int i = 0; i < player_hp_; ++i) {
        const f32 x = 30.0f + static_cast<f32>(i) * 26.0f;
        const f32 y = 30.0f;
        r.draw_quad({ x,     y     },
                    { x + 20, y     },
                    { x + 20, y + 12 },
                    { x,      y + 12 },
                    COLOR_CORE_EDGE);
    }

    // Below HP: kill count as red squares.
    for (int i = 0; i < kills_; ++i) {
        const f32 x = 30.0f + static_cast<f32>(i) * 16.0f;
        const f32 y = 56.0f;
        r.draw_quad({ x,     y     },
                    { x + 10, y     },
                    { x + 10, y + 10 },
                    { x,      y + 10 },
                    COLOR_ENEMY_EDGE);
    }
}

} // namespace inc::game