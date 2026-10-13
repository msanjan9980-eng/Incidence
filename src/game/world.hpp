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
// World — everything in the current room.
//
// Owns the drone, up to 8 turrets, and a fixed pool of 256 projectiles.
// Handles:
//   - stepping the simulation (movement, firing, integration)
//   - collision + reflection (bullet vs shield, bullet vs core, bullet vs turret)
//   - death / respawn
//   - drawing every entity to the renderer
//
// The World is the only place in the codebase that calls `reflect(v, n)`
// for gameplay purposes. That single line is the game.
// ---------------------------------------------------------------------------
class World {
public:
    World();

    // Start / restart the current room.
    void reset();

    // One fixed simulation tick.
    void step(f32 dt, const platform::InputState& input);

    // Draw everything to the renderer.
    void draw(render::Renderer& r) const;

    // ---- Introspection (for HUD and diagnostics) -------------------------
    [[nodiscard]] int  player_hp()   const noexcept { return player_hp_; }
    [[nodiscard]] int  kills()       const noexcept { return kills_; }
    [[nodiscard]] f32  last_incidence_deg() const noexcept { return last_incidence_deg_; }

private:
    // ---- Spawn helpers ---------------------------------------------------
    void spawn_turret(Vec2 pos);
    void spawn_bullet(Vec2 pos, Vec2 dir, f32 speed);

    // ---- Collision helpers ----------------------------------------------
    // Returns true if the projectile was consumed this tick (hit core,
    // hit turret, or expired). Wall and shield bounces are handled inline.
    bool resolve_collisions(Projectile& p);

    // ---- Room content ---------------------------------------------------
    Drone    drone_;
    Turret   turrets_[8];
    Projectile projectiles_[256];

    int  turret_count_ = 0;

    // ---- Player state ---------------------------------------------------
    int  player_hp_    = 3;
    int  kills_        = 0;

    // ---- Diagnostics ----------------------------------------------------
    f32  last_incidence_deg_ = 0.0f;
};

} // namespace inc::game