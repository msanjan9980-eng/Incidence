#pragma once

#include "core/vec2.hpp"
#include "core/color.hpp"

namespace inc::render { class Renderer; }

namespace inc::game {

struct Drone;

// ---------------------------------------------------------------------------
// Turret — the first enemy. A stationary fragment of THEOREM that fires slow
// straight projectiles at the player. Bullets that bounce off the player's
// shield and then hit the turret kill it. Any other bullet just passes
// through or expires.
//
// The turret is deliberately dumb: it fires at the player's position at the
// moment of firing, not at a predicted position. This is the "Lancer"
// archetype from the design doc. Later enemies will lead their shots.
// ---------------------------------------------------------------------------
struct Turret {
    Vec2 pos;
    f32  radius        = 25.0f;
    int  hp            = 3;
    f32  fire_timer    = 0.0f;
    f32  fire_interval = 2.0f;    // seconds between shots
    f32  bullet_speed  = 350.0f;  // virtual px / sec
    bool alive         = true;

    // Called every simulation tick. Returns true if the turret wants to fire
    // this tick — in which case the World will spawn a projectile aimed at
    // `target_pos`.
    bool step(f32 dt, Vec2 target_pos);

    void draw(render::Renderer& r) const;
};

} // namespace inc::game