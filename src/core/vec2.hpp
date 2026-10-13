#pragma once

#include "core/math.hpp"

namespace inc {

// ---------------------------------------------------------------------------
// Vec2 — the atom of INCIDENCE.
//
// Everything the player does is a change to one of these. The shield's
// normal is a Vec2. Every projectile's velocity is a Vec2. The reflection
// law is a single line (see `reflect` below). Keep this file lean; anything
// that isn't used by the sim belongs in a higher layer.
// ---------------------------------------------------------------------------
struct Vec2 {
    f32 x = 0.0f;
    f32 y = 0.0f;

    constexpr Vec2() noexcept = default;
    constexpr Vec2(f32 x_, f32 y_) noexcept : x(x_), y(y_) {}

    // --- arithmetic ---------------------------------------------------------
    [[nodiscard]] constexpr Vec2 operator+(Vec2 o) const noexcept { return {x + o.x, y + o.y}; }
    [[nodiscard]] constexpr Vec2 operator-(Vec2 o) const noexcept { return {x - o.x, y - o.y}; }
    [[nodiscard]] constexpr Vec2 operator-()      const noexcept { return {-x, -y}; }
    [[nodiscard]] constexpr Vec2 operator*(f32 s) const noexcept { return {x * s, y * s}; }
    [[nodiscard]] constexpr Vec2 operator/(f32 s) const noexcept { return {x / s, y / s}; }

    constexpr Vec2& operator+=(Vec2 o) noexcept { x += o.x; y += o.y; return *this; }
    constexpr Vec2& operator-=(Vec2 o) noexcept { x -= o.x; y -= o.y; return *this; }
    constexpr Vec2& operator*=(f32 s)  noexcept { x *= s; y *= s; return *this; }
    constexpr Vec2& operator/=(f32 s)  noexcept { x /= s; y /= s; return *this; }

    // --- comparison ---------------------------------------------------------
    [[nodiscard]] constexpr bool operator==(Vec2 o) const noexcept {
        return x == o.x && y == o.y;
    }
    [[nodiscard]] constexpr bool operator!=(Vec2 o) const noexcept {
        return !(*this == o);
    }

    // --- products -----------------------------------------------------------
    [[nodiscard]] constexpr f32 dot(Vec2 o) const noexcept { return x * o.x + y * o.y; }

    // 2D cross product = scalar (z-component of the 3D cross).
    // Used for ray/segment intersection and for signed angle tests.
    [[nodiscard]] constexpr f32 cross(Vec2 o) const noexcept { return x * o.y - y * o.x; }

    // --- length -------------------------------------------------------------
    [[nodiscard]] inline f32 length_sq() const noexcept { return x * x + y * y; }
    [[nodiscard]] inline f32 length()    const noexcept { return std::sqrt(length_sq()); }

    [[nodiscard]] inline Vec2 normalized() const noexcept {
        const f32 l = length();
        if (l < EPS) return {0.0f, 0.0f};
        return {x / l, y / l};
    }

    // --- geometry helpers ---------------------------------------------------
    // Perpendicular (90 deg CCW). Used to build facet basis vectors.
    [[nodiscard]] constexpr Vec2 perp() const noexcept { return {-y, x}; }

    // Rotate by angle a (CCW). This is the 2x2 rotation matrix applied inline
    // rather than via Mat3 — cheaper and clearer at every call site.
    [[nodiscard]] inline Vec2 rotated(f32 a) const noexcept {
        const f32 c = std::cos(a);
        const f32 s = std::sin(a);
        return {x * c - y * s, x * s + y * c};
    }

    // Heading angle in (-pi, pi]. Used for shield aim and enemy facing.
    [[nodiscard]] inline f32 angle() const noexcept { return std::atan2(y, x); }

    // Signed angle of *this* relative to another vector, in (-pi, pi].
    [[nodiscard]] inline f32 signed_angle_to(Vec2 o) const noexcept {
        return wrap_pi(o.angle() - angle());
    }

    // --- static constructors ------------------------------------------------
    [[nodiscard]] static inline Vec2 from_angle(f32 a) noexcept {
        return {std::cos(a), std::sin(a)};
    }

    [[nodiscard]] static inline Vec2 from_angle(f32 a, f32 r) noexcept {
        return {std::cos(a) * r, std::sin(a) * r};
    }

    [[nodiscard]] static constexpr Vec2 zero()   noexcept { return {0.0f, 0.0f}; }
    [[nodiscard]] static constexpr Vec2 unit_x() noexcept { return {1.0f, 0.0f}; }
    [[nodiscard]] static constexpr Vec2 unit_y() noexcept { return {0.0f, 1.0f}; }
};

// --- free functions --------------------------------------------------------
[[nodiscard]] constexpr Vec2 operator*(f32 s, Vec2 v) noexcept { return v * s; }

[[nodiscard]] inline f32 distance(Vec2 a, Vec2 b) noexcept {
    return (b - a).length();
}

[[nodiscard]] inline f32 distance_sq(Vec2 a, Vec2 b) noexcept {
    return (b - a).length_sq();
}

[[nodiscard]] inline Vec2 lerp(Vec2 a, Vec2 b, f32 t) noexcept {
    return {lerp(a.x, b.x, t), lerp(a.y, b.y, t)};
}

// ---------------------------------------------------------------------------
// reflect — the single most important function in the game.
//
//   v' = v - 2 (v . n) n
//
// `n` must be a unit normal pointing AWAY from the reflecting surface, i.e.
// on the same side as the incoming projectile. `v` must be moving toward the
// surface (v . n < 0) for a physical reflection; callers must check this.
//
// `restitution` in [0,1] scales the reflected component. 1.0 = perfect mirror
// (the default for the shield). Lower values model damping facets.
//
// We deliberately do NOT normalize v inside this function. The caller owns
// the invariant that n is unit-length; enforcing it here would silently mask
// bugs where a facet was built wrong.
// ---------------------------------------------------------------------------
[[nodiscard]] inline Vec2 reflect(Vec2 v, Vec2 n, f32 restitution = 1.0f) noexcept {
    const f32 vn = v.dot(n);
    return v - n * ((1.0f + restitution) * vn);
}

} // namespace inc