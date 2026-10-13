#pragma once

#include "core/vec2.hpp"
#include "core/color.hpp"

namespace inc::render { class Renderer; }

namespace inc::game {

struct Drone;

// ---------------------------------------------------------------------------
// EnemyType — the archetype of a stationary enemy.
//
//   Lancer  : fires straight at the player's CURRENT position. Baseline.
//   Banker  : same aim, but bullets survive many more wall bounces.
//   Lead    : fires at the player's PREDICTED position, using drone.velocity.
// ---------------------------------------------------------------------------
enum class EnemyType : u8 {
    Lancer = 0,
    Banker = 1,
    Lead   = 2,
};

// ---------------------------------------------------------------------------
// Turret — a stationary enemy. Despite the name, this struct now handles all
// three archetypes; the firing and drawing logic branches on `type`.
// ---------------------------------------------------------------------------
struct Turret {
    EnemyType type        = EnemyType::Lancer;
    Vec2      pos;
    f32       radius        = 25.0f;
    int       hp            = 3;
    f32       fire_timer    = 0.0f;
    f32       fire_interval = 2.0f;
    f32       bullet_speed  = 350.0f;
    bool      alive         = true;

    // Number of wall bounces a fired bullet may survive. Lancer / Lead use
    // 4; Banker uses 8. Set at spawn time in World::spawn_turret.
    int       wall_bounces_per_bullet = 4;

    // Bullet colors, chosen from the enemy palette.
    Color     bullet_core = COLOR_BULLET_CORE;
    Color     bullet_glow = COLOR_BULLET_GLOW;

    // The telegraph line becomes visible this many seconds before firing.
    static constexpr f32 TELEGRAPH_S = 0.35f;

    // True when we're currently inside the telegraph window.
    [[nodiscard]] bool telegraphing() const noexcept {
        return fire_timer >= (fire_interval - TELEGRAPH_S);
    }

    // Advance the firing clock. On fire, returns true and writes the
    // world-space aim target into `out_target`. That target may be the
    // drone's current position (Lancer / Banker) or a predicted one (Lead).
    bool step(f32 dt, const Drone& drone, Vec2& out_target);

    // Draw the turret body, plus an aim line if `show_telegraph`.
    void draw(render::Renderer& r, bool show_telegraph, Vec2 telegraph_target) const;
};

} // namespace inc::game