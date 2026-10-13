#pragma once

#include "core/math.hpp"
#include "core/vec2.hpp"
#include "core/color.hpp"

#include "game/drone.hpp"
#include "game/turret.hpp"
#include "game/projectile.hpp"
#include "game/room.hpp"
#include "game/warden.hpp"
#include "game/screen.hpp"

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
    void step(f32 dt, const platform::InputState& input);
    void draw(render::Renderer& r) const;

    [[nodiscard]] int  player_hp()  const noexcept { return player_hp_; }
    [[nodiscard]] int  kills()      const noexcept { return kills_; }
    [[nodiscard]] i32  score()      const noexcept { return score_; }
    [[nodiscard]] int  room_index() const noexcept { return current_room_; }
    [[nodiscard]] bool warden_active() const noexcept { return warden_active_; }
    [[nodiscard]] bool player_dead() const noexcept { return player_dead_; }
    [[nodiscard]] bool warden_defeated() const noexcept { return warden_defeated_; }

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

    // ---- Content -------------------------------------------------------
    Drone      drone_;
    Turret     turrets_[8];
    Projectile projectiles_[256];
    HitFlash   flashes_[32];
    Warden     warden_;

    Wall       walls_[32];
    int        wall_count_     = 0;
    int        turret_count_   = 0;
    int        current_room_   = 0;
    bool       warden_active_  = false;

    // ---- Player state --------------------------------------------------
    int  player_hp_ = 3;
    int  kills_     = 0;
    i32  score_     = 0;

    bool player_dead_       = false;
    bool warden_defeated_   = false;

    f32  room_clear_timer_    = -1.0f;
    f32  warden_death_timer_  = -1.0f;
    f32  death_screen_timer_  = 0.0f;
    f32  victory_screen_timer_= 0.0f;
    f32  last_incidence_deg_  = 0.0f;
};

} // namespace inc::game