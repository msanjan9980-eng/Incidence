#pragma once

#include "core/math.hpp"

namespace inc {

using Color = u32;

[[nodiscard]] constexpr Color rgba(u8 r, u8 g, u8 b, u8 a = 255) noexcept {
    return (u32(r) << 24) | (u32(g) << 16) | (u32(b) << 8) | u32(a);
}

[[nodiscard]] constexpr u8 color_r(Color c) noexcept { return u8((c >> 24) & 0xffu); }
[[nodiscard]] constexpr u8 color_g(Color c) noexcept { return u8((c >> 16) & 0xffu); }
[[nodiscard]] constexpr u8 color_b(Color c) noexcept { return u8((c >>  8) & 0xffu); }
[[nodiscard]] constexpr u8 color_a(Color c) noexcept { return u8( c        & 0xffu); }

[[nodiscard]] constexpr Color with_alpha(Color c, u8 a) noexcept {
    return (c & 0xFFFFFF00u) | u32(a);
}

[[nodiscard]] constexpr Color mix(Color a, Color b, f32 t) noexcept {
    const f32 ia = 1.0f - t;
    const u8 r = u8(f32(color_r(a)) * ia + f32(color_r(b)) * t);
    const u8 g = u8(f32(color_g(a)) * ia + f32(color_g(b)) * t);
    const u8 bl= u8(f32(color_b(a)) * ia + f32(color_b(b)) * t);
    const u8 al= u8(f32(color_a(a)) * ia + f32(color_a(b)) * t);
    return rgba(r, g, bl, al);
}

// World
constexpr Color COLOR_VOID          = rgba(  5,  14,  18);
constexpr Color COLOR_WALL_FILL     = rgba( 15,  40,  55);
constexpr Color COLOR_WALL_EDGE     = rgba( 60, 160, 200);

// Drone
constexpr Color COLOR_CORE_GLOW     = rgba( 60, 180, 220,  40);
constexpr Color COLOR_CORE_FILL     = rgba( 25,  40,  50);
constexpr Color COLOR_CORE_EDGE     = rgba(120, 235, 255);
constexpr Color COLOR_CORE_INNER    = rgba(180, 245, 255);
constexpr Color COLOR_FACET_FILL    = rgba( 40, 130, 170, 160);
constexpr Color COLOR_FACET_EDGE    = rgba(140, 240, 255);
constexpr Color COLOR_FACET_NORMAL  = rgba( 90, 220, 255, 200);
constexpr Color COLOR_ENGINE_GLOW   = rgba( 90, 220, 255, 120);

// Enemy — Lancer (red)
constexpr Color COLOR_ENEMY_FILL    = rgba(180,  50,  60);
constexpr Color COLOR_ENEMY_EDGE    = rgba(255, 130, 130);
constexpr Color COLOR_ENEMY_CORE    = rgba(255, 200,  80);

// Enemy — Banker (amber)
constexpr Color COLOR_BANKER_FILL   = rgba(180, 100,  30);
constexpr Color COLOR_BANKER_EDGE   = rgba(255, 190, 100);
constexpr Color COLOR_BANKER_CORE   = rgba(255, 240, 180);

// Enemy — Lead (magenta)
constexpr Color COLOR_LEAD_FILL     = rgba(140,  50, 160);
constexpr Color COLOR_LEAD_EDGE     = rgba(230, 130, 255);
constexpr Color COLOR_LEAD_CORE     = rgba(255, 220, 255);

// Projectiles — color is chosen based on source enemy type
constexpr Color COLOR_BULLET_CORE   = rgba(255, 230, 180);
constexpr Color COLOR_BULLET_GLOW   = rgba(255, 120,  60, 140);

// Trajectory ghost + scoring
constexpr Color COLOR_GHOST_DOT     = rgba(255, 220, 140, 100);
constexpr Color COLOR_GHOST_DOT_HOT = rgba(255, 250, 220, 200);
constexpr Color COLOR_HIT_FLASH     = rgba(220, 250, 255);
constexpr Color COLOR_SCORE_BAR     = rgba( 90, 220, 255);
constexpr Color COLOR_SCORE_TICK    = rgba(180, 240, 255, 120);

} // namespace inc