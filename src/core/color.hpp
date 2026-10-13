#pragma once

#include "core/math.hpp"

namespace inc {

// ---------------------------------------------------------------------------
// Color — packed RGBA as a u32 (R in the high byte, A in the low byte).
// ---------------------------------------------------------------------------
using Color = u32;

[[nodiscard]] constexpr Color rgba(u8 r, u8 g, u8 b, u8 a = 255) noexcept {
    return (u32(r) << 24) | (u32(g) << 16) | (u32(b) << 8) | u32(a);
}

[[nodiscard]] constexpr u8 color_r(Color c) noexcept { return u8((c >> 24) & 0xffu); }
[[nodiscard]] constexpr u8 color_g(Color c) noexcept { return u8((c >> 16) & 0xffu); }
[[nodiscard]] constexpr u8 color_b(Color c) noexcept { return u8((c >>  8) & 0xffu); }
[[nodiscard]] constexpr u8 color_a(Color c) noexcept { return u8( c        & 0xffu); }

// Replace the alpha byte of a color, keeping RGB.
[[nodiscard]] constexpr Color with_alpha(Color c, u8 a) noexcept {
    return (c & 0xFFFFFF00u) | u32(a);
}

// Linearly blend between two colors. t=0 -> a, t=1 -> b.
[[nodiscard]] constexpr Color mix(Color a, Color b, f32 t) noexcept {
    const f32 ia = 1.0f - t;
    const u8 r = u8(f32(color_r(a)) * ia + f32(color_r(b)) * t);
    const u8 g = u8(f32(color_g(a)) * ia + f32(color_g(b)) * t);
    const u8 bl= u8(f32(color_b(a)) * ia + f32(color_b(b)) * t);
    const u8 al= u8(f32(color_a(a)) * ia + f32(color_a(b)) * t);
    return rgba(r, g, bl, al);
}

// ---------------------------------------------------------------------------
// The INCIDENCE palette.
// ---------------------------------------------------------------------------

// World
constexpr Color COLOR_VOID          = rgba(  5,  14,  18);
constexpr Color COLOR_WALL_FILL     = rgba( 15,  40,  55);
constexpr Color COLOR_WALL_EDGE     = rgba( 60, 160, 200);

// Drone core
constexpr Color COLOR_CORE_GLOW     = rgba( 60, 180, 220,  40);
constexpr Color COLOR_CORE_FILL     = rgba( 25,  40,  50);
constexpr Color COLOR_CORE_EDGE     = rgba(120, 235, 255);
constexpr Color COLOR_CORE_INNER    = rgba(180, 245, 255);

// Drone shield
constexpr Color COLOR_FACET_FILL    = rgba( 40, 130, 170, 160);
constexpr Color COLOR_FACET_EDGE    = rgba(140, 240, 255);
constexpr Color COLOR_FACET_NORMAL  = rgba( 90, 220, 255, 200);

// Engine trail
constexpr Color COLOR_ENGINE_GLOW   = rgba( 90, 220, 255, 120);

// Enemies
constexpr Color COLOR_ENEMY_FILL    = rgba(180,  50,  60);
constexpr Color COLOR_ENEMY_EDGE    = rgba(255, 130, 130);
constexpr Color COLOR_ENEMY_CORE    = rgba(255, 200,  80);

// Projectiles
constexpr Color COLOR_BULLET_CORE   = rgba(255, 230, 180);
constexpr Color COLOR_BULLET_GLOW   = rgba(255, 120,  60, 140);

// M5 additions
constexpr Color COLOR_GHOST_DOT     = rgba(255, 220, 140, 100);  // trajectory preview
constexpr Color COLOR_GHOST_DOT_HOT = rgba(255, 250, 220, 200);  // preview near impact
constexpr Color COLOR_HIT_FLASH     = rgba(220, 250, 255);        // reflection impact
constexpr Color COLOR_SCORE_BAR     = rgba( 90, 220, 255);        // score indicator
constexpr Color COLOR_SCORE_TICK    = rgba(180, 240, 255, 120);

} // namespace inc