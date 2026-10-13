#include "game/world.hpp"
#include "render/renderer.hpp"
#include "platform/platform.hpp"
#include "audio/audio.hpp"
#include "game/save.hpp"

#include <cmath>
#include <cstdio>

namespace inc::game {

// XP value awarded per enemy type on kill.
static int xp_for_enemy(EnemyType type) {
    switch (type) {
        case EnemyType::Lancer: return 40;
        case EnemyType::Banker: return 55;
        case EnemyType::Lead:   return 65;
    }
    return 40;
}

World::World() { reset(); }

void World::reset() {
    character_       = Character{};
    player_hp_       = 3;
    kills_           = 0;
    score_           = 0;
    current_room_    = 0;
    warden_active_   = false;
    player_dead_     = false;
    warden_defeated_ = false;
    death_screen_timer_   = 0.0f;
    victory_screen_timer_ = 0.0f;
    levelup_banner_timer_ = 0.0f;
    levelup_banner_level_ = 1;
    load_room(0);
}

void World::jump_to_room(int index) {
    if (index < 0) index = 0;
    if (index >= ROOM_COUNT) index = ROOM_COUNT - 1;
    player_dead_     = false;
    warden_defeated_ = false;
    warden_active_   = false;
    levelup_banner_timer_ = 0.0f;
    load_room(index);
}

void World::load_room(int index) {
    player_hp_ = 3;

    const Room& r = room_at(index);
    current_room_ = index;
    warden_active_ = false;

    wall_count_ = r.wall_count;
    for (int i = 0; i < wall_count_; ++i) walls_[i] = r.walls[i];
    for (int i = wall_count_; i < 32; ++i) walls_[i] = Wall{};

    for (auto& t : turrets_) t.alive = false;
    turret_count_ = 0;
    for (int i = 0; i < r.enemy_count && turret_count_ < 8; ++i) {
        spawn_turret(r.enemies[i]);
    }

    drone_.position    = r.drone_start;
    drone_.velocity    = { 0.0f, 0.0f };
    drone_.heading     = -HALF_PI;
    drone_.spin        = 0.0f;
    drone_.dash_cd     = 0.0f;
    drone_.shield_span = character_.shield_span_rad();

    for (auto& p : projectiles_) p.alive = false;
    for (auto& f : flashes_)     f.alive = false;

    room_clear_timer_   = -1.0f;
    warden_death_timer_ = -1.0f;

    std::fprintf(stderr, "[world] loaded room %d '%s' (%d enemies, %d walls)\n",
                 index, r.name, r.enemy_count, r.wall_count);
    std::fflush(stderr);
}

void World::spawn_warden() {
    player_hp_ = 3;
    warden_.reset();
    warden_active_ = true;
    for (auto& p : projectiles_) p.alive = false;
    for (auto& f : flashes_)     f.alive = false;
    std::fprintf(stderr, "[world] WARDEN OF MIRROR engaged\n");
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
            t.fire_interval = 2.6f;
            t.bullet_speed  = 320.0f;
            break;
        case EnemyType::Lead:
            t.wall_bounces_per_bullet = 4;
            t.bullet_core = COLOR_LEAD_CORE;
            t.bullet_glow = with_alpha(COLOR_LEAD_EDGE, 160);
            t.fire_interval = 2.2f;
            t.bullet_speed  = 420.0f;
            break;
    }
}

void World::spawn_bullet(Vec2 pos, Vec2 dir, f32 speed,
                         int wall_bounces, Color core_c, Color glow_c) {
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
        if (!f.alive) { f.pos = pos; f.age = 0.0f; f.alive = true; return; }
    }
}

void World::award_kill_xp(EnemyType type) {
    const int amount = xp_for_enemy(type);
    const int gained = character_.add_xp(amount);
    if (gained > 0) {
        levelup_banner_timer_ = 2.0f;
        levelup_banner_level_ = character_.level;
        // Auto-save on level-up so progress is never lost.
        save_game("save.sav", character_, current_room_);
    }
}

