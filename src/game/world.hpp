#pragma once

#include "core/math.hpp"
#include "core/vec2.hpp"
#include "core/color.hpp"

#include "game/drone.hpp"
#include "game/turret.hpp"
#include "game/projectile.hpp"

namespace inc::render { class Renderer; }
namespace inc::platform { struct InputState; }

namespace inc::game {

// ---------------------------------------------------------------------------
// HitFlash — a short-lived visual marker spawned at the point where a
// projectile reflected off the shield. Expands and fades over its lifetime.
// ---------------------------------------------------------------------------
struct HitFlash {
    Vec2 pos;
    f32  age    = 0.0f;
    bool alive  = false;
    static constexpr f32 LIFETIME = 0.35f;
};

// ---------------------------------------------------------------------------
// World — everything in the current room.
// ---------------------------------------------------------------------------
class World {
public:
    World();

    void reset();
    void step(f32 dt, const platform::InputState& input);
    void draw(render::Renderer& r) const;

    [[nodiscard]] int  player_hp()  const noexcept { return player_hp_; }
    [[nodiscard]] int  kills()      const noexcept { return kills_; }
    [[nodiscard]] i32  score()      const noexcept { return score_; }
    [[nodiscard]] f32  last_incidence_deg() const noexcept { return last_incidence_deg_; }

private:
    void spawn_turret(Vec2 pos);
    void spawn_bullet(Vec2 pos, Vec2 dir, f32 speed);
    void spawn_flash(Vec2 pos);

    bool resolve_collisions(Projectile& p);

    // ---- Content --------------------------------------------------------
    Drone      drone_;
    Turret     turrets_[8];
    Projectile projectiles_[256];
    HitFlash   flashes_[32];
    int        turret_count_ = 0;

    // ---- Player state ---------------------------------------------------
    int  player_hp_ = 3;
    int  kills_     = 0;
    i32  score_     = 0;

    f32  last_incidence_deg_ = 0.0f;
};

} // namespace inc::game