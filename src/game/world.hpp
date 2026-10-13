#pragma once

#include "core/math.hpp"
#include "core/vec2.hpp"
#include "core/color.hpp"

#include "game/drone.hpp"
#include "game/turret.hpp"
#include "game/projectile.hpp"
#include "game/room.hpp"

namespace inc::render { class Renderer; }
namespace inc::platform { struct InputState; }

namespace inc::game {

struct HitFlash {
    Vec2 pos;
    f32  age    = 0.0f;
    bool alive  = false;
    static constexpr f32 LIFETIME = 0.35f;
};

// ---------------------------------------------------------------------------
// World — the entire game state.
//
// Owns the drone, the current room's walls and turrets, the projectile pool,
// and the hit-flash pool. Handles:
//   - stepping every entity,
//   - reflection + collision (walls, shield, core, turrets),
//   - room loading and progression,
//   - the HUD.
// ---------------------------------------------------------------------------
class World {
public:
    World();

    // Restart the entire run from room 0.
    void reset();

    void step(f32 dt, const platform::InputState& input);
    void draw(render::Renderer& r) const;

    [[nodiscard]] int  player_hp()  const noexcept { return player_hp_; }
    [[nodiscard]] int  kills()      const noexcept { return kills_; }
    [[nodiscard]] i32  score()      const noexcept { return score_; }
    [[nodiscard]] int  room_index() const noexcept { return current_room_; }
    [[nodiscard]] f32  last_incidence_deg() const noexcept { return last_incidence_deg_; }

private:
    void load_room(int index);
    void spawn_turret(Vec2 pos);
    void spawn_bullet(Vec2 pos, Vec2 dir, f32 speed);
    void spawn_flash(Vec2 pos);

    bool resolve_collisions(Projectile& p);
    bool collide_with_walls(Projectile& p);

    // ---- Content --------------------------------------------------------
    Drone      drone_;
    Turret     turrets_[8];
    Projectile projectiles_[256];
    HitFlash   flashes_[32];

    Wall       walls_[32];
    int        wall_count_     = 0;
    int        turret_count_   = 0;
    int        current_room_   = 0;

    // ---- Player state ---------------------------------------------------
    int  player_hp_ = 3;
    int  kills_     = 0;
    i32  score_     = 0;

    // ---- Room progression -----------------------------------------------
    f32  room_clear_timer_ = -1.0f;   // -1 = not counting

    f32  last_incidence_deg_ = 0.0f;
};

} // namespace inc::game