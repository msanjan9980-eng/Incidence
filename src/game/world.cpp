#include "game/world.hpp"
#include "render/renderer.hpp"
#include "render/icons.hpp"
#include "platform/platform.hpp"
#include "audio/audio.hpp"
#include "game/save.hpp"
#include "game/room.hpp"
#include "render/drone_fx.hpp"
#include "game/profile.hpp"
#include "render/levelup_fx.hpp"

#include <cmath>
#include <cstdio>

namespace inc::game {

static Vec2 door_center(u8 side) {
    constexpr f32 W = VIRTUAL_W;
    constexpr f32 H = VIRTUAL_H;
    switch (side) {
        case 0: return { W * 0.5f, 0.0f };
        case 1: return { W,        H * 0.5f };
        case 2: return { W * 0.5f, H };
        case 3: return { 0.0f,     H * 0.5f };
    }
    return { 0.0f, 0.0f };
}

static Vec2 door_spawn(u8 side) {
    constexpr f32 W = VIRTUAL_W;
    constexpr f32 H = VIRTUAL_H;
    constexpr f32 INSET = 150.0f;
    switch (side) {
        case 0: return { W * 0.5f, INSET };
        case 1: return { W - INSET, H * 0.5f };
        case 2: return { W * 0.5f, H - INSET };
        case 3: return { INSET,    H * 0.5f };
    }
    return { W * 0.5f, H * 0.5f };
}

static u8 door_opposite(u8 side) {
    switch (side) {
        case 0: return 2;
        case 1: return 3;
        case 2: return 0;
        case 3: return 1;
    }
    return side;
}

static bool door_trigger(const Door& d, Vec2 p) {
    constexpr f32 EDGE  = 130.0f;
    constexpr f32 WIDTH = 130.0f;
    const f32 cx = VIRTUAL_W * 0.5f;
    const f32 cy = VIRTUAL_H * 0.5f;
    switch (d.side) {
        case 0: return p.y < EDGE && std::fabs(p.x - cx) < WIDTH;
        case 1: return p.x > VIRTUAL_W - EDGE && std::fabs(p.y - cy) < WIDTH;
        case 2: return p.y > VIRTUAL_H - EDGE && std::fabs(p.x - cx) < WIDTH;
        case 3: return p.x < EDGE && std::fabs(p.y - cy) < WIDTH;
    }
    return false;
}

static f32 reflect_speed_mult(const Character& ch) {
    f32 m = 1.0f;
    if (ch.has_skill(SkillId::HeavyShot))   m *= 0.75f;
    if (ch.has_skill(SkillId::CurvedFacet)) m *= 1.10f;
    return m;
}

static f32 damage_mult(const Character& ch, int bounces) {
    f32 m = 1.0f;
    if (ch.has_skill(SkillId::PerfectReflection)) {
        for (int i = 0; i < bounces; ++i) m *= 1.15f;
    }
    if (ch.has_skill(SkillId::HeavyShot)) m *= 1.60f;
    if (ch.has_skill(SkillId::KleinLock) && bounces >= 2) m *= 1.50f;
    return m;
}

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
    rooms_cleared_   = 0;
    rooms_visited_   = 0;
    door_cooldown_   = 0.0f;
    room_clear_pulse_= -1.0f;
    warden_death_timer_ = -1.0f;
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
    pending_log_trigger_  = -1;
    effects_.reset();
    load_room(0);
}

void World::jump_to_room(int index) {
    if (index < 0) index = 0;
    if (index >= ROOM_COUNT) index = ROOM_COUNT - 1;
    player_dead_     = false;
    warden_defeated_ = false;
    warden_active_   = false;
    levelup_banner_timer_ = 0.0f;
    door_cooldown_   = 0.0f;
    load_room(index);
}

void World::travel_to_room(int index) {
    jump_to_room(index);
    drone_.position = { VIRTUAL_W * 0.5f, VIRTUAL_H * 0.5f };
    door_cooldown_  = 0.5f;
}

