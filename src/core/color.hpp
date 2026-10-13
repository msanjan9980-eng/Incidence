#pragma once

#include "core/math.hpp"

namespace inc {

// ---------------------------------------------------------------------------
// Color — packed RGBA as a u32 (R in the high byte, A in the low byte).
//
// We pass colors around as u32 everywhere in the game; the renderer unpacks
// to float RGBA only at vertex submission. This is cheap to copy, cheap to
// compare, and reads cleanly in struct initializers.
// ---------------------------------------------------------------------------
using Color = u32;

[[nodiscard]] constexpr Color rgba(u8 r, u8 g, u8 b, u8 a = 255) noexcept {
    return (u32(r) << 24) | (u32(g) << 16) | (u32(b) << 8) | u32(a);
}

[[nodiscard]] constexpr u8 color_r(Color c) noexcept { return u8((c >> 24) & 0xffu); }
[[nodiscard]] constexpr u8 color_g(Color c) noexcept { return u8((c >> 16) & 0xffu); }
[[nodiscard]] constexpr u8 color_b(Color c) noexcept { return u8((c >>  8) & 0xffu); }
[[nodiscard]] constexpr u8 color_a(Color c) noexcept { return u8( c        & 0xffu); }

// ---------------------------------------------------------------------------
// The INCIDENCE palette.
//
// Background: deep teal, the "void" of a dead research station.
// Player: cool cyans. Everything about KITE-9 is engineered, clinical, cold.
// Enemies: warm reds and ambers. THEOREM's broken fragments glow angrily.
// ---------------------------------------------------------------------------

// World
constexpr Color COLOR_VOID          = rgba(  5,  14,  18);          // background
constexpr Color COLOR_WALL_FILL     = rgba( 15,  40,  55);
constexpr Color COLOR_WALL_EDGE     = rgba( 60, 160, 200);

// Drone core
constexpr Color COLOR_CORE_GLOW     = rgba( 60, 180, 220,  40);
constexpr Color COLOR_CORE_FILL     = rgba( 25,  40,  50);
constexpr Color COLOR_CORE_EDGE     = rgba(120, 235, 255);
constexpr Color COLOR_CORE_INNER    = rgba(180, 245, 255);

// Drone shield — the mirror facets
constexpr Color COLOR_FACET_FILL    = rgba( 40, 130, 170, 160);
constexpr Color COLOR_FACET_EDGE    = rgba(140, 240, 255);
constexpr Color COLOR_FACET_NORMAL  = rgba( 90, 220, 255, 200);

// Engine trail
constexpr Color COLOR_ENGINE_GLOW   = rgba( 90, 220, 255, 120);

// Enemies (used in M4)
constexpr Color COLOR_ENEMY_FILL    = rgba(180,  50,  60);
constexpr Color COLOR_ENEMY_EDGE    = rgba(255, 130, 130);
constexpr Color COLOR_ENEMY_CORE    = rgba(255, 200,  80);

// Projectiles (used in M4)
constexpr Color COLOR_BULLET_CORE   = rgba(255, 230, 180);
constexpr Color COLOR_BULLET_GLOW   = rgba(255, 120,  60, 140);

} // namespace inc