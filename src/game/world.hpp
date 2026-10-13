#pragma once

#include "core/math.hpp"
#include "core/vec2.hpp"
#include "core/color.hpp"

#include "game/drone.hpp"
#include "game/turret.hpp"
#include "game/projectile.hpp"
#include "game/room.hpp"
#include "game/warden.hpp"
#include "game/character.hpp"
#include "game/screen.hpp"
#include "render/effects.hpp"
#include "render/particles.hpp"

namespace inc::render { class Renderer; }
namespace inc::platform { struct InputState; }

namespace inc::game {

struct HitFlash {
    Vec2 pos;
    f32  age    = 0.0f;
    bool alive  = false;
    static constexpr f32 LIFETIME = 0.35f;
};

class World {
public:
    World();

    void reset();
    void jump_to_room(int index);
    void travel_to_room(int index);
    void step(f32 dt, const platform::InputState& input);
    void draw(render::Renderer& r) const;

    [[nodiscard]] int  player_hp()  const noexcept { return player_hp_; }
    [[nodiscard]] int  kills()      const noexcept { return kills_; }
    [[nodiscard]] i32  score()      const noexcept { return score_; }
    [[nodiscard]] int  room_index() const noexcept { return current_room_; }
    [[nodiscard]] bool warden_active() const noexcept { return warden_active_; }
    [[nodiscard]] bool player_dead() const noexcept { return player_dead_; }
    [[nodiscard]] bool warden_defeated() const noexcept { return warden_defeated_; }
    [[nodiscard]] bool is_room_cleared(int index) const noexcept {
        return index >= 0 && index < 16
            && (rooms_cleared_ & (1u << index)) != 0;
    }

    [[nodiscard]] bool is_room_visited(int index) const noexcept {
        return index >= 0 && index < 16
            && (rooms_visited_ & (1u << index)) != 0;
    }

    [[nodiscard]] u16 rooms_visited() const noexcept { return rooms_visited_; }
    void set_rooms_visited(u16 v) noexcept { rooms_visited_ = v; }

    [[nodiscard]] Character&       character()       noexcept { return character_; }
    [[nodiscard]] const Character& character() const noexcept { return character_; }

    [[nodiscard]] int  pending_log_trigger() const noexcept { return pending_log_trigger_; }

    [[nodiscard]] render::Effects&       effects()       noexcept { return effects_; }
    [[nodiscard]] const render::Effects& effects() const noexcept { return effects_; }
    void clear_pending_log_trigger() noexcept { pending_log_trigger_ = -1; }

private:
    void load_room(int index);
    void spawn_turret(const EnemySpawn& spawn);
    void spawn_bullet(Vec2 pos, Vec2 dir, f32 speed,
                      int wall_bounces,
                      Color core_c, Color glow_c);
    void spawn_flash(Vec2 pos);

    void spawn_warden();
    void step_warden(f32 dt);
    bool resolve_warden_collisions(Projectile& p);

    bool resolve_collisions(Projectile& p);
    bool collide_with_walls(Projectile& p);

    void award_kill_xp(EnemyType type);
    void attempt_door_transition();

    Drone      drone_;
    Turret     turrets_[8];
    Projectile projectiles_[256];
    HitFlash   flashes_[32];
    Warden     warden_;
    Character  character_;

    Wall       walls_[32];
    int        wall_count_     = 0;
    int        turret_count_   = 0;
    int        current_room_   = 0;
    bool       warden_active_  = false;

    int  player_hp_ = 3;
    int  kills_     = 0;
    i32  score_     = 0;

    bool player_dead_       = false;
    bool warden_defeated_   = false;

    u16  rooms_cleared_       = 0;
    u16  rooms_visited_       = 0;
    f32  door_cooldown_       = 0.0f;
    f32  room_clear_pulse_    = -1.0f;
    f32  warden_death_timer_  = -1.0f;
    f32  death_screen_timer_  = 0.0f;
    f32  victory_screen_timer_= 0.0f;
    f32  last_incidence_deg_  = 0.0f;

    f32  levelup_banner_timer_ = 0.0f;
    int  levelup_banner_level_ = 1;

    render::Effects effects_;
    fx::ParticleSystem game_particles_;
    int  pending_log_trigger_ = -1;
};

} // namespace inc::game