void World::load_room(int index) {
    player_hp_ = character_.max_hp();

    const Room& r = room_at(index);
    current_room_ = index;
    rooms_visited_ |= u16(1u << index);
    warden_active_ = false;

    wall_count_ = r.wall_count;
    for (int i = 0; i < wall_count_; ++i) walls_[i] = r.walls[i];
    for (int i = wall_count_; i < 32; ++i) walls_[i] = Wall{};

    for (auto& t : turrets_) t.alive = false;
    turret_count_ = 0;

    const bool already_cleared = (rooms_cleared_ & (1u << index)) != 0;
    if (!already_cleared) {
        for (int i = 0; i < r.enemy_count && turret_count_ < 8; ++i) {
            spawn_turret(r.enemies[i]);
        }
    }

    drone_.position    = r.drone_start;
    drone_.velocity    = { 0.0f, 0.0f };
    drone_.heading     = -HALF_PI;
    drone_.spin        = 0.0f;
    drone_.dash_cd     = 0.0f;
    drone_.shield_span = character_.shield_span_rad();

    // Landing pulse - drone drops into a new sector
    game_particles_.clear();
    game_particles_.burst(drone_.position, 24,
                          120.0f, 340.0f, 0.5f, 1.0f,
                          0.0f, TAU, 1.8f, 0.2f,
                          COLOR_CORE_INNER, COLOR_CORE_EDGE,
                          fx::ParticleKind::Spark, 160.0f, 0.94f, 0.0f);
    game_particles_.burst(drone_.position, 8,
                          40.0f, 120.0f, 0.4f, 0.8f,
                          0.0f, TAU, 2.4f, 0.2f,
                          COLOR_CORE_EDGE, COLOR_FACET_EDGE,
                          fx::ParticleKind::Ember, 80.0f, 0.96f, 2.0f);
    effects_.spawn_ring(drone_.position, 90.0f, COLOR_CORE_INNER, 0.4f);

    for (auto& p : projectiles_) p.alive = false;
    for (auto& f : flashes_)     f.alive = false;
    game_particles_.clear();

    room_clear_pulse_   = -1.0f;
    warden_death_timer_ = -1.0f;

    std::fprintf(stderr, "[world] loaded room %d '%s' (%d enemies, %d walls)\n",
                 index, r.name, r.enemy_count, r.wall_count);
    std::fflush(stderr);
}

void World::spawn_warden() {
    player_hp_ = character_.max_hp();
    warden_.reset();
    warden_active_ = true;
    for (auto& p : projectiles_) p.alive = false;
    for (auto& f : flashes_)     f.alive = false;
    pending_log_trigger_ = 9;
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
        profile_save_active(character_, current_room_, rooms_visited_);
        effects_.add_flash(COLOR_CORE_INNER, 0.30f);
        effects_.add_hitstop(0.06f);
        const f32 lev_cy = f32(VIRTUAL_H) * 0.42f;
        game_particles_.burst({40.0f, lev_cy}, 60,
                              260.0f, 640.0f, 0.9f, 1.6f,
                              0.0f, PI * 0.8f, 2.4f, 0.2f,
                              COLOR_CORE_INNER, COLOR_CORE_EDGE,
                              fx::ParticleKind::Ember, 220.0f, 0.94f, 4.0f);
        game_particles_.burst({f32(VIRTUAL_W) - 40.0f, lev_cy}, 60,
                              260.0f, 640.0f, 0.9f, 1.6f,
                              PI, PI * 0.8f, 2.4f, 0.2f,
                              COLOR_CORE_INNER, COLOR_CORE_EDGE,
                              fx::ParticleKind::Ember, 220.0f, 0.94f, 4.0f);
    }
}

void World::attempt_door_transition() {
    if (door_cooldown_ > 0.0f) return;
    if (warden_active_) return;

    const Room& r = room_at(current_room_);
    for (int i = 0; i < r.door_count; ++i) {
        const Door& d = r.doors[i];
        if (!door_trigger(d, drone_.position)) continue;

        const bool forward = (d.target_room > current_room_) ||
                             (d.target_room < 0);
        if (forward && !(rooms_cleared_ & (1u << current_room_))) {
            return;
        }

        if (d.target_room < 0) {
            spawn_warden();
            door_cooldown_ = 0.5f;
            return;
        }

        const int  next       = d.target_room;
        const u8   entry_side = door_opposite(d.side);
        jump_to_room(next);
        drone_.position = door_spawn(entry_side);
        door_cooldown_  = 0.6f;
        return;
    }
}

