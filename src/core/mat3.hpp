#pragma once

#include "core/vec2.hpp"

namespace inc {

// ---------------------------------------------------------------------------
// Mat3 — a 3x3 affine transform for 2D points.
//
//   | a  c  tx |   | x |
//   | b  d  ty | * | y |
//   | 0  0   1 |   | 1 |
//
// Column-major storage matches GLSL and sokol_gfx's expectation. Used for:
//   - Portal transforms (position + rotation + reflection)
//   - Rotating-arena change of basis
//   - Virtual-to-screen letterbox projection
//
// The determinant sign tells you whether the transform includes a reflection:
// positive = orientation-preserving, negative = the transform flips handedness
// (this is how we implement Möbius/Klein portals later).
// ---------------------------------------------------------------------------
struct Mat3 {
    // Column-major: m[0..2] = col 0, m[3..5] = col 1, m[6..8] = col 2.
    f32 m[9] = {
        1, 0, 0,   // col 0
        0, 1, 0,   // col 1
        0, 0, 1    // col 2 (translation)
    };

    // --- constructors -------------------------------------------------------
    [[nodiscard]] static constexpr Mat3 identity() noexcept { return Mat3{}; }

    [[nodiscard]] static inline Mat3 translation(Vec2 t) noexcept {
        Mat3 r{};
        r.m[6] = t.x;
        r.m[7] = t.y;
        return r;
    }

    [[nodiscard]] static inline Mat3 rotation(f32 a) noexcept {
        const f32 c = std::cos(a);
        const f32 s = std::sin(a);
        Mat3 r{};
        r.m[0] =  c; r.m[1] = s;
        r.m[3] = -s; r.m[4] = c;
        return r;
    }

    [[nodiscard]] static inline Mat3 scale(f32 sx, f32 sy) noexcept {
        Mat3 r{};
        r.m[0] = sx;
        r.m[4] = sy;
        return r;
    }

    // --- composition --------------------------------------------------------
    // `this * rhs` applies rhs first, then this. Standard math convention.
    [[nodiscard]] inline Mat3 operator*(const Mat3& rhs) const noexcept {
        Mat3 out{};
        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 3; ++r) {
                f32 sum = 0.0f;
                for (int k = 0; k < 3; ++k) {
                    sum += m[k * 3 + r] * rhs.m[c * 3 + k];
                }
                out.m[c * 3 + r] = sum;
            }
        }
        return out;
    }

    // --- apply to a point (includes translation) ---------------------------
    [[nodiscard]] inline Vec2 transform_point(Vec2 p) const noexcept {
        return {
            m[0] * p.x + m[3] * p.y + m[6],
            m[1] * p.x + m[4] * p.y + m[7]
        };
    }

    // --- apply to a direction (ignores translation) ------------------------
    // Use for velocities: a portal rotates your velocity but doesn't translate it.
    [[nodiscard]] inline Vec2 transform_direction(Vec2 v) const noexcept {
        return {
            m[0] * v.x + m[3] * v.y,
            m[1] * v.x + m[4] * v.y
        };
    }

    // --- determinant of the linear part ------------------------------------
    // Negative means the transform includes a reflection. Portal code checks
    // this to decide whether to flip the projectile's chirality.
    [[nodiscard]] inline f32 det() const noexcept {
        return m[0] * m[4] - m[1] * m[3];
    }
};

} // namespace inc