// ---------------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------------

void World::step(f32 dt, const platform::InputState& input) {
    // Banner timer ticks even while paused-ish states run.
    if (levelup_banner_timer_ > 0.0f) {
        levelup_banner_timer_ -= dt;
        if (levelup_banner_timer_ < 0.0f) levelup_banner_timer_ = 0.0f;
    }

    if (player_dead_)     { death_screen_timer_   += dt; return; }
    if (warden_defeated_) { victory_screen_timer_ += dt; return; }

    const auto kc = [&](platform::Key k) {
        return input.key_down[static_cast<int>(k)];
    };

    drone_.step(dt, character_,
                kc(platform::Key::Left),  kc(platform::Key::Right),
                kc(platform::Key::Up),    kc(platform::Key::Down),
                kc(platform::Key::Q),     kc(platform::Key::E),
                kc(platform::Key::Space));

    if (warden_active_) {
        step_warden(dt);
    } else {
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
                                 t.bullet_speed, t.wall_bounces_per_bullet,
                                 t.bullet_core, t.bullet_glow);
                    audio::play_enemy_fire();
                }
            }
        }
    }

    for (auto& p : projectiles_) {
        if (!p.alive) continue;
        p.step(dt);
        resolve_collisions(p);
    }

    for (auto& f : flashes_) {
        if (!f.alive) continue;
        f.age += dt;
        if (f.age >= HitFlash::LIFETIME) f.alive = false;
    }

    if (warden_active_) {
        if (!warden_.alive && warden_death_timer_ < 0.0f) {
            warden_death_timer_ = 0.0f;
        }
        if (warden_death_timer_ >= 0.0f) {
            warden_death_timer_ += dt;
        }
        return;
    }

    if (room_clear_timer_ < 0.0f) {
        int alive_count = 0;
        for (int i = 0; i < turret_count_; ++i) {
            if (turrets_[i].alive) ++alive_count;
        }
        if (alive_count == 0) {
            room_clear_timer_ = 0.0f;
            std::fprintf(stderr, "[world] room cleared - advancing\n");
            std::fflush(stderr);
        }
    } else {
        room_clear_timer_ += dt;
        if (room_clear_timer_ >= 2.0f) {
            if (current_room_ == ROOM_COUNT - 1) {
                spawn_warden();
            } else {
                const int next = current_room_ + 1;
                load_room(next);
                // Auto-save on room advance.
                save_game("save.sav", character_, next);
            }
        }
    }
}