void World::step(f32 dt, const platform::InputState& input) {
    game_particles_.update(dt);

    effects_.update(dt);
    if (effects_.hitstop > 0.0f) return;

    if (levelup_banner_timer_ > 0.0f) {
        levelup_banner_timer_ -= dt;
        if (levelup_banner_timer_ < 0.0f) levelup_banner_timer_ = 0.0f;
    }
    if (door_cooldown_ > 0.0f) door_cooldown_ -= dt;
    if (room_clear_pulse_ >= 0.0f) {
        room_clear_pulse_ += dt;
        if (room_clear_pulse_ >= 2.0f) room_clear_pulse_ = -1.0f;
    }

    if (player_dead_)     { death_screen_timer_   += dt; return; }
    if (warden_defeated_) { victory_screen_timer_ += dt; return; }

    const auto kc = [&](platform::Key k) {
        return input.key_down[static_cast<int>(k)];
    };

    if (character_.has_skill(SkillId::FrameShift) &&
        input.key_pressed[static_cast<int>(platform::Key::F)]) {
        drone_.heading = wrap_pi(drone_.heading + PI);
    }

    if (character_.has_skill(SkillId::VoidFacet) &&
        input.key_pressed[static_cast<int>(platform::Key::V)]) {
        f32 best_d2 = 400.0f * 400.0f;
        Projectile* best = nullptr;
        for (auto& p : projectiles_) {
            if (!p.alive) continue;
            const f32 d2 = (p.pos - drone_.position).length_sq();
            if (d2 < best_d2) { best_d2 = d2; best = &p; }
        }
        if (best) {
            spawn_flash(best->pos);
            // reflect spark - cyan burst at the shield impact point
            game_particles_.burst(best->pos, 14,
                                  180.0f, 420.0f, 0.18f, 0.35f,
                                  0.0f, TAU, 1.6f, 0.2f,
                                  COLOR_CORE_INNER, COLOR_CORE_EDGE,
                                  fx::ParticleKind::Spark, 60.0f, 0.93f, 0.0f);
            best->alive = false;
        }
    }

    drone_.step(dt, character_,
                kc(platform::Key::Left),  kc(platform::Key::Right),
                kc(platform::Key::Up),    kc(platform::Key::Down),
                kc(platform::Key::Q),     kc(platform::Key::E),
                kc(platform::Key::Space));

    attempt_door_transition();

    if (character_.has_skill(SkillId::Singularity)) {
        for (auto& p : projectiles_) {
            if (!p.alive) continue;
            Vec2 to_drone = drone_.position - p.pos;
            const f32 d = to_drone.length();
            if (d > 1.0f && d < 600.0f) {
                const f32 strength = 40.0f * (1.0f - d / 600.0f);
                p.vel += to_drone * (strength * dt / d);
            }
        }
    }

    if (character_.has_skill(SkillId::GravityWell)) {
        for (auto& p : projectiles_) {
            if (!p.alive || p.bounces == 0) continue;
            f32 best_d2 = 700.0f * 700.0f;
            const Turret* best = nullptr;
            for (int i = 0; i < turret_count_; ++i) {
                if (!turrets_[i].alive) continue;
                const f32 d2 = (turrets_[i].pos - p.pos).length_sq();
                if (d2 < best_d2) { best_d2 = d2; best = &turrets_[i]; }
            }
            if (best) {
                Vec2 to_t = best->pos - p.pos;
                const f32 d = to_t.length();
                if (d > 1.0f) p.vel += to_t * (120.0f * dt / d);
            }
        }
    }

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
        if (!warden_.alive && warden_death_timer_ < 0.0f) warden_death_timer_ = 0.0f;
        if (warden_death_timer_ >= 0.0f) warden_death_timer_ += dt;
        return;
    }

    if (!(rooms_cleared_ & (1u << current_room_))) {
        int alive_count = 0;
        for (int i = 0; i < turret_count_; ++i) {
            if (turrets_[i].alive) ++alive_count;
        }
        if (alive_count == 0) {
            rooms_cleared_ |= u16(1u << current_room_);
            room_clear_pulse_ = 0.0f;
            if (current_room_ < ROOM_COUNT) {
                pending_log_trigger_ = current_room_;
            }
            std::fprintf(stderr, "[world] room %d cleared\n", current_room_);
            std::fflush(stderr);
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
        game_particles_.burst(p.pos, 5,
                              80.0f, 200.0f, 0.12f, 0.25f,
                              0.0f, TAU, 1.0f, 0.1f,
                              COLOR_CORE_EDGE, COLOR_FACET_EDGE,
                              fx::ParticleKind::Spark, 0.0f, 0.92f, 0.0f);
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
        if (warden_.phase != prev_phase) {
            game_particles_.burst(warden_.pos, 60,
                                  240.0f, 640.0f, 0.6f, 1.3f,
                                  0.0f, TAU, 2.4f, 0.2f,
                                  COLOR_ENEMY_CORE, COLOR_ENEMY_EDGE,
                                  fx::ParticleKind::Ember, 180.0f, 0.94f, 4.0f);
            audio::play_warden_phase();
            effects_.add_flash(COLOR_ENEMY_CORE, 0.35f);
            effects_.add_shake(22.0f, 0.55f);
            effects_.add_hitstop(0.08f);
            effects_.spawn_burst(warden_.pos, 26, 480.0f, COLOR_ENEMY_CORE);
            effects_.spawn_ring(warden_.pos, 180.0f, COLOR_ENEMY_EDGE, 0.7f);
        }
        spawn_flash(p.pos);
        if (warden_.alive) {
            const int b = (p.bounces < 6) ? p.bounces : 6;
            score_ += 50 * (1 << b);
        } else {
            kills_++;
            score_ += 5000;
            warden_defeated_ = true;
            pending_log_trigger_ = 10;
            audio::play_victory();
            effects_.add_flash(COLOR_CORE_INNER, 0.9f);
            effects_.add_shake(34.0f, 1.1f);
            effects_.add_hitstop(0.12f);
            effects_.spawn_burst(warden_.pos, 60, 720.0f, COLOR_ENEMY_CORE);
            effects_.spawn_ring(warden_.pos, 320.0f, COLOR_CORE_INNER, 1.2f);
            // Warden death - layered particle cascade
            game_particles_.burst(warden_.pos, 120,
                                  280.0f, 820.0f, 1.0f, 2.2f,
                                  0.0f, TAU, 3.2f, 0.2f,
                                  COLOR_ENEMY_CORE, COLOR_CORE_EDGE,
                                  fx::ParticleKind::Ember, 220.0f, 0.94f, 6.0f);
            game_particles_.burst(warden_.pos, 40,
                                  180.0f, 460.0f, 0.7f, 1.4f,
                                  0.0f, TAU, 2.0f, 0.15f,
                                  COLOR_CORE_INNER, COLOR_CORE_EDGE,
                                  fx::ParticleKind::Spark, 120.0f, 0.93f, 0.0f);
            for (int k = 0; k < 6; ++k) {
                const f32 a = TAU * f32(k) / 6.0f;
                const Vec2 origin {warden_.pos.x + std::cos(a) * 40.0f,
                                   warden_.pos.y + std::sin(a) * 40.0f};
                game_particles_.burst(origin, 14,
                                      200.0f, 520.0f, 0.6f, 1.2f,
                                      a, PI * 0.5f, 2.4f, 0.2f,
                                      COLOR_ENEMY_EDGE, COLOR_CORE_EDGE,
                                      fx::ParticleKind::Ember, 200.0f, 0.94f, 4.0f);
            }
            const int gained = character_.add_xp(2500);
            if (gained > 0) {
                levelup_banner_timer_ = 2.0f;
                levelup_banner_level_ = character_.level;
            }
            profile_save_active(character_, current_room_, rooms_visited_);
            std::fprintf(stderr, "[world] WARDEN DEFEATED\n");
            std::fflush(stderr);
        }
        return true;
    }
    return false;
}

bool World::resolve_collisions(Projectile& p) {
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

                    effects_.spawn_reflect(p.pos, n, COLOR_CORE_INNER);
                    effects_.add_shake(4.5f, 0.15f);
                    p.vel = reflect(p.vel, n);
                    p.vel *= character_.reflect_multiplier() * reflect_speed_mult(character_);
                    const f32 out_speed = p.vel.length();
                    constexpr f32 MAX_OUT = 2200.0f;
                    if (out_speed > MAX_OUT) p.vel = p.vel * (MAX_OUT / out_speed);

                    Vec2 dir_out = to_bullet * (1.0f / (dist < 1e-3f ? 1.0f : dist));
                    p.pos = drone_.position + dir_out * (Drone::SHIELD_R_OUTER + p.radius + 1.0f);

                    ++p.bounces;
                    spawn_flash(p.pos);

                    if (character_.has_skill(SkillId::PrismFacet)) {
                        constexpr f32 spread = 0.35f;
                        const Vec2 v0 = p.vel.rotated(-spread);
                        const Vec2 v1 = p.vel.rotated(+spread);
                        const Vec2 saved_p = p.pos;
                        const int  saved_b = p.bounces;
                        const int  saved_w = p.wall_bounces_left;
                        const Vec2 extra[2] = { v0, v1 };
                        for (const Vec2& nv : extra) {
                            for (auto& q : projectiles_) {
                                if (!q.alive) {
                                    q.pos               = saved_p;
                                    q.vel               = nv;
                                    q.age               = 0.0f;
                                    q.bounces           = saved_b;
                                    q.wall_bounces_left = saved_w;
                                    q.core_color        = COLOR_CORE_INNER;
                                    q.glow_color        = COLOR_FACET_EDGE;
                                    q.alive             = true;
                                    break;
                                }
                            }
                        }
                    }
                    return false;
                }
            }
        }
    }

    {
        Vec2 to_bullet = p.pos - drone_.position;
        if (to_bullet.length() < Drone::CORE_RADIUS + p.radius) {
            p.alive = false;
            effects_.spawn_burst(p.pos, 14, 320.0f, COLOR_ENEMY_CORE);
            effects_.spawn_ring(p.pos, 90.0f, COLOR_ENEMY_EDGE, 0.45f);
            effects_.add_shake(16.0f, 0.45f);
            effects_.add_hitstop(0.06f);
            player_hp_--;
    // damage burst - red ember explosion from the drone
    game_particles_.burst(drone_.position, 44,
                          140.0f, 420.0f, 0.45f, 0.9f,
                          0.0f, TAU, 2.8f, 0.2f,
                          COLOR_ENEMY_CORE, COLOR_ENEMY_EDGE,
                          fx::ParticleKind::Ember, 200.0f, 0.92f, 5.0f);
            audio::play_player_hit();
            if (player_hp_ <= 0) player_dead_ = true;
            return true;
        }
    }

    for (int i = 0; i < turret_count_; ++i) {
        Turret& t = turrets_[i];
        if (!t.alive) continue;
        Vec2 delta = p.pos - t.pos;
        if (delta.length() < t.radius + p.radius) {
            p.alive = false;
            const f32 dmg = damage_mult(character_, p.bounces);
            const int hp_lost = (int(dmg) > 1) ? int(dmg) : 1;
            t.hp -= hp_lost;
            if (t.hp <= 0) {
                t.alive = false;
                kills_++;
            game_particles_.burst(t.pos, 44,
                                  180.0f, 540.0f, 0.5f, 1.1f,
                                  0.0f, TAU, 2.6f, 0.2f,
                                  COLOR_ENEMY_CORE, COLOR_ENEMY_EDGE,
                                  fx::ParticleKind::Ember, 200.0f, 0.93f, 5.0f);
            game_particles_.burst(t.pos, 12,
                                  120.0f, 320.0f, 0.3f, 0.6f,
                                  0.0f, TAU, 1.4f, 0.2f,
                                  COLOR_CORE_INNER, COLOR_CORE_EDGE,
                                  fx::ParticleKind::Spark, 120.0f, 0.94f, 0.0f);
                audio::play_enemy_death();
                const int b = (p.bounces < 6) ? p.bounces : 6;
                const int gained = 100 * (1 << b);
                score_ += gained;
                effects_.spawn_burst(t.pos, 22, 380.0f, COLOR_ENEMY_CORE);
                effects_.spawn_ring(t.pos, 120.0f, COLOR_ENEMY_EDGE);
                effects_.add_shake(8.0f, 0.28f);
                effects_.add_hitstop(0.05f);
                {
                    char buf[24];
                    std::snprintf(buf, sizeof(buf), "+%d x%d", gained, b);
                    effects_.add_label(t.pos, buf, COLOR_CORE_INNER);
                }
                award_kill_xp(t.type);
            }
            return true;
        }
    }
    return false;
}

