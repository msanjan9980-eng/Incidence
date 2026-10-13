#include "game/world.hpp"
#include "render/renderer.hpp"
#include "platform/platform.hpp"

#include <cmath>
#include <cstdio>

namespace inc::game {

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

World::World() {
    reset();
}

void World::reset() {
    player_hp_    = 3;
    kills_        = 0;
    score_        = 0;
    current_room_ = 0;
    load_room(0);
}

void World::load_room(int index) {
    const Room& r = room_at(index);
    current_room_ = index;

    // ---- Walls ---------------------------------------------------------
    wall_count_ = r.wall_count;
    for (int i = 0; i < wall_count_; ++i) walls_[i] = r.walls[i];
    for (int i = wall_count_; i < 32; ++i) walls_[i] = Wall{};

    // ---- Enemies -------------------------------------------------------
    for (auto& t : turrets_) t.alive = false;
    turret_count_ = 0;
    for (int i = 0; i < r.enemy_count && turret_count_ < 8; ++i) {
        spawn_turret(r.enemies[i]);
    }

    // ---- Drone ---------------------------------------------------------
    drone_.position = r.drone_start;
    drone_.velocity = { 0.0f, 0.0f };
    drone_.heading  = -HALF_PI;
    drone_.spin     = 0.0f;

    // ---- Clear state ---------------------------------------------------
    for (auto& p : projectiles_) p.alive = false;
    for (auto& f : flashes_)     f.alive = false;

    room_clear_timer_ = -1.0f;

    std::fprintf(stderr, "[world] loaded room %d '%s' (%d enemies, %d walls)\n",
                 index, r.name, r.enemy_count, r.wall_count);
    std::fflush(stderr);
}

void World::spawn_turret(const EnemySpawn& spawn) {
    if (turret_count_ >= 8) return;
    Turret& t = turrets_[turret_count_++];
    t.type    = spawn.type;
    t.pos     = spawn.pos;
    t.alive   = true;
    t.hp      = 3;
    t.fire_timer    = 0.6f;
    t.fire_interval = 2.0f;
    t.bullet_speed  = 350.0f;

    switch (spawn.type) {
        case EnemyType::Lancer:
            t.wall_bounces_per_bullet = 4;
            t.bullet_core = COLOR_BULLET_CORE;
            t.bullet_glow = COLOR_BULLET_GLOW;
            break;
        case EnemyType::Banker:
            t.wall_bounces_per_bullet = 8;
            t.bullet_core = COLOR_BANKER_CORE;
            t.bullet_glow = with_alpha(COLOR_BANKER_EDGE, 160);
            t.fire_interval = 2.6f;    // slower fire, longer-lived bullet
            t.bullet_speed  = 320.0f;
            break;
        case EnemyType::Lead:
            t.wall_bounces_per_bullet = 4;
            t.bullet_core = COLOR_LEAD_CORE;
            t.bullet_glow = with_alpha(COLOR_LEAD_EDGE, 160);
            t.fire_interval = 2.2f;
            t.bullet_speed  = 420.0f;   // faster, since it predicts
            break;
    }
}

void World::spawn_bullet(Vec2 pos, Vec2 dir, f32 speed,
                         int wall_bounces,
                         Color core_c, Color glow_c) {
    for (auto& p : projectiles_) {
        if (!p.alive) {
            p.pos               = pos;
            p.vel               = dir * speed;
            p.age               = 0.0f;
            p.bounces           = 0;
            p.wall_bounces_left = wall_bounces;
            p.core_color        = core_c;
            p.glow_color        = glow_c;
            p.alive             = true;
            return;
        }
    }
}

void World::spawn_flash(Vec2 pos) {
    for (auto& f : flashes_) {
        if (!f.alive) {
            f.pos   = pos;
            f.age   = 0.0f;
            f.alive = true;
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

    drone_.step(dt,
                kc(platform::Key::Left),  kc(platform::Key::Right),
                kc(platform::Key::Up),    kc(platform::Key::Down),
                kc(platform::Key::Q),     kc(platform::Key::E));

    // ---- Turrets fire ---------------------------------------------------
    for (int i = 0; i < turret_count_; ++i) {
        Turret& t = turrets_[i];
        if (!t.alive) continue;

        Vec2 target;
        if (t.step(dt, drone_, target)) {
            Vec2 dir_v = target - t.pos;
            const f32 dist = dir_v.length();
            if (dist > 1e-3f) {
                Vec2 dir = dir_v * (1.0f / dist);
                spawn_bullet(t.pos + dir * (t.radius + 4.0f), dir,
                             t.bullet_speed,
                             t.wall_bounces_per_bullet,
                             t.bullet_core, t.bullet_glow);
            }
        }
    }

    // ---- Projectiles ----------------------------------------------------
    for (auto& p : projectiles_) {
        if (!p.alive) continue;
        p.step(dt);
        resolve_collisions(p);
    }

    // ---- Flashes --------------------------------------------------------
    for (auto& f : flashes_) {
        if (!f.alive) continue;
        f.age += dt;
        if (f.age >= HitFlash::LIFETIME) f.alive = false;
    }

    // ---- Room progression ------------------------------------------------
    if (room_clear_timer_ < 0.0f) {
        int alive_count = 0;
        for (int i = 0; i < turret_count_; ++i) {
            if (turrets_[i].alive) ++alive_count;
        }
        if (alive_count == 0) {
            room_clear_timer_ = 0.0f;
            std::fprintf(stderr, "[world] room cleared — advancing\n");
            std::fflush(stderr);
        }
    } else {
        room_clear_timer_ += dt;
        if (room_clear_timer_ >= 2.0f) {
            current_room_ = (current_room_ + 1) % ROOM_COUNT;
            load_room(current_room_);
        }
    }
}

// ---------------------------------------------------------------------------
// Collision helpers
// ---------------------------------------------------------------------------
bool World::collide_with_walls(Projectile& p) {
    if (p.wall_bounces_left <= 0) return false;

    for (int i = 0; i < wall_count_; ++i) {
        Vec2 closest;
        const f32 d = closest_point_on_segment(p.pos, walls_[i].a, walls_[i].b, closest);
        if (d >= p.radius) continue;

        Vec2 ab = walls_[i].b - walls_[i].a;
        Vec2 n  = ab.perp().normalized();
        if ((p.pos - closest).dot(n) < 0.0f) n = -n;
        if (p.vel.dot(n) >= 0.0f) continue;

        p.vel = reflect(p.vel, n);
        p.pos = closest + n * (p.radius + 0.5f);
        ++p.bounces;
        --p.wall_bounces_left;
        return true;
    }
    return false;
}

bool World::resolve_collisions(Projectile& p) {
    // ---- Screen boundary -----------------------------------------------
    if (p.wall_bounces_left > 0) {
        bool bounced = false;
        if (p.pos.x < p.radius)              { p.pos.x = p.radius;              p.vel.x = -p.vel.x; bounced = true; }
        else if (p.pos.x > VIRTUAL_W - p.radius) { p.pos.x = VIRTUAL_W - p.radius; p.vel.x = -p.vel.x; bounced = true; }
        if (p.pos.y < p.radius)              { p.pos.y = p.radius;              p.vel.y = -p.vel.y; bounced = true; }
        else if (p.pos.y > VIRTUAL_H - p.radius) { p.pos.y = VIRTUAL_H - p.radius; p.vel.y = -p.vel.y; bounced = true; }
        if (bounced) { ++p.bounces; --p.wall_bounces_left; }
    } else {
        if (p.pos.x < 0.0f || p.pos.x > VIRTUAL_W ||
            p.pos.y < 0.0f || p.pos.y > VIRTUAL_H)
        {
            p.alive = false;
            return true;
        }
    }

    // ---- Interior walls --------------------------------------------------
    collide_with_walls(p);

    // ---- Shield ---------------------------------------------------------
    {
        Vec2 to_bullet = p.pos - drone_.position;
        const f32 dist  = to_bullet.length();

        if (dist > Drone::SHIELD_R_INNER - p.radius &&
            dist < Drone::SHIELD_R_OUTER + p.radius)
        {
            const f32 bullet_angle = std::atan2(to_bullet.y, to_bullet.x);
            const f32 relative     = wrap_pi(bullet_angle - drone_.heading);

            if (std::fabs(relative) <= Drone::SHIELD_SPAN * 0.5f) {
                const f32 facet_span = Drone::SHIELD_SPAN / static_cast<f32>(Drone::SHIELD_FACETS);
                int facet = static_cast<int>((relative + Drone::SHIELD_SPAN * 0.5f) / facet_span);
                if (facet < 0) facet = 0;
                if (facet >= Drone::SHIELD_FACETS) facet = Drone::SHIELD_FACETS - 1;

                const Vec2 n = drone_.facet_normal(facet);
                if (p.vel.dot(n) < 0.0f) {
                    const f32 cos_i = clamp(-p.vel.normalized().dot(n), -1.0f, 1.0f);
                    last_incidence_deg_ = rad_to_deg(std::acos(cos_i));

                    p.vel = reflect(p.vel, n);

                    Vec2 dir_out = to_bullet * (1.0f / (dist < 1e-3f ? 1.0f : dist));
                    p.pos = drone_.position + dir_out * (Drone::SHIELD_R_OUTER + p.radius + 1.0f);

                    ++p.bounces;
                    spawn_flash(p.pos);
                    return false;
                }
            }
        }
    }

    // ---- Core ----------------------------------------------------------
    {
        Vec2 to_bullet = p.pos - drone_.position;
        if (to_bullet.length() < Drone::CORE_RADIUS + p.radius) {
            p.alive = false;
            player_hp_--;
            if (player_hp_ <= 0) reset();
            return true;
        }
    }

    // ---- Turrets -------------------------------------------------------
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
                const int b = (p.bounces < 6) ? p.bounces : 6;
                score_ += 100 * (1 << b);
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
    // ---- Frame ---------------------------------------------------------
    constexpr f32 W = VIRTUAL_W;
    constexpr f32 H = VIRTUAL_H;
    constexpr f32 T = 3.0f;
    r.draw_line({ T, T }, { W - T, T }, T, COLOR_WALL_EDGE);
    r.draw_line({ W - T, T }, { W - T, H - T }, T, COLOR_WALL_EDGE);
    r.draw_line({ W - T, H - T }, { T, H - T }, T, COLOR_WALL_EDGE);
    r.draw_line({ T, H - T }, { T, T }, T, COLOR_WALL_EDGE);

    // ---- Interior walls -------------------------------------------------
    for (int i = 0; i < wall_count_; ++i) {
        const Wall& w = walls_[i];
        r.draw_line(w.a, w.b, 12.0f, COLOR_WALL_FILL);
        r.draw_line(w.a, w.b,  5.0f, COLOR_WALL_EDGE);
    }

    // ---- Ghost previews -------------------------------------------------
    for (const auto& p : projectiles_) {
        p.draw_predicted(r, drone_, walls_, wall_count_);
    }

    // ---- Hit flashes ----------------------------------------------------
    for (const auto& f : flashes_) {
        if (!f.alive) continue;
        const f32 t = f.age / HitFlash::LIFETIME;
        const f32 radius  = 4.0f + t * 26.0f;
        const u8  a = u8((1.0f - t) * 255.0f);
        r.draw_ring(f.pos, radius, radius + 3.0f, with_alpha(COLOR_HIT_FLASH, a), 20);
    }

    // ---- Turrets --------------------------------------------------------
    for (int i = 0; i < turret_count_; ++i) {
        const Turret& t = turrets_[i];
        if (!t.alive) continue;

        const bool show_tele = t.telegraphing();

        // Compute the aim target for the telegraph line. We recompute rather
        // than cache so the line tracks the drone in real time.
        Vec2 tele_target = drone_.position;
        if (show_tele && t.type == EnemyType::Lead) {
            const f32 dist = (drone_.position - t.pos).length();
            f32 flight = dist / t.bullet_speed;
            for (int k = 0; k < 2; ++k) {
                tele_target = drone_.position + drone_.velocity * flight;
                flight = (tele_target - t.pos).length() / t.bullet_speed;
            }
        }

        t.draw(r, show_tele, tele_target);
    }

    // ---- Projectiles ----------------------------------------------------
    for (const auto& p : projectiles_) {
        p.draw(r);
    }

    // ---- Drone ----------------------------------------------------------
    drone_.draw(r);

    // ---- HUD ------------------------------------------------------------
    for (int i = 0; i < player_hp_; ++i) {
        const f32 x = 30.0f + f32(i) * 26.0f;
        const f32 y = 30.0f;
        r.draw_quad({ x,      y      }, { x + 20, y      },
                    { x + 20, y + 12 }, { x,      y + 12 }, COLOR_CORE_EDGE);
    }
    for (int i = 0; i < kills_; ++i) {
        const f32 x = 30.0f + f32(i) * 16.0f;
        const f32 y = 56.0f;
        r.draw_quad({ x,      y      }, { x + 10, y      },
                    { x + 10, y + 10 }, { x,      y + 10 }, COLOR_ENEMY_EDGE);
    }

    // Score bar
    {
        constexpr f32 BAR_X = 1580.0f;
        constexpr f32 BAR_Y = 30.0f;
        constexpr f32 BAR_W = 300.0f;
        constexpr f32 BAR_H = 12.0f;
        constexpr f32 SCORE_MAX = 20000.0f;

        r.draw_quad({ BAR_X,           BAR_Y           },
                    { BAR_X + BAR_W,   BAR_Y           },
                    { BAR_X + BAR_W,   BAR_Y + BAR_H   },
                    { BAR_X,           BAR_Y + BAR_H   },
                    with_alpha(COLOR_SCORE_BAR, 40));

        const f32 fill_w = BAR_W * clamp(f32(score_) / SCORE_MAX, 0.0f, 1.0f);
        if (fill_w > 0.5f) {
            r.draw_quad({ BAR_X,             BAR_Y           },
                        { BAR_X + fill_w,    BAR_Y           },
                        { BAR_X + fill_w,    BAR_Y + BAR_H   },
                        { BAR_X,             BAR_Y + BAR_H   },
                        COLOR_SCORE_BAR);
        }
        for (int i = 1; i <= 3; ++i) {
            const f32 tx = BAR_X + BAR_W * f32(i) * 0.25f;
            r.draw_line({ tx, BAR_Y - 3 }, { tx, BAR_Y + BAR_H + 3 },
                        1.5f, COLOR_SCORE_TICK);
        }
    }

    // Room indicators
    {
        constexpr f32 CX = VIRTUAL_W * 0.5f;
        constexpr f32 CY = 40.0f;
        constexpr f32 SPACING = 24.0f;
        constexpr f32 R = 6.0f;
        const f32 start_x = CX - f32(ROOM_COUNT - 1) * SPACING * 0.5f;
        for (int i = 0; i < ROOM_COUNT; ++i) {
            const Vec2 c { start_x + f32(i) * SPACING, CY };
            if (i < current_room_) {
                r.draw_circle(c, R, with_alpha(COLOR_CORE_EDGE, 140), 12);
            } else if (i == current_room_) {
                r.draw_circle(c, R, COLOR_CORE_EDGE, 12);
                r.draw_ring(c, R + 2.0f, R + 4.0f, COLOR_CORE_INNER, 16);
            } else {
                r.draw_ring(c, R - 1.0f, R, with_alpha(COLOR_CORE_EDGE, 100), 12);
            }
        }
    }

    // Room-clear pulse
    if (room_clear_timer_ >= 0.0f) {
        const f32 t = clamp(room_clear_timer_ / 2.0f, 0.0f, 1.0f);
        const f32 radius = 30.0f + t * 700.0f;
        const u8  a = u8((1.0f - t) * 200.0f);
        r.draw_ring(drone_.position, radius - 4.0f, radius + 4.0f,
                    with_alpha(COLOR_CORE_INNER, a), 40);
    }
}

} // namespace inc::game