void World::step_warden(f32 dt) {
    if (!warden_.alive) return;

    const bool wants_fire = warden_.step(dt);

    if (wants_fire && warden_.alive) {
        const Vec2 drone_pos = drone_.position;
        const Vec2 to_drone = drone_pos - warden_.pos;

        switch (warden_.phase) {
            case 1: {
                const f32 d = to_drone.length();
                if (d > 1e-3f) {
                    Vec2 dir = to_drone * (1.0f / d);
                    spawn_bullet(warden_.pos + dir * (Warden::CORE_RADIUS + 6.0f),
                                 dir, 350.0f, 4,
                                 COLOR_CORE_INNER, COLOR_CORE_EDGE);
                    audio::play_enemy_fire();
                }
                break;
            }
            case 2: {
                const f32 d = to_drone.length();
                if (d > 1e-3f) {
                    f32 flight = d / 420.0f;
                    Vec2 predicted = drone_pos;
                    for (int k = 0; k < 2; ++k) {
                        predicted = drone_pos + drone_.velocity * flight;
                        flight = (predicted - warden_.pos).length() / 420.0f;
                    }
                    Vec2 base_dir = (predicted - warden_.pos).normalized();
                    const f32 spread = 0.14f;
                    const f32 angles[2] = { -spread, +spread };
                    for (f32 a : angles) {
                        Vec2 dir = base_dir.rotated(a);
                        spawn_bullet(warden_.pos + dir * (Warden::CORE_RADIUS + 6.0f),
                                     dir, 420.0f, 4,
                                     COLOR_LEAD_CORE, COLOR_LEAD_EDGE);
                    }
                    audio::play_enemy_fire();
                }
                break;
            }
            case 3: {
                constexpr int N = 8;
                for (int i = 0; i < N; ++i) {
                    const f32 a = TAU * f32(i) / f32(N) + warden_.rotation;
                    Vec2 dir = Vec2::from_angle(a);
                    spawn_bullet(warden_.pos + dir * (Warden::CORE_RADIUS + 6.0f),
                                 dir, 300.0f, 6,
                                 COLOR_ENEMY_CORE, COLOR_ENEMY_EDGE);
                }
                audio::play_enemy_fire();
                break;
            }
            default: break;
        }
    }

    if (warden_.phase_flash > 0.0f) {
        warden_.phase_flash -= dt;
        if (warden_.phase_flash < 0.0f) warden_.phase_flash = 0.0f;
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

bool World::resolve_warden_collisions(Projectile& p) {
    if (!warden_active_ || !warden_.alive) return false;

    const int facet = warden_.facet_hit_by(p.pos, p.radius);
    if (facet >= 0) {
        const Vec2 n = warden_.facet_normal(facet);
        if (p.vel.dot(n) < 0.0f) {
            p.vel = reflect(p.vel, n);
            p.pos += n * (p.radius + 1.0f);
            ++p.bounces;
            return false;
        }
    }

    Vec2 to_core = p.pos - warden_.pos;
    if (to_core.length() < Warden::CORE_RADIUS + p.radius) {
        p.alive = false;
        const int prev_phase = warden_.phase;
        warden_.damage(1);
        if (warden_.phase != prev_phase) audio::play_warden_phase();
        spawn_flash(p.pos);
        if (warden_.alive) {
            const int b = (p.bounces < 6) ? p.bounces : 6;
            score_ += 50 * (1 << b);
        } else {
            kills_++;
            score_ += 5000;
            warden_defeated_ = true;
            audio::play_victory();
            const int gained = character_.add_xp(2500);
            if (gained > 0) {
                levelup_banner_timer_ = 2.0f;
                levelup_banner_level_ = character_.level;
            }
            save_game("save.sav", character_, current_room_);
            std::fprintf(stderr, "[world] WARDEN DEFEATED\n");
            std::fflush(stderr);
        }
        return true;
    }

    return false;
}

bool World::resolve_collisions(Projectile& p) {
    // ---- Screen boundary ----------------------------------------------
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

    collide_with_walls(p);
    if (resolve_warden_collisions(p)) return true;

    // ---- Shield --------------------------------------------------------
    {
        Vec2 to_bullet = p.pos - drone_.position;
        const f32 dist  = to_bullet.length();
        if (dist > Drone::SHIELD_R_INNER - p.radius &&
            dist < Drone::SHIELD_R_OUTER + p.radius)
        {
            const f32 bullet_angle = std::atan2(to_bullet.y, to_bullet.x);
            const f32 relative     = wrap_pi(bullet_angle - drone_.heading);
            const f32 half_span    = drone_.shield_span * 0.5f;

            if (std::fabs(relative) <= half_span) {
                const f32 facet_span = drone_.shield_span / f32(Drone::SHIELD_FACETS);
                int facet = int((relative + half_span) / facet_span);
                if (facet < 0) facet = 0;
                if (facet >= Drone::SHIELD_FACETS) facet = Drone::SHIELD_FACETS - 1;

                const Vec2 n = drone_.facet_normal(facet);
                if (p.vel.dot(n) < 0.0f) {
                    const f32 cos_i = clamp(-p.vel.normalized().dot(n), -1.0f, 1.0f);
                    last_incidence_deg_ = rad_to_deg(std::acos(cos_i));
                    audio::play_shield_reflect(last_incidence_deg_);

                    p.vel = reflect(p.vel, n);

                    // CHARGE stat multiplies the reflected bullet's speed.
                    p.vel *= character_.reflect_multiplier();
                    // Cap to avoid runaway acceleration from repeated hits.
                    const f32 out_speed = p.vel.length();
                    constexpr f32 MAX_OUT = 2200.0f;
                    if (out_speed > MAX_OUT) p.vel = p.vel * (MAX_OUT / out_speed);

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
            audio::play_player_hit();
            if (player_hp_ <= 0) player_dead_ = true;
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
                audio::play_enemy_death();
                const int b = (p.bounces < 6) ? p.bounces : 6;
                score_ += 100 * (1 << b);
                award_kill_xp(t.type);
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
    r.set_layer(render::Renderer::Layer::Alpha);

    constexpr f32 W = VIRTUAL_W;
    constexpr f32 H = VIRTUAL_H;
    constexpr f32 T = 3.0f;
    r.draw_line({ T, T }, { W - T, T }, T, COLOR_WALL_EDGE);
    r.draw_line({ W - T, T }, { W - T, H - T }, T, COLOR_WALL_EDGE);
    r.draw_line({ W - T, H - T }, { T, H - T }, T, COLOR_WALL_EDGE);
    r.draw_line({ T, H - T }, { T, T }, T, COLOR_WALL_EDGE);

    for (int i = 0; i < wall_count_; ++i) {
        const Wall& w = walls_[i];
        r.draw_line(w.a, w.b, 12.0f, COLOR_WALL_FILL);
        r.draw_line(w.a, w.b,  5.0f, COLOR_WALL_EDGE);
    }

    for (const auto& p : projectiles_) {
        p.draw_predicted(r, drone_, walls_, wall_count_);
    }

    for (const auto& f : flashes_) {
        if (!f.alive) continue;
        const f32 t = f.age / HitFlash::LIFETIME;
        const f32 radius = 4.0f + t * 26.0f;
        const u8  a = u8((1.0f - t) * 255.0f);
        r.draw_ring(f.pos, radius, radius + 3.0f, with_alpha(COLOR_HIT_FLASH, a), 20);
    }

    for (int i = 0; i < turret_count_; ++i) {
        const Turret& t = turrets_[i];
        if (!t.alive) continue;
        const bool show_tele = t.telegraphing();
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

    if (warden_active_) warden_.draw(r);
    for (const auto& p : projectiles_) p.draw(r);
    drone_.draw(r);

    // ---- Additive glow -------------------------------------------------
    r.set_layer(render::Renderer::Layer::Additive);

    for (const auto& p : projectiles_) {
        if (!p.alive) continue;
        r.draw_circle(p.pos, p.radius + 14.0f, with_alpha(p.glow_color, 45), 14);
    }
    r.draw_circle(drone_.position, Drone::CORE_RADIUS + 22.0f,
                  with_alpha(COLOR_CORE_EDGE, 30), 20);
    r.draw_circle(drone_.position, Drone::SHIELD_R_OUTER + 12.0f,
                  with_alpha(COLOR_FACET_EDGE, 14), 32);
    if (warden_active_) warden_.draw_glow(r);

    for (int i = 0; i < turret_count_; ++i) {
        const Turret& t = turrets_[i];
        if (!t.alive) continue;
        Color glow = COLOR_ENEMY_EDGE;
        switch (t.type) {
            case EnemyType::Lancer: glow = COLOR_ENEMY_EDGE;  break;
            case EnemyType::Banker: glow = COLOR_BANKER_EDGE; break;
            case EnemyType::Lead:   glow = COLOR_LEAD_EDGE;   break;
        }
        r.draw_circle(t.pos, t.radius + 16.0f, with_alpha(glow, 30), 22);
    }

    // ---- HUD -----------------------------------------------------------
    r.set_layer(render::Renderer::Layer::Alpha);

    // HP bars (top-left).
    for (int i = 0; i < player_hp_; ++i) {
        const f32 x = 30.0f + f32(i) * 26.0f;
        const f32 y = 30.0f;
        r.draw_quad({ x,      y      }, { x + 20, y      },
                    { x + 20, y + 12 }, { x,      y + 12 }, COLOR_CORE_EDGE);
    }

    // Kills (below HP).
    for (int i = 0; i < kills_ && i < 20; ++i) {
        const f32 x = 30.0f + f32(i) * 16.0f;
        const f32 y = 56.0f;
        r.draw_quad({ x,      y      }, { x + 10, y      },
                    { x + 10, y + 10 }, { x,      y + 10 }, COLOR_ENEMY_EDGE);
    }

    // ---- Level + XP bar (below kills) ---------------------------------
    {
        const f32 LVL_X = 30.0f;
        const f32 LVL_Y = 86.0f;

        char lv_buf[16];
        std::snprintf(lv_buf, sizeof(lv_buf), "LV %d", character_.level);
        r.draw_text({ LVL_X, LVL_Y }, lv_buf, 2.2f, COLOR_CORE_INNER);

        // XP bar to the right of "LV N".
        const f32 bar_x = LVL_X + 90.0f;
        const f32 bar_y = LVL_Y + 4.0f;
        const f32 bar_w = 160.0f;
        const f32 bar_h = 10.0f;

        r.draw_quad({ bar_x,         bar_y         },
                    { bar_x + bar_w, bar_y         },
                    { bar_x + bar_w, bar_y + bar_h },
                    { bar_x,         bar_y + bar_h },
                    with_alpha(COLOR_CORE_EDGE, 40));

        const f32 fill = bar_w * character_.xp_progress();
        if (fill > 0.5f) {
            r.draw_quad({ bar_x,        bar_y         },
                        { bar_x + fill, bar_y         },
                        { bar_x + fill, bar_y + bar_h },
                        { bar_x,        bar_y + bar_h },
                        COLOR_CORE_EDGE);
        }

        // Stat-points pulse indicator if any available.
        if (character_.stat_points > 0) {
            const f32 pulse = 0.5f + 0.5f * std::sin(platform::time_seconds() * 6.0f);
            const Vec2 c { bar_x + bar_w + 20.0f, bar_y + bar_h * 0.5f };
            r.draw_circle(c, 6.0f + pulse * 2.0f,
                          with_alpha(COLOR_CORE_INNER,
                                     u8(180 + 75 * pulse)), 14);
            r.draw_ring(c, 9.0f, 11.0f,
                        with_alpha(COLOR_CORE_EDGE,
                                   u8(120 + 100 * pulse)), 16);
        }
    }

    // ---- Score bar (top-right) -----------------------------------------
    {
        constexpr f32 BAR_X = 1580.0f;
        constexpr f32 BAR_Y = 30.0f;
        constexpr f32 BAR_W = 300.0f;
        constexpr f32 BAR_H = 12.0f;
        const f32 SCORE_MAX = f32(ROOM_COUNT + 1) * 40000.0f;

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

    // ---- Room indicator (top-center) -----------------------------------
    {
        constexpr f32 CX = VIRTUAL_W * 0.5f;
        constexpr f32 CY = 40.0f;
        const int DOTS = ROOM_COUNT + 1;
        const f32 SPACING = (DOTS <= 6) ? 24.0f : 16.0f;
        constexpr f32 R = 6.0f;
        const f32 start_x = CX - f32(DOTS - 1) * SPACING * 0.5f;
        for (int i = 0; i < DOTS; ++i) {
            const Vec2 c { start_x + f32(i) * SPACING, CY };
            const bool is_boss_dot = (i == DOTS - 1);
            bool completed = false, current = false;
            if (is_boss_dot) {
                completed = warden_active_ && !warden_.alive;
                current   = warden_active_ && warden_.alive;
            } else {
                if (warden_active_) completed = true;
                else { completed = (i < current_room_); current = (i == current_room_); }
            }
            const Color dot_color = is_boss_dot ? COLOR_ENEMY_EDGE : COLOR_CORE_EDGE;
            if (completed)      r.draw_circle(c, R, with_alpha(dot_color, 140), 12);
            else if (current)   { r.draw_circle(c, R, dot_color, 12);
                                  r.draw_ring(c, R + 2.0f, R + 4.0f, dot_color, 16); }
            else                r.draw_ring(c, R - 1.0f, R, with_alpha(dot_color, 100), 12);
        }
    }

    // ---- Warden HP bar --------------------------------------------------
    if (warden_active_ && warden_.alive) {
        constexpr f32 BAR_W = 700.0f;
        constexpr f32 BAR_H = 18.0f;
        constexpr f32 BAR_X = (VIRTUAL_W - BAR_W) * 0.5f;
        constexpr f32 BAR_Y = VIRTUAL_H - 60.0f;

        r.draw_quad({ BAR_X,         BAR_Y         },
                    { BAR_X + BAR_W, BAR_Y         },
                    { BAR_X + BAR_W, BAR_Y + BAR_H },
                    { BAR_X,         BAR_Y + BAR_H },
                    with_alpha(COLOR_ENEMY_EDGE, 60));

        const f32 fill_w = BAR_W * clamp(f32(warden_.hp) / f32(warden_.hp_max), 0.0f, 1.0f);
        if (fill_w > 0.5f) {
            const f32 hp_ratio = f32(warden_.hp) / f32(warden_.hp_max);
            const Color fill = mix(COLOR_ENEMY_EDGE, COLOR_ENEMY_CORE, hp_ratio);
            r.draw_quad({ BAR_X,           BAR_Y         },
                        { BAR_X + fill_w,  BAR_Y         },
                        { BAR_X + fill_w,  BAR_Y + BAR_H },
                        { BAR_X,           BAR_Y + BAR_H },
                        fill);
        }
        for (int i = 1; i <= 2; ++i) {
            const f32 tx = BAR_X + BAR_W * f32(i) / 3.0f;
            r.draw_line({ tx, BAR_Y - 4 }, { tx, BAR_Y + BAR_H + 4 },
                        2.0f, COLOR_ENEMY_CORE);
        }
    }

    // ---- Room-clear pulse ----------------------------------------------
    if (!warden_active_ && room_clear_timer_ >= 0.0f) {
        const f32 t = clamp(room_clear_timer_ / 2.0f, 0.0f, 1.0f);
        const f32 radius = 30.0f + t * 700.0f;
        const u8  a = u8((1.0f - t) * 200.0f);
        r.draw_ring(drone_.position, radius - 4.0f, radius + 4.0f,
                    with_alpha(COLOR_CORE_INNER, a), 40);
    }

    // ---- Level-up banner -----------------------------------------------
    if (levelup_banner_timer_ > 0.0f) {
        const f32 t = 1.0f - (levelup_banner_timer_ / 2.0f);   // 0 -> 1 over 2 s
        const f32 alpha = clamp(1.0f - t * t, 0.0f, 1.0f);      // fade out

        const f32 cx = VIRTUAL_W * 0.5f;
        const f32 cy = VIRTUAL_H * 0.42f;

        char buf[32];
        std::snprintf(buf, sizeof(buf), "LEVEL %d", levelup_banner_level_);

        r.set_layer(render::Renderer::Layer::Alpha);
        r.draw_text_centered({ cx, cy }, buf, 7.5f,
                             with_alpha(COLOR_CORE_INNER, u8(alpha * 255.0f)));
        r.draw_text_centered({ cx, cy + 90.0f }, "+1 STAT POINT",
                             2.4f,
                             with_alpha(COLOR_FACET_EDGE, u8(alpha * 220.0f)));

        r.set_layer(render::Renderer::Layer::Additive);
        r.draw_circle({ cx, cy + 30.0f }, 320.0f,
                      with_alpha(COLOR_CORE_EDGE, u8(alpha * 40.0f)), 40);
        r.set_layer(render::Renderer::Layer::Alpha);
    }
}

} // namespace inc::game