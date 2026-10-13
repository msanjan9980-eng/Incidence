#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace inc {

// ---------------------------------------------------------------------------
// Scalar aliases. We use f32 everywhere in simulation. Determinism requires
// one float type; mixing double into the sim would break per-platform replay
// across compilers.
// ---------------------------------------------------------------------------
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;

inline constexpr f32 PI       = 3.14159265358979323846f;
inline constexpr f32 TAU      = PI * 2.0f;
inline constexpr f32 HALF_PI  = PI * 0.5f;
inline constexpr f32 EPS      = 1.0e-6f;
inline constexpr f32 EPS_SQ   = EPS * EPS;

// The game runs at a fixed 120 Hz simulation step. Every timestep-dependent
// constant in the codebase is expressed per-second and multiplied by the
// fixed dt at the call site, never the render dt.
inline constexpr f32 FIXED_DT     = 1.0f / 120.0f;
inline constexpr f32 FIXED_DT_INV = 120.0f;

// Virtual resolution. All gameplay math is in virtual pixels; the renderer
// letterboxes this into whatever the real window size is.
inline constexpr f32 VIRTUAL_W = 1920.0f;
inline constexpr f32 VIRTUAL_H = 1080.0f;

// ---------------------------------------------------------------------------
// Scalar utilities
// ---------------------------------------------------------------------------
[[nodiscard]] constexpr f32 deg_to_rad(f32 d) noexcept { return d * (PI / 180.0f); }
[[nodiscard]] constexpr f32 rad_to_deg(f32 r) noexcept { return r * (180.0f / PI); }

[[nodiscard]] constexpr f32 clamp(f32 v, f32 lo, f32 hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}

[[nodiscard]] constexpr f32 lerp(f32 a, f32 b, f32 t) noexcept {
    return a + (b - a) * t;
}

[[nodiscard]] constexpr f32 sign(f32 v) noexcept {
    return v < 0.0f ? -1.0f : 1.0f;
}

// Wrap an angle into (-pi, pi]. Used constantly for shield-arc containment.
[[nodiscard]] inline f32 wrap_pi(f32 a) noexcept {
    a = std::fmod(a + PI, TAU);
    if (a < 0.0f) a += TAU;
    return a - PI;
}

// Wrap an angle into [0, tau).
[[nodiscard]] inline f32 wrap_tau(f32 a) noexcept {
    a = std::fmod(a, TAU);
    if (a < 0.0f) a += TAU;
    return a;
}

// Snap to nearest multiple of `step`. Used for gamepad shield aiming.
[[nodiscard]] inline f32 snap(f32 v, f32 step) noexcept {
    return std::round(v / step) * step;
}

} // namespace inc