void World::draw(render::Renderer& r) const {
    r.set_view_offset(effects_.shake_offset);
    r.set_layer(render::Renderer::Layer::Alpha);

    constexpr f32 W = VIRTUAL_W;
    constexpr f32 H = VIRTUAL_H;
    constexpr f32 T = 3.0f;
    r.draw_line({ T, T }, { W - T, T }, T, COLOR_WALL_EDGE);
    r.draw_line({ W - T, T }, { W - T, H - T }, T, COLOR_WALL_EDGE);
    r.draw_line({ W - T, H - T }, { T, H - T }, T, COLOR_WALL_EDGE);
    r.draw_line({ T, H - T }, { T, T }, T, COLOR_WALL_EDGE);

    const Room& cur = room_at(current_room_);
    for (int i = 0; i < cur.door_count; ++i) {
        const Door& d = cur.doors[i];
        const bool forward = (d.target_room > current_room_) ||
                             (d.target_room < 0);
        const bool locked  = forward && !(rooms_cleared_ & (1u << current_room_));
        const Color dc = locked
            ? with_alpha(COLOR_ENEMY_EDGE, 140)
            : with_alpha(COLOR_CORE_INNER, 220);
        const f32 thickness = locked ? 3.0f : 5.0f;

        constexpr f32 HALF = 120.0f;
        switch (d.side) {
            case 0:
                r.draw_line({ W*0.5f - HALF, 4.0f }, { W*0.5f + HALF, 4.0f }, thickness, dc);
                break;
            case 1:
                r.draw_line({ W - 4.0f, H*0.5f - HALF }, { W - 4.0f, H*0.5f + HALF }, thickness, dc);
                break;
            case 2:
                r.draw_line({ W*0.5f - HALF, H - 4.0f }, { W*0.5f + HALF, H - 4.0f }, thickness, dc);
                break;
            case 3:
                r.draw_line({ 4.0f, H*0.5f - HALF }, { 4.0f, H*0.5f + HALF }, thickness, dc);
                break;
        }
    }

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
    drone_fx::draw_reticle(r, drone_, character_, f32(platform::time_seconds()));
    game_particles_.draw(r);

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

    effects_.draw(r);
    r.set_view_offset({0.0f, 0.0f});
    effects_.draw_flash(r);

    r.set_layer(render::Renderer::Layer::Alpha);

    // ========================================================================
    // HUD - vitals, level/xp, score, room progress
    // ========================================================================

    // Low-HP warning: pulsing red border on all four edges
    if (player_hp_ <= 1 && !player_dead_) {
        const f32 wp = 0.5f + 0.5f * std::sin(platform::time_seconds() * 5.5f);
        const u8 wa = u8(60 + 90 * wp);
        const f32 th = 6.0f + wp * 4.0f;
        r.draw_quad({0, 0}, {VIRTUAL_W, 0}, {VIRTUAL_W, th}, {0, th},
                    with_alpha(COLOR_ENEMY_EDGE, wa));
        r.draw_quad({0, VIRTUAL_H - th}, {VIRTUAL_W, VIRTUAL_H - th},
                    {VIRTUAL_W, VIRTUAL_H}, {0, VIRTUAL_H},
                    with_alpha(COLOR_ENEMY_EDGE, wa));
        r.draw_quad({0, th}, {th, th}, {th, VIRTUAL_H - th}, {0, VIRTUAL_H - th},
                    with_alpha(COLOR_ENEMY_EDGE, wa));
        r.draw_quad({VIRTUAL_W - th, th}, {VIRTUAL_W, th},
                    {VIRTUAL_W, VIRTUAL_H - th}, {VIRTUAL_W - th, VIRTUAL_H - th},
                    with_alpha(COLOR_ENEMY_EDGE, wa));
    }

    // --- HP hearts: filled for alive slots, outlined for lost slots ---
    {
        const int max_hp = character_.max_hp();
        for (int i = 0; i < max_hp; ++i) {
            const f32 x = 42.0f + f32(i) * 34.0f;
            const f32 y = 44.0f;
            if (i < player_hp_) {
                r.set_layer(render::Renderer::Layer::Additive);
                r.draw_circle({x, y}, 16.0f,
                              with_alpha(COLOR_CORE_EDGE, 60), 16);
                r.set_layer(render::Renderer::Layer::Alpha);
                render::icon_heart(r, {x, y}, 26.0f, COLOR_CORE_INNER);
            } else {
                render::icon_heart(r, {x, y}, 24.0f,
                                   with_alpha(COLOR_CORE_EDGE, 70));
                render::icon_heart(r, {x, y}, 17.0f,
                                   with_alpha(COLOR_VOID, 255));
            }
        }
    }

    // --- Kills as bullets, with running count ---
    {
        const int shown = (kills_ < 20) ? kills_ : 20;
        for (int i = 0; i < shown; ++i) {
            const f32 x = 46.0f + f32(i) * 20.0f;
            const f32 y = 82.0f;
            render::icon_bullet(r, {x, y}, 12.0f, COLOR_ENEMY_EDGE);
        }
        if (kills_ > 0) {
            char kb[16];
            std::snprintf(kb, sizeof(kb), "x%d", kills_);
            r.draw_text({46.0f + f32(shown) * 20.0f + 8.0f, 74.0f},
                        kb, 2.0f, with_alpha(COLOR_ENEMY_EDGE, 220));
        }
    }

    // --- Level + XP bar with shimmer sweep ---
    {
        const f32 LX = 30.0f;
        const f32 LY = 112.0f;
        char lv_buf[16];
        std::snprintf(lv_buf, sizeof(lv_buf), "LV %d", character_.level);
        r.draw_text({LX, LY}, lv_buf, 2.6f, COLOR_CORE_INNER);

        const f32 bar_x = LX + 90.0f;
        const f32 bar_y = LY + 4.0f;
        const f32 bar_w = 220.0f;
        const f32 bar_h = 14.0f;

        r.draw_quad({bar_x, bar_y}, {bar_x + bar_w, bar_y},
                    {bar_x + bar_w, bar_y + bar_h}, {bar_x, bar_y + bar_h},
                    with_alpha(COLOR_CORE_EDGE, 30));
        r.draw_line({bar_x, bar_y}, {bar_x + bar_w, bar_y}, 1.0f,
                    with_alpha(COLOR_CORE_EDGE, 120));
        r.draw_line({bar_x, bar_y + bar_h}, {bar_x + bar_w, bar_y + bar_h}, 1.0f,
                    with_alpha(COLOR_CORE_EDGE, 120));

        const f32 fill = bar_w * character_.xp_progress();
        if (fill > 0.5f) {
            r.draw_quad({bar_x, bar_y}, {bar_x + fill, bar_y},
                        {bar_x + fill, bar_y + bar_h}, {bar_x, bar_y + bar_h},
                        COLOR_CORE_EDGE);
            const f32 sweep = std::fmod(f32(platform::time_seconds()) * 180.0f,
                                        bar_w + 80.0f) - 40.0f;
            if (sweep > 0.0f && sweep < fill + 40.0f) {
                r.set_layer(render::Renderer::Layer::Additive);
                r.draw_quad({bar_x + sweep - 30.0f, bar_y},
                            {bar_x + sweep + 8.0f, bar_y},
                            {bar_x + sweep + 8.0f, bar_y + bar_h},
                            {bar_x + sweep - 30.0f, bar_y + bar_h},
                            with_alpha(COLOR_CORE_INNER, 70));
                r.set_layer(render::Renderer::Layer::Alpha);
            }
        }

        char pct[16];
        std::snprintf(pct, sizeof(pct), "%d%%",
                      int(character_.xp_progress() * 100.0f));
        r.draw_text({bar_x + bar_w + 12.0f, LY}, pct, 2.0f,
                    with_alpha(COLOR_FACET_EDGE, 180));

        if (character_.stat_points > 0) {
            const Vec2 c {bar_x + bar_w + 90.0f, bar_y + bar_h * 0.5f};
            const f32 p = 0.5f + 0.5f * std::sin(platform::time_seconds() * 6.0f);
            r.set_layer(render::Renderer::Layer::Additive);
            r.draw_circle(c, 12.0f + p * 4.0f,
                          with_alpha(COLOR_CORE_EDGE, u8(80 + 60 * p)), 16);
            r.set_layer(render::Renderer::Layer::Alpha);
            r.draw_circle(c, 8.0f, COLOR_CORE_INNER, 14);
            char sb[8]; std::snprintf(sb, sizeof(sb), "%d", character_.stat_points);
            const f32 sw = r.text_width(sb, 1.8f);
            r.draw_text({c.x - sw * 0.5f, c.y - 6.0f}, sb, 1.8f, COLOR_VOID);
        }
        if (character_.skill_points > 0) {
            const Vec2 c {bar_x + bar_w + 130.0f, bar_y + bar_h * 0.5f};
            const f32 p = 0.5f + 0.5f * std::sin(platform::time_seconds() * 5.0f + 1.0f);
            r.set_layer(render::Renderer::Layer::Additive);
            r.draw_circle(c, 12.0f + p * 4.0f,
                          with_alpha(COLOR_LEAD_EDGE, u8(80 + 60 * p)), 16);
            r.set_layer(render::Renderer::Layer::Alpha);
            r.draw_circle(c, 8.0f, COLOR_LEAD_CORE, 14);
            char sb[8]; std::snprintf(sb, sizeof(sb), "%d", character_.skill_points);
            const f32 sw = r.text_width(sb, 1.8f);
            r.draw_text({c.x - sw * 0.5f, c.y - 6.0f}, sb, 1.8f, COLOR_VOID);
        }
    }

    // --- Score bar with shimmer + numeric readout ---
    {
        constexpr f32 BAR_W = 400.0f;
        constexpr f32 BAR_H = 16.0f;
        const f32 BAR_X = VIRTUAL_W - BAR_W - 40.0f;
        const f32 BAR_Y = 52.0f;
        const f32 SCORE_MAX = f32(ROOM_COUNT + 1) * 40000.0f;

        r.draw_text({BAR_X, BAR_Y - 24.0f}, "SCORE", 2.0f,
                    with_alpha(COLOR_SCORE_BAR, 220));

        char sc_v[24];
        std::snprintf(sc_v, sizeof(sc_v), "%d", int(score_));
        const f32 svw = r.text_width(sc_v, 2.4f);
        r.draw_text({BAR_X + BAR_W - svw, BAR_Y - 26.0f}, sc_v, 2.4f,
                    COLOR_SCORE_BAR);

        r.draw_quad({BAR_X, BAR_Y}, {BAR_X + BAR_W, BAR_Y},
                    {BAR_X + BAR_W, BAR_Y + BAR_H}, {BAR_X, BAR_Y + BAR_H},
                    with_alpha(COLOR_SCORE_BAR, 30));

        const f32 fill_w = BAR_W * clamp(f32(score_) / SCORE_MAX, 0.0f, 1.0f);
        if (fill_w > 0.5f) {
            r.draw_quad({BAR_X, BAR_Y}, {BAR_X + fill_w, BAR_Y},
                        {BAR_X + fill_w, BAR_Y + BAR_H}, {BAR_X, BAR_Y + BAR_H},
                        COLOR_SCORE_BAR);
            const f32 sweep = std::fmod(f32(platform::time_seconds()) * 220.0f,
                                        BAR_W + 80.0f) - 40.0f;
            if (sweep > 0.0f && sweep < fill_w + 40.0f) {
                r.set_layer(render::Renderer::Layer::Additive);
                r.draw_quad({BAR_X + sweep - 30.0f, BAR_Y},
                            {BAR_X + sweep + 8.0f, BAR_Y},
                            {BAR_X + sweep + 8.0f, BAR_Y + BAR_H},
                            {BAR_X + sweep - 30.0f, BAR_Y + BAR_H},
                            with_alpha(COLOR_SCORE_TICK, 80));
                r.set_layer(render::Renderer::Layer::Alpha);
            }
        }

        for (int i = 1; i <= 3; ++i) {
            const f32 tx = BAR_X + BAR_W * f32(i) * 0.25f;
            r.draw_line({tx, BAR_Y - 3}, {tx, BAR_Y + BAR_H + 3}, 1.5f,
                        with_alpha(COLOR_SCORE_TICK, 140));
        }
    }

    // --- Room progress: hexagons ---
    {
        constexpr f32 CX = VIRTUAL_W * 0.5f;
        constexpr f32 CY = 50.0f;
        const int DOTS = ROOM_COUNT + 1;
        const f32 SPACING = (DOTS <= 6) ? 30.0f : 22.0f;
        const f32 start_x = CX - f32(DOTS - 1) * SPACING * 0.5f;

        for (int i = 0; i < DOTS; ++i) {
            const Vec2 c {start_x + f32(i) * SPACING, CY};
            const bool is_boss = (i == DOTS - 1);
            bool completed = false, current = false;
            if (is_boss) {
                completed = warden_defeated_;
                current   = warden_active_ && warden_.alive;
            } else {
                completed = (rooms_cleared_ & (1u << i)) != 0;
                current   = (i == current_room_);
            }
            const Color dc = is_boss ? COLOR_ENEMY_EDGE : COLOR_CORE_EDGE;

            render::icon_hex(r, c, 12.0f, 0, with_alpha(dc, 120));

            if (completed && !current) {
                render::icon_hex(r, c, 8.0f, with_alpha(dc, 200), 0);
            } else if (current) {
                const f32 p = 0.5f + 0.5f * std::sin(platform::time_seconds() * 4.0f);
                render::icon_hex(r, c, 8.0f, dc, 0);
                r.set_layer(render::Renderer::Layer::Additive);
                render::icon_hex(r, c, 14.0f + p * 3.0f, 0,
                                 with_alpha(dc, u8(80 + 80 * p)));
                r.set_layer(render::Renderer::Layer::Alpha);
            }
        }
    }
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

    if (room_clear_pulse_ >= 0.0f) {
        const f32 t = clamp(room_clear_pulse_ / 2.0f, 0.0f, 1.0f);
        const f32 radius = 30.0f + t * 700.0f;
        const u8  a = u8((1.0f - t) * 200.0f);
        r.draw_ring(drone_.position, radius - 4.0f, radius + 4.0f,
                    with_alpha(COLOR_CORE_INNER, a), 40);
    }

    if (levelup_banner_timer_ > 0.0f) {
        const f32 elapsed = 2.0f - levelup_banner_timer_;
        levelup_fx::draw_banner(r, levelup_banner_level_, elapsed, 2.0f);
    }
}

} // namespace